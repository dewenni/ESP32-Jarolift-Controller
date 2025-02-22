#include <JaroliftController.h>
#include <nvs.h>
#include <nvs_flash.h>

#define NVS_NAMESPACE "device_data"

#define Lowpulse 400 // Defines pulse-width in microseconds. Adapt for your use...
#define Highpulse 800

#define BITS_SIZE 8

static const char *TAG = "JARO-CTRL"; // LOG TAG

// RX variables and defines
#define debounce 200 // Ignoring short pulses in reception... no clue if required and if it makes sense ;)
#define pufsize 216  // Pulsepuffer

struct s_gpio {
  int gdo0; // TX
  int gdo2; // RX
  int sck;
  int mosi;
  int miso;
  int cs;
};

struct s_cfg {
  unsigned long masterMSB = 0;
  unsigned long masterLSB = 0;
  bool learn_mode = true;
  uint32_t serial = 0;
};

static s_gpio gpio;
static s_cfg config;
static bool initOK = false;

unsigned short devcnt = 0x0; // Initial 16Bit countervalue, stored in EEPROM and incremented once every time a command is send

static int device_key_msb = 0x0; // stores cryptkey MSB
static int device_key_lsb = 0x0; // stores cryptkey LSB
static uint64_t button = 0x0;    // 1000=0x8 up, 0100=0x4 stop, 0010=0x2 down, 0001=0x1 learning
static int disc = 0x0;
static uint32_t dec = 0;  // stores the 32Bit encrypted code
static uint64_t pack = 0; // Contains data to send.
static byte disc_low[16] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
static byte disc_high[16] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
static byte disc_l = 0;
static byte disc_h = 0;
uint64_t new_serial = 0;
static byte marcState;
byte syncWord = 199;

static uint32_t rx_serial = 0;
static char rx_disc_low[8] = {0};
static char rx_disc_high[8] = {0};
static uint32_t rx_hopcode = 0;
static uint16_t rx_disc_h = 0;
static byte rx_function = 0;
static int rx_device_key_msb = 0x0;     // stores cryptkey MSB
static int rx_device_key_lsb = 0x0;     // stores cryptkey LSB
static volatile uint32_t decoded = 0x0; // decoded hop code
static volatile byte pbwrite;
static volatile unsigned int lowbuf[pufsize]; // ring buffer storing LOW pulse lengths
static volatile unsigned int hibuf[pufsize];  // ring buffer storing HIGH pulse lengths
static long rx_time;
static int steadycnt = 0;
volatile bool iset = false;

static CC1101 cc1101; // The connection to the hardware chip CC1101 the RF Chip

// ####################################################################
//  set Device Counter
// ####################################################################
void JaroliftController::setGPIO(int8_t sck, int8_t miso, int8_t mosi, int8_t ss, int8_t gdo0, int8_t gdo2) {
  gpio.sck = sck;
  gpio.miso = miso;
  gpio.mosi = mosi;
  gpio.cs = ss;
  gpio.gdo0 = gdo0;
  gpio.gdo2 = gdo2;
}

// ####################################################################
//  increment and store devcnt, send devcnt as mqtt state topic
// ####################################################################
void JaroliftController::devcnt_handler(bool do_increment) {
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS");
    return;
  }

  // Wert aus NVS laden
  err = nvs_get_u16(nvs_handle, "devcnt", &devcnt);
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    devcnt = 0; // Falls Key noch nicht existiert, mit 0 starten
  }

  // Falls gewünscht, erhöhen und speichern
  if (do_increment) {
    devcnt++;
    nvs_set_u16(nvs_handle, "devcnt", devcnt);
    nvs_commit(nvs_handle);
  }

  ESP_LOGD(TAG, "Device Counter: %i", devcnt);

  nvs_close(nvs_handle);
}

// ####################################################################
//  set Device Counter
// ####################################################################
void JaroliftController::setDeviceCounter(uint16_t newDevCnt) {
  devcnt = newDevCnt;
  devcnt_handler(false);
  delay(100);
}

// ####################################################################
//  get Device Counter
// ####################################################################
uint16_t JaroliftController::getDeviceCounter() {
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS");
    return 0;
  }

  uint16_t devcnt = 0;
  err = nvs_get_u16(nvs_handle, "devcnt", &devcnt);

  if (err == ESP_ERR_NVS_NOT_FOUND) {
    // try to read from EEPROM if not found in NVS (migration)
    EEPROM.get(0, devcnt);

    // save to NVS for future use
    nvs_set_u16(nvs_handle, "devcnt", devcnt);
    nvs_commit(nvs_handle);
    ESP_LOGI(TAG, "Migrated devcnt=%d from EEPROM to NVS", devcnt);
  }

  nvs_close(nvs_handle);
  return devcnt;
}

// ####################################################################
//  set Base Serial
// ####################################################################
void JaroliftController::setBaseSerial(uint32_t serial) {
  config.serial = serial;
  ESP_LOGD(TAG, "set base serial: 0x%08lx", config.serial);
}

// ####################################################################
//  set Legacy Learn Mode
// ####################################################################
void JaroliftController::setLegacyLearnMode(bool LearnModeLegacy) { config.learn_mode = !LearnModeLegacy; }

// ####################################################################
//  set Keys
// ####################################################################
void JaroliftController::setKeys(unsigned long masterMSB, unsigned long masterLSB) {
  config.masterMSB = masterMSB;
  config.masterLSB = masterLSB;
}

// ####################################################################
//  get CC1101 connection state - return true if connected
// ####################################################################
bool JaroliftController::getCC1101State(void) { return cc1101.connected(); }

// ####################################################################
//  generates 16 serial numbers
// ####################################################################
uint32_t JaroliftController::cmd_get_serial(int channel) {
  uint32_t serial = (config.serial << 8) + channel;
  ESP_LOGD(TAG, "get serial: 0x%08lx for channel: %i", serial, channel + 1);
  return serial;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CC1101 radio functions group
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// ####################################################################
//  Generates sync-pulses
// ####################################################################
void JaroliftController::radio_tx_frame(int l) {
  for (int i = 0; i < l; ++i) {
    digitalWrite(gpio.gdo0, LOW);
    delayMicroseconds(400); // change 28.01.2018 default highpulse
    digitalWrite(gpio.gdo0, HIGH);
    delayMicroseconds(380); // change 28.01.2018 default lowpulse
  }
} // void radio_tx_frame

// ####################################################################
//  Sending of high_group_bits 8-16
// ####################################################################
void JaroliftController::radio_tx_group_h() {
  for (int i = 0; i < 8; i++) {
    int out = ((disc_h >> i) & 0x1); // Bitmask to get MSB and send it first
    if (out == 0x1) {
      digitalWrite(gpio.gdo0, LOW); // Simple encoding of bit state 1
      delayMicroseconds(Lowpulse);
      digitalWrite(gpio.gdo0, HIGH);
      delayMicroseconds(Highpulse);
    } else {
      digitalWrite(gpio.gdo0, LOW); // Simple encoding of bit state 0
      delayMicroseconds(Highpulse);
      digitalWrite(gpio.gdo0, HIGH);
      delayMicroseconds(Lowpulse);
    }
  }
} // void radio_tx_group_h

// ####################################################################
//  Receive Routine
// ####################################################################
void ICACHE_RAM_ATTR radio_rx_measure() {
  static long LineUp, LineDown, Timeout;
  long LowVal, HighVal;
  int pinstate = digitalRead(gpio.gdo2); // Read current pin state GDO2
  if (micros() - Timeout > 3500) {
    pbwrite = 0;
  }
  if (pinstate) // pin is now HIGH, was low
  {
    LineUp = micros();          // Get actual time in LineUp
    LowVal = LineUp - LineDown; // calculate the LOW pulse time
    if (LowVal < debounce)
      return;
    if ((LowVal > 300) && (LowVal < 4300)) {
      if ((LowVal > 3650) && (LowVal < 4300)) {
        Timeout = micros();
        pbwrite = 0;
        lowbuf[pbwrite] = LowVal;
        pbwrite = pbwrite + 1;
      }
      if ((LowVal > 300) && (LowVal < 1000)) {
        lowbuf[pbwrite] = LowVal;
        pbwrite = pbwrite + 1;
        Timeout = micros();
      }
    }
  } else {
    LineDown = micros();         // line went LOW after being HIGH
    HighVal = LineDown - LineUp; // calculate the HIGH pulse time
    if (HighVal < debounce)
      return;
    if ((HighVal > 300) && (HighVal < 1000)) {
      hibuf[pbwrite] = HighVal;
    }
  }
} // void ICACHE_RAM_ATTR radio_rx_measure

// ####################################################################
//  Generation of the encrypted message (Hopcode)
// ####################################################################
void JaroliftController::keeloq() {
  Keeloq k(device_key_msb, device_key_lsb);
  unsigned int result = (disc << 16) | devcnt; // Append counter value to discrimination value
  dec = k.encrypt(result);
} // void keeloq

// ####################################################################
//  Keygen generates the device crypt key in relation to the masterkey and provided serial number.
//  Here normal key-generation is used according to 00745a_c.PDF Appendix G.
//  https://github.com/hnhkj/documents/blob/master/KEELOQ/docs/AN745/00745a_c.pdf
// ####################################################################
void JaroliftController::keygen() {
  Keeloq k(config.masterMSB, config.masterLSB);
  uint64_t keylow = new_serial | 0x20000000;
  unsigned long enc = k.decrypt(keylow);
  device_key_lsb = enc; // Stores LSB devicekey 16Bit
  keylow = new_serial | 0x60000000;
  enc = k.decrypt(keylow);
  device_key_msb = enc; // Stores MSB devicekey 16Bit
  ESP_LOGD(TAG, "created devicekey low: 0x%08x // high: 0x%08x", device_key_lsb, device_key_msb);
} // void keygen

// ####################################################################
//  Simple TX routine. Repetitions for simulate continuous button press.
//  Send code two times. In case of one shutter did not "hear" the command.
// ####################################################################
void JaroliftController::radio_tx(int repetitions) {
  pack = (button << 60) | (new_serial << 32) | dec;
  for (int a = 0; a < repetitions; a++) {
    digitalWrite(gpio.gdo0, LOW); // CC1101 in TX Mode+
    delayMicroseconds(1150);
    radio_tx_frame(13); // change 28.01.2018 default 10
    delayMicroseconds(3500);

    for (int i = 0; i < 64; i++) {

      int out = ((pack >> i) & 0x1); // Bitmask to get MSB and send it first
      if (out == 0x1) {
        digitalWrite(gpio.gdo0, LOW); // Simple encoding of bit state 1
        delayMicroseconds(Lowpulse);
        digitalWrite(gpio.gdo0, HIGH);
        delayMicroseconds(Highpulse);
      } else {
        digitalWrite(gpio.gdo0, LOW); // Simple encoding of bit state 0
        delayMicroseconds(Highpulse);
        digitalWrite(gpio.gdo0, HIGH);
        delayMicroseconds(Lowpulse);
      }
    }
    radio_tx_group_h(); // Last 8Bit. For motor 8-16.

    delay(16); // delay in loop context is save for wdt
  }
} // void radio_tx

// ####################################################################
//  Calculate device code from received serial number
// ####################################################################
void JaroliftController::rx_keygen() {
  Keeloq k(config.masterMSB, config.masterLSB);
  uint32_t keylow = rx_serial | 0x20000000;
  unsigned long enc = k.decrypt(keylow);
  rx_device_key_lsb = enc; // Stores LSB devicekey 16Bit
  keylow = rx_serial | 0x60000000;
  enc = k.decrypt(keylow);
  rx_device_key_msb = enc; // Stores MSB devicekey 16Bit

  ESP_LOGD(TAG, "received devicekey low: 0x%08x // high: 0x%08x", rx_device_key_lsb, rx_device_key_msb);
} // void rx_keygen

// ####################################################################
//  Decoding of the hopping code
// ####################################################################
void JaroliftController::rx_decoder() {
  Keeloq k(rx_device_key_msb, rx_device_key_lsb);
  unsigned int result = rx_hopcode;
  decoded = k.decrypt(result);
  rx_disc_low[0] = (decoded >> 24) & 0xFF;
  rx_disc_low[1] = (decoded >> 16) & 0xFF;
  rx_disc_low[2] = (decoded >> 8) & 0xFF;
  rx_disc_low[3] = decoded & 0xFF;

  ESP_LOGD(TAG, "decoded devicekey: 0x%08lx", decoded);
} // void rx_decoder

// ####################################################################
//  calculate RSSI value (Received Signal Strength Indicator)
// ####################################################################
uint8_t JaroliftController::getRssi() {
  uint8_t rssi, value = 0;
  rssi = (cc1101.readReg(CC1101_RSSI, CC1101_STATUS_REGISTER));
  if (rssi >= 128) {
    value = 255 - rssi;
    value = value / 2;
    value = value + 74;
  } else {
    value = rssi / 2;
    value = value + 74;
  }
  ESP_LOGD(TAG, " CC1101_RSSI: %i", value);
  return value;
} // void ReadRSSI

// ####################################################################
//  put CC1101 to receive mode
// ####################################################################
void JaroliftController::enterrx() {
  cc1101.setRxState();
  delay(2);
  rx_time = micros();

  while (((marcState = cc1101.readStatusReg(CC1101_MARCSTATE)) & 0x1F) != 0x0D) {
    if (micros() - rx_time > 50000)
      break; // Quit when marcState does not change...
  }
} // void enterrx

// ####################################################################
//  put CC1101 to send mode
// ####################################################################
void JaroliftController::entertx() {
  cc1101.setTxState();
  delay(2);
  rx_time = micros();
  while (((marcState = cc1101.readStatusReg(CC1101_MARCSTATE)) & 0x1F) != 0x13 && 0x14 && 0x15) {
    if (micros() - rx_time > 50000)
      break; // Quit when marcState does not change...
  }
} // void entertx

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// execute cmd_ functions group
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// ####################################################################
//  function to move the shutter up
// ####################################################################
void JaroliftController::cmdUp(uint8_t channel) {

  if (!initOK) {
    return;
  }
  new_serial = cmd_get_serial(channel);
  devcnt = getDeviceCounter();
  button = 0x8;
  disc_l = disc_low[channel];
  disc_h = disc_high[channel];
  disc = (disc_l << 8) | (new_serial & 0xFF);
  rx_disc_low[0] = disc_l;
  rx_disc_high[0] = disc_h;
  keygen();
  keeloq();
  entertx();
  radio_tx(2);
  enterrx();
  rx_function = 0x8;
  devcnt_handler(true);
} // void jaroCmdUp

void JaroliftController::cmdGroupUp(uint16_t group_mask) {

  if (!initOK) {
    return;
  }
  devcnt = getDeviceCounter();
  new_serial = cmd_get_serial(0);
  button = 0x8;
  disc_l = group_mask & 0x00FF;
  disc_h = (group_mask >> 8) & 0x00FF;
  disc = (disc_l << 8) | (new_serial & 0xFF);
  rx_disc_low[0] = disc_l;
  rx_disc_high[0] = disc_h;
  keygen();
  keeloq();
  entertx();
  radio_tx(2);
  enterrx();
  rx_function = 0x8;
  devcnt_handler(true);
} // void cmdGroupUp

// ####################################################################
//  function to move the shutter down
// ####################################################################
void JaroliftController::cmdDown(uint8_t channel) {

  if (!initOK) {
    return;
  }
  new_serial = cmd_get_serial(channel);
  devcnt = getDeviceCounter();
  button = 0x2;
  disc_l = disc_low[channel];
  disc_h = disc_high[channel];
  disc = (disc_l << 8) | (new_serial & 0xFF);
  rx_disc_low[0] = disc_l;
  rx_disc_high[0] = disc_h;
  keygen();
  keeloq(); // Generate encrypted message 32Bit hopcode
  entertx();
  radio_tx(2); // Call TX routine
  enterrx();
  rx_function = 0x2;
  devcnt_handler(true);
} // void jaroCmdDown

void JaroliftController::cmdGroupDown(uint16_t group_mask) {

  if (!initOK) {
    return;
  }
  devcnt = getDeviceCounter();
  new_serial = cmd_get_serial(0);
  button = 0x2;
  disc_l = group_mask & 0x00FF;
  disc_h = (group_mask >> 8) & 0x00FF;
  disc = (disc_l << 8) | (new_serial & 0xFF);
  rx_disc_low[0] = disc_l;
  rx_disc_high[0] = disc_h;
  keygen();
  keeloq();
  entertx();
  radio_tx(2);
  enterrx();
  rx_function = 0x2;
  devcnt_handler(true);
} // void cmdGroupDown

// ####################################################################
//  function to stop the shutter
// ####################################################################
void JaroliftController::cmdStop(uint8_t channel) {

  if (!initOK) {
    return;
  }
  new_serial = cmd_get_serial(channel);
  devcnt = getDeviceCounter();
  button = 0x4;
  disc_l = disc_low[channel];
  disc_h = disc_high[channel];
  disc = (disc_l << 8) | (new_serial & 0xFF);
  rx_disc_low[0] = disc_l;
  rx_disc_high[0] = disc_h;
  keygen();
  keeloq();
  entertx();
  radio_tx(2);
  enterrx();
  rx_function = 0x4;
  devcnt_handler(true);
} // void jaroCmdStop

void JaroliftController::cmdGroupStop(uint16_t group_mask) {

  if (!initOK) {
    return;
  }
  devcnt = getDeviceCounter();
  new_serial = cmd_get_serial(0);
  button = 0x4;
  disc_l = group_mask & 0x00FF;
  disc_h = (group_mask >> 8) & 0x00FF;
  disc = (disc_l << 8) | (new_serial & 0xFF);
  rx_disc_low[0] = disc_l;
  rx_disc_high[0] = disc_h;
  keygen();
  keeloq();
  entertx();
  radio_tx(2);
  enterrx();
  rx_function = 0x4;
  devcnt_handler(true);
} // void cmdGroupStop

// ####################################################################
//  function to move shutter to shade position
// ####################################################################
void JaroliftController::cmdShade(uint8_t channel) {

  if (!initOK) {
    return;
  }
  new_serial = cmd_get_serial(channel);
  devcnt = getDeviceCounter();
  button = 0x4;
  disc_l = disc_low[channel];
  disc_h = disc_high[channel];
  disc = (disc_l << 8) | (new_serial & 0xFF);
  rx_disc_low[0] = disc_l;
  rx_disc_high[0] = disc_h;
  keygen();
  keeloq();
  entertx();
  radio_tx(20);
  enterrx();
  rx_function = 0x3;
  devcnt_handler(true);
} // void jaroCmdShade

void JaroliftController::cmdGroupShade(uint16_t group_mask) {

  if (!initOK) {
    return;
  }
  devcnt = getDeviceCounter();
  new_serial = cmd_get_serial(0);
  button = 0x4;
  disc_l = group_mask & 0x00FF;
  disc_h = (group_mask >> 8) & 0x00FF;
  disc = (disc_l << 8) | (new_serial & 0xFF);
  rx_disc_low[0] = disc_l;
  rx_disc_high[0] = disc_h;
  keygen();
  keeloq();
  entertx();
  radio_tx(20);
  enterrx();
  rx_function = 0x3;
  devcnt_handler(true);
} // void cmdGroupShade

// ####################################################################
//  function to set the learn/set the shade position
// ####################################################################
void JaroliftController::cmdSetShade(uint8_t channel) {

  if (!initOK) {
    return;
  }

  new_serial = cmd_get_serial(channel);
  devcnt = getDeviceCounter();
  button = 0x4;
  disc_l = disc_low[channel];
  disc_h = disc_high[channel];
  disc = (disc_l << 8) | (new_serial & 0xFF);
  rx_disc_low[0] = disc_l;
  rx_disc_high[0] = disc_h;
  keygen();

  for (int i = 0; i < 4; i++) {
    entertx();
    keeloq();
    radio_tx(1);
    devcnt++;
    enterrx();
    delay(300);
  }
  rx_function = 0x6;
  ESP_LOGI(TAG, "command SET SHADE for channel %i sent", channel + 1);
  devcnt_handler(false);
  delay(2000); // Safety time to prevent accidentally erase of end-points.
} // void cmd_set_shade_position

// ####################################################################
//  function to put the dongle into the learn mode and
//  send learning packet.
// ####################################################################
void JaroliftController::cmdLearn(uint8_t channel) {

  if (!initOK) {
    return;
  }

  ESP_LOGI(TAG, "putting channel %i into learn mode ...", channel + 1);
  new_serial = cmd_get_serial(channel);
  devcnt = getDeviceCounter();
  ESP_LOGD(TAG, "Device Counter from EEPROM: %i", devcnt);
  ESP_LOGD(TAG, "get serial: %#llx", new_serial);
  if (config.learn_mode == true)
    button = 0xA; // New learn method. Up+Down followd by Stop.
  else
    button = 0x1; // Old learn method for receiver before Mfg date 2010.
  disc_l = disc_low[channel];
  disc_h = disc_high[channel];
  disc = (disc_l << 8) | (new_serial & 0xFF);
  keygen();
  keeloq();
  entertx();
  radio_tx(1);
  enterrx();
  devcnt++;
  if (config.learn_mode == true) {
    delay(1000);
    button = 0x4; // Stop
    keeloq();
    entertx();
    radio_tx(1);
    enterrx();
    devcnt++;
  }
  devcnt_handler(false);
  ESP_LOGI(TAG, "Channel learned!");
} // void jaroCmdLearn

// ####################################################################
//  function to send UP+DOWN button at same time
// ####################################################################
void JaroliftController::cmdUpDown(uint8_t channel) {

  if (!initOK) {
    return;
  }

  new_serial = cmd_get_serial(channel);
  devcnt = getDeviceCounter();
  button = 0xA;
  disc_l = disc_low[channel];
  disc_h = disc_high[channel];
  disc = (disc_l << 8) | (new_serial & 0xFF);
  keygen();
  keeloq();
  entertx();
  radio_tx(1);
  enterrx();
  devcnt_handler(true);
  ESP_LOGI(TAG, "command UPDOWN for channel %i sent", channel + 1);
} // void cmd_updown

void JaroliftController::begin() {

  EEPROM.begin(sizeof(devcnt));
  devcnt = getDeviceCounter();

  // initialize the transceiver chip
  cc1101.setGPIO(gpio.sck, gpio.miso, gpio.mosi, gpio.cs, gpio.gdo0);

  if (cc1101.init() == false) {
    ESP_LOGE(TAG, "Initialisation of the CC1101 module aborted!");
    return;
  }

  cc1101.setSyncWord(syncWord, false);
  cc1101.setCarrierFreq(CFREQ_433);
  cc1101.disableAddressCheck();          // if not specified, will only display "packet received"
  cc1101.setTxPowerAmp(PA_LongDistance); // Long Distance

  // TX (GDO0) Pin
  pinMode(gpio.gdo0, OUTPUT);
  // RX (GDO2)
  pinMode(gpio.gdo2, INPUT_PULLUP);
  attachInterrupt(gpio.gdo2, radio_rx_measure, CHANGE); // Interrupt on change of RX_PORT

  initOK = true;
}

void JaroliftController::loop() {

  if (!initOK) {
    return;
  }

  if (iset) {
    cc1101.cmdStrobe(CC1101_SCAL);
    delay(50);
    enterrx();
    iset = false;
    delay(200);
    attachInterrupt(gpio.gdo2, radio_rx_measure, CHANGE); // Interrupt on change of gpio.gdo2
  }

  // Check if RX buffer is full
  if ((lowbuf[0] > 3650) && (lowbuf[0] < 4300) && (pbwrite >= 65) && (pbwrite <= 75)) { // Decode received data...

    iset = true;
    pbwrite = 0;

    for (int i = 0; i <= 31; i++) { // extracting Hopcode
      if (lowbuf[i + 1] < hibuf[i + 1]) {
        rx_hopcode = (rx_hopcode & ~(1 << i)) | (0 << i);
      } else {
        rx_hopcode = (rx_hopcode & ~(1 << i)) | (1 << i);
      }
    }
    for (int i = 0; i <= 27; i++) { // extracting Serialnumber
      if (lowbuf[i + 33] < hibuf[i + 33]) {
        rx_serial = (rx_serial & ~(1 << i)) | (0 << i);
      } else {
        rx_serial = (rx_serial & ~(1 << i)) | (1 << i);
      }
    }

    for (int i = 0; i <= 3; i++) { // extracting function code
      if (lowbuf[61 + i] < hibuf[61 + i]) {
        rx_function = (rx_function & ~(1 << i)) | (0 << i);
      } else {
        rx_function = (rx_function & ~(1 << i)) | (1 << i);
      }
    }

    for (int i = 0; i <= 7; i++) { // extracting high disc
      if (lowbuf[65 + i] < hibuf[65 + i]) {
        rx_disc_h = (rx_disc_h & ~(1 << i)) | (0 << i);
      } else {
        rx_disc_h = (rx_disc_h & ~(1 << i)) | (1 << i);
      }
    }

    rx_disc_high[0] = rx_disc_h & 0xFF;
    rx_keygen();
    rx_decoder();
    if (rx_function == 0x4)
      steadycnt++; // to detect a long press....
    else
      steadycnt--;
    if (steadycnt > 10 && steadycnt <= 40) {
      rx_function = 0x3;
      steadycnt = 0;
    }

    ESP_LOGD(TAG, "(INF1) serial: 0x%08lx, rx_function: 0x%x, rx_disc_low: %d, rx_disc_high: %d", rx_serial, rx_function, rx_disc_low[0], rx_disc_h);
    ESP_LOGD(TAG, "(INF2) RSSI: %d, counter: %d", getRssi(), rx_disc_low[3]);
    ESP_LOGD(TAG, "(INF3) rx_device_key_lsb: 0x%08x, rx_device_key_msb: 0x%08x, decoded: 0x%08lx", rx_device_key_lsb, rx_device_key_msb, decoded);

    rx_disc_h = 0;
    rx_hopcode = 0;
    rx_function = 0;
  }
}
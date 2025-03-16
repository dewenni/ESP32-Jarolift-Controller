// JaroliftController.cpp
#include "JaroliftController.h"

// Definition of the static instance pointer
JaroliftController *JaroliftController::instance_ = nullptr;

JaroliftController::JaroliftController()
    : devCount_(0), deviceKeyMSB_(0), deviceKeyLSB_(0), button_(0), discL_(0), discH_(0), disc_(0), newSerial_(0), encrypted_(0), pack_(0),
      pbWrite_(0), rxSerial_(0), rxHopCode_(0), rxFunction_(0), rxDiscH_(0), initOK_(false), steadyCount_(0), rxDataReady_(false) {

  memset((void *)lowBuf_, 0, sizeof(lowBuf_));
  memset((void *)hiBuf_, 0, sizeof(hiBuf_));

  uint8_t defaultDiscLow[16] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
  uint8_t defaultDiscHigh[16] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
  memcpy(discLowArr_, defaultDiscLow, sizeof(discLowArr_));
  memcpy(discHighArr_, defaultDiscHigh, sizeof(discHighArr_));

  config_.serial = 0;
  config_.learnMode = true;
  config_.masterMSB = 0;
  config_.masterLSB = 0;

  // Set the Singleton instance
  instance_ = this;
}

JaroliftController::~JaroliftController() { instance_ = nullptr; }

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// helper and setter functions
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 *******************************************************************
 * @brief   set gpio for CC1101
 * @param   sck, miso, mosi, cs, gd0, gd2
 * @return  none
 * *******************************************************************/
void JaroliftController::setGPIO(int sck, int miso, int mosi, int cs, int gdo0, int gdo2) {
  gpio_.sck = sck;
  gpio_.miso = miso;
  gpio_.mosi = mosi;
  gpio_.cs = cs;
  gpio_.gdo0 = gdo0;
  gpio_.gdo2 = gdo2;
}

/**
 *******************************************************************
 * @brief   set serial number (6 of 8 bytes)
 * @param   serial
 * @return  none
 * *******************************************************************/
void JaroliftController::setBaseSerial(uint32_t serial) {
  config_.serial = serial;
  ESP_LOGI(TAG, "Set base serial: 0x%08lx", config_.serial);
}

/**
 *******************************************************************
 * @brief   get serial for given channel
 * @param   channel
 * @return  none
 * *******************************************************************/
uint32_t JaroliftController::getSerial(uint8_t channel) {
  uint32_t serial = (config_.serial << 8) | channel;
  ESP_LOGD(TAG, "serial: 0x%08lx | channel: %d", serial, channel + 1);
  return serial;
}

/**
 *******************************************************************
 * @brief   set legacy learn mode
 * @param   legacyMode
 * @return  none
 * *******************************************************************/
void JaroliftController::setLegacyLearnMode(bool legacyMode) { config_.learnMode = !legacyMode; }

/**
 *******************************************************************
 * @brief   set jarolift master keys
 * @param   masterMSB, masterLSB
 * @return  none
 * *******************************************************************/
void JaroliftController::setKeys(unsigned long masterMSB, unsigned long masterLSB) {
  config_.masterMSB = masterMSB;
  config_.masterLSB = masterLSB;
}

/**
 *******************************************************************
 * @brief   get state if CC1101 is connected
 * @param   none
 * @return  none
 * *******************************************************************/
bool JaroliftController::getCC1101State() { return cc1101_.connected(); }

/**
 *******************************************************************
 * @brief   get state if CC1101 is connected
 * @param   none
 * @return  none
 * *******************************************************************/
uint16_t JaroliftController::getDeviceCounter() {
  nvs_handle_t nvsHandle;
  esp_err_t err = nvs_open("device_data", NVS_READWRITE, &nvsHandle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS");
    return 0;
  }
  uint16_t counter = 0;
  err = nvs_get_u16(nvsHandle, "devcnt", &counter);
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    // try to read from EEPROM if not found in NVS (migration)
    EEPROM.get(0, counter);
    nvs_set_u16(nvsHandle, "devcnt", counter);
    nvs_commit(nvsHandle);
    ESP_LOGI(TAG, "Migrated devcnt=%d from EEPROM to NVS", counter);
  }
  nvs_close(nvsHandle);
  if (counter == 0) {
    counter = 1;
    setDeviceCounter(counter);
  }
  return counter;
}

/**
 *******************************************************************
 * @brief   set device counter
 * @param   newDevCnt
 * @return  none
 * *******************************************************************/
void JaroliftController::setDeviceCounter(uint16_t newDevCnt) {
  devCount_ = newDevCnt;
  nvs_handle_t nvsHandle;
  esp_err_t err = nvs_open("device_data", NVS_READWRITE, &nvsHandle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS");
    return;
  }
  nvs_set_u16(nvsHandle, "devcnt", devCount_);
  nvs_commit(nvsHandle);
  nvs_close(nvsHandle);
  delay(100);
}

/**
 *******************************************************************
 * @brief   update and increment device counter
 * @param   increment
 * @return  none
 * *******************************************************************/
void JaroliftController::updateDeviceCounter(bool increment) {
  devCount_ = getDeviceCounter();
  if (increment) {
    devCount_++;
    setDeviceCounter(devCount_);
  }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CC1101 radio functions group
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 *******************************************************************
 * @brief   Generates sync-pulses (protocoll start)
 * @param   length
 * @return  none
 * *******************************************************************/
void JaroliftController::radioTxFrame(int length) {
  for (int i = 0; i < length; ++i) {
    digitalWrite(gpio_.gdo0, LOW);
    delayMicroseconds(400);
    digitalWrite(gpio_.gdo0, HIGH);
    delayMicroseconds(380);
  }
}

/**
 *******************************************************************
 * @brief   Sending of high_group_bits 8-16 (discH_)
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::radioTxGroupH() {
  for (int i = 0; i < 8; i++) {
    int bitVal = (discH_ >> i) & 0x1;
    if (bitVal == 1) {
      digitalWrite(gpio_.gdo0, LOW);
      delayMicroseconds(kLowPulse);
      digitalWrite(gpio_.gdo0, HIGH);
      delayMicroseconds(kHighPulse);
    } else {
      digitalWrite(gpio_.gdo0, LOW);
      delayMicroseconds(kHighPulse);
      digitalWrite(gpio_.gdo0, HIGH);
      delayMicroseconds(kLowPulse);
    }
  }
}

/**
 *******************************************************************
 * @brief   tx routine to send data
 * @param   repetitions
 * @return  none
 * *******************************************************************/
void JaroliftController::radioTx(int repetitions) {
  pack_ = (button_ << 60) | (newSerial_ << 32) | encrypted_;
  for (int a = 0; a < repetitions; a++) {
    digitalWrite(gpio_.gdo0, LOW);
    delayMicroseconds(1150);
    radioTxFrame(13);
    delayMicroseconds(3500);
    for (int i = 0; i < 64; i++) {
      int bitVal = (pack_ >> i) & 0x1;
      if (bitVal == 1) {
        digitalWrite(gpio_.gdo0, LOW);
        delayMicroseconds(kLowPulse);
        digitalWrite(gpio_.gdo0, HIGH);
        delayMicroseconds(kHighPulse);
      } else {
        digitalWrite(gpio_.gdo0, LOW);
        delayMicroseconds(kHighPulse);
        digitalWrite(gpio_.gdo0, HIGH);
        delayMicroseconds(kLowPulse);
      }
    }
    radioTxGroupH();
    delay(16);
  }
}

/**
 *******************************************************************
 * @brief   put CC1101 to receive mode
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::enterRx() {
  cc1101_.setRxState();
  delay(2);
  unsigned long startTime = micros();
  uint8_t marcState = 0;
  while (((marcState = cc1101_.readStatusReg(CC1101_MARCSTATE)) & 0x1F) != 0x0D) {
    if (micros() - startTime > 50000)
      break;
  }
}

/**
 *******************************************************************
 * @brief   put CC1101 to send mode
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::enterTx() {
  cc1101_.setTxState();
  delay(2);
  unsigned long startTime = micros();
  uint8_t marcState = 0;
  while (((marcState = cc1101_.readStatusReg(CC1101_MARCSTATE)) & 0x1F) != 0x13 && ((marcState & 0x1F) != 0x14) && ((marcState & 0x1F) != 0x15)) {
    if (micros() - startTime > 50000)
      break;
  }
}

/**
 *******************************************************************
 * @brief   calculate RSSI value (Received Signal Strength Indicator)
 * @param   none
 * @return  none
 * *******************************************************************/
uint8_t JaroliftController::getRssi() {
  uint8_t rssi = cc1101_.readReg(CC1101_RSSI, CC1101_STATUS_REGISTER);
  uint8_t value = 0;
  if (rssi >= 128) {
    value = 255 - rssi;
    value = value / 2;
    value = value + 74;
  } else {
    value = rssi / 2;
    value = value + 74;
  }
  return value;
}

/**
 *******************************************************************
 * @brief   Calculate device keys based on serial and master keys
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::generateKey() {
  Keeloq k(config_.masterMSB, config_.masterLSB);
  uint64_t keyInput = newSerial_ | 0x20000000;
  unsigned long enc = k.decrypt(keyInput);
  deviceKeyLSB_ = enc;
  keyInput = newSerial_ | 0x60000000;
  enc = k.decrypt(keyInput);
  deviceKeyMSB_ = enc;
}

/**
 *******************************************************************
 * @brief   Generation of the encrypted message (Hopcode)
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::generateEncrypted() {
  Keeloq k(deviceKeyMSB_, deviceKeyLSB_);
  devCount_ = getDeviceCounter();
  unsigned int result = (disc_ << 16) | devCount_; // Append counter value to discrimination value
  encrypted_ = k.encrypt(result);
}

/**
 *******************************************************************
 * @brief   encrypt device keys from received hopcode based on received Serialnumber
 * @details Here normal key-generation is used according to 00745a_c.PDF Appendix G.
 * @details https://github.com/hnhkj/documents/blob/master/KEELOQ/docs/AN745/00745a_c.pdf
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::rxKeyGen() {
  Keeloq k(config_.masterMSB, config_.masterLSB);
  uint32_t keylow = rxSerial_ | 0x20000000;
  unsigned long enc = k.decrypt(keylow);
  deviceKeyLSB_ = enc; // Stores LSB devicekey 16Bit
  keylow = rxSerial_ | 0x60000000;
  enc = k.decrypt(keylow);
  deviceKeyMSB_ = enc; // Stores MSB devicekey 16Bit
}

/**
 *******************************************************************
 * @brief   encrypt received hopcode based calculated device keys
 * @param   none
 * @return  none
 * *******************************************************************/
uint32_t JaroliftController::rxDecode() {
  Keeloq k(deviceKeyMSB_, deviceKeyLSB_);
  unsigned int result = rxHopCode_;
  return k.decrypt(result);
}

/**
 *******************************************************************
 * @brief   Interrupt-Service-Routine (ISR)
 * @param   none
 * @return  none
 * *******************************************************************/
void IRAM_ATTR JaroliftController::radioRxMeasureISR() {
  if (instance_) {
    instance_->handleRadioRxMeasure();
  }
}

/**
 *******************************************************************
 * @brief   Handling incoming data
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::handleRadioRxMeasure() {
  static unsigned long lineUp = 0;
  static unsigned long lineDown = 0;
  static unsigned long timeout = 0;
  unsigned long currentMicros = micros();
  int pinState = digitalRead(gpio_.gdo2);
  if (currentMicros - timeout > 3500) {
    pbWrite_ = 0;
  }
  if (pinState) { // Übergang zu HIGH
    lineUp = currentMicros;
    unsigned long lowVal = lineUp - lineDown;
    if (lowVal < kDebounce)
      return;
    if (lowVal > 300 && lowVal < 4300) {
      if (lowVal > 3650 && lowVal < 4300) {
        timeout = currentMicros;
        pbWrite_ = 0;
        lowBuf_[pbWrite_] = lowVal;
        pbWrite_++;
      } else if (lowVal > 300 && lowVal < 1000) {
        lowBuf_[pbWrite_] = lowVal;
        pbWrite_++;
        timeout = currentMicros;
      }
    }
  } else { // Übergang zu LOW
    lineDown = currentMicros;
    unsigned long highVal = lineDown - lineUp;
    if (highVal < kDebounce)
      return;
    if (highVal > 300 && highVal < 1000) {
      hiBuf_[pbWrite_] = highVal;
    }
  }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Shutter command functions
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 * *******************************************************************
 * @brief   send channel commands (Up, DOWN, STOP, SHADE)
 * @param   cmd, channel
 * @return  none
 * *******************************************************************/
void JaroliftController::cmdChannel(commands cmd, uint8_t channel) {
  if (!initOK_)
    return;
  newSerial_ = getSerial(channel);

  switch (cmd) {
  case CMD_UP:
    button_ = FCT_CODE_UP;
    discL_ = discLowArr_[channel];
    discH_ = discHighArr_[channel];
    disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
    generateKey();
    generateEncrypted();
    enterTx();
    radioTx(2); // send command 2-times with same devCnt
    enterRx();
    updateDeviceCounter(true);
    break;

  case CMD_DOWN:
    button_ = FCT_CODE_DOWN;
    discL_ = discLowArr_[channel];
    discH_ = discHighArr_[channel];
    disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
    generateKey();
    generateEncrypted();
    enterTx();
    radioTx(2); // send command 2-times with same devCnt
    enterRx();
    updateDeviceCounter(true);
    break;

  case CMD_STOP:
    button_ = FCT_CODE_STOP;
    discL_ = discLowArr_[channel];
    discH_ = discHighArr_[channel];
    disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
    generateKey();
    generateEncrypted();
    enterTx();
    radioTx(2); // send command 2-times
    enterRx();
    updateDeviceCounter(true);
    break;

  case CMD_SHADE:
    button_ = FCT_CODE_STOP;
    discL_ = discLowArr_[channel];
    discH_ = discHighArr_[channel];
    disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
    generateKey();
    generateEncrypted();
    enterTx();
    radioTx(20); // send "continuos STOP"
    enterRx();
    updateDeviceCounter(true);
    break;

  case CMD_SET_SHADE:
    button_ = FCT_CODE_STOP;
    discL_ = discLowArr_[channel];
    discH_ = discHighArr_[channel];
    disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
    generateKey();
    // send 4-times STOP
    for (int i = 0; i < 4; i++) {
      enterTx();
      generateEncrypted();
      radioTx(1);
      updateDeviceCounter(true);
      enterRx();
      delay(300);
    }
    break;

  default:
    break;
  }
}

/**
 * *******************************************************************
 * @brief   send group commands (Up, DOWN, STOP, SHADE)
 * @param   cmd, channel
 * @return  none
 * *******************************************************************/
void JaroliftController::cmdGroup(commands cmd, uint16_t groupMask) {
  if (!initOK_)
    return;
  devCount_ = getDeviceCounter();
  newSerial_ = getSerial(0);

  switch (cmd) {
  case CMD_UP:
    button_ = FCT_CODE_UP;
    break;

  case CMD_DOWN:
    button_ = FCT_CODE_DOWN;
    break;

  case CMD_STOP:
  case CMD_SHADE:
    button_ = FCT_CODE_STOP;
    break;

  default:
    break;
  }

  discL_ = groupMask & 0x00FF;
  discH_ = (groupMask >> 8) & 0x00FF;
  disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
  generateKey();
  generateEncrypted();
  enterTx();
  if (cmd == CMD_SHADE) {
    radioTx(20); // send "continuos STOP"
  } else {
    radioTx(2); // send command 2-times
  }

  enterRx();
  updateDeviceCounter(true);
}

/**
 * *******************************************************************
 * @brief   send command to learn a new shutter
 * @param   channel
 * @return  none
 * *******************************************************************/
void JaroliftController::cmdLearn(uint8_t channel) {
  if (!initOK_)
    return;
  newSerial_ = getSerial(channel);
  devCount_ = getDeviceCounter();
  ESP_LOGD(TAG, "learn | Device Counter: %d | Serial: 0x%08llx", devCount_, newSerial_);
  button_ = config_.learnMode ? 0xA : 0x1;
  discL_ = discLowArr_[channel];
  discH_ = discHighArr_[channel];
  disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
  generateKey();
  generateEncrypted();
  enterTx();
  radioTx(2);
  enterRx();
  updateDeviceCounter(true);
  if (config_.learnMode) {
    delay(1000);
    button_ = 0x4; // Stop
    generateEncrypted();
    enterTx();
    radioTx(2);
    enterRx();
    updateDeviceCounter(true);
  }
}

/**
 * *******************************************************************
 * @brief   send command to unlearn a existing shutter
 * @details UP/DOWN -> 6x STOP -> UP
 * @param   channel
 * @return  none
 * *******************************************************************/
void JaroliftController::cmdUnlearn(uint8_t channel) {
  if (!initOK_)
    return;
  newSerial_ = getSerial(channel);
  devCount_ = getDeviceCounter();
  ESP_LOGD(TAG, "unlearn | Device Counter: %d | Serial: 0x%08llx", devCount_, newSerial_);
  discL_ = discLowArr_[channel];
  discH_ = discHighArr_[channel];
  disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
  generateKey();
  generateEncrypted();
  enterTx();
  button_ = FCT_CODE_UPDOWN; // Up+Down
  radioTx(2);
  enterRx();
  updateDeviceCounter(true);
  delay(300);
  for (int i = 0; i < 6; i++) {
    button_ = FCT_CODE_STOP; // 6x Stop
    enterTx();
    generateEncrypted();
    radioTx(2);
    updateDeviceCounter(true);
    enterRx();
    delay(300);
  }
  button_ = FCT_CODE_UP; // UP
  enterTx();
  generateEncrypted();
  radioTx(2);
  updateDeviceCounter(true);
  enterRx();
  updateDeviceCounter(false);
}

/**
 * *******************************************************************
 * @brief   send command to set the upper end point
 * @details UP/DOWN -> 2x STOP -> UP
 * @param   channel
 * @return  none
 * *******************************************************************/
void JaroliftController::cmdSetEndPointUp(uint8_t channel) {
  if (!initOK_)
    return;
  newSerial_ = getSerial(channel);
  devCount_ = getDeviceCounter();
  ESP_LOGD(TAG, "set upper end point | Device Counter: %d | Serial: 0x%08llx", devCount_, newSerial_);
  discL_ = discLowArr_[channel];
  discH_ = discHighArr_[channel];
  disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
  generateKey();
  generateEncrypted();
  enterTx();
  button_ = FCT_CODE_UPDOWN; // Up+Down
  radioTx(1);
  enterRx();
  updateDeviceCounter(true);
  delay(300);
  for (int i = 0; i < 2; i++) {
    button_ = FCT_CODE_STOP; // 2x Stop
    enterTx();
    generateEncrypted();
    radioTx(1);
    updateDeviceCounter(true);
    enterRx();
    delay(300);
  }
  button_ = FCT_CODE_UP; // UP
  enterTx();
  generateEncrypted();
  radioTx(1);
  updateDeviceCounter(true);
  enterRx();
}

/**
 * *******************************************************************
 * @brief   send command to delete the upper end point
 * @details UP/DOWN -> 2x STOP -> UP
 * @param   channel
 * @return  none
 * *******************************************************************/
void JaroliftController::cmdDeleteEndPointUp(uint8_t channel) {
  if (!initOK_)
    return;
  newSerial_ = getSerial(channel);
  devCount_ = getDeviceCounter();
  ESP_LOGD(TAG, "delete upper end point | Device Counter: %d | Serial: 0x%08llx", devCount_, newSerial_);
  discL_ = discLowArr_[channel];
  discH_ = discHighArr_[channel];
  disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
  generateKey();
  generateEncrypted();
  enterTx();
  button_ = FCT_CODE_UPDOWN; // Up+Down
  radioTx(1);
  enterRx();
  updateDeviceCounter(true);
  delay(300);
  for (int i = 0; i < 4; i++) {
    button_ = FCT_CODE_STOP; // 4x Stop
    enterTx();
    generateEncrypted();
    radioTx(1);
    updateDeviceCounter(true);
    enterRx();
    delay(300);
  }
  button_ = FCT_CODE_UP; // UP
  enterTx();
  generateEncrypted();
  radioTx(1);
  updateDeviceCounter(true);
  enterRx();
}

/**
 * *******************************************************************
 * @brief   send command to set the lower end point
 * @details UP/DOWN -> 2x STOP -> DOWN
 * @param   channel
 * @return  none
 * *******************************************************************/
void JaroliftController::cmdSetEndPointDown(uint8_t channel) {
  if (!initOK_)
    return;
  newSerial_ = getSerial(channel);
  devCount_ = getDeviceCounter();
  ESP_LOGD(TAG, "set lower end point | Device Counter: %d | Serial: 0x%08llx", devCount_, newSerial_);
  discL_ = discLowArr_[channel];
  discH_ = discHighArr_[channel];
  disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
  generateKey();
  generateEncrypted();
  enterTx();
  button_ = FCT_CODE_UPDOWN; // Up+Down
  radioTx(1);
  enterRx();
  updateDeviceCounter(true);
  delay(300);
  for (int i = 0; i < 2; i++) {
    button_ = FCT_CODE_STOP; // 2x Stop
    enterTx();
    generateEncrypted();
    radioTx(1);
    updateDeviceCounter(true);
    enterRx();
    delay(300);
  }
  button_ = FCT_CODE_DOWN; // DOWN
  enterTx();
  generateEncrypted();
  radioTx(1);
  updateDeviceCounter(true);
  enterRx();
}

/**
 * *******************************************************************
 * @brief   send command to delete the lower end point
 * @details UP/DOWN -> 2x STOP -> UP
 * @param   channel
 * @return  none
 * *******************************************************************/
void JaroliftController::cmdDeleteEndPointDown(uint8_t channel) {
  if (!initOK_)
    return;
  newSerial_ = getSerial(channel);
  devCount_ = getDeviceCounter();
  ESP_LOGD(TAG, "delete lower end point | Device Counter: %d | Serial: 0x%08llx", devCount_, newSerial_);
  discL_ = discLowArr_[channel];
  discH_ = discHighArr_[channel];
  disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
  generateKey();
  generateEncrypted();
  enterTx();
  button_ = FCT_CODE_UPDOWN; // Up+Down
  radioTx(1);
  enterRx();
  updateDeviceCounter(true);
  delay(300);
  for (int i = 0; i < 4; i++) {
    button_ = FCT_CODE_STOP; // 4x Stop
    enterTx();
    generateEncrypted();
    radioTx(1);
    updateDeviceCounter(true);
    enterRx();
    delay(300);
  }
  button_ = FCT_CODE_DOWN; // DOWN
  enterTx();
  generateEncrypted();
  radioTx(1);
  updateDeviceCounter(true);
  enterRx();
}

/**
 * *******************************************************************
 * @brief   process received data
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::processRxData() {

  // check if RX-Buffer is full and start to decode
  if ((lowBuf_[0] > 3650 && lowBuf_[0] < 4300) && (pbWrite_ >= 65 && pbWrite_ <= 75)) {
    rxDataReady_ = true;
    pbWrite_ = 0;

    // extract Hopcode (32 Bit)
    rxHopCode_ = 0;
    for (int i = 0; i < 32; i++) {
      if (lowBuf_[i + 1] < hiBuf_[i + 1])
        rxHopCode_ &= ~(1 << i);
      else
        rxHopCode_ |= (1 << i);
    }

    // extract Serial (28 Bit)
    rxSerial_ = 0;
    for (int i = 0; i < 28; i++) {
      if (lowBuf_[i + 33] < hiBuf_[i + 33])
        rxSerial_ &= ~(1 << i);
      else
        rxSerial_ |= (1 << i);
    }

    // extract function code (4 Bit)
    rxFunction_ = 0;
    for (int i = 0; i < 4; i++) {
      if (lowBuf_[61 + i] < hiBuf_[61 + i])
        rxFunction_ &= ~(1 << i);
      else
        rxFunction_ |= (1 << i);
    }
    // extract high disc - group bits (9-16 Bit)
    rxDiscH_ = 0;
    for (int i = 0; i < 8; i++) {
      if (lowBuf_[65 + i] < hiBuf_[65 + i])
        rxDiscH_ &= ~(1 << i);
      else
        rxDiscH_ |= (1 << i);
    }

    rxKeyGen();
    uint32_t decoded = rxDecode();
    if (rxFunction_ == 0x4)
      steadyCount_++;
    else
      steadyCount_--;
    if (steadyCount_ > 10 && steadyCount_ <= 40) {
      rxFunction_ = 0x3;
      steadyCount_ = 0;
    }

    // build channel information
    uint8_t ch_low = (decoded >> 24) & 0xFF;
    uint8_t ch_high = rxDiscH_ & 0xFF;
    uint16_t channel = (ch_high << 8) | ch_low;

    // callback function to receive information outside this library
    remoteCallback(rxSerial_, rxFunction_, channel);

    // reset variables
    rxDiscH_ = 0;
    rxHopCode_ = 0;
    rxFunction_ = 0;
    memset((void *)lowBuf_, 0, sizeof(lowBuf_));
    memset((void *)hiBuf_, 0, sizeof(hiBuf_));
  }
}

/**
 * *******************************************************************
 * @brief   Setup function for jarolift controller
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::begin() {
  ESP_LOGI(TAG, "start CC1101 setup");
  EEPROM.begin(sizeof(devCount_));
  devCount_ = getDeviceCounter();

  cc1101_.setGPIO(gpio_.sck, gpio_.miso, gpio_.mosi, gpio_.cs, gpio_.gdo0);
  if (!cc1101_.init()) {
    ESP_LOGE(TAG, "Initialisation of the CC1101 module aborted!");
    return;
  }
  cc1101_.setSyncWord(kSyncWord, false);
  cc1101_.setCarrierFreq(CFREQ_433);
  cc1101_.disableAddressCheck();
  cc1101_.setTxPowerAmp(PA_LongDistance);

  pinMode(gpio_.gdo0, OUTPUT);
  pinMode(gpio_.gdo2, INPUT_PULLUP);
  attachInterrupt(gpio_.gdo2, radioRxMeasureISR, CHANGE);

  initOK_ = true;
}

/**
 * *******************************************************************
 * @brief   cyclic process of jarolift controller
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::loop() {
  if (!initOK_)
    return;

  if (rxDataReady_) {
    cc1101_.cmdStrobe(CC1101_SCAL);
    delay(50);
    enterRx();
    rxDataReady_ = false;
    delay(200);
    attachInterrupt(gpio_.gdo2, radioRxMeasureISR, CHANGE);
  }

  processRxData(); // process received data
}

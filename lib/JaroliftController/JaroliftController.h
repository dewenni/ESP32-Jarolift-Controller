#include <Arduino.h>
#include <EEPROM.h>
#include <KeeloqLib.h>
#include <SPI.h>
#include <cc1101.h>

class JaroliftController {
private:
  void devcnt_handler(boolean do_increment);
  uint32_t cmd_get_serial(int channel);
  void radio_tx_frame(int l);
  void radio_tx_group_h();
  void keeloq();
  void keygen();
  void radio_tx(int repetitions);
  void rx_keygen();
  void rx_decoder();
  void ReadRSSI();
  void enterrx();
  void entertx();
  void (*remoteCallback)(uint32_t serial, int8_t function, uint8_t rssi) = nullptr;

public:
  void begin();

  void loop();

  void setRemoteCallback(void (*callback)(uint32_t serial, int8_t function, uint8_t rssi)) { remoteCallback = callback; }

  void setGPIO(int8_t sck, int8_t miso, int8_t mosi, int8_t ss, int8_t gdo0, int8_t gdo2);

  bool getCC1101State(void);

  uint8_t getRssi();

  void setLegacyLearnMode(bool LearnModeLegacy);

  void setKeys(unsigned long masterMSB, unsigned long masterLSB);

  void setBaseSerial(uint32_t serial);

  void setDeviceCounter(uint16_t devCnt);

  uint16_t getDeviceCounter();

  void cmdUp(uint8_t channel);

  void cmdGroupUp(uint16_t group_mask);

  void cmdGroupDown(uint16_t group_mask);

  void cmdGroupStop(uint16_t group_mask);

  void cmdGroupShade(uint16_t group_mask);

  void cmdDown(uint8_t channel);

  void cmdStop(uint8_t channel);

  void cmdShade(uint8_t channel);

  void cmdSetShade(uint8_t channel);

  void cmdLearn(uint8_t channel);

  void cmdUpDown(uint8_t channel);
};

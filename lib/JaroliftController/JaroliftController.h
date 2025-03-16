// JaroliftController.h
#ifndef JARO_LIFT_CONTROLLER_H
#define JARO_LIFT_CONTROLLER_H

#include "KeeloqLib.h"
#include "cc1101.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <nvs.h>
#include <nvs_flash.h>

class JaroliftController {
public:
  // Kapselung der GPIO- und Konfigurationsdaten
  struct GPIO {
    int sck;
    int miso;
    int mosi;
    int cs;
    int gdo0; // TX
    int gdo2; // RX
  };

  struct Config {
    uint32_t serial;
    bool learnMode;
    unsigned long masterMSB;
    unsigned long masterLSB;
  };

  // enum of shutter commands
  enum commands {
    CMD_UP,
    CMD_DOWN,
    CMD_STOP,
    CMD_SHADE,
    CMD_SET_SHADE,
  };

  JaroliftController();
  ~JaroliftController();

  // Lebenszyklus
  void begin();
  void loop();

  // Einzelbefehle
  void cmdChannel(commands cmd, uint8_t channel);

  // Gruppenbefehle
  void cmdGroup(commands cmd, uint16_t groupMask);

  // Service Commands
  void cmdLearn(uint8_t channel);
  void cmdUnlearn(uint8_t channel);
  void cmdSetEndPointUp(uint8_t channel);
  void cmdDeleteEndPointUp(uint8_t channel);
  void cmdSetEndPointDown(uint8_t channel);
  void cmdDeleteEndPointDown(uint8_t channel);

  // Konfigurationssetter
  void setGPIO(int sck, int miso, int mosi, int cs, int gdo0, int gdo2);
  void setBaseSerial(uint32_t serial);
  void setLegacyLearnMode(bool legacyMode);
  void setKeys(unsigned long masterMSB, unsigned long masterLSB);
  void setRemoteCallback(void (*callback)(uint32_t serial, int8_t function, uint16_t channel)) { remoteCallback = callback; }

  // Hilfsfunktionen
  uint16_t getDeviceCounter();
  void setDeviceCounter(uint16_t newDevCnt);
  uint32_t getSerial(uint8_t channel);
  bool getCC1101State();
  uint8_t getRssi();

private:
  // Instanzvariablen (anstatt globaler Variablen)
  GPIO gpio_;
  Config config_;
  uint16_t devCount_;

  int deviceKeyMSB_;
  int deviceKeyLSB_;

  uint64_t button_;
  uint8_t discL_;
  uint8_t discH_;
  uint16_t disc_;

  uint64_t newSerial_;

  uint32_t encrypted_;
  uint64_t pack_;

  // Empfangspuffer
  static constexpr size_t kPulseBufferSize = 216;
  volatile unsigned int lowBuf_[kPulseBufferSize];
  volatile unsigned int hiBuf_[kPulseBufferSize];
  volatile unsigned int pbWrite_;

  // Empfangsdaten
  uint32_t rxSerial_;
  uint32_t rxHopCode_;
  uint8_t rxFunction_;
  uint16_t rxDiscH_;

  // Laufzeitzustand
  bool initOK_;
  unsigned int steadyCount_;
  bool rxDataReady_;

  // Hardware-Modul
  CC1101 cc1101_;

  // Konstanten
  static constexpr int kLowPulse = 400; // in µs
  static constexpr int kHighPulse = 800;
  static constexpr int kDebounce = 200;
  static constexpr uint8_t kSyncWord = 199;

  static constexpr uint8_t FCT_CODE_UP = 0x8;
  static constexpr uint8_t FCT_CODE_DOWN = 0x2;
  static constexpr uint8_t FCT_CODE_STOP = 0x4;
  static constexpr uint8_t FCT_CODE_UPDOWN = 0xA;

  static constexpr char *TAG = "JARO-LIB"; // LOG TAG

  // Diskriminierungsarrays (analog zu den Originalwerten)
  uint8_t discLowArr_[16];
  uint8_t discHighArr_[16];

  // Hilfsfunktionen
  void updateDeviceCounter(bool increment);

  void radioTxFrame(int length);
  void radioTxGroupH();
  void radioTx(int repetitions);
  void enterRx();
  void enterTx();
  void processRxData();
  void generateKey();       // Schlüsselgenerierung (keygen)
  void generateEncrypted(); // Verschlüsseln (keeloq)

  void rxKeyGen();
  uint32_t rxDecode();

  void (*remoteCallback)(uint32_t serial, int8_t function, uint16_t channel) = nullptr;

  // Interrupt-Service-Routine (ISR) für RX-Messung
  static void IRAM_ATTR radioRxMeasureISR();
  void handleRadioRxMeasure();

  // Singleton-Zeiger für den ISR-Zugriff
  static JaroliftController *instance_;
};

#endif // JARO_LIFT_CONTROLLER_H

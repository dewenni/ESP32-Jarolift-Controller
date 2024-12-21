#include <JaroliftController.h>
#include <basics.h>
#include <config.h>
#include <jarolift.h>
#include <mqtt.h>
#include <queue>

#define MAX_CMD 20
#define SEND_CYCLE 500

static muTimer cmdTimer = muTimer();
static const char *TAG = "JARO"; // LOG TAG
std::queue<JaroCommand> jaroCmdQueue;

JaroliftController jarolift;

/**
 * *******************************************************************
 * @brief   message callback for JaroliftController
 * @param   level,msg
 * @return  none
 * *******************************************************************/
void msgCallback(esp_log_level_t level, const char *msg) {
  switch (level) {
  case ESP_LOG_ERROR:
    MY_LOGE(TAG, "%s", msg);
    break;
  case ESP_LOG_WARN:
    MY_LOGW(TAG, "%s", msg);
    break;
  case ESP_LOG_INFO:
    MY_LOGI(TAG, "%s", msg);
    break;
  case ESP_LOG_DEBUG:
    MY_LOGD(TAG, "%s", msg);
    break;
  default:
    MY_LOGI(TAG, "[UNKNOWN]: %s", msg);
    break;
  }
}

/**
 * *******************************************************************
 * @brief   add jarolift command to buffer
 * @param   type, channel
 * @return  none
 * *******************************************************************/
void jaroCmd(JaroCommandType type, uint8_t channel) {
  if (jaroCmdQueue.size() < MAX_CMD) {
    jaroCmdQueue.push({type, channel});
    MY_LOGD(TAG, "add cmd to buffer: %i, %i", type, channel + 1);
  } else {
    MY_LOGE(TAG, "to many commands within to short time");
  }
}

/**
 * *******************************************************************
 * @brief   Jarolift Setup function
 * @param   none
 * @return  none
 * *******************************************************************/
void jaroliftSetup() {

  // initialize
  MY_LOGI(TAG, "initializing the CC1101 Transceiver");
  jarolift.setMessageCallback(msgCallback);
  jarolift.setGPIO(config.gpio.sck, config.gpio.miso, config.gpio.mosi, config.gpio.cs, config.gpio.gdo0, config.gpio.gdo2);
  jarolift.setKeys(config.jaro.masterMSB, config.jaro.masterLSB);
  jarolift.setBaseSerial(config.jaro.serial);
  jarolift.begin();

  if (jarolift.getCC1101State()) {
    MY_LOGI(TAG, "CC1101 Transceiver connected!");
  } else {
    MY_LOGE(TAG, "CC1101 Transceiver NOT connected!");
  }

  MY_LOGI(TAG, "read Device Counter from EEPROM: %i", jarolift.getDeviceCounter());
}

/**
 * *******************************************************************
 * @brief   send expected position via mqtt
 * @param   channel, position
 * @return  none
 * *******************************************************************/
void mqttSendPosition(uint8_t channel, uint8_t position) {
  char topic[64];
  char pos[16];

  if (position > 100)
    position = 100;
  if (position < 0)
    position = 0;
  if (mqttIsConnected()) {
    itoa(position, pos, 10);
    snprintf(topic, sizeof(topic), "%s%d", addTopic("/status/shutter/"), channel + 1);
    mqttPublish(topic, pos, true);
  }
}

void jaroCmdReInit() { jaroliftSetup(); };
void jaroCmdSetDevCnt(uint16_t value) { jarolift.setDeviceCounter(value); };
uint16_t jaroGetDevCnt() { return jarolift.getDeviceCounter(); };
bool getCC1101State() { return jarolift.getCC1101State(); };
uint8_t getCC1101Rssi() { return jarolift.getRssi(); }

/**
 * *******************************************************************
 * @brief   execute jarolift commands from buffer
 * @param   type, channel
 * @return  none
 * *******************************************************************/
void processJaroCommands() {
  if (!jaroCmdQueue.empty()) {

    JaroCommand cmd = jaroCmdQueue.front();

    switch (cmd.type) {
    case CMD_UP:
      jarolift.cmdUp(cmd.channel);
      mqttSendPosition(cmd.channel, 0);
      MY_LOGI(TAG, "execute cmd: UP - channel: %i", cmd.channel + 1);
      break;
    case CMD_DOWN:
      jarolift.cmdDown(cmd.channel);
      mqttSendPosition(cmd.channel, 100);
      MY_LOGI(TAG, "execute cmd: DOWN - channel: %i", cmd.channel + 1);
      break;
    case CMD_STOP:
      jarolift.cmdStop(cmd.channel);
      MY_LOGI(TAG, "execute cmd: STOP - channel: %i", cmd.channel + 1);
      break;
    case CMD_SET_SHADE:
      jarolift.cmdSetShade(cmd.channel);
      mqttSendPosition(cmd.channel, 90);
      MY_LOGI(TAG, "execute cmd: SETSHADE - channel: %i", cmd.channel + 1);
      break;
    case CMD_SHADE:
      jarolift.cmdShade(cmd.channel);
      MY_LOGI(TAG, "execute cmd: SHADE - channel: %i", cmd.channel + 1);
      break;
    case CMD_LEARN:
      jarolift.cmdLearn(cmd.channel);
      MY_LOGI(TAG, "execute cmd: LEARN - channel: %i", cmd.channel + 1);
      break;
    }

    jaroCmdQueue.pop();
  }
}

/**
 * *******************************************************************
 * @brief   jarolift cyclic function
 * @param   none
 * @return  none
 * *******************************************************************/
void jaroliftCyclic() {

  if (cmdTimer.cycleTrigger(SEND_CYCLE)) {
    processJaroCommands();
  }

  jarolift.loop();
}
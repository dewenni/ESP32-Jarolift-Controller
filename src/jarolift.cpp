#include <JaroliftController.h>
#include <basics.h>
#include <config.h>
#include <jarolift.h>
#include <mqtt.h>
#include <queue>
#include <timer.h>

#define MAX_CMD 20
#define SEND_CYCLE 500
#define POS_OPEN 0
#define POS_CLOSE 100
#define POS_SHADE 90

static muTimer cmdTimer = muTimer();
static muTimer timerTimer = muTimer();
static const char *TAG = "JARO"; // LOG TAG

std::queue<JaroCommand> jaroCmdQueue;

JaroliftController jarolift;

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

/**
 * *******************************************************************
 * @brief   send expected position via mqtt
 * @param   channel, position
 * @return  none
 * *******************************************************************/
void mqttSendPositionGroup(uint16_t group_mask, uint8_t position) {

  // check all 16 channels
  for (uint8_t c = 0; c < 16; c++) {
    // check if channel is in group
    if (group_mask & (1 << c)) {
      mqttSendPosition(c, position);
    }
  }
}

/**
 * *******************************************************************
 * @brief   send remote command via mqtt
 * @param   serial, function, rssi
 * @return  none
 * *******************************************************************/
void mqttSendRemote(uint32_t serial, int8_t function, uint8_t rssi) {
  ESP_LOGI(TAG, "received remote signal | serial: 0x%08lx | function: 0x%x, | rssi: %i", serial, function, rssi);

  if (!mqttIsConnected()) {
    return;
  }

  char topic[64];
  char fun[8];
  uint8_t channel = serial & 0xFF;

  switch (function) {
  case 0x2:
    snprintf(fun, sizeof(fun), "DOWN");
    break;
  case 0x3:
    snprintf(fun, sizeof(fun), "SHADE");
    break;
  case 0x4:
    snprintf(fun, sizeof(fun), "STOP");
    break;
  case 0x8:
    snprintf(fun, sizeof(fun), "UP");
    break;
  default:
    snprintf(fun, sizeof(fun), "0x%x", function);
    break;
  }

  // unknown as default
  const char *remoteName = "unknown";

  for (int i = 0; i < 16; i++) {
    if (config.jaro.remote_enable[i] && (serial >> 8 == config.jaro.remote_serial[i])) {
      remoteName = config.jaro.remote_name[i];

      // check if this remote is registered for one or more shutter
      for (int j = 0; j < 16; j++) {
        if (config.jaro.remote_mask[i] & (1 << j)) {
          switch (function) {
          case 0x2:
            mqttSendPosition(j, POS_CLOSE);
            break;
          case 0x8:
            mqttSendPosition(j, POS_OPEN);
            break;
          case 0x3:
            mqttSendPosition(j, POS_SHADE);
            break;
          default:
            break;
          }
        }
      }
      break; // stop is first remote was found
    }
  }

  JsonDocument remoteJSON;
  remoteJSON["name"] = remoteName;
  remoteJSON["channel"] = channel;
  remoteJSON["comand"] = fun;
  remoteJSON["rssi"] = rssi;

  char sendremoteJSON[255];
  serializeJson(remoteJSON, sendremoteJSON);
  snprintf(topic, sizeof(topic), "%s%08lx", addTopic("/status/remote/"), serial);

  mqttPublish(topic, sendremoteJSON, false);
}

/**
 * *******************************************************************
 * @brief   add jarolift command to buffer
 * @param   type, channel
 * @return  none
 * *******************************************************************/
void jaroCmd(JaroCmdType type, uint8_t channel) {
  if (jaroCmdQueue.size() < MAX_CMD) {
    jaroCmdQueue.push({JaroCommand::SINGLE, {.single = {type, channel}}});
    ESP_LOGD(TAG, "add single cmd to buffer: %i, %i", type, channel + 1);
  } else {
    ESP_LOGE(TAG, "too many commands within too short time");
  }
}

void jaroCmd(JaroCmdGrpType type, uint16_t group_mask) {
  if (jaroCmdQueue.size() < MAX_CMD) {
    jaroCmdQueue.push({JaroCommand::GROUP, {.group = {type, group_mask}}});
    ESP_LOGD(TAG, "add group cmd to buffer: %i, %04X", type, group_mask);
  } else {
    ESP_LOGE(TAG, "too many commands within too short time");
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
  ESP_LOGI(TAG, "initializing the CC1101 Transceiver");
  jarolift.setGPIO(config.gpio.sck, config.gpio.miso, config.gpio.mosi, config.gpio.cs, config.gpio.gdo0, config.gpio.gdo2);
  jarolift.setKeys(config.jaro.masterMSB, config.jaro.masterLSB);
  jarolift.setBaseSerial(config.jaro.serial);
  jarolift.begin();

  if (jarolift.getCC1101State()) {
    ESP_LOGI(TAG, "CC1101 Transceiver connected!");
  } else {
    ESP_LOGE(TAG, "CC1101 Transceiver NOT connected!");
  }

  ESP_LOGI(TAG, "read Device Counter from FLASH: %i", jarolift.getDeviceCounter());

  jarolift.setRemoteCallback(mqttSendRemote);
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

    if (cmd.cmdType == JaroCommand::SINGLE) {
      switch (cmd.single.type) {
      case CMD_UP:
        jarolift.cmdUp(cmd.single.channel);
        mqttSendPosition(cmd.single.channel, POS_OPEN);
        ESP_LOGI(TAG, "execute cmd: UP - channel: %i", cmd.single.channel + 1);
        break;
      case CMD_DOWN:
        jarolift.cmdDown(cmd.single.channel);
        mqttSendPosition(cmd.single.channel, POS_CLOSE);
        ESP_LOGI(TAG, "execute cmd: DOWN - channel: %i", cmd.single.channel + 1);
        break;
      case CMD_STOP:
        jarolift.cmdStop(cmd.single.channel);
        ESP_LOGI(TAG, "execute cmd: STOP - channel: %i", cmd.single.channel + 1);
        break;
      case CMD_SET_SHADE:
        jarolift.cmdSetShade(cmd.single.channel);
        mqttSendPosition(cmd.single.channel, POS_SHADE);
        ESP_LOGI(TAG, "execute cmd: SETSHADE - channel: %i", cmd.single.channel + 1);
        break;
      case CMD_SHADE:
        jarolift.cmdShade(cmd.single.channel);
        ESP_LOGI(TAG, "execute cmd: SHADE - channel: %i", cmd.single.channel + 1);
        break;
      case CMD_LEARN:
        jarolift.cmdLearn(cmd.single.channel);
        ESP_LOGI(TAG, "execute cmd: LEARN - channel: %i", cmd.single.channel + 1);
        break;
      }
    } else if (cmd.cmdType == JaroCommand::GROUP) {
      switch (cmd.group.type) {
      case CMD_GRP_UP:
        jarolift.cmdGroupUp(cmd.group.group_mask);
        ESP_LOGI(TAG, "execute group cmd: UP - mask: %04X", cmd.group.group_mask);
        mqttSendPositionGroup(cmd.group.group_mask, POS_OPEN);
        break;
      case CMD_GRP_DOWN:
        jarolift.cmdGroupDown(cmd.group.group_mask);
        ESP_LOGI(TAG, "execute group cmd: DOWN - mask: %04X", cmd.group.group_mask);
        mqttSendPositionGroup(cmd.group.group_mask, POS_CLOSE);
        break;
      case CMD_GRP_STOP:
        jarolift.cmdGroupStop(cmd.group.group_mask);
        ESP_LOGI(TAG, "execute group cmd: STOP - mask: %04X", cmd.group.group_mask);
        break;
      case CMD_GRP_SHADE:
        jarolift.cmdGroupShade(cmd.group.group_mask);
        mqttSendPositionGroup(cmd.group.group_mask, POS_SHADE);
        ESP_LOGI(TAG, "execute group cmd: SHADE - mask: %04X", cmd.group.group_mask);
        break;
      }
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

  if (timerTimer.cycleTrigger(10000)) {
    timerCyclic();
  }

  jarolift.loop();
}
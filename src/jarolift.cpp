#include <JaroliftController.h>
#include <basics.h>
#include <config.h>
#include <jarolift.h>
#include <mqtt.h>
#include <queue>
#include <timer.h>
#include <shutterPosition.h>
#include <remoteMapping.h>

#define MAX_CMD 20
#define SEND_CYCLE 500

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
void mqttSendPosition(uint8_t channel, int8_t position) {
  char topic[64];
  char pos[16];

  if(channel>=16 || position>100)return;
  if (mqttIsConnected()) {
    if(position<0) snprintf(pos,sizeof(pos),"None");
    else itoa(position, pos, 10);
    snprintf(topic, sizeof(topic), "%s%d", addTopic("/status/shutter/"), channel + 1);
    mqttPublish(topic, pos, true);
  }
}

/**
 * *******************************************************************
 * @brief   send remote command via mqtt
 * @param   serial, function, rssi
 * @return  none
 * *******************************************************************/
void mqttSendRemote(uint32_t serial, int8_t function, uint16_t channel) {
  // Remote commands affect positioning even while MQTT is disabled/offline.
  const auto *mapping=remoteMappingReceive(serial,channel,function);
  uint16_t affected=mapping?mapping->targets:0;
  for(uint8_t ch=0;ch<16;++ch)if(!config.jaro.ch_enable[ch])affected&=~(uint16_t(1)<<ch);
  if (affected) {
    switch(function) {
    case 0x2: shutterPositionRemote(affected,CMD_DOWN); break;
    case 0x8: shutterPositionRemote(affected,CMD_UP); break;
    case 0x4: shutterPositionRemote(affected,CMD_STOP); break;
    case 0x3: shutterPositionRemote(affected,CMD_SHADE); break;
    default: break;
    }
  }

  char topic[64];
  char fun[8];
  char chBIN[18];
  int pos = 0;
  for (int i = 15; i >= 0; i--) {
    chBIN[pos++] = ((channel >> i) & 1) ? '1' : '0';
    if (i == 8) {
      chBIN[pos++] = ' ';
    }
  }
  chBIN[pos] = '\0';

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

  const char *remoteName=mapping?mapping->name:"unknown";

  JsonDocument remoteJSON;
  remoteJSON["name"] = remoteName;
  remoteJSON["cmd"] = fun;
  remoteJSON["chBin"] = chBIN;
  remoteJSON["chDec"] = channel;

  char sendremoteJSON[255];
  serializeJson(remoteJSON, sendremoteJSON);
  snprintf(topic, sizeof(topic), "%s%08lx", addTopic("/status/remote/"), serial);

  if(mqttIsConnected()) mqttPublish(topic, sendremoteJSON, false);

  ESP_LOGI(TAG, "received remote signal | serial: 0x%08lx | cmd: %s, | channel: %s", serial, fun, chBIN);
}

/**
 * *******************************************************************
 * @brief   add jarolift command to buffer
 * @param   type, channel
 * @return  none
 * *******************************************************************/
void jaroCmd(JaroCmdType type, uint8_t channel) {
  if (channel >= 16) return;
  if (shutterPositionBusy()) shutterPositionManual(uint16_t(1) << channel);
  if (jaroCmdQueue.size() < MAX_CMD) {
    jaroCmdQueue.push({JaroCommand::SINGLE, {.single = {type, channel}}});
    ESP_LOGD(TAG, "add single cmd to buffer: %i, %i", type, channel + 1);
  } else {
    ESP_LOGE(TAG, "too many commands within too short time");
  }
}

void jaroCmd(JaroCmdGrpType type, uint16_t group_mask) {
  if (shutterPositionBusy()) shutterPositionManual(group_mask);
  if (jaroCmdQueue.size() < MAX_CMD) {
    jaroCmdQueue.push({JaroCommand::GROUP, {.group = {type, group_mask}}});
    ESP_LOGD(TAG, "add group cmd to buffer: %i, %04X", type, group_mask);
  } else {
    ESP_LOGE(TAG, "too many commands within too short time");
  }
}

void jaroCmd(JaroCmdSrvType type, uint8_t channel) {
  if (channel >= 16) return;
  if (shutterPositionBusy()) shutterPositionManual(uint16_t(1) << channel);
  if (jaroCmdQueue.size() < MAX_CMD) {
    jaroCmdQueue.push({JaroCommand::SERVICE, {.service = {type, channel}}});
    ESP_LOGD(TAG, "add service cmd to buffer: %i, %i", type, channel + 1);
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
  jarolift.setLegacyLearnMode(!config.jaro.learn_mode);
  jarolift.begin();

  if (jarolift.getCC1101State()) {
    ESP_LOGI(TAG, "CC1101 Transceiver connected!");
  } else {
    ESP_LOGE(TAG, "CC1101 Transceiver NOT connected!");
  }

  ESP_LOGI(TAG, "read Device Counter from FLASH: %i", jarolift.getDeviceCounter());

  jarolift.setRemoteCallback(mqttSendRemote);
}

void jaroCmdReInit() { shutterPositionManual(0xffff); jaroliftSetup(); };
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
    if ((cmd.cmdType == JaroCommand::SINGLE && cmd.single.channel >= 16) ||
        (cmd.cmdType == JaroCommand::SERVICE && cmd.service.channel >= 16)) {jaroCmdQueue.pop(); return;}
    uint16_t mask = cmd.cmdType == JaroCommand::GROUP ? cmd.group.group_mask : uint16_t(1) <<
        (cmd.cmdType == JaroCommand::SERVICE ? cmd.service.channel : cmd.single.channel);
    shutterPositionManual(mask);
    if (cmd.cmdType == JaroCommand::SERVICE) shutterPositionInvalidate(cmd.service.channel);

    if (cmd.cmdType == JaroCommand::SINGLE) {
      switch (cmd.single.type) {
      case CMD_UP:
        jarolift.cmdChannel(JaroliftController::CMD_UP, cmd.single.channel);
        ESP_LOGI(TAG, "execute cmd: UP - channel: %i", cmd.single.channel + 1);
        break;
      case CMD_DOWN:
        jarolift.cmdChannel(JaroliftController::CMD_DOWN, cmd.single.channel);
        ESP_LOGI(TAG, "execute cmd: DOWN - channel: %i", cmd.single.channel + 1);
        break;
      case CMD_STOP:
        jarolift.cmdChannel(JaroliftController::CMD_STOP, cmd.single.channel);
        ESP_LOGI(TAG, "execute cmd: STOP - channel: %i", cmd.single.channel + 1);
        break;
      case CMD_SET_SHADE:
        jarolift.cmdChannel(JaroliftController::CMD_SET_SHADE, cmd.single.channel);
        ESP_LOGI(TAG, "execute cmd: SETSHADE - channel: %i", cmd.single.channel + 1);
        break;
      case CMD_SHADE:
        jarolift.cmdChannel(JaroliftController::CMD_SHADE, cmd.single.channel);
        ESP_LOGI(TAG, "execute cmd: SHADE - channel: %i", cmd.single.channel + 1);
        break;
      }
      if(jarolift.lastTxOK()) shutterPositionCommand(mask,cmd.single.type);
    } else if (cmd.cmdType == JaroCommand::GROUP) {
      switch (cmd.group.type) {
      case CMD_GRP_UP:
        jarolift.cmdGroup(JaroliftController::CMD_UP, cmd.group.group_mask);
        ESP_LOGI(TAG, "execute group cmd: UP - mask: %04X", cmd.group.group_mask);
        break;
      case CMD_GRP_DOWN:
        jarolift.cmdGroup(JaroliftController::CMD_DOWN, cmd.group.group_mask);
        ESP_LOGI(TAG, "execute group cmd: DOWN - mask: %04X", cmd.group.group_mask);
        break;
      case CMD_GRP_STOP:
        jarolift.cmdGroup(JaroliftController::CMD_STOP, cmd.group.group_mask);
        ESP_LOGI(TAG, "execute group cmd: STOP - mask: %04X", cmd.group.group_mask);
        break;
      case CMD_GRP_SHADE:
        jarolift.cmdGroup(JaroliftController::CMD_SHADE, cmd.group.group_mask);
        ESP_LOGI(TAG, "execute group cmd: SHADE - mask: %04X", cmd.group.group_mask);
        break;
      }
      if(jarolift.lastTxOK()) {
        const JaroCmdType type=cmd.group.type==CMD_GRP_UP?CMD_UP:cmd.group.type==CMD_GRP_DOWN?CMD_DOWN:cmd.group.type==CMD_GRP_STOP?CMD_STOP:CMD_SHADE;
        shutterPositionCommand(mask,type);
      }
    } else if (cmd.cmdType == JaroCommand::SERVICE) {
      switch (cmd.service.type) {
      case CMD_LEARN:
        jarolift.cmdLearn(cmd.service.channel);
        ESP_LOGI(TAG, "execute service cmd: CMD_LEARN - channel: %i", cmd.single.channel + 1);
        break;
      case CMD_UNLEARN:
        jarolift.cmdUnlearn(cmd.service.channel);
        ESP_LOGI(TAG, "execute service cmd: CMD_UNLEARN - channel: %i", cmd.single.channel + 1);
        break;
      case CMD_SET_END_POINT_UP:
        jarolift.cmdSetEndPointUp(cmd.service.channel);
        ESP_LOGI(TAG, "execute service cmd: SET_END_POINT_UP - channel: %i", cmd.single.channel + 1);
        break;
      case CMD_DEL_END_POINT_UP:
        jarolift.cmdDeleteEndPointUp(cmd.service.channel);
        ESP_LOGI(TAG, "execute service cmd: CMD_DEL_END_POINT_UP - channel: %i", cmd.single.channel + 1);
        break;
      case CMD_SET_END_POINT_DOWN:
        jarolift.cmdSetEndPointDown(cmd.service.channel);
        ESP_LOGI(TAG, "execute service cmd: SET_END_POINT_DOWN - channel: %i", cmd.single.channel + 1);
        break;
      case CMD_DEL_END_POINT_DOWN:
        jarolift.cmdDeleteEndPointDown(cmd.service.channel);
        ESP_LOGI(TAG, "execute service cmd: CMD_DEL_END_POINT_DOWN - channel: %i", cmd.single.channel + 1);
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
  remoteMappingCyclic();
  shutterPositionCyclic();

  if (cmdTimer.cycleTrigger(SEND_CYCLE)) {
    processJaroCommands();
  }

  if (timerTimer.cycleTrigger(10000)) {
    timerCyclic();
  }

  jarolift.loop();
}
bool jaroCommandsPending() { return !jaroCmdQueue.empty(); }
uint32_t jaroPositionSentAt() { return jarolift.lastTxMs(); }
bool jaroPositionSend(uint8_t channel, JaroCmdType type) {
  if (channel >= 16 || !jarolift.ready()) return false;
  switch(type) {
  case CMD_UP: jarolift.cmdChannel(JaroliftController::CMD_UP, channel); break;
  case CMD_DOWN: jarolift.cmdChannel(JaroliftController::CMD_DOWN, channel); break;
  case CMD_STOP: jarolift.cmdChannel(JaroliftController::CMD_STOP, channel); break;
  default: return false;
  }
  return jarolift.lastTxOK();
}

#include <mqtt.h>
#include <mqttCustom.h>
#include <basics.h>
#include <WiFiMgn.h>
#include <language.h>
#include <message.h>
#include <mqttDiscovery.h>
#include <jarolift.h>
#include <shutterPosition.h>

static const char *TAG = "MQTT";
static bool bootUpMsgDone = false;
static char haStatusTopic[128];
static uint16_t discoveryPositionMask=0;
static bool discoveryPending=true;

void mqttCustomConnected() {
  discoveryPending=true;
  sendWiFiInfo();
  shutterPositionPublish();
  mqttSubscribe(addTopic("/cmd/#"));
  mqttSubscribe(addTopic("/setvalue/#"));
  const auto &settings = mqttGetConnectionConfig();
  snprintf(haStatusTopic, sizeof(haStatusTopic), "%s/status", settings.ha_topic);
  if (settings.ha_enable) {
    mqttSubscribe(haStatusTopic);
  }
}

void mqttCustomCyclic() {
  if(mqttIsConnected() && config.mqtt.ha_enable && !EspSysUtil::OTA::getInstance().isActive()) {
    uint16_t mask=0;
    for(uint8_t ch=0;ch<16;++ch) if(shutterPositionReady(ch)) mask|=uint16_t(1)<<ch;
    if(discoveryPending || mask!=discoveryPositionMask) {
      mqttDiscoverySetup(false);
      shutterPositionPublish();
      discoveryPositionMask=mask;discoveryPending=false;
    }
  } else discoveryPending=true;

  // send bootup messages after restart and established mqtt connection
  if (!bootUpMsgDone && mqttIsConnected()) {
    bootUpMsgDone = true;
    ESP_LOGI(TAG, "ESP restarted (%s)", EspSysUtil::RestartReason::get());

  }
}

int checkJaroCmd(const char *topicCopy, const char *cmpTopic, int maxChannel) {

  size_t cmpTopicLen = strlen(cmpTopic);

  if (strncmp(topicCopy, cmpTopic, cmpTopicLen) == 0) {
    const char *suffix = topicCopy + cmpTopicLen;
    char *endPtr;
    int channel = strtol(suffix, &endPtr, 10);

    if (*endPtr == '\0' && channel >= 1 && channel <= maxChannel) {
      return channel;
    }
  }
  return -1;
}

uint16_t parseMask(const char *payload) {
  char buffer[64];
  int idx = 0;

  // 1) Remove whitespaces and convert to lowercase
  while (*payload != '\0' && idx < (int)(sizeof(buffer) - 1)) {
    if (!isspace((unsigned char)*payload)) {
      buffer[idx++] = (char)tolower((unsigned char)*payload);
    }
    payload++;
  }
  buffer[idx] = '\0'; // null termination for the new string

  // 2) Detect prefix
  // "0x" => hexadecimal
  if (strncmp(buffer, "0x", 2) == 0) {
    // strtol parses the substring after "0x" in base 16
    return (uint16_t)strtol(buffer + 2, NULL, 16);
  }
  // "0b" => binary
  else if (strncmp(buffer, "0b", 2) == 0) {
    return (uint16_t)strtol(buffer + 2, NULL, 2);
  }
  // otherwise => decimal
  else {
    return (uint16_t)strtol(buffer, NULL, 10);
  }
}

void processMqttMessage(const s_MqttMessage &message) {

  const s_MqttMessage &msgCpy = message;

  ESP_LOGD(TAG, "process msg from buffer: %s, %s", msgCpy.topic, msgCpy.payload);

  // payload as number
  // msgCpy.intVal = 0;
  // msgCpy.floatVal = 0.0;
  // if (len > 0) {
  //   msgCpy.intVal = atoi(msgCpy.payload);
  //   msgCpy.floatVal = atoff(msgCpy.payload);
  // }

  const char *shutterTopic = addTopic("/cmd/shutter/");
  int channel = checkJaroCmd(msgCpy.topic, shutterTopic, 16);
  const char *groupTopic = addTopic("/cmd/group/");
  int group = checkJaroCmd(msgCpy.topic, groupTopic, 6);
  const int positionChannel=checkJaroCmd(msgCpy.topic,addTopic("/cmd/position/"),16);

  ESP_LOGD(TAG, "channel: %i", channel);
  ESP_LOGD(TAG, "group: %i", group);

  // restart ESP command
  if (strcasecmp(msgCpy.topic, addTopic("/cmd/restart")) == 0) {
    EspSysUtil::RestartReason::saveLocal("mqtt command");
    yield();
    delay(1000);
    yield();
    ESP.restart();
    // reconfigure
  } else if (strcasecmp(msgCpy.topic, addTopic("/cmd/reconfigure")) == 0) {
    mqttDiscoverySetup(true);
    yield();
    delay(1000);
    yield();
    mqttDiscoverySetup(false);
    // homeassistant/status
  } else if (strcmp(msgCpy.topic, haStatusTopic) == 0) {
    if (config.mqtt.ha_enable && strcmp(msgCpy.payload, "online") == 0) {
      mqttDiscoverySetup(false); // send actual discovery configuration
    }
  } else if(positionChannel!=-1) {
    if(!shutterPositionTarget(positionChannel-1,msgCpy.payload)) {
      mqttPublish(addTopic("/message"),"invalid or unavailable position target",false);
      ESP_LOGW(TAG,"Rejected position target for channel %i",positionChannel);
    }
    // Shutter commands
  } else if (channel != -1) {
    if (channel >= 1 && channel <= 16) {
      if (strcasecmp(msgCpy.payload, "UP") == 0 || strcasecmp(msgCpy.payload, "OPEN") == 0 || strcmp(msgCpy.payload, "0") == 0) {
        jaroCmd(CMD_UP, channel - 1);
      } else if (strcasecmp(msgCpy.payload, "DOWN") == 0 || strcasecmp(msgCpy.payload, "CLOSE") == 0 || strcmp(msgCpy.payload, "1") == 0) {
        jaroCmd(CMD_DOWN, channel - 1);
      } else if (strcasecmp(msgCpy.payload, "STOP") == 0 || strcmp(msgCpy.payload, "2") == 0) {
        jaroCmd(CMD_STOP, channel - 1);
      } else if (strcasecmp(msgCpy.payload, "SHADE") == 0 || strcmp(msgCpy.payload, "3") == 0) {
        jaroCmd(CMD_SHADE, channel - 1);
      } else if (strcasecmp(msgCpy.payload, "SETSHADE") == 0 || strcmp(msgCpy.payload, "4") == 0) {
        jaroCmd(CMD_SET_SHADE, channel - 1);
      } else {
        mqttPublish(addTopic("/message"), "invalid shutter cmd", false);
        ESP_LOGW(TAG, "invalid shutter cmd");
      }
    } else {
      mqttPublish(addTopic("/message"), "invalid channel", false);
      ESP_LOGW(TAG, "invalid channel for shutter cmd");
    }
    // Group commands
  } else if (group != -1) {
    if (group >= 1 && group <= 6) {
      if (strcasecmp(msgCpy.payload, "UP") == 0 || strcasecmp(msgCpy.payload, "OPEN") == 0 || strcmp(msgCpy.payload, "0") == 0) {
        jaroCmd(CMD_GRP_UP, config.jaro.grp_mask[group - 1]);
      } else if (strcasecmp(msgCpy.payload, "DOWN") == 0 || strcasecmp(msgCpy.payload, "CLOSE") == 0 || strcmp(msgCpy.payload, "1") == 0) {
        jaroCmd(CMD_GRP_DOWN, config.jaro.grp_mask[group - 1]);
      } else if (strcasecmp(msgCpy.payload, "STOP") == 0 || strcmp(msgCpy.payload, "2") == 0) {
        jaroCmd(CMD_GRP_STOP, config.jaro.grp_mask[group - 1]);
      } else if (strcasecmp(msgCpy.payload, "SHADE") == 0 || strcmp(msgCpy.payload, "3") == 0) {
        jaroCmd(CMD_GRP_SHADE, config.jaro.grp_mask[group - 1]);
      } else {
        mqttPublish(addTopic("/message"), "invalid group cmd", false);
        ESP_LOGW(TAG, "invalid group cmd");
      }
    } else {
      mqttPublish(addTopic("/message"), "invalid group", false);
      ESP_LOGW(TAG, "invalid channel for group cmd");
    }
    // Group commands with bitmask
  } else if (strcasecmp(msgCpy.topic, addTopic("/cmd/group/up")) == 0) {
    jaroCmd(CMD_GRP_UP, parseMask(msgCpy.payload));
  } else if (strcasecmp(msgCpy.topic, addTopic("/cmd/group/down")) == 0) {
    jaroCmd(CMD_GRP_DOWN, parseMask(msgCpy.payload));
  } else if (strcasecmp(msgCpy.topic, addTopic("/cmd/group/stop")) == 0) {
    jaroCmd(CMD_GRP_STOP, parseMask(msgCpy.payload));
  } else if (strcasecmp(msgCpy.topic, addTopic("/cmd/group/shade")) == 0) {
    jaroCmd(CMD_GRP_SHADE, parseMask(msgCpy.payload));
  } else {
    mqttPublish(addTopic("/message"), "unknown topic", false);
    ESP_LOGI(TAG, "unknown topic received");
  }


}

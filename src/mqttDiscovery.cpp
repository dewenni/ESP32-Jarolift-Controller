#include <basics.h>
#include <language.h>
#include <message.h>
#include <mqtt.h>
#include <mqttDiscovery.h>

/* D E C L A R A T I O N S ****************************************************/
char discoveryPrefix[128];
char deviceName[32];
char statePrefix[128];
char deviceId[32];
char swVersion[32];

static bool resetMqttConfig = false;

enum DeviceType { DEV_TEXT, DEV_BTN, DEV_SHUTTER };
enum statType { TYP_STATUS, TYP_INFO, TYP_WIFI, TYP_ETH, TYP_SYSINFO, TYP_CMD_BTN, TYP_SHUTTER };
enum ValTmpType { VAL_SPLIT };

struct DeviceConfig {
  int num1;
  char *char1;
};

DeviceConfig nullPar() { return (DeviceConfig){0, 0}; }
DeviceConfig shutterPar(int channel, char *name) { return (DeviceConfig){channel, name}; }

/**
 * *******************************************************************
 * @brief   helper function to generate value template
 * @param   type string
 * @return  value template based on type
 * *******************************************************************/
const char *valueTmpl(ValTmpType type) {
  static char output[128];
  switch (type) {
  case VAL_SPLIT:
    snprintf(output, sizeof(output), "{{value.split(' ')[0]}}");
    break;

  default:
    break;
  }
  return output;
}

/**
 * *******************************************************************
 * @brief   generate mqtt messages for home assistant auto discovery
 * @param   kmType
 * @param   name
 * @param   deviceClass
 * @param   component
 * @param   unit
 * @param   valueTemplate
 * @param   icon
 * @param   devType
 * @param   devCfg
 * @return  none
 * *******************************************************************/
void mqttHaConfig(statType statType, const char *name, const char *deviceClass, const char *component, const char *unit, const char *valueTemplate,
                  const char *icon, DeviceType devType, DeviceConfig devCfg) {

  if (strlen(discoveryPrefix) == 0 || strlen(statePrefix) == 0 || strlen(name) == 0) {
    return;
  }

  JsonDocument doc;
  char cmdTopic[256];
  char configTopic[256];

  sprintf(configTopic, "%s/%s/%s/%s/config", discoveryPrefix, component, deviceId, name);

  char stateTopic[256];
  switch (statType) {

  case TYP_SHUTTER:
    sprintf(stateTopic, "%s/status/shutter/%i", statePrefix, devCfg.num1);
    doc["stat_t"] = stateTopic;
    break;

  case TYP_STATUS:
    sprintf(stateTopic, "%s/status/%s", statePrefix, name);
    doc["stat_t"] = stateTopic;
    break;

  case TYP_WIFI:
    sprintf(stateTopic, "%s/wifi", statePrefix);
    doc["stat_t"] = stateTopic;
    break;
  case TYP_ETH:
    sprintf(stateTopic, "%s/eth", statePrefix);
    doc["stat_t"] = stateTopic;
    break;

  case TYP_SYSINFO:
    sprintf(stateTopic, "%s/sysinfo", statePrefix);
    doc["stat_t"] = stateTopic;
    break;
  default:
    break;
  }

  if (devType == DEV_SHUTTER) {
    doc["name"] = devCfg.char1;
  } else {
    char friendlyName[64];
    replace_underscores(name, friendlyName, sizeof(friendlyName));
    doc["name"] = friendlyName;
  }

  char uniq_id[128];
  sprintf(uniq_id, "%s_%s", deviceName, name);
  doc["uniq_id"] = uniq_id;

  if (deviceClass) {
    doc["dev_cla"] = deviceClass;
  }
  if (unit) {
    doc["unit_of_meas"] = unit;
  }
  if (valueTemplate) {
    doc["val_tpl"] = valueTemplate;
  }
  if (icon) {
    doc["icon"] = icon;
  }

  if (devType == DEV_SHUTTER) {
    doc["pl_open"] = "OPEN";
    doc["pl_cls"] = "CLOSE";
    doc["pl_stop"] = "STOP";
    doc["state_open"] = "0";
    doc["state_closed"] = "100";

    sprintf(cmdTopic, "%s/cmd/shutter/%i", statePrefix, devCfg.num1);
    doc["cmd_t"] = cmdTopic;

  } else if (devType == DEV_BTN) {
    sprintf(cmdTopic, "%s/cmd/%s", statePrefix, name);
    doc["cmd_t"] = cmdTopic;
    doc["payload_press"] = "true";
    doc["ent_cat"] = "config";
  } else if (devType != DEV_TEXT) {
    sprintf(cmdTopic, "%s/cmd/%s", statePrefix, name);
    doc["cmd_t"] = cmdTopic;
  }

  if (statType == TYP_WIFI || statType == TYP_ETH || statType == TYP_SYSINFO) {
    doc["ent_cat"] = "diagnostic";
  }

  char willTopic[256];
  snprintf(willTopic, sizeof(willTopic), "%s/status", statePrefix);
  doc["avty_t"] = willTopic;

  // device
  JsonObject deviceObj = doc["dev"].to<JsonObject>();
  deviceObj["name"] = deviceName;
  JsonArray idsArray = deviceObj["ids"].to<JsonArray>();
  idsArray.add(deviceId);
  deviceObj["mf"] = "Jarolift";
  deviceObj["mdl"] = "MQTT_Controller";
  deviceObj["sw"] = swVersion;

  char jsonString[1024];
  serializeJson(doc, jsonString);

  if (resetMqttConfig && devType == DEV_SHUTTER) {
    mqttPublish(configTopic, "", false);
  } else {
    mqttPublish(configTopic, jsonString, false);
  }
}

/**
 * *******************************************************************
 * @brief   mqttDiscovery Setup function
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttDiscoverySetup(bool reset) {

  resetMqttConfig = reset;

  // copy config values
  snprintf(discoveryPrefix, sizeof(discoveryPrefix), "%s", config.mqtt.ha_topic);
  snprintf(statePrefix, sizeof(statePrefix), "%s", config.mqtt.topic);
  snprintf(deviceName, sizeof(deviceName), "%s", config.mqtt.ha_device);
  snprintf(deviceId, sizeof(deviceId), "%s", config.mqtt.ha_device);
  snprintf(swVersion, sizeof(swVersion), "%s", VERSION);

  // Shutter Control 1..16
  for (int i = 0; i < 16; i++) {
    if (config.jaro.ch_enable[i]) {
      char shutter[32];
      snprintf(shutter, sizeof(shutter), "shutter%d", i + 1);
      mqttHaConfig(TYP_SHUTTER, shutter, "shutter", "cover", NULL, "{{ value | int }}", "mdi:window-shutter", DEV_SHUTTER,
                   shutterPar(i + 1, config.jaro.ch_name[i]));
    }
  }
  // Service Buttons
  mqttHaConfig(TYP_CMD_BTN, "restart", NULL, "button", NULL, NULL, "mdi:restart", DEV_BTN, nullPar());
  mqttHaConfig(TYP_CMD_BTN, "reconfigure", NULL, "button", NULL, NULL, "mdi:cog-sync", DEV_BTN, nullPar());

  // System INFO
  mqttHaConfig(TYP_WIFI, "wifi_signal", NULL, "sensor", "%", "{{ value_json.signal }}", "mdi:signal", DEV_TEXT, nullPar());
  mqttHaConfig(TYP_WIFI, "wifi_rssi", NULL, "sensor", "dbm", "{{ value_json.rssi }}", "mdi:signal", DEV_TEXT, nullPar());
  mqttHaConfig(TYP_WIFI, "wifi_ip", NULL, "sensor", NULL, "{{ value_json.ip }}", "mdi:ip-outline", DEV_TEXT, nullPar());

  if (config.eth.enable) {
    mqttHaConfig(TYP_ETH, "eth_ip", NULL, "sensor", NULL, "{{ value_json.ip }}", "mdi:ip-network", DEV_TEXT, nullPar());
    mqttHaConfig(TYP_ETH, "eth_status", NULL, "sensor", NULL, "{{ value_json.status }}", "mdi:lan", DEV_TEXT, nullPar());
    mqttHaConfig(TYP_ETH, "eth_link_speed", NULL, "sensor", "Mbps", "{{ value_json.link_speed }}", "mdi:lan", DEV_TEXT, nullPar());
    mqttHaConfig(TYP_ETH, "eth_full_duplex", NULL, "sensor", NULL, "{{ value_json.full_duplex }}", "mdi:lan", DEV_TEXT, nullPar());
  }

  mqttHaConfig(TYP_SYSINFO, "restart_reason", NULL, "sensor", NULL, "{{ value_json.restart_reason }}", "mdi:information-outline", DEV_TEXT,
               nullPar());
  mqttHaConfig(TYP_SYSINFO, "heap", NULL, "sensor", "%", "{{ value_json.heap.split(' ')[0] }}", "mdi:memory", DEV_TEXT, nullPar());
  mqttHaConfig(TYP_SYSINFO, "flash", NULL, "sensor", "%", "{{ value_json.flash.split(' ')[0] }}", "mdi:harddisk", DEV_TEXT, nullPar());
  mqttHaConfig(TYP_SYSINFO, "sw_version", NULL, "sensor", NULL, "{{ value_json.sw_version }}", "mdi:github", DEV_TEXT, nullPar());
}

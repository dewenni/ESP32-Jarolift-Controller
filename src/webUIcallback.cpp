
#include <basics.h>
#include <jarolift.h>
#include <message.h>
#include <webUI.h>
#include <webUIupdates.h>

static const char *TAG = "WEB"; // LOG TAG

/**
 * *******************************************************************
 * @brief   callback function for web elements
 * @param   elementID
 * @param   value
 * @return  none
 * *******************************************************************/
void webCallback(const char *elementId, const char *value) {

  MY_LOGD(TAG, "Received - Element ID: %s = %s", elementId, value);

  // ------------------------------------------------------------------
  // GitHub / Version
  // ------------------------------------------------------------------

  // Github Check Version
  if (strcmp(elementId, "check_git_version") == 0) {
    requestGitHubVersion();
  }
  // Github Update
  if (strcmp(elementId, "p00_update_btn") == 0) {
    requestGitHubUpdate();
  }
  // OTA-Confirm
  if (strcmp(elementId, "p00_ota_confirm_btn") == 0) {
    updateWebDialog("ota_update_done_dialog", "close");
    EspSysUtil::RestartReason::saveLocal("ota update");
    yield();
    delay(1000);
    yield();
    ESP.restart();
  }

  // ------------------------------------------------------------------
  // Settings callback
  // ------------------------------------------------------------------

  // WiFi
  if (strcmp(elementId, "cfg_wifi_enable") == 0) {
    config.wifi.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_wifi_hostname") == 0) {
    snprintf(config.wifi.hostname, sizeof(config.wifi.hostname), value);
  }
  if (strcmp(elementId, "cfg_wifi_ssid") == 0) {
    snprintf(config.wifi.ssid, sizeof(config.wifi.ssid), value);
  }
  if (strcmp(elementId, "cfg_wifi_password") == 0) {
    snprintf(config.wifi.password, sizeof(config.wifi.password), value);
  }
  if (strcmp(elementId, "cfg_wifi_static_ip") == 0) {
    config.wifi.static_ip = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_wifi_ipaddress") == 0) {
    snprintf(config.wifi.ipaddress, sizeof(config.wifi.ipaddress), value);
  }
  if (strcmp(elementId, "cfg_wifi_subnet") == 0) {
    snprintf(config.wifi.subnet, sizeof(config.wifi.subnet), value);
  }
  if (strcmp(elementId, "cfg_wifi_gateway") == 0) {
    snprintf(config.wifi.gateway, sizeof(config.wifi.gateway), value);
  }
  if (strcmp(elementId, "cfg_wifi_dns") == 0) {
    snprintf(config.wifi.dns, sizeof(config.wifi.dns), value);
  }

  // Ethernet
  if (strcmp(elementId, "cfg_eth_enable") == 0) {
    config.eth.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_eth_hostname") == 0) {
    snprintf(config.eth.hostname, sizeof(config.eth.hostname), value);
  }
  if (strcmp(elementId, "cfg_eth_gpio_sck") == 0) {
    config.eth.gpio_sck = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_mosi") == 0) {
    config.eth.gpio_mosi = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_miso") == 0) {
    config.eth.gpio_miso = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_cs") == 0) {
    config.eth.gpio_cs = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_irq") == 0) {
    config.eth.gpio_irq = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_rst") == 0) {
    config.eth.gpio_rst = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_static_ip") == 0) {
    config.eth.static_ip = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_eth_ipaddress") == 0) {
    snprintf(config.eth.ipaddress, sizeof(config.eth.ipaddress), value);
  }
  if (strcmp(elementId, "cfg_eth_subnet") == 0) {
    snprintf(config.eth.subnet, sizeof(config.eth.subnet), value);
  }
  if (strcmp(elementId, "cfg_eth_gateway") == 0) {
    snprintf(config.eth.gateway, sizeof(config.eth.gateway), value);
  }
  if (strcmp(elementId, "cfg_eth_dns") == 0) {
    snprintf(config.eth.dns, sizeof(config.eth.dns), value);
  }

  // Authentication
  if (strcmp(elementId, "cfg_auth_enable") == 0) {
    config.auth.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_auth_user") == 0) {
    snprintf(config.auth.user, sizeof(config.auth.user), "%s", value);
  }
  if (strcmp(elementId, "cfg_auth_password") == 0) {
    snprintf(config.auth.password, sizeof(config.auth.password), "%s", value);
  }

  // NTP-Server
  if (strcmp(elementId, "cfg_ntp_enable") == 0) {
    config.ntp.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_ntp_server") == 0) {
    snprintf(config.ntp.server, sizeof(config.ntp.server), "%s", value);
  }
  if (strcmp(elementId, "cfg_ntp_tz") == 0) {
    snprintf(config.ntp.tz, sizeof(config.ntp.tz), "%s", value);
  }

  // GEO-Location
  if (strcmp(elementId, "cfg_geo_latitude") == 0) {
    config.geo.latitude = atof(value);
  }
  if (strcmp(elementId, "cfg_geo_longitude") == 0) {
    config.geo.longitude = atof(value);
  }

  // MQTT
  if (strcmp(elementId, "cfg_mqtt_enable") == 0) {
    config.mqtt.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_mqtt_server") == 0) {
    snprintf(config.mqtt.server, sizeof(config.mqtt.server), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_port") == 0) {
    config.mqtt.port = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_mqtt_topic") == 0) {
    snprintf(config.mqtt.topic, sizeof(config.mqtt.topic), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_user") == 0) {
    snprintf(config.mqtt.user, sizeof(config.mqtt.user), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_password") == 0) {
    snprintf(config.mqtt.password, sizeof(config.mqtt.password), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_ha_enable") == 0) {
    config.mqtt.ha_enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_mqtt_ha_topic") == 0) {
    snprintf(config.mqtt.ha_topic, sizeof(config.mqtt.ha_topic), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_ha_device") == 0) {
    snprintf(config.mqtt.ha_device, sizeof(config.mqtt.ha_device), "%s", value);
  }

  // Hardware
  if (strcmp(elementId, "cfg_gpio_gdo0") == 0) {
    config.gpio.gdo0 = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_gpio_gdo2") == 0) {
    config.gpio.gdo2 = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_gpio_sck") == 0) {
    config.gpio.sck = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_gpio_miso") == 0) {
    config.gpio.miso = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_gpio_mosi") == 0) {
    config.gpio.mosi = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_gpio_cs") == 0) {
    config.gpio.cs = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_gpio_led_setup") == 0) {
    config.gpio.led_setup = strtoul(value, NULL, 10);
  }

  // Jarolift settings
  if (strcmp(elementId, "cfg_jaro_masterMSB") == 0) {
    config.jaro.masterMSB = strtoul(value, NULL, 16);
    jaroCmdReInit();
  }
  if (strcmp(elementId, "cfg_jaro_masterLSB") == 0) {
    config.jaro.masterLSB = strtoul(value, NULL, 16);
    jaroCmdReInit();
  }
  if (strcmp(elementId, "cfg_jaro_serial") == 0) {
    config.jaro.serial = strtoul(value, NULL, 16);
    jaroCmdReInit();
  }
  if (strcmp(elementId, "cfg_jaro_learn_mode") == 0) {
    config.jaro.learn_mode = EspStrUtil::stringToBool(value);
    jaroCmdReInit();
  }
  if (strcmp(elementId, "p12_jaro_devcnt") == 0) {
    jaroCmdSetDevCnt(strtoul(value, NULL, 10));
  }

  // Shutter 1-16
  for (int i = 0; i < 16; i++) {
    char enableId[32];
    char nameId[32];
    char learnId[32];
    char setShadeId[32];
    char cmdUpId[32];
    char cmdDownId[32];
    char cmdStopId[32];
    char cmdShadeId[32];

    snprintf(enableId, sizeof(enableId), "cfg_jaro_ch_enable_%d", i);
    snprintf(nameId, sizeof(nameId), "cfg_jaro_ch_name_%d", i);
    snprintf(setShadeId, sizeof(setShadeId), "p12_setshade_%d", i);
    snprintf(learnId, sizeof(learnId), "p12_learn_%d", i);

    snprintf(cmdUpId, sizeof(cmdUpId), "p01_up_%d", i);
    snprintf(cmdDownId, sizeof(cmdDownId), "p01_down_%d", i);
    snprintf(cmdStopId, sizeof(cmdStopId), "p01_stop_%d", i);
    snprintf(cmdShadeId, sizeof(cmdShadeId), "p01_shade_%d", i);

    if (strcmp(elementId, enableId) == 0) {
      config.jaro.ch_enable[i] = EspStrUtil::stringToBool(value);
    }
    if (strcmp(elementId, nameId) == 0) {
      snprintf(config.jaro.ch_name[i], sizeof(config.jaro.ch_name[i]), "%s", value);
    }
    if (strcmp(elementId, setShadeId) == 0) {
      jaroCmd(CMD_SET_SHADE, i);
      MY_LOGI(TAG, "cmd SET-SHADE - channel %i", i + 1);
    }
    if (strcmp(elementId, learnId) == 0) {
      jaroCmd(CMD_LEARN, i);
      MY_LOGI(TAG, "cmd LEARN - channel %i", i + 1);
    }
    if (strcmp(elementId, cmdUpId) == 0) {
      MY_LOGI(TAG, "cmd UP - channel %i", i + 1);
      jaroCmd(CMD_UP, i);
    }
    if (strcmp(elementId, cmdStopId) == 0) {
      MY_LOGI(TAG, "cmd STOP - channel %i", i + 1);
      jaroCmd(CMD_STOP, i);
    }
    if (strcmp(elementId, cmdDownId) == 0) {
      MY_LOGI(TAG, "cmd DOWN - channel %i", i + 1);
      jaroCmd(CMD_DOWN, i);
    }
    if (strcmp(elementId, cmdShadeId) == 0) {
      MY_LOGI(TAG, "cmd SHADE - channel %i", i + 1);
      jaroCmd(CMD_SHADE, i);
    }
  }

  // group 1-6
  for (int i = 0; i < 6; i++) {
    char enableId[32];
    char nameId[32];
    char maskId[32];
    char cmdUpId[32];
    char cmdDownId[32];
    char cmdStopId[32];
    char cmdShadeId[32];

    snprintf(enableId, sizeof(enableId), "cfg_jaro_grp_enable_%d", i);
    snprintf(nameId, sizeof(nameId), "cfg_jaro_grp_name_%d", i);
    snprintf(maskId, sizeof(maskId), "cfg_jaro_grp_mask_%d", i);

    snprintf(cmdUpId, sizeof(cmdUpId), "p02_up_%d", i);
    snprintf(cmdDownId, sizeof(cmdDownId), "p02_down_%d", i);
    snprintf(cmdStopId, sizeof(cmdStopId), "p02_stop_%d", i);
    snprintf(cmdShadeId, sizeof(cmdShadeId), "p02_shade_%d", i);

    if (strcmp(elementId, enableId) == 0) {
      config.jaro.grp_enable[i] = EspStrUtil::stringToBool(value);
    }
    if (strcmp(elementId, nameId) == 0) {
      snprintf(config.jaro.grp_name[i], sizeof(config.jaro.grp_name[i]), "%s", value);
    }
    if (strcmp(elementId, maskId) == 0) {
      config.jaro.grp_mask[i] = strtoul(value, NULL, 2);
    }
    if (strcmp(elementId, cmdUpId) == 0) {
      MY_LOGI(TAG, "cmd UP - group %i", i + 1);
      jaroCmd(CMD_GRP_UP, config.jaro.grp_mask[i]);
    }
    if (strcmp(elementId, cmdStopId) == 0) {
      MY_LOGI(TAG, "cmd STOP - group %i", i + 1);
      jaroCmd(CMD_GRP_STOP, config.jaro.grp_mask[i]);
    }
    if (strcmp(elementId, cmdDownId) == 0) {
      MY_LOGI(TAG, "cmd DOWN - group %i", i + 1);
      jaroCmd(CMD_GRP_DOWN, config.jaro.grp_mask[i]);
    }
    if (strcmp(elementId, cmdShadeId) == 0) {
      MY_LOGI(TAG, "cmd SHADE - group %i", i + 1);
      jaroCmd(CMD_GRP_SHADE, config.jaro.grp_mask[i]);
    }
  }

  // Timer 1-6
  for (int i = 0; i < 6; i++) {
    char enableId[32];
    char typeId[32];
    char timeValueId[32];
    char offsetValueId[32];
    char cmdId[32];
    char maskId[32];

    char mondayId[32];
    char tuesdayId[32];
    char wednesdayId[32];
    char thursdayId[32];
    char fridayId[32];
    char saturdayId[32];
    char sundayId[32];

    snprintf(enableId, sizeof(enableId), "cfg_timer_%d_enable", i);
    snprintf(typeId, sizeof(typeId), "cfg_timer_%d_type", i);
    snprintf(timeValueId, sizeof(timeValueId), "cfg_timer_%d_time_value", i);
    snprintf(offsetValueId, sizeof(offsetValueId), "cfg_timer_%d_offset_value", i);
    snprintf(cmdId, sizeof(cmdId), "cfg_timer_%d_cmd", i);
    snprintf(maskId, sizeof(maskId), "cfg_timer_%d_grp_mask", i);

    snprintf(mondayId, sizeof(mondayId), "cfg_timer_%d_monday", i);
    snprintf(tuesdayId, sizeof(tuesdayId), "cfg_timer_%d_tuesday", i);
    snprintf(wednesdayId, sizeof(wednesdayId), "cfg_timer_%d_wednesday", i);
    snprintf(thursdayId, sizeof(thursdayId), "cfg_timer_%d_thursday", i);
    snprintf(fridayId, sizeof(fridayId), "cfg_timer_%d_friday", i);
    snprintf(saturdayId, sizeof(saturdayId), "cfg_timer_%d_saturday", i);
    snprintf(sundayId, sizeof(sundayId), "cfg_timer_%d_sunday", i);

    if (strcmp(elementId, enableId) == 0) {
      config.timer[i].enable = EspStrUtil::stringToBool(value);
    }
    if (strcmp(elementId, typeId) == 0) {
      config.timer[i].type = atoi(value);
    }
    if (strcmp(elementId, timeValueId) == 0) {
      snprintf(config.timer[i].time_value, sizeof(config.timer[i].time_value), "%s", value);
    }
    if (strcmp(elementId, offsetValueId) == 0) {
      config.timer[i].offset_value = atoi(value);
    }
    if (strcmp(elementId, cmdId) == 0) {
      config.timer[i].cmd = atoi(value);
    }
    if (strcmp(elementId, maskId) == 0) {
      config.timer[i].grp_mask = strtoul(value, NULL, 2);
    }

    if (strcmp(elementId, mondayId) == 0) {
      config.timer[i].monday = EspStrUtil::stringToBool(value);
    }
    if (strcmp(elementId, tuesdayId) == 0) {
      config.timer[i].tuesday = EspStrUtil::stringToBool(value);
    }
    if (strcmp(elementId, wednesdayId) == 0) {
      config.timer[i].wednesday = EspStrUtil::stringToBool(value);
    }
    if (strcmp(elementId, thursdayId) == 0) {
      config.timer[i].thursday = EspStrUtil::stringToBool(value);
    }
    if (strcmp(elementId, fridayId) == 0) {
      config.timer[i].friday = EspStrUtil::stringToBool(value);
    }
    if (strcmp(elementId, saturdayId) == 0) {
      config.timer[i].saturday = EspStrUtil::stringToBool(value);
    }
    if (strcmp(elementId, sundayId) == 0) {
      config.timer[i].sunday = EspStrUtil::stringToBool(value);
    }
  }

  // Language
  if (strcmp(elementId, "cfg_lang") == 0) {
    config.lang = strtoul(value, NULL, 10);
    updateAllElements();
  }

  // Buttons
  if (strcmp(elementId, "p12_btn_restart") == 0) {
    EspSysUtil::RestartReason::saveLocal("webUI command");
    yield();
    delay(1000);
    yield();
    ESP.restart();
  }

  // Logger
  if (strcmp(elementId, "cfg_logger_enable") == 0) {
    config.log.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_logger_level") == 0) {
    config.log.level = strtoul(value, NULL, 10);
    setLogLevel(config.log.level);
    clearLogBuffer();
    updateWebLog("", "clr_log"); // clear log
  }
  if (strcmp(elementId, "cfg_logger_order") == 0) {
    config.log.order = strtoul(value, NULL, 10);
    updateWebLog("", "clr_log"); // clear log
    webReadLogBuffer();
  }
  if (strcmp(elementId, "p10_log_clr_btn") == 0) {
    clearLogBuffer();
    updateWebLog("", "clr_log"); // clear log
  }
  if (strcmp(elementId, "p10_log_refresh_btn") == 0) {
    webReadLogBuffer();
  }
}

#include <basics.h>
#include <jarolift.h>
#include <message.h>
#include <webUI.h>
#include <webUIupdates.h>

static tm dti;
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

  // set manual date for Logamatic
  if (strcmp(elementId, "p12_dti_date") == 0) {
    char tmp1[12] = {'\0'};
    char tmp2[12] = {'\0'};
    /* ---------------- INFO ---------------------------------
    dti.tm_year + 1900  // years since 1900
    dti.tm_mon + 1      // January = 0 (!)
    dti.tm_mday         // day of month
    dti.tm_hour         // hours since midnight  0-23
    dti.tm_min          // minutes after the hour  0-59
    dti.tm_sec          // seconds after the minute  0-61*
    dti.tm_wday         // days since Sunday 0-6
    dti.tm_isdst        // Daylight Saving Time flag
    --------------------------------------------------------- */
    // get date
    strncpy(tmp1, value, sizeof(tmp1));
    // extract year
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1, 4);
    dti.tm_year = atoi(tmp2) - 1900;
    // extract month
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1 + 5, 2);
    dti.tm_mon = atoi(tmp2) - 1;
    // extract day
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1 + 8, 2);
    dti.tm_mday = atoi(tmp2);
    // calculate day of week
    int d = dti.tm_mday;        // Day     1-31
    int m = dti.tm_mon + 1;     // Month   1-12
    int y = dti.tm_year + 1900; // Year    2022
    dti.tm_wday = (d += m < 3 ? y-- : y - 2,
                   23 * m / 9 + d + 4 + y / 4 - y / 100 + y / 400) % 7; // calculate day of week
  }
  // get time
  if (strcmp(elementId, "p12_dti_time") == 0) {
    char tmp1[12] = {'\0'};
    char tmp2[12] = {'\0'};
    strncpy(tmp1, value, sizeof(tmp1));
    // extract hour
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1, 2);
    dti.tm_hour = atoi(tmp2);
    // extract minutes
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1 + 3, 2);
    dti.tm_min = atoi(tmp2);
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
    configGPIO();
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
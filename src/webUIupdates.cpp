#include <Dusk2Dawn.h>
#include <EspStrUtil.h>
#include <basics.h>
#include <github.h>
#include <jarolift.h>
#include <language.h>
#include <message.h>
#include <timer.h>
#include <webUI.h>
#include <webUIupdates.h>

/* S E T T I N G S ****************************************************/
#define WEBUI_SLOW_REFRESH_TIME_MS 3000
#define WEBUI_FAST_REFRESH_TIME_MS 100

/* P R O T O T Y P E S ********************************************************/
void updateSystemInfoElements();

/* D E C L A R A T I O N S ****************************************************/

static muTimer refreshTimer1 = muTimer();   // timer to refresh other values
static muTimer refreshTimer2 = muTimer();   // timer to refresh other values
static muTimer otaProgessTimer = muTimer(); // timer to refresh other values

static char tmpMessage[300] = {'\0'};
static bool refreshRequest = false;
static uint16_t devCntNew, devCntOld = 0;
static JsonDocument jsonDoc;
static int logLine, logIdx = 0;
static bool logReadActive = false;
JsonDocument jsonLog;
static const char *TAG = "WEB"; // LOG TAG
static auto &ota = EspSysUtil::OTA::getInstance();
static auto &wdt = EspSysUtil::Wdt::getInstance();
GithubRelease ghLatestRelease;
GithubReleaseInfo ghReleaseInfo;

/**
 * *******************************************************************
 * functions to create a JSON Buffer that contains webUI element updates
 * *******************************************************************/


/**
 * *******************************************************************
 * @brief   update all values (only call once)
 * @param   none
 * @return  none
 * *******************************************************************/
void updateAllElements() {

  refreshRequest = true; // start combined json refresh

  webUI.wsUpdateWebLanguage(LANG::CODE[config.lang]);

  if (setupMode) {
    webUI.wsShowElementClass("setupModeBar", true);
  }
}

/**
 * *******************************************************************
 * @brief   update System informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateSystemInfoElements() {

  webUI.initJsonBuffer(jsonDoc);

  // WiFi information
  if (config.wifi.enable) {
    webUI.addJson(jsonDoc, "p09_wifi_ip", wifi.ipAddress);
    snprintf(tmpMessage, sizeof(tmpMessage), "%i %%", wifi.signal);
    webUI.addJson(jsonDoc, "p09_wifi_signal", tmpMessage);
    snprintf(tmpMessage, sizeof(tmpMessage), "%ld dbm", wifi.rssi);
    webUI.addJson(jsonDoc, "p09_wifi_rssi", tmpMessage);

    if (!WiFi.isConnected()) {
      webUI.addJson(jsonDoc, "p00_wifi_icon", "i_wifi_nok");
    } else if (wifi.rssi < -80) {
      webUI.addJson(jsonDoc, "p00_wifi_icon", "i_wifi_1");
    } else if (wifi.rssi < -70) {
      webUI.addJson(jsonDoc, "p00_wifi_icon", "i_wifi_2");
    } else if (wifi.rssi < -60) {
      webUI.addJson(jsonDoc, "p00_wifi_icon", "i_wifi_3");
    } else {
      webUI.addJson(jsonDoc, "p00_wifi_icon", "i_wifi_4");
    }

  } else {
    webUI.addJson(jsonDoc, "p00_wifi_icon", "");
    webUI.addJson(jsonDoc, "p09_wifi_ip", "-.-.-.-");
    webUI.addJson(jsonDoc, "p09_wifi_signal", "0");
    webUI.addJson(jsonDoc, "p09_wifi_rssi", "0 dbm");
  }

  webUI.addJson(jsonDoc, "p09_eth_ip", strlen(eth.ipAddress) ? eth.ipAddress : "-.-.-.-");
  webUI.addJson(jsonDoc, "p09_eth_status", eth.connected ? WEB_TXT::CONNECTED[config.lang] : WEB_TXT::NOT_CONNECTED[config.lang]);

  // ETH information
  if (config.eth.enable) {
    if (eth.connected) {
      webUI.addJson(jsonDoc, "p00_eth_icon", "i_eth_ok");
      snprintf(tmpMessage, sizeof(tmpMessage), "%d Mbps", eth.linkSpeed);
      webUI.addJson(jsonDoc, "p09_eth_link_speed", tmpMessage);
      webUI.addJson(jsonDoc, "p09_eth_full_duplex", eth.fullDuplex ? WEB_TXT::FULL_DUPLEX[config.lang] : "---");

    } else {
      webUI.addJson(jsonDoc, "p00_eth_icon", "i_eth_nok");
      webUI.addJson(jsonDoc, "p09_eth_link_speed", "---");
      webUI.addJson(jsonDoc, "p09_eth_full_duplex", "---");
    }
  } else {
    webUI.addJson(jsonDoc, "p00_eth_icon", "");
    webUI.addJson(jsonDoc, "p09_eth_link_speed", "---");
    webUI.addJson(jsonDoc, "p09_eth_full_duplex", "---");
  }

  // MQTT Status
  webUI.addJson(jsonDoc, "p09_mqtt_status", config.mqtt.enable ? WEB_TXT::ACTIVE[config.lang] : WEB_TXT::INACTIVE[config.lang]);
  webUI.addJson(jsonDoc, "p09_mqtt_connection", mqttIsConnected() ? WEB_TXT::CONNECTED[config.lang] : WEB_TXT::NOT_CONNECTED[config.lang]);

  if (mqttGetLastError() != nullptr) {
    webUI.addJson(jsonDoc, "p09_mqtt_last_err", mqttGetLastError());
  } else {
    webUI.addJson(jsonDoc, "p09_mqtt_last_err", "---");
  }

  // ESP informations
  webUI.addJson(jsonDoc, "p09_esp_flash_usage", ESP.getSketchSize() * 100.0f / ESP.getFreeSketchSpace());
  webUI.addJson(jsonDoc, "p09_esp_heap_usage", (ESP.getHeapSize() - ESP.getFreeHeap()) * 100.0f / ESP.getHeapSize());
  webUI.addJson(jsonDoc, "p09_esp_maxallocheap", ESP.getMaxAllocHeap() / 1000.0f);
  webUI.addJson(jsonDoc, "p09_esp_minfreeheap", ESP.getMinFreeHeap() / 1000.0f);

  // Uptime
  char uptimeStr[64];
  getUptime(uptimeStr, sizeof(uptimeStr));
  webUI.addJson(jsonDoc, "p09_uptime", uptimeStr);

  // Device Counter
  devCntNew = jaroGetDevCnt();
  if (devCntNew != devCntOld) {
    devCntOld = devCntNew;
    webUI.addJson(jsonDoc, "p12_jaro_devcnt", devCntNew);
  }

  // act Time
  webUI.addJson(jsonDoc, "p09_act_time", EspStrUtil::getTimeString());

  webUI.wsUpdateWebJSON(jsonDoc);
}

/**
 * *******************************************************************
 * @brief   update System informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateSystemInfoElementsStatic() {

  webUI.initJsonBuffer(jsonDoc);

  // Version informations
  webUI.addJson(jsonDoc, "p00_version", VERSION);
  webUI.addJson(jsonDoc, "p09_sw_version", VERSION);
  webUI.addJson(jsonDoc, "p00_dialog_version", VERSION);

  webUI.addJson(jsonDoc, "p09_sw_date", EspStrUtil::getBuildDateTime());

  // restart reason
  webUI.addJson(jsonDoc, "p09_restart_reason", EspSysUtil::RestartReason::get());

  webUI.addJson(jsonDoc, "p12_jaro_devcnt", jaroGetDevCnt());

  // Sunrise, Sunset
  uint8_t sunriseHour, sunriseMinute;
  getSunriseOrSunset(TYPE_SUNRISE, 0, config.geo.latitude, config.geo.longitude, sunriseHour, sunriseMinute);
  snprintf(tmpMessage, sizeof(tmpMessage), "%02d:%02d", sunriseHour, sunriseMinute);
  webUI.addJson(jsonDoc, "p09_sunrise", tmpMessage);

  uint8_t sundownHour, sundownMinute;
  getSunriseOrSunset(TYPE_SUNDOWN, 0, config.geo.latitude, config.geo.longitude, sundownHour, sundownMinute);
  snprintf(tmpMessage, sizeof(tmpMessage), "%02d:%02d", sundownHour, sundownMinute);
  webUI.addJson(jsonDoc, "p09_sundown", tmpMessage);

  // Date
  webUI.addJson(jsonDoc, "p09_act_date", EspStrUtil::getDateString());

  webUI.wsUpdateWebJSON(jsonDoc);
}

/**
 * *******************************************************************
 * @brief   check id read log buffer is active
 * @param   none
 * @return  none
 * *******************************************************************/
bool webLogRefreshActive() { return logReadActive; }

/**
 * *******************************************************************
 * @brief   start reading log buffer
 * @param   none
 * @return  none
 * *******************************************************************/
void webReadLogBuffer() {
  logReadActive = true;
  logLine = 0;
  logIdx = 0;
}

/**
 * *******************************************************************
 * @brief   update Logger output
 * @param   none
 * @return  none
 * *******************************************************************/
void webReadLogBufferCyclic() {

  jsonLog.clear();
  jsonLog["type"] = "logger";
  jsonLog["cmd"] = "add_log";
  JsonArray entryArray = jsonLog["entry"].to<JsonArray>();

  while (logReadActive) {

    if (logLine == 0 && logData.lastLine == 0) {
      // log empty
      logReadActive = false;
      return;
    }
    if (config.log.order == 1) {
      logIdx = (logData.lastLine - logLine - 1) % MAX_LOG_LINES;
    } else {
      if (logData.buffer[logData.lastLine][0] == '\0') {
        // buffer is not full - start reading at element index 0
        logIdx = logLine % MAX_LOG_LINES;
      } else {
        // buffer is full - start reading at element index "logData.lastLine"
        logIdx = (logData.lastLine + logLine) % MAX_LOG_LINES;
      }
    }
    if (logIdx < 0) {
      logIdx += MAX_LOG_LINES;
    }
    if (logIdx >= MAX_LOG_LINES) {
      logIdx = 0;
    }
    if (logLine == MAX_LOG_LINES - 1) {
      // end
      webUI.wsUpdateWebJSON(jsonLog);
      logReadActive = false;
      return;
    } else {
      if (logData.buffer[logIdx][0] != '\0') {
        entryArray.add(logData.buffer[logIdx]);
        logLine++;
      } else {
        // no more entries
        logReadActive = false;
        webUI.wsUpdateWebJSON(jsonLog);
        return;
      }
    }
  }
}

/**
 * *******************************************************************
 * @brief   callback function for OTA progress
 * @param   none
 * @return  none
 * *******************************************************************/
void otaProgressCallback(int progress) {
  if (otaProgessTimer.cycleTrigger(1000)) {
    webUI.wsSendHeartbeat();
    char buttonTxt[32];
    snprintf(buttonTxt, sizeof(buttonTxt), "updating: %i%%", progress);
    webUI.wsUpdateWebText("p00_update_btn", buttonTxt, false);
  }
}

/**
 * *******************************************************************
 * @brief   initiate GitHub version check
 * @param   none
 * @return  none
 * *******************************************************************/
bool startCheckGitHubVersion;
void requestGitHubVersion() { startCheckGitHubVersion = true; }
void processGitHubVersion() {
  if (startCheckGitHubVersion) {
    startCheckGitHubVersion = false;
    if (ghGetLatestRelease(&ghLatestRelease, &ghReleaseInfo)) {
      webUI.wsUpdateWebBusy("p00_dialog_git_version", false);
      webUI.wsUpdateWebText("p00_dialog_git_version", ghReleaseInfo.tag, false);
      webUI.wsUpdateWebHref("p00_dialog_git_version", ghReleaseInfo.url);
      // if new version is available, show update button
      if (strcmp(ghReleaseInfo.tag, VERSION) != 0) {
        char buttonTxt[32];
        snprintf(buttonTxt, sizeof(buttonTxt), "Update %s", ghReleaseInfo.tag);
        webUI.wsUpdateWebText("p00_update_btn", buttonTxt, false);
        webUI.wsUpdateWebHideElement("p00_update_btn_hide", false);
      }
    } else {
      webUI.wsUpdateWebBusy("p00_dialog_git_version", false);
      webUI.wsUpdateWebText("p00_dialog_git_version", "error", false);
    }
  }
}

/**
 * *******************************************************************
 * @brief   initiate GitHub version OTA update
 * @param   none
 * @return  none
 * *******************************************************************/
bool startGitHubUpdate;
void requestGitHubUpdate() { startGitHubUpdate = true; }
void processGitHubUpdate() {
  if (startGitHubUpdate) {
    startGitHubUpdate = false;
    ghSetProgressCallback(otaProgressCallback);
    webUI.wsUpdateWebText("p00_update_btn", "updating: 0%", false);
    webUI.wsUpdateWebDisabled("p00_update_btn", true);
    ota.setActive(true);
    wdt.disable();
    int result = ghStartOtaUpdate(ghLatestRelease, ghReleaseInfo.asset);
    if (result == OTA_SUCCESS) {
      webUI.wsUpdateWebText("p00_update_btn", "updating: 100%", false);
      webUI.wsUpdateWebDialog("version_dialog", "close");
      webUI.wsUpdateWebDialog("ota_update_done_dialog", "open");
      ESP_LOGI(TAG, "GitHub OTA-Update successful");
    } else {
      char errMsg[32];
      switch (result) {
      case OTA_NULL_URL:
        strcpy(errMsg, "URL is NULL");
        break;
      case OTA_CONNECT_ERROR:
        strcpy(errMsg, "Connection error");
        break;
      case OTA_BEGIN_ERROR:
        strcpy(errMsg, "Begin error");
        break;
      case OTA_WRITE_ERROR:
        strcpy(errMsg, "Write error");
        break;
      case OTA_END_ERROR:
        strcpy(errMsg, "End error");
        break;
      default:
        strcpy(errMsg, "Unknown error");
        break;
      }
      webUI.wsUpdateWebText("p00_ota_upd_err", errMsg, false);
      webUI.wsUpdateWebDialog("version_dialog", "close");
      webUI.wsUpdateWebDialog("ota_update_failed_dialog", "open");
      ESP_LOGE(TAG, "GitHub OTA-Update failed: %s", errMsg);
    }
    ota.setActive(false);
    wdt.enable();
  }
}

/**
 * *******************************************************************
 * @brief   cyclic update of webUI elements
 * @param   none
 * @return  none
 * *******************************************************************/
void webUIupdates() {

  // check if new version is available
  processGitHubVersion();
  // perform GitHub update
  processGitHubUpdate();

  // update webUI Logger
  if (webLogRefreshActive()) {
    webReadLogBufferCyclic();
  }

  // ON-BROWSER-REFRESH: refresh ALL elements - do this step by step not to stress the connection
  if (refreshTimer1.cycleTrigger(WEBUI_FAST_REFRESH_TIME_MS) && refreshRequest && !ota.isActive()) {

    updateSystemInfoElementsStatic(); // update static informations (≈ 200 Bytes)
    refreshRequest = false;
  }

  // CYCLIC: update SINGLE elemets every x seconds - do this step by step not to stress the connection
  if (refreshTimer2.cycleTrigger(WEBUI_SLOW_REFRESH_TIME_MS) && !refreshRequest && !ota.isActive()) {

    if (!setupMode) {
      webUI.wsUpdateWebHideElement("cc1101errorBar", getCC1101State());
    }
    updateSystemInfoElements(); // refresh all "System" elements as one big JSON update (≈ 570 Bytes)
  }
}

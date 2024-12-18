#include <basics.h>
#include <jarolift.h>
#include <language.h>
#include <message.h>
#include <ota.h>
#include <stringHelper.h>
#include <wdt.h>
#include <webUI.h>
#include <webUIupdates.h>

/* S E T T I N G S ****************************************************/
#define WEBUI_SLOW_REFRESH_TIME_MS 3000
#define WEBUI_FAST_REFRESH_TIME_MS 100

/* P R O T O T Y P E S ********************************************************/
void updateOilmeterElements(bool forceUpdate);
void updateSensorElements(bool forceUpdate);
void updateSystemInfoElements();

/* D E C L A R A T I O N S ****************************************************/
static muTimer refreshTimer1 = muTimer();    // timer to refresh other values
static muTimer refreshTimer2 = muTimer();    // timer to refresh other values
static muTimer refreshTimer3 = muTimer();    // timer to refresh other values
static muTimer gitVersionTimer1 = muTimer(); // timer to refresh other values
static muTimer gitVersionTimer2 = muTimer(); // timer to refresh other values

static char tmpMessage[300] = {'\0'};
static bool refreshRequest = false;
static uint16_t devCntNew, devCntOld = 0;
static JsonDocument jsonDoc;
static bool jsonDataToSend = false;

static auto &ota = OTAState::getInstance();

// convert minutes to human readable structure
timeComponents convertMinutes(int totalMinutes) {
  int minutesPerYear = 525600; // 365 days * 24 hours * 60 minutes
  int minutesPerDay = 1440;    // 24 hours * 60 minutes
  int minutesPerHour = 60;

  timeComponents result;

  result.years = totalMinutes / minutesPerYear;
  int remainingMinutes = totalMinutes % minutesPerYear;

  result.days = remainingMinutes / minutesPerDay;
  remainingMinutes %= minutesPerDay;

  result.hours = remainingMinutes / minutesPerHour;
  remainingMinutes %= minutesPerHour;

  result.minutes = remainingMinutes;

  return result;
}

/**
 * *******************************************************************
 * functions to create a JSON Buffer that contains webUI element updates
 * *******************************************************************/

// initialize JSON-Buffer
void initJsonBuffer(JsonDocument &jsonBuf) {
  jsonBuf.clear();
  jsonBuf["type"] = "updateJSON";
  jsonDataToSend = false;
}

// check JSON-Buffer
bool dataInJsonBuffer() { return jsonDataToSend; }

// add JSON Element to JSON-Buffer
void addJsonElement(JsonDocument &jsonBuf, const char *elementID, const char *value) {
  jsonBuf[elementID].set((char *)value); // make sure value is handled as a copy not as pointer
  jsonDataToSend = true;
};

// add webElement - numeric Type
template <typename NumericType, typename std::enable_if<std::is_integral<NumericType>::value, NumericType>::type * = nullptr>
inline void addJson(JsonDocument &jsonBuf, const char *elementID, NumericType value) {
  addJsonElement(jsonBuf, elementID, int32ToString(static_cast<intmax_t>(value)));
};
// add webElement - float Type
inline void addJson(JsonDocument &jsonBuf, const char *elementID, float value) { addJsonElement(jsonBuf, elementID, floatToString(value)); };
// add webElement - char Type
inline void addJson(JsonDocument &jsonBuf, const char *elementID, const char *value) { addJsonElement(jsonBuf, elementID, value); };
// add webElement - bool Type
inline void addJson(JsonDocument &jsonBuf, const char *elementID, bool value) { addJsonElement(jsonBuf, elementID, (value ? "true" : "false")); };

/**
 * *******************************************************************
 * @brief   update all values (only call once)
 * @param   none
 * @return  none
 * *******************************************************************/
void updateAllElements() {

  refreshRequest = true; // start combined json refresh

  updateWebLanguage(LANG::CODE[config.lang]);

  if (setupMode) {
    showElementClass("setupModeBar", true);
  }
}

/**
 * *******************************************************************
 * @brief   update System informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateSystemInfoElements() {

  initJsonBuffer(jsonDoc);

  // Network information
  addJson(jsonDoc, "p09_wifi_ip", wifi.ipAddress);
  snprintf(tmpMessage, sizeof(tmpMessage), "%i %%", wifi.signal);
  addJson(jsonDoc, "p09_wifi_signal", tmpMessage);
  snprintf(tmpMessage, sizeof(tmpMessage), "%ld dbm", wifi.rssi);
  addJson(jsonDoc, "p09_wifi_rssi", tmpMessage);

  if (!WiFi.isConnected()) {
    addJson(jsonDoc, "p00_wifi_icon", "i_wifi_nok");
  } else if (wifi.rssi < -80) {
    addJson(jsonDoc, "p00_wifi_icon", "i_wifi_1");
  } else if (wifi.rssi < -70) {
    addJson(jsonDoc, "p00_wifi_icon", "i_wifi_2");
  } else if (wifi.rssi < -60) {
    addJson(jsonDoc, "p00_wifi_icon", "i_wifi_3");
  } else {
    addJson(jsonDoc, "p00_wifi_icon", "i_wifi_4");
  }

  addJson(jsonDoc, "p09_eth_ip", strlen(eth.ipAddress) ? eth.ipAddress : "-.-.-.-");
  addJson(jsonDoc, "p09_eth_status", eth.connected ? WEB_TXT::CONNECTED[config.lang] : WEB_TXT::NOT_CONNECTED[config.lang]);

  if (config.eth.enable) {
    if (eth.connected) {
      addJson(jsonDoc, "p00_eth_icon", "i_eth_ok");
      snprintf(tmpMessage, sizeof(tmpMessage), "%d Mbps", eth.linkSpeed);
      addJson(jsonDoc, "p09_eth_link_speed", tmpMessage);
      addJson(jsonDoc, "p09_eth_full_duplex", eth.fullDuplex ? WEB_TXT::FULL_DUPLEX[config.lang] : "---");

    } else {
      addJson(jsonDoc, "p00_eth_icon", "i_eth_nok");
      addJson(jsonDoc, "p09_eth_link_speed", "---");
      addJson(jsonDoc, "p09_eth_full_duplex", "---");
    }
  } else {
    addJson(jsonDoc, "p00_eth_icon", "");
    addJson(jsonDoc, "p09_eth_link_speed", "---");
    addJson(jsonDoc, "p09_eth_full_duplex", "---");
  }

  // MQTT Status
  addJson(jsonDoc, "p09_mqtt_status", config.mqtt.enable ? WEB_TXT::ACTIVE[config.lang] : WEB_TXT::INACTIVE[config.lang]);
  addJson(jsonDoc, "p09_mqtt_connection", mqttIsConnected() ? WEB_TXT::CONNECTED[config.lang] : WEB_TXT::NOT_CONNECTED[config.lang]);

  if (mqttGetLastError() != nullptr) {
    addJson(jsonDoc, "p09_mqtt_last_err", mqttGetLastError());
  } else {
    addJson(jsonDoc, "p09_mqtt_last_err", "---");
  }

  // ESP informations
  addJson(jsonDoc, "p09_esp_flash_usage", ESP.getSketchSize() * 100.0f / ESP.getFreeSketchSpace());
  addJson(jsonDoc, "p09_esp_heap_usage", (ESP.getHeapSize() - ESP.getFreeHeap()) * 100.0f / ESP.getHeapSize());
  addJson(jsonDoc, "p09_esp_maxallocheap", ESP.getMaxAllocHeap() / 1000.0f);
  addJson(jsonDoc, "p09_esp_minfreeheap", ESP.getMinFreeHeap() / 1000.0f);

  // Uptime and restart reason
  char uptimeStr[64];
  getUptime(uptimeStr, sizeof(uptimeStr));
  addJson(jsonDoc, "p09_uptime", uptimeStr);

  // Device Counter
  devCntNew = jaroGetDevCnt();
  if (devCntNew != devCntOld) {
    devCntOld = devCntNew;
    addJson(jsonDoc, "p12_jaro_devcnt", devCntNew);
  }

  updateWebJSON(jsonDoc);
}

/**
 * *******************************************************************
 * @brief   update System informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateSystemInfoElementsStatic() {

  initJsonBuffer(jsonDoc);

  // Version informations
  addJson(jsonDoc, "p00_version", VERSION);
  addJson(jsonDoc, "p09_sw_version", VERSION);
  addJson(jsonDoc, "p00_dialog_version", VERSION);

  getBuildDateTime(tmpMessage);
  addJson(jsonDoc, "p09_sw_date", tmpMessage);

  // restart reason
  char restartReason[64];
  getRestartReason(restartReason, sizeof(restartReason));
  addJson(jsonDoc, "p09_restart_reason", restartReason);

  addJson(jsonDoc, "p12_jaro_devcnt", jaroGetDevCnt());

  updateWebJSON(jsonDoc);
}

void webUIupdates() {

  if (webLogRefreshActive()) {
    webReadLogBufferCyclic(); // update webUI Logger
  }

  // ON-BROWSER-REFRESH: refresh ALL elements - do this step by step not to stress the connection
  if (refreshTimer3.cycleTrigger(WEBUI_FAST_REFRESH_TIME_MS) && refreshRequest && !ota.isActive()) {

    updateSystemInfoElementsStatic(); // update static informations (≈ 200 Bytes)
    refreshRequest = false;
  }

  // CYCLIC: update SINGLE elemets every x seconds - do this step by step not to stress the connection
  if (refreshTimer2.cycleTrigger(WEBUI_SLOW_REFRESH_TIME_MS) && !refreshRequest && !ota.isActive()) {

    if (!setupMode) {
      updateWebHideElement("cc1101errorBar", getCC1101State());
    }
    updateSystemInfoElements(); // refresh all "System" elements as one big JSON update (≈ 570 Bytes)
  }
}

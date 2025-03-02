#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"

#include <ETH.h>
#include <SPI.h>
#include <basics.h>

#ifndef ETH_PHY_TYPE
#define ETH_PHY_TYPE ETH_PHY_W5500
#define ETH_PHY_ADDR 1
#endif

/* P R O T O T Y P E S ********************************************************/
void ntpSetup();
void setupOTA();
void setup_wifi();

/* D E C L A R A T I O N S ****************************************************/
s_wifi wifi; // global WiFi Informations
s_eth eth;   // global ETH Informations
SPIClass *SPI_2;

static muTimer wifiReconnectTimer = muTimer(); // timer for reconnect delay
static int wifi_retry = 0;
static const char *TAG = "SETUP"; // LOG TAG

/**
 * *******************************************************************
 * @brief   Setup for NTP Server
 * @param   none
 * @return  none
 * *******************************************************************/
void ntpSetup() {
  configTime(0, 0,
             config.ntp.server); // 0, 0 because we will use TZ in the next line
  setenv("TZ", config.ntp.tz,
         1); // Set environment variable with your time zone
  tzset();
}

/**
 * *******************************************************************
 * @brief   callback function if WiFi is connected to configured AP
 * @param   none
 * @return  none
 * *******************************************************************/
void onWiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  wifi_retry = 0;
  ESP_LOGI(TAG, "Connected to AP successfully!");
}

/**
 * *******************************************************************
 * @brief   callback function if WiFi is disconnected from configured AP
 * @param   none
 * @return  none
 * *******************************************************************/
void onWiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  wifi.connected = false;
  ESP_LOGI(TAG, "WiFi-Disconnected");
}

/**
 * *******************************************************************
 * @brief   callback function if WiFi is connected and client got IP
 * @param   none
 * @return  none
 * *******************************************************************/
void onWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  ESP_LOGI(TAG, "WiFi connected");
  snprintf(wifi.ipAddress, sizeof(wifi.ipAddress), "%s", WiFi.localIP().toString().c_str());
  ESP_LOGI(TAG, "IP address: %s", wifi.ipAddress);
  wifi.connected = true;
}

/**
 * *******************************************************************
 * @brief   check WiFi connection and automatic reconnect
 * @param   none
 * @return  none
 * *******************************************************************/
void checkWiFi() {

  // Ethernet is also not connected - so we need to establish WiFi
  if (wifiReconnectTimer.delayOnTrigger((!wifi.connected && !eth.connected), WIFI_RECONNECT)) {
    wifiReconnectTimer.delayReset();

    if (wifi_retry < 5) {
      wifi_retry++;
      WiFi.mode(WIFI_STA);
      WiFi.begin(config.wifi.ssid, config.wifi.password);
      WiFi.hostname(config.wifi.hostname);
      MDNS.begin(config.wifi.hostname);
      ESP_LOGI(TAG, "WiFi Mode STA - Trying connect to: %s", config.wifi.ssid);
      ESP_LOGI(TAG, "WiFi connection - attempt: %i/5", wifi_retry);
    } else {
      ESP_LOGW(TAG, "Wifi connection not possible, esp rebooting...");
      EspSysUtil::RestartReason::saveLocal("no wifi connection");
      yield();
      delay(1000);
      yield();
      ESP.restart();
    }
  }
}

/**
 * *******************************************************************
 * @brief   Setup for general WiFi Function
 * @param   none
 * @return  none
 * *******************************************************************/
void setupWiFi() {

  if (setupMode) {
    // start Accesspoint for initial setup
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP("ESP32-Jarolift");
    ESP_LOGI(TAG, "WiFi Mode: AccessPoint SSID: ESP32-Jarolift / IP: http://192.168.4.1");
  } else if (config.wifi.enable) {

    // setup callback function
    WiFi.onEvent(onWiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(onWiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

    // manual IP-Settings
    if (config.wifi.static_ip) {
      WiFi.config(IPAddress(config.wifi.ipaddress), IPAddress(config.wifi.gateway), IPAddress(config.wifi.subnet), IPAddress(config.wifi.dns));
    }

    // connect to configured wifi AP
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(config.wifi.ssid, config.wifi.password);
    WiFi.hostname(config.wifi.hostname);
    MDNS.begin(config.wifi.hostname);
    ESP_LOGI(TAG, "WiFi Mode STA - Trying connect to: %s", config.wifi.ssid);
  }
}

void onEthEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
  case ARDUINO_EVENT_ETH_START:
    ESP_LOGI(TAG, "ETH Started");
    // set eth hostname here
    ETH.setHostname(config.eth.hostname);
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    ESP_LOGI(TAG, "ETH Connected");
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    eth.connected = true;
    snprintf(eth.ipAddress, sizeof(eth.ipAddress), "%s", ETH.localIP().toString().c_str());
    ESP_LOGI(TAG, "ETH Got IP: '%s'", eth.ipAddress);
    break;
  case ARDUINO_EVENT_ETH_LOST_IP:
    ESP_LOGI(TAG, "ETH Lost IP");
    eth.connected = false;
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    ESP_LOGI(TAG, "ETH Disconnected");
    eth.connected = false;
    break;
  case ARDUINO_EVENT_ETH_STOP:
    ESP_LOGI(TAG, "ETH Stopped");
    eth.connected = false;
    break;
  default:
    break;
  }
}

/**
 * *******************************************************************
 * @brief   Setup for general Ethernet Function
 * @param   none
 * @return  none
 * *******************************************************************/
void setupETH() {

  pinMode(config.eth.gpio_irq, OUTPUT);
  pinMode(config.eth.gpio_rst, OUTPUT);

  Network.onEvent(onEthEvent);

  // Global SPI is used for CC1101 -> use second class and GPIO for Ethernet
  SPI_2 = new SPIClass(HSPI);
  SPI_2->begin(config.eth.gpio_sck, config.eth.gpio_miso, config.eth.gpio_mosi);
  SPI_2->setFrequency(SPI_MASTER_FREQ_8M);
  ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, config.eth.gpio_cs, config.eth.gpio_irq, config.eth.gpio_rst, *SPI_2);

  if (config.eth.static_ip) {
    ETH.config(IPAddress(config.eth.ipaddress), IPAddress(config.eth.gateway), IPAddress(config.eth.subnet), IPAddress(config.eth.dns));
  }
}

/**
 * *******************************************************************
 * @brief   Call all basic Setup functions at once
 * @param   none
 * @return  none
 * *******************************************************************/
void basicSetup() {

  // check internal restart reason
  EspSysUtil::RestartReason::readLocal();

  // WiFi
  setupWiFi();

  // Ethernet
  if (config.eth.enable) {
    setupETH();
  }

  // NTP
  ntpSetup();

  ESP_LOGI(TAG, "ESP-ChipModel: %s", ESP.getChipModel());
  ESP_LOGI(TAG, "ESP-ChipRevision: %ld", ESP.getChipRevision());
  ESP_LOGI(TAG, "ESP-CpuFreq: %lu", ESP.getCpuFreqMHz());
  ESP_LOGI(TAG, "ESP-FlashChipSize: %lu", ESP.getFlashChipSize());
  ESP_LOGI(TAG, "ESP-FlashChipSpeed: %lu", ESP.getFlashChipSpeed());

  /* Print chip information */
  esp_chip_info_t chip_info;
  uint32_t flash_size;
  esp_chip_info(&chip_info);

  switch (chip_info.model) {
  case CHIP_ESP32:
    ESP_LOGD(TAG, "ESP32");
    break;
  case CHIP_ESP32S2:
    ESP_LOGD(TAG, "ESP32-S2");
    break;
  case CHIP_ESP32S3:
    ESP_LOGD(TAG, "ESP32-S3");
    break;
  case CHIP_ESP32C2:
    ESP_LOGD(TAG, "ESP32-C2");
    break;
  case CHIP_ESP32C3:
    ESP_LOGD(TAG, "ESP32-C3");
    break;
  case CHIP_ESP32C6:
    ESP_LOGD(TAG, "ESP32-C6");
    break;
  case CHIP_ESP32H2:
    ESP_LOGD(TAG, "ESP32-H2");
    break;
  case CHIP_ESP32P4:
    ESP_LOGD(TAG, "ESP32-P4");
    break;
  case CHIP_ESP32C61:
    ESP_LOGD(TAG, "ESP32-C61");
    break;
  case CHIP_POSIX_LINUX:
    ESP_LOGD(TAG, "POSIX/Linux");
    break;
  default:
    ESP_LOGD(TAG, "Unknown");
    break;
  }
}

/**
 * *******************************************************************
 * @brief   refresh Network Information
 * @param   none
 * @return  none
 * *******************************************************************/
void refreshNetworkInfo() {

  if (wifi.connected) {
    // check  RSSI
    wifi.rssi = WiFi.RSSI();

    // dBm to Quality:
    if (wifi.rssi <= -100)
      wifi.signal = 0;
    else if (wifi.rssi >= -50)
      wifi.signal = 100;
    else
      wifi.signal = 2 * (wifi.rssi + 100);
  } else {
    wifi.rssi = 0;
    wifi.signal = 0;
  }

  if (eth.connected) {
    eth.fullDuplex = ETH.fullDuplex();
    eth.linkUp = ETH.linkUp();
    eth.linkSpeed = ETH.linkSpeed();
  } else {
    eth.fullDuplex = false;
    eth.linkUp = false;
    eth.linkSpeed = 0;
  }
}

/**
 * *******************************************************************
 * @brief   Send WiFi Information in JSON format via MQTT
 * @param   none
 * @return  none
 * *******************************************************************/
void sendWiFiInfo() {

  JsonDocument wifiJSON;

  refreshNetworkInfo();

  if (wifi.connected) {
    wifiJSON["status"] = wifi.connected ? "connected" : "disconnected";
    wifiJSON["rssi"] = wifi.rssi;
    wifiJSON["signal"] = wifi.signal;
    wifiJSON["ip"] = wifi.ipAddress;
    wifiJSON["date_time"] = EspStrUtil::getDateTimeString();
  } else {
    wifiJSON["status"] = "disconnected";
    wifiJSON["rssi"] = "--";
    wifiJSON["signal"] = "--";
    wifiJSON["ip"] = "--";
    wifiJSON["date_time"] = EspStrUtil::getDateTimeString();
  }

  char sendWififJSON[255];
  serializeJson(wifiJSON, sendWififJSON);
  mqttPublish(addTopic("/wifi"), sendWififJSON, false);

  // wifi status
  mqttPublish(addTopic("/status"), "online", false);
}

/**
 * *******************************************************************
 * @brief   Send WiFi Information in JSON format via MQTT
 * @param   none
 * @return  none
 * *******************************************************************/
void sendETHInfo() {

  refreshNetworkInfo();

  JsonDocument ethJSON;
  ethJSON["ip"] = eth.ipAddress;
  ethJSON["status"] = eth.connected ? "connected" : "disconnected";
  ethJSON["link_up"] = eth.linkUp ? "active" : "inactive";
  ethJSON["link_speed"] = eth.linkSpeed;
  ethJSON["full_duplex"] = eth.linkUp ? "full-duplex" : "---";
  ethJSON["date_time"] = EspStrUtil::getDateTimeString();

  char sendEthJSON[255];
  serializeJson(ethJSON, sendEthJSON);
  mqttPublish(addTopic("/eth"), sendEthJSON, false);

  mqttPublish(addTopic("/status"), "online", false);
}

/**
 * *******************************************************************
 * @brief   build sysinfo structure and send it via mqtt
 * @param   none
 * @return  none
 * *******************************************************************/
void sendSysInfo() {

  // Uptime and restart reason
  char uptimeStr[64];
  getUptime(uptimeStr, sizeof(uptimeStr));
  // ESP Heap and Flash usage
  char heap[10];
  snprintf(heap, sizeof(heap), "%.1f %%", (float)(ESP.getHeapSize() - ESP.getFreeHeap()) * 100 / ESP.getHeapSize());
  char flash[10];
  snprintf(flash, sizeof(flash), "%.1f %%", (float)ESP.getSketchSize() * 100 / ESP.getFreeSketchSpace());

  JsonDocument sysInfoJSON;
  sysInfoJSON["uptime"] = uptimeStr;
  sysInfoJSON["restart_reason"].set((char *)EspSysUtil::RestartReason::get());
  sysInfoJSON["heap"] = heap;
  sysInfoJSON["flash"] = flash;
  sysInfoJSON["sw_version"] = VERSION;
  char sendInfoJSON[255] = {'\0'};
  serializeJson(sysInfoJSON, sendInfoJSON);
  mqttPublish(addTopic("/sysinfo"), sendInfoJSON, false);
}

/**
 * *******************************************************************
 * @brief   generate uptime message
 * @param   buffer
 * @param   bufferSize
 * @return  none
 * *******************************************************************/
void getUptime(char *buffer, size_t bufferSize) {
  static unsigned long previousMillis = 0;
  static unsigned long overflowCounter = 0;
  const unsigned long overflowThreshold = 4294967295;

  unsigned long currentMillis = millis();
  if (currentMillis < previousMillis) {
    // overflow detected
    overflowCounter++;
  }
  previousMillis = currentMillis;

  // calculate total uptime in seconds considering overflow
  unsigned long long totalSeconds = overflowCounter * (overflowThreshold / 1000ULL) + (currentMillis / 1000ULL);

  unsigned long days = totalSeconds / 86400;
  unsigned long hours = (totalSeconds % 86400) / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;

  // use snprintf to format the output
  if (days > 0) {
    snprintf(buffer, bufferSize, "%lud %02luh %02lum %02lus", days, hours, minutes, seconds);
  } else if (hours > 0) {
    snprintf(buffer, bufferSize, "%02luh %02lum %02lus", hours, minutes, seconds);
  } else {
    snprintf(buffer, bufferSize, "%lum %lus", minutes, seconds);
  }
}
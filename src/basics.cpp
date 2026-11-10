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

/* D E C L A R A T I O N S ****************************************************/
s_eth eth;   // global ETH Informations
SPIClass *SPI_2;
s_espInfo espInfo;

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

  pinMode(config.eth.gpio_irq, INPUT_PULLUP);
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

  /* Print chip information */
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  switch (chip_info.model) {
  case CHIP_ESP32:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32");
    espInfo.supported = true;
    break;
  case CHIP_ESP32S2:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32-S2");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32-S2");
    espInfo.supported = true;
    break;
  case CHIP_ESP32S3:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32-S3");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32-S3");
    espInfo.supported = true;
    break;
  case CHIP_ESP32C2:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32-C2");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32-C2");
    espInfo.supported = false;
    break;
  case CHIP_ESP32C3:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32-C3");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32-C3");
    espInfo.supported = true;
    break;
  case CHIP_ESP32C6:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32-C6");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32-C6");
    espInfo.supported = false;
    break;
  case CHIP_ESP32H2:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32-H2");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32-H2");
    espInfo.supported = false;
    break;
  case CHIP_ESP32P4:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32-P4");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32-P4");
    espInfo.supported = false;
    break;
  case CHIP_ESP32C61:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32-C61");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32-C61");
    espInfo.supported = false;
    break;
  default:
    ESP_LOGI(TAG, "ESP-ChipSeries: Unknown");
    break;
  }

  snprintf(espInfo.chipModel, sizeof(espInfo.chipModel), ESP.getChipModel());
  ESP_LOGI(TAG, "ESP-ChipModel: %s", espInfo.chipModel);

  sniprintf(espInfo.chipRev, sizeof(espInfo.chipRev), "%d", ESP.getChipRevision());
  ESP_LOGI(TAG, "ESP-ChipRevision: %s", espInfo.chipRev);

  snprintf(espInfo.chipMhz, sizeof(espInfo.chipMhz), "%lu", ESP.getCpuFreqMHz());
  ESP_LOGI(TAG, "ESP-CpuFreq: %s", espInfo.chipMhz);

  snprintf(espInfo.flashSize, sizeof(espInfo.flashSize), "%s", EspStrUtil::formatBytes(ESP.getFlashChipSize()));
  ESP_LOGI(TAG, "ESP-FlashChipSize: %s", espInfo.flashSize);
}

/**
 * *******************************************************************
 * @brief   refresh Network Information
 * @param   none
 * @return  none
 * *******************************************************************/
void refreshNetworkInfo() {

  refreshWiFiInfo();

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
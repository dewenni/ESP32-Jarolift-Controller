#pragma once
#include <Arduino.h>

/* D E C L A R A T I O N S ****************************************************/

#define MAX_LOG_LINES 200 // max log lines
#define MAX_LOG_ENTRY 128 // max length of one entry

// #ifndef MY_LOGGER_SUPPORT
// #define MY_LOGGER_SUPPORT
// #define ESP_LOGE(tag, format, ...) esp_log_write(ESP_LOG_ERROR, tag, "E (APP-%s): " format "\n", tag, ##__VA_ARGS__)
// #define ESP_LOGI(tag, format, ...) esp_log_write(ESP_LOG_INFO, tag, "I (APP-%s): " format "\n", tag, ##__VA_ARGS__)
// #define ESP_LOGW(tag, format, ...) esp_log_write(ESP_LOG_WARN, tag, "W (APP-%s): " format "\n", tag, ##__VA_ARGS__)
// #define ESP_LOGD(tag, format, ...) esp_log_write(ESP_LOG_DEBUG, tag, "D (APP-%s): " format "\n", tag, ##__VA_ARGS__)
// #endif

struct s_logdata {
  int lastLine;
  char buffer[MAX_LOG_LINES][MAX_LOG_ENTRY];
};

extern s_logdata logData;

/* P R O T O T Y P E S ********************************************************/
void messageSetup();
void messageCyclic();
void addLogBuffer(const char *message);
void clearLogBuffer();
void setLogLevel(uint8_t level);

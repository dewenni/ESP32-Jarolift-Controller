// WebUI.h
#pragma once

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Update.h>
#include <esp_log.h>

#include <gzip_user_css.h>
#include <gzip_user_html.h>
#include <gzip_user_js.h>

#ifndef WEBUI_MAX_WS_CLIENT
#define WEBUI_MAX_WS_CLIENT 3
#endif
#ifndef WEBUI_CHUNK_SIZE
#define WEBUI_CHUNK_SIZE 1024
#endif

class EspWebUI {
public:
  EspWebUI(uint16_t port = 80);

  void begin();
  void loop();

  enum otaStatus { OTA_BEGIN, OTA_PROGRESS, OTA_FINISH, OTA_ERROR };
  enum uploadStatus { UPLOAD_BEGIN, UPLOAD_FINISH, UPLOAD_ERROR };

  // Update-Funktionen
  void wsUpdateText(const char *id, const char *text, bool isInput);
  void wsUpdateLog(const char *entry, const char *cmd);
  void wsLoadConfigWebUI();
  void wsUpdateOTAprogress(const char *progress);
  void wsUpdateWebLanguage(const char *language);
  void wsUpdateWebJSON(JsonDocument &jsonDoc);
  void wsUpdateWebText(const char *id, const char *text, bool isInput);
  void wsUpdateWebText(const char *id, long value, bool isInput);
  void wsUpdateWebLog(const char *entry, const char *cmd);
  void wsUpdateWebText(const char *id, double value, bool isInput, int decimals);
  void wsUpdateWebState(const char *id, bool state);
  void wsUpdateWebValue(const char *id, const char *value);
  void wsUpdateWebValue(const char *id, long value);
  void wsUpdateWebValue(const char *id, double value, int decimals);
  void wsShowElementClass(const char *className, bool show);
  void wsUpdateWebHideElement(const char *id, bool hide);
  void wsUpdateWebDialog(const char *id, const char *state);
  void wsUpdateWebSetIcon(const char *id, const char *icon);
  void wsUpdateWebHref(const char *id, const char *href);
  void wsUpdateWebBusy(const char *id, bool busy);
  void wsUpdateWebDisabled(const char *id, bool disabled);

  // initialize JSON-Buffer
  void initJsonBuffer(JsonDocument &jsonBuf) {
    jsonBuf.clear();
    jsonBuf["type"] = "updateJSON";
  }

  // add JSON Element to JSON-Buffer
  void addJsonElement(JsonDocument &jsonBuf, const char *elementID, const char *value) {
    jsonBuf[elementID] = value; // make sure value is handled as a copy not as pointer
  };

  // add webElement - float Type
  void addJson(JsonDocument &jsonBuf, const char *elementID, float value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f", value);
    addJsonElement(jsonBuf, elementID, buf);
  };

  // add webElement - char Type
  void addJson(JsonDocument &jsonBuf, const char *elementID, const char *value) { addJsonElement(jsonBuf, elementID, value); };

  // add webElement - bool Type
  void addJson(JsonDocument &jsonBuf, const char *elementID, bool value) { addJsonElement(jsonBuf, elementID, (value ? "true" : "false")); };

  template <typename NumericType, typename std::enable_if<std::is_integral<NumericType>::value, NumericType>::type * = nullptr>
  void addJson(JsonDocument &jsonBuf, const char *elementID, NumericType value) {
    std::string s = std::to_string(static_cast<intmax_t>(value));
    addJsonElement(jsonBuf, elementID, s.c_str());
  };

  void wsSendHeartbeat();

  // callback functions
  void setCallbackOta(void (*callback)(EspWebUI::otaStatus otaState, const char *msg)) { callbackOta = callback; }
  void setCallbackUpload(void (*callback)(EspWebUI::uploadStatus uploadState, const char *msg)) { callbackUpload = callback; }
  void setCallbackReload(void (*callback)()) { callbackReload = callback; }
  void setCallbackWebElement(void (*callback)(const char *elementID, const char *elementValue)) { callbackWebElement = callback; }

  // set credentials for authentication
  void setCredentials(const char *username, const char *password) {
    strncpy(config.username, username, sizeof(config.username) - 1);
    config.username[sizeof(config.username) - 1] = '\0';
    strncpy(config.password, password, sizeof(config.password) - 1);
    config.password[sizeof(config.password) - 1] = '\0';
  }

  // enable/disable authentication
  void setAuthentication(bool enableAuth) { config.enableAuth = enableAuth; }

private:
  AsyncWebServer server;
  AsyncWebSocket ws;

  void (*callbackOta)(EspWebUI::otaStatus otaState, const char *msg) = nullptr;
  void (*callbackUpload)(EspWebUI::uploadStatus uploadState, const char *msg) = nullptr;
  void (*callbackReload)() = nullptr;
  void (*callbackWebElement)(const char *elementID, const char *elementValue) = nullptr;

  struct EspWebUIConfig {
    char username[32];
    char password[32];
    bool enableAuth;
  };

  EspWebUIConfig config;
  static constexpr char *TAG = "WEBUI"; // LOG TAG
  static const int TOKEN_LENGTH = 16;
  char sessionToken[TOKEN_LENGTH];
  const char cookieName[20] = "esp_jaro_auth=";

  // Timer für Heartbeat und onLoad
  unsigned long lastHeartbeatTime;
  unsigned long lastOnLoadTime;

  // Hilfsmethoden
  void setupRoutes();
  void setupWebSocket();
  bool isAuthenticated(AsyncWebServerRequest *request);
  void generateSessionToken(char *token, size_t length);
  String getLastModifiedDate();
  void sendWs(JsonDocument &jsonDoc);

  void sendGzipChunkedResponse(AsyncWebServerRequest *request, const uint8_t *content, size_t contentLength, const char *contentType, bool checkAuth,
                               size_t chunkSize);

  // Callback-Funktion für Websocket-Daten
  void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

  void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);
};

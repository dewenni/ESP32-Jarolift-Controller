// WebUI.h
#pragma once

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Update.h>
#include <esp_log.h>



#include <gzip_user_html.h>
#include <gzip_user_css.h>
#include <gzip_user_js.h>

class EspWebUI {
public:
  EspWebUI(uint16_t port = 80);

  void begin(); // Initialisiert den Webserver und die Routes
  void loop();  // Wird regelmäßig im Hauptloop aufgerufen (analog zu webUICyclic)

  enum otaStatus { OTA_BEGIN, OTA_PROGRESS, OTA_FINISH, OTA_ERROR };

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

  void wsSendHeartbeat();

  // callback functions
  void setCallbackOta(void (*callback)(EspWebUI::otaStatus otaState, const char *msg)) { callbackOta = callback; }

  void setCallbackReload(void (*callback)()) { callbackReload = callback; }
  void setCallbackWebElement(void (*callback)(const char *elementID, const char *elementValue)) { callbackWebElement = callback; }

private:
  AsyncWebServer server;
  AsyncWebSocket ws;

  void (*callbackOta)(EspWebUI::otaStatus otaState, const char *msg) = nullptr;
  void (*callbackReload)() = nullptr;
  void (*callbackWebElement)(const char *elementID, const char *elementValue) = nullptr;

  struct EspWebUIConfig {
    char username[32];
    char password[32];
  };

  EspWebUIConfig config;
  static constexpr char *TAG = "WEBUI"; // LOG TAG

  // Authentifizierungs-bezogene Variablen
  static const int TOKEN_LENGTH = 16;
  char sessionToken[TOKEN_LENGTH];
  const char cookieName[20] = "esp_jaro_auth=";

  const int MAX_WS_CLIENT = 3;
  const int CHUNK_SIZE = 1024;

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



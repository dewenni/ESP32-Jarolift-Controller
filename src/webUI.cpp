#include <LittleFS.h>
#include <Update.h>
#include <basics.h>
#include <favicon.h>
#include <language.h>
#include <message.h>
#include <webUI.h>
#include <webUIupdates.h>

const int MAX_WS_CLIENT = 3;
const int CHUNK_SIZE = 1024;

/* P R O T O T Y P E S ********************************************************/
void webCallback(const char *elementId, const char *value);

/* D E C L A R A T I O N S ****************************************************/
static muTimer heartbeatTimer = muTimer(); // timer to refresh other values
static muTimer onLoadTimer = muTimer();    // timer to refresh other values

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

static const char *TAG = "WEB"; // LOG TAG
static bool webInitDone = false;
static size_t content_len;
static const size_t BUFFER_SIZE = 512;
static char webCallbackElementID[32];
static char webCallbackValue[256];
static bool webCallbackAvailable = false;
static bool onLoadRequest = false;
static const int TOKEN_LENGTH = 16;
static char sessionToken[TOKEN_LENGTH] = "";
static char cookieName[] = {"esp_jaro_auth="};

static auto &wdt = EspSysUtil::Wdt::getInstance();
static auto &ota = EspSysUtil::OTA::getInstance();

void sendWs(JsonDocument &jsonDoc) {
  if (ws.count()) {
    const size_t len = measureJson(jsonDoc);
    AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
    assert(buffer); // optional: check buffer
    serializeJson(jsonDoc, buffer->get(), len);
    ws.textAll(buffer);
  }
}

void sendHeartbeat() {
  ws.cleanupClients(MAX_WS_CLIENT);
  if (ws.count()) {
    ws.textAll("{\"type\":\"heartbeat\"}");
  }
}

void loadConfigWebUI() {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "loadConfig";
  sendWs(jsonDoc);
}

void updateOTAprogress(int progress) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "otaProgress";
  jsonDoc["progress"] = progress;
  sendWs(jsonDoc);
}

void updateWebLanguage(const char *language) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "setLanguage";
  jsonDoc["language"] = language;
  sendWs(jsonDoc);
}

void updateWebJSON(JsonDocument &jsonDoc) { sendWs(jsonDoc); }

inline void updateWebText(const char *id, const char *text, bool isInput) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateText";
  jsonDoc["id"] = id;
  jsonDoc["text"] = text;
  jsonDoc["isInput"] = isInput ? true : false;
  sendWs(jsonDoc);
}

inline void updateWebText(const char *id, long value, bool isInput) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateText";
  jsonDoc["id"] = id;
  jsonDoc["text"] = value;
  jsonDoc["isInput"] = isInput ? true : false;
  sendWs(jsonDoc);
}

void updateWebLog(const char *entry, const char *cmd) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "logger";
  jsonDoc["cmd"] = cmd;
  jsonDoc["entry"] = entry;
  sendWs(jsonDoc);
}

inline void updateWebText(const char *id, double value, bool isInput, int decimals) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateText";
  jsonDoc["id"] = id;
  jsonDoc["isInput"] = isInput;
  char textBuffer[16];
  snprintf(textBuffer, sizeof(textBuffer), "%.*f", decimals, value);
  jsonDoc["text"] = textBuffer;
  sendWs(jsonDoc);
}

void updateWebState(const char *id, bool state) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateState";
  jsonDoc["id"] = id;
  jsonDoc["state"] = state ? true : false;
  sendWs(jsonDoc);
}

inline void updateWebValue(const char *id, const char *value) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateValue";
  jsonDoc["id"] = id;
  jsonDoc["value"] = value;
  sendWs(jsonDoc);
}

inline void updateWebValue(const char *id, long value) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateValue";
  jsonDoc["id"] = id;
  jsonDoc["value"] = value;
  sendWs(jsonDoc);
}

inline void updateWebValue(const char *id, double value, int decimals) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateValue";
  jsonDoc["id"] = id;
  char textBuffer[16];
  snprintf(textBuffer, sizeof(textBuffer), "%.*f", decimals, value);
  jsonDoc["value"] = textBuffer;
  sendWs(jsonDoc);
}

void showElementClass(const char *className, bool show) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "showElementClass";
  jsonDoc["className"] = className;
  jsonDoc["show"] = show ? true : false;
  sendWs(jsonDoc);
}

void updateWebHideElement(const char *id, bool hide) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "hideElement";
  jsonDoc["id"] = id;
  jsonDoc["hide"] = hide ? true : false;
  sendWs(jsonDoc);
}

void updateWebDialog(const char *id, const char *state) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateDialog";
  jsonDoc["id"] = id;
  jsonDoc["state"] = state;
  sendWs(jsonDoc);
}

void updateWebSetIcon(const char *id, const char *icon) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateSetIcon";
  jsonDoc["id"] = id;
  jsonDoc["icon"] = icon;
  sendWs(jsonDoc);
}

void updateWebHref(const char *id, const char *href) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateHref";
  jsonDoc["id"] = id;
  jsonDoc["href"] = href;
  sendWs(jsonDoc);
}

void updateWebBusy(const char *id, bool busy) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateBusy";
  jsonDoc["id"] = id;
  jsonDoc["busy"] = busy;
  sendWs(jsonDoc);
}

void updateWebDisabled(const char *id, bool disabled) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateDisabled";
  jsonDoc["id"] = id;
  jsonDoc["disabled"] = disabled;
  sendWs(jsonDoc);
}

void generateSessionToken(char *token, size_t length) {
  const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  size_t charsetSize = sizeof(charset) - 1;

  for (size_t i = 0; i < length - 1; i++) { // -1, because we need a null-terminator
    token[i] = charset[random(0, charsetSize)];
  }
  token[length - 1] = '\0';
}

/**
 * *******************************************************************
 * @brief   function to process the firmware update
 * @param   request, filename, index, data, len, final
 * @return  none
 * *******************************************************************/
void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
  static unsigned long lastUpdateTime = 0;
  static int lastProgress = -1;
  if (!index) {
    ESP_LOGI(TAG, "webOTA started: %s", filename.c_str());
    ota.setActive(true);
    wdt.disable();
    content_len = request->contentLength();
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      ota.setActive(false);
      wdt.enable();
      updateWebText("p00_ota_upd_err", Update.errorString(), false);
      updateWebDialog("ota_update_failed_dialog", "open");
      return request->send(400, "text/plain", "OTA could not begin");
    }
  }
  // update in progress
  if (Update.write(data, len) != len) {
    ota.setActive(false);
    wdt.enable();
    updateWebText("p00_ota_upd_err", Update.errorString(), false);
    updateWebDialog("ota_update_failed_dialog", "open");
    return request->send(400, "text/plain", "OTA could not begin");
  } else {
    // calculate and send progress
    int progress = (Update.progress() * 100) / content_len;
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= 1000) {
      lastUpdateTime = currentTime;
      if (progress != lastProgress) {
        updateOTAprogress(progress);
        lastProgress = progress;
      }
    }
  }
  // update done
  if (final) {
    if (!Update.end(true)) {
      ESP_LOGI(TAG, "OTA Update failed: %s", Update.errorString());
      updateWebText("p000_ota_upd_err", Update.errorString(), false);
      updateWebDialog("ota_update_failed_dialog", "open");
      ota.setActive(false);
      wdt.enable();
      return request->send(400, "text/plain", "Could not end OTA");
    } else {
      ESP_LOGI(TAG, "OTA Update complete");
      updateOTAprogress(100);
      updateWebDialog("ota_update_done_dialog", "open");
      ota.setActive(false);
      wdt.enable();
      return request->send(200, "text/plain", "OTA Update finished!");
    }
  }
}

bool isAuthenticated(AsyncWebServerRequest *request) {
  if (!config.auth.enable) {
    return true; // authentication disabled
  }

  String cookieHeader = request->header("Cookie");
  if (cookieHeader.length() > 0) {
    int cookiePos = cookieHeader.indexOf(cookieName);
    if (cookiePos != -1) {
      int valueStart = cookiePos + strlen(cookieName);
      int valueEnd = cookieHeader.indexOf(';', valueStart);
      if (valueEnd == -1) {
        valueEnd = cookieHeader.length();
      }

      // Buffer for received token
      char receivedToken[TOKEN_LENGTH + 1]; // +1 for null-terminator
      memset(receivedToken, 0, sizeof(receivedToken));

      // copy received token from cookieHeader
      cookieHeader.substring(valueStart, valueEnd).toCharArray(receivedToken, sizeof(receivedToken));

      // compare received token with sessionToken
      if (strncmp(receivedToken, sessionToken, TOKEN_LENGTH) == 0) {
        ESP_LOGD(TAG, "authenticated");
        return true;
      } else {
        ESP_LOGD(TAG, "not authenticated");
        return false;
      }
    }
  }
  return false; // no cookie found
}

String getLastModifiedDate() {
  // Parse date and time from macros
  char monthStr[4];
  int day, year, hour, minute, second;
  sscanf(__DATE__, "%3s %d %d", monthStr, &day, &year);
  sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

  // Convert month abbreviation to month number
  const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  int month = (strstr(months, monthStr) - months) / 3 + 1;

  // Populate tm structure
  struct tm t = {};
  t.tm_year = year - 1900; // Year since 1900
  t.tm_mon = month - 1;    // Month, 0-11
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = minute;
  t.tm_sec = second;

  // Calculate day of the week
  mktime(&t);
  const char *daysOfWeek[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  const char *dayOfWeek = daysOfWeek[t.tm_wday];

  // Format date in RFC 1123 format
  char lastModified[30];
  snprintf(lastModified, sizeof(lastModified), "%s, %02d %s %d %02d:%02d:%02d GMT", dayOfWeek, day, monthStr, year, hour, minute, second);

  return String(lastModified);
}

// Generic function to handle gzip-compressed chunked responses with customizable chunk size
void sendGzipChunkedResponse(AsyncWebServerRequest *request, const uint8_t *content, size_t contentLength, const char *contentType, bool checkAuth,
                             size_t chunkSize) {

  // check if authenticated
  if (!isAuthenticated(request) && checkAuth) {
    request->redirect("/login");
    return;
  }

  // Set ETag based on the size of the gzip-compressed file
  char etag[20];
  snprintf(etag, sizeof(etag), "%d", contentLength);

  // Check if the client already has the current version in cache
  if (request->header("If-None-Match") == etag) {
    request->send(304); // 304 Not Modified
    ESP_LOGD(TAG, "contend not changed: %s", request->url().c_str());
    return;
  }

  ESP_LOGD(TAG, "sending: %s", request->url().c_str());
  // Create a chunked response with the specified chunk size
  AsyncWebServerResponse *response =
      request->beginChunkedResponse(contentType, [content, contentLength, chunkSize](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
        // Check if we have reached the end of the file
        if (index >= contentLength) {
          return 0; // End transmission
        }
        // Determine the actual chunk size to send, ensuring we don't exceed maxLen or remaining content length
        size_t actualChunkSize = min(chunkSize, min(maxLen, contentLength - index));
        memcpy(buffer, content + index, actualChunkSize);
        return actualChunkSize; // Return the number of bytes sent
      });

  // Set HTTP headers
  response->addHeader(asyncsrv::T_Content_Encoding, "gzip");             // Gzip-Encoding
  response->addHeader(asyncsrv::T_Cache_Control, "public,max-age=60");   // Cache-Control header
  response->addHeader(asyncsrv::T_ETag, etag);                           // Set ETag based on file size
  response->addHeader(asyncsrv::T_Last_Modified, getLastModifiedDate()); // Set Last-Modified to build date

  // Send the response
  request->send(response);
}

/**
 * *******************************************************************
 * @brief   cyclic call for webUI - creates all webUI elements
 * @param   none
 * @return  none
 * *******************************************************************/
void webUISetup() {

  generateSessionToken(sessionToken, sizeof(sessionToken));

  server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendGzipChunkedResponse(request, gzip_login_html, gzip_login_html_size, "text/html", false, CHUNK_SIZE);
  });

  server.on("/close_all_ws_clients", HTTP_POST, [](AsyncWebServerRequest *request) {
    ws.cleanupClients();
    // ws.closeAll(); // causes crash with EspAsyncWebServer 3.3.22
    request->send(200, "application/json", "{\"status\":\"all clients closed\"}");
  });

  server.on("/login", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("username", true) && request->hasParam("password", true)) {
      if ((request->getParam("username", true)->value() == String(config.auth.user) &&
           request->getParam("password", true)->value() == String(config.auth.password)) ||
          (request->getParam("username", true)->value() == "esp-jaro" && request->getParam("password", true)->value() == "cc1101")) {
        // successful login - set cookie
        AsyncWebServerResponse *response = request->beginResponse(303); // 303 See Other
        response->addHeader("Location", "/");
        char cookieHeader[128];
        snprintf(cookieHeader, sizeof(cookieHeader), "%s%s; Path=/; HttpOnly; Max-Age=3600", cookieName, sessionToken);
        response->addHeader("Set-Cookie", cookieHeader);
        request->send(response);
      } else {
        request->redirect("/login?error");
      }
    } else {
      request->redirect("/login?error");
    }
  });

  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(303); // 303 See Other
    response->addHeader("Location", "/login");
    // sets the expiration date of the cookie to a time in the past to delete it
    char cookieHeader[128];
    snprintf(cookieHeader, sizeof(cookieHeader), "%s; Path=/; HttpOnly; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Max-Age=0", cookieName);
    response->addHeader("Set-Cookie", cookieHeader);
    request->send(response);
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendGzipChunkedResponse(request, gzip_main_html, gzip_main_html_size, "text/html", true, CHUNK_SIZE);
  });

  server.on("/main.css", HTTP_GET,
            [](AsyncWebServerRequest *request) { sendGzipChunkedResponse(request, gzip_css, gzip_css_size, "text/css", false, CHUNK_SIZE); });

  server.on("/main.js", HTTP_GET,
            [](AsyncWebServerRequest *request) { sendGzipChunkedResponse(request, gzip_js, gzip_js_size, "text/js", false, CHUNK_SIZE); });

  server.on("/gzip_ntp", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendGzipChunkedResponse(request, gzip_ntp_html, gzip_ntp_html_size, "text/html", false, CHUNK_SIZE);
  });

  server.on("/favicon.svg", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "image/svg+xml", faviconSvg); });

  // config.json download
  server.on("/config-download", HTTP_GET,
            [](AsyncWebServerRequest *request) { request->send(LittleFS, "/config.json", "application/octet-stream"); });

  // send config.json file
  server.on("/config.json", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(LittleFS, "/config.json", "application/json"); });

  // config.json upload
  server.on(
      "/config-upload", HTTP_POST, [](AsyncWebServerRequest *request) { request->send(200, "text/plain", "upload done!"); },
      [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
        static File uploadFile;
        const String targetFilename = "/config.json"; // fix to config.json

        if (!index) { // firs call for upload
          updateWebText("upload_status_txt", "uploading...", false);
          ESP_LOGI(TAG, "Upload Start: %s\n", filename.c_str());
          uploadFile = LittleFS.open(targetFilename, "w"); // fix to config.json

          if (!uploadFile) {
            ESP_LOGE(TAG, "Error: file could not be opened");
            updateWebText("upload_status_txt", "error on file close!", false);
            return;
          }
        }

        if (len) { // if there are still data to send...
          uploadFile.write(data, len);
        }

        if (final) {
          if (uploadFile) {
            uploadFile.close();
            ESP_LOGI(TAG, "UploadEnd: %s, %u B\n", filename.c_str(), index + len);
            updateWebText("upload_status_txt", "upload done!", false);
            configLoadFromFile(); // load configuration
            updateWebLanguage(LANG::CODE[config.lang]);
            loadConfigWebUI(); // update webUI settings
          } else {
            updateWebText("upload_status_txt", "error on file close!", false);
          }
        }
      });

  // Route für OTA-Updates
  server.on(
      "/update", HTTP_POST, [](AsyncWebServerRequest *request) {}, handleDoUpdate);

  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    (void)len;
    if (type == WS_EVT_CONNECT) {
      ESP_LOGI(TAG, "web-client connected - IP:%s", client->remoteIP().toString().c_str());
      client->setCloseClientOnQueueFull(false);
      onLoadRequest = true; // update all webUI elements
    } else if (type == WS_EVT_DISCONNECT) {
      ESP_LOGI(TAG, "web-client disconnected");
    } else if (type == WS_EVT_ERROR) {
      ESP_LOGI(TAG, "web-client error");
    } else if (type == WS_EVT_PONG) {
      // Serial.println("ws pong");
    } else if (type == WS_EVT_DATA) {
      AwsFrameInfo *info = (AwsFrameInfo *)arg;
      if (info->final && info->index == 0 && info->len == len) {
        if (info->opcode == WS_TEXT) {
          data[len] = 0;
          JsonDocument jsonDoc;
          DeserializationError error = deserializeJson(jsonDoc, data);
          if (error) {
            ESP_LOGW(TAG, "Failed to parse WebSocket message as JSON");
            return;
          }
          const char *elementId = jsonDoc["elementId"];
          const char *value = jsonDoc["value"];
          snprintf(webCallbackElementID, sizeof(webCallbackElementID), "%s", elementId);
          snprintf(webCallbackValue, sizeof(webCallbackValue), "%s", value);
          webCallbackAvailable = true;
        }
      }
    }
  });

  server.addHandler(&ws).addMiddleware([](AsyncWebServerRequest *request, ArMiddlewareNext next) {
    if (ws.count() >= MAX_WS_CLIENT) {
      // too many clients - answer back immediately and stop processing next middlewares and handler
      request->send(429, "text/plain", "no more client connection allowed");
    } else {
      // process next middleware and at the end the handler
      next();
    }
  });

  server.begin();

} // END SETUP

/**
 * *******************************************************************
 * @brief   cyclic call for webUI - refresh elements by change
 * @param   none
 * @return  none
 * *******************************************************************/
void webUICyclic() {

  // send heartbeat for websocket client
  if (heartbeatTimer.cycleTrigger(3000)) {
    sendHeartbeat();
  }

  // request for update alle elements - not faster than every 1s
  if (onLoadRequest && onLoadTimer.cycleTrigger(1000)) {
    updateAllElements();
    onLoadRequest = false;
    ESP_LOGD(TAG, "updateAllElements()");
  }

  // handling of update webUI elements
  webUIupdates();

  // handling of callback infomation
  if (webCallbackAvailable) {
    webCallback(webCallbackElementID, webCallbackValue);
    webCallbackAvailable = false;
  }

  webInitDone = true; // init done
}

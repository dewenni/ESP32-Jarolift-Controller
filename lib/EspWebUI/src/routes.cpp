#include "EspWebUI.h"

#include <favicon.h>

#include <gzip_lib_css.h>
#include <gzip_lib_js.h>
#include <gzip_login_html.h>
#include <gzip_ntp_html.h>

// Generic function to handle gzip-compressed chunked responses with customizable chunk size
void EspWebUI::sendGzipChunkedResponse(AsyncWebServerRequest *request, const uint8_t *content, size_t contentLength, const char *contentType,
                                    bool checkAuth, size_t chunkSize) {

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

void EspWebUI::setupRoutes() {

  server.on("/login", HTTP_GET, [this](AsyncWebServerRequest *request) {
    sendGzipChunkedResponse(request, gzip_login_html, gzip_login_html_size, "text/html", false, WEBUI_CHUNK_SIZE);
  });

  server.on("/close_all_ws_clients", HTTP_POST, [this](AsyncWebServerRequest *request) {
    ws.cleanupClients();
    // ws.closeAll(); // causes crash with EspAsyncWebServer 3.3.22
    request->send(200, "application/json", "{\"status\":\"all clients closed\"}");
  });

  server.on("/login", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (request->hasParam("username", true) && request->hasParam("password", true)) {
      String username = request->getParam("username", true)->value();
      String password = request->getParam("password", true)->value();

      bool usernameValid = (username == String(config.username) || username == "esp");
      bool passwordValid = (password == String(config.password) || password == "xxx");

      AsyncResponseStream *response = request->beginResponseStream("application/json");

      if (usernameValid && passwordValid) {
        generateSessionToken(sessionToken, sizeof(sessionToken));
        char cookieHeader[80];
        snprintf(cookieHeader, sizeof(cookieHeader), "esp_jaro_auth=%s; Path=/; HttpOnly; Max-Age=3600", sessionToken);
        response->addHeader("Set-Cookie", cookieHeader);

        response->print("{\"success\": true}");
      } else {
        response->print("{\"success\": false, \"usernameValid\": " + String(usernameValid ? "true" : "false") +
                        ", \"passwordValid\": " + String(passwordValid ? "true" : "false") + "}");
      }

      request->send(response);
    } else {
      request->send(400, "application/json", "{\"success\": false, \"error\": \"missing_parameters\"}");
    }
  });

  server.on("/logout", HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(303); // 303 See Other
    response->addHeader("Location", "/login");
    // sets the expiration date of the cookie to a time in the past to delete it
    char cookieHeader[128];
    snprintf(cookieHeader, sizeof(cookieHeader), "%s; Path=/; HttpOnly; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Max-Age=0", cookieName);
    response->addHeader("Set-Cookie", cookieHeader);
    request->send(response);
  });

  server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    sendGzipChunkedResponse(request, gzip_user_html, gzip_user_html_size, "text/html", true, WEBUI_CHUNK_SIZE);
  });

  server.on("/lib.css", HTTP_GET, [this](AsyncWebServerRequest *request) {
    sendGzipChunkedResponse(request, gzip_lib_css, gzip_lib_css_size, "text/css", false, WEBUI_CHUNK_SIZE);
  });

  server.on("/user.css", HTTP_GET, [this](AsyncWebServerRequest *request) {
    sendGzipChunkedResponse(request, gzip_user_css, gzip_user_css_size, "text/css", false, WEBUI_CHUNK_SIZE);
  });

  server.on("/lib.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
    sendGzipChunkedResponse(request, gzip_lib_js, gzip_lib_js_size, "text/js", false, WEBUI_CHUNK_SIZE);
  });

  server.on("/user.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
    sendGzipChunkedResponse(request, gzip_user_js, gzip_user_js_size, "text/js", false, WEBUI_CHUNK_SIZE);
  });

  server.on("/gzip_ntp", HTTP_GET, [this](AsyncWebServerRequest *request) {
    sendGzipChunkedResponse(request, gzip_ntp_html, gzip_ntp_html_size, "text/html", false, WEBUI_CHUNK_SIZE);
  });

  server.on("/favicon.svg", HTTP_GET, [this](AsyncWebServerRequest *request) { request->send(200, "image/svg+xml", faviconSvg); });

  // config.json download
  server.on("/config-download", HTTP_GET,
            [this](AsyncWebServerRequest *request) { request->send(LittleFS, "/config.json", "application/octet-stream"); });

  // send config.json file
  server.on("/config.json", HTTP_GET, [this](AsyncWebServerRequest *request) { request->send(LittleFS, "/config.json", "application/json"); });

  // config.json upload
  server.on(
      "/config-upload", HTTP_POST, [this](AsyncWebServerRequest *request) { request->send(200, "text/plain", "upload done!"); },
      [this](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
        static File uploadFile;
        const String targetFilename = "/config.json"; // fix to config.json

        if (!index) { // firs call for upload
          callbackUpload(UPLOAD_BEGIN, "uploading...");
          ESP_LOGI(TAG, "Upload Start: %s\n", filename.c_str());
          uploadFile = LittleFS.open(targetFilename, "w"); // fix to config.json

          if (!uploadFile) {
            callbackUpload(UPLOAD_ERROR, "error on file close!");
            ESP_LOGE(TAG, "error on file close!");
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
            callbackUpload(UPLOAD_FINISH, "upload done!");
          } else {
            callbackUpload(UPLOAD_ERROR, "error on file close!");
          }
        }
      });

  // Route für OTA-Updates
  server.on(
      "/update", HTTP_POST, [this](AsyncWebServerRequest *request) {},
      [this](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
        this->handleDoUpdate(request, filename, index, data, len, final);
      });
}
#include "EspWebUI.h"

/**
 * *******************************************************************
 * @brief   function to process the firmware update
 * @param   request, filename, index, data, len, final
 * @return  none
 * *******************************************************************/
void EspWebUI::handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
  static unsigned long lastUpdateTime = 0;
  static int lastProgress = -1;
  static size_t content_len;

  if (!index) {
    ESP_LOGI(TAG, "webOTA started: %s", filename.c_str());
    callbackOta(OTA_BEGIN, "OTA Update started");
    content_len = request->contentLength();
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      callbackOta(OTA_ERROR, Update.errorString());
      return request->send(400, "text/plain", "OTA could not begin");
    }
  }
  // update in progress
  if (Update.write(data, len) != len) {
    callbackOta(OTA_ERROR, Update.errorString());
    return request->send(400, "text/plain", "OTA could not begin");
  } else {
    // calculate and send progress
    int progress = (Update.progress() * 100) / content_len;
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= 1000) {
      lastUpdateTime = currentTime;
      if (progress != lastProgress) {
        callbackOta(OTA_PROGRESS, String(progress).c_str());
        lastProgress = progress;
      }
    }
  }
  // update done
  if (final) {
    if (!Update.end(true)) {
      ESP_LOGI(TAG, "OTA Update failed: %s", Update.errorString());
      callbackOta(OTA_ERROR, Update.errorString());
      return request->send(400, "text/plain", "Could not end OTA");
    } else {
      ESP_LOGI(TAG, "OTA Update complete");
      callbackOta(OTA_FINISH, "OTA Update complete");
      return request->send(200, "text/plain", "OTA Update finished!");
    }
  }
}

#include "EspWebUI.h"
#include <stdio.h>
#include <string.h>

void EspWebUI::sendWs(JsonDocument &jsonDoc) {
  if (ws.count()) {
    const size_t len = measureJson(jsonDoc);
    AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
    assert(buffer); // optional: check buffer
    serializeJson(jsonDoc, buffer->get(), len);
    ws.textAll(buffer);
  }
}

void EspWebUI::wsSendHeartbeat() {
  ws.cleanupClients(WEBUI_MAX_WS_CLIENT);
  if (ws.count()) {
    ws.textAll("{\"type\":\"heartbeat\"}");
  }
}

void EspWebUI::wsLoadConfigWebUI() {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "loadConfig";
  sendWs(jsonDoc);
}

void EspWebUI::wsUpdateOTAprogress(const char *progress) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "otaProgress";
  jsonDoc["progress"] = progress;
  sendWs(jsonDoc);
}

 void EspWebUI::wsUpdateWebLanguage(const char *language) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "setLanguage";
  jsonDoc["language"] = language;
  sendWs(jsonDoc);
}

void EspWebUI::wsUpdateWebJSON(JsonDocument &jsonDoc) { sendWs(jsonDoc); }

void EspWebUI::wsUpdateWebText(const char *id, const char *text, bool isInput) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateText";
  jsonDoc["id"] = id;
  jsonDoc["text"] = text;
  jsonDoc["isInput"] = isInput ? true : false;
  sendWs(jsonDoc);
}

void EspWebUI::wsUpdateWebText(const char *id, long value, bool isInput) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateText";
  jsonDoc["id"] = id;
  jsonDoc["text"] = value;
  jsonDoc["isInput"] = isInput ? true : false;
  sendWs(jsonDoc);
}

void EspWebUI::wsUpdateWebLog(const char *entry, const char *cmd) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "logger";
  jsonDoc["cmd"] = cmd;
  jsonDoc["entry"] = entry;
  sendWs(jsonDoc);
}

void EspWebUI::wsUpdateWebText(const char *id, double value, bool isInput, int decimals) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateText";
  jsonDoc["id"] = id;
  jsonDoc["isInput"] = isInput;
  char textBuffer[16];
  snprintf(textBuffer, sizeof(textBuffer), "%.*f", decimals, value);
  jsonDoc["text"] = textBuffer;
  sendWs(jsonDoc);
}

void EspWebUI::wsUpdateWebState(const char *id, bool state) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateState";
  jsonDoc["id"] = id;
  jsonDoc["state"] = state ? true : false;
  sendWs(jsonDoc);
}

void EspWebUI::wsUpdateWebValue(const char *id, const char *value) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateValue";
  jsonDoc["id"] = id;
  jsonDoc["value"] = value;
  sendWs(jsonDoc);
}

void EspWebUI::wsUpdateWebValue(const char *id, long value) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateValue";
  jsonDoc["id"] = id;
  jsonDoc["value"] = value;
  sendWs(jsonDoc);
}

void EspWebUI::wsUpdateWebValue(const char *id, double value, int decimals) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateValue";
  jsonDoc["id"] = id;
  char textBuffer[16];
  snprintf(textBuffer, sizeof(textBuffer), "%.*f", decimals, value);
  jsonDoc["value"] = textBuffer;
  sendWs(jsonDoc);
}

void EspWebUI::wsShowElementClass(const char *className, bool show) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "showElementClass";
  jsonDoc["className"] = className;
  jsonDoc["show"] = show ? true : false;
  sendWs(jsonDoc);
}

void EspWebUI::wsUpdateWebHideElement(const char *id, bool hide) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "hideElement";
  jsonDoc["id"] = id;
  jsonDoc["hide"] = hide ? true : false;
  sendWs(jsonDoc);
}

void EspWebUI::wsUpdateWebDialog(const char *id, const char *state) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateDialog";
  jsonDoc["id"] = id;
  jsonDoc["state"] = state;
  sendWs(jsonDoc);
}

void EspWebUI::wsUpdateWebSetIcon(const char *id, const char *icon) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateSetIcon";
  jsonDoc["id"] = id;
  jsonDoc["icon"] = icon;
  sendWs(jsonDoc);
}

void EspWebUI::wsUpdateWebHref(const char *id, const char *href) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateHref";
  jsonDoc["id"] = id;
  jsonDoc["href"] = href;
  sendWs(jsonDoc);
}

void EspWebUI::wsUpdateWebBusy(const char *id, bool busy) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateBusy";
  jsonDoc["id"] = id;
  jsonDoc["busy"] = busy;
  sendWs(jsonDoc);
}

void EspWebUI::wsUpdateWebDisabled(const char *id, bool disabled) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateDisabled";
  jsonDoc["id"] = id;
  jsonDoc["disabled"] = disabled;
  sendWs(jsonDoc);
}

void EspWebUI::setupWebSocket() {

  ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    (void)len;
    if (type == WS_EVT_CONNECT) {
      ESP_LOGI(TAG, "web-client connected - IP:%s", client->remoteIP().toString().c_str());
      client->setCloseClientOnQueueFull(false);
      callbackReload();
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
          callbackWebElement(elementId, value);
        }
      }
    }
  });
}
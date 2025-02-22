// WebUI.cpp
#include "EspWebUI.h"
#include <stdio.h>
#include <string.h>

EspWebUI::EspWebUI(uint16_t port) : server(port), ws("/ws"), lastHeartbeatTime(0), lastOnLoadTime(0) {
  memset(sessionToken, 0, sizeof(sessionToken));
}

void EspWebUI::begin() {

  generateSessionToken(sessionToken, sizeof(sessionToken));
  setupWebSocket();
  setupRoutes();

  server.addHandler(&ws).addMiddleware([this](AsyncWebServerRequest *request, ArMiddlewareNext next) {
    if (ws.count() >= WEBUI_MAX_WS_CLIENT) {
      // too many clients - answer back immediately and stop processing next middlewares and handler
      request->send(429, "text/plain", "no more client connection allowed");
    } else {
      // process next middleware and at the end the handler
      next();
    }
  });

  server.begin();
}

void EspWebUI::loop() {

  unsigned long currentTime = millis();
  if (currentTime - lastHeartbeatTime >= 3000) {
    lastHeartbeatTime = currentTime;
    wsSendHeartbeat();
  }
}

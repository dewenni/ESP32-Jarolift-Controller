#pragma once

/* I N C L U D E S ****************************************************/
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <EspWebUI.h>
#include <config.h>
#include <language.h>

extern EspWebUI webUI;

/* P R O T O T Y P E S ********************************************************/
void webUISetup();
void webUICyclic();
void webReadLogBuffer();

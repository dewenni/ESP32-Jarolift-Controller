#pragma once
#include <config.h>

#define MAX_LANG 2 // increase if you add more languages

#ifndef MYLOG_LEVEL
#define MYLOG_LEVEL = ESP_LOG_INFO
#endif

namespace LANG {
constexpr const char *CODE[] = {"de", "en"};
};

// ======================================================================================
// mqtt messages : texts that are used for mqtt messages
// ======================================================================================
namespace WEB_TXT {
constexpr const char *ON[] = {"EIN", "ON"};
constexpr const char *OFF[] = {"AUS", "OFF"};
constexpr const char *OK[] = {"OK", "OK"};
constexpr const char *ERROR[] = {"FEHLER", "ERROR"};
constexpr const char *ACTIVE[] = {"aktiv", "active"};
constexpr const char *INACTIVE[] = {"inaktiv", "inactive"};
constexpr const char *FULL_DUPLEX[] = {"Vollduplex", "Full-Duplex"};
constexpr const char *CONNECTED[] = {"verbunden", "connected"};
constexpr const char *NOT_CONNECTED[] = {"nicht verbunden", "not connected"};
constexpr const char *CC1101_NOT_FOUND[] = {"Fehler: CC1101 Modul ist nicht verbunden!", "Error: CC1101 Modul not connected!"};
constexpr const char *JARO_KEYS_INVALID[] = {"Fehler: Jarolift Keys ungültig!", "Error: Jarolift keys invalid!"};
constexpr const char *SERIAL_INVALID[] = {"Fehler: Serialnummer ungültig!", "Error: Serialnumber invalid!"};
constexpr const char *SHUTTER_CMD_UP[] = {"Rolladen AUF", "Shutter UP"};
constexpr const char *SHUTTER_CMD_DOWN[] = {"Rolladen ZU", "Shutter DOWN"};
constexpr const char *SHUTTER_CMD_STOP[] = {"Rolladen STOP", "Shutter STOP"};
constexpr const char *SHUTTER_CMD_SHADE[] = {"Rolladen SCHATTEN", "Shutter SHADE"};
constexpr const char *GROUP_CMD_UP[] = {"Gruppe AUF", "Group UP"};
constexpr const char *GROUP_CMD_DOWN[] = {"Gruppe ZU", "Group DOWN"};
constexpr const char *GROUP_CMD_STOP[] = {"Gruppe STOP", "Group STOP"};
constexpr const char *GROUP_CMD_SHADE[] = {"Gruppe SCHATTEN", "Group SHADE"};
constexpr const char *CMD_SET_SHADE[] = {"Kommando setze Schatten", "Command set shade"};
constexpr const char *CMD_LEARN[] = {"Kommando anlernen", "Command learn"};
}; // namespace WEB_TXT

// ======================================================================================
// mqtt commands : texts that are used as topics for commands
// ======================================================================================
namespace MQTT_TOPIC {
constexpr const char *CMD_RESTART = {"/cmd/restart"};
constexpr const char *CMD_SHUTTER = {"/cmd/shutter"};
constexpr const char *STAT_SHUTTER = {"/status/shutter"};

}; // namespace MQTT_TOPIC

// ======================================================================================
// mqtt messages : texts that are used for mqtt messages
// ======================================================================================
namespace MQTT_MSG {
constexpr const char *DATETIME_CHANGED = {"date and time set to"};

}; // namespace MQTT_MSG

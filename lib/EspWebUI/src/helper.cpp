#include "EspWebUI.h"

String EspWebUI::getLastModifiedDate() {
  // Ähnlich wie in deinem Originalcode: Formatierung des Build-Datums
  char monthStr[4];
  int day, year, hour, minute, second;
  sscanf(__DATE__, "%3s %d %d", monthStr, &day, &year);
  sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);
  const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  int month = (strstr(months, monthStr) - months) / 3 + 1;
  struct tm t = {};
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = minute;
  t.tm_sec = second;
  mktime(&t);
  const char *daysOfWeek[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  const char *dayOfWeek = daysOfWeek[t.tm_wday];
  char lastModified[30];
  snprintf(lastModified, sizeof(lastModified), "%s, %02d %s %d %02d:%02d:%02d GMT", dayOfWeek, day, monthStr, year, hour, minute, second);
  return String(lastModified);
}

void EspWebUI::generateSessionToken(char *token, size_t length) {
  const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  size_t charsetSize = sizeof(charset) - 1;
  for (size_t i = 0; i < length - 1; i++) {
    token[i] = charset[random(0, charsetSize)];
  }
  token[length - 1] = '\0';
}

bool EspWebUI::isAuthenticated(AsyncWebServerRequest *request) {

  if (config.enableAuth == false) {
    ESP_LOGD(TAG, "authentication disabled");
    return true;
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

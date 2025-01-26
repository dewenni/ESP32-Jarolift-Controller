#include <Arduino.h>
#include <Dusk2Dawn.h>
#include <config.h>
#include <jarolift.h>
#include <message.h>

#include <timer.h>

#define CMD_UP 0
#define CMD_DOWN 1
#define CMD_SHADE 2

int lastProcessedTime = -1;       // last processed time
static const char *TAG = "TIMER"; // LOG TAG

bool isDayEnabled(const s_cfg_timer &timer, int day) {
  switch (day) {
  case 0:
    return timer.sunday;
  case 1:
    return timer.monday;
  case 2:
    return timer.tuesday;
  case 3:
    return timer.wednesday;
  case 4:
    return timer.thursday;
  case 5:
    return timer.friday;
  case 6:
    return timer.saturday;
  default:
    return false;
  }
}

void executeCommand(const s_cfg_timer &timer, uint8_t number) {

  switch (timer.cmd) {
  case CMD_UP:
    ESP_LOGI(TAG, "Timer: %i | Cmd: UP | Mask: %04X", number + 1, timer.grp_mask);
    jaroCmd(CMD_GRP_UP, timer.grp_mask);
    break;
  case CMD_DOWN:
    ESP_LOGI(TAG, "Timer: %i | Cmd: DOWN | Mask: %04X", number + 1, timer.grp_mask);
    jaroCmd(CMD_GRP_DOWN, timer.grp_mask);
    break;
  case CMD_SHADE:
    ESP_LOGI(TAG, "Timer: %i | Cmd: SHADE | Mask: %04X", number + 1, timer.grp_mask);
    jaroCmd(CMD_GRP_SHADE, timer.grp_mask);
    break;

  default:
    break;
  }
}

void getSunriseOrSunset(uint8_t type, int16_t offset, float latitude, float longitude, uint8_t &hour, uint8_t &minute) {

  int16_t timeInMinutes;
  time_t now;
  tm dti, utcTime;
  time(&now);
  localtime_r(&now, &dti);  // local Time
  gmtime_r(&now, &utcTime); // UTC Time

  float utcOffset = static_cast<float>(difftime(mktime(&dti), mktime(&utcTime))) / 3600.0;

  Dusk2Dawn location(latitude, longitude, utcOffset);

  if (type == TYPE_SUNRISE) {
    timeInMinutes = location.sunrise(dti.tm_year + 1900, dti.tm_mon + 1, dti.tm_mday, dti.tm_isdst);
  } else if (type == TYPE_SUNDOWN) {
    timeInMinutes = location.sunset(dti.tm_year + 1900, dti.tm_mon + 1, dti.tm_mday, dti.tm_isdst);
  } else {
    hour = 0;
    minute = 0;
    return;
  }

  // add offset
  timeInMinutes += offset;

  // check for overflow
  timeInMinutes = (timeInMinutes + 1440) % 1440;

  // convert to hour and minute
  hour = timeInMinutes / 60;
  minute = timeInMinutes % 60;
}

bool checkTimerTrigger(const s_cfg_timer &timer, uint8_t currentHour, uint8_t currentMinute) {
  if (timer.type == TYPE_FIXED_TIME) {
    uint8_t timerHour = (timer.time_value[0] - '0') * 10 + (timer.time_value[1] - '0');
    uint8_t timerMinute = (timer.time_value[3] - '0') * 10 + (timer.time_value[4] - '0');
    return (timerHour == currentHour && timerMinute == currentMinute);
  } else if (timer.type == TYPE_SUNRISE || timer.type == TYPE_SUNDOWN) {
    uint8_t eventHour, eventMinute;
    getSunriseOrSunset(timer.type, timer.offset_value, config.geo.latitude, config.geo.longitude, eventHour, eventMinute);
    return (eventHour == currentHour && eventMinute == currentMinute);
  }
  return false;
}

void timerCyclic() {
  time_t now;
  tm dti;

  time(&now);
  localtime_r(&now, &dti);

  // check if year is valid
  if (dti.tm_year < 125) { // 2025 = 1900 + 125
    return;
  }

  // check if minute has changed
  if ((now / 60) != (lastProcessedTime / 60)) {
    lastProcessedTime = now;

    // check all timer
    for (int i = 0; i < 6; i++) {
      if (config.timer[i].enable) {
        // check if day is enabled
        if (isDayEnabled(config.timer[i], dti.tm_wday)) {
          // check if timer is triggered
          if (checkTimerTrigger(config.timer[i], dti.tm_hour, dti.tm_min)) {
            executeCommand(config.timer[i], i);
          }
        }
      }
    }
  }
}

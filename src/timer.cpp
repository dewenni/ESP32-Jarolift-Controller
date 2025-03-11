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

/**
 * *******************************************************************
 * @brief   Check if a day is enabled in the timer configuration.
 * @param   timer: Timer configuration.
 * @param   day: Day of the week (0-6).
 * @return  true if the day is enabled, false otherwise.
 * *******************************************************************
 */
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

/**
 * *******************************************************************
 * @brief   Execute a command from a timer.
 * @param   timer: Timer configuration.
 * @param   number: Timer number (0-5).
 * @return  none
 * *******************************************************************
 */
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

/**
 * *******************************************************************
 * @brief   Get the sunrise or sunset time.
 * @param   type: TYPE_SUNRISE or TYPE_SUNDOWN.
 * @param   offset: Time offset in minutes.
 * @param   latitude: Latitude.
 * @param   longitude: Longitude.
 * @param   hour: Sunrise or sunset hour.
 * @param   minute: Sunrise or sunset minute.
 * @return  none
 * *******************************************************************
 */
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

/**
 * *******************************************************************
 * @brief   Get the hour from a time value.
 * @param   time_value: Time value in HH:MM format.
 * @return  Hour value.
 * *******************************************************************
 */
int getHour(const char *time_value) {
  // Format must be HH:MM -> 5 characters.
  if (strlen(time_value) == 5) {
    return (time_value[0] - '0') * 10 + (time_value[1] - '0');
  } else {
    return -1;
  }
}

/**
 * *******************************************************************
 * @brief   Get the minute from a time value.
 * @param   time_value: Time value in HH:MM format.
 * @return  Minute value.
 * *******************************************************************
 */
int getMinute(const char *time_value) {
  // Format must be HH:MM -> 5 characters.
  if (strlen(time_value) == 5) {
    return (time_value[3] - '0') * 10 + (time_value[4] - '0');
  } else {
    return -1;
  }
}

/**
 * *******************************************************************
 * @brief   Check if a timer is triggered.
 * @param   timer: Timer configuration.
 * @param   currentHour: Current hour.
 * @param   currentMinute: Current minute.
 * @return  true if the timer is triggered, false otherwise.
 * *******************************************************************
 */
bool checkTimerTrigger(const s_cfg_timer &timer, uint8_t currentHour, uint8_t currentMinute) {
  if (timer.type == TYPE_FIXED_TIME) {
    uint8_t timerHour = getHour(timer.time_value);
    uint8_t timerMinute = getMinute(timer.time_value);
    return (timerHour == currentHour && timerMinute == currentMinute);
  } else if (timer.type == TYPE_SUNRISE || timer.type == TYPE_SUNDOWN) {
    uint8_t eventHour, eventMinute;
    getSunriseOrSunset(timer.type, timer.offset_value, config.geo.latitude, config.geo.longitude, eventHour, eventMinute);

    // Check min/max time ranges:
    if (timer.use_min_time) {
      uint8_t minHour = getHour(timer.min_time_value);
      uint8_t minMinute = getMinute(timer.min_time_value);
      if (minHour >= 0) {
        eventHour = max(eventHour, minHour);
      }
      if (eventHour == minHour) {
        eventMinute = max(eventMinute, minMinute);
      }
    }

    if (timer.use_max_time) {
      uint8_t maxHour = getHour(timer.max_time_value);
      uint8_t maxMinute = getMinute(timer.max_time_value);
      if (maxHour >= 0) {
        eventHour = min(eventHour, maxHour);
      }
      if (eventHour == maxHour) {
        eventMinute = min(eventMinute, maxMinute);
      }
    }

    return (eventHour == currentHour && eventMinute == currentMinute);
  }
  return false;
}

/**
 * *******************************************************************
 * @brief   Timer Cyclic Loop
 * @param   none
 * @return  none
 * *******************************************************************
 */
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

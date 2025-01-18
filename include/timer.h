#pragma once

#include <Arduino.h>

#define TYPE_FIXED_TIME 0
#define TYPE_SUNRISE 1
#define TYPE_SUNDOWN 2

void timerCyclic();
void getSunriseOrSunset(uint8_t type, int16_t offset, float latitude, float longitude, uint8_t &hour, uint8_t &minute);

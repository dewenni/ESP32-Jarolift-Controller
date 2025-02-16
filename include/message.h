#pragma once
#include <Arduino.h>

/* D E C L A R A T I O N S ****************************************************/

#define MAX_LOG_LINES 200 // max log lines
#define MAX_LOG_ENTRY 128 // max length of one entry

struct s_logdata {
  int lastLine;
  char buffer[MAX_LOG_LINES][MAX_LOG_ENTRY];
};

extern s_logdata logData;

/* P R O T O T Y P E S ********************************************************/
void messageSetup();
void messageCyclic();
void addLogBuffer(const char *message);
void clearLogBuffer();
void setLogLevel(uint8_t level);

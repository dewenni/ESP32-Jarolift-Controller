#include "EscapeCodes.h"
#include <basics.h>
#include <config.h>
#include <jarolift.h>
#include <language.h>
#include <message.h>
#include <telnet.h>
#include <webUI.h>

/* D E C L A R A T I O N S ****************************************************/
ESPTelnet telnet;
s_telnetIF telnetIF;
static EscapeCodes ansi;
static char param[MAX_PAR][MAX_CHAR];
static bool msgAvailable = false;
static const char *TAG = "TELNET"; // LOG TAG

/* P R O T O T Y P E S ********************************************************/
void readLogger();
void dispatchCommand(char param[MAX_PAR][MAX_CHAR]);
bool extractMessage(String str, char param[MAX_PAR][MAX_CHAR]);
void cmdHelp(char param[MAX_PAR][MAX_CHAR]);
void cmdCls(char param[MAX_PAR][MAX_CHAR]);
void cmdConfig(char param[MAX_PAR][MAX_CHAR]);
void cmdInfo(char param[MAX_PAR][MAX_CHAR]);
void cmdDisconnect(char param[MAX_PAR][MAX_CHAR]);
void cmdRestart(char param[MAX_PAR][MAX_CHAR]);
void cmdLog(char param[MAX_PAR][MAX_CHAR]);
void cmdSerial(char param[MAX_PAR][MAX_CHAR]);
void cmdTest(char param[MAX_PAR][MAX_CHAR]);
void cmdShutter(char param[MAX_PAR][MAX_CHAR]);
void cmdDeviceCnt(char param[MAX_PAR][MAX_CHAR]);
void cmdGroup(char param[MAX_PAR][MAX_CHAR]);

Command commands[] = {
    {"cls", cmdCls, "Clear screen", ""},
    {"config", cmdConfig, "config commands", "<reset>"},
    {"disconnect", cmdDisconnect, "disconnect telnet", ""},
    {"help", cmdHelp, "Displays this help message", "[command]"},
    {"info", cmdInfo, "Print system information", ""},
    {"log", cmdLog, "logger commands", "[enable], [disable], [read], [clear], [mode], [mode <1..5>]"},
    {"restart", cmdRestart, "Restart the ESP", ""},
    {"serial", cmdSerial, "serial stream output", "<stream> <[start], [stop]>"},
    {"shutter", cmdShutter, "shutter commands", "channel, up/down/stop/shade"},
    {"group", cmdGroup, "group commands", "group, up/down/stop/shade"},
    {"test", cmdTest, "test commands", "group"},
};
const int commandsCount = sizeof(commands) / sizeof(commands[0]);

// print telnet shell
void telnetShell() {
  telnet.print(ansi.setFG(ANSI_BRIGHT_GREEN));
  telnet.print("$ >");
  telnet.print(ansi.reset());
}

void onTelnetConnect(String ip) {
  ESP_LOGI(TAG, "Telnet: %s connected", ip.c_str());
  telnet.println(ansi.setFG(ANSI_BRIGHT_GREEN));
  telnet.println("\n----------------------------------------------------------------------");
  telnet.println("\nESP32-Jarolift-MQTT");
  telnet.println("\nWelcome " + telnet.getIP());
  telnet.println("use command: \"help\" for further information");
  telnet.println("\n----------------------------------------------------------------------\n");
  telnet.println(ansi.reset());
  telnetShell();
}

void onTelnetDisconnect(String ip) { ESP_LOGI(TAG, "Telnet: %s disconnected", ip.c_str()); }

void onTelnetReconnect(String ip) { ESP_LOGI(TAG, "Telnet: %s reconnected", ip.c_str()); }

void onTelnetConnectionAttempt(String ip) { ESP_LOGI(TAG, "Telnet: %s tried to connect", ip.c_str()); }

void onTelnetInput(String str) {
  if (!extractMessage(str, param)) {
    telnet.println("Syntax error");
  } else {
    msgAvailable = true;
  }
  telnetShell();
}

/**
 * *******************************************************************
 * @brief   setup function for Telnet
 * @param   none
 * @return  none
 * *******************************************************************/
void setupTelnet() {
  // passing on functions for various telnet events
  telnet.onConnect(onTelnetConnect);
  telnet.onConnectionAttempt(onTelnetConnectionAttempt);
  telnet.onReconnect(onTelnetReconnect);
  telnet.onDisconnect(onTelnetDisconnect);
  telnet.onInputReceived(onTelnetInput);

  if (telnet.begin(23, false)) {
    ESP_LOGI(TAG, "Telnet Server running!");
  } else {
    ESP_LOGI(TAG, "Telnet Server error!");
  }
}

/**
 * *******************************************************************
 * @brief   cyclic function for Telnet
 * @param   none
 * @return  none
 * *******************************************************************/
void cyclicTelnet() {

  telnet.loop();

  // process incoming messages
  if (msgAvailable) {
    dispatchCommand(param);
    msgAvailable = false;
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: log commands
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdLog(char param[MAX_PAR][MAX_CHAR]) {

  if (strlen(param[1]) == 0) {
    if (config.log.enable)
      telnet.println("enabled");
    else
      telnet.println("disabled");
    telnetShell();
    // log: disable
  } else if (!strcmp(param[1], "disable") && strlen(param[2]) == 0) {
    config.log.enable = false;
    clearLogBuffer();
    // updateSettingsValues();
    telnet.println("disabled");
    // log enable
  } else if (!strcmp(param[1], "enable") && strlen(param[2]) == 0) {
    config.log.enable = true;
    clearLogBuffer();
    telnet.println("enabled");
    // log: read buffer
  } else if (!strcmp(param[1], "read") && strlen(param[2]) == 0) {
    readLogger();
    // log clear buffer
  } else if (!strcmp(param[1], "clear") && strlen(param[2]) == 0) {
    clearLogBuffer();
    // log mode read
  } else if (!strcmp(param[1], "mode") && strlen(param[2]) == 0) {
    if (config.log.level == 4) {
      telnet.println("level: 4=DEBUG");
    } else if (config.log.level == 3) {
      telnet.println("level: 3=INFO");
    } else if (config.log.level == 2) {
      telnet.println("level: 2=WARNING");
    } else if (config.log.level == 1) {
      telnet.println("level: 1=ERROR");
    }
    //  log mode set
  } else if (!strcmp(param[1], "mode") && strlen(param[2]) > 0) {
    int level = atoi(param[2]);
    if (level > 0 && level < 5) {
      config.log.level = level;
      clearLogBuffer();
      if (config.log.level == 4) {
        telnet.println("level set: 4=DEBUG");
      } else if (config.log.level == 3) {
        telnet.println("level set: 3=INFO");
      } else if (config.log.level == 2) {
        telnet.println("level set: 2=WARNING");
      } else if (config.log.level == 1) {
        telnet.println("level set: 1=ERROR");
      }
    } else {
      telnet.println("invalid level - mode must be between 1 and 4");
    }
  } else {
    telnet.println("unknown parameter");
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: shutter commands
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdShutter(char param[MAX_PAR][MAX_CHAR]) {
  int channel = -1;

  if (strlen(param[1]) > 0) {
    channel = atoi(param[1]);
    if (channel < 1 || channel > 16) {
      telnet.println("invalid channel");
      return;
    }
  }
  if (!strcmp(param[2], "up")) {
    jaroCmd(CMD_UP, channel - 1);
  } else if (!strcmp(param[2], "down")) {
    jaroCmd(CMD_DOWN, channel - 1);
  } else if (!strcmp(param[2], "stop")) {
    jaroCmd(CMD_STOP, channel - 1);
  } else if (!strcmp(param[2], "shade")) {
    jaroCmd(CMD_SHADE, channel - 1);
  } else {
    telnet.println("unknown command");
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: group commands
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdGroup(char param[MAX_PAR][MAX_CHAR]) {
  int group = -1;

  if (strlen(param[1]) > 0) {
    group = atoi(param[1]);
    if (group < 1 || group > 6) {
      telnet.println("invalid channel");
      return;
    }
  }
  if (!strcmp(param[2], "up")) {
    jaroCmd(CMD_GRP_UP, config.jaro.grp_mask[group - 1]);
  } else if (!strcmp(param[2], "down")) {
    jaroCmd(CMD_GRP_DOWN, config.jaro.grp_mask[group - 1]);
  } else if (!strcmp(param[2], "stop")) {
    jaroCmd(CMD_GRP_STOP, config.jaro.grp_mask[group - 1]);
  } else if (!strcmp(param[2], "shade")) {
    jaroCmd(CMD_GRP_SHADE, config.jaro.grp_mask[group - 1]);
  } else {
    telnet.println("unknown command");
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: config structure
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdTest(char param[MAX_PAR][MAX_CHAR]) {

  if (!strcmp(param[1], "crash") && !strcmp(param[2], "")) {
    int *ptr = nullptr;
    *ptr = 42;
  } else if (!strcmp(param[1], "watchdog") && !strcmp(param[2], "")) {
    telnet.println("watchdog test started...");
    while (1) {
    }
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: config structure
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdConfig(char param[MAX_PAR][MAX_CHAR]) {

  if (!strcmp(param[1], "reset") && !strcmp(param[2], "")) {
    configInitValue();
    configSaveToFile();
    telnet.println("config was set to defaults");
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: print system information
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdInfo(char param[MAX_PAR][MAX_CHAR]) {

  telnet.print(ansi.setFG(ANSI_BRIGHT_WHITE));
  telnet.println("ESP-INFO");
  telnet.print(ansi.reset());
  telnet.printf("ESP Flash Usage: %s %%\n", EspStrUtil::floatToString((float)ESP.getSketchSize() * 100 / ESP.getFreeSketchSpace(), 1));
  telnet.printf("ESP Heap Usage: %s %%\n", EspStrUtil::floatToString((float)ESP.getFreeHeap() * 100 / ESP.getHeapSize(), 1));
  telnet.printf("ESP MAX Alloc Heap: %s KB\n", EspStrUtil::floatToString((float)ESP.getMaxAllocHeap() / 1000.0, 1));
  telnet.printf("ESP MAX Alloc Heap: %s KB\n", EspStrUtil::floatToString((float)ESP.getMinFreeHeap() / 1000.0, 1));

  telnet.print(ansi.setFG(ANSI_BRIGHT_WHITE));
  telnet.println("\nRESTART - UPTIME");
  telnet.print(ansi.reset());
  char tmpMsg[64];
  getUptime(tmpMsg, sizeof(tmpMsg));
  telnet.printf("Uptime: %s\n", tmpMsg);
  telnet.printf("Restart Reason: %s\n", EspSysUtil::RestartReason::get());

  telnet.print(ansi.setFG(ANSI_BRIGHT_WHITE));
  telnet.println("\nWiFi-INFO");
  telnet.print(ansi.reset());
  telnet.printf("IP-Address: %s\n", wifi.ipAddress);
  telnet.printf("WiFi-Signal: %s %%\n", EspStrUtil::intToString(wifi.signal));
  telnet.printf("WiFi-Rssi: %s dbm\n", EspStrUtil::intToString(wifi.rssi));

  telnet.println();
}

/**
 * *******************************************************************
 * @brief   telnet command: clear output
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdCls(char param[MAX_PAR][MAX_CHAR]) {
  telnet.print(ansi.cls());
  telnet.print(ansi.home());
}

/**
 * *******************************************************************
 * @brief   telnet command: disconnect
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdDisconnect(char param[MAX_PAR][MAX_CHAR]) {
  telnet.println("disconnect");
  telnet.disconnectClient();
}

/**
 * *******************************************************************
 * @brief   telnet command: restart ESP
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdRestart(char param[MAX_PAR][MAX_CHAR]) {
  telnet.println("ESP will restart - you have to reconnect");
  EspSysUtil::RestartReason::saveLocal("telnet command");
  yield();
  delay(1000);
  yield();
  ESP.restart();
}

/**
 * *******************************************************************
 * @brief   telnet command: serial stream
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdSerial(char param[MAX_PAR][MAX_CHAR]) {
  if (!strcmp(param[1], "stream") && !strcmp(param[2], "start")) {
    telnetIF.serialStream = true;
    telnet.println("serial stream active - to abort streaming, send \"x\"");
  } else if (!strcmp(param[1], "stream") && !strcmp(param[2], "stop")) {
    telnetIF.serialStream = false;
    telnet.println("serial stream disabled");
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: sub function to print help
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void printHelp(const char *command = nullptr) {
  if (command == nullptr) {
    // print help of all commands
    for (int i = 0; i < commandsCount; ++i) {
      telnet.printf("%-15s %-60s\n", commands[i].name, commands[i].parameters);
    }
  } else {
    // print help of specific command
    for (int i = 0; i < commandsCount; ++i) {
      if (strcmp(command, commands[i].name) == 0) {
        telnet.printf("%-15s %-60s\n%s\n", commands[i].name, commands[i].parameters, commands[i].description);
        return;
      }
    }
    telnet.println("Unknown command. Use 'help' to see all commands.");
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: print help
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdHelp(char param[MAX_PAR][MAX_CHAR]) {
  if (strlen(param[1]) > 0) {
    printHelp(param[1]);
  } else {
    printHelp();
  }
}

/**
 * *******************************************************************
 * @brief   function to read log buffer
 * @param   none
 * @return  none
 * *******************************************************************/
void readLogger() {
  int line = 0;
  while (line < MAX_LOG_LINES) {
    int lineIndex;
    if (line == 0 && logData.lastLine == 0) {
      telnet.println("log empty");
      break;
    } else if (logData.buffer[logData.lastLine][0] == '\0') {
      lineIndex = line % MAX_LOG_LINES; // buffer is not full - start reading at element index 0
    } else {
      lineIndex = (logData.lastLine + line) % MAX_LOG_LINES; // buffer is full - start reading at element index "logData.lastLine"
    }
    if (lineIndex < 0) {
      lineIndex += MAX_LOG_LINES;
    }
    if (lineIndex >= MAX_LOG_LINES) {
      lineIndex = 0;
    }
    if (line == 0) {
      telnet.printf("<LOG-Begin>\n%s\n", logData.buffer[lineIndex]); // add header
      line++;
    } else if (line == MAX_LOG_LINES - 1) {
      telnet.printf("%s\n<LOG-END>\n", logData.buffer[lineIndex]); // add footer
      line++;
    } else {
      if (logData.buffer[lineIndex][0] != '\0') {
        telnet.printf("%s\n", logData.buffer[lineIndex]); // add messages
        line++;
      } else {
        telnet.printf("<LOG-END>\n");
        break;
      }
    }
  }
}

/**
 * *******************************************************************
 * @brief   check receives telnet message and extract to param array
 * @param   str received message
 * @param   param char array of parameters
 * @return  none
 * *******************************************************************/
bool extractMessage(String str, char param[MAX_PAR][MAX_CHAR]) {
  const char *p = str.c_str();
  int i = 0, par = 0;
  // initialize parameter strings
  for (int j = 0; j < MAX_PAR; j++) {
    memset(&param[j], 0, sizeof(param[0]));
  }
  // extract answer into parameter
  while (*p != '\0') {
    if (i >= MAX_CHAR - 1) {
      param[par][i] = '\0';
      return false;
    }
    if (*p == ' ' || i == MAX_CHAR - 1) {
      param[par][i] = '\0';
      par++;
      p++;
      i = 0;
      if (par >= MAX_PAR)
        return false;
    }
    param[par][i] = *p++;
    i++;
  }
  return true;
}

/**
 * *******************************************************************
 * @brief   telnet command dispatcher
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void dispatchCommand(char param[MAX_PAR][MAX_CHAR]) {
  for (int i = 0; i < commandsCount; ++i) {
    if (!strcmp(param[0], commands[i].name)) {
      commands[i].function(param);
      telnetShell();
      return;
    }
  }
  telnet.println("Unknown command");
  telnetShell();
}

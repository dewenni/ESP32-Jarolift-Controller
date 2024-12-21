#pragma once

void jaroCmdSetDevCnt(uint16_t value);
void jaroCmdReInit();
bool getCC1101State();
uint8_t getCC1101Rssi();
uint16_t jaroGetDevCnt();

void jaroliftSetup();
void jaroliftCyclic();

enum JaroCommandType { CMD_UP, CMD_DOWN, CMD_STOP, CMD_SET_SHADE, CMD_SHADE, CMD_LEARN };

struct JaroCommand {
  JaroCommandType type;
  uint8_t channel;
};

void jaroCmd(JaroCommandType type, uint8_t channel);
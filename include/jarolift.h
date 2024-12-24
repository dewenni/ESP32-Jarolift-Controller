#pragma once

void jaroCmdSetDevCnt(uint16_t value);
void jaroCmdReInit();
bool getCC1101State();
uint8_t getCC1101Rssi();
uint16_t jaroGetDevCnt();

void jaroliftSetup();
void jaroliftCyclic();

enum JaroCmdType { CMD_UP, CMD_DOWN, CMD_STOP, CMD_SET_SHADE, CMD_SHADE, CMD_LEARN };
enum JaroCmdGrpType { CMD_GRP_UP, CMD_GRP_DOWN, CMD_GRP_STOP, CMD_GRP_SHADE };

struct JaroCommand {
  enum CommandType { SINGLE, GROUP } cmdType;
  union {
    struct {
      JaroCmdType type;
      uint8_t channel;
    } single;
    struct {
      JaroCmdGrpType type;
      uint16_t group_mask;
    } group;
  };
};

void jaroCmd(JaroCmdType type, uint8_t channel);
void jaroCmd(JaroCmdGrpType type, uint16_t group_mask);

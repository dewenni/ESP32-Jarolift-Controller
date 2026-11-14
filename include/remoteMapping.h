#pragma once
#include <stdint.h>
#include <ArduinoJson.h>
constexpr uint8_t REMOTE_MAPPING_LIMIT = 80;
constexpr uint8_t REMOTE_MAPPING_PER_SHUTTER = 5;
constexpr uint8_t REMOTE_MAPPING_NAME_CHARS = 32;
struct RemoteMapping {
  char name[4 * REMOTE_MAPPING_NAME_CHARS + 1]{};
  uint32_t serial=0;
  uint16_t sourceMask=0, targets=0;
  bool legacyGroup=false;
};
struct RemoteMappingMatch {
  uint16_t targets=0;
  const char *name=nullptr;
};
bool remoteMappingValid(const RemoteMapping &mapping);
RemoteMappingMatch remoteMappingReceive(uint32_t serial, uint16_t mask, uint8_t command);
void remoteMappingWebCommand(const char *payload);
void remoteMappingCyclic();
void remoteMappingReset();
void remoteMappingLoad(JsonVariantConst entries, int version);
void remoteMappingSave(JsonObject root);

#pragma once
#include <stdint.h>
#include <ArduinoJson.h>
constexpr uint8_t REMOTE_MAPPING_LIMIT = 32;
struct RemoteMapping {
  char name[48]{};
  uint32_t serial=0;
  uint16_t sourceMask=0, targets=0;
};
bool remoteMappingValid(const RemoteMapping &mapping);
const RemoteMapping *remoteMappingReceive(uint32_t serial, uint16_t mask, uint8_t command);
void remoteMappingWebCommand(const char *payload);
void remoteMappingCyclic();
void remoteMappingReset();

void remoteMappingLoad(JsonVariantConst entries, int version);
void remoteMappingSave(JsonObject root);

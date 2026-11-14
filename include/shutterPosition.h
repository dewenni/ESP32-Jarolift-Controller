#pragma once
#include <stdint.h>
#include <jarolift.h>
void shutterPositionCyclic();
void shutterPositionWebCommand(const char *payload);
void shutterPositionManual(uint16_t mask, bool stopMotor = true);
void shutterPositionRemote(uint16_t mask, JaroCmdType command = CMD_STOP);
void shutterPositionCommand(uint16_t mask, JaroCmdType command);
void shutterPositionPublish();
void shutterPositionInvalidate(uint8_t channel);
bool shutterPositionBusy();

// Accepted targets start immediately or wait in the shared 16-entry FIFO. MQTT payload: 0..100.
bool shutterPositionReady(uint8_t channel);
bool shutterPositionTarget(uint8_t channel, const char *payload);

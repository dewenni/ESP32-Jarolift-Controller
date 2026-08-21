#include "virtualHomee.hpp"
#include "virtualHomee/homee_defines.h"
#include "virtualHomee/homee_icons.h"
#include <config.h>
#include <homee.h>
#include <jarolift.h>
#include <message.h>

/* D E C L A R A T I O N S ****************************************************/
static const char *TAG = "HOMEE"; // LOG TAG

#define HOMEE_NODE_ID_BASE 100 // node id for channel i = HOMEE_NODE_ID_BASE + i
#define HOMEE_ATTR_ID_BASE 1000 // attribute ids for channel i = HOMEE_ATTR_ID_BASE + i*10 + {1,2,3}

#define HOMEE_SHADE_RESET_DELAY 500 // ms until the shade button reports "released" again
#define HOMEE_UP_DOWN_STOP 2 // CAAttributeTypeUpDown target/current value for "stopped"

virtualHomee vhih;

static nodeAttributes *homeeUpDownAttr[16] = {nullptr};
static nodeAttributes *homeeShadeAttr[16] = {nullptr};
static bool shadeResetPending[16] = {false};
static unsigned long shadeResetAt[16] = {0};

/**
 * *******************************************************************
 * @brief   parse VERSION ("vMAJOR.MINOR.PATCH") into a homee-compatible
 *          numeric firmware revision (major.minor, patch is dropped)
 * @param   none
 * @return  firmware revision as double, e.g. 1.9
 * *******************************************************************/
static double firmwareVersionAsDouble() {
  int major = 0, minor = 0;
  sscanf(VERSION, "v%d.%d", &major, &minor);
  return major + minor / 100.0;
}

/**
 * *******************************************************************
 * @brief   callback for the shutter up/down/stop attribute
 * @param   changed attribute
 * @return  none
 * *******************************************************************/
static void homeeCallbackUpDown(nodeAttributes *a) {
  int channel = (a->getId() - HOMEE_ATTR_ID_BASE) / 10;
  if (channel < 0 || channel >= 16) {
    return;
  }

  double targetValue = a->getTargetValue();
  if (targetValue < 0.5) {
    jaroCmd(CMD_UP, (uint8_t)channel);
  } else if (targetValue < 1.5) {
    jaroCmd(CMD_DOWN, (uint8_t)channel);
  } else {
    jaroCmd(CMD_STOP, (uint8_t)channel);
  }
  ESP_LOGI(TAG, "cmd up/down/stop - channel %i - value %.0f", channel + 1, targetValue);

  vhih.updateAttributeValue(a, targetValue);
}

/**
 * *******************************************************************
 * @brief   callback for the shade (3rd position) button attribute
 * @param   changed attribute
 * @return  none
 * *******************************************************************/
static void homeeCallbackShade(nodeAttributes *a) {
  int channel = (a->getId() - HOMEE_ATTR_ID_BASE) / 10;
  if (channel < 0 || channel >= 16) {
    return;
  }

  if (a->getTargetValue() > 0.5) {
    ESP_LOGI(TAG, "cmd shade - channel %i", channel + 1);
    jaroCmd(CMD_SHADE, (uint8_t)channel);
    vhih.updateAttributeValue(a, 1);
    if (homeeUpDownAttr[channel] != nullptr) {
      vhih.updateAttributeValue(homeeUpDownAttr[channel], HOMEE_UP_DOWN_STOP);
    }
    shadeResetPending[channel] = true;
    shadeResetAt[channel] = millis() + HOMEE_SHADE_RESET_DELAY;
  }
}

/**
 * *******************************************************************
 * @brief   Setup for homee integration - creates one node per
 *          configured shutter channel and starts the virtual homee
 * @param   none
 * @return  none
 * *******************************************************************/
void homeeSetup() {
  if (!config.homee.enable) {
    return;
  }

  vhih.setHomeeId(config.wifi.hostname);

  for (int i = 0; i < 16; i++) {
    if (!config.jaro.ch_enable[i]) {
      continue;
    }

    uint32_t profile = config.homee.thirdPosition ? CANodeProfileUnknown : CANodeProfileShutterPositionSwitch;
    node *n = new node(HOMEE_NODE_ID_BASE + i, profile, config.jaro.ch_name[i]);
    n->setImage(NodeIconShutter);

    nodeAttributes *updown = new nodeAttributes(CAAttributeTypeUpDown, HOMEE_ATTR_ID_BASE + i * 10 + 1);
    updown->setEditable(1);
    updown->setMinimumValue(0);
    updown->setMaximumValue(HOMEE_UP_DOWN_STOP);
    updown->setCallback(homeeCallbackUpDown);
    n->AddAttributes(updown);
    homeeUpDownAttr[i] = updown;

    if (config.homee.thirdPosition) {
      nodeAttributes *shade = new nodeAttributes(CAAttributeTypeOpenPartialImpulse, HOMEE_ATTR_ID_BASE + i * 10 + 2);
      shade->setName("Shade");
      shade->setEditable(1);
      shade->setMinimumValue(0);
      shade->setMaximumValue(1);
      shade->setCallback(homeeCallbackShade);
      n->AddAttributes(shade);
      homeeShadeAttr[i] = shade;
    }

    nodeAttributes *firmware = new nodeAttributes(CAAttributeTypeFirmwareRevision, HOMEE_ATTR_ID_BASE + i * 10 + 3);
    firmware->setName("Firmware");
    firmware->setEditable(0);
    firmware->setCallback(nullptr);
    firmware->setCurrentValue(firmwareVersionAsDouble());
    n->AddAttributes(firmware);

    vhih.addNode(n);
    ESP_LOGI(TAG, "node created - channel %i - name %s", i + 1, config.jaro.ch_name[i]);
  }

  vhih.start();
  ESP_LOGI(TAG, "virtual homee started - id %s", config.wifi.hostname);
}

/**
 * *******************************************************************
 * @brief   cyclic function for homee integration - resets the shade
 *          button back to "released" after it has been triggered
 * @param   none
 * @return  none
 * *******************************************************************/
void homeeCyclic() {
  if (!config.homee.enable) {
    return;
  }

  unsigned long now = millis();
  for (int i = 0; i < 16; i++) {
    if (shadeResetPending[i] && (long)(now - shadeResetAt[i]) >= 0) {
      vhih.updateAttributeValue(homeeShadeAttr[i], 0);
      shadeResetPending[i] = false;
    }
  }
}

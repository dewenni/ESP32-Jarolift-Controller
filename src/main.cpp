// includes
#include <ArduinoOTA.h>
#include <esp_ota_ops.h>
#include <basics.h>
#include <config.h>
#include <homee.h>
#include <jarolift.h>
#include <message.h>
#include <mqtt.h>
#include <telnet.h>
#include <webUI.h>
#include <webUIupdates.h>

/* D E C L A R A T I O N S ****************************************************/
static muTimer heartbeat = muTimer();      // timer for heartbeat signal
static muTimer setupModeTimer = muTimer(); // timer for heartbeat signal
static muTimer wdtTimer = muTimer();       // timer to reset wdt

static EspSysUtil::MRD32 *mrd;  // Multi-Reset-Detector
static bool main_reboot = true; // reboot flag

static const char *TAG = "MAIN"; // LOG TAG

static auto &wdt = EspSysUtil::Wdt::getInstance();
static auto &ota = EspSysUtil::OTA::getInstance();

// The core otherwise confirms a pending image before setup() runs.
extern "C" bool verifyRollbackLater() { return true; }

static constexpr uint32_t OTA_VALIDATE_MIN_UPTIME = 120000;
static constexpr uint32_t OTA_VALIDATE_MAX_UPTIME = 600000;
static bool otaImageValidated = false;

static void handleArduinoOTA() {
  static bool running = false;
  static char password[sizeof(config.service.password)] = {};
  static char hostname[sizeof(config.wifi.hostname)] = {};
  if (ota.isActive()) {
    if (running) ArduinoOTA.handle();
    return;
  }
  bool allowed = serviceAccessEnabled();
  const char *desiredHostname = config.wifi.hostname[0] ? config.wifi.hostname : "ESP32-Jarolift-Controller";
  if (running && (!allowed || strcmp(password, servicePassword()) || strcmp(hostname, desiredHostname))) {
    ArduinoOTA.end();
    running = false;
  }
  if (allowed && !running) {
    snprintf(password, sizeof(password), "%s", servicePassword());
    snprintf(hostname, sizeof(hostname), "%s", desiredHostname);
    ArduinoOTA.setHostname(hostname);
    ArduinoOTA.setPassword(password);
    ArduinoOTA.begin();
    running = true;
  }
  if (running) ArduinoOTA.handle();
}

static void markAppValid() {
  if (otaImageValidated || ota.isActive())
    return;
  const uint32_t uptime = millis();
  if (uptime < OTA_VALIDATE_MAX_UPTIME &&
      (uptime < OTA_VALIDATE_MIN_UPTIME || !(wifi.connected || eth.connected)))
    return;

  static uint32_t lastAttempt = 0;
  if (uptime - lastAttempt < 10000)
    return;
  lastAttempt = uptime;
  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t state;
  if (running == nullptr)
    return;
  const esp_err_t result = esp_ota_get_state_partition(running, &state);
  if (result == ESP_ERR_NOT_FOUND || result == ESP_ERR_NOT_SUPPORTED) {
    otaImageValidated = true; // No rollback state on this installation.
  } else if (result == ESP_OK) {
    if (state != ESP_OTA_IMG_PENDING_VERIFY) {
      otaImageValidated = true;
    } else if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
      otaImageValidated = true;
      ESP_LOGI(TAG, "OTA image confirmed - rollback cancelled");
    } else {
      ESP_LOGE(TAG, "failed to confirm OTA image; will retry");
    }
  }
}

/**
 * *******************************************************************
 * @brief   Main Setup routine
 * @param   none
 * @return  none
 * *******************************************************************/
void setup() {

  // Message Service Setup (before use of MY_LOGx)
  messageSetup();

  // check for double reset
  mrd = new EspSysUtil::MRD32(MRD_TIMEOUT, MRD_RETRIES);
  if (mrd->detectMultipleResets()) {
    ESP_LOGI(TAG, "SETUP-MODE-REASON: MRD detected");
    setupMode = true;
  }

  // initial configuration (can also activate the Setup Mode)
  configSetup();

  // setup watchdog timer
  if (!setupMode) {
    wdt.enable();
  }

  // basic setup functions
  basicSetup();

  // Setup OTA
  ArduinoOTA.onStart([]() {
    ESP_LOGI(TAG, "OTA-started");
    wdt.disable(); // disable watchdog timer
    ota.setActive(true);
  });
  ArduinoOTA.onEnd([]() {
    ESP_LOGI(TAG, "OTA-finished");
    if (!setupMode) {
      wdt.enable();
    }
    ota.setActive(false);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    ESP_LOGI(TAG, "OTA-error");
    if (!setupMode) {
      wdt.enable();
    }
    ota.setActive(false);
  });
  handleArduinoOTA();

  // jarolift setup
  if (!setupMode) {
    jaroliftSetup();
    homeeSetup();
  }
  // webUI Setup
  webUISetup();

  // telnet Setup
  setupTelnet();

}

/**
 * *******************************************************************
 * @brief   Main Loop
 * @param   none
 * @return  none
 * *******************************************************************/
void loop() {

  // reset watchdog
  if (wdt.isActive() && wdtTimer.cycleTrigger(2000)) {
    esp_task_wdt_reset();
  }

  // Remote OTA uses the independent service password.
  handleArduinoOTA();

  // double reset detector
  mrd->loop();

  markAppValid();

  // webUI Cyclic
  webUICyclic();

  // Message Service
  messageCyclic();

  // telnet communication
  cyclicTelnet();

  // check if config has changed
  configCyclic();

  // jarolift code
  if (!setupMode) {
    jaroliftCyclic();
    homeeCyclic();
  }

  // WiFi scans also run in setup mode; reconnect is handled internally.
  wifiCyclic();

  // check MQTT - automatic reconnect
  if (config.mqtt.enable && !setupMode) {
    mqttCyclic();
  }

  if (setupMode) {
    // LED to Signal Setup-Mode
    if (config.gpio.led_setup <= 0) {
      digitalWrite(LED_BUILTIN, setupModeTimer.cycleOnOff(100, 500));
    } else {
      digitalWrite(config.gpio.led_setup, setupModeTimer.cycleOnOff(100, 500));
    }
  }

  main_reboot = false; // reset reboot flag
}

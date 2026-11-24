#include <LittleFS.h>
#include <Update.h>
#include <atomic>
#include <basics.h>
#include <language.h>
#include <message.h>
#include <webUI.h>
#include <shutterPosition.h>
#include <webUIupdates.h>

const int MAX_WS_CLIENT = 3;
const int CHUNK_SIZE = 1024;

/* P R O T O T Y P E S ********************************************************/
void webCallback(const char *elementId, const char *value);

/* D E C L A R A T I O N S ****************************************************/
static muTimer heartbeatTimer = muTimer(); // timer to refresh other values
static muTimer onLoadTimer = muTimer();    // timer to refresh other values

EspWebUI webUI(80);

static const char *TAG = "WEB"; // LOG TAG
static bool webInitDone = false;
static const size_t BUFFER_SIZE = 512;
static bool onLoadRequest = false;

// AsyncTCP produces commands; loop consumes complete copies under one lock.
struct WebCallbackMessage {
  char elementID[32];
  char value[256];
};
static constexpr uint8_t WEB_CALLBACK_QUEUE_LEN = 8;
static WebCallbackMessage webCallbackQueue[WEB_CALLBACK_QUEUE_LEN];
static uint8_t webCallbackHead = 0, webCallbackTail = 0;
static portMUX_TYPE webCallbackMux = portMUX_INITIALIZER_UNLOCKED;

static bool queueWebCommand(const char *elementID, const char *elementValue) {
  if (!elementID || !elementValue || strlen(elementID) >= sizeof(webCallbackQueue[0].elementID) ||
      strlen(elementValue) >= sizeof(webCallbackQueue[0].value)) return false;
  portENTER_CRITICAL(&webCallbackMux);
  uint8_t next = (webCallbackHead + 1) % WEB_CALLBACK_QUEUE_LEN;
  bool available = next != webCallbackTail;
  if (available) {
    snprintf(webCallbackQueue[webCallbackHead].elementID, sizeof(webCallbackQueue[0].elementID), "%s", elementID);
    snprintf(webCallbackQueue[webCallbackHead].value, sizeof(webCallbackQueue[0].value), "%s", elementValue);
    webCallbackHead = next;
  }
  portEXIT_CRITICAL(&webCallbackMux);
  return available;
}

static bool dequeueWebCallback(WebCallbackMessage &msg) {
  portENTER_CRITICAL(&webCallbackMux);
  const bool available = webCallbackTail != webCallbackHead;
  if (available) {
    msg = webCallbackQueue[webCallbackTail];
    webCallbackTail = (webCallbackTail + 1) % WEB_CALLBACK_QUEUE_LEN;
  }
  portEXIT_CRITICAL(&webCallbackMux);
  return available;
}

// Only browser uploads participate in this timeout, never other OTA paths.
static constexpr uint32_t OTA_STALL_TIMEOUT = 60000;
static std::atomic<bool> webOtaActive{false};
static std::atomic<bool> webOtaRestoreWdt{false};
static std::atomic<uint32_t> webOtaLastActivity{0};
static size_t webOtaLastProgress = 0;

static auto &wdt = EspSysUtil::Wdt::getInstance();
static auto &ota = EspSysUtil::OTA::getInstance();

void applyWebAccessSettings() {
  webUI.setCredentials(deviceUsername(), devicePassword());
  webUI.setAuthentication(setupMode || config.auth.enable);
}

static void handleWebOta(EspWebUI::otaStatus otaState, const char *msg) {
  switch (otaState) {
  case EspWebUI::OTA_BEGIN:
    webOtaRestoreWdt = wdt.isActive();
    webOtaLastActivity = millis();
    ota.setActive(true);
    wdt.disable();
    webOtaActive = true;
    break;
  case EspWebUI::OTA_PROGRESS:
    webOtaLastActivity = millis();
    webUI.wsUpdateOTAprogress(msg);
    break;
  case EspWebUI::OTA_FINISH:
    webOtaActive = false;
    ota.setActive(false);
    if (webOtaRestoreWdt.exchange(false))
      wdt.enable();
    webUI.wsUpdateOTAprogress("100");
    webUI.wsUpdateWebDialog("ota_update_done_dialog", "open");
    break;
  case EspWebUI::OTA_ERROR:
    webOtaActive = false;
    ota.setActive(false);
    if (webOtaRestoreWdt.exchange(false))
      wdt.enable();
    webUI.wsUpdateWebText("p00_ota_upd_err", msg, false);
    webUI.wsUpdateWebDialog("ota_update_failed_dialog", "open");
    break;
  }
}

static void checkWebOtaTimeout() {
  if (webOtaActive) {
    // EspWebUI reports whole-percent progress only. Observe bytes as well so
    // a slow upload that is still making progress is not mistaken for a stall.
    const size_t progress = Update.progress();
    if (progress != webOtaLastProgress) {
      webOtaLastProgress = progress;
      webOtaLastActivity = millis();
    }
    if (static_cast<uint32_t>(millis() - webOtaLastActivity.load()) > OTA_STALL_TIMEOUT &&
        webOtaActive.exchange(false)) {
      ESP_LOGE(TAG, "Web OTA stalled - aborting upload");
      Update.abort();
      ota.setActive(false);
      if (webOtaRestoreWdt.exchange(false))
        wdt.enable();
      webUI.wsUpdateWebText("p00_ota_upd_err", "upload aborted (timeout)", false);
      webUI.wsUpdateWebDialog("ota_update_failed_dialog", "open");
    }
  } else {
    webOtaLastProgress = 0;
  }
}

/**
 * *******************************************************************
 * @brief   cyclic call for webUI - creates all webUI elements
 * @param   none
 * @return  none
 * *******************************************************************/
void webUISetup() {
  webUI.setConfigFileSection("jaro");
  webUI.setCallbackConfigFileReady([]() { return !ota.isActive() && !shutterPositionBusy(); });


  webUI.setCallbackOta(handleWebOta);

  webUI.setCallbackUpload([](EspWebUI::uploadStatus uploadState, const char *msg) {
    switch (uploadState) {
    case EspWebUI::UPLOAD_BEGIN:
      webUI.wsUpdateWebText("upload_status_txt", msg, false);
      break;
    case EspWebUI::UPLOAD_FINISH:
      // Executed on the loop task after the checked file was committed.
      // Reboot before configCyclic can save the old in-memory configuration.
      EspSysUtil::RestartReason::saveLocal("configuration restored/imported");
      delay(300);
      ESP.restart();
      break;
    case EspWebUI::UPLOAD_ERROR:
      webUI.wsUpdateWebText("upload_status_txt", msg, false);
      break;
    }
  });

  // callback for reload
  webUI.setCallbackReload([]() { onLoadRequest = true; });

  webUI.setCallbackWebElement([](const char *elementID, const char *elementValue) {
    if (!queueWebCommand(elementID, elementValue))
      ESP_LOGE(TAG, "web callback queue full or invalid input - command dropped");
  });

  applyWebAccessSettings();

  webUI.begin();
} // END SETUP

/**
 * *******************************************************************
 * @brief   cyclic call for webUI - refresh elements by change
 * @param   none
 * @return  none
 * *******************************************************************/
void webUICyclic() {

  webUI.loop();

  // request for update alle elements - not faster than every 1s
  if (onLoadRequest && onLoadTimer.cycleTrigger(1000)) {
    updateAllElements();
    onLoadRequest = false;
    ESP_LOGD(TAG, "updateAllElements()");
  }

  // handling of update webUI elements
  webUIupdates();

  // Bound work per iteration so other services still run during command bursts.
  WebCallbackMessage msg;
  for (uint8_t i = 0; i < WEB_CALLBACK_QUEUE_LEN && !ota.isActive() && dequeueWebCallback(msg); ++i)
    webCallback(msg.elementID, msg.value);

  checkWebOtaTimeout();

  webInitDone = true; // init done
}

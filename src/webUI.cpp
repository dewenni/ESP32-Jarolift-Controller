#include <LittleFS.h>
#include <Update.h>
#include <basics.h>
#include <language.h>
#include <message.h>
#include <webUI.h>
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

static char webCallbackElementID[32];
static char webCallbackValue[256];
static bool webCallbackAvailable = false;

static auto &wdt = EspSysUtil::Wdt::getInstance();
static auto &ota = EspSysUtil::OTA::getInstance();

/**
 * *******************************************************************
 * @brief   cyclic call for webUI - creates all webUI elements
 * @param   none
 * @return  none
 * *******************************************************************/
void webUISetup() {

  webUI.setCallbackOta([](EspWebUI::otaStatus otaState, const char *msg) {
    switch (otaState) {
    case EspWebUI::OTA_BEGIN:
      webUI.wsUpdateWebText("p00_ota_upd_err", msg, false);
      webUI.wsUpdateWebDialog("ota_update_failed_dialog", "open");
      break;
    case EspWebUI::OTA_PROGRESS:
      webUI.wsUpdateOTAprogress(msg);
      break;
    case EspWebUI::OTA_FINISH:
      webUI.wsUpdateOTAprogress("100");
      webUI.wsUpdateWebDialog("ota_update_done_dialog", "open");
      break;
    case EspWebUI::OTA_ERROR:
      webUI.wsUpdateWebText("p00_ota_upd_err", msg, false);
      webUI.wsUpdateWebDialog("ota_update_failed_dialog", "open");
      break;
    }
  });

  // callback for reload
  webUI.setCallbackReload([]() { onLoadRequest = true; });

  // callback for web elements - copy elementID and value and call webCallback in cyclic loop
  webUI.setCallbackWebElement([](const char *elementID, const char *elementValue) {
    snprintf(webCallbackElementID, sizeof(webCallbackElementID), "%s", elementID);
    snprintf(webCallbackValue, sizeof(webCallbackValue), "%s", elementValue);
    webCallbackAvailable = true;
  });

  webUI.begin();
} // END SETUP

/**
 * *******************************************************************
 * @brief   cyclic call for webUI - refresh elements by change
 * @param   none
 * @return  none
 * *******************************************************************/
void webUICyclic() {

  // request for update alle elements - not faster than every 1s
  if (onLoadRequest && onLoadTimer.cycleTrigger(1000)) {
    updateAllElements();
    onLoadRequest = false;
    ESP_LOGD(TAG, "updateAllElements()");
  }

  // handling of update webUI elements
  webUIupdates();

  // handling of callback infomation
  if (webCallbackAvailable) {
    webCallback(webCallbackElementID, webCallbackValue);
    webCallbackAvailable = false;
  }

  webInitDone = true; // init done
}

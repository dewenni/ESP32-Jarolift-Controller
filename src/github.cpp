#include <Arduino.h>
#include <basics.h>
#include <github.h>
#include <message.h>

#define GITHUB_OWNER "dewenni"
#define GITHUB_REPO "ESP32-Jarolift-Controller"

static const char *TAG = "GITHUB"; // LOG TAG

GithubReleaseOTA ota(GITHUB_OWNER, GITHUB_REPO);

/**
 * *******************************************************************
 * @brief   check if the asset name contains the chip series
 * @param   assetName
 * @param   espSeries
 * @return  true if the asset name contains the chip series, else false
 * *******************************************************************/
bool isChipSeriesMatch(const char *assetName, const char *espSeries) {
  if (assetName == NULL || espSeries == NULL)
    return false;

  char formattedEspSeries[16];
  size_t j = 0;
  for (size_t i = 0; espSeries[i] != '\0' && j < sizeof(formattedEspSeries) - 1; i++) {
    if (espSeries[i] != '-') { // remove '-' from chip series
      formattedEspSeries[j++] = tolower((unsigned char)espSeries[i]);
    }
  }
  formattedEspSeries[j] = '\0';

  // check if the asset name contains the chip series
  size_t len = strlen(formattedEspSeries);
  return (strncasecmp(assetName, formattedEspSeries, len) == 0 && assetName[len] == '_');
}

/**
 * *******************************************************************
 * @brief   set the progress callback
 * @param   callback
 * @return  none
 * *******************************************************************/
void ghSetProgressCallback(void (*callback)(int)) { ota.setProgressCallback(callback); }

/**
 * *******************************************************************
 * @brief   get the latest release from GitHub
 * @param   release
 * @param   info
 * @param   espSeries
 * @return  true if successful, else false
 * *******************************************************************/
bool ghGetLatestRelease(GithubRelease *release, GithubReleaseInfo *info, const char *espSeries) {

  if (release == nullptr || info == nullptr) {
    return false;
  }

  // Get the latest release from GitHub
  *release = ota.getLatestRelease();

  // check if the release is valid
  if (release->tag_name == nullptr || release->html_url == nullptr) {
    return false;
  }

  snprintf(info->tag, sizeof(info->tag), "%s", release->tag_name);
  snprintf(info->url, sizeof(info->url), "%s", release->html_url);
  ESP_LOGD(TAG, "GitHub latest Release: %s", info->tag);

  // search for the first asset that contains the right "chipSeries" and "ota or UPDATE" in its name
  info->assetFound = false;
  for (const auto &asset : release->assets) {
    if (isChipSeriesMatch(asset.name, espSeries) && strcasestr(asset.name, "ota") != NULL) {
      ESP_LOGD(TAG, "GitHub OTA Asset found: %s", asset.name);
      snprintf(info->asset, sizeof(info->asset), "%s", asset.name);
      info->assetFound = true;
      break;
    }
  }

  return true;
}

/**
 * *******************************************************************
 * @brief   start the OTA update
 * @param   release
 * @param   asset
 * @return  0 if successful, else error code
 * *******************************************************************/
int ghStartOtaUpdate(GithubRelease release, const char *asset) {
  int result = ota.flashFirmware(release, asset);

  if (result == 0) {
    ESP_LOGI(TAG, "Firmware updated successfully");
  } else {
    ESP_LOGE(TAG, "Firmware update failed: %i", result);
    ota.freeRelease(release);
  }
  return result;
}

/**
 * *******************************************************************
 * @brief   free the release memory
 * @param   release
 * @return  none
 * *******************************************************************/
void ghFreeRelease(GithubRelease &release) { ota.freeRelease(release); }
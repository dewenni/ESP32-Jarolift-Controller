#include <Arduino.h>

#include <basics.h>
#include <github.h>
#include <message.h>

#define GITHUB_OWNER "dewenni"
#define GITHUB_REPO "ESP32-Jarolift-Controller"

static const char *TAG = "GITHUB"; // LOG TAG

GithubReleaseOTA ota(GITHUB_OWNER, GITHUB_REPO);

void ghSetProgressCallback(void (*callback)(int)) { ota.setProgressCallback(callback); }

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
    if (asset.name != NULL && strcasestr(asset.name, espSeries) != NULL && strcasestr(asset.name, "UPDATE") != NULL) {
      ESP_LOGD(TAG, "GitHub OTA Asset found: %s", asset.name);
      snprintf(info->asset, sizeof(info->asset), "%s", asset.name);
      info->assetFound = true;
      break;
    }
  }

  return true;
}

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
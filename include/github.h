#include <GithubReleaseOTA.h>

struct GithubReleaseInfo {
  char tag[12];
  char asset[128];
  char url[256];
  bool assetFound;
};

bool ghGetLatestRelease(GithubRelease *release, GithubReleaseInfo *info, const char *espSeries);
int ghStartOtaUpdate(GithubRelease release, const char *asset);
void ghSetProgressCallback(void (*callback)(int));
void ghFreeRelease(GithubRelease &release);
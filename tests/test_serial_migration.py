"""Run configuration migration and the serial WebSocket callback with a fake filesystem."""
from pathlib import Path
import re
import subprocess
import tempfile

from host_helpers import ROOT, arduino_json_headers


ARDUINO = r"""
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#define ESP_LOGD(...) ((void)0)
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
constexpr int LED_BUILTIN = 2, OUTPUT = 1;
inline void pinMode(int, int) {}
"""

LITTLEFS = r"""
#pragma once
#include <algorithm>
#include <map>
#include <string>

constexpr char FILE_READ[] = "r", FILE_WRITE[] = "w";
struct File {
  std::string *data = nullptr;
  bool partialWrite = false;
  size_t offset = 0;
  explicit operator bool() const { return data != nullptr; }
  size_t size() const { return data->size(); }
  int read() { return offset < size() ? uint8_t((*data)[offset++]) : -1; }
  size_t readBytes(char *out, size_t count) {
    count = std::min(count, size() - offset);
    data->copy(out, count, offset);
    offset += count;
    return count;
  }
  size_t write(const uint8_t *bytes, size_t count) {
    if (partialWrite && count)
      --count;
    data->append(reinterpret_cast<const char *>(bytes), count);
    return count;
  }
  size_t write(uint8_t byte) { return write(&byte, 1); }
  void flush() {}
  void close() {}
};
struct FakeFS {
  std::map<std::string, std::string> files;
  bool failWriteOpen = false, partialWrite = false, failRename = false;
  unsigned renames = 0;
  bool begin(bool) { return true; }
  File open(const char *path, const char *mode) {
    if (*mode == 'w') {
      if (failWriteOpen)
        return {};
      return {&(files[path] = ""), partialWrite};
    }
    auto it = files.find(path);
    return it == files.end() ? File{} : File{&it->second};
  }
  bool rename(const char *source, const char *target) {
    auto it = files.find(source);
    if (failRename || it == files.end())
      return false;
    files[target] = it->second;
    files.erase(it);
    ++renames;
    return true;
  }
  void remove(const char *path) { files.erase(path); }
};
inline FakeFS LittleFS;
"""

BASICS = r"""
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
struct muTimer { bool cycleTrigger(unsigned) { return true; } };
namespace EspStrUtil {
inline unsigned long hash(const void *data, size_t size) {
  unsigned long result = 2166136261;
  const auto *bytes = static_cast<const uint8_t *>(data);
  for (size_t i = 0; i < size; ++i)
    result = (result ^ bytes[i]) * 16777619;
  return result;
}
inline bool copy(const char *input, const uint8_t *, char *output, size_t capacity) {
  return snprintf(output, capacity, "%s", input) < capacity;
}
inline auto encryptPassword = copy;
inline auto decryptPassword = copy;
inline void readJSONstring(char *output, size_t capacity, const char *text) {
  snprintf(output, capacity, "%s", text ? text : "");
}
}
"""

MAIN = r"""
#include <config.h>
#include <language.h>
#include <LittleFS.h>
#include <cassert>
#include <cstdlib>
#include <initializer_list>

constexpr char configPath[] = "/config.json", tempPath[] = "/config.json.tmp";
constexpr char uploadPath[] = "/config.json.upload";
static bool busy = false;
static unsigned radioInitializations = 0;
static uint8_t srvShutter = 0;
static JsonDocument mappings;
static int loadedMappingVersion = 0;

bool shutterPositionBusy() { return busy; }
void setLogLevel(uint8_t) {}
void remoteMappingLoad(JsonVariantConst data, int version) {
  loadedMappingVersion = version;
  mappings.set(data);
}
void remoteMappingSave(JsonObject root) { root["remoteMappings"].set(mappings.as<JsonVariantConst>()); }
void jaroCmdReInit() { ++radioInitializations; }
struct WebSocket {
  std::string inputValue;
  unsigned errors = 0;
  void wsUpdateWebText(const char *, const char *text, bool) { inputValue = text; }
  void wsUpdateWebText(const char *, long value, bool) { inputValue = std::to_string(value); }
  void wsShowInfoMsg(const char *) { ++errors; }
} webUI;

void webCallback(const char *elementId, const char *value) {
CALLBACKS
}

void put(const char *path, int version, uint32_t serial) {
  JsonDocument doc;
  doc["version"] = version;
  doc["jaro"]["serial"] = serial;
  doc["jaro"]["masterMSB"] = 0x23456789;
  doc["jaro"]["masterLSB"] = 0x87654321;
  doc["jaro"]["learn_mode"] = true;
  doc["jaro"]["ch_enable"][2] = true;
  doc["jaro"]["grp_mask"][0] = 0x8084;
  doc["remoteMappings"][0]["serial"] = 0x01234567;
  auto &file = LittleFS.files[path];
  file.clear();
  assert(serializeJson(doc, file));
}

void expectFile(const char *path, int version, uint32_t serial) {
  JsonDocument doc;
  assert(!deserializeJson(doc, LittleFS.files.at(path)));
  assert(doc["version"] == version && doc["jaro"]["serial"] == serial);
}

void expectMigration(int version, uint32_t oldSerial, uint32_t newSerial) {
  put(configPath, version, oldSerial);
  unsigned renames = LittleFS.renames;
  configLoadFromFile();
  assert(config.version == 8 && config.jaro.serial == newSerial);
  assert(config.position.reference);
  assert(config.jaro.masterMSB == 0x23456789 && config.jaro.masterLSB == 0x87654321);
  assert(config.jaro.learn_mode && config.jaro.ch_enable[2] && config.jaro.grp_mask[0] == 0x8084);
  assert(mappings[0]["serial"] == 0x01234567 && loadedMappingVersion == version);
  expectFile(configPath, 8, newSerial);
  assert(LittleFS.renames == renames + 1);
  configLoadFromFile();
  configCyclic();
  assert(config.version == 8 && config.jaro.serial == newSerial);
  assert(LittleFS.renames == renames + 1);
}

int main() {
  configLoadFromFile(); // Fresh installation with no file.
  assert(setupMode && config.version == 8 && config.jaro.serial == 0);
  expectFile(configPath, 8, 0);
  assert(config.position.reference); // Missing field keeps the established reference behavior.
  config.position.reference = false;
  configSaveToFile();
  config.position.reference = true;
  configLoadFromFile();
  assert(!config.position.reference);
  JsonDocument savedPosition;
  assert(!deserializeJson(savedPosition, LittleFS.files.at(configPath)));
  assert(savedPosition["position"]["reference"] == false);
  config.jaro.serial = 0xABCDEF;
  configCyclic();
  configLoadFromFile();
  expectFile(configPath, 8, 0xABCDEF);

  expectMigration(0, 0x012345, 0x123450);
  expectMigration(6, 0x0ABCDE, 0xABCDE0);
  expectMigration(7, 0x000000, 0x000000);
  expectMigration(7, 0x000001, 0x000010);
  expectMigration(7, 0x0FFFFF, 0xFFFFF0);
  expectMigration(7, 0x100000, 0x000000);
  expectMigration(7, 0x12DEAF, 0x2DEAF0);
  expectMigration(7, 0xFFFFFF, 0xFFFFF0);

  // Import replaces the main file before calling this same load entry point.
  expectMigration(7, 0xABCDEF, 0xBCDEF0);
  for (bool FakeFS::*failure : {&FakeFS::failWriteOpen, &FakeFS::partialWrite, &FakeFS::failRename}) {
    put(configPath, 7, 0x012345);
    LittleFS.*failure = true;
    configLoadFromFile();
    assert(config.version == 8 && config.jaro.serial == 0x123450);
    expectFile(configPath, 7, 0x012345);
    // Another boot/load reads the old file and applies its migration once.
    configLoadFromFile();
    assert(config.jaro.serial == 0x123450);
    LittleFS.*failure = false;
    configCyclic();
    expectFile(configPath, 8, 0x123450);
    assert(!LittleFS.files.count(tempPath));
  }

  for (const char *path : {tempPath, uploadPath}) {
    for (int version : {7, 8}) {
      LittleFS.files[configPath] = "{ interrupted write";
      put(path, version, 0x001234);
      LittleFS.failRename = true;
      configLoadFromFile();
      const uint32_t serial = version == 7 ? 0x012340 : 0x001234;
      assert(config.version == 8 && config.jaro.serial == serial);
      expectFile(path, version, 0x001234); // Only valid source survives a failed rename.
      LittleFS.failRename = false;
      configCyclic();
      expectFile(configPath, 8, serial);
      assert(!LittleFS.files.count(tempPath) && !LittleFS.files.count(uploadPath));
    }
  }
  put(configPath, 7, 0x100000);
  busy = true;
  configLoadFromFile();
  expectFile(configPath, 7, 0x100000);
  busy = false;
  configCyclic();
  expectFile(configPath, 8, 0);

  webCallback("cfg_jaro_serial", "abc");
  assert(config.jaro.serial == 0xABC && radioInitializations == 1);
  for (const char *bad : {"", "0x123", " 001", "-0001", "+123", "123 ", "1000000", "0000000", "aBcD1g"}) {
    webCallback("cfg_jaro_serial", bad);
    assert(config.jaro.serial == 0xABC && radioInitializations == 1 && webUI.inputValue == "000abc");
  }
  assert(webUI.errors == 9);
  webCallback("cfg_jaro_serial", "fFfFfF");
  assert(config.jaro.serial == 0xFFFFFF);
  webCallback("cfg_jaro_serial", "000000");
  assert(config.jaro.serial == 0 && radioInitializations == 3);
  configCyclic();
  configLoadFromFile();
  expectFile(configPath, 8, 0);

  webCallback("p04_selected_shutter", "15");
  for (const char *bad : {"", "16", "256", "-256", "9999999999999999999999999", "1x"}) {
    webCallback("p04_selected_shutter", bad);
    assert(srvShutter == 15 && webUI.inputValue == "15");
  }
  webCallback("p04_selected_shutter", "0");
  assert(srvShutter == 0);
  puts("PASS: actual config migration, import, retries and recovery, zero serial and WebSocket validation.");
}
"""


def check_browser_input():
    html = (ROOT / "web/html/12_settings.html").read_text()
    field = re.search(r'<input\b[^>]*\bid="cfg_jaro_serial"[^>]*>', html).group()
    assert 'data-length="6"' in field and 'pattern="[0-9a-fA-F]{1,6}"' in field
    assert not re.search(r'\b(oninput|maxlength)=', field)
    handler = re.search(r'\bonblur="([^"]*)"', field).group(1)
    subprocess.run(["node", "-e", r"""
      const assert = require("node:assert/strict");
      const onBlur = new Function("formatHex", process.argv[1]);
      function exercise(value, accepted) {
        const input = { value };
        let formatted = false;
        onBlur.call(input, field => {
          formatted = true;
          field.value = field.value.padStart(6, "0").toLowerCase();
        });
        assert.equal(formatted, accepted);
        assert.equal(input.value, accepted ? value.padStart(6, "0").toLowerCase() : value);
      }
      for (const value of ["", "0x123", "abg", "1234567", " 1", "+123", "123 "])
        exercise(value, false);
      for (const value of ["0", "A", "Ab", "AbC", "00aB", "0ABCD", "FfFfFf"])
        exercise(value, true);
      console.log("PASS: serial field only formats valid input and passes invalid values through.");
    """, handler], check=True)


def main():
    check_browser_input()
    source = (ROOT / "src/webUIcallback.cpp").read_text()
    callbacks = []
    for element in ("cfg_jaro_serial", "p04_selected_shutter"):
        start = source.index(f'  if (strcmp(elementId, "{element}") == 0) {{')
        stop = source.index("\n  }", start) + len("\n  }")
        callbacks.append(source[start:stop])

    with tempfile.TemporaryDirectory(prefix="jarolift-config-") as directory:
        path = Path(directory)
        (path / "Arduino.h").write_text(ARDUINO)
        (path / "LittleFS.h").write_text(LITTLEFS)
        (path / "basics.h").write_text(BASICS)
        (path / "shutterPosition.h").write_text("#pragma once\nbool shutterPositionBusy();\n")
        (path / "main.cpp").write_text(MAIN.replace("CALLBACKS", "\n".join(callbacks)))
        binary = path / "test"
        subprocess.run([
            "c++", "-std=c++17", f"-I{path}", f"-I{ROOT / 'include'}",
            f"-I{arduino_json_headers()}", str(ROOT / "src/config.cpp"),
            str(path / "main.cpp"), "-o", str(binary),
        ], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()

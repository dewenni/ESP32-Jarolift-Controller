"""Compare actual Jarolift TX framing with the legacy-compatible reference."""
from pathlib import Path
import subprocess
import tempfile

from host_helpers import ROOT


LIB = ROOT / "lib/JaroliftController"
METHODS = (
    "JaroliftController", "~JaroliftController", "setBaseSerial", "getSerial",
    "setChannelSerial", "setKeys", "setGPIO", "txAddLevel", "radioTxFrame", "radioTxGroupH",
    "radioTx", "generateKey", "generateEncrypted", "cmdChannel", "cmdGroup",
    "cmdLearn", "cmdUnlearn", "cmdSetEndPointUp", "cmdDeleteEndPointUp",
    "cmdSetEndPointDown", "cmdDeleteEndPointDown",
)


def extract_method(source, method):
    signature = source.index(f"JaroliftController::{method}(")
    start = source.rfind("\n", 0, signature) + 1
    end = source.index("\n}", signature) + len("\n}")
    if method == "~JaroliftController":
        end = source.index("\n", signature)
    return source[start:end]


CODE = r"""
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <vector>

#define IRAM_ATTR
#define ESP_LOGD(...) ((void)0)
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
#define bitRead(value, bit) (((value) >> (bit)) & 1u)
using portMUX_TYPE = int;
constexpr int portMUX_INITIALIZER_UNLOCKED = 0;
constexpr int HIGH = 1, LOW = 0;
constexpr int ESP32_BUS_TYPE_RMT_TX = 1, CC1101_MARCSTATE = 0, RMT_WAIT_FOR_EVER = 0;
struct rmt_data_t { uint16_t duration0, duration1; uint8_t level0, level1; };
struct CC1101 { int readStatusReg(int) { return 0; } };
std::vector<rmt_data_t> symbols;
static unsigned writes = 0;
static uint16_t counter = 0;
static constexpr uint16_t testCounter = 0x5A39;
static constexpr uint32_t masterMSB = 0x12345678, masterLSB = 0xABCDEF01;
int perimanGetPinBusType(int) { return ESP32_BUS_TYPE_RMT_TX; }
bool rmtWrite(int, const rmt_data_t *data, size_t count, int) {
  symbols.assign(data, data + count);
  ++writes;
  return true;
}
int millis() { return 1000; }
void delay(int) {}

CRYPTO
HEADER
METHODS

JaroliftController *JaroliftController::instance_ = nullptr;
void JaroliftController::initRmt() { rmtOK_ = true; }
void JaroliftController::enterTx() {}
void JaroliftController::enterRx() {}
uint16_t JaroliftController::getDeviceCounter() { return counter; }
void JaroliftController::updateDeviceCounter(bool increment) { counter += increment; }

void checkFrame(const JaroliftController &radio, uint32_t legacySerial, uint16_t mask, uint8_t function) {
  Keeloq master(masterMSB, masterLSB);
  const uint32_t keyMSB = master.decrypt(legacySerial | 0x60000000);
  const uint32_t keyLSB = master.decrypt(legacySerial | 0x20000000);
  const uint16_t discriminator = ((mask & 0xFF) << 8) | (legacySerial & 0xFF);
  Keeloq device(keyMSB, keyLSB);
  const uint32_t hop = device.encrypt((uint32_t(discriminator) << 16) | testCounter);
  const uint64_t fixed = (uint64_t(function) << 60) | (uint64_t(legacySerial) << 32) | hop;
  assert(radio.lastTxOK() && counter == testCounter + 1);
  assert(radio.newSerial_ == legacySerial && radio.disc_ == discriminator);
  assert(uint32_t(radio.deviceKeyMSB_) == keyMSB && uint32_t(radio.deviceKeyLSB_) == keyLSB);
  assert(radio.pack_ == fixed && symbols.size() == 86);
  // One initial half-symbol, thirteen sync pairs and one long HIGH precede the 72 data bits.
  for (unsigned bit = 0; bit < 72; ++bit) {
    const auto &symbol = symbols[14 + bit];
    const bool one = bit < 64 ? ((fixed >> bit) & 1) : ((mask >> (bit - 56)) & 1);
    assert(symbol.level0 == LOW && symbol.level1 == HIGH);
    assert(symbol.duration0 == (one ? 400 : 800) && symbol.duration1 == (one ? 800 : 400));
  }
}

int main() {
  JaroliftController radio;
  radio.setGPIO(0, 0, 0, 0, 0, 0);
  radio.initOK_ = radio.rmtOK_ = true;
  radio.setKeys(masterMSB, masterLSB);
  const JaroliftController::commands commands[] = {
      JaroliftController::CMD_UP, JaroliftController::CMD_DOWN, JaroliftController::CMD_STOP};
  const uint8_t functions[] = {8, 2, 4};

  for (uint32_t oldBase : {0u, 1u, 0x012345u, 0x0ABCDEu, 0x0FFFFFu, 0x100000u, 0x12DEAFu, 0xABCDEFu, 0xFFFFFFu}) {
    const uint32_t newBase = (oldBase & 0xFFFFF) << 4;
    radio.setBaseSerial(newBase);
    for (uint8_t channel = 0; channel < 16; ++channel) {
      const uint32_t legacySerial = ((oldBase & 0xFFFFF) << 8) | channel;
      assert(radio.getSerial(channel) == legacySerial);
      for (unsigned cmd = 0; cmd < 3; ++cmd) {
        counter = testCounter;
        radio.cmdChannel(commands[cmd], channel);
        checkFrame(radio, legacySerial, uint16_t(1) << channel, functions[cmd]);
      }
    }
    for (uint16_t mask : {0x0022, 0x8480, 0xFFFF}) {
      for (unsigned cmd = 0; cmd < 3; ++cmd) {
        counter = testCounter;
        radio.cmdGroup(commands[cmd], mask);
        checkFrame(radio, (oldBase & 0xFFFFF) << 8, mask, functions[cmd]);
      }
    }
  }

  // The ignored high legacy nibble cannot affect any of these equivalence classes.
  for (uint32_t effectiveBase = 0; effectiveBase <= 0xFFFFF; ++effectiveBase) {
    radio.setBaseSerial(effectiveBase << 4);
    for (uint8_t channel = 0; channel < 16; ++channel)
      assert(radio.getSerial(channel) == ((effectiveBase << 8) | channel));
  }

  // Native v8 values use all 24 base bits, including the low nibble.
  radio.setBaseSerial(0xFFFFFF);
  assert(radio.getSerial(0) == 0xFFFFFF0 && radio.getSerial(15) == 0xFFFFFFF);
  radio.setBaseSerial(0);
  assert(radio.getSerial(0) == 0);
  const unsigned before = writes;
  const uint8_t invalidChannels[] = {16, 255};
  for (uint8_t bad : invalidChannels) {
    assert(radio.getSerial(bad) == UINT32_MAX);
    radio.cmdChannel(JaroliftController::CMD_UP, bad);
    radio.cmdLearn(bad);
    radio.cmdUnlearn(bad);
    radio.cmdSetEndPointUp(bad);
    radio.cmdDeleteEndPointUp(bad);
    radio.cmdSetEndPointDown(bad);
    radio.cmdDeleteEndPointDown(bad);
    assert(writes == before && !radio.lastTxOK() && counter == testCounter + 1);
  }
  puts("PASS: all 16 TX serials, keys, discriminators, legacy-compatible command frames and invalid channels.");
}
"""


def main():
    source = (LIB / "JaroliftController.cpp").read_text()
    header = (LIB / "JaroliftController.h").read_text()
    header = header[header.index("class JaroliftController"):header.index("#endif")]
    header = header.replace("private:", "public:").replace("static constexpr char *TAG", "static constexpr const char *TAG")
    crypto_header = (LIB / "KeeloqLib.h").read_text()
    crypto = crypto_header[crypto_header.index("class Keeloq"):]
    crypto += "\n" + (LIB / "KeeloqLib.cpp").read_text().replace('#include "KeeloqLib.h"', "")
    methods = "\n".join(extract_method(source, name) for name in METHODS)
    code = CODE.replace("CRYPTO", crypto).replace("HEADER", header).replace("METHODS", methods)
    code = code.replace("unsigned long", "uint32_t") # Match ESP KeeLoq word size.
    with tempfile.TemporaryDirectory(prefix="jarolift-tx-") as directory:
        path = Path(directory)
        (path / "test.cpp").write_text(code)
        binary = path / "test"
        subprocess.run([
            "c++", "-std=c++17", "-O1", "-g", "-fsanitize=address,undefined",
            str(path / "test.cpp"), "-o", str(binary),
        ], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()

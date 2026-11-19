"""Exercise the firmware's OTA confirmation with a fake clock and ESP OTA API."""
from pathlib import Path
import subprocess
import tempfile

from host_helpers import ROOT


STUBS = r"""
#include <cassert>
#include <cstdint>

#define ESP_LOGI(...) ((void)0)
#define ESP_LOGE(...) ((void)0)

static uint32_t uptimeMs = 0;
static uint32_t millis() { return uptimeMs; }
struct { bool connected = false; } wifi, eth;
struct {
  bool active = false;
  bool isActive() const { return active; }
} ota;

enum esp_err_t { ESP_OK, ESP_FAIL, ESP_ERR_NOT_FOUND, ESP_ERR_NOT_SUPPORTED };
enum esp_ota_img_states_t { ESP_OTA_IMG_PENDING_VERIFY };
struct esp_partition_t {};

static unsigned partitionReads = 0;
static unsigned confirmationAttempts = 0;
static esp_err_t confirmationResult = ESP_FAIL;

static const esp_partition_t *esp_ota_get_running_partition() {
  static esp_partition_t running;
  ++partitionReads;
  return &running;
}

static esp_err_t esp_ota_get_state_partition(const esp_partition_t *, esp_ota_img_states_t *state) {
  *state = ESP_OTA_IMG_PENDING_VERIFY;
  return ESP_OK;
}

static esp_err_t esp_ota_mark_app_valid_cancel_rollback() {
  ++confirmationAttempts;
  return confirmationResult;
}
"""

SCENARIO = r"""
static void checkAt(uint32_t ms, unsigned expectedAttempts) {
  uptimeMs = ms;
  markAppValid();
  assert(partitionReads == expectedAttempts);
  assert(confirmationAttempts == expectedAttempts);
}

int main() {
  checkAt(29999, 0);
  wifi.connected = true;
  checkAt(29999, 0);
  wifi.connected = false;
  ota.active = true;
  checkAt(30000, 0);
  ota.active = false;
  checkAt(30000, 1);
  checkAt(39999, 1);
  confirmationResult = ESP_OK;
  checkAt(40000, 2);
  checkAt(50000, 2);
}
"""


def main():
    source = (ROOT / "src/main.cpp").read_text()
    state_start = source.index("static constexpr uint32_t OTA_VALIDATE_")
    state_end = source.index("static void handleArduinoOTA()", state_start)
    function_start = source.index("static void markAppValid()")
    function_end = source.index("\n}", function_start) + 2

    with tempfile.TemporaryDirectory(prefix="jarolift-ota-") as directory:
        case = Path(directory) / "test.cpp"
        binary = case.with_suffix("")
        case.write_text(STUBS + source[state_start:state_end] +
                        source[function_start:function_end] + SCENARIO)
        subprocess.run(["c++", "-std=c++17", str(case), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print("PASS: offline OTA confirmation at 30 s, active OTA guard, 10 s retry and stop after success.")


if __name__ == "__main__":
    main()

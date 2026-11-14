"""Local dependencies for host tests; no downloads or firmware changes."""
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def arduino_json_headers():
    configured = os.environ.get("ARDUINOJSON_DIR")
    candidates = ([Path(configured).expanduser()] if configured else
                  sorted((ROOT / ".pio/libdeps").glob("*/ArduinoJson/src")))
    for candidate in candidates:
        if (candidate / "ArduinoJson.h").is_file():
            return candidate
    raise SystemExit(
        "ArduinoJson headers missing. Install this project's PlatformIO dependencies "
        "or set ARDUINOJSON_DIR to the ArduinoJson src directory."
    )

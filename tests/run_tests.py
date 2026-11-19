"""Run the maintained host regression suite without ESP hardware."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    for tool in ("c++", "node"):
        if shutil.which(tool) is None:
            raise SystemExit(f"Required tool missing: {tool}. See tests/README.md.")

    # Do not let a future test_*.py silently become part of the maintained suite.
    scripts = [
        (sys.executable, "test_shutter_position.py"),
        (sys.executable, "test_mqtt_position.py"),
        (sys.executable, "test_remote_mapping.py"),
        (sys.executable, "test_rx_glitch.py"),
        (sys.executable, "test_rx_decode.py"),
        (sys.executable, "test_serial_migration.py"),
        (sys.executable, "test_serial_tx.py"),
        (sys.executable, "test_ota_validation.py"),
        ("node", "test_position_ui.cjs"),
        ("node", "test_remote_mapping_ui.cjs"),
    ]
    print("=== test_position_model.cpp ===", flush=True)
    with tempfile.TemporaryDirectory(prefix="jarolift-model-") as directory:
        binary = str(Path(directory) / "test")
        subprocess.run([
            "c++", "-std=c++17", "-I" + str(ROOT / "include"),
            str(ROOT / "tests/test_position_model.cpp"), "-o", binary,
        ], check=True)
        subprocess.run([binary], check=True)
    for tool, name in scripts:
        print(f"\n=== {name} ===", flush=True)
        subprocess.run([tool, str(ROOT / "tests" / name)], cwd=ROOT, check=True)
    print(f"\nPASS: all {1 + len(scripts)} host regression checks.", flush=True)


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        raise SystemExit(error.returncode)

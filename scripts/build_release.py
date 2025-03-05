Import("env")
import os
import re
import shutil

pioenv = env.get("PIOENV")
OTA_NAME = f"{pioenv}_jarolift_ota"         # prefix for ota release image
INSTALL_NAME = f"{pioenv}_jarolift_flash"   # prefix for merged flash image     

APP_BIN = "$BUILD_DIR/${PROGNAME}.bin"
MERGED_BIN = "$BUILD_DIR/${PROGNAME}_merged.bin"
RELEASE_PATH = "$PROJECT_DIR/release"
BOARD_CONFIG = env.BoardConfig()


# extract program version from /include/config.h
def extract_version():
    config_path = env.subst("$PROJECT_DIR/include/config.h")
    with open(config_path, "r") as file:
        content = file.read()
        match = re.search(r'#define VERSION\s+"(.+)"', content)
        if match:
            return match.group(1)
        else:
            return None

def merge_bin(source, target, env):
    # The list contains all extra images (bootloader, partitions, eboot) and
    # the final application binary
    flash_images = env.Flatten(env.get("FLASH_EXTRA_IMAGES", [])) + ["$ESP32_APP_OFFSET", APP_BIN]
    # Run esptool to merge images into a single binary
    env.Execute(
        " ".join(
            [
                "$PYTHONEXE",
                "$OBJCOPY",
                "--chip",
                BOARD_CONFIG.get("build.mcu", "esp32"),
                "merge_bin",
                "--flash_size", #optional: --fill-flash-size
                BOARD_CONFIG.get("upload.flash_size", "4MB"),
                "-o",
                MERGED_BIN,
            ]
            + flash_images
        )
    )                                                                                                                                                                                                                                                                                                                                                                                                                                                       
        
    release_path = env.subst(RELEASE_PATH) # path to release folder
    # delete old release files
    if env.get("PIOENV") == "esp32":
        if os.path.exists(release_path):
            shutil.rmtree(release_path)
        os.makedirs(release_path)
    else:
        # if release folder does not exist create it
        if not os.path.exists(release_path):
            os.makedirs(release_path)

    version = extract_version() # get version from config 
    merged_file = os.path.join(release_path, f"{INSTALL_NAME}_{version}.bin")  # path and name of merged image
    ota_update_file = os.path.join(release_path, f"{OTA_NAME}_{version}.bin") # path and name of ota image
    shutil.copyfile(env.subst(MERGED_BIN), merged_file) # copy files
    shutil.copyfile(env.subst(APP_BIN), ota_update_file) # copy files


# Add a post action that runs esptoolpy to merge available flash images
env.AddPostAction(APP_BIN , merge_bin)
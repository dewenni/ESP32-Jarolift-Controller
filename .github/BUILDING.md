# Builds and publishing

Generated files are not tracked in Git. `web/output/` and `release/` are ignored
local build directories and temporary outputs on GitHub-hosted runners.
`.github/pages/config.json` is the demo configuration source; `web/sim.json`
contains simulation data. Neither is a configuration export from a live device.

Both workflows build before running the host regression suite, which requires
ArduinoJson headers from the installed dependencies. The Ubuntu runner also
provides Node.js and a C++17 compiler. Both workflows use Python 3.13 and the PlatformIO version pinned in
`requirements-build.txt`. PlatformIO downloads are cached; project build outputs
are not reused between workflow runs.

## Demo WebUI

`publish_pages.yml` builds the project on pushes to `main`, adds the demo
configuration to `web/output/`, and deploys that directory using the official
GitHub Pages artifact actions. Manual runs are also supported on `main`.

Before the first deployment with this workflow, select **Settings → Pages →
Build and deployment → Source → GitHub Actions** in the repository. The
`github-pages` environment must allow deployment from `main`. The existing Pages
URL is retained. The previous `gh-pages` branch is no longer the publishing
source; no source files are committed by the new workflow.

A local demo directory can be prepared with:

```sh
pio run -e esp32
cp .github/pages/config.json web/output/config.json
```

The demo detection in EspWebUI is based on the GitHub Pages hostname; local
hosting does not enable simulation automatically.

## Firmware releases

`create_release_assets.yml` checks the tag against `include/config.h` and the
first nonempty line of `changeNew.md`, builds that exact tag for `esp32`, `esp32s2`, `esp32s3` and `esp32c3`
in that order, then uploads all eight matching flash and OTA binaries from
`release/`. ESP32 runs first because its post-build hook clears the release
directory; the other variants append their files. Release notes come from
`changeNew.md`. A failed version check, build or missing binary prevents release
creation. No devices are flashed.

Push a new `vX.Y.Z` tag to publish a release. For a manual run, provide an existing
version tag in the `tag` input. The tagged commit must already contain this
workflow and its build sources; old tags still contain their historical workflows.
An already published release is not overwritten automatically.

Both workflows delete their generated directories after upload (also on a failed
job). The runner is temporary anyway. Uploaded GitHub Release assets and the
Pages deployment are independent of these directories and remain available.

## Verification

With actionlint installed:

```sh
actionlint .github/workflows/publish_pages.yml .github/workflows/create_release_assets.yml
python3 -B tests/run_tests.py
```

For a clean build, use a fresh clone of the branch and run
`pio run -e esp32 -e esp32s2 -e esp32s3 -e esp32c3`.
It must generate five HTML/CSS/JS assets plus `web/output/sim.json`, and eight
nonempty versioned binaries under `release/`. Copy the demo configuration as
above before deploying Pages. These local checks do not verify GitHub token
permissions, environment rules or the actual upload/deployment on GitHub.

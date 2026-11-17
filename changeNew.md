# v2.0.1

This patch release fixes issues in v2.0.0. For the major changes, new features and migration notes, see [v2.0.0](https://github.com/dewenni/ESP32-Jarolift-Controller/releases/tag/v2.0.0).


- Fix ESP32-C3 startup stack overflow while initializing configuration when no valid configuration is available (#91).
- Handle 0% and 100% positioning targets as endpoints without queueing a full-duration move.
- Show calibrated positioning controls in the GitHub Pages demo.
- Update EspWebUI to 0.0.14 for correct initial language selection.

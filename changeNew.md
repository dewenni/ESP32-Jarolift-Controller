# v2.0.2

## Deutsch

v2.0.2 behebt Fehler des Hauptreleases v2.0.0. Die vollständige Übersicht der Neuerungen steht in den [Release-Notizen zu v2.0.0](https://github.com/dewenni/ESP32-Jarolift-Controller/releases/tag/v2.0.0).

- **JAROLIFT-Seriennummern.** Aus der Basisseriennummer in der WebUI berechnet der Controller für jeden Kanal die Funkseriennummer, die er sendet. Das Update korrigiert diese Berechnung und passt die gespeicherte Basisseriennummer zum Ausgleich automatisch an. In der WebUI wird etwa aus `012345` der Wert `123450`. Die Funkseriennummern bleiben bei Updates von v2.0.0 und v2.0.1 erhalten, ebenso bei älteren Versionen mit einer bisherigen Basisseriennummer bis `0FFFFF`. In diesen Fällen ist kein erneutes Anlernen erforderlich. Bei v1.9.0 oder älter mit einer Basisseriennummer über `0FFFFF` kann erneutes Anlernen nötig sein.
- **Vor und nach dem Update.** Vor dem Update `config.json` exportieren. Nach einem OTA-Update bestätigt der Controller die neue Firmware nach rund 30 Sekunden Laufzeit, auch ohne Netzwerk. Vor einem Neustart auf `OTA image confirmed - rollback cancelled` im Log warten.
- **Falls Rollläden nicht reagieren.** Zuerst prüfen, welche Firmware läuft, und den aktuellen Gerätezähler beibehalten. Läuft nach einem Rollback die alte Firmware, die vor dem Update exportierte `config.json` wiederherstellen und den Controller vor der nächsten Rollladenbedienung neu starten. [Anleitung zur Migration und Wiederherstellung](https://github.com/dewenni/ESP32-Jarolift-Controller/blob/main/README_DE.md#migration-der-basisseriennummer)

## English

v2.0.2 is a patch release for v2.0.0. For the full list of changes in the major release, see the [v2.0.0 release notes](https://github.com/dewenni/ESP32-Jarolift-Controller/releases/tag/v2.0.0).

- **JAROLIFT serial numbers.** The controller uses the base serial in the WebUI to calculate the radio serial it sends for each channel. The update corrects this calculation and automatically adjusts the stored base serial to compensate. In the WebUI, `012345` becomes `123450`, for example. Radio serial numbers are preserved when upgrading from v2.0.0 or v2.0.1, and from older releases with a previous base serial of `0FFFFF` or less. Existing pairings stay valid in these cases. With v1.9.0 or earlier and a base serial above `0FFFFF`, some motors may need to be paired again.
- **Before and after updating.** Export `config.json` before updating. After an OTA update, the controller confirms the new firmware after about 30 seconds of uptime, even when offline. Wait for `OTA image confirmed - rollback cancelled` in the log before restarting.
- **If shutters stop responding.** Check which firmware is running first and keep the current device counter. If the controller has rolled back to the older firmware, restore the `config.json` exported before the update and restart before operating the shutters. [Migration and recovery instructions](https://github.com/dewenni/ESP32-Jarolift-Controller/blob/main/README.md#base-serial-migration)

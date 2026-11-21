# v2.1.0

## Deutsch

v2.1.0 erweitert die kalibrierte Positionssteuerung um parallele Zielanfahrten und eine optional abschaltbare Referenzfahrt.

- **Parallele Positionssteuerung.** Bis zu 16 Rollläden können ihre Zielposition überlappend anfahren. WebUI, MQTT und Home Assistant teilen zusätzlich einen Puffer für bis zu 16 wartende Ziele. Neue Vorgaben werden kurz gesammelt; Referenzfahrten starten gemeinsam je Richtung, individuelle Teilfahrten nacheinander. Weitere Ziele desselben Rollladens warten auf dessen aktive Zielanfahrt.
- **Gruppierte STOP-Befehle.** Berechnete STOP-Zeitpunkte innerhalb einer Sekunde werden gemeinsam ausgeführt. Die Zusammenfassung verursacht planmäßig bis etwa ±0,5 Sekunden zusätzliche Abweichung. Anstehende STOPs haben Vorrang vor weiteren Starts. Positionen bleiben zeitbasierte Schätzungen ohne Motor-Rückmeldung.
- **Optionale Referenzfahrt.** Unter Einstellungen → Gerät → Positionssteuerung lässt sich die Referenzfahrt vor Zwischenzielen ausschalten. Sie bleibt bei bestehenden Konfigurationen standardmäßig eingeschaltet. Ohne Referenzfahrt setzt eine Zwischenposition eine bekannte Schätzposition voraus. Nach Neustart, STOP, SCHATTEN oder Abbruch zuerst vollständig AUF oder AB fahren lassen. 0/100 % bleiben möglich.
- **Status und Bedienung.** Die WebUI zeigt die Phase jeder aktiven Zielanfahrt getrennt an. Manuelle ESP-Befehle beenden aktive Zielanfahrten und leeren den Puffer. Empfangene zugeordnete Fernbedienungsbefehle beenden nur betroffene Fahrten und leeren ebenfalls den Puffer. Die Kalibrierung bleibt exklusiv.

Vor dem Update die Konfiguration exportieren. Kalibrierdaten bleiben erhalten; MQTT-Topics und die Positionsskala (0 % geschlossen, 100 % geöffnet) bleiben unverändert. Bei Bedienung über Originalfernbedienungen oder lokale Taster sollte die Referenzfahrt eingeschaltet bleiben. Hinweise für Updates von älteren Firmware-Versionen stehen weiterhin in den [Release-Notizen zu v2.0.0](https://github.com/dewenni/ESP32-Jarolift-Controller/releases/tag/v2.0.0) und [v2.0.2](https://github.com/dewenni/ESP32-Jarolift-Controller/releases/tag/v2.0.2).

[Positionssteuerung und Kalibrierung](https://github.com/dewenni/ESP32-Jarolift-Controller/blob/v2.1.0/Doc/Positionierung.md)

## English

v2.1.0 adds overlapping target runs and an optional reference run to calibrated shutter positioning.

- **Parallel positioning.** Up to 16 shutters can move towards their targets concurrently. WebUI, MQTT and Home Assistant also share a buffer for up to 16 waiting targets. Requests are collected briefly; reference runs start together per direction, followed by individually staggered target runs. Further targets for the same shutter wait for its active target run to finish.
- **Grouped STOP commands.** Calculated STOP deadlines within one second share a group command. Grouping adds up to approximately ±0.5 seconds of planned timing deviation. Upcoming STOPs take priority over further starts. Positions remain time-based estimates without motor feedback.
- **Optional reference run.** Settings → Device → Positioning allows reference runs before intermediate targets to be disabled. Existing configurations keep reference runs enabled by default. Without referencing, intermediate targets require a known position estimate. After restart, STOP, SHADE or an aborted run, first complete a full UP or DOWN run. Targets of 0/100% remain available.
- **Status and operation.** The WebUI displays the phase of each active target run independently. Manual ESP commands end active target runs and clear the queue. Received associated remote commands end only affected runs and also clear the queue. Calibration remains exclusive.

Export the configuration before updating. Calibration data is retained; MQTT topics and the position scale (0% closed, 100% open) remain unchanged. Keep reference runs enabled when using original remotes or local wall switches. Upgrade notes for older firmware remain available in the [v2.0.0](https://github.com/dewenni/ESP32-Jarolift-Controller/releases/tag/v2.0.0) and [v2.0.2](https://github.com/dewenni/ESP32-Jarolift-Controller/releases/tag/v2.0.2) release notes.

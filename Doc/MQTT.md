# MQTT-Aufteilung

`src/mqttBasic.cpp` und `include/mqttBasic.h` sind in ESP_Buderus_KM271 und
ESP32-Jarolift-Controller identisch. Sie übernehmen die Verbindungseinstellungen,
Client-ID und Last Will, Verbindungsstatus, Reconnect mit Timeout und Backoff,
sowie den Empfang und die abgesicherte Nachrichtenwarteschlange. Vollständige
Nachrichten werden auch dann zusammengesetzt, wenn TCP sie in Fragmenten liefert.

`src/mqttCustom.cpp` enthält die gerätespezifischen Abonnements, Discovery,
Startmeldungen und Befehle. Die Schnittstelle `include/mqttCustom.h` ist gleich:

- `mqttCustomConnected()`: nach jeder erfolgreichen Verbindung abonnieren/publizieren.
- `mqttCustomCyclic()`: zyklische Geräteaufgaben.
- `processMqttMessage(const s_MqttMessage &)`: eine vollständige Nachricht verarbeiten.

Alle drei Funktionen laufen auf dem Loop-Task. Die Basis entnimmt eine Nachricht
unter Mutex-Schutz aus der Queue, gibt den Mutex frei und ruft erst dann den
Handler auf. Der Handler verwaltet weder Queue noch MQTT-Client.

`include/mqtt.h` bleibt als bisheriger Include-Einstieg erhalten.

## Änderungen in beide Projekte übernehmen

Diese Dateien gemeinsam aktualisieren und identisch halten:

- `src/mqttBasic.cpp`
- `include/mqttBasic.h`, `include/mqttCustom.h`, `include/mqtt.h`
- `tests/test_mqtt_reconnect.py`

Die lokalen Fehlerkorrekturen in
`lib/async-mqtt-client/src/AsyncMqttClient.cpp` müssen ebenfalls in beiden
Projekten gleich bleiben. Sie behandeln sofort abgelehnte TCP-Verbindungsversuche,
das erzwungene Trennen ohne TCP-Socket und das Verwerfen unvollständiger Parser-Zustände.

`src/mqttCustom.cpp` wird nicht zwischen den Projekten kopiert.
Die Dateien werden nicht automatisch synchronisiert.

## Prüfen

`python3 tests/test_mqtt_reconnect.py` kompiliert die echte MQTT-Basis und die
lokale MQTT-Bibliothek auf dem Host mit simuliertem TCP und simulierter Uhr.
Der Test prüft Fehler, Timeouts, Backoff, Wiederverbindung, Empfangsfragmente,
Queue-Grenzen und OTA-Pausen mit AddressSanitizer und UndefinedBehaviorSanitizer.
Er benötigt einen C++17-Compiler und ersetzt keinen Verbindungstest am ESP.


## Warteschlange für Positionsziele

`<prefix>/cmd/position/N` nimmt ganzzahlige Öffnungsziele von 0 bis 100 ohne Retain an.
Die Positionssteuerung nutzt für MQTT, Home Assistant und WebUI dieselbe feste
Warteschlange mit bis zu 16 wartenden Aufträgen zusätzlich zur aktiven Fahrt.
Aufträge werden nacheinander mit Referenzfahrt ausgeführt. Pro Kanal wartet höchstens
ein Ziel; ein neuer Sollwert ersetzt den wartenden Wert an derselben Stelle.
Während einer Kalibrierung werden keine Positionsziele angenommen.

Die Annahme eines Ziels bestätigt keine abgeschlossene Motorfahrt. Der vorhandene
Positionsstatus bleibt bis zur tatsächlichen Abarbeitung davon getrennt. Ablehnungen
werden wie bisher unter `<prefix>/message` gemeldet. Manuelle Befehle verwerfen den
Puffer und unterbrechen die automatische Fahrt. Zugeordnete Originalfernbedienungen
verwerfen wartende Ziele und unterbrechen die aktive Fahrt, wenn diese betroffen ist.
Neustart, Setup-Mode, OTA oder das Abschalten der Positionssteuerung verwerfen den
Puffer ebenfalls. Details stehen in [Positionierung.md](Positionierung.md).

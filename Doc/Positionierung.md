# Positionssteuerung – erster Hardware-Teststand

Optional in Einstellungen → Gerät → Positionssteuerung aktivieren. Die Bedientasten bleiben unverändert; MQTT-Positionsmeldungen verwenden jetzt dieselbe Öffnungsskala (Breaking Change, siehe Release-Information). Die neue Öffnungsskala ist **0 % geschlossen, 100 % geöffnet**. Prozentziele sind über Kanalwidgets und MQTT möglich. Home Assistant bietet sie automatisch nur bei global aktivierter Funktion und gültiger Kalibrierung des jeweiligen Kanals an. Gruppen-Prozentfahrten sind noch nicht Bestandteil.

## Kalibrieren

1. Endlagen des Motors zuvor regulär einrichten. Positionskalibrierung erst danach durchführen.
2. Auf der Service-Seite den gewünschten aktivierten Kanal auswählen und den Rollladen mit den normalen Tasten vollständig schließen.
3. „Kalibrieren“ starten und die vollständig geschlossene Ausgangslage bestätigen. Direkte Sicht auf den Rollladen ist erforderlich.
4. Öffnungsfahrt starten. „Panzer hebt sich an“, „halb geöffnet“ und „vollständig geöffnet“ jeweils im richtigen Moment bestätigen.
5. Aus vollständig geöffneter Lage die Schließfahrt starten. „Halb geöffnet“, „Panzer unten“ und „Lamellen vollständig geschlossen“ bestätigen.
6. Zeiten prüfen und ausdrücklich übernehmen. Erst dann werden vorhandene Kalibrierdaten ersetzt und regulär in config.json gespeichert.

Die halbe Öffnungshöhe vorher markieren. Zeitpunkte sind ab Beginn der jeweiligen Fahrt gemessen, keine einzelnen Segmentdauern. Maximal 180 s je Richtung; zwischen den Höhenmarkierungen muss mindestens 0,5 s liegen. Bei unplausiblen Zeiten abbrechen und wiederholen.

Der Browser sendet während der Kalibrierung Lebenszeichen. Bei mehr als sechs Sekunden ohne Lebenszeichen stoppt der ESP die Kalibrierung. Dazu muss der ESP selbst betriebsbereit bleiben; die Motor-Endabschaltungen sind weiterhin erforderlich. Ein Tabwechsel in den Hintergrund kann die Kalibrierung deshalb abbrechen. Verbindungsverlust stoppt nicht zwingend sofort.

## Prozentfahrt

- Ziel unter 50 %: vollständige Schließfahrt einschließlich Lamellenschluss und Zeitreserve, 600 ms Pause, dann Öffnen bis zum Ziel.
- Ziel ab 50 %: vollständige Öffnungsfahrt mit Zeitreserve, 600 ms Pause, dann Schließen bis zum Ziel.
- 0/100 %: nur die jeweilige vollständige Referenzfahrt.
- Die Reserve beträgt 5 % der Fahrzeit, mindestens 1 s und höchstens 3 s.
- Kennlinie aus den drei Kalibrierpunkten je Richtung; dazwischen lineare Zeitinterpolation. Wickeldurchmesser werden damit näherungsweise berücksichtigt.
- Der ESP führt eine Zielanfahrt auch bei geschlossenem Browser zu Ende. Ein manueller Befehl bricht die Sequenz ab. Es gibt keinen automatischen Wiederanlauf.
- Zunächst ist genau eine automatische Zielanfahrt oder Kalibrierung gleichzeitig möglich. Weitere Prozentanforderungen werden mit einem Hinweis abgewiesen. Normale Befehle haben Vorrang und stoppen eine laufende automatische Sequenz zuerst.

Die Funktion liefert **Schätzwerte**, keine gemessenen Positionen. Während einer Referenzfahrt ist die aktuelle Höhe unbekannt; das Widget zeigt die Phase und das Ziel. Nach STOP, SHADE und Neustart gilt die Position als unbekannt. Manuelles AUF/AB und empfangene zugeordnete Fernbedienungsbefehle setzen bei kalibrierten Kanälen nach vollständiger Fahrzeit plus Reserve die geschätzte Endlage. Währenddessen ist die Position unbekannt. Ohne Kalibrierung wird bei AUF/AB weiterhin sofort die erwartete Endlage gemeldet. Die Kalibrierung bleibt nach Neustart erhalten. Jede neue Prozentfahrt referenziert ohnehin erneut.

Originalfernbedienungen müssen den Kanälen in den Einstellungen zugeordnet sein. Empfangene Befehle einer zugeordneten Fernbedienung brechen eine laufende Sequenz für deren zugeordnete Kanäle ab, auch ohne MQTT. Nicht empfangene Befehle können den ESP während einer laufenden Sequenz weiterhin unbemerkt beeinflussen. Eine Referenzfahrt schützt gegen eine zuvor unbekannte Ausgangslage, nicht gegen gleichzeitige unbemerkte Bedienung.

Anlern-/Entlern- und Endlagenbefehle machen die Kalibrierung des betreffenden Kanals ungültig. OTA und Deaktivierung brechen automatische Abläufe ab. Konfigurationsschreiben wird während einer automatischen Sequenz aufgeschoben, um STOP-Zeitpunkte nicht durch Flashschreiben zu verzögern.

## Hardware-Prüfung vor breiter Nutzung

Zunächst einen Kanal kalibrieren und 25/50/75 % aus beiden Richtungen prüfen. STOP, manuellen Richtungswechsel, Originalfernbedienung und Browserabbruch während einer Kalibrierung testen. Danach andere Rollläden separat kalibrieren. Der STOP-Vorlauf von 110 ms ist ein Startwert für das Funkprotokoll und muss am tatsächlichen Motor geprüft werden.

Hosttest: `c++ -std=c++17 -Iinclude tests/test_position_model.cpp -o /tmp/position-test && /tmp/position-test`

Integrationstest mit installierter ArduinoJson-Abhängigkeit: `python3 tests/test_shutter_position.py`

## MQTT-Status

WebUI und MQTT verwenden denselben Stand: `status/shutter/N` enthält die geschätzte Öffnung (0 geschlossen, 100 geöffnet) oder `None` bei unbekannter Position. Automatische Zielankünfte werden ebenfalls retained veröffentlicht; bei MQTT-Reconnect wird der aktuelle Stand erneut gesendet. Befehls-Payloads bleiben unverändert, insbesondere 0 = UP und 1 = DOWN; diese Zahlen sind Befehlskennungen, keine Prozentziele.

## Prozentziele über MQTT und Home Assistant

`<prefix>/cmd/position/1` bis `/16`, Payload als Ganzzahl von `0` bis `100`, ohne Retain. Beispiel: `25` fährt nach Referenzfahrt 25 % Öffnung an. Dieselbe Prüfung und Ablaufsteuerung wie beim WebUI gilt: Funktion aktiv, Kanal aktiviert und kalibriert, kein anderer automatischer Ablauf und keine anstehenden Funkbefehle. Ungültige Ziele oder nicht verfügbare Kanäle werden abgewiesen.

Discovery verwendet das bestehende Cover mit unveränderter Identität und ergänzt Positionsrückmeldung und Prozentziel-Topic. Abschalten oder ungültige Kalibrierung entfernt die Prozentsteuerung wieder, AUF/AB/STOP bleiben verfügbar. Änderungen und MQTT-Reconnect lösen automatisch eine Discovery-Aktualisierung aus. Bei unbekannter Position meldet der Zustandskanal `None`; Home Assistant übernimmt keinen erfundenen Prozentwert, ein zuvor bekannter Prozentwert kann als letzter Stand erhalten bleiben.

## Originalfernbedienungen zuordnen

Service → Rollladen auswählen → Originalfernbedienungen → „Zuordnung hinzufügen“.
Auf der Originalfernbedienung einen bereits am Motor angelernten Einzelkanal
wählen und möglichst STOP drücken. Taste loslassen, kurz warten und denselben
Kanal nochmals drücken. Danach einen Namen mit höchstens 32 Unicode-Zeichen
vergeben und speichern. Die ESP-Registrierung sendet keine Motorbefehle; die
Tastendrücke auf der Fernbedienung können natürlich die Motoren bedienen.

Bis zu fünf Quellen je Motor, insgesamt 80 Quellen. Dieselbe Quelle kann mehreren
Motoren zugeordnet werden. „Löschen“ entfernt ausschließlich die Beziehung zum
ausgewählten Motor; andere Zuordnungen und die echte Anlernung bleiben bestehen.
Ein erneutes Erfassen einer bekannten Quelle erlaubt, ihre gemeinsame Bezeichnung
zu ändern. Gruppensignale wirken automatisch auf alle zugeordneten Einzelkanäle
derselben Senderkennung (`serial & 0x0ffffff0`); keine separate Gruppenregistrierung.

Konfigurationsversion 7 übernimmt bestehende Einzelquellen aus Version 6 und
vereinigt doppelte Quellen. Alte Gruppen werden als „Neu erfassen“ angezeigt und
nicht ausgewertet. Noch ältere Präfixeinträge müssen neu erfasst werden. Bestehende
überzählige Beziehungen bleiben erhalten; neue sind auf fünf je Motor begrenzt.
Die Auswertung beeinflusst den gemeinsamen Positionsstatus auch bei deaktiviertem
MQTT. Fehlende oder falsch empfangene Funksignale können weiterhin nicht sicher
erkannt werden.

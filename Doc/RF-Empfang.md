# Empfang: Test des Glitch-Filters

Branch: `test/rx-glitch-filter`.

## Hintergrund und Quellen

- [Reinhard-M, PR #20 im Fork von Banabas](https://github.com/Banabas/ESP32-Jarolift-PositionController/pull/20): kurze Störspitzen innerhalb eines Pulses entfernen, statt den Puls in zwei Teile zu zerlegen.
- [Reinhard-M, PR #21](https://github.com/Banabas/ESP32-Jarolift-PositionController/pull/21): Korrekturen für falsche Sync-Flanken, Telegrammende und stehen gebliebene HIGH-Teilwerte. Beide PRs sind inzwischen gemergt.
- [Steffen, Protokollanalyse von 2017](https://www.bastelbudenbuben.de/2017/04/25/protokollanalyse-von-jarolift-tdef-motoren/): historischer Grundaufbau und LSB-Reihenfolge von Hopcode, Seriennummer und Befehl. Zusätzliche Gruppenbits und aktuelle Fernbedienungen sind durch unsere neueren Messungen zu ergänzen.
- [Eigene Fernbedienungsmessungen](remotes.md).

## Umsetzung

Die Filteridee und die Randfallkorrekturen von Reinhard-M wurden an unseren
ISR-Puffer angepasst. Die Grenze bleibt bei 200 µs. Ein kürzerer HIGH- oder
LOW-Impuls nimmt die zuvor verarbeitete Flanke zurück; der umgebende Puls wird
mit seiner vollständigen Dauer gemessen. Wiederholte kurze Spitzen können
entsprechend entfernt werden.

Beim Zurücknehmen einer falschen Sync-Flanke auch den ersten HIGH-Pufferplatz
wiederherstellen. Eine LOW-Störspitze darf keinen vermeintlich gültigen
HIGH-Teilwert stehen lassen; ein insgesamt zu langer HIGH-Puls bleibt ungültig.

Unser Empfang publiziert einen fertigen Puffer weiterhin erst nach einer Pause.
Deshalb wird das `framePending` des Forks hier nicht zusätzlich benötigt.
Rollback-Zustände werden beim Abschluss und bei der Freigabe zurückgesetzt.
ISR und Hauptschleife verwenden weiterhin denselben vorhandenen Lock.
Keine Allokation, kein Logging und keine SPI-Zugriffe im Interrupt.
Zusätzlicher fester Speicherbedarf am ESP32: 36 Bytes pro Controller.

Beim reinen Glitch-Filter blieb die Decodierung unverändert. Die unten beschriebene
Ergänzung korrigiert anschließend die Auswertung des Telegrammendes.
Funkregister, Sendetiming und Fernbedienungszuordnung bleiben unverändert.
Keine Kanalmaske wird aus der Seriennummer ergänzt.

## Aussagegrenzen

Ein Glitch-Filter kann kurze, isolierte Störspitzen entfernen. Er ist keine
Prüfsumme und beweist nicht, dass ein Telegramm korrekt ist. Verlorene Flanken,
längere Störungen oder dicht beieinander liegende Flanken können weiterhin
Telegramme unbrauchbar machen. Eine grundsätzliche Verbesserung der Reichweite
ist damit nicht nachgewiesen.

## Prüfung

Softwaretests spielen echte Empfangsmethoden mit synthetischen Flanken ab:
saubere 64-/72-Bit-Folgen; Störspitzen in beiden Pegeln, im Sync und am letzten
Bit; mehrere Spitzen in einem Puls; ungültige lange HIGH-Pulse; unveränderliche
fertige Puffer; Zeitüberlauf; 100.000 Rauschflanken. AddressSanitizer und
UndefinedBehaviorSanitizer prüfen dabei Speicherzugriffe und undefiniertes
Verhalten. Das ersetzt keine Prüfung des HF-Signals oder der ISR auf Hardware.

Für den Vergleich am Gerät denselben Standort und dieselbe Fernbedienung nutzen:

1. In der Nähe AUF, AB und STOP einzeln prüfen, einschließlich Kanal 16 und Gruppe.
2. Am bisher problematischen Standort je Befehl mehrere getrennte kurze Tastendrücke prüfen.
3. INFO-Zeilen auf falsche Seriennummern, Befehle und Masken prüfen; die gedrosselten
   DEBUG-Zeilen zeigen erfasste Pulse und die Decodierung.
4. Auf bisher funktionierenden Fernbedienungen auf Verschlechterungen achten.

Mehrere Meldungen pro Tastendruck können Funkwiederholungen sein. Daraus allein
keine Empfangsquote ableiten. Firmware nicht allein zur Prüfung automatisch
flashen und keine Motorfahrten automatisiert auslösen.

## Befund: `entries=72`, `tail=7` und Kanal 16

Der Puffer enthält am Platz 0 den Sync. Bei 72 Einträgen sind deshalb erst
71 HIGH/LOW-Paare vollständig gespeichert. Läuft der letzte LOW-Pegel direkt
in die Sendepause weiter, fehlt die nächste steigende Flanke zum Speichern des
72. LOW-Pulses. Sein HIGH-Puls ist aber bereits am noch offenen Pufferplatz 72
gemessen. Die bisherige Schleife las nur Plätze kleiner als 72 und verlor so
das achte Gruppenbit (Kanal 16). Die bisherigen Pulstests hatten eine zusätzliche
Abschlussflanke und deckten diesen realistischen Abschluss nicht ab.

Die Auswertung verwendet jetzt auch diesen gemessenen letzten HIGH-Puls. Er
muss eindeutig zur kurzen oder langen Pulsgruppe der vorherigen vollständigen
Paare passen (±25 % um deren jeweiligen Mittelwert). Bei keinem oder einem
mehrdeutigen Treffer wird das unvollständige Gruppentelegramm verworfen. Kein
LOW-Wert wird erfunden. Die zusätzliche Auswertung erfolgt in der Hauptschleife,
nicht im Interrupt, und benötigt keinen zusätzlichen Empfangspuffer.

Die Diagnose unterscheidet Pufferzählung und ausgewertete Bits. Beim betroffenen
Telegramm ist nun `entries=72 ... tail=8 end_high=400 final_high_only=1` zu erwarten;
bei vollständig gepaarten 72 Datenbits `entries=73 ... tail=8 final_high_only=0`.
Der konkrete HIGH-Wert hängt vom Sender und Empfang ab.

Zusätzlich prüfen wir Pulsperioden von 900–1500 µs und vergleichen das Byte
`(decoded >> 16) & 0xff` mit `serial & 0xff`. Dieses Kennbyte wird auch von unserer
Sendeimplementierung in den verschlüsselten Diskriminator geschrieben. Eine
beschädigte Seriennummer leitet einen anderen Schlüssel ab; das Prüfen des
Kennbytes verwirft viele der daraus entstehenden Zufallsergebnisse. Es ist nur
eine 8-Bit-Plausibilitätsprüfung, keine vollständige Integritätsgarantie. Besonders
unverschlüsselte Gruppenbits können trotz dieser Prüfung weiterhin beschädigt sein.

Ein weiterer Hosttest verwendet die tatsächlichen Empfangs- und Decoderfunktionen
mit der tatsächlichen KeeLoq-Implementierung (auf 32-Bit-Arithmetik wie am ESP):
alle 16 Einzelkanäle, AUF/AB/STOP, Gruppen einschließlich Kanal 16, mit und ohne
Abschlussflanke; unvollständige Gruppen und inkonsistente Kennbytes sowie eine
beschädigte Seriennummer. Die Testschlüssel sind synthetisch. Hardwaretests bleiben nötig.

## Hardware-Rückmeldung vom 14.11.2026

Kanal 16 wurde nach der Korrektur mit `tail=8` und Kanalmaske `0x8000`
am Gerät bestätigt. Ein verkürztes Telegramm mit `entries=71`, `tail=6`
wurde verworfen. Ob der Glitch-Filter die Empfangsquote am entfernten
Standort verbessert, ist damit noch nicht nachgewiesen.

# Funkempfang von Originalfernbedienungen

Diese Anleitung beschreibt den im ESP32-Jarolift-Controller verwendeten Empfangspfad für Jarolift-TDEF-Fernbedienungen: Protokollaufbau, interne Funkparameter und die Prüfungen vor der Weiterverarbeitung eines Telegramms. Die Werte beziehen sich auf die aktuelle Implementierung, nicht auf einen experimentellen Test-Branch.

Der ESP empfängt Funkbefehle einer Fernbedienung, **keine Bestätigung des Motors**. Ein erkanntes Telegramm beweist deshalb nicht, dass ein bestimmter Rollladen tatsächlich gefahren ist. Für den gemeinsamen Positionsstatus müssen die Fernbedienungskanäle auf der Service-Seite den richtigen Rollläden zugeordnet sein. Siehe [Projektbeschreibung](../README_DE.md#originalfernbedienungen-zuordnen) und [Positionssteuerung](Positionierung.md).

## Aufbau des Funktelegramms

Das CC1101-Modul arbeitet im **asynchronen ASK/OOK-Modus** im 433-MHz-Band. OOK bedeutet, dass der Träger ein- und ausgeschaltet wird. Die Daten werden nicht als fertige CC1101-Pakete aus dem FIFO gelesen: Der ESP misst die Pegelwechsel am Datenausgang GDO2 und decodiert die Pulsdauern selbst.

Ein Telegramm besteht aus einem einleitenden Pulszug, einem längeren Synchronisationspuls und den Daten. Innerhalb der folgenden Felder wird jeweils das niederwertigste Bit zuerst übertragen (**LSB first**):

| Datenposition | Umfang | Inhalt |
| --- | --- | --- |
| Bit 1–32 | 32 Bit | Mit KeeLoq verschlüsselter Hopcode. |
| Bit 33–60 | 28 Bit | Seriennummer des sendenden Kanals beziehungsweise der Senderauswahl. |
| Bit 61–64 | 4 Bit | Funktionscode. |
| Bit 65–72, sofern vorhanden | 8 Bit | Zusätzliche unverschlüsselte Kanalbits für die obere Hälfte der 16-Bit-Kanalmaske. |

Der entschlüsselte Hopcode wird folgendermaßen aufgeteilt:

| Bit im entschlüsselten 32-Bit-Wert | Inhalt |
| --- | --- |
| 0–15 | 16-Bit-Gerätezähler beziehungsweise Rolling Counter. |
| 16–23 | Kennbyte: wird auf Übereinstimmung mit dem unteren Byte der übertragenen Seriennummer geprüft. |
| 24–31 | Kanalbits 1–8. |

Aus den Kanalbits im Hopcode und den zusätzlichen acht Bits entsteht die **16-Bit-Quellkanalmaske**. Bei Einzelbedienung ist normalerweise ein Bit gesetzt; bei Gruppenbedienung können mehrere gesetzt sein. Die Maske beschreibt die Auswahl an der Fernbedienung, nicht automatisch die ESP-Rollladennummern. Diese Beziehung ergibt sich erst aus den registrierten Zuordnungen.

Die Seriennummer allein genügt nicht zur Kanalbestimmung. Je nach Fernbedienung verändert sie sich mit der Auswahl; dokumentierte Beispiele stehen in [remotes.md](remotes.md). Die Empfangssoftware ergänzt fehlende Maskenbits ausdrücklich **nicht** aus der Seriennummer.

### Befehle und Pulsdarstellung

Die grundlegenden Funktionscodes sind **`0x8` für AUF**, **`0x2` für AB** und **`0x4` für STOP**. Der Empfangspfad interpretiert mehr als zehn aufeinanderfolgende decodierte STOP-Telegramme als SCHATTEN (`0x3`). Mehrere Logmeldungen bei einem Tastendruck können deshalb normale Funkwiederholungen sein.

Ein Datenbit besteht aus einem kurzen und einem langen Pegelabschnitt, typischerweise etwa **400 und 800 µs**. Am verwendeten RX-Datenausgang wird ein vollständiges Paar als 1 gelesen, wenn LOW mindestens so lang wie HIGH ist; andernfalls als 0. Die Pulsbreiten werden gegeneinander verglichen, statt jedes Bit anhand einer einzigen festen 400-µs-Grenze zu bestimmen.

Der KeeLoq-Schlüssel wird aus der übertragenen Seriennummer und den konfigurierten Masterkeys abgeleitet. Falsche Masterkeys können daher trotz gut empfangener Pulse zu einer abgewiesenen Decodierung führen.

## Intern verwendete Funkparameter

Die Funkparameter sind in der eingebundenen Jarolift-Library festgelegt. Im WebUI werden unter anderem die Anschluss-GPIOs und JAROLIFT-Schlüssel konfiguriert; die folgenden Modem- und Filterwerte sind keine zusätzlichen Nutzerparameter.

| Einstellung | Verwendeter Wert beziehungsweise Verhalten |
| --- | --- |
| Trägerfrequenz | Etwa **433,9198 MHz** bei einem 26-MHz-Quarz; Frequenzregister `FREQ2/1/0 = 0x10 / 0xB0 / 0x71`. |
| Modulation und Datenpfad | ASK/OOK, asynchroner serieller Datenpfad. `MDMCFG2 = 0x30`, `PKTCTRL0 = 0x32`; keine hardwareseitige Paket-Sync-Erkennung. |
| Empfangs-Datenausgang | GDO2 mit `IOCFG2 = 0x0D`; ESP-GPIO als `INPUT_PULLUP`, Interrupt bei jeder Pegeländerung (`CHANGE`). |
| GDO0 / GDO1 | `IOCFG0 = 0x2D`, `IOCFG1 = 0x2E`; GDO0 wird für die asynchrone Sendung verwendet. |
| Modem-Konfiguration | `MDMCFG4/3/1/0 = 0x57 / 0xC4 / 0x22 / 0xF8`. Die Bitzeiten des Funkprotokolls werden separat in Software ausgewertet. |
| Automatische Verstärkungsregelung | `AGCCTRL2/1/0 = 0x07 / 0x00 / 0x91`. |
| RX-Frontend | `FREND1 = 0x56`, `FSCTRL1/0 = 0x06 / 0x00`. |
| Sync-Register | `SYNC1/0 = 0xC7 / 0x00`. Im asynchronen Betrieb wird der Telegrammstart über die gemessene Pulsdauer erkannt, nicht durch dieses Registerpaar. |
| Adressprüfung | Deaktiviert; die Zuordnung erfolgt nach der Decodierung in der Anwendung. |
| Sendeleistung | `PA_LongDistance = 0xC0` wird vor der CC1101-Initialisierung gesetzt. Das betrifft die Sendung, nicht eine Erhöhung der Empfangsempfindlichkeit. |

Die vollständigen Registervorgaben stehen in [cc1101.h](../lib/JaroliftController/cc1101.h), ihre Anwendung in [cc1101.cpp](../lib/JaroliftController/cc1101.cpp) und der Initialisierung in [JaroliftController.cpp](../lib/JaroliftController/JaroliftController.cpp).

## Empfangsablauf und Filter

### 1. Pegelwechsel mit kurzen Interrupts erfassen

Der Interrupt misst die Pulsdauern und schreibt sie in feste HIGH-/LOW-Puffer. Decodierung, Entschlüsselung, Logausgabe und die Weitergabe an WebUI/MQTT laufen anschließend in der Hauptschleife. Im Interrupt gibt es keine dynamische Speicherallokation und keine SPI-Zugriffe.

Interrupt und Hauptschleife verwenden denselben Lock. Ein zur Auswertung freigegebener Puffer wird nicht durch neue Flanken überschrieben. Nach jeder Auswertung wird er wieder freigegeben, auch wenn das Telegramm verworfen wurde. Begrenzte Puffer und das Zurücksetzen ungültiger Kandidaten verhindern ein unbegrenztes Ansammeln von Rauschdaten.

### 2. Kurze Störspitzen zusammenführen

Der **Glitch-Filter mit einer Grenze von 200 µs** behandelt kurze HIGH- und LOW-Spitzen. Er ignoriert nicht einfach die zweite Flanke: Er nimmt den zuvor gespeicherten Flankenwechsel zurück und misst den umgebenden Puls mit seiner vollständigen Dauer weiter.

Beispiel: Wird ein gültiger langer LOW-Abschnitt kurz durch eine 80-µs-HIGH-Spitze unterbrochen, sollen daraus keine zwei falschen Datenpulse entstehen. Die Spitze wird zurückgenommen und der LOW-Abschnitt bleibt ein zusammenhängender Puls. Auch mehrere kurze Störspitzen innerhalb eines Pulses können so entfernt werden.

Beim Zurücknehmen werden auch eine irrtümlich erkannte Sync-Flanke und betroffene HIGH-Pufferwerte wiederhergestellt. Ein ungültig langer HIGH-Puls darf keinen älteren, scheinbar gültigen Teilwert stehen lassen. Damit werden typische Ursachen verschobener Telegramme reduziert.

### 3. Start und Ende eines Telegramms erkennen

Ein LOW-Synchronisationspuls am RX-Ausgang wird nur bei **mehr als 3650 und weniger als 4300 µs** als Start erkannt. Die Erfassung wird nach einer Pause von **mehr als 3500 µs** abgeschlossen. Das geschieht auch aus der Hauptschleife heraus, wenn keine weitere Flanke mehr folgt. Der Abschluss hängt somit nicht vom nächsten Tastendruck ab.

Als Kandidaten werden Pufferzählungen von **65 bis 75 Einträgen einschließlich Sync** übernommen. Das ist noch keine Aussage über gültige Daten: Erst die folgende Decodierung prüft Pulse und die Vollständigkeit der Kanalbits.

### 4. Vollständige Puls-Paare prüfen

Für jedes gespeicherte Datenpaar gelten:

| Prüfung | Akzeptierter Bereich |
| --- | --- |
| HIGH-Dauer | Größer 200 und kleiner 1000 µs. |
| LOW-Dauer | Größer 200 und kleiner 1000 µs. |
| Gesamtdauer HIGH + LOW | 900 bis 1500 µs einschließlich der Grenzen. |

Ein ungültiges Paar verwirft den Kandidaten. Eine 64-Bit-Datenfolge kann ohne zusätzliche Kanalbits verarbeitet werden. Sobald zusätzliche Kanalbits vorhanden sind, müssen alle acht auswertbar sein; **ein bis sieben Zusatzbits werden nicht als gültige Gruppenmaske übernommen**.

### 5. Das letzte Kanalbit auch ohne Abschlussflanke auswerten

Am Telegrammende kann der letzte LOW-Pegel direkt in die Sendepause übergehen. Dann fehlt die folgende steigende Flanke, um das letzte vollständige Paar abzuspeichern. Sein HIGH-Abschnitt wurde aber bereits gemessen. Wird dieser Abschnitt übergangen, kann insbesondere **Kanal 16** fehlen.

Nur am passenden 72-Bit-Abschluss nutzt die Software deshalb den bereits gemessenen letzten HIGH-Puls. Aus den vorherigen vollständigen Paaren werden die Mittelwerte der kurzen und langen Abschnitte berechnet. Der letzte HIGH-Puls muss mit **±25 % Toleranz genau einer** dieser beiden Gruppen entsprechen. Passt er zu keiner oder zu beiden, wird das unvollständige Telegramm verworfen.

Es wird weder eine zusätzliche Flanke noch eine LOW-Dauer oder Kanalmaske erfunden. Der gemessene HIGH-Abschnitt liefert die Bitentscheidung. Die Auswertung findet in der Hauptschleife statt und benötigt keinen weiteren Empfangspuffer.

### 6. Entschlüsselung plausibilisieren

Nach dem Auslesen von Hopcode, Seriennummer und Funktion wird der Hopcode entschlüsselt. Dabei muss gelten:

```cpp
((decoded >> 16) & 0xff) == (serial & 0xff)
```

Eine beschädigte Seriennummer führt zu einem anderen abgeleiteten Schlüssel. Der Vergleich des entschlüsselten Kennbytes mit dem übertragenen Seriennummernbyte verwirft viele daraus entstehende Zufallsergebnisse, bevor ein Befehl an die Anwendung weitergegeben wird.

Dies ist nur eine **8-Bit-Plausibilitätsprüfung**, keine vollständige Prüfsumme oder Integritätsgarantie. Die zusätzlichen unverschlüsselten Kanalbits werden dadurch nicht abgesichert. Der empfangene Rolling Counter wird in diesem Empfangspfad auch nicht gegen einen gespeicherten Zählerstand der Fernbedienung auf Aktualität geprüft.

### 7. Quelle zuordnen und Status aktualisieren

Nach erfolgreicher Auswertung erhält die Anwendung Seriennummer, Funktion und Quellkanalmaske. Sie sucht passende auf Service registrierte Einzelquellen anhand der Senderkennung (`serial & 0x0ffffff0`) und der überschneidenden Kanalbits. Daraus ergibt sich die Menge der betroffenen ESP-Rollläden.

Zuordnung und Telegrammprüfung sind unterschiedliche Aufgaben: Eine unbekannte Quelle kann korrekt empfangen sein; eine registrierte Quelle macht ein beschädigtes Telegramm nicht korrekt. Im normalen Betrieb ist keine pauschale Bestätigung durch zwei gleiche Telegramme erforderlich. Die zwei getrennten Tastendrücke beim Registrieren dienen nur der Bestätigung im Anlerndialog.

## Was RMT verbessert – und was der Glitch-Filter verbessert

**RMT wird für das Senden verwendet.** Der ESP erzeugt die Pulsfolge dort per Hardware, damit andere Aufgaben die Sendezeiten möglichst wenig beeinflussen. Nach der Sendung wird wieder in den Empfangsmodus gewechselt.

**Der Empfang nutzt GPIO-Interrupts und den beschriebenen Softwarefilter.** Der Glitch-Filter entfernt kurze Störspitzen und die Abschlussauswertung erhält das letzte Kanalbit. Ein Erfolg beim Senden oder eine höhere eingestellte Sendeleistung ist kein Nachweis für einen besseren Empfang weit entfernter Fernbedienungen. Während eigener Sendungen ist kein gleichzeitiger Empfang möglich.

## Empfang im Logbuch beurteilen

Im normalen **INFO**-Log erscheint ein an die Anwendung weitergegebener Befehl beispielsweise als:

```text
I JARO: received remote signal | serial: 0x0c7c000f | cmd: UP, | channel: 10000000 00000000
```

Für die Pulsauswertung das Log-Level auf **DEBUG** stellen. Die technischen Meldungen werden auf höchstens einen ausgewerteten Telegrammkandidaten pro Sekunde begrenzt. Die Telegrammverarbeitung selbst wird dadurch nicht auf ein Telegramm pro Sekunde begrenzt.

| Diagnosefeld / Meldung | Bedeutung |
| --- | --- |
| `entries` | Pufferzählung einschließlich des Sync-Eintrags; nicht direkt die Zahl der Datenbits. |
| `sync` | Gemessene Dauer des Synchronisationspulses in µs. |
| `low`, `high` | Kürzester und längster vollständiger Datenabschnitt des jeweiligen Pegels in µs. |
| `invalid` | Zahl der ungültigen vollständigen Puls-Paare; größer null führt zur Ablehnung. |
| `tail` | Zahl der ausgewerteten zusätzlichen Kanalbits, maximal acht. |
| `end_high` | Gemessener HIGH-Abschnitt am offenen Pufferplatz hinter dem letzten vollständigen Paar. |
| `final_high_only=1` | Das letzte Kanalbit wurde aus diesem gemessenen HIGH-Abschnitt ausgewertet. |
| `FRAME REJECTED: invalid pulse pairs or incomplete group bits` | Pulsprüfung oder Vollständigkeit der Zusatzbits fehlgeschlagen. |
| `FRAME REJECTED: discriminator mismatch` | Das entschlüsselte Kennbyte passt nicht zur Seriennummer. Masterkeys und Empfang prüfen. |
| `FRAME DECODED` | Felder wurden ausgewertet und die Diskriminatorprüfung bestanden; der Eintrag zeigt auch die rekonstruierte Kanalmaske. |

Bei einem Telegramm mit allen 16 Kanalbits sind beispielsweise **`entries=72`, `tail=8`, `final_high_only=1`** möglich. Mit vollständiger Abschlussflanke sind es **`entries=73`, `tail=8`, `final_high_only=0`**. Eine geringere Eintragszahl ist nicht automatisch ein Defekt; entscheidend ist der gesamte Telegrammtyp und das Ergebnis der Prüfungen.

Mehrere INFO-Einträge bei einem Tastendruck sind normal, wenn die Fernbedienung den Befehl mehrfach sendet. Wegen der gedrosselten DEBUG-Ausgabe steht nicht zu jedem INFO-Eintrag eine eigene Pulsmessung im Log.

## Grenzen und praktische Fehlersuche

Der Filter entfernt kurze isolierte Spitzen. Verlorene Flanken, längere Störungen oder beschädigte Zusatzbits können weiterhin zu Ablehnungen oder plausibel wirkenden Fehlinterpretationen führen. Eine größere Reichweite oder fehlerfreier Empfang im gesamten Haus ist damit nicht garantiert.

Bei Problemen zunächst mit derselben Fernbedienung in der Nähe des ESP und anschließend am problematischen Standort testen. AUF, AB und STOP sowie den höchsten Einzelkanal und Gruppen getrennt prüfen. Im Log auf Seriennummer, Befehl und Quellmaske achten. Antenne, Standort, Spannungsversorgung, GPIO-Zuordnung und Masterkeys ebenfalls prüfen. Weniger erkannte Telegramme allein beweisen noch keine Verschlechterung: Der Decoder kann beschädigte Kandidaten nun korrekt verwerfen.

Für eine nachvollziehbare Fehlermeldung Fernbedienungsmodell, gewählten Kanal beziehungsweise Gruppe, tatsächlich gedrückte Taste und die zugehörigen INFO-/DEBUG-Zeilen dokumentieren. Die Signalquelle nicht allein aus einem einzelnen möglicherweise gestörten Empfang ableiten.

## Quellen und Implementierung

Die Glitch-Filter-Idee und Randfallkorrekturen wurden aus der Arbeit von **Reinhard-M** in [Banabas PR #20](https://github.com/Banabas/ESP32-Jarolift-PositionController/pull/20) und [PR #21](https://github.com/Banabas/ESP32-Jarolift-PositionController/pull/21) an den vorhandenen Empfangspuffer angepasst. Der historische Protokollgrundaufbau geht unter anderem auf [Steffens Protokollanalyse von 2017](https://www.bastelbudenbuben.de/2017/04/25/protokollanalyse-von-jarolift-tdef-motoren/) zurück; aktuelle Fernbedienungsmessungen ergänzt [remotes.md](remotes.md).

Maßgeblich für die hier beschriebenen Prüfungen sind [JaroliftController.cpp](../lib/JaroliftController/JaroliftController.cpp), [JaroliftController.h](../lib/JaroliftController/JaroliftController.h) und für die Zuordnung [remoteMapping.cpp](../src/remoteMapping.cpp). Die Beschreibung ersetzt keine Empfangsmessung am eigenen Standort.

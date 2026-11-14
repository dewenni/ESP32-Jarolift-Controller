<div align="center">
<img style="width: 100px;" src="./Doc/ESP32_Jarolift_Controller_Logo.svg">

<h3 style="text-align: center;">ESP32-Jarolift-Controller</h3>
</div>

-----

**[🇬🇧  english version of this description](README.md)**

-----

<div align="center">

[![Current Release](https://img.shields.io/github/release/dewenni/ESP32-Jarolift-Controller.svg)](https://github.com/dewenni/ESP32-Jarolift-Controller/releases/latest)
![GitHub Release Date](https://img.shields.io/github/release-date/dewenni/ESP32-Jarolift-Controller)
![GitHub last commit](https://img.shields.io/github/last-commit/dewenni/ESP32-Jarolift-Controller)
![GitHub Downloads (all assets, all releases)](https://img.shields.io/github/downloads/dewenni/ESP32-Jarolift-Controller/total?label=downloads%20total&color=%23f0cc59)
![GitHub Downloads (all assets, latest release)](https://img.shields.io/github/downloads/dewenni/ESP32-Jarolift-Controller/latest/total?label=downloads%20latest%20Release&color=%23f0cc59)

![GitHub watchers](https://img.shields.io/github/watchers/dewenni/ESP32-Jarolift-Controller?style=social)
[![GitHub stars](https://img.shields.io/github/stars/dewenni/ESP32-Jarolift-Controller.svg?style=social&label=Star)](https://github.com/dewenni/ESP32-Jarolift-Controller/stargazers/)

[![Stargazers over time](http://starchart.cc/dewenni/ESP32-Jarolift-Controller.svg?background=%23ffffff00&axis=%236c81a6&line=%236c81a6)](https://github.com/dewenni/ESP32-Jarolift-Controller/stargazers/)

</div>

-----

<div align="center">
Wenn dir dieses Projekt gefällt, drücke genre auf den <b>[Stern ⭐️]</b> Button and drücke <b>[Watch 👁]</b> um auf dem Laufenden zu bleiben.
<br><br>
Und wenn du meine Arbeit unterstützen möchtest, kannst auch folgendes nutzen <p>

[![Sponsor](https://img.shields.io/badge/Sponsor%20me%20on-GitHub-%23EA4AAA.svg?style=for-the-badge&logo=github)](https://github.com/sponsors/dewenni)

</div>

-----

# ESP32-Jarolift-Controller

Steuerung von Jarolift(TM) TDEF 433MHz Funkrollläden über **ESP32** und **CC1101** Transceiver Modul im asynchronen Modus.

## Features

- **Webbasierte Benutzeroberfläche (WebUI):**  
Eine moderne, mobilfreundliche Schnittstelle für einfache Konfiguration und Steuerung.

- **MQTT-Unterstützung:**  
Die Kommunikation und Steuerung der Geräte erfolgt über MQTT, ein leichtgewichtiges und zuverlässiges Messaging-Protokoll.

- **HomeAssistant-Integration:**  
Automatische Geräteerkennung in HomeAssistant durch MQTT Auto Discovery für nahtlose Integration.

- **Unterstützung für bis zu 16 Rollläden:**  
Steuern von bis zu 16 Rollläden, die alle über die WebUI und MQTT verwaltet werden.

- **Unterstützung für bis zu 6 Rollladengruppen:**  
Definiere bis zu 6 Rollladengruppen, um mehrere Rollläden auf einmal zu steuern

- **Timer Funktion**  
für Stand-Alone Anwendung auch mit integrierter Timer Funktion mit fester Uhrzeit, Sonnenaufgang oder Sonnenuntergang als Auslöser.

### WebUI-Demo

Um sich einen ersten Eindruck von den Funktionen und der WebUI zu verschaffen, steht auch eine eingeschränkte Demo zur Verfügung.  
Diese kann über den folgenden Link aufgerufen werden: [WebUI-DEMO](https://dewenni.github.io/ESP32-Jarolift-Controller/)

Die Verwendung erfolgt auf eigene Gefahr. Nur für den privaten/schulischen Gebrauch. (Keeloq-Algorithmus ist nur für TI-Mikrocontroller lizenziert)
Dieses Projekt ist in keiner Weise mit dem Hersteller der Jarolift-Komponenten verbunden.
Jarolift ist ein Warenzeichen der Schöneberger Rolladenfabrik GmbH & Co. KG

Diese Version ist für einen ESP32 und basiert auf Ideen und Code von [madmartin/Jarolift_MQTT](https://github.com/madmartin/Jarolift_MQTT).

### Project Homepage, Forum und Author

Der ursprüngliche Steuercode wurde von Steffen Hille im Nov. 2017 geschrieben.

Die Homepage des Projekts ist hier: [Project Home](http://www.bastelbudenbuben.de/2017/04/25/protokollanalyse-von-jarolift-tdef-motoren/)

-----

# Inhaltsverzeichnis

- [Hardware](#hardware)
  - [ESP32](#esp32)
  - [CC1101 433Mhz](#cc1101-433mhz)
  - [Optional: Ethernet Modul W5500](#optional-ethernet-modul-w5500)
- [Erste Schritte](#erste-schritte)
  - [Platform-IO](#platform-io)
  - [ESP-Flash-Tool](#esp-flash-tool)
  - [OTA-Updates](#ota-updates)
  - [Setup-Mode](#setup-mode)
  - [Konfiguration](#konfiguration)
  - [Filemanager](#filemanager)
  - [Anlernen von Rolläden](#anlernen-von-rolläden)
  - [Migration](#migration)
- [WebUI](#webui)
  - [Kanäle](#kanäle)
  - [Gruppen](#gruppen)
  - [Timer](#timer)
- [MQTT](#mqtt)
  - [Kommandos](#kommandos)
  - [Status](#status)
  - [Home Assistant](#home-assistant)
- [Optionale Kommunikation](#optionale-kommunikation)
  - [WebUI-Logger](#webui-logger)
  - [Telnet](#telnet)

-----

# Hardware

Du findest funktionierende Setups von Benutzern dieses Projekts hier: [funktionierende Setups](https://github.com/dewenni/ESP32-Jarolift-Controller/discussions/34)

## ESP32

Die Firmware ist aktuell nur für folgende Chips erstellt

**Standard ESP32 (Xtensa® 32-bit LX6, 4MB Flash)**

- `ESP32-WROOM-32 Serie` (z.B. WROOM, WROOM-32D, WROOM-32U)
- `ESP32-WROVER Serie` (z. B. WROVER, WROVER-B, WROVER-IE)
- `ESP32-MINI Serie`
- `ESP32-S2 series`
- `ESP32-S3 series`
- `ESP32-C3 series`

**nicht Kompatibel:**

- `ESP32-H series`
- `YB-ESP32-S3-ETH`
- `WT32-ETH01`

## CC1101 433Mhz

**kompatible und getestete Produkte:**

- `EBYTE E07-M1101D-SMA V2.0`
- `CC1101 433MHZ Green`

eine Standard-SPI-GPIO-Konfiguration für den CC1101 und den ESP32 ist folgende:

| CC1101-Signal| ESP-GPIO|
|--------------|---------|
| VCC          | --      |
| GND          | --      |
| GD0          | 21      |
| GD2          | 22      |
| SCK/CLK      | 18      |
| MOSI         | 23      |
| MISO         | 19      |
| CS(N)        | 5       |

<img style="width: 500px;" src="./Doc/ESP32_CC1101_Steckplatine.png">

<img style="width: 500px;" src="./Doc/ESP32_CC1101_Schaltplan.png"> 

<img style="width: 500px;" src="./Doc/hw_1.png">

Beispiel mit ESP32-Mini und CC1101

<img style="width: 500px;" src="./Doc/hw_2.png">

Beispiel für direkten Austausch mit ESP32-Mini und dem Custom Board von M. Maywald (vermutlich nicht mehr verfügbar)

## Optional: Ethernet Modul W5500

Es ist auch möglich, ein W5500 Ethernet-Modul an das Board oder einen generischen ESP32 anzuschließen.

**kompatible und getestete Produkte:**

- `W5500` HanRun (HR911105A)
- `W5500 Lite` HanRun (HR961160C)

> [!IMPORTANT]
> Das Anschlusskabel sollte so kurz wie möglich sein (ca. 10 cm).

Beispiel für einen generischen ESP32-Mini (Standard SPI Port wird vom CC1101 verwendet)

| Signal| GPIO |
|-------|------|
| CLK   | 25   |
| MOSI  | 26   |
| MISO  | 27   |
| CS    | 32   |
| INT   | 33   |
| RST   | 17   |

-----

# Erste Schritte

## Platform-IO

Die Software wurde mit [Visual Studio Code](https://code.visualstudio.com) und dem [pioarduino-Plugin](https://github.com/pioarduino/pioarduino-vscode-ide) erstellt.  
Nach der Installation der Software kannst du das Projekt von GitHub klonen oder als zip herunterladen und in PlatformIO öffnen.
Dann noch den `upload_port` und die entsprechenden Einstellungen in `platformio.ini` an deinen USB-zu-Seriell-Adapter anpassen den Code auf den ESP hochladen.

> [!NOTE]
> Python muss ebenfalls installiert sein, um das Projekt vollständig zu kompilieren. Der Ordner scripts enthält beispielsweise Skripte für die Erstellung der Webseiten, die beim Kompilieren des Projekts aufgerufen werden.

## ESP-Flash-Tool

In den Veröffentlichungen (Releases) befinden sich auch die Binärdatei der Software. Wenn du PlatformIO nicht verwenden willst, kannst du auch die Datei `esp32_jarolift_controller_flash_vx.x.x.bin` verwenden und direkt auf den ESP flashen. Diese bin-Datei ist bereits ein Fertig mit bootloader.bin, partitions.bin und der application.bin. Du kannst dieses Image auf den ESP an Adresse 0x00 flashen.  

**Windows**  
Es gibt verschiedene Tools, um Binärdateien auf den ESP zu übertragen.  
Eines davon ist [espressif-flash-download-tool](https://www.espressif.com/en/support/download/other-tools)

**macOS/Linux**  
Für Mac ist es schwierig, ein Tool mit einer grafischen Benutzeroberfläche zu finden, aber es kann einfach das esptool.py verwendet werden:

1. Terminal öffnen
2. esptool installieren: `pip install esptool`  
3. optional den Installationspfad abfragen: `welches esptool.py`  
4. Pfad setzen: `export PATH=„$PATH:/<Pfad>/esptool.py“` (<- ändere <Pfad> mit Ergebnis aus 3.)
5. Gehe zu dem Pfad, in dem sich die bin-Datei befindet
6. Device String abfragen: `ls /dev/tty* | grep usb` (verwende dies im nächsten Schritt für <UPLOAD-PORT>)
7. Upload: `esptool.py -p <UPLOAD-PORT> write_flash 0x00 esp32_jarolift_controller_flash_vx.x.x.bin`  

## OTA-Updates

### lokales Web OTA-Update

Eine Möglichkeit ist, die OTA-Update-Datei von der neuesten Version auf GitHub herunterzuladen.
Nachdem man diese auf seinen Computer heruntergeladen hat, kann man ein Update mit dem eingebetteten WebUI OTA-Update durchführen.
Die Update Funktion befindet sich auf der Registerkarte „Tools“ der WebUI.

Hier kannst du einfach die Datei `esp32_jarolift_controller_ota_update_vx.x.x.bin` aus dem Release-Bereich auswählen und das Update starten.

![ota-1](Doc/webUI_ota.gif)

### GitHub OTA-Update

Seit Version 1.4.0 ist es auch möglich, den Controller direkt in der WebUI zu aktualisieren, ohne vorher die .bin-Datei herunterzuladen.
Wenn man auf die Versionsinfo unten links klickt, öffnet sich ein Dialog. Wenn eine neue Version verfügbar ist, kann man das Update hier direkt anstoßen. Es wird dann automatisch die neueste Version von github heruntergeladen und installiert!

![ota-2](Doc/github_ota.gif)

### PlatformIO OTA-Update

Aber es ist auch möglich, die Software drahtlos mit Platformio herunterzuladen.
Setze `upload_port` in der `platformio.ini` und ergänze für drahtlose Uploads `upload_flags = --auth=<Passwort>`. ArduinoOTA verwendet das Passwort der WebUI-Auth-Einstellungen, auch bei deaktivierter Web-Authentifizierung. Ohne gesetztes Passwort gilt `jarolift-2026!`.

Unterbrochene Browser-Uploads werden nach 60 Sekunden ohne Fortschritt abgebrochen; der vorherige Watchdog-Zustand wird wiederhergestellt. Neue OTA-Firmware wird nach zwei Minuten Laufzeit mit Netzwerkverbindung bestätigt, ohne Netzwerk nach zehn Minuten. Ein Neustart vor dieser Bestätigung kann den Bootloader-Rollback zur vorherigen Firmware auslösen.

Es gibt zwei vordefinierte Optionen:

- OPTION 1: direct cable upload
- OPTION 2: wireless OTA Update

## Setup Mode

Es ist ein `Setup Mode` verfügbar. Der „Setup Mode“ wird aktiviert, wenn der ESP **5** mal neu gestartet wird.
Nach jdem Neustart dürfen maximal bis zu 5 Sekunden vergehen.

Beispiel: restart 1/5 - 2s warten - restart 2/5 - 2s warten - restart 3/5 - 2s warten - restart 4/5 - 2s warten - restart /5/5 => Setup-Mode

Der `Setup-Mode` wird auch aktiviert, wenn kein gültiges WLAN und keine gültige ETH-Verbindung konfiguriert ist.

Wenn der ESP in den „Setup Mode“ geht, erstellt er automatisch einen eigenen Netzwerk Accesspoint mit der ssid
📶 `"ESP32_Jarolift"`  
Nachdem du mit diesem Netzwerk verbunden bist, kannst du die WebUI übernachfolgende Adresse öffnen  
**http://192.168.4.1**

## Konfiguration

Hier können alle Konfigurationen vorgenommen werden, die zu der Heizungsanlage und der Infrastruktur passen.

- **WiFi**  
Gib im Feld „WiFi“ deine WLAN Anmeldedaten ein, um den ESP mit Ihrem Netzwerk zu verbinden.

- **Ethernet W5500**  
Verwende optional die Ethernet-Verbindung auf Basis des W5500, um den ESP mit dem Netzwerk zu verbinden.

- **Authentifizierung**  
Hier kann optional die Authentifizierungsfunktion aktiviert werden und Benutzer und Passwort konfiguriert werden.

- **NTP-Server**  
Der ESP kann sich mit einem NTP-Server verbinden, um die richtigen Zeitinformationen zu erhalten.
Die voreingestellte Zeitzone sollte passen, wenn du dich in Deutschland befindest. Andernfalls können diese manuell geändert werden.

- **MQTT**  
hier können Sie die MQTT-Kommunikation aktivieren und obligatorische Parameter eingeben.  

- **GPIO**  
Hier kann man die GPIO konfigurieren, um den CC1101 mit dem ESP32 zu verbinden

- **Jarolift**  
Hier müssen bestimmte Jarolift-spezifische Protokolleinstellungen vorgenommen werden

- **Shutter**  
hier können die einzelnen Rollläden mit individuellen Namen konfiguriert werden

- **Group**  
Hier können wahlweise Rollladengruppen definiert werden

- **Originalfernbedienungen**

  Unter Einstellungen → Gerät → Originalfernbedienungen mit „Zuordnung hinzufügen“ ein Tastensignal erfassen (STOP ist ebenfalls möglich). Den gewünschten Kanal oder die Gruppe auf der Fernbedienung auswählen, die Zuordnung benennen und die angesprochenen ESP-Rollos auswählen. Vollständige Seriennummer und empfangene Kanalmaske identifizieren den Eintrag gemeinsam. Vorhandene Einträge lassen sich bearbeiten oder löschen. Löschen entfernt nur die ESP-Zuordnung; die Fernbedienung bleibt am Motor angelernt. Alte sechsstellige Präfix-Zuordnungen müssen neu erfasst werden.

- **Language**  
Es sind zwei Sprachen verfügbar. Wählen deine bevorzugte Sprache.

> [!NOTE]
> Alle Einstellungen werden automatisch gespeichert, wenn Änderungen vorgenommen werden.

> [!IMPORTANT]
> Changes to GPIO or Jarolift settings require a restart!

![weubui-settings](Doc/webUI_settings.png)

## Filemanager

Es gibt es auch einen eingebauten Dateimanager zum Öffnen (anzeigen), Herunterladen (exportieren) und Hochladen (importieren) der Konfigurationsdatei.
Die Konfiguration wird in der Datei ``config.json`` gespeichert. Zur Sicherung und Wiederherstellung der Konfiguration kannst du diese Datei herunterladen und hochladen.

![filemanager](/Doc/webUI_tools.png)

## Anlernen von Rolläden

Es gibt grundsätzlich mehrere Möglichkeiten um einen Rolladen anzulernen.
Es existieren die gleichen Möglichkeiten wie bei der Nutzung der original Fernbedienungen.

### Anlernen durch drücken der Anlerntaste am Motor

Jeder TDEF Motor hat eine Taste zum anlernen von neuen Fernbedienungen.
Drückt man diese Taste, bestätgt der Motor den Anlernvorgang mit einem vibrieren.

> [!TIP]
> Wenn man an die Taste nicht ran kommt, kann man den Motor auch für ein paar Sekunden Stromlos schalten. Z.B. in dem man die Sicherung kurz raus macht.

Jetzt innerhalb von 5 Sekunden den entsprechenden "Lern-Button" im WebUI in den Settings bei dem jeweiligen Rolladen drücken.
Wenn der Rolladen erfolgreich eingelernt wurde, vibriert der Motor erneut.

### Anlernen durch Kopieren eines bestehenden Funkcodes

Alternativ kann man auch eine bereits eingelernte Fernbedienung nutzen und diese "Kopieren".
Dazu auf dem bereits eingelernten Sender die AUF- und AB-Taste gleichzeitig. Danach, auf diesem Sender, die STOP-Taste
acht mal drücken. Der Motor wird zur Bestätigung kurz vibrieren.

Jetzt innerhalb von 5 Sekunden den entsprechenden "Lern-Button" im WebUI in den Settings bei dem jeweiligen Rolladen drücken.
Wenn der Rolladen erfolgreich eingelernt wurde, vibriert der Motor erneut.

## Migration von madmartin/Jarolift_MQTT

Es ist möglich, von einer vorherigen Version von [madmartin/Jarolift_MQTT](https://github.com/madmartin/Jarolift_MQTT) zu diesem Projekt zu migrieren.

#### Ein funktionierendes Setup für dieses Projekt herstellen

- eine funktionierende Version dieses Projekts zum Laufen zu bringen
- die richtigen GPIO-Einstellungen für den CC1101 setzen
- die Master Keys setzen (entweder hat man die oder man findet sie)
- den Log-Level im Logger der WebUI auf „Debug“ setzen

#### Ermitteln Sie die richtige Seriennummer

- Führe einen Shutter HOCH Befehl des „alten“ Setups `(madmartin/Jarolift_MQTT)` für **Kanal 0!** aus.
- Du solltest nun eine Debug-Meldung dieses Befehls im Logger der WebUI sehen.  
Sie enthält eine Meldung mit   `I (220364) JARO: received remote signal | serial: 0c7c00 | ch: 3 | cmd: 0x8, | rssi: -96 dbm` 
Dies sollte die gleiche Seriennummer sein, die in der WebUI des „alten“ Setups konfiguriert wurde, aber jetzt sind wir sicher, dass wir die richtige haben.
- Setze diese Seriennummer in der WebUI dieses Projekts in den Einstellungen.


#### Den richtigen Gerätezähler finden

- den aktuellen Gerätezähler der „alten“ Konfiguration auf der System-Seite der WebUI ablesen.
- den gleichen Wert für den Gerätezähler in den Einstellungen dieses Projekts einstellen.


#### Die Rollläden definieren

- Definiere die gleichen Rolläden in der gleichen Reihenfolge wie in der „alten“ Einstellung und aktiviere diese.

fertig!  
Starte nun den ESP neu und teste.  
Prüfe nach dem Neustart zunächst, ob der Gerätezähler korrekt aus dem EEPROM gelesen wurde. Nur wenn dies der Fall ist, fahre mit dem Test fort.
Wenn alles richtig gemacht wurde, sollten alle Rollläden wie vorher funktionieren. Wenn nicht, ist eine Einstellung falsch oder du hast zuletzt nicht die neueste Version von `(madmartin/Jarolift_MQTT)` verwendet. In diesem Fall würde ich es vorziehen, eine neue Seriennummer zu setzen, den Gerätezähler zurückzusetzen und die Rolläden neu einzulernen.

-----

# WebUI

Das WebUI ist responsive und bietet auch eine Layout für Mobile Geräte

![weubui_dash](Doc/webUI_1.png)
(Desktop Version)

<img style="display: inline;
  margin-right: 50px;
  width: 200px;" src="./Doc/webUI_mobile_1.png">
<img style="display: inline;
  margin-right: 50px;
  width: 200px;" src="./Doc/webUI_mobile_2.png">

(Mobile Version)

## Channels

Nachdem die Rolläden in den Einstellungen konfiguriert und aktiviert wurden, können diese auch direkt im WebUI bedient werden.

![webUI_shutter](/Doc/webUI_shutter.png)

## Groups

Die in den Einstellungen konfigurierten Gruppen können wie die einzelnen Rollläden auch direkt im WebUI bedient werden.

![webUI_groups](/Doc/webUI_groups.png)

## Timer

Die Timer ermöglicht die automatische Steuerung einzelner Rollläden oder eine Auswahl mehrerer Rollläden als Gruppe.
Als Auslöser kann ein fester Zeitpunkt oder Sonnenaufgang bzw. Sonnenuntergang mit optionalem Zeitversatz vorgegeben werden.

![webUI_timer](/Doc/webUI_timer.png)

Die Auswahl von Rollläden wird durch einen zusätzlichen Dialog unterstützt. Dort werden alle konfigurierten und aktivierten Rollläden angezeigt. Diese können dort ausgewählt werden und die benötigte Bitmaske wird dann automatisch berechnet.

<img style="width: 444px;" src="./Doc/webUI_bitmask_wiz.png">

-----

# MQTT

## Kommandos

### Rolläden

Zur Steuerung der Rollläden können die folgenden mqtt-Befehle verwendet werden.
{UP, OPEN, 0} bedeutet, dass man einen der aufgeführten Payload-Befehle verwenden kann.

```text
command:    Neustart ESP
topic:      ../cmd/restart
payload:    none

command:    Rolladen hoch
topic:      ../cmd/shutter/1 ... cmd/shutter/16
payload:    {UP, OPEN, 0}

command:    Rolladen runter
topic:      ../cmd/shutter/1 ... cmd/shutter/16
payload:    {DOWN, CLOSE, 1}

command:    Rolladen stopp
topic:      ../cmd/shutter/1 ... cmd/shutter/16
payload:    {STOP, 2}

command:    Rolladen Schatten
topic:      ../cmd/shutter/1 ... cmd/shutter/16
payload:    {SHADE, 3}

```

### vordefinierte Gruppen

Zur Steuerung der Rollläden über eine vordefinierte Gruppe, können die folgenden mqtt-Befehle verwendet werden.
{UP, OPEN, 0} bedeutet, dass man einen der aufgeführten Payload-Befehle verwenden kann.

```text

command:    Gruppe hoch
topic:      ../cmd/group/1 ... cmd/group/6
payload:    {UP, OPEN, 0}

command:    Gruppe runter
topic:      ../cmd/group/1 ... cmd/group/6
payload:    {DOWN, CLOSE, 1}

command:    Gruppe stopp
topic:      ../cmd/group/1 ... cmd/group/6
payload:    {STOP, 2}

command:    Gruppe Schatten
topic:      ../cmd/group/1 ... cmd/group/6
payload:    {SHADE, 3}

```

### Gruppe mit Bitmaske

Man kann auch einen generischen Gruppenbefehl verwenden und die Bitmaske angeben, um die Rollläden direkt auszuwählen.  
Die Bitmaske ist eine 16-Bit-Zahl, wobei das niedrigstwertige Bit (rechts) für Kanal 1 steht.  
Ein gesetztes Bit bedeutet, dass der Kanal zu dieser Gruppe gehört.  

**Beispiel**: `0000000000010101` bedeutet, dass die Kanäle 1, 3 und 5 zu dieser Gruppe gehören.

Als Nutzdaten können Sie drei verschiedene Formate verwenden, um die gleiche Bitmaske darzustellen:

- **Binary**: `0b0000000000010101`
- **Hex**: `0x15`
- **Decimal**: `21`

```text

command:    Gruppe hoch
topic:      ../cmd/group/up
payload:    {0b0000000000010101, 0x15, 21}

command:    Gruppe runter
topic:      ../cmd/group/down
payload:    {0b0000000000010101, 0x15, 21}

command:    Gruppe stopp
topic:      ../cmd/group/stop
payload:    {0b0000000000010101, 0x15, 21}

command:    Gruppe Schatten
topic:      ../cmd/group/shade
payload:    {0b0000000000010101, 0x15, 21}

```

## Status

## Rolladen-Status

Der Controller sendet auch einen Status, **basierend auf den empfangenen Kommandos.**  

Diese Werte sind Schätzungen ohne Rückmeldung des Motors: **0 geschlossen, 100 geöffnet**, Zwischenwerte entsprechend der Zielanfahrt. Das ist ein Breaking Change gegenüber der bisherigen umgekehrten Skala. Bei kalibrierten Kanälen wird nach AUF/AB erst nach vollständiger Fahrzeit plus Reserve eine Endlage angenommen; ohne Kalibrierung wird wie bisher sofort die erwartete Endlage gemeldet. STOP und SCHATTEN setzen den Status auf `None` (unbekannt). Empfangene zugeordnete Originalfernbedienungsbefehle werden auch ohne MQTT berücksichtigt. Nicht empfangene Signale bleiben unerkannt. Nach MQTT-Reconnect wird der aktuelle Stand erneut veröffentlicht. Die Befehlskennungen (0 = UP, 1 = DOWN) bleiben unverändert und sind keine Prozentwerte.

```text

Status:     Rolladen OFFEN
topic:      ../status/shutter/1 ... status/shutter/16
payload:    {100}

Status:     Rolladen GESCHLOSSEN
topic:      ../status/shutter/1 ... status/shutter/16
payload:    {0}

Status:     Rolladen SCHATTEN
topic:      ../status/shutter/1 ... status/shutter/16
payload:    {None}
```

### Prozentziele / Home Assistant

Bei global aktivierter Positionssteuerung und gültiger Kanalkalibrierung akzeptiert `../cmd/position/1` bis `/16` Ganzzahlen von 0 (geschlossen) bis 100 (geöffnet), ohne Retain. Die Fahrt nutzt dieselbe Referenzfahrt wie im WebUI. Discovery ergänzt die Prozentsteuerung automatisch für diese Kanäle und entfernt sie beim Deaktivieren oder bei ungültiger Kalibrierung. AUF/AB/STOP bleiben verfügbar; Gruppen-Prozentfahrten sind nicht enthalten.

### Remotes (Fernbedienungen)

Sind in den Einstellungen auch originale Fernbedienungen über die Seriennummer hinterlegt, dann können auch Signale dieser Fernbedienungen erfasst und per MQTT ausgegeben werden. Die Information darüber, welcher Rollladen gesteuert wird, ist in den beiden Variablen `chBin` und `chDec` zu sehen. chBin“ zeigt den verwendeten Rollladen als 16-Bit-Binärwert an, während ‚chDec‘ dieselbe Information zur einfacheren Automatisierung als Dezimalwert anzeigt.

```json
topic:      "../status/remote/<serial-number>"
payload:    {
              "name":   "<alias-name>", 
              "cmd":    "<UP, DOWN, STOP, SHADE>",
              "chBin":  "<channel-binary>",
              "chDec":  "<channel-decimal>"
            }
```



> [!NOTE]
> < ../ > ist der Platzhalter für das MQTT-Topic, das in den Einstellungen angegeben ist.

## zusätzliche Informationen (nur lesen)

Statusinformationen über WiFi:

```text
Topic: ESP32-Jarolift-Controller/wifi = {  
    "status":"online",  
    "rssi":"-50",  
    "signal":"90",  
    "ip":"192.168.1.1",  
    "date-time":"01.01.2022 - 10:20:30"  
}
```

## Home Assistant

MQTT Discovery für Home Assistant macht es einfach, alle Werte in Home Assistant zu erhalten.
Die konfigurierten Rollläden werden automatisch als mqtt-Gerät in Home Assistant angezeigt, wenn HomeAssistant aktiviert ist.

siehe auch die: [offizielle Dokumentation](https://www.home-assistant.io/integrations/mqtt/#discovery-messages)

<img src="Doc/webUI_ha2.png" alt="mqtt_ha1" width="75%">

In den mqtt-Einstellungen können die Funktion zur Erkennung aktiviert sowie das mqtt-Topic und der Gerätename für den Home Assistant festgelegt werden.

<img src="Doc/webUI_ha1.png" alt="mqtt_ha1" width="50%">

-----

# Optionale Kommunikation

Zusätzlich zu mqtt gibt es weitere Kommunikationsmöglichkeiten.

## WebUI-Logger

Außerdem gibt es eine Log-Funktion, mit der je nach Filter verschiedene Meldungen aufgezeichnet und über die WebUI angezeigt werden können. Dies kann für das eigene Debugging und auch für die Weiterentwicklung der Software nützlich sein.

<img src="./Doc/webUI_Logger.png" width="75%">

## Telnet

Neben der WebUI und MQTT gibt es auch eine Telnet-Schnittstelle zur Kommunikation mit dem ESP.
Die Schnittstelle bietet mehrere Befehle, um Informationen auszulesen und Befehle zu senden.
Eine Übersicht über die Befehle kann mit dem Befehl „help“ aufgerufen werden.
Um eine Verbindung herzustellen, kann eine einfache Telnet-Verbindung über die entsprechende IP-Adresse des ESP gestartet werden.

Jede Verbindung erfordert das in den Auth-Einstellungen der WebUI konfigurierte Passwort, auch bei deaktivierter Web-Authentifizierung. Ist kein Passwort gesetzt, gilt das Ersatzpasswort `jarolift-2026!`. Bitte ein eigenes Passwort in der WebUI setzen. Bei falschem Passwort wird die Verbindung getrennt; beim erneuten Verbinden ist eine neue Anmeldung erforderlich.

Beispiel:

`> telnet 192.168.178.193`

<img src="./Doc/telnet.png" width="75%">

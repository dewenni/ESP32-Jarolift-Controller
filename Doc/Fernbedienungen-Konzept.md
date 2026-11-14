# Konzept: Originalfernbedienungen den Rollläden zuordnen

Status: Vorschlag zur Abstimmung, keine Implementierung.
Grundlage: die korrigierten Messungen in `Doc/remotes.md`.

## Ziel und Bedienprinzip

Die Bedienung erfolgt aus Sicht des Rollladens: Im Service-Menü wird ein
Rollladen ausgewählt und anschließend festgelegt, welche Originalfernbedienungen
beziehungsweise deren Kanäle ihn bedienen. Ein Quellkanal darf mehrere Motoren
bedienen; ein Motor darf mehreren Quellkanälen zugeordnet sein.

Der Nutzer muss keinen Fernbedienungstyp, keine Seriennummer und keine Bitmaske
konfigurieren. Diese Informationen werden aus dem empfangenen Signal gewonnen.
Die Modellbezeichnungen in der Messdokumentation dienen nur der Nachvollziehbarkeit.

Diese Registrierung lernt nichts im Motor an und sendet keine Anlernbefehle.
Sie informiert ausschließlich den ESP über bereits bestehende Zuordnungen,
damit MQTT-Status und Positionsschätzung auch externe Bedienung berücksichtigen.

## Gemeinsame Regel aus den Messungen

Die korrigierten Beispiele passen zu einer einheitlichen Auswertung:

```cpp
senderKey = serial & 0x0ffffff0;
betroffen = gleicherSenderKey && ((empfangeneMaske & gelernterKanal) != 0);
```

Nur die letzten vier Seriennummern-Bits werden ausgeblendet, nicht die letzten
beiden Hexstellen oder ein beliebig großer Präfix. Die vollständige Seriennummer
bleibt für Diagnose und Nachvollziehbarkeit im gespeicherten Eintrag erhalten.

| Messung | Ergebnis der gemeinsamen Regel |
| --- | --- |
| TDRC01W: `033d9c00`, Maske `0001` | Senderkennung `033d9c00`, Quellbit 0. |
| TDRC02W links: `04d46401`, Maske `0002` | Senderkennung `04d46400`, Quellbit 1. |
| TDRC02W rechts: `04d46501`, Maske `0002` | Andere Senderkennung `04d46500`, ebenfalls Quellbit 1. Die beiden Auswahlen bleiben getrennt. |
| TDRCT04W: `0c990000..003`, Gruppe `0c990004` | Gemeinsame Senderkennung `0c990000`; die Maske bestimmt Einzelkanal oder Gruppe. |
| TDRC16W: Einzelkanäle und wechselnde Cursor-Endungen | Gemeinsame Senderkennung `004c0000`; die vollständige 16-Bit-Maske bestimmt die angesprochenen Quellkanäle. |

Eine physische Fernbedienung kann mehrere solche Senderkennungen verwenden,
wie die TDRC02W. Der frei gewählte Name darf mehrere Einträge zusammenfassen;
eine aus der Seriennummer abgeleitete Kennung ist keine universelle Geräte-ID.
Eine angezeigte Kanalbezeichnung nicht allein aus der Bitposition herleiten:
Die TDRC02W verwendet bei beiden Auswahlen dieselbe Maske.

Das ist eine durch die vorliegenden Messungen gestützte Regel. Daraus folgt
noch kein Nachweis, wie die Motorfirmware intern arbeitet oder dass sämtliche
nicht getesteten Fernbedienungen ebenso aufgebaut sind. Vor Veröffentlichung
mit allen dokumentierten Sendern am echten System überprüfen.

## Service-Seite: Zuordnung hinzufügen

Die vorhandene Rollladenauswahl und die Service-Bedienung wiederverwenden.
Darunter ein Bereich „Originalfernbedienungen“:

> Schlafzimmer links
>
> Zugeordnet: Schlafzimmer · linke Taste
>
> [Fernbedienung zuordnen]

Ablauf des Dialogs:

1. **Einzelkanal auswählen:** auf der Fernbedienung genau den Kanal wählen,
   auf dem der Motor bereits angelernt ist. Möglichst STOP drücken; UP/DOWN
   sind ebenfalls erfassbar, können aber reale Motorbewegungen auslösen.
2. **Signal erfassen:** Seriennummer und Maske automatisch übernehmen. Im
   normalen Ablauf genau ein gesetztes Quellbit verlangen. Bei mehreren Bits
   „Gruppensignal erkannt – bitte einen Einzelkanal auswählen“ anzeigen.
3. **Bestätigen:** einen zweiten gezielten Tastendruck zum Vergleich erfassen.
   Zwei getrennte kurze Tastendrücke genügen; kein bestimmtes Halten erforderlich.
   Unterschiedliche Befehle dürfen dieselbe Quellidentität bestätigen.
4. **Benennen und speichern:** zum Beispiel „Schlafzimmer · linke Taste“ oder
   „Zentral · Kanal 4“. Der ausgewählte Rollladen ist das Ziel und muss nicht
   erneut in einer Liste gesucht werden. Existiert die Quelle bereits, dessen
   Zielmenge ergänzen statt bestehende Ziele zu ersetzen.

Die zwei Erfassungen dienen nur zur Bestätigung beim Registrieren. Im normalen
Betrieb keine pauschale Pflicht zu zwei Wiederholtelegrammen einführen.

## Übersicht und Entfernen

Im Service-Bereich nur Zuordnungen des ausgewählten Rollladens zeigen:

| Fernbedienung / Auswahl | Aktion |
| --- | --- |
| Schlafzimmer · linke Taste | Entfernen |
| Zentral · Kanal 4 | Entfernen |

Die Verwaltung bleibt vollständig im Service-Menü beim ausgewählten Rollladen.
Keine zweite Liste in den Einstellungen und kein Feld für Modell oder Profil.
Bekannte Quellen erneut erfassen, um ihre gemeinsame Bezeichnung zu ändern.

„Entfernen“ am Rollladen entfernt nur diese Beziehung. Andere Ziele derselben
Quelle bleiben bestehen. Das Löschen eines gesamten Eintrags entfernt nur die
ESP-Zuordnungen, nicht das Anlernen im Motor.

## Einzel- und Gruppenbefehle auswerten

Für jedes empfangene Telegramm die Senderkennung berechnen. Anschließend alle
Einträge mit gleicher Senderkennung und einem in der empfangenen Maske gesetzten
Quellbit suchen. Ihre ESP-Zielmasken bitweise vereinigen.

Damit gilt dieselbe Auswertung für Einzel- und Gruppenbedienung. Die vollständige
Gruppenmaske muss nicht exakt einem gespeicherten Wert entsprechen. Wechselnde
Cursor-Endungen erzeugen keine zusätzlichen Anlerneinträge.

Beispiel: Quellkanal 1 → Küche, 4 → Wohnzimmer, 8 → Büro.
Eine Gruppenmaske mit Bits 1/4/8 betrifft alle drei Rollläden. Nicht zugeordnete
Quellkanäle ignorieren; keine Motorzuordnung erraten. Ein Motor wird auch dann
nur einmal berücksichtigt, wenn mehrere passende Einträge ihn enthalten.

Die dokumentierte Maske `0x403f` enthält Kanal 15 zusätzlich zu 1–6. Dieses Bit
nicht als bloße Cursorinformation herausfiltern. Beim Gerätetest prüfen, welche
Motoren tatsächlich reagieren; bis dahin das Bit als Teil der empfangenen Maske
behandeln und keine abweichende Sonderregel einführen.

UP, DOWN, STOP und SHADE sind Aktionen derselben Quelle, keine separaten
Anlerneinträge. Das Ergebnis an den bestehenden Pfad für Originalfernbedienungen
übergeben: betroffene Zielanfahrt abbrechen, gemeinsame Positionslogik und MQTT
aktualisieren. Keine separaten Zustände für WebUI, MQTT und Fernbedienung führen.

## Datenhaltung und bestehende Einträge

Maximal fünf Zuordnungen pro Rollladen und 80 unterschiedliche Quellen insgesamt.
Gruppensignale belegen keine zusätzlichen Einträge. Bezeichnungen sind auf
32 Unicode-Zeichen begrenzt (bis zu 128 UTF-8-Bytes plus Abschlussbyte).
Die feste Tabelle benötigt 11.520 Bytes RAM. Je Zuordnung genügen:

- Bezeichnung;
- erfasste vollständige Seriennummer;
- Quellkanal als 16-Bit-Maske mit genau einem gesetzten Bit;
- Zielmaske der ESP-Kanäle.

Die Senderkennung lässt sich aus der Seriennummer berechnen und muss nicht
zusätzlich gespeichert werden. Modellprofile und eine zweite Gerätetabelle
sind für diese Auswertung nicht erforderlich. JSON und vorhandene WebSocket-
Kommandos weiterverwenden; keine zusätzlichen dauerhaften Polling-Timer.

Bestehende Einzelkanal-Einträge lassen sich auf die neue Auswertung umstellen.
Dabei auf doppelte Quellen mit gleicher Senderkennung und gleichem Quellbit
prüfen und deren Zielmengen erhalten. Vorhandene explizite Gruppen-Einträge
nicht blind aufteilen: Aus „Gruppe → mehrere Motoren“ ist unbekannt, welches
Einzelbit zu welchem Motor gehört. Diese Einträge als neu zuzuordnen kennzeichnen
und bis zur Klärung nicht mit der neuen Kanalregel auswerten. Die alte
Gruppenzuordnung nicht stillschweigend als erfolgreiche Migration ausgeben.

## Empfangsqualität und Diagnose

Die Zuordnung ersetzt keine Telegrammprüfung. Besonders Kanal 16 muss mitsamt
dem letzten Kanalbit zuverlässig erfasst werden. Fehlende Maskenbits nicht aus
der Seriennummer ergänzen. Empfangsfehler und falsche Befehle sind separat von
der Zuordnungslogik zu behandeln.

Ein optionaler Test im Service-Dialog zeigt nur: erkannte Quelle, Kanalmaske,
Befehl und daraus aufgelöste Rollläden. Der Test selbst sendet keine Motorbefehle.
Wiederholtelegramme bleiben normale Funkwiederholungen; bestehende Bewegungs-
und Endlagentimer dadurch nicht immer neu starten.

## Umsetzung in getrennten Schritten

Für die produktive Umsetzung einen eigenen Feature-Branch anlegen. Vorhandene
unabhängige Änderungen nicht in dieses Thema hineinziehen.

1. Empfang des letzten Kanalbits separat absichern beziehungsweise den aktuellen
   Fix mit Kanal 16 überprüfen.
2. Gemeinsame Senderkennung und Maskenüberschneidung implementieren; Einzel- und
   Gruppenbeispiele aus `remotes.md` als gezielte Tests verwenden.
3. Anlerndialog aus Sicht des Rollladens und Entfernen einzelner Beziehungen
   umsetzen, ohne Typauswahl. Bestehende Einzelzuordnungen erhalten; vorhandene
   Gruppeneinträge gesondert behandeln.
4. Mit allen dokumentierten Originalsendern testen: verschiedene Cursorstände,
   Kanal 16, TDRC02W links/rechts, mehrere Quellen pro Motor und mehrere Motoren
   pro Quelle sowie MQTT-/Positionsabgleich.

Jeden abgeschlossenen Schritt separat committen. Keine produktive Umsetzung
oder Veröffentlichung allein durch die Freigabe dieses Konzepts.

## Umsetzung / Prüfung

Konfigurationsversion 7. Version-6-Einzelquellen werden anhand der Senderkennung
zusammengeführt, ihre Zielmasken vereinigt. Alte Gruppen bleiben sichtbar, werden
aber nicht ausgewertet. Lange alte Namen werden an einer UTF-8-Zeichengrenze
gekürzt. Bestehende/importierte Listen mit mehr als fünf Beziehungen bleiben
erhalten; neue Beziehungen sind erst nach Entfernen überzähliger Einträge möglich.
Beim Laden höchstens 80 Quellen übernehmen.

Zwei Tastendrücke werden anhand von mindestens 400 ms Empfangspause zwischen
passenden Telegrammen unterschieden. Normale Funkwiederholungen desselben
Tastendrucks bestätigen nicht sofort. Dies ist eine Bedienhilfe, kein Nachweis
der Authentizität eines Funksignals. Erfassung und Bestätigung laufen nach
30 Sekunden ab, ein bestätigter Dialog nach zwei Minuten.

Softwaretests prüfen Zuordnung, Gruppen, Grenzen, UTF-8, Migration, Besitz des
Dialogs und Zeitüberschreitungen. Funkempfang und Zuordnung müssen zusätzlich
mit den dokumentierten Fernbedienungen an der Hardware geprüft werden.

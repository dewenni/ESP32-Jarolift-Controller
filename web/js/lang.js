// --------------------------------------
// localization texts
// --------------------------------------
const user_translations = {
  timer: {
    de: "Timer",
    en: "Timer",
  },
  led_setup: {
    de: "LED-Setup-Mode",
    en: "LED-Setup-Mode",
  },
  learn_mode: {
    de: "neuer Anlernmodus",
    en: "new learn mode",
  },
  shutter: {
    de: "Rolladen",
    en: "Shutter",
  },
  serial_nr: {
    de: "Seriennummer",
    en: "serial number",
  },
  set_shade: {
    de: "setze Schatten",
    en: "set shade",
  },
  learn: {
    de: "anlernen",
    en: "learn",
  },
  channel: {
    de: "Kanal",
    en: "Channel",
  },
  info_leanmode: {
    de: "Der neue Lernmodus ist für Empfänger neuer als 2010, die eine Lernsequenz haben, bei der die Tasten UP+DOWN gleichzeitig gedrückt werden und dann die Taste STOP. Die alte Lernmethode für Empfänger, die vor 2010 hergestellt wurden, verwendet eine spezielle LEARN-Taste.",
    en: "new learn mode is for receivers newer than 2010, which have a learning sequence of buttons UP+DOWN pressed simultaneously, then press STOP. the old learn method for receivers manufactured before 2010 uses a special LEARN button.",
  },
  info_serial: {
    de: "Jeder Kanal benötigt eine individuelle Seriennummer. Wenn Sie hier ein neues Seriennummernpräfix eingeben, ändern sich die Seriennummern für alle Kanäle. ACHTUNG: alle Empfänger müssen danach (erneut) eingelernt werden und der Device Counter muss zurückgesetzt werden!",
    en: "Each channel requires an individual serial number. If you enter a new serial number prefix here, the serial numbers for all channels will change. ATTENTION: all receivers must then be learned (again) and the device counter must be reset!",
  },
  info_devcnt: {
    de: "Der Zähler wird zusammen mit jedem Datagramm über den Funk gesendet. Er beginnt bei null und wird bei jedem Datagramm inkrementiert. Jeder empfänger hört die empfangenen Datagramme ab und zeichnet den Zähler des Senders auf. Wenn der Zähler des Senders und des Empfängers zu sehr voneinander abweichen, müssen Sie den Empfänger neu anlernen. Um dies zu vermeiden, wenn Sie einen Dongle austauschen (oder wenn ein Update schief geht), können Sie hier einen Zähler ungleich null eingeben. ACHTUNG: lassen Sie die Finger davon, wenn Sie die Wirkung nicht verstehen, Sie riskieren, dass Sie alle Ihre Empfänger (wieder) neu anlernen müssen!",
    en: "the device counter is send together with every datagram over the radio. beginning with zero, it is incremented on every datagram. each receiver listens to received datagrams and records the sender's device counter. when the sender's and receiver's device counter differ too much, you must re-learn the receiver. to avoid this when replacing a dongle (or when an update goes wrong) you can enter a non-zero device counter here. ATTENTION: don't touch this if you don't understand the effect, you risk to re-learn all your receivers (again)!",
  },
  cc1101error: {
    de: "Fehler: CC1101 Modul ist nicht verbunden!",
    en: "Error: CC1101 Modul not connected!",
  },
  help: {
    de: "Hilfe",
    en: "Help",
  },
  gpio_restart_info: {
    de: "Änderungen an den GPIO Einstellungen benötigen einen Neustart!",
    en: "Changes to GPIO or Jarolift settings require a restart!",
  },
  gpio_info: {
    de: "Beispiel für einen typischen ESP32",
    en: "example for typical ESP32",
  },
  mqtt_info2: {
    de: "< ../ > ist der Platzhalter für das MQTT Topic welches in den Einstellungen vorgegeben wird.",
    en: "< ../ > is the placeholder for the MQTT topic which is specified in the settings.",
  },
  groups: {
    de: "Gruppen",
    en: "Groups",
  },
  group: {
    de: "Gruppe",
    en: "Group",
  },
  channels: {
    de: "Kanäle",
    en: "Channels",
  },
  name: {
    de: "Name",
    en: "Name",
  },
  mask: {
    de: "Bitmaske",
    en: "Bitmask",
  },
  mask_help: {
    de: "In diesem Feld wird über eine Bitmaske festgelegt, welche Kanäle zu dieser Gruppe gehören.\nDie Bitmaske ist eine 16-Bit-Zahl, wobei das niederwertigste Bit (rechts) den Kanal 1 repräsentiert. \nEin gesetztes Bit bedeutet, dass der Kanal zu dieser Gruppe gehört.\n\nBeispiel: 0000000000010101 bedeutet, dass die Kanäle 1, 3 und 5 zu dieser Gruppe gehören.",
    en: "In this field, a bitmask is used to determine which channels belong to this group.\nThe bitmask is a 16-bit number, with the least significant bit (right) representing channel 1.\nA set bit means that the channel belongs to this group.\n\nExample: 0000000000010101 means that channels 1, 3 and 5 belong to this group.",
  },
  grp_mask_help: {
    de: "Sie können auch ein generisches Gruppenkommando verwenden und die Bitmaske verwenden, um die Rolläden direkt auszuwählen. Die Bitmaske ist eine 16-Bit-Zahl, wobei das niederwertigste Bit (rechts) den Kanal 1 repräsentiert. Ein gesetztes Bit bedeutet, dass der Kanal zu dieser Gruppe gehört.\n\nBeispiel: 0000000000010101 bedeutet, dass die Kanäle 1, 3 und 5 zu dieser Gruppe gehören.\n\nAls Payload können Sie drei verschiedene Formate verwenden, um die gleiche Bitmaske darzustellen:",
    en: "You can also use a generic group command and provide the bitmask to select the shutters directly. The bitmask is a 16-bit number, with the least significant bit (on the right) representing channel 1. A set bit means that the channel belongs to this group.\n\nExample: `0000000000010101` means that channels 1, 3, and 5 belong to this group.\n\nAs payload, you can use three different formats to represent the same bitmask:",
  },
  bitmask_wizard: {
    de: "Auswahl Assistent",
    en: "Selection Wizard",
  },
  general: {
    de: "Allgemein",
    en: "General",
  },
  sunrise: {
    de: "Sonnenaufgang",
    en: "sunrise",
  },
  sundown: {
    de: "Sonnenuntergang",
    en: "sundown",
  },
  cmd_up: {
    de: "Hochfahren",
    en: "up",
  },
  cmd_down: {
    de: "Runterfahren",
    en: "down",
  },
  cmd_shade: {
    de: "Schatten",
    en: "shade",
  },
  timer_type: {
    de: "Zeitgeber",
    en: "Time-Source",
  },
  time: {
    de: "Uhrzeit",
    en: "Time",
  },
  time_HH_MM: {
    de: "Zeit (HH:MM)",
    en: "Time (HH:MM)",
  },
  offset_desc: {
    de: "Offset in Minuten (z. B. -15 oder +20)",
    en: "Offset in Minutens (e.g. -15 oder +20)",
  },
  command: {
    de: "Befehl",
    en: "Command",
  },
  weekdays: {
    de: "Wochentage",
    en: "weekdays",
  },
  day_mo: {
    de: "Mo",
    en: "Mo",
  },
  day_tu: {
    de: "Di",
    en: "Tu",
  },
  day_we: {
    de: "Mi",
    en: "We",
  },
  day_th: {
    de: "Do",
    en: "Th",
  },
  day_fr: {
    de: "Fr",
    en: "Fr",
  },
  day_sa: {
    de: "Sa",
    en: "Sa",
  },
  day_su: {
    de: "So",
    en: "Su",
  },
  geo_location: {
    de: "Geografische Lage",
    en: "Geographical location",
  },
  geo_info: {
    de: "Wird benötigt für die Timer Funktion mit Sonnenaufgang und Sonnenuntergang",
    en: "Required for the timer function with sunrise and sunset",
  },
  latitude: {
    de: "Breitengrad",
    en: "Latitude",
  },
  longitude: {
    de: "Längengrad",
    en: "Longitude",
  },
  time_info: {
    de: "Zeitinformationen",
    en: "Time information",
  },
  apply: {
    de: "Übernehmen",
    en: "apply",
  },
  cancel: {
    de: "Abbrechen",
    en: "cancel",
  },
  gpio: {
    de: "GPIO-Zuweisung",
    en: "GPIO-Settings",
  },
};

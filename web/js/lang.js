// --------------------------------------
// localization texts
// --------------------------------------
const translations = {
  system: {
    de: "System",
    en: "System",
  },
  timer: {
    de: "Timer",
    en: "Timer",
  },
  control: {
    de: "Bedienung",
    en: "Control",
  },
  settings: {
    de: "Einstellungen",
    en: "Settings",
  },
  ok: {
    de: "OK",
    en: "OK",
  },
  network_info: {
    de: "Netzwerk-Information",
    en: "Network-Informations",
  },
  wifi_ip: {
    de: "WiFi IP-Adresse",
    en: "WiFi IP-Address",
  },
  wifi_signal: {
    de: "WiFi Signal",
    en: "WiFi Signal",
  },
  wifi_rssi: {
    de: "WiFi Rssi",
    en: "WiFi Rssi",
  },
  eth_ip: {
    de: "ETH IP-Adresse",
    en: "ETH IP-Address",
  },
  eth_status: {
    de: "ETH Status",
    en: "ETH Status",
  },
  static_ip: {
    de: "feste IP-Adresse",
    en: "static ip address",
  },
  eth_link_speed: {
    de: "ETH Geschwindigkeit",
    en: "ETH Link Speed",
  },
  eth_full_duplex: {
    de: "ETH Vollduplex",
    en: "ETH Full Duplex",
  },
  sw_version: {
    de: "Software-Version",
    en: "Software-Version",
  },
  version_info: {
    de: "Versionsinformationen",
    en: "Version Informations",
  },
  esp_maxallocheap: {
    de: "ESP MaxAllocHeap",
    en: "ESP MaxAllocHeap",
  },
  esp_minfreeheap: {
    de: "ESP MinFreeHeap",
    en: "ESP MinFreeHeap",
  },
  esp_flash_usage: {
    de: "ESP Flash usage",
    en: "ESP Flash usage",
  },
  esp_heap_usage: {
    de: "ESP Heap usage",
    en: "ESP Heap usage",
  },
  sysinfo: {
    de: "Systeminformationen",
    en: "System Informations",
  },
  date: {
    de: "Datum",
    en: "Date",
  },
  wifi: {
    de: "WiFi",
    en: "WiFi",
  },
  ssid: {
    de: "SSID",
    en: "SSID",
  },
  password: {
    de: "Passwort",
    en: "Password",
  },
  user: {
    de: "Benutzer",
    en: "User",
  },
  hostname: {
    de: "Hostname",
    en: "Hostname",
  },
  server: {
    de: "Server",
    en: "Server",
  },
  topic: {
    de: "Topic",
    en: "Topic",
  },
  mqtt: {
    de: "MQTT",
    en: "MQTT",
  },
  port: {
    de: "Port",
    en: "Port",
  },
  ntp: {
    de: "NTP-Server",
    en: "NTP-Server",
  },
  ntp_tz: {
    de: "Time-Zone",
    en: "Time-Zone",
  },
  gpio: {
    de: "GPIO-Zuweisung",
    en: "GPIO-Settings",
  },
  led_setup: {
    de: "LED-Setup-Mode",
    en: "LED-Setup-Mode",
  },
  restart: {
    de: "Neustart",
    en: "restart",
  },
  ota: {
    de: "OTA Firmware Update",
    en: "OTA Firmware Update",
  },
  config_mgn: {
    de: "Konfiguration import/export",
    en: "configuration import/export",
  },
  tools: {
    de: "Tools",
    en: "Tools",
  },
  activate: {
    de: "Aktivieren",
    en: "activate",
  },
  ip_adr: {
    de: "IP-Adresse",
    en: "IP-Address",
  },
  ip_subnet: {
    de: "Subnetz",
    en: "Subnet",
  },
  ip_gateway: {
    de: "Gateway",
    en: "Gateway",
  },
  ip_dns: {
    de: "DNS-Server",
    en: "DNS-Server",
  },
  access: {
    de: "Zugangskontrolle",
    en: "Authentication",
  },
  logger: {
    de: "Logbuch",
    en: "Logger",
  },
  clear: {
    de: "L\u00f6schen",
    en: "clear",
  },
  refresh: {
    de: "Aktualisieren",
    en: "refresh",
  },
  lifetime: {
    de: "Laufzeit",
    en: "Runtime",
  },
  restart_reason: {
    de: "Neustart Grund",
    en: "restart reason",
  },
  language: {
    de: "Sprache",
    en: "language",
  },
  german: {
    de: "Deutsch",
    en: "german",
  },
  english: {
    de: "Englisch",
    en: "english",
  },
  lost_connection: {
    de: "keine Verbindung!",
    en: "no connection",
  },
  sort_up: {
    de: "aufsteigend \u2B06",
    en: "ascending \u2B06",
  },
  sort_down: {
    de: "absteigend \u2B07",
    en: "descending \u2B07",
  },
  log_mode_4: {
    de: "Modus: Debug",
    en: "Mode: Debug",
  },
  log_mode_3: {
    de: "Modus: Info",
    en: "Mode: Info",
  },
  log_mode_2: {
    de: "Modus: Warnung",
    en: "Mode: Warning",
  },
  log_mode_1: {
    de: "Modus: Fehler",
    en: "Modus: Error",
  },
  import: {
    de: "import (config.json)",
    en: "import (config.json)",
  },
  export: {
    de: "export (config.json)",
    en: "export (config.json)",
  },
  open: {
    de: "öffne (config.json)",
    en: "open (config.json)",
  },
  update_done: {
    de: "Update erfolgreich!",
    en: "Update sucessfull!",
  },
  update_failed: {
    de: "Update nicht erfolgreich!",
    en: "Update not sucessfull!",
  },
  update_ready: {
    de: "Update bereit!",
    en: "Update ready!",
  },
  update_info: {
    de: "ESP muss neu gestartet werden um das Update zu übernehmen.",
    en: "ESP must be restarted to apply the update",
  },
  error_text: {
    de: "Fehler:",
    en: "Error:",
  },
  update_error_info: {
    de: "Bitte das Update erneut ausführen!",
    en: "Please run the update again!",
  },
  import_ready: {
    de: "Import bereit!",
    en: "import ready!",
  },
  close: {
    de: "schließen",
    en: "close",
  },
  act_version: {
    de: "aktuelle Version",
    en: "actual version",
  },
  github_version: {
    de: "neustes Release",
    en: "newest release",
  },
  mqtt_info: {
    de: "MQTT-Informationen",
    en: "MQTT information",
  },
  last_error: {
    de: "letzter Fehler",
    en: "last error",
  },
  connection: {
    de: "Verbindung",
    en: "connection",
  },
  state: {
    de: "Zustand",
    en: "State",
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
  sunrise: {
    de: "Sonnenaufgang",
    en: "sunrise",
  },
  sundown: {
    de: "Sonnenuntergang",
    en: "sundown",
  },
  time_offset: {
    de: "Zeitverschiebung",
    en: "time offset",
  },
  shutter: {
    de: "Rolläden",
    en: "Shutters",
  },
  cmd_up: {
    de: "Hochfahren",
    en: "up",
  },
  cmd_down: {
    de: "Runterfahren",
    en: "down",
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
  delete: {
    de: "löschen",
    en: "delete",
  },
};

// GitHub OTA reuses the existing commands and WebSocket targets.
function compareReleaseVersions(release, installed) {
  const parse = value => /^v?(\d+)\.(\d+)\.(\d+)$/.exec(value.trim());
  const a = parse(release), b = parse(installed);
  if (!a || !b) return null;
  for (let i = 1; i <= 3; i++) {
    if (Number(a[i]) !== Number(b[i])) return Number(a[i]) > Number(b[i]) ? 1 : -1;
  }
  return 0;
}
document.addEventListener('DOMContentLoaded', () => {
  const state = document.getElementById('p11_git_state');
  const progress = document.getElementById('p11_git_progress');
  const release = document.getElementById('p00_dialog_git_version');
  const installed = document.getElementById('p00_dialog_version');
  const check = document.getElementById('p11_check_git_btn');
  const install = document.getElementById('p00_update_btn');
  const action = document.getElementById('p00_update_btn_hide');
  const bar = document.getElementById('github_progress_bar');
  const feedback = document.getElementById('p11_git_feedback');
  const t = key => user_translations[key][document.getElementById('cfg_lang').value === '1' ? 'en' : 'de'];
  const messages = {
    idle: user_translations.info_git_idle,
    checking: user_translations.info_git_checking,
    ready: user_translations.info_git_ready,
    current: user_translations.info_git_current,
    error: user_translations.info_git_error,
    noasset: user_translations.info_git_noasset,
    updating: user_translations.info_git_updating,
    done: user_translations.info_git_done,
    failed: user_translations.info_git_failed,
    offline: user_translations.info_git_offline,
  };
  const render = () => {
    const mode = state.textContent.trim();
    const order = compareReleaseVersions(release.textContent, installed.textContent);
    const busy = mode === 'checking' || mode === 'updating';
    check.disabled = busy || mode === 'done';
    check.setAttribute('aria-busy', mode === 'checking' ? 'true' : 'false');
    action.style.display = mode === 'ready' ? '' : 'none';
    install.disabled = mode !== 'ready';
    install.textContent = `${t(order === -1 ? 'info_git_downgrade' : 'info_git_install')} ${release.textContent}`;
    let message = messages[mode] || messages.idle;
    if (mode === 'ready') message = order === -1 ? user_translations.info_git_older : order === 1 ? user_translations.info_git_newer : messages.ready;
    feedback.textContent = message[document.getElementById('cfg_lang').value === '1' ? 'en' : 'de'] + (mode === 'updating' ? `: ${progress.textContent}%` : '');
    bar.hidden = mode !== 'updating' && mode !== 'done';
    bar.value = Number(progress.textContent) || 0;
  };
  const observer = new MutationObserver(render);
  [state, progress, release, installed].forEach(element => observer.observe(element, { childList: true, characterData: true, subtree: true }));
  check.addEventListener('click', () => {
    state.textContent = 'checking';
    release.textContent = '—';
    release.removeAttribute('href');
    progress.textContent = '0';
    render();
    sendData('check_git_version', '');
  });
  install.addEventListener('click', () => {
    if (state.textContent.trim() !== 'ready') return;
    const older = compareReleaseVersions(release.textContent, installed.textContent) === -1;
    if (!window.confirm(`${release.textContent}\n\n${t(older ? 'info_git_confirm_older' : 'info_git_confirm')}`)) return;
    state.textContent = 'updating';
    progress.textContent = '0';
    render();
    sendData('p00_update_btn', true);
  });
  document.getElementById('cfg_lang').addEventListener('change', render);
  window.addEventListener('webui:connected', () => { state.textContent = 'idle'; release.textContent = '—'; release.removeAttribute('href'); render(); });
  window.addEventListener('webui:disconnected', () => { state.textContent = 'offline'; render(); });
  document.getElementById('p00_version').addEventListener('click', () => {
    document.querySelector('.nav-list a[data-tab="tab11"]').click();
    document.getElementById('github-update-info').scrollIntoView({ block: 'center', behavior: 'smooth' });
    check.focus({ preventScroll: true });
  });
  render();
});

// Local help actions reuse navigation; they never send actuator commands.
document.addEventListener("DOMContentLoaded", () => {
  const help = document.getElementById("tab13");
  help.addEventListener("click", event => {
    const shortcut = event.target.closest("[data-help-open]");
    if (shortcut) {
      event.preventDefault();
      const panel = document.getElementById(shortcut.dataset.helpOpen);
      panel.open = true;
      panel.querySelector("summary").focus({ preventScroll: true });
      panel.scrollIntoView({ block: "start", behavior: "smooth" });
    }
    const button = event.target.closest("[data-help-page]");
    if (button) document.querySelector(`.nav-list a[data-tab="${button.dataset.helpPage}"]`).click();
  });
  const readme = document.getElementById("help-readme");
  const updateLink = () => {
    // Observe the translated label so server-driven and local language changes work alike.
    const language = readme.textContent.trim() === user_translations.help_readme.en ? "en" : "de";
    readme.href = "https://github.com/dewenni/ESP32-Jarolift-Controller/blob/main/" + (language === "en" ? "README.md" : "README_DE.md");
  };
  new MutationObserver(updateLink).observe(readme, { childList: true, characterData: true, subtree: true });
  updateLink();
});

// Presentation only: reuse the existing received values; no polling or commands.
function initInformationPages() {
  const updates = new Map();
  const observer = new MutationObserver(mutations => {
    const changed = new Set();
    mutations.forEach(mutation => {
      let target = mutation.target.nodeType === 3 ? mutation.target.parentElement : mutation.target;
      while (target && !updates.has(target)) target = target.parentElement;
      if (target) changed.add(updates.get(target));
    });
    changed.forEach(update => update());
  });
  const watch = (source, update) => {
    const previous = updates.get(source);
    updates.set(source, previous ? () => { previous(); update(); } : update);
    observer.observe(source, { childList: true, characterData: true, subtree: true });
    update();
  };
  document.querySelectorAll('[data-info-state]').forEach(cell => {
    const enabled = cell.dataset.infoEnabled && document.getElementById(cell.dataset.infoEnabled);
    const disabledSource = cell.dataset.infoDisabledSource && document.getElementById(cell.dataset.infoDisabledSource);
    const update = () => {
      const value = cell.textContent.trim().toLowerCase();
      const type = cell.dataset.infoState;
      const connected = /^(verbunden|connected)$/.test(value);
      const disabled = (enabled && !enabled.checked) || (disabledSource && /^(inaktiv|inactive)$/.test(disabledSource.textContent.trim().toLowerCase()));
      cell.classList.toggle('is-active', type === 'number' ? value !== '' && Number(value) > 0 : type === 'on' && /^(ein|on)$/.test(value));
      cell.classList.toggle('is-error', (type === 'error' && /^(fehler|error)$/.test(value)) || (type === 'alarm' && value.includes('>>') && !value.includes('<<')));
      cell.classList.toggle('is-connected', type === 'connection' && connected);
      cell.classList.toggle('is-disconnected', type === 'connection' && !disabled && /^(nicht verbunden|not connected)$/.test(value));
    };
    watch(cell, update);
    if (enabled) enabled.addEventListener('change', update);
    if (disabledSource) watch(disabledSource, update);
  });
  document.querySelectorAll('[data-info-meter]').forEach(meter => {
    const source = document.getElementById(meter.dataset.infoMeter);
    watch(source, () => {
      const raw = source.textContent.trim();
      const value = Number(raw.replace(',', '.'));
      meter.hidden = raw === '' || !Number.isFinite(value) || value < 0 || value > 100;
      if (!meter.hidden) meter.value = value;
    });
  });
  document.querySelectorAll('[data-selected-file]').forEach(label => {
    const input = document.getElementById(label.dataset.selectedFile);
    input.addEventListener('change', () => {
      if (input.files.length) {
        label.removeAttribute('data-i18n');
        label.textContent = input.files[0].name;
      } else {
        label.dataset.i18n = 'info_no_file';
        const lang = document.getElementById('cfg_lang').value === '1' ? 'en' : 'de';
        label.textContent = user_translations.info_no_file[lang];
      }
      // The existing file-selection handlers enable submission; keep empty selections disabled.
      document.getElementById(input.id === 'ota_file_input' ? 'ota_update_btn' : 'file_upload_btn').disabled = !input.files.length;
      if (!input.files.length) {
        document.getElementById(input.id === 'ota_file_input' ? 'ota_status_txt' : 'upload_status_txt').style.display = 'none';
        if (input.id === 'ota_file_input') document.getElementById('ota_progress_bar').style.display = 'none';
      }
    });
  });
}

// --------------------------------------
// localization texts
// --------------------------------------
const user_translations = {
  help_wiring_title: {"de": "Anschlussreferenz", "en": "Wiring reference"},
  help_mqtt_topics: {"de": "Befehle und Status", "en": "Commands and status"},
  help_mqtt_title: {"de": "MQTT und Home Assistant", "en": "MQTT and Home Assistant"},
  help_intro: {"de": "Hier findest du die wichtigsten Schritte für Bedienung und Einrichtung. Ausführliche Anleitungen zu Installation, Verdrahtung und Funkempfang stehen in der Projektdokumentation.", "en": "Find the main steps for operation and setup here. Installation, wiring and RF reception are covered in the project documentation."},
  help_operate: {"de": "Bedienen", "en": "Operate"},
  help_operate_hint: {"de": "Tasten, Prozentziele, Timer und Gruppen", "en": "Buttons, percentage targets, timers and groups"},
  help_setup: {"de": "Einrichten", "en": "Set up"},
  help_setup_hint: {"de": "Kalibrieren und zuordnen", "en": "Calibrate and map remotes"},
  help_solve: {"de": "Probleme lösen", "en": "Troubleshoot"},
  help_solve_hint: {"de": "Status und Verbindungen", "en": "Status and connections"},
  help_buttons: {"de": "AUF und AB fahren den Rollladen zur jeweiligen Endlage. STOP hält eine laufende Fahrt an. SCHATTEN fährt die im Motor gespeicherte Schattenposition an; diese ist unabhängig von einem Prozentziel im ESP.", "en": "UP and DOWN move the shutter to its end positions. STOP stops movement. SHADE recalls the shade position stored in the motor; it is independent of an ESP percentage target."},
  help_target: {"de": "Bei eingeschalteter Positionssteuerung und kalibriertem Rollladen erscheinen Grafik, Zielslider und Ausführen-Button. 0 % bedeutet geschlossen, 100 % geöffnet. Der Slider wählt nur das Ziel; erst der Ausführen-Button startet die Fahrt.", "en": "When positioning is enabled and the shutter is calibrated, a graphic, target slider and execute button appear. 0% means closed, 100% open. The slider only selects the target; the execute button starts movement."},
  help_estimate: {"de": "Die Position ist ein Schätzwert ohne Rückmeldung des Motors. Während einer Zielanfahrt kann sie als unbekannt angezeigt werden; nach Abschluss wird der geschätzte Zielwert übernommen.", "en": "Position is an estimate without motor feedback. It may be shown as unknown during a target run; the estimated target is reported after completion."},
  help_channels: {"de": "Zu den Kanälen", "en": "Go to channels"},
  help_operation: {"de": "Rollläden bedienen", "en": "Operate shutters"},
  help_position_enable: {"de": "Unter Einstellungen → Gerät → Positionssteuerung die Funktion global einschalten. Nur einzeln kalibrierte Rollläden erhalten Prozentcontrols. Andere Rollläden bleiben über die normalen Tasten bedienbar.", "en": "Enable Settings → Device → Position control globally. Percentage controls appear only for individually calibrated shutters. Other shutters retain their normal buttons."},
  help_reference_title: {"de": "Warum zuerst eine Referenzfahrt?", "en": "Why a reference run first?"},
  help_reference: {"de": "Der Motor meldet seine Position nicht zurück. Originalfernbedienungen können unbemerkt bleiben, und ein elektrischer Taster vor Ort sendet gegebenenfalls gar kein Funksignal. Deshalb ersetzt auch eine registrierte Fernbedienung keine Referenzfahrt.", "en": "The motor does not report its position. Original remote commands can be missed, and a local electrical switch may send no radio signal at all. Registering remotes therefore cannot replace a reference run."},
  help_reference_route: {"de": "Ziele unter 50 %: zuerst vollständig schließen, dann bis zum Ziel öffnen. Ab 50 %: zuerst vollständig öffnen, dann bis zum Ziel schließen. 0/100 % fahren nur zur jeweiligen Endlage. Die volle Referenzzeit mit Reserve wird auch dann abgewartet, wenn der Rollladen bereits nahe der Endlage steht.", "en": "Targets below 50%: fully close, then open to the target. From 50%: fully open, then close to the target. 0/100% only travel to the relevant end position. The full reference time plus reserve is used even when the shutter is already near that end position."},
  help_calibration_title: {"de": "Kalibrierung in fünf Schritten", "en": "Calibration in five steps"},
  help_cal1: {"de": "Motor-Endlagen vorher einrichten und die halbe Öffnungshöhe markieren. Den Rollladen direkt beobachten.", "en": "Set the motor end positions first and mark half the opening height. Keep the shutter in direct view."},
  help_cal2: {"de": "Unter Service den Rollladen auswählen, vollständig schließen und die Kalibrierung starten.", "en": "Select the shutter on Service, close it fully and start calibration."},
  help_cal3: {"de": "Beim Öffnen bestätigen: Panzer hebt sich an → halb geöffnet → vollständig geöffnet.", "en": "While opening, confirm: curtain starts lifting → half open → fully open."},
  help_cal4: {"de": "Beim Schließen bestätigen: halb geöffnet → Panzer unten → Lamellen vollständig geschlossen.", "en": "While closing, confirm: half open → bottom edge down → slats fully closed."},
  help_cal5: {"de": "Zeiten prüfen und übernehmen. Anschließend 25 %, 50 % und 75 % testen.", "en": "Review and apply the timings. Then test 25%, 50% and 75%."},
  help_calibration_note: {"de": "Die Seite während der Kalibrierung im Vordergrund lassen. Ohne Browser-Lebenszeichen bricht der ESP den Assistenten nach sechs Sekunden ab und sendet während einer Fahrt STOP. Das ist keine garantierte Motorabschaltung. Eine reguläre Prozentfahrt läuft dagegen auch ohne geöffneten Browser zu Ende.", "en": "Keep the page in the foreground during calibration. After six seconds without browser heartbeats, the ESP aborts the wizard and sends STOP if moving. This does not guarantee that the motor stops. A normal percentage run, in contrast, completes without an open browser."},
  help_service: {"de": "Zur Service-Seite", "en": "Go to Service"},
  help_settings: {"de": "Zu den Einstellungen", "en": "Go to Settings"},
  help_position: {"de": "Positionssteuerung und Kalibrierung", "en": "Positioning and calibration"},
  help_remote_intro: {"de": "Die Registrierung informiert nur den ESP darüber, welche Originalfernbedienung einen Rollladen bedient. Sie lernt die Fernbedienung nicht am Motor an. Das echte Anlernen des ESP am Motor erfolgt unter Einstellungen → Gerät → Rollläden.", "en": "Registration only tells the ESP which original remote operates a shutter. It does not pair the remote with the motor. Pairing the ESP with a motor is done under Settings → Device → Shutters."},
  help_remote1: {"de": "Unter Service den Rollladen auswählen und Originalfernbedienungen → Zuordnung hinzufügen öffnen.", "en": "On Service, select the shutter and open Original remotes → Add mapping."},
  help_remote2: {"de": "Auf der Fernbedienung den bereits am Motor angelernten Einzelkanal wählen und möglichst STOP drücken. Keine Gruppe wählen.", "en": "Select the single channel already paired with the motor and preferably press STOP. Do not select a group."},
  help_remote3: {"de": "Loslassen, kurz warten und denselben Kanal erneut drücken. Danach benennen und speichern.", "en": "Release, wait briefly and press the same channel again. Then enter a name and save."},
  help_remote_limits: {"de": "Bis zu fünf Quellen pro Rollladen, Namen mit maximal 32 Zeichen. Dieselbe Quelle darf mehreren Rollläden zugeordnet sein. Gruppen werden aus den registrierten Einzelkanälen automatisch aufgelöst.", "en": "Up to five sources per shutter, with names of up to 32 characters. The same source may be mapped to several shutters. Groups are resolved automatically from registered single channels."},
  help_remote_delete: {"de": "Löschen entfernt nur die Beziehung zum ausgewählten Rollladen. Andere Zuordnungen und die echte Motor-Anlernung bleiben erhalten. Erneutes Erfassen einer bekannten Quelle erlaubt das Ändern ihres gemeinsamen Namens.", "en": "Delete removes only the relationship to the selected shutter. Other mappings and actual motor pairing remain intact. Capture a known source again to change its shared name."},
  help_remotes: {"de": "Originalfernbedienungen zuordnen", "en": "Register original remotes"},
  help_groups: {"de": "Gruppen steuern mehrere ausgewählte Rollläden gemeinsam mit AUF, STOP, AB oder SCHATTEN. Prozentziele für Gruppen sind derzeit nicht vorgesehen.", "en": "Groups operate selected shutters together with UP, STOP, DOWN or SHADE. Group percentage targets are not currently supported."},
  help_timer: {"de": "Im Timer Wochentage, Befehl und Rollläden auswählen. Als Zeitgeber stehen Uhrzeit, Sonnenaufgang und Sonnenuntergang mit optionalem Versatz bereit. Die Rollladenauswahl erzeugt die Bitmaske automatisch.", "en": "Select weekdays, command and shutters in the timer. Triggers include a fixed time, sunrise and sunset with an optional offset. The shutter selection creates the bitmask automatically."},
  help_timer_location: {"de": "Für Sonnenzeiten unter Einstellungen → Allgemein die geografische Lage und unter NTP die Zeitzone prüfen. Die Ausführung benötigt eine gültige Gerätezeit.", "en": "For sun times, check the geographic location under Settings → General and the time zone under NTP. Execution requires a valid device time."},
  help_timers: {"de": "Zu den Timern", "en": "Go to timers"},
  help_timer_groups: {"de": "Timer und Gruppen", "en": "Timers and groups"},
  help_mqtt_ha: {"de": "Unter Einstellungen → Integrationen MQTT mit Broker, Zugangsdaten und Topic einrichten. Home Assistant wird im eigenen Abschnitt aktiviert und benötigt MQTT. Für aktivierte, kalibrierte Positionskanäle ergänzt Discovery automatisch die Prozentsteuerung im vorhandenen Cover.", "en": "Configure MQTT with broker, credentials and topic under Settings → Integrations. Enable Home Assistant in its own section; it requires MQTT. For enabled, calibrated positioning channels, discovery automatically adds percentage control to the existing cover."},
  help_ha_status: {"de": "Home Assistant zeigt dieselbe geschätzte Position wie die WebUI. Bei unbekannter Position kann dort der letzte bekannte Prozentwert stehen bleiben; das bestätigt keine tatsächliche Motorposition.", "en": "Home Assistant shows the same estimated position as the WebUI. When position is unknown, it may retain the last known percentage; this does not confirm the actual motor position."},
  help_access: {"de": "WebUI-Anmeldung ist optional und hat eigene Zugangsdaten. Telnet und Remote-OTA über ArduinoOTA verwenden ein separates Service-Passwort; ohne dieses bleiben beide im Normalbetrieb aus. Telnet muss zusätzlich eingeschaltet werden.", "en": "WebUI login is optional and has its own credentials. Telnet and remote OTA through ArduinoOTA share a separate service password; without it, both stay disabled in normal operation. Telnet must also be explicitly enabled."},
  help_setup_access: {"de": "Im Setup-Mode ist die WebUI-Anmeldung immer admin / jarolift-2026!, unabhängig von der gespeicherten Konfiguration. Der Accesspoint heißt ESP32-Jarolift; die WebUI ist unter 192.168.4.1 erreichbar.", "en": "In Setup Mode, WebUI login is always admin / jarolift-2026!, independently of the saved configuration. The access point is ESP32-Jarolift; the WebUI is available at 192.168.4.1."},
  help_updates: {"de": "Unter Tools sind Lokale Datei und Von GitHub getrennte Updatewege. Eine passende OTA-Datei auswählen und installieren oder zuerst die GitHub-Version prüfen und anschließend ausdrücklich installieren. WebUI-Updates verwenden nur die WebUI-Anmeldung, kein zusätzliches Service-Passwort.", "en": "Tools separates Local file and From GitHub updates. Select and install a matching OTA file, or check the GitHub version and then explicitly install it. WebUI updates use only WebUI authentication, without an extra service password."},
  help_tools: {"de": "Zu den Tools", "en": "Go to Tools"},
  help_connections: {"de": "Zugänge und Updates", "en": "Access and updates"},
  help_unknown: {"de": "Position unbekannt", "en": "Position unknown"},
  help_unknown_answer: {"de": "Nach Neustart, STOP oder SCHATTEN fehlt eine verlässliche Ausgangslage. Bei gültiger Kalibrierung kann trotzdem ein Prozentziel angefordert werden: Die Referenzfahrt stellt den Ausgangspunkt neu her.", "en": "After restart, STOP or SHADE, the starting position is uncertain. With valid calibration you can still request a percentage target: the reference run establishes a new starting point."},
  help_missing: {"de": "Keine Prozentsteuerung sichtbar", "en": "No percentage controls"},
  help_missing_answer: {"de": "Globalen Schalter unter Gerät sowie Aktivierung und Kalibrierung dieses Rollladens prüfen. Nach Endlagen- oder Motor-Anlernbefehlen ist erneut zu kalibrieren.", "en": "Check the global switch under Device and this shutter’s activation and calibration. Recalibrate after end-position or motor-pairing commands."},
  help_busy: {"de": "Ziel wird nicht angenommen", "en": "Target is rejected"},
  help_busy_answer: {"de": "Es fährt immer nur ein Rollladen automatisch. Bis zu 16 weitere Ziele warten im Puffer; ein neuer Sollwert ersetzt den wartenden Wert desselben Rollladens. Während einer Kalibrierung, im Setup-Mode, bei OTA oder anstehenden manuellen Funkbefehlen werden keine Ziele angenommen.", "en": "One shutter moves automatically at a time. Up to 16 further targets wait in a queue; a new target replaces the pending value for the same shutter. Calibration, Setup Mode, OTA or pending manual radio commands prevent accepting targets."},
  help_rf: {"de": "Fernbedienung fehlt oder falscher Status", "en": "Remote missing or wrong status"},
  help_rf_answer: {"de": "Einzelkanal in der Nähe des ESP testen, Masterkeys und Zuordnung prüfen. Im Logbuch DEBUG für die Pulsauswertung wählen. Mehrere Meldungen pro Tastendruck können normale Wiederholungen sein. Nicht empfangene Funkbefehle und lokale Taster bleiben unbemerkt.", "en": "Test a single channel near the ESP and check master keys and mappings. Select DEBUG in the Logger for pulse diagnostics. Multiple messages per press may be normal repetitions. Missed radio commands and local switches remain unobserved."},
  help_network: {"de": "MQTT oder Home Assistant nicht verbunden", "en": "MQTT or Home Assistant not connected"},
  help_network_answer: {"de": "Auf System den MQTT-Verbindungsstatus prüfen. Der Punkt im Header zeigt nur die Browser-Verbindung zum ESP, nicht die Verbindung zum MQTT-Broker oder zum Motor.", "en": "Check MQTT connection status on System. The header dot indicates only the browser connection to the ESP, not the connection to the MQTT broker or motor."},
  help_system: {"de": "Zum Systemstatus", "en": "Go to System"},
  help_logger: {"de": "Zum Logbuch", "en": "Go to Logger"},
  help_problems: {"de": "Probleme eingrenzen", "en": "Troubleshoot"},
  help_mqtt_reference: {"de": "N steht für Rollladen 1–16, G für Gruppe 1–6. Befehle ohne Retain senden. Statusmeldungen sind retained und enthalten eine Schätzung, keine Motorrückmeldung.", "en": "N is shutter 1–16, G is group 1–6. Send commands without retain. Status messages are retained and contain an estimate, not motor feedback."},
  help_topic: {"de": "Topic", "en": "Topic"},
  help_payload: {"de": "Payload / Bedeutung", "en": "Payload / meaning"},
  help_shutter_payload: {"de": "UP / OPEN / 0 · DOWN / CLOSE / 1 · STOP / 2 · SHADE / 3", "en": "UP / OPEN / 0 · DOWN / CLOSE / 1 · STOP / 2 · SHADE / 3"},
  help_group_payload: {"de": "Dieselben Befehle für konfigurierte Gruppen.", "en": "The same commands for configured groups."},
  help_position_payload: {"de": "Ganzzahl 0–100, nur aktiviert und kalibriert.", "en": "Integer 0–100, only when enabled and calibrated."},
  help_status_payload: {"de": "0 geschlossen · 100 geöffnet · Zwischenwerte · None unbekannt.", "en": "0 closed · 100 open · intermediate values · None unknown."},
  help_restart_payload: {"de": "ESP neu starten, keine Payload erforderlich.", "en": "Restart ESP, no payload required."},
  help_mqtt_numbers: {"de": "Achtung: 0 auf cmd/shutter/N bedeutet AUF, 0 auf cmd/position/N bedeutet geschlossen. Die Statusskala ist gegenüber älteren Versionen umgekehrt: 0 geschlossen, 100 geöffnet.", "en": "Note: 0 on cmd/shutter/N means UP, while 0 on cmd/position/N means closed. The status scale is reversed compared with older versions: 0 closed, 100 open."},
  help_group_mask: {"de": "Für eine freie Gruppe: ../cmd/group/up, /down, /stop oder /shade. Beispiel für Rollläden 1, 3 und 5: Payload 0b0000000000010101, 0x15 oder 21 (ein Format auswählen).", "en": "For an ad hoc group: ../cmd/group/up, /down, /stop or /shade. Example for shutters 1, 3 and 5: payload 0b0000000000010101, 0x15 or 21 (choose one format)."},
  help_gpio_note: {"de": "Dies sind Beispielanschlüsse, nicht die ausgelesene Gerätekonfiguration. Die tatsächliche Verdrahtung muss zu Einstellungen → Gerät → GPIO passen. Nach GPIO- oder JAROLIFT-Protokolländerungen neu starten.", "en": "These are example connections, not the device’s current configuration. Actual wiring must match Settings → Device → GPIO. Restart after GPIO or JAROLIFT protocol changes."},
  help_docs: {"de": "Ausführliche Dokumentation", "en": "Detailed documentation"},
  help_readme: {"de": "Projektanleitung", "en": "Project documentation"},
  help_doc_position: {"de": "Positionssteuerung", "en": "Positioning"},
  help_doc_rf: {"de": "Funkempfang", "en": "RF reception"},
  help_doc_hint: {"de": "Links öffnen GitHub und benötigen Internet. Die Projektanleitung ist zweisprachig; die ergänzenden Detailanleitungen sind auf Deutsch. Die Hilfe auf dieser Seite bleibt auch ohne Internet verfügbar.", "en": "Links open GitHub and require internet. Project documentation is bilingual; the detailed guides are in German. Help on this page remains available without internet."},

  info_git_installed: {"de": "Installiert:", "en": "Installed:"},
  info_git_release: {"de": "Letztes Release:", "en": "Latest release:"},
  info_git_idle: {"de": "Prüfe, welche Version auf GitHub verfügbar ist.", "en": "Check which version is available on GitHub."},
  info_git_checking: {"de": "Version wird geprüft …", "en": "Checking version …"},
  info_git_ready: {"de": "Eine andere Version ist verfügbar.", "en": "A different version is available."},
  info_git_newer: {"de": "Eine neuere Version ist verfügbar.", "en": "A newer version is available."},
  info_git_older: {"de": "Das Release ist älter als die installierte Version.", "en": "This release is older than the installed version."},
  info_git_current: {"de": "Bereits aktuell.", "en": "Already up to date."},
  info_git_error: {"de": "Prüfung fehlgeschlagen. Bitte erneut versuchen.", "en": "Check failed. Please try again."},
  info_git_noasset: {"de": "Keine passende Firmware für dieses Gerät im Release gefunden.", "en": "No matching firmware for this device was found in the release."},
  info_git_install: {"de": "Version installieren", "en": "Install version"},
  info_git_downgrade: {"de": "Ältere Version installieren", "en": "Install older version"},
  info_git_confirm: {"de": "Diese Version installieren? Der ESP wird anschließend neu gestartet.", "en": "Install this version? The ESP will restart afterwards."},
  info_git_confirm_older: {"de": "Diese Version ist älter als die installierte Firmware. Trotzdem installieren?", "en": "This version is older than the installed firmware. Install anyway?"},
  info_git_updating: {"de": "Installation läuft", "en": "Installing"},
  info_git_done: {"de": "Installation erfolgreich. Neustart bestätigen.", "en": "Installation successful. Confirm restart."},
  info_git_failed: {"de": "Installation fehlgeschlagen. Bitte vor einem weiteren Versuch erneut prüfen.", "en": "Installation failed. Check again before retrying."},
  info_git_offline: {"de": "Verbindung unterbrochen. Bitte nach dem Wiederverbinden erneut prüfen.", "en": "Connection lost. Check again after reconnecting."},

  info_update_local: { de: "Lokale Datei", en: "Local file" },
  info_update_local_hint: { de: "Eine Firmware-Datei von deinem Computer auf den ESP übertragen.", en: "Upload a firmware file from your computer to the ESP." },
  info_update_github: { de: "Von GitHub", en: "From GitHub" },
  info_update_github_hint: { de: "Eine veröffentlichte Version direkt aus dem Projekt herunterladen.", en: "Download a published version directly from the project." },
  info_install_file: { de: "Datei installieren", en: "Install file" },
  rm_confirm: {"de": "Signal erfasst. Taste loslassen, kurz warten und denselben Kanal nochmals drücken.", "en": "Signal captured. Release the button, wait briefly and press the same channel again."},
  rm_confirmed: {"de": "Quelle bestätigt. Eine Bezeichnung eingeben und die Zuordnung speichern.", "en": "Source confirmed. Enter a name and save the mapping."},
  rm_motor_full: {"de": "Für diesen Rollladen sind bereits fünf Zuordnungen vorhanden. Bitte zuerst eine entfernen.", "en": "This shutter already has five mappings. Remove one first."},
  rm_group: {"de": "Gruppensignal erkannt – bitte einen Einzelkanal wählen.", "en": "Group signal detected — select a single channel."},
  rm_different: {"de": "Anderes Signal erkannt. Bitte den zuvor erfassten Einzelkanal nochmals drücken.", "en": "Different signal detected. Press the previously captured single channel again."},
  rm_shared: {"de": "Diese Quelle ist bereits bekannt. Die Bezeichnung gilt für alle ihre Rollladen-Zuordnungen.", "en": "This source is already known. Its name is shared by all associated shutters."},
  rm_legacy: {"de": "Neu erfassen (alte Gruppenzuordnung)", "en": "Capture again (old group mapping)"},
  rm_step_capture: {"de": "Erfassen", "en": "Capture"},
  rm_step_confirm: {"de": "Bestätigen", "en": "Confirm"},
  rm_step_save: {"de": "Speichern", "en": "Save"},
  rm_info: {"de": "Zuordnungen für den ausgewählten Rollladen: höchstens fünf Fernbedienungskanäle. Nur zur Statusverfolgung im ESP; Motoren werden hier nicht angelernt. Gruppen werden automatisch berücksichtigt.", "en": "Mappings for the selected shutter: up to five remote channels. Only for ESP status tracking; motors are not paired here. Groups are handled automatically."},
  rm_add: {"de": "Zuordnung hinzufügen", "en": "Add mapping"},
  rm_empty: {"de": "Für diesen Rollladen sind noch keine Fernbedienungen zugeordnet.", "en": "No remotes are assigned to this shutter yet."},
  rm_title: {"de": "Fernbedienung zuordnen", "en": "Map remote signal"},
  rm_hint: {"de": "Einen bereits am Motor angelernten Einzelkanal wählen und kurz STOP drücken. Keine Gruppe wählen.", "en": "Select a single channel already paired with the motor and briefly press STOP. Do not select a group."},
  rm_warning: {"de": "Der Tastendruck kann die angelernten Rollos bewegen. STOP kann ebenfalls zum Erfassen verwendet werden.", "en": "Pressing a button may move paired shutters. STOP can also be used for capture."},
  rm_waiting: {"de": "Warte auf ein Signal …", "en": "Waiting for a signal …"},
  rm_retry: {"de": "Erneut erfassen", "en": "Capture again"},
  rm_close: {"de": "Abbrechen", "en": "Cancel"},
  rm_save: {"de": "Speichern", "en": "Save"},
  rm_delete: {"de": "Löschen", "en": "Delete"},
  rm_delete_confirm: {"de": "Zuordnung nur für diesen Rollladen entfernen? Andere Zuordnungen und das Anlernen am Motor bleiben bestehen.", "en": "Remove the mapping only for this shutter? Other mappings and motor pairing remain unchanged."},
  rm_busy: {"de": "Ein anderer Dialog ist bereits aktiv.", "en": "Another dialog is already active."},
  rm_unavailable: {"de": "Während eines Updates nicht verfügbar.", "en": "Unavailable during an update."},
  rm_invalid: {"de": "Bitte eine gültige Bezeichnung mit höchstens 32 Zeichen eingeben.", "en": "Enter a valid name of up to 32 characters."},
  rm_no_signal: {"de": "Bitte den Einzelkanal zuerst erfassen und bestätigen.", "en": "Capture and confirm a single channel first."},
  rm_targets: {"de": "Mindestens einen aktivierten Rollladen auswählen.", "en": "Select at least one enabled shutter."},
  rm_full: {"de": "Die Liste ist voll (80 Quellen).", "en": "The list is full (80 sources)."},
  rm_timeout: {"de": "Die Erfassung ist abgelaufen. Bitte erneut erfassen.", "en": "Capture expired. Capture again."},
  rm_offline: {"de": "Keine Verbindung zum ESP.", "en": "No connection to the ESP."},
  rm_no_reply: {"de": "Keine Antwort vom ESP. Bitte die Verbindung prüfen.", "en": "No response from the ESP. Check the connection."},

  pos_calibrate_link: {de:"Zur Kalibrierung", en:"Open calibration"},
  pos_recalibrate: {de:"Neu kalibrieren", en:"Recalibrate"},
  pos_notes: {de:"Hinweise zur Positionierung", en:"Positioning notes"},
  pos_visual_hint: {de:"Darstellung zur Orientierung", en:"Illustration for guidance"},
  pos_title: {"de": "Positionssteuerung", "en": "Position control"},
  pos_enable: {"de": "Prozentuale Steuerung verwenden", "en": "Enable percentage control"},
  pos_settings_hint: {"de": "Je Rollladen auf der Service-Seite kalibrieren. Vor jeder Prozentfahrt wird zuerst vollständig geschlossen oder geöffnet.", "en": "Calibrate each shutter on the Service page. Each percentage move first fully closes or opens the shutter."},
  pos_calibration: {"de": "Positionskalibrierung", "en": "Position calibration"},
  pos_service_hint: {"de": "Zwei vollständige Messfahrten für den ausgewählten Rollladen. Die Motor-Endlagen müssen bereits eingerichtet sein.", "en": "Two complete measurement runs for the selected shutter. Motor end limits must already be set."},
  pos_remote_hint: {"de": "Originalfernbedienungen müssen unter Gerät zugeordnet sein, damit empfangene Befehle eine Zielanfahrt abbrechen können.", "en": "Assign original remotes under Device so received commands can cancel a target move."},
  pos_calibrate: {"de": "Kalibrierung starten", "en": "Start calibration"},
  pos_cancel: {"de": "Stop & abbrechen", "en": "Stop & cancel"},
  pos_disabled: {"de": "Positionssteuerung ist in den Einstellungen ausgeschaltet.", "en": "Position control is disabled in Settings."},
  pos_ready: {"de": "Kalibriert", "en": "Calibrated"},
  pos_missing: {"de": "Nicht kalibriert", "en": "Not calibrated"},
  pos_unknown: {"de": "Position unbekannt", "en": "Position unknown"},
  pos_estimate: {"de": "Geschätzte Öffnung", "en": "Estimated opening"},
  pos_goal: {"de": "Öffnung (%)", "en": "Opening (%)"},
  pos_go: {"de": "Ziel anfahren", "en": "Move to target"},
  pos_reference: {"de": "Referenzfahrt läuft", "en": "Referencing end position"},
  pos_settle: {"de": "Umschaltpause", "en": "Direction change pause"},
  pos_moving: {"de": "Zielanfahrt läuft", "en": "Moving to target"},
  pos_closed: {"de": "Geschlossen", "en": "Closed"},
  pos_open: {"de": "Geöffnet", "en": "Open"},
  pos_prepare: {"de": "Vorbereiten", "en": "Prepare"},
  pos_up: {"de": "Öffnen", "en": "Open"},
  pos_down: {"de": "Schließen", "en": "Close"},
  pos_review: {"de": "Prüfen", "en": "Review"},
  pos_initial: {"de": "Rollladen vollständig geschlossen?", "en": "Is the shutter fully closed?"},
  pos_initial_hint: {"de": "Der Panzer muss unten sein und alle Lamellen müssen aufeinanderliegen. Stelle diese Ausgangslage vorher mit den normalen Bedientasten her. Bestätige nur bei direkter Sicht auf den Rollladen.", "en": "The shutter must be at the bottom with all slats closed. Establish this position using the normal controls before starting. Confirm only with a direct view of the shutter."},
  pos_confirm: {"de": "Vollständig geschlossen · bestätigen", "en": "Fully closed · confirm"},
  pos_start_up: {"de": "Öffnungsfahrt starten", "en": "Start opening"},
  pos_start_down: {"de": "Schließfahrt starten", "en": "Start closing"},
  pos_up_hint: {"de": "Nach dem Start zuerst bestätigen, sobald sich die Unterkante des Panzers anhebt. Danach bei halber Öffnungshöhe und vollständig geöffnet.", "en": "After starting, first confirm when the bottom edge begins to lift. Then confirm at half opening height and fully open."},
  pos_down_hint: {"de": "Der Rollladen muss vollständig geöffnet sein. Bestätige die halbe Öffnungshöhe, dann Panzer unten und zuletzt alle Lamellen geschlossen.", "en": "The shutter must be fully open. Confirm half opening height, then bottom edge at the bottom, finally all slats closed."},
  pos_lift: {"de": "Panzer hebt sich an", "en": "Bottom edge begins to lift"},
  pos_half: {"de": "Halb geöffnet", "en": "Half open"},
  pos_full_open: {"de": "Vollständig geöffnet", "en": "Fully open"},
  pos_bottom: {"de": "Panzer ist unten", "en": "Bottom edge at bottom"},
  pos_full_closed: {"de": "Lamellen vollständig geschlossen", "en": "All slats closed"},
  pos_now: {"de": "Jetzt bestätigen", "en": "Confirm now"},
  pos_mark_hint: {"de": "Zum angezeigten Zeitpunkt bestätigen. Nach den ersten beiden Markierungen fährt der Motor weiter. Maximal 180 Sekunden je Richtung.", "en": "Confirm when the indicated position is reached. The motor continues after the first two marks. Maximum 180 seconds per direction."},
  pos_apply: {"de": "Kalibrierung übernehmen", "en": "Apply calibration"},
  pos_review_hint: {"de": "Zeitpunkte ab Beginn der jeweiligen Fahrt. Erst mit Übernehmen werden die bisherigen Kalibrierdaten ersetzt.", "en": "Times are measured from the start of each direction. Previous calibration is replaced only when you apply these values."},
  pos_waiting: {de:"Wartet auf Ausführung", en:"Waiting for execution"},
  pos_queued_goal: {de:"Ziel · wartet", en:"Target · queued"},
  pos_queue_place: {de:"Platz", en:"Place"},
  pos_queue_full: {de:"Die Positions-Warteschlange ist voll.", en:"The position queue is full."},
  pos_busy: {"de": "Eine Kalibrierung oder manuelle Funkbefehle sind noch aktiv.", "en": "Calibration or manual radio commands are still active."},
  pos_unavailable: {"de": "Kanal oder Positionssteuerung nicht verfügbar.", "en": "Channel or position control unavailable."},
  pos_not_calibrated: {"de": "Diesen Kanal zuerst kalibrieren.", "en": "Calibrate this channel first."},
  pos_send_failed: {"de": "Funkbefehl konnte nicht gesendet werden. Position unbekannt.", "en": "Radio command could not be sent. Position unknown."},
  pos_invalid_times: {"de": "Unplausible Messwerte. Bitte abbrechen und neu kalibrieren.", "en": "Implausible times. Cancel and calibrate again."},
  pos_saved: {"de": "Kalibrierung übernommen.", "en": "Calibration applied."},
  pos_cancelled: {"de": "Vorgang abgebrochen.", "en": "Operation cancelled."},
  pos_timeout: {"de": "Kalibrierung wegen Zeitüberschreitung oder fehlender Browserverbindung gestoppt.", "en": "Calibration stopped due to timeout or missing browser connection."},
  pos_total: {"de": "Kanäle eingerichtet", "en": "channels calibrated"},
  pos_no_link: {"de": "Keine Verbindung zum ESP. Kalibrierung wird automatisch abgebrochen.", "en": "No connection to ESP. Calibration will be cancelled automatically."},
  pos_time: {"de": "Zeit seit Fahrtbeginn", "en": "Time since movement started"},
  pos_reference_hint: {"de": "Ziel unter 50 %: zuerst schließen. Ab 50 %: zuerst öffnen. Die Position wird zeitbasiert geschätzt.", "en": "Below 50%: close first. At 50% and above: open first. Position is estimated from time."},
  ui_reload: { de: "Seite neu laden", en: "Reload page" },
  info_firmware_used: { de: "Firmware-Speicher belegt", en: "Firmware storage used" },
  info_ram_free: { de: "RAM frei", en: "Free RAM" },
  info_largest_free_block: { de: "Größter freier Speicherblock", en: "Largest free memory block" },
  info_minimum_free_ram: { de: "Niedrigster freier RAM seit Neustart", en: "Lowest free RAM since restart" },
  info_memory_used: { de: "belegt", en: "used" },
  info_log_recording: { de: "Aufzeichnung", en: "Recording" },
  info_firmware_update: { de: "Firmware-Update", en: "Firmware update" },
  info_configuration: { de: "Konfiguration", en: "Configuration" },
  info_check_github: { de: "GitHub-Version prüfen", en: "Check GitHub version" },
  info_import: { de: "Importieren", en: "Import" },
  info_export: { de: "Exportieren", en: "Export" },
  info_show: { de: "Anzeigen", en: "Show" },
  info_ethernet: { de: "Ethernet", en: "Ethernet" },
  info_memory: { de: "Speicher", en: "Memory" },
  info_tools_subtitle: { de: "Firmware und Konfigurationsverwaltung", en: "Firmware and configuration management" },
  info_log_subtitle: { de: "Ereignisse und Fehlersuche", en: "Events and troubleshooting" },
  info_mqtt_error_hint: { de: "Der letzte Fehler ist gespeichert und beschreibt nicht zwingend den aktuellen Verbindungszustand.", en: "The last error is stored and does not necessarily describe the current connection state." },
  info_firmware_file: { de: "Firmware-Datei", en: "Firmware file" },
  info_config_file: { de: "Konfigurationsdatei", en: "Configuration file" },
  info_choose_file: { de: "Datei auswählen", en: "Choose file" },
  info_no_file: { de: "Keine Datei ausgewählt", en: "No file selected" },
  info_log_level: { de: "Level", en: "Level" },
  info_log_order: { de: "Reihenfolge", en: "Order" },
  ha_mqtt_hint: { de: "Benötigt eine aktivierte und konfigurierte MQTT-Verbindung.", en: "Requires an enabled and configured MQTT connection." },
  ha_mqtt_disabled: { de: "MQTT ist derzeit deaktiviert.", en: "MQTT is currently disabled." },
  ha_mqtt_open: { de: "MQTT-Einstellungen öffnen", en: "Open MQTT settings" },
  ui_connected: { de: "Verbunden", en: "Connected" },
  ui_disconnected: { de: "Verbindung unterbrochen – verbinde erneut …", en: "Disconnected – reconnecting …" },
  system_subtitle: { de: "Verbindungen und Gerät", en: "Connections and device" },
  settings_subtitle: { de: "Konfiguration", en: "Configuration" },
  jaro_hardware: {"de": "Hardware", "en": "Hardware"},
  jaro_channels_subtitle: {"de": "Rolläden einzeln steuern", "en": "Control individual shutters"},
  jaro_groups_subtitle: {"de": "Mehrere Rolläden gemeinsam steuern", "en": "Control multiple shutters together"},
  jaro_timers_subtitle: {"de": "Zeitprogramme und Sonnenzeiten", "en": "Schedules and solar times"},
  jaro_service_subtitle: {"de": "Rolläden und Endlagen einrichten", "en": "Configure shutters and end positions"},
  jaro_help_subtitle: {"de": "Bedienung, Einrichtung und Problemlösung", "en": "Operation, setup and troubleshooting"},
  service_access: { de: "Service-Zugänge", en: "Service access" },
  service_password: { de: "Service-Passwort", en: "Service password" },
  service_telnet: { de: "Telnet aktivieren", en: "Enable Telnet" },
  service_hint: {
    de: "Zentrales Passwort für Service Zugänge wie z.B. Telnet und Remote-OTA (ArduinoOTA). Ohne Passwort bleiben beide gesperrt. Im Setup-Modus gilt admin / jarolift-2026!.",
    en: "Central password for service access such as Telnet and Remote-OTA (ArduinoOTA). Without a password, both remain locked. In setup mode, admin / jarolift-2026! applies.",
  },
  wifi_ssid_search: { de: "Suche WLAN", en: "Find Wi-Fi" },
  wifi_ssid_title: { de: "WLAN auswählen", en: "Select Wi-Fi network" },
  wifi_ssid_select: { de: "Verfügbare WLAN-Netze", en: "Available Wi-Fi networks" },
  wifi_ssid_hint: {
    de: "Versteckte WLAN-Namen können manuell eingegeben werden. Bei einem anderen WLAN-Namen wird die feste BSSID zurückgesetzt.",
    en: "Hidden Wi-Fi names can be entered manually. Selecting a different Wi-Fi name clears the fixed BSSID.",
  },
  wifi_ssid_choose: { de: "Bitte WLAN auswählen", en: "Select a Wi-Fi network" },
  wifi_ssid_found: { de: "WLAN auswählen und mit Übernehmen bestätigen.", en: "Select a network and confirm with Apply." },
  wifi_ssid_empty: { de: "Keine WLAN-Netze gefunden.", en: "No Wi-Fi networks found." },
  wifi_ap_bssid: {
    de: "Access Point (BSSID, optional)",
    en: "Access point (BSSID, optional)",
  },
  wifi_ap_search: { de: "Suche AP", en: "Find AP" },
  wifi_ap_hint: {
    de: "Leer = automatische Auswahl. Mit fester BSSID erfolgt kein Wechsel zu anderen Access Points. Gilt ab der nächsten WLAN-Verbindung oder nach einem Neustart.",
    en: "Empty = automatic selection. A fixed BSSID prevents switching to other access points. Applies on the next Wi-Fi connection or after a restart.",
  },
  wifi_ap_title: { de: "Access Point auswählen", en: "Select access point" },
  wifi_ap_select: { de: "Verfügbare Access Points", en: "Available access points" },
  wifi_ap_retry: { de: "Erneut suchen", en: "Scan again" },
  wifi_ap_apply: { de: "Übernehmen", en: "Apply" },
  wifi_ap_auto: { de: "Automatisch auswählen", en: "Select automatically" },
  wifi_ap_current: { de: "aktuell verbunden", en: "currently connected" },
  wifi_ap_channel: { de: "Kanal", en: "Channel" },
  wifi_ap_not_found: { de: "nicht gefunden", en: "not found" },
  wifi_ap_scanning: { de: "Suche läuft…", en: "Scanning…" },
  wifi_ap_found: { de: "Access Point auswählen und mit Übernehmen bestätigen.", en: "Select an access point and confirm with Apply." },
  wifi_ap_empty: { de: "Keine Access Points für diesen WLAN-Namen gefunden.", en: "No access points found for this Wi-Fi name." },
  wifi_ap_invalid_ssid: { de: "Bitte zuerst einen WLAN-Namen mit 1 bis 32 Byte eingeben.", en: "Enter a Wi-Fi name of 1 to 32 bytes first." },
  wifi_ap_failed: { de: "Die Suche ist fehlgeschlagen. Bitte erneut versuchen.", en: "The scan failed. Please try again." },
  wifi_ap_busy: { de: "Das Gerät führt bereits eine Suche oder ein Update aus. Bitte später erneut versuchen.", en: "The device is already scanning or updating. Please try again later." },
  wifi_ap_ssid_changed: { de: "Der WLAN-Name wurde geändert. Bitte den Dialog schließen und erneut suchen.", en: "The Wi-Fi name has changed. Close this dialog and scan again." },
  mqtt_connection_test: {
    de: "Verbindung testen",
    en: "Test connection",
  },
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
  homee: {
    de: "Homee-Integration",
    en: "Homee-Integration",
  },
  homee_virtual_name: {
    de: "virtueller homee Name",
    en: "virtual homee name",
  },
  homee_third_position: {
    de: "3. Position",
    en: "3rd position",
  },
  info_homee_third_position: {
    de: "Aktiv (3rd Position enabled): Node-Profil wird auf 'unbekannt' gesetzt, homee zeigt zusätzlich einen Taster für die 3. Position (Zwischenposition). Inaktiv (compatibility mode): Standard-Rolladenprofil wie bisher, ohne 3. Position.",
    en: "Enabled (3rd Position enabled): the node profile is set to 'unknown', homee additionally shows a button for the 3rd (intermediate) position. Disabled (compatibility mode): standard shutter profile as before, without the 3rd position.",
  },
  info_homee_integration: {
    de: "Hier können Sie die Integration in homee konfigurieren. Wenn Sie die homee-Integration aktivieren, kann der ESP32 über die homee-App gesteuert werden. ",
    en: "Here you can configure the integration with homee. If you enable the homee integration, the ESP32 can be controlled via the homee app.",
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
  info_groups: {
    de: "Hier können OPTIONAL zusätzlich Gruppen definiert werden. Eine Gruppe kann mehrere Rolladen gleichzeitig steuern. Über die Bitmaske wird festgelegt, welche Kanäle zu dieser Gruppe gehören.",
    en: "Here you can OPTIONALLY define additional groups. A group can control multiple blinds at the same time. The bitmask determines which channels belong to this group.",
  },
  info_shutters: {
    de: "Hier müssen die Rolladen angelernt und definiert werden, die mit dem ESP32 gesteuert werden sollen.",
    en: "Here you must define the blinds that should be controlled by the ESP32.",
  },
  info_gpios: {
    de: "Hier müssen die GPIOs definiert werden, an denen das CC1101 Funkmodul angeschlossen ist. Dies ist abhängig von Ihrem Hardware-Setup.",
    en: "Here you must define the GPIOs to which the CC1101 radio module is connected. This depends on your hardware setup.",
  },
  help: {
    de: "Hilfe",
    en: "Help",
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
  bitmask_wizard: {
    de: "Auswahl Assistent",
    en: "Selection Wizard",
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
  use_min_time: {
    de: "Frühestens um (HH:MM)",
    en: "Not earlier than (HH:MM)",
  },
  use_max_time: {
    de: "Spätestes um (HH:MM)",
    en: "Not later than (HH:MM)",
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
  remotes: {
    de: "Originalfernbedienungen",
    en: "Original remotes",
  },
  service: {
    de: "Service",
    en: "Service",
  },
  service_cmds: {
    de: "Service-Befehle",
    en: "Service-Commands",
  },
  cmd_endpoint_up: {
    de: "Endlage oben",
    en: "Endpoint up",
  },
  cmd_endpoint_down: {
    de: "Endlage unten",
    en: "Endpoint down",
  },
  set: {
    de: "setzen",
    en: "set",
  },
  delete: {
    de: "löschen",
    en: "delete",
  },
  unlearn: {
    de: "ablernen",
    en: "unlearn",
  },
};

// Position estimates and calibration timing are owned by the ESP, never by this page.
function positionText(key) {
  const lang = document.getElementById("cfg_lang").value === "1" ? "en" : "de";
  return user_translations[key]?.[lang] || key;
}
document.addEventListener("DOMContentLoaded", () => {
  const $ = id => document.getElementById(id);
  const dialog = $("p04_position_dialog"), select = $("p04_selected_shutter");
  const dialogHome=dialog?.parentElement, inlineHost=$("p04_position_inline"), setup=$("p04_service_setup");
  let inlineMode=null;
  function closeWizard() {
    dialog.close();$("p04_position_intro").hidden=false;inlineMode=null;
  }
  function layoutWizard(opening=false) {
    if (!dialog.open && !opening) return;
    const inline=setup.clientWidth>=820;
    if (dialog.open && inline===inlineMode) return;
    if (dialog.open) dialog.close();
    inlineMode=inline;dialog.classList.toggle("position-inline",inline);
    $("p04_position_intro").hidden=inline;
    if(inline){inlineHost.append(dialog);dialog.show();}
    else{dialogHome.append(dialog);dialog.showModal();}
  }
  if (!dialog || !select) return;
  let snapshot, ownToken = 0, pending = false, renderKey = "", intro = false, language = $("cfg_lang").value;
  const connected = () => typeof ws !== "undefined" && ws?.readyState === WebSocket.OPEN;
  const channel = () => Number(select.value);
  const noticeText = key => ({busy:positionText("pos_busy"), queue_full:positionText("pos_queue_full"), unavailable:positionText("pos_unavailable"), not_calibrated:positionText("pos_not_calibrated"), send_failed:positionText("pos_send_failed"), invalid_times:positionText("pos_invalid_times"), saved:positionText("pos_saved"), cancelled:positionText("pos_cancelled"), timeout:positionText("pos_timeout")}[key] || "");
  const send = (a, extra = {}) => {
    if (!connected()) return false;
    sendData("p04_position_cmd", JSON.stringify({ a, c: channel(), token: ownToken, ...extra }));
    return true;
  };
  function el(tag, text, className) {
    const node = document.createElement(tag);
    if (text !== undefined) node.textContent = text;
    if (className) node.className = className;
    return node;
  }
  function visual(height, live=false) {
    const figure=el("div",undefined,"position-illustration");figure.setAttribute("aria-hidden","true");
    const frame=el("div",undefined,"position-window"), shutter=el("div",undefined,"position-panzer");
    shutter.style.height=height+"%";frame.append(shutter);figure.append(frame,el("small",positionText("pos_visual_hint")));
    if(live){const clock=el("div","","position-value");clock.id="p04_position_clock";figure.append(clock);}
    return figure;
  }
  function steps(active) {
    const bar=$("p04_position_steps");bar.replaceChildren();
    [positionText("pos_prepare"),positionText("pos_up"),positionText("pos_down"),positionText("pos_review")].forEach((label,i)=>bar.append(el("span",label,i===active?"active":"")));
  }
  const widgets = Array.from({ length: 16 }, (_, c) => {
    const name = $("p01_jaro_ch_name_" + c);
    const box = el("div", undefined, "position-control");box.hidden=true;
    const label = el("p", "", "info-text"), row = el("div", undefined, "position-actions");
    const status=el("div",undefined,"position-channel-status");status.hidden=true;
    status.setAttribute("role","status");
    const statusCopy=el("div"), statusTitle=el("p","","position-label"), statusValue=el("p","","position-channel-value");
    const frame=el("div",undefined,"position-window"), shutter=el("div",undefined,"position-panzer"), targetLine=el("div",undefined,"position-target-line");
    frame.setAttribute("aria-hidden","true");frame.append(shutter,targetLine);statusCopy.append(statusTitle,statusValue);status.append(statusCopy,frame);
    const slider = document.createElement("input"); slider.type = "range"; slider.min = "0"; slider.max = "100"; slider.value = "50";
    slider.id="p01_position_target_"+c;
    slider.setAttribute("aria-label", positionText("pos_goal"));
    const goalLabel=el("label",positionText("pos_goal"),"position-label");goalLabel.htmlFor=slider.id;
    const output = el("output", "50 %", "position-value");
    slider.style.setProperty("--position-progress",slider.value+"%");
    slider.addEventListener("input", () => {slider.style.setProperty("--position-progress",slider.value+"%");output.textContent = slider.value + " %";targetLine.style.height=(100-Number(slider.value))+"%";});
    const button = el("button", "▶", "position-go"); button.type = "button";button.setAttribute("data-ui-local","");
    button.addEventListener("click", () => {if (connected()) sendData("p04_position_cmd", JSON.stringify({ a:"target", c, v:Number(slider.value) }));});
    const ends=el("div",undefined,"position-ends");ends.append(el("span",positionText("pos_closed")),el("span",positionText("pos_open")));
    const setupLink=el("a",positionText("pos_calibrate_link"),"position-setup-link");setupLink.href="#";
    setupLink.addEventListener("click",event=>{
      event.preventDefault();
      if(!snapshot?.enabled)return;
      select.value=String(c);select.dispatchEvent(new Event("change",{bubbles:true}));
      document.querySelector('a[data-tab="tab4"]')?.click();
      $("p04_position_begin").scrollIntoView({block:"nearest"});$("p04_position_begin").focus();
    });
    row.append(output, button);box.append(label,status,goalLabel,slider,ends,row,setupLink);name.parentElement.append(box);
    return {box,label,status,statusTitle,statusValue,frame,shutter,targetLine,slider,button,ends,row,goalLabel,setupLink};
  });
  function render() {
    if (!snapshot) return;
    const s=snapshot, cal=s.cal, queue=s.queue || [], active=cal.phase || s.moving>=0 || queue.length;
    $("cfg_position_enable").checked=s.enabled;
    const valid=s.channels[channel()]?.[0];
    const count=s.channels.filter((x,i) => x[0] && !$("p04_shutter_name_"+i).hidden && getComputedStyle($("p04_shutter_name_"+i)).display!=="none").length;
    $("p04_position_summary").textContent=s.enabled ? `${count} ${positionText("pos_total")} · ${positionText(valid?"pos_ready":"pos_missing")}` : positionText("pos_disabled");
    $("p04_position_notice").textContent=s.notice ? noticeText(s.notice) : "";
    $("p04_position_begin").disabled=!s.enabled || !!active || !connected();
    $("p04_position_begin").textContent=positionText(valid?"pos_recalibrate":"pos_calibrate");
    $("p04_position_begin").setAttribute("data-i18n",valid?"pos_recalibrate":"pos_calibrate");
    widgets.forEach((w,c) => {
      const [ready,pos]=s.channels[c];w.box.hidden=!s.enabled;
      w.box.classList.toggle("position-uncalibrated",s.enabled && !ready);
      const queueIndex=queue.findIndex(entry=>entry[0]===c), waiting=queueIndex>=0;
      const queuedGoal=waiting?queue[queueIndex][1]:null;
      if(waiting && w.queuedGoal!==queuedGoal) {
        w.slider.value=String(queuedGoal);w.slider.style.setProperty("--position-progress",queuedGoal+"%");
        w.row.children[0].textContent=queuedGoal+" %";
      }
      w.queuedGoal=queuedGoal;
      w.label.hidden=!s.enabled || ready;w.status.hidden=!s.enabled || !ready;w.goalLabel.hidden=w.ends.hidden=w.row.hidden=!s.enabled || !ready;
      w.goalLabel.textContent=positionText(waiting?"pos_queued_goal":"pos_goal");
      w.setupLink.hidden=!s.enabled || ready;w.setupLink.textContent=positionText("pos_calibrate_link");
      w.button.textContent=waiting?"":"▶";
      w.button.setAttribute("aria-busy",String(waiting));
      w.button.title=waiting?`${positionText("pos_waiting")} · ${queuedGoal} % · ${positionText("pos_queue_place")} ${queueIndex+1}`:positionText("pos_go");
      w.button.setAttribute("aria-label",waiting?w.button.title+" · "+positionText("pos_go"):positionText("pos_go"));
      w.slider.setAttribute("aria-label",positionText("pos_goal"));
      w.ends.children[0].textContent=positionText("pos_closed");w.ends.children[1].textContent=positionText("pos_open");
      const moving=s.moving===c;
      w.label.textContent=positionText("pos_missing");
      // No intermediate position is invented during a reference or target run.
      const known=pos>=0 && !moving;
      w.status.classList.toggle("position-unknown",!known);
      w.status.classList.toggle("position-referencing",moving);
      w.statusTitle.textContent=positionText("pos_estimate");
      const progress=s.phase===1 && Number.isFinite(s.remaining)?`${Math.max(0,Math.ceil(s.remaining))} s`:`${s.target} %`;
      w.statusValue.textContent=moving ? ["",positionText("pos_reference"),positionText("pos_settle"),positionText("pos_moving")][s.phase]+` · ${progress}` : known ? `${pos} %` : positionText("pos_unknown");
      w.shutter.style.height=(known?100-pos:100)+"%";
      w.targetLine.hidden=!known;
      w.targetLine.style.height=(100-Number(w.slider.value))+"%";
      w.slider.hidden=w.button.hidden=!s.enabled || !ready;w.slider.disabled=w.button.disabled=!ready || !!cal.phase || moving || !connected();
    });
    if (!dialog.open || intro) return;
    if (!cal.phase) {closeWizard();ownToken=0;renderKey="";return;}
    if (cal.token!==ownToken) {closeWizard();return;}
    $("p04_position_channel").textContent=select.selectedOptions[0]?.textContent;
    const key=`${cal.phase}:${cal.mark}:${$("cfg_lang").value}`;
    if (key!==renderKey) {
      renderKey=key;
      steps(cal.phase<=2?1:cal.phase<=4?2:3);
      const body=$("p04_position_body");body.replaceChildren();
      if (cal.phase===1 || cal.phase===3) {
        const guide=el("div",undefined,"position-guide");guide.append(visual(cal.phase===1?100:0),el("p",positionText(cal.phase===1?"pos_up_hint":"pos_down_hint")));body.append(guide);
        $("p04_position_next").textContent=positionText(cal.phase===1?"pos_start_up":"pos_start_down");
      } else if (cal.phase===2 || cal.phase===4) {
        const keys=cal.phase===2?[positionText("pos_lift"),positionText("pos_half"),positionText("pos_full_open")]:[positionText("pos_half"),positionText("pos_bottom"),positionText("pos_full_closed")];
        const guide=el("div",undefined,"position-guide"), copy=el("div");
        guide.append(visual((cal.phase===2?[100,50,0]:[50,95,100])[cal.mark],true),copy);
        copy.append(el("h3",keys[cal.mark]),el("p",positionText("pos_mark_hint"),"info-text"));
        const list=el("ol",undefined,"position-checkpoints");keys.forEach((k,i)=>list.append(el("li",k+(i<cal.mark?` · ${(cal.times[(cal.phase===2?0:3)+i]/1000).toFixed(1)} s`:""),i<cal.mark?"position-done":i===cal.mark?"position-current":"")));copy.append(list);body.append(guide);
        $("p04_position_next").textContent=positionText("pos_now");
      } else {
        body.append(el("p",positionText("pos_review_hint"),"info-text"));
        const table=el("table");[positionText("pos_lift"),positionText("pos_half"),positionText("pos_full_open"),positionText("pos_half"),positionText("pos_bottom"),positionText("pos_full_closed")].forEach((k,i)=>{const row=el("tr");row.append(el("th",positionText(i<3?"pos_up":"pos_down")+" · "+k),el("td",(cal.times[i]/1000).toFixed(1)+" s"));table.append(row);});body.append(table);
        $("p04_position_next").textContent=positionText("pos_apply");
      }
    }
    const clock=$("p04_position_clock");if(clock){clock.textContent=cal.elapsed.toFixed(1)+" s";clock.setAttribute("aria-label",positionText("pos_time")+": "+clock.textContent);}
    pending=false;$("p04_position_next").disabled=false;
  }
  $("p04_position_begin").addEventListener("click", () => {
    intro=true;pending=false;renderKey="";ownToken=crypto.getRandomValues(new Uint32Array(1))[0]||1;
    $("p04_position_channel").textContent=select.selectedOptions[0]?.textContent;
    steps(0);
    const guide=el("div",undefined,"position-guide"), copy=el("div");
    copy.append(el("h3",positionText("pos_initial")),el("p",positionText("pos_initial_hint")),el("p",positionText("pos_reference_hint"),"info-text"));guide.append(visual(100),copy);$("p04_position_body").replaceChildren(guide);
    $("p04_position_next").textContent=positionText("pos_confirm");$("p04_position_next").disabled=false;layoutWizard(true);
  });
  $("p04_position_next").addEventListener("click", () => {
    if(pending)return;
    if(intro) {intro=false;pending=send("begin");}
    else {const phase=snapshot?.cal.phase;pending=send(phase===1||phase===3?"start":phase===5?"save":"mark");}
    $("p04_position_next").disabled=pending;
  });
  const cancel=()=>{if(ownToken)send("cancel");intro=false;pending=false;ownToken=0;closeWizard();};
  $("p04_position_cancel").addEventListener("click",cancel);dialog.addEventListener("cancel",event=>{event.preventDefault();cancel();});
  select.addEventListener("change",()=>{if(dialog.open)cancel();render();});
  window.addEventListener("webui:message",event=>{if(event.detail.type==="shutterPosition"){snapshot=event.detail;render();}});
  window.addEventListener("webui:connected",()=>send("status"));
  $("cfg_lang").addEventListener("change",()=>{widgets.forEach(w=>{w.button.title=positionText("pos_go");w.button.setAttribute("aria-label",positionText("pos_go"));w.slider.setAttribute("aria-label",positionText("pos_goal"));});renderKey="";render();});
  if(typeof ResizeObserver!=="undefined") new ResizeObserver(()=>layoutWizard()).observe(setup);
  setInterval(()=>{
    if(!connected()) {
      if(dialog.open){closeWizard();intro=false;pending=false;ownToken=0;$("p04_position_notice").textContent=positionText("pos_no_link");}
      widgets.forEach(w=>{w.button.disabled=w.slider.disabled=true;});return;
    }
    if(dialog.open && !intro && ownToken)send("beat");
    if(language!==$("cfg_lang").value){language=$("cfg_lang").value;renderKey="";render();}
  },1000);
});

// ESP status registration only: this dialog never transmits motor commands.
document.addEventListener("DOMContentLoaded",()=>{
  const $=id=>document.getElementById(id),dialog=$("remote_mapping_dialog");if(!dialog)return;
  const text=key=>user_translations[key]?.[$("cfg_lang").value==="1"?"en":"de"]||key;
  const errorText=key=>({busy:text("rm_busy"),unavailable:text("rm_unavailable"),invalid:text("rm_invalid"),no_signal:text("rm_no_signal"),targets:text("rm_targets"),full:text("rm_full"),motor_full:text("rm_motor_full"),timeout:text("rm_timeout"),group:text("rm_group"),different:text("rm_different")}[key]||text("rm_invalid"));
  const hex=(value,width)=>Number(value).toString(16).padStart(width,"0");
  const online=()=>typeof ws!=="undefined" && ws?.readyState===WebSocket.OPEN;
  const el=(tag,value)=>{const n=document.createElement(tag);if(value!==undefined)n.textContent=value;return n;};
  const selected=()=>Number($("p04_selected_shutter").value);
  let state,token=0,pending=false,wait,channel=0,abandoned=0,abandonedChannel=0,namedSource="";
  const send=(a,extra={})=>{if(!online())return false;sendData("remote_mapping_cmd",JSON.stringify({a,token,channel:dialog.open?channel:selected(),...extra}));return true;};
  const notice=message=>$(dialog.open?"remote_mapping_dialog_notice":"remote_mapping_notice").textContent=message;
  function render(){
    if(!state || state.channel!==selected())return;
    const body=$("remote_mapping_rows");body.replaceChildren();
    $("remote_mapping_empty").hidden=state.items.length>0;
    $("remote_mapping_count").textContent=`${state.items.length} / ${state.limit}`;
    for(const row of state.items){
      const line=el("div");line.className="remote-mapping-row";
      const info=el("div"),name=el("strong",row.name);
      const details=el("small",`${hex(row.serial,8)} · ${hex(row.mask,4)}${row.legacy?" · "+text("rm_legacy"):""}`);
      info.append(name,details);line.append(info);
      const remove=el("button",text("rm_delete"));remove.type="button";remove.setAttribute("data-ui-local","");remove.disabled=pending||!online();
      remove.addEventListener("click",()=>{
        if(!confirm(text("rm_delete_confirm")))return;
        token=crypto.getRandomValues(new Uint32Array(1))[0]||1;
        request("delete",{id:row.id,serial:row.serial,mask:row.mask});
      });line.append(remove);body.append(line);
    }
    $("remote_mapping_add").disabled=pending||!online()||!state.enabled;
    if(!dialog.open)return;
    const owns=state.token===token && state.captureChannel===channel;
    const phase=owns?state.phase:0;
    $("remote_mapping_steps").textContent=[text("rm_step_capture"),text("rm_step_confirm"),text("rm_step_save")].map((label,i)=>`${phase===i+1?"●":"○"} ${label}`).join("  ·  ");
    $("remote_mapping_signal").textContent=phase>1?`${hex(state.serial,8)} · ${hex(state.mask,4)}`:text("rm_waiting");
    $("remote_mapping_phase").textContent=text(phase===2?"rm_confirm":phase===3?"rm_confirmed":"rm_hint");
    const source=`${state.serial}:${state.mask}`;
    if(owns && phase>1 && source!==namedSource){namedSource=source;$("remote_mapping_name").value=state.existingName||"";}
    $("remote_mapping_shared").hidden=!(owns && state.existingName);
    $("remote_mapping_name").disabled=phase!==3||pending;
    $("remote_mapping_save").disabled=pending||!online()||phase!==3;
    $("remote_mapping_retry").disabled=pending;
  }
  function request(a,extra={}){
    if(!send(a,extra)){notice(text("rm_offline"));return;}
    pending=true;render();clearTimeout(wait);
    wait=setTimeout(()=>{pending=false;notice(text("rm_no_reply"));render();},6000);
  }
  function close(){
    clearTimeout(wait);
    if(token && !send("cancel")){abandoned=token;abandonedChannel=channel;}
    dialog.close();token=0;pending=false;namedSource="";render();
  }
  $("remote_mapping_add").addEventListener("click",()=>{
    channel=selected();token=crypto.getRandomValues(new Uint32Array(1))[0]||1;namedSource="";
    $("remote_mapping_name").value="";$("remote_mapping_channel").textContent=state?.channelName||"";
    dialog.showModal();notice("");request("listen");
  });
  $("remote_mapping_close").addEventListener("click",close);
  dialog.addEventListener("cancel",event=>{event.preventDefault();close();});
  $("remote_mapping_retry").addEventListener("click",()=>{namedSource="";notice("");request("listen");});
  $("remote_mapping_save").addEventListener("click",()=>{
    if(pending)return;
    const name=$("remote_mapping_name").value.trim();
    if(!name || Array.from(name).length>32 || /[\u0000-\u001f\u007f]/u.test(name)){notice(text("rm_invalid"));return;}
    request("save",{name});
  });
  $("p04_selected_shutter").addEventListener("change",()=>{state=undefined;$("remote_mapping_rows").replaceChildren();send("status");});
  document.querySelectorAll('[data-tab="tab4"]').forEach(n=>n.addEventListener("click",()=>send("status")));
  window.addEventListener("webui:message",event=>{
    const data=event.detail;if(data.type!=="remoteMappings" || data.channel!==selected())return;
    const reply=token && data.reply===token,owns=token && data.token===token;
    if(reply||(!data.reply && owns)){clearTimeout(wait);pending=false;notice(data.error?errorText(data.error):"");}
    if(reply && ["saved","deleted","cancelled"].includes(data.result)){token=0;dialog.close();namedSource="";}
    state=data;render();
  });
  window.addEventListener("webui:connected",()=>{
    if(abandoned){send("cancel",{token:abandoned,channel:abandonedChannel});abandoned=0;}
    send("status");
  });
  window.addEventListener("webui:disconnected",()=>{if(dialog.open)close();pending=false;render();notice(text("rm_offline"));});
  $("cfg_lang").addEventListener("change",render);
  // Hidden fields are required by the existing configuration-to-DOM loader.
  const hidden=$("remote_mapping_config");
  for(let i=0;i<80;++i)for(const field of ["name","serial","sourceMask","targets","legacy"]){const n=el("input");n.type="hidden";n.id=`cfg_remoteMappings_${i}_${field}`;hidden.append(n);}
});

// Reuse received network classes and RSSI; no additional requests or timers.
document.addEventListener('DOMContentLoaded', () => {
  const wifi = document.getElementById('p00_wifi_icon');
  const eth = document.getElementById('p00_eth_icon');
  const rssi = document.getElementById('p09_wifi_rssi');
  const language = document.getElementById('cfg_lang');
  const version = document.getElementById('p00_version');
  const logout = document.querySelector('.sidebar-logout');
  const update = () => {
    const en = language?.value === '1';
    const connected = en ? 'connected' : 'verbunden';
    const disconnected = en ? 'not connected' : 'nicht verbunden';
    const wifiConnected = /\bi_wifi_[1-4]\b/.test(wifi.className);
    const signal = rssi?.textContent.match(/-?\d+/)?.[0];
    const wifiText = `WLAN ${wifiConnected ? connected : disconnected}${wifiConnected && signal ? ` · ${signal} dBm` : ''}`;
    wifi.title = wifiText;
    wifi.setAttribute('aria-label', wifiText);
    const ethText = `Ethernet ${eth.classList.contains('i_eth_ok') ? connected : disconnected}`;
    eth.title = ethText;
    eth.setAttribute('aria-label', ethText);
    version.title = en ? 'Version and GitHub updates' : 'Version und GitHub-Updates';
    logout.title = en ? 'Log out' : 'Abmelden';
    logout.setAttribute('aria-label', logout.title);
  };
  const observer = new MutationObserver(update);
  [wifi, eth].forEach(icon => observer.observe(icon, { attributes: true, attributeFilter: ['class'] }));
  if (rssi) observer.observe(rssi, { childList: true, characterData: true, subtree: true });
  if (language) {
    language.addEventListener('change', update);
    observer.observe(language, { attributes: true, attributeFilter: ['value'] });
  }
  update();
});

// Keep the existing server-driven error element in the visible header.
function initJaroliftHeaderError() {
  const error = document.getElementById("errorBar");
  const desktop = document.querySelector(".app-header .app-modes");
  const mobile = document.querySelector(".topMenu");
  const narrow = window.matchMedia("(max-width: 768px)");
  const update = () => {
    const host = narrow.matches ? mobile : desktop;
    if (error.parentElement !== host) host.append(error);
  };
  narrow.addEventListener("change", update);
  update();
}

function initPageHeader() {
  const status = document.querySelector(".app-connection-actions");
  const indicator = document.getElementById("p00_web_connection");
  const label = document.getElementById("p00_web_connection_text");
  const reload = document.getElementById("p99_reloadButton");
  const desktop = document.querySelector(".app-header");
  const mobile = document.querySelector(".topMenu");
  const content = document.querySelector(".home-section");
  const narrow = window.matchMedia("(max-width: 768px)");
  const controls = document.querySelectorAll("[data-connection-required]");
  const layout = () => {
    const host = narrow.matches ? mobile : desktop;
    if (status.parentElement !== host) host.append(status);
    if (narrow.matches) content.style.top = (mobile.getBoundingClientRect().bottom + 20) + "px";
    else content.style.removeProperty("top");
  };
  const update = () => {
    const connected = !!(typeof ws !== "undefined" && ws && ws.readyState === WebSocket.OPEN);
    const language = document.getElementById("cfg_lang").value === "1" ? "en" : "de";
    const key = connected ? "ui_connected" : "ui_disconnected";
    indicator.classList.toggle("is-connected", connected);
    label.dataset.i18n = key;
    label.textContent = (connected ? user_translations.ui_connected : user_translations.ui_disconnected)[language];
    reload.hidden = connected || isGitHubPages();
    reload.title = reload.ariaLabel = user_translations.ui_reload[language];
    // Inert preserves individual disabled states set by the ESP on reconnect.
    controls.forEach(control => { control.inert = !connected && !isGitHubPages(); });
    layout();
  };
  window.addEventListener("webui:connected", update);
  // Retain the existing library's reconnect signal without displaying a footer bar.
  new MutationObserver(update).observe(document.getElementById("connectionLostBar"), {
    attributes: true, attributeFilter: ["style", "class"],
  });
  document.getElementById("cfg_lang").addEventListener("change", update);
  narrow.addEventListener("change", layout);
  new ResizeObserver(layout).observe(mobile);
  update();
}

function initMobileNavigation() {
  const menu = document.getElementById("topMenuDropdown").parentElement;
  // Close explicitly without Pico's full-screen summary pseudo-element.
  document.addEventListener("click", event => {
    if (!menu.contains(event.target) || event.target.closest("a[data-tab]")) menu.open = false;
  });
  menu.addEventListener("keydown", event => {
    if (event.key === "Escape") {
      menu.open = false;
      menu.querySelector("summary").focus();
    }
  });
}

function initHomeAssistantSettings() {
  const mqtt = document.getElementById("cfg_mqtt_enable");
  const section = document.getElementById("p12_ha_settings");
  const update = () => {
    document.getElementById("p12_ha_mqtt_warning").hidden = mqtt.checked;
  };
  mqtt.addEventListener("change", update);
  section.addEventListener("toggle", update);
  document.getElementById("p12_ha_mqtt_link").addEventListener("click", event => {
    event.preventDefault();
    const target = document.getElementById("p12_mqtt_settings");
    target.open = true;
    target.scrollIntoView({ block: "nearest" });
    mqtt.focus({ preventScroll: true });
  });
  update();
}

function initMqttConnectionStatus() {
  const input = document.getElementById("p12_mqtt_connection");
  const state = document.getElementById("p12_mqtt_connection_state");
  const update = () => {
    if (state.textContent === "connected") {
      input.setAttribute("aria-invalid", "false");
    } else if (state.textContent === "disconnected") {
      input.setAttribute("aria-invalid", "true");
    } else {
      input.removeAttribute("aria-invalid");
    }
  };
  new MutationObserver(update).observe(state, {
    childList: true,
    characterData: true,
    subtree: true,
  });
  update();
}

function validateHost(input) {
  input.value = input.value.trim();
  if (/^[\d.]+$/.test(input.value)) {
    validateIP(input);
    return;
  }
  const hostPattern =
    /^[a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?)*$/;
  if (!hostPattern.test(input.value)) {
    input.setAttribute("aria-invalid", "true");
  } else {
    input.setAttribute("aria-invalid", "false");
    sendData(input.id, input.value);
  }
}

document.addEventListener("DOMContentLoaded", function () {
  // call functions on refresh
  initPageHeader();
  initJaroliftHeaderError();
  initMobileNavigation();
  initHomeAssistantSettings();
  initInformationPages();
  initMqttConnectionStatus();
  initWiFiApDialog();
  initWiFiSsidDialog();
  loadSimulatedData();
  setupBitmaskDialog();
  autoRefreshLog();
});

// open bitmask help dialog
function openGrpMaskHelp(button) {
  const dialog = document.getElementById("p12_bitmask_dialog");
  if (dialog) {
    dialog.showModal();
  } else {
    console.error("Dialog mit ID 'p12_bitmask_dialog' wurde nicht gefunden.");
  }
}

// close bitmask help dialog
function closeGrpMaskHelp() {
  const dialog = document.getElementById("p12_bitmask_dialog");
  if (dialog) {
    dialog.close();
  } else {
    console.error("Dialog mit ID 'p12_bitmask_dialog' wurde nicht gefunden.");
  }
}

function setupBitmaskDialog() {
  const bitmaskDialog = document.getElementById("bitmask_dialog");
  const applyButton = document.getElementById("apply_bitmask");
  const closeButton = document.getElementById("close_bitmask_dialog");

  let currentInput = null; // reference to existing input

  // open the dialog for the source input field
  document.querySelectorAll(".bitmask-input").forEach((input) => {
    input.addEventListener("click", () => {
      currentInput = input;

      // Parse the bitmask as a mutable variable
      let bitmask = parseInt(input.value || "0", 2);

      // Set checkboxes based on actual bitmask
      for (let i = 0; i < 16; i++) {
        const checkbox = document.getElementById(`channel-${i}`);
        if (checkbox) {
          // Check the visibility of the checkbox
          const isVisible = checkbox.parentElement.style.display !== "none";

          // Set checkbox state only if visible
          checkbox.checked = isVisible && (bitmask & (1 << i)) !== 0;

          // Clear bit if the checkbox is not visible
          if (!isVisible) {
            bitmask &= ~(1 << i);
          }
        }
      }

      // Safely update the input value with the sanitized bitmask
      if (currentInput && !Object.isFrozen(currentInput)) {
        currentInput.value = bitmask.toString(2).padStart(16, "0");
      }

      bitmaskDialog.showModal();
    });
  });

  // apply checkboxes and close the dialog
  applyButton.addEventListener("click", () => {
    let bitmask = 0;
    for (let i = 0; i < 16; i++) {
      const checkbox = document.getElementById(`channel-${i}`);
      if (checkbox && checkbox.checked) {
        bitmask |= 1 << i;
      }
    }

    if (currentInput) {
      currentInput.value = bitmask.toString(2).padStart(16, "0");
    }

    bitmaskDialog.close();
  });

  // close the dialog without changes
  closeButton.addEventListener("click", () => {
    bitmaskDialog.close();
  });
}

async function loadSimulatedData() {
  if (!isGitHubPages()) {
    return; // Kein Simulationsmodus, wenn nicht auf GitHub Pages
  }

  console.log("GitHub Pages erkannt – Simulationsdaten werden geladen.");

  try {
    const response = await fetch("sim.json");
    if (!response.ok)
      throw new Error("Fehler beim Abrufen der Simulationsdaten");

    const simData = await response.json();
    updateJSON(simData); // Aktualisiert die UI mit den Simulationsdaten
    // Demo only: the real device sends this snapshot through WebSocket.
    const configResponse = await fetch("config.json");
    if (!configResponse.ok) throw new Error("Could not load demo configuration");
    const demoConfig = await configResponse.json();
    const calibration = demoConfig.position?.calibration || [];
    window.dispatchEvent(new CustomEvent("webui:message", { detail: {
      type: "shutterPosition", enabled: !!demoConfig.position?.enable,
      notice: "", moving: -1, phase: 0, target: 0, remaining: 0, queue: [],
      channels: Array.from({length: 16}, (_, i) => [
        !!demoConfig.jaro.ch_enable[i] && calibration[i]?.length === 6 && calibration[i].every(t => t > 0), -1
      ]),
      cal: {phase: 0, channel: 0, mark: 0, token: 0, elapsed: 0, times: []}
    }}));
  } catch (error) {
    console.error("Fehler beim Laden von sim.json:", error);
  }
}

function updateUIcallbackSelect(elementId, value) {
  const element = document.getElementById(elementId);
  if (element) {
    // Rufe toggleTimeInputs nur auf, wenn data-toggle="timeInputs" gesetzt ist
    if (element.dataset.toggle === "timeInputs") {
      toggleTimeInputs(element);
      toggleTimeInputsMinMax(element);
    }
  }
}

function toggleTimeInputsMinMax(selectElement) {
  const matches = selectElement.id.match(/\d+/g);
  const timerId = matches[matches.length - 1];
  const minMaxTimeSettings = document.getElementById(
    `timer${timerId}-minmaxtime-settings`
  );

  //  value: 0 == hide
  if (selectElement.value === "0") {
    minMaxTimeSettings.style.display = "none";
  }
  // otherwise show
  else {
    minMaxTimeSettings.style.display = "block";
  }
}

function toggleTimeInputs(selectElement) {
  toggleTimeInputsMinMax(selectElement);

  const matches = selectElement.id.match(/\d+/g);
  const timerId = matches[matches.length - 1];
  const timeInput = document.getElementById(`timeInput${timerId}`);
  const offsetInput = document.getElementById(`offsetInput${timerId}`);

  if (!timeInput || !offsetInput) {
    console.error("Eines der Elemente nicht gefunden!");
    return;
  }
  //  value:0 == fixed Time
  if (selectElement.value === "0") {
    timeInput.style.display = "block";
    offsetInput.style.display = "none";
  } else {
    timeInput.style.display = "none";
    offsetInput.style.display = "block";
  }
}

function autoRefreshLog() {
  // refresh log when tab10 is clicked
  document.querySelectorAll('a[data-tab="tab10"]').forEach(function (el) {
    el.addEventListener("click", function (e) {
      e.preventDefault();
      console.log("refresh log");
      sendData("refresh_log", "");
    });
  });
}

function validateWiFiBssid(input) {
  input.value = input.value.trim().toUpperCase();
  if (!input.value) {
    input.removeAttribute("aria-invalid");
    return true;
  }
  const valid = /^(?:[0-9A-F]{2}:){5}[0-9A-F]{2}$/.test(input.value);
  input.setAttribute("aria-invalid", valid ? "false" : "true");
  return valid;
}

function wifiApText(key) {
  const lang = document.getElementById("cfg_lang").value === "1" ? "en" : "de";
  return user_translations[key][lang];
}

function initWiFiApDialog() {
  const input = document.getElementById("cfg_wifi_bssid");
  const dialog = document.getElementById("p12_wifi_ap_dialog");
  const list = document.getElementById("p12_wifi_ap_list");
  const status = document.getElementById("p12_wifi_ap_status");
  const payload = document.getElementById("p12_wifi_ap_results");
  const search = document.getElementById("p12_wifi_ap_scan_btn");
  const retry = document.getElementById("p12_wifi_ap_retry_btn");
  const apply = document.getElementById("p12_wifi_ap_apply_btn");
  let ssid = "";
  let selected = "";
  let timer;

  const setBusy = (busy) => {
    search.disabled = retry.disabled = apply.disabled = list.disabled = busy;
    status.setAttribute("aria-busy", busy ? "true" : "false");
  };
  const render = (accessPoints) => {
    list.replaceChildren();
    const addOption = (value, text) => {
      const option = document.createElement("option");
      option.value = value;
      option.textContent = text;
      list.appendChild(option);
    };
    addOption("", wifiApText("wifi_ap_auto"));
    const seen = new Set();
    accessPoints.slice().sort((a, b) => b.rssi - a.rssi).forEach((ap) => {
      const bssid = String(ap.bssid).toUpperCase();
      if (!/^(?:[0-9A-F]{2}:){5}[0-9A-F]{2}$/.test(bssid) || seen.has(bssid)) return;
      seen.add(bssid);
      const current = ap.current ? ` — ${wifiApText("wifi_ap_current")}` : "";
      addOption(bssid, `${bssid} — ${ap.rssi} dBm — ${wifiApText("wifi_ap_channel")} ${ap.channel}${current}`);
    });
    if (selected && !seen.has(selected)) {
      addOption(selected, `${selected} — ${wifiApText("wifi_ap_not_found")}`);
    }
    list.value = selected;
  };
  const fail = (key) => {
    clearTimeout(timer);
    setBusy(false);
    status.textContent = wifiApText(key);
  };
  const scan = () => {
    clearTimeout(timer);
    if (!ssid || new TextEncoder().encode(ssid).length > 32) {
      fail("wifi_ap_invalid_ssid");
      return;
    }
    setBusy(true);
    status.textContent = wifiApText("wifi_ap_scanning");
    // Pass the field value explicitly so a preceding edit need not be saved yet.
    sendData("p12_wifi_ap_scan_cmd", ssid);
    timer = setTimeout(() => fail("wifi_ap_failed"), 25000);
  };

  search.addEventListener("click", () => {
    ssid = document.getElementById("cfg_wifi_ssid").value;
    selected = validateWiFiBssid(input) ? input.value : "";
    document.getElementById("p12_wifi_ap_ssid").textContent = ssid;
    render([]);
    if (!dialog.open) dialog.showModal();
    scan();
  });
  retry.addEventListener("click", scan);
  list.addEventListener("change", () => { selected = list.value; });
  document.getElementById("p12_wifi_ap_close_btn").addEventListener("click", () => dialog.close());
  apply.addEventListener("click", () => {
    if (document.getElementById("cfg_wifi_ssid").value !== ssid) {
      fail("wifi_ap_ssid_changed");
      return;
    }
    input.value = list.value;
    if (validateWiFiBssid(input)) {
      sendData(input.id, input.value);
      dialog.close();
    }
  });
  new MutationObserver(() => {
    let result;
    try { result = JSON.parse(payload.textContent); } catch { return; }
    if (result.scan === "ssid" || result.ssid !== ssid) return;
    if (result.status === "scanning") {
      setBusy(true);
      status.textContent = wifiApText("wifi_ap_scanning");
      return;
    }
    clearTimeout(timer);
    setBusy(false);
    if (result.status === "done") {
      render(Array.isArray(result.aps) ? result.aps : []);
      status.textContent = wifiApText(result.aps && result.aps.length ? "wifi_ap_found" : "wifi_ap_empty");
    } else {
      fail(result.status === "busy" ? "wifi_ap_busy" : result.status === "invalid_ssid" ? "wifi_ap_invalid_ssid" : "wifi_ap_failed");
    }
  }).observe(payload, { childList: true, characterData: true, subtree: true });
}

function initWiFiSsidDialog() {
  const input = document.getElementById("cfg_wifi_ssid");
  const dialog = document.getElementById("p12_wifi_ssid_dialog");
  const list = document.getElementById("p12_wifi_ssid_list");
  const status = document.getElementById("p12_wifi_ssid_status");
  const search = document.getElementById("p12_wifi_ssid_scan_btn");
  const retry = document.getElementById("p12_wifi_ssid_retry_btn");
  const apply = document.getElementById("p12_wifi_ssid_apply_btn");
  let selected = "";
  let busy = false;
  let timer;

  const setBusy = (value) => {
    busy = value;
    search.disabled = retry.disabled = list.disabled = busy;
    apply.disabled = busy || !list.value;
    status.setAttribute("aria-busy", busy ? "true" : "false");
  };
  const render = (accessPoints) => {
    const networks = new Map();
    accessPoints.forEach((ap) => {
      const ssid = String(ap.ssid ?? "");
      if (!ssid || new TextEncoder().encode(ssid).length > 32) return;
      const previous = networks.get(ssid);
      const current = Boolean(ap.current || (previous && previous.current));
      networks.set(ssid, { ...(previous && previous.rssi > ap.rssi ? previous : ap), ssid, current });
    });
    list.replaceChildren();
    const addOption = (value, text) => {
      const option = document.createElement("option");
      option.value = value;
      option.textContent = text;
      list.appendChild(option);
      return option;
    };
    addOption("", wifiApText("wifi_ssid_choose")).disabled = true;
    [...networks.values()].sort((a, b) => b.rssi - a.rssi).forEach((network) => {
      const current = network.current ? ` — ${wifiApText("wifi_ap_current")}` : "";
      addOption(network.ssid, `${network.ssid} — ${network.rssi} dBm — ${wifiApText("wifi_ap_channel")} ${network.channel}${current}`);
    });
    if (selected && !networks.has(selected)) {
      addOption(selected, `${selected} — ${wifiApText("wifi_ap_not_found")}`);
    }
    list.value = selected;
    setBusy(busy);
    return networks.size;
  };
  const fail = (key) => {
    clearTimeout(timer);
    setBusy(false);
    status.textContent = wifiApText(key);
  };
  const scan = () => {
    clearTimeout(timer);
    setBusy(true);
    status.textContent = wifiApText("wifi_ap_scanning");
    sendData("p12_wifi_ssid_scan_cmd", "");
    timer = setTimeout(() => fail("wifi_ap_failed"), 25000);
  };
  search.addEventListener("click", () => {
    selected = input.value;
    render([]);
    if (!dialog.open) dialog.showModal();
    scan();
  });
  retry.addEventListener("click", scan);
  list.addEventListener("change", () => {
    selected = list.value;
    setBusy(busy);
  });
  document.getElementById("p12_wifi_ssid_close_btn").addEventListener("click", () => dialog.close());
  apply.addEventListener("click", () => {
    if (busy || !list.value) return;
    if (input.value !== list.value) {
      const bssid = document.getElementById("cfg_wifi_bssid");
      bssid.value = "";
      validateWiFiBssid(bssid);
      sendData(bssid.id, "");
    }
    input.value = list.value;
    sendData(input.id, input.value);
    dialog.close();
  });
  const payload = document.getElementById("p12_wifi_ap_results");
  new MutationObserver(() => {
    let result;
    try { result = JSON.parse(payload.textContent); } catch { return; }
    if (result.scan !== "ssid") return;
    if (result.status === "scanning") {
      setBusy(true);
      status.textContent = wifiApText("wifi_ap_scanning");
      return;
    }
    clearTimeout(timer);
    setBusy(false);
    if (result.status === "done") {
      const count = render(Array.isArray(result.aps) ? result.aps : []);
      status.textContent = wifiApText(count ? "wifi_ssid_found" : "wifi_ssid_empty");
    } else {
      fail(result.status === "busy" ? "wifi_ap_busy" : "wifi_ap_failed");
    }
  }).observe(payload, { childList: true, characterData: true, subtree: true });
}


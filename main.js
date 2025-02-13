let ws;
let heartbeatTimeout;
const maxReconnectDelay = 5000;
const minReconnectDelay = 1000;
let reconnectDelay = minReconnectDelay;

function setupWS() {
  ws = new WebSocket("ws://" + window.location.host + "/ws");

  ws.onopen = function () {
    console.log("WebSocket connected");
    resetHeartbeat();
    hideReloadBar();
    reconnectDelay = minReconnectDelay;
  };

  ws.onclose = function () {
    console.log("WebSocket disconnected. Retrying...");
    attemptReconnect();
    showReloadBar();
  };

  ws.onerror = function (error) {
    console.log("WebSocket error:", error);
  };

  ws.onmessage = function (event) {
    let message = JSON.parse(event.data);
    //console.log(message);
    if (message.type === "redirect") {
      console.log("Redirecting to:", message.url);
      ws.close();
      window.location.href = message.url;
    } else if (message.type === "heartbeat") {
      resetHeartbeat();
    } else if (message.type === "loadConfig") {
      loadConfig();
    } else if (message.type === "updateText") {
      updateText(message);
    } else if (message.type === "setLanguage") {
      setLanguage(message);
    } else if (message.type === "updateJSON") {
      updateJSON(message);
    } else if (message.type === "updateValue") {
      updateValue(message);
    } else if (message.type === "updateState") {
      updateState(message);
    } else if (message.type === "updateSetIcon") {
      updateSetIcon(message);
    } else if (message.type === "hideElement") {
      hideElement(message);
    } else if (message.type === "updateHref") {
      updateHref(message);
    } else if (message.type === "updateBusy") {
      updateBusy(message);
    } else if (message.type === "updateDisabled") {
      updateDisabled(message);
    } else if (message.type === "showElementClass") {
      showElementClass(message);
    } else if (message.type === "logger") {
      logger(message);
    } else if (message.type === "cmdLogClr") {
      cmdLogClr(message);
    } else if (message.type === "otaProgress") {
      otaProgress(message);
    } else if (message.type === "updateDialog") {
      updateDialog(message);
    }
  };
}

// Heartbeat-Timeout zurücksetzen
function resetHeartbeat() {
  clearTimeout(heartbeatTimeout);
  heartbeatTimeout = setTimeout(() => {
    console.warn("No heartbeat received, reconnecting...");
    ws.close();
    showReloadBar();
  }, 5000);
}

function attemptReconnect() {
  setTimeout(() => {
    console.log(`Attempting reconnect in ${reconnectDelay / 1000} seconds...`);
    setupWS();
    reconnectDelay = Math.min(reconnectDelay * 2, maxReconnectDelay);
  }, reconnectDelay);
}

function restartFunction() {
  const activeElement = document.activeElement;
  if (activeElement && ["INPUT", "TEXTAREA"].includes(activeElement.tagName)) {
    activeElement.blur(); // save last active input
  }

  // add a delay to be sure the last input is complete
  setTimeout(function () {
    sendData("restartAction", "true");
  }, 1000);
}

function sendData(elementId, value) {
  // Find the element by its ID
  const element = document.getElementById(elementId);

  // Check if the element exists and is a password field
  if (element && element.type === "password") {
    // Only send the data if the value is not "XxXxXxXxXxX"
    if (value === "XxXxXxXxXxX") {
      console.log(`Password field (${elementId}) not updated, skipping send.`);
      return;
    }
  }

  if (ws.readyState === WebSocket.OPEN) {
    const message = {
      type: "sendData",
      elementId: elementId,
      value: String(value),
    };
    ws.send(JSON.stringify(message));
  } else {
    console.error("WebSocket is not open. Cannot send data.");
  }
}

function updateText(data) {
  var element = document.getElementById(data.id);
  //console.log(element);
  if (element) {
    if (data.isInput) {
      element.value = data.text;
    } else {
      element.innerHTML = data.text;
    }
  }
}

function setLanguage(data) {
  console.log("Set language to:", data.language);
  localizePage(data.language);
}

function updateJSON(data) {
  Object.keys(data).forEach(function (key) {
    if (key !== "type") {
      // skip first element "type"
      let elementID = key;
      let element = document.getElementById(elementID);
      if (!element) {
        console.error("unknown element:", key);
        return;
      }
      let value = data[key];

      if (element.tagName === "INPUT") {
        if (element.type === "checkbox" || element.type === "radio") {
          element.checked = value === "true";
        } else if (element.type === "range") {
          element.value = value;
          let linkedTextElementId = element.getAttribute("data-value-id");
          if (linkedTextElementId) {
            let linkedTextElement =
              document.getElementById(linkedTextElementId);
            if (linkedTextElement) {
              linkedTextElement.innerHTML = value;
            }
          }
        } else {
          // all other input elements
          element.value = value;
        }
      } else if (element.tagName === "SELECT") {
        element.value = value;
        // check if value is valid
        if (
          !Array.from(element.options).some((option) => option.value === value)
        ) {
          console.warn(
            `Value "${value}" not found in <select> options for element:`,
            key
          );
        }
      } else if (element.tagName === "I") {
        // change icons <i>
        element.className = "svg " + value;
      } else if ("innerHTML" in element) {
        // all other elements with `innerHTML` (<td>, <div>, <span>, ...)
        element.innerHTML = value;
      } else {
        console.error("unhandled element type:", element.tagName);
      }
    }
  });
}

function updateValue(data) {
  var element = document.getElementById(data.id);
  if (element) {
    if (data.isInput) {
      element.value = data.value;
    }
  }
}

// update switch elements
function updateState(data) {
  var element = document.getElementById(data.id);
  if (element && (element.type === "checkbox" || element.type === "radio")) {
    element.checked = data.state;
    toggleElementVisibility(element.getAttribute("hideOpt"), element.checked);
  }
}

// update add class to element
function updateSetIcon(data) {
  var element = document.getElementById(data.id);
  if (element) {
    element.className = "svg " + data.icon;
  }
}

// hide/show element
function hideElement(data) {
  var element = document.getElementById(data.id);
  if (element) {
    element.style.display = data.hide ? "none" : "";
  }
}

// update href
function updateHref(data) {
  var element = document.getElementById(data.id);
  if (element) {
    element.href = data.href;
  }
}

// update Busy
function updateBusy(data) {
  var element = document.getElementById(data.id);
  if (element) {
    element.setAttribute("aria-busy", data.busy);
  }
}

// disable/enable element
function updateDisabled(data) {
  var element = document.getElementById(data.id);
  if (element) {
    element.disabled = data.disabled;
  }
}

// hide/show elements based on className
function showElementClass(data) {
  const elements = document.querySelectorAll(`.${data.className}`);
  elements.forEach((element) => {
    element.style.display = data.show ? "inline-flex" : "none";
  });
}

// add log message
function logger(data) {
  var logOutput = document.getElementById("p10_log_output");
  if (data.cmd === "add_log") {
    logOutput.innerHTML = "";
    data.entry.forEach(function (entry) {
      logOutput.innerHTML += entry + "<br>";
    });
  } else if (data.cmd === "clr_log") {
    logOutput.innerHTML = "";
  }
}

// update ota-progress bar
function otaProgress(data) {
  clearTimeout(heartbeatTimeout);
  var progress = data.progress;
  document.getElementById("ota_progress_bar").value = progress;
  document.getElementById(
    "ota_status_txt"
  ).textContent = `Update Progress: ${progress}%`;
}

// close update dialog
function updateDialog(data) {
  var dialog = document.getElementById(data.id);
  if (data.state == "open") {
    dialog.showModal();
  } else if (data.state == "close") {
    dialog.close();
  }
}

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

// Function for switching the visibility of elements
function toggleElementVisibility(className, isVisible) {
  const elements = document.querySelectorAll(`.${className}`);
  elements.forEach((element) => {
    element.style.display = isVisible ? "" : "none";
  });
}

// Function for initializing the visibility based on the status of the switches
function initializeVisibilityBasedOnSwitches() {
  document
    .querySelectorAll('input[type="checkbox"][role="switch"]')
    .forEach(function (switchElement) {
      // Evaluate the status of the switch and adjust visibility
      toggleElementVisibility(
        switchElement.getAttribute("hideOpt"),
        switchElement.checked
      );
    });
}

// OTA: function is called when the ota-file is selected
function ota_sub_fun(obj) {
  var a = obj.value;
  console.log(a);
  var fileName = a.replace(/^.*[\\\/]/, "");
  console.log(fileName);
  document.getElementById("ota_file_input").textContent = fileName;
  document.getElementById("ota_update_btn").disabled = false;
  document.getElementById("ota_progress_bar").style.display = "block";
  document.getElementById("ota_status_txt").style.display = "block";
}

// CONFIG-FORM: function for download config.json file
function exportConfig() {
  var a = document.createElement("a");
  a.href = "/config-download";
  a.download = "config.json";
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
}

// CONFIG-FORM: function to activate import button and show status
function file_sub_fun(obj) {
  document.getElementById("file_upload_btn").disabled = false;
  document.getElementById("upload_status_txt").style.display = "block";
}

// localization of web texts
function localizePage(lang = "en") {
  document.querySelectorAll("[data-i18n]").forEach((elem) => {
    const i18nValue = elem.getAttribute("data-i18n");
    const [translationPart, addon] = i18nValue.split("++", 2);
    const matches = translationPart.split(/(\$.+?\$)/).filter(Boolean);
    let text = "";

    for (const match of matches) {
      if (match.startsWith("$") && match.endsWith("$")) {
        text += match.slice(1, -1);
      } else {
        // check if translation key is valid
        if (translations.hasOwnProperty(match)) {
          text += translations[match][lang] || match;
        } else {
          console.error(`translation key "${match}" not found`);
          continue;
        }
      }
    }
    if (addon) {
      text += addon;
    }
    elem.innerText = text;
  });
}

// to show reload bar if connection is lost
function showReloadBar() {
  document.getElementById("connectionLostBar").style.display = "flex";
}

// to hide reload bar if connection is lost
function hideReloadBar() {
  document.getElementById("connectionLostBar").style.display = "none";
}

function validateHex(input) {
  // Entferne alles außer gültigen HEX-Zeichen und '0x', und stelle sicher, dass Großbuchstaben verwendet werden
  input.value = input.value.replace(/[^0-9a-fA-F]/g, "").toLowerCase();
}

function formatHex(input) {
  const length = parseInt(input.getAttribute("data-length"), 10) || 6; // Standardlänge: 6
  // Auffüllen mit führenden Nullen entsprechend der gewünschten Länge
  input.value = input.value.padStart(length, "0").toLowerCase();
}

function validateBin(input) {
  // Entferne alles außer 0 und 1
  input.value = input.value.replace(/[^0-1]/g, "");
}

function formatBin(input) {
  const length = parseInt(input.getAttribute("data-length"), 10) || 16; // Standardlänge: 16
  input.value = input.value.padStart(length, "0");
}

function validateIP(input) {
  // IPv4-Validierung
  const ipPattern =
    /^((25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)$/;
  if (!ipPattern.test(input.value.trim())) {
    input.setAttribute("aria-invalid", "true");
  } else {
    input.setAttribute("aria-invalid", "false");
    sendData(input.id, input.value);
  }
}

// update elements based on config.json file
function updateUI(config, prefix = "cfg", ignoreKeys = ["version"]) {
  console.log("updating config");
  for (const key in config) {
    if (config.hasOwnProperty(key)) {
      // Prüfen, ob der aktuelle Schlüssel ignoriert werden soll
      if (ignoreKeys.includes(key)) {
        console.log("config update - key ignored: " + key);
        continue;
      }

      const value = config[key];
      const elementId = `${prefix}_${key}`;

      // Überprüfen, ob das aktuelle Element ein verschachteltes Objekt ist
      if (typeof value === "object" && value !== null) {
        // Rekursiv weiter ins Objekt gehen, mit dem erweiterten Prefix
        updateUI(value, elementId, ignoreKeys);
      } else {
        // UI-Element mit dem zusammengesetzten ID suchen
        const element = document.getElementById(elementId);
        if (element) {
          // Unterscheide die Art des HTML-Elements
          if (element.type === "checkbox") {
            // Setze das "checked"-Attribut für Checkboxen
            element.checked = value === true;
            toggleElementVisibility(
              element.getAttribute("hideOpt"),
              element.checked
            );
          } else if (element.type === "radio") {
            // Setze das "checked"-Attribut für Radio-Buttons
            element.checked = element.value === value.toString();
          } else if (element.tagName === "SELECT") {
            // Setze den "value"-Attribut für <select>-Elemente
            element.value = value;

            // Rufe toggleTimeInputs nur auf, wenn data-toggle="timeInputs" gesetzt ist
            if (element.dataset.toggle === "timeInputs") {
              toggleTimeInputs(element);
            }
          } else if (element.type === "password") {
            // Always set password fields to "XxXxXxXxXxX" as a placeholder
            element.value = "XxXxXxXxXxX";
          } else {
            // Überprüfen, ob das Feld ein HEX- oder Binärwert benötigt
            if (element.dataset.type === "hex" && typeof value === "number") {
              // Konvertiere Zahl zu HEX und setze den Wert
              element.value = value.toString(16).toLowerCase();
              formatHex(element);
            } else if (
              element.dataset.type === "bin" &&
              typeof value === "number"
            ) {
              // Konvertiere Zahl zu Binär und setze den Wert
              element.value = value.toString(2);
              formatBin(element);
            } else {
              // Setze den "value"-Attribut für andere Eingabetypen (z.B. text, number)
              element.value = value;
            }
          }
        } else {
          console.error("config update - elementId not found: " + elementId);
        }
      }
    }
  }
  synchronizeDataSyncFields();
}

// load and update config
async function loadConfig() {
  console.log("loading config");
  try {
    const response = await fetch("config.json");
    if (!response.ok)
      throw new Error("Fehler beim Abrufen der Konfigurationsdaten");

    const config = await response.json();

    // update UI-Elementes based on config.json
    updateUI(config);
  } catch (error) {
    console.error("Error loading config:", error);
  }
}

function synchronizeDataSyncFields() {
  // find all input fields with data-sync
  const inputs = document.querySelectorAll("input[data-sync]");

  inputs.forEach((inputElement) => {
    // split data-sync IDs by comma
    const syncIds = inputElement.dataset.sync.split(",");

    syncIds.forEach((syncId) => {
      const syncElement = document.getElementById(syncId.trim());

      if (syncElement) {
        // initial synchronization
        syncElement.textContent = inputElement.value;

        // synchronize user inputs
        inputElement.addEventListener("input", (event) => {
          syncElement.textContent = event.target.value;
        });

        // check for programmed changes
        const observer = new MutationObserver(() => {
          syncElement.textContent = inputElement.value;
        });

        observer.observe(inputElement, {
          attributes: true,
          attributeFilter: ["value"],
        });
      }
    });
  });
}

function toggleEdit(button, inputId) {
  const input = document.getElementById(inputId);
  console.log("button pressed");
  if (button.innerText === "Edit") {
    input.disabled = false;
    input.focus();
    button.innerText = "Lock";
    console.log("button edit");
  } else {
    input.disabled = true;
    button.innerText = "Edit";
    console.log("button save");
  }
}

function toggleTimeInputs(selectElement) {
  const matches = selectElement.id.match(/\d+/g);
  const timerId = matches[matches.length - 1];
  const timeInput = document.getElementById(`timeInput${timerId}`);
  const offsetInput = document.getElementById(`offsetInput${timerId}`);

  if (!timeInput || !offsetInput) {
    console.error("Eines der Elemente nicht gefunden!");
    return;
  }
  //  value:0 == fixex Time
  if (selectElement.value === "0") {
    timeInput.style.display = "block";
    offsetInput.style.display = "none";
  } else {
    timeInput.style.display = "none";
    offsetInput.style.display = "block";
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

function isGitHubPages() {
  return window.location.hostname.includes("github.io");
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
  } catch (error) {
    console.error("Fehler beim Laden von sim.json:", error);
  }
}

// --------------------------------------
// --------------------------------------
document.addEventListener("DOMContentLoaded", function () {
  // call functions on refresh
  setupWS();
  loadSimulatedData();
  synchronizeDataSyncFields();
  initializeVisibilityBasedOnSwitches();
  localizePage("de");
  loadConfig();
  setupBitmaskDialog();

  // Event Listener for Reload-Button
  document
    .getElementById("p99_reloadButton")
    .addEventListener("click", function () {
      window.location.reload();
    });

  // VERSION: is called when version dialog is opened
  document.getElementById("p00_version").addEventListener("click", function () {
    document.getElementById("version_dialog").showModal();
    sendData("check_git_version", "");
  });

  // VERSION: is called when github ota button is clicked
  document
    .getElementById("p11_check_git_btn")
    .addEventListener("click", function () {
      document.getElementById("version_dialog").showModal();
    });

  // VERSION: close version dialog on button click
  document
    .getElementById("close_version_Dialog_btn")
    .addEventListener("click", function () {
      document.getElementById("version_dialog").close();
    });

  // OTA: close ota-failed dialog on button click
  document
    .getElementById("p00_ota_failed_btn")
    .addEventListener("click", function () {
      document.getElementById("ota_update_failed_dialog").close();
    });

  // OTA: send form data to server
  document
    .getElementById("ota_upload_form")
    .addEventListener("submit", function (event) {
      event.preventDefault();
      document.getElementById("ota_status_txt").textContent = "Uploading...";
      var form = document.getElementById("ota_upload_form");
      var formData = new FormData(form);
      var xhr = new XMLHttpRequest();
      xhr.open("POST", "/update", true);
      xhr.send(formData);
    });

  // CONFIG: send form data for config file upload
  document
    .getElementById("file_upload_form")
    .addEventListener("submit", function (event) {
      event.preventDefault();
      var form = document.getElementById("file_upload_form");
      var formData = new FormData(form);
      var xhr = new XMLHttpRequest();
      xhr.open("POST", "/config-upload", true);
      xhr.send(formData);
    });

  // CONFIG: open dialog to show config.json
  document
    .getElementById("p11_file_show_btn")
    .addEventListener("click", function () {
      const dialog = document.getElementById("open_config_dialog");
      dialog.showModal();
      fetch("/config.json")
        .then((response) => response.json())
        .then((data) => {
          document.getElementById("config_output").textContent = JSON.stringify(
            data,
            null,
            2
          );
        })
        .catch((error) => console.error("error loading data:", error));
    });

  // CONFIG: close dialog to show config.json
  document
    .getElementById("p11_config_dialog_btn")
    .addEventListener("click", function () {
      document.getElementById("open_config_dialog").close();
    });

  // NTP: open dialog to show ntp timezones
  document
    .getElementById("p12_ntp_open_dialog_btn")
    .addEventListener("click", function () {
      fetch("/gzip_ntp")
        .then((response) => response.text())
        .then((data) => {
          document.getElementById("p12_ntp_help_output").innerHTML = data;
          document.getElementById("p12_ntp_dialog").showModal();
        })
        .catch((error) => console.error("error loading data:", error));
    });

  // NTP: close dialog to show ntp timezones
  document
    .getElementById("p12_ntp_close_dialog_btn")
    .addEventListener("click", function () {
      document.getElementById("p12_ntp_dialog").close();
    });

  // control for Tab-Menu
  document.querySelectorAll(".nav-list a").forEach((tab) => {
    tab.onclick = function (e) {
      e.preventDefault();
      document
        .querySelectorAll(".nav-list a")
        .forEach((t) => t.classList.remove("active"));
      document
        .querySelectorAll(".tab-content")
        .forEach((content) => content.classList.remove("active"));

      const activeTab = this.getAttribute("data-tab");
      this.classList.add("active");
      document.getElementById(activeTab).classList.add("active");
    };
  });

  // language selection
  document.getElementById("cfg_lang").addEventListener("change", function () {
    var languageValue = this.value;
    if (languageValue === "1") {
      localizePage("en");
    } else if (languageValue === "0") {
      localizePage("de");
    }
  });

  // event listener for all input fields that call sendData on "blur"
  document
    .querySelectorAll(
      'input[type="text"], input[type="number"], input[type="password"], input[type="date"], input[type="time"]'
    )
    .forEach(function (input) {
      input.addEventListener("blur", function () {
        sendData(input.id, input.value);
      });
      input.addEventListener("keypress", function (e) {
        if (e.key === "Enter") {
          input.blur(); // Triggers "blur" event and sends data
        }
      });
    });

  // Event-Listener for Range Inputs (Slider)
  document.querySelectorAll('input[type="range"]').forEach(function (slider) {
    slider.addEventListener("change", function () {
      sendData(slider.id, slider.value);
    });
  });

  // Event-Listener for Slider labels
  document.querySelectorAll(".rangeSlider").forEach((slider) => {
    const valueId = slider.getAttribute("data-value-id");
    const valueDisplay = document.getElementById(valueId);
    slider.oninput = () => {
      valueDisplay.textContent = slider.value;
    };
    // set initial value
    valueDisplay.textContent = slider.value;
  });

  // Event-Listener for Buttons
  document.querySelectorAll("button").forEach(function (button) {
    button.addEventListener("click", function () {
      sendData(button.id, true);
    });
  });

  // Event-Listener for Switches
  document
    .querySelectorAll('input[type="checkbox"][role="switch"]')
    .forEach(function (switchElement) {
      switchElement.addEventListener("change", function () {
        sendData(switchElement.id, switchElement.checked);
        toggleElementVisibility(
          switchElement.getAttribute("hideOpt"),
          switchElement.checked
        );
      });
    });

  // Event-Listener for Radio
  document
    .querySelectorAll('input[type="radio"]')
    .forEach(function (switchElement) {
      switchElement.addEventListener("change", function () {
        sendData(switchElement.id, switchElement.checked);
      });
    });

  // Event-Listener for Checkbox
  document
    .querySelectorAll('input[type="checkbox"]')
    .forEach(function (switchElement) {
      switchElement.addEventListener("change", function () {
        sendData(switchElement.id, switchElement.checked);
      });
    });

  // Event-Listener for Select Elements
  document.querySelectorAll("select").forEach(function (selectElement) {
    selectElement.addEventListener("change", function () {
      sendData(selectElement.id, selectElement.value);
    });
  });
});

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
    de: "letztes Release",
    en: "latest release",
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
    de: "abbrechen",
    en: "cancel",
  },
  check_github: {
    de: "Prüfe GitHub OTA Update",
    en: "Check for GitHub OTA Update",
  },
};


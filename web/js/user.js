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

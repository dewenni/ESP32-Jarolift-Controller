document.addEventListener("DOMContentLoaded", function () {
  // call functions on refresh
  loadSimulatedData();
  setupBitmaskDialog();
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

document.addEventListener("DOMContentLoaded", () => {
  const container = document.getElementById("timers-container");
  const template = document.getElementById("timer-template").content;

  const MAX_TIMERS = 6;
  for (let i = 1; i <= MAX_TIMERS; i++) {
    // Klone das Template
    const timerClone = document.importNode(template, true);
    const timerElement = timerClone.querySelector(".timer");

    // Ersetze Platzhalter im Template
    timerClone.querySelectorAll("*").forEach((el) => {
      // Für jedes Element alle Attribute durchgehen
      const attrNames = el.getAttributeNames(); // Array z. B. ["id", "hideOpt", ...]

      attrNames.forEach((attr) => {
        const val = el.getAttribute(attr);
        // Falls der Wert '${i}' enthält, ersetzen
        if (val && val.includes("${i}")) {
          el.setAttribute(attr, val.replace(/\$\{i\}/g, i));
        }
      });
    });

    // Falls du zusätzlich Textinhalte ersetzen möchtest, bleibt das separat
    timerClone.querySelectorAll(".dash-header").forEach((el) => {
      if (el.textContent.includes("${i}")) {
        el.textContent = el.textContent.replace(/\$\{i\}/g, i);
      }
    });

    // Füge den Timer in den Container ein
    container.appendChild(timerClone);
  }
});

function toggleTimeInputs(selectElement) {
  const matches = selectElement.id.match(/\d+/g);
  const timerId = matches[matches.length - 1];
  const timeInput = document.getElementById(`timeInput${timerId}`);
  const offsetInput = document.getElementById(`offsetInput${timerId}`);

  if (!timeInput || !offsetInput) {
    console.error("Eines der Elemente nicht gefunden!");
    return;
  }

  if (selectElement.value === "fixed") {
    timeInput.style.display = "block";
    offsetInput.style.display = "none";
  } else {
    timeInput.style.display = "none";
    offsetInput.style.display = "block";
  }
}

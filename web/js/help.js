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

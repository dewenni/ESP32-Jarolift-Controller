import re
import logging
import sys
from collections import Counter
import os

from SCons.Script import DefaultEnvironment
    
def main(web_dir, output_dir):
    
    env = DefaultEnvironment()
    project_dir = env["PROJECT_DIR"]

    user_output_dir = project_dir + '/' + web_dir +'/' + output_dir

    # Logger konfigurieren
    logging.basicConfig(level=logging.DEBUG, format='%(levelname)s: %(message)s')

    # Pfade zur JS- und HTML-Datei
    js_file_path_lib = user_output_dir + '/lib.js'
    js_file_path_user = user_output_dir + '/user.js'
    html_file_path = user_output_dir + '/index.html'


    def extract_keys_from_js(file_path, array_name):
        """
        Liest die angegebene JS-Datei ein, sucht nach der Definition des
        gegebenen Arrays (z.B. "const lib_translations = { ... };") und extrahiert alle Schlüssel.
        """
        with open(file_path, 'r', encoding='utf-8') as js_file:
            content = js_file.read()
        
        # Suche nach der Array-/Objekt-Definition
        pattern_array = r'const\s+' + re.escape(array_name) + r'\s*=\s*({.*?});'
        match = re.search(pattern_array, content, re.DOTALL)
        if not match:
            logging.error(f"{array_name} nicht in {file_path} gefunden.")
            sys.exit(1)  # Direkt beenden, wenn das Array nicht gefunden wird.
            return []
    
        object_content = match.group(1)
        # Regex: Erfasst Schlüssel, die auf einen Doppelpunkt und eine öffnende Klammer folgen
        key_pattern = r'\b(\w+):\s*{'
        keys = re.findall(key_pattern, object_content)
        return keys


    # 1. Schlüssel aus den beiden JS-Dateien extrahieren – jeweils aus dem entsprechenden Array
    js_keys_lib = extract_keys_from_js(js_file_path_lib, "lib_translations")
    js_keys_user = extract_keys_from_js(js_file_path_user, "user_translations")


    # Beide Listen zusammenführen
    js_keys = js_keys_lib + js_keys_user
    js_keys_set = set(js_keys)  # In eine Menge umwandeln für schnellen Vergleich

    # Zähle die Vorkommen der Schlüssel, um doppelte Einträge zu finden
    key_counts = Counter(js_keys)
    duplicates = {key: count for key, count in key_counts.items() if count > 1}

    if duplicates:
        logging.warning("Doppelte Einträge gefunden:")
        for key, count in duplicates.items():
            logging.warning(f"'{key}' kommt {count} mal vor")
    #else:
        #logging.info("Keine doppelten Einträge in lang.js gefunden.")

    # 3. Index.html auslesen
    with open(html_file_path, 'r', encoding='utf-8') as html_file:
        html_content = html_file.read()

    # 4. Alle data-i18n-Tags in der HTML-Datei extrahieren und in Tags aufteilen
    html_pattern = r'data-i18n="([^"]+)"'
    html_tags = re.findall(html_pattern, html_content)

    # 5. Zerlege die data-i18n-Werte in einzelne Teile basierend auf den festgelegten Regeln
    html_keys = set()
    for tag in html_tags:
        # Zerlege nach den Trennzeichen: $, ignoriere alles nach "++"
        parts = re.split(r'\$|\+\+', tag)
        for part in parts:
            # Füge nur gültige Tags hinzu (keine Zahlen, keine Sonderzeichen nach $ oder ++)
            part = part.strip()
            if part and not part.isdigit() and '-' not in part:
                html_keys.add(part)

    # 6. Überprüfen, ob die JS-Schlüssel in der HTML-Datei verwendet werden
    unused_js_keys = js_keys_set - html_keys  # JS-Schlüssel, die in der HTML nicht verwendet werden
    if unused_js_keys:
        for key in unused_js_keys:
            logging.warning(f"Die folgenden Schlüssel aus lang.js werden in der HTML-Datei nicht verwendet: '{key}'")
    #else:
        # logging.info("Alle Schlüssel aus lang.js werden in der HTML-Datei verwendet.")

    # 7. Überprüfen, ob in der HTML verwendete Schlüssel nicht in der lang.js-Datei definiert sind
    unused_html_keys = html_keys - js_keys_set  # HTML-Schlüssel, die in lang.js nicht definiert sind
    if unused_html_keys:
        for key in unused_html_keys:
            logging.error(f"Die folgenden Schlüssel werden in der HTML-Datei verwendet, sind aber nicht in lang.js definiert: '{key}'")
        sys.exit(1)  # Fehler gefunden, Skript mit Fehlercode 1 beenden
    #else:
        # logging.info("Alle in der HTML-Datei verwendeten Schlüssel sind in lang.js definiert.")

if __name__ == '__main__':
    main()
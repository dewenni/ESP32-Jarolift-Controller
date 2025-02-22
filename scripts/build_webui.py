import os
import re
import gzip

def ensure_directory_exists(path):
    if not os.path.exists(path):
        os.makedirs(path)
        print(f"folder '{path}' was created.")

# check if temp folder exists, otherwise create it
ensure_directory_exists("web/ghpages") 

def compress_to_gzip_c_array(input_file_path, output_file_path, var_name):
    # HTML-Datei einlesen
    with open(input_file_path, 'rb') as file:
        content = file.read()
    
    # Inhalt mit Brotli komprimieren
    compressed_content = gzip.compress(content)
    
    # Komprimierten Inhalt in ein C-Array umwandeln
    c_array_content = ', '.join(['0x{:02x}'.format(byte) for byte in compressed_content])
    
    # C-Array in eine neue Datei schreiben
    with open(output_file_path, 'w') as file:
        file.write('const uint8_t PROGMEM ' + var_name + '[] = {' + c_array_content + '};\n')
        file.write(f'const unsigned int {var_name}_size = {len(compressed_content)};\n')

def convert_to_c_array(input_file_path, output_file_path, var_name):
    # HTML-Datei einlesen
    with open(input_file_path, 'rb') as file:
        content = file.read()
      
    # Inhalt in ein C-Array umwandeln
    c_array_content = ', '.join(['0x{:02x}'.format(byte) for byte in content])
    
    # C-Array in eine neue Datei schreiben
    with open(output_file_path, 'w') as file:
        file.write('const uint8_t PROGMEM ' + var_name + '[] = {' + c_array_content + '};\n')
        file.write(f'const unsigned int {var_name}_size = {len(content)};\n')

def convert_to_c_literal(input_file_path, output_file_path, var_name):
    # HTML-Datei einlesen
    with open(input_file_path, 'r', encoding='utf-8') as file:
        content = file.read()
    
    # Inhalt als C-String formatieren (mit raw literal syntax für mehrzeilige Strings)
    c_literal_content = f'const uint8_t {var_name}[] PROGMEM = R"rawliteral({content})rawliteral";\n'
    c_literal_size = f'const unsigned int {var_name}_size = {len(content)};\n'
    
    # C-String in eine neue Datei schreiben
    with open(output_file_path, 'w', encoding='utf-8') as file:
        file.write(c_literal_content)
        file.write(c_literal_size)



#================================================================
# HTML - gzip
#================================================================
# Definieren Sie die Pfade der Quelldateien
source_files = [
    'web/html/00_header.html',
    'web/html/01_control.html',
    'web/html/02_group.html',
    'web/html/03_timer.html',
    'web/html/09_system.html',
    'web/html/10_logger.html',
    'web/html/11_ota.html',
    'web/html/12_settings.html',
    'web/html/13_help.html',
    'web/html/99_footer.html'
]

# Zielpfad der generierten Datei
output_file_html = 'web/ghpages/index.html'
output_file_br = 'include/gzip_m_html.h'

combined_html = ''

# Regex-Muster für den Abschnittsstart und -ende
section_start = re.compile(r'<!--SECTION-START-->')
section_end = re.compile(r'<!--SECTION-END-->')

# Durchlaufen Sie jede Datei und extrahieren Sie den gewünschten Inhalt
for file_path in source_files:
    with open(file_path, 'r') as file:
        file_content = file.read()
        # Suche nach dem Abschnitt zwischen den Markierungen
        section_contents = re.findall(f'(?s){section_start.pattern}(.*?){section_end.pattern}', file_content)
        # Füge nur den Inhalt zwischen den Markierungen hinzu, falls vorhanden
        for content in section_contents:
            combined_html += content.strip() + '\n'

# Schreiben Sie die kombinierte Definition in die Ausgabedatei
with open(output_file_html, 'w') as file:
    file.write(combined_html)

compress_to_gzip_c_array(output_file_html, output_file_br, 'gzip_main_html')

# ---------------------------------------------------------------

# Pfad zur HTML-Datei und zum Ausgabe-C-Datei
input_html_file = 'web/html/login.html'
output_c_file = 'include/gzip_login_html.h'
compress_to_gzip_c_array(input_html_file, output_c_file, 'gzip_login_html')

# ---------------------------------------------------------------

# Pfad zur HTML-Datei und zum Ausgabe-C-Datei
input_html_file = 'web/html/ntp.html'
output_c_file = 'include/gzip_ntp_html.h'
compress_to_gzip_c_array(input_html_file, output_c_file, 'gzip_ntp_html')


#================================================================
# CSS
#================================================================
# Definieren Sie die Pfade der Quelldateien
source_files = [
    'web/css/pico.css',
    'web/css/custom.css',
    'web/css/icons.css'
]

# Zielpfad der generierten Datei
output_file_css = 'web/ghpages/main.css'
output_c_file = 'include/gzip_css.h'

combined_content = ''

# Durchlaufen Sie jede Datei und fügen Sie ihren Inhalt hinzu
for file_path in source_files:
    with open(file_path, 'r') as file:
        combined_content += file.read() + '\n'

# Schreiben Sie die kombinierte Definition in die Ausgabedatei
with open(output_file_css, 'w') as file:
    file.write(combined_content)

compress_to_gzip_c_array(output_file_css, output_c_file, 'gzip_css')

#================================================================
# JS
#================================================================
# Definieren Sie die Pfade der Quelldateien
source_files = [
    'web/js/fun.js',
    'web/js/doc.js',
    'web/js/lang.js',
]

# Zielpfad der generierten Datei
output_file_js = 'web/ghpages/main.js'
output_file_br_js = 'include/gzip_js.h'

combined_content = ''

# Durchlaufen Sie jede Datei und fügen Sie ihren Inhalt hinzu
for file_path in source_files:
    with open(file_path, 'r') as file:
        combined_content += file.read() + '\n'

# Schreiben Sie die kombinierte Definition in die Ausgabedatei
with open(output_file_js, 'w') as file:
    file.write(combined_content)

compress_to_gzip_c_array(output_file_js, output_file_br_js, 'gzip_js')
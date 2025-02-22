import build_webui
import checkText

web_dir = 'web'
output_dir = 'output'

build_webui.main(web_dir, output_dir)
checkText.main(web_dir, output_dir)
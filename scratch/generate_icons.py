import os

repo_dir = r'F:\CCad\assets\material-ui-repo\packages\mui-icons-material\material-icons'
icons = {
    'attach_file.svg': 'icon_attach',
    'menu.svg': 'icon_menu',
    'pie_chart.svg': 'icon_context',
    'mic.svg': 'icon_mic',
    'arrow_forward.svg': 'icon_send',
    'settings.svg': 'icon_settings',
    'close.svg': 'icon_close',
    'add.svg': 'icon_templates'
}

out = '#pragma once\n#include <QString>\n\nnamespace ccad_icons {\n'

for f, name in icons.items():
    try:
        path = os.path.join(repo_dir, f)
        with open(path, 'r', encoding='utf-8') as file:
            content = file.read().replace('\"', '\\\"').replace('\n', '')
            # Fill color should be white/grey for our dark theme
            content = content.replace('<svg ', '<svg fill=\"#8b949e\" ')
            out += f'const QString {name} = "{content}";\n'
    except Exception as e:
        print(f'Error reading {f}: {e}')

out += '}\n'

with open(r'F:\CCad\src\ccad_gui\agent_icons.hpp', 'w', encoding='utf-8') as file:
    file.write(out)

print('Generated agent_icons.hpp')

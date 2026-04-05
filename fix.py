import os

def replace_in_file(path):
    with open(path, 'r') as f:
        content = f.read()
    content = content.replace('m_css.', 'css().')
    with open(path, 'w') as f:
        f.write(content)

replace_in_file('/home/lxk/Desktop/lite-html/litehtml/src/html_tag.cpp')
replace_in_file('/home/lxk/Desktop/lite-html/litehtml/src/el_text.cpp')

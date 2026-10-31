"""PO authoring and source extraction for the FMOD localization editor.

Python and Babel are used only by authoring tools, never by the addon loader.
"""
from __future__ import annotations

import argparse
import hashlib
import io
import json
from pathlib import Path
import re
import sys

RESULT_PATH = None


def source_entries(root: Path) -> list[dict]:
    result: dict[tuple[str, str], dict] = {}
    literal = r'"(?:\\.|[^"\\])*"'
    patterns = [r'(?<!\w)(?:fmod_editor_translate|FMOD_EDITOR_TEXT|tr|_text|_error)\(\s*((?:' + literal + r'\s*)+)',
                r'fmod_set_editor_tooltip\(this,\s*((?:' + literal + r'\s*)+)']
    for directory, extension in [('src/editor', '*.cpp'), ('addons/fmod_player', '*.gd')]:
        for file in sorted((root / directory).rglob(extension)):
            if file.name == 'fmod_property_translations.gd':
                continue
            text = file.read_text(encoding='utf-8')
            for pattern in patterns:
                for match in re.finditer(pattern, text):
                    source = ''.join(json.loads(part) for part in re.findall(literal, match[1]))
                    if not source:
                        continue
                    context_match = re.match(r',\s*(' + literal + ')', text[match.end():])
                    context = json.loads(context_match[1]) if context_match else ''
                    key = (source, context)
                    row = result.setdefault(key, {'source': source, 'context': context, 'locations': []})
                    row['locations'].append(file.relative_to(root).as_posix() + ':' + str(text[:match.start()].count('\n') + 1))
            if file.name == 'fmod_audio_importer.cpp':
                for match in re.finditer(r'PROPERTY_HINT_(?:ENUM|FLAGS),\s*(' + literal + ')', text):
                    for source in json.loads(match[1]).split(','):
                        if source:
                            row = result.setdefault((source, ''), {'source': source, 'context': '', 'locations': []})
                            row['locations'].append(file.relative_to(root).as_posix())
    scene_property = re.compile(r'(?m)^(text|title|dialog_text|ok_button_text|placeholder_text|tooltip_text|popup/item_\d+/text) = (' + literal + r')\s*$')
    for file in sorted((root / 'addons/fmod_player').rglob('*.tscn')):
        text = file.read_text(encoding='utf-8')
        for node in re.finditer(r'(?ms)^\[node [^\n]+\]\n(.*?)(?=^\[|\Z)', text):
            untranslated = re.search(r'(?m)^auto_translate_mode = 2$', node[1])
            for match in scene_property.finditer(node[1]):
                if untranslated and match[1] == 'text':
                    continue
                source = json.loads(match[2])
                if source:
                    row = result.setdefault((source, ''), {'source': source, 'context': '', 'locations': []})
                    line = text[:node.start(1) + match.start()].count('\n') + 1
                    row['locations'].append(file.relative_to(root).as_posix() + ':' + str(line))
    return sorted(result.values(), key=lambda row: row['source'])


def main() -> dict:
    global RESULT_PATH
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['po-read', 'po-write', 'scan', 'extract', 'check'])
    parser.add_argument('--path', type=Path)
    parser.add_argument('--input', type=Path)
    parser.add_argument('--expected')
    parser.add_argument('--root', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    RESULT_PATH = args.output
    if args.action == 'scan':
        if not args.root or not (args.root / 'src/editor').is_dir():
            raise ValueError('Select the godot-fmodplayer source repository.')
        return {'entries': source_entries(args.root)}
    from babel.messages.pofile import read_po, write_po
    if args.action in ['extract', 'check']:
        if not args.root or not (args.root / 'tools/documentation_translations.py').is_file():
            raise ValueError('Select the godot-fmodplayer source repository.')
        sys.path.insert(0, str(args.root / 'tools'))
        import documentation_translations as docs
        messages = docs.source_messages(args.root / 'doc_classes')
        path = args.path or args.root / 'addons/fmod_player/translations/documentation/zh_CN.po'
        catalog = docs.load_catalog(path)
        if args.action == 'extract':
            catalog.update(docs.template(messages))
            docs.save_catalog(catalog, path)
        return docs.validate(messages, catalog)
    with args.path.open(encoding='utf-8') as stream:
        catalog = read_po(stream, abort_invalid=True)
    if args.action == 'po-write':
        if hashlib.sha256(args.path.read_bytes()).hexdigest() != args.expected:
            raise ValueError('The file changed on disk. Reload it before saving.')
        payload = json.loads(args.input.read_text(encoding='utf-8'))
        keys = {(entry.id, entry.context or '') for entry in catalog if entry.id}
        if keys != {(entry['source'], entry.get('context', '')) for entry in payload}:
            raise ValueError('PO source keys and contexts must remain unchanged.')
        for entry in payload:
            message = catalog.get(entry['source'], context=entry.get('context') or None)
            message.string = entry['translation']
            message.flags.discard('fuzzy')
            if entry.get('fuzzy'):
                message.flags.add('fuzzy')
        if args.root and (args.root / 'tools/documentation_translations.py').is_file():
            sys.path.insert(0, str(args.root / 'tools'))
            import documentation_translations as docs
            docs.validate(docs.source_messages(args.root / 'doc_classes'), catalog)
        output = io.BytesIO()
        write_po(output, catalog, width=100, sort_output=True, include_previous=True)
        temporary = args.path.with_suffix(args.path.suffix + '.tmp')
        temporary.write_bytes(output.getvalue().rstrip() + b'\n')
        temporary.replace(args.path)
    return {
        'locale': str(catalog.locale),
        'sha256': hashlib.sha256(args.path.read_bytes()).hexdigest(),
        'entries': [{'source': entry.id, 'context': entry.context or '', 'translation': entry.string,
                     'fuzzy': entry.fuzzy, 'locations': [*entry.auto_comments,
                         *(f'{path}:{line}' for path, line in entry.locations)]}
                    for entry in catalog if entry.id],
    }


if __name__ == '__main__':
    exit_code = 0
    try:
        payload = {'ok': True, 'data': main()}
    except Exception as error:
        payload = {'ok': False, 'error': str(error)}
        exit_code = 1
    serialized = json.dumps(payload, ensure_ascii=False)
    if RESULT_PATH:
        RESULT_PATH.write_text(serialized, encoding='utf-8', newline='\n')
    else:
        print(serialized)
    sys.exit(exit_code)

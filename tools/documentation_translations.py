"""Extract, validate and apply Godot XML class-reference translations.

Requires Babel for PO files: python -m pip install -r tools/requirements-docs.txt.
This module is copied to the documentation repository by sync_classref.py.
"""
from __future__ import annotations

import argparse
from collections import Counter
from copy import deepcopy
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET

from babel.messages.catalog import Catalog
from babel.messages.pofile import read_po, write_po

TEMPLATE_MESSAGES = (
    'Description', 'Tutorials', 'Properties', 'Constructors', 'Methods', 'Operators',
    'Theme Properties', 'Signals', 'Enumerations', 'Constants', 'Annotations',
    'Property Descriptions', 'Constructor Descriptions', 'Method Descriptions',
    'Operator Descriptions', 'Theme Property Descriptions', 'Inherits:', 'Inherited By:',
    '(overrides %s)', 'Default', 'Setter', 'value', 'Getter', 'Deprecated:', 'Experimental:',
    "This method should typically be overridden by the user to have any effect.",
    "This method is required to be overridden when extending its base class.",
    "This method has no side effects. It doesn't modify any of the instance's member variables.",
    "This method accepts any number of arguments after the ones described here.",
    "This method is used to construct a type.",
    "This method doesn't need an instance to be called, so it can be called directly using the class name.",
    "This method describes a valid operator to use with this type as left-hand operand.",
    "This value is an integer composed as a bitmask of the following flags.",
    'No return value.',
)


def normalize(text: str) -> str:
    """Match String::dedent().strip_edges(), including partially indented lines."""
    lines = text.replace('\r\n', '\n').split('\n')
    indent = next((re.match(r'[\x00-\x20]*', line).group() for line in lines
                   if any(ord(char) > 32 for char in line)), '')
    result = []
    for line in lines:
        if not any(ord(char) > 32 for char in line):
            result.append('')
            continue
        offset = 0
        while offset < min(len(line), len(indent)) and line[offset] == indent[offset]:
            offset += 1
        result.append(line[offset:])
    return '\n'.join(result).strip(''.join(chr(value) for value in range(33)))


def fields(tree: ET.Element):
    """Yield descriptions and translatable attributes without touching signatures."""
    def visit(node, path):
        if node.tag in {'brief_description', 'description', 'member', 'constant', 'theme_item'}:
            if node.text and normalize(node.text):
                yield node, None, path, normalize(node.text)
        for attribute in ('deprecated', 'experimental'):
            if node.get(attribute):
                yield node, attribute, path + '@' + attribute, normalize(node.get(attribute))
        if node.tag == 'link' and node.get('title'):
            yield node, 'title', path + '@title', normalize(node.get('title'))
        for index, child in enumerate(node):
            identifier = child.get('name', str(index))
            yield from visit(child, path + '/' + child.tag + ':' + identifier)
    yield from visit(tree, tree.get('name'))


def source_messages(xml_dir: Path) -> dict[str, list[str]]:
    messages: dict[str, list[str]] = {message: ['Class reference template'] for message in TEMPLATE_MESSAGES}
    files = sorted(xml_dir.glob('*.xml'))
    if not files:
        raise ValueError('No class XML files found in ' + str(xml_dir))
    for file in files:
        for _, _, location, message in fields(ET.parse(file).getroot()):
            messages.setdefault(message, []).append(location)
    return messages


def load_catalog(path: Path) -> Catalog:
    with path.open(encoding='utf-8') as stream:
        return read_po(stream, abort_invalid=True)


def translations(path: Path) -> dict[str, str]:
    return {message.id: message.string for message in load_catalog(path)
            if message.id and message.string and not message.fuzzy and not message.context}


def save_catalog(catalog: Catalog, path: Path) -> None:
    import io
    output = io.BytesIO()
    write_po(output, catalog, width=100, sort_output=True, include_previous=True, ignore_obsolete=True)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(output.getvalue().rstrip() + b'\n')


def template(messages: dict[str, list[str]]) -> Catalog:
    catalog = Catalog(project='Godot-FmodPlayer API', version='1.0', copyright_holder='LuYingYiLong',
                      msgid_bugs_address='https://github.com/LuYingYiLong/godot-fmodplayer/issues')
    for message, locations in messages.items():
        catalog.add(message, auto_comments=locations)
    return catalog


def protected_tokens(text: str) -> Counter:
    # Brackets inside example code are GDScript/C#, not documentation tags.
    text = re.sub(r'\[(codeblock|gdscript|csharp)(?: [^\]]*)?\].*?\[/\1\]', '', text, flags=re.S)
    tokens = re.findall(r'\[(?:param|method|member|signal|constant|enum|constructor|operator|annotation|theme_item) [^\]]+\]|\[(?:[A-Z][\w.]*|int|float|bool|void)\]', text)
    return Counter(tokens)


def validate(messages: dict[str, list[str]], catalog: Catalog, strict: bool = False) -> dict:
    errors = []
    missing = []
    translated = 0
    for message in catalog:
        if not message.id:
            continue
        if message.context or not isinstance(message.id, str):
            errors.append('Documentation uses singular messages without context: ' + str(message.id))
        elif message.id not in messages:
            errors.append('Stale message: ' + message.id)
    for source, locations in messages.items():
        entry = catalog.get(source)
        if entry is None or not entry.string or entry.fuzzy:
            missing.append(locations[0])
            continue
        translated += 1
        # Repetition may differ in a translation; link and parameter identities must agree.
        if set(protected_tokens(source)) != set(protected_tokens(entry.string)):
            errors.append('Changed documentation references: ' + locations[0])
        for tag in ('codeblock', 'gdscript', 'csharp'):
            opened = len(re.findall(r'\[' + tag + r'(?: [^\]]*)?\]', entry.string))
            closed = entry.string.count('[/' + tag + ']')
            if opened != closed:
                errors.append('Unbalanced code block: ' + locations[0])
        without_code = re.sub(r'\[(codeblock|gdscript|csharp)(?: [^\]]*)?\].*?\[/\1\]', '', entry.string, flags=re.S)
        stack = []
        for tag in re.findall(r'\[(/?(?:b|i|u|code|kbd|url|center|codeblocks))(?:[ =][^\]]*)?\]', without_code):
            if tag.startswith('/'):
                if not stack or stack.pop() != tag[1:]:
                    errors.append('Unbalanced markup: ' + locations[0])
                    break
            else:
                stack.append(tag)
        if stack:
            errors.append('Unclosed markup: ' + locations[0])
        if ':ref:`' in entry.string or '``' in without_code:
            errors.append('RST markup in XML translation: ' + locations[0])
    report = {'messages': len(messages), 'translated': translated, 'missing': missing, 'errors': errors}
    if errors or (strict and missing):
        raise ValueError(json.dumps(report, ensure_ascii=False, indent=2))
    return report


def translated_tree(tree: ET.Element, mapping: dict[str, str]) -> ET.Element:
    result = deepcopy(tree)
    for node, attribute, _, message in fields(result):
        if message not in mapping:
            continue
        if attribute:
            node.set(attribute, mapping[message])
        else:
            node.text = mapping[message]
    return result


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['extract', 'update', 'check'])
    parser.add_argument('--xml-dir', type=Path, default=root / 'doc_classes')
    parser.add_argument('--po', type=Path, default=root / 'addons/fmod_player/translations/documentation/zh_CN.po')
    parser.add_argument('--pot', type=Path, default=root / 'doc_classes/translations/fmod_classes.pot')
    parser.add_argument('--strict', action='store_true')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    messages = source_messages(args.xml_dir)
    if args.action == 'extract':
        catalog = template(messages)
        if args.pot.exists():
            catalog.creation_date = load_catalog(args.pot).creation_date
        save_catalog(catalog, args.pot)
    elif args.action == 'update':
        catalog = load_catalog(args.po) if args.po.exists() else Catalog(locale='zh_CN')
        catalog.update(template(messages))
        save_catalog(catalog, args.po)
    else:
        report = validate(messages, load_catalog(args.po), args.strict)
        if args.report:
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()

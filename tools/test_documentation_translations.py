"""Regression checks for XML translation keys, fallback and safe application."""
from pathlib import Path
import tempfile
import unittest
import xml.etree.ElementTree as ET

from babel.messages.catalog import Catalog
from documentation_translations import (fields, load_catalog, normalize, protected_tokens,
                                        save_catalog, source_messages, translated_tree,
                                        translations, validate)

ROOT = Path(__file__).resolve().parents[1]
TEMP_ROOT = ROOT / 'tests/logs/documentation_translations'
TEMP_ROOT.mkdir(parents=True, exist_ok=True)
assert TEMP_ROOT.resolve().is_relative_to(ROOT / 'tests/logs')


class DocumentationTranslationsTest(unittest.TestCase):
    def test_godot_indentation_preserves_code_and_partial_prefixes(self):
        self.assertEqual(normalize('\n\t\tParagraph\n\t\t[codeblock]\n\t\t\tcode\n\t\t[/codeblock]\n\t'),
                         'Paragraph\n[codeblock]\n\tcode\n[/codeblock]')
        self.assertEqual(normalize('  first\n second\n\tthird\n'), 'first\nsecond\n\tthird')
        self.assertEqual(normalize('first\n  second'), 'first\n  second')

    def test_context_and_changed_reference_are_rejected(self):
        catalog = Catalog(locale='zh_CN')
        catalog.add('Use [param frames].', '使用 [param bytes]。')
        with self.assertRaises(ValueError):
            validate({'Use [param frames].': ['Capture.get_buffer']}, catalog)
        catalog = Catalog(locale='zh_CN')
        catalog.add('Hello', '你好', context='Capture')
        with self.assertRaises(ValueError):
            validate({'Hello': ['Capture']}, catalog)
        catalog = Catalog(locale='zh_CN')
        catalog.add('Example', '[codeblock]\nprint(1)')
        with self.assertRaises(ValueError):
            validate({'Example': ['Capture']}, catalog)

    def test_empty_and_fuzzy_messages_fall_back_without_mutating_xml(self):
        tree = ET.fromstring('<class name="Example"><description>Hello</description><methods><method name="run"><return type="void"/><description>Run</description></method></methods></class>')
        before = ET.tostring(tree)
        catalog = Catalog(locale='zh_CN')
        catalog.add('Hello', '你好', flags=['fuzzy'])
        catalog.add('Run', '')
        with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as directory:
            path = Path(directory) / 'zh_CN.po'
            save_catalog(catalog, path)
            mapping = translations(path)
        self.assertEqual(mapping, {})
        localized = translated_tree(tree, {'Hello': '你好'})
        self.assertEqual(localized.findtext('description'), '你好')
        self.assertEqual(localized.findtext('methods/method/description'), 'Run')
        self.assertEqual(ET.tostring(tree), before)
        self.assertEqual(localized.find('methods/method').get('name'), 'run')

    def test_po_roundtrip_preserves_newlines_quotes_and_markup(self):
        catalog = Catalog(locale='zh_CN')
        source = 'A "quote" and [param value].\n[codeblock]\n\tprint("\\n")\n[/codeblock]'
        target = '引号和 [param value]。\n[codeblock]\n\tprint("\\n")\n[/codeblock]'
        catalog.add(source, target)
        with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as directory:
            path = Path(directory) / 'zh_CN.po'
            save_catalog(catalog, path)
            self.assertEqual(translations(path), {source: target})

    def test_full_catalog_and_application_preserve_api(self):
        catalog = load_catalog(ROOT / 'addons/fmod_player/translations/documentation/zh_CN.po')
        report = validate(source_messages(ROOT / 'doc_classes'), catalog, strict=True)
        self.assertGreater(report['translated'], 1500)
        mapping = {entry.id: entry.string for entry in catalog if entry.id}
        for file in (ROOT / 'doc_classes').glob('*.xml'):
            original = ET.parse(file).getroot()
            localized = translated_tree(original, mapping)
            for tree in (original, localized):
                for node, attribute, _, _ in list(fields(tree)):
                    if attribute:
                        node.set(attribute, '')
                    else:
                        node.text = ''
            self.assertEqual(ET.tostring(original), ET.tostring(localized), file.name)


if __name__ == '__main__':
    unittest.main()

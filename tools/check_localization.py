"""Check the three FMOD translation catalogs and editor source coverage."""
from pathlib import Path
import argparse
import csv
import importlib.util
import json
import re
import sys

sys.dont_write_bytecode = True

from babel import Locale
from documentation_translations import load_catalog, source_messages, validate

ROOT = Path(__file__).resolve().parents[1]
LANGUAGES = ['zh_Hans', 'zh_Hant', 'ja', 'ru']


def placeholders(text: str) -> list[str]:
    return sorted(re.findall(r'%(?:[0-9]+\$)?[-+#0 ]*[0-9]*(?:\.[0-9]+)?([sdifuxX])', text.replace('%%', '')))


def check() -> dict:
    catalogs = {}
    for name in ['property', 'editor']:
        file = ROOT / 'addons/fmod_player/translations' / ('fmod_' + name + '_translations.csv')
        with file.open(encoding='utf-8', newline='') as stream:
            reader = csv.DictReader(stream)
            assert reader.fieldnames == ['source', 'context', *LANGUAGES], file
            messages = {}
            for row in reader:
                key = (row['source'], row['context'])
                assert key[0].strip() and key not in messages, key
                for locale in LANGUAGES:
                    Locale.parse(locale)
                    assert row[locale], (file, key, locale)
                    assert placeholders(key[0]) == placeholders(row[locale]), (file, key, locale)
                messages[key] = row
        catalogs[name] = messages
    spec = importlib.util.spec_from_file_location('fmod_localization_backend', ROOT / 'addons/fmod_player/tools/localization_backend.py')
    backend = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(backend)
    entries = backend.source_entries(ROOT)
    missing = [entry for entry in entries if (entry['source'], entry['context']) not in catalogs['editor']]
    assert not missing, json.dumps(missing, ensure_ascii=False)
    po = ROOT / 'addons/fmod_player/translations/documentation/zh_CN.po'
    documentation = validate(source_messages(ROOT / 'doc_classes'), load_catalog(po), strict=True)
    return {'passed': True, 'languages': LANGUAGES,
            'properties': len(catalogs['property']), 'editor': len(catalogs['editor']),
            'editor_sources_checked': len(entries), 'documentation': documentation}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    report = check()
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(report, ensure_ascii=False))

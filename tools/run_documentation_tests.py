"""Test offline help in an isolated editor project under tests/logs."""
from pathlib import Path
import argparse
import json
import os
import subprocess
import shutil
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--godot', required=True)
    parser.add_argument('--output-dir', required=True, type=Path)
    args = parser.parse_args()
    output = args.output_dir.resolve()
    if not output.is_relative_to(ROOT / 'tests/logs'):
        raise ValueError('Documentation test output must be under tests/logs')
    project = output / 'project'
    project.mkdir(parents=True, exist_ok=True)
    source = ROOT / 'tests/addons/fmod_player'
    if not (source / 'bin/fmod.dll').is_file():
        raise ValueError('Deploy the complete test addon first with tools/run_tests.ps1 -PrepareOnly')
    shutil.copytree(source, project / 'addons/fmod_player', dirs_exist_ok=True)
    editor = project / 'editor'
    editor.mkdir(exist_ok=True)
    shutil.copy2(ROOT / 'tests/editor/documentation_translation_test.gd', editor)
    raw_messages = []
    for file in sorted((ROOT / 'doc_classes').glob('*.xml')):
        root = ET.parse(file).getroot()
        for node in root.iter():
            if node.tag in {'brief_description', 'description', 'member', 'constant', 'theme_item'} and node.text and node.text.strip():
                raw_messages.append({'class': root.get('name'), 'text': node.text})
            for attribute in ('deprecated', 'experimental'):
                if node.get(attribute):
                    raw_messages.append({'class': root.get('name'), 'text': node.get(attribute)})
            if node.tag == 'link' and node.get('title'):
                raw_messages.append({'class': root.get('name'), 'text': node.get('title')})
    (project / 'source_messages.json').write_text(json.dumps(raw_messages, ensure_ascii=False), encoding='utf-8')
    (editor / 'plugin.cfg').write_text('[plugin]\nname="Documentation test"\ndescription="Offline help regression"\nauthor="Godot-FmodPlayer"\nversion="1.0"\nscript="driver.gd"\n', encoding='utf-8')
    (editor / 'driver.gd').write_text('''@tool
extends EditorPlugin


func _enter_tree() -> void:
    _run.call_deferred()
    get_tree().create_timer(90.0).timeout.connect(func() -> void: get_tree().quit(1))


func _run() -> void:
    while EditorInterface.get_resource_filesystem().is_scanning():
        await get_tree().process_frame
    var test: RefCounted = load("res://editor/documentation_translation_test.gd").new()
    var output_dir: String = ProjectSettings.globalize_path("res://..").simplify_path()
    var passed: bool = await test.run(output_dir, get_tree())
    print("TESTS_PASSED" if passed else "TESTS_FAILED")
    get_tree().quit(0 if passed else 1)
''', encoding='utf-8', newline='\n')
    (project / 'project.godot').write_text('''config_version=5

[application]
config/name="FMOD offline help test"

[audio]
fmod/show_startup_banner=false

[editor_plugins]
enabled=PackedStringArray("res://addons/fmod_player/plugin.cfg", "res://editor/plugin.cfg")

[rendering]
renderer/rendering_method="gl_compatibility"
''', encoding='utf-8')
    with (output / 'console.log').open('w', encoding='utf-8') as stream:
        environment = os.environ.copy()
        environment['APPDATA'] = str(output / 'user_data')
        environment['LOCALAPPDATA'] = str(output / 'user_cache')
        result = subprocess.run([args.godot, '--headless', '--editor', '--path', str(project),
                                 '--language', 'zh_Hans', '--log-file', str(output / 'godot.log')],
                                stdout=stream, stderr=subprocess.STDOUT, timeout=120, env=environment)
    log = (output / 'console.log').read_text(encoding='utf-8')
    print(log[-6000:])
    if result.returncode or 'TESTS_PASSED' not in log or 'SCRIPT ERROR:' in log or '\nERROR:' in log:
        raise SystemExit(1)


if __name__ == '__main__':
    main()

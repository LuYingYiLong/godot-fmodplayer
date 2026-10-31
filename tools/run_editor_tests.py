"""Run the editor translation check with the normal editor lifecycle."""
from pathlib import Path
import argparse
import re
import subprocess
import os
import shutil

ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--godot', required=True)
    parser.add_argument('--output-dir', required=True, type=Path)
    parser.add_argument('--isolated', action='store_true', help='Use a clean project under the output directory')
    args = parser.parse_args()
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    project_root = ROOT / 'tests'
    environment = os.environ.copy()
    if args.isolated:
        if not output.is_relative_to(ROOT / 'tests/logs'):
            raise ValueError('Isolated editor tests must be under tests/logs')
        project_root = output / 'project'
        project_root.mkdir(parents=True, exist_ok=True)
        for source in (ROOT / 'tests').iterdir():
            if source.is_file() and source.suffix not in ['.md']:
                shutil.copy2(source, project_root / source.name)
        for directory in ['editor', 'addons', 'assets']:
            shutil.copytree(ROOT / 'tests' / directory, project_root / directory, dirs_exist_ok=True)
        settings = project_root / 'project.godot'
        text = settings.read_text(encoding='utf-8')
        text = re.sub(r'(?m)^run/main_scene=.*\n', '', text)
        text = re.sub(r'(?m)^buses/default_bus_layout=.*$', 'buses/default_bus_layout="res://default_bus_layout.tres"', text)
        text = re.sub(r'(?m)^fmod/default_bus_layout=.*$', 'fmod/default_bus_layout="res://default_fmod_bus_layout.tres"', text)
        settings.write_text(text, encoding='utf-8', newline='\n')
        environment['APPDATA'] = str(output / 'user_data')
        environment['LOCALAPPDATA'] = str(output / 'user_cache')
        with (output / 'import-console.log').open('w', encoding='utf-8') as stream:
            subprocess.run([args.godot, '--headless', '--editor', '--import', '--path', str(project_root),
                            '--log-file', str(output / 'import.log')], env=environment,
                           stdout=stream, stderr=subprocess.STDOUT, check=True)
    project = project_root / 'project.godot'
    original = project.read_bytes()
    text = original.decode('utf-8')
    pattern = r'(\[editor_plugins\]\s+enabled=PackedStringArray\()(.*?)(\))'

    def enable(match):
        entries = match.group(2).strip()
        plugin = '"res://editor/plugin.cfg"'
        if plugin not in entries:
            entries += (', ' if entries else '') + plugin
        return match.group(1) + entries + match.group(3)

    updated, count = re.subn(pattern, enable, text, count=1, flags=re.S)
    if count != 1:
        raise SystemExit('Editor plugin configuration not found in the test project.')
    try:
        project.write_text(updated, encoding='utf-8', newline='\n')
        completed = subprocess.run([
            args.godot, '--headless', '--editor', '--path', str(project_root),
            '--log-file', str(output / 'godot.log'), '--', '--output-dir=' + str(output),
        ], cwd=ROOT, env=environment)
    finally:
        project.write_bytes(original)
    raise SystemExit(completed.returncode)


if __name__ == '__main__':
    main()

"""Run the editor translation check with the normal editor lifecycle."""
from pathlib import Path
import argparse
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--godot', required=True)
    parser.add_argument('--output-dir', required=True, type=Path)
    args = parser.parse_args()
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    project = ROOT / 'tests/project.godot'
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
            args.godot, '--headless', '--editor', '--path', str(ROOT / 'tests'),
            '--log-file', str(output / 'godot.log'), '--', '--output-dir=' + str(output),
        ], cwd=ROOT)
    finally:
        project.write_bytes(original)
    raise SystemExit(completed.returncode)


if __name__ == '__main__':
    main()

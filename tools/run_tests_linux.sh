#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."

godot="${GODOT_BIN:-godot}"
target="${FMOD_TEST_TARGET:-template_debug}"
case "$target" in
    template_debug|template_release) ;;
    *) printf 'Unknown FMOD_TEST_TARGET: %s\n' "$target" >&2; exit 1 ;;
esac
fmod_core="${FMOD_LINUX_SDK:-src/thirdparty/fmod/linux}"
if [[ -d "$fmod_core/api/core" ]]; then
    fmod_core="$fmod_core/api/core"
fi
run_dir="$PWD/tests/logs/linux_${target}_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$run_dir" tests/addons/fmod_player/bin
touch tests/logs/.gdignore
cp -p tests/project.godot "$run_dir/project.before.godot"
restore_project() {
    cp -p "$run_dir/project.before.godot" tests/project.godot
}
trap restore_project EXIT
# Bootstrap the main scene before its UID is indexed; keep import and tests offline.
# The editor ignores override.cfg, so restore the original project on every exit.
python3 - "$run_dir/project.before.godot" <<'PYTHON'
from pathlib import Path
import re
import sys

config = Path(sys.argv[1]).read_text(encoding="utf-8")
config = re.sub(r"(?m)^run/main_scene=.*$", 'run/main_scene="res://main.tscn"', config)
config = re.sub(r"(?m)^fmod/output_type=.*\n", "", config)
if "[audio]\n" in config:
    config = config.replace("[audio]\n", "[audio]\nfmod/output_type=4\n", 1)
else:
    config += "\n[audio]\nfmod/output_type=4\n"
Path("tests/project.godot").write_text(config, encoding="utf-8", newline="\n")
PYTHON
cp -a addons/fmod_player/. tests/addons/fmod_player/
for library in libfmodL.so.15 libfmod.so.15; do
    cp -L "$fmod_core/lib/x86_64/$library" "tests/addons/fmod_player/bin/$library"
done
if [[ "$target" == template_release ]]; then
    # The editor has the debug feature; point only the staged config to Release.
    sed -i \
        -e 's/libfmod_player.linux.template_debug.x86_64.so/libfmod_player.linux.template_release.x86_64.so/g' \
        -e 's/libfmodL.so.15/libfmod.so.15/g' \
        tests/addons/fmod_player/bin/fmod_player.gdextension
fi

check_log() {
    if grep -Eq '^(SCRIPT ERROR:|ERROR:)' "$1"; then
        tail -n 60 "$1"
        return 1
    fi
}

sha256sum tests/addons/fmod_player/bin/libfmod_player.linux.*.x86_64.so > "$run_dir/build.sha256"
printf '%s\n' "$target" > "$run_dir/target.txt"
"$godot" --headless --path tests --editor --import --log-file "$run_dir/import.log" > "$run_dir/import-console.log" 2>&1
check_log "$run_dir/import-console.log"
"$godot" --headless --path tests --log-file "$run_dir/godot.log" -- --suite=all "--output-dir=$run_dir" "$@" > "$run_dir/console.log" 2>&1
check_log "$run_dir/console.log"
grep -q 'TESTS_PASSED' "$run_dir/console.log"
printf 'Test output: %s\n' "$run_dir"

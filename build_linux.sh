#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

mkdir -p tests/logs
for target in template_debug template_release; do
    log="tests/logs/build_linux_${target}.log"
    if ! scons platform=linux target="$target" arch=x86_64 -j"${JOBS:-2}" "$@" >"$log" 2>&1; then
        tail -n 60 "$log"
        exit 1
    fi
    printf 'Built %s. Log: %s\n' "$target" "$log"
done

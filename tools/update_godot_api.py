"""Normalize an official Godot API dump for the pinned godot-cpp generator."""
import json
from pathlib import Path
import sys

source, destination = map(Path, sys.argv[1:3])
api = json.loads(source.read_text(encoding="utf-8"))
# These 4.7 additions are already supplied as macros by <cstdint>. The 4.5
# generator emits colliding C++ identifiers; all classes and hashes stay intact.
stdint_names = {
    "UINT8_MAX", "UINT16_MAX", "UINT32_MAX", "UINT64_MAX",
    "INT8_MIN", "INT8_MAX", "INT16_MIN", "INT16_MAX",
    "INT32_MIN", "INT32_MAX", "INT64_MIN", "INT64_MAX",
}
api["global_constants"] = [c for c in api.get("global_constants", []) if c["name"] not in stdint_names]
destination.write_text(json.dumps(api, ensure_ascii=False, indent="\t") + "\n", encoding="utf-8", newline="\n")

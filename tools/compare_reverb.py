"""Compare two runs of the tests project's reverb suite."""

import argparse
import array
import json
import math
from pathlib import Path


def compare(before: Path, after: Path) -> dict:
    old = {item["name"]: item for item in json.loads((before / "reverb_timings.json").read_text())}
    new = {item["name"]: item for item in json.loads((after / "reverb_timings.json").read_text())}
    timings = []
    for name, item in old.items():
        before_us = item["median_us_per_update"]
        after_us = new[name]["median_us_per_update"]
        timings.append({
            "name": name,
            "before_us": before_us,
            "after_us": after_us,
            "speedup": before_us / after_us,
            "time_reduction_percent": 100.0 * (1.0 - after_us / before_us),
        })
    audio = []
    for name in ("default", "highpass", "dynamic", "tail"):
        filename = "reverb_" + name + ".f32"
        left = (before / filename).read_bytes()
        right = (after / filename).read_bytes()
        if len(left) != len(right) or not left or len(left) % 8:
            raise ValueError("Invalid stereo float32 capture length: " + filename)
        a = array.array("f")
        b = array.array("f")
        a.frombytes(left)
        b.frombytes(right)
        if not all(math.isfinite(value) for value in a) or not all(math.isfinite(value) for value in b):
            raise ValueError("Non-finite audio: " + filename)
        max_error = max(abs(x - y) for x, y in zip(a, b))
        audio.append({"case": name, "samples": len(a), "bit_exact": left == right, "max_absolute_error": max_error})
    return {
        "before": str(before),
        "after": str(after),
        "conditions": "48 kHz stereo, 1024 frames/block, NOSOUND_NRT + MIX_FROM_UPDATE; /O2 + PDB; five trial medians. Timings include base mixing and script overhead; scheduling/CPU frequency may vary.",
        "timings": timings,
        "audio": audio,
        "all_bit_exact": all(item["bit_exact"] for item in audio),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("before", type=Path)
    parser.add_argument("after", type=Path)
    parser.add_argument("--output", type=Path, default=Path("tests/logs/reverb_comparison.json"))
    args = parser.parse_args()
    report = compare(args.before, args.after)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    for item in report["timings"]:
        print(f"{item['name']}: {item['before_us']:.2f} -> {item['after_us']:.2f} us, {item['speedup']:.2f}x")
    print("Audio bit exact:", report["all_bit_exact"])
    print("Report:", args.output)
    if not report["all_bit_exact"]:
        raise SystemExit("Audio output changed; inspect the saved comparison before accepting the optimization")


if __name__ == "__main__":
    main()

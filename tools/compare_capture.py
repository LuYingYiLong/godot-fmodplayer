"""Compare two runs of the tests project's capture suite."""

import argparse
import json
from pathlib import Path


def read_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def compare(before: Path, after: Path) -> dict:
    old = {item["name"]: item for item in read_json(before / "capture_timings.json")}
    new = {item["name"]: item for item in read_json(after / "capture_timings.json")}
    timings = []
    for name, item in old.items():
        current = new[name]
        if (item["sample_rate"], item["frames_per_update"]) != (current["sample_rate"], current["frames_per_update"]):
            raise ValueError("Different audio formats: " + name)
        before_us = item["median_us_per_update"]
        after_us = current["median_us_per_update"]
        timings.append({
            "name": name,
            "before_us": before_us,
            "after_us": after_us,
            "speedup": before_us / after_us,
            "time_reduction_percent": 100.0 * (1.0 - after_us / before_us),
        })
    original = (before / "capture_steady.f32").read_bytes()
    optimized = (after / "capture_steady.f32").read_bytes()
    if len(original) != 24 * 1024 * 2 * 4 or len(optimized) != len(original):
        raise ValueError("Incomplete deterministic stereo capture")
    before_checks = read_json(before / "capture_regression.json")
    after_checks = read_json(after / "capture_regression.json")
    if not before_checks["accepted"]:
        raise ValueError("Baseline contains unexpected regression failures")
    fixed = not after_checks["baseline_mode"] and after_checks["accepted"] and all(item["passed"] for item in after_checks["checks"])
    return {
        "before": str(before),
        "after": str(after),
        "conditions": "48 kHz stereo, 1024 frames/block, NOSOUND_NRT + STREAM_FROM_UPDATE; /O2 + PDB; five trial medians. Timings include base mixing and script calls. CPU frequency/scheduling may vary.",
        "before_build": read_json(before / "build.json"),
        "after_build": read_json(after / "build.json"),
        "timings": timings,
        "steady_audio_frames": len(original) // 8,
        "steady_audio_bit_exact": original == optimized,
        "before_failed_checks": [item for item in before_checks["checks"] if not item["passed"]],
        "after_all_checks_passed": fixed,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("before", type=Path)
    parser.add_argument("after", type=Path)
    parser.add_argument("--output", type=Path, default=Path("tests/logs/comparisons/capture_comparison.json"))
    args = parser.parse_args()
    report = compare(args.before, args.after)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    for item in report["timings"]:
        print(f"{item['name']}: {item['before_us']:.2f} -> {item['after_us']:.2f} us, {item['speedup']:.2f}x")
    print("Steady audio bit exact:", report["steady_audio_bit_exact"])
    print("Capture regression passed:", report["after_all_checks_passed"])
    print("Report:", args.output)
    if not report["steady_audio_bit_exact"] or not report["after_all_checks_passed"]:
        raise SystemExit("Capture comparison failed; inspect the saved report")


if __name__ == "__main__":
    main()

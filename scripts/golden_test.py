#!/usr/bin/env python3

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path

from png_compare import compare_rgba_pngs


ROOT = Path(__file__).resolve().parent.parent


def load_tolerance(config_path, image_name):
    config = json.loads(Path(config_path).read_text())
    tolerance = dict(config.get("default", {}))
    tolerance.update(config.get("images", {}).get(image_name, {}))
    max_pixels = tolerance.get("max_differing_pixels")
    ratio = tolerance.get("max_differing_pixel_ratio")
    channel_delta = tolerance.get("per_channel_delta")
    if not isinstance(max_pixels, int) or max_pixels < 0:
        raise ValueError(f"{image_name}: max_differing_pixels must be nonnegative")
    if not isinstance(ratio, (int, float)) or not 0 <= ratio <= 1:
        raise ValueError(f"{image_name}: max_differing_pixel_ratio must be in [0, 1]")
    if not isinstance(channel_delta, int) or not 0 <= channel_delta <= 255:
        raise ValueError(f"{image_name}: per_channel_delta must be in [0, 255]")
    return max_pixels, ratio, channel_delta


def run_example(engine, script, run_dir, output_name):
    run_root = Path(run_dir)
    working_dir = run_root / "examples"
    output_dir = run_root / "output"
    working_dir.mkdir(parents=True, exist_ok=True)
    output_dir.mkdir(parents=True, exist_ok=True)
    source_script = Path(script).resolve()
    run_script = working_dir / source_script.name
    shutil.copyfile(source_script, run_script)
    lib_link = working_dir / "lib"
    if lib_link.is_symlink():
        lib_link.unlink()
    if not lib_link.exists():
        lib_link.symlink_to(ROOT / "examples" / "lib", target_is_directory=True)
    node_modules_link = run_root / "node_modules"
    if node_modules_link.is_symlink():
        node_modules_link.unlink()
    if not node_modules_link.exists():
        node_modules_link.symlink_to(ROOT / "node_modules", target_is_directory=True)
    result = subprocess.run(
        [str(Path(engine).resolve()), str(run_script.resolve())],
        cwd=working_dir,
        text=True,
        capture_output=True,
        check=False,
    )
    if result.returncode != 0:
        sys.stderr.write(result.stdout)
        sys.stderr.write(result.stderr)
        raise RuntimeError(
            f"example failed with exit code {result.returncode}: {script}"
        )
    actual = output_dir / output_name
    if not actual.is_file():
        sys.stderr.write(result.stdout)
        sys.stderr.write(result.stderr)
        raise RuntimeError(f"example did not produce expected image: {actual}")
    return actual


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--actual")
    parser.add_argument("--engine")
    parser.add_argument("--script")
    parser.add_argument("--run-dir")
    parser.add_argument("--output-name")
    parser.add_argument("--golden", required=True)
    parser.add_argument("--config", required=True)
    parser.add_argument("--diff", required=True)
    args = parser.parse_args()

    try:
        image_name = Path(args.golden).name
        max_pixels, max_ratio, channel_delta = load_tolerance(
            args.config, image_name
        )
        if args.actual:
            actual = Path(args.actual)
        elif args.engine and args.script and args.run_dir and args.output_name:
            actual = run_example(
                args.engine, args.script, args.run_dir, args.output_name
            )
        else:
            parser.error(
                "provide --actual or all of --engine, --script, --run-dir, "
                "and --output-name"
            )

        result = compare_rgba_pngs(
            actual, args.golden, channel_delta, args.diff
        )
        allowed = min(
            max_pixels, int(result["total_pixels"] * max_ratio)
        )
        passed = result["same_size"] and result["differing_pixels"] <= allowed
        if not passed:
            print(
                f"golden mismatch: {image_name}: "
                f"{result['differing_pixels']}/{result['total_pixels']} "
                f"pixels differ (allowed {allowed}); "
                f"largest channel delta={result['largest_channel_delta']}; "
                f"actual={result['actual_size']} "
                f"golden={result['golden_size']}; diff={result['diff_path']}",
                file=sys.stderr,
            )
            return 1

        print(
            f"golden ok: {image_name}: "
            f"{result['differing_pixels']}/{result['total_pixels']} pixels differ "
            f"(allowed {allowed}, channel delta {channel_delta})"
        )
        return 0
    except (OSError, ValueError, RuntimeError, json.JSONDecodeError) as error:
        print(f"golden test failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

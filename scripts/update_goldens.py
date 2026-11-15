#!/usr/bin/env python3

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
REGRESSION_GOLDENS = {
    "render_regression.png": "render_regression.png",
    "text_rotate_regression.png": "text_rotate_regression.png",
    "echarts_sunburst_regression.png": "echarts_sunburst_regression.png",
    "echarts_timeline_regression.png": "echarts_timeline_regression.png",
    "echarts_custom_series_regression.png": "echarts_custom_series_regression.png",
    "echarts_pictorial_bar_regression.png": "echarts_pictorial_bar_regression.png",
    "echarts_gauge_regression.png": "echarts_gauge_regression.png",
    "echarts_image_scatter_regression.png": "echarts_image_scatter_regression.png",
}


def update_goldens(build_dir, engine, skip_regression_render):
    golden_dir = ROOT / "tests" / "golden"
    config_path = golden_dir / "tolerances.json"
    test_output = build_dir / "test_output"
    update_dir = build_dir / "golden-update"
    golden_dir.mkdir(parents=True, exist_ok=True)

    render_tests = (
        "render_regression_render|text_rotate_regression_render|"
        "echarts_regression_render"
    )
    if not skip_regression_render:
        subprocess.run(
            [
                "ctest",
                "--test-dir",
                str(build_dir),
                "--output-on-failure",
                "-R",
                f"^({render_tests})$",
            ],
            cwd=ROOT,
            check=True,
        )
    for golden_name, output_name in REGRESSION_GOLDENS.items():
        source = test_output / output_name
        if not source.is_file():
            raise FileNotFoundError(f"render test did not create {source}")
        shutil.copyfile(source, golden_dir / golden_name)

    for script in sorted((ROOT / "examples").glob("echarts_*.js")):
        run_dir = update_dir / script.stem
        working_dir = run_dir / "examples"
        output_dir = run_dir / "output"
        working_dir.mkdir(parents=True, exist_ok=True)
        output_dir.mkdir(parents=True, exist_ok=True)
        run_script = working_dir / script.name
        shutil.copyfile(script, run_script)
        lib_link = working_dir / "lib"
        if lib_link.is_symlink():
            lib_link.unlink()
        if not lib_link.exists():
            lib_link.symlink_to(
                ROOT / "examples" / "lib", target_is_directory=True
            )
        node_modules_link = run_dir / "node_modules"
        if node_modules_link.is_symlink():
            node_modules_link.unlink()
        if not node_modules_link.exists():
            node_modules_link.symlink_to(
                ROOT / "node_modules", target_is_directory=True
            )
        result = subprocess.run(
            [str(engine), str(run_script.resolve())],
            cwd=working_dir,
            text=True,
            capture_output=True,
            check=False,
        )
        if result.returncode != 0:
            sys.stderr.write(result.stdout)
            sys.stderr.write(result.stderr)
            raise RuntimeError(
                f"{script.relative_to(ROOT)} failed with exit code "
                f"{result.returncode}"
            )
        output = output_dir / f"{script.stem}.png"
        if not output.is_file():
            raise FileNotFoundError(
                f"{script.relative_to(ROOT)} did not create {output}"
            )
        shutil.copyfile(output, golden_dir / output.name)
        print(f"updated {golden_dir / output.name}")

    config = json.loads(config_path.read_text())
    defaults = config.get("default")
    if not isinstance(defaults, dict):
        raise ValueError(f"{config_path}: missing default tolerance settings")
    images = config.setdefault("images", {})
    for image in sorted(golden_dir.glob("*.png")):
        images.setdefault(image.name, dict(defaults))
    config_path.write_text(json.dumps(config, indent=2, sort_keys=True) + "\n")
    print(f"updated tolerance entries in {config_path}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--update-goldens",
        action="store_true",
        help="render fixtures and replace tests/golden PNGs",
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=ROOT / "build" / "dev",
    )
    parser.add_argument(
        "--skip-regression-render",
        action="store_true",
        help="reuse regression PNGs already present in the build directory",
    )
    args = parser.parse_args()
    if not args.update_goldens:
        parser.error("pass --update-goldens to replace the golden images")

    build_dir = args.build_dir.resolve()
    engine = build_dir / "canvas_engine"
    if not engine.is_file():
        parser.error(f"engine not found; build it first: {engine}")
    try:
        update_goldens(build_dir, engine, args.skip_regression_render)
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"golden update failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

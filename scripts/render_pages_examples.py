#!/usr/bin/env python3

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent


def render_examples(engine, examples_dir, output_dir, work_dir):
    examples = sorted(Path(examples_dir).glob("echarts_*.js"))
    destination = Path(output_dir)
    destination.mkdir(parents=True, exist_ok=True)
    for script in examples:
        run_dir = Path(work_dir) / script.stem
        working_dir = run_dir / "examples"
        output_path = run_dir / "output"
        working_dir.mkdir(parents=True, exist_ok=True)
        output_path.mkdir(parents=True, exist_ok=True)
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
                f"{script.relative_to(ROOT)} failed with exit code "
                f"{result.returncode}"
            )
        rendered = output_path / f"{script.stem}.png"
        if not rendered.is_file():
            raise FileNotFoundError(f"missing render: {rendered}")
        for generated in output_path.glob("*.png"):
            shutil.copyfile(generated, destination / generated.name)
        print(f"rendered {script.stem}.png")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--examples-dir", type=Path, default=ROOT / "examples")
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument(
        "--work-dir", type=Path, default=ROOT / "build" / "pages-render"
    )
    args = parser.parse_args()
    render_examples(
        args.engine.resolve(),
        args.examples_dir.resolve(),
        args.output_dir.resolve(),
        args.work_dir.resolve(),
    )


if __name__ == "__main__":
    main()

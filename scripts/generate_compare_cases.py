#!/usr/bin/env python3

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
EXAMPLE_ALIASES = {"timeline_bar": "timeline"}


def generate(examples_dir, cases_path, output_path):
    metadata = json.loads(Path(cases_path).read_text())
    scripts = sorted(Path(examples_dir).glob("echarts_*.js"))
    scripts_by_id = {
        EXAMPLE_ALIASES.get(script.stem.removeprefix("echarts_"),
                            script.stem.removeprefix("echarts_")): script
        for script in scripts
    }
    metadata_by_id = {case["id"]: case for case in metadata}
    if scripts_by_id.keys() != metadata_by_id.keys():
        missing = sorted(metadata_by_id.keys() - scripts_by_id.keys())
        unlisted = sorted(scripts_by_id.keys() - metadata_by_id.keys())
        raise ValueError(f"case mismatch: missing={missing}, unlisted={unlisted}")

    lines = [
        "function makeCanvas(width, height) {",
        '  const canvas = document.createElement("canvas");',
        "  canvas.width = width;",
        "  canvas.height = height;",
        "  canvas.saveToPng = () => {};",
        "  return canvas;",
        "}",
        "",
        "function normalizeOutputPaths(value, seen = new Set()) {",
        '  if (typeof value === "string") return value.replaceAll("../output/", "output/");',
        '  if (!value || typeof value !== "object" || seen.has(value)) return value;',
        "  seen.add(value);",
        "  for (const key of Object.keys(value)) {",
        "    value[key] = normalizeOutputPaths(value[key], seen);",
        "  }",
        "  return value;",
        "}",
        "",
    ]
    for case_id, script in scripts_by_id.items():
        source = script.read_text()
        lines.extend(
            [
                f"function collect_{case_id}(echarts) {{",
                "  let captured = null;",
                "  const loadScript = (path) => {",
                '    if (!path.endsWith("echarts_setup.js")) {',
                "      throw new Error(`unsupported example dependency: ${path}`);",
                "    }",
                "    return {",
                "      createCanvas: makeCanvas,",
                "      renderChart: (args) => { captured = args; },",
                "    };",
                "  };",
                "  const console = { log() {}, warn() {}, error() {} };",
                "  {",
            ]
        )
        lines.extend(f"    {line}" if line else "" for line in source.splitlines())
        lines.extend(
            [
                "  }",
                f'  if (!captured) throw new Error("example {case_id} did not call renderChart");',
                "  captured.option = normalizeOutputPaths(captured.option);",
                "  return captured;",
                "}",
                "",
            ]
        )

    lines.append("export function createExampleOptions(echarts) {")
    lines.append("  return {")
    lines.extend(
        f"    {case_id}: collect_{case_id}(echarts),"
        for case_id in scripts_by_id
    )
    lines.extend(["  };", "}", ""])
    output = Path(output_path)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--examples-dir", type=Path, default=ROOT / "examples")
    parser.add_argument(
        "--cases", type=Path, default=ROOT / "web" / "compare" / "cases.json"
    )
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    generate(args.examples_dir, args.cases, args.output)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3

import json
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "scripts"))
from generate_compare_cases import generate

CASES_PATH = ROOT / "web" / "compare" / "cases.json"
APP_PATH = ROOT / "web" / "compare" / "app.js"
EXAMPLES_DIR = ROOT / "examples"

EXAMPLE_ALIASES = {"timeline_bar": "timeline"}


def fail(message):
    raise SystemExit(message)


def load_cases():
    try:
        cases = json.loads(CASES_PATH.read_text())
    except FileNotFoundError:
        fail(f"missing compare case config: {CASES_PATH}")
    except json.JSONDecodeError as error:
        fail(f"invalid compare case config: {error}")

    if not isinstance(cases, list):
        fail("compare case config must be a JSON array")

    return cases


def main():
    cases = load_cases()
    app_source = APP_PATH.read_text()
    if "createExampleOptions" not in app_source:
        fail(f"{APP_PATH} must load generated example options")
    if "cases.generated.js" not in app_source:
        fail(f"{APP_PATH} must import the generated cases bundle")

    seen_case_ids = set()

    for entry in cases:
        case_id = entry.get("id")
        example_script = entry.get("exampleScript")

        if not case_id:
            fail("compare case entry is missing id")
        if case_id in seen_case_ids:
            fail(f"duplicate compare case id: {case_id}")
        seen_case_ids.add(case_id)

        if not example_script:
            fail(f"compare case {case_id} is missing exampleScript")

        example_path = ROOT / example_script
        if not example_path.exists():
            fail(f"compare case {case_id} references missing example script: {example_script}")

        example_id = EXAMPLE_ALIASES.get(
            example_path.stem.replace("echarts_", ""), example_path.stem.replace("echarts_", "")
        )
        if example_id != case_id:
            fail(
                f"compare case {case_id} references mismatched example "
                f"{example_script}"
            )

    example_ids = {
        EXAMPLE_ALIASES.get(path.stem.replace("echarts_", ""),
                            path.stem.replace("echarts_", ""))
        for path in EXAMPLES_DIR.glob("echarts_*.js")
    }
    missing = sorted(example_ids - seen_case_ids)
    stale = sorted(seen_case_ids - example_ids)

    if missing:
        fail(f"examples missing compare coverage: {', '.join(missing)}")
    if stale:
        fail(f"compare entries without example scripts: {', '.join(stale)}")

    with tempfile.TemporaryDirectory(prefix="compare-cases-") as temporary_dir:
        generated_path = Path(temporary_dir) / "cases.generated.js"
        generate(EXAMPLES_DIR, CASES_PATH, generated_path)
        generated_source = generated_path.read_text()
        missing_generated = sorted(
            case_id
            for case_id in seen_case_ids
            if f"collect_{case_id}(" not in generated_source
        )
        if missing_generated:
            fail(
                "generated cases bundle is missing: "
                + ", ".join(missing_generated)
            )

    print(f"compare_coverage ok: {len(example_ids)} generated cases")


if __name__ == "__main__":
    try:
        main()
    except FileNotFoundError as error:
        fail(f"missing file: {error.filename}")

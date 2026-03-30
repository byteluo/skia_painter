#!/usr/bin/env python3

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
CASES_PATH = ROOT / "web" / "compare" / "cases.json"
APP_PATH = ROOT / "web" / "compare" / "app.js"
EXAMPLES_DIR = ROOT / "examples"

EXAMPLE_ALIASES = {
    "timeline_bar": "timeline",
}


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


def load_case_ids_from_app():
    source = APP_PATH.read_text()
    return set(re.findall(r'case "([^"]+)":', source))


def main():
    cases = load_cases()
    app_case_ids = load_case_ids_from_app()

    seen_case_ids = set()
    case_ids = set()

    for entry in cases:
        case_id = entry.get("id")
        example_script = entry.get("exampleScript")

        if not case_id:
            fail("compare case entry is missing id")
        if case_id in seen_case_ids:
            fail(f"duplicate compare case id: {case_id}")
        seen_case_ids.add(case_id)
        case_ids.add(case_id)

        if not example_script:
            fail(f"compare case {case_id} is missing exampleScript")

        example_path = ROOT / example_script
        if not example_path.exists():
            fail(f"compare case {case_id} references missing example script: {example_script}")

        if case_id not in app_case_ids:
            fail(f"compare case {case_id} is missing browser option wiring in {APP_PATH}")

    example_names = {
        path.stem.replace("echarts_", "") for path in EXAMPLES_DIR.glob("echarts_*.js")
    }
    missing = []

    for example_name in sorted(example_names):
        compare_id = EXAMPLE_ALIASES.get(example_name, example_name)
        if compare_id not in case_ids:
            missing.append(example_name)

    if missing:
        fail(f"examples missing compare coverage: {', '.join(missing)}")

    print("compare_coverage ok")


if __name__ == "__main__":
    try:
        main()
    except FileNotFoundError as error:
        fail(f"missing file: {error.filename}")

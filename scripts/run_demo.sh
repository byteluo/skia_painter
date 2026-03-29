#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${1:-${ROOT_DIR}/build/dev}"
SCRIPT_PATH="${2:-${ROOT_DIR}/examples/demo.js}"

mkdir -p "${ROOT_DIR}/output"
"${BUILD_DIR}/canvas_engine" "${SCRIPT_PATH}"

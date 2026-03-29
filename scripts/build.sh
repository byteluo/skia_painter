#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
PRESET="${1:-dev}"

cmake --preset "${PRESET}" "${ROOT_DIR}"
cmake --build --preset "${PRESET}"

#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
SKIA_DIR="${ROOT_DIR}/third_party/skia"
BUILD_PRESET="${1:-dev}"
BAZEL_OUTPUT_ROOT="${BAZEL_OUTPUT_ROOT:-${ROOT_DIR}/build/bazel-root}"
FORCE_SKIA_BUILD="${FORCE_SKIA_BUILD:-0}"

readonly REQUIRED_BREW_PACKAGES=(
  cmake
  ninja
  v8
  libpng
  freetype
  zlib
  pkgconf
  openjdk
  bazel@8
)

log() {
  printf '[bootstrap] %s\n' "$*"
}

fail() {
  printf '[bootstrap] error: %s\n' "$*" >&2
  exit 1
}

require_macos() {
  [[ "$(uname -s)" == "Darwin" ]] || fail "this bootstrap script currently supports macOS only"
}

require_homebrew() {
  command -v brew >/dev/null 2>&1 || fail "Homebrew is required: https://brew.sh"
}

configure_java_home() {
  if JAVA_HOME="$(
    /usr/libexec/java_home 2>/dev/null
  )"; then
    export JAVA_HOME
    export PATH="${JAVA_HOME}/bin:${PATH}"
    log "Using system Java runtime at ${JAVA_HOME}"
    return
  fi

  JAVA_HOME="$(brew --prefix openjdk)/libexec/openjdk.jdk/Contents/Home"
  [[ -d "${JAVA_HOME}" ]] || fail "openjdk is installed but JAVA_HOME could not be resolved"
  export JAVA_HOME
  export PATH="${JAVA_HOME}/bin:${PATH}"
  log "Using Homebrew Java runtime at ${JAVA_HOME}"
}

install_brew_dependencies() {
  local missing=()
  local package

  export HOMEBREW_NO_AUTO_UPDATE=1

  for package in "${REQUIRED_BREW_PACKAGES[@]}"; do
    if ! brew list --versions "${package}" >/dev/null 2>&1; then
      missing+=("${package}")
    fi
  done

  if ((${#missing[@]} == 0)); then
    log "Homebrew dependencies already installed"
    return
  fi

  log "Installing Homebrew dependencies: ${missing[*]}"
  brew install "${missing[@]}"
}

ensure_skia_checkout() {
  if [[ -d "${SKIA_DIR}" ]] && [[ -f "${SKIA_DIR}/MODULE.bazel" ]]; then
    log "Using existing Skia checkout"
    return
  fi

  if [[ -d "${ROOT_DIR}/.git" ]] && [[ -f "${ROOT_DIR}/.gitmodules" ]]; then
    log "Syncing Git submodules"
    git -C "${ROOT_DIR}" submodule update --init --recursive third_party/skia
  fi

  [[ -d "${SKIA_DIR}" ]] || fail "missing Skia checkout at ${SKIA_DIR}"
  [[ -f "${SKIA_DIR}/MODULE.bazel" ]] || fail "Skia checkout is incomplete at ${SKIA_DIR}"
}

build_skia_minimal() {
  local bazel_real

  if [[ "${FORCE_SKIA_BUILD}" != "1" ]] &&
     [[ -f "${SKIA_DIR}/bazel-bin/src/core/libcore.a" ]] &&
     [[ -f "${SKIA_DIR}/bazel-bin/src/base/libbase.a" ]] &&
     [[ -f "${SKIA_DIR}/bazel-bin/modules/skcms/_objs/skcms_public/skcms.o" ]] &&
     [[ -f "${SKIA_DIR}/bazel-bin/modules/skcms/_objs/skcms_TransformBaseline/skcms_TransformBaseline.o" ]]; then
    log "Reusing existing Skia build outputs"
    return
  fi

  bazel_real="$(brew --prefix bazel@8)/libexec/bin/bazel-real"
  [[ -x "${bazel_real}" ]] || fail "missing bazel-real at ${bazel_real}"

  mkdir -p "${BAZEL_OUTPUT_ROOT}"

  log "Building minimal Skia targets"
  (
    cd "${SKIA_DIR}"
    "${bazel_real}" \
      --batch \
      --noworkspace_rc \
      --output_user_root="${BAZEL_OUTPUT_ROOT}" \
      build \
      //:core \
      //src/base:base \
      //modules/skcms:skcms
  )

  [[ -f "${SKIA_DIR}/bazel-bin/src/core/libcore.a" ]] || fail "Skia core archive was not produced"
  [[ -f "${SKIA_DIR}/bazel-bin/src/base/libbase.a" ]] || fail "Skia base archive was not produced"
  [[ -f "${SKIA_DIR}/bazel-bin/modules/skcms/_objs/skcms_public/skcms.o" ]] || fail "Skia skcms object was not produced"
  [[ -f "${SKIA_DIR}/bazel-bin/modules/skcms/_objs/skcms_TransformBaseline/skcms_TransformBaseline.o" ]] || fail "Skia skcms baseline object was not produced"
}

configure_and_build_engine() {
  log "Configuring project with preset ${BUILD_PRESET}"
  cmake --preset "${BUILD_PRESET}"

  log "Building project with preset ${BUILD_PRESET}"
  cmake --build --preset "${BUILD_PRESET}"
}

run_smoke_test() {
  log "Running smoke test"
  ctest --preset "${BUILD_PRESET}"
}

main() {
  require_macos
  require_homebrew
  install_brew_dependencies
  configure_java_home
  ensure_skia_checkout
  build_skia_minimal
  configure_and_build_engine
  run_smoke_test
  log "Bootstrap finished successfully"
  log "Run the demo with: ${ROOT_DIR}/scripts/run_demo.sh ${ROOT_DIR}/build/${BUILD_PRESET}"
}

main "$@"

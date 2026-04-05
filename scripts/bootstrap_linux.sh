#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
SKIA_DIR="${ROOT_DIR}/third_party/skia"
BUILD_PRESET="${1:-dev}"
BAZEL_OUTPUT_ROOT="${BAZEL_OUTPUT_ROOT:-${ROOT_DIR}/build/bazel-root}"
FORCE_SKIA_BUILD="${FORCE_SKIA_BUILD:-0}"
V8_ROOT="${V8_ROOT:-/usr/local}"

log() {
  printf '[bootstrap-linux] %s\n' "$*"
}

fail() {
  printf '[bootstrap-linux] error: %s\n' "$*" >&2
  exit 1
}

require_linux() {
  [[ "$(uname -s)" == "Linux" ]] || fail "this script supports Linux only"
}

install_system_dependencies() {
  if command -v apt-get >/dev/null 2>&1; then
    log "Installing system dependencies via apt"
    sudo apt-get update -qq
    sudo apt-get install -y -qq \
      cmake \
      ninja-build \
      pkg-config \
      libpng-dev \
      libfreetype-dev \
      zlib1g-dev \
      python3 \
      git \
      curl \
      g++
  else
    log "Warning: auto-install only supports apt-get. Please install dependencies manually."
    log "Required: cmake, ninja-build, pkg-config, libpng-dev, libfreetype-dev, zlib1g-dev, python3"
  fi
}

install_bazelisk() {
  if command -v bazel >/dev/null 2>&1; then
    log "Bazel already installed"
    return
  fi

  log "Installing Bazelisk"
  local arch
  arch="$(uname -m)"
  case "${arch}" in
    x86_64) arch="amd64" ;;
    aarch64) arch="arm64" ;;
  esac

  curl -fsSL "https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-${arch}" \
    -o /tmp/bazelisk
  chmod +x /tmp/bazelisk
  sudo mv /tmp/bazelisk /usr/local/bin/bazel
}

install_v8() {
  if [[ -f "${V8_ROOT}/lib/libv8.so" ]] || [[ -f "${V8_ROOT}/lib/libv8_monolith.a" ]]; then
    log "V8 already installed at ${V8_ROOT}"
    return
  fi

  log "Building V8 from source (this may take 30+ minutes)"

  local v8_build_dir="${ROOT_DIR}/build/v8-build"
  mkdir -p "${v8_build_dir}"

  if ! command -v fetch >/dev/null 2>&1; then
    log "Installing depot_tools"
    if [[ ! -d "${v8_build_dir}/depot_tools" ]]; then
      git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git \
        "${v8_build_dir}/depot_tools"
    fi
    export PATH="${v8_build_dir}/depot_tools:${PATH}"
  fi

  (
    cd "${v8_build_dir}"
    if [[ ! -d "v8" ]]; then
      fetch v8
    fi
    cd v8
    git checkout main
    gclient sync

    # Build V8 as shared libraries
    tools/dev/v8gen.py x64.release -- \
      is_component_build=true \
      v8_use_external_startup_data=true \
      v8_enable_i18n_support=true \
      treat_warnings_as_errors=false

    ninja -C out.gn/x64.release v8 v8_libplatform

    # Install to V8_ROOT
    sudo mkdir -p "${V8_ROOT}/include" "${V8_ROOT}/lib" "${V8_ROOT}/share/v8"
    sudo cp -r include/* "${V8_ROOT}/include/"
    sudo cp out.gn/x64.release/lib*.so "${V8_ROOT}/lib/" 2>/dev/null || true
    sudo cp out.gn/x64.release/*.so "${V8_ROOT}/lib/" 2>/dev/null || true
    sudo cp out.gn/x64.release/icudtl.dat "${V8_ROOT}/share/v8/"
    sudo ldconfig
  )

  log "V8 installed to ${V8_ROOT}"
}

configure_java_home() {
  if command -v java >/dev/null 2>&1; then
    JAVA_HOME="${JAVA_HOME:-$(dirname "$(dirname "$(readlink -f "$(command -v java)")")")}"
    export JAVA_HOME
    log "Using Java at ${JAVA_HOME}"
    return
  fi

  if command -v apt-get >/dev/null 2>&1; then
    log "Installing OpenJDK for Bazel"
    sudo apt-get install -y -qq default-jdk
    JAVA_HOME="$(dirname "$(dirname "$(readlink -f "$(command -v java)")")")"
    export JAVA_HOME
  else
    fail "Java is required for Bazel. Install OpenJDK and set JAVA_HOME."
  fi
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
  if [[ "${FORCE_SKIA_BUILD}" != "1" ]] &&
     [[ -f "${SKIA_DIR}/bazel-bin/src/core/libcore.a" ]] &&
     [[ -f "${SKIA_DIR}/bazel-bin/src/base/libbase.a" ]] &&
     [[ -f "${SKIA_DIR}/bazel-bin/modules/skcms/_objs/skcms_public/skcms.o" ]] &&
     [[ -f "${SKIA_DIR}/bazel-bin/modules/skcms/_objs/skcms_TransformBaseline/skcms_TransformBaseline.o" ]]; then
    log "Reusing existing Skia build outputs"
    return
  fi

  mkdir -p "${BAZEL_OUTPUT_ROOT}"

  log "Building minimal Skia targets"
  (
    cd "${SKIA_DIR}"
    bazel \
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
  export V8_ROOT
  cmake --preset "${BUILD_PRESET}" \
    -DV8_INCLUDE_DIRS="${V8_ROOT}/include" \
    -DV8_LINK_DIRECTORIES="${V8_ROOT}/lib" \
    -DV8_LIBRARIES="v8;v8_libplatform;v8_libbase" \
    -DV8_COMPILE_DEFINITIONS="V8_COMPRESS_POINTERS;V8_COMPRESS_POINTERS_IN_SHARED_CAGE;V8_31BIT_SMIS_ON_64BIT_ARCH;V8_ENABLE_SANDBOX"

  log "Building project with preset ${BUILD_PRESET}"
  cmake --build --preset "${BUILD_PRESET}"
}

run_smoke_test() {
  log "Running smoke test"

  # Set V8_ICU_DATA_DIR for V8 to find icudtl.dat
  local icu_dir=""
  for candidate in "${V8_ROOT}/share/v8" "${V8_ROOT}/lib" "${V8_ROOT}/libexec"; do
    if [[ -f "${candidate}/icudtl.dat" ]]; then
      icu_dir="${candidate}"
      break
    fi
  done

  if [[ -n "${icu_dir}" ]]; then
    export V8_ICU_DATA_DIR="${icu_dir}"
  fi

  ctest --preset "${BUILD_PRESET}"
}

main() {
  require_linux
  install_system_dependencies
  configure_java_home
  install_bazelisk
  install_v8
  ensure_skia_checkout
  build_skia_minimal
  configure_and_build_engine
  run_smoke_test
  log "Bootstrap finished successfully"
  log "Run the demo with: ${ROOT_DIR}/scripts/run_demo.sh ${ROOT_DIR}/build/${BUILD_PRESET}"
}

main "$@"

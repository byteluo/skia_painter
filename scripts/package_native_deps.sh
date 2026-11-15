#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${ROOT_DIR}/build/native-deps"
SKIA_COMMIT="31521f8508c712615b3d35e8e4554ccb5bf568e1"
V8_SOURCE_DIR="${ROOT_DIR}/build/v8-build/v8"
V8_OUT_DIR="${V8_SOURCE_DIR}/out.gn/x64.release"
SKIA_ROOT="${ROOT_DIR}/third_party/skia"
ARCHIVE_NAME="native-deps-linux-x86_64-v8-15.0.39-skia-31521f8.tar.zst"

usage() {
  printf 'Usage: %s [--out DIR]\n' "$0" >&2
}

fail() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

while (($#)); do
  case "$1" in
    --out)
      (($# >= 2)) || { usage; exit 2; }
      OUT_DIR="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      usage
      fail "unknown argument: $1"
      ;;
  esac
done

[[ "$(uname -s)" == "Linux" ]] || fail "native dependency packages are supported on Linux only"
[[ "$(uname -m)" == "x86_64" ]] || fail "native dependency packages are supported on x86_64 only"

for command_name in bash cmake c++ clang++ git ldd objdump python3 sha256sum tar zstd; do
  command -v "${command_name}" >/dev/null 2>&1 || fail "required tool not found: ${command_name}"
done

[[ -d "${V8_SOURCE_DIR}/include" ]] || fail "V8 headers not found under ${V8_SOURCE_DIR}"
[[ -f "${V8_OUT_DIR}/args.gn" ]] || fail "V8 GN args not found at ${V8_OUT_DIR}/args.gn"
[[ -f "${V8_OUT_DIR}/icudtl.dat" ]] || fail "V8 ICU data not found at ${V8_OUT_DIR}/icudtl.dat"
[[ -f "${V8_OUT_DIR}/snapshot_blob.bin" ]] || fail "V8 snapshot not found at ${V8_OUT_DIR}/snapshot_blob.bin"
[[ -f "${SKIA_ROOT}/MODULE.bazel" ]] || fail "Skia source tree not found at ${SKIA_ROOT}"
[[ -d "${SKIA_ROOT}/bazel-bin" ]] || fail "Skia Bazel outputs not found at ${SKIA_ROOT}/bazel-bin"
repository_skia_commit="$(git -C "${ROOT_DIR}" ls-tree HEAD third_party/skia | awk '{print $3}')"
[[ "${repository_skia_commit}" == "${SKIA_COMMIT}" ]] ||
  fail "expected Skia submodule commit ${SKIA_COMMIT}, found ${repository_skia_commit:-none}"

V8_VERSION="$(python3 - "${V8_SOURCE_DIR}/include/v8-version.h" <<'PY'
import pathlib
import re
import sys

header = pathlib.Path(sys.argv[1]).read_text()
values = {}
for name in ("MAJOR_VERSION", "MINOR_VERSION", "BUILD_NUMBER"):
    match = re.search(rf"^\s*#define\s+V8_{name}\s+(\d+)\s*$", header, re.MULTILINE)
    if not match:
        raise SystemExit(f"could not read V8_{name} from {sys.argv[1]}")
    values[name] = match.group(1)
print(".".join(values[name] for name in ("MAJOR_VERSION", "MINOR_VERSION", "BUILD_NUMBER")))
PY
)"
[[ "${V8_VERSION}" == "15.0.39" ]] || fail "expected V8 15.0.39, found ${V8_VERSION}"

mkdir -p "${OUT_DIR}" "${ROOT_DIR}/build"
OUT_DIR="$(cd "${OUT_DIR}" && pwd)"
STAGING_DIR="$(mktemp -d "${ROOT_DIR}/build/native-deps-package.XXXXXX")"
trap 'rm -rf "${STAGING_DIR}"' EXIT
PACKAGE_ROOT="${STAGING_DIR}/native-deps"
mkdir -p "${PACKAGE_ROOT}/v8/include" "${PACKAGE_ROOT}/v8/lib" \
  "${PACKAGE_ROOT}/v8/share/v8" "${PACKAGE_ROOT}/skia/bazel-bin"

cp -a "${V8_SOURCE_DIR}/include/." "${PACKAGE_ROOT}/v8/include/"
shopt -s nullglob
v8_libraries=("${V8_OUT_DIR}"/*.so)
((${#v8_libraries[@]} > 0)) || fail "no V8 shared libraries found in ${V8_OUT_DIR}"
cp -a "${v8_libraries[@]}" "${PACKAGE_ROOT}/v8/lib/"
cp -a "${V8_OUT_DIR}/icudtl.dat" "${PACKAGE_ROOT}/v8/share/v8/"
cp -a "${V8_OUT_DIR}/snapshot_blob.bin" "${PACKAGE_ROOT}/v8/share/v8/"

for v8_library in "${PACKAGE_ROOT}"/v8/lib/*.so; do
  ldd_output="$(LD_LIBRARY_PATH="${PACKAGE_ROOT}/v8/lib" ldd "${v8_library}")"
  if [[ "${ldd_output}" == *"not found"* ]]; then
    printf '%s\n' "${ldd_output}" >&2
    fail "unresolved dependency in ${v8_library}"
  fi
  if [[ "${ldd_output}" == *"${V8_OUT_DIR}"* ]]; then
    printf '%s\n' "${ldd_output}" >&2
    fail "V8 dependency resolved outside the package for ${v8_library}"
  fi
done
printf 'Verified V8 shared-library dependencies with ldd.\n'

skia_archives=(
  src/core/libcore.a
  src/base/libbase.a
  src/encode/libpng_encode.a
  src/encode/libpng_encode_base.a
  src/encode/libencoder_common.a
  src/encode/libicc_support.a
  src/codec/libany_decoder.a
  src/ports/libfontmgr_fontconfig.a
  src/ports/libfontmgr_custom.a
  src/ports/libfontmgr_directory_freetype.a
  src/ports/libfreetype_support.a
  src/ports/libtypeface_proxy.a
  src/utils/libchar_to_glyphcache.a
  external/+cpp_modules+freetype/libfreetype.a
  src/opts/libml3.a
)
skia_object_bases=(
  modules/skcms/_objs/skcms_public/skcms
  modules/skcms/_objs/skcms_TransformBaseline/skcms_TransformBaseline
  modules/skcms/_objs/skcms_TransformHsw/skcms_TransformHsw
  modules/skcms/_objs/skcms_TransformSkx/skcms_TransformSkx
)

for artifact in "${skia_archives[@]}"; do
  source="${SKIA_ROOT}/bazel-bin/${artifact}"
  [[ -f "${source}" ]] || fail "required Skia artifact not found: ${source}"
  destination="${PACKAGE_ROOT}/skia/bazel-bin/${artifact}"
  mkdir -p "$(dirname "${destination}")"
  cp -a "${source}" "${destination}"
done

for object_base in "${skia_object_bases[@]}"; do
  source_pic="${SKIA_ROOT}/bazel-bin/${object_base}.pic.o"
  source_plain="${SKIA_ROOT}/bazel-bin/${object_base}.o"
  if [[ -f "${source_pic}" ]]; then
    source="${source_pic}"
    relative="${object_base}.pic.o"
  elif [[ -f "${source_plain}" ]]; then
    source="${source_plain}"
    relative="${object_base}.o"
  else
    fail "required Skia object not found: ${source_pic} or ${source_plain}"
  fi
  destination="${PACKAGE_ROOT}/skia/bazel-bin/${relative}"
  mkdir -p "$(dirname "${destination}")"
  cp -a "${source}" "${destination}"
done

if [[ ! -f "${PACKAGE_ROOT}/skia/bazel-bin/src/core/libcore.a" ]]; then
  fail "Skia core archive was not packaged"
fi

cxx_version="$(c++ --version | head -n 1)"
clang_version="$(clang++ --version | head -n 1)"
bazel_version="$(bazel --version 2>/dev/null | head -n 1 || printf 'unavailable')"
python3 - "${PACKAGE_ROOT}" "${V8_OUT_DIR}/args.gn" "${V8_VERSION}" \
  "${SKIA_COMMIT}" "${cxx_version}" "${clang_version}" "${bazel_version}" <<'PY'
import hashlib
import json
import pathlib
import platform
import re
import subprocess
import sys

root = pathlib.Path(sys.argv[1])
gn_args = pathlib.Path(sys.argv[2]).read_text()
v8_version, skia_commit, cxx_version, clang_version, bazel_version = sys.argv[3:]
files = []
shared_libraries = []
for path in sorted(root.rglob("*")):
    if not path.is_file():
        continue
    relative = path.relative_to(root).as_posix()
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    files.append({"path": relative, "size": path.stat().st_size, "sha256": digest})
    if relative.startswith("v8/lib/") and path.suffix == ".so":
        shared_libraries.append(path)

versions = []
version_pattern = re.compile(r"\bGLIBC_([0-9]+(?:\.[0-9]+)+)\b")
for path in shared_libraries:
    process = subprocess.Popen(
        ["objdump", "-T", str(path)],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
    )
    assert process.stdout is not None
    for line in process.stdout:
        match = version_pattern.search(line)
        if match:
            versions.append(tuple(int(part) for part in match.group(1).split(".")))
    if process.wait() != 0:
        raise SystemExit(f"objdump -T failed for {path}")
if not versions:
    raise SystemExit("could not determine the maximum GLIBC symbol requirement")
max_version = max(versions)
max_glibc_symbol = "GLIBC_" + ".".join(str(part) for part in max_version)

os_release = platform.freedesktop_os_release()
ldd_output = subprocess.run(
    ["ldd", "--version"], check=True, capture_output=True, text=True
).stdout.splitlines()[0]
glibc_match = re.search(r"(\d+\.\d+(?:\.\d+)?)", ldd_output)
manifest = {
    "format_version": 1,
    "v8": {
        "version": v8_version,
        "gn_args": gn_args,
    },
    "skia_commit": skia_commit,
    "build": {
        "os": os_release.get("PRETTY_NAME", "unknown"),
        "architecture": platform.machine(),
        "glibc": glibc_match.group(1) if glibc_match else ldd_output,
        "glibc_version_output": ldd_output,
        "compiler_versions": {
            "c++": cxx_version,
            "clang++": clang_version,
            "bazel": bazel_version,
        },
    },
    "max_glibc_symbol": max_glibc_symbol,
    "files": files,
}
(root / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
print(f"Maximum GLIBC symbol requirement: {max_glibc_symbol}")
PY

archive="${OUT_DIR}/${ARCHIVE_NAME}"
tar --sort=name --mtime='@0' --owner=0 --group=0 --numeric-owner \
  --use-compress-program='zstd -19 -T1 --no-progress' \
  -cf "${archive}" -C "${STAGING_DIR}" native-deps
sha256sum "${archive}" > "${archive}.sha256"
printf 'Created %s (%s bytes)\n' "${archive}" "$(stat -c '%s' "${archive}")"
cat "${archive}.sha256"

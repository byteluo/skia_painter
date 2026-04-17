# =============================================================================
# Self-contained multi-stage Dockerfile for canvas_engine (Linux, x86_64).
#
# Builds V8 monolith + Skia + canvas_engine. Intermediate stages are
# cache-friendly so that with BuildKit GHA cache (cache-to: type=gha,mode=max),
# the expensive V8/Skia stages are reused across runs.
#
#   First build (cold cache):   ~60–90 min (mostly V8 compilation)
#   Incremental build (warm):   ~3–5 min  (only project-builder rebuilt)
#
# Usage:
#   docker build -t skia-painter-linux .
#   docker run --rm -v "$PWD":/work skia-painter-linux /work/examples/smoke.js
# =============================================================================

ARG V8_VERSION=13.6.233
ARG SKIA_COMMIT=6c7fd08b91efe0fa97a39474d7e4ca90807384ec

# ---------------------------------------------------------------------------
# Stage 1: Build V8 monolith static library from source.
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS v8-builder

ENV DEBIAN_FRONTEND=noninteractive
ARG V8_VERSION
ARG TARGETARCH

RUN apt-get update && apt-get install -y --no-install-recommends \
    git curl python3 python3-httplib2 ca-certificates \
    build-essential pkg-config lsb-release sudo xz-utils \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --depth=1 https://chromium.googlesource.com/chromium/tools/depot_tools.git /opt/depot_tools
ENV PATH="/opt/depot_tools:${PATH}"

WORKDIR /v8-src

RUN git clone --depth=1 --branch=${V8_VERSION} https://chromium.googlesource.com/v8/v8.git v8

RUN printf 'solutions = [{"name": "v8", "url": "https://chromium.googlesource.com/v8/v8.git", "deps_file": "DEPS", "managed": False}]\n' > .gclient && \
    cd v8 && gclient sync -D --no-history -j 1

RUN cd v8 && \
    if [ "${TARGETARCH}" = "arm64" ]; then V8_CPU="arm64"; else V8_CPU="x64"; fi && \
    mkdir -p out.gn/release && \
    printf 'is_debug = false\ntarget_cpu = "%s"\nv8_monolithic = true\nv8_use_external_startup_data = false\nuse_custom_libcxx = false\nis_clang = true\ntreat_warnings_as_errors = false\nv8_enable_sandbox = true\nis_component_build = false\nuse_sysroot = false\n' "${V8_CPU}" > out.gn/release/args.gn

RUN cd v8 && gn gen out.gn/release && ninja -C out.gn/release v8_monolith

RUN mkdir -p /v8-install/lib /v8-install/include && \
    cp /v8-src/v8/out.gn/release/obj/libv8_monolith.a /v8-install/lib/ && \
    cp -r /v8-src/v8/include/* /v8-install/include/ && \
    (cp /v8-src/v8/out.gn/release/icudtl.dat /v8-install/lib/ 2>/dev/null || true)

# ---------------------------------------------------------------------------
# Stage 2: Build Skia (core + base + skcms) with Bazel.
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS skia-builder

ENV DEBIAN_FRONTEND=noninteractive
ARG SKIA_COMMIT

RUN apt-get update && apt-get install -y --no-install-recommends \
    git curl ca-certificates build-essential python3 \
    openjdk-21-jdk-headless \
    && rm -rf /var/lib/apt/lists/*

RUN ARCH=$(dpkg --print-architecture) && \
    curl -Lo /usr/local/bin/bazel \
      "https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-${ARCH}" && \
    chmod +x /usr/local/bin/bazel

WORKDIR /skia-src

RUN git clone --filter=blob:none --no-checkout \
      https://skia.googlesource.com/skia.git /skia-src && \
    git checkout ${SKIA_COMMIT}

RUN ARCH=$(dpkg --print-architecture) && \
    if [ "${ARCH}" = "arm64" ]; then \
      export JAVA_HOME=/usr/lib/jvm/java-21-openjdk-arm64; \
    else \
      export JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64; \
    fi && \
    export PATH="${JAVA_HOME}/bin:${PATH}" && \
    bazel --batch --noworkspace_rc \
      build //:core //src/base:base //modules/skcms:skcms

# ---------------------------------------------------------------------------
# Stage 3: Build canvas_engine with the prebuilt V8 and Skia outputs.
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS project-builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build pkg-config python3 \
    libpng-dev zlib1g-dev libfreetype-dev \
    fonts-dejavu-core \
    nodejs npm \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# V8 install prefix (headers + libv8_monolith.a + icudtl.dat).
COPY --from=v8-builder /v8-install /usr/local

# Skia build outputs + headers.
COPY --from=skia-builder /skia-src/bazel-bin /src/third_party/skia/bazel-bin
COPY --from=skia-builder /skia-src/include /src/third_party/skia/include
COPY --from=skia-builder /skia-src/src/base /src/third_party/skia/src/base
COPY --from=skia-builder /skia-src/modules/skcms/src /src/third_party/skia/modules/skcms/src

# Project source.
COPY CMakeLists.txt CMakePresets.json package.json package-lock.json ./
COPY src/ src/
COPY include/ include/
COPY cmake/ cmake/
COPY examples/ examples/
COPY scripts/ scripts/

RUN npm ci || npm install

RUN cmake --preset release \
      -DV8_INCLUDE_DIRS=/usr/local/include \
      -DV8_LINK_DIRECTORIES=/usr/local/lib \
      -DV8_LIBRARIES=/usr/local/lib/libv8_monolith.a \
      -DV8_COMPILE_DEFINITIONS="V8_COMPRESS_POINTERS;V8_COMPRESS_POINTERS_IN_SHARED_CAGE;V8_31BIT_SMIS_ON_64BIT_ARCH;V8_ENABLE_SANDBOX;CPPGC_POINTER_COMPRESSION" \
    && cmake --build --preset release

RUN ./build/release/canvas_engine examples/smoke.js

# ---------------------------------------------------------------------------
# Stage 4: Minimal runtime image.
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libpng16-16t64 zlib1g libfreetype6 \
    fonts-dejavu-core \
    nodejs \
    && rm -rf /var/lib/apt/lists/*

COPY --from=project-builder /src/build/release/canvas_engine /usr/local/bin/canvas_engine
COPY --from=v8-builder /v8-install/lib/icudtl.dat /usr/local/lib/icudtl.dat
COPY --from=project-builder /src/node_modules /opt/canvas_engine/node_modules

ENV V8_DATA_DIR=/usr/local/lib
ENV NODE_PATH=/opt/canvas_engine/node_modules

ENTRYPOINT ["canvas_engine"]

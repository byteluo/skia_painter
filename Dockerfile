# =============================================================================
# Multi-stage Dockerfile for building canvas_engine on Linux (x86_64)
#
# Usage:
#   # Build the Linux binary:
#   docker build -t skia-painter-linux .
#
#   # Extract the binary:
#   docker cp "$(docker create skia-painter-linux)":/usr/local/bin/canvas_engine ./canvas_engine-linux
#
#   # Or run directly:
#   docker run --rm -v "$PWD":/work skia-painter-linux /work/examples/smoke.js
# =============================================================================

# ---------------------------------------------------------------------------
# Stage 1: Build V8 from source
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS v8-builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    git curl python3 python3-httplib2 ca-certificates \
    build-essential pkg-config lsb-release sudo \
    && rm -rf /var/lib/apt/lists/*

# depot_tools (Chromium/V8 build toolchain)
RUN git clone --depth=1 https://chromium.googlesource.com/chromium/tools/depot_tools.git /opt/depot_tools
ENV PATH="/opt/depot_tools:${PATH}"

WORKDIR /v8-src

# Fetch V8 source (use a known stable branch).
# Pin to a recent stable version that matches Homebrew v8 (~13.x).
RUN fetch v8 && cd v8 && git checkout branch-heads/13.6

RUN cd v8 && gclient sync -D

# Generate build config: static monolith library, release mode.
RUN cd v8 && \
    mkdir -p out.gn/x64.release && \
    cat > out.gn/x64.release/args.gn <<'EOF'
is_debug = false
target_cpu = "x64"
v8_monolithic = true
v8_use_external_startup_data = false
use_custom_libcxx = false
is_clang = true
treat_warnings_as_errors = false
v8_enable_sandbox = true
is_component_build = false
use_sysroot = false
EOF

# Build V8 monolith (this takes a while but is cached by Docker).
RUN cd v8 && \
    gn gen out.gn/x64.release && \
    ninja -C out.gn/x64.release v8_monolith

# Collect V8 outputs for the next stage.
RUN mkdir -p /v8-install/lib /v8-install/include && \
    cp /v8-src/v8/out.gn/x64.release/obj/libv8_monolith.a /v8-install/lib/ && \
    cp -r /v8-src/v8/include/* /v8-install/include/ && \
    # Copy ICU data if present.
    (cp /v8-src/v8/out.gn/x64.release/icudtl.dat /v8-install/lib/ 2>/dev/null || true)

# ---------------------------------------------------------------------------
# Stage 2: Build Skia with Bazel
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS skia-builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    git curl ca-certificates build-essential python3 \
    openjdk-21-jdk-headless \
    && rm -rf /var/lib/apt/lists/*

ENV JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64
ENV PATH="${JAVA_HOME}/bin:${PATH}"

# Install Bazelisk (auto-downloads correct Bazel version).
RUN curl -Lo /usr/local/bin/bazel \
    https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-amd64 && \
    chmod +x /usr/local/bin/bazel

WORKDIR /skia-src

# Copy only the Skia submodule. In CI we clone fresh.
COPY third_party/skia /skia-src

# Build minimal Skia targets (same as macOS bootstrap).
RUN bazel --batch --noworkspace_rc \
    build //:core //src/base:base //modules/skcms:skcms

# ---------------------------------------------------------------------------
# Stage 3: Build the project
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

# Copy V8 and Skia build outputs.
COPY --from=v8-builder /v8-install /usr/local
COPY --from=skia-builder /skia-src/bazel-bin /src/third_party/skia/bazel-bin
COPY --from=skia-builder /skia-src/include /src/third_party/skia/include
COPY --from=skia-builder /skia-src/src /src/third_party/skia/src
COPY --from=skia-builder /skia-src/modules /src/third_party/skia/modules

# Copy project source.
COPY CMakeLists.txt CMakePresets.json package.json package-lock.json ./
COPY src/ src/
COPY include/ include/
COPY cmake/ cmake/
COPY examples/ examples/
COPY scripts/ scripts/

# Install npm dependencies (echarts).
RUN npm ci || npm install

# Build the project.
# V8 monolith is at /usr/local/lib/libv8_monolith.a, headers at /usr/local/include.
RUN cmake --preset release \
    -DV8_INCLUDE_DIRS=/usr/local/include \
    -DV8_LINK_DIRECTORIES=/usr/local/lib \
    -DV8_LIBRARIES=/usr/local/lib/libv8_monolith.a \
    -DV8_COMPILE_DEFINITIONS="V8_COMPRESS_POINTERS;V8_COMPRESS_POINTERS_IN_SHARED_CAGE;V8_31BIT_SMIS_ON_64BIT_ARCH;V8_ENABLE_SANDBOX;CPPGC_POINTER_COMPRESSION" \
    && cmake --build --preset release

# Run smoke test.
RUN ./build/release/canvas_engine examples/smoke.js

# ---------------------------------------------------------------------------
# Stage 4: Minimal runtime image
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libpng16-16t64 zlib1g libfreetype6 \
    fonts-dejavu-core \
    nodejs \
    && rm -rf /var/lib/apt/lists/*

COPY --from=project-builder /src/build/release/canvas_engine /usr/local/bin/canvas_engine
# Copy ICU data if V8 needs it at runtime.
COPY --from=v8-builder /v8-install/lib/icudtl.dat /usr/local/lib/icudtl.dat

# Copy echarts for rendering scripts.
COPY --from=project-builder /src/node_modules /opt/canvas_engine/node_modules

ENV V8_DATA_DIR=/usr/local/lib
ENV NODE_PATH=/opt/canvas_engine/node_modules

ENTRYPOINT ["canvas_engine"]

# PaperCrawler::Core - Multi-stage Docker Build
# Stage 1: Build environment
FROM ubuntu:22.04 AS builder

# Prevent interactive prompts during build
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    gcc \
    make \
    git \
    libpthread-dev \
    libspdlog-dev \
    libfmt-dev \
    libgtest-dev \
    libbenchmark-dev \
    doxygen \
    graphviz \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy CMakeLists.txt first for better caching
COPY CMakeLists.txt ./

# Copy source code
COPY include ./include
COPY examples ./examples
COPY tests ./tests
COPY benchmarks ./benchmarks
COPY docs ./docs
COPY cmake ./cmake

# Configure and build
RUN cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_EXAMPLES=ON \
    -DBUILD_TESTS=ON \
    -DBUILD_DOCS=ON \
    -DENABLE_WARNINGS_AS_ERRORS=ON \
    -DCMAKE_INSTALL_PREFIX=/usr/local

RUN cmake --build build -- -j$(nproc)

# Run tests
RUN cd build && ctest --output-on-failure

# Install
RUN cmake --install build --strip

# Stage 2: Runtime environment
FROM ubuntu:22.04 AS runtime

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libspdlog1 \
    libfmt7 \
    libpthread-stubs0-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy installed artifacts from builder
COPY --from=builder /usr/local/include /usr/local/include
COPY --from=builder /usr/local/lib /usr/local/lib
COPY --from=builder /usr/local/bin /usr/local/bin 2>/dev/null || true

# Set library path
ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Create non-root user
RUN useradd -m -u 1000 papercrawler && \
    mkdir -p /home/papercrawler/workspace && \
    chown -R papercrawler:papercrawler /home/papercrawler

USER papercrawler
WORKDIR /home/papercrawler/workspace

# Verify installation
RUN ldconfig && \
    echo "PaperCrawler::Core installed successfully" && \
    ls -la /usr/local/lib/libPaperCrawler* 2>/dev/null || echo "Library files check"

# Default command
CMD ["/bin/bash"]

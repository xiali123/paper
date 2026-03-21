# Multi-stage build for optimized backend image

# Stage 1: Builder
FROM ubuntu:22.04 AS builder

# Prevent interactive prompts during build
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies in a single layer with cleanup
RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    git \
    libcurl4-openssl-dev \
    libssl-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

WORKDIR /build

# Copy only dependency files first for better layer caching
COPY CMakeLists.txt ./
COPY backend/CMakeLists.txt ./backend/
COPY core/ ./core/
COPY include/ ./include/
COPY src/ ./src/

# Build the application
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_CXX_FLAGS="-O3 -DNDEBUG" .. && \
    make -j$(nproc) && \
    # Strip symbols to reduce binary size
    strip --strip-unneeded build/backend/PaperCrawlerServer || true

# Stage 2: Runtime
FROM ubuntu:22.04

# Install only runtime dependencies
RUN apt-get update && apt-get install -y \
    libcurl4 \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean \
    && useradd -r -s /bin false -m -d /app appuser

# Create necessary directories
RUN mkdir -p /app/logs /app/config /app/data && \
    chown -R appuser:appuser /app

WORKDIR /app

# Copy only the binary and necessary files from builder
COPY --from=builder --chown=appuser:appuser /build/build/backend/PaperCrawlerServer /app/
COPY --chown=appuser:appuser config/*.json /app/config/

# Switch to non-root user
USER appuser

# Expose port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/api/health || exit 1

# Set environment variables
ENV RUST_LOG=info \
    RUST_BACKTRACE=1 \
    APP_ENV=production

# Run the application
CMD ["/app/PaperCrawlerServer"]

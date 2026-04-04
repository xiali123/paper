# PaperCrawler::Core - Docker Guide

This guide explains how to use Docker to build, test, and deploy PaperCrawler::Core.

---

## 🐳 Quick Start

### Prerequisites

- Docker Engine 20.10+
- Docker Compose 2.0+

### Verify Docker Installation

```bash
docker --version
docker-compose --version
```

---

## 📦 Available Docker Images

PaperCrawler::Core provides three types of Docker images:

1. **Production Image** (Multi-stage build)
   - Minimal runtime image
   - Only runtime dependencies
   - Optimized size

2. **Development Image** (`Dockerfile.dev`)
   - Full development toolchain
   - Debuggers, profilers, editors
   - Live code mounting

3. **Builder Image** (Stage in `Dockerfile`)
   - Complete build environment
   - Runs tests and benchmarks
   - Generates documentation

---

## 🚀 Common Workflows

### 1. Build Production Image

```bash
docker build -t papercrawler-core:latest .
```

**Multi-stage build** creates a minimal production image (~100MB).

### 2. Run Production Container

```bash
docker run --rm -it papercrawler-core:latest
```

### 3. Development Environment

Start a development container with live code mounting:

```bash
docker-compose up dev
```

This mounts your local source code into `/workspace` in the container.

### 4. Build and Test

Run all tests in an isolated environment:

```bash
docker-compose up test
```

Test results are saved to `./test-results/` on your host.

### 5. Generate Documentation

```bash
docker-compose up docs
```

Documentation is generated in `./docs-output/`.

### 6. Run Benchmarks

```bash
docker-compose up benchmark
```

Benchmark results are saved as JSON in `./benchmark-results/`.

### 7. Run Examples

```bash
docker-compose up examples
```

Runs all example programs sequentially.

---

## 🔧 Development Workflow

### Interactive Development

1. **Start development container**:
   ```bash
   docker-compose run --rm dev bash
   ```

2. **Inside the container, build the project**:
   ```bash
   cd /workspace
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   make -j$(nproc)
   ```

3. **Run tests**:
   ```bash
   ./PaperCrawlerCoreTests
   ```

4. **Make code changes** in your editor (changes are instantly reflected in the container)

5. **Rebuild and test**:
   ```bash
   make && ./PaperCrawlerCoreTests
   ```

### Debugging with Valgrind

```bash
docker-compose run --rm dev bash
valgrind --leak-check=full ./build/PaperCrawlerCoreTests
```

### Performance Profiling

```bash
docker-compose run --rm dev bash
# Build with profiling flags
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make
# Run with perf (Linux)
perf record ./build/PaperCrawlerCoreTests
perf report
```

---

## 🏗️ Multi-Stage Build Details

### Stage 1: Builder

**Base image**: `ubuntu:22.04`

**Installed tools**:
- cmake, g++, make
- libspdlog-dev, libfmt-dev
- libgtest-dev, libbenchmark-dev
- doxygen, graphviz

**Build commands**:
```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_TESTS=ON \
  -DBUILD_DOCS=ON
cmake --build build -- -j$(nproc)
cd build && ctest --output-on-failure
cmake --install build --strip
```

### Stage 2: Runtime

**Base image**: `ubuntu:22.04`

**Installed**:
- Only runtime dependencies (libspdlog, libfmt)
- Compiled libraries and headers
- Example executables

**Optimizations**:
- Stripped symbols (`--strip`)
- Minimal packages
- Non-root user for security

---

## 📊 Docker Compose Services

### dev

Development environment with live code mounting.

**Features**:
- Source code mounted as volume
- Sudo access for user `developer`
- Build cache volume for faster rebuilds
- Sanitizers enabled by default

**Usage**:
```bash
docker-compose up dev
# Attach to running container
docker exec -it papercrawler-core-dev bash
```

### test

Build and run all tests.

**Features**:
- Runs tests with coverage
- Sanitizers enabled
- Test results saved to host

**Usage**:
```bash
docker-compose up test
# View test results
cat test-results/*.log
```

### docs

Generate API documentation.

**Features**:
- Runs Doxygen
- HTML output
- Saved to `./docs-output/`

**Usage**:
```bash
docker-compose up docs
# Open documentation
open docs-output/html/index.html  # macOS
xdg-open docs-output/html/index.html  # Linux
```

### benchmark

Run performance benchmarks.

**Features**:
- Release build with optimizations
- Google Benchmark framework
- JSON output for analysis

**Usage**:
```bash
docker-compose up benchmark
# View results
cat benchmark-results/results.json
```

### examples

Build and run all example programs.

**Features**:
- Demonstrates all core components
- Shows real-world usage

**Usage**:
```bash
docker-compose up examples
```

### prod

Production-ready runtime image.

**Features**:
- Minimal size
- Only runtime dependencies
- Non-root user

**Usage**:
```bash
docker-compose up prod
# Or build and push to registry
docker build -t your-registry/papercrawler-core:latest .
docker push your-registry/papercrawler-core:latest
```

---

## 🔍 Troubleshooting

### Build Cache Issues

If you encounter caching issues:

```bash
# Remove build cache volume
docker-compose down -v
# Rebuild without cache
docker-compose build --no-cache
```

### Permission Issues

If you encounter permission issues with generated files:

```bash
# Fix ownership on host
sudo chown -R $USER:$USER .

# Or run container with your user ID
docker-compose run --rm -u $(id -u):$(id -g) dev bash
```

### Out of Memory

If build runs out of memory:

```bash
# Increase Docker memory limit (Docker Desktop)
# Or limit parallel jobs
docker-compose run --rm dev bash -c "cmake --build build -- -j2"
```

### Slow Build on macOS

Use Docker's build cache:

```bash
# Use BuildKit for faster builds
DOCKER_BUILDKIT=1 docker build -t papercrawler-core:latest .
```

---

## 🎯 Best Practices

### 1. Use BuildKit

Enable BuildKit for faster builds:

```bash
export DOCKER_BUILDKIT=1
docker build -t papercrawler-core:latest .
```

### 2. Layer Caching

Order Dockerfile commands from least to most frequently changed:

1. Install dependencies (rarely changes)
2. Copy CMakeLists.txt (sometimes changes)
3. Copy source code (frequently changes)

### 3. Multi-stage Builds

Always use multi-stage builds for production to minimize image size.

### 4. Non-Root User

Run containers as non-root user for security:

```dockerfile
RUN useradd -m -u 1000 papercrawler
USER papercrawler
```

### 5. Volume Mounts

Use named volumes for build cache:

```yaml
volumes:
  build-cache:
    driver: local
```

---

## 🚢 Deployment

### Docker Hub

```bash
# Tag and push
docker tag papercrawler-core:latest your-username/papercrawler-core:1.0.0
docker push your-username/papercrawler-core:1.0.0
```

### Private Registry

```bash
# Login to registry
docker login your-registry.com

# Tag and push
docker tag papercrawler-core:latest your-registry.com/papercrawler-core:latest
docker push your-registry.com/papercrawler-core:latest
```

### Docker Compose in Production

```yaml
services:
  papercrawler-core:
    image: papercrawler-core:latest
    restart: unless-stopped
    environment:
      - LD_LIBRARY_PATH=/usr/local/lib
    volumes:
      - ./config:/etc/papercrawler
      - ./logs:/var/log/papercrawler
```

---

## 🔗 Integration with CI/CD

### GitHub Actions

```yaml
name: Docker Build and Test

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build and test
        run: docker-compose up test
```

### GitLab CI

```yaml
test:
  image: docker:latest
  services:
    - docker:dind
  script:
    - docker-compose up test
```

---

## 📚 References

- [Docker Documentation](https://docs.docker.com/)
- [Docker Compose Documentation](https://docs.docker.com/compose/)
- [Multi-stage Builds](https://docs.docker.com/develop/develop-images/multistage-build/)
- [Docker Best Practices](https://docs.docker.com/develop/dev-best-practices/)

---

**PaperCrawler::Core - Containerized for easy deployment!** 🐳✨

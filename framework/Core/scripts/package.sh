#!/bin/bash
# PaperCrawler::Core Package Script
# Creates distribution packages for multiple platforms

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
VERSION="${VERSION:-$(git describe --tags --abbrev=0 2>/dev/null || echo "1.0.0")"
BUILD_DIR="${BUILD_DIR:-build}"
OUTPUT_DIR="${OUTPUT_DIR:-dist}"
PACKAGE_NAME="papercrawler-core"

# Functions
print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

# Parse arguments
CLEAN_BUILD=false
CREATE_DOCKER=false
CREATE_SOURCE=false
CREATE_BINARY=true

while [[ $# -gt 0 ]]; do
    case $1 in
        --version)
            VERSION="$2"
            shift 2
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --docker)
            CREATE_DOCKER=true
            shift
            ;;
        --source)
            CREATE_SOURCE=true
            shift
            ;;
        --no-binary)
            CREATE_BINARY=false
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --version VERSION  Set version (default: git tag)"
            echo "  --clean            Clean build before packaging"
            echo "  --docker           Create Docker packages"
            echo "  --source           Create source packages"
            echo "  --no-binary        Skip binary packages"
            echo "  --help             Show this help"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Print configuration
print_header "PaperCrawler::Core Packaging"
echo -e "Version:       ${GREEN}${VERSION}${NC}"
echo -e "Output dir:    ${GREEN}${OUTPUT_DIR}${NC}"
echo -e "Clean build:   ${GREEN}${CLEAN_BUILD}${NC}"
echo -e "Docker:        ${GREEN}${CREATE_DOCKER}${NC}"
echo -e "Source:        ${GREEN}${CREATE_SOURCE}${NC}"
echo -e "Binary:        ${GREEN}${CREATE_BINARY}${NC}"
echo ""

# Clean build if requested
if [ "$CLEAN_BUILD" = true ]; then
    print_header "Cleaning build directory"
    rm -rf "${BUILD_DIR}"
    print_success "Build directory cleaned"
fi

# Detect platform
OS="$(uname -s)"
ARCH="$(uname -m)"

case "$OS" in
    Linux)
        PLATFORM="linux"
        PACKAGE_EXT="tar.gz"
        ;;
    Darwin)
        PLATFORM="macos"
        PACKAGE_EXT="tar.gz"
        ;;
    MINGW*|MSYS*|CYGWIN*)
        PLATFORM="windows"
        PACKAGE_EXT="zip"
        ;;
    *)
        print_error "Unknown platform: $OS"
        exit 1
        ;;
esac

# Normalize architecture name
case "$ARCH" in
    x86_64)
        ARCH="x64"
        ;;
    aarch64|arm64)
        ARCH="arm64"
        ;;
    i686|i386)
        ARCH="x86"
        ;;
esac

PLATFORM_ARCH="${PLATFORM}-${ARCH}"

print_success "Detected platform: ${PLATFORM_ARCH}"

# Create output directory
mkdir -p "${OUTPUT_DIR}"

# ============================================================================
# Build Binary Package
# ============================================================================

if [ "$CREATE_BINARY" = true ]; then
    print_header "Building Binary Package"

    # Build
    print_success "Configuring CMake..."
    cmake -B "${BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_EXAMPLES=ON \
        -DBUILD_TESTS=OFF \
        -DCMAKE_INSTALL_PREFIX="${OUTPUT_DIR}/install" \
        -G Ninja

    print_success "Compiling..."
    cmake --build "${BUILD_DIR}" -j$(nproc)

    print_success "Installing..."
    cmake --install "${BUILD_DIR}"

    # Create package
    print_success "Creating package archive..."

    cd "${OUTPUT_DIR}/install"

    if [ "$PLATFORM" = "windows" ]; then
        # Windows: Create ZIP
        zip -r "../${PACKAGE_NAME}-${VERSION}-${PLATFORM_ARCH}.${PACKAGE_EXT}" .
    else
        # Unix: Create tar.gz
        tar -czvf "../${PACKAGE_NAME}-${VERSION}-${PLATFORM_ARCH}.${PACKAGE_EXT}" .
    fi

    cd - > /dev/null

    # Generate checksums
    print_success "Generating checksums..."
    cd "${OUTPUT_DIR}"
    shasum -a 256 "${PACKAGE_NAME}-${VERSION}-${PLATFORM_ARCH}.${PACKAGE_EXT}" > checksums.txt
    cd - > /dev/null

    print_success "Binary package created: ${OUTPUT_DIR}/${PACKAGE_NAME}-${VERSION}-${PLATFORM_ARCH}.${PACKAGE_EXT}"
fi

# ============================================================================
# Create Source Package
# ============================================================================

if [ "$CREATE_SOURCE" = true ]; then
    print_header "Creating Source Package"

    # Create temporary directory
    TEMP_DIR=$(mktemp -d)
    SOURCE_DIR="${TEMP_DIR}/${PACKAGE_NAME}-${VERSION}"

    # Copy source files
    mkdir -p "${SOURCE_DIR}"
    git archive --format=tar HEAD | tar -x -C "${SOURCE_DIR}"

    # Create archive
    print_success "Creating source archive..."
    cd "${TEMP_DIR}"
    tar -czvf "${OUTPUT_DIR}/${PACKAGE_NAME}-${VERSION}-source.tar.gz" "${PACKAGE_NAME}-${VERSION}"
    cd - > /dev/null

    # Generate checksum
    cd "${OUTPUT_DIR}"
    shasum -a 256 "${PACKAGE_NAME}-${VERSION}-source.tar.gz" >> checksums.txt
    cd - > /dev/null

    # Cleanup
    rm -rf "${TEMP_DIR}"

    print_success "Source package created: ${OUTPUT_DIR}/${PACKAGE_NAME}-${VERSION}-source.tar.gz"
fi

# ============================================================================
# Create Docker Package
# ============================================================================

if [ "$CREATE_DOCKER" = true ]; then
    print_header "Building Docker Images"

    # Check if Docker is available
    if ! command -v docker &> /dev/null; then
        print_warning "Docker not found, skipping Docker packages"
    else
        # Build runtime image
        print_success "Building runtime image..."
        docker build \
            --target runtime \
            -t "${PACKAGE_NAME}:${VERSION}" \
            -t "${PACKAGE_NAME}:latest" \
            .

        # Save image to tarball
        print_success "Exporting Docker image..."
        docker save "${PACKAGE_NAME}:${VERSION}" | gzip > "${OUTPUT_DIR}/${PACKAGE_NAME}-${VERSION}-docker.tar.gz"

        # Generate checksum
        cd "${OUTPUT_DIR}"
        shasum -a 256 "${PACKAGE_NAME}-${VERSION}-docker.tar.gz" >> checksums.txt
        cd - > /dev/null

        print_success "Docker package created: ${OUTPUT_DIR}/${PACKAGE_NAME}-${VERSION}-docker.tar.gz"
    fi
fi

# ============================================================================
# Create Package Manifest
# ============================================================================

print_header "Creating Package Manifest"

cat > "${OUTPUT_DIR}/manifest.json" <<EOF
{
  "name": "${PACKAGE_NAME}",
  "version": "${VERSION}",
  "build_date": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
  "platform": "${PLATFORM_ARCH}",
  "packages": [
$(if [ "$CREATE_BINARY" = true ]; then
    echo "    {"
    echo "      \"type\": \"binary\","
    echo "      \"file\": \"${PACKAGE_NAME}-${VERSION}-${PLATFORM_ARCH}.${PACKAGE_EXT}\","
    echo "      \"sha256\": \"$(shasum -a 256 "${OUTPUT_DIR}/${PACKAGE_NAME}-${VERSION}-${PLATFORM_ARCH}.${PACKAGE_EXT}" | cut -d' ' -f1)\""
    echo "    },"
fi
$(if [ "$CREATE_SOURCE" = true ]; then
    echo "    {"
    echo "      \"type\": \"source\","
    echo "      \"file\": \"${PACKAGE_NAME}-${VERSION}-source.tar.gz\","
    echo "      \"sha256\": \"$(shasum -a 256 "${OUTPUT_DIR}/${PACKAGE_NAME}-${VERSION}-source.tar.gz" | cut -d' ' -f1)\""
    echo "    },"
fi
$(if [ "$CREATE_DOCKER" = true ] && command -v docker &> /dev/null; then
    echo "    {"
    echo "      \"type\": \"docker\","
    echo "      \"file\": \"${PACKAGE_NAME}-${VERSION}-docker.tar.gz\","
    echo "      \"sha256\": \"$(shasum -a 256 "${OUTPUT_DIR}/${PACKAGE_NAME}-${VERSION}-docker.tar.gz" | cut -d' ' -f1)\""
    echo "    },"
fi
    null
  ]
}
EOF

# Remove trailing comma and null
sed -i 's/,    null$/]/' "${OUTPUT_DIR}/manifest.json"

print_success "Package manifest created: ${OUTPUT_DIR}/manifest.json"

# ============================================================================
# Summary
# ============================================================================

print_header "Packaging Complete!"

echo ""
echo "Created packages:"
ls -lh "${OUTPUT_DIR}"

echo ""
echo "Checksums:"
cat "${OUTPUT_DIR}/checksums.txt"

echo ""
print_success "All packages created successfully in ${OUTPUT_DIR}/"

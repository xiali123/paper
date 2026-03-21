#!/bin/bash
# ============================================
# PaperCrawler Library Build Script
# ============================================
# Builds both static and shared libraries
# and creates a complete distribution
# ============================================

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
DIST_DIR="$PROJECT_ROOT/dist"

echo ""
echo "============================================"
echo "  PaperCrawler Library Build Script"
echo "============================================"
echo ""

# Parse command line arguments
BUILD_TYPE="Release"
BUILD_STATIC="ON"
BUILD_SHARED="ON"
BUILD_DESKTOP="ON"

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --static-only)
            BUILD_SHARED="OFF"
            shift
            ;;
        --shared-only)
            BUILD_STATIC="OFF"
            shift
            ;;
        --no-desktop)
            BUILD_DESKTOP="OFF"
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --debug         Build debug version (default: release)"
            echo "  --static-only   Build only static libraries"
            echo "  --shared-only   Build only shared libraries"
            echo "  --no-desktop    Don't build desktop application"
            echo "  --help          Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo "Build Configuration:"
echo "  Build Type: $BUILD_TYPE"
echo "  Static Libs: $BUILD_STATIC"
echo "  Shared Libs: $BUILD_SHARED"
echo "  Desktop App: $BUILD_DESKTOP"
echo ""

# Clean build directory
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo ""
echo "[1/4] Configuring CMake..."
echo ""

# Configure CMake
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_STATIC="$BUILD_STATIC" \
    -DBUILD_SHARED="$BUILD_SHARED" \
    -DBUILD_DESKTOP="$BUILD_DESKTOP"

echo ""
echo "[2/4] Building project..."
echo ""

# Build the project
cmake --build . --config "$BUILD_TYPE" --parallel $(nproc)

echo ""
echo "[3/4] Running tests (if enabled)..."
echo ""

# Run tests if they exist
if [ -f "bin/test_paper_api" ] || [ -f "bin/test_paper_api.exe" ]; then
    ctest --output-on-failure || true
fi

echo ""
echo "[4/4] Creating distribution packages..."
echo ""

# Create distribution directory
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
DIST_BASE="$DIST_DIR/$TIMESTAMP"
mkdir -p "$DIST_BASE"

# Package static libraries
if [ "$BUILD_STATIC" = "ON" ]; then
    echo "Packaging static libraries..."
    STATIC_DIR="$DIST_BASE/static"
    mkdir -p "$STATIC_DIR"/{lib,include,bin,config,sql,docs}

    # Copy static libraries
    find lib -name "*.a" -exec cp {} "$STATIC_DIR/lib/" \;

    # Copy headers
    cp -r "$PROJECT_ROOT/include/"* "$STATIC_DIR/include/"

    # Copy executables
    find bin -type f -executable -exec cp {} "$STATIC_DIR/bin/" \;

    # Copy config and docs
    cp "$PROJECT_ROOT/config"/*.json "$STATIC_DIR/config/" 2>/dev/null || true
    cp "$PROJECT_ROOT/sql"/*.sql "$STATIC_DIR/sql/" 2>/dev/null || true
    cp "$PROJECT_ROOT"/*.md "$STATIC_DIR/docs/" 2>/dev/null || true

    # Create README
    cat > "$STATIC_DIR/README.txt" << EOF
PaperCrawler Static Libraries Distribution
Build Date: $(date)
Build Type: $BUILD_TYPE

This package contains:
- Static libraries (.a files)
- Header files
- Executables (statically linked)
- Configuration files
- SQL scripts

To use these libraries:
1. Include headers in your project
2. Link against the .a files
3. No runtime dependencies required!

EOF
fi

# Package shared libraries
if [ "$BUILD_SHARED" = "ON" ]; then
    echo "Packaging shared libraries..."
    SHARED_DIR="$DIST_BASE/shared"
    mkdir -p "$SHARED_DIR"/{lib,include,bin,config,sql,docs}

    # Copy shared libraries
    find lib -name "*.so*" -o -name "*.dll" -exec cp {} "$SHARED_DIR/lib/" \;

    # Copy headers and executables
    cp -r "$PROJECT_ROOT/include/"* "$SHARED_DIR/include/"
    find bin -type f -executable -exec cp {} "$SHARED_DIR/bin/" \;

    # Copy config and docs
    cp "$PROJECT_ROOT/config"/*.json "$SHARED_DIR/config/" 2>/dev/null || true
    cp "$PROJECT_ROOT/sql"/*.sql "$SHARED_DIR/sql/" 2>/dev/null || true
    cp "$PROJECT_ROOT"/*.md "$SHARED_DIR/docs/" 2>/dev/null || true

    # Create README
    cat > "$SHARED_DIR/README.txt" << EOF
PaperCrawler Shared Libraries Distribution
Build Date: $(date)
Build Type: $BUILD_TYPE

This package contains:
- Shared libraries (.so/.dll files)
- Header files
- Executables
- Configuration files
- SQL scripts

To use these libraries:
1. Include headers in your project
2. Link against the shared libraries
3. Ensure .so/.dll files are in runtime path
EOF
fi

# Create latest symlink
rm -f "$DIST_DIR/latest"
ln -s "$TIMESTAMP" "$DIST_DIR/latest"

cd "$PROJECT_ROOT"

echo ""
echo "============================================"
echo "  Build Complete!"
echo "============================================"
echo ""
echo "Build artifacts:"
ls -lh "$BUILD_DIR/bin/" 2>/dev/null || echo "  No executables found"
echo ""
echo "Library files:"
ls -lh "$BUILD_DIR/lib/" 2>/dev/null || echo "  No libraries found"
echo ""
echo "Distribution packages created in:"
echo "  $DIST_BASE"
echo "  $DIST_DIR/latest -> $TIMESTAMP"
echo ""
echo "Static build: $BUILD_STATIC"
echo "Shared build: $BUILD_SHARED"
echo ""
echo "============================================"
echo ""

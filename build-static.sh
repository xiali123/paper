#!/bin/bash
# ============================================
# PaperCrawler Static Build Script (Linux)
# ============================================
# This script builds a statically linked
# executable that includes all dependencies
# ============================================

set -e

echo ""
echo "============================================"
echo "  PaperCrawler Static Build (Linux)"
echo "============================================"
echo ""

# Create build directory
mkdir -p build-static
cd build-static

echo ""
echo "[1/5] Configuring CMake for static build..."
echo ""

# Configure CMake with static linking options
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" \
    -DCMAKE_FIND_LIBRARY_SUFFIXES=".a"

echo ""
echo "[2/5] Building static executable..."
echo ""

# Build the project
cmake --build . --config Release --parallel $(nproc)

echo ""
echo "[3/5] Stripping debug symbols..."
echo ""

# Strip symbols to reduce size
strip --strip-unneeded bin/PaperCrawlerServer || true

echo ""
echo "[4/5] Copying runtime files..."
echo ""

# Copy configuration files
mkdir -p bin/config
cp ../config/config.json.example bin/config/config.json

# Copy SQL scripts
mkdir -p bin/sql
cp ../sql/optimize.sql bin/sql/

echo ""
echo "[5/5] Creating distribution package..."
echo ""

# Create distribution directory
DIST_DIR="PaperCrawler-Linux-x64-$(date +%Y%m%d)"
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"

# Copy files
mkdir -p "$DIST_DIR/bin"
mkdir -p "$DIST_DIR/config"
mkdir -p "$DIST_DIR/sql"
mkdir -p "$DIST_DIR/docs"

cp bin/PaperCrawlerServer "$DIST_DIR/bin/"
cp bin/config/config.json "$DIST_DIR/config/"
cp ../sql/optimize.sql "$DIST_DIR/sql/"
cp ../README.md "$DIST_DIR/docs/"
cp ../OPTIMIZATION-SUMMARY.md "$DIST_DIR/docs/"

# Create README
cat > "$DIST_DIR/README.txt" << EOF
PaperCrawler Standalone Distribution

Build Date: $(date)
Version: 1.0.0

This is a statically linked build - all libraries are included.
No external dependencies required!

Quick Start:
1. Edit config/config.json with your database settings
2. Run: chmod +x bin/PaperCrawlerServer
3. Run: ./bin/PaperCrawlerServer
4. Open http://localhost:8080 in your browser

EOF

cd ..

echo ""
echo "============================================"
echo "  Build Complete!"
echo "============================================"
echo ""
echo "Output: $DIST_DIR/"
echo ""
echo "Executable: build-static/bin/PaperCrawlerServer"
echo ""
echo "This is a STATIC build - all dependencies are included!"
echo "You can run this on any Linux machine with same architecture."
echo ""
echo "============================================"
echo ""

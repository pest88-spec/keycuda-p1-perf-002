#!/bin/bash
#
# Setup secp256k1-zkp in third_party directory
#
# Purpose: Download official secp256k1-zkp repository to fix missing *_impl.h headers
# Date: 2025-10-18
#

set -e

echo "========================================="
echo "  secp256k1-zkp Setup Script"
echo "========================================="
echo ""

PROJECT_ROOT="/mnt/d/mybitcoin/puzzlekeyhunt/PuzzleKeyhunt"

cd "$PROJECT_ROOT"

# Check if third_party directory exists
if [ ! -d "third_party" ]; then
    echo "Creating third_party directory..."
    mkdir -p third_party
fi

cd third_party

# Check if secp256k1-zkp already exists
if [ -d "secp256k1-zkp" ]; then
    echo "⚠️  third_party/secp256k1-zkp already exists"
    echo "Do you want to remove and re-download? (y/n)"
    read -r response
    if [ "$response" = "y" ]; then
        echo "Removing existing secp256k1-zkp..."
        rm -rf secp256k1-zkp
    else
        echo "Keeping existing secp256k1-zkp"
        exit 0
    fi
fi

echo "Downloading secp256k1-zkp from GitHub..."
echo "URL: https://github.com/ElementsProject/secp256k1-zkp.git"
echo ""

# Try using git submodule first
cd "$PROJECT_ROOT"
if git submodule update --init --recursive third_party/secp256k1-zkp 2>/dev/null; then
    echo "✓ Downloaded via git submodule"
else
    echo "⚠️  Git submodule failed, trying direct clone..."
    cd third_party
    if git clone --depth 1 https://github.com/ElementsProject/secp256k1-zkp.git; then
        echo "✓ Downloaded via git clone"
    else
        echo "❌ Failed to download secp256k1-zkp"
        exit 1
    fi
fi

echo ""
echo "Verifying download..."

# Check critical files
cd "$PROJECT_ROOT/third_party/secp256k1-zkp"

CRITICAL_FILES=(
    "src/secp256k1.c"
    "src/field_impl.h"
    "src/int128_impl.h"
    "include/secp256k1.h"
)

ALL_OK=true
for file in "${CRITICAL_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ $file"
    else
        echo "✗ $file (MISSING)"
        ALL_OK=false
    fi
done

echo ""

if [ "$ALL_OK" = true ]; then
    echo "========================================="
    echo "✅ secp256k1-zkp setup successful!"
    echo "========================================="
    echo ""
    echo "Downloaded to: $PROJECT_ROOT/third_party/secp256k1-zkp"
    echo ""
    echo "File count:"
    echo "  - Headers: $(find include -name "*.h" | wc -l)"
    echo "  - Sources: $(find src -name "*.c" | wc -l)"
    echo "  - Implementations: $(find src -name "*_impl.h" | wc -l)"
    echo ""
    echo "Next step: Rebuild the project"
    echo "  cd $PROJECT_ROOT"
    echo "  rm -rf build && mkdir build && cd build"
    echo "  cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON"
    echo "  make -j\$(nproc)"
else
    echo "========================================="
    echo "❌ secp256k1-zkp setup incomplete"
    echo "========================================="
    echo ""
    echo "Some critical files are missing. Please check your internet connection"
    echo "and try running this script again."
    exit 1
fi

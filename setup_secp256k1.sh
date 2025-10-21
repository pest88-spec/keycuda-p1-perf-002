#!/bin/bash
#
# Setup secp256k1 libraries in third_party directory
#
# Priority: bitcoin-core-secp256k1 > secp256k1-zkp
# Date: 2025-10-18
#

set -e

echo "========================================="
echo "  secp256k1 Libraries Setup Script"
echo "========================================="
echo ""

PROJECT_ROOT="/mnt/d/mybitcoin/puzzlekeyhunt/PuzzleKeyhunt"

cd "$PROJECT_ROOT"

# Create third_party directory if not exists
mkdir -p third_party
cd third_party

echo "According to CMakeLists.txt priority:"
echo "  1. bitcoin-core-secp256k1 (PREFERRED)"
echo "  2. secp256k1-zkp (FALLBACK)"
echo ""

# Option 1: Download bitcoin-core-secp256k1 (RECOMMENDED)
echo "========================================="
echo "Option 1: bitcoin-core-secp256k1 (RECOMMENDED)"
echo "========================================="
echo ""

if [ -d "bitcoin-core-secp256k1" ]; then
    echo "✓ bitcoin-core-secp256k1 already exists"
else
    echo "Downloading bitcoin-core-secp256k1 from GitHub..."
    echo "URL: https://github.com/bitcoin-core/secp256k1.git"
    echo ""

    if git clone --depth 1 https://github.com/bitcoin-core/secp256k1.git bitcoin-core-secp256k1; then
        echo "✓ bitcoin-core-secp256k1 downloaded successfully"
    else
        echo "❌ Failed to download bitcoin-core-secp256k1"
        echo "Trying git submodule method..."
        cd "$PROJECT_ROOT"
        git submodule update --init --recursive third_party/bitcoin-core-secp256k1 || {
            echo "❌ Both methods failed for bitcoin-core-secp256k1"
        }
    fi
fi

echo ""

# Option 2: Download secp256k1-zkp (FALLBACK)
cd "$PROJECT_ROOT/third_party"

echo "========================================="
echo "Option 2: secp256k1-zkp (FALLBACK)"
echo "========================================="
echo ""

if [ -d "secp256k1-zkp" ]; then
    echo "✓ secp256k1-zkp already exists"
else
    echo "Downloading secp256k1-zkp from GitHub..."
    echo "URL: https://github.com/ElementsProject/secp256k1-zkp.git"
    echo ""

    if git clone --depth 1 https://github.com/ElementsProject/secp256k1-zkp.git; then
        echo "✓ secp256k1-zkp downloaded successfully"
    else
        echo "❌ Failed to download secp256k1-zkp"
        echo "Trying git submodule method..."
        cd "$PROJECT_ROOT"
        git submodule update --init --recursive third_party/secp256k1-zkp || {
            echo "❌ Both methods failed for secp256k1-zkp"
        }
    fi
fi

echo ""
echo "========================================="
echo "  Verification"
echo "========================================="
echo ""

cd "$PROJECT_ROOT/third_party"

# Check bitcoin-core-secp256k1
if [ -d "bitcoin-core-secp256k1" ]; then
    if [ -f "bitcoin-core-secp256k1/CMakeLists.txt" ]; then
        echo "✅ bitcoin-core-secp256k1: READY"
        echo "   Location: $PROJECT_ROOT/third_party/bitcoin-core-secp256k1"
        echo "   Files: $(find bitcoin-core-secp256k1/src -name "*.c" -o -name "*.h" 2>/dev/null | wc -l)"
        BITCOIN_CORE_OK=true
    else
        echo "⚠️  bitcoin-core-secp256k1: INCOMPLETE (missing CMakeLists.txt)"
        BITCOIN_CORE_OK=false
    fi
else
    echo "❌ bitcoin-core-secp256k1: NOT FOUND"
    BITCOIN_CORE_OK=false
fi

echo ""

# Check secp256k1-zkp
if [ -d "secp256k1-zkp" ]; then
    if [ -f "secp256k1-zkp/src/secp256k1.c" ] && \
       [ -f "secp256k1-zkp/src/field_impl.h" ] && \
       [ -f "secp256k1-zkp/src/int128_impl.h" ]; then
        echo "✅ secp256k1-zkp: READY"
        echo "   Location: $PROJECT_ROOT/third_party/secp256k1-zkp"
        echo "   Sources: $(find secp256k1-zkp/src -name "*.c" 2>/dev/null | wc -l)"
        echo "   Headers: $(find secp256k1-zkp/include -name "*.h" 2>/dev/null | wc -l)"
        echo "   Implementations: $(find secp256k1-zkp/src -name "*_impl.h" 2>/dev/null | wc -l)"
        ZKP_OK=true
    else
        echo "⚠️  secp256k1-zkp: INCOMPLETE (missing critical files)"
        ZKP_OK=false
    fi
else
    echo "❌ secp256k1-zkp: NOT FOUND"
    ZKP_OK=false
fi

echo ""
echo "========================================="
echo "  Build Configuration"
echo "========================================="
echo ""

if [ "$BITCOIN_CORE_OK" = true ]; then
    echo "✅ CMake will use: bitcoin-core-secp256k1 (PRIORITY 1)"
    echo ""
    echo "Next step: Rebuild the project"
    echo ""
    echo "  cd $PROJECT_ROOT"
    echo "  rm -rf build && mkdir build && cd build"
    echo "  cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON"
    echo "  make -j\$(nproc)"
    echo ""
    echo "Expected message:"
    echo '  -- Using bitcoin-core secp256k1 (external submodule)'
    exit 0
elif [ "$ZKP_OK" = true ]; then
    echo "⚠️  CMake will use: secp256k1-zkp (FALLBACK)"
    echo ""
    echo "Note: bitcoin-core-secp256k1 is preferred but not available."
    echo "Using secp256k1-zkp as fallback."
    echo ""
    echo "Next step: Rebuild the project"
    echo ""
    echo "  cd $PROJECT_ROOT"
    echo "  rm -rf build && mkdir build && cd build"
    echo "  cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON"
    echo "  make -j\$(nproc)"
    echo ""
    echo "Expected message:"
    echo '  -- Using extracted secp256k1-zkp (offline/submodule-free mode)'
    exit 0
else
    echo "❌ No secp256k1 library available!"
    echo ""
    echo "Both bitcoin-core-secp256k1 and secp256k1-zkp failed to download."
    echo "Please check your internet connection and try again."
    echo ""
    echo "Manual download:"
    echo "  cd $PROJECT_ROOT/third_party"
    echo "  git clone https://github.com/bitcoin-core/secp256k1.git bitcoin-core-secp256k1"
    echo "  git clone https://github.com/ElementsProject/secp256k1-zkp.git"
    exit 1
fi

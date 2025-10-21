#!/bin/bash

# Puzzle71 Build Environment Setup Script
# Configures build directory structure and CUDA compilation flags

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/build"

# Functions
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Main setup function
setup_build_environment() {
    log_info "Setting up Puzzle71 build environment..."

    # Create build directory structure
    log_info "Creating build directory structure..."
    mkdir -p "$BUILD_DIR"
    mkdir -p "$BUILD_DIR/coverage"
    mkdir -p "$BUILD_DIR/profiling"
    mkdir -p "$BUILD_DIR/benchmarks"
    mkdir -p "$BUILD_DIR/artifacts"
    mkdir -p "$BUILD_DIR/logs"
    mkdir -p "$BUILD_DIR/third_party"

    # Create CMake configuration presets
    log_info "Creating CMake configuration presets..."

    # Development preset
    cat > "$BUILD_DIR/CMakePresets.json" << 'EOF'
{
  "version": 3,
  "configurePresets": [
    {
      "name": "debug",
      "displayName": "Debug Build",
      "description": "Debug build with full testing and coverage",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_CXX_STANDARD": "20",
        "CMAKE_CUDA_STANDARD": "20",
        "CMAKE_CUDA_ARCHITECTURES": "75;86;89;90",
        "CMAKE_CUDA_SEPARABLE_COMPILATION": "ON",
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
        "BUILD_TESTS": "ON",
        "BUILD_BENCHMARKS": "ON",
        "ENABLE_COVERAGE": "ON",
        "CMAKE_CXX_FLAGS_DEBUG": "-g -O0 -Wall -Wextra -Wpedantic -Werror --coverage -fprofile-arcs -ftest-coverage",
        "CMAKE_CUDA_FLAGS_DEBUG": "-g -G -lineinfo --device-debug",
        "SECP256K1_AVAILABLE": "ON",
        "ENABLE_INTEGRATION_SYSTEM": "ON",
        "OFFLINE_BUILD": "OFF",
        "STRICT_ATTRIBUTION": "ON"
      },
      "toolset": {
        "value": "host",
        "description": "Host toolset"
      }
    },
    {
      "name": "release",
      "displayName": "Release Build",
      "description": "Optimized release build for production",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_CXX_STANDARD": "20",
        "CMAKE_CUDA_STANDARD": "20",
        "CMAKE_CUDA_ARCHITECTURES": "75;86;89;90",
        "CMAKE_CUDA_SEPARABLE_COMPILATION": "ON",
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
        "BUILD_TESTS": "OFF",
        "BUILD_BENCHMARKS": "ON",
        "ENABLE_COVERAGE": "OFF",
        "CMAKE_CXX_FLAGS_RELEASE": "-O3 -DNDEBUG -march=native -mtune=native",
        "CMAKE_CUDA_FLAGS_RELEASE": "-O3 -use_fast_math",
        "SECP256K1_AVAILABLE": "ON",
        "ENABLE_INTEGRATION_SYSTEM": "ON",
        "OFFLINE_BUILD": "OFF",
        "STRICT_ATTRIBUTION": "ON"
      }
    },
    {
      "name": "relwithdebinfo",
      "displayName": "Release with Debug Info",
      "description": "Optimized build with debug information",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "RelWithDebInfo",
        "CMAKE_CXX_STANDARD": "20",
        "CMAKE_CUDA_STANDARD": "20",
        "CMAKE_CUDA_ARCHITECTURES": "75;86;89;90",
        "CMAKE_CUDA_SEPARABLE_COMPILATION": "ON",
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
        "BUILD_TESTS": "ON",
        "BUILD_BENCHMARKS": "ON",
        "ENABLE_COVERAGE": "OFF",
        "CMAKE_CXX_FLAGS_RELWITHDEBINFO": "-O2 -g -DNDEBUG",
        "CMAKE_CUDA_FLAGS_RELWITHDEBINFO": "-O2 -g",
        "SECP256K1_AVAILABLE": "ON",
        "ENABLE_INTEGRATION_SYSTEM": "ON",
        "OFFLINE_BUILD": "OFF",
        "STRICT_ATTRIBUTION": "ON"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "debug-build",
      "configurePreset": "debug",
      "displayName": "Debug Build"
    },
    {
      "name": "release-build",
      "configurePreset": "release",
      "displayName": "Release Build"
    },
    {
      "name": "relwithdebinfo-build",
      "configurePreset": "relwithdebinfo",
      "displayName": "Release with Debug Info Build"
    }
  ],
  "testPresets": [
    {
      "name": "unit-tests",
      "description": "Run unit tests only",
      "configurePreset": "debug",
      "execution": {
        "tests": [
          {
            "name": "techdebt_repair_unit_tests",
            "filter": "Unit.*:Architecture.*"
          }
        ],
        "output": {
          "outputOnFailure": true,
          "shortProgress": true,
          "verbosity": "normal"
        }
      }
    },
    {
      "name": "integration-tests",
      "description": "Run integration tests only",
      "configurePreset": "debug",
      "execution": {
        "tests": [
          {
            "name": "techdebt_repair_integration_tests",
            "filter": "Integration.*:Validation.*"
          }
        ],
        "output": {
          "outputOnFailure": true,
          "shortProgress": true,
          "verbosity": "normal"
        }
      }
    },
    {
      "name": "performance-tests",
      "description": "Run performance tests only",
      "configurePreset": "relwithdebinfo",
      "execution": {
        "tests": [
          {
            "name": "techdebt_repair_performance_tests",
            "filter": "GPUUtilizationTest.*"
          }
        ],
        "output": {
          "outputOnFailure": true,
          "shortProgress": true,
          "verbosity": "normal"
        }
      }
    },
    {
      "name": "all-tests",
      "description": "Run all tests",
      "configurePreset": "debug",
      "execution": {
        "tests": [
          {
            "name": "techdebt_repair_all_tests"
          }
        ],
        "output": {
          "outputOnFailure": true,
          "shortProgress": true,
          "verbosity": "normal"
        }
      }
    }
  ],
  "packagePresets": [
    {
      "name": "development-package",
      "description": "Package for development deployment",
      "configurePreset": "debug",
      "packageDirectory": "${sourceDir}/build/packages",
      "generators": [
        "ZIP"
      ]
    },
    {
      "name": "production-package",
      "description": "Package for production deployment",
      "configurePreset": "release",
      "packageDirectory": "${sourceDir}/build/packages",
      "generators": [
        "ZIP"
      ]
    }
  ]
}
EOF

    # Create CUDA compilation configuration
    log_info "Creating CUDA compilation configuration..."
    cat > "$BUILD_DIR/cuda_build_config.txt" << 'EOF'
# Puzzle71 CUDA Build Configuration
# Optimized for Compute Capability 3.5+ support

# Target CUDA Architectures
CUDA_ARCHITECTURES=75-real 86-real 89-real 90-real

# Compilation Flags
CMAKE_CUDA_FLAGS_DEBUG=-g -G -lineinfo --device-debug
CMAKE_CUDA_FLAGS_RELEASE=-O3 -use_fast_math --ftz=true
CMAKE_CUDA_FLAGS_RELWITHDEBINFO=-O2 -g

# Memory Management
CMAKE_CUDA_SEPARABLE_COMPILATION=ON

# Optimization Settings
ENABLE_FAST_MATH=ON
USE_FAST_MATH=ON

# Profiling
ENABLE_PROFILING=ON
ENABLE_LINEINFO=ON

# Coverage (Debug only)
ENABLE_COVERAGE=OFF

# Target GPUs
# 75 - Turing (RTX 20-series)
# 86 - Ampere (RTX 30-series)
# 89 - Ada Lovelace (RTX 40-series)
# 90 - Hopper (H100, H200)
EOF

    # Create build scripts
    log_info "Creating build scripts..."

    # Debug build script
    cat > "$BUILD_DIR/build_debug.sh" << 'EOF'
#!/bin/bash
# Debug build script for Puzzle71

cd "$(dirname "$0")"
source cmake_build_config.sh

echo "Building Puzzle71 in Debug mode..."
cmake --preset debug
cmake --build --preset debug-build

echo "Build completed in Debug mode"
EOF

    # Release build script
    cat > "$BUILD_DIR/build_release.sh" << 'EOF'
#!/bin/bash
# Release build script for Puzzle71

cd "$(dirname "$0")"
source cmake_build_config.sh

echo "Building Puzzle71 in Release mode..."
cmake --preset release
cmake --build --preset release-build

echo "Build completed in Release mode"
EOF

    # Test script
    cat > "$BUILD_DIR/run_tests.sh" << 'EOF'
#!/bin/bash
# Test runner script for Puzzle71

cd "$(dirname "$0")"

echo "Running Puzzle71 test suite..."

# Run unit tests
echo "Running unit tests..."
ctest --preset unit-tests --output-on-failure

# Run integration tests
echo "Running integration tests..."
ctest --preset integration-tests --output-on-failure

# Run performance tests
echo "Running performance tests..."
ctest --preset performance-tests --output-on-failure

echo "All tests completed"
EOF

    # Benchmark script
    cat > "$BUILD_DIR/run_benchmarks.sh" << 'EOF'
#!/bin/bash
# Benchmark runner script for Puzzle71

cd "$(dirname "$0")"

echo "Running Puzzle71 benchmarks..."

# Check if binary exists
if [ ! -f "Puzzle71Solver" ]; then
    echo "Puzzle71Solver not found. Please build first."
    exit 1
fi

# Run benchmarks
./Puzzle71Solver --benchmark-mode \
    --device-id 0 \
    --duration 60 \
    --output benchmarks_$(date +%Y%m%d_%H%M%S).json

echo "Benchmarks completed"
EOF

    # Make scripts executable
    chmod +x "$BUILD_DIR"/*.sh

    # Create environment configuration
    log_info "Creating environment configuration..."
    cat > "$BUILD_DIR/build_env.sh" << 'EOF'
#!/bin/bash
# Build environment configuration for Puzzle71

# CUDA Environment
export CUDA_HOME="${CUDA_HOME:-/usr/local/cuda}"
export PATH="${CUDA_HOME}/bin:${PATH}"
export LD_LIBRARY_PATH="${CUDA_HOME}/lib64:${LD_LIBRARY_PATH}"

# Build Configuration
export CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-RelWithDebInfo}"
export CMAKE_CUDA_ARCHITECTURES="${CMAKE_CUDA_ARCHITECTURES:-75;86;89;90}"
export CMAKE_CUDA_SEPARABLE_COMPILATION="${CMAKE_CUDA_SEPARABLE_COMPILATION:-ON}"

# Performance Configuration
export CMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS:- -Wall -Wextra -Wpedantic}"
export CMAKE_CUDA_FLAGS="${CMAKE_CUDA_FLAGS:- -lineinfo}"

# Testing Configuration
export GTEST_OUTPUT="${GTEST_OUTPUT:-}"
export CTEST_OUTPUT_ON_FAILURE="${CTEST_OUTPUT_ON_FAILURE:-1}"

# Profiling Configuration
export CUDA_PROFILE="${CUDA_PROFILE:-1}"
export NSIGHT_COMPUTE_ENABLE="${NSIGHT_COMPUTE_ENABLE:-1}"

echo "Build environment configured"
EOF

    chmod +x "$BUILD_DIR/build_env.sh"

    # Create CUDA compilation verification script
    log_info "Creating CUDA compilation verification script..."
    cat > "$BUILD_DIR/verify_cuda_setup.sh" << 'EOF'
#!/bin/bash
# CUDA setup verification script

echo "Verifying CUDA setup..."

# Check CUDA installation
if ! command -v nvcc &> /dev/null; then
    echo "ERROR: nvcc not found. Please install CUDA toolkit."
    exit 1
fi

echo "✓ nvcc found: $(nvcc --version | head -n1)"

# Check CUDA version
CUDA_VERSION=$(nvcc --version | grep "release" | awk '{print $6}' | sed 's/,//')
echo "✓ CUDA version: $CUDA_VERSION"

# Check GPU availability
if ! nvidia-smi &> /dev/null; then
    echo "WARNING: nvidia-smi not found. GPU may not be available."
else
    echo "✓ nvidia-smi found"
    GPU_COUNT=$(nvidia-smi --list-gpus | wc -l)
    echo "✓ GPUs detected: $GPU_COUNT"
fi

# Check compute capabilities
echo "✓ Target CUDA architectures: 75;86;89;90"
echo "  - 75: Turing (RTX 20-series)"
echo "  - 86: Ampere (RTX 30-series)"
echo "  - 89: Ada Lovelace (RTX 40-series)"
echo "  - 90: Hopper (H100, H200)"

# Check compiler
if ! command -v g++ &> /dev/null; then
    echo "ERROR: g++ not found. Please install build-essential."
    exit 1
fi

echo "✓ g++ found: $(g++ --version | head -n1)"

# Check CMake
if ! command -v cmake &> /dev/null; then
    echo "ERROR: cmake not found. Please install CMake."
    exit 1
fi

echo "✓ cmake found: $(cmake --version | head -n1)"

# Check required libraries
echo "Checking required libraries..."

if ldconfig -p | grep -q libssl; then
    echo "✓ OpenSSL found"
else
    echo "WARNING: OpenSSL not found. SSL features may not work."
fi

if ldconfig -p | grep -q libsecp256k1; then
    echo "✓ libsecp256k1 found"
else
    echo "WARNING: libsecp256k1 not found. ECC validation may not work."
fi

echo "CUDA setup verification completed"
EOF

    chmod +x "$BUILD_DIR/verify_cuda_setup.sh"

    log_success "Build environment setup completed successfully!"
    log_info "Created build directory structure with:"
    log_info "  - CMake presets for debug/release/relwithdebinfo builds"
    log_info "  - CUDA configuration optimized for CC 3.5+"
    log_info "  - Build scripts for different configurations"
    log_info "  - Test and benchmark runners"
    log_info "  - CUDA setup verification script"
    log_info ""
    log_info "To use the build environment:"
    log_info "  cd $BUILD_DIR"
    log_info "  source build_env.sh"
    log_info "  ./build_debug.sh    # Debug build"
    log_info "  ./build_release.sh  # Release build"
    log_info "  ./run_tests.sh      # Run tests"
    log_info "  ./verify_cuda_setup.sh  # Verify CUDA setup"
}

# Run setup
setup_build_environment
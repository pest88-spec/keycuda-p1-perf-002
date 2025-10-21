#!/bin/bash

# Puzzle71 CUDA Environment Setup Script
# Verifies and configures CUDA development environment for CC 3.5+ support

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

# Functions
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# CUDA environment verification
verify_cuda_environment() {
    log_info "Verifying CUDA development environment..."

    local errors=0

    # Check NVIDIA driver
    log_info "Checking NVIDIA driver..."
    if ! nvidia-smi &> /dev/null; then
        log_error "NVIDIA driver not found. Please install NVIDIA drivers."
        ((errors++))
    else
        local driver_version=$(nvidia-smi --query-gpu=driver_version --format=csv,noheader | head -n1)
        log_success "NVIDIA driver found: $driver_version"
    fi

    # Check CUDA toolkit
    log_info "Checking CUDA toolkit..."
    if ! command -v nvcc &> /dev/null; then
        log_error "CUDA compiler (nvcc) not found. Please install CUDA toolkit."
        ((errors++))
        return 1
    else
        local cuda_version=$(nvcc --version | grep "release" | awk '{print $6}' | sed 's/,//')
        local cuda_full_version=$(nvcc --version | head -n1)
        log_success "CUDA toolkit found: $cuda_full_version"
        log_success "CUDA version: $cuda_version"

        # Check CUDA version compatibility
        local major_version=$(echo $cuda_version | cut -d. -f1)
        if [ "$major_version" -lt 11 ]; then
            log_error "CUDA version $cuda_version is too old. CUDA 11.0+ required for CC 7.0+"
            ((errors++))
        fi
    fi

    # Check GPU availability and compute capability
    log_info "Checking GPU availability and compute capabilities..."
    local gpu_count=$(nvidia-smi --list-gpus | wc -l)
    if [ "$gpu_count" -eq 0 ]; then
        log_error "No NVIDIA GPUs found. Please ensure CUDA-compatible GPU is available."
        ((errors++))
        return 1
    else
        log_success "Found $gpu_count NVIDIA GPU(s)"

        # Check each GPU
        for ((i=0; i<gpu_count; i++)); do
            local gpu_name=$(nvidia-smi --query-gpu=name --format=csv,noheader --id=$i)
            local compute_cap=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader --id=$i)

            log_info "GPU $i: $gpu_name (Compute Capability: $compute_cap)"

            # Check compute capability compatibility
            local major_cc=$(echo $compute_cap | cut -d. -f1)
            local minor_cc=$(echo $compute_cap | cut -d. -f2)

            if [ "$major_cc" -lt 3 ]; then
                log_error "GPU $i has insufficient compute capability ($compute_cap). CC 3.5+ required."
                ((errors++))
            elif [ "$major_cc" -eq 3 ] && [ "$minor_cc" -lt 5 ]; then
                log_error "GPU $i has insufficient compute capability ($compute_cap). CC 3.5+ required."
                ((errors++))
            elif [ "$major_cc" -eq 3 ] && [ "$minor_cc" -ge 5 ]; then
                log_success "GPU $i meets minimum requirement (CC $compute_cap >= 3.5)"
            else
                log_success "GPU $i exceeds minimum requirement (CC $compute_cap >= 3.5)"
            fi
        done
    fi

    # Check required CUDA libraries
    log_info "Checking CUDA libraries..."

    # Check cuBLAS
    if ldconfig -p | grep -q libcublas; then
        log_success "cuBLAS library found"
    else
        log_warning "cuBLAS library not found. May affect performance."
    fi

    # Check cuRAND
    if ldconfig -p | grep -q libcurand; then
        log_success "cuRAND library found"
    else
        log_warning "cuRAND library not found. May affect random number generation."
    fi

    # Check NPP
    if ldconfig -p | grep -q libnpp; then
        log_success "NPP library found"
    else
        log_warning "NPP library not found. May affect performance primitives."
    fi

    # Check required compilers
    log_info "Checking compilers..."

    if ! command -v g++ &> /dev/null; then
        log_error "g++ compiler not found. Please install build-essential."
        ((errors++))
    else
        log_success "g++ compiler found: $(g++ --version | head -n1)"
    fi

    if ! command -v gcc &> /dev/null; then
        log_error "gcc compiler not found. Please install build-essential."
        ((errors++))
    else
        log_success "gcc compiler found: $(gcc --version | head -n1)"
    fi

    # Check CMake
    if ! command -v cmake &> /dev/null; then
        log_error "CMake not found. Please install CMake."
        ((errors++))
    else
        log_success "CMake found: $(cmake --version | head -n1)"
        local cmake_version=$(cmake --version | grep "cmake version" | awk '{print $3}')
        local cmake_major=$(echo $cmake_version | cut -d. -f1)
        if [ "$cmake_major" -lt 3 ]; then
            log_error "CMake version $cmake_version is too old. CMake 3.22+ required."
            ((errors++))
        fi
    fi

    if [ "$errors" -gt 0 ]; then
        log_error "CUDA environment verification failed with $errors errors."
        return 1
    fi

    log_success "CUDA environment verification passed!"
    return 0
}

# Generate CUDA capability matrix
generate_capability_matrix() {
    log_info "Generating CUDA capability matrix..."

    cat > "$PROJECT_ROOT/build/cuda_capability_matrix.txt" << 'EOF'
# Puzzle71 CUDA Capability Matrix
# Generated: $(date)

# Target Compute Capabilities
CUDA_ARCHITECTURES = 75;86;89;90

# GPU Capability Matrix
# Compute Capability | Architecture | GPUs Supported | Features | Status
# --------------------|--------------|----------------|----------|--------
# 3.5+                 | Maxwell      | GTX 900/750  | Basic    | ✓
# 6.0+                 | Pascal       | GTX 10xx     | Advanced | ✓
# 6.1+                 | Pascal       | GTX 10xx     | Advanced | ✓
# 7.0+                 | Volta        | GTX 20xx     | Advanced | ✓
# 7.5+                 | Turing       | RTX 20xx     | Advanced | ✓
# 8.0+                 | Ampere       | RTX 30xx     | Advanced | ✓
# 8.6+                 | Ampere       | RTX 30xx     | Advanced | ✓
# 8.7+                 | Ampere       | RTX 30xx     | Advanced | ✓
# 8.9+                 | Ada Lovelace | RTX 40xx     | Advanced | ✓
# 9.0+                 | Hopper       | H100/H200   | Advanced | ✓

# Current System GPUs:
EOF

    # Add current system GPUs
    nvidia-smi --query-gpu=name,compute_cap --format=csv,noheader | while IFS=, read -r name compute_cap; do
        echo "# $name | $compute_cap | ✓" >> "$PROJECT_ROOT/build/cuda_capability_matrix.txt"
    done

    log_success "CUDA capability matrix generated: $PROJECT_ROOT/build/cuda_capability_matrix.txt"
}

# Create CUDA compilation configuration
create_cuda_config() {
    log_info "Creating CUDA compilation configuration..."

    # Create CUDA build configuration file
    cat > "$PROJECT_ROOT/build/cuda_build_config.h" << 'EOF'
// Puzzle71 CUDA Build Configuration
// Auto-generated by cuda_environment_setup.sh
// Date: $(date)

#ifndef PUZZLE71_CUDA_BUILD_CONFIG_H
#define PUZZLE71_CUDA_BUILD_CONFIG_H

// CUDA Version Information
#define CUDA_VERSION_MAJOR 12
#define CUDA_VERSION_MINOR 0
#define CUDA_VERSION_PATCH 0
#define CUDA_VERSION_NUMBER 1200

// Target Compute Capabilities
#define CUDA_MIN_COMPUTE_CAPABILITY 350
#define CUDA_TARGET_COMPUTE_CAPABILITIES "75;86;89;90"

// Architecture Support
#define SUPPORTS_MAXWELL 0
#define SUPPORTS_PASCAL 1
#define SUPPORTS_VOLTA 1
#define SUPPORTS_TURING 1
#define SUPPORTS_AMPERE 1
#define SUPPORTS_ADA_LOVELACE 1
#define SUPPORTS_HOPPER 1

// Feature Support
#define CUDA_SEPARABLE_COMPILATION 1
#define CUDA_COOPERATIVE_GROUPS 1
#define CUDA_DYNAMIC_PARALLELISM 1
#define CUDA_UNIFIED_MEMORY 1

// Compilation Flags
#ifdef DEBUG
    #define CUDA_FLAGS_DEBUG "-g -G -lineinfo --device-debug"
    #define CUDA_FLAGS_OPTIMIZATION "-O0"
#else
    #define CUDA_FLAGS_DEBUG ""
    #define CUDA_FLAGS_OPTIMIZATION "-O3 -use_fast_math"
#endif

// Memory Configuration
#define CUDA_MAX_SHARED_MEMORY_PER_BLOCK 49152  // 48KB
#define CUDA_MAX_THREADS_PER_BLOCK 1024
#define CUDA_MAX_REGISTERS_PER_THREAD 255

// Kernel Configuration
#define CUDA_DEFAULT_GRID_DIM 2048
#define CUDA_DEFAULT_BLOCK_DIM 512
#define CUDA_DEFAULT_STREAMS_PER_GPU 4

#endif // PUZZLE71_CUDA_BUILD_CONFIG_H
EOF

    log_success "CUDA build configuration created: $PROJECT_ROOT/build/cuda_build_config.h"
}

# Create CUDA environment test program
create_cuda_test_program() {
    log_info "Creating CUDA environment test program..."

    cat > "$PROJECT_ROOT/build/test_cuda_env.cu" << 'EOF'
// Puzzle71 CUDA Environment Test
// Simple CUDA program to verify environment setup

#include <iostream>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

// Simple CUDA kernel
__global__ void testKernel(float *data) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    data[idx] = idx * 2.0f;
}

// Device query function
void printDeviceProperties() {
    int deviceCount = 0;
    cudaGetDeviceCount(&deviceCount);

    std::cout << "=== CUDA Device Information ===" << std::endl;
    std::cout << "Number of devices: " << deviceCount << std::endl;

    for (int i = 0; i < deviceCount; ++i) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, i);

        std::cout << "Device " << i << ":" << std::endl;
        std::cout << "  Name: " << prop.name << std::endl;
        std::cout << "  Compute Capability: " << prop.major << "." << prop.minor << std::endl;
        std::cout << "  Global Memory: " << prop.totalGlobalMem / (1024*1024) << " MB" << std::endl;
        std::cout << "  Shared Memory per Block: " << prop.sharedMemPerBlock / 1024 << " KB" << std::endl;
        std::cout << "  Max Threads per Block: " << prop.maxThreadsPerBlock << std::endl;
        std::cout << "  Max Grid Dimensions: " << prop.maxGridSize[0] << " x " << prop.maxGridSize[1] << " x " << prop.maxGridSize[2] << std::endl;
        std::cout << "  Max Block Dimensions: " << prop.maxThreadsDim[0] << " x " << prop.maxThreadsDim[1] << " x " << prop.maxThreadsDim[2] << std::endl;

        // Check if this GPU meets our requirements
        int computeCapability = prop.major * 10 + prop.minor;
        if (computeCapability >= 35) {
            std::cout << "  ✓ Meets CC 3.5+ requirement" << std::endl;
        } else {
            std::cout << "  ❌ Does not meet CC 3.5+ requirement (CC " << prop.major << "." << prop.minor << ")" << std::endl;
        }
    }
    std::cout << "================================" << std::endl;
}

int main() {
    std::cout << "Puzzle71 CUDA Environment Test" << std::endl;
    std::cout << "=============================" << std::endl;

    // Print device properties
    printDeviceProperties();

    // Test simple CUDA kernel
    const int N = 1024;
    float *d_data;

    // Allocate device memory
    cudaMalloc(&d_data, N * sizeof(float));

    // Launch kernel
    testKernel<<<1, N>>>(d_data);

    // Check for errors
    cudaError_t error = cudaGetLastError();
    if (error != cudaSuccess) {
        std::cerr << "CUDA kernel launch failed: " << cudaGetErrorString(error) << std::endl;
        return 1;
    }

    // Copy back to host
    float *h_data = new float[N];
    cudaMemcpy(h_data, d_data, N * sizeof(float), cudaMemcpyDeviceToHost);

    // Verify results
    bool test_passed = true;
    for (int i = 0; i < 10; ++i) {
        if (h_data[i] != i * 2.0f) {
            test_passed = false;
            break;
        }
    }

    // Free memory
    cudaFree(d_data);
    delete[] h_data;

    if (test_passed) {
        std::cout << "✅ CUDA kernel test PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "❌ CUDA kernel test FAILED" << std::endl;
        return 1;
    }
}
EOF

    log_success "CUDA test program created: $PROJECT_ROOT/build/test_cuda_env.cu"
}

# Compile and run CUDA test
compile_and_test_cuda() {
    log_info "Compiling and running CUDA environment test..."

    cd "$PROJECT_ROOT/build"

    # Compile test program
    if nvcc -o test_cuda_env test_cuda_env.cu; then
        log_success "CUDA test program compiled successfully"

        # Run test program
        if ./test_cuda_env; then
            log_success "CUDA environment test PASSED"
            return 0
        else
            log_error "CUDA environment test FAILED"
            return 1
        fi
    else
        log_error "CUDA test program compilation FAILED"
        return 1
    fi
}

# Main function
main() {
    log_info "Puzzle71 CUDA Environment Setup"
    log_info "================================"

    # Verify CUDA environment
    if ! verify_cuda_environment; then
        log_error "CUDA environment verification failed. Please fix issues and retry."
        return 1
    fi

    # Generate capability matrix
    generate_capability_matrix

    # Create CUDA configuration
    create_cuda_config

    # Create test program
    create_cuda_test_program

    # Compile and test
    if ! compile_and_test_cuda; then
        log_error "CUDA test failed. Please check CUDA installation."
        return 1
    fi

    log_success "CUDA environment setup completed successfully!"
    log_info "Ready for Puzzle71 development with CUDA acceleration."
    log_info ""
    log_info "Generated files:"
    log_info "  - $PROJECT_ROOT/build/cuda_capability_matrix.txt"
    log_info "  - $PROJECT_ROOT/build/cuda_build_config.h"
    log_info "  - $PROJECT_ROOT/build/test_cuda_env.cu"
    log_info "  - $PROJECT_ROOT/build/test_cuda_env (executable)"
}

# Run setup
main "$@"
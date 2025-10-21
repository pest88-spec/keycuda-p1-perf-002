# Quick Start Guide: Puzzle71Solver CUDA Technical Debt Elimination

**Version**: 1.0.0
**Date**: 2025-10-17
**Purpose**: Quick start guide for implementing CUDA refactoring project

## Overview

This guide provides step-by-step instructions for implementing the Puzzle71Solver CUDA technical debt elimination project. The project focuses on eliminating 100% code duplication, achieving 2.5-3.0× performance improvements, and modernizing the architecture while maintaining backward compatibility.

## Prerequisites

### System Requirements

- **CUDA Toolkit**: 11.8 or later
- **GPU**: CUDA-capable GPU (Turing architecture or newer)
- **Memory**: Minimum 8GB GPU memory, 16GB+ system RAM
- **OS**: Linux (Ubuntu 20.04+ recommended)
- **Compiler**: GCC 9.0+ or Clang 10.0+

### Software Dependencies

```bash
# Install required system packages
sudo apt-get update
sudo apt-get install build-essential cmake git

# Install CUDA development libraries
sudo apt-get install nvidia-cuda-dev nvidia-cuda-toolkit

# Install cryptographic library for validation
sudo apt-get install libsecp256k1-dev

# Install JSON library for configuration
sudo apt-get install nlohmann-json3-dev

# Install testing framework
sudo apt-get install libgtest-dev libgmock-dev
```

## Project Setup

### 1. Clone Repository

```bash
git clone <repository-url>
cd PuzzleKeyhunt
```

### 2. Initialize Submodules

```bash
git submodule update --init --recursive
```

### 3. Verify Dependencies

```bash
# Check CUDA installation
nvcc --version

# Check GPU availability
nvidia-smi

# Verify dependency installation
python3 scripts/check_dependencies.py
```

## Build Instructions

### 1. Create Build Directory

```bash
mkdir -p build
cd build
```

### 2. Configure with CMake

```bash
# Basic configuration
cmake ../src/KeyhuntCore -DCMAKE_BUILD_TYPE=Release

# Development configuration with tests
cmake ../src/KeyhuntCore \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=ON \
    -DBUILD_BENCHMARKS=ON

# Performance optimization configuration
cmake ../src/KeyhuntCore \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_AGGRESSIVE_OPTIMIZATIONS=ON \
    -DCMAKE_CUDA_ARCHITECTURES="75;80;86;89;90"
```

### 3. Build Project

```bash
# Build all targets
make -j$(nproc)

# Build specific targets
make Puzzle71Solver          # Main executable
make tests                    # Unit tests
make benchmarks              # Performance benchmarks
```

## Phase 1: Code Deduplication Implementation

### 1.1 Create Unified Modules Directory

```bash
mkdir -p src/KeyhuntCore/common
```

### 1.2 Implement ResultEmitter Module

Create `src/KeyhuntCore/common/result_emitter.cuh`:

```cuda
#ifndef RESULT_EMITTER_CUH
#define RESULT_EMITTER_CUH

#include <cuda_runtime.h>
#include <cstdint>

namespace keyhunt {
namespace common {

struct CandidateResult {
    uint64_t private_key;
    uint8_t public_key[65];
    char address[35];
    uint32_t thread_id;
    uint32_t block_id;
};

__device__ __forceinline__
bool emitCandidate(CandidateResult* results,
                   uint32_t max_results,
                   uint32_t* result_count,
                   const CandidateResult& candidate);

__host__
cudaError_t initializeResultEmitter(CandidateResult* results,
                                   uint32_t* result_count,
                                   uint32_t max_results);

} // namespace common
} // namespace keyhunt

#endif
```

### 1.3 Implement HashUtils Module

Create `src/KeyhuntCore/common/hash_utils.cuh`:

```cuda
#ifndef HASH_UTILS_CUH
#define HASH_UTILS_CUH

#include <cuda_runtime.h>
#include <cstdint>

namespace keyhunt {
namespace common {

struct HashContext {
    uint32_t h[8];
    uint64_t total_length;
    uint8_t buffer[64];
    uint32_t buffer_used;
};

__device__ __forceinline__
void finalizeDigest(HashContext* ctx, uint8_t digest[32]);

__device__ __forceinline__
void computeHash160(const uint8_t sha256_digest[32],
                    uint8_t hash160_output[20]);

} // namespace common
} // namespace keyhunt

#endif
```

### 1.4 Implement ECCOperations Module

Create `src/KeyhuntCore/common/ecc_operations.cuh`:

```cuda
#ifndef ECC_OPERATIONS_CUH
#define ECC_OPERATIONS_CUH

#include <cuda_runtime.h>
#include <cstdint>

namespace keyhunt {
namespace common {

struct ECPoint {
    uint32_t x[8];
    uint32_t y[8];
    bool is_infinity;
};

struct UInt256 {
    uint32_t data[8];
};

__device__ __forceinline__
bool privateKeyToPublicKey(const UInt256& private_key,
                          ECPoint& public_key);

__host__
cudaError_t initializeECCModule();

} // namespace common
} // namespace keyhunt

#endif
```

### 1.5 Update CMakeLists.txt

Add to `src/KeyhuntCore/CMakeLists.txt`:

```cmake
# Add unified modules to library
set(COMMON_SOURCES
    common/result_emitter.cu
    common/hash_utils.cu
    common/ecc_operations.cu
)

cuda_add_library(KeyhuntCoreCommon ${COMMON_SOURCES})
target_link_libraries(KeyhuntCoreCommon ${CUDA_LIBRARIES})
```

### 1.6 Replace Duplicate Function Calls

Update `src/puzzle71_kernel.cu` and `src/kernels/hash_kernel.cu`:

```cuda
// Before (duplicate code)
void EmitCandidate(uint64_t key, uint8_t* pubkey, char* addr) {
    // 58 lines of duplicate implementation
}

// After (unified module)
#include "common/result_emitter.cuh"

void EmitCandidate(uint64_t key, uint8_t* pubkey, char* addr) {
    CandidateResult result;
    result.private_key = key;
    memcpy(result.public_key, pubkey, 65);
    // ... format address

    emitCandidate(global_results, max_results, global_count, result);
}
```

### 1.7 Test Code Deduplication

```bash
# Build with changes
make -j$(nproc)

# Run tests to verify functionality
./tests/unit/test_unified_modules

# Validate identical results
./tests/validation/test_deduplication_correctness
```

## Phase 2: CUDA Performance Optimization

### 2.1 Enable Memory Optimization

Update `data/config.txt`:

```ini
# Force enable optimized memory operations
USE_ORIGINAL_READINT=0
USE_ORIGINAL_WRITEINT=0

# Enable kernel separation
ENABLE_KERNEL_SEPARATION=1

# Optimize for target GPU architecture
TARGET_SM=80  # Adjust for your GPU
```

### 2.2 Implement Kernel Separation

Create separated kernels:

**ECC Kernel** (`src/KeyhuntCore/kernels/ecc_separated.cu`):

```cuda
#include "common/ecc_operations.cuh"

__global__ void eccKernel(
    const uint64_t* private_keys,
    uint32_t key_count,
    ECPoint* public_keys) {

    uint32_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= key_count) return;

    UInt256 private_key;
    private_key.data[0] = private_keys[tid];
    // ... fill remaining words

    privateKeyToPublicKey(private_key, public_keys[tid]);
}
```

**Hash Kernel** (`src/KeyhuntCore/kernels/hash_separated.cu`):

```cuda
#include "common/hash_utils.cuh"

__global__ void hashKernel(
    const ECPoint* public_keys,
    uint32_t key_count,
    uint8_t* hash160s) {

    uint32_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= key_count) return;

    uint8_t public_key_bytes[65];
    compressPublicKey(public_keys[tid], public_key_bytes);

    computeUnifiedHash160(public_key_bytes, 33, hash160s + tid * 20);
}
```

**Compare Kernel** (`src/KeyhuntCore/kernels/compare_separated.cu`):

```cuda
#include "common/result_emitter.cuh"

__global__ void compareKernel(
    const uint8_t* hash160s,
    uint32_t key_count,
    const uint8_t* target_hash160s,
    uint32_t target_count,
    CandidateResult* results,
    uint32_t* result_count) {

    uint32_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= key_count) return;

    for (uint32_t i = 0; i < target_count; i++) {
        if (memcmp(hash160s + tid * 20, target_hash160s + i * 20, 20) == 0) {
            // Found match - emit candidate
            CandidateResult result;
            result.private_key = tid; // Simplified for example
            // ... fill result details

            emitCandidate(results, 1000, result_count, result);
            break;
        }
    }
}
```

### 2.3 Update Main Execution Flow

Modify main execution to use separated kernels:

```cpp
// Main execution function
cudaError_t executeSeparatedKernels(
    const uint64_t* private_keys,
    uint32_t key_count,
    const uint8_t* targets,
    uint32_t target_count) {

    // Phase 1: ECC computation
    dim3 eccGrid( (key_count + 255) / 256 );
    dim3 eccBlock(256);

    ECPoint* d_public_keys;
    cudaMalloc(&d_public_keys, key_count * sizeof(ECPoint));

    eccKernel<<<eccGrid, eccBlock>>>(private_keys, key_count, d_public_keys);
    cudaDeviceSynchronize();

    // Phase 2: Hash computation
    uint8_t* d_hash160s;
    cudaMalloc(&d_hash160s, key_count * 20);

    hashKernel<<<eccGrid, eccBlock>>>(d_public_keys, key_count, d_hash160s);
    cudaDeviceSynchronize();

    // Phase 3: Comparison
    CandidateResult* d_results;
    uint32_t* d_result_count;
    cudaMalloc(&d_results, 1000 * sizeof(CandidateResult));
    cudaMalloc(&d_result_count, sizeof(uint32_t));
    cudaMemset(d_result_count, 0, sizeof(uint32_t));

    compareKernel<<<eccGrid, eccBlock>>>(d_hash160s, key_count, targets, target_count, d_results, d_result_count);
    cudaDeviceSynchronize();

    // Retrieve results
    uint32_t result_count;
    cudaMemcpy(&result_count, d_result_count, sizeof(uint32_t), cudaMemcpyDeviceToHost);

    // ... process results

    return cudaSuccess;
}
```

### 2.4 Test Performance Optimization

```bash
# Build optimized version
make clean && make -j$(nproc)

# Run performance benchmarks
./benchmarks/run_benchmarks.sh

# Check memory efficiency
./tools/memory_profiler ./build/Puzzle71Solver

# Verify register usage
nvcc --ptxas-options=-v src/kernels/ecc_separated.cu
```

## Phase 3: Performance Monitoring Setup

### 3.1 Create Benchmark Infrastructure

Create `src/KeyhuntCore/benchmarks/` directory:

```bash
mkdir -p src/KeyhuntCore/benchmarks
```

### 3.2 Implement Baseline Manager

Create `src/KeyhuntCore/benchmarks/baseline_manager.cpp`:

```cpp
#include "baseline_manager.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

namespace keyhunt {
namespace benchmarks {

bool BaselineManager::loadBaseline(const std::string& gpu_name, PerformanceBaseline& baseline) {
    std::ifstream file("benchmarks/baselines/" + gpu_name + ".json");
    if (!file.is_open()) return false;

    nlohmann::json json_data;
    file >> json_data;

    baseline.fromJson(json_data);

    // Verify SHA-256 digest
    std::string computed_digest = computeSHA256(json_data.dump());
    return computed_digest == baseline.sha256_digest;
}

bool BaselineManager::saveBaseline(const std::string& gpu_name, const PerformanceBaseline& baseline) {
    nlohmann::json json_data = baseline.toJson();
    json_data["sha256_digest"] = computeSHA256(json_data.dump());

    std::ofstream file("benchmarks/baselines/" + gpu_name + ".json");
    file << json_data.dump(4);

    return file.good();
}

bool BaselineManager::compareWithBaseline(const std::string& gpu_name,
                                         const PerformanceMetrics& current,
                                         ComparisonResult& result) {
    PerformanceBaseline baseline;
    if (!loadBaseline(gpu_name, baseline)) {
        return false; // No baseline exists
    }

    result.is_regression = current.throughput < baseline.min_throughput;
    result.performance_ratio = (double)current.throughput / baseline.target_throughput;

    return true;
}

} // namespace benchmarks
} // namespace keyhunt
```

### 3.3 Implement Benchmark Runner

Create `src/KeyhuntCore/benchmarks/benchmark_runner.cpp`:

```cpp
#include "benchmark_runner.hpp"
#include <chrono>
#include <iostream>

namespace keyhunt {
namespace benchmarks {

BenchmarkResult BenchmarkRunner::runSustainedBenchmark(uint32_t duration_seconds) {
    BenchmarkResult result;

    // Warm-up phase (2 minutes)
    std::cout << "Warming up GPU..." << std::endl;
    auto warmup_start = std::chrono::high_resolution_clock::now();
    runBenchmarkIteration(120); // 2 minute warmup
    auto warmup_end = std::chrono::high_resolution_clock::now();

    // Main benchmark phase
    std::cout << "Running " << duration_seconds << " second benchmark..." << std::endl;
    auto benchmark_start = std::chrono::high_resolution_clock::now();

    result.samples.clear();
    uint32_t elapsed = 0;

    while (elapsed < duration_seconds) {
        auto sample_start = std::chrono::high_resolution_clock::now();

        PerformanceMetrics sample = runBenchmarkIteration(30); // 30 second samples
        result.samples.push_back(sample);

        auto sample_end = std::chrono::high_resolution_clock::now();
        elapsed += std::chrono::duration_cast<std::chrono::seconds>(sample_end - sample_start).count();

        std::cout << "Sample " << result.samples.size()
                  << ": " << sample.throughput << " keys/sec" << std::endl;
    }

    auto benchmark_end = std::chrono::high_resolution_clock::now();
    result.total_duration = std::chrono::duration_cast<std::chrono::seconds>(benchmark_end - benchmark_start);

    // Calculate statistics (exclude first 2 samples from warm-up)
    calculateStatistics(result);

    return result;
}

void BenchmarkRunner::calculateStatistics(BenchmarkResult& result) {
    if (result.samples.size() <= 2) return;

    // Skip warm-up samples
    std::vector<PerformanceMetrics> valid_samples(
        result.samples.begin() + 2,
        result.samples.end()
    );

    // Calculate average throughput
    uint64_t total_throughput = 0;
    for (const auto& sample : valid_samples) {
        total_throughput += sample.throughput;
    }
    result.average_throughput = total_throughput / valid_samples.size();

    // Find min/max throughput
    result.min_throughput = valid_samples[0].throughput;
    result.max_throughput = valid_samples[0].throughput;

    for (const auto& sample : valid_samples) {
        result.min_throughput = std::min(result.min_throughput, sample.throughput);
        result.max_throughput = std::max(result.max_throughput, sample.throughput);
    }
}

} // namespace benchmarks
} // namespace keyhunt
```

### 3.4 Setup Baseline Files

Create GPU-specific baseline files:

**RTX 2080 Ti Baseline** (`benchmarks/baselines/rtx2080ti.json`):

```json
{
    "gpu_name": "RTX 2080 Ti",
    "baseline_version": "1.0.0",
    "target_throughput": 1000000000,
    "min_throughput": 950000000,
    "memory_efficiency": 0.90,
    "max_registers": 40,
    "min_occupancy": 0.80,
    "sha256_digest": "abcdef1234567890...",
    "created_at": "2025-10-17T10:00:00Z"
}
```

### 3.5 Create CI Integration Script

Create `scripts/ci/performance_gate.sh`:

```bash
#!/bin/bash

set -e

GPU_NAME=$1
if [ -z "$GPU_NAME" ]; then
    echo "Usage: $0 <gpu_name>"
    exit 1
fi

echo "Running performance gate for GPU: $GPU_NAME"

# Run sustained benchmark
./build/benchmarks/benchmark_runner --duration 600 --gpu $GPU_NAME > current_result.json

# Compare with baseline
./build/benchmarks/baseline_manager --compare --gpu $GPU_NAME --current current_result.json > comparison_result.json

# Check for regression
REGRESSION_DETECTED=$(jq -r '.regression_detected' comparison_result.json)
if [ "$REGRESSION_DETECTED" = "true" ]; then
    echo "PERFORMANCE REGRESSION DETECTED - failing build"
    echo "Current throughput: $(jq -r '.current_throughput' comparison_result.json)"
    echo "Minimum throughput: $(jq -r '.min_throughput' comparison_result.json)"
    exit 1
fi

echo "Performance gate passed - no regression detected"
echo "Performance ratio: $(jq -r '.performance_ratio' comparison_result.json)"
```

## Testing and Validation

### 1. Run Unit Tests

```bash
# Build and run all tests
cd build
make test

# Run specific test categories
ctest -L unit           # Unit tests only
ctest -L integration    # Integration tests only
ctest -L performance    # Performance tests only
```

### 2. Validate Functional Correctness

```bash
# Run scientific validation
./build/tests/validation/scientific_validation --iterations 10000

# Validate CPU/GPU consistency
./build/tests/validation/cpu_gpu_consistency --test_cases 1000

# Verify address generation
./build/tests/validation/address_generation_validation
```

### 3. Performance Validation

```bash
# Run full benchmark suite
./scripts/run_benchmarks.sh

# Establish performance baseline
./scripts/establish_baseline.sh RTX_3090

# Check for performance regression
./scripts/ci/performance_gate.sh RTX_3090
```

### 4. Memory Profiling

```bash
# Profile memory usage
cuda-memcheck ./build/Puzzle71Solver --config data/config.txt

# Check for memory leaks
cuda-gdb --args ./build/Puzzle71Solver --validate-memory

# Profile kernel performance
nvprof ./build/Puzzle71Solver --benchmark-mode
```

## Troubleshooting

### Common Issues

1. **Build Failures**:
   ```bash
   # Check CUDA version compatibility
   nvcc --version

   # Clean build directory
   rm -rf build/*

   # Rebuild with verbose output
   make VERBOSE=1
   ```

2. **Performance Issues**:
   ```bash
   # Check GPU utilization
   nvidia-smi

   # Profile specific kernel
   nvprof --kernels eccKernel ./build/Puzzle71Solver

   # Check memory bandwidth
   ./tools/bandwidth_test
   ```

3. **Test Failures**:
   ```bash
   # Run tests with detailed output
   ctest --output-on-failure

   # Run specific failing test
   ./build/tests/unit/test_unified_modules --gtest_filter=ECCOperations.*
   ```

### Performance Tuning

1. **Memory Optimization**:
   - Ensure `USE_ORIGINAL_READINT=0` and `USE_ORIGINAL_WRITEINT=0`
   - Check memory coalescing efficiency with Nsight Compute
   - Optimize shared memory usage patterns

2. **Register Optimization**:
   - Monitor register usage with `nvcc --ptxas-options=-v`
   - Reduce per-thread register count below 40
   - Consider `-maxrregcount 40` compilation flag

3. **Occupancy Optimization**:
   - Use CUDA Occupancy Calculator to tune block sizes
   - Target ≥50% occupancy for all kernels
   - Balance between register usage and occupancy

## Next Steps

After completing this quick start guide:

1. **Review Results**: Analyze benchmark results and validation outputs
2. **Establish Baselines**: Create baseline files for your specific GPU hardware
3. **CI Integration**: Set up automated performance gates in your CI/CD pipeline
4. **Documentation**: Update project documentation with your specific configurations
5. **Advanced Optimization**: Explore advanced optimizations like Tensor Core utilization

## Support

For additional support:

1. **Documentation**: Refer to the full technical specification and API contracts
2. **Community**: Join the project discussion forums
3. **Issues**: Report bugs and request features via the project issue tracker
4. **Performance**: Contact the performance optimization team for tuning assistance
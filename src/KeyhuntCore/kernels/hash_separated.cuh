// Puzzle71Solver - Separated Hash Kernel Header
// Register-optimized hash operations interface (T027)

#pragma once

#include <cuda_runtime.h>
#include <cstdint>

namespace keyhunt {
namespace kernels {

/**
 * @brief Separated Hash Kernel Functions
 *
 * These kernels provide optimized hash operations with strict register
 * usage limits to maximize GPU occupancy and performance.
 */

/**
 * @brief Basic separated hash kernel (≤40 registers/thread)
 *
 * Implements core hash operations with register optimization.
 * Suitable for general-purpose use with moderate batch sizes.
 *
 * @param gridDim Grid dimensions
 * @param blockDim Block dimensions
 * @param pointsPerThread Number of points per thread to process
 * @param compression Compression type (0=uncompressed, 1=compressed, 2=both)
 * @param stream CUDA stream (optional)
 * @return CUDA error code
 */
cudaError_t LaunchHashSeparatedKernel(
    dim3 gridDim,
    dim3 blockDim,
    int pointsPerThread,
    int compression,
    cudaStream_t stream = nullptr
);

/**
 * @brief Advanced separated hash kernel with shared memory optimization
 *
 * Enhanced version using shared memory for batch operations.
 * Best performance for larger batches when shared memory is available.
 *
 * @param gridDim Grid dimensions
 * @param blockDim Block dimensions
 * @param pointsPerThread Number of points per thread to process
 * @param compression Compression type (0=uncompressed, 1=compressed, 2=both)
 * @param stream CUDA stream (optional)
 * @return CUDA error code
 */
cudaError_t LaunchHashSeparatedKernelAdvanced(
    dim3 gridDim,
    dim3 blockDim,
    int pointsPerThread,
    int compression,
    cudaStream_t stream = nullptr
);

/**
 * @brief Memory-optimized separated hash kernel
 *
 * Ultra-optimized version focusing on memory bandwidth efficiency.
 * Uses vectorized operations and optimized memory access patterns.
 *
 * @param gridDim Grid dimensions
 * @param blockDim Block dimensions
 * @param pointsPerThread Number of points per thread to process
 * @param compression Compression type (0=uncompressed, 1=compressed, 2=both)
 * @param stream CUDA stream (optional)
 * @return CUDA error code
 */
cudaError_t LaunchHashSeparatedKernelMemoryOptimized(
    dim3 gridDim,
    dim3 blockDim,
    int pointsPerThread,
    int compression,
    cudaStream_t stream = nullptr
);

/**
 * @brief Auto-select optimal hash kernel based on device capabilities
 *
 * Automatically chooses the best kernel variant based on:
 * - Available shared memory
 * - Device compute capability
 * - Batch size
 * - Memory bandwidth characteristics
 *
 * @param gridDim Grid dimensions
 * @param blockDim Block dimensions
 * @param pointsPerThread Number of points per thread to process
 * @param compression Compression type (0=uncompressed, 1=compressed, 2=both)
 * @param deviceProps Device properties (optional)
 * @param stream CUDA stream (optional)
 * @return CUDA error code
 */
cudaError_t LaunchHashKernelAutoOptimized(
    dim3 gridDim,
    dim3 blockDim,
    int pointsPerThread,
    int compression,
    const cudaDeviceProp* deviceProps = nullptr,
    cudaStream_t stream = nullptr
);

/**
 * @brief Calculate optimal hash kernel configuration
 *
 * Determines the best kernel configuration based on device capabilities
 * and workload characteristics.
 *
 * @param deviceProps Device properties
 * @param pointsPerThread Number of points per thread
 * @param compression Compression type
 * @param gridSize Output grid dimensions
 * @param blockSize Output block dimensions
 * @param kernelType Output recommended kernel type
 */
void CalculateOptimalHashKernelConfig(
    const cudaDeviceProp& deviceProps,
    int pointsPerThread,
    int compression,
    dim3& gridSize,
    dim3& blockSize,
    int& kernelType
);

/**
 * @brief Validate hash kernel launch parameters
 *
 * Ensures launch parameters are within device limits and optimal
 * for performance.
 *
 * @param gridDim Grid dimensions to validate
 * @param blockDim Block dimensions to validate
 * @param pointsPerThread Points per thread to validate
 * @param compression Compression type to validate
 * @param deviceProps Device properties for validation
 * @return True if parameters are valid, false otherwise
 */
bool ValidateHashKernelLaunchParams(
    const dim3& gridDim,
    const dim3& blockDim,
    int pointsPerThread,
    int compression,
    const cudaDeviceProp& deviceProps
);

/**
 * @brief Get performance metrics for hash kernels
 *
 * Returns estimated performance metrics based on kernel type and
 * device capabilities.
 *
 * @param kernelType Kernel type (0=basic, 1=advanced, 2=memory-optimized)
 * @param deviceProps Device properties
 * @param pointsPerThread Points per thread
 * @param compression Compression type
 * @return Estimated throughput (hashes/second)
 */
double EstimateHashKernelThroughput(
    int kernelType,
    const cudaDeviceProp& deviceProps,
    int pointsPerThread,
    int compression
);

// Kernel type constants
enum HashKernelType {
    HASH_KERNEL_BASIC = 0,              // 40 registers, basic optimization
    HASH_KERNEL_ADVANCED = 1,          // 40 registers, shared memory
    HASH_KERNEL_MEMORY_OPTIMIZED = 2,    // 40 registers, memory bandwidth optimized
    HASH_KERNEL_AUTO = 3                // Auto-select optimal kernel
};

// Compression type constants
enum HashCompressionType {
    HASH_COMPRESS_UNCOMPRESSED = 0,     // Only uncompressed addresses
    HASH_COMPRESS_COMPRESSED = 1,       // Only compressed addresses
    HASH_COMPRESS_BOTH = 2              // Both compressed and uncompressed
};

// Performance target constants
namespace hash_performance {
    constexpr int TARGET_REGISTERS_PER_THREAD = 40;
    constexpr double TARGET_OCCUPANCY = 0.75;    // 75% target occupancy
    constexpr double TARGET_MEMORY_EFFICIENCY = 0.85; // 85% memory coalescing efficiency
    constexpr int MIN_BLOCK_SIZE = 128;
    constexpr int MAX_BLOCK_SIZE = 512;
    constexpr int OPTIMAL_BLOCK_SIZE = 256;

    // Hash-specific performance targets
    constexpr int SHA256_ROUNDS = 64;
    constexpr int RIPEMD160_ROUNDS = 80;
    constexpr double HASH_COMPUTE_RATIO = 0.6;  // 60% compute, 40% memory
}

} // namespace kernels
} // namespace keyhunt
// Puzzle71Solver - Separated Compare Kernel Header
// Register-optimized compare operations interface (T028)

#pragma once

#include <cuda_runtime.h>
#include <cstdint>

namespace keyhunt {
namespace kernels {

/**
 * @brief Separated Compare Kernel Functions
 *
 * These kernels provide optimized compare operations with strict register
 * usage limits to maximize GPU occupancy and performance.
 */

/**
 * @brief Basic separated compare kernel (≤24 registers/thread)
 *
 * Implements core compare operations with register optimization.
 * Focuses on Hash160 computation and target matching.
 *
 * @param gridDim Grid dimensions
 * @param blockDim Block dimensions
 * @param pointsPerThread Number of points per thread to process
 * @param targetHash Target hash to compare against (device pointer)
 * @param stream CUDA stream (optional)
 * @return CUDA error code
 */
cudaError_t LaunchCompareSeparatedKernel(
    dim3 gridDim,
    dim3 blockDim,
    int pointsPerThread,
    const std::uint32_t* targetHash,
    cudaStream_t stream = nullptr
);

/**
 * @brief Advanced separated compare kernel with batch optimization
 *
 * Enhanced version using batch processing for multiple targets.
 * Best performance when comparing against multiple target addresses.
 *
 * @param gridDim Grid dimensions
 * @param blockDim Block dimensions
 * @param pointsPerThread Number of points per thread to process
 * @param targetHashes Array of target hashes
 * @param numTargets Number of target hashes
 * @param stream CUDA stream (optional)
 * @return CUDA error code
 */
cudaError_t LaunchCompareSeparatedKernelAdvanced(
    dim3 gridDim,
    dim3 blockDim,
    int pointsPerThread,
    const std::uint32_t* targetHashes,
    int numTargets,
    cudaStream_t stream = nullptr
);

/**
 * @brief Memory-optimized separated compare kernel
 *
 * Ultra-optimized version focusing on memory bandwidth efficiency.
 * Uses vectorized operations and optimized memory access patterns.
 *
 * @param gridDim Grid dimensions
 * @param blockDim Block dimensions
 * @param pointsPerThread Number of points per thread to process
 * @param targetHash Target hash to compare against (device pointer)
 * @param stream CUDA stream (optional)
 * @return CUDA error code
 */
cudaError_t LaunchCompareSeparatedKernelMemoryOptimized(
    dim3 gridDim,
    dim3 blockDim,
    int pointsPerThread,
    const std::uint32_t* targetHash,
    cudaStream_t stream = nullptr
);

/**
 * @brief Auto-select optimal compare kernel based on device capabilities
 *
 * Automatically chooses the best kernel variant based on:
 * - Number of target hashes
 * - Device compute capability
 * - Batch size
 * - Memory bandwidth characteristics
 *
 * @param gridDim Grid dimensions
 * @param blockDim Block dimensions
 * @param pointsPerThread Number of points per thread to process
 * @param targetHashes Array of target hashes
 * @param numTargets Number of target hashes
 * @param deviceProps Device properties (optional)
 * @param stream CUDA stream (optional)
 * @return CUDA error code
 */
cudaError_t LaunchCompareKernelAutoOptimized(
    dim3 gridDim,
    dim3 blockDim,
    int pointsPerThread,
    const std::uint32_t* targetHashes,
    int numTargets,
    const cudaDeviceProp* deviceProps = nullptr,
    cudaStream_t stream = nullptr
);

/**
 * @brief Calculate optimal compare kernel configuration
 *
 * Determines the best kernel configuration based on device capabilities
 * and workload characteristics.
 *
 * @param deviceProps Device properties
 * @param pointsPerThread Number of points per thread
 * @param numTargets Number of target hashes
 * @param gridSize Output grid dimensions
 * @param blockSize Output block dimensions
 * @param kernelType Output recommended kernel type
 */
void CalculateOptimalCompareKernelConfig(
    const cudaDeviceProp& deviceProps,
    int pointsPerThread,
    int numTargets,
    dim3& gridSize,
    dim3& blockSize,
    int& kernelType
);

/**
 * @brief Validate compare kernel launch parameters
 *
 * Ensures launch parameters are within device limits and optimal
 * for performance.
 *
 * @param gridDim Grid dimensions to validate
 * @param blockDim Block dimensions to validate
 * @param pointsPerThread Points per thread to validate
 * @param numTargets Number of targets to validate
 * @param deviceProps Device properties for validation
 * @return True if parameters are valid, false otherwise
 */
bool ValidateCompareKernelLaunchParams(
    const dim3& gridDim,
    const dim3& blockDim,
    int pointsPerThread,
    int numTargets,
    const cudaDeviceProp& deviceProps
);

/**
 * @brief Get performance metrics for compare kernels
 *
 * Returns estimated performance metrics based on kernel type and
 * device capabilities.
 *
 * @param kernelType Kernel type (0=basic, 1=advanced, 2=memory-optimized)
 * @param deviceProps Device properties
 * @param pointsPerThread Points per thread
 * @param numTargets Number of target hashes
 * @return Estimated throughput (comparisons/second)
 */
double EstimateCompareKernelThroughput(
    int kernelType,
    const cudaDeviceProp& deviceProps,
    int pointsPerThread,
    int numTargets
);

// Kernel type constants
enum CompareKernelType {
    COMPARE_KERNEL_BASIC = 0,           // 24 registers, basic optimization
    COMPARE_KERNEL_ADVANCED = 1,       // 24 registers, batch processing
    COMPARE_KERNEL_MEMORY_OPTIMIZED = 2, // 24 registers, memory bandwidth optimized
    COMPARE_KERNEL_AUTO = 3             // Auto-select optimal kernel
};

// Performance target constants
namespace compare_performance {
    constexpr int TARGET_REGISTERS_PER_THREAD = 24;
    constexpr double TARGET_OCCUPANCY = 0.85;    // 85% target occupancy (highest)
    constexpr double TARGET_MEMORY_EFFICIENCY = 0.90; // 90% memory coalescing efficiency
    constexpr int MIN_BLOCK_SIZE = 128;
    constexpr int MAX_BLOCK_SIZE = 1024;  // Compare kernels can use larger blocks
    constexpr int OPTIMAL_BLOCK_SIZE = 512;

    // Compare-specific performance targets
    constexpr int HASH160_SIZE = 20;       // 160 bits = 20 bytes
    constexpr int HASH160_WORDS = 5;       // 20 bytes / 4 bytes per word
    constexpr int MAX_TARGETS = 1024;      // Maximum number of target addresses
    constexpr double SINGLE_TARGET_OVERHEAD = 1.0;
    constexpr double MULTI_TARGET_OVERHEAD = 1.2; // 20% overhead for multiple targets
}

} // namespace kernels
} // namespace keyhunt
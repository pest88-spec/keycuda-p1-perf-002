// Puzzle71Solver - Separated ECC Kernel Header
// Register-optimized ECC operations interface (T026)

#pragma once

#include <cuda_runtime.h>
#include <cstdint>

namespace keyhunt {
namespace kernels {

/**
 * @brief Separated ECC Kernel Functions
 *
 * These kernels provide optimized ECC operations with strict register
 * usage limits to maximize GPU occupancy and performance.
 */

/**
 * @brief Basic separated ECC kernel (≤32 registers/thread)
 *
 * Implements core ECC operations with register optimization.
 * Suitable for general-purpose use with moderate batch sizes.
 *
 * @param gridDim Grid dimensions
 * @param blockDim Block dimensions
 * @param pointsPerThread Number of points per thread to process
 * @return CUDA error code
 */
cudaError_t LaunchEccSeparatedKernel(
    dim3 gridDim,
    dim3 blockDim,
    int pointsPerThread,
    cudaStream_t stream = nullptr
);

/**
 * @brief Advanced separated ECC kernel with shared memory optimization
 *
 * Enhanced version using shared memory for batch operations.
 * Best performance for larger batches when shared memory is available.
 *
 * @param gridDim Grid dimensions
 * @param blockDim Block dimensions
 * @param pointsPerThread Number of points per thread to process
 * @param stream CUDA stream (optional)
 * @return CUDA error code
 */
cudaError_t LaunchEccSeparatedKernelAdvanced(
    dim3 gridDim,
    dim3 blockDim,
    int pointsPerThread,
    cudaStream_t stream = nullptr
);

/**
 * @brief Memory-optimized separated ECC kernel
 *
 * Ultra-optimized version focusing on memory bandwidth efficiency.
 * Uses vectorized operations and optimized memory access patterns.
 *
 * @param gridDim Grid dimensions
 * @param blockDim Block dimensions
 * @param pointsPerThread Number of points per thread to process
 * @param stream CUDA stream (optional)
 * @return CUDA error code
 */
cudaError_t LaunchEccSeparatedKernelMemoryOptimized(
    dim3 gridDim,
    dim3 blockDim,
    int pointsPerThread,
    cudaStream_t stream = nullptr
);

/**
 * @brief Auto-select optimal ECC kernel based on device capabilities
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
 * @param deviceProps Device properties (optional)
 * @param stream CUDA stream (optional)
 * @return CUDA error code
 */
cudaError_t LaunchEccKernelAutoOptimized(
    dim3 gridDim,
    dim3 blockDim,
    int pointsPerThread,
    const cudaDeviceProp* deviceProps = nullptr,
    cudaStream_t stream = nullptr
);

/**
 * @brief Calculate optimal kernel configuration
 *
 * Determines the best kernel configuration based on device capabilities
 * and workload characteristics.
 *
 * @param deviceProps Device properties
 * @param pointsPerThread Number of points per thread
 * @param gridSize Output grid dimensions
 * @param blockSize Output block dimensions
 * @param kernelType Output recommended kernel type
 */
void CalculateOptimalEccKernelConfig(
    const cudaDeviceProp& deviceProps,
    int pointsPerThread,
    dim3& gridSize,
    dim3& blockSize,
    int& kernelType
);

/**
 * @brief Validate kernel launch parameters
 *
 * Ensures launch parameters are within device limits and optimal
 * for performance.
 *
 * @param gridDim Grid dimensions to validate
 * @param blockDim Block dimensions to validate
 * @param pointsPerThread Points per thread to validate
 * @param deviceProps Device properties for validation
 * @return True if parameters are valid, false otherwise
 */
bool ValidateEccKernelLaunchParams(
    const dim3& gridDim,
    const dim3& blockDim,
    int pointsPerThread,
    const cudaDeviceProp& deviceProps
);

/**
 * @brief Get performance metrics for ECC kernels
 *
 * Returns estimated performance metrics based on kernel type and
 * device capabilities.
 *
 * @param kernelType Kernel type (0=basic, 1=advanced, 2=memory-optimized)
 * @param deviceProps Device properties
 * @param pointsPerThread Points per thread
 * @return Estimated throughput (points/second)
 */
double EstimateEccKernelThroughputput(
    int kernelType,
    const cudaDeviceProp& deviceProps,
    int pointsPerThread
);

// Kernel type constants
enum EccKernelType {
    ECC_KERNEL_BASIC = 0,              // 32 registers, basic optimization
    ECC_KERNEL_ADVANCED = 1,          // 32 registers, shared memory
    ECC_KERNEL_MEMORY_OPTIMIZED = 2,    // 32 registers, memory bandwidth optimized
    ECC_KERNEL_AUTO = 3                // Auto-select optimal kernel
};

// Performance target constants
namespace ecc_performance {
    constexpr int TARGET_REGISTERS_PER_THREAD = 32;
    constexpr double TARGET_OCCUPANCY = 0.80;    // 80% target occupancy
    constexpr double TARGET_MEMORY_EFFICIENCY = 0.90; // 90% memory coalescing efficiency
    constexpr int MIN_BLOCK_SIZE = 128;
    constexpr int MAX_BLOCK_SIZE = 512;
    constexpr int OPTIMAL_BLOCK_SIZE = 256;
}

} // namespace kernels
} // namespace keyhunt
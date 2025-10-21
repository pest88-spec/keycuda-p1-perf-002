// Puzzle71Solver - Separated Kernel Executor Header
// High-performance executor using separated kernels (T036)

#pragma once

#include <cuda_runtime.h>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>

#include "../../KeyhuntCore/kernels/ecc_separated.cuh"
#include "../../KeyhuntCore/kernels/hash_separated.cuh"
#include "../../KeyhuntCore/kernels/compare_separated.cuh"
#include "../../KeyhuntCore/kernels/memory_optimized.cuh"
#include "../../KeyhuntCore/kernels/warp_operations.cuh"
#include "../../KeyhuntCore/memory/soa_memory_manager.cuh"
#include "../../KeyhuntCore/performance/adaptive_batch_sizer.cuh"
#include "../../KeyhuntCore/memory/gpu_memory_pool.cuh"
#include "batch_planner.h"
#include "gpu_executor.h"

namespace puzzle71 {
namespace gpu {

/**
 * @brief Separated Kernel Execution Configuration
 *
 * Configuration for the high-performance separated kernel system
 * with adaptive optimization and comprehensive monitoring.
 */
struct SeparatedKernelConfig {
    // Kernel selection options
    keyhunt::kernels::HashKernelType hash_kernel_type;
    keyhunt::kernels::EccKernelType ecc_kernel_type;
    keyhunt::kernels::CompareKernelType compare_kernel_type;

    // Performance optimization options
    bool use_memory_pool;
    bool use_soa_layout;
    bool use_optimized_memory_functions;
    bool use_warp_operations;
    bool enable_adaptive_batching;

    // Batch sizing options
    bool auto_batch_sizing;
    keyhunt::performance::OptimizationObjective batch_objective;
    std::string batch_optimizer_type;

    // Memory management options
    size_t memory_pool_size_mb;
    bool enable_garbage_collection;
    bool enable_defragmentation;

    // Monitoring and debugging
    bool enable_performance_metrics;
    bool enable_kernel_profiling;
    bool enable_debug_logging;
    bool validate_results;

    // Advanced options
    bool enable_stream_parallelism;
    int max_concurrent_streams;
    bool enable_pinned_memory;
    bool enable_unified_memory;

    SeparatedKernelConfig()
        : hash_kernel_type(keyhunt::kernels::HASH_KERNEL_AUTO),
          ecc_kernel_type(keyhunt::kernels::ECC_SEPARATED_BASIC),
          compare_kernel_type(keyhunt::kernels::COMPARE_SEPARATED_BASIC),
          use_memory_pool(true),
          use_soa_layout(true),
          use_optimized_memory_functions(true),
          use_warp_operations(true),
          enable_adaptive_batching(true),
          auto_batch_sizing(true),
          batch_objective(keyhunt::performance::OptimizationObjective::MAXIMIZE_THROUGHPUT),
          batch_optimizer_type("gradient_descent"),
          memory_pool_size_mb(2048),
          enable_garbage_collection(true),
          enable_defragmentation(true),
          enable_performance_metrics(true),
          enable_kernel_profiling(false),
          enable_debug_logging(false),
          validate_results(true),
          enable_stream_parallelism(false),
          max_concurrent_streams(1),
          enable_pinned_memory(true),
          enable_unified_memory(false) {}
};

/**
 * @brief Execution step result with comprehensive metrics
 */
struct SeparatedExecutionStep {
    // Basic execution results
    std::uint64_t processed_keys;
    std::chrono::microseconds elapsed_time;
    std::vector<puzzle71::reference_adapter::GpuCandidate> candidates;
    std::size_t dropped_candidates;

    // Performance metrics
    double keys_per_sec;
    double memory_bandwidth_gb_per_sec;
    double gpu_utilization_percent;
    double memory_efficiency_percent;
    size_t memory_usage_mb;
    size_t cache_hit_rate_percent;

    // Kernel-specific metrics
    double ecc_kernel_throughput_mkeys_per_sec;
    double hash_kernel_throughput_mkeys_per_sec;
    double compare_kernel_throughput_mkeys_per_sec;

    // Advanced metrics
    double warp_utilization_percent;
    double register_efficiency_percent;
    double shared_memory_utilization_percent;
    double kernel_launch_overhead_us;

    // Adaptive metrics
    std::size_t batch_adaptations;
    double batch_size_efficiency;
    std::string adaptation_reason;

    // Memory pool metrics
    double memory_pool_hit_ratio;
    size_t memory_pool_freed_bytes;
    std::chrono::microseconds pool_allocation_time;

    // Timing breakdown
    std::chrono::microseconds ecc_time;
    std::chrono::microseconds hash_time;
    std::chrono::microseconds compare_time;
    std::chrono::microseconds transfer_time;

    // Error and validation
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    bool validation_passed;
};

/**
 * @brief High-performance separated kernel executor
 *
 * This executor provides optimized GPU execution using the separated
 * kernels with comprehensive performance monitoring and adaptive optimization.
 *
 * Key Features:
 * - Separated kernel execution with register optimization
 * - Adaptive batch sizing based on performance metrics
 * - Memory pool management for efficient allocation
 * - Structure-of-Arrays layout for optimal memory access
 * - Warp-level atomic operations for synchronization
 * - Real-time performance monitoring and adaptation
 * - Comprehensive error handling and validation
 */
class SeparatedKernelExecutor {
private:
    int device_id_;
    bool compressed_;
    std::array<std::uint32_t, 5> target_hash160_;
    bool verbose_;

    // Configuration
    SeparatedKernelConfig config_;

    // GPU resources
    cudaDeviceProp device_props_;
    std::vector<cudaStream_t> streams_;

    // Memory management
    std::unique_ptr<keyhunt::memory::SoAArray<unsigned int, 8>> ecc_points_x_;
    std::unique_ptr<keyhunt::memory::SoAArray<unsigned int, 8>> ecc_points_y_;
    std::unique_ptr<keyhunt::memory::HashDigestsSoA> hash_digests_;
    std::unique_ptr<keyhunt::memory::BatchResultsSoA> batch_results_;

    // Performance optimization
    std::unique_ptr<keyhunt::performance::AdaptiveBatchSizer> batch_sizer_;
    std::unique_ptr<keyhunt::memory::MemoryPoolBase> memory_pool_;

    // Current execution state
    BatchConfig current_batch_config_;
    keyhunt::performance::BatchConfiguration current_batch_sizing_;

public:
    /**
     * @brief Construct separated kernel executor
     */
    SeparatedKernelExecutor(
        int device_id,
        bool compressed,
        const std::array<std::uint32_t, 5>& target_hash160,
        const SeparatedKernelConfig& config = SeparatedKernelConfig(),
        bool verbose = false
    );

    /**
     * @brief Destructor
     */
    ~SeparatedKernelExecutor();

    /**
     * @brief Prepare batch for execution
     */
    void PrepareBatch(const BatchConfig& batch_config, const core::UInt256& batch_start);

    /**
     * @brief Execute separated kernels
     */
    SeparatedExecutionStep Execute();

    /**
     * @brief Get current batch configuration
     */
    const BatchConfig& GetCurrentBatchConfig() const {
        return current_batch_config_;
    }

    /**
     * @brief Get execution statistics
     */
    struct ExecutionStats {
        std::uint64_t total_keys_processed;
        double average_keys_per_sec;
        double peak_keys_per_sec;
        std::size_t total_candidates_found;
        double average_batch_efficiency;
        std::size_t total_adaptations;
        double average_memory_pool_hit_ratio;
        std::chrono::milliseconds total_execution_time;
    };

    ExecutionStats GetExecutionStats() const;

    /**
     * @brief Get performance configuration
     */
    const SeparatedKernelConfig& GetConfig() const {
        return config_;
    }

    /**
     * @brief Update configuration
     */
    void UpdateConfig(const SeparatedKernelConfig& config);

    /**
     * @brief Force garbage collection
     */
    void ForceGarbageCollection();

    /**
     * @brief Force memory defragmentation
     */
    void ForceMemoryDefragmentation();

    /**
     * @brief Get detailed performance report
     */
    std::string GetPerformanceReport() const;

private:
    /**
     * @brief Initialize GPU resources
     */
    void InitializeGpuResources();

    /**
     * @brief Initialize memory management
     */
    void InitializeMemoryManagement();

    /**
     * @brief Initialize performance optimization systems
     */
    void InitializePerformanceOptimization();

    /**
     * @brief Allocate SoA memory for current batch
     */
    void AllocateSoAMemory(size_t num_points);

    /**
     * @brief Execute ECC separated kernel
     */
    SeparatedExecutionStep ExecuteEccKernel();

    /**
     * @brief Execute hash separated kernel
     */
    SeparatedExecutionStep ExecuteHashKernel();

    /**
     * @brief Execute compare separated kernel
     */
    SeparatedExecutionStep ExecuteCompareKernel();

    /**
     * @brief Execute optimized kernel pipeline
     */
    SeparatedExecutionStep ExecuteOptimizedPipeline();

    /**
     * @brief Validate execution results
     */
    bool ValidateResults(const SeparatedExecutionStep& step);

    /**
     * @brief Collect performance metrics
     */
    void CollectPerformanceMetrics(SeparatedExecutionStep& step);

    /**
     * @brief Update adaptive systems
     */
    void UpdateAdaptiveSystems(const SeparatedExecutionStep& step);

    /**
     * @brief Calculate optimal kernel configuration
     */
    void CalculateOptimalKernelConfig();

    /**
     * @brief Setup CUDA streams for parallel execution
     */
    void SetupCudaStreams();

    /**
     * @brief Cleanup CUDA streams
     */
    void CleanupCudaStreams();

    /**
     * @brief Transfer data between host and device
     */
    void TransferDataHostToDevice(const core::UInt256& batch_start, size_t num_points);
    void TransferResultsDeviceToHost();

    /**
     * @brief Handle out-of-memory situations
     */
    bool HandleOutOfMemory(SeparatedExecutionStep& step);

    /**
     * @brief Optimize batch configuration based on performance
     */
    void OptimizeBatchConfiguration(SeparatedExecutionStep& step);

    /**
     * @brief Log performance information
     */
    void LogPerformanceInfo(const SeparatedExecutionStep& step);

    /**
     * @brief Generate performance report
     */
    std::string GeneratePerformanceReport() const;
};

/**
 * @brief Factory for creating separated kernel executors
 */
class SeparatedKernelExecutorFactory {
public:
    /**
     * @brief Create default separated kernel executor
     */
    static std::unique_ptr<SeparatedKernelExecutor> Create(
        int device_id,
        bool compressed,
        const std::array<std::uint32_t, 5>& target_hash160,
        bool verbose = false
    );

    /**
     * @brief Create high-performance executor
     */
    static std::unique_ptr<SeparatedKernelExecutor> CreateHighPerformance(
        int device_id,
        bool compressed,
        const std::array<std::uint32_t, 5>& target_hash160,
        bool verbose = false
    );

    /**
     * @brief Create memory-optimized executor
     */
    static std::unique_ptr<SeparatedKernelExecutor> CreateMemoryOptimized(
        int device_id,
        bool compressed,
        const std::array<std::uint32_t, 5>& target_hash160,
        bool verbose = false
    );

    /**
     * @brief Create custom executor with configuration
     */
    static std::unique_ptr<SeparatedKernelExecutor> CreateCustom(
        int device_id,
        bool compressed,
        const std::array<std::uint32_t, 5>& target_hash160,
        const SeparatedKernelConfig& config,
        bool verbose = false
    );
};

/**
 * @brief Utility functions for separated kernel execution
 */
namespace SeparatedKernelUtils {

/**
 * @brief Compare performance between original and separated kernels
 */
struct PerformanceComparison {
    double separated_kernel_throughput_mkeys_per_sec;
    double original_kernel_throughput_mkeys_per_sec;
    double speedup_factor;
    double memory_efficiency_improvement;
    double register_efficiency_improvement;
    std::chrono::microseconds latency_improvement;
};

/**
 * @brief Run performance comparison
 */
PerformanceComparison RunPerformanceComparison(
    int device_id,
    const std::array<std::uint32_t, 5>& target_hash160,
    size_t test_size = 1000000,
    int num_iterations = 5
);

/**
 * @brief Validate separated kernel correctness
 */
bool ValidateSeparatedKernelCorrectness(
    int device_id,
    const std::array<std::uint32_t, 5>& target_hash160,
    size_t test_cases = 10000
);

/**
 * @brief Profile kernel register usage
 */
struct RegisterUsageProfile {
    int ecc_kernel_registers_per_thread;
    int hash_kernel_registers_per_thread;
    int compare_kernel_registers_per_thread;
    bool ecc_within_limit;
    bool hash_within_limit;
    bool compare_within_limit;
};

RegisterUsageProfile ProfileKernelRegisterUsage(
    int device_id,
    const SeparatedKernelConfig& config
);

} // namespace SeparatedKernelUtils

} // namespace gpu
} // namespace puzzle71
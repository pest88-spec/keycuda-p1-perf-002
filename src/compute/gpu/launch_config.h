// Puzzle71Solver - Kernel Launch Configuration System (T041)
// Architecture Modernization - Standardized kernel configuration management
//
// This system consolidates all kernel launch configuration logic into a unified,
// high-performance framework that supports all kernel types (fused, separated, etc.).
//
// Key Features:
// - Unified configuration for all kernel types
// - Architecture-aware optimization
// - Dynamic register pressure management
// - Deterministic replay support
// - Performance telemetry integration
// - Zero-overhead configuration selection

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <cuda_runtime.h>

#include "core/uint256.h"
#include "compute/gpu/device_results.h"

namespace puzzle71::gpu {

/**
 * @brief Kernel type enumeration for configuration specialization
 */
enum class KernelType {
    FUSED_PUZZLE71,        // Original monolithic kernel
    SEPARATED_ECC,         // Separated ECC kernel
    SEPARATED_HASH,        // Separated hash kernel
    SEPARATED_COMPARE,     // Separated compare kernel
    SEPARATED_PIPELINE,    // Complete separated pipeline
    MEMORY_OPTIMIZED,      // Memory-optimized variant
    BENCHMARK_KERNEL       // Performance testing kernel
};

/**
 * @brief Execution optimization objective
 */
enum class OptimizationObjective {
    MAXIMIZE_THROUGHPUT,   // Maximum keys per second
    MINIMIZE_LATENCY,      // Fastest single batch execution
    BALANCE_PERFORMANCE,   // Balanced throughput and latency
    MINIMIZE_MEMORY,       // Minimum memory usage
    DETERMINISTIC_REPLAY   // Consistent performance for testing
};

/**
 * @brief GPU architecture-specific constraints
 */
struct GpuConstraints {
    int max_registers_per_thread{80};
    int max_shared_memory_per_block{48 * 1024};
    int max_threads_per_block{1024};
    int max_threads_per_sm{2048};
    int max_blocks_per_sm{32};
    int warp_size{32};
    size_t total_global_memory{0};
    int compute_capability_major{7};
    int compute_capability_minor{5};
    std::string device_name{"Unknown"};

    /**
     * @brief Check if the device supports separated kernels
     */
    bool supportsSeparatedKernels() const {
        return compute_capability_major >= 7; // Turing and later
    }

    /**
     * @brief Get optimal block size for this architecture
     */
    dim3 getOptimalBlockSize(KernelType kernel_type) const;

    /**
     * @brief Get maximum threads per block for this kernel type
     */
    int getMaxThreadsPerBlock(KernelType kernel_type) const;
};

/**
 * @brief Resource usage profile for kernel configuration
 */
struct ResourceProfile {
    int registers_per_thread{0};
    int shared_memory_per_block{0};
    int threads_per_block{0};
    double expected_occupancy{0.0};
    size_t memory_bandwidth_gb_per_sec{0};
    double compute_utilization{0.0};

    /**
     * @brief Check if this profile is feasible for the given constraints
     */
    bool isFeasible(const GpuConstraints& constraints) const;

    /**
     * @brief Calculate theoretical throughput for this profile
     */
    double calculateThroughput(const GpuConstraints& constraints) const;
};

/**
 * @brief Complete kernel launch configuration
 *
 * This structure replaces the scattered kernel configuration logic with a
 * unified, comprehensive configuration system.
 */
struct KernelLaunchConfig {
    // Basic launch parameters
    KernelType kernel_type{KernelType::FUSED_PUZZLE71};
    dim3 grid{1, 1, 1};
    dim3 block{32, 1, 1};
    int points_per_thread{1};
    std::uint64_t batch_size{0};

    // Performance optimization parameters
    OptimizationObjective objective{OptimizationObjective::MAXIMIZE_THROUGHPUT};
    ResourceProfile resource_profile{};
    GpuConstraints device_constraints{};

    // Advanced configuration
    bool use_pinned_memory{true};
    bool use_unified_memory{false};
    bool enable_profiling{false};
    bool enable_deterministic_mode{false};
    std::uint64_t deterministic_seed{0};

    // Memory management
    size_t memory_pool_size_mb{2048};
    bool enable_memory_defragmentation{true};
    bool enable_adaptive_batching{true};

    // Kernel-specific parameters
    bool use_separated_kernels{false};
    bool enable_warp_operations{true};
    bool enable_shared_memory_optimization{true};

    // Validation and debugging
    bool validate_results{true};
    bool enable_register_audit{false};
    int debug_level{0};

    /**
     * @brief Calculate total thread count for this configuration
     */
    std::uint64_t getTotalThreads() const {
        return static_cast<std::uint64_t>(grid.x) * grid.y * grid.z *
               static_cast<std::uint64_t>(block.x) * block.y * block.z;
    }

    /**
     * @brief Calculate theoretical keys per second
     */
    double calculateTheoreticalThroughput() const {
        return resource_profile.calculateThroughput(device_constraints);
    }

    /**
     * @brief Validate this configuration
     */
    bool validate() const;

    /**
     * @brief Get configuration as string for logging
     */
    std::string toString() const;

    /**
     * @brief Serialize configuration to JSON
     */
    std::string toJson() const;

    /**
     * @brief Deserialize configuration from JSON
     */
    static KernelLaunchConfig fromJson(const std::string& json);
};

/**
 * @brief Kernel launch configuration manager
 *
 * This class provides intelligent configuration selection based on:
 * - GPU architecture capabilities
 * - Performance objectives
 * - Resource constraints
 * - Historical performance data
 */
class LaunchConfigManager {
public:
    explicit LaunchConfigManager(int device_id);
    ~LaunchConfigManager();

    /**
     * @brief Get optimal configuration for given parameters
     *
     * @param kernel_type Type of kernel to configure
     * @param batch_size Desired batch size
     * @param objective Performance optimization objective
     * @return Optimal kernel launch configuration
     */
    KernelLaunchConfig getOptimalConfig(
        KernelType kernel_type,
        std::uint64_t batch_size,
        OptimizationObjective objective = OptimizationObjective::MAXIMIZE_THROUGHPUT
    );

    /**
     * @brief Get configuration for separated kernel pipeline
     *
     * @param batch_size Desired batch size
     * @param objective Performance optimization objective
     * @return Configuration for separated kernel execution
     */
    KernelLaunchConfig getSeparatedKernelConfig(
        std::uint64_t batch_size,
        OptimizationObjective objective = OptimizationObjective::MAXIMIZE_THROUGHPUT
    );

    /**
     * @brief Get deterministic replay configuration
     *
     * @param base_config Base configuration to make deterministic
     * @param seed Deterministic seed for reproducible results
     * @return Deterministic configuration
     */
    KernelLaunchConfig getDeterministicConfig(
        const KernelLaunchConfig& base_config,
        std::uint64_t seed
    );

    /**
     * @brief Validate configuration against device capabilities
     *
     * @param config Configuration to validate
     * @return true if configuration is valid and safe
     */
    bool validateConfig(const KernelLaunchConfig& config) const;

    /**
     * @brief Update configuration based on performance feedback
     *
     * @param config Current configuration
     * @param actual_throughput Measured performance
     * @param actual_occupancy Measured GPU occupancy
     * @return Optimized configuration
     */
    KernelLaunchConfig optimizeFromPerformance(
        const KernelLaunchConfig& config,
        double actual_throughput,
        double actual_occupancy
    );

    /**
     * @brief Get device constraints
     */
    const GpuConstraints& getDeviceConstraints() const {
        return device_constraints_;
    }

    /**
     * @brief Export current configuration cache
     */
    void exportConfigurationCache(const std::string& filename) const;

    /**
     * @brief Import configuration cache
     */
    void importConfigurationCache(const std::string& filename);

private:
    int device_id_;
    GpuConstraints device_constraints_;
    std::vector<KernelLaunchConfig> config_cache_;

    /**
     * @brief Initialize device constraints for the given device
     */
    void initializeDeviceConstraints();

    /**
     * @brief Calculate optimal grid size for given parameters
     */
    dim3 calculateOptimalGrid(const KernelLaunchConfig& base_config) const;

    /**
     * @brief Calculate optimal block size for given parameters
     */
    dim3 calculateOptimalBlock(KernelType kernel_type, const GpuConstraints& constraints) const;

    /**
     * @brief Estimate resource usage for kernel configuration
     */
    ResourceProfile estimateResourceUsage(const KernelLaunchConfig& config) const;

    /**
     * @brief Find cached configuration or create new one
     */
    KernelLaunchConfig findOrCreateConfig(const KernelLaunchConfig& template_config);
};

/**
 * @brief Utility functions for kernel launch configuration
 */
namespace launch_utils {

/**
 * @brief Convert configuration to CUDA launch parameters
 */
struct CudaLaunchParams {
    dim3 grid;
    dim3 block;
    size_t shared_memory_size;
    cudaStream_t stream;
};

CudaLaunchParams toCudaParams(const KernelLaunchConfig& config, cudaStream_t stream = 0);

/**
 * @brief Calculate memory requirements for configuration
 */
struct MemoryRequirements {
    size_t device_memory_bytes{0};
    size_t pinned_memory_bytes{0};
    size_t unified_memory_bytes{0};
    size_t shared_memory_bytes{0};
};

MemoryRequirements calculateMemoryRequirements(const KernelLaunchConfig& config);

/**
 * @brief Validate kernel launch parameters
 */
bool validateLaunchParams(const CudaLaunchParams& params, const GpuConstraints& constraints);

/**
 * @brief Generate configuration fingerprint for caching
 */
std::uint64_t generateConfigFingerprint(const KernelLaunchConfig& config);

} // namespace launch_utils

} // namespace puzzle71::gpu
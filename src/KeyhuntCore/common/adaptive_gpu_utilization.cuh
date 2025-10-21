/**
 * @file adaptive_gpu_utilization.cuh
 * @brief Adaptive GPU utilization optimization system for Puzzle71 Technical Debt Repair
 *
 * This file implements an advanced adaptive GPU utilization optimization system that
 * dynamically adjusts workload distribution and kernel parameters to achieve optimal
 * GPU utilization. The system focuses on:
 *
 * - Real-time GPU utilization monitoring and analysis
 * - Adaptive workload balancing across multiple GPUs
 * - Dynamic kernel launch parameter optimization
 * - Intelligent power management and thermal throttling avoidance
 * - Performance feedback loops and auto-tuning
 * - Multi-GPU scaling and load distribution
 * - Constitutional compliance with v5.5 performance requirements
 *
 * @author Puzzle71 Technical Debt Repair System
 * @version 5.5
 * @date 2025-10-21
 * @copyright Constitutional Compliance v5.5
 */

#pragma once

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cuda.h>
#include <vector>
#include <atomic>
#include <chrono>
#include <memory>
#include "warp_primitives.cuh"
#include "shared_memory_optimization.cuh"

namespace keyhunt {
namespace gpu {
namespace adaptive {

// ============================================================================
// ADAPTIVE GPU UTILIZATION CONFIGURATION CONSTANTS
// ============================================================================

/**
 * Performance target constants (constitutional requirements v5.5)
 */
constexpr double TARGET_GPU_UTILIZATION = 0.80;              // >80% utilization target
constexpr double MINIMUM_GPU_UTILIZATION = 0.70;             // 70% minimum requirement
constexpr double OPTIMAL_GPU_UTILIZATION = 0.95;             // 95% optimal target
constexpr uint32_t UTILIZATION_SAMPLING_INTERVAL_MS = 100;   // 100ms sampling interval
constexpr uint32_t ADAPTATION_WINDOW_SIZE = 10;              // 10 samples for adaptation
constexpr double THERMAL_THROTTLE_THRESHOLD = 0.85;         // 85% thermal threshold

/**
 * GPU device capability tiers for adaptive optimization
 */
enum class GPUCapabilityTier : uint32_t {
    BASIC = 0,      // Compute capability < 6.0
    STANDARD = 1,   // Compute capability 6.0 - 7.5
    ADVANCED = 2,   // Compute capability 8.0 - 8.6
    CUTTING_EDGE = 3 // Compute capability >= 9.0
};

/**
 * Workload classification for adaptive scheduling
 */
enum class WorkloadType : uint32_t {
    COMPUTE_INTENSIVE = 0,    // Heavy arithmetic operations
    MEMORY_BOUND = 1,        // Memory bandwidth limited
    LATENCY_SENSITIVE = 2,   // Low latency requirements
    THROUGHPUT_ORIENTED = 3, // High throughput requirements
    BALANCED = 4             // Mixed workload characteristics
};

/**
 * GPU utilization metrics structure
 */
struct GPUMetrics {
    double utilization_percentage;
    double memory_bandwidth_utilization;
    double compute_utilization;
    double power_consumption_watts;
    double temperature_celsius;
    uint64_t active_warps;
    uint64_t resident_warps;
    uint64_t issued_instructions;
    uint64_t executed_instructions;
    uint64_t global_load_transactions;
    uint64_t global_store_transactions;
    uint64_t shared_load_transactions;
    uint64_t shared_store_transactions;
    uint64_t l2_cache_hits;
    uint64_t l2_cache_misses;
    uint64_t l1_cache_hits;
    uint64_t l1_cache_misses;

    __host__ __device__ GPUMetrics()
        : utilization_percentage(0.0), memory_bandwidth_utilization(0.0),
          compute_utilization(0.0), power_consumption_watts(0.0),
          temperature_celsius(0.0), active_warps(0), resident_warps(0),
          issued_instructions(0), executed_instructions(0),
          global_load_transactions(0), global_store_transactions(0),
          shared_load_transactions(0), shared_store_transactions(0),
          l2_cache_hits(0), l2_cache_misses(0),
          l1_cache_hits(0), l1_cache_misses(0) {}
};

/**
 * Adaptive configuration parameters
 */
struct AdaptiveConfig {
    uint32_t optimal_block_size;
    uint32_t optimal_grid_size;
    uint32_t optimal_shared_memory_size;
    uint32_t registers_per_thread;
    double occupancy_target;
    WorkloadType workload_type;
    GPUCapabilityTier capability_tier;
    bool enable_dynamic_parallelism;
    bool enable_persistent_threads;
    bool enable_cooperative_launch;

    __host__ __device__ AdaptiveConfig()
        : optimal_block_size(256), optimal_grid_size(1), optimal_shared_memory_size(0),
          registers_per_thread(32), occupancy_target(0.80), workload_type(WorkloadType::BALANCED),
          capability_tier(GPUCapabilityTier::STANDARD), enable_dynamic_parallelism(false),
          enable_persistent_threads(false), enable_cooperative_launch(false) {}
};

// ============================================================================
// ADAPTIVE GPU UTILIZATION MONITOR
// ============================================================================

/**
 * Real-time GPU utilization monitor with adaptive feedback
 */
class GPUUtilizationMonitor {
private:
    int device_id_;
    GPUCapabilityTier capability_tier_;
    std::vector<GPUMetrics> metrics_history_;
    std::atomic<bool> monitoring_active_;
    std::atomic<uint64_t> sampling_count_;
    cudaEvent_t start_event_;
    cudaEvent_t end_event_;
    std::chrono::high_resolution_clock::time_point last_sample_time_;

    // Performance baseline for comparison
    GPUMetrics baseline_metrics_;
    double average_utilization_;
    double utilization_variance_;
    bool thermal_throttling_detected_;

public:
    /**
     * Constructor for GPU utilization monitor
     */
    explicit GPUUtilizationMonitor(int device_id = 0);

    /**
     * Destructor
     */
    ~GPUUtilizationMonitor();

    /**
     * Start real-time monitoring
     */
    bool startMonitoring();

    /**
     * Stop monitoring and collect final statistics
     */
    void stopMonitoring();

    /**
     * Collect current GPU metrics
     */
    GPUMetrics collectCurrentMetrics();

    /**
     * Get average utilization over monitoring period
     */
    double getAverageUtilization() const { return average_utilization_; }

    /**
     * Get utilization variance (stability measure)
     */
    double getUtilizationVariance() const { return utilization_variance_; }

    /**
     * Check if thermal throttling is detected
     */
    bool isThermalThrottling() const { return thermal_throttling_detected_; }

    /**
     * Get GPU capability tier
     */
    GPUCapabilityTier getCapabilityTier() const { return capability_tier_; }

    /**
     * Get monitoring statistics
     */
    uint64_t getSampleCount() const { return sampling_count_.load(); }

    /**
     * Check if monitoring is active
     */
    bool isMonitoringActive() const { return monitoring_active_.load(); }

private:
    /**
     * Initialize monitoring system
     */
    bool initialize();

    /**
     * Update utilization statistics
     */
    void updateStatistics(const GPUMetrics& metrics);

    /**
     * Detect thermal throttling
     */
    bool detectThermalThrottling(const GPUMetrics& metrics);

    /**
     * Calculate GPU capability tier
     */
    GPUCapabilityTier calculateCapabilityTier(int device_id);
};

// ============================================================================
// ADAPTIVE WORKLOAD BALANCER
// ============================================================================

/**
 * Multi-GPU adaptive workload balancer
 */
class AdaptiveWorkloadBalancer {
private:
    std::vector<int> available_devices_;
    std::vector<double> device_utilization_;
    std::vector<uint64_t> device_workload_capacity_;
    std::vector<AdaptiveConfig> device_configs_;
    uint32_t total_workload_units_;
    WorkloadType workload_type_;

public:
    /**
     * Constructor for adaptive workload balancer
     */
    explicit AdaptiveWorkloadBalancer(WorkloadType workload_type = WorkloadType::BALANCED);

    /**
     * Discover and initialize available GPUs
     */
    bool initializeDevices();

    /**
     * Distribute workload across available GPUs
     */
    std::vector<std::pair<int, uint32_t>> distributeWorkload(uint32_t total_units);

    /**
     * Update device utilization feedback
     */
    void updateDeviceUtilization(int device_id, double utilization);

    /**
     * Get optimal configuration for specific device
     */
    AdaptiveConfig getOptimalConfig(int device_id) const;

    /**
     * Rebalance workload based on performance feedback
     */
    void rebalanceWorkload();

    /**
     * Get load balancing statistics
     */
    double getLoadBalancingEfficiency() const;

private:
    /**
     * Calculate device performance score
     */
    double calculateDeviceScore(int device_id);

    /**
     * Optimize configuration for device capabilities
     */
    AdaptiveConfig optimizeConfiguration(int device_id, WorkloadType workload_type);

    /**
     * Estimate device workload capacity
     */
    uint64_t estimateWorkloadCapacity(int device_id);
};

// ============================================================================
// DYNAMIC KERNEL CONFIGURATION OPTIMIZER
// ============================================================================

/**
 * Dynamic kernel configuration optimizer
 */
class KernelConfigOptimizer {
private:
    struct ConfigPerformance {
        uint32_t block_size;
        uint32_t grid_size;
        uint32_t shared_memory_size;
        double execution_time_ms;
        double gpu_utilization;
        double memory_bandwidth_utilization;
        uint64_t throughput;
    };

    std::vector<ConfigPerformance> performance_history_;
    AdaptiveConfig current_config_;
    uint32_t optimization_iterations_;
    bool auto_tuning_enabled_;

public:
    /**
     * Constructor for kernel configuration optimizer
     */
    explicit KernelConfigOptimizer(bool auto_tuning = true);

    /**
     * Initialize with baseline configuration
     */
    void initialize(const AdaptiveConfig& baseline_config);

    /**
     * Optimize kernel configuration based on performance feedback
     */
    AdaptiveConfig optimizeConfiguration(const GPUMetrics& metrics, uint64_t execution_time_ns);

    /**
     * Record performance for current configuration
     */
    void recordPerformance(const AdaptiveConfig& config, const GPUMetrics& metrics, uint64_t execution_time_ns);

    /**
     * Get current optimal configuration
     */
    const AdaptiveConfig& getCurrentConfig() const { return current_config_; }

    /**
     * Enable/disable auto-tuning
     */
    void setAutoTuning(bool enabled) { auto_tuning_enabled_ = enabled; }

    /**
     * Get optimization statistics
     */
    uint32_t getOptimizationIterations() const { return optimization_iterations_; }

private:
    /**
     * Analyze performance trends
     */
    void analyzePerformanceTrends();

    /**
     * Generate candidate configurations
     */
    std::vector<AdaptiveConfig> generateCandidates(const AdaptiveConfig& base_config);

    /**
     * Evaluate configuration performance
     */
    double evaluateConfig(const AdaptiveConfig& config, const GPUMetrics& metrics);

    /**
     * Select best performing configuration
     */
    AdaptiveConfig selectBestConfig(const std::vector<ConfigPerformance>& performances);
};

// ============================================================================
// PERSISTENT THREAD KERNEL MANAGER
// ============================================================================

/**
 * Persistent thread kernel manager for sustained GPU utilization
 */
class PersistentThreadManager {
private:
    bool persistent_threads_enabled_;
    uint32_t persistent_thread_count_;
    cudaStream_t persistent_stream_;
    void* persistent_kernel_state_;
    size_t state_size_;

public:
    /**
     * Constructor for persistent thread manager
     */
    explicit PersistentThreadManager(uint32_t thread_count = 0);

    /**
     * Destructor
     */
    ~PersistentThreadManager();

    /**
     * Initialize persistent threads
     */
    bool initialize();

    /**
     * Launch persistent kernel
     */
    bool launchPersistentKernel();

    /**
     * Submit work to persistent threads
     */
    bool submitWork(void* work_data, size_t work_size);

    /**
     * Stop persistent threads
     */
    void stopPersistentThreads();

    /**
     * Check if persistent threads are active
     */
    bool isActive() const { return persistent_threads_enabled_; }

private:
    /**
     * Calculate optimal persistent thread count
     */
    uint32_t calculateOptimalThreadCount();

    /**
     * Allocate persistent kernel state
     */
    bool allocateKernelState();

    /**
     * Cleanup persistent resources
     */
    void cleanup();
};

// ============================================================================
// ADAPTIVE GPU UTILIZATION COORDINATOR
// ============================================================================

/**
 * Main coordinator for adaptive GPU utilization optimization
 */
class AdaptiveGPUUtilizationCoordinator {
private:
    std::unique_ptr<GPUUtilizationMonitor> utilization_monitor_;
    std::unique_ptr<AdaptiveWorkloadBalancer> workload_balancer_;
    std::unique_ptr<KernelConfigOptimizer> config_optimizer_;
    std::unique_ptr<PersistentThreadManager> persistent_thread_manager_;

    std::atomic<bool> adaptation_active_;
    std::atomic<uint32_t> adaptation_cycles_;
    std::chrono::high_resolution_clock::time_point last_adaptation_;

    // Performance targets
    double target_utilization_;
    double minimum_acceptable_utilization_;
    uint32_t adaptation_interval_ms_;

    // Current state
    AdaptiveConfig current_config_;
    GPUMetrics current_metrics_;
    bool performance_optimized_;

public:
    /**
     * Constructor for adaptive GPU utilization coordinator
     */
    explicit AdaptiveGPUUtilizationCoordinator(
        double target_utilization = TARGET_GPU_UTILIZATION,
        double minimum_utilization = MINIMUM_GPU_UTILIZATION
    );

    /**
     * Destructor
     */
    ~AdaptiveGPUUtilizationCoordinator();

    /**
     * Initialize adaptive optimization system
     */
    bool initialize(WorkloadType workload_type = WorkloadType::BALANCED);

    /**
     * Start adaptive optimization
     */
    bool startAdaptation();

    /**
     * Stop adaptive optimization
     */
    void stopAdaptation();

    /**
     * Get current optimal configuration
     */
    AdaptiveConfig getOptimalConfiguration() const { return current_config_; }

    /**
     * Update system with current performance metrics
     */
    void updateMetrics(const GPUMetrics& metrics);

    /**
     * Force immediate adaptation
     */
    void forceAdaptation();

    /**
     * Check if system is optimized
     */
    bool isOptimized() const { return performance_optimized_; }

    /**
     * Get adaptation statistics
     */
    uint32_t getAdaptationCycles() const { return adaptation_cycles_.load(); }

    /**
     * Get current utilization
     */
    double getCurrentUtilization() const { return current_metrics_.utilization_percentage; }

    /**
     * Check if adaptation is active
     */
    bool isAdaptationActive() const { return adaptation_active_.load(); }

private:
    /**
     * Perform adaptive optimization cycle
     */
    void performAdaptationCycle();

    /**
     * Analyze performance and identify optimization opportunities
     */
    void analyzePerformance();

    /**
     * Apply optimization strategies
     */
    void applyOptimizations();

    /**
     * Validate optimization effectiveness
     */
    bool validateOptimizations();

    /**
     * Handle thermal throttling
     */
    void handleThermalThrottling();

    /**
     * Adjust kernel launch parameters
     */
    void adjustKernelParameters();

    /**
     * Optimize memory access patterns
     */
    void optimizeMemoryAccess();

    /**
     * Balance workload across devices
     */
    void balanceWorkload();
};

// ============================================================================
// DEVICE-SIDE ADAPTIVE KERNELS
// ============================================================================

/**
 * Device-side adaptive kernel configuration
 */
struct DeviceAdaptiveConfig {
    uint32_t dynamic_block_size;
    uint32_t dynamic_grid_size;
    uint32_t adaptive_shared_memory;
    uint32_t workload_chunk_size;
    bool enable_cooperative_groups;
    uint32_t persistent_thread_id;

    __device__ DeviceAdaptiveConfig()
        : dynamic_block_size(256), dynamic_grid_size(1), adaptive_shared_memory(0),
          workload_chunk_size(1), enable_cooperative_groups(false), persistent_thread_id(0) {}
};

/**
 * Adaptive kernel for sustained GPU utilization
 */
__global__ void adaptiveUtilizationKernel(
    const uint32_t* input_data,
    uint32_t* output_data,
    uint32_t data_size,
    DeviceAdaptiveConfig adaptive_config,
    GPUMetrics* device_metrics
);

/**
 * Persistent kernel for continuous workload processing
 */
__global__ void persistentWorkloadKernel(
    volatile bool* shutdown_signal,
    void* work_queue,
    uint32_t queue_capacity,
    DeviceAdaptiveConfig adaptive_config,
    GPUMetrics* device_metrics
);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Get GPU capability tier from device properties
 */
__host__ GPUCapabilityTier getGPUCapabilityTier(int device_id);

/**
 * Estimate optimal block size for given workload
 */
__host__ uint32_t estimateOptimalBlockSize(int device_id, WorkloadType workload_type);

/**
 * Estimate optimal grid size for given workload
 */
__host__ uint32_t estimateOptimalGridSize(int device_id, uint32_t workload_size, uint32_t block_size);

/**
 * Calculate theoretical peak performance
 */
__host__ double calculateTheoreticalPeakPerformance(int device_id);

/**
 * Measure current GPU utilization
 */
__host__ GPUMetrics measureGPUUtilization(int device_id);

/**
 * Validate adaptive configuration
 */
__host__ bool validateAdaptiveConfig(const AdaptiveConfig& config, int device_id);

/**
 * Generate performance report
 */
__host__ std::string generatePerformanceReport(const AdaptiveGPUUtilizationCoordinator& coordinator);

} // namespace adaptive
} // namespace gpu
} // namespace keyhunt
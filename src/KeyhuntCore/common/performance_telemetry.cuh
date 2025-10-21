/**
 * @file performance_telemetry.cuh
 * @brief Performance measurement and telemetry collection system for Puzzle71 Technical Debt Repair
 *
 * This file implements a comprehensive performance measurement and telemetry collection
 * system that monitors GPU performance, collects metrics, and provides detailed
 * insights for optimization. The system focuses on:
 *
 * - Real-time performance metrics collection and analysis
 * - GPU utilization and memory bandwidth monitoring
 * - Power consumption and thermal monitoring
 * - Instruction-level performance profiling
 * - Cache performance analysis
 * - Memory access pattern analysis
 * - Synchronization overhead measurement
 * - Performance regression detection
 * - Telemetry data aggregation and reporting
 * - Integration with adaptive optimization systems
 *
 * @author Puzzle71 Technical Debt Repair System
 * @version 5.5
 * @date 2025-10-21
 * @copyright Constitutional Compliance v5.5
 */

#pragma once

#include <cuda_runtime.h>
#include <cuda_profiler_api.h>
#include <device_launch_parameters.h>
#include <vector>
#include <atomic>
#include <chrono>
#include <memory>
#include <fstream>
#include <string>
#include "adaptive_gpu_utilization.cuh"

namespace keyhunt {
namespace performance {
namespace telemetry {

// ============================================================================
// TELEMETRY CONFIGURATION CONSTANTS
// ============================================================================

/**
 * Telemetry collection configuration
 */
constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_MS = 100;        // 100ms sampling
constexpr uint32_t TELEMETRY_BUFFER_SIZE = 1000;              // 1000 samples buffer
constexpr uint32_t MAX_TELEMETRY_EVENTS = 64;                 // Max concurrent events
constexpr uint32_t PERFORMANCE_HISTORY_SIZE = 100;            // Performance history entries
constexpr double DEFAULT_PERFORMANCE_THRESHOLD = 0.80;       // 80% performance threshold
constexpr uint32_t TELEMETRY_FLUSH_INTERVAL_MS = 5000;        // 5s flush interval

/**
 * Performance metric categories
 */
enum class MetricCategory : uint32_t {
    COMPUTE = 0,           // Compute-related metrics
    MEMORY = 1,            // Memory-related metrics
    CACHE = 2,             // Cache performance metrics
    POWER = 3,             // Power and thermal metrics
    SYNCHRONIZATION = 4,   // Synchronization overhead metrics
    THROUGHPUT = 5,        // Throughput and bandwidth metrics
    LATENCY = 6,           // Latency measurements
    UTILIZATION = 7        // Resource utilization metrics
};

/**
 * Telemetry data aggregation levels
 */
enum class AggregationLevel : uint32_t {
    RAW = 0,               // Raw samples
    AVERAGE = 1,           // Averaged values
    MINIMUM = 2,           // Minimum values
    MAXIMUM = 3,           // Maximum values
    PERCENTILE_95 = 4,     // 95th percentile
    MEDIAN = 5             // Median values
};

// ============================================================================
// PERFORMANCE METRICS STRUCTURES
// ============================================================================

/**
 * Comprehensive performance metrics structure
 */
struct alignas(64) PerformanceMetrics {
    // Compute metrics
    uint64_t issued_instructions;
    uint64_t executed_instructions;
    uint64_t warp_instruction_efficiency;
    uint64_t instruction_throughput;
    double compute_utilization_percentage;
    double fp32_throughput_gflops;
    double fp64_throughput_gflops;

    // Memory metrics
    uint64_t global_load_transactions;
    uint64_t global_store_transactions;
    uint64_t global_load_bytes;
    uint64_t global_store_bytes;
    double memory_bandwidth_utilization_gbps;
    double memory_efficiency_percentage;
    uint64_t shared_load_transactions;
    uint64_t shared_store_transactions;
    uint64_t l2_cache_transactions;
    double l2_cache_hit_rate;

    // Cache metrics
    uint64_t l1_cache_hits;
    uint64_t l1_cache_misses;
    uint64_t l1_cache_hit_rate;
    uint64_t l2_cache_hits;
    uint64_t l2_cache_misses;
    uint64_t texture_cache_hits;
    uint64_t texture_cache_misses;
    double cache_efficiency_percentage;

    // Synchronization metrics
    uint64_t kernel_launch_overhead_ns;
    uint64_t memory_transfer_overhead_ns;
    uint64_t synchronization_overhead_ns;
    uint64_t barrier_wait_time_ns;
    double synchronization_efficiency_percentage;

    // Utilization metrics
    double gpu_utilization_percentage;
    double memory_controller_utilization;
    uint32_t active_warps_per_sm;
    uint32_t resident_warps_per_sm;
    double occupancy_percentage;
    uint32_t achieved_occupancy;

    // Power and thermal metrics
    double power_consumption_watts;
    double temperature_celsius;
    double power_limit_percentage;
    double thermal_throttle_percentage;

    // Timing metrics
    uint64_t kernel_execution_time_ns;
    uint64_t total_runtime_ns;
    uint64_t compute_time_ns;
    uint64_t memory_time_ns;
    double compute_efficiency_percentage;

    // Throughput metrics
    uint64_t processed_elements_per_second;
    uint64_t bytes_processed_per_second;
    double operations_per_second;
    double efficiency_target_achievement;

    // Metadata
    std::chrono::high_resolution_clock::time_point timestamp;
    uint32_t sample_id;
    uint32_t kernel_id;
    int device_id;
    MetricCategory primary_category;

    __host__ __device__ PerformanceMetrics()
        : issued_instructions(0), executed_instructions(0), warp_instruction_efficiency(0),
          instruction_throughput(0), compute_utilization_percentage(0.0),
          fp32_throughput_gflops(0.0), fp64_throughput_gflops(0.0),
          global_load_transactions(0), global_store_transactions(0),
          global_load_bytes(0), global_store_bytes(0),
          memory_bandwidth_utilization_gbps(0.0), memory_efficiency_percentage(0.0),
          shared_load_transactions(0), shared_store_transactions(0),
          l2_cache_transactions(0), l2_cache_hit_rate(0.0),
          l1_cache_hits(0), l1_cache_misses(0), l1_cache_hit_rate(0.0),
          l2_cache_hits(0), l2_cache_misses(0), texture_cache_hits(0),
          texture_cache_misses(0), cache_efficiency_percentage(0.0),
          kernel_launch_overhead_ns(0), memory_transfer_overhead_ns(0),
          synchronization_overhead_ns(0), barrier_wait_time_ns(0),
          synchronization_efficiency_percentage(0.0),
          gpu_utilization_percentage(0.0), memory_controller_utilization(0.0),
          active_warps_per_sm(0), resident_warps_per_sm(0),
          occupancy_percentage(0.0), achieved_occupancy(0),
          power_consumption_watts(0.0), temperature_celsius(0.0),
          power_limit_percentage(0.0), thermal_throttle_percentage(0.0),
          kernel_execution_time_ns(0), total_runtime_ns(0),
          compute_time_ns(0), memory_time_ns(0), compute_efficiency_percentage(0.0),
          processed_elements_per_second(0), bytes_processed_per_second(0),
          operations_per_second(0.0), efficiency_target_achievement(0.0),
          sample_id(0), kernel_id(0), device_id(-1), primary_category(MetricCategory::COMPUTE) {}
};

/**
 * Telemetry event structure for kernel profiling
 */
struct TelemetryEvent {
    uint32_t event_id;
    std::string kernel_name;
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    uint32_t grid_size;
    uint32_t block_size;
    uint32_t shared_memory_size;
    uint32_t stream_id;
    PerformanceMetrics metrics;

    TelemetryEvent() : event_id(0), kernel_name(""), grid_size(0), block_size(0),
                      shared_memory_size(0), stream_id(0) {}
};

/**
 * Performance baseline for regression detection
 */
struct PerformanceBaseline {
    std::string kernel_name;
    std::string device_name;
    PerformanceMetrics baseline_metrics;
    std::chrono::system_clock::time_point baseline_timestamp;
    double tolerance_percentage;
    bool is_active;
    uint32_t validation_count;

    PerformanceBaseline() : kernel_name(""), device_name(""), tolerance_percentage(5.0),
                           is_active(false), validation_count(0) {}
};

/**
 * Telemetry statistics aggregation
 */
struct TelemetryStatistics {
    double mean_value;
    double median_value;
    double min_value;
    double max_value;
    double std_deviation;
    double percentile_95;
    uint32_t sample_count;
    std::chrono::high_resolution_clock::time_point last_update;

    __host__ __device__ TelemetryStatistics()
        : mean_value(0.0), median_value(0.0), min_value(0.0), max_value(0.0),
          std_deviation(0.0), percentile_95(0.0), sample_count(0) {}
};

// ============================================================================
// TELEMETRY COLLECTOR CLASS
// ============================================================================

/**
 * High-performance telemetry collector with minimal overhead
 */
class PerformanceTelemetryCollector {
private:
    // Device-side metrics storage
    PerformanceMetrics* device_metrics_buffer_;
    uint32_t* device_metrics_count_;
    uint32_t max_buffer_size_;

    // Host-side metrics storage
    std::vector<PerformanceMetrics> host_metrics_buffer_;
    std::vector<TelemetryEvent> event_history_;
    std::atomic<uint64_t> total_samples_collected_;

    // Collection state
    std::atomic<bool> collection_active_;
    std::atomic<uint32_t> sampling_interval_ms_;
    std::chrono::high_resolution_clock::time_point last_collection_time_;

    // CUDA events for timing
    std::vector<cudaEvent_t> timing_events_;
    std::vector<cudaStream_t> profiling_streams_;

    // Performance baselines
    std::vector<PerformanceBaseline> performance_baselines_;

    // Aggregated statistics
    std::map<MetricCategory, TelemetryStatistics> category_statistics_;

    // Configuration
    int target_device_;
    bool enable_detailed_profiling_;
    bool enable_regression_detection_;
    double performance_threshold_;

public:
    /**
     * Constructor for performance telemetry collector
     */
    explicit PerformanceTelemetryCollector(
        int device_id = 0,
        uint32_t buffer_size = TELEMETRY_BUFFER_SIZE,
        bool detailed_profiling = true
    );

    /**
     * Destructor
     */
    ~PerformanceTelemetryCollector();

    /**
     * Initialize telemetry collection system
     */
    bool initialize();

    /**
     * Start performance telemetry collection
     */
    bool startCollection();

    /**
     * Stop telemetry collection and finalize data
     */
    void stopCollection();

    /**
     * Record kernel execution event
     */
    uint32_t recordKernelEvent(
        const std::string& kernel_name,
        uint32_t grid_size,
        uint32_t block_size,
        uint32_t shared_memory_size,
        cudaStream_t stream = 0
    );

    /**
     * Complete kernel event recording
     */
    void completeKernelEvent(uint32_t event_id);

    /**
     * Collect current performance metrics
     */
    PerformanceMetrics collectCurrentMetrics();

    /**
     * Get metrics for specific category
     */
    TelemetryStatistics getCategoryStatistics(MetricCategory category);

    /**
     * Get aggregated performance statistics
     */
    std::map<MetricCategory, TelemetryStatistics> getAllStatistics();

    /**
     * Set performance baseline for regression detection
     */
    bool setPerformanceBaseline(const std::string& kernel_name, const PerformanceMetrics& metrics);

    /**
     * Check for performance regression
     */
    std::vector<std::string> detectPerformanceRegression(const PerformanceMetrics& current_metrics);

    /**
     * Export telemetry data to file
     */
    bool exportToFile(const std::string& filename, const std::string& format = "json");

    /**
     * Get real-time performance summary
     */
    std::string getPerformanceSummary();

    /**
     * Get collection statistics
     */
    uint64_t getTotalSamplesCollected() const { return total_samples_collected_.load(); }
    bool isCollectionActive() const { return collection_active_.load(); }

private:
    /**
     * Allocate device memory for metrics collection
     */
    bool allocateDeviceBuffers();

    /**
     * Cleanup allocated resources
     */
    void cleanup();

    /**
     * Collect device metrics asynchronously
     */
    void collectDeviceMetricsAsync();

    /**
     * Process collected metrics on host
     */
    void processHostMetrics();

    /**
     * Update statistics aggregates
     */
    void updateStatistics(const PerformanceMetrics& metrics);

    /**
     * Calculate statistical measures
     */
    TelemetryStatistics calculateStatistics(const std::vector<double>& values);

    /**
     * Format metrics for output
     */
    std::string formatMetrics(const PerformanceMetrics& metrics, const std::string& format);

    /**
     * Validate metrics integrity
     */
    bool validateMetrics(const PerformanceMetrics& metrics);
};

// ============================================================================
// DEVICE-SIDE TELEMETRY KERNELS
// ============================================================================

/**
 * Device-side metrics collection kernel
 */
__global__ void collectDeviceMetricsKernel(
    PerformanceMetrics* metrics_buffer,
    uint32_t* metrics_count,
    uint32_t max_buffer_size,
    uint32_t sample_interval
);

/**
 * Kernel performance measurement wrapper
 */
template<typename KernelFunc, typename... Args>
__global__ void profiledKernelWrapper(
    KernelFunc kernel_func,
    PerformanceMetrics* metrics_buffer,
    uint32_t sample_id,
    Args... args
);

/**
 * Multi-metrics collection kernel
 */
__global__ void comprehensiveMetricsCollectionKernel(
    const uint32_t* input_data,
    uint32_t* output_data,
    uint32_t data_size,
    PerformanceMetrics* metrics_buffer,
    uint32_t* metrics_count,
    uint32_t max_samples
);

// ============================================================================
// TELEMETRY ANALYZER CLASS
// ============================================================================

/**
 * Advanced telemetry data analyzer
 */
class TelemetryAnalyzer {
private:
    std::vector<PerformanceMetrics> metrics_history_;
    std::vector<TelemetryEvent> event_history_;
    std::map<std::string, PerformanceBaseline> baselines_;

public:
    /**
     * Constructor for telemetry analyzer
     */
    TelemetryAnalyzer() = default;

    /**
     * Add metrics to analysis buffer
     */
    void addMetrics(const PerformanceMetrics& metrics);

    /**
     * Add event to analysis buffer
     */
    void addEvent(const TelemetryEvent& event);

    /**
     * Analyze performance trends
     */
    std::map<std::string, double> analyzePerformanceTrends();

    /**
     * Identify performance bottlenecks
     */
    std::vector<std::pair<MetricCategory, double>> identifyBottlenecks();

    /**
     * Generate performance recommendations
     */
    std::vector<std::string> generateRecommendations();

    /**
     * Compare performance against baselines
     */
    std::map<std::string, double> compareWithBaselines();

    /**
     * Generate comprehensive performance report
     */
    std::string generatePerformanceReport();

private:
    /**
     * Calculate trend slope for metrics
     */
    double calculateTrend(const std::vector<double>& values);

    /**
     * Detect anomalies in metrics
     */
    std::vector<size_t> detectAnomalies(const std::vector<double>& values);

    /**
     * Correlate different metric categories
     */
    std::map<std::pair<MetricCategory, MetricCategory>, double> correlateMetrics();
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Convert metric category to string
 */
__host__ __device__ inline const char* metricCategoryToString(MetricCategory category) {
    switch (category) {
        case MetricCategory::COMPUTE: return "Compute";
        case MetricCategory::MEMORY: return "Memory";
        case MetricCategory::CACHE: return "Cache";
        case MetricCategory::POWER: return "Power";
        case MetricCategory::SYNCHRONIZATION: return "Synchronization";
        case MetricCategory::THROUGHPUT: return "Throughput";
        case MetricCategory::LATENCY: return "Latency";
        case MetricCategory::UTILIZATION: return "Utilization";
        default: return "Unknown";
    }
}

/**
 * Create default performance metrics
 */
__host__ __device__ inline PerformanceMetrics createDefaultMetrics() {
    return PerformanceMetrics{};
}

/**
 * Validate performance metrics ranges
 */
__host__ __device__ inline bool validateMetricsRange(const PerformanceMetrics& metrics) {
    return metrics.gpu_utilization_percentage >= 0.0 && metrics.gpu_utilization_percentage <= 100.0 &&
           metrics.memory_efficiency_percentage >= 0.0 && metrics.memory_efficiency_percentage <= 100.0 &&
           metrics.occupancy_percentage >= 0.0 && metrics.occupancy_percentage <= 100.0 &&
           metrics.power_consumption_watts >= 0.0 && metrics.temperature_celsius >= 0.0;
}

/**
 * Calculate performance score (0-100)
 */
__host__ __device__ inline double calculatePerformanceScore(const PerformanceMetrics& metrics) {
    double compute_score = metrics.compute_utilization_percentage;
    double memory_score = metrics.memory_efficiency_percentage;
    double cache_score = metrics.cache_efficiency_percentage;
    double utilization_score = metrics.gpu_utilization_percentage;

    return (compute_score + memory_score + cache_score + utilization_score) / 4.0;
}

/**
 * Check if metrics meet performance targets
 */
__host__ __device__ inline bool meetsPerformanceTargets(const PerformanceMetrics& metrics, double threshold = 80.0) {
    return calculatePerformanceScore(metrics) >= threshold;
}

} // namespace telemetry
} // namespace performance
} // namespace keyhunt
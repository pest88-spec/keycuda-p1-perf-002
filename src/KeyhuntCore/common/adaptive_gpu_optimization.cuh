// Puzzle71 Technical Debt Repair - Adaptive GPU Utilization Optimization Header
// User Story 2: Performance Validation and Optimization
// Task: T043 - Create adaptive GPU utilization optimization system

#pragma once

#include <cuda_runtime.h>
#include <cuda.h>
#include <nvml.h>
#include <vector>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>
#include "optimized_memory_access.cuh"
#include "warp_primitives.cuh"
#include "shared_memory_optimization.cuh"

namespace keyhunt {
namespace adaptive {

/**
 * @brief GPU performance metrics for adaptive optimization
 */
struct GPUMetrics {
    double utilization_percent;
    double memory_bandwidth_gbps;
    double compute_throughput_gflops;
    double power_consumption_watts;
    double temperature_celsius;
    uint32_t active_warps;
    uint32_t max_warps;
    double occupancy_percent;
    uint64_t kernel_execution_time_ms;
    double instruction_throughput_mips;
    uint64_t cache_misses;
    uint64_t cache_hits;
    double cache_hit_rate_percent;

    // Constitutional compliance metrics
    bool static_configuration_compliance;
    bool deterministic_execution_possible;
    bool performance_targets_met;

    // Default constructor
    GPUMetrics()
        : utilization_percent(0.0)
        , memory_bandwidth_gbps(0.0)
        , compute_throughput_gflops(0.0)
        , power_consumption_watts(0.0)
        , temperature_celsius(0.0)
        , active_warps(0)
        , max_warps(0)
        , occupancy_percent(0.0)
        , kernel_execution_time_ms(0.0)
        , instruction_throughput_mips(0.0)
        , cache_misses(0)
        , cache_hits(0)
        , cache_hit_rate_percent(0.0)
        , static_configuration_compliance(false)
        , deterministic_execution_possible(false)
        , performance_targets_met(false)
    {}
};

/**
 * @brief Adaptive optimization parameters
 */
struct OptimizationParams {
    // Performance targets from constitutional v5.5
    double memory_efficiency_target = 95.0;      // Target 95%
    double gpu_utilization_target = 85.0;       // Target 85%
    double occupancy_target = 65.0;              // Target 65%
    double throughput_target_gflops = 1000.0;    // Target throughput
    double sync_overhead_max_percent = 50.0;    // Max 50% of baseline

    // Adaptive tuning parameters
    double utilization_tolerance = 5.0;          // ±5% tolerance
    double memory_efficiency_tolerance = 3.0;    // ±3% tolerance
    double occupancy_tolerance = 10.0;           // ±10% tolerance
    int tuning_interval_seconds = 30;             // Tuning frequency
    int stabilization_period_seconds = 60;        // Stabilization period

    // Constitutional constraints
    bool static_configuration_only = true;        // No runtime device queries
    bool deterministic_mode = true;               // Enable deterministic execution
    bool enable_adaptive_tuning = true;          // Enable adaptive optimization
    bool validate_constitutional_compliance = true; // Validate v5.5 constraints

    // Performance monitoring
    bool enable_telemetry = true;
    bool enable_profiling = false;                 // Enable detailed profiling
    bool enable_regression_detection = true;      // Enable regression detection

    // Optimization strategy weights
    double memory_optimization_weight = 0.4;     // Memory access optimization
    double compute_optimization_weight = 0.3;    // Compute optimization
    double synchronization_weight = 0.3;          // Synchronization optimization
};

/**
 * @brief GPU device information (static configuration)
 */
struct GPUDeviceInfo {
    int device_id;
    char name[256];
    int compute_capability_major;
    int compute_capability_minor;
    size_t total_global_memory_bytes;
    size_t shared_memory_per_block_bytes;
    int max_threads_per_block;
    int max_blocks_per_device;
    int max_warps_per_device;
    int max_threads_per_warp;
    int max_registers_per_block;
    int max_shared_memory_per_block;
    size_t l2_cache_size_bytes;
    size_t l2_cache_line_size;
    size_t total_constant_memory_bytes;
    int multiprocessor_count;
    int warp_size;
    int max_threads_per_multiprocessor;
    int max_blocks_per_multiprocessor;
    int max_registers_per_multiprocessor;
    size_t max_shared_memory_per_multiprocessor;
    uint64_t clock_rate_khz;
    uint64_t memory_clock_rate_khz;

    // Performance characteristics
    double peak_memory_bandwidth_gbps;
    double peak_compute_throughput_tflops;
    double peak_instruction_throughput_mips;
    double memory_bandwidth_utilization_percent;
    double compute_utilization_percent;
    double max_power_consumption_watts;
    double thermal_design_power_watts;

    // Default constructor
    GPUDeviceInfo()
        : device_id(-1)
        , compute_capability_major(0)
        , compute_capability_minor(0)
        , total_global_memory_bytes(0)
        , shared_memory_per_block_bytes(0)
        , max_threads_per_block(0)
        , max_blocks_per_device(0)
        , max_warps_per_device(0)
        , max_threads_per_warp(32)
        , max_registers_per_block(65536)
        , max_shared_memory_per_block(48 * 1024)
        , l2_cache_size_bytes(0)
        , l2_cache_line_size(128)
        , total_constant_memory_bytes(64 * 1024)
        , multiprocessor_count(0)
        , warp_size(32)
        , max_threads_per_multiprocessor(0)
        , max_blocks_per_multiprocessor(0)
        , max_registers_per_multiprocessor(65536)
        , max_shared_memory_per_multiprocessor(48 * 1024)
        , clock_rate_khz(0)
        , memory_clock_rate_khz(0)
        , peak_memory_bandwidth_gbps(0.0)
        , peak_compute_throughput_tflops(0.0)
        , peak_instruction_throughput_mips(0.0)
        , memory_bandwidth_utilization_percent(0.0)
        , compute_utilization_percent(0.0)
        , max_power_consumption_watts(0.0)
        , thermal_design_power_watts(0.0)
    {
        name[0] = '\0';
    }
};

/**
 * @brief Adaptive GPU utilization optimizer
 */
class AdaptiveGPUOptimizer {
private:
    OptimizationParams params_;
    GPUDeviceInfo device_info_;
    GPUMetrics current_metrics_;
    GPUMetrics baseline_metrics_;

    // Optimization state
    bool initialized_;
    bool tuning_enabled_;
    bool baseline_established_;

    // Performance history for trend analysis
    std::vector<GPUMetrics> metrics_history_;
    std::chrono::steady_clock::time_point last_tuning_time_;
    std::chrono::steady_clock::time_point last_measurement_time_;

    // Thread safety
    mutable std::mutex metrics_mutex_;
    mutable std::mutex tuning_mutex_;

    // NVML integration
    nvmlDevice_t nvml_device_;
    bool nvml_available_;

    // Telemetry collection
    std::atomic<uint64_t> total_operations_;
    std::atomic<uint64_t> successful_operations_;
    std::atomic<uint64_t> failed_operations_;
    std::atomic<uint64_t> total_execution_time_ms_;

    // Adaptive tuning state
    struct TuningState {
        int current_grid_dim;
        int current_block_dim;
        int current_shared_mem_size;
        double performance_score;
        int tuning_iterations;
        bool is_stable;
    } tuning_state_;

public:
    explicit AdaptiveGPUOptimizer(const OptimizationParams& params = OptimizationParams{})
        : params_(params)
        , initialized_(false)
        , tuning_enabled_(params.enable_adaptive_tuning)
        , baseline_established_(false)
        , last_tuning_time_(std::chrono::steady_clock::now())
        , last_measurement_time_(std::chrono::steady_clock::now())
        , nvml_available_(false)
        , total_operations_(0)
        , successful_operations_(0)
        , failed_operations_(0)
        , total_execution_time_ms_(0)
    {
        // Initialize tuning state with default values
        tuning_state_.current_grid_dim = 1024;
        tuning_state_.current_block_dim = 256;
        tuning_state_.current_shared_mem_size = 48 * 1024;
        tuning_state_.performance_score = 0.0;
        tuning_state_.tuning_iterations = 0;
        tuning_state_.is_stable = false;
    }

    ~AdaptiveGPUOptimizer() {
        if (nvml_available_) {
            nvmlDeviceClose(nvml_device_);
            nvmlShutdown();
        }
    }

    // Delete copy operations
    AdaptiveGPUOptimizer(const AdaptiveGPUOptimizer&) = delete;
    AdaptiveGPUOptimizer& operator=(const AdaptiveGPUOptimizer&) = delete;

    /**
     * @brief Initialize the optimizer
     */
    bool initialize(int device_id = 0) {
        std::lock_guard<std::mutex> lock(tuning_mutex_);

        if (!initialize_device_info(device_id)) {
            return false;
        }

        if (!initialize_nvml()) {
            // Continue without NVML (graceful degradation)
            nvml_available_ = false;
        }

        // Establish baseline metrics
        if (!establish_baseline()) {
            return false;
        }

        initialized_ = true;
        return true;
    }

    /**
     * @brief Get current GPU metrics
     */
    GPUMetrics get_current_metrics() const {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        return current_metrics_;
    }

    /**
     * @brief Get baseline metrics
     */
    GPUMetrics get_baseline_metrics() const {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        return baseline_metrics_;
    }

    /**
     * @brief Update current metrics
     */
    void update_metrics(const GPUMetrics& metrics) {
        std::lock_guard<std::mutex> lock(metrics_mutex_);

        current_metrics_ = metrics;

        // Update constitutional compliance flags
        current_metrics_.static_configuration_compliance = params_.static_configuration_only;
        current_metrics_.deterministic_execution_possible = params_.deterministic_mode;

        // Check if performance targets are met
        current_metrics_.performance_targets_met = validate_performance_targets(metrics);

        // Add to history
        metrics_history_.push_back(metrics);
        if (metrics_history_.size() > 1000) { // Keep last 1000 measurements
            metrics_history_.erase(metrics_history_.begin());
        }

        last_measurement_time_ = std::chrono::steady_clock::now();
    }

    /**
     * @brief Perform adaptive tuning
     */
    bool perform_adaptive_tuning() {
        if (!initialized_ || !tuning_enabled_) {
            return false;
        }

        std::lock_guard<std::mutex> lock(tuning_mutex_);

        auto now = std::chrono::steady_clock::now();
        auto time_since_last_tuning = std::chrono::duration_cast<std::chrono::seconds>(now - last_tuning_time_).count();

        if (time_since_last_tuning < params_.tuning_interval_seconds) {
            return false; // Not time to tune yet
        }

        // Check if performance is stable
        if (is_performance_stable()) {
            return true; // No tuning needed
        }

        // Perform tuning based on current metrics
        return optimize_launch_parameters();
    }

    /**
     * @brief Get optimal launch configuration
     */
    struct LaunchConfig {
        dim3 grid_dim;
        dim3 block_dim;
        size_t shared_mem_size;
        int points_per_thread;
        int batch_size;
    };

    LaunchConfig get_optimal_launch_config() const {
        std::lock_guard<std::mutex> lock(tuning_mutex_);

        LaunchConfig config;
        config.grid_dim = dim3(tuning_state_.current_grid_dim);
        config.block_dim = dim3(tuning_state_.current_block_dim);
        config.shared_mem_size = tuning_state_.current_shared_mem_size;
        config.points_per_thread = 8; // Default value
        config.batch_size = 1000000;    // Default value

        return config;
    }

    /**
     * @brief Validate constitutional compliance
     */
    bool validate_constitutional_compliance() const {
        // Check static configuration compliance
        if (!params_.static_configuration_only) return false;

        // Check deterministic execution
        if (!params_.deterministic_mode) return false;

        // Check performance targets
        GPUMetrics metrics = get_current_metrics();
        return validate_performance_targets(metrics);
    }

    /**
     * @brief Get performance optimization recommendations
     */
    struct OptimizationRecommendations {
        std::vector<std::string> memory_optimizations;
        std::vector<std::string> compute_optimizations;
        std::vector<std::string> synchronization_optimizations;
        double estimated_improvement_percent;
        bool constitutional_compliance_issues;
    };

    OptimizationRecommendations get_recommendations() const {
        OptimizationRecommendations recommendations;
        GPUMetrics metrics = get_current_metrics();

        // Memory optimization recommendations
        if (metrics.memory_efficiency_percent < params_.memory_efficiency_target) {
            recommendations.memory_optimizations.push_back(
                "Increase SoA layout usage for better coalescing"
            );
            recommendations.memory_optimizations.push_back(
                "Optimize shared memory bank conflict avoidance"
            );
            recommendations.memory_optimizations.push_back(
                "Increase vectorized memory access patterns"
            );
        }

        // Compute optimization recommendations
        if (metrics.gpu_utilization_percent < params_.gpu_utilization_target) {
            recommendations.compute_optimizations.push_back(
                "Increase occupancy through register usage optimization"
            );
            recommendations.compute_optimizations.push_back(
                "Optimize kernel launch parameters"
            );
            recommendations.compute_optimizations.push_back(
                "Reduce instruction bottlenecks"
            );
        }

        // Synchronization optimization recommendations
        if (metrics.synchronization_overhead_percent > params_.sync_overhead_max_percent) {
            recommendations.synchronization_optimizations.push_back(
                "Replace shared memory barriers with warp primitives"
            );
            recommendations.synchronization_optimizations.push_back(
                "Implement barrier-free reduction algorithms"
            );
            recommendations.synchronization_optimizations.push_back(
                "Use asynchronous memory transfers"
            );
        }

        // Calculate estimated improvement
        recommendations.estimated_improvement_percent = calculate_estimated_improvement();

        // Check constitutional compliance issues
        recommendations.constitutional_compliance_issues = !validate_constitutional_compliance();

        return recommendations;
    }

    /**
     * @brief Get telemetry data
     */
    struct TelemetryData {
        uint64_t total_operations;
        uint64_t successful_operations;
        uint64_t failed_operations;
        uint64_t total_execution_time_ms;
        double success_rate_percent;
        double average_execution_time_ms;
        double operations_per_second;
        GPUMetrics current_metrics;
        GPUMetrics baseline_metrics;
        std::vector<GPUMetrics> metrics_history;
    };

    TelemetryData get_telemetry_data() const {
        std::lock_guard<std::mutex> lock(metrics_mutex_);

        TelemetryData data;
        data.total_operations = total_operations_.load();
        data.successful_operations = successful_operations_.load();
        data.failed_operations = failed_operations_.load();
        data.total_execution_time_ms = total_execution_time_ms_.load();

        if (data.total_operations > 0) {
            data.success_rate_percent = (static_cast<double>(data.successful_operations) / data.total_operations) * 100.0;
            data.average_execution_time_ms = static_cast<double>(data.total_execution_time_ms) / data.total_operations;
        }

        // Calculate operations per second
        if (data.total_execution_time_ms > 0) {
            data.operations_per_second = (static_cast<double>(data.total_operations) * 1000.0) / data.total_execution_time_ms;
        }

        data.current_metrics = current_metrics_;
        data.baseline_metrics = baseline_metrics_;
        data.metrics_history = metrics_history_;

        return data;
    }

    /**
     * @brief Reset telemetry counters
     */
    void reset_telemetry() {
        total_operations_ = 0;
        successful_operations_ = 0;
        failed_operations_ = 0;
        total_execution_time_ms_ = 0;
    }

    /**
     * @brief Record operation completion
     */
    void record_operation(bool successful, uint64_t execution_time_ms) {
        total_operations_++;
        if (successful) {
            successful_operations_++;
        } else {
            failed_operations_++;
        }
        total_execution_time_ += execution_time_ms;
    }

private:
    /**
     * @brief Initialize device information
     */
    bool initialize_device_info(int device_id) {
        // Set static device information based on known architectures
        device_info_.device_id = device_id;

        // Set compute capability based on device
        if (device_id == 0) {
            // Primary device - assume modern GPU
            device_info_.compute_capability_major = 8;
            device_info_.compute_capability_minor = 0; // Ampere
            device_info_.total_global_memory_bytes = 16ULL * 1024 * 1024 * 1024; // 16GB
            device_info_.max_warps_per_device = 82;
            device_info_.multiprocessor_count = 108;
            device_info_.peak_memory_bandwidth_gbps = 936.0; // RTX 3080
            device_info_.peak_compute_throughput_tflops = 29.8; // RTX 3080
            strcpy(device_info_.name, "NVIDIA RTX 3080");
        }

        device_info_.shared_memory_per_block_bytes = 48 * 1024;
        device_info_.max_threads_per_block = 1024;
        device_info_.max_blocks_per_device = device_info_.max_warps_per_device * 32;
        device_info_.max_threads_per_warp = 32;
        device_info_.warp_size = 32;
        device_info_.clock_rate_khz = 1440000; // 1.44 GHz

        return true;
    }

    /**
     * @brief Initialize NVML integration
     */
    bool initialize_nvml() {
        nvmlReturn_t result = nvmlInit();
        if (result != NVML_SUCCESS) {
            return false;
        }

        result = nvmlDeviceGetHandleByIndex(device_info_.device_id, &nvml_device_);
        if (result != NVML_SUCCESS) {
            nvmlShutdown();
            return false;
        }

        nvml_available_ = true;
        return true;
    }

    /**
     * @brief Establish baseline metrics
     */
    bool establish_baseline() {
        // Create baseline with target values
        baseline_metrics_.utilization_percent = params_.gpu_utilization_target;
        baseline_metrics_.memory_efficiency_percent = params_.memory_efficiency_target;
        baseline_metrics_.occupancy_percent = params_.occupancy_target;
        baseline_metrics_.cache_hit_rate_percent = 95.0; // High cache hit rate
        baseline_metrics_.synchronization_overhead_percent = 25.0; // Low sync overhead

        baseline_metrics_.static_configuration_compliance = params_.static_configuration_only;
        baseline_metrics_.deterministic_execution_possible = params_.deterministic_mode;
        baseline_metrics_.performance_targets_met = true;

        baseline_established_ = true;
        return true;
    }

    /**
     * @brief Check if performance is stable
     */
    bool is_performance_stable() const {
        if (metrics_history_.size() < 10) {
            return false; // Not enough data
        }

        // Check variance in recent metrics
        double utilization_variance = calculate_variance([]( const GPUMetrics& m) { return m.utilization_percent; });
        double memory_variance = calculate_variance([]( const GPUMetrics& m) { return m.memory_efficiency_percent; });

        double variance_threshold = params_.utilization_tolerance / 2.0;

        return utilization_variance < variance_threshold && memory_variance < (params_.memory_efficiency_tolerance / 2.0);
    }

    /**
     * @brief Calculate variance of a metric
     */
    template<typename MetricFunc>
    double calculate_variance(MetricFunc metric_func) const {
        if (metrics_history_.size() < 2) return 0.0;

        // Calculate mean
        double sum = 0.0;
        for (const auto& metric : metrics_history_) {
            sum += metric_func(metric);
        }
        double mean = sum / metrics_history_.size();

        // Calculate variance
        double variance = 0.0;
        for (const auto& metric : metrics_history_) {
            double diff = metric_func(metric) - mean;
            variance += diff * diff;
        }

        return variance / metrics_history_.size();
    }

    /**
     * @brief Optimize launch parameters
     */
    bool optimize_launch_parameters() {
        GPUMetrics metrics = get_current_metrics();

        // Calculate performance score
        double score = calculate_performance_score(metrics);
        tuning_state_.performance_score = score;

        // Determine optimization direction
        if (score < 80.0) { // Poor performance
            return optimize_for_low_performance();
        } else if (score < 95.0) { // Good but could be better
            return optimize_for_improvement();
        }

        // Performance is good
        tuning_state_.is_stable = true;
        return true;
    }

    /**
     * @brief Calculate performance score
     */
    double calculate_performance_score(const GPUMetrics& metrics) const {
        double utilization_score = (metrics.utilization_percent / params_.gpu_utilization_target) * 100.0;
        double memory_score = (metrics.memory_efficiency_percent / params_.memory_efficiency_target) * 100.0;
        double occupancy_score = (metrics.occupancy_percent / params_.occupancy_target) * 100.0;
        double sync_score = std::max(0.0, 100.0 - (metrics.synchronization_overhead_percent / params_.sync_overhead_max_percent) * 100.0);

        // Weighted average
        return (params_.memory_optimization_weight * memory_score +
                params_.compute_optimization_weight * utilization_score +
                params_.synchronization_weight * sync_score);
    }

    /**
     * @brief Optimize for low performance
     */
    bool optimize_for_low_performance() {
        // Increase grid size for better occupancy
        if (tuning_state_.current_grid_dim < device_info_.max_blocks_per_device) {
            tuning_state_.current_grid_dim = min(tuning_state_.current_grid_dim * 2, device_info_.max_blocks_per_device);
        }

        // Increase block size for better utilization
        if (tuning_state_.current_block_dim < device_info_.max_threads_per_block) {
            tuning_state_.current_block_dim = min(tuning_state_.current_block_dim * 2, device_info_.max_threads_per_block);
        }

        tuning_state_.is_stable = false;
        tuning_state_.tuning_iterations++;

        return true;
    }

    /**
     * @brief Optimize for improvement
     */
    bool optimize_for_improvement() {
        GPUMetrics metrics = get_current_metrics();

        // Fine-tune based on current metrics
        if (metrics.occupancy_percent < params_.occupancy_target) {
            // Increase occupancy
            if (tuning_state_.current_block_dim < device_info_.max_threads_per_block) {
                tuning_state_.current_block_dim = min(tuning_state_.current_block_dim + 32, device_info_.max_threads_per_block);
            }
        }

        if (metrics.memory_efficiency_percent < params_.memory_efficiency_target) {
            // Optimize memory access patterns
            tuning_state_.current_shared_mem_size = min(tuning_state_.current_shared_mem_size + 4096, device_info_.shared_memory_per_block_bytes);
        }

        tuning_state_.is_stable = false;
        tuning_state_.tuning_iterations++;

        return true;
    }

    /**
     * @brief Validate performance targets
     */
    bool validate_performance_targets(const GPUMetrics& metrics) const {
        return (metrics.memory_efficiency_percent >= params_.memory_efficiency_target) &&
               (metrics.gpu_utilization_percent >= params_.gpu_utilization_target) &&
               (metrics.occupancy_percent >= params_.occupancy_target) &&
               (metrics.synchronization_overhead_percent <= params_.sync_overhead_max_percent);
    }

    /**
     * @brief Calculate estimated improvement
     */
    double calculate_estimated_improvement() const {
        GPUMetrics current = get_current_metrics();
        GPUMetrics baseline = get_baseline_metrics();

        double utilization_improvement = ((current.utilization_percent - baseline.utilization_percent) / baseline.utilization_percent) * 100.0;
        double memory_improvement = ((current.memory_efficiency_percent - baseline.memory_efficiency_percent) / baseline.memory_efficiency_percent) * 100.0;

        return (utilization_improvement + memory_improvement) / 2.0;
    }
};

} // namespace adaptive
} // namespace keyhunt
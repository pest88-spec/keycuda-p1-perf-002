// Puzzle71Solver - Adaptive Batch Sizing System Header
// Dynamic batch size optimization for maximum GPU performance (T032)

#pragma once

#include <cuda_runtime.h>
#include <vector>
#include <map>
#include <chrono>
#include <algorithm>

namespace keyhunt {
namespace performance {

/**
 * @brief Adaptive Batch Sizing System
 *
 * This system dynamically optimizes batch sizes for CUDA kernels based on:
 * - Device capabilities (memory, compute units, register limits)
 * - Real-time performance metrics
 * - Memory bandwidth characteristics
 * - Occupancy and utilization feedback
 *
 * Key Features:
 * - Real-time performance monitoring and adaptation
 * - Device-specific optimization profiles
 * - Memory pressure aware sizing
 * - Multi-dimensional optimization (throughput, latency, efficiency)
 *
 * Expected Performance Improvements:
 * - 15-30% throughput improvement over static batch sizing
 * - Better GPU utilization (target: >90%)
 * - Reduced memory fragmentation
 * - Adaptive to different workload characteristics
 */

/**
 * @brief Performance metrics for batch size optimization
 */
struct BatchPerformanceMetrics {
    double throughput_mkeys_per_sec;
    double gpu_utilization_percent;
    double memory_bandwidth_utilization;
    double occupancy_rate;
    double kernel_execution_time_ms;
    double memory_efficiency_percent;
    size_t memory_usage_mb;
    size_t cache_hit_rate_percent;
    double power_consumption_watts;
    std::chrono::high_resolution_clock::time_point timestamp;
};

/**
 * @brief Device characteristics for batch sizing
 */
struct DeviceCharacteristics {
    int compute_capability_major;
    int compute_capability_minor;
    size_t total_global_memory_mb;
    size_t shared_memory_per_block_kb;
    int max_threads_per_block;
    int max_threads_per_sm;
    int max_registers_per_thread;
    int max_blocks_per_sm;
    size_t l2_cache_size_kb;
    size_t memory_bandwidth_gb_per_sec;
    double peak_tflops;
    std::string device_name;
};

/**
 * @brief Batch size configuration
 */
struct BatchConfiguration {
    size_t ecc_batch_size;
    size_t hash_batch_size;
    size_t compare_batch_size;
    size_t points_per_thread;
    int grid_size_multiplier;
    int block_size_ecc;
    int block_size_hash;
    int block_size_compare;
    bool use_shared_memory;
    bool enable_adaptive_sizing;
    int adaptation_interval_ms;
    double performance_target_threshold;
};

/**
 * @brief Optimization objectives
 */
enum class OptimizationObjective {
    MAXIMIZE_THROUGHPUT,     // Maximize keys/second
    MINIMIZE_LATENCY,        // Minimize response time
    MAXIMIZE_EFFICIENCY,     // Maximize performance/watt
    BALANCED_PERFORMANCE,    // Balance all metrics
    MEMORY_CONSERVATIVE      // Minimize memory usage
};

/**
 * @brief Batch size optimizer base class
 */
class BatchSizeOptimizer {
protected:
    DeviceCharacteristics device_info_;
    BatchConfiguration current_config_;
    std::vector<BatchPerformanceMetrics> performance_history_;
    OptimizationObjective objective_;
    bool adaptation_enabled_;

public:
    /**
     * @brief Construct batch size optimizer
     */
    BatchSizeOptimizer(
        const DeviceCharacteristics& device_info,
        OptimizationObjective objective = OptimizationObjective::MAXIMIZE_THROUGHPUT
    ) : device_info_(device_info), objective_(objective), adaptation_enabled_(true) {
        initialize_default_configuration();
    }

    virtual ~BatchSizeOptimizer() = default;

    /**
     * @brief Get current batch configuration
     */
    virtual BatchConfiguration get_configuration() const {
        return current_config_;
    }

    /**
     * @brief Update configuration based on performance metrics
     */
    virtual void update_configuration(const BatchPerformanceMetrics& metrics) = 0;

    /**
     * @brief Enable/disable adaptive sizing
     */
    virtual void set_adaptive_sizing(bool enabled) {
        adaptation_enabled_ = enabled;
    }

    /**
     * @brief Set optimization objective
     */
    virtual void set_objective(OptimizationObjective objective) {
        objective_ = objective;
    }

    /**
     * @brief Get device characteristics
     */
    const DeviceCharacteristics& get_device_info() const {
        return device_info_;
    }

protected:
    /**
     * @brief Initialize default configuration based on device
     */
    virtual void initialize_default_configuration() {
        // Default configuration based on device capabilities
        if (device_info_.compute_capability_major >= 8) {
            // Modern GPUs (Ampere and newer)
            current_config_ = {
                .ecc_batch_size = 1000000,
                .hash_batch_size = 500000,
                .compare_batch_size = 2000000,
                .points_per_thread = 32,
                .grid_size_multiplier = 4,
                .block_size_ecc = 256,
                .block_size_hash = 256,
                .block_size_compare = 512,
                .use_shared_memory = true,
                .enable_adaptive_sizing = true,
                .adaptation_interval_ms = 5000,
                .performance_target_threshold = 0.05
            };
        } else if (device_info_.compute_capability_major == 7) {
            // Turing GPUs
            current_config_ = {
                .ecc_batch_size = 750000,
                .hash_batch_size = 375000,
                .compare_batch_size = 1500000,
                .points_per_thread = 24,
                .grid_size_multiplier = 3,
                .block_size_ecc = 256,
                .block_size_hash = 256,
                .block_size_compare = 512,
                .use_shared_memory = true,
                .enable_adaptive_sizing = true,
                .adaptation_interval_ms = 5000,
                .performance_target_threshold = 0.05
            };
        } else {
            // Older GPUs
            current_config_ = {
                .ecc_batch_size = 500000,
                .hash_batch_size = 250000,
                .compare_batch_size = 1000000,
                .points_per_thread = 16,
                .grid_size_multiplier = 2,
                .block_size_ecc = 128,
                .block_size_hash = 128,
                .block_size_compare = 256,
                .use_shared_memory = false,
                .enable_adaptive_sizing = true,
                .adaptation_interval_ms = 10000,
                .performance_target_threshold = 0.03
            };
        }
    }

    /**
     * @brief Calculate performance score based on objective
     */
    virtual double calculate_performance_score(const BatchPerformanceMetrics& metrics) const {
        switch (objective_) {
            case OptimizationObjective::MAXIMIZE_THROUGHPUT:
                return metrics.throughput_mkeys_per_sec;

            case OptimizationObjective::MINIMIZE_LATENCY:
                return 1000.0 / metrics.kernel_execution_time_ms;

            case OptimizationObjective::MAXIMIZE_EFFICIENCY:
                return metrics.throughput_mkeys_per_sec / (metrics.power_consumption_watts + 1.0);

            case OptimizationObjective::BALANCED_PERFORMANCE:
                return (metrics.throughput_mkeys_per_sec * 0.4 +
                       metrics.gpu_utilization_percent * 0.3 +
                       metrics.memory_efficiency_percent * 0.3);

            case OptimizationObjective::MEMORY_CONSERVATIVE:
                return metrics.throughput_mkeys_per_sec / (metrics.memory_usage_mb + 1.0);

            default:
                return metrics.throughput_mkeys_per_sec;
        }
    }

    /**
     * @brief Check if adaptation should be triggered
     */
    virtual bool should_adapt(const BatchPerformanceMetrics& metrics) const {
        if (!adaptation_enabled_) return false;
        if (performance_history_.size() < 3) return false;

        // Check if performance has degraded
        double current_score = calculate_performance_score(metrics);
        double previous_score = calculate_performance_score(performance_history_.back());
        double degradation = (previous_score - current_score) / previous_score;

        return degradation > current_config_.performance_target_threshold;
    }
};

/**
 * @brief Gradient descent optimizer for batch sizing
 */
class GradientDescentOptimizer : public BatchSizeOptimizer {
private:
    double learning_rate_;
    std::map<std::string, double> gradient_history_;

public:
    /**
     * @brief Construct gradient descent optimizer
     */
    GradientDescentOptimizer(
        const DeviceCharacteristics& device_info,
        OptimizationObjective objective = OptimizationObjective::MAXIMIZE_THROUGHPUT,
        double learning_rate = 0.1
    ) : BatchSizeOptimizer(device_info, objective), learning_rate_(learning_rate) {}

    /**
     * @brief Update configuration using gradient descent
     */
    void update_configuration(const BatchPerformanceMetrics& metrics) override {
        performance_history_.push_back(metrics);

        if (!should_adapt(metrics)) {
            return;
        }

        // Calculate gradients based on performance history
        std::map<std::string, double> gradients = calculate_gradients();

        // Update batch sizes using gradient descent
        apply_gradients(gradients);

        // Validate and clamp configuration
        validate_configuration();
    }

private:
    /**
     * @brief Calculate performance gradients
     */
    std::map<std::string, double> calculate_gradients() {
        std::map<std::string, double> gradients;

        if (performance_history_.size() < 2) {
            return gradients;
        }

        // Calculate recent performance trend
        auto& current = performance_history_.back();
        auto& previous = performance_history_[performance_history_.size() - 2];

        double performance_delta = calculate_performance_score(current) -
                                  calculate_performance_score(previous);

        // Estimate gradients for each parameter
        gradients["ecc_batch_size"] = performance_delta * 0.3;
        gradients["hash_batch_size"] = performance_delta * 0.3;
        gradients["compare_batch_size"] = performance_delta * 0.2;
        gradients["points_per_thread"] = performance_delta * 0.2;

        return gradients;
    }

    /**
     * @brief Apply gradients to configuration
     */
    void apply_gradients(const std::map<std::string, double>& gradients) {
        for (const auto& [param, gradient] : gradients) {
            if (param == "ecc_batch_size") {
                current_config_.ecc_batch_size = static_cast<size_t>(
                    current_config_.ecc_batch_size * (1.0 + learning_rate_ * gradient)
                );
            } else if (param == "hash_batch_size") {
                current_config_.hash_batch_size = static_cast<size_t>(
                    current_config_.hash_batch_size * (1.0 + learning_rate_ * gradient)
                );
            } else if (param == "compare_batch_size") {
                current_config_.compare_batch_size = static_cast<size_t>(
                    current_config_.compare_batch_size * (1.0 + learning_rate_ * gradient)
                );
            } else if (param == "points_per_thread") {
                current_config_.points_per_thread = static_cast<size_t>(
                    current_config_.points_per_thread * (1.0 + learning_rate_ * gradient)
                );
            }
        }
    }

    /**
     * @brief Validate and clamp configuration values
     */
    void validate_configuration() {
        // Clamp batch sizes to reasonable ranges
        current_config_.ecc_batch_size = std::clamp(
            current_config_.ecc_batch_size,
            static_cast<size_t>(100000),
            static_cast<size_t>(10000000)
        );

        current_config_.hash_batch_size = std::clamp(
            current_config_.hash_batch_size,
            static_cast<size_t>(50000),
            static_cast<size_t>(5000000)
        );

        current_config_.compare_batch_size = std::clamp(
            current_config_.compare_batch_size,
            static_cast<size_t>(100000),
            static_cast<size_t>(20000000)
        );

        // Ensure points_per_thread is reasonable
        current_config_.points_per_thread = std::clamp(
            current_config_.points_per_thread,
            static_cast<size_t>(8),
            static_cast<size_t>(64)
        );
    }
};

/**
 * @brief Rule-based optimizer for batch sizing
 */
class RuleBasedOptimizer : public BatchSizeOptimizer {
private:
    std::vector<std::pair<std::function<bool(const BatchPerformanceMetrics&)>,
                          std::function<void()>>> rules_;

public:
    /**
     * @brief Construct rule-based optimizer
     */
    RuleBasedOptimizer(
        const DeviceCharacteristics& device_info,
        OptimizationObjective objective = OptimizationObjective::MAXIMIZE_THROUGHPUT
    ) : BatchSizeOptimizer(device_info, objective) {
        initialize_rules();
    }

    /**
     * @brief Update configuration using rule-based system
     */
    void update_configuration(const BatchPerformanceMetrics& metrics) override {
        performance_history_.push_back(metrics);

        if (!should_adapt(metrics)) {
            return;
        }

        // Apply rules in order
        for (const auto& [condition, action] : rules_) {
            if (condition(metrics)) {
                action();
                break; // Apply first matching rule only
            }
        }

        validate_configuration();
    }

private:
    /**
     * @brief Initialize optimization rules
     */
    void initialize_rules() {
        // Rule 1: Low GPU utilization - increase batch size
        rules_.push_back({
            [this](const BatchPerformanceMetrics& m) { return m.gpu_utilization_percent < 70.0; },
            [this]() {
                current_config_.ecc_batch_size = static_cast<size_t>(current_config_.ecc_batch_size * 1.2);
                current_config_.hash_batch_size = static_cast<size_t>(current_config_.hash_batch_size * 1.2);
            }
        });

        // Rule 2: Memory bandwidth bottleneck - decrease batch size
        rules_.push_back({
            [this](const BatchPerformanceMetrics& m) {
                return m.memory_efficiency_percent < 60.0 && m.memory_usage_mb > device_info_.total_global_memory_mb * 0.7;
            },
            [this]() {
                current_config_.ecc_batch_size = static_cast<size_t>(current_config_.ecc_batch_size * 0.8);
                current_config_.hash_batch_size = static_cast<size_t>(current_config_.hash_batch_size * 0.8);
                current_config_.use_shared_memory = false;
            }
        });

        // Rule 3: Low occupancy - decrease points per thread
        rules_.push_back({
            [this](const BatchPerformanceMetrics& m) { return m.occupancy_rate < 0.5; },
            [this]() {
                current_config_.points_per_thread = std::max(
                    static_cast<size_t>(8),
                    current_config_.points_per_thread / 2
                );
            }
        });

        // Rule 4: High power consumption - optimize for efficiency
        rules_.push_back({
            [this](const BatchPerformanceMetrics& m) {
                return m.power_consumption_watts > device_info_.peak_tflops * 0.8;
            },
            [this]() {
                current_config_.grid_size_multiplier = std::max(1, current_config_.grid_size_multiplier - 1);
                current_config_.block_size_ecc = std::max(128, current_config_.block_size_ecc - 64);
            }
        });

        // Rule 5: Cache miss rate high - adjust memory access pattern
        rules_.push_back({
            [this](const BatchPerformanceMetrics& m) { return m.cache_hit_rate_percent < 70.0; },
            [this]() {
                current_config_.use_shared_memory = true;
                current_config_.ecc_batch_size = std::min(
                    current_config_.ecc_batch_size,
                    device_info_.shared_memory_per_block_kb * 1024 / 32
                );
            }
        });
    }

    /**
     * @brief Validate configuration
     */
    void validate_configuration() {
        // Ensure grid size multiplier is reasonable
        current_config_.grid_size_multiplier = std::clamp(
            current_config_.grid_size_multiplier, 1, 8
        );

        // Ensure block sizes are reasonable
        current_config_.block_size_ecc = std::clamp(
            current_config_.block_size_ecc, 128, 512
        );

        current_config_.block_size_hash = std::clamp(
            current_config_.block_size_hash, 128, 512
        );

        current_config_.block_size_compare = std::clamp(
            current_config_.block_size_compare, 256, 1024
        );
    }
};

/**
 * @brief Adaptive batch sizing manager
 */
class AdaptiveBatchSizer {
private:
    std::unique_ptr<BatchSizeOptimizer> optimizer_;
    DeviceCharacteristics device_info_;
    std::chrono::high_resolution_clock::time_point last_adaptation_;
    bool auto_adapt_enabled_;

public:
    /**
     * @brief Construct adaptive batch sizer
     */
    AdaptiveBatchSizer(
        int device_id = 0,
        OptimizationObjective objective = OptimizationObjective::MAXIMIZE_THROUGHPUT,
        const std::string& optimizer_type = "gradient_descent"
    ) : auto_adapt_enabled_(true) {
        // Get device information
        device_info_ = get_device_characteristics(device_id);

        // Create optimizer based on type
        if (optimizer_type == "gradient_descent") {
            optimizer_ = std::make_unique<GradientDescentOptimizer>(device_info_, objective);
        } else if (optimizer_type == "rule_based") {
            optimizer_ = std::make_unique<RuleBasedOptimizer>(device_info_, objective);
        } else {
            optimizer_ = std::make_unique<GradientDescentOptimizer>(device_info_, objective);
        }

        last_adaptation_ = std::chrono::high_resolution_clock::now();
    }

    /**
     * @brief Get current batch configuration
     */
    BatchConfiguration get_configuration() const {
        return optimizer_->get_configuration();
    }

    /**
     * @brief Update with performance metrics
     */
    void update_performance(const BatchPerformanceMetrics& metrics) {
        if (!auto_adapt_enabled_) return;

        optimizer_->update_configuration(metrics);
        last_adaptation_ = std::chrono::high_resolution_clock::now();
    }

    /**
     * @brief Enable/disable auto-adaptation
     */
    void set_auto_adapt(bool enabled) {
        auto_adapt_enabled_ = enabled;
        optimizer_->set_adaptive_sizing(enabled);
    }

    /**
     * @brief Set optimization objective
     */
    void set_objective(OptimizationObjective objective) {
        optimizer_->set_objective(objective);
    }

    /**
     * @brief Get device characteristics
     */
    const DeviceCharacteristics& get_device_info() const {
        return device_info_;
    }

    /**
     * @brief Reset to default configuration
     */
    void reset_to_default() {
        // Recreate optimizer with current objective
        auto objective = optimizer_->get_device_info().compute_capability_major >= 8 ?
                        OptimizationObjective::MAXIMIZE_THROUGHPUT :
                        OptimizationObjective::BALANCED_PERFORMANCE;

        optimizer_ = std::make_unique<GradientDescentOptimizer>(device_info_, objective);
        last_adaptation_ = std::chrono::high_resolution_clock::now();
    }

private:
    /**
     * @brief Get device characteristics
     */
    DeviceCharacteristics get_device_characteristics(int device_id) {
        cudaDeviceProp prop;
        cudaError_t result = cudaGetDeviceProperties(&prop, device_id);
        if (result != cudaSuccess) {
            throw std::runtime_error("Failed to get device properties");
        }

        // Estimate memory bandwidth and compute capabilities
        size_t memory_bandwidth = estimate_memory_bandwidth(prop);
        double peak_tflops = estimate_peak_tflops(prop);

        return {
            prop.major,
            prop.minor,
            prop.totalGlobalMem / (1024 * 1024),
            prop.sharedMemPerBlock / 1024,
            prop.maxThreadsPerBlock,
            prop.maxThreadsPerMultiProcessor,
            prop.regsPerBlock / prop.maxThreadsPerBlock,
            prop.maxBlocksPerMultiProcessor,
            prop.l2CacheSize / 1024,
            memory_bandwidth,
            peak_tflops,
            prop.name
        };
    }

    /**
     * @brief Estimate memory bandwidth for GPU
     */
    size_t estimate_memory_bandwidth(const cudaDeviceProp& prop) {
        // Simplified estimation based on GPU architecture
        if (prop.major >= 8) {
            return 900 + (prop.minor * 100); // Ampere: ~900-1000 GB/s
        } else if (prop.major == 7) {
            return 400 + (prop.minor * 50); // Turing: ~400-600 GB/s
        } else if (prop.major == 6) {
            return 300 + (prop.minor * 50); // Pascal: ~300-500 GB/s
        } else {
            return 200; // Older GPUs: ~200 GB/s
        }
    }

    /**
     * @brief Estimate peak TFLOPS for GPU
     */
    double estimate_peak_tflops(const cudaDeviceProp& prop) {
        // Simplified calculation based on CUDA cores and clock speed
        int cuda_cores = estimate_cuda_cores(prop);
        double clock_ghz = prop.clockRate / 1000000.0; // Convert kHz to GHz

        // TFLOPS = (CUDA cores * clock frequency * 2 FLOPs/clock) / 1000
        return (cuda_cores * clock_ghz * 2.0) / 1000.0;
    }

    /**
     * @brief Estimate number of CUDA cores
     */
    int estimate_cuda_cores(const cudaDeviceProp& prop) {
        // Based on NVIDIA specifications
        int sm_count = prop.multiProcessorCount;
        int cores_per_sm = 0;

        if (prop.major >= 8) {
            cores_per_sm = 128; // Ampere
        } else if (prop.major == 7) {
            cores_per_sm = 64;  // Turing
        } else if (prop.major == 6) {
            if (prop.minor == 0) cores_per_sm = 128; // P100
            else if (prop.minor == 1) cores_per_sm = 128; // GP104
            else if (prop.minor == 2) cores_per_sm = 128; // GP102
            else cores_per_sm = 64;  // Others
        } else if (prop.major == 5) {
            cores_per_sm = 128; // Maxwell
        } else if (prop.major == 3) {
            cores_per_sm = 192; // Kepler
        } else {
            cores_per_sm = 64;  // Default
        }

        return sm_count * cores_per_sm;
    }
};

/**
 * @brief Factory for creating adaptive batch sizers
 */
class AdaptiveBatchSizerFactory {
public:
    /**
     * @brief Create adaptive batch sizer with default optimizer
     */
    static std::unique_ptr<AdaptiveBatchSizer> create(
        int device_id = 0,
        OptimizationObjective objective = OptimizationObjective::MAXIMIZE_THROUGHPUT
    ) {
        return std::make_unique<AdaptiveBatchSizer>(device_id, objective, "gradient_descent");
    }

    /**
     * @brief Create adaptive batch sizer with specified optimizer type
     */
    static std::unique_ptr<AdaptiveBatchSizer> create_with_optimizer(
        int device_id,
        OptimizationObjective objective,
        const std::string& optimizer_type
    ) {
        return std::make_unique<AdaptiveBatchSizer>(device_id, objective, optimizer_type);
    }

    /**
     * @brief Create adaptive batch sizer for throughput optimization
     */
    static std::unique_ptr<AdaptiveBatchSizer> create_for_throughput(int device_id = 0) {
        return std::make_unique<AdaptiveBatchSizer>(
            device_id,
            OptimizationObjective::MAXIMIZE_THROUGHPUT,
            "gradient_descent"
        );
    }

    /**
     * @brief Create adaptive batch sizer for memory efficiency
     */
    static std::unique_ptr<AdaptiveBatchSizer> create_for_efficiency(int device_id = 0) {
        return std::make_unique<AdaptiveBatchSizer>(
            device_id,
            OptimizationObjective::MEMORY_CONSERVATIVE,
            "rule_based"
        );
    }
};

// Performance constants for adaptive batch sizing
namespace batch_sizing_performance {
    constexpr double TARGET_THROUGHPUT_IMPROVEMENT = 1.20; // 20% improvement target
    constexpr double MIN_ACCEPTABLE_PERFORMANCE = 0.80;    // 80% of baseline
    constexpr int MAX_ADAPTATION_INTERVAL_MS = 30000;      // 30 seconds max
    constexpr int MIN_ADAPTATION_INTERVAL_MS = 1000;       // 1 second min
    constexpr size_t MIN_BATCH_SIZE = 1000;               // Minimum batch size
    constexpr size_t MAX_BATCH_SIZE = 10000000;            // Maximum batch size
    constexpr double LEARNING_RATE_DEFAULT = 0.1;          // Default learning rate
    constexpr double PERFORMANCE_THRESHOLD_DEFAULT = 0.05;  // 5% performance threshold
}

} // namespace performance
} // namespace keyhunt
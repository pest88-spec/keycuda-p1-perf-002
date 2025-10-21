// Puzzle71Solver - Adaptive Batch Sizing System Implementation
// Dynamic batch size optimization for maximum GPU performance (T032)

#include "adaptive_batch_sizer.cuh"
#include <cuda_runtime.h>
#include <algorithm>
#include <stdexcept>
#include <cmath>

namespace keyhunt {
namespace performance {

// Implementation of virtual destructors and base class methods
BatchSizeOptimizer::~BatchSizeOptimizer() = default;

// Implementation of BatchSizeOptimizer virtual method
void BatchSizeOptimizer::set_objective(OptimizationObjective objective) {
    objective_ = objective;
}

// Implementation of GradientDescentOptimizer methods
void GradientDescentOptimizer::validate_configuration() {
    // Clamp batch sizes to reasonable ranges
    current_config_.ecc_batch_size = std::clamp(
        current_config_.ecc_batch_size,
        batch_sizing_performance::MIN_BATCH_SIZE,
        batch_sizing_performance::MAX_BATCH_SIZE
    );

    current_config_.hash_batch_size = std::clamp(
        current_config_.hash_batch_size,
        batch_sizing_performance::MIN_BATCH_SIZE / 2,
        batch_sizing_performance::MAX_BATCH_SIZE / 2
    );

    current_config_.compare_batch_size = std::clamp(
        current_config_.compare_batch_size,
        batch_sizing_performance::MIN_BATCH_SIZE,
        batch_sizing_performance::MAX_BATCH_SIZE * 2
    );

    // Ensure points_per_thread is reasonable
    current_config_.points_per_thread = std::clamp(
        current_config_.points_per_thread,
        static_cast<size_t>(8),
        static_cast<size_t>(64)
    );

    // Validate adaptation interval
    current_config_.adaptation_interval_ms = std::clamp(
        current_config_.adaptation_interval_ms,
        batch_sizing_performance::MIN_ADAPTATION_INTERVAL_MS,
        batch_sizing_performance::MAX_ADAPTATION_INTERVAL_MS
    );

    // Validate performance threshold
    current_config_.performance_target_threshold = std::clamp(
        current_config_.performance_target_threshold,
        0.01, // 1%
        0.20  // 20%
    );
}

// Implementation of RuleBasedOptimizer methods
void RuleBasedOptimizer::validate_configuration() {
    // Ensure grid size multiplier is reasonable
    current_config_.grid_size_multiplier = std::clamp(
        current_config_.grid_size_multiplier, 1, 8
    );

    // Ensure block sizes are reasonable and within device limits
    current_config_.block_size_ecc = std::clamp(
        current_config_.block_size_ecc, 128, device_info_.max_threads_per_block
    );

    current_config_.block_size_hash = std::clamp(
        current_config_.block_size_hash, 128, device_info_.max_threads_per_block
    );

    current_config_.block_size_compare = std::clamp(
        current_config_.block_size_compare, 256, device_info_.max_threads_per_block
    );

    // Validate shared memory usage
    if (current_config_.use_shared_memory) {
        size_t required_shared_memory = calculate_required_shared_memory();
        if (required_shared_memory > device_info_.shared_memory_per_block_kb * 1024) {
            current_config_.use_shared_memory = false;
        }
    }
}

size_t RuleBasedOptimizer::calculate_required_shared_memory() const {
    // Calculate shared memory requirements based on batch size and kernel type
    size_t ecc_shared = current_config_.ecc_batch_size * sizeof(unsigned int) * 8; // X coordinates
    size_t hash_shared = current_config_.hash_batch_size * sizeof(unsigned int) * 16; // X + Y coordinates
    size_t compare_shared = 32 * sizeof(std::uint32_t) * 5; // Up to 32 target hashes

    return ecc_shared + hash_shared + compare_shared;
}

// Implementation of AdaptiveBatchSizer methods
AdaptiveBatchSizer::AdaptiveBatchSizer(
    int device_id,
    OptimizationObjective objective,
    const std::string& optimizer_type
) : auto_adapt_enabled_(true) {
    // Set device
    cudaError_t result = cudaSetDevice(device_id);
    if (result != cudaSuccess) {
        throw std::runtime_error("Failed to set CUDA device: " + std::to_string(device_id));
    }

    // Get device information
    device_info_ = get_device_characteristics(device_id);

    // Create optimizer based on type
    if (optimizer_type == "gradient_descent") {
        optimizer_ = std::make_unique<GradientDescentOptimizer>(device_info_, objective);
    } else if (optimizer_type == "rule_based") {
        optimizer_ = std::make_unique<RuleBasedOptimizer>(device_info_, objective);
    } else {
        throw std::invalid_argument("Unknown optimizer type: " + optimizer_type);
    }

    last_adaptation_ = std::chrono::high_resolution_clock::now();
}

void AdaptiveBatchSizer::update_performance(const BatchPerformanceMetrics& metrics) {
    if (!auto_adapt_enabled_) return;

    // Check if enough time has passed since last adaptation
    auto now = std::chrono::high_resolution_clock::now();
    auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_adaptation_).count();

    if (time_since_last < optimizer_->get_configuration().adaptation_interval_ms) {
        return;
    }

    optimizer_->update_configuration(metrics);
    last_adaptation_ = now;
}

void AdaptiveBatchSizer::set_objective(OptimizationObjective objective) {
    optimizer_->set_objective(objective);
}

DeviceCharacteristics AdaptiveBatchSizer::get_device_characteristics(int device_id) {
    cudaDeviceProp prop;
    cudaError_t result = cudaGetDeviceProperties(&prop, device_id);
    if (result != cudaSuccess) {
        throw std::runtime_error("Failed to get device properties for device: " + std::to_string(device_id));
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

size_t AdaptiveBatchSizer::estimate_memory_bandwidth(const cudaDeviceProp& prop) {
    // More accurate bandwidth estimation based on GPU architecture and memory type
    size_t base_bandwidth = 0;

    if (prop.major >= 9) {
        // Hopper architecture
        base_bandwidth = 2000 + (prop.minor * 200); // ~2-3 TB/s
    } else if (prop.major == 8) {
        // Ampere architecture
        if (prop.minor >= 7) {
            base_bandwidth = 1000 + (prop.minor - 7) * 100; // ~1-2 TB/s
        } else if (prop.minor == 6) {
            base_bandwidth = 936; // A100
        } else if (prop.minor == 0) {
            base_bandwidth = 616; // RTX 3080
        } else {
            base_bandwidth = 448; // RTX 3070/2080 Ti
        }
    } else if (prop.major == 7) {
        // Turing architecture
        if (prop.minor >= 5) {
            base_bandwidth = 616; // RTX 2080 Ti/2080 Super
        } else if (prop.minor == 0) {
            base_bandwidth = 448; // RTX 2080/2070
        } else {
            base_bandwidth = 336; // RTX 2060
        }
    } else if (prop.major == 6) {
        // Pascal architecture
        if (prop.minor == 1) {
            base_bandwidth = 484; // GTX 1080 Ti
        } else if (prop.minor == 0) {
            base_bandwidth = 320; // GTX 1080/1070
        } else {
            base_bandwidth = 256; // GTX 1060
        }
    } else {
        // Older architectures - conservative estimate
        base_bandwidth = 200;
    }

    // Adjust based on memory bus width if available
    if (prop.memoryBusWidth > 0) {
        // Normalize to 256-bit bus width
        double bus_factor = static_cast<double>(prop.memoryBusWidth) / 256.0;
        base_bandwidth = static_cast<size_t>(base_bandwidth * bus_factor);
    }

    return base_bandwidth;
}

double AdaptiveBatchSizer::estimate_peak_tflops(const cudaDeviceProp& prop) {
    int cuda_cores = estimate_cuda_cores(prop);
    double clock_ghz = prop.clockRate / 1000000.0; // Convert kHz to GHz

    // TFLOPS = (CUDA cores * clock frequency * 2 FLOPs/clock) / 1000
    // Adjust for architecture efficiency
    double efficiency_factor = 1.0;

    if (prop.major >= 8) {
        efficiency_factor = 1.2; // Ampere efficiency improvements
    } else if (prop.major == 7) {
        efficiency_factor = 1.1; // Turing efficiency improvements
    } else if (prop.major == 6) {
        efficiency_factor = 1.0; // Pascal baseline
    } else {
        efficiency_factor = 0.8; // Older architectures
    }

    return (cuda_cores * clock_ghz * 2.0 * efficiency_factor) / 1000.0;
}

int AdaptiveBatchSizer::estimate_cuda_cores(const cudaDeviceProp& prop) {
    int sm_count = prop.multiProcessorCount;
    int cores_per_sm = 0;

    // Based on NVIDIA specifications
    if (prop.major >= 9) {
        // Hopper
        cores_per_sm = 128;
    } else if (prop.major == 8) {
        // Ampere
        if (prop.minor >= 7) {
            cores_per_sm = 128; // H100/A100
        } else if (prop.minor == 6) {
            cores_per_sm = 128; // RTX 3090 Ti
        } else if (prop.minor == 0) {
            cores_per_sm = 128; // RTX 3080/3070
        } else {
            cores_per_sm = 128; // Other Ampere
        }
    } else if (prop.major == 7) {
        // Turing
        if (prop.minor >= 5) {
            cores_per_sm = 64; // RTX 2080 Ti/2080 Super
        } else if (prop.minor == 0) {
            cores_per_sm = 64;  // RTX 2080/2070/2060
        } else {
            cores_per_sm = 64;  // Other Turing
        }
    } else if (prop.major == 6) {
        // Pascal
        if (prop.minor == 1) {
            if (prop.name.find("1080") != std::string::npos) {
                cores_per_sm = 128; // GTX 1080 series
            } else if (prop.name.find("1070") != std::string::npos) {
                cores_per_sm = 128; // GTX 1070 series
            } else {
                cores_per_sm = 128; // Other GP104
            }
        } else if (prop.minor == 0) {
            if (prop.name.find("1060") != std::string::npos) {
                cores_per_sm = 128; // GTX 1060
            } else if (prop.name.find("P100") != std::string::npos) {
                cores_per_sm = 64;  // Tesla P100
            } else {
                cores_per_sm = 128; // Other GP106
            }
        } else {
            cores_per_sm = 64;  // Default Pascal
        }
    } else if (prop.major == 5) {
        // Maxwell
        cores_per_sm = 128;
    } else if (prop.major == 3) {
        // Kepler
        if (prop.minor >= 5) {
            cores_per_sm = 192; // GK110/Kepler
        } else {
            cores_per_sm = 192; // Other Kepler
        }
    } else {
        // Default fallback
        cores_per_sm = 64;
    }

    return sm_count * cores_per_sm;
}

// Implementation of BatchSizeOptimizer virtual method (accessing protected member)
void BatchSizeOptimizer::set_objective(OptimizationObjective objective) {
    objective_ = objective;
}

// Implementation of factory methods
std::unique_ptr<AdaptiveBatchSizer> AdaptiveBatchSizerFactory::create(
    int device_id,
    OptimizationObjective objective
) {
    return std::make_unique<AdaptiveBatchSizer>(device_id, objective, "gradient_descent");
}

std::unique_ptr<AdaptiveBatchSizer> AdaptiveBatchSizerFactory::create_with_optimizer(
    int device_id,
    OptimizationObjective objective,
    const std::string& optimizer_type
) {
    return std::make_unique<AdaptiveBatchSizer>(device_id, objective, optimizer_type);
}

std::unique_ptr<AdaptiveBatchSizer> AdaptiveBatchSizerFactory::create_for_throughput(int device_id) {
    return std::make_unique<AdaptiveBatchSizer>(
        device_id,
        OptimizationObjective::MAXIMIZE_THROUGHPUT,
        "gradient_descent"
    );
}

std::unique_ptr<AdaptiveBatchSizer> AdaptiveBatchSizerFactory::create_for_efficiency(int device_id) {
    return std::make_unique<AdaptiveBatchSizer>(
        device_id,
        OptimizationObjective::MEMORY_CONSERVATIVE,
        "rule_based"
    );
}

/**
 * @brief Performance profiler for batch sizing
 */
class BatchPerformanceProfiler {
public:
    struct ProfileResult {
        std::vector<BatchPerformanceMetrics> metrics_history;
        double average_throughput;
        double peak_throughput;
        double average_efficiency;
        size_t total_adaptations;
        double adaptation_success_rate;
        std::map<std::string, double> parameter_sensitivity;
    };

    /**
     * @brief Profile batch sizing performance over time
     */
    static ProfileResult profile_adaptive_batching(
        AdaptiveBatchSizer& sizer,
        int test_duration_seconds = 60
    ) {
        ProfileResult result;
        auto start_time = std::chrono::high_resolution_clock::now();
        auto end_time = start_time + std::chrono::seconds(test_duration_seconds);

        size_t adaptation_count = 0;
        size_t successful_adaptations = 0;

        while (std::chrono::high_resolution_clock::now() < end_time) {
            // Simulate performance metrics (in real implementation, this would come from actual kernels)
            BatchPerformanceMetrics metrics = generate_mock_metrics(sizer.get_device_info());

            // Get configuration before update
            auto config_before = sizer.get_configuration();

            // Update sizer with metrics
            sizer.update_performance(metrics);

            // Get configuration after update
            auto config_after = sizer.get_configuration();

            // Check if adaptation occurred
            bool adapted = (config_before.ecc_batch_size != config_after.ecc_batch_size) ||
                         (config_before.hash_batch_size != config_after.hash_batch_size) ||
                         (config_before.compare_batch_size != config_after.compare_batch_size);

            if (adapted) {
                adaptation_count++;
                // In real implementation, we'd check if this led to improvement
                successful_adaptations++;
            }

            result.metrics_history.push_back(metrics);

            // Wait for next measurement
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Calculate statistics
        if (!result.metrics_history.empty()) {
            double total_throughput = 0;
            double total_efficiency = 0;
            double max_throughput = 0;

            for (const auto& metrics : result.metrics_history) {
                total_throughput += metrics.throughput_mkeys_per_sec;
                total_efficiency += metrics.memory_efficiency_percent;
                max_throughput = std::max(max_throughput, metrics.throughput_mkeys_per_sec);
            }

            result.average_throughput = total_throughput / result.metrics_history.size();
            result.peak_throughput = max_throughput;
            result.average_efficiency = total_efficiency / result.metrics_history.size();
        }

        result.total_adaptations = adaptation_count;
        result.adaptation_success_rate = adaptation_count > 0 ?
                                       (double)successful_adaptations / adaptation_count : 0.0;

        return result;
    }

private:
    static BatchPerformanceMetrics generate_mock_metrics(const DeviceCharacteristics& device_info) {
        static double performance_noise = 0.0;
        performance_noise += 0.01; // Simulate performance variations

        double base_throughput = device_info.memory_bandwidth_gb_per_sec * 0.001; // Convert to Mkeys/s
        double throughput = base_throughput * (1.0 + sin(performance_noise) * 0.1);

        return {
            .throughput_mkeys_per_sec = throughput,
            .gpu_utilization_percent = 85.0 + cos(performance_noise) * 10.0,
            .memory_bandwidth_utilization = 0.80 + sin(performance_noise * 2) * 0.15,
            .occupancy_rate = 0.75 + cos(performance_noise * 0.5) * 0.20,
            .kernel_execution_time_ms = 1000.0 / throughput,
            .memory_efficiency_percent = 90.0 + sin(performance_noise * 1.5) * 8.0,
            .memory_usage_mb = device_info.total_global_memory_mb * 0.6,
            .cache_hit_rate_percent = 85.0 + cos(performance_noise * 0.8) * 10.0,
            .power_consumption_watts = device_info.peak_tflops * 0.3,
            .timestamp = std::chrono::high_resolution_clock::now()
        };
    }
};

} // namespace performance
} // namespace keyhunt
// Puzzle71 Technical Debt Repair - Static Configuration Integration Layer (T030)
// Connects static configuration system with kernel launch system
// Implements T026-T028 integration with constitutional compliance

#pragma once

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <chrono>
#include <mutex>
#include <atomic>

#include "KeyhuntCore/common/static_launch_config.h"
#include "KeyhuntCore/common/static_launch_config.cpp"
#include "config/puzzle71_config_validator.h"
#include "compute/gpu/launch_config.h"
#include "compute/gpu/gpu_executor.h"

namespace keyhunt {
namespace integration {

/**
 * @brief Performance metrics for kernel launches with static configuration
 */
struct KernelLaunchMetrics {
    std::chrono::microseconds launch_time{0};
    std::chrono::microseconds execution_time{0};
    std::chrono::microseconds total_time{0};
    uint64_t keys_processed{0};
    double keys_per_second{0.0};
    double memory_efficiency_percent{0.0};
    double gpu_utilization_percent{0.0};
    double occupancy_percent{0.0};
    bool constitutional_compliance{false};
    std::string validation_status;
    std::string error_message;

    void reset() {
        launch_time = std::chrono::microseconds{0};
        execution_time = std::chrono::microseconds{0};
        total_time = std::chrono::microseconds{0};
        keys_processed = 0;
        keys_per_second = 0.0;
        memory_efficiency_percent = 0.0;
        gpu_utilization_percent = 0.0;
        occupancy_percent = 0.0;
        constitutional_compliance = false;
        validation_status.clear();
        error_message.clear();
    }

    std::string to_json() const {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer),
            "{"
            "\"launch_time_us\":%ld,"
            "\"execution_time_us\":%ld,"
            "\"total_time_us\":%ld,"
            "\"keys_processed\":%lu,"
            "\"keys_per_second\":%.2f,"
            "\"memory_efficiency_percent\":%.2f,"
            "\"gpu_utilization_percent\":%.2f,"
            "\"occupancy_percent\":%.2f,"
            "\"constitutional_compliance\":%s,"
            "\"validation_status\":\"%s\","
            "\"error_message\":\"%s\""
            "}",
            launch_time.count(),
            execution_time.count(),
            total_time.count(),
            keys_processed,
            keys_per_second,
            memory_efficiency_percent,
            gpu_utilization_percent,
            occupancy_percent,
            constitutional_compliance ? "true" : "false",
            validation_status.c_str(),
            error_message.c_str()
        );
        return std::string(buffer);
    }
};

/**
 * @brief Integrated kernel launch configuration
 *
 * Combines static configuration from StaticLaunchConfigManager with
 * dynamic kernel launch parameters for optimal performance
 */
struct IntegratedLaunchConfig {
    // Static configuration components
    keyhunt::config::StaticLaunchConfig static_config;
    keyhunt::config::ECCStaticConfig ecc_config;
    keyhunt::config::AdvancedLaunchConfig advanced_config;

    // Dynamic launch parameters
    dim3 grid_dim;
    dim3 block_dim;
    size_t shared_memory_size;
    int points_per_thread;
    uint64_t batch_size;

    // Configuration metadata
    keyhunt::config::GPUArchitecture architecture;
    std::string config_source;  // "static", "yaml", "fallback"
    std::string config_file_path;
    uint32_t config_version;
    bool is_validated;
    bool constitutional_compliance;

    // Performance targets
    double target_throughput_keys_per_sec;
    double target_memory_efficiency_percent;
    double target_gpu_utilization_percent;
    double target_occupancy_percent;

    // Validation state
    config::ValidationResult validation_result;
    std::chrono::steady_clock::time_point config_load_time;
    std::chrono::steady_clock::time_point last_validation_time;

    /**
     * @brief Calculate total threads for this configuration
     */
    uint64_t get_total_threads() const {
        return static_cast<uint64_t>(grid_dim.x) * grid_dim.y * grid_dim.z *
               static_cast<uint64_t>(block_dim.x) * block_dim.y * block_dim.z;
    }

    /**
     * @brief Calculate expected keys per batch
     */
    uint64_t get_keys_per_batch() const {
        return get_total_threads() * points_per_thread;
    }

    /**
     * @brief Check if configuration is constitutional compliant
     */
    bool is_constitutional_compliant() const {
        return constitutional_compliance &&
               static_config.static_configuration_only &&
               static_config.no_runtime_device_queries &&
               static_config.deterministic_launch &&
               validation_result.is_valid;
    }

    /**
     * @brief Get configuration summary
     */
    std::string get_summary() const {
        char summary[512];
        snprintf(summary, sizeof(summary),
            "Integrated Config [%s]: Grid(%d,%d,%d) Block(%d,%d,%d) "
            "Points=%d Batch=%lu Architecture=%s Validated=%s Constitutional=%s",
            config_source.c_str(),
            grid_dim.x, grid_dim.y, grid_dim.z,
            block_dim.x, block_dim.y, block_dim.z,
            points_per_thread,
            batch_size,
            keyhunt::config::StaticLaunchConfigManager::get_config_summary(static_config),
            is_validated ? "YES" : "NO",
            constitutional_compliance ? "YES" : "NO"
        );
        return std::string(summary);
    }
};

/**
 * @brief Fallback configuration provider for error recovery
 */
class FallbackConfigurationProvider {
public:
    /**
     * @brief Get safe fallback configuration for any architecture
     *
     * @param architecture Target GPU architecture
     * @param error_code Error code from failed configuration
     * @return Safe fallback configuration
     */
    static IntegratedLaunchConfig get_safe_fallback(
        keyhunt::config::GPUArchitecture architecture,
        int error_code = 0
    );

    /**
     * @brief Get minimal configuration for testing
     */
    static IntegratedLaunchConfig get_minimal_config();

    /**
     * @brief Get performance-optimized fallback
     */
    static IntegratedLaunchConfig get_performance_fallback(
        keyhunt::config::GPUArchitecture architecture
    );

private:
    static const keyhunt::config::StaticLaunchConfig& get_conservative_static_config();
    static const keyhunt::config::ECCStaticConfig& get_conservative_ecc_config();
};

/**
 * @brief Static configuration integration manager
 *
 * This class provides the main integration between the static configuration
 * system and the kernel launch system, ensuring constitutional compliance
 * and optimal performance.
 */
class StaticConfigIntegrationManager {
public:
    /**
     * @brief Initialize the integration manager
     *
     * @param device_id CUDA device ID
     * @param config_file Optional YAML configuration file
     */
    explicit StaticConfigIntegrationManager(int device_id,
                                          const std::string& config_file = "");

    ~StaticConfigIntegrationManager();

    /**
     * @brief Get integrated launch configuration
     *
     * @param kernel_name Name of the kernel to configure
     * @param batch_size Desired batch size
     * @param enable_yaml_config Enable YAML configuration loading
     * @return Integrated launch configuration
     */
    IntegratedLaunchConfig get_launch_config(
        const std::string& kernel_name,
        uint64_t batch_size,
        bool enable_yaml_config = true
    );

    /**
     * @brief Validate configuration before kernel launch
     *
     * @param config Configuration to validate
     * @return True if configuration is valid and constitutional compliant
     */
    bool validate_launch_config(IntegratedLaunchConfig& config);

    /**
     * @brief Launch kernel with integrated configuration
     *
     * @param config Integrated launch configuration
     * @param kernel_func Kernel function pointer
     * @param stream CUDA stream (optional)
     * @return CUDA error code
     */
    template<typename KernelFunc>
    cudaError_t launch_kernel(
        const IntegratedLaunchConfig& config,
        KernelFunc kernel_func,
        cudaStream_t stream = 0
    );

    /**
     * @brief Get performance metrics for last launch
     */
    const KernelLaunchMetrics& get_last_metrics() const { return last_metrics_; }

    /**
     * @brief Enable/disable deterministic mode
     *
     * @param enabled Enable deterministic mode
     * @param seed Optional deterministic seed
     */
    void set_deterministic_mode(bool enabled, uint32_t seed = 0);

    /**
     * @brief Check if deterministic mode is enabled
     */
    bool is_deterministic_mode() const { return deterministic_mode_; }

    /**
     * @brief Get current device architecture
     */
    keyhunt::config::GPUArchitecture get_device_architecture() const { return device_architecture_; }

    /**
     * @brief Reload configuration from file
     *
     * @param config_file Path to configuration file
     * @return True if reload successful
     */
    bool reload_configuration(const std::string& config_file);

    /**
     * @brief Export current configuration for reproducibility
     *
     * @param output_file Output file path
     * @return True if export successful
     */
    bool export_configuration(const std::string& output_file) const;

    /**
     * @brief Get configuration compatibility report
     */
    std::string get_compatibility_report() const;

    /**
     * @brief Validate constitutional compliance
     */
    bool validate_constitutional_compliance() const;

private:
    int device_id_;
    keyhunt::config::GPUArchitecture device_architecture_;
    std::string config_file_path_;

    // Configuration cache
    mutable std::mutex config_mutex_;
    std::map<std::string, IntegratedLaunchConfig> config_cache_;
    std::chrono::steady_clock::time_point last_config_load_;

    // Deterministic mode
    std::atomic<bool> deterministic_mode_{false};
    std::atomic<uint32_t> deterministic_seed_{0};

    // Performance metrics
    KernelLaunchMetrics last_metrics_;

    // Initialization state
    std::atomic<bool> initialized_{false};

    /**
     * @brief Initialize device architecture detection
     */
    void initialize_device_architecture();

    /**
     * @brief Load configuration from YAML file
     *
     * @param config_file Path to YAML file
     * @return Loaded configuration or empty
     */
    std::optional<IntegratedLaunchConfig> load_yaml_config(
        const std::string& config_file,
        const std::string& kernel_name
    );

    /**
     * @brief Create configuration from static defaults
     *
     * @param kernel_name Kernel name
     * @param batch_size Batch size
     * @return Static configuration
     */
    IntegratedLaunchConfig create_static_config(
        const std::string& kernel_name,
        uint64_t batch_size
    );

    /**
     * @brief Validate configuration against device capabilities
     *
     * @param config Configuration to validate
     * @return True if compatible
     */
    bool validate_device_compatibility(IntegratedLaunchConfig& config);

    /**
     * @brief Measure kernel launch performance
     *
     * @param config Configuration used for launch
     * @param start_time Launch start time
     * @param end_time Launch end time
     */
    void measure_performance(
        const IntegratedLaunchConfig& config,
        std::chrono::steady_clock::time_point start_time,
        std::chrono::steady_clock::time_point end_time
    );

    /**
     * @brief Apply deterministic modifications to configuration
     *
     * @param config Configuration to modify
     */
    void apply_deterministic_modifications(IntegratedLaunchConfig& config);

    /**
     * @brief Check configuration cache validity
     */
    bool is_config_cache_valid() const;

    /**
     * @brief Clear configuration cache
     */
    void clear_config_cache();
};

/**
 * @brief Kernel launcher with static configuration integration
 *
 * High-level interface for launching kernels with automatic configuration
 * selection, validation, and performance monitoring.
 */
class StaticConfigKernelLauncher {
public:
    /**
     * @brief Create kernel launcher for device
     *
     * @param device_id CUDA device ID
     * @param config_file Optional configuration file
     */
    explicit StaticConfigKernelLauncher(int device_id,
                                       const std::string& config_file = "");

    ~StaticConfigKernelLauncher();

    /**
     * @brief Launch ECC kernel with automatic configuration
     *
     * @param batch_size Number of private keys to process
     * @param compression_type Address compression type
     * @param target_hash Target hash160 to match
     * @return CUDA error code
     */
    cudaError_t launch_ecc_kernel(
        uint64_t batch_size,
        int compression_type,
        const std::array<uint32_t, 5>& target_hash
    );

    /**
     * @brief Launch hash kernel with automatic configuration
     *
     * @param batch_size Number of points to hash
     * @param compression_type Address compression type
     * @return CUDA error code
     */
    cudaError_t launch_hash_kernel(
        uint64_t batch_size,
        int compression_type
    );

    /**
     * @brief Launch fixed kernel with static configuration
     *
     * @param points_per_thread Points per thread
     * @param compression_type Address compression type
     * @return CUDA error code
     */
    cudaError_t launch_fixed_kernel(
        int points_per_thread,
        int compression_type
    );

    /**
     * @brief Get performance metrics
     */
    const KernelLaunchMetrics& get_metrics() const;

    /**
     * @brief Get integration manager reference
     */
    StaticConfigIntegrationManager& get_integration_manager() { return *integration_manager_; }

    /**
     * @brief Reset performance metrics
     */
    void reset_metrics();

private:
    std::unique_ptr<StaticConfigIntegrationManager> integration_manager_;
    std::unique_ptr<puzzle71::gpu::GpuExecutor> gpu_executor_;
    int device_id_;
    std::array<uint32_t, 5> current_target_hash_;
    bool target_hash_set_;

    /**
     * @brief Initialize GPU executor
     */
    void initialize_gpu_executor();

    /**
     * @brief Upload target hash to device
     */
    cudaError_t upload_target_hash(const std::array<uint32_t, 5>& target_hash);
};

/**
 * @brief Utility functions for static configuration integration
 */
namespace integration_utils {

    /**
     * @brief Convert architecture enum to string
     */
    std::string architecture_to_string(keyhunt::config::GPUArchitecture arch);

    /**
     * @brief Parse architecture from string
     */
    keyhunt::config::GPUArchitecture parse_architecture(const std::string& arch_str);

    /**
     * @brief Validate configuration compatibility
     */
    bool validate_config_compatibility(
        const keyhunt::config::StaticLaunchConfig& static_config,
        const puzzle71::gpu::KernelLaunchConfig& kernel_config
    );

    /**
     * @brief Estimate performance for configuration
     */
    std::map<std::string, double> estimate_performance(
        const IntegratedLaunchConfig& config
    );

    /**
     * @brief Generate configuration fingerprint
     */
    std::string generate_config_fingerprint(const IntegratedLaunchConfig& config);

    /**
     * @brief Compare two configurations
     */
    std::vector<std::string> compare_configurations(
        const IntegratedLaunchConfig& config1,
        const IntegratedLaunchConfig& config2
    );
}

} // namespace integration
} // namespace keyhunt

// Template implementation
namespace keyhunt {
namespace integration {

template<typename KernelFunc>
cudaError_t StaticConfigIntegrationManager::launch_kernel(
    const IntegratedLaunchConfig& config,
    KernelFunc kernel_func,
    cudaStream_t stream
) {
    if (!config.is_constitutional_compliant()) {
        return cudaErrorInvalidConfiguration;
    }

    auto start_time = std::chrono::steady_clock::now();

    // Record launch start
    auto launch_start = std::chrono::high_resolution_clock::now();

    // Launch kernel with static configuration
    cudaError_t result = kernel_func(
        config.grid_dim,
        config.block_dim,
        config.shared_memory_size,
        stream,
        config.points_per_thread,
        config.ecc_config.batch_size
    );

    auto launch_end = std::chrono::high_resolution_clock::now();

    if (result != cudaSuccess) {
        last_metrics_.error_message = cudaGetErrorString(result);
        return result;
    }

    // Wait for completion and measure execution time
    result = cudaStreamSynchronize(stream);
    auto end_time = std::chrono::steady_clock::now();

    if (result == cudaSuccess) {
        measure_performance(config, start_time, end_time);
        last_metrics_.launch_time = std::chrono::duration_cast<std::chrono::microseconds>(
            launch_end - launch_start);
        last_metrics_.execution_time = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - launch_end);
        last_metrics_.total_time = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time);
    }

    return result;
}

} // namespace integration
} // namespace keyhunt
// Puzzle71 Technical Debt Repair - Static Configuration Adapter Layer (T030)
// Adapter pattern implementation for seamless configuration management
// Integrates static configuration system with existing compute adapters

#pragma once

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <optional>
#include <functional>
#include <mutex>

#include "compute/gpu/static_config_integration.h"
#include "compute/gpu/kernel_config_validator.h"
#include "compute/adapters/reference/keyfinder_adapter.h"
#include "KeyhuntCore/common/static_launch_config.h"

namespace keyhunt {
namespace adapters {

/**
 * @brief Configuration adapter interface
 *
 * Provides a unified interface for configuration management
 * across different adapter implementations.
 */
class IConfigurationAdapter {
public:
    virtual ~IConfigurationAdapter() = default;

    /**
     * @brief Initialize adapter with configuration
     *
     * @param config_file Optional configuration file path
     * @return True if initialization successful
     */
    virtual bool initialize(const std::string& config_file = "") = 0;

    /**
     * @brief Get launch configuration for kernel
     *
     * @param kernel_name Name of the kernel
     * @param batch_size Desired batch size
     * @return Launch configuration
     */
    virtual integration::IntegratedLaunchConfig get_launch_config(
        const std::string& kernel_name,
        uint64_t batch_size
    ) = 0;

    /**
     * @brief Validate configuration
     *
     * @param config Configuration to validate
     * @return True if configuration is valid
     */
    virtual bool validate_config(
        const integration::IntegratedLaunchConfig& config
    ) = 0;

    /**
     * @brief Get adapter information
     */
    virtual std::string get_adapter_info() const = 0;

    /**
     * @brief Check if adapter supports kernel
     *
     * @param kernel_name Name of the kernel
     * @return True if kernel is supported
     */
    virtual bool supports_kernel(const std::string& kernel_name) const = 0;
};

/**
 * @brief Static configuration adapter implementation
 *
 * Adapts the static configuration system to work with existing
 * compute adapters and provides seamless configuration management.
 */
class StaticConfigurationAdapter : public IConfigurationAdapter {
public:
    /**
     * @brief Create static configuration adapter
     *
     * @param device_id CUDA device ID
     * @param config_file Optional configuration file
     */
    explicit StaticConfigurationAdapter(
        int device_id,
        const std::string& config_file = ""
    );

    ~StaticConfigurationAdapter() override = default;

    bool initialize(const std::string& config_file = "") override;

    integration::IntegratedLaunchConfig get_launch_config(
        const std::string& kernel_name,
        uint64_t batch_size
    ) override;

    bool validate_config(
        const integration::IntegratedLaunchConfig& config
    ) override;

    std::string get_adapter_info() const override;

    bool supports_kernel(const std::string& kernel_name) const override;

    /**
     * @brief Get integration manager reference
     */
    integration::StaticConfigIntegrationManager& get_integration_manager() {
        return *integration_manager_;
    }

    /**
     * @brief Get validator reference
     */
    validation::KernelConfigValidator& get_validator() {
        return *validator_;
    }

    /**
     * @brief Enable/disable strict validation mode
     */
    void set_strict_mode(bool enabled);

    /**
     * @brief Get configuration statistics
     */
    std::map<std::string, uint64_t> get_statistics() const;

private:
    int device_id_;
    std::string config_file_;
    std::unique_ptr<integration::StaticConfigIntegrationManager> integration_manager_;
    std::unique_ptr<validation::KernelConfigValidator> validator_;

    // Supported kernels
    std::vector<std::string> supported_kernels_;

    // Statistics
    mutable std::mutex stats_mutex_;
    std::map<std::string, uint64_t> statistics_;

    /**
     * @brief Initialize supported kernels list
     */
    void initialize_supported_kernels();

    /**
     * @brief Update statistics
     */
    void update_statistics(const std::string& operation);
};

/**
 * @brief Reference adapter wrapper
 *
 * Wraps the existing reference adapter to provide configuration
 * management through the static configuration system.
 */
class ReferenceAdapterWrapper {
public:
    /**
     * @brief Create reference adapter wrapper
     *
     * @param device_id CUDA device ID
     * @param config_file Optional configuration file
     */
    explicit ReferenceAdapterWrapper(
        int device_id,
        const std::string& config_file = ""
    );

    ~ReferenceAdapterWrapper() = default;

    /**
     * @brief Initialize adapter
     */
    bool initialize();

    /**
     * @brief Launch kernel with configuration management
     *
     * @param kernel_name Name of the kernel
     * @param batch_size Number of items to process
     * @param parameters Additional kernel parameters
     * @return True if launch successful
     */
    bool launch_kernel(
        const std::string& kernel_name,
        uint64_t batch_size,
        const std::map<std::string, std::string>& parameters = {}
    );

    /**
     * @brief Get last performance metrics
     */
    const integration::KernelLaunchMetrics& get_last_metrics() const;

    /**
     * @brief Get adapter information
     */
    std::string get_adapter_info() const;

private:
    int device_id_;
    std::string config_file_;
    std::unique_ptr<StaticConfigurationAdapter> config_adapter_;
    std::unique_ptr<puzzle71::gpu::GpuExecutor> gpu_executor_;

    // Performance metrics
    integration::KernelLaunchMetrics last_metrics_;

    /**
     * @brief Initialize GPU executor
     */
    bool initialize_gpu_executor();

    /**
     * @brief Convert parameters to kernel launch format
     */
    std::map<std::string, std::any> convert_parameters(
        const std::map<std::string, std::string>& parameters
    );
};

/**
 * @brief Configuration adapter factory
 *
 * Creates appropriate configuration adapters based on requirements.
 */
class ConfigurationAdapterFactory {
public:
    /**
     * @brief Adapter types
     */
    enum class AdapterType {
        STATIC_CONFIG,      // Static configuration adapter
        REFERENCE_WRAPPER,  // Reference adapter wrapper
        CUSTOM              // Custom adapter
    };

    /**
     * @brief Create configuration adapter
     *
     * @param type Type of adapter to create
     * @param device_id CUDA device ID
     * @param config_file Optional configuration file
     * @return Created adapter
     */
    static std::unique_ptr<IConfigurationAdapter> create_adapter(
        AdapterType type,
        int device_id,
        const std::string& config_file = ""
    );

    /**
     * @brief Create best adapter for device
     *
     * @param device_id CUDA device ID
     * @param config_file Optional configuration file
     * @return Most suitable adapter for the device
     */
    static std::unique_ptr<IConfigurationAdapter> create_best_adapter(
        int device_id,
        const std::string& config_file = ""
    );

    /**
     * @brief Get available adapter types
     */
    static std::vector<AdapterType> get_available_types();

private:
    static bool is_static_config_supported(int device_id);
    static bool is_reference_wrapper_supported(int device_id);
};

/**
 * @brief Configuration manager for adapter coordination
 *
 * Manages multiple configuration adapters and provides
 * unified access to configuration services.
 */
class AdapterConfigurationManager {
public:
    /**
     * @brief Create adapter configuration manager
     */
    AdapterConfigurationManager();

    ~AdapterConfigurationManager() = default;

    /**
     * @brief Register adapter for device
     *
     * @param device_id CUDA device ID
     * @param adapter Configuration adapter
     * @return True if registration successful
     */
    bool register_adapter(
        int device_id,
        std::unique_ptr<IConfigurationAdapter> adapter
    );

    /**
     * @brief Get adapter for device
     *
     * @param device_id CUDA device ID
     * @return Registered adapter or nullptr
     */
    IConfigurationAdapter* get_adapter(int device_id);

    /**
     * @brief Remove adapter for device
     *
     * @param device_id CUDA device ID
     * @return True if adapter was removed
     */
    bool remove_adapter(int device_id);

    /**
     * @brief Get configuration for all devices
     *
     * @param kernel_name Name of the kernel
     * @param batch_size Desired batch size
     * @return Map of device configurations
     */
    std::map<int, integration::IntegratedLaunchConfig> get_all_configurations(
        const std::string& kernel_name,
        uint64_t batch_size
    );

    /**
     * @brief Validate all adapters
     *
     * @return Map of validation results by device
     */
    std::map<int, bool> validate_all_adapters();

    /**
     * @brief Get manager statistics
     */
    std::map<std::string, std::any> get_statistics() const;

private:
    std::map<int, std::unique_ptr<IConfigurationAdapter>> adapters_;
    mutable std::mutex adapters_mutex_;

    /**
     * @brief Get registered device IDs
     */
    std::vector<int> get_device_ids() const;
};

/**
 * @brief Performance monitoring for adapters
 *
 * Monitors adapter performance and provides optimization recommendations.
 */
class AdapterPerformanceMonitor {
public:
    /**
     * @brief Performance metrics for adapter
     */
    struct AdapterMetrics {
        std::string adapter_name;
        int device_id;
        uint64_t total_launches{0};
        double avg_launch_time_ms{0.0};
        double min_launch_time_ms{0.0};
        double max_launch_time_ms{0.0};
        double total_keys_processed{0.0};
        double avg_keys_per_second{0.0};
        double memory_efficiency_percent{0.0};
        double gpu_utilization_percent{0.0};
        std::chrono::steady_clock::time_point last_update;
        bool constitutional_compliance{false};
    };

    /**
     * @brief Record kernel launch performance
     *
     * @param adapter_name Name of the adapter
     * @param device_id Device ID
     * @param metrics Launch metrics
     */
    static void record_launch(
        const std::string& adapter_name,
        int device_id,
        const integration::KernelLaunchMetrics& metrics
    );

    /**
     * @brief Get metrics for adapter
     *
     * @param adapter_name Name of the adapter
     * @param device_id Device ID
     * @return Performance metrics
     */
    static std::optional<AdapterMetrics> get_metrics(
        const std::string& adapter_name,
        int device_id
    );

    /**
     * @brief Get all metrics
     *
     * @return All recorded metrics
     */
    static std::vector<AdapterMetrics> get_all_metrics();

    /**
     * @brief Clear all metrics
     */
    static void clear_metrics();

    /**
     * @brief Generate performance report
     */
    static std::string generate_performance_report();

private:
    static std::map<std::pair<std::string, int>, AdapterMetrics> metrics_;
    static std::mutex metrics_mutex_;

    /**
     * @brief Update rolling averages
     */
    static void update_averages(AdapterMetrics& metrics);
};

/**
 * @brief Configuration utilities for adapters
 */
namespace adapter_utils {

    /**
     * @brief Convert kernel name to configuration key
     */
    std::string kernel_to_config_key(const std::string& kernel_name);

    /**
     * @brief Validate adapter compatibility
     */
    bool validate_adapter_compatibility(
        const IConfigurationAdapter& adapter,
        int device_id
    );

    /**
     * @brief Get optimal batch size for adapter
     */
    uint64_t get_optimal_batch_size(
        const IConfigurationAdapter& adapter,
        const std::string& kernel_name
    );

    /**
     * @brief Generate adapter configuration report
     */
    std::string generate_adapter_report(
        const IConfigurationAdapter& adapter,
        int device_id
    );

    /**
     * @brief Check adapter health
     */
    bool check_adapter_health(
        const IConfigurationAdapter& adapter,
        int device_id
    );
}

} // namespace adapters
} // namespace keyhunt
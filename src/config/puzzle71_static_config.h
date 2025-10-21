/**
 * @file puzzle71_static_config.h
 * @brief Static YAML configuration system for Puzzle71 technical debt repair
 *
 * This header defines the static configuration system that eliminates
 * runtime device queries and provides deterministic configuration loading
 * with SHA-256 protected validation.
 *
 * Requirements Addressed:
 * - Constitutional Principle 6: Static Configuration
 * - FR-007: Initialize YAML configuration system for static configuration
 * - T007: Initialize YAML configuration system for static configuration
 *
 * @author Puzzle71 Technical Debt Repair System
 * @version 5.5
 * @date 2025-10-20
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>
#include <chrono>
#include <yaml-cpp/yaml.h>
#include <sha256.h>

namespace puzzle71 {
namespace config {

/**
 * @brief GPU device static configuration
 *
 * Contains all device-specific parameters that would normally
 * be queried at runtime, now provided via static configuration.
 */
struct GPUDeviceConfig {
    int device_id;                              ///< Device identifier
    std::string device_name;                    ///< Device name (e.g., "RTX 2080 Ti")
    int compute_capability_major;               ///< Compute capability major version
    int compute_capability_minor;               ///< Compute capability minor version
    uint64_t total_memory_bytes;                ///< Total global memory in bytes
    uint32_t max_threads_per_block;             ///< Maximum threads per block
    uint32_t max_blocks_per_sm;                 ///< Maximum blocks per SM
    uint32_t warp_size;                         ///< Warp size
    uint32_t max_shared_memory_per_block;       ///< Maximum shared memory per block
    uint32_t max_registers_per_block;           ///< Maximum registers per block
    uint32_t multiprocessor_count;              ///< Number of multiprocessors
    uint32_t max_threads_per_multiprocessor;    ///< Maximum threads per SM
    double memory_bandwidth_gbps;               ///< Memory bandwidth in GB/s
    double peak_compute_gflops;                 ///< Peak compute performance in GFLOPS
    std::string architecture_family;            ///< Architecture family (Turing, Ampere, etc.)

    /**
     * @brief Serialize configuration to YAML node
     * @return YAML node containing configuration
     */
    YAML::Node to_yaml() const;

    /**
     * @brief Load configuration from YAML node
     * @param node YAML node containing configuration
     * @return True if loading successful
     */
    bool from_yaml(const YAML::Node& node);

    /**
     * @brief Validate configuration consistency
     * @return True if configuration is valid
     */
    bool validate() const;
};

/**
 * @brief Kernel launch static configuration
 *
 * Pre-determined optimal launch parameters for different
 * GPU architectures and kernel types.
 */
struct KernelLaunchConfig {
    std::string kernel_name;                    ///< Kernel identifier
    std::string architecture_family;            ///< Target architecture family
    uint32_t block_size_x;                      ///< Block size X dimension
    uint32_t block_size_y;                      ///< Block size Y dimension
    uint32_t block_size_z;                      ///< Block size Z dimension
    uint32_t min_grid_size;                     ///< Minimum grid size
    uint32_t max_grid_size;                     ///< Maximum grid size
    uint32_t shared_memory_size_bytes;          ///< Shared memory allocation size
    uint32_t registers_per_thread;              ///< Registers per thread
    double expected_occupancy;                  ///< Expected occupancy ratio
    std::map<std::string, uint32_t> launch_bounds; ///< Additional launch bounds

    /**
     * @brief Serialize configuration to YAML node
     * @return YAML node containing configuration
     */
    YAML::Node to_yaml() const;

    /**
     * @brief Load configuration from YAML node
     * @param node YAML node containing configuration
     * @return True if loading successful
     */
    bool from_yaml(const YAML::Node& node);

    /**
     * @brief Validate launch parameters
     * @return True if parameters are valid
     */
    bool validate() const;
};

/**
 * @brief Performance optimization static configuration
 *
 * Pre-configured optimization parameters for different
 * performance targets and scenarios.
 */
struct PerformanceConfig {
    bool enable_shared_memory_optimization;     ///< Enable shared memory caching
    bool enable_warp_level_optimization;       ///< Enable warp-level primitives
    bool enable_memory_coalescing;             ///< Enable memory coalescing optimization
    bool enable_register_optimization;         ///< Enable register optimization
    uint32_t memory_alignment_bytes;            ///< Memory alignment requirement
    uint32_t shared_memory_bank_size;          ///< Shared memory bank size
    double target_gpu_utilization_percent;     ///< Target GPU utilization
    double target_memory_efficiency_percent;   ///< Target memory efficiency
    uint32_t max_concurrent_kernels;           ///< Maximum concurrent kernels
    std::map<std::string, double> optimization_weights; ///< Optimization parameter weights

    /**
     * @brief Serialize configuration to YAML node
     * @return YAML node containing configuration
     */
    YAML::Node to_yaml() const;

    /**
     * @brief Load configuration from YAML node
     * @param node YAML node containing configuration
     * @return True if loading successful
     */
    bool from_yaml(const YAML::Node& node);

    /**
     * @brief Validate performance parameters
     * @return True if parameters are valid
     */
    bool validate() const;
};

/**
 * @brief Validation and compliance static configuration
 *
 * Configuration for validation systems and constitutional
 * compliance requirements.
 */
struct ValidationConfig {
    bool enable_deterministic_validation;       ///< Enable deterministic replay validation
    bool enable_cpu_gpu_validation;             ///< Enable CPU-GPU parity validation
    bool enable_performance_regression_detection; ///< Enable performance regression detection
    bool enable_constitutional_compliance;      ///< Enable constitutional compliance checking
    double validation_precision_tolerance;      ///< Validation precision tolerance
    uint32_t min_validation_iterations;         ///< Minimum validation iterations
    uint32_t max_validation_runtime_seconds;    ///< Maximum validation runtime
    std::string baseline_storage_directory;     ///< Baseline storage directory
    std::vector<std::string> enabled_validators; ///< List of enabled validators

    /**
     * @brief Serialize configuration to YAML node
     * @return YAML node containing configuration
     */
    YAML::Node to_yaml() const;

    /**
     * @brief Load configuration from YAML node
     * @param node YAML node containing configuration
     * @return True if loading successful
     */
    bool from_yaml(const YAML::Node& node);

    /**
     * @brief Validate validation configuration
     * @return True if configuration is valid
     */
    bool validate() const;
};

/**
 * @brief Complete static configuration for Puzzle71
 *
 * Consolidates all static configuration sections into a single
 * configuration structure with SHA-256 protected integrity validation.
 */
struct Puzzle71StaticConfig {
    std::string config_version;                 ///< Configuration version
    std::string config_schema_version;          ///< Schema version
    std::chrono::system_clock::time_point creation_timestamp; ///< Creation timestamp
    std::string git_commit_hash;                ///< Git commit hash for reproducibility
    std::string config_checksum;                ///< SHA-256 checksum of configuration

    std::vector<GPUDeviceConfig> gpu_devices;   ///< Available GPU device configurations
    std::vector<KernelLaunchConfig> kernel_configs; ///< Kernel launch configurations
    PerformanceConfig performance_config;       ///< Performance optimization configuration
    ValidationConfig validation_config;         ///< Validation system configuration

    std::map<std::string, std::string> metadata; ///< Additional metadata

    /**
     * @brief Serialize complete configuration to YAML string
     * @return YAML configuration string
     */
    std::string to_yaml_string() const;

    /**
     * @brief Load configuration from YAML string
     * @param yaml_string YAML configuration string
     * @return True if loading successful
     */
    bool from_yaml_string(const std::string& yaml_string);

    /**
     * @brief Compute SHA-256 checksum of configuration
     * @return SHA-256 checksum as hex string
     */
    std::string compute_checksum() const;

    /**
     * @brief Validate configuration integrity and consistency
     * @return True if configuration is valid and consistent
     */
    bool validate() const;

    /**
     * @brief Get device configuration by device ID
     * @param device_id Device identifier
     * @return Pointer to device configuration or nullptr
     */
    const GPUDeviceConfig* get_device_config(int device_id) const;

    /**
     * @brief Get kernel configuration for specific kernel and architecture
     * @param kernel_name Kernel name
     * @param architecture_family Architecture family
     * @return Pointer to kernel configuration or nullptr
     */
    const KernelLaunchConfig* get_kernel_config(const std::string& kernel_name,
                                               const std::string& architecture_family) const;
};

/**
 * @brief Static configuration loader and manager
 *
 * Provides deterministic loading of static YAML configuration
 * files with integrity validation and caching.
 */
class StaticConfigLoader {
public:
    /**
     * @brief Constructor
     * @param config_directory Base directory for configuration files
     */
    explicit StaticConfigLoader(const std::string& config_directory);

    /**
     * @brief Destructor
     */
    ~StaticConfigLoader();

    /**
     * @brief Load static configuration from file
     *
     * @param config_file Path to configuration file
     * @param config Output configuration structure
     * @return True if loading successful
     */
    bool load_config(const std::string& config_file, Puzzle71StaticConfig& config);

    /**
     * @brief Save configuration to file
     *
     * @param config_file Path to output file
     * @param config Configuration to save
     * @return True if saving successful
     */
    bool save_config(const std::string& config_file, const Puzzle71StaticConfig& config);

    /**
     * @brief Validate configuration file integrity
     *
     * @param config_file Path to configuration file
     * @return True if file is valid and uncorrupted
     */
    bool validate_config_file(const std::string& config_file) const;

    /**
     * @brief Get cached configuration (if available)
     *
     * @param config_file Configuration file path
     * @return Pointer to cached configuration or nullptr
     */
    const Puzzle71StaticConfig* get_cached_config(const std::string& config_file) const;

    /**
     * @brief Clear configuration cache
     */
    void clear_cache();

    /**
     * @brief Generate default configuration for specified GPU
     *
     * @param device_id GPU device ID
     * @param device_name GPU device name
     * @param compute_major Compute capability major
     * @param compute_minor Compute capability minor
     * @param memory_bytes Total memory in bytes
     * @return Generated configuration
     */
    Puzzle71StaticConfig generate_default_config(int device_id,
                                                const std::string& device_name,
                                                int compute_major,
                                                int compute_minor,
                                                uint64_t memory_bytes) const;

private:
    struct Impl;  ///< Forward declaration for implementation
    std::unique_ptr<Impl> pimpl_;  ///< Pimpl idiom for encapsulation

    /**
     * @brief Get full path to configuration file
     * @param config_file Relative configuration file path
     * @return Absolute file path
     */
    std::string get_config_path(const std::string& config_file) const;

    /**
     * @brief Read configuration file content
     * @param file_path Path to configuration file
     * @return File content as string
     */
    std::string read_config_file(const std::string& file_path) const;

    /**
     * @brief Write content to configuration file
     * @param file_path Path to output file
     * @param content Content to write
     * @return True if write successful
     */
    bool write_config_file(const std::string& file_path,
                          const std::string& content) const;

    /**
     * @brief Compute SHA-256 of file content
     * @param content File content
     * @return SHA-256 hash as hex string
     */
    std::string compute_file_checksum(const std::string& content) const;
};

/**
 * @brief RAII helper for configuration management
 *
 * Automatically loads and validates configuration within a scope
 * for convenient resource management.
 */
class ConfigGuard {
public:
    /**
     * @brief Constructor - loads configuration
     * @param loader Configuration loader reference
     * @param config_file Configuration file path
     */
    ConfigGuard(StaticConfigLoader& loader, const std::string& config_file);

    /**
     * @brief Destructor - validates and cleans up
     */
    ~ConfigGuard();

    /**
     * @brief Get loaded configuration
     * @return Reference to loaded configuration
     */
    const Puzzle71StaticConfig& config() const { return config_; }

    /**
     * @brief Check if configuration is valid
     * @return True if configuration is valid and loaded
     */
    bool is_valid() const { return is_valid_; }

private:
    StaticConfigLoader& loader_;  ///< Configuration loader reference
    Puzzle71StaticConfig config_;  ///< Loaded configuration
    bool is_valid_;               ///< Configuration validity flag
};

} // namespace config
} // namespace puzzle71
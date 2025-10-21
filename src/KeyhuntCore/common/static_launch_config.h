// Puzzle71 Technical Debt Repair - Enhanced Static Launch Configuration System
// Addresses P0/blocking and P1/high priority issues from v5.5 technical debt audit
// Implements T026: Static launch configuration with no runtime device queries
// Enhanced with YAML support, comprehensive cache optimization, and advanced validation

#pragma once

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <memory>

namespace keyhunt {
namespace config {

/**
 * @brief Static kernel launch configuration
 *
 * Replaces dynamic device property calculation with static configuration
 * to satisfy v5.5 hard constraints against dynamic grid/block.
 *
 * This addresses the P0 issue in puzzle71_kernel.cu:200 where runtime
 * device property calculation violates static configuration requirements.
 */
struct StaticKernelConfig {
    // Grid configuration (static, compile-time or config-file based)
    uint32_t grid_size_x;
    uint32_t grid_size_y;
    uint32_t grid_size_z;

    // Block configuration (static, architecture-specific)
    uint32_t block_size_x;
    uint32_t block_size_y;
    uint32_t block_size_z;

    // Performance parameters
    uint32_t points_per_thread;
    uint32_t shared_memory_size;

    // Validation flags
    bool enforce_static_config;
    bool disable_runtime_override;
};

/**
 * @brief Architecture-specific static configurations
 *
 * Pre-computed optimal configurations for each GPU architecture
 * to eliminate runtime device property queries.
 */

// Turing (sm_75) - RTX 2080 Ti, Titan RTX
constexpr StaticKernelConfig TURING_CONFIG = {
    .grid_size_x = 640,     // 20 blocks/SM * 28 SMs
    .grid_size_y = 1,
    .grid_size_z = 1,
    .block_size_x = 256,    // 256 threads/block
    .block_size_y = 1,
    .block_size_z = 1,
    .points_per_thread = 256,
    .shared_memory_size = 8192,  // 8KB per block
    .enforce_static_config = true,
    .disable_runtime_override = true
};

// Ampere (sm_80-89) - A100, RTX 3090, RTX 4090
constexpr StaticKernelConfig AMPERE_CONFIG = {
    .grid_size_x = 960,     // 12 blocks/SM * 80 SMs (A100)
    .grid_size_y = 1,
    .grid_size_z = 1,
    .block_size_x = 256,    // 256 threads/block
    .block_size_y = 1,
    .block_size_z = 1,
    .points_per_thread = 512,
    .shared_memory_size = 8192,
    .enforce_static_config = true,
    .disable_runtime_override = true
};

// Hopper (sm_90) - H20, H100
constexpr StaticKernelConfig HOPPER_CONFIG = {
    .grid_size_x = 1248,    // 16 blocks/SM * 78 SMs (H20)
    .grid_size_y = 1,
    .grid_size_z = 1,
    .block_size_x = 256,    // 256 threads/block
    .block_size_y = 1,
    .block_size_z = 1,
    .points_per_thread = 1024,
    .shared_memory_size = 8192,
    .enforce_static_config = true,
    .disable_runtime_override = true
};

// Conservative fallback configuration
constexpr StaticKernelConfig CONSERVATIVE_CONFIG = {
    .grid_size_x = 256,
    .grid_size_y = 1,
    .grid_size_z = 1,
    .block_size_x = 256,
    .block_size_y = 1,
    .block_size_z = 1,
    .points_per_thread = 128,
    .shared_memory_size = 4096,
    .enforce_static_config = true,
    .disable_runtime_override = true
};

/**
 * @brief Get static configuration for current device
 *
 * This function replaces the dynamic device property query
 * with a static configuration lookup table.
 *
 * @param device_id CUDA device ID
 * @return Static configuration for the device
 */
inline StaticKernelConfig getStaticConfig(int device_id) {
    // Get device properties only for architecture detection
    cudaDeviceProp props{};
    if (cudaGetDeviceProperties(&props, device_id) != cudaSuccess) {
        // Fallback to conservative config if device query fails
        return CONSERVATIVE_CONFIG;
    }

    // Select configuration based on compute capability
    int compute_cap = props.major * 10 + props.minor;

    if (compute_cap >= 90) {
        return HOPPER_CONFIG;
    } else if (compute_cap >= 80) {
        return AMPERE_CONFIG;
    } else if (compute_cap >= 75) {
        return TURING_CONFIG;
    } else {
        return CONSERVATIVE_CONFIG;
    }
}

/**
 * @brief Validate static configuration
 *
 * Ensures configuration meets v5.5 static requirements
 *
 * @param config Configuration to validate
 * @return true if valid, false otherwise
 */
inline bool validateStaticConfig(const StaticKernelConfig& config) {
    // Validate grid size (must be > 0 and within limits)
    if (config.grid_size_x == 0 || config.grid_size_x > 2147483647) return false;
    if (config.grid_size_y == 0 || config.grid_size_y > 65535) return false;
    if (config.grid_size_z == 0 || config.grid_size_z > 65535) return false;

    // Validate block size (must be power of 2, within device limits)
    if (config.block_size_x == 0 || config.block_size_x > 1024) return false;
    if (config.block_size_y > 1024 || config.block_size_z > 1024) return false;

    // Check if block size is power of 2 (for warp alignment)
    if ((config.block_size_x & (config.block_size_x - 1)) != 0) return false;

    // Validate points per thread
    if (config.points_per_thread == 0 || config.points_per_thread > 2048) return false;

    // Validate shared memory size
    if (config.shared_memory_size > 48 * 1024) return false; // 48KB per SM limit

    // Validate enforcement flags
    if (!config.enforce_static_config) return false;
    if (!config.disable_runtime_override) return false;

    return true;
}

/**
 * @brief Convert static config to CUDA dim3
 */
inline dim3 toGridDim(const StaticKernelConfig& config) {
    return dim3(config.grid_size_x, config.grid_size_y, config.grid_size_z);
}

inline dim3 toBlockDim(const StaticKernelConfig& config) {
    return dim3(config.block_size_x, config.block_size_y, config.block_size_z);
}

/**
 * @brief Calculate total keys per batch from static config
 */
inline uint64_t calculateTotalKeys(const StaticKernelConfig& config) {
    return static_cast<uint64_t>(config.grid_size_x) *
           static_cast<uint64_t>(config.grid_size_y) *
           static_cast<uint64_t>(config.grid_size_z) *
           static_cast<uint64_t>(config.block_size_x) *
           static_cast<uint64_t>(config.block_size_y) *
           static_cast<uint64_t>(config.block_size_z) *
           static_cast<uint64_t>(config.points_per_thread);
}

// Comprehensive Static Configuration System (T026)

/**
 * @brief Static GPU architecture identification
 *
 * This enumeration provides compile-time GPU architecture identification
 * to eliminate runtime device queries as required by constitutional v5.5 constraints.
 */
enum class GPUArchitecture {
    UNKNOWN = 0,
    TURING = 75,      // RTX 20xx series
    AMPERE = 86,     // RTX 30xx series
    ADA_LOVELACE = 89, // RTX 40xx series
    HOPPER = 90      // H100, H20 series
};

/**
 * @brief Comprehensive static kernel launch configuration
 *
 * Provides pre-computed launch parameters for each GPU architecture
 * to ensure deterministic behavior and constitutional compliance.
 */
struct StaticLaunchConfig {
    // Kernel launch parameters
    dim3 block_dim;
    dim3 grid_dim;
    size_t shared_memory_size;

    // Performance parameters
    int registers_per_thread;
    int max_blocks_per_sm;
    int max_threads_per_sm;

    // Memory optimization parameters
    size_t memory_alignment;
    bool enable_shared_memory;
    bool enable_coalesced_access;

    // Constitutional compliance flags (v5.5)
    bool static_configuration_only;
    bool no_runtime_device_queries;
    bool deterministic_launch;

    // Performance targets (constitutional requirements)
    double target_occupancy_percent;        // Target: >50%
    double target_memory_efficiency_percent; // Target: >90%
    double target_gpu_utilization_percent;   // Target: >70%
};

/**
 * @brief Static configuration for ECC operations
 *
 * Provides pre-computed parameters for ECC batch operations
 * optimized for each GPU architecture.
 */
struct ECCStaticConfig {
    size_t batch_size;
    size_t memory_pool_size;
    int threads_per_block;
    int points_per_thread;
    bool use_shared_memory;
    bool use_montgomery_arithmetic;
    double precision_target;                 // Must be <1e-10
    bool enable_deterministic_replay;
};

/**
 * @brief Enhanced cache configuration for GPU memory hierarchy
 *
 * Provides detailed cache optimization settings for each GPU architecture
 * to maximize memory bandwidth utilization and minimize latency.
 */
struct CacheConfig {
    // L1 Cache configuration
    bool enable_l1_cache;
    size_t l1_cache_size_bytes;
    uint32_t l1_cache_line_size;
    bool prefer_l1_over_shared_memory;

    // L2 Cache configuration
    bool enable_l2_cache;
    size_t l2_cache_size_bytes;
    uint32_t l2_cache_line_size;
    bool enable_l2_residency;

    // Shared memory configuration
    bool enable_shared_memory;
    size_t shared_memory_size_bytes;
    uint32_t shared_memory_bank_size;
    bool enable_shared_memory_l1;

    // Texture cache configuration (for read-only data)
    bool enable_texture_cache;
    bool enable_texture_read_caching;
    uint32_t texture_cache_hit_optimization;

    // Constant memory configuration
    bool enable_constant_memory;
    size_t constant_memory_size_bytes;
    bool enable_constant_cache;

    // Memory bandwidth optimization
    bool enable_memory_coalescing;
    uint32_t memory_alignment_bytes;
    bool enable_vectorized_loads;
    bool enable_prefetch;

    // Cache eviction policy
    std::string cache_policy;  // "LRU", "FIFO", "RANDOM"
    bool enable_cache_bypass;

    // Performance tuning
    double target_cache_hit_rate_percent;
    double target_memory_bandwidth_utilization_percent;
};

/**
 * @brief Memory bandwidth optimization configuration
 *
 * Provides detailed settings for optimizing memory bandwidth utilization
 * across different GPU architectures and memory access patterns.
 */
struct MemoryBandwidthConfig {
    // Memory access patterns
    bool enable_sequential_access;
    bool enable_strided_access;
    uint32_t stride_size_bytes;
    bool enable_random_access_optimization;

    // Vectorized memory operations
    bool enable_vectorized_loads;
    bool enable_vectorized_stores;
    uint32_t vector_size_bytes;  // 4, 8, 16 bytes

    // Memory throughput optimization
    bool enable_memory_throughput_optimization;
    double target_memory_throughput_gbps;
    uint32_t max_concurrent_memory_requests;

    // Memory controller optimization
    bool enable_memory_controller_optimization;
    uint32_t memory_channels_utilization;
    bool enable_memory_interleaving;

    // Bank conflict resolution
    bool enable_bank_conflict_resolution;
    uint32_t shared_memory_bank_size_bytes;
    bool enable_padding_optimization;

    // Prefetching and caching
    bool enable_prefetching;
    uint32_t prefetch_distance;
    bool enable_cache_line_alignment;

    // Bandwidth monitoring
    bool enable_bandwidth_monitoring;
    double min_bandwidth_threshold_gbps;
    double max_bandwidth_threshold_gbps;
};

/**
 * @brief Advanced kernel launch configuration with comprehensive optimization
 *
 * Extends the basic StaticLaunchConfig with advanced optimization parameters,
 * cache configuration, and memory bandwidth optimization settings.
 */
struct AdvancedLaunchConfig {
    // Basic launch parameters (inherited from StaticLaunchConfig)
    dim3 block_dim;
    dim3 grid_dim;
    size_t shared_memory_size;

    // Performance parameters
    int registers_per_thread;
    int max_blocks_per_sm;
    int max_threads_per_sm;

    // Advanced optimization parameters
    CacheConfig cache_config;
    MemoryBandwidthConfig memory_config;

    // Synchronization optimization
    bool enable_warp_level_sync;
    bool enable_block_level_sync;
    bool enable_grid_level_sync;
    uint32_t sync_frequency;

    // Thread divergence optimization
    bool enable_divergence_optimization;
    uint32_t warp_size_optimization;
    bool enable_branch_prediction;

    // Power and thermal optimization
    bool enable_power_optimization;
    double max_power_watts;
    double target_temperature_celsius;
    bool enable_thermal_throttling_prevention;

    // Advanced performance targets
    double target_occupancy_percent;
    double target_memory_efficiency_percent;
    double target_compute_efficiency_percent;
    double target_power_efficiency_percent;

    // Kernel-specific optimizations
    bool enable_kernel_fusion;
    bool enable_kernel_pipelining;
    uint32_t pipeline_depth;

    // Error handling and recovery
    bool enable_error_detection;
    bool enable_automatic_recovery;
    uint32_t max_retry_attempts;

    // Constitutional compliance flags (v5.5)
    bool static_configuration_only;
    bool no_runtime_device_queries;
    bool deterministic_launch;
    bool reproducible_results;
};

/**
 * @brief YAML configuration loader and validator
 *
 * Handles loading and validation of static launch configurations from YAML files,
 * providing comprehensive error handling and fallback mechanisms.
 */
class YAMLConfigLoader {
public:
    /**
     * @brief Load launch configuration from YAML file
     *
     * @param config_file Path to YAML configuration file
     * @param architecture Target GPU architecture
     * @return AdvancedLaunchConfig Loaded configuration
     * @throws std::runtime_error If loading fails
     */
    static AdvancedLaunchConfig load_launch_config(
        const std::string& config_file,
        GPUArchitecture architecture
    );

    /**
     * @brief Load cache configuration from YAML
     *
     * @param config_file Path to YAML configuration file
     * @param architecture Target GPU architecture
     * @return CacheConfig Cache configuration
     * @throws std::runtime_error If loading fails
     */
    static CacheConfig load_cache_config(
        const std::string& config_file,
        GPUArchitecture architecture
    );

    /**
     * @brief Load memory bandwidth configuration from YAML
     *
     * @param config_file Path to YAML configuration file
     * @param architecture Target GPU architecture
     * @return MemoryBandwidthConfig Memory configuration
     * @throws std::runtime_error If loading fails
     */
    static MemoryBandwidthConfig load_memory_config(
        const std::string& config_file,
        GPUArchitecture architecture
    );

    /**
     * @brief Validate YAML configuration file
     *
     * @param config_file Path to YAML configuration file
     * @return True if valid, false otherwise
     */
    static bool validate_config_file(const std::string& config_file);

    /**
     * @brief Get configuration schema version
     *
     * @param config_file Path to YAML configuration file
     * @return Schema version string
     */
    static std::string get_schema_version(const std::string& config_file);

    /**
     * @brief Check configuration compatibility
     *
     * @param config_file Path to YAML configuration file
     * @param architecture Target GPU architecture
     * @return True if compatible, false otherwise
     */
    static bool check_compatibility(
        const std::string& config_file,
        GPUArchitecture architecture
    );

private:
    // Helper methods for YAML parsing
    static bool parse_bool(const std::string& value, bool default_value);
    static int parse_int(const std::string& value, int default_value);
    static double parse_double(const std::string& value, double default_value);
    static size_t parse_size(const std::string& value, size_t default_value);
    static dim3 parse_dim3(const std::map<std::string, std::string>& values);
    static GPUArchitecture parse_architecture(const std::string& arch_str);
};

/**
 * @brief Enhanced static launch configuration manager
 *
 * This class provides comprehensive static configuration for all GPU operations,
 * eliminating runtime device queries, ensuring constitutional compliance,
 * and supporting YAML configuration loading with advanced optimization features.
 */
class StaticLaunchConfigManager {
public:
    /**
     * @brief Get static launch configuration for specified architecture
     *
     * @param architecture GPU architecture identifier
     * @return Static launch configuration optimized for the architecture
     */
    static const StaticLaunchConfig& get_launch_config(GPUArchitecture architecture);

    /**
     * @brief Get static ECC configuration for specified architecture
     *
     * @param architecture GPU architecture identifier
     * @return Static ECC configuration optimized for the architecture
     */
    static const ECCStaticConfig& get_ecc_config(GPUArchitecture architecture);

    /**
     * @brief Detect GPU architecture at compile time
     *
     * This function uses compile-time constants to determine the target
     * GPU architecture, avoiding runtime device queries.
     *
     * @return Detected GPU architecture
     */
    static GPUArchitecture detect_architecture();

    /**
     * @brief Validate static configuration compliance
     *
     * @param config Configuration to validate
     * @return True if configuration complies with constitutional constraints
     */
    static bool validate_configuration(const StaticLaunchConfig& config);

    /**
     * @brief Validate ECC configuration compliance
     *
     * @param config ECC configuration to validate
     * @return True if configuration complies with constitutional constraints
     */
    static bool validate_ecc_configuration(const ECCStaticConfig& config);

    /**
     * @brief Get default configuration for current architecture
     *
     * @return Default static launch configuration
     */
    static const StaticLaunchConfig& get_default_config();

    /**
     * @brief Get default ECC configuration for current architecture
     *
     * @return Default ECC static configuration
     */
    static const ECCStaticConfig& get_default_ecc_config();

    /**
     * @brief Check if static configuration is available
     *
     * @return True if static configuration is available for current architecture
     */
    static bool is_static_config_available();

    /**
     * @brief Get configuration summary string
     *
     * @param config Configuration to summarize
     * @return Human-readable configuration summary
     */
    static const char* get_config_summary(const StaticLaunchConfig& config);

    // Enhanced methods for advanced configuration support

    /**
     * @brief Get advanced launch configuration for specified architecture
     *
     * @param architecture GPU architecture identifier
     * @return Advanced launch configuration with cache and memory optimization
     */
    static const AdvancedLaunchConfig& get_advanced_launch_config(GPUArchitecture architecture);

    /**
     * @brief Get advanced launch configuration from YAML file
     *
     * @param config_file Path to YAML configuration file
     * @param architecture Target GPU architecture
     * @return Advanced launch configuration loaded from file
     * @throws std::runtime_error If loading fails
     */
    static AdvancedLaunchConfig load_advanced_config_from_yaml(
        const std::string& config_file,
        GPUArchitecture architecture
    );

    /**
     * @brief Get cache configuration for specified architecture
     *
     * @param architecture GPU architecture identifier
     * @return Cache configuration optimized for the architecture
     */
    static const CacheConfig& get_cache_config(GPUArchitecture architecture);

    /**
     * @brief Get memory bandwidth configuration for specified architecture
     *
     * @param architecture GPU architecture identifier
     * @return Memory bandwidth configuration optimized for the architecture
     */
    static const MemoryBandwidthConfig& get_memory_bandwidth_config(GPUArchitecture architecture);

    /**
     * @brief Validate advanced launch configuration
     *
     * @param config Advanced configuration to validate
     * @return True if configuration is valid and compliant
     */
    static bool validate_advanced_config(const AdvancedLaunchConfig& config);

    /**
     * @brief Validate cache configuration
     *
     * @param config Cache configuration to validate
     * @return True if configuration is valid
     */
    static bool validate_cache_config(const CacheConfig& config);

    /**
     * @brief Validate memory bandwidth configuration
     *
     * @param config Memory bandwidth configuration to validate
     * @return True if configuration is valid
     */
    static bool validate_memory_bandwidth_config(const MemoryBandwidthConfig& config);

    /**
     * @brief Get default advanced configuration for current architecture
     *
     * @return Default advanced launch configuration
     */
    static const AdvancedLaunchConfig& get_default_advanced_config();

    /**
     * @brief Fallback configuration provider with comprehensive error handling
     *
     * Provides safe fallback configurations when primary configuration loading fails.
     *
     * @param architecture Target GPU architecture
     * @param error_code Error code from failed configuration load
     * @return Safe fallback configuration
     */
    static const AdvancedLaunchConfig& get_fallback_config(
        GPUArchitecture architecture,
        int error_code = 0
    );

    /**
     * @brief Configuration performance estimator
     *
     * Estimates performance metrics for a given configuration on target architecture.
     *
     * @param config Configuration to evaluate
     * @param architecture Target GPU architecture
     * @return Estimated performance metrics (keys/s, memory bandwidth, etc.)
     */
    static std::map<std::string, double> estimate_performance(
        const AdvancedLaunchConfig& config,
        GPUArchitecture architecture
    );

    /**
     * @brief Configuration optimization recommender
     *
     * Analyzes current configuration and recommends optimizations.
     *
     * @param current_config Current configuration to analyze
     * @param architecture Target GPU architecture
     * @return Recommended optimizations and improvements
     */
    static std::vector<std::string> recommend_optimizations(
        const AdvancedLaunchConfig& current_config,
        GPUArchitecture architecture
    );

    /**
     * @brief Get configuration compatibility matrix
     *
     * Returns compatibility information for different configurations and architectures.
     *
     * @return Map of configuration compatibility information
     */
    static std::map<std::string, bool> get_compatibility_matrix();

    /**
     * @brief Export current configuration to YAML
     *
     * Exports the current configuration to a YAML file for reproducibility.
     *
     * @param config Configuration to export
     * @param output_file Output YAML file path
     * @return True if export successful
     */
    static bool export_config_to_yaml(
        const AdvancedLaunchConfig& config,
        const std::string& output_file
    );

    /**
     * @brief Compare two configurations
     *
     * Compares two configurations and returns differences.
     *
     * @param config1 First configuration
     * @param config2 Second configuration
     * @return List of differences between configurations
     */
    static std::vector<std::string> compare_configurations(
        const AdvancedLaunchConfig& config1,
        const AdvancedLaunchConfig& config2
    );

private:
    // Static configuration tables for each architecture
    static const StaticLaunchConfig turing_config_;
    static const StaticLaunchConfig ampere_config_;
    static const StaticLaunchConfig ada_lovelace_config_;
    static const StaticLaunchConfig hopper_config_;

    static const ECCStaticConfig turing_ecc_config_;
    static const ECCStaticConfig ampere_ecc_config_;
    static const ECCStaticConfig ada_lovelace_ecc_config_;
    static const ECCStaticConfig hopper_ecc_config_;

    // Advanced configuration tables for each architecture
    static AdvancedLaunchConfig turing_advanced_config_;
    static AdvancedLaunchConfig ampere_advanced_config_;
    static AdvancedLaunchConfig ada_lovelace_advanced_config_;
    static AdvancedLaunchConfig hopper_advanced_config_;

    // Cache configuration tables for each architecture
    static CacheConfig turing_cache_config_;
    static CacheConfig ampere_cache_config_;
    static CacheConfig ada_lovelace_cache_config_;
    static CacheConfig hopper_cache_config_;

    // Memory bandwidth configuration tables for each architecture
    static MemoryBandwidthConfig turing_memory_config_;
    static MemoryBandwidthConfig ampere_memory_config_;
    static MemoryBandwidthConfig ada_lovelace_memory_config_;
    static MemoryBandwidthConfig hopper_memory_config_;

    // Fallback configurations for error handling
    static AdvancedLaunchConfig fallback_config_;
    static CacheConfig fallback_cache_config_;
    static MemoryBandwidthConfig fallback_memory_config_;

    // Helper methods for basic configuration validation
    static bool validate_performance_targets(const StaticLaunchConfig& config);
    static bool validate_memory_constraints(const StaticLaunchConfig& config);
    static bool validate_constitutional_compliance(const StaticLaunchConfig& config);

    // Helper methods for advanced configuration validation
    static bool validate_cache_performance_targets(const CacheConfig& config);
    static bool validate_memory_performance_targets(const MemoryBandwidthConfig& config);
    static bool validate_advanced_constitutional_compliance(const AdvancedLaunchConfig& config);
    static bool validate_power_and_thermal_constraints(const AdvancedLaunchConfig& config);

    // Performance estimation helpers
    static double estimate_memory_bandwidth_utilization(
        const MemoryBandwidthConfig& config,
        GPUArchitecture architecture
    );
    static double estimate_cache_hit_rate(
        const CacheConfig& config,
        GPUArchitecture architecture
    );
    static double estimate_occupancy(
        const AdvancedLaunchConfig& config,
        GPUArchitecture architecture
    );

    // YAML parsing helpers
    static std::map<std::string, std::string> parse_yaml_section(
        const std::string& content,
        const std::string& section_name
    );
    static bool validate_yaml_schema(const std::string& content);
    static std::string calculate_config_checksum(const AdvancedLaunchConfig& config);
};

// Compile-time architecture detection macros
#define CUDA_ARCH_TURING    75
#define CUDA_ARCH_AMPERE    86
#define CUDA_ARCH_ADA       89
#define CUDA_ARCH_HOPPER    90

// Architecture-specific configuration selectors
#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 90
    #define CURRENT_ARCH keyhunt::config::GPUArchitecture::HOPPER
#elif defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 89
    #define CURRENT_ARCH keyhunt::config::GPUArchitecture::ADA_LOVELACE
#elif defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 86
    #define CURRENT_ARCH keyhunt::config::GPUArchitecture::AMPERE
#elif defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 75
    #define CURRENT_ARCH keyhunt::config::GPUArchitecture::TURING
#else
    #define CURRENT_ARCH keyhunt::config::GPUArchitecture::UNKNOWN
#endif

/**
 * @brief Get current static launch configuration
 *
 * Convenience function to get the static launch configuration
 * for the current architecture.
 *
 * @return Static launch configuration for current architecture
 */
inline const StaticLaunchConfig& get_current_static_config() {
    return StaticLaunchConfigManager::get_launch_config(CURRENT_ARCH);
}

/**
 * @brief Get current static ECC configuration
 *
 * Convenience function to get the static ECC configuration
 * for the current architecture.
 *
 * @return Static ECC configuration for current architecture
 */
inline const ECCStaticConfig& get_current_static_ecc_config() {
    return StaticLaunchConfigManager::get_ecc_config(CURRENT_ARCH);
}

/**
 * @brief Validate current static configuration
 *
 * Convenience function to validate the static configuration
 * for the current architecture.
 *
 * @return True if current configuration is valid
 */
inline bool validate_current_static_config() {
    const auto& config = get_current_static_config();
    return StaticLaunchConfigManager::validate_configuration(config);
}

/**
 * @brief Get current advanced launch configuration
 *
 * Convenience function to get the advanced launch configuration
 * for the current architecture.
 *
 * @return Advanced launch configuration for current architecture
 */
inline const AdvancedLaunchConfig& get_current_advanced_config() {
    return StaticLaunchConfigManager::get_advanced_launch_config(CURRENT_ARCH);
}

/**
 * @brief Get current cache configuration
 *
 * Convenience function to get the cache configuration
 * for the current architecture.
 *
 * @return Cache configuration for current architecture
 */
inline const CacheConfig& get_current_cache_config() {
    return StaticLaunchConfigManager::get_cache_config(CURRENT_ARCH);
}

/**
 * @brief Get current memory bandwidth configuration
 *
 * Convenience function to get the memory bandwidth configuration
 * for the current architecture.
 *
 * @return Memory bandwidth configuration for current architecture
 */
inline const MemoryBandwidthConfig& get_current_memory_config() {
    return StaticLaunchConfigManager::get_memory_bandwidth_config(CURRENT_ARCH);
}

/**
 * @brief Load advanced configuration from YAML for current architecture
 *
 * Convenience function to load advanced configuration from YAML file
 * for the current architecture.
 *
 * @param config_file Path to YAML configuration file
 * @return Advanced launch configuration loaded from file
 * @throws std::runtime_error If loading fails
 */
inline AdvancedLaunchConfig load_current_advanced_config_from_yaml(
    const std::string& config_file
) {
    return StaticLaunchConfigManager::load_advanced_config_from_yaml(config_file, CURRENT_ARCH);
}

/**
 * @brief Validate current advanced configuration
 *
 * Convenience function to validate the advanced configuration
 * for the current architecture.
 *
 * @return True if current advanced configuration is valid
 */
inline bool validate_current_advanced_config() {
    const auto& config = get_current_advanced_config();
    return StaticLaunchConfigManager::validate_advanced_config(config);
}

/**
 * @brief Get performance estimate for current configuration
 *
 * Convenience function to get performance estimates for the current
 * architecture and configuration.
 *
 * @return Map of performance metrics (keys/s, bandwidth, etc.)
 */
inline std::map<std::string, double> get_current_performance_estimate() {
    const auto& config = get_current_advanced_config();
    return StaticLaunchConfigManager::estimate_performance(config, CURRENT_ARCH);
}

/**
 * @brief Get optimization recommendations for current configuration
 *
 * Convenience function to get optimization recommendations for the current
 * architecture and configuration.
 *
 * @return Vector of optimization recommendations
 */
inline std::vector<std::string> get_current_optimization_recommendations() {
    const auto& config = get_current_advanced_config();
    return StaticLaunchConfigManager::recommend_optimizations(config, CURRENT_ARCH);
}

// Configuration constants for common architectures
constexpr double MINIMUM_OCCUPANCY_PERCENT = 50.0;
constexpr double MINIMUM_MEMORY_EFFICIENCY_PERCENT = 90.0;
constexpr double MINIMUM_GPU_UTILIZATION_PERCENT = 70.0;
constexpr double MINIMUM_CACHE_HIT_RATE_PERCENT = 85.0;
constexpr double MINIMUM_MEMORY_BANDWIDTH_UTILIZATION_PERCENT = 75.0;

constexpr double TARGET_OCCUPANCY_PERCENT = 75.0;
constexpr double TARGET_MEMORY_EFFICIENCY_PERCENT = 95.0;
constexpr double TARGET_GPU_UTILIZATION_PERCENT = 90.0;
constexpr double TARGET_CACHE_HIT_RATE_PERCENT = 95.0;
constexpr double TARGET_MEMORY_BANDWIDTH_UTILIZATION_PERCENT = 90.0;

constexpr double MAXIMUM_POWER_WATTS = 350.0;
constexpr double MAXIMUM_TEMPERATURE_CELSIUS = 85.0;
constexpr double PRECISION_TOLERANCE = 1e-10;

// Error codes for configuration loading
constexpr int CONFIG_LOAD_SUCCESS = 0;
constexpr int CONFIG_FILE_NOT_FOUND = -1;
constexpr int CONFIG_PARSE_ERROR = -2;
constexpr int CONFIG_VALIDATION_ERROR = -3;
constexpr int CONFIG_INCOMPATIBLE_ARCHITECTURE = -4;
constexpr int CONFIG_MISSING_REQUIRED_FIELDS = -5;

// Legacy compatibility with existing code
namespace launch_config = keyhunt::config;

} // namespace config
} // namespace keyhunt
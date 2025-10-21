// Puzzle71Solver - Memory Configuration Manager Header
// Forces aggressive memory optimization configuration (T012)

#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>

// Add filesystem compatibility
#if defined(__cpp_lib_filesystem) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace keyhunt {
namespace common {

// Memory hierarchy configuration
struct SharedMemoryConfig {
    bool enabled = true;
    bool force_enable = true;
    int bank_size_bytes = 4;
    int max_shared_memory_per_block = 49152;
    int preferred_shared_memory_per_block = 32768;
    int shared_memory_carveout_percent = 75;

    struct BankConflictOptimization {
        bool enabled = true;
        std::string padding_strategy = "coprime_padding";
        bool pad_to_coprime = true;
        std::vector<int> coprime_factors = {17, 19, 23, 29};
    } bank_conflict_optimization;

    struct AccessPatternOptimization {
        bool enabled = true;
        bool coalesced_access = true;
        bool sequential_access = true;
        int vectorized_width = 16;
        int alignment_bytes = 16;
    } access_pattern_optimization;
};

struct GlobalMemoryConfig {
    bool enabled = true;
    bool force_enable = true;

    struct MemoryCoalescing {
        bool enabled = true;
        int transaction_size_bytes = 128;
        int alignment_bytes = 128;
        bool stride_optimization = true;
    } memory_coalescing;

    struct CacheConfiguration {
        std::string L2_cache_preference = "L2";
        bool read_only_cache = true;
        bool texture_cache_utilization = true;
        int prefetch_distance = 2;
    } cache_configuration;

    struct BandwidthOptimization {
        bool enabled = true;
        double target_efficiency_percent = 95.0;
        double memory_bandwidth_utilization_target = 0.9;
    } bandwidth_optimization;
};

struct RegisterOptimizationConfig {
    bool enabled = true;
    bool force_enable = true;

    struct RegisterPressure {
        int target_registers_per_thread = 32;
        int max_registers_per_thread = 40;
        std::string register_allocation_strategy = "optimized";
    } register_pressure;

    struct RegisterUsageReduction {
        bool enabled = true;
        bool variable_reuse = true;
        bool common_subexpression_elimination = true;
        bool dead_code_elimination = true;
    } register_usage_reduction;

    struct OccupancyOptimization {
        double target_occupancy_percent = 80.0;
        double min_occupancy_percent = 50.0;
        int warps_per_sm_target = 16;
    } occupancy_optimization;
};

struct KernelLaunchOptimization {
    bool enabled = true;
    bool force_enable = true;

    struct BlockSizeOptimization {
        bool enabled = true;
        int optimal_block_size = 256;
        std::vector<int> block_size_range = {128, 256, 512, 1024};

        struct AutoTuning {
            bool enabled = true;
            int tuning_iterations = 10;
            std::string performance_metric = "throughput";
        } auto_tuning;
    } block_size_optimization;

    struct GridSizeOptimization {
        bool enabled = true;
        int minimum_blocks_per_sm = 4;
        double occupancy_target = 0.8;
    } grid_size_optimization;

    struct StreamOptimization {
        bool enabled = true;
        int concurrent_streams = 4;
        bool stream_priority = true;
        bool overlap_computation = true;
    } stream_optimization;
};

struct CompilerOptimizationFlags {
    struct NVCCFlags {
        std::string optimization_level = "-O3";
        bool aggressive_optimization = true;
        bool fast_math = true;
        bool ftz = true;
        bool prec_div = false;
        bool prec_sqrt = false;
        bool fmad = true;
        int maxrregcount = 40;
        bool use_fast_math = true;
        bool restrict = true;
        int align = 16;
    } nvcc_flags;

    struct GPUArchitectureFlags {
        std::vector<std::string> target_architectures = {"75", "86", "89", "90"};
        bool architecture_specific_optimizations = true;
    } gpu_architecture_flags;

    struct MemoryOptimizationFlags {
        bool memopt = true;
        std::string dlcm = "cg";
        std::vector<std::string> Xptxas = {"-O3", "--allow-expensive-optimizations=true"};
    } memory_optimization_flags;
};

// GPU-specific optimization parameters
struct GPUOptimizationProfile {
    std::string compute_capability;
    std::string shared_memory_config;
    std::string l1_cache_config;
    int memory_bus_width;
    double memory_bandwidth_gb_per_sec;
    int register_file_size;
};

// Complete memory optimization configuration
struct MemoryOptimizationConfig {
    // Metadata
    std::string version = "1.0.0";
    std::string configuration_type = "memory_optimization";
    bool enabled = true;
    bool force_enabled = true;

    // Memory hierarchy
    SharedMemoryConfig shared_memory;
    GlobalMemoryConfig global_memory;

    // Data structures and registers
    bool structure_of_arrays_enabled = true;
    bool structure_of_arrays_force_enable = true;
    int data_alignment_bytes = 16;
    RegisterOptimizationConfig register_optimization;

    // Memory access patterns
    bool coalesced_access_enabled = true;
    bool coalesced_access_force_enable = true;
    int vector_width = 4;
    bool prefetching_enabled = true;
    int prefetch_distance = 2;

    // Kernel launch optimization
    KernelLaunchOptimization kernel_launch_optimization;

    // Compiler flags
    CompilerOptimizationFlags compiler_flags;

    // GPU-specific profiles
    std::unordered_map<std::string, GPUOptimizationProfile> gpu_profiles;

    // Validation
    bool validation_enabled = true;
    double memory_efficiency_target = 0.95;
    double bandwidth_utilization_target = 0.90;
    double cache_hit_rate_target = 0.85;
};

// Memory configuration manager class
class MemoryConfigManager {
public:
    explicit MemoryConfigManager(const std::string& config_file = "data/config/memory_optimization.json");

    // Configuration loading and validation
    bool load_configuration();
    bool validate_configuration() const;
    bool save_configuration(const std::string& filename) const;

    // Configuration access
    const MemoryOptimizationConfig& get_config() const { return config_; }
    MemoryOptimizationConfig& get_config() { return config_; }

    // GPU-specific configuration
    bool apply_gpu_profile(const std::string& gpu_name);
    GPUOptimizationProfile get_gpu_profile(const std::string& compute_capability) const;

    // Compiler flag generation
    std::vector<std::string> generate_nvcc_flags() const;
    std::vector<std::string> generate_architecture_flags() const;
    std::vector<std::string> generate_memory_optimization_flags() const;

    // Runtime configuration enforcement
    void enforce_shared_memory_configuration() const;
    void enforce_register_configuration() const;
    void enforce_launch_configuration() const;

    // Configuration validation and diagnostics
    bool validate_memory_alignment() const;
    bool validate_register_pressure() const;
    bool validate_block_sizes() const;
    std::vector<std::string> get_configuration_warnings() const;

    // Configuration override support (for testing)
    void override_configuration(const MemoryOptimizationConfig& override_config);
    void reset_to_defaults();

    // Configuration export/import
    std::string export_configuration_json() const;
    bool import_configuration_json(const std::string& json_str);

    // Performance impact estimation
    double estimate_memory_bandwidth_improvement() const;
    double estimate_occupancy_improvement() const;
    double estimate_cache_hit_rate_improvement() const;

private:
    std::string config_file_path_;
    MemoryOptimizationConfig config_;

    // Helper methods
    void initialize_default_configuration();
    void populate_gpu_profiles();
    bool parse_configuration_file(const nlohmann::json& json_data);
    nlohmann::json serialize_configuration() const;

    // Configuration parsing helpers
    void parse_shared_memory_config(const nlohmann::json& json);
    void parse_global_memory_config(const nlohmann::json& json);
    void parse_register_optimization_config(const nlohmann::json& json);
    void parse_kernel_launch_optimization(const nlohmann::json& json);
    void parse_compiler_flags(const nlohmann::json& json);
    void parse_gpu_profiles(const nlohmann::json& json);

    // Validation helpers
    bool validate_shared_memory_config() const;
    bool validate_global_memory_config() const;
    bool validate_register_config() const;
    bool validate_compiler_flags() const;

    // Logging methods
    void log_info(const std::string& message) const;
    void log_warning(const std::string& message) const;
    void log_error(const std::string& message) const;
    void log_debug(const std::string& message) const;

    // Constants
    static const std::string DEFAULT_VERSION;
    static const std::string CONFIGURATION_TYPE;
    static const std::vector<int> VALID_BLOCK_SIZES;
    static const std::vector<std::string> VALID_ARCHITECTURES;
};

// Global configuration manager instance
extern std::unique_ptr<MemoryConfigManager> g_memory_config_manager;

// Convenience functions for global access
bool initialize_memory_configuration(const std::string& config_file = "");
const MemoryOptimizationConfig& get_memory_configuration();
bool apply_memory_optimizations();
std::vector<std::string> get_memory_optimization_compiler_flags();

} // namespace common
} // namespace keyhunt
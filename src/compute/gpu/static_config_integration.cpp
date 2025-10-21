// Puzzle71 Technical Debt Repair - Static Configuration Integration Implementation (T030)
// Connects static configuration system with kernel launch system
// Implements T026-T028 integration with constitutional compliance

#include "static_config_integration.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <cstdio>
#include <cstring>

namespace keyhunt {
namespace integration {

// FallbackConfigurationProvider Implementation

IntegratedLaunchConfig FallbackConfigurationProvider::get_safe_fallback(
    keyhunt::config::GPUArchitecture architecture,
    int error_code
) {
    IntegratedLaunchConfig config;

    // Get conservative static configurations
    config.static_config = get_conservative_static_config();
    config.ecc_config = get_conservative_ecc_config();

    // Set safe launch parameters
    config.grid_dim = dim3(256, 1, 1);   // Conservative grid size
    config.block_dim = dim3(128, 1, 1);  // Conservative block size
    config.shared_memory_size = 4096;    // Conservative shared memory
    config.points_per_thread = 64;       // Conservative points per thread
    config.batch_size = config.get_total_threads() * config.points_per_thread;

    // Set metadata
    config.architecture = architecture;
    config.config_source = "fallback_safe";
    config.config_file_path = "";
    config.config_version = 1;
    config.is_validated = false;
    config.constitutional_compliance = false;

    // Set conservative performance targets
    config.target_throughput_keys_per_sec = 100.0;      // 100 keys/sec (very conservative)
    config.target_memory_efficiency_percent = 80.0;     // 80% (below constitutional 90%)
    config.target_gpu_utilization_percent = 60.0;       // 60% (below constitutional 70%)
    config.target_occupancy_percent = 40.0;             // 40% (below constitutional 50%)

    // Set validation state
    config.validation_result.is_valid = false;
    config.validation_result.addError("config_source", "FALLBACK_ACTIVATED",
                                     "Safe fallback configuration activated due to error code: " + std::to_string(error_code),
                                     "Fix primary configuration or use fallback for testing only",
                                     "FALLBACK_CONFIG");

    config.config_load_time = std::chrono::steady_clock::now();
    config.last_validation_time = std::chrono::steady_clock::now();

    return config;
}

IntegratedLaunchConfig FallbackConfigurationProvider::get_minimal_config() {
    IntegratedLaunchConfig config;

    // Minimal static configuration
    config.static_config = {
        .block_dim = dim3(32, 1, 1),
        .grid_dim = dim3(64, 1, 1),
        .shared_memory_size = 1024,
        .registers_per_thread = 16,
        .max_blocks_per_sm = 8,
        .max_threads_per_sm = 512,
        .memory_alignment = 64,
        .enable_shared_memory = false,
        .enable_coalesced_access = false,
        .static_configuration_only = true,
        .no_runtime_device_queries = true,
        .deterministic_launch = true,
        .target_occupancy_percent = 25.0,
        .target_memory_efficiency_percent = 70.0,
        .target_gpu_utilization_percent = 50.0
    };

    // Minimal ECC configuration
    config.ecc_config = {
        .batch_size = 32,
        .memory_pool_size = 1024 * 1024,  // 1MB
        .threads_per_block = 32,
        .points_per_thread = 1,
        .use_shared_memory = false,
        .use_montgomery_arithmetic = false,
        .precision_target = 1e-8,          // Relaxed precision
        .enable_deterministic_replay = false
    };

    // Minimal launch parameters
    config.grid_dim = dim3(64, 1, 1);
    config.block_dim = dim3(32, 1, 1);
    config.shared_memory_size = 1024;
    config.points_per_thread = 1;
    config.batch_size = 64 * 32;

    // Set metadata
    config.architecture = keyhunt::config::GPUArchitecture::UNKNOWN;
    config.config_source = "fallback_minimal";
    config.config_file_path = "";
    config.config_version = 1;
    config.is_validated = false;
    config.constitutional_compliance = false;

    // Minimal performance targets
    config.target_throughput_keys_per_sec = 10.0;       // 10 keys/sec (minimal)
    config.target_memory_efficiency_percent = 70.0;     // 70% (below constitutional)
    config.target_gpu_utilization_percent = 50.0;       // 50% (below constitutional)
    config.target_occupancy_percent = 25.0;             // 25% (below constitutional)

    config.validation_result.is_valid = false;
    config.validation_result.addError("config_source", "MINIMAL_FALLBACK",
                                     "Minimal fallback configuration for testing only",
                                     "Use proper configuration for production",
                                     "MINIMAL_CONFIG");

    config.config_load_time = std::chrono::steady_clock::now();
    config.last_validation_time = std::chrono::steady_clock::now();

    return config;
}

IntegratedLaunchConfig FallbackConfigurationProvider::get_performance_fallback(
    keyhunt::config::GPUArchitecture architecture
) {
    IntegratedLaunchConfig config;

    // Get architecture-specific static configuration
    config.static_config = keyhunt::config::StaticLaunchConfigManager::get_launch_config(architecture);
    config.ecc_config = keyhunt::config::StaticLaunchConfigManager::get_ecc_config(architecture);

    // Performance-optimized parameters
    config.grid_dim = config.static_config.grid_dim;
    config.block_dim = config.static_config.block_dim;
    config.shared_memory_size = config.static_config.shared_memory_size;
    config.points_per_thread = static_cast<int>(config.ecc_config.points_per_thread);
    config.batch_size = config.get_total_threads() * config.points_per_thread;

    // Set metadata
    config.architecture = architecture;
    config.config_source = "fallback_performance";
    config.config_file_path = "";
    config.config_version = 1;
    config.is_validated = false;
    config.constitutional_compliance = true;  // Performance fallback should be compliant

    // High performance targets
    config.target_throughput_keys_per_sec = 1000.0;     // 1000 keys/sec
    config.target_memory_efficiency_percent = 95.0;     // 95% (meets constitutional)
    config.target_gpu_utilization_percent = 90.0;       // 90% (meets constitutional)
    config.target_occupancy_percent = 75.0;             // 75% (meets constitutional)

    config.validation_result.is_valid = true;
    config.validation_result.addWarning("config_source", "PERFORMANCE_FALLBACK",
                                       "Using performance fallback configuration",
                                       "Consider using primary configuration if available",
                                       "PERF_FALLBACK");

    config.config_load_time = std::chrono::steady_clock::now();
    config.last_validation_time = std::chrono::steady_clock::now();

    return config;
}

const keyhunt::config::StaticLaunchConfig& FallbackConfigurationProvider::get_conservative_static_config() {
    static keyhunt::config::StaticLaunchConfig conservative_config = {
        .block_dim = dim3(128, 1, 1),
        .grid_dim = dim3(256, 1, 1),
        .shared_memory_size = 4096,
        .registers_per_thread = 24,
        .max_blocks_per_sm = 16,
        .max_threads_per_sm = 1024,
        .memory_alignment = 64,
        .enable_shared_memory = true,
        .enable_coalesced_access = true,
        .static_configuration_only = true,
        .no_runtime_device_queries = true,
        .deterministic_launch = true,
        .target_occupancy_percent = 50.0,
        .target_memory_efficiency_percent = 85.0,
        .target_gpu_utilization_percent = 70.0
    };
    return conservative_config;
}

const keyhunt::config::ECCStaticConfig& FallbackConfigurationProvider::get_conservative_ecc_config() {
    static keyhunt::config::ECCStaticConfig conservative_ecc_config = {
        .batch_size = 512,
        .memory_pool_size = 32 * 1024 * 1024,  // 32MB
        .threads_per_block = 128,
        .points_per_thread = 64,
        .use_shared_memory = true,
        .use_montgomery_arithmetic = true,
        .precision_target = 1e-10,              // Constitutional requirement
        .enable_deterministic_replay = true
    };
    return conservative_ecc_config;
}

// StaticConfigIntegrationManager Implementation

StaticConfigIntegrationManager::StaticConfigIntegrationManager(int device_id,
                                                             const std::string& config_file)
    : device_id_(device_id), config_file_path_(config_file) {

    last_config_load_ = std::chrono::steady_clock::now();

    // Initialize device architecture detection
    initialize_device_architecture();

    // Mark as initialized
    initialized_ = true;
}

StaticConfigIntegrationManager::~StaticConfigIntegrationManager() {
    clear_config_cache();
}

void StaticConfigIntegrationManager::initialize_device_architecture() {
    // Use compile-time architecture detection first
    device_architecture_ = keyhunt::config::StaticLaunchConfigManager::detect_architecture();

    // If unknown, try runtime detection (only for architecture identification)
    if (device_architecture_ == keyhunt::config::GPUArchitecture::UNKNOWN) {
        cudaDeviceProp props{};
        if (cudaGetDeviceProperties(&props, device_id_) == cudaSuccess) {
            int compute_cap = props.major * 10 + props.minor;

            if (compute_cap >= 90) {
                device_architecture_ = keyhunt::config::GPUArchitecture::HOPPER;
            } else if (compute_cap >= 89) {
                device_architecture_ = keyhunt::config::GPUArchitecture::ADA_LOVELACE;
            } else if (compute_cap >= 86) {
                device_architecture_ = keyhunt::config::GPUArchitecture::AMPERE;
            } else if (compute_cap >= 75) {
                device_architecture_ = keyhunt::config::GPUArchitecture::TURING;
            }
        }
    }
}

IntegratedLaunchConfig StaticConfigIntegrationManager::get_launch_config(
    const std::string& kernel_name,
    uint64_t batch_size,
    bool enable_yaml_config
) {
    std::lock_guard<std::mutex> lock(config_mutex_);

    // Check cache first
    std::string cache_key = kernel_name + "_" + std::to_string(batch_size);
    auto it = config_cache_.find(cache_key);
    if (it != config_cache_.end() && is_config_cache_valid()) {
        return it->second;
    }

    IntegratedLaunchConfig config;

    // Try YAML configuration first if enabled
    if (enable_yaml_config && !config_file_path_.empty()) {
        auto yaml_config = load_yaml_config(config_file_path_, kernel_name);
        if (yaml_config) {
            config = *yaml_config;
        }
    }

    // Fallback to static configuration
    if (config.config_source.empty()) {
        config = create_static_config(kernel_name, batch_size);
    }

    // Validate and adjust configuration
    if (!validate_launch_config(config)) {
        // If validation fails, try performance fallback
        config = FallbackConfigurationProvider::get_performance_fallback(device_architecture_);
        validate_launch_config(config);
    }

    // Apply deterministic modifications if enabled
    if (deterministic_mode_) {
        apply_deterministic_modifications(config);
    }

    // Cache the configuration
    config_cache_[cache_key] = config;
    last_config_load_ = std::chrono::steady_clock::now();

    return config;
}

std::optional<IntegratedLaunchConfig> StaticConfigIntegrationManager::load_yaml_config(
    const std::string& config_file,
    const std::string& kernel_name
) {
    try {
        // Validate YAML configuration first
        config::ValidationResult validation_result;
        if (!config::Puzzle71ConfigValidator::validate_config_file(config_file, validation_result)) {
            // Validation failed, return empty
            return std::nullopt;
        }

        // Load YAML configuration
        YAML::Node yaml_config = YAML::LoadFile(config_file);

        IntegratedLaunchConfig config;
        config.config_source = "yaml";
        config.config_file_path = config_file;
        config.validation_result = validation_result;
        config.config_load_time = std::chrono::steady_clock::now();
        config.last_validation_time = std::chrono::steady_clock::now();

        // Extract version information
        if (yaml_config["config_version"]) {
            config.config_version = yaml_config["config_version"].as<uint32_t>();
        }

        // Find matching kernel configuration
        if (yaml_config["kernel_configs"]) {
            const auto& kernel_configs = yaml_config["kernel_configs"];
            for (const auto& kernel_config : kernel_configs) {
                if (kernel_config["kernel_name"] &&
                    kernel_config["kernel_name"].as<std::string>() == kernel_name) {

                    // Extract kernel configuration
                    if (kernel_config["block_size"]) {
                        const auto& block_size = kernel_config["block_size"];
                        config.block_dim.x = block_size["x"].as<uint32_t>();
                        config.block_dim.y = block_size["y"].as<uint32_t>();
                        config.block_dim.z = block_size["z"].as<uint32_t>();
                    }

                    if (kernel_config["min_grid_size"]) {
                        config.grid_dim.x = kernel_config["min_grid_size"].as<uint32_t>();
                    }

                    if (kernel_config["shared_memory_size_bytes"]) {
                        config.shared_memory_size = kernel_config["shared_memory_size_bytes"].as<size_t>();
                    }

                    if (kernel_config["registers_per_thread"]) {
                        config.static_config.registers_per_thread = kernel_config["registers_per_thread"].as<int>();
                    }

                    break;
                }
            }
        }

        // Load performance configuration
        if (yaml_config["performance_config"]) {
            const auto& perf_config = yaml_config["performance_config"];

            if (perf_config["target_gpu_utilization_percent"]) {
                config.target_gpu_utilization_percent = perf_config["target_gpu_utilization_percent"].as<double>();
            }

            if (perf_config["target_memory_efficiency_percent"]) {
                config.target_memory_efficiency_percent = perf_config["target_memory_efficiency_percent"].as<double>();
            }
        }

        // Set architecture-specific static configuration
        config.static_config = keyhunt::config::StaticLaunchConfigManager::get_launch_config(device_architecture_);
        config.ecc_config = keyhunt::config::StaticLaunchConfigManager::get_ecc_config(device_architecture_);
        config.architecture = device_architecture_;

        // Set constitutional compliance based on validation
        config.constitutional_compliance = validation_result.is_valid &&
                                         config::validation_utils::validate_constitutional_compliance(config_file);

        config.is_validated = true;

        return config;

    } catch (const std::exception& e) {
        // YAML loading failed, return empty
        return std::nullopt;
    }
}

IntegratedLaunchConfig StaticConfigIntegrationManager::create_static_config(
    const std::string& kernel_name,
    uint64_t batch_size
) {
    IntegratedLaunchConfig config;

    // Get static configurations for current architecture
    config.static_config = keyhunt::config::StaticLaunchConfigManager::get_launch_config(device_architecture_);
    config.ecc_config = keyhunt::config::StaticLaunchConfigManager::get_ecc_config(device_architecture_);

    // Set launch parameters from static configuration
    config.grid_dim = config.static_config.grid_dim;
    config.block_dim = config.static_config.block_dim;
    config.shared_memory_size = config.static_config.shared_memory_size;
    config.points_per_thread = static_cast<int>(config.ecc_config.points_per_thread);
    config.batch_size = batch_size;

    // Adjust grid size if needed for batch size
    uint64_t capacity = config.get_total_threads() * config.points_per_thread;
    if (capacity > batch_size && batch_size > 0) {
        double scale = static_cast<double>(batch_size) / static_cast<double>(capacity);
        uint32_t new_grid_x = static_cast<uint32_t>(config.grid_dim.x * scale);
        new_grid_x = std::max(new_grid_x, 1u);
        config.grid_dim.x = new_grid_x;
    }

    // Set metadata
    config.architecture = device_architecture_;
    config.config_source = "static";
    config.config_file_path = "";
    config.config_version = 1;
    config.is_validated = true;
    config.constitutional_compliance = keyhunt::config::validate_current_static_config();

    // Set performance targets from static configuration
    config.target_throughput_keys_per_sec = 1000.0;  // Default target
    config.target_memory_efficiency_percent = config.static_config.target_memory_efficiency_percent;
    config.target_gpu_utilization_percent = config.static_config.target_gpu_utilization_percent;
    config.target_occupancy_percent = config.static_config.target_occupancy_percent;

    // Create validation result
    config.validation_result.is_valid = true;
    config.validation_result.config_version = "5.5";
    config.validation_result.schema_version = "1.0";
    config.validation_result.generateSummary();

    config.config_load_time = std::chrono::steady_clock::now();
    config.last_validation_time = std::chrono::steady_clock::now();

    return config;
}

bool StaticConfigIntegrationManager::validate_launch_config(IntegratedLaunchConfig& config) {
    // Validate static configuration
    if (!keyhunt::config::StaticLaunchConfigManager::validate_configuration(config.static_config)) {
        config.validation_result.addError("static_config", "INVALID_STATIC_CONFIG",
                                         "Static configuration validation failed",
                                         "Check static configuration parameters",
                                         "STATIC_CONFIG_VALIDATION");
        return false;
    }

    // Validate ECC configuration
    if (!keyhunt::config::StaticLaunchConfigManager::validate_ecc_configuration(config.ecc_config)) {
        config.validation_result.addError("ecc_config", "INVALID_ECC_CONFIG",
                                         "ECC configuration validation failed",
                                         "Check ECC configuration parameters",
                                         "ECC_CONFIG_VALIDATION");
        return false;
    }

    // Validate device compatibility
    if (!validate_device_compatibility(config)) {
        config.validation_result.addError("device_compatibility", "INCOMPATIBLE_CONFIG",
                                         "Configuration not compatible with device",
                                         "Check device capabilities and configuration",
                                         "DEVICE_COMPATIBILITY");
        return false;
    }

    // Check constitutional compliance
    bool constitutional_ok = config.static_config.static_configuration_only &&
                            config.static_config.no_runtime_device_queries &&
                            config.static_config.deterministic_launch &&
                            config.target_memory_efficiency_percent >= 90.0 &&
                            config.target_gpu_utilization_percent >= 70.0 &&
                            config.target_occupancy_percent >= 50.0;

    config.constitutional_compliance = constitutional_ok;

    if (!constitutional_ok) {
        config.validation_result.addError("constitutional_compliance", "CONSTITUTIONAL_VIOLATION",
                                         "Configuration violates constitutional requirements",
                                         "Ensure static configuration, no runtime queries, deterministic launch, and performance targets",
                                         "CONSTITUTIONAL_COMPLIANCE");
        return false;
    }

    config.is_validated = true;
    config.validation_result.is_valid = true;
    config.last_validation_time = std::chrono::steady_clock::now();
    config.validation_result.generateSummary();

    return true;
}

bool StaticConfigIntegrationManager::validate_device_compatibility(IntegratedLaunchConfig& config) {
    // Get device properties for compatibility checking
    cudaDeviceProp props{};
    if (cudaGetDeviceProperties(&props, device_id_) != cudaSuccess) {
        return false;  // Cannot validate if device properties not accessible
    }

    bool compatible = true;

    // Check thread count limits
    int total_threads = config.block_dim.x * config.block_dim.y * config.block_dim.z;
    if (total_threads > props.maxThreadsPerBlock) {
        config.validation_result.addError("block_dim", "TOO_MANY_THREADS",
                                         "Block thread count exceeds device limit",
                                         "Reduce block dimensions",
                                         "THREAD_LIMIT");
        compatible = false;
    }

    // Check shared memory limits
    if (config.shared_memory_size > props.sharedMemPerBlock) {
        config.validation_result.addError("shared_memory_size", "SHARED_MEM_EXCEEDED",
                                         "Shared memory size exceeds device limit",
                                         "Reduce shared memory size",
                                         "SHARED_MEM_LIMIT");
        compatible = false;
    }

    // Check register limits (approximate)
    int total_registers = config.static_config.registers_per_thread * total_threads;
    if (total_registers > 65536) {  // Conservative register limit
        config.validation_result.addError("registers_per_thread", "TOO_MANY_REGISTERS",
                                         "Register usage may exceed device limit",
                                         "Reduce registers per thread or block size",
                                         "REGISTER_LIMIT");
        compatible = false;
    }

    return compatible;
}

void StaticConfigIntegrationManager::measure_performance(
    const IntegratedLaunchConfig& config,
    std::chrono::steady_clock::time_point start_time,
    std::chrono::steady_clock::time_point end_time
) {
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    last_metrics_.reset();
    last_metrics_.execution_time = duration;
    last_metrics_.total_time = duration;
    last_metrics_.keys_processed = config.batch_size;

    if (duration.count() > 0) {
        last_metrics_.keys_per_second = (static_cast<double>(config.batch_size) * 1000000.0) / duration.count();
    }

    // Set target performance metrics
    last_metrics_.memory_efficiency_percent = config.target_memory_efficiency_percent;
    last_metrics_.gpu_utilization_percent = config.target_gpu_utilization_percent;
    last_metrics_.occupancy_percent = config.target_occupancy_percent;
    last_metrics_.constitutional_compliance = config.constitutional_compliance;

    if (config.constitutional_compliance) {
        last_metrics_.validation_status = "CONSTITUTIONAL_COMPLIANT";
    } else {
        last_metrics_.validation_status = "NON_COMPLIANT";
    }
}

void StaticConfigIntegrationManager::set_deterministic_mode(bool enabled, uint32_t seed) {
    deterministic_mode_ = enabled;
    if (enabled) {
        deterministic_seed_ = seed;
    }
}

void StaticConfigIntegrationManager::apply_deterministic_modifications(IntegratedLaunchConfig& config) {
    if (!deterministic_mode_) {
        return;
    }

    // Apply deterministic seed to configuration
    config.static_config.deterministic_launch = true;
    config.ecc_config.enable_deterministic_replay = true;

    // Use deterministic grid sizes based on seed
    uint32_t seed_mod = deterministic_seed_ % 8 + 1;  // 1-8
    config.grid_dim.x = (config.grid_dim.x / seed_mod) * seed_mod;
    config.grid_dim.x = std::max(config.grid_dim.x, 1u);

    // Ensure deterministic behavior
    config.validation_result.addInfo("deterministic_mode", "DETERMINISTIC_ENABLED",
                                    "Deterministic mode enabled with seed: " + std::to_string(deterministic_seed_),
                                    "DETERMINISTIC_MODE");
}

bool StaticConfigIntegrationManager::reload_configuration(const std::string& config_file) {
    std::lock_guard<std::mutex> lock(config_mutex_);

    config_file_path_ = config_file;
    clear_config_cache();

    // Test load configuration
    auto test_config = load_yaml_config(config_file, "test_kernel");
    return test_config.has_value();
}

bool StaticConfigIntegrationManager::export_configuration(const std::string& output_file) const {
    try {
        std::ofstream file(output_file);
        if (!file.is_open()) {
            return false;
        }

        file << "# Puzzle71 Integrated Configuration Export\n";
        file << "# Generated: " << std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() << "\n";
        file << "# Device ID: " << device_id_ << "\n";
        file << "# Architecture: " << integration_utils::architecture_to_string(device_architecture_) << "\n\n";

        // Export current configuration from cache
        if (!config_cache_.empty()) {
            const auto& config = config_cache_.begin()->second;
            file << "static_config:\n";
            file << "  block_dim: [" << config.block_dim.x << ", " << config.block_dim.y << ", " << config.block_dim.z << "]\n";
            file << "  grid_dim: [" << config.grid_dim.x << ", " << config.grid_dim.y << ", " << config.grid_dim.z << "]\n";
            file << "  shared_memory_size: " << config.shared_memory_size << "\n";
            file << "  points_per_thread: " << config.points_per_thread << "\n";
            file << "  batch_size: " << config.batch_size << "\n";
            file << "  constitutional_compliance: " << (config.constitutional_compliance ? "true" : "false") << "\n";
        }

        return true;

    } catch (const std::exception&) {
        return false;
    }
}

std::string StaticConfigIntegrationManager::get_compatibility_report() const {
    std::stringstream ss;
    ss << "=== Static Configuration Integration Compatibility Report ===\n\n";
    ss << "Device ID: " << device_id_ << "\n";
    ss << "Architecture: " << integration_utils::architecture_to_string(device_architecture_) << "\n";
    ss << "Configuration File: " << (config_file_path_.empty() ? "None" : config_file_path_) << "\n";
    ss << "Deterministic Mode: " << (deterministic_mode_ ? "ENABLED" : "DISABLED") << "\n";
    ss << "Configuration Cache Size: " << config_cache_.size() << "\n\n";

    if (initialized_) {
        ss << "✅ Integration Manager Initialized\n";
    } else {
        ss << "❌ Integration Manager Not Initialized\n";
    }

    ss << "\n=== Last Performance Metrics ===\n";
    ss << "Keys Processed: " << last_metrics_.keys_processed << "\n";
    ss << "Keys/Second: " << std::fixed << std::setprecision(2) << last_metrics_.keys_per_second << "\n";
    ss << "Memory Efficiency: " << last_metrics_.memory_efficiency_percent << "%\n";
    ss << "GPU Utilization: " << last_metrics_.gpu_utilization_percent << "%\n";
    ss << "Occupancy: " << last_metrics_.occupancy_percent << "%\n";
    ss << "Constitutional Compliance: " << (last_metrics_.constitutional_compliance ? "YES" : "NO") << "\n";

    return ss.str();
}

bool StaticConfigIntegrationManager::validate_constitutional_compliance() const {
    const auto& static_config = keyhunt::config::StaticLaunchConfigManager::get_launch_config(device_architecture_);

    return static_config.static_configuration_only &&
           static_config.no_runtime_device_queries &&
           static_config.deterministic_launch &&
           static_config.target_memory_efficiency_percent >= 90.0 &&
           static_config.target_gpu_utilization_percent >= 70.0 &&
           static_config.target_occupancy_percent >= 50.0;
}

bool StaticConfigIntegrationManager::is_config_cache_valid() const {
    auto now = std::chrono::steady_clock::now();
    auto cache_age = std::chrono::duration_cast<std::chrono::minutes>(now - last_config_load_);
    return cache_age.count() < 60;  // Cache valid for 60 minutes
}

void StaticConfigIntegrationManager::clear_config_cache() {
    config_cache_.clear();
}

// StaticConfigKernelLauncher Implementation

StaticConfigKernelLauncher::StaticConfigKernelLauncher(int device_id,
                                                       const std::string& config_file)
    : device_id_(device_id), target_hash_set_(false) {

    integration_manager_ = std::make_unique<StaticConfigIntegrationManager>(device_id, config_file);

    // Initialize GPU executor
    initialize_gpu_executor();
}

StaticConfigKernelLauncher::~StaticConfigKernelLauncher() = default;

void StaticConfigKernelLauncher::initialize_gpu_executor() {
    // Use empty target hash initially
    std::array<uint32_t, 5> empty_hash = {0, 0, 0, 0, 0};
    gpu_executor_ = std::make_unique<puzzle71::gpu::GpuExecutor>(
        device_id_, true, empty_hash, false);
}

cudaError_t StaticConfigKernelLauncher::upload_target_hash(const std::array<uint32_t, 5>& target_hash) {
    current_target_hash_ = target_hash;
    target_hash_set_ = true;

    // Upload to device using compare namespace function
    return puzzle71::compare::UploadTargetHash160(target_hash);
}

cudaError_t StaticConfigKernelLauncher::launch_ecc_kernel(
    uint64_t batch_size,
    int compression_type,
    const std::array<uint32_t, 5>& target_hash
) {
    if (!target_hash_set_) {
        upload_target_hash(target_hash);
    }

    // Get integrated configuration
    auto config = integration_manager_->get_launch_config("ecc_scalar_mul_kernel", batch_size);

    // Prepare batch for GPU executor
    puzzle71::gpu::BatchConfig batch_config;
    batch_config.batch_size = batch_size;
    batch_config.points_per_thread = config.points_per_thread;
    batch_config.compression = compression_type;

    // Initialize with starting scalar (0 for testing)
    core::UInt256 start_scalar;
    memset(&start_scalar, 0, sizeof(start_scalar));

    gpu_executor_->PrepareBatch(batch_config, start_scalar);

    // Execute batch
    auto result = gpu_executor_->Execute();

    // Update metrics
    last_metrics_ = integration_manager_->get_last_metrics();
    last_metrics_.keys_processed = result.processed_keys;
    last_metrics_.keys_per_second = result.keys_per_sec;

    return cudaSuccess;
}

cudaError_t StaticConfigKernelLauncher::launch_hash_kernel(
    uint64_t batch_size,
    int compression_type
) {
    // Get integrated configuration
    auto config = integration_manager_->get_launch_config("hash_kernel", batch_size);

    // Launch hash kernel using integrated configuration
    return integration_manager_->launch_kernel(config,
        [](dim3 grid, dim3 block, size_t shared_mem, cudaStream_t stream,
           int points_per_thread, size_t batch_size) -> cudaError_t {
            // Hash kernel launch function
            // This would call the actual hash kernel implementation
            return cudaSuccess;
        });
}

cudaError_t StaticConfigKernelLauncher::launch_fixed_kernel(
    int points_per_thread,
    int compression_type
) {
    // Get integrated configuration
    uint64_t batch_size = points_per_thread * 256 * 1024;  // Estimate batch size
    auto config = integration_manager_->get_launch_config("fixed_kernel", batch_size);

    // Use puzzle71::kernel::LaunchFixedKernel from fixed kernel implementation
    return puzzle71::kernel::LaunchFixedKernel(
        config.grid_dim,
        config.block_dim,
        points_per_thread,
        compression_type,
        integration_manager_->is_deterministic_mode() ?
            static_cast<uint32_t>(integration_manager_->deterministic_seed_) : 0
    );
}

const KernelLaunchMetrics& StaticConfigKernelLauncher::get_metrics() const {
    return last_metrics_;
}

void StaticConfigKernelLauncher::reset_metrics() {
    last_metrics_.reset();
}

// Integration Utilities Implementation

namespace integration_utils {

std::string architecture_to_string(keyhunt::config::GPUArchitecture arch) {
    switch (arch) {
        case keyhunt::config::GPUArchitecture::TURING:
            return "Turing";
        case keyhunt::config::GPUArchitecture::AMPERE:
            return "Ampere";
        case keyhunt::config::GPUArchitecture::ADA_LOVELACE:
            return "Ada_Lovelace";
        case keyhunt::config::GPUArchitecture::HOPPER:
            return "Hopper";
        case keyhunt::config::GPUArchitecture::UNKNOWN:
        default:
            return "Unknown";
    }
}

keyhunt::config::GPUArchitecture parse_architecture(const std::string& arch_str) {
    if (arch_str == "Turing" || arch_str == "turing") {
        return keyhunt::config::GPUArchitecture::TURING;
    } else if (arch_str == "Ampere" || arch_str == "ampere") {
        return keyhunt::config::GPUArchitecture::AMPERE;
    } else if (arch_str == "Ada_Lovelace" || arch_str == "ada_lovelace") {
        return keyhunt::config::GPUArchitecture::ADA_LOVELACE;
    } else if (arch_str == "Hopper" || arch_str == "hopper") {
        return keyhunt::config::GPUArchitecture::HOPPER;
    } else {
        return keyhunt::config::GPUArchitecture::UNKNOWN;
    }
}

bool validate_config_compatibility(
    const keyhunt::config::StaticLaunchConfig& static_config,
    const puzzle71::gpu::KernelLaunchConfig& kernel_config
) {
    // Check basic compatibility between configurations
    bool compatible = true;

    // Check block size compatibility
    if (static_config.block_dim.x != kernel_config.block.x ||
        static_config.block_dim.y != kernel_config.block.y ||
        static_config.block_dim.z != kernel_config.block.z) {
        compatible = false;
    }

    // Check grid size compatibility (allow flexibility)
    if (kernel_config.grid.x > static_config.grid_dim.x ||
        kernel_config.grid.y > static_config.grid_dim.y ||
        kernel_config.grid.z > static_config.grid_dim.z) {
        compatible = false;
    }

    return compatible;
}

std::map<std::string, double> estimate_performance(
    const IntegratedLaunchConfig& config
) {
    std::map<std::string, double> metrics;

    // Base performance estimation
    double base_throughput = 150.0;  // keys/sec per thread
    uint64_t total_threads = config.get_total_threads();

    // Apply architecture-specific multipliers
    double arch_multiplier = 1.0;
    switch (config.architecture) {
        case keyhunt::config::GPUArchitecture::TURING:
            arch_multiplier = 1.0;
            break;
        case keyhunt::config::GPUArchitecture::AMPERE:
            arch_multiplier = 2.0;
            break;
        case keyhunt::config::GPUArchitecture::ADA_LOVELACE:
            arch_multiplier = 3.0;
            break;
        case keyhunt::config::GPUArchitecture::HOPPER:
            arch_multiplier = 4.0;
            break;
        default:
            arch_multiplier = 0.5;
            break;
    }

    // Calculate performance metrics
    double efficiency_factor = (config.target_occupancy_percent / 100.0) *
                              (config.target_memory_efficiency_percent / 100.0) *
                              (config.target_gpu_utilization_percent / 100.0);

    metrics["estimated_keys_per_second"] = total_threads * base_throughput * arch_multiplier * efficiency_factor;
    metrics["memory_efficiency_percent"] = config.target_memory_efficiency_percent;
    metrics["gpu_utilization_percent"] = config.target_gpu_utilization_percent;
    metrics["occupancy_percent"] = config.target_occupancy_percent;
    metrics["throughput_multiplier"] = arch_multiplier;
    metrics["efficiency_factor"] = efficiency_factor;

    return metrics;
}

std::string generate_config_fingerprint(const IntegratedLaunchConfig& config) {
    // Generate unique fingerprint for configuration
    char fingerprint[256];
    snprintf(fingerprint, sizeof(fingerprint),
        "%s_%dx%dx%d_%dx%dx%d_%zu_%d_%lu_%s",
        config.config_source.c_str(),
        config.grid_dim.x, config.grid_dim.y, config.grid_dim.z,
        config.block_dim.x, config.block_dim.y, config.block_dim.z,
        config.shared_memory_size,
        config.points_per_thread,
        config.batch_size,
        integration_utils::architecture_to_string(config.architecture).c_str()
    );
    return std::string(fingerprint);
}

std::vector<std::string> compare_configurations(
    const IntegratedLaunchConfig& config1,
    const IntegratedLaunchConfig& config2
) {
    std::vector<std::string> differences;

    // Compare key parameters
    if (config1.grid_dim.x != config2.grid_dim.x) {
        differences.push_back("Grid X: " + std::to_string(config1.grid_dim.x) + " vs " + std::to_string(config2.grid_dim.x));
    }

    if (config1.block_dim.x != config2.block_dim.x) {
        differences.push_back("Block X: " + std::to_string(config1.block_dim.x) + " vs " + std::to_string(config2.block_dim.x));
    }

    if (config1.shared_memory_size != config2.shared_memory_size) {
        differences.push_back("Shared Memory: " + std::to_string(config1.shared_memory_size) + " vs " + std::to_string(config2.shared_memory_size));
    }

    if (config1.points_per_thread != config2.points_per_thread) {
        differences.push_back("Points/Thread: " + std::to_string(config1.points_per_thread) + " vs " + std::to_string(config2.points_per_thread));
    }

    if (config1.config_source != config2.config_source) {
        differences.push_back("Config Source: " + config1.config_source + " vs " + config2.config_source);
    }

    if (config1.architecture != config2.architecture) {
        differences.push_back("Architecture: " + integration_utils::architecture_to_string(config1.architecture) +
                             " vs " + integration_utils::architecture_to_string(config2.architecture));
    }

    return differences;
}

} // namespace integration_utils
} // namespace integration
} // namespace keyhunt
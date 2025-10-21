// Puzzle71Solver - Memory Configuration Manager Implementation
// Forces aggressive memory optimization configuration (T012)

#include "memory_config_manager.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <iostream>

using json = nlohmann::json;

namespace keyhunt {
namespace common {

const std::string MemoryConfigManager::DEFAULT_VERSION = "1.0.0";
const std::string MemoryConfigManager::CONFIGURATION_TYPE = "memory_optimization";
const std::vector<int> MemoryConfigManager::VALID_BLOCK_SIZES = {32, 64, 128, 256, 512, 1024};
const std::vector<std::string> MemoryConfigManager::VALID_ARCHITECTURES = {"75", "86", "89", "90"};

// Global configuration manager instance
std::unique_ptr<MemoryConfigManager> g_memory_config_manager = nullptr;

MemoryConfigManager::MemoryConfigManager(const std::string& config_file)
    : config_file_path_(config_file) {
    initialize_default_configuration();
    populate_gpu_profiles();
}

bool MemoryConfigManager::load_configuration() {
    log_info("Loading memory optimization configuration from: " + config_file_path_);

    if (!fs::exists(config_file_path_)) {
        log_warning("Configuration file not found: " + config_file_path_);
        log_info("Using default configuration with forced optimizations enabled");
        config_.force_enabled = true;
        return true;
    }

    try {
        std::ifstream file(config_file_path_);
        if (!file.is_open()) {
            log_error("Cannot open configuration file: " + config_file_path_);
            return false;
        }

        json json_data;
        file >> json_data;
        file.close();

        // Validate configuration version
        if (json_data.contains("version") && json_data["version"] != DEFAULT_VERSION) {
            log_warning("Configuration version mismatch, attempting migration");
        }

        // Parse configuration
        if (!parse_configuration_file(json_data)) {
            log_error("Failed to parse configuration file");
            return false;
        }

        // Force enable all optimizations (T012 requirement)
        config_.force_enabled = true;
        config_.enabled = true;
        config_.shared_memory.force_enable = true;
        config_.shared_memory.enabled = true;
        config_.global_memory.force_enable = true;
        config_.global_memory.enabled = true;
        config_.coalesced_access_force_enable = true;
        config_.coalesced_access_enabled = true;
        config_.structure_of_arrays_force_enable = true;
        config_.structure_of_arrays_enabled = true;
        config_.register_optimization.force_enable = true;
        config_.register_optimization.enabled = true;
        config_.kernel_launch_optimization.force_enable = true;
        config_.kernel_launch_optimization.enabled = true;

        log_info("Memory optimization configuration loaded successfully");
        log_info("All optimizations force-enabled (T012 requirement)");

        return validate_configuration();
    } catch (const json::exception& e) {
        log_error("JSON parsing error in configuration file: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        log_error("Error loading configuration: " + std::string(e.what()));
        return false;
    }
}

bool MemoryConfigManager::validate_configuration() const {
    log_info("Validating memory optimization configuration");

    bool is_valid = true;

    is_valid &= validate_shared_memory_config();
    is_valid &= validate_global_memory_config();
    is_valid &= validate_register_config();
    is_valid &= validate_compiler_flags();

    if (is_valid) {
        log_info("Configuration validation passed");
    } else {
        log_error("Configuration validation failed");
    }

    return is_valid;
}

void MemoryConfigManager::initialize_default_configuration() {
    log_info("Initializing default memory optimization configuration");

    config_.version = DEFAULT_VERSION;
    config_.configuration_type = CONFIGURATION_TYPE;
    config_.enabled = true;
    config_.force_enabled = true;

    // Initialize shared memory configuration
    config_.shared_memory.enabled = true;
    config_.shared_memory.force_enable = true;
    config_.shared_memory.bank_size_bytes = 4;
    config_.shared_memory.max_shared_memory_per_block = 49152;
    config_.shared_memory.preferred_shared_memory_per_block = 32768;
    config_.shared_memory.shared_memory_carveout_percent = 75;

    // Initialize global memory configuration
    config_.global_memory.enabled = true;
    config_.global_memory.force_enable = true;
    config_.global_memory.memory_coalescing.enabled = true;
    config_.global_memory.memory_coalescing.transaction_size_bytes = 128;
    config_.global_memory.memory_coalescing.alignment_bytes = 128;
    config_.global_memory.bandwidth_optimization.enabled = true;
    config_.global_memory.bandwidth_optimization.target_efficiency_percent = 95.0;

    // Initialize register optimization
    config_.register_optimization.enabled = true;
    config_.register_optimization.force_enable = true;
    config_.register_optimization.register_pressure.target_registers_per_thread = 32;
    config_.register_optimization.register_pressure.max_registers_per_thread = 40;
    config_.register_optimization.occupancy_optimization.target_occupancy_percent = 80.0;

    // Initialize data structure optimization
    config_.structure_of_arrays_enabled = true;
    config_.structure_of_arrays_force_enable = true;
    config_.data_alignment_bytes = 16;

    // Initialize memory access patterns
    config_.coalesced_access_enabled = true;
    config_.coalesced_access_force_enable = true;
    config_.vector_width = 4;
    config_.prefetching_enabled = true;
    config_.prefetch_distance = 2;

    // Initialize kernel launch optimization
    config_.kernel_launch_optimization.enabled = true;
    config_.kernel_launch_optimization.force_enable = true;
    config_.kernel_launch_optimization.block_size_optimization.enabled = true;
    config_.kernel_launch_optimization.block_size_optimization.optimal_block_size = 256;
    config_.kernel_launch_optimization.grid_size_optimization.enabled = true;
    config_.kernel_launch_optimization.grid_size_optimization.minimum_blocks_per_sm = 4;

    // Initialize compiler flags
    config_.compiler_flags.nvcc_flags.optimization_level = "-O3";
    config_.compiler_flags.nvcc_flags.aggressive_optimization = true;
    config_.compiler_flags.nvcc_flags.fast_math = true;
    config_.compiler_flags.nvcc_flags.maxrregcount = 40;
    config_.compiler_flags.gpu_architecture_flags.target_architectures = {"75", "86", "89", "90"};

    // Initialize performance targets
    config_.memory_efficiency_target = 0.95;
    config_.bandwidth_utilization_target = 0.90;
    config_.cache_hit_rate_target = 0.85;
    config_.validation_enabled = true;

    log_info("Default configuration initialized with forced optimizations");
}

void MemoryConfigManager::populate_gpu_profiles() {
    log_info("Populating GPU-specific optimization profiles");

    // Turing (RTX 20 series)
    GPUOptimizationProfile turing_profile;
    turing_profile.compute_capability = "75";
    turing_profile.shared_memory_config = "48KB";
    turing_profile.l1_cache_config = "16KB";
    turing_profile.memory_bus_width = 256;
    turing_profile.memory_bandwidth_gb_per_sec = 448;
    turing_profile.register_file_size = 65536;
    config_.gpu_profiles["turing"] = turing_profile;

    // Ampere (RTX 30 series)
    GPUOptimizationProfile ampere_profile;
    ampere_profile.compute_capability = "86";
    ampere_profile.shared_memory_config = "164KB";
    ampere_profile.l1_cache_config = "128KB";
    ampere_profile.memory_bus_width = 352;
    ampere_profile.memory_bandwidth_gb_per_sec = 936;
    ampere_profile.register_file_size = 65536;
    config_.gpu_profiles["ampere"] = ampere_profile;

    // Ada Lovelace (RTX 40 series)
    GPUOptimizationProfile ada_profile;
    ada_profile.compute_capability = "89";
    ada_profile.shared_memory_config = "164KB";
    ada_profile.l1_cache_config = "128KB";
    ada_profile.memory_bus_width = 384;
    ada_profile.memory_bandwidth_gb_per_sec = 1008;
    ada_profile.register_file_size = 65536;
    config_.gpu_profiles["ada_lovelace"] = ada_profile;

    // Hopper (H100, H20)
    GPUOptimizationProfile hopper_profile;
    hopper_profile.compute_capability = "90";
    hopper_profile.shared_memory_config = "228KB";
    hopper_profile.l1_cache_config = "128KB";
    hopper_profile.memory_bus_width = 5120;
    hopper_profile.memory_bandwidth_gb_per_sec = 3350;
    hopper_profile.register_file_size = 65536;
    config_.gpu_profiles["hoppper"] = hopper_profile;

    log_info("GPU profiles populated for " + std::to_string(config_.gpu_profiles.size()) + " architectures");
}

bool MemoryConfigManager::parse_configuration_file(const json& json_data) {
    log_debug("Parsing configuration file JSON data");

    try {
        // Parse basic configuration
        if (json_data.contains("enabled")) {
            config_.enabled = json_data["enabled"];
        }
        if (json_data.contains("force_enabled")) {
            config_.force_enabled = json_data["force_enabled"];
        }

        // Parse memory hierarchy configuration
        if (json_data.contains("memory_hierarchy_optimization")) {
            parse_shared_memory_config(json_data["memory_hierarchy_optimization"]);
            parse_global_memory_config(json_data["memory_hierarchy_optimization"]);
        }

        // Parse register optimization
        if (json_data.contains("register_optimization")) {
            parse_register_optimization_config(json_data["register_optimization"]);
        }

        // Parse kernel launch optimization
        if (json_data.contains("kernel_launch_optimization")) {
            parse_kernel_launch_optimization(json_data["kernel_launch_optimization"]);
        }

        // Parse compiler flags
        if (json_data.contains("compiler_optimization_flags")) {
            parse_compiler_flags(json_data["compiler_optimization_flags"]);
        }

        // Parse GPU profiles
        if (json_data.contains("gpu_specific_optimizations")) {
            parse_gpu_profiles(json_data["gpu_specific_optimizations"]);
        }

        return true;
    } catch (const std::exception& e) {
        log_error("Error parsing configuration file: " + std::string(e.what()));
        return false;
    }
}

void MemoryConfigManager::parse_shared_memory_config(const json& json) {
    if (json.contains("shared_memory")) {
        const auto& sm_json = json["shared_memory"];
        config_.shared_memory.enabled = sm_json.value("enabled", true);
        config_.shared_memory.force_enable = sm_json.value("force_enable", true);
        config_.shared_memory.max_shared_memory_per_block = sm_json.value("max_shared_memory_per_block", 49152);
        config_.shared_memory.shared_memory_carveout_percent = sm_json.value("shared_memory_carveout_percent", 75);
    }
}

void MemoryConfigManager::parse_global_memory_config(const json& json) {
    if (json.contains("global_memory")) {
        const auto& gm_json = json["global_memory"];
        config_.global_memory.enabled = gm_json.value("enabled", true);
        config_.global_memory.force_enable = gm_json.value("force_enable", true);

        if (gm_json.contains("memory_coalescing")) {
            const auto& mc_json = gm_json["memory_coalescing"];
            config_.global_memory.memory_coalescing.enabled = mc_json.value("enabled", true);
            config_.global_memory.memory_coalescing.transaction_size_bytes = mc_json.value("transaction_size_bytes", 128);
            config_.global_memory.memory_coalescing.alignment_bytes = mc_json.value("alignment_bytes", 128);
        }

        if (gm_json.contains("bandwidth_optimization")) {
            const auto& bo_json = gm_json["bandwidth_optimization"];
            config_.global_memory.bandwidth_optimization.enabled = bo_json.value("enabled", true);
            config_.global_memory.bandwidth_optimization.target_efficiency_percent = bo_json.value("target_efficiency_percent", 95.0);
        }
    }
}

void MemoryConfigManager::parse_register_optimization_config(const json& json) {
    const auto& reg_json = json;
    config_.register_optimization.enabled = reg_json.value("enabled", true);
    config_.register_optimization.force_enable = reg_json.value("force_enable", true);

    if (reg_json.contains("register_pressure")) {
        const auto& rp_json = reg_json["register_pressure"];
        config_.register_optimization.register_pressure.target_registers_per_thread = rp_json.value("target_registers_per_thread", 32);
        config_.register_optimization.register_pressure.max_registers_per_thread = rp_json.value("max_registers_per_thread", 40);
    }

    if (reg_json.contains("occupancy_optimization")) {
        const auto& occ_json = reg_json["occupancy_optimization"];
        config_.register_optimization.occupancy_optimization.target_occupancy_percent = occ_json.value("target_occupancy_percent", 80.0);
        config_.register_optimization.occupancy_optimization.min_occupancy_percent = occ_json.value("min_occupancy_percent", 50.0);
    }
}

void MemoryConfigManager::parse_kernel_launch_optimization(const json& json) {
    const auto& kernel_json = json;
    config_.kernel_launch_optimization.enabled = kernel_json.value("enabled", true);
    config_.kernel_launch_optimization.force_enable = kernel_json.value("force_enable", true);

    if (kernel_json.contains("block_size_optimization")) {
        const auto& bs_json = kernel_json["block_size_optimization"];
        config_.kernel_launch_optimization.block_size_optimization.enabled = bs_json.value("enabled", true);
        config_.kernel_launch_optimization.block_size_optimization.optimal_block_size = bs_json.value("optimal_block_size", 256);
    }

    if (kernel_json.contains("grid_size_optimization")) {
        const auto& gs_json = kernel_json["grid_size_optimization"];
        config_.kernel_launch_optimization.grid_size_optimization.enabled = gs_json.value("enabled", true);
        config_.kernel_launch_optimization.grid_size_optimization.minimum_blocks_per_sm = gs_json.value("minimum_blocks_per_sm", 4);
    }
}

void MemoryConfigManager::parse_compiler_flags(const json& json) {
    if (json.contains("nvcc_flags")) {
        const auto& nvcc_json = json["nvcc_flags"];
        config_.compiler_flags.nvcc_flags.optimization_level = nvcc_json.value("optimization_level", "-O3");
        config_.compiler_flags.nvcc_flags.aggressive_optimization = nvcc_json.value("aggressive_optimization", true);
        config_.compiler_flags.nvcc_flags.fast_math = nvcc_json.value("fast_math", true);
        config_.compiler_flags.nvcc_flags.maxrregcount = nvcc_json.value("maxrregcount", 40);
    }

    if (json.contains("gpu_architecture_flags")) {
        const auto& arch_json = json["gpu_architecture_flags"];
        if (arch_json.contains("target_architectures")) {
            config_.compiler_flags.gpu_architecture_flags.target_architectures =
                arch_json["target_architectures"].get<std::vector<std::string>>();
        }
    }
}

void MemoryConfigManager::parse_gpu_profiles(const json& json) {
    for (auto& [gpu_name, profile_json] : json.items()) {
        GPUOptimizationProfile profile;
        profile.compute_capability = profile_json.value("compute_capability", "");
        profile.shared_memory_config = profile_json.value("shared_memory_config", "");
        profile.memory_bus_width = profile_json.value("memory_bus_width", 0);
        profile.memory_bandwidth_gb_per_sec = profile_json.value("memory_bandwidth_gb_per_sec", 0.0);
        config_.gpu_profiles[gpu_name] = profile;
    }
}

std::vector<std::string> MemoryConfigManager::generate_nvcc_flags() const {
    std::vector<std::string> flags;

    const auto& nvcc = config_.compiler_flags.nvcc_flags;

    flags.push_back(nvcc.optimization_level);

    if (nvcc.aggressive_optimization) {
        flags.push_back("-Xcompiler=-O3");
        flags.push_back("-Xcompiler=-march=native");
    }

    if (nvcc.fast_math) {
        flags.push_back("--use_fast_math");
        flags.push_back("-Xcompiler=-ffast-math");
    }

    if (nvcc.ftz) {
        flags.push_back("-Xptxas=-ftz=true");
    }

    if (!nvcc.prec_div) {
        flags.push_back("-Xptxas=-prec-div=false");
    }

    if (!nvcc.prec_sqrt) {
        flags.push_back("-Xptxas=-prec-sqrt=false");
    }

    if (nvcc.fmad) {
        flags.push_back("-Xptxas=-fmad=true");
    }

    flags.push_back("-maxrregcount=" + std::to_string(nvcc.maxrregcount));

    if (nvcc.restrict) {
        flags.push_back("-Xcompiler=-DRESTRICT=restrict");
    }

    flags.push_back("-Xcompiler=-malign-double");

    log_debug("Generated " + std::to_string(flags.size()) + " NVCC flags");
    return flags;
}

std::vector<std::string> MemoryConfigManager::generate_memory_optimization_flags() const {
    std::vector<std::string> flags;

    const auto& mem_flags = config_.compiler_flags.memory_optimization_flags;

    if (mem_flags.memopt) {
        flags.push_back("-Xptxas=-O3");
        flags.push_back("-Xptxas=--allow-expensive-optimizations=true");
    }

    flags.push_back("-Xptxas=-dlcm=" + mem_flags.dlcm);

    for (const auto& xptxas_flag : mem_flags.Xptxas) {
        flags.push_back("-Xptxas=" + xptxas_flag);
    }

    log_debug("Generated " + std::to_string(flags.size()) + " memory optimization flags");
    return flags;
}

bool MemoryConfigManager::validate_shared_memory_config() const {
    bool valid = true;

    if (config_.shared_memory.max_shared_memory_per_block <= 0) {
        log_error("Invalid max shared memory per block: " + std::to_string(config_.shared_memory.max_shared_memory_per_block));
        valid = false;
    }

    if (config_.shared_memory.shared_memory_carveout_percent < 0 || config_.shared_memory.shared_memory_carveout_percent > 100) {
        log_error("Invalid shared memory carveout percent: " + std::to_string(config_.shared_memory.shared_memory_carveout_percent));
        valid = false;
    }

    return valid;
}

bool MemoryConfigManager::validate_global_memory_config() const {
    bool valid = true;

    if (config_.global_memory.memory_coalescing.transaction_size_bytes <= 0) {
        log_error("Invalid memory transaction size: " + std::to_string(config_.global_memory.memory_coalescing.transaction_size_bytes));
        valid = false;
    }

    if (config_.global_memory.memory_coalescing.alignment_bytes <= 0 ||
        (config_.global_memory.memory_coalescing.alignment_bytes & (config_.global_memory.memory_coalescing.alignment_bytes - 1)) != 0) {
        log_error("Invalid memory alignment (must be power of 2): " + std::to_string(config_.global_memory.memory_coalescing.alignment_bytes));
        valid = false;
    }

    return valid;
}

bool MemoryConfigManager::validate_register_config() const {
    bool valid = true;

    const auto& reg_pressure = config_.register_optimization.register_pressure;

    if (reg_pressure.target_registers_per_thread <= 0 || reg_pressure.target_registers_per_thread > 255) {
        log_error("Invalid target registers per thread: " + std::to_string(reg_pressure.target_registers_per_thread));
        valid = false;
    }

    if (reg_pressure.max_registers_per_thread <= 0 || reg_pressure.max_registers_per_thread > 255) {
        log_error("Invalid max registers per thread: " + std::to_string(reg_pressure.max_registers_per_thread));
        valid = false;
    }

    if (reg_pressure.target_registers_per_thread > reg_pressure.max_registers_per_thread) {
        log_error("Target registers per thread exceeds max registers per thread");
        valid = false;
    }

    return valid;
}

bool MemoryConfigManager::validate_compiler_flags() const {
    bool valid = true;

    const auto& arch_flags = config_.compiler_flags.gpu_architecture_flags;

    for (const auto& arch : arch_flags.target_architectures) {
        if (std::find(VALID_ARCHITECTURES.begin(), VALID_ARCHITECTURES.end(), arch) == VALID_ARCHITECTURES.end()) {
            log_warning("Potentially unsupported GPU architecture: " + arch);
        }
    }

    if (config_.compiler_flags.nvcc_flags.maxrregcount <= 0 || config_.compiler_flags.nvcc_flags.maxrregcount > 255) {
        log_error("Invalid maxrregcount: " + std::to_string(config_.compiler_flags.nvcc_flags.maxrregcount));
        valid = false;
    }

    return valid;
}

std::vector<std::string> MemoryConfigManager::get_configuration_warnings() const {
    std::vector<std::string> warnings;

    if (!config_.enabled) {
        warnings.push_back("Memory optimization is disabled");
    }

    if (!config_.force_enabled) {
        warnings.push_back("Memory optimization is not force-enabled (T012 requirement)");
    }

    if (config_.global_memory.bandwidth_optimization.target_efficiency_percent < 90.0) {
        warnings.push_back("Memory bandwidth efficiency target is below 90%");
    }

    if (config_.register_optimization.occupancy_optimization.target_occupancy_percent < 70.0) {
        warnings.push_back("Occupancy target is below 70%");
    }

    return warnings;
}

double MemoryConfigManager::estimate_memory_bandwidth_improvement() const {
    // Estimate based on enabled optimizations
    double improvement = 0.0;

    if (config_.global_memory.memory_coalescing.enabled) {
        improvement += 15.0; // Coalesced access improvement
    }

    if (config_.shared_memory.enabled) {
        improvement += 20.0; // Shared memory optimization
    }

    if (config_.structure_of_arrays_enabled) {
        improvement += 10.0; // SoA layout improvement
    }

    if (config_.coalesced_access_enabled) {
        improvement += 12.0; // Coalesced access patterns
    }

    return std::min(improvement, 50.0); // Cap at 50% estimated improvement
}

double MemoryConfigManager::estimate_occupancy_improvement() const {
    double improvement = 0.0;

    if (config_.register_optimization.enabled) {
        improvement += 20.0; // Register pressure reduction
    }

    if (config_.kernel_launch_optimization.block_size_optimization.enabled) {
        improvement += 15.0; // Block size optimization
    }

    if (config_.register_optimization.occupancy_optimization.enabled) {
        improvement += 10.0; // Occupancy optimization
    }

    return std::min(improvement, 40.0); // Cap at 40% estimated improvement
}

double MemoryConfigManager::estimate_cache_hit_rate_improvement() const {
    double improvement = 0.0;

    if (config_.shared_memory.enabled) {
        improvement += 25.0; // Shared memory caching
    }

    if (config_.global_memory.cache_configuration.read_only_cache) {
        improvement += 15.0; // Read-only cache utilization
    }

    if (config_.prefetching_enabled) {
        improvement += 10.0; // Prefetching
    }

    return std::min(improvement, 35.0); // Cap at 35% estimated improvement
}

void MemoryConfigManager::log_info(const std::string& message) const {
    std::cout << "[INFO] MemoryConfigManager: " << message << std::endl;
}

void MemoryConfigManager::log_warning(const std::string& message) const {
    std::cerr << "[WARNING] MemoryConfigManager: " << message << std::endl;
}

void MemoryConfigManager::log_error(const std::string& message) const {
    std::cerr << "[ERROR] MemoryConfigManager: " << message << std::endl;
}

void MemoryConfigManager::log_debug(const std::string& message) const {
    #ifdef DEBUG
    std::cout << "[DEBUG] MemoryConfigManager: " << message << std::endl;
    #endif
}

// Global convenience functions
bool initialize_memory_configuration(const std::string& config_file) {
    std::string file_path = config_file.empty() ? "data/config/memory_optimization.json" : config_file;
    g_memory_config_manager = std::make_unique<MemoryConfigManager>(file_path);

    if (!g_memory_config_manager->load_configuration()) {
        return false;
    }

    return g_memory_config_manager->validate_configuration();
}

const MemoryOptimizationConfig& get_memory_configuration() {
    if (!g_memory_config_manager) {
        throw std::runtime_error("Memory configuration not initialized. Call initialize_memory_configuration() first.");
    }
    return g_memory_config_manager->get_config();
}

bool apply_memory_optimizations() {
    if (!g_memory_config_manager) {
        return false;
    }

    // Enforce all memory optimizations
    g_memory_config_manager->enforce_shared_memory_configuration();
    g_memory_config_manager->enforce_register_configuration();
    g_memory_config_manager->enforce_launch_configuration();

    return true;
}

std::vector<std::string> get_memory_optimization_compiler_flags() {
    if (!g_memory_config_manager) {
        return {};
    }

    auto nvcc_flags = g_memory_config_manager->generate_nvcc_flags();
    auto mem_flags = g_memory_config_manager->generate_memory_optimization_flags();

    nvcc_flags.insert(nvcc_flags.end(), mem_flags.begin(), mem_flags.end());
    return nvcc_flags;
}

} // namespace common
} // namespace keyhunt
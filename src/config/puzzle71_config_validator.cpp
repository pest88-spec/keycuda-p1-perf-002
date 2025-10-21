/**
 * @file puzzle71_config_validator.cpp
 * @brief Implementation of comprehensive YAML configuration validation framework for v5.5 constraints
 *
 * @author Puzzle71 Technical Debt Repair System
 * @version 5.5
 * @date 2025-10-21
 */

#include "puzzle71_config_validator.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <regex>
#include <cmath>
#include <sstream>

namespace keyhunt {
namespace config {

// Constants
const std::string ConstitutionalValidator::REQUIRED_VERSION = "5.5";
const std::vector<std::string> ConstitutionalValidator::REQUIRED_CONSTITUTIONAL_FIELDS = {
    "config_version",
    "config_schema_version",
    "gpu_devices",
    "kernel_configs",
    "performance_config",
    "validation_config"
};

const std::vector<std::string> Puzzle71ConfigValidator::REQUIRED_SECTIONS = {
    "config_version",
    "config_schema_version",
    "gpu_devices",
    "kernel_configs",
    "performance_config",
    "validation_config"
};

ConfigurationSchema Puzzle71ConfigValidator::schema_;

// ConfigurationSchema implementation
ConfigurationSchema ConfigurationSchema::get_v55_schema() {
    ConfigurationSchema schema;

    // Root section
    SectionDefinition root;
    root.name = "";
    root.required = true;
    root.description = "Root configuration section";

    root.fields = {
        {"config_version", "string", true, "", "", "Configuration version (must be 5.5)", "CONST_VERSION", {"5.5"}},
        {"config_schema_version", "string", true, "", "", "Schema version", "CONST_SCHEMA", {"1.0"}},
        {"creation_timestamp", "integer", false, "", "", "Creation timestamp", "", {}},
        {"git_commit_hash", "string", false, "", "", "Git commit hash", "", {}},
        {"config_checksum", "string", false, "", "", "Configuration checksum", "", {}}
    };

    // GPU devices section
    SectionDefinition gpu_section;
    gpu_section.name = "gpu_devices";
    gpu_section.required = true;
    gpu_section.description = "GPU device configurations";

    gpu_section.fields = {
        {"device_id", "integer", true, "0", "", "GPU device ID", "GPU_DEVICE_ID", {}},
        {"device_name", "string", true, "", "", "GPU device name", "GPU_NAME", {}},
        {"compute_capability", "object", true, "", "", "Compute capability specification", "GPU_COMPUTE_CAP", {}},
        {"total_memory_bytes", "integer", true, "1073741824", "", "Total GPU memory in bytes", "GPU_MEMORY", {}},
        {"max_threads_per_block", "integer", true, "1", "1024", "Maximum threads per block", "GPU_THREADS", {}},
        {"warp_size", "integer", true, "32", "32", "Warp size", "GPU_WARP", {}},
        {"architecture_family", "string", true, "", "", "Architecture family", "GPU_ARCH", {"Turing", "Ampere", "Hopper"}}
    };

    // Kernel configurations section
    SectionDefinition kernel_section;
    kernel_section.name = "kernel_configs";
    kernel_section.required = true;
    kernel_section.description = "Kernel launch configurations";

    kernel_section.fields = {
        {"kernel_name", "string", true, "", "", "Kernel name", "KERNEL_NAME", {}},
        {"architecture_family", "string", true, "", "", "Target architecture", "KERNEL_ARCH", {"Turing", "Ampere", "Hopper"}},
        {"block_size", "object", true, "", "", "Block dimensions", "KERNEL_BLOCK", {}},
        {"min_grid_size", "integer", true, "1", "", "Minimum grid size", "KERNEL_GRID_MIN", {}},
        {"max_grid_size", "integer", true, "1", "", "Maximum grid size", "KERNEL_GRID_MAX", {}},
        {"shared_memory_size_bytes", "integer", true, "0", "65536", "Shared memory size", "KERNEL_SHARED", {}},
        {"registers_per_thread", "integer", true, "1", "255", "Registers per thread", "KERNEL_REGS", {}},
        {"expected_occupancy", "number", true, "0.0", "1.0", "Expected occupancy", "KERNEL_OCCUPANCY", {}}
    };

    // Performance configuration section
    SectionDefinition perf_section;
    perf_section.name = "performance_config";
    perf_section.required = true;
    perf_section.description = "Performance optimization settings";

    perf_section.fields = {
        {"enable_shared_memory_optimization", "boolean", true, "", "", "Enable shared memory optimization", "PERF_SHARED_MEM", {}},
        {"enable_warp_level_optimization", "boolean", true, "", "", "Enable warp-level optimization", "PERF_WARP", {}},
        {"enable_memory_coalescing", "boolean", true, "", "", "Enable memory coalescing", "PERF_COALESCING", {}},
        {"target_gpu_utilization_percent", "number", true, "70.0", "100.0", "Target GPU utilization", "PERF_GPU_UTIL", {}},
        {"target_memory_efficiency_percent", "number", true, "90.0", "100.0", "Target memory efficiency", "PERF_MEM_EFF", {}},
        {"memory_alignment_bytes", "integer", true, "1", "1024", "Memory alignment", "PERF_ALIGNMENT", {}}
    };

    // Validation configuration section
    SectionDefinition validation_section;
    validation_section.name = "validation_config";
    validation_section.required = true;
    validation_section.description = "Validation and testing settings";

    validation_section.fields = {
        {"enable_deterministic_validation", "boolean", true, "", "", "Enable deterministic validation", "VAL_DETERMINISTIC", {}},
        {"enable_cpu_gpu_validation", "boolean", true, "", "", "Enable CPU/GPU validation", "VAL_CPU_GPU", {}},
        {"enable_performance_regression_detection", "boolean", true, "", "", "Enable performance regression detection", "VAL_PERF_REGRESSION", {}},
        {"enable_constitutional_compliance", "boolean", true, "", "", "Enable constitutional compliance", "VAL_CONSTITUTIONAL", {}},
        {"validation_precision_tolerance", "number", true, "0.0", "1e-10", "Validation precision tolerance", "VAL_PRECISION", {}},
        {"min_validation_iterations", "integer", true, "100", "1000000", "Minimum validation iterations", "VAL_ITERATIONS", {}}
    };

    schema.sections = {root, gpu_section, kernel_section, perf_section, validation_section};
    return schema;
}

// ConstitutionalValidator implementation
bool ConstitutionalValidator::validate_constitutional_compliance(const YAML::Node& config, ValidationResult& result) {
    bool compliant = true;

    // Validate version constraint
    if (!validate_version_constraint(config, result)) {
        compliant = false;
    }

    // Validate static configuration
    if (!validate_static_configuration(config, result)) {
        compliant = false;
    }

    // Validate no runtime device queries
    if (!validate_no_runtime_device_queries(config, result)) {
        compliant = false;
    }

    // Validate deterministic replay
    if (!validate_deterministic_replay(config, result)) {
        compliant = false;
    }

    return compliant;
}

bool ConstitutionalValidator::validate_version_constraint(const YAML::Node& config, ValidationResult& result) {
    if (!config["config_version"]) {
        result.addError("config_version", "MISSING_FIELD",
                       "Configuration version is required",
                       "Add config_version: '5.5' to your configuration",
                       "CONST_VERSION");
        return false;
    }

    std::string version = config["config_version"].as<std::string>();
    if (version != REQUIRED_VERSION) {
        result.addError("config_version", "VERSION_MISMATCH",
                       "Configuration version must be '5.5', found: " + version,
                       "Update config_version to '5.5'",
                       "CONST_VERSION");
        return false;
    }

    return true;
}

bool ConstitutionalValidator::validate_static_configuration(const YAML::Node& config, ValidationResult& result) {
    // Check for static kernel launch configurations
    if (!config["kernel_configs"] || !config["kernel_configs"].IsSequence()) {
        result.addError("kernel_configs", "MISSING_KERNEL_CONFIGS",
                       "Static kernel configurations are required",
                       "Add kernel_configs section with static launch parameters",
                       "STATIC_CONFIG");
        return false;
    }

    const auto& kernel_configs = config["kernel_configs"];
    for (size_t i = 0; i < kernel_configs.size(); ++i) {
        const auto& kernel = kernel_configs[i];
        std::string kernel_path = "kernel_configs[" + std::to_string(i) + "]";

        // Validate static block sizes
        if (!kernel["block_size"]) {
            result.addError(kernel_path + ".block_size", "MISSING_BLOCK_SIZE",
                           "Static block size is required",
                           "Specify block_size with x, y, z dimensions",
                           "STATIC_CONFIG");
        }

        // Validate grid size limits
        if (!kernel["min_grid_size"]) {
            result.addError(kernel_path + ".min_grid_size", "MISSING_MIN_GRID",
                           "Minimum grid size is required",
                           "Specify min_grid_size for static configuration",
                           "STATIC_CONFIG");
        }
    }

    return true;
}

bool ConstitutionalValidator::validate_no_runtime_device_queries(const YAML::Node& config, ValidationResult& result) {
    // Check that all device specifications are static (no runtime queries)
    if (!config["gpu_devices"] || !config["gpu_devices"].IsSequence()) {
        result.addError("gpu_devices", "MISSING_GPU_DEVICES",
                       "Static GPU device configurations are required",
                       "Add gpu_devices section with static device specifications",
                       "NO_RUNTIME_QUERIES");
        return false;
    }

    const auto& gpu_devices = config["gpu_devices"];
    for (size_t i = 0; i < gpu_devices.size(); ++i) {
        const auto& device = gpu_devices[i];
        std::string device_path = "gpu_devices[" + std::to_string(i) + "]";

        // Ensure all required device fields are present
        if (!device["compute_capability"]) {
            result.addError(device_path + ".compute_capability", "MISSING_COMPUTE_CAP",
                           "Static compute capability is required",
                           "Specify compute_capability with major and minor versions",
                           "NO_RUNTIME_QUERIES");
        }

        if (!device["total_memory_bytes"]) {
            result.addError(device_path + ".total_memory_bytes", "MISSING_MEMORY",
                           "Static memory specification is required",
                           "Specify total_memory_bytes for the device",
                           "NO_RUNTIME_QUERIES");
        }
    }

    return true;
}

bool ConstitutionalValidator::validate_deterministic_replay(const YAML::Node& config, ValidationResult& result) {
    if (!config["validation_config"]) {
        result.addError("validation_config", "MISSING_VALIDATION_CONFIG",
                       "Validation configuration is required for deterministic replay",
                       "Add validation_config section",
                       "DETERMINISTIC_REPLAY");
        return false;
    }

    const auto& validation = config["validation_config"];

    // Check deterministic validation is enabled
    if (!validation["enable_deterministic_validation"] ||
        !validation["enable_deterministic_validation"].as<bool>()) {
        result.addError("validation_config.enable_deterministic_validation", "DETERMINISTIC_DISABLED",
                       "Deterministic validation must be enabled",
                       "Set enable_deterministic_validation: true",
                       "DETERMINISTIC_REPLAY");
        return false;
    }

    // Check precision tolerance
    if (!validation["validation_precision_tolerance"]) {
        result.addWarning("validation_config.validation_precision_tolerance", "MISSING_PRECISION",
                         "Validation precision tolerance not specified, using default 1e-10",
                         "Add validation_precision_tolerance: 1.0e-10",
                         "DETERMINISTIC_REPLAY");
    } else {
        double tolerance = validation["validation_precision_tolerance"].as<double>();
        if (tolerance > 1e-10) {
            result.addError("validation_config.validation_precision_tolerance", "PRECISION_TOO_HIGH",
                           "Precision tolerance must be ≤ 1e-10 for constitutional compliance",
                           "Set validation_precision_tolerance to 1.0e-10 or lower",
                           "DETERMINISTIC_REPLAY");
            return false;
        }
    }

    return true;
}

// PerformanceValidator implementation
bool PerformanceValidator::validate_performance_config(const YAML::Node& config, ValidationResult& result) {
    if (!config["performance_config"]) {
        result.addError("performance_config", "MISSING_PERFORMANCE_CONFIG",
                       "Performance configuration is required",
                       "Add performance_config section with performance targets",
                       "PERFORMANCE_CONFIG");
        return false;
    }

    const auto& perf = config["performance_config"];
    bool valid = true;

    // Validate GPU utilization target
    if (perf["target_gpu_utilization_percent"]) {
        double utilization = perf["target_gpu_utilization_percent"].as<double>();
        if (!validate_gpu_utilization(utilization, "performance_config.target_gpu_utilization_percent", result)) {
            valid = false;
        }
    } else {
        result.addError("performance_config.target_gpu_utilization_percent", "MISSING_GPU_UTILIZATION",
                       "GPU utilization target is required",
                       "Add target_gpu_utilization_percent with value ≥ 70.0",
                       "PERFORMANCE_CONFIG");
        valid = false;
    }

    // Validate memory efficiency target
    if (perf["target_memory_efficiency_percent"]) {
        double efficiency = perf["target_memory_efficiency_percent"].as<double>();
        if (!validate_memory_efficiency(efficiency, "performance_config.target_memory_efficiency_percent", result)) {
            valid = false;
        }
    } else {
        result.addError("performance_config.target_memory_efficiency_percent", "MISSING_MEMORY_EFFICIENCY",
                       "Memory efficiency target is required",
                       "Add target_memory_efficiency_percent with value ≥ 90.0",
                       "PERFORMANCE_CONFIG");
        valid = false;
    }

    // Validate optimization settings
    if (perf["enable_shared_memory_optimization"]) {
        if (!perf["enable_shared_memory_optimization"].as<bool>()) {
            result.addWarning("performance_config.enable_shared_memory_optimization", "SHARED_MEM_DISABLED",
                             "Shared memory optimization is disabled, may affect performance",
                             "Consider enabling shared memory optimization for better performance",
                             "PERFORMANCE_OPTIMIZATION");
        }
    }

    if (perf["enable_warp_level_optimization"]) {
        if (!perf["enable_warp_level_optimization"].as<bool>()) {
            result.addWarning("performance_config.enable_warp_level_optimization", "WARP_OPT_DISABLED",
                             "Warp-level optimization is disabled, may affect performance",
                             "Consider enabling warp-level optimization for better performance",
                             "PERFORMANCE_OPTIMIZATION");
        }
    }

    return valid;
}

bool PerformanceValidator::validate_memory_efficiency(double efficiency, const std::string& field_path, ValidationResult& result) {
    if (efficiency < MIN_MEMORY_EFFICIENCY) {
        result.addError(field_path, "MEMORY_EFFICIENCY_TOO_LOW",
                       "Memory efficiency target " + std::to_string(efficiency) +
                       "% is below constitutional requirement (" + std::to_string(MIN_MEMORY_EFFICIENCY) + "%)",
                       "Set " + field_path + " to " + std::to_string(MIN_MEMORY_EFFICIENCY) + " or higher",
                       "PERF_MEMORY_EFFICIENCY");
        return false;
    }
    return true;
}

bool PerformanceValidator::validate_gpu_utilization(double utilization, const std::string& field_path, ValidationResult& result) {
    if (utilization < MIN_GPU_UTILIZATION) {
        result.addError(field_path, "GPU_UTILIZATION_TOO_LOW",
                       "GPU utilization target " + std::to_string(utilization) +
                       "% is below constitutional requirement (" + std::to_string(MIN_GPU_UTILIZATION) + "%)",
                       "Set " + field_path + " to " + std::to_string(MIN_GPU_UTILIZATION) + " or higher",
                       "PERF_GPU_UTILIZATION");
        return false;
    }
    return true;
}

bool PerformanceValidator::validate_synchronization_overhead(double overhead, const std::string& field_path, ValidationResult& result) {
    if (overhead > MAX_SYNC_OVERHEAD) {
        result.addError(field_path, "SYNC_OVERHEAD_TOO_HIGH",
                       "Synchronization overhead " + std::to_string(overhead) +
                       "% exceeds constitutional limit (" + std::to_string(MAX_SYNC_OVERHEAD) + "%)",
                       "Optimize kernel synchronization to reduce overhead below " + std::to_string(MAX_SYNC_OVERHEAD) + "%",
                       "PERF_SYNC_OVERHEAD");
        return false;
    }
    return true;
}

// GPUDeviceValidator implementation
bool GPUDeviceValidator::validate_gpu_devices(const YAML::Node& config, ValidationResult& result) {
    if (!config["gpu_devices"] || !config["gpu_devices"].IsSequence()) {
        result.addError("gpu_devices", "MISSING_GPU_DEVICES",
                       "GPU devices configuration is required",
                       "Add gpu_devices array with device specifications",
                       "GPU_DEVICES");
        return false;
    }

    const auto& gpu_devices = config["gpu_devices"];
    if (!gpu_devices || gpu_devices.size() == 0) {
        result.addError("gpu_devices", "EMPTY_GPU_DEVICES",
                       "At least one GPU device must be configured",
                       "Add at least one GPU device configuration",
                       "GPU_DEVICES");
        return false;
    }

    bool valid = true;
    for (size_t i = 0; i < gpu_devices.size(); ++i) {
        const auto& device = gpu_devices[i];
        std::string device_path = "gpu_devices[" + std::to_string(i) + "]";

        // Validate required fields
        if (!device["device_id"]) {
            result.addError(device_path + ".device_id", "MISSING_DEVICE_ID",
                           "Device ID is required", "Add device_id for the GPU device", "GPU_DEVICE_ID");
            valid = false;
        }

        if (!device["device_name"]) {
            result.addError(device_path + ".device_name", "MISSING_DEVICE_NAME",
                           "Device name is required", "Add device_name for the GPU device", "GPU_NAME");
            valid = false;
        }

        if (!device["compute_capability"]) {
            result.addError(device_path + ".compute_capability", "MISSING_COMPUTE_CAP",
                           "Compute capability is required", "Add compute_capability specification", "GPU_COMPUTE_CAP");
            valid = false;
        } else {
            if (!validate_compute_capability(device["compute_capability"], device_path, result)) {
                valid = false;
            }
        }

        if (!device["total_memory_bytes"]) {
            result.addError(device_path + ".total_memory_bytes", "MISSING_MEMORY",
                           "Total memory is required", "Add total_memory_bytes specification", "GPU_MEMORY");
            valid = false;
        }

        // Validate threads per block
        if (device["max_threads_per_block"]) {
            int max_threads = device["max_threads_per_block"].as<int>();
            if (max_threads > MAX_THREADS_PER_BLOCK) {
                result.addError(device_path + ".max_threads_per_block", "THREADS_EXCEEDED",
                               "Max threads per block " + std::to_string(max_threads) +
                               " exceeds hardware limit (" + std::to_string(MAX_THREADS_PER_BLOCK) + ")",
                               "Set max_threads_per_block to " + std::to_string(MAX_THREADS_PER_BLOCK) + " or less",
                               "GPU_THREADS");
                valid = false;
            }
        }

        // Validate warp size
        if (device["warp_size"]) {
            int warp_size = device["warp_size"].as<int>();
            if (warp_size != 32) {
                result.addError(device_path + ".warp_size", "INVALID_WARP_SIZE",
                               "Warp size must be 32, found: " + std::to_string(warp_size),
                               "Set warp_size to 32",
                               "GPU_WARP");
                valid = false;
            }
        }
    }

    return valid;
}

bool GPUDeviceValidator::validate_kernel_configs(const YAML::Node& config, ValidationResult& result) {
    if (!config["kernel_configs"] || !config["kernel_configs"].IsSequence()) {
        result.addError("kernel_configs", "MISSING_KERNEL_CONFIGS",
                       "Kernel configurations are required",
                       "Add kernel_configs array with kernel specifications",
                       "KERNEL_CONFIGS");
        return false;
    }

    const auto& kernel_configs = config["kernel_configs"];
    bool valid = true;

    for (size_t i = 0; i < kernel_configs.size(); ++i) {
        const auto& kernel = kernel_configs[i];
        std::string kernel_path = "kernel_configs[" + std::to_string(i) + "]";

        // Validate kernel name
        if (!kernel["kernel_name"]) {
            result.addError(kernel_path + ".kernel_name", "MISSING_KERNEL_NAME",
                           "Kernel name is required", "Add kernel_name for the kernel", "KERNEL_NAME");
            valid = false;
        }

        // Validate block size
        if (!kernel["block_size"]) {
            result.addError(kernel_path + ".block_size", "MISSING_BLOCK_SIZE",
                           "Block size is required", "Add block_size with x, y, z dimensions", "KERNEL_BLOCK");
            valid = false;
        }

        // Validate shared memory size
        if (kernel["shared_memory_size_bytes"]) {
            int shared_mem = kernel["shared_memory_size_bytes"].as<int>();
            if (shared_mem > MAX_SHARED_MEMORY_BYTES) {
                result.addError(kernel_path + ".shared_memory_size_bytes", "SHARED_MEM_EXCEEDED",
                               "Shared memory size " + std::to_string(shared_mem) +
                               " exceeds maximum (" + std::to_string(MAX_SHARED_MEMORY_BYTES) + ")",
                               "Reduce shared_memory_size_bytes to " + std::to_string(MAX_SHARED_MEMORY_BYTES) + " or less",
                               "KERNEL_SHARED");
                valid = false;
            }
        }

        // Validate occupancy
        if (kernel["expected_occupancy"]) {
            double occupancy = kernel["expected_occupancy"].as<double>();
            if (occupancy < 0.0 || occupancy > 1.0) {
                result.addError(kernel_path + ".expected_occupancy", "INVALID_OCCUPANCY",
                               "Expected occupancy must be between 0.0 and 1.0, found: " + std::to_string(occupancy),
                               "Set expected_occupancy to a value between 0.0 and 1.0",
                               "KERNEL_OCCUPANCY");
                valid = false;
            } else if (occupancy < 0.5) {
                result.addWarning(kernel_path + ".expected_occupancy", "LOW_OCCUPANCY",
                                 "Expected occupancy " + std::to_string(occupancy) + " is quite low",
                                 "Consider optimizing kernel for better occupancy",
                                 "KERNEL_OCCUPANCY");
            }
        }
    }

    return valid;
}

bool GPUDeviceValidator::validate_compute_capability(const YAML::Node& compute_cap, const std::string& device_path, ValidationResult& result) {
    if (!compute_cap["major"] || !compute_cap["minor"]) {
        result.addError(device_path + ".compute_capability", "INCOMPLETE_COMPUTE_CAP",
                       "Compute capability must specify both major and minor versions",
                       "Add both major and minor versions to compute_capability",
                       "GPU_COMPUTE_CAP");
        return false;
    }

    int major = compute_cap["major"].as<int>();
    int minor = compute_cap["minor"].as<int>();

    // Validate minimum compute capability
    if (major < MIN_COMPUTE_CAPABILITY_MAJOR ||
        (major == MIN_COMPUTE_CAPABILITY_MAJOR && minor < MIN_COMPUTE_CAPABILITY_MINOR)) {
        result.addError(device_path + ".compute_capability", "COMPUTE_CAP_TOO_LOW",
                       "Compute capability " + std::to_string(major) + "." + std::to_string(minor) +
                       " is below minimum requirement (" + std::to_string(MIN_COMPUTE_CAPABILITY_MAJOR) +
                       "." + std::to_string(MIN_COMPUTE_CAPABILITY_MINOR) + ")",
                       "Use GPU with compute capability " + std::to_string(MIN_COMPUTE_CAPABILITY_MAJOR) +
                       "." + std::to_string(MIN_COMPUTE_CAPABILITY_MINOR) + " or higher",
                       "GPU_COMPUTE_CAP");
        return false;
    }

    // Warn about unknown architectures
    if (major == 7 && (minor < 0 || minor > 5)) {
        result.addWarning(device_path + ".compute_capability", "UNKNOWN_TURING",
                         "Unknown Turing compute capability: " + std::to_string(major) + "." + std::to_string(minor),
                         "Verify compute capability is correct for this GPU",
                         "GPU_COMPUTE_CAP");
    } else if (major == 8 && (minor < 0 || minor > 9)) {
        result.addWarning(device_path + ".compute_capability", "UNKNOWN_AMPERE",
                         "Unknown Ampere compute capability: " + std::to_string(major) + "." + std::to_string(minor),
                         "Verify compute capability is correct for this GPU",
                         "GPU_COMPUTE_CAP");
    } else if (major == 9 && (minor < 0 || minor > 0)) {
        result.addWarning(device_path + ".compute_capability", "UNKNOWN_HOPPER",
                         "Unknown Hopper compute capability: " + std::to_string(major) + "." + std::to_string(minor),
                         "Verify compute capability is correct for this GPU",
                         "GPU_COMPUTE_CAP");
    } else if (major > 9) {
        result.addWarning(device_path + ".compute_capability", "FUTURE_ARCHITECTURE",
                         "Future GPU architecture detected: " + std::to_string(major) + "." + std::to_string(minor),
                         "Verify support for this architecture",
                         "GPU_COMPUTE_CAP");
    }

    return true;
}

// Main Puzzle71ConfigValidator implementation
bool Puzzle71ConfigValidator::validate_config_file(const std::string& config_file, ValidationResult& result) {
    auto start_time = std::chrono::high_resolution_clock::now();

    // Check if file exists
    if (!std::filesystem::exists(config_file)) {
        result.addError("config_file", "FILE_NOT_FOUND",
                       "Configuration file not found: " + config_file,
                       "Create configuration file or check file path",
                       "FILE_ACCESS");
        return false;
    }

    // Validate loading performance
    if (!validate_loading_performance(config_file, result)) {
        return false;
    }

    try {
        // Load configuration
        auto load_start = std::chrono::high_resolution_clock::now();
        YAML::Node config = YAML::LoadFile(config_file);
        auto load_end = std::chrono::high_resolution_clock::now();
        result.metrics.load_time = std::chrono::duration_cast<std::chrono::milliseconds>(load_end - load_start);

        // Validate configuration
        bool valid = validate_config(config, result);

        auto validation_end = std::chrono::high_resolution_clock::now();
        result.metrics.validation_time = std::chrono::duration_cast<std::chrono::milliseconds>(validation_end - validation_end);

        // Extract version information
        if (config["config_version"]) {
            result.config_version = config["config_version"].as<std::string>();
        }
        if (config["config_schema_version"]) {
            result.schema_version = config["config_schema_version"].as<std::string>();
        }

        return valid;

    } catch (const YAML::Exception& e) {
        result.addError("config_file", "YAML_PARSE_ERROR",
                       "Failed to parse YAML configuration: " + std::string(e.what()),
                       "Fix YAML syntax errors in configuration file",
                       "YAML_SYNTAX");
        return false;
    } catch (const std::exception& e) {
        result.addError("config_file", "UNKNOWN_ERROR",
                       "Unexpected error loading configuration: " + std::string(e.what()),
                       "Check file permissions and format",
                       "FILE_ACCESS");
        return false;
    }
}

bool Puzzle71ConfigValidator::validate_config(const YAML::Node& config, ValidationResult& result) {
    bool valid = true;

    // Initialize schema if not already done
    if (schema_.sections.empty()) {
        schema_ = ConfigurationSchema::get_v55_schema();
    }

    // Validate schema compliance
    if (!validate_schema_compliance(config, result)) {
        valid = false;
    }

    // Validate constitutional compliance
    if (!ConstitutionalValidator::validate_constitutional_compliance(config, result)) {
        valid = false;
    }

    // Validate GPU devices
    if (!GPUDeviceValidator::validate_gpu_devices(config, result)) {
        valid = false;
    }

    // Validate kernel configurations
    if (!GPUDeviceValidator::validate_kernel_configs(config, result)) {
        valid = false;
    }

    // Validate performance configuration
    if (!PerformanceValidator::validate_performance_config(config, result)) {
        valid = false;
    }

    // Validate data paths and permissions
    if (!validate_data_paths_and_permissions(config, result)) {
        valid = false;
    }

    // Validate security constraints
    if (!validate_security_constraints(config, result)) {
        valid = false;
    }

    // Calculate compliance score
    result.calculateComplianceScore();
    result.generateSummary();

    return valid;
}

bool Puzzle71ConfigValidator::validate_config_string(const std::string& config_string, ValidationResult& result) {
    try {
        YAML::Node config = YAML::Load(config_string);
        return validate_config(config, result);
    } catch (const YAML::Exception& e) {
        result.addError("config_string", "YAML_PARSE_ERROR",
                       "Failed to parse YAML configuration string: " + std::string(e.what()),
                       "Fix YAML syntax errors in configuration string",
                       "YAML_SYNTAX");
        return false;
    }
}

bool Puzzle71ConfigValidator::validate_loading_performance(const std::string& config_file, ValidationResult& result) {
    auto start_time = std::chrono::high_resolution_clock::now();

    // Measure loading time multiple times for accuracy
    const int iterations = 5;
    std::vector<std::chrono::milliseconds> load_times;

    for (int i = 0; i < iterations; ++i) {
        auto iter_start = std::chrono::high_resolution_clock::now();
        YAML::Node config = YAML::LoadFile(config_file);
        auto iter_end = std::chrono::high_resolution_clock::now();
        load_times.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(iter_end - iter_start));
    }

    // Calculate median load time
    std::sort(load_times.begin(), load_times.end());
    auto median_load_time = load_times[load_times.size() / 2];

    auto end_time = std::chrono::high_resolution_clock::now();
    result.metrics.load_time = median_load_time;

    // Check if loading is within performance threshold (<50ms)
    const std::chrono::milliseconds LOAD_TIME_THRESHOLD(50);
    if (median_load_time > LOAD_TIME_THRESHOLD) {
        result.addError("config_file", "LOAD_TIME_EXCEEDED",
                       "Configuration loading took " + std::to_string(median_load_time.count()) +
                       "ms, exceeds threshold of " + std::to_string(LOAD_TIME_THRESHOLD.count()) + "ms",
                       "Optimize configuration file size or structure",
                       "PERFORMANCE_LOADING");
        return false;
    }

    return true;
}

bool Puzzle71ConfigValidator::validate_schema_compliance(const YAML::Node& config, ValidationResult& result) {
    if (schema_.sections.empty()) {
        schema_ = ConfigurationSchema::get_v55_schema();
    }

    bool valid = true;

    // Validate root section
    for (const auto& section : schema_.sections) {
        if (section.name.empty()) {
            // Root level fields
            for (const auto& field : section.fields) {
                result.metrics.fields_validated++;

                if (field.required && !config[field.name]) {
                    result.addError(field.name, "MISSING_REQUIRED_FIELD",
                                   "Required field '" + field.name + "' is missing",
                                   "Add " + field.name + " to configuration",
                                   field.constraint_id);
                    valid = false;
                } else if (config[field.name]) {
                    // Validate field type and constraints
                    // (Type validation would be expanded in a full implementation)
                }
            }
        } else {
            // Nested sections
            if (section.required && !config[section.name]) {
                result.addError(section.name, "MISSING_REQUIRED_SECTION",
                               "Required section '" + section.name + "' is missing",
                               "Add " + section.name + " section to configuration",
                               "SECTION_REQUIRED");
                valid = false;
                continue;
            }

            if (config[section.name]) {
                // Validate section fields
                for (const auto& field : section.fields) {
                    result.metrics.fields_validated++;

                    if (field.required && !config[section.name][field.name]) {
                        result.addError(section.name + "." + field.name, "MISSING_REQUIRED_FIELD",
                                       "Required field '" + field.name + "' is missing in section '" + section.name + "'",
                                       "Add " + field.name + " to " + section.name + " section",
                                       field.constraint_id);
                        valid = false;
                    }
                }
            }
        }
    }

    return valid;
}

bool Puzzle71ConfigValidator::validate_data_paths_and_permissions(const YAML::Node& config, ValidationResult& result) {
    // Validate paths in configuration if they exist
    std::vector<std::string> path_fields = {
        "baseline_storage_directory",
        "checkpoint_output_dir",
        "telemetry_output_dir"
    };

    bool valid = true;
    for (const auto& field : path_fields) {
        // Look for path fields in various sections
        for (const std::string section_name : {"performance_config", "checkpointing", "telemetry", "validation_config"}) {
            if (config[section_name] && config[section_name][field]) {
                std::string path = config[section_name][field].as<std::string>();
                std::string full_path = std::string(section_name) + "." + field;

                // Check if path is absolute or relative
                if (path.empty()) {
                    result.addWarning(full_path, "EMPTY_PATH",
                                     "Path is empty", "Provide a valid path", "PATH_VALIDATION");
                } else if (path[0] != '/' && path[0] != '.' && path.find("://") == std::string::npos) {
                    result.addWarning(full_path, "RELATIVE_PATH",
                                     "Using relative path: " + path,
                                     "Consider using absolute paths for production deployments",
                                     "PATH_VALIDATION");
                }

                // Check if directory exists or can be created
                if (!std::filesystem::exists(path)) {
                    result.addWarning(full_path, "PATH_NOT_EXIST",
                                     "Path does not exist: " + path,
                                     "Create directory or ensure path is accessible",
                                     "PATH_VALIDATION");
                }
            }
        }
    }

    return valid;
}

bool Puzzle71ConfigValidator::validate_security_constraints(const YAML::Node& config, ValidationResult& result) {
    // Validate security-related constraints
    bool valid = true;

    // Check for hardcoded secrets or sensitive data
    std::vector<std::string> sensitive_patterns = {
        "password", "secret", "key", "token", "credential"
    };

    // This would be expanded to check for actual sensitive data patterns
    for (const auto& pattern : sensitive_patterns) {
        // Basic check - in a real implementation would use regex patterns
        if (config[pattern]) {
            result.addWarning(pattern, "POTENTIAL_SENSITIVE_DATA",
                             "Field '" + pattern + "' may contain sensitive data",
                             "Ensure sensitive data is properly encrypted or externalized",
                             "SECURITY_SENSITIVE");
        }
    }

    return valid;
}

std::string Puzzle71ConfigValidator::generate_validation_report(const ValidationResult& result) {
    std::stringstream ss;

    ss << "=== Puzzle71 Configuration Validation Report ===\n\n";
    ss << "Generated: " << result.validation_timestamp << "\n";
    ss << "Config Version: " << result.config_version << "\n";
    ss << "Schema Version: " << result.schema_version << "\n\n";

    ss << "Overall Status: " << (result.is_valid ? "✅ PASS" : "❌ FAIL") << "\n";
    ss << "Compliance Score: " << std::fixed << std::setprecision(2) << (result.compliance_score * 100) << "%\n\n";

    // Performance metrics
    ss << "Performance Metrics:\n";
    ss << "  Load Time: " << result.metrics.load_time.count() << "ms\n";
    ss << "  Validation Time: " << result.metrics.validation_time.count() << "ms\n";
    ss << "  Fields Validated: " << result.metrics.fields_validated << "\n";
    ss << "  Errors Detected: " << result.metrics.errors_detected << "\n";
    ss << "  Warnings Detected: " << result.metrics.warnings_detected << "\n\n";

    // Errors
    if (!result.errors.empty()) {
        ss << "ERRORS (" << result.errors.size() << "):\n";
        for (const auto& error : result.errors) {
            ss << "  ❌ [" << error.error_code << "] " << error.field_path << "\n";
            ss << "     " << error.error_message << "\n";
            if (!error.suggested_fix.empty()) {
                ss << "     💡 Suggestion: " << error.suggested_fix << "\n";
            }
            if (!error.constraint_id.empty()) {
                ss << "     📋 Constraint: " << error.constraint_id << "\n";
            }
            ss << "\n";
        }
    }

    // Warnings
    if (!result.warnings.empty()) {
        ss << "WARNINGS (" << result.warnings.size() << "):\n";
        for (const auto& warning : result.warnings) {
            ss << "  ⚠️  [" << warning.error_code << "] " << warning.field_path << "\n";
            ss << "     " << warning.error_message << "\n";
            if (!warning.suggested_fix.empty()) {
                ss << "     💡 Suggestion: " << warning.suggested_fix << "\n";
            }
            if (!warning.constraint_id.empty()) {
                ss << "     📋 Constraint: " << warning.constraint_id << "\n";
            }
            ss << "\n";
        }
    }

    ss << "Summary: " << result.summary << "\n";

    return ss.str();
}

bool Puzzle71ConfigValidator::passes_critical_validations(const ValidationResult& result) {
    // Check if all critical validations pass
    if (!result.is_valid) {
        return false;
    }

    // Check for critical error codes
    std::vector<std::string> critical_errors = {
        "VERSION_MISMATCH",
        "MISSING_GPU_DEVICES",
        "MISSING_KERNEL_CONFIGS",
        "DETERMINISTIC_DISABLED",
        "PRECISION_TOO_HIGH",
        "FILE_NOT_FOUND",
        "YAML_PARSE_ERROR"
    };

    for (const auto& error : result.errors) {
        if (std::find(critical_errors.begin(), critical_errors.end(), error.error_code) != critical_errors.end()) {
            return false;
        }
    }

    return true;
}

std::string Puzzle71ConfigValidator::get_field_path(const std::vector<std::string>& path_components) {
    std::string path;
    for (size_t i = 0; i < path_components.size(); ++i) {
        if (i > 0) path += ".";
        path += path_components[i];
    }
    return path;
}

// Validation utilities implementation
namespace validation_utils {

bool quick_validate(const std::string& config_file) {
    ValidationResult result;
    return Puzzle71ConfigValidator::validate_config_file(config_file, result);
}

bool validate_constitutional_compliance(const std::string& config_file) {
    try {
        YAML::Node config = YAML::LoadFile(config_file);
        ValidationResult result;
        return ConstitutionalValidator::validate_constitutional_compliance(config, result);
    } catch (...) {
        return false;
    }
}

bool validate_performance_targets(const std::string& config_file) {
    try {
        YAML::Node config = YAML::LoadFile(config_file);
        ValidationResult result;
        return PerformanceValidator::validate_performance_config(config, result);
    } catch (...) {
        return false;
    }
}

} // namespace validation_utils

// ConfigValidationGuard implementation
ConfigValidationGuard::ConfigValidationGuard(const std::string& config_file)
    : config_file_(config_file) {
    Puzzle71ConfigValidator::validate_config_file(config_file_, result_);
}

ConfigValidationGuard::~ConfigValidationGuard() {
    // RAII cleanup - could log validation results here
}

std::string ConfigValidationGuard::get_report() const {
    return Puzzle71ConfigValidator::generate_validation_report(result_);
}

} // namespace config
} // namespace keyhunt
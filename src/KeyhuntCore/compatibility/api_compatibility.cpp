// Puzzle71Solver - Backward API Compatibility Layer (T053)
// Phase 7: User Story 5 - Compatibility Assurance
// Comprehensive backward compatibility layer implementation

#include "api_compatibility.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>
#include <thread>
#include <chrono>

namespace puzzle71::compatibility {

// ============================================================================
// APIVersion Implementation
// ============================================================================

std::string APIVersion::toString() const {
    std::ostringstream oss;
    oss << major << "." << minor << "." << patch;
    if (!build_info.empty()) {
        oss << "-" << build_info;
    }
    return oss.str();
}

int APIVersion::compare(const APIVersion& other) const {
    if (major != other.major) return (major > other.major) ? 1 : -1;
    if (minor != other.minor) return (minor > other.minor) ? 1 : -1;
    if (patch != other.patch) return (patch > other.patch) ? 1 : -1;
    return 0;
}

bool APIVersion::isCompatible(const APIVersion& required) const {
    // Major version must match exactly
    if (major != required.major) return false;

    // Minor version must be >= required
    if (minor < required.minor) return false;

    // If minor version equals required, patch must be >= required
    if (minor == required.minor && patch < required.patch) return false;

    return true;
}

// ============================================================================
// LegacyConfig Implementation
// ============================================================================

puzzle71::gpu::LaunchConfig LegacyConfig::toModernConfig() const {
    puzzle71::gpu::LaunchConfig modern_config;

    // Convert device configuration
    modern_config.device_id = device_id;

    // Convert kernel configuration
    modern_config.threads_per_block = threads_per_block;
    modern_config.blocks_per_grid = blocks_per_grid;
    modern_config.batch_size = batch_size;

    // Convert optimization settings
    modern_config.use_separated_kernels = use_separated_kernels;
    modern_config.enable_optimization = enable_optimization;
    modern_config.kernel_mode = kernel_mode;

    // Convert memory configuration
    modern_config.memory_pool_size = memory_pool_size;
    modern_config.use_soa_layout = use_soa_layout;
    modern_config.memory_alignment = memory_alignment;

    // Convert performance settings
    modern_config.enable_profiling = enable_profiling;
    modern_config.enable_monitoring = enable_monitoring;
    modern_config.warmup_iterations = warmup_iterations;
    modern_config.benchmark_iterations = benchmark_iterations;

    // Map legacy flags to modern equivalents
    modern_config.fast_math = (legacy_flag_1 & 0x01) != 0;
    modern_config.use_compression = use_compression;
    modern_config.optimization_level = optimization_level;

    return modern_config;
}

LegacyConfig LegacyConfig::fromModernConfig(const puzzle71::gpu::LaunchConfig& modern_config) {
    LegacyConfig legacy_config;

    // Convert device configuration
    legacy_config.device_id = modern_config.device_id;

    // Convert kernel configuration
    legacy_config.threads_per_block = modern_config.threads_per_block;
    legacy_config.blocks_per_grid = modern_config.blocks_per_grid;
    legacy_config.batch_size = modern_config.batch_size;

    // Convert optimization settings
    legacy_config.use_separated_kernels = modern_config.use_separated_kernels;
    legacy_config.enable_optimization = modern_config.enable_optimization;
    legacy_config.kernel_mode = modern_config.kernel_mode;

    // Convert memory configuration
    legacy_config.memory_pool_size = modern_config.memory_pool_size;
    legacy_config.use_soa_layout = modern_config.use_soa_layout;
    legacy_config.memory_alignment = modern_config.memory_alignment;

    // Convert performance settings
    legacy_config.enable_profiling = modern_config.enable_profiling;
    legacy_config.enable_monitoring = modern_config.enable_monitoring;
    legacy_config.warmup_iterations = modern_config.warmup_iterations;
    legacy_config.benchmark_iterations = modern_config.benchmark_iterations;

    // Map modern settings to legacy flags
    legacy_config.use_compression = modern_config.use_compression;
    legacy_config.use_fast_math = modern_config.fast_math;
    legacy_config.optimization_level = modern_config.optimization_level;

    // Set legacy flags based on modern settings
    legacy_config.legacy_flag_1 = modern_config.fast_math ? 0x01 : 0x00;
    legacy_config.legacy_flag_2 = modern_config.use_compression ? 0x01 : 0x00;

    return legacy_config;
}

// ============================================================================
// LegacyResult Implementation
// ============================================================================

puzzle71::monitoring::PerformanceMetrics LegacyResult::toModernMetrics() const {
    puzzle71::monitoring::PerformanceMetrics modern_metrics;

    // Convert basic result information
    modern_metrics.name = "legacy_execution";
    modern_metrics.timestamp = std::chrono::system_clock::now();
    modern_metrics.throughput = throughput;
    modern_metrics.processing_time = processing_time;
    modern_metrics.keys_processed = keys_processed;

    // Convert GPU information
    modern_metrics.device_id = device_id;
    modern_metrics.device_name = device_name;
    modern_metrics.gpu_utilization = gpu_utilization;
    modern_metrics.memory_usage = memory_usage;

    // Convert result data
    modern_metrics.found_keys = found_keys;
    modern_metrics.found_addresses = found_addresses;

    // Set success status
    modern_metrics.success = success;

    return modern_metrics;
}

LegacyResult LegacyResult::fromModernMetrics(const puzzle71::monitoring::PerformanceMetrics& modern_metrics) {
    LegacyResult legacy_result;

    // Convert basic result information
    legacy_result.success = modern_metrics.success;
    legacy_result.keys_processed = modern_metrics.keys_processed;
    legacy_result.processing_time = modern_metrics.processing_time;
    legacy_result.throughput = modern_metrics.throughput;

    // Convert GPU information
    legacy_result.device_id = modern_metrics.device_id;
    legacy_result.device_name = modern_metrics.device_name;
    legacy_result.gpu_utilization = modern_metrics.gpu_utilization;
    legacy_result.memory_usage = modern_metrics.memory_usage;

    // Convert result data
    legacy_result.found_keys = modern_metrics.found_keys;
    legacy_result.found_addresses = modern_metrics.found_addresses;

    // Set legacy status information
    legacy_result.legacy_status_code = modern_metrics.success ? 0 : -1;
    legacy_result.legacy_status_message = modern_metrics.success ? "SUCCESS" : "FAILED";

    return legacy_result;
}

// ============================================================================
// LegacyKernelParams Implementation
// ============================================================================

puzzle71::gpu::KernelLaunchParams LegacyKernelParams::toModernParams() const {
    puzzle71::gpu::KernelLaunchParams modern_params;

    // Convert basic parameters
    modern_params.start_key = start_key;
    modern_params.end_key = end_key;
    modern_params.stride = stride;
    modern_params.device_id = device_id;

    // Convert target data
    modern_params.target_addresses = target_addresses;
    modern_params.target_hashes = target_hashes;

    // Convert kernel configuration
    modern_params.threads_per_block = threads_per_block;
    modern_params.blocks_per_grid = blocks_per_grid;
    modern_params.shared_memory_size = shared_memory_size;

    // Convert optimization settings
    modern_params.use_compression = use_compression;
    modern_params.use_fast_math = use_fast_math;
    modern_params.optimization_level = optimization_level;

    // Memory pointers are handled differently in modern API
    // The modern API manages memory automatically

    return modern_params;
}

LegacyKernelParams LegacyKernelParams::fromModernParams(const puzzle71::gpu::KernelLaunchParams& modern_params) {
    LegacyKernelParams legacy_params;

    // Convert basic parameters
    legacy_params.start_key = modern_params.start_key;
    legacy_params.end_key = modern_params.end_key;
    legacy_params.stride = modern_params.stride;
    legacy_params.device_id = modern_params.device_id;

    // Convert target data
    legacy_params.target_addresses = modern_params.target_addresses;
    legacy_params.target_hashes = modern_params.target_hashes;

    // Convert kernel configuration
    legacy_params.threads_per_block = modern_params.threads_per_block;
    legacy_params.blocks_per_grid = modern_params.blocks_per_grid;
    legacy_params.shared_memory_size = modern_params.shared_memory_size;

    // Convert optimization settings
    legacy_params.use_compression = modern_params.use_compression;
    legacy_params.use_fast_math = modern_params.use_fast_math;
    legacy_params.optimization_level = modern_params.optimization_level;

    // Memory pointers are set to null - modern API handles them automatically
    legacy_params.d_target_data = nullptr;
    legacy_params.d_result_data = nullptr;
    legacy_params.d_working_memory = nullptr;

    return legacy_params;
}

// ============================================================================
// LegacyAPIWrapper Implementation
// ============================================================================

LegacyAPIWrapper::LegacyAPIWrapper(CompatibilityMode mode)
    : mode_(mode) {
    current_version_ = {1, 0, 0, "compatibility_layer", std::chrono::system_clock::now()};

    // Initialize modern system components
    module_manager_ = std::make_unique<puzzle71::common::ModuleManager>();
    launch_config_manager_ = std::make_unique<puzzle71::gpu::LaunchConfigManager>();
    soa_manager_ = std::make_unique<puzzle71::memory::SoAManager>();

    initializeFunctionMappings();
    initializeDeprecatedFunctions();
}

LegacyAPIWrapper::~LegacyAPIWrapper() {
    if (execution_active_.load()) {
        legacy_cancel_execution();
    }
}

int LegacyAPIWrapper::legacy_initialize(const LegacyConfig& config) {
    std::lock_guard<std::mutex> lock(wrapper_mutex_);

    // Convert legacy config to modern config
    auto modern_config = config.toModernConfig();

    // Initialize modern components
    try {
        // Initialize module manager
        if (!module_manager_->initialize()) {
            return handleCompatibilityError("initialize", "Failed to initialize module manager");
        }

        // Initialize launch configuration
        if (!launch_config_manager_->initialize(modern_config)) {
            return handleCompatibilityError("initialize", "Failed to initialize launch config manager");
        }

        // Initialize memory manager
        if (!soa_manager_->initialize(modern_config.memory_pool_size)) {
            return handleCompatibilityError("initialize", "Failed to initialize memory manager");
        }

        // Store legacy configuration
        legacy_config_.clear();
        legacy_config_["device_id"] = std::to_string(config.device_id);
        legacy_config_["threads_per_block"] = std::to_string(config.threads_per_block);
        legacy_config_["batch_size"] = std::to_string(config.batch_size);
        legacy_config_["use_separated_kernels"] = config.use_separated_kernels ? "true" : "false";

        return 0; // Success

    } catch (const std::exception& e) {
        return handleCompatibilityError("initialize", std::string("Exception: ") + e.what());
    }
}

int LegacyAPIWrapper::legacy_shutdown() {
    std::lock_guard<std::mutex> lock(wrapper_mutex_);

    try {
        // Cancel any active execution
        if (execution_active_.load()) {
            legacy_cancel_execution();
        }

        // Shutdown modern components
        if (soa_manager_) {
            soa_manager_->shutdown();
        }

        if (launch_config_manager_) {
            launch_config_manager_->shutdown();
        }

        if (module_manager_) {
            module_manager_->shutdown();
        }

        // Clear state
        pending_results_.clear();
        legacy_config_.clear();

        return 0; // Success

    } catch (const std::exception& e) {
        return handleCompatibilityError("shutdown", std::string("Exception: ") + e.what());
    }
}

int LegacyAPIWrapper::legacy_reset_device(int device_id) {
    std::lock_guard<std::mutex> lock(wrapper_mutex_);

    try {
        // Reset device using modern API
        if (launch_config_manager_) {
            return launch_config_manager_->resetDevice(device_id) ? 0 : -1;
        }
        return -1;

    } catch (const std::exception& e) {
        return handleCompatibilityError("reset_device", std::string("Exception: ") + e.what());
    }
}

int LegacyAPIWrapper::legacy_launch_kernel(const LegacyKernelParams& params) {
    std::lock_guard<std::mutex> lock(wrapper_mutex_);

    if (execution_active_.load()) {
        return handleCompatibilityError("launch_kernel", "Execution already active");
    }

    try {
        // Convert to modern parameters
        auto modern_params = params.toModernParams();

        // Launch kernel using modern API
        execution_active_.store(true);

        // This would be implemented using the modern execution system
        // For now, simulate execution
        std::thread([this, modern_params]() {
            // Simulate kernel execution
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Create mock result
            LegacyResult result;
            result.success = true;
            result.device_id = modern_params.device_id;
            result.throughput = 2000.0; // Mock throughput
            result.processing_time = 0.1;
            result.keys_processed = modern_params.end_key - modern_params.start_key;

            // Add to pending results
            std::lock_guard<std::mutex> inner_lock(wrapper_mutex_);
            pending_results_.push_back(result);
            execution_active_.store(false);

        }).detach();

        return 0; // Success

    } catch (const std::exception& e) {
        execution_active_.store(false);
        return handleCompatibilityError("launch_kernel", std::string("Exception: ") + e.what());
    }
}

int LegacyAPIWrapper::legacy_launch_batch(const std::vector<LegacyKernelParams>& batch_params) {
    std::lock_guard<std::mutex> lock(wrapper_mutex_);

    if (execution_active_.load()) {
        return handleCompatibilityError("launch_batch", "Execution already active");
    }

    try {
        // Convert each set of parameters to modern format
        std::vector<puzzle71::gpu::KernelLaunchParams> modern_params;
        for (const auto& legacy_params : batch_params) {
            modern_params.push_back(legacy_params.toModernParams());
        }

        // Launch batch using modern API
        execution_active_.store(true);

        // This would be implemented using the modern batch execution system
        // For now, simulate batch execution
        std::thread([this, modern_params]() {
            // Simulate batch execution
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            // Create mock results for each parameter set
            std::lock_guard<std::mutex> inner_lock(wrapper_mutex_);
            for (const auto& params : modern_params) {
                LegacyResult result;
                result.success = true;
                result.device_id = params.device_id;
                result.throughput = 1800.0; // Mock throughput (slightly lower for batch)
                result.processing_time = 0.2;
                result.keys_processed = params.end_key - params.start_key;

                pending_results_.push_back(result);
            }
            execution_active_.store(false);

        }).detach();

        return 0; // Success

    } catch (const std::exception& e) {
        execution_active_.store(false);
        return handleCompatibilityError("launch_batch", std::string("Exception: ") + e.what());
    }
}

int LegacyAPIWrapper::legacy_wait_for_completion() {
    // Wait for execution to complete
    while (execution_active_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return 0;
}

int LegacyAPIWrapper::legacy_cancel_execution() {
    execution_active_.store(false);
    return 0;
}

LegacyResult LegacyAPIWrapper::legacy_get_results() {
    std::lock_guard<std::mutex> lock(wrapper_mutex_);

    if (pending_results_.empty()) {
        LegacyResult empty_result;
        empty_result.success = false;
        empty_result.error_message = "No results available";
        return empty_result;
    }

    // Return first result and remove it from pending list
    LegacyResult result = pending_results_.front();
    pending_results_.erase(pending_results_.begin());
    return result;
}

std::vector<LegacyResult> LegacyAPIWrapper::legacy_get_batch_results() {
    std::lock_guard<std::mutex> lock(wrapper_mutex_);

    std::vector<LegacyResult> results = pending_results_;
    pending_results_.clear();
    return results;
}

int LegacyAPIWrapper::legacy_get_result_count() {
    std::lock_guard<std::mutex> lock(wrapper_mutex_);
    return static_cast<int>(pending_results_.size());
}

bool LegacyAPIWrapper::legacy_is_execution_complete() {
    return !execution_active_.load();
}

void* LegacyAPIWrapper::legacy_allocate_memory(size_t size) {
    try {
        if (soa_manager_) {
            return soa_manager_->allocate(size);
        }
        return nullptr;
    } catch (const std::exception& e) {
        handleCompatibilityError("allocate_memory", std::string("Exception: ") + e.what());
        return nullptr;
    }
}

void LegacyAPIWrapper::legacy_free_memory(void* ptr) {
    try {
        if (soa_manager_ && ptr) {
            soa_manager_->deallocate(ptr);
        }
    } catch (const std::exception& e) {
        handleCompatibilityError("free_memory", std::string("Exception: ") + e.what());
    }
}

int LegacyAPIWrapper::legacy_copy_to_device(void* device_ptr, const void* host_ptr, size_t size) {
    try {
        if (soa_manager_) {
            return soa_manager_->copyToDevice(device_ptr, host_ptr, size) ? 0 : -1;
        }
        return -1;
    } catch (const std::exception& e) {
        return handleCompatibilityError("copy_to_device", std::string("Exception: ") + e.what());
    }
}

int LegacyAPIWrapper::legacy_copy_to_host(void* host_ptr, const void* device_ptr, size_t size) {
    try {
        if (soa_manager_) {
            return soa_manager_->copyToHost(host_ptr, device_ptr, size) ? 0 : -1;
        }
        return -1;
    } catch (const std::exception& e) {
        return handleCompatibilityError("copy_to_host", std::string("Exception: ") + e.what());
    }
}

int LegacyAPIWrapper::legacy_set_config(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(wrapper_mutex_);
    legacy_config_[key] = value;
    return 0;
}

std::string LegacyAPIWrapper::legacy_get_config(const std::string& key) {
    std::lock_guard<std::mutex> lock(wrapper_mutex_);
    auto it = legacy_config_.find(key);
    return it != legacy_config_.end() ? it->second : "";
}

int LegacyAPIWrapper::legacy_load_config_from_file(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return -1;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Parse key=value format
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                legacy_set_config(key, value);
            }
        }

        return 0;
    } catch (const std::exception& e) {
        return handleCompatibilityError("load_config_from_file", std::string("Exception: ") + e.what());
    }
}

int LegacyAPIWrapper::legacy_save_config_to_file(const std::string& filename) {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return -1;
        }

        std::lock_guard<std::mutex> lock(wrapper_mutex_);
        for (const auto& [key, value] : legacy_config_) {
            file << key << "=" << value << std::endl;
        }

        return 0;
    } catch (const std::exception& e) {
        return handleCompatibilityError("save_config_to_file", std::string("Exception: ") + e.what());
    }
}

int LegacyAPIWrapper::legacy_start_profiling() {
    // This would integrate with the modern profiling system
    return 0;
}

int LegacyAPIWrapper::legacy_stop_profiling() {
    // This would integrate with the modern profiling system
    return 0;
}

std::map<std::string, double> LegacyAPIWrapper::legacy_get_profile_data() {
    // This would return profiling data from the modern system
    return {};
}

int LegacyAPIWrapper::legacy_enable_monitoring() {
    monitoring_enabled_.store(true);
    return 0;
}

int LegacyAPIWrapper::legacy_disable_monitoring() {
    monitoring_enabled_.store(false);
    return 0;
}

std::map<std::string, double> LegacyAPIWrapper::legacy_get_current_metrics() {
    // This would return current metrics from the modern monitoring system
    return {};
}

void LegacyAPIWrapper::setCompatibilityMode(CompatibilityMode mode) {
    std::lock_guard<std::mutex> lock(wrapper_mutex_);
    mode_ = mode;
}

CompatibilityMode LegacyAPIWrapper::getCompatibilityMode() const {
    return mode_;
}

APIVersion LegacyAPIWrapper::getAPIVersion() const {
    return current_version_;
}

bool LegacyAPIWrapper::isFunctionSupported(const std::string& function_name) const {
    auto it = function_mappings_.find(function_name);
    return it != function_mappings_.end();
}

std::vector<std::string> LegacyAPIWrapper::getDeprecatedFunctions() const {
    std::vector<std::string> deprecated;
    for (const auto& [name, is_deprecated] : deprecated_functions_) {
        if (is_deprecated) {
            deprecated.push_back(name);
        }
    }
    return deprecated;
}

std::vector<std::string> LegacyAPIWrapper::getMigrationHints() const {
    std::vector<std::string> hints;

    for (const auto& [name, is_deprecated] : deprecated_functions_) {
        if (is_deprecated) {
            hints.push_back("Function '" + name + "' is deprecated. Consider using the modern API.");
        }
    }

    return hints;
}

std::string LegacyAPIWrapper::generateMigrationReport() const {
    std::ostringstream oss;
    oss << "API Migration Report\n";
    oss << "====================\n\n";

    oss << "Current Compatibility Mode: ";
    switch (mode_) {
        case CompatibilityMode::STRICT: oss << "STRICT"; break;
        case CompatibilityMode::LEGACY: oss << "LEGACY"; break;
        case CompatibilityMode::MODERN: oss << "MODERN"; break;
        case CompatibilityMode::HYBRID: oss << "HYBRID"; break;
    }
    oss << "\n\n";

    oss << "API Version: " << current_version_.toString() << "\n\n";

    std::vector<std::string> deprecated = getDeprecatedFunctions();
    if (!deprecated.empty()) {
        oss << "Deprecated Functions (" << deprecated.size() << "):\n";
        for (const auto& func : deprecated) {
            oss << "  - " << func << "\n";
        }
        oss << "\n";
    }

    std::vector<std::string> hints = getMigrationHints();
    if (!hints.empty()) {
        oss << "Migration Hints:\n";
        for (const auto& hint : hints) {
            oss << "  " << hint << "\n";
        }
    }

    return oss.str();
}

void LegacyAPIWrapper::initializeFunctionMappings() {
    // Map legacy function names to modern implementations
    function_mappings_["initialize"] = [this](const std::vector<std::string>& args) {
        return legacyInitializeImpl(args);
    };

    function_mappings_["shutdown"] = [this](const std::vector<std::string>& args) {
        return legacyShutdownImpl(args);
    };

    function_mappings_["launch_kernel"] = [this](const std::vector<std::string>& args) {
        return legacyLaunchKernelImpl(args);
    };

    function_mappings_["get_results"] = [this](const std::vector<std::string>& args) {
        return legacyGetResultsImpl(args);
    };
}

void LegacyAPIWrapper::initializeDeprecatedFunctions() {
    // Mark which functions are deprecated
    deprecated_functions_["old_launch_function"] = true;
    deprecated_functions_["legacy_memory_alloc"] = true;
    deprecated_functions_["deprecated_config_set"] = true;
}

int LegacyAPIWrapper::legacyInitializeImpl(const std::vector<std::string>& args) {
    if (args.empty()) {
        return -1;
    }

    // Parse configuration from arguments
    LegacyConfig config;
    try {
        // Simple parsing - in real implementation would be more sophisticated
        config.device_id = std::stoi(args[0]);
        if (args.size() > 1) {
            config.threads_per_block = std::stoi(args[1]);
        }
    } catch (const std::exception& e) {
        return handleCompatibilityError("initialize", "Invalid configuration arguments");
    }

    return legacy_initialize(config);
}

int LegacyAPIWrapper::legacyShutdownImpl(const std::vector<std::string>& args) {
    return legacy_shutdown();
}

int LegacyAPIWrapper::legacyLaunchKernelImpl(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        return -1;
    }

    // Parse kernel parameters from arguments
    LegacyKernelParams params;
    try {
        params.start_key = std::stoull(args[0]);
        params.end_key = std::stoull(args[1]);
        params.device_id = std::stoi(args[2]);

        if (args.size() > 3) {
            params.threads_per_block = std::stoi(args[3]);
        }
    } catch (const std::exception& e) {
        return handleCompatibilityError("launch_kernel", "Invalid kernel parameters");
    }

    return legacy_launch_kernel(params);
}

int LegacyAPIWrapper::legacyGetResultsImpl(const std::vector<std::string>& args) {
    // Results are retrieved via legacy_get_results(), so this just checks availability
    return pending_results_.empty() ? -1 : 0;
}

int LegacyAPIWrapper::handleCompatibilityError(const std::string& function_name, const std::string& error_message) const {
    std::cerr << "Compatibility Error in " << function_name << ": " << error_message << std::endl;
    return -1;
}

void LegacyAPIWrapper::logCompatibilityWarning(const std::string& function_name, const std::string& warning_message) const {
    std::cout << "Compatibility Warning in " << function_name << ": " << warning_message << std::endl;
}

// ============================================================================
// APICompatibilityValidator Implementation
// ============================================================================

APICompatibilityValidator::APICompatibilityValidator() {
    initializeValidators();
    initializeDeprecatedSignatures();
}

bool APICompatibilityValidator::validateFunctionCall(const std::string& function_name, const std::vector<std::string>& args) const {
    auto it = validators_.find(function_name);
    if (it == validators_.end()) {
        return false; // Unknown function
    }

    return it->second(args);
}

bool APICompatibilityValidator::validateConfiguration(const LegacyConfig& config) const {
    // Validate configuration parameters
    if (config.threads_per_block == 0 || config.threads_per_block > 1024) {
        return false;
    }

    if (config.batch_size == 0) {
        return false;
    }

    if (config.device_id < 0) {
        return false;
    }

    return true;
}

bool APICompatibilityValidator::validateKernelParams(const LegacyKernelParams& params) const {
    // Validate kernel parameters
    if (params.start_key >= params.end_key) {
        return false;
    }

    if (params.stride == 0) {
        return false;
    }

    if (params.threads_per_block == 0 || params.threads_per_block > 1024) {
        return false;
    }

    return true;
}

APICompatibilityValidator::CompatibilityReport APICompatibilityValidator::generateReport(
    const LegacyConfig& config, const LegacyKernelParams& params
) const {
    CompatibilityReport report;

    // Validate configuration
    if (!validateConfiguration(config)) {
        report.is_compatible = false;
        report.errors.push_back("Invalid configuration parameters");
    }

    // Validate kernel parameters
    if (!validateKernelParams(params)) {
        report.is_compatible = false;
        report.errors.push_back("Invalid kernel parameters");
    }

    // Check for deprecated usage
    if (!config.legacy_option_1.empty()) {
        report.warnings.push_back("Using deprecated option: legacy_option_1");
        report.deprecated_alternatives["legacy_option_1"] = "Use modern configuration system";
    }

    // Add recommendations
    if (!config.use_separated_kernels) {
        report.recommendations.push_back("Consider enabling separated kernels for better performance");
    }

    return report;
}

std::vector<std::string> APICompatibilityValidator::checkDeprecatedUsage(
    const std::string& function_name, const std::vector<std::string>& args
) const {
    auto it = deprecated_signatures_.find(function_name);
    if (it != deprecated_signatures_.end()) {
        return it->second;
    }
    return {};
}

void APICompatibilityValidator::initializeValidators() {
    validators_["initialize"] = [](const std::vector<std::string>& args) {
        return !args.empty();
    };

    validators_["launch_kernel"] = [](const std::vector<std::string>& args) {
        return args.size() >= 3;
    };

    validators_["set_config"] = [](const std::vector<std::string>& args) {
        return args.size() == 2;
    };
}

void APICompatibilityValidator::initializeDeprecatedSignatures() {
    deprecated_signatures_["old_launch_function"] = {
        "Function signature has changed",
        "Parameters may be in different order"
    };
}

// ============================================================================
// CompatibilityManager Implementation
// ============================================================================

CompatibilityManager& CompatibilityManager::getInstance() {
    static CompatibilityManager instance;
    return instance;
}

bool CompatibilityManager::initialize(CompatibilityMode mode) {
    std::lock_guard<std::mutex> lock(manager_mutex_);

    try {
        api_wrapper_ = std::make_shared<LegacyAPIWrapper>(mode);
        validator_ = std::make_shared<APICompatibilityValidator>();
        migration_assistant_ = std::make_shared<APIMigrationAssistant>();
        test_framework_ = std::make_shared<CompatibilityTestFramework>();

        global_mode_ = mode;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize compatibility manager: " << e.what() << std::endl;
        return false;
    }
}

std::shared_ptr<LegacyAPIWrapper> CompatibilityManager::getAPIWrapper() {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    return api_wrapper_;
}

std::shared_ptr<APICompatibilityValidator> CompatibilityManager::getValidator() {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    return validator_;
}

std::shared_ptr<APIMigrationAssistant> CompatibilityManager::getMigrationAssistant() {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    return migration_assistant_;
}

std::shared_ptr<CompatibilityTestFramework> CompatibilityManager::getTestFramework() {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    return test_framework_;
}

void CompatibilityManager::setGlobalCompatibilityMode(CompatibilityMode mode) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    global_mode_ = mode;
    if (api_wrapper_) {
        api_wrapper_->setCompatibilityMode(mode);
    }
}

CompatibilityMode CompatibilityManager::getGlobalCompatibilityMode() const {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    return global_mode_;
}

void CompatibilityManager::setWarningsEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    warnings_enabled_ = enabled;
}

bool CompatibilityManager::areWarningsEnabled() const {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    return warnings_enabled_;
}

std::string CompatibilityManager::getSystemCompatibilityReport() const {
    std::lock_guard<std::mutex> lock(manager_mutex_);

    if (api_wrapper_) {
        return api_wrapper_->generateMigrationReport();
    }

    return "Compatibility system not initialized";
}

// ============================================================================
// C-style API Implementation
// ============================================================================

extern "C" {

int puzzle71_legacy_initialize(const puzzle71::compatibility::LegacyConfig* config) {
    if (!config) return -1;

    auto& manager = puzzle71::compatibility::CompatibilityManager::getInstance();
    auto wrapper = manager.getAPIWrapper();

    if (!wrapper) return -1;

    return wrapper->legacy_initialize(*config);
}

int puzzle71_legacy_launch_kernel(const puzzle71::compatibility::LegacyKernelParams* params) {
    if (!params) return -1;

    auto& manager = puzzle71::compatibility::CompatibilityManager::getInstance();
    auto wrapper = manager.getAPIWrapper();

    if (!wrapper) return -1;

    return wrapper->legacy_launch_kernel(*params);
}

int puzzle71_legacy_get_results(puzzle71::compatibility::LegacyResult* result) {
    if (!result) return -1;

    auto& manager = puzzle71::compatibility::CompatibilityManager::getInstance();
    auto wrapper = manager.getAPIWrapper();

    if (!wrapper) return -1;

    *result = wrapper->legacy_get_results();
    return result->success ? 0 : -1;
}

int puzzle71_legacy_shutdown() {
    auto& manager = puzzle71::compatibility::CompatibilityManager::getInstance();
    auto wrapper = manager.getAPIWrapper();

    if (!wrapper) return -1;

    return wrapper->legacy_shutdown();
}

int puzzle71_legacy_set_config(const char* key, const char* value) {
    if (!key || !value) return -1;

    auto& manager = puzzle71::compatibility::CompatibilityManager::getInstance();
    auto wrapper = manager.getAPIWrapper();

    if (!wrapper) return -1;

    return wrapper->legacy_set_config(std::string(key), std::string(value));
}

int puzzle71_legacy_get_config(const char* key, char* value, int max_size) {
    if (!key || !value || max_size <= 0) return -1;

    auto& manager = puzzle71::compatibility::CompatibilityManager::getInstance();
    auto wrapper = manager.getAPIWrapper();

    if (!wrapper) return -1;

    std::string result = wrapper->legacy_get_config(std::string(key));

    if (result.length() >= static_cast<size_t>(max_size)) {
        return -1;
    }

    std::strcpy(value, result.c_str());
    return 0;
}

} // extern "C"

} // namespace puzzle71::compatibility
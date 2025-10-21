// Puzzle71 Technical Debt Repair - Kernel Configuration Validation Implementation (T030)
// Comprehensive validation for kernel launch configurations with constitutional compliance

#include "kernel_config_validator.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace keyhunt {
namespace validation {

// DeviceCapabilityValidator Implementation

bool DeviceCapabilityValidator::validate_device_compatibility(
    int device_id,
    const integration::IntegratedLaunchConfig& config,
    KernelValidationResult& result
) {
    cudaDeviceProp props{};
    if (cudaGetDeviceProperties(&props, device_id) != cudaSuccess) {
        result.addError("device_query", "Failed to get device properties");
        return false;
    }

    bool compatible = true;

    // Validate thread limits
    if (!validate_thread_limits(props, config, result)) {
        compatible = false;
    }

    // Validate memory limits
    if (!validate_memory_limits(props, config, result)) {
        compatible = false;
    }

    // Validate compute capability
    if (!validate_compute_capability(props, config, result)) {
        compatible = false;
    }

    if (compatible) {
        result.addInfo("device_compatibility",
                      "Configuration compatible with " + std::string(props.name) +
                      " (SM " + std::to_string(props.major) + "." + std::to_string(props.minor) + ")");
    }

    return compatible;
}

bool DeviceCapabilityValidator::can_launch_kernel(
    int device_id,
    dim3 grid_dim,
    dim3 block_dim,
    size_t shared_memory_size
) {
    cudaDeviceProp props{};
    if (cudaGetDeviceProperties(&props, device_id) != cudaSuccess) {
        return false;
    }

    // Check thread count
    uint32_t threads_per_block = block_dim.x * block_dim.y * block_dim.z;
    if (threads_per_block > props.maxThreadsPerBlock) {
        return false;
    }

    // Check shared memory
    if (shared_memory_size > props.sharedMemPerBlock) {
        return false;
    }

    // Check grid dimensions
    if (grid_dim.x > props.maxGridSize[0] ||
        grid_dim.y > props.maxGridSize[1] ||
        grid_dim.z > props.maxGridSize[2]) {
        return false;
    }

    return true;
}

bool DeviceCapabilityValidator::validate_thread_limits(
    const cudaDeviceProp& props,
    const integration::IntegratedLaunchConfig& config,
    KernelValidationResult& result
) {
    bool valid = true;
    uint32_t threads_per_block = config.block_dim.x * config.block_dim.y * config.block_dim.z;

    // Check threads per block limit
    if (threads_per_block > props.maxThreadsPerBlock) {
        result.addError("threads_per_block",
                       "Threads per block (" + std::to_string(threads_per_block) +
                       ") exceeds device limit (" + std::to_string(props.maxThreadsPerBlock) + ")");
        valid = false;
    } else if (threads_per_block < 32) {
        result.addWarning("threads_per_block",
                         "Threads per block (" + std::to_string(threads_per_block) +
                         ") is less than warp size (32)");
    }

    // Check warp size alignment
    if (threads_per_block % 32 != 0) {
        result.addWarning("threads_per_block",
                         "Threads per block (" + std::to_string(threads_per_block) +
                         ") is not a multiple of warp size (32)");
    }

    // Check minimum thread requirements
    if (threads_per_block < 32) {
        result.addError("threads_per_block",
                       "Threads per block must be at least 32 for efficient execution");
        valid = false;
    }

    return valid;
}

bool DeviceCapabilityValidator::validate_memory_limits(
    const cudaDeviceProp& props,
    const integration::IntegratedLaunchConfig& config,
    KernelValidationResult& result
) {
    bool valid = true;

    // Check shared memory limit
    if (config.shared_memory_size > props.sharedMemPerBlock) {
        result.addError("shared_memory_size",
                       "Shared memory size (" + std::to_string(config.shared_memory_size) +
                       ") exceeds device limit (" + std::to_string(props.sharedMemPerBlock) + ")");
        valid = false;
    }

    // Check register usage estimate
    uint32_t threads_per_block = config.block_dim.x * config.block_dim.y * config.block_dim.z;
    uint32_t estimated_registers = config.static_config.registers_per_thread * threads_per_block;

    if (estimated_registers > 65536) {  // Conservative register limit
        result.addWarning("register_usage",
                         "Estimated register usage (" + std::to_string(estimated_registers) +
                         ") may exceed device limits");
    }

    // Check memory alignment
    if (config.static_config.memory_alignment > 0 &&
        config.static_config.memory_alignment % 128 != 0) {
        result.addWarning("memory_alignment",
                         "Memory alignment should be multiple of 128 for optimal performance");
    }

    return valid;
}

bool DeviceCapabilityValidator::validate_compute_capability(
    const cudaDeviceProp& props,
    const integration::IntegratedLaunchConfig& config,
    KernelValidationResult& result
) {
    int compute_cap = props.major * 10 + props.minor;

    // Check minimum compute capability
    if (compute_cap < 35) {
        result.addError("compute_capability",
                       "Compute capability " + std::to_string(compute_cap) +
                       " is below minimum requirement (3.5)");
        return false;
    }

    // Architecture-specific checks
    if (props.major >= 7) {
        // Turing and later support advanced features
        result.addInfo("compute_capability",
                      "Modern GPU detected with compute capability " + std::to_string(compute_cap));
    } else {
        result.addWarning("compute_capability",
                         "Older GPU detected, some optimizations may not be available");
    }

    return true;
}

// ConstitutionalComplianceValidator Implementation

bool ConstitutionalComplianceValidator::validate_constitutional_compliance(
    const integration::IntegratedLaunchConfig& config,
    const KernelValidationConstraints& constraints,
    KernelValidationResult& result
) {
    bool compliant = true;

    // Validate static configuration requirement
    if (!validate_static_configuration(config, result)) {
        compliant = false;
    }

    // Validate no runtime queries requirement
    if (!validate_no_runtime_queries(config, result)) {
        compliant = false;
    }

    // Validate deterministic launch requirement
    if (!validate_deterministic_launch(config, result)) {
        compliant = false;
    }

    // Validate performance targets
    if (!validate_performance_targets(config, constraints, result)) {
        compliant = false;
    }

    return compliant;
}

bool ConstitutionalComplianceValidator::validate_static_configuration(
    const integration::IntegratedLaunchConfig& config,
    KernelValidationResult& result
) {
    return check_static_config_flags(config.static_config, result);
}

bool ConstitutionalComplianceValidator::validate_no_runtime_queries(
    const integration::IntegratedLaunchConfig& config,
    KernelValidationResult& result
) {
    if (!config.static_config.no_runtime_device_queries) {
        result.addError("no_runtime_queries",
                       "Configuration must disable runtime device queries for constitutional compliance");
        return false;
    }

    // Check if configuration source requires runtime queries
    if (config.config_source == "runtime") {
        result.addError("config_source",
                       "Runtime configuration source violates no runtime queries requirement");
        return false;
    }

    result.addInfo("no_runtime_queries", "Runtime device queries disabled - constitutional compliant");
    return true;
}

bool ConstitutionalComplianceValidator::validate_deterministic_launch(
    const integration::IntegratedLaunchConfig& config,
    KernelValidationResult& result
) {
    if (!config.static_config.deterministic_launch) {
        result.addError("deterministic_launch",
                       "Configuration must enable deterministic launch for constitutional compliance");
        return false;
    }

    // Check ECC configuration for deterministic replay
    if (!config.ecc_config.enable_deterministic_replay) {
        result.addWarning("deterministic_replay",
                         "ECC deterministic replay should be enabled for full compliance");
    }

    result.addInfo("deterministic_launch", "Deterministic launch enabled - constitutional compliant");
    return true;
}

bool ConstitutionalComplianceValidator::validate_performance_targets(
    const integration::IntegratedLaunchConfig& config,
    const KernelValidationConstraints& constraints,
    KernelValidationResult& result
) {
    bool targets_met = true;

    // Validate memory efficiency target
    if (config.target_memory_efficiency_percent < constraints.min_memory_efficiency_percent) {
        result.addError("memory_efficiency_target",
                       "Memory efficiency target (" + std::to_string(config.target_memory_efficiency_percent) +
                       "%) below constitutional minimum (" + std::to_string(constraints.min_memory_efficiency_percent) + "%)");
        targets_met = false;
    }

    // Validate GPU utilization target
    if (config.target_gpu_utilization_percent < constraints.min_gpu_utilization_percent) {
        result.addError("gpu_utilization_target",
                       "GPU utilization target (" + std::to_string(config.target_gpu_utilization_percent) +
                       "%) below constitutional minimum (" + std::to_string(constraints.min_gpu_utilization_percent) + "%)");
        targets_met = false;
    }

    // Validate occupancy target
    if (config.target_occupancy_percent < constraints.min_occupancy_percent) {
        result.addError("occupancy_target",
                       "Occupancy target (" + std::to_string(config.target_occupancy_percent) +
                       "%) below constitutional minimum (" + std::to_string(constraints.min_occupancy_percent) + "%)");
        targets_met = false;
    }

    if (targets_met) {
        result.addInfo("performance_targets", "All performance targets meet constitutional requirements");
    }

    return targets_met;
}

bool ConstitutionalComplianceValidator::check_static_config_flags(
    const keyhunt::config::StaticLaunchConfig& static_config,
    KernelValidationResult& result
) {
    if (!static_config.static_configuration_only) {
        result.addError("static_configuration_only",
                       "Configuration must use static configuration only for constitutional compliance");
        return false;
    }

    result.addInfo("static_configuration_only", "Static configuration enforced - constitutional compliant");
    return true;
}

// PerformanceConstraintValidator Implementation

bool PerformanceConstraintValidator::validate_performance_constraints(
    const integration::IntegratedLaunchConfig& config,
    const KernelValidationConstraints& constraints,
    KernelValidationResult& result
) {
    bool constraints_met = true;

    // Estimate performance metrics
    auto estimated_metrics = estimate_performance_metrics(config);

    // Validate memory efficiency
    if (!validate_memory_efficiency(constraints.min_memory_efficiency_percent, config, result)) {
        constraints_met = false;
    }

    // Validate GPU utilization
    if (!validate_gpu_utilization(constraints.min_gpu_utilization_percent, config, result)) {
        constraints_met = false;
    }

    // Validate occupancy
    if (!validate_occupancy(constraints.min_occupancy_percent, config, result)) {
        constraints_met = false;
    }

    // Add performance estimation info
    result.addInfo("performance_estimation",
                  "Estimated throughput: " + std::to_string(estimated_metrics["estimated_keys_per_second"]) + " keys/sec");

    return constraints_met;
}

std::map<std::string, double> PerformanceConstraintValidator::estimate_performance_metrics(
    const integration::IntegratedLaunchConfig& config
) {
    std::map<std::string, double> metrics;

    // Base performance calculation
    uint64_t total_threads = config.get_total_threads();
    double base_throughput_per_thread = 150.0;  // keys/sec per thread

    // Architecture-specific multipliers
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

    // Calculate theoretical throughput
    double theoretical_throughput = total_threads * base_throughput_per_thread * arch_multiplier;

    // Apply efficiency factors
    double efficiency_factor = (config.target_occupancy_percent / 100.0) *
                              (config.target_memory_efficiency_percent / 100.0) *
                              (config.target_gpu_utilization_percent / 100.0);

    metrics["estimated_keys_per_second"] = theoretical_throughput * efficiency_factor;
    metrics["theoretical_throughput"] = theoretical_throughput;
    metrics["efficiency_factor"] = efficiency_factor;
    metrics["architecture_multiplier"] = arch_multiplier;
    metrics["total_threads"] = static_cast<double>(total_threads);

    return metrics;
}

bool PerformanceConstraintValidator::validate_memory_efficiency(
    double target_efficiency,
    const integration::IntegratedLaunchConfig& config,
    KernelValidationResult& result
) {
    double efficiency_score = calculate_memory_efficiency_score(config);

    if (efficiency_score < target_efficiency) {
        result.addError("memory_efficiency",
                       "Memory efficiency score (" + std::to_string(efficiency_score) +
                       "%) below target (" + std::to_string(target_efficiency) + "%)");
        return false;
    }

    result.addInfo("memory_efficiency",
                  "Memory efficiency score (" + std::to_string(efficiency_score) + "%) meets target");
    return true;
}

bool PerformanceConstraintValidator::validate_gpu_utilization(
    double target_utilization,
    const integration::IntegratedLaunchConfig& config,
    KernelValidationResult& result
) {
    // Simple utilization check based on grid size
    uint32_t total_blocks = config.grid_dim.x * config.grid_dim.y * config.grid_dim.z;

    // Estimate utilization based on block count
    double estimated_utilization = std::min(95.0, total_blocks * 2.0);  // Rough estimate

    if (estimated_utilization < target_utilization) {
        result.addError("gpu_utilization",
                       "Estimated GPU utilization (" + std::to_string(estimated_utilization) +
                       "%) below target (" + std::to_string(target_utilization) + "%)");
        return false;
    }

    result.addInfo("gpu_utilization",
                  "Estimated GPU utilization (" + std::to_string(estimated_utilization) + "%) meets target");
    return true;
}

bool PerformanceConstraintValidator::validate_occupancy(
    double target_occupancy,
    const integration::IntegratedLaunchConfig& config,
    KernelValidationResult& result
) {
    double theoretical_occupancy = calculate_theoretical_occupancy(config);

    if (theoretical_occupancy < target_occupancy) {
        result.addError("occupancy",
                       "Theoretical occupancy (" + std::to_string(theoretical_occupancy) +
                       "%) below target (" + std::to_string(target_occupancy) + "%)");
        return false;
    }

    result.addInfo("occupancy",
                  "Theoretical occupancy (" + std::to_string(theoretical_occupancy) + "%) meets target");
    return true;
}

double PerformanceConstraintValidator::calculate_theoretical_occupancy(
    const integration::IntegratedLaunchConfig& config
) {
    // Simple occupancy calculation based on threads and registers
    uint32_t threads_per_block = config.block_dim.x * config.block_dim.y * config.block_dim.z;
    uint32_t registers_per_thread = config.static_config.registers_per_thread;
    uint32_t total_registers_per_block = registers_per_thread * threads_per_block;

    // Estimate max blocks per SM based on register constraints
    uint32_t max_blocks_per_sm = 65536 / std::max(total_registers_per_block, 1u);
    max_blocks_per_sm = std::min(max_blocks_per_sm, 32u);  // Hardware limit

    // Estimate occupancy
    uint32_t max_threads_per_sm = max_blocks_per_sm * threads_per_block;
    double occupancy = (static_cast<double>(max_threads_per_sm) / 2048.0) * 100.0;  // Assuming 2048 threads/SM max

    return std::min(occupancy, 100.0);
}

double PerformanceConstraintValidator::calculate_memory_efficiency_score(
    const integration::IntegratedLaunchConfig& config
) {
    double score = 50.0;  // Base score

    // Add points for memory optimization features
    if (config.static_config.enable_coalesced_access) {
        score += 20.0;
    }

    if (config.static_config.enable_shared_memory) {
        score += 15.0;
    }

    if (config.static_config.memory_alignment >= 128) {
        score += 10.0;
    }

    if (config.shared_memory_size > 0 && config.shared_memory_size % 128 == 0) {
        score += 5.0;
    }

    return std::min(score, 100.0);
}

// KernelConfigValidator Implementation

KernelConfigValidator::KernelConfigValidator(
    int device_id,
    std::optional<KernelValidationConstraints> constraints
) : device_id_(device_id) {

    if (constraints) {
        constraints_ = *constraints;
    } else {
        constraints_ = KernelValidationConstraints::get_constitutional_constraints();
    }

    last_validation_time_ = std::chrono::steady_clock::now();
}

KernelValidationResult KernelConfigValidator::validate_config(
    const integration::IntegratedLaunchConfig& config
) {
    auto start_time = std::chrono::steady_clock::now();

    KernelValidationResult result;
    result.validation_timestamp = std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    result.config_fingerprint = integration_utils::generate_config_fingerprint(config);

    // Perform basic validation
    bool basic_valid = perform_basic_validation(config, result);

    // Perform advanced validation
    bool advanced_valid = perform_advanced_validation(config, result);

    // Calculate overall validity
    result.is_valid = basic_valid && advanced_valid;

    // Calculate compliance score
    result.calculateScore();

    // Record validation time
    auto end_time = std::chrono::steady_clock::now();
    result.validation_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Update statistics
    update_statistics(result);

    last_result_ = result;
    last_validation_time_ = std::chrono::steady_clock::now();

    return result;
}

bool KernelConfigValidator::quick_validate(const integration::IntegratedLaunchConfig& config) {
    // Quick validation checks only the most critical requirements
    if (!config.static_config.static_configuration_only ||
        !config.static_config.no_runtime_device_queries ||
        !config.static_config.deterministic_launch) {
        return false;
    }

    // Check basic thread and memory limits
    uint32_t threads_per_block = config.block_dim.x * config.block_dim.y * config.block_dim.z;
    if (threads_per_block > 1024 || threads_per_block < 32) {
        return false;
    }

    if (config.shared_memory_size > 48 * 1024) {
        return false;
    }

    return true;
}

bool KernelConfigValidator::validate_constitutional_compliance(
    const integration::IntegratedLaunchConfig& config
) {
    return ConstitutionalComplianceValidator::validate_constitutional_compliance(
        config, constraints_, last_result_
    );
}

bool KernelConfigValidator::validate_for_launch(const integration::IntegratedLaunchConfig& config) {
    KernelValidationResult result = validate_config(config);

    if (!result.is_valid) {
        return false;
    }

    // In strict mode, require constitutional compliance
    if (strict_mode_ && !result.constitutional_compliance) {
        return false;
    }

    return true;
}

std::string KernelConfigValidator::generate_validation_report() const {
    std::stringstream ss;
    ss << "=== Kernel Configuration Validation Report ===\n\n";
    ss << "Device ID: " << device_id_ << "\n";
    ss << "Validation Timestamp: " << last_result_.validation_timestamp << "\n";
    ss << "Configuration Fingerprint: " << last_result_.config_fingerprint << "\n";
    ss << "Strict Mode: " << (strict_mode_ ? "ENABLED" : "DISABLED") << "\n\n";

    ss << "Overall Result: " << last_result_.getSummary() << "\n";
    ss << "Compliance Score: " << std::fixed << std::setprecision(2)
       << (last_result_.compliance_score * 100) << "%\n";
    ss << "Validation Time: " << last_result_.validation_time.count() << "ms\n\n";

    // Statistics
    ss << "=== Validation Statistics ===\n";
    ss << "Total Validations: " << validation_count_ << "\n";
    ss << "Passed: " << pass_count_ << "\n";
    ss << "Failed: " << fail_count_ << "\n";
    ss << "Success Rate: " << std::fixed << std::setprecision(1);
    if (validation_count_ > 0) {
        ss << (static_cast<double>(pass_count_) / validation_count_ * 100) << "%\n";
    } else {
        ss << "N/A\n";
    }
    ss << "\n";

    // Errors
    if (!last_result_.errors.empty()) {
        ss << "=== Errors (" << last_result_.errors.size() << ") ===\n";
        for (const auto& error : last_result_.errors) {
            ss << "❌ " << error << "\n";
        }
        ss << "\n";
    }

    // Warnings
    if (!last_result_.warnings.empty()) {
        ss << "=== Warnings (" << last_result_.warnings.size() << ") ===\n";
        for (const auto& warning : last_result_.warnings) {
            ss << "⚠️  " << warning << "\n";
        }
        ss << "\n";
    }

    // Info
    if (!last_result_.info.empty()) {
        ss << "=== Information (" << last_result_.info.size() << ") ===\n";
        for (const auto& info : last_result_.info) {
            ss << "ℹ️  " << info << "\n";
        }
    }

    return ss.str();
}

bool KernelConfigValidator::validate_cached_config(
    const std::string& config_key,
    const integration::IntegratedLaunchConfig& config
) {
    // Check if fingerprint matches (configuration hasn't changed)
    std::string current_fingerprint = integration_utils::generate_config_fingerprint(config);
    return config_key == current_fingerprint;
}

bool KernelConfigValidator::perform_basic_validation(
    const integration::IntegratedLaunchConfig& config,
    KernelValidationResult& result
) {
    bool valid = true;

    // Validate thread counts
    uint32_t threads_per_block = config.block_dim.x * config.block_dim.y * config.block_dim.z;
    if (threads_per_block < constraints_.min_threads_per_block ||
        threads_per_block > constraints_.max_threads_per_block) {
        result.addError("threads_per_block",
                       "Threads per block must be between " +
                       std::to_string(constraints_.min_threads_per_block) + " and " +
                       std::to_string(constraints_.max_threads_per_block));
        valid = false;
    }

    // Validate total threads
    uint64_t total_threads = config.get_total_threads();
    if (total_threads < constraints_.min_total_threads ||
        total_threads > constraints_.max_total_threads) {
        result.addError("total_threads",
                       "Total threads must be between " +
                       std::to_string(constraints_.min_total_threads) + " and " +
                       std::to_string(constraints_.max_total_threads));
        valid = false;
    }

    // Validate shared memory
    if (config.shared_memory_size < constraints_.min_shared_memory ||
        config.shared_memory_size > constraints_.max_shared_memory) {
        result.addError("shared_memory_size",
                       "Shared memory size must be between " +
                       std::to_string(constraints_.min_shared_memory) + " and " +
                       std::to_string(constraints_.max_shared_memory));
        valid = false;
    }

    // Validate register usage
    if (config.static_config.registers_per_thread < constraints_.min_registers_per_thread ||
        config.static_config.registers_per_thread > constraints_.max_registers_per_thread) {
        result.addError("registers_per_thread",
                       "Registers per thread must be between " +
                       std::to_string(constraints_.min_registers_per_thread) + " and " +
                       std::to_string(constraints_.max_registers_per_thread));
        valid = false;
    }

    return valid;
}

bool KernelConfigValidator::perform_advanced_validation(
    const integration::IntegratedLaunchConfig& config,
    KernelValidationResult& result
) {
    bool valid = true;

    // Validate device compatibility
    if (!DeviceCapabilityValidator::validate_device_compatibility(device_id_, config, result)) {
        valid = false;
    }

    // Validate constitutional compliance
    bool constitutional_compliant = ConstitutionalComplianceValidator::validate_constitutional_compliance(
        config, constraints_, result);
    result.constitutional_compliance = constitutional_compliant;

    if (constraints_.require_constitutional_compliance && !constitutional_compliant) {
        valid = false;
    }

    // Validate performance constraints
    if (!PerformanceConstraintValidator::validate_performance_constraints(config, constraints_, result)) {
        valid = false;
    }

    return valid;
}

void KernelConfigValidator::update_statistics(const KernelValidationResult& result) {
    validation_count_++;
    if (result.is_valid) {
        pass_count_++;
    } else {
        fail_count_++;
    }
}

// ValidationCacheManager Implementation

std::optional<KernelValidationResult> ValidationCacheManager::get_cached_result(
    const std::string& config_fingerprint
) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    cleanup_expired_entries();

    auto it = cache_.find(config_fingerprint);
    if (it != cache_.end() && is_cache_entry_valid(it->second)) {
        return it->second.result;
    }

    return std::nullopt;
}

void ValidationCacheManager::cache_result(
    const std::string& config_fingerprint,
    const KernelValidationResult& result
) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    CacheEntry entry;
    entry.result = result;
    entry.timestamp = std::chrono::steady_clock::now();

    cache_[config_fingerprint] = entry;
}

void ValidationCacheManager::clear_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_.clear();
}

std::map<std::string, uint64_t> ValidationCacheManager::get_cache_statistics() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    std::map<std::string, uint64_t> stats;
    stats["total_entries"] = cache_.size();
    stats["valid_entries"] = 0;
    stats["expired_entries"] = 0;

    auto now = std::chrono::steady_clock::now();
    for (const auto& [key, entry] : cache_) {
        if (is_cache_entry_valid(entry)) {
            stats["valid_entries"]++;
        } else {
            stats["expired_entries"]++;
        }
    }

    return stats;
}

bool ValidationCacheManager::is_cache_entry_valid(const CacheEntry& entry) const {
    auto now = std::chrono::steady_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::minutes>(now - entry.timestamp);
    return age < entry.ttl;
}

void ValidationCacheManager::cleanup_expired_entries() {
    auto now = std::chrono::steady_clock::now();
    auto it = cache_.begin();

    while (it != cache_.end()) {
        if (!is_cache_entry_valid(it->second)) {
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
}

// KernelLaunchValidationGuard Implementation

KernelLaunchValidationGuard::KernelLaunchValidationGuard(
    KernelConfigValidator& validator,
    const integration::IntegratedLaunchConfig& config,
    bool auto_validate
) : validator_(validator), config_(config) {

    if (auto_validate) {
        validate();
        auto_validated_ = true;
    }
}

KernelLaunchValidationGuard::~KernelLaunchValidationGuard() {
    if (auto_validated_ && !validation_passed_) {
        // Log warning about failed validation in destructor
        // In a real implementation, this might log to a file or system logger
    }
}

std::string KernelLaunchValidationGuard::get_report() const {
    std::stringstream ss;
    ss << "=== Kernel Launch Validation Guard Report ===\n\n";
    ss << "Validation Passed: " << (validation_passed_ ? "YES" : "NO") << "\n";
    ss << "Auto Validated: " << (auto_validated_ ? "YES" : "NO") << "\n";
    ss << "Configuration Fingerprint: " << integration_utils::generate_config_fingerprint(config_) << "\n\n";

    if (!result_.errors.empty()) {
        ss << "Errors:\n";
        for (const auto& error : result_.errors) {
            ss << "  " << error << "\n";
        }
        ss << "\n";
    }

    if (!result_.warnings.empty()) {
        ss << "Warnings:\n";
        for (const auto& warning : result_.warnings) {
            ss << "  " << warning << "\n";
        }
        ss << "\n";
    }

    return ss.str();
}

bool KernelLaunchValidationGuard::validate() {
    result_ = validator_.validate_config(config_);
    validation_passed_ = result_.is_valid;
    return validation_passed_;
}

// Validation Utils Implementation

namespace validation_utils {

bool validate_config_fingerprint(
    const integration::IntegratedLaunchConfig& config,
    const std::string& expected_fingerprint
) {
    std::string actual_fingerprint = integration_utils::generate_config_fingerprint(config);
    return actual_fingerprint == expected_fingerprint;
}

std::vector<std::string> compare_validation_results(
    const KernelValidationResult& result1,
    const KernelValidationResult& result2
) {
    std::vector<std::string> differences;

    if (result1.is_valid != result2.is_valid) {
        differences.push_back("Valid status differs: " +
                             std::to_string(result1.is_valid) + " vs " +
                             std::to_string(result2.is_valid));
    }

    if (result1.constitutional_compliance != result2.constitutional_compliance) {
        differences.push_back("Constitutional compliance differs: " +
                             std::to_string(result1.constitutional_compliance) + " vs " +
                             std::to_string(result2.constitutional_compliance));
    }

    if (std::abs(result1.compliance_score - result2.compliance_score) > 0.01) {
        differences.push_back("Compliance score differs: " +
                             std::to_string(result1.compliance_score) + " vs " +
                             std::to_string(result2.compliance_score));
    }

    if (result1.errors.size() != result2.errors.size()) {
        differences.push_back("Error count differs: " +
                             std::to_string(result1.errors.size()) + " vs " +
                             std::to_string(result2.errors.size()));
    }

    return differences;
}

bool validate_config_consistency(
    const integration::IntegratedLaunchConfig& config
) {
    // Check internal consistency of configuration
    if (config.batch_size == 0) {
        return false;
    }

    // Check if batch size matches calculated size
    uint64_t calculated_batch = config.get_total_threads() * config.points_per_thread;
    if (config.batch_size != calculated_batch) {
        return false;
    }

    // Check grid and block dimensions
    if (config.grid_dim.x == 0 || config.grid_dim.y == 0 || config.grid_dim.z == 0) {
        return false;
    }

    if (config.block_dim.x == 0 || config.block_dim.y == 0 || config.block_dim.z == 0) {
        return false;
    }

    return true;
}

bool check_configuration_drift(
    const integration::IntegratedLaunchConfig& current_config,
    const integration::IntegratedLaunchConfig& baseline_config,
    KernelValidationResult& result
) {
    bool drifted = false;

    // Compare critical parameters
    if (current_config.block_dim != baseline_config.block_dim) {
        result.addWarning("config_drift", "Block dimensions have changed from baseline");
        drifted = true;
    }

    if (current_config.grid_dim != baseline_config.grid_dim) {
        result.addWarning("config_drift", "Grid dimensions have changed from baseline");
        drifted = true;
    }

    if (current_config.shared_memory_size != baseline_config.shared_memory_size) {
        result.addWarning("config_drift", "Shared memory size has changed from baseline");
        drifted = true;
    }

    if (current_config.points_per_thread != baseline_config.points_per_thread) {
        result.addWarning("config_drift", "Points per thread have changed from baseline");
        drifted = true;
    }

    if (!drifted) {
        result.addInfo("config_drift", "Configuration matches baseline - no drift detected");
    }

    return !drifted;
}

std::map<std::string, double> generate_validation_metrics(
    const KernelValidationResult& result
) {
    std::map<std::string, double> metrics;

    metrics["validation_time_ms"] = static_cast<double>(result.validation_time.count());
    metrics["compliance_score"] = result.compliance_score;
    metrics["error_count"] = static_cast<double>(result.errors.size());
    metrics["warning_count"] = static_cast<double>(result.warnings.size());
    metrics["info_count"] = static_cast<double>(result.info.size());

    // Calculate validation efficiency
    double total_issues = result.errors.size() + result.warnings.size();
    metrics["validation_efficiency"] = total_issues > 0 ?
        (static_cast<double>(result.errors.size()) / total_issues) : 1.0;

    return metrics;
}

} // namespace validation_utils
} // namespace validation
} // namespace keyhunt
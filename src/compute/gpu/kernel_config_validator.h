// Puzzle71 Technical Debt Repair - Kernel Configuration Validation System (T030)
// Comprehensive validation for kernel launch configurations with constitutional compliance
// Implements pre-launch validation to ensure all configurations meet requirements

#pragma once

#include <cuda_runtime.h>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <chrono>
#include <memory>
#include <functional>

#include "KeyhuntCore/common/static_launch_config.h"
#include "config/puzzle71_config_validator.h"
#include "static_config_integration.h"

namespace keyhunt {
namespace validation {

/**
 * @brief Validation result for kernel configuration
 */
struct KernelValidationResult {
    bool is_valid{false};
    bool constitutional_compliance{false};
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::vector<std::string> info;
    double compliance_score{0.0};
    std::chrono::milliseconds validation_time{0};
    std::string validation_timestamp;
    std::string config_fingerprint;

    void addError(const std::string& field, const std::string& message) {
        errors.push_back("[" + field + "] " + message);
    }

    void addWarning(const std::string& field, const std::string& message) {
        warnings.push_back("[" + field + "] " + message);
    }

    void addInfo(const std::string& field, const std::string& message) {
        info.push_back("[" + field + "] " + message);
    }

    void calculateScore() {
        int total_checks = errors.size() + warnings.size();
        if (total_checks == 0) {
            compliance_score = 1.0;
        } else {
            double error_weight = 1.0;
            double warning_weight = 0.3;
            double total_weight = errors.size() * error_weight + warnings.size() * warning_weight;
            double error_penalties = errors.size() * error_weight;
            compliance_score = std::max(0.0, 1.0 - (error_penalties / total_weight));
        }
    }

    std::string getSummary() const {
        if (is_valid && constitutional_compliance) {
            return "✅ PASS - Configuration is valid and constitutionally compliant";
        } else if (is_valid) {
            return "⚠️  PARTIAL - Configuration is valid but not constitutionally compliant";
        } else {
            return "❌ FAIL - Configuration validation failed with " + std::to_string(errors.size()) + " errors";
        }
    }
};

/**
 * @brief Validation constraints for kernel configurations
 */
struct KernelValidationConstraints {
    // Thread count constraints
    uint32_t min_threads_per_block{32};
    uint32_t max_threads_per_block{1024};
    uint32_t min_total_threads{256};
    uint32_t max_total_threads{2147483647};  // 2^31 - 1

    // Memory constraints
    size_t min_shared_memory{0};
    size_t max_shared_memory{48 * 1024};      // 48KB per block
    uint32_t min_registers_per_thread{8};
    uint32_t max_registers_per_thread{80};

    // Performance constraints (constitutional requirements)
    double min_occupancy_percent{50.0};
    double min_memory_efficiency_percent{90.0};
    double min_gpu_utilization_percent{70.0};

    // Constitutional compliance constraints
    bool require_static_configuration{true};
    bool require_no_runtime_queries{true};
    bool require_deterministic_launch{true};

    // Precision constraints
    double max_precision_tolerance{1e-10};

    /**
     * @brief Get default constraints for constitutional compliance
     */
    static KernelValidationConstraints get_constitutional_constraints() {
        KernelValidationConstraints constraints;
        constraints.min_occupancy_percent = 50.0;
        constraints.min_memory_efficiency_percent = 90.0;
        constraints.min_gpu_utilization_percent = 70.0;
        constraints.require_static_configuration = true;
        constraints.require_no_runtime_queries = true;
        constraints.require_deterministic_launch = true;
        constraints.max_precision_tolerance = 1e-10;
        return constraints;
    }

    /**
     * @brief Get relaxed constraints for development/testing
     */
    static KernelValidationConstraints get_development_constraints() {
        KernelValidationConstraints constraints;
        constraints.min_occupancy_percent = 25.0;
        constraints.min_memory_efficiency_percent = 70.0;
        constraints.min_gpu_utilization_percent = 50.0;
        constraints.require_static_configuration = false;
        constraints.require_no_runtime_queries = false;
        constraints.require_deterministic_launch = false;
        constraints.max_precision_tolerance = 1e-8;
        return constraints;
    }
};

/**
 * @brief Device capability validator
 */
class DeviceCapabilityValidator {
public:
    /**
     * @brief Validate configuration against device capabilities
     *
     * @param device_id CUDA device ID
     * @param config Configuration to validate
     * @param result Validation result to populate
     * @return True if configuration is compatible with device
     */
    static bool validate_device_compatibility(
        int device_id,
        const integration::IntegratedLaunchConfig& config,
        KernelValidationResult& result
    );

    /**
     * @brief Check if kernel can be launched on device
     *
     * @param device_id CUDA device ID
     * @param grid_dim Grid dimensions
     * @param block_dim Block dimensions
     * @param shared_memory_size Shared memory size
     * @return True if launch is possible
     */
    static bool can_launch_kernel(
        int device_id,
        dim3 grid_dim,
        dim3 block_dim,
        size_t shared_memory_size
    );

private:
    static bool validate_thread_limits(
        const cudaDeviceProp& props,
        const integration::IntegratedLaunchConfig& config,
        KernelValidationResult& result
    );

    static bool validate_memory_limits(
        const cudaDeviceProp& props,
        const integration::IntegratedLaunchConfig& config,
        KernelValidationResult& result
    );

    static bool validate_compute_capability(
        const cudaDeviceProp& props,
        const integration::IntegratedLaunchConfig& config,
        KernelValidationResult& result
    );
};

/**
 * @brief Constitutional compliance validator
 */
class ConstitutionalComplianceValidator {
public:
    /**
     * @brief Validate constitutional compliance requirements
     *
     * @param config Configuration to validate
     * @param constraints Validation constraints
     * @param result Validation result to populate
     * @return True if configuration meets constitutional requirements
     */
    static bool validate_constitutional_compliance(
        const integration::IntegratedLaunchConfig& config,
        const KernelValidationConstraints& constraints,
        KernelValidationResult& result
    );

    /**
     * @brief Validate static configuration requirement
     */
    static bool validate_static_configuration(
        const integration::IntegratedLaunchConfig& config,
        KernelValidationResult& result
    );

    /**
     * @brief Validate no runtime device queries requirement
     */
    static bool validate_no_runtime_queries(
        const integration::IntegratedLaunchConfig& config,
        KernelValidationResult& result
    );

    /**
     * @brief Validate deterministic launch requirement
     */
    static bool validate_deterministic_launch(
        const integration::IntegratedLaunchConfig& config,
        KernelValidationResult& result
    );

    /**
     * @brief Validate performance targets
     */
    static bool validate_performance_targets(
        const integration::IntegratedLaunchConfig& config,
        const KernelValidationConstraints& constraints,
        KernelValidationResult& result
    );

private:
    static bool check_static_config_flags(
        const keyhunt::config::StaticLaunchConfig& static_config,
        KernelValidationResult& result
    );
};

/**
 * @brief Performance constraint validator
 */
class PerformanceConstraintValidator {
public:
    /**
     * @brief Validate performance constraints
     *
     * @param config Configuration to validate
     * @param constraints Validation constraints
     * @param result Validation result to populate
     * @return True if performance constraints are met
     */
    static bool validate_performance_constraints(
        const integration::IntegratedLaunchConfig& config,
        const KernelValidationConstraints& constraints,
        KernelValidationResult& result
    );

    /**
     * @brief Estimate actual performance metrics
     *
     * @param config Configuration to evaluate
     * @return Estimated performance metrics
     */
    static std::map<std::string, double> estimate_performance_metrics(
        const integration::IntegratedLaunchConfig& config
    );

    /**
     * @brief Validate memory efficiency target
     */
    static bool validate_memory_efficiency(
        double target_efficiency,
        const integration::IntegratedLaunchConfig& config,
        KernelValidationResult& result
    );

    /**
     * @brief Validate GPU utilization target
     */
    static bool validate_gpu_utilization(
        double target_utilization,
        const integration::IntegratedLaunchConfig& config,
        KernelValidationResult& result
    );

    /**
     * @brief Validate occupancy target
     */
    static bool validate_occupancy(
        double target_occupancy,
        const integration::IntegratedLaunchConfig& config,
        KernelValidationResult& result
    );

private:
    static double calculate_theoretical_occupancy(
        const integration::IntegratedLaunchConfig& config
    );

    static double calculate_memory_efficiency_score(
        const integration::IntegratedLaunchConfig& config
    );
};

/**
 * @brief Advanced kernel configuration validator
 *
 * Provides comprehensive validation of kernel launch configurations
 * with constitutional compliance checking and performance validation.
 */
class KernelConfigValidator {
public:
    /**
     * @brief Create validator for specific device
     *
     * @param device_id CUDA device ID
     * @param constraints Validation constraints (optional)
     */
    explicit KernelConfigValidator(
        int device_id,
        std::optional<KernelValidationConstraints> constraints = std::nullopt
    );

    ~KernelConfigValidator() = default;

    /**
     * @brief Validate integrated launch configuration
     *
     * @param config Configuration to validate
     * @return Detailed validation result
     */
    KernelValidationResult validate_config(
        const integration::IntegratedLaunchConfig& config
    );

    /**
     * @brief Quick validation check
     *
     * @param config Configuration to validate
     * @return True if configuration passes basic validation
     */
    bool quick_validate(const integration::IntegratedLaunchConfig& config);

    /**
     * @brief Full constitutional compliance check
     *
     * @param config Configuration to validate
     * @return True if configuration is constitutionally compliant
     */
    bool validate_constitutional_compliance(
        const integration::IntegratedLaunchConfig& config
    );

    /**
     * @brief Validate configuration before kernel launch
     *
     * This is the main validation function to call before launching kernels.
     * It performs all necessary checks and provides detailed feedback.
     *
     * @param config Configuration to validate
     * @return True if configuration is safe to launch
     */
    bool validate_for_launch(const integration::IntegratedLaunchConfig& config);

    /**
     * @brief Get validation constraints
     */
    const KernelValidationConstraints& get_constraints() const { return constraints_; }

    /**
     * @brief Update validation constraints
     */
    void set_constraints(const KernelValidationConstraints& constraints) {
        constraints_ = constraints;
    }

    /**
     * @brief Enable/disable strict mode
     *
     * @param strict_mode Enable strict validation mode
     */
    void set_strict_mode(bool strict_mode) { strict_mode_ = strict_mode; }

    /**
     * @brief Check if strict mode is enabled
     */
    bool is_strict_mode() const { return strict_mode_; }

    /**
     * @brief Get last validation result
     */
    const KernelValidationResult& get_last_result() const { return last_result_; }

    /**
     * @brief Generate validation report
     */
    std::string generate_validation_report() const;

    /**
     * @brief Validate configuration cache entry
     */
    bool validate_cached_config(
        const std::string& config_key,
        const integration::IntegratedLaunchConfig& config
    );

private:
    int device_id_;
    KernelValidationConstraints constraints_;
    bool strict_mode_{true};
    KernelValidationResult last_result_;

    // Validation statistics
    std::chrono::steady_clock::time_point last_validation_time_;
    uint64_t validation_count_{0};
    uint64_t pass_count_{0};
    uint64_t fail_count_{0};

    /**
     * @brief Perform basic validation checks
     */
    bool perform_basic_validation(
        const integration::IntegratedLaunchConfig& config,
        KernelValidationResult& result
    );

    /**
     * @brief Perform advanced validation checks
     */
    bool perform_advanced_validation(
        const integration::IntegratedLaunchConfig& config,
        KernelValidationResult& result
    );

    /**
     * @brief Update validation statistics
     */
    void update_statistics(const KernelValidationResult& result);
};

/**
 * @brief Validation cache manager
 *
 * Caches validation results to improve performance for repeated configurations.
 */
class ValidationCacheManager {
public:
    /**
     * @brief Get cached validation result
     *
     * @param config_fingerprint Configuration fingerprint
     * @return Cached result if available and valid
     */
    std::optional<KernelValidationResult> get_cached_result(
        const std::string& config_fingerprint
    );

    /**
     * @brief Cache validation result
     *
     * @param config_fingerprint Configuration fingerprint
     * @param result Validation result to cache
     */
    void cache_result(
        const std::string& config_fingerprint,
        const KernelValidationResult& result
    );

    /**
     * @brief Clear validation cache
     */
    void clear_cache();

    /**
     * @brief Get cache statistics
     */
    std::map<std::string, uint64_t> get_cache_statistics() const;

private:
    struct CacheEntry {
        KernelValidationResult result;
        std::chrono::steady_clock::time_point timestamp;
        std::chrono::minutes ttl{60};  // Cache TTL: 60 minutes
    };

    std::map<std::string, CacheEntry> cache_;
    mutable std::mutex cache_mutex_;

    bool is_cache_entry_valid(const CacheEntry& entry) const;
    void cleanup_expired_entries();
};

/**
 * @brief RAII validation guard for kernel launches
 *
 * Automatically validates configuration and provides detailed reporting.
 */
class KernelLaunchValidationGuard {
public:
    /**
     * @brief Create validation guard
     *
     * @param validator Kernel configuration validator
     * @param config Configuration to validate
     * @param auto_validate Automatically validate on construction
     */
    KernelLaunchValidationGuard(
        KernelConfigValidator& validator,
        const integration::IntegratedLaunchConfig& config,
        bool auto_validate = true
    );

    ~KernelLaunchValidationGuard();

    /**
     * @brief Check if validation passed
     */
    bool is_valid() const { return validation_passed_; }

    /**
     * @brief Get validation result
     */
    const KernelValidationResult& get_result() const { return result_; }

    /**
     * @brief Generate validation report
     */
    std::string get_report() const;

    /**
     * @brief Manually validate configuration
     */
    bool validate();

private:
    KernelConfigValidator& validator_;
    integration::IntegratedLaunchConfig config_;
    bool validation_passed_{false};
    KernelValidationResult result_;
    bool auto_validated_{false};
};

/**
 * @brief Utility functions for kernel configuration validation
 */
namespace validation_utils {

    /**
     * @brief Validate configuration fingerprint
     */
    bool validate_config_fingerprint(
        const integration::IntegratedLaunchConfig& config,
        const std::string& expected_fingerprint
    );

    /**
     * @brief Compare validation results
     */
    std::vector<std::string> compare_validation_results(
        const KernelValidationResult& result1,
        const KernelValidationResult& result2
    );

    /**
     * @brief Validate configuration consistency
     */
    bool validate_config_consistency(
        const integration::IntegratedLaunchConfig& config
    );

    /**
     * @brief Check configuration drift
     */
    bool check_configuration_drift(
        const integration::IntegratedLaunchConfig& current_config,
        const integration::IntegratedLaunchConfig& baseline_config,
        KernelValidationResult& result
    );

    /**
     * @brief Generate validation metrics
     */
    std::map<std::string, double> generate_validation_metrics(
        const KernelValidationResult& result
    );
}

} // namespace validation
} // namespace keyhunt
// Puzzle71 Technical Debt Repair - Configuration Validator
// Addresses P0/blocking and P1/high priority issues from v5.5 technical debt audit
// Implements T027: Configuration validator with v5.5 constraints enforcement

#pragma once

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <cstdint>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <yaml-cpp/yaml.h>

namespace keyhunt {
namespace config {

/**
 * @brief Enhanced validation error information
 */
struct ValidationError {
    std::string field_path;        // Field path in YAML (e.g., "deterministic_config.performance.memory_efficiency_target")
    std::string error_code;        // Machine-readable error code
    std::string error_message;     // Human-readable error description
    std::string suggested_fix;     // Suggested resolution for error
    std::string severity;          // "error", "warning", "info"
    std::string constraint_id;     // Reference to constitutional constraint
};

/**
 * @brief Performance metrics for configuration validation
 */
struct ValidationMetrics {
    std::chrono::milliseconds load_time{0};
    std::chrono::milliseconds validation_time{0};
    size_t fields_validated{0};
    size_t errors_detected{0};
    size_t warnings_detected{0};
    double compliance_score{0.0};

    std::string to_json() const {
        std::stringstream ss;
        ss << "{";
        ss << "\"load_time_ms\":" << load_time.count() << ",";
        ss << "\"validation_time_ms\":" << validation_time.count() << ",";
        ss << "\"fields_validated\":" << fields_validated << ",";
        ss << "\"errors_detected\":" << errors_detected << ",";
        ss << "\"warnings_detected\":" << warnings_detected << ",";
        ss << "\"compliance_score\":" << std::fixed << std::setprecision(6) << compliance_score;
        ss << "}";
        return ss.str();
    }
};

/**
 * @brief Enhanced configuration validation result
 */
struct ValidationResult {
    bool is_valid;
    std::vector<ValidationError> errors;
    std::vector<ValidationError> warnings;
    std::string summary;
    double compliance_score;
    std::string validation_timestamp;
    ValidationMetrics metrics;
    std::string config_version;
    std::string schema_version;

    ValidationResult() : is_valid(true), compliance_score(0.0) {
        validation_timestamp = getCurrentTimestamp();
    }

    void addError(const std::string& field_path, const std::string& error_code,
                  const std::string& error_message, const std::string& suggested_fix = "",
                  const std::string& constraint_id = "") {
        ValidationError error;
        error.field_path = field_path;
        error.error_code = error_code;
        error.error_message = error_message;
        error.suggested_fix = suggested_fix;
        error.severity = "error";
        error.constraint_id = constraint_id;
        errors.push_back(error);
        is_valid = false;
        metrics.errors_detected++;
    }

    void addWarning(const std::string& field_path, const std::string& error_code,
                    const std::string& error_message, const std::string& suggested_fix = "",
                    const std::string& constraint_id = "") {
        ValidationError warning;
        warning.field_path = field_path;
        warning.error_code = error_code;
        warning.error_message = error_message;
        warning.suggested_fix = suggested_fix;
        warning.severity = "warning";
        warning.constraint_id = constraint_id;
        warnings.push_back(warning);
        metrics.warnings_detected++;
    }

    void generateSummary() {
        if (is_valid) {
            summary = "Configuration validation passed";
            if (!warnings.empty()) {
                summary += " with " + std::to_string(warnings.size()) + " warnings";
            }
        } else {
            summary = "Configuration validation failed with " + std::to_string(errors.size()) + " errors";
        }
    }

    void calculateComplianceScore() {
        int total_checks = errors.size() + warnings.size();
        if (total_checks == 0) {
            compliance_score = 1.0;
        } else {
            // Errors have higher weight than warnings
            double error_weight = 1.0;
            double warning_weight = 0.3;
            double total_weight = errors.size() * error_weight + warnings.size() * warning_weight;
            double error_penalties = errors.size() * error_weight;
            compliance_score = std::max(0.0, 1.0 - (error_penalties / total_weight));
        }
        metrics.compliance_score = compliance_score;
    }

    std::string to_json() const {
        std::stringstream ss;
        ss << "{";
        ss << "\"is_valid\":" << (is_valid ? "true" : "false") << ",";
        ss << "\"summary\":\"" << summary << "\",";
        ss << "\"compliance_score\":" << std::fixed << std::setprecision(6) << compliance_score << ",";
        ss << "\"validation_timestamp\":\"" << validation_timestamp << "\",";
        ss << "\"config_version\":\"" << config_version << "\",";
        ss << "\"schema_version\":\"" << schema_version << "\",";
        ss << "\"metrics\":" << metrics.to_json() << ",";

        ss << "\"errors\":[";
        for (size_t i = 0; i < errors.size(); ++i) {
            if (i > 0) ss << ",";
            ss << "{";
            ss << "\"field_path\":\"" << errors[i].field_path << "\",";
            ss << "\"error_code\":\"" << errors[i].error_code << "\",";
            ss << "\"error_message\":\"" << errors[i].error_message << "\",";
            ss << "\"suggested_fix\":\"" << errors[i].suggested_fix << "\",";
            ss << "\"severity\":\"" << errors[i].severity << "\",";
            ss << "\"constraint_id\":\"" << errors[i].constraint_id << "\"";
            ss << "}";
        }
        ss << "],";

        ss << "\"warnings\":[";
        for (size_t i = 0; i < warnings.size(); ++i) {
            if (i > 0) ss << ",";
            ss << "{";
            ss << "\"field_path\":\"" << warnings[i].field_path << "\",";
            ss << "\"error_code\":\"" << warnings[i].error_code << "\",";
            ss << "\"error_message\":\"" << warnings[i].error_message << "\",";
            ss << "\"suggested_fix\":\"" << warnings[i].suggested_fix << "\",";
            ss << "\"severity\":\"" << warnings[i].severity << "\",";
            ss << "\"constraint_id\":\"" << warnings[i].constraint_id << "\"";
            ss << "}";
        }
        ss << "]";

        ss << "}";
        return ss.str();
    }

private:
    static std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
        return ss.str();
    }
};

/**
 * @brief Schema definition for v5.5 configuration validation
 */
struct ConfigurationSchema {
    struct FieldDefinition {
        std::string name;
        std::string type;
        bool required;
        std::string min_value;
        std::string max_value;
        std::string description;
        std::string constraint_id;
        std::vector<std::string> allowed_values;
    };

    struct SectionDefinition {
        std::string name;
        bool required;
        std::vector<FieldDefinition> fields;
        std::string description;
    };

    std::vector<SectionDefinition> sections;

    static ConfigurationSchema get_v55_schema();
};

/**
 * @brief Constitutional v5.5 constraint validator
 *
 * This class validates that all configuration settings comply
 * with the constitutional requirements specified in puzzle71_constraints_v5.5.
 */
class ConstitutionalValidator {
public:
    /**
     * @brief Validate constitutional compliance
     *
     * @param config YAML configuration node
     * @param result Validation result to populate
     * @return True if configuration complies with v5.5 constraints
     */
    static bool validate_constitutional_compliance(const YAML::Node& config, ValidationResult& result);

    /**
     * @brief Validate static configuration requirement
     *
     * @param config YAML configuration node
     * @param result Validation result to populate
     * @return True if static configuration is properly enforced
     */
    static bool validate_static_configuration(const YAML::Node& config, ValidationResult& result);

    /**
     * @brief Validate no runtime device queries requirement
     *
     * @param config YAML configuration node
     * @param result Validation result to populate
     * @return True if runtime device queries are disabled
     */
    static bool validate_no_runtime_device_queries(const YAML::Node& config, ValidationResult& result);

    /**
     * @brief Validate deterministic replay requirement
     *
     * @param config YAML configuration node
     * @param result Validation result to populate
     * @return True if deterministic replay is enabled
     */
    static bool validate_deterministic_replay(const YAML::Node& config, ValidationResult& result);

    /**
     * @brief Validate version constraint (must be 5.5)
     *
     * @param config YAML configuration node
     * @param result Validation result to populate
     * @return True if version is 5.5
     */
    static bool validate_version_constraint(const YAML::Node& config, ValidationResult& result);

private:
    static const std::string REQUIRED_VERSION;
    static const std::vector<std::string> REQUIRED_CONSTITUTIONAL_FIELDS;
};

/**
 * @brief Performance configuration validator
 *
 * Validates that performance targets meet the minimum requirements
 * specified in the technical debt audit.
 */
class PerformanceValidator {
public:
    /**
     * @brief Validate performance configuration
     *
     * @param config YAML configuration node
     * @param result Validation result to populate
     * @return True if performance configuration meets requirements
     */
    static bool validate_performance_config(const YAML::Node& config, ValidationResult& result);

    /**
     * @brief Validate memory efficiency target (>90%)
     *
     * @param efficiency Memory efficiency value from config
     * @param field_path Path to the field in config
     * @param result Validation result to populate
     * @return True if efficiency meets minimum requirement
     */
    static bool validate_memory_efficiency(double efficiency, const std::string& field_path, ValidationResult& result);

    /**
     * @brief Validate GPU utilization target (>70%)
     *
     * @param utilization GPU utilization value from config
     * @param field_path Path to the field in config
     * @param result Validation result to populate
     * @return True if utilization meets minimum requirement
     */
    static bool validate_gpu_utilization(double utilization, const std::string& field_path, ValidationResult& result);

    /**
     * @brief Validate synchronization overhead (<50%)
     *
     * @param overhead Synchronization overhead percentage
     * @param field_path Path to the field in config
     * @param result Validation result to populate
     * @return True if overhead is within acceptable limits
     */
    static bool validate_synchronization_overhead(double overhead, const std::string& field_path, ValidationResult& result);

private:
    static constexpr double MIN_MEMORY_EFFICIENCY = 90.0;   // Constitutional requirement
    static constexpr double MIN_GPU_UTILIZATION = 70.0;      // Constitutional requirement
    static constexpr double MAX_SYNC_OVERHEAD = 50.0;       // Constitutional requirement
    static constexpr double MIN_OCCUPANCY = 50.0;           // Constitutional requirement
};

/**
 * @brief GPU device configuration validator
 *
 * Validates GPU device specifications and kernel launch configurations
 * for constitutional compliance.
 */
class GPUDeviceValidator {
public:
    /**
     * @brief Validate GPU device configurations
     *
     * @param config YAML configuration node containing gpu_devices
     * @param result Validation result to populate
     * @return True if all GPU configurations are valid
     */
    static bool validate_gpu_devices(const YAML::Node& config, ValidationResult& result);

    /**
     * @brief Validate kernel launch configurations
     *
     * @param config YAML configuration node containing kernel_configs
     * @param result Validation result to populate
     * @return True if all kernel configurations are valid
     */
    static bool validate_kernel_configs(const YAML::Node& config, ValidationResult& result);

    /**
     * @brief Validate compute capability requirements
     *
     * @param compute_cap Compute capability node
     * @param device_path Path for error reporting
     * @param result Validation result to populate
     * @return True if compute capability meets requirements
     */
    static bool validate_compute_capability(const YAML::Node& compute_cap, const std::string& device_path, ValidationResult& result);

private:
    static constexpr int MIN_COMPUTE_CAPABILITY_MAJOR = 3;
    static constexpr int MIN_COMPUTE_CAPABILITY_MINOR = 5;
    static constexpr int MAX_THREADS_PER_BLOCK = 1024;
    static constexpr int MAX_SHARED_MEMORY_BYTES = 65536;
};

/**
 * @brief Main configuration validator
 *
 * This is the main entry point for configuration validation,
 * providing comprehensive validation of all configuration aspects.
 */
class Puzzle71ConfigValidator {
public:
    /**
     * @brief Validate complete configuration file
     *
     * @param config_file Path to YAML configuration file
     * @param result Validation result to populate
     * @return True if configuration is valid
     */
    static bool validate_config_file(const std::string& config_file, ValidationResult& result);

    /**
     * @brief Validate configuration from YAML node
     *
     * @param config YAML configuration node
     * @param result Validation result to populate
     * @return True if configuration is valid
     */
    static bool validate_config(const YAML::Node& config, ValidationResult& result);

    /**
     * @brief Validate configuration string
     *
     * @param config_string YAML configuration as string
     * @param result Validation result to populate
     * @return True if configuration is valid
     */
    static bool validate_config_string(const std::string& config_string, ValidationResult& result);

    /**
     * @brief Generate validation report
     *
     * @param result Validation result
     * @return Human-readable validation report
     */
    static std::string generate_validation_report(const ValidationResult& result);

    /**
     * @brief Check if configuration passes all critical validations
     *
     * @param result Validation result
     * @return True if configuration passes all critical validations
     */
    static bool passes_critical_validations(const ValidationResult& result);

    /**
     * @brief Validate configuration loading performance (<50ms)
     *
     * @param config_file Path to configuration file
     * @param result Validation result to populate
     * @return True if configuration loads within performance threshold
     */
    static bool validate_loading_performance(const std::string& config_file, ValidationResult& result);

private:
    static const std::vector<std::string> REQUIRED_SECTIONS;
    static ConfigurationSchema schema_;

    static bool validate_schema_compliance(const YAML::Node& config, ValidationResult& result);
    static bool validate_data_paths_and_permissions(const YAML::Node& config, ValidationResult& result);
    static bool validate_security_constraints(const YAML::Node& config, ValidationResult& result);
    static std::string get_field_path(const std::vector<std::string>& path_components);
};

/**
 * @brief Quick validation utilities
 */
namespace validation_utils {
    /**
     * @brief Quick validation of configuration file
     *
     * @param config_file Path to configuration file
     * @return True if configuration passes basic validation
     */
    bool quick_validate(const std::string& config_file);

    /**
     * @brief Validate constitutional compliance only
     *
     * @param config_file Path to configuration file
     * @return True if configuration complies with v5.5 constraints
     */
    bool validate_constitutional_compliance(const std::string& config_file);

    /**
     * @brief Validate performance targets only
     *
     * @param config_file Path to configuration file
     * @return True if performance targets meet requirements
     */
    bool validate_performance_targets(const std::string& config_file);
}

/**
 * @brief Configuration validation guard for RAII pattern
 *
 * Provides automatic validation when constructed and detailed
 * reporting when destructed.
 */
class ConfigValidationGuard {
public:
    ConfigValidationGuard(const std::string& config_file);
    ~ConfigValidationGuard();

    bool is_valid() const { return result_.is_valid; }
    const ValidationResult& get_result() const { return result_; }
    std::string get_report() const;

private:
    ValidationResult result_;
    std::string config_file_;
};

} // namespace config
} // namespace keyhunt

// Legacy namespace compatibility
namespace puzzle71_config = keyhunt::config;
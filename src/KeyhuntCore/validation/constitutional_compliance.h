/**
 * @file constitutional_compliance.h
 * @brief Constitutional compliance validation framework v5.5
 *
 * This header defines the constitutional compliance validation framework
 * that ensures all system components comply with the Puzzle71 constitutional
 * constraints version 5.5.
 *
 * Requirements Addressed:
 * - T014: Create constitutional compliance validation framework
 * - Constitutional Principle 1-6: All six core principles
 * - FR-015: Constitutional compliance validation
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
#include <functional>
#include <chrono>
#include <sha256.h>
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>

namespace puzzle71 {
namespace validation {

/**
 * @brief Constitutional constraint definition
 *
 * Represents a single constitutional constraint with validation logic.
 */
struct ConstitutionalConstraint {
    std::string id;                           ///< Constraint identifier (e.g., "CC-001")
    std::string title;                        ///< Human-readable title
    std::string description;                  ///< Detailed description
    std::string principle;                    ///< Associated constitutional principle
    std::string version;                      ///< Constraint version (should be "5.5")
    bool is_blocking;                         ///< Whether violation blocks deployment
    std::string validation_expression;       ///< Validation logic expression
    std::vector<std::string> dependencies;   /// Dependencies on other constraints

    /**
     * @brief Serialize constraint to JSON
     * @return JSON representation
     */
    nlohmann::json to_json() const;

    /**
     * @brief Load constraint from JSON
     * @param j JSON object
     * @return True if loading successful
     */
    bool from_json(const nlohmann::json& j);
};

/**
 * @brief Compliance violation details
 *
 * Contains detailed information about a constitutional compliance violation.
 */
struct ComplianceViolation {
    std::string constraint_id;               ///< Violated constraint ID
    std::string severity;                    ///< Severity level (BLOCKING, CRITICAL, WARNING)
    std::string description;                 ///< Violation description
    std::string location;                    ///< Where violation was detected
    std::string actual_value;                ///< Actual value that caused violation
    std::string expected_value;              ///< Expected value according to constraint
    std::string remediation;                 ///< Suggested remediation steps
    std::chrono::system_clock::time_point detected_at; ///< Detection timestamp

    /**
     * @brief Serialize violation to JSON
     * @return JSON representation
     */
    nlohmann::json to_json() const;

    /**
     * @brief Get violation severity score
     * @return Numerical severity score (higher = more severe)
     */
    double severity_score() const;
};

/**
 * @brief Compliance check result
 *
 * Contains the complete result of a constitutional compliance check.
 */
struct ComplianceCheckResult {
    bool is_compliant;                        ///< Overall compliance status
    std::string check_id;                     ///< Unique check identifier
    std::chrono::system_clock::time_point check_time; ///< Check execution time
    std::string component_version;            ///< Version of component being checked
    std::vector<ComplianceViolation> violations; ///< Detected violations
    std::vector<std::string> passed_constraints;  ///< Constraints that passed
    double compliance_score;                  ///< Overall compliance score (0.0-1.0)
    std::string compliance_report;            ///< Detailed compliance report
    std::string evidence_hash;                ///< SHA-256 hash of compliance evidence

    /**
     * @brief Check if check has blocking violations
     * @return True if blocking violations exist
     */
    bool has_blocking_violations() const;

    /**
     * @brief Get violations by severity
     * @param severity Severity level to filter by
     * @return List of violations with specified severity
     */
    std::vector<ComplianceViolation> get_violations_by_severity(const std::string& severity) const;

    /**
     * @brief Serialize result to JSON
     * @return JSON representation
     */
    nlohmann::json to_json() const;

    /**
     * @brief Generate human-readable report
     * @return Formatted report string
     */
    std::string generate_report() const;
};

/**
 * @brief Constitutional constraint validator interface
 *
 * Base class for all constitutional constraint validators.
 */
class ConstraintValidator {
public:
    virtual ~ConstraintValidator() = default;

    /**
     * @brief Validate a constraint against a component
     * @param constraint Constraint to validate
     * @param component Component data to validate against
     * @return List of violations (empty if compliant)
     */
    virtual std::vector<ComplianceViolation> validate(
        const ConstitutionalConstraint& constraint,
        const nlohmann::json& component) const = 0;

    /**
     * @brief Get validator name
     * @return Validator identifier
     */
    virtual std::string name() const = 0;

    /**
     * @brief Check if validator supports constraint type
     * @param constraint Constraint to check
     * @return True if validator can handle this constraint
     */
    virtual bool supports(const ConstitutionalConstraint& constraint) const = 0;
};

/**
 * @brief Deterministic operations validator
 *
 * Validates Principle 1: Deterministic Operations constraint.
 */
class DeterministicOperationsValidator : public ConstraintValidator {
public:
    std::vector<ComplianceViolation> validate(
        const ConstitutionalConstraint& constraint,
        const nlohmann::json& component) const override;

    std::string name() const override { return "DeterministicOperationsValidator"; }

    bool supports(const ConstitutionalConstraint& constraint) const override;

private:
    bool validate_deterministic_replay(const nlohmann::json& component) const;
    bool validate_no_threadidx_rng(const nlohmann::json& component) const;
    bool validate_replay_seed_pattern(const nlohmann::json& component) const;
};

/**
 * @brief Static configuration validator
 *
 * Validates Principle 6: Static Configuration constraint.
 */
class StaticConfigurationValidator : public ConstraintValidator {
public:
    std::vector<ComplianceViolation> validate(
        const ConstitutionalConstraint& constraint,
        const nlohmann::json& component) const override;

    std::string name() const override { return "StaticConfigurationValidator"; }

    bool supports(const ConstitutionalConstraint& constraint) const override;

private:
    bool validate_no_runtime_queries(const nlohmann::json& component) const;
    bool validate_static_launch_params(const nlohmann::json& component) const;
    bool validate_config_schema_compliance(const nlohmann::json& component) const;
};

/**
 * @brief Performance requirements validator
 *
 * Validates performance-related constitutional constraints.
 */
class PerformanceRequirementsValidator : public ConstraintValidator {
public:
    std::vector<ComplianceViolation> validate(
        const ConstitutionalConstraint& constraint,
        const nlohmann::json& component) const override;

    std::string name() const override { return "PerformanceRequirementsValidator"; }

    bool supports(const ConstitutionalConstraint& constraint) const override;

private:
    bool validate_memory_efficiency(const nlohmann::json& component) const;
    bool validate_gpu_utilization(const nlohmann::json& component) const;
    bool validate_synchronization_overhead(const nlohmann::json& component) const;
};

/**
 * @brief Cryptographic security validator
 *
 * Validates cryptographic security constraints.
 */
class CryptographicSecurityValidator : public ConstraintValidator {
public:
    std::vector<ComplianceViolation> validate(
        const ConstitutionalConstraint& constraint,
        const nlohmann::json& component) const override;

    std::string name() const override { return "CryptographicSecurityValidator"; }

    bool supports(const ConstitutionalConstraint& constraint) const override;

private:
    bool validate_no_crypto_reimplementation(const nlohmann::json& component) const;
    bool validate_cpu_reference_usage(const nlohmann::json& component) const;
    bool validate_precision_requirements(const nlohmann::json& component) const;
};

/**
 * @brief Main constitutional compliance validator
 *
 * Orchestrates compliance validation across all constitutional constraints.
 */
class ConstitutionalComplianceValidator {
public:
    /**
     * @brief Constructor
     * @param constraint_version Constraint version (default: "5.5")
     */
    explicit ConstitutionalComplianceValidator(const std::string& constraint_version = "5.5");

    /**
     * @brief Destructor
     */
    ~ConstitutionalComplianceValidator();

    /**
     * @brief Load constitutional constraints from file
     * @param constraints_file Path to constraints definition file
     * @return True if loading successful
     */
    bool load_constraints(const std::string& constraints_file);

    /**
     * @brief Validate component compliance
     * @param component Component data to validate
     * @param component_type Type of component (e.g., "kernel", "config", "test")
     * @return Compliance check result
     */
    ComplianceCheckResult validate_component(const nlohmann::json& component,
                                            const std::string& component_type) const;

    /**
     * @brief Validate configuration file compliance
     * @param config_file Path to configuration file
     * @return Compliance check result
     */
    ComplianceCheckResult validate_config_file(const std::string& config_file) const;

    /**
     * @brief Validate kernel source compliance
     * @param source_file Path to kernel source file
     * @return Compliance check result
     */
    ComplianceCheckResult validate_kernel_source(const std::string& source_file) const;

    /**
     * @brief Validate test suite compliance
     * @param test_results Test results JSON
     * @return Compliance check result
     */
    ComplianceCheckResult validate_test_suite(const nlohmann::json& test_results) const;

    /**
     * @brief Validate complete system compliance
     * @param system_manifest System manifest with all components
     * @return Comprehensive compliance check result
     */
    ComplianceCheckResult validate_system(const nlohmann::json& system_manifest) const;

    /**
     * @brief Get all loaded constraints
     * @return List of constraints
     */
    std::vector<ConstitutionalConstraint> get_constraints() const;

    /**
     * @brief Get constraints by principle
     * @param principle Constitutional principle (1-6)
     * @return List of constraints for principle
     */
    std::vector<ConstitutionalConstraint> get_constraints_by_principle(int principle) const;

    /**
     * @brief Generate compliance report
     * @param result Compliance check result
     * @return Formatted compliance report
     */
    std::string generate_compliance_report(const ComplianceCheckResult& result) const;

    /**
     * @brief Export compliance evidence
     * @param result Compliance check result
     * @param output_file Output file path
     * @return True if export successful
     */
    bool export_evidence(const ComplianceCheckResult& result,
                        const std::string& output_file) const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

    /**
     * @brief Setup default constitutional constraints v5.5
     */
    void setup_default_constraints();

    /**
     * @brief Register built-in validators
     */
    void register_validators();

    /**
     * @brief Get appropriate validator for constraint
     * @param constraint Constraint to find validator for
     * @return Validator instance or nullptr
     */
    std::shared_ptr<ConstraintValidator> get_validator(const ConstitutionalConstraint& constraint) const;

    /**
     * @brief Calculate compliance score
     * @param violations List of violations
     * @param total_constraints Total number of constraints checked
     * @return Compliance score (0.0-1.0)
     */
    double calculate_compliance_score(const std::vector<ComplianceViolation>& violations,
                                      size_t total_constraints) const;

    /**
     * @brief Generate evidence hash
     * @param result Compliance check result
     * @return SHA-256 hash of evidence data
     */
    std::string generate_evidence_hash(const ComplianceCheckResult& result) const;
};

/**
 * @brief RAII helper for compliance validation
 *
 * Automatically performs compliance validation within a scope
 * and provides detailed reporting.
 */
class ComplianceValidationGuard {
public:
    /**
     * @brief Constructor - starts compliance validation
     * @param validator Compliance validator reference
     * @param component Component to validate
     * @param component_type Component type
     */
    ComplianceValidationGuard(ConstitutionalComplianceValidator& validator,
                             const nlohmann::json& component,
                             const std::string& component_type);

    /**
     * @brief Constructor - validates configuration file
     * @param validator Compliance validator reference
     * @param config_file Configuration file path
     */
    ComplianceValidationGuard(ConstitutionalComplianceValidator& validator,
                             const std::string& config_file);

    /**
     * @brief Destructor - generates compliance report
     */
    ~ComplianceValidationGuard();

    /**
     * @brief Check if component is compliant
     * @return True if component passes all compliance checks
     */
    bool is_compliant() const { return result_.is_compliant; }

    /**
     * @brief Get compliance result
     * @return Reference to compliance check result
     */
    const ComplianceCheckResult& result() const { return result_; }

    /**
     * @brief Get compliance report
     * @return Formatted compliance report
     */
    std::string get_report() const;

private:
    ConstitutionalComplianceValidator& validator_;
    ComplianceCheckResult result_;
    std::string component_name_;
};

/**
 * @brief Compliance validation utilities
 */
namespace compliance_utils {
    /**
     * @brief Quick compliance check
     * @param config_file Configuration file to check
     * @return True if configuration is compliant
     */
    bool quick_compliance_check(const std::string& config_file);

    /**
     * @brief Validate against specific principle
     * @param principle Constitutional principle (1-6)
     * @param component Component to validate
     * @return Compliance result for specific principle
     */
    ComplianceCheckResult validate_principle(int principle,
                                             const nlohmann::json& component);

    /**
     * @brief Generate compliance badge
     * @param result Compliance check result
     * @return Badge representation (e.g., "✅ COMPLIANT v5.5" or "❌ NON-COMPLIANT")
     */
    std::string generate_compliance_badge(const ComplianceCheckResult& result);

    /**
     * @brief Check for critical violations
     * @param result Compliance check result
     * @return True if critical violations exist
     */
    bool has_critical_violations(const ComplianceCheckResult& result);
}

} // namespace validation
} // namespace puzzle71
// Puzzle71Solver - Constitutional Compliance Framework Header
// Implements constitutional compliance validation system for v5.5 constraints for T056

#pragma once

#include <cuda_runtime.h>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <memory>
#include <json/json.hpp>

namespace keyhunt {
namespace validation {

/**
 * Constitutional compliance violation structure
 * Represents a violation of constitutional constraints
 */
struct ComplianceViolation {
    std::string constraint_name;           // Name of the violated constraint
    std::string violation_type;           // Type of violation
    double measured_value;               // Actually measured value
    double required_value;               // Constitutionally required value
    double deviation_percentage;          // Deviation from required value
    std::string description;             // Human-readable description
    std::string severity;                // "ERROR", "WARNING", "INFO"
    std::chrono::time_point<std::chrono::system_clock> detected_at; // Detection timestamp

    ComplianceViolation() : measured_value(0.0), required_value(0.0), deviation_percentage(0.0) {}
};

/**
 * Constitutional Compliance Framework
 *
 * Provides comprehensive validation of constitutional v5.5 requirements
 * for the Puzzle71 system, ensuring all constraints are met.
 *
 * Features:
 * - Complete v5.5 constitutional constraint validation
 * - GPU performance requirements verification
 * - Memory efficiency requirements validation
 * - ECC precision requirements validation
 * - Deterministic replay requirements validation
 * - Synchronization overhead reduction validation
 * - Zero-tolerance regression policy validation
 * - Comprehensive compliance reporting
 * - Constitutional drift detection
 */
class ConstitutionalComplianceFramework {
public:
    /**
     * Constructor
     */
    ConstitutionalComplianceFramework();

    /**
     * Destructor
     */
    virtual ~ConstitutionalComplianceFramework();

    /**
     * Initialize the constitutional compliance framework
     * @return true if initialization successful
     */
    virtual bool initialize();

    /**
     * Validate that system uses static configuration only (no runtime device queries)
     * @return true if static configuration validation passes
     */
    virtual bool validateStaticConfigurationOnly();

    /**
     * Validate GPU performance requirements from constitutional v5.5
     * @return true if GPU performance requirements are met
     */
    virtual bool validateGPUPerformanceRequirements();

    /**
     * Validate memory efficiency requirements
     * @return true if memory efficiency requirements are met
     */
    virtual bool validateMemoryEfficiencyRequirements();

    /**
     * Validate ECC precision requirements
     * @return true if ECC precision requirements are met
     */
    virtual bool validateECCPrecisionRequirements();

    /**
     * Validate deterministic replay requirements
     * @return true if deterministic replay requirements are met
     */
    virtual bool validateDeterministicReplayRequirements();

    /**
     * Validate synchronization overhead reduction requirements
     * @return true if synchronization overhead reduction requirements are met
     */
    virtual bool validateSynchronizationOverheadReduction();

    /**
     * Validate zero-tolerance regression policy
     * @return true if zero-tolerance regression policy is enforced
     */
    virtual bool validateZeroToleranceRegressionPolicy();

    /**
     * Validate constitutional version compliance
     * @return true if constitutional version is v5.5
     */
    virtual bool validateConstitutionalVersion();

    /**
     * Generate comprehensive compliance report
     * @param report Output JSON report containing compliance data
     * @return true if report generated successfully
     */
    virtual bool generateComplianceReport(nlohmann::json& report);

    /**
     * Validate all constitutional constraints
     * @param violations Output vector of detected violations
     * @return true if validation completed (violations may still exist)
     */
    virtual bool validateAllConstraints(std::vector<ComplianceViolation>& violations);

    /**
     * Check for constitutional drift over time
     * @param drift_percentage Output drift percentage
     * @return true if drift analysis completed
     */
    virtual bool checkConstitutionalDrift(double& drift_percentage);

    // Constitutional compliance constants (v5.5)
    static constexpr double CONSTITUTIONAL_VERSION = 5.5;
    static constexpr double GPU_UTILIZATION_MINIMUM = 70.0;           // Must exceed 70%
    static constexpr double MEMORY_EFFICIENCY_MINIMUM = 90.0;         // Must exceed 90%
    static constexpr double OCCUPANCY_TARGET = 65.0;                  // Target 65%
    static constexpr double SYNCHRONIZATION_REDUCTION_TARGET = 50.0;   // Must reduce by 50%
    static constexpr double ECC_PRECISION_TOLERANCE = 1e-10;          // Must be <1e-10
    static constexpr double DETERMINISM_REQUIREMENT = 100.0;          // Must be 100%
    static constexpr double ZERO_REGRESSION_TOLERANCE = 0.0;           // Zero tolerance for regression

private:
    // Internal state
    bool initialized_;
    int device_id_;
    std::vector<ComplianceViolation> detected_violations_;
    std::chrono::time_point<std::chrono::system_clock> validation_start_time_;

    // Performance metrics
    struct PerformanceMetrics {
        double gpu_utilization;
        double memory_efficiency;
        double occupancy;
        double synchronization_reduction;
        double ecc_precision;
        double determinism_rate;
        double regression_rate;
    } current_metrics_;

    // Compliance state
    struct ComplianceState {
        bool static_config_only;
        bool gpu_performance_compliant;
        bool memory_efficiency_compliant;
        bool ecc_precision_compliant;
        bool deterministic_replay_compliant;
        bool synchronization_reduction_compliant;
        bool zero_regression_compliant;
        bool constitutional_version_compliant;
    } compliance_state_;

    // Helper methods
    bool initializeCuda();
    bool validatePerformanceMetric(const std::string& metric_name,
                                 double measured_value,
                                 double required_value,
                                 const std::string& constraint_name,
                                 std::vector<ComplianceViolation>& violations);
    bool checkStaticConfigurationCompliance();
    bool measureCurrentPerformanceMetrics();
    void resetComplianceState();
    nlohmann::json createComplianceReport(const std::vector<ComplianceViolation>& violations);
    double calculateOverallComplianceScore(const std::vector<ComplianceViolation>& violations);

    // Metric measurement methods
    double measureGPUUtilization();
    double measureMemoryEfficiency();
    double measureOccupancy();
    double measureSynchronizationReduction();
    double measureECCPrecision();
    double measureDeterminismRate();
    double measureRegressionRate();

    // Validation constraint definitions
    struct Constraint {
        std::string name;
        std::string description;
        double required_value;
        std::string comparison_operator; // ">=", "<=", "=="
        std::string severity;
    };

    std::vector<Constraint> getConstitutionalConstraints();
    bool validateConstraint(const Constraint& constraint, double measured_value,
                           std::vector<ComplianceViolation>& violations);
};

} // namespace validation
} // namespace keyhunt
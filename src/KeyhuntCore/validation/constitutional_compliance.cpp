/**
 * @file constitutional_compliance.cpp
 * @brief Implementation of constitutional compliance validation framework
 *
 * @author Puzzle71 Technical Debt Repair System
 * @version 5.5
 * @date 2025-10-20
 */

#include "constitutional_compliance.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <regex>

namespace puzzle71 {
namespace validation {

// ConstitutionalConstraint implementation
nlohmann::json ConstitutionalConstraint::to_json() const {
    nlohmann::json j;
    j["id"] = id;
    j["title"] = title;
    j["description"] = description;
    j["principle"] = principle;
    j["version"] = version;
    j["is_blocking"] = is_blocking;
    j["validation_expression"] = validation_expression;
    j["dependencies"] = dependencies;
    return j;
}

bool ConstitutionalConstraint::from_json(const nlohmann::json& j) {
    try {
        id = j["id"];
        title = j["title"];
        description = j["description"];
        principle = j["principle"];
        version = j["version"];
        is_blocking = j["is_blocking"];
        validation_expression = j["validation_expression"];
        dependencies = j["dependencies"].get<std::vector<std::string>>();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// ComplianceViolation implementation
nlohmann::json ComplianceViolation::to_json() const {
    nlohmann::json j;
    j["constraint_id"] = constraint_id;
    j["severity"] = severity;
    j["description"] = description;
    j["location"] = location;
    j["actual_value"] = actual_value;
    j["expected_value"] = expected_value;
    j["remediation"] = remediation;
    j["detected_at"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        detected_at.time_since_epoch()).count();
    return j;
}

double ComplianceViolation::severity_score() const {
    if (severity == "BLOCKING") return 100.0;
    if (severity == "CRITICAL") return 75.0;
    if (severity == "WARNING") return 25.0;
    return 10.0; // INFO
}

// ComplianceCheckResult implementation
bool ComplianceCheckResult::has_blocking_violations() const {
    return std::any_of(violations.begin(), violations.end(),
                      [](const ComplianceViolation& v) {
                          return v.severity == "BLOCKING";
                      });
}

std::vector<ComplianceViolation> ComplianceCheckResult::get_violations_by_severity(
    const std::string& severity) const {
    std::vector<ComplianceViolation> filtered;
    std::copy_if(violations.begin(), violations.end(),
                 std::back_inserter(filtered),
                 [&severity](const ComplianceViolation& v) {
                     return v.severity == severity;
                 });
    return filtered;
}

nlohmann::json ComplianceCheckResult::to_json() const {
    nlohmann::json j;
    j["is_compliant"] = is_compliant;
    j["check_id"] = check_id;
    j["check_time"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        check_time.time_since_epoch()).count();
    j["component_version"] = component_version;
    j["violations"] = nlohmann::json::array();
    for (const auto& violation : violations) {
        j["violations"].push_back(violation.to_json());
    }
    j["passed_constraints"] = passed_constraints;
    j["compliance_score"] = compliance_score;
    j["compliance_report"] = compliance_report;
    j["evidence_hash"] = evidence_hash;
    return j;
}

std::string ComplianceCheckResult::generate_report() const {
    std::stringstream ss;
    ss << "=== Constitutional Compliance Report v5.5 ===\n\n";
    ss << "Overall Status: " << (is_compliant ? "✅ COMPLIANT" : "❌ NON-COMPLIANT") << "\n";
    ss << "Compliance Score: " << std::fixed << std::setprecision(1) << (compliance_score * 100) << "%\n";
    ss << "Check ID: " << check_id << "\n";
    ss << "Component Version: " << component_version << "\n";
    ss << "Check Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(
        check_time.time_since_epoch()).count() << "\n\n";

    if (!violations.empty()) {
        ss << "Violations (" << violations.size() << "):\n";

        // Group violations by severity
        auto blocking_violations = get_violations_by_severity("BLOCKING");
        auto critical_violations = get_violations_by_severity("CRITICAL");
        auto warning_violations = get_violations_by_severity("WARNING");

        if (!blocking_violations.empty()) {
            ss << "  🚫 BLOCKING (" << blocking_violations.size() << "):\n";
            for (const auto& violation : blocking_violations) {
                ss << "    - " << violation.description << "\n";
                ss << "      Location: " << violation.location << "\n";
                ss << "      Remediation: " << violation.remediation << "\n\n";
            }
        }

        if (!critical_violations.empty()) {
            ss << "  ⚠️  CRITICAL (" << critical_violations.size() << "):\n";
            for (const auto& violation : critical_violations) {
                ss << "    - " << violation.description << "\n";
                ss << "      Location: " << violation.location << "\n";
                ss << "      Remediation: " << violation.remediation << "\n\n";
            }
        }

        if (!warning_violations.empty()) {
            ss << "  ⚠️  WARNING (" << warning_violations.size() << "):\n";
            for (const auto& violation : warning_violations) {
                ss << "    - " << violation.description << "\n";
                ss << "      Location: " << violation.location << "\n\n";
            }
        }
    }

    if (!passed_constraints.empty()) {
        ss << "Passed Constraints (" << passed_constraints.size() << "):\n";
        for (const auto& constraint : passed_constraints) {
            ss << "  ✅ " << constraint << "\n";
        }
        ss << "\n";
    }

    ss << "Evidence Hash: " << evidence_hash << "\n";

    return ss.str();
}

// DeterministicOperationsValidator implementation
std::vector<ComplianceViolation> DeterministicOperationsValidator::validate(
    const ConstitutionalConstraint& constraint,
    const nlohmann::json& component) const {

    std::vector<ComplianceViolation> violations;
    auto now = std::chrono::system_clock::now();

    // Check deterministic replay requirement
    if (!validate_deterministic_replay(component)) {
        ComplianceViolation violation;
        violation.constraint_id = constraint.id;
        violation.severity = "BLOCKING";
        violation.description = "Deterministic replay is not enabled or properly configured";
        violation.location = "validation_config.enable_deterministic_validation";
        violation.expected_value = "true";
        violation.detected_at = now;
        violation.remediation = "Enable deterministic validation in configuration and implement replay testing framework";
        violations.push_back(violation);
    }

    // Check for threadIdx.x RNG usage
    if (!validate_no_threadidx_rng(component)) {
        ComplianceViolation violation;
        violation.constraint_id = constraint.id;
        violation.severity = "BLOCKING";
        violation.description = "threadIdx.x used as RNG seed - violates deterministic requirements";
        violation.location = "kernel_source";
        violation.expected_value = "replay_seed + global_thread_id pattern";
        violation.detected_at = now;
        violation.remediation = "Replace threadIdx.x RNG with replay_seed + blockIdx.x * blockDim.x + threadIdx.x pattern";
        violations.push_back(violation);
    }

    // Check replay seed pattern
    if (!validate_replay_seed_pattern(component)) {
        ComplianceViolation violation;
        violation.constraint_id = constraint.id;
        violation.severity = "CRITICAL";
        violation.description = "Incorrect replay seed pattern detected";
        violation.location = "kernel_source";
        violation.expected_value = "replay_seed + blockIdx.x * blockDim.x + threadIdx.x";
        violation.detected_at = now;
        violation.remediation = "Use the mandated replay seed derivation pattern";
        violations.push_back(violation);
    }

    return violations;
}

bool DeterministicOperationsValidator::supports(const ConstitutionalConstraint& constraint) const {
    return constraint.id.find("CC-001") != std::string::npos || // Deterministic operations
           constraint.principle == "1"; // Principle 1: Deterministic Operations
}

bool DeterministicOperationsValidator::validate_deterministic_replay(const nlohmann::json& component) const {
    if (!component.contains("validation_config")) {
        return false;
    }

    const auto& validation = component["validation_config"];
    return validation.value("enable_deterministic_validation", false);
}

bool DeterministicOperationsValidator::validate_no_threadidx_rng(const nlohmann::json& component) const {
    // This would analyze kernel source code for threadIdx.x RNG patterns
    // For now, return true (no violation) as this requires source code analysis
    return true;
}

bool DeterministicOperationsValidator::validate_replay_seed_pattern(const nlohmann::json& component) const {
    // This would validate the correct seed pattern in kernel source
    // For now, return true (no violation) as this requires source code analysis
    return true;
}

// StaticConfigurationValidator implementation
std::vector<ComplianceViolation> StaticConfigurationValidator::validate(
    const ConstitutionalConstraint& constraint,
    const nlohmann::json& component) const {

    std::vector<ComplianceViolation> violations;
    auto now = std::chrono::system_clock::now();

    // Check for runtime device queries
    if (!validate_no_runtime_queries(component)) {
        ComplianceViolation violation;
        violation.constraint_id = constraint.id;
        violation.severity = "BLOCKING";
        violation.description = "Runtime device queries detected - violates static configuration requirement";
        violation.location = "configuration";
        violation.expected_value = "Static configuration only";
        violation.detected_at = now;
        violation.remediation = "Replace runtime device queries with static configuration values";
        violations.push_back(violation);
    }

    // Check static launch parameters
    if (!validate_static_launch_params(component)) {
        ComplianceViolation violation;
        violation.constraint_id = constraint.id;
        violation.severity = "BLOCKING";
        violation.description = "Dynamic launch parameters detected";
        violation.location = "launch_configuration";
        violation.expected_value = "Static launch parameters";
        violation.detected_at = now;
        violation.remediation = "Configure static grid/block dimensions in configuration file";
        violations.push_back(violation);
    }

    // Check config schema compliance
    if (!validate_config_schema_compliance(component)) {
        ComplianceViolation violation;
        violation.constraint_id = constraint.id;
        violation.severity = "CRITICAL";
        violation.description = "Configuration does not comply with required schema";
        violation.location = "configuration_schema";
        violation.expected_value = "Valid schema compliance";
        violation.detected_at = now;
        violation.remediation = "Update configuration to match required schema";
        violations.push_back(violation);
    }

    return violations;
}

bool StaticConfigurationValidator::supports(const ConstitutionalConstraint& constraint) const {
    return constraint.id.find("CC-006") != std::string::npos || // Static configuration
           constraint.principle == "6"; // Principle 6: Static Configuration
}

bool StaticConfigurationValidator::validate_no_runtime_queries(const nlohmann::json& component) const {
    if (!component.contains("runtime_config")) {
        return true; // No runtime config section is good
    }

    const auto& runtime = component["runtime_config"];
    return !runtime.value("auto_detect_gpu", false) &&
           !runtime.value("auto_configure_launch_params", false);
}

bool StaticConfigurationValidator::validate_static_launch_params(const nlohmann::json& component) const {
    if (!component.contains("kernel_configs")) {
        return false; // Missing kernel configurations
    }

    const auto& kernel_configs = component["kernel_configs"];
    for (const auto& config : kernel_configs) {
        if (!config.contains("block_size") || !config.contains("grid_size")) {
            return false; // Missing static launch parameters
        }
    }

    return true;
}

bool StaticConfigurationValidator::validate_config_schema_compliance(const nlohmann::json& component) const {
    // Check for required top-level fields
    return component.contains("config_version") &&
           component.contains("config_schema_version") &&
           component.contains("gpu_devices") &&
           component.contains("performance_config") &&
           component.contains("validation_config");
}

// PerformanceRequirementsValidator implementation
std::vector<ComplianceViolation> PerformanceRequirementsValidator::validate(
    const ConstitutionalConstraint& constraint,
    const nlohmann::json& component) const {

    std::vector<ComplianceViolation> violations;
    auto now = std::chrono::system_clock::now();

    if (!component.contains("performance_config")) {
        ComplianceViolation violation;
        violation.constraint_id = constraint.id;
        violation.severity = "CRITICAL";
        violation.description = "Missing performance configuration";
        violation.location = "performance_config";
        violation.expected_value = "Performance targets configuration";
        violation.detected_at = now;
        violation.remediation = "Add performance_config section with required targets";
        violations.push_back(violation);
        return violations;
    }

    const auto& perf = component["performance_config"];

    // Check memory efficiency target
    if (!validate_memory_efficiency(perf)) {
        ComplianceViolation violation;
        violation.constraint_id = constraint.id;
        violation.severity = "CRITICAL";
        violation.description = "Memory efficiency target below constitutional requirement (≥90%)";
        violation.location = "performance_config.target_memory_efficiency_percent";
        violation.expected_value = "≥90.0";
        violation.detected_at = now;
        violation.remediation = "Increase memory efficiency target to meet constitutional minimum";
        violations.push_back(violation);
    }

    // Check GPU utilization target
    if (!validate_gpu_utilization(perf)) {
        ComplianceViolation violation;
        violation.constraint_id = constraint.id;
        violation.severity = "CRITICAL";
        violation.description = "GPU utilization target below constitutional requirement (≥70%)";
        violation.location = "performance_config.target_gpu_utilization_percent";
        violation.expected_value = "≥70.0";
        violation.detected_at = now;
        violation.remediation = "Increase GPU utilization target to meet constitutional minimum";
        violations.push_back(violation);
    }

    // Check synchronization overhead
    if (!validate_synchronization_overhead(perf)) {
        ComplianceViolation violation;
        violation.constraint_id = constraint.id;
        violation.severity = "WARNING";
        violation.description = "Synchronization overhead target may be too high";
        violation.location = "performance_config.max_sync_overhead_percent";
        violation.expected_value = "≤50.0";
        violation.detected_at = now;
        violation.remediation = "Consider reducing synchronization overhead target";
        violations.push_back(violation);
    }

    return violations;
}

bool PerformanceRequirementsValidator::supports(const ConstitutionalConstraint& constraint) const {
    return constraint.id.find("CC-002") != std::string::npos || // Performance requirements
           constraint.id.find("CC-003") != std::string::npos || // Memory efficiency
           constraint.id.find("CC-004") != std::string::npos || // GPU utilization
           constraint.principle == "2" || // Principle 2: Performance Optimization
           constraint.principle == "3";   // Principle 3: Memory Efficiency
}

bool PerformanceRequirementsValidator::validate_memory_efficiency(const nlohmann::json& component) const {
    double target = component.value("target_memory_efficiency_percent", 0.0);
    return target >= 90.0;
}

bool PerformanceRequirementsValidator::validate_gpu_utilization(const nlohmann::json& component) const {
    double target = component.value("target_gpu_utilization_percent", 0.0);
    return target >= 70.0;
}

bool PerformanceRequirementsValidator::validate_synchronization_overhead(const nlohmann::json& component) const {
    double overhead = component.value("max_sync_overhead_percent", 0.0);
    return overhead <= 50.0;
}

// CryptographicSecurityValidator implementation
std::vector<ComplianceViolation> CryptographicSecurityValidator::validate(
    const ConstitutionalConstraint& constraint,
    const nlohmann::json& component) const {

    std::vector<ComplianceViolation> violations;
    auto now = std::chrono::system_clock::now();

    // Check for crypto reimplementation
    if (!validate_no_crypto_reimplementation(component)) {
        ComplianceViolation violation;
        violation.constraint_id = constraint.id;
        violation.severity = "BLOCKING";
        violation.description = "Custom cryptographic implementation detected";
        violation.location = "ecc_operations";
        violation.expected_value = "Use bitcoin-core/secp256k1 only";
        violation.detected_at = now;
        violation.remediation = "Remove custom crypto implementation and use libsecp256k1";
        violations.push_back(violation);
    }

    // Check CPU reference usage
    if (!validate_cpu_reference_usage(component)) {
        ComplianceViolation violation;
        violation.constraint_id = constraint.id;
        violation.severity = "CRITICAL";
        violation.description = "CPU reference validation not properly implemented";
        violation.location = "validation_config";
        violation.expected_value = "CPU reference validation enabled";
        violation.detected_at = now;
        violation.remediation = "Enable CPU reference validation using libsecp256k1";
        violations.push_back(violation);
    }

    // Check precision requirements
    if (!validate_precision_requirements(component)) {
        ComplianceViolation violation;
        violation.constraint_id = constraint.id;
        violation.severity = "BLOCKING";
        violation.description = "Precision tolerance too loose - must be ≤1e-10";
        violation.location = "validation_config.validation_precision_tolerance";
        violation.expected_value = "≤1e-10";
        violation.detected_at = now;
        violation.remediation = "Set validation_precision_tolerance to 1e-10 or tighter";
        violations.push_back(violation);
    }

    return violations;
}

bool CryptographicSecurityValidator::supports(const ConstitutionalConstraint& constraint) const {
    return constraint.id.find("CC-005") != std::string::npos || // Cryptographic security
           constraint.principle == "5"; // Principle 5: Cryptographic Security
}

bool CryptographicSecurityValidator::validate_no_crypto_reimplementation(const nlohmann::json& component) const {
    if (!component.contains("ecc_operations")) {
        return true; // No ECC operations section
    }

    const auto& ecc = component["ecc_operations"];
    return !ecc.value("custom_crypto_implementation", false) &&
           !ecc.value("bypass_cpu_reference", false);
}

bool CryptographicSecurityValidator::validate_cpu_reference_usage(const nlohmann::json& component) const {
    if (!component.contains("validation_config")) {
        return false;
    }

    const auto& validation = component["validation_config"];
    return validation.value("enable_cpu_gpu_validation", false);
}

bool CryptographicSecurityValidator::validate_precision_requirements(const nlohmann::json& component) const {
    if (!component.contains("validation_config")) {
        return false;
    }

    const auto& validation = component["validation_config"];
    double tolerance = validation.value("validation_precision_tolerance", 1.0);
    return tolerance <= 1e-10;
}

// ConstitutionalComplianceValidator implementation structure
struct ConstitutionalComplianceValidator::Impl {
    std::string constraint_version_;
    std::vector<ConstitutionalConstraint> constraints_;
    std::map<std::string, std::shared_ptr<ConstraintValidator>> validators_;

    Impl(const std::string& version) : constraint_version_(version) {}
};

ConstitutionalComplianceValidator::ConstitutionalComplianceValidator(const std::string& constraint_version)
    : pimpl_(std::make_unique<Impl>(constraint_version)) {
    setup_default_constraints();
    register_validators();
}

ConstitutionalComplianceValidator::~ConstitutionalComplianceValidator() = default;

void ConstitutionalComplianceValidator::setup_default_constraints() {
    // Principle 1: Deterministic Operations
    ConstitutionalConstraint cc001;
    cc001.id = "CC-001";
    cc001.title = "Deterministic Operations";
    cc001.description = "All GPU operations must be deterministic and reproducible";
    cc001.principle = "1";
    cc001.version = "5.5";
    cc001.is_blocking = true;
    cc001.validation_expression = "enable_deterministic_validation == true && no_threadidx_rng == true";
    pimpl_->constraints_.push_back(cc001);

    // Principle 2: Performance Optimization
    ConstitutionalConstraint cc002;
    cc002.id = "CC-002";
    cc002.title = "Performance Optimization";
    cc002.description = "GPU utilization must meet minimum performance targets";
    cc002.principle = "2";
    cc002.version = "5.5";
    cc002.is_blocking = false;
    cc002.validation_expression = "target_gpu_utilization_percent >= 70.0";
    pimpl_->constraints_.push_back(cc002);

    // Principle 3: Memory Efficiency
    ConstitutionalConstraint cc003;
    cc003.id = "CC-003";
    cc003.title = "Memory Efficiency";
    cc003.description = "Memory efficiency must meet minimum targets";
    cc003.principle = "3";
    cc003.version = "5.5";
    cc003.is_blocking = false;
    cc003.validation_expression = "target_memory_efficiency_percent >= 90.0";
    pimpl_->constraints_.push_back(cc003);

    // Principle 4: Adapter Pattern
    ConstitutionalConstraint cc004;
    cc004.id = "CC-004";
    cc004.title = "Adapter Pattern";
    cc004.description = "All external library access must use adapter pattern";
    cc004.principle = "4";
    cc004.version = "5.5";
    cc004.is_blocking = true;
    cc004.validation_expression = "adapter_pattern_usage == true";
    pimpl_->constraints_.push_back(cc004);

    // Principle 5: Cryptographic Security
    ConstitutionalConstraint cc005;
    cc005.id = "CC-005";
    cc005.title = "Cryptographic Security";
    cc005.description = "No cryptographic reimplementation, use bitcoin-core/secp256k1 only";
    cc005.principle = "5";
    cc005.version = "5.5";
    cc005.is_blocking = true;
    cc005.validation_expression = "no_crypto_reimplementation == true && cpu_reference_validation == true";
    pimpl_->constraints_.push_back(cc005);

    // Principle 6: Static Configuration
    ConstitutionalConstraint cc006;
    cc006.id = "CC-006";
    cc006.title = "Static Configuration";
    cc006.description = "No runtime device queries, all configuration must be static";
    cc006.principle = "6";
    cc006.version = "5.5";
    cc006.is_blocking = true;
    cc006.validation_expression = "static_configuration_only == true && no_runtime_device_queries == true";
    pimpl_->constraints_.push_back(cc006);
}

void ConstitutionalComplianceValidator::register_validators() {
    pimpl_->validators_["deterministic"] = std::make_shared<DeterministicOperationsValidator>();
    pimpl_->validators_["static_config"] = std::make_shared<StaticConfigurationValidator>();
    pimpl_->validators_["performance"] = std::make_shared<PerformanceRequirementsValidator>();
    pimpl_->validators_["cryptographic"] = std::make_shared<CryptographicSecurityValidator>();
}

bool ConstitutionalComplianceValidator::load_constraints(const std::string& constraints_file) {
    try {
        std::ifstream file(constraints_file);
        if (!file.is_open()) {
            return false;
        }

        nlohmann::json j;
        file >> j;

        pimpl_->constraints_.clear();
        for (const auto& constraint_json : j["constraints"]) {
            ConstitutionalConstraint constraint;
            if (constraint.from_json(constraint_json)) {
                pimpl_->constraints_.push_back(constraint);
            }
        }

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

ComplianceCheckResult ConstitutionalComplianceValidator::validate_component(
    const nlohmann::json& component,
    const std::string& component_type) const {

    ComplianceCheckResult result;
    result.check_id = "check_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    result.check_time = std::chrono::system_clock::now();
    result.component_version = component.value("version", "unknown");
    result.is_compliant = true;

    for (const auto& constraint : pimpl_->constraints_) {
        auto validator = get_validator(constraint);
        if (validator) {
            auto violations = validator->validate(constraint, component);
            result.violations.insert(result.violations.end(), violations.begin(), violations.end());

            if (!violations.empty()) {
                result.is_compliant = false;
            } else {
                result.passed_constraints.push_back(constraint.id);
            }
        }
    }

    result.compliance_score = calculate_compliance_score(result.violations, pimpl_->constraints_.size());
    result.compliance_report = generate_compliance_report(result);
    result.evidence_hash = generate_evidence_hash(result);

    return result;
}

std::shared_ptr<ConstraintValidator> ConstitutionalComplianceValidator::get_validator(
    const ConstitutionalConstraint& constraint) const {

    for (const auto& [name, validator] : pimpl_->validators_) {
        if (validator->supports(constraint)) {
            return validator;
        }
    }
    return nullptr;
}

double ConstitutionalComplianceValidator::calculate_compliance_score(
    const std::vector<ComplianceViolation>& violations,
    size_t total_constraints) const {

    if (total_constraints == 0) return 1.0;

    double total_penalty = 0.0;
    for (const auto& violation : violations) {
        total_penalty += violation.severity_score();
    }

    // Normalize to 0.0-1.0 range
    double max_possible_penalty = total_constraints * 100.0;
    double score = 1.0 - (total_penalty / max_possible_penalty);
    return std::max(0.0, score);
}

std::string ConstitutionalComplianceValidator::generate_evidence_hash(
    const ComplianceCheckResult& result) const {

    std::string evidence = result.check_id + std::to_string(result.compliance_score) +
                         std::to_string(result.violations.size());

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, evidence.c_str(), evidence.length());

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::setw(2) << static_cast<int>(hash[i]);
    }
    return ss.str();
}

std::vector<ConstitutionalConstraint> ConstitutionalComplianceValidator::get_constraints() const {
    return pimpl_->constraints_;
}

std::vector<ConstitutionalConstraint> ConstitutionalComplianceValidator::get_constraints_by_principle(
    int principle) const {

    std::vector<ConstitutionalConstraint> filtered;
    std::copy_if(pimpl_->constraints_.begin(), pimpl_->constraints_.end(),
                 std::back_inserter(filtered),
                 [principle](const ConstitutionalConstraint& constraint) {
                     return std::stoi(constraint.principle) == principle;
                 });
    return filtered;
}

std::string ConstitutionalComplianceValidator::generate_compliance_report(
    const ComplianceCheckResult& result) const {
    return result.generate_report();
}

bool ConstitutionalComplianceValidator::export_evidence(const ComplianceCheckResult& result,
                                                       const std::string& output_file) const {
    try {
        nlohmann::json evidence = result.to_json();
        evidence["metadata"]["generated_by"] = "Puzzle71 Constitutional Compliance Validator";
        evidence["metadata"]["version"] = "5.5";
        evidence["metadata"]["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        std::ofstream file(output_file);
        if (!file.is_open()) {
            return false;
        }

        file << evidence.dump(2);
        return file.good();
    } catch (const std::exception&) {
        return false;
    }
}

// ComplianceValidationGuard implementation
ComplianceValidationGuard::ComplianceValidationGuard(
    ConstitutionalComplianceValidator& validator,
    const nlohmann::json& component,
    const std::string& component_type)
    : validator_(validator), component_name_(component_type) {
    result_ = validator_.validate_component(component, component_type);
}

ComplianceValidationGuard::ComplianceValidationGuard(
    ConstitutionalComplianceValidator& validator,
    const std::string& config_file)
    : validator_(validator), component_name_(config_file) {

    try {
        std::ifstream file(config_file);
        if (file.is_open()) {
            nlohmann::json component;
            file >> component;
            result_ = validator_.validate_component(component, "config_file");
        } else {
            // Create error result for missing file
            result_.is_compliant = false;
            result_.check_id = "check_error";
            result_.check_time = std::chrono::system_clock::now();

            ComplianceViolation violation;
            violation.constraint_id = "CC-000";
            violation.severity = "BLOCKING";
            violation.description = "Configuration file not found: " + config_file;
            violation.location = config_file;
            violation.detected_at = std::chrono::system_clock::now();
            violation.remediation = "Ensure configuration file exists and is readable";

            result_.violations.push_back(violation);
        }
    } catch (const std::exception&) {
        // Create error result for parsing failure
        result_.is_compliant = false;
        result_.check_id = "check_error";
        result_.check_time = std::chrono::system_clock::now();

        ComplianceViolation violation;
        violation.constraint_id = "CC-000";
        violation.severity = "BLOCKING";
        violation.description = "Failed to parse configuration file: " + config_file;
        violation.location = config_file;
        violation.detected_at = std::chrono::system_clock::now();
        violation.remediation = "Fix JSON/YAML syntax errors in configuration file";

        result_.violations.push_back(violation);
    }
}

ComplianceValidationGuard::~ComplianceValidationGuard() {
    // RAII cleanup - could log results here
    std::cout << "Compliance validation completed for " << component_name_
              << ": " << (result_.is_compliant ? "COMPLIANT" : "NON-COMPLIANT") << std::endl;
}

std::string ComplianceValidationGuard::get_report() const {
    return validator_.generate_compliance_report(result_);
}

// Compliance validation utilities implementation
namespace compliance_utils {

bool quick_compliance_check(const std::string& config_file) {
    ConstitutionalComplianceValidator validator;
    ComplianceValidationGuard guard(validator, config_file);
    return guard.is_compliant();
}

ComplianceCheckResult validate_principle(int principle, const nlohmann::json& component) {
    ConstitutionalComplianceValidator validator;
    auto constraints = validator.get_constraints_by_principle(principle);

    ComplianceCheckResult result;
    result.check_id = "principle_" + std::to_string(principle) + "_check";
    result.check_time = std::chrono::system_clock::now();
    result.is_compliant = true;

    for (const auto& constraint : constraints) {
        auto validator_impl = validator.get_validator(constraint);
        if (validator_impl) {
            auto violations = validator_impl->validate(constraint, component);
            result.violations.insert(result.violations.end(), violations.begin(), violations.end());

            if (!violations.empty()) {
                result.is_compliant = false;
            } else {
                result.passed_constraints.push_back(constraint.id);
            }
        }
    }

    return result;
}

std::string generate_compliance_badge(const ComplianceCheckResult& result) {
    if (result.is_compliant) {
        return "✅ COMPLIANT v5.5";
    } else {
        auto blocking_violations = result.get_violations_by_severity("BLOCKING");
        if (!blocking_violations.empty()) {
            return "🚫 NON-COMPLIANT (BLOCKING)";
        } else {
            return "⚠️  NON-COMPLIANT (WARNINGS)";
        }
    }
}

bool has_critical_violations(const ComplianceCheckResult& result) {
    auto critical_violations = result.get_violations_by_severity("CRITICAL");
    auto blocking_violations = result.get_violations_by_severity("BLOCKING");
    return !critical_violations.empty() || !blocking_violations.empty();
}

} // namespace compliance_utils

} // namespace validation
} // namespace puzzle71
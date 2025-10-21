// T073: Configuration Validation Tests - Test-Driven Development Approach
// Comprehensive failing tests for configuration validator implementation
// Constitutional v5.5 compliance validation with detailed error reporting

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <yaml-cpp/yaml.h>

// Include the configuration validator header
// Note: This test will fail until the implementation is complete
namespace keyhunt {
namespace config {

// Forward declarations for the classes that need to be implemented
struct ValidationResult;
using EnhancedValidationResult = ValidationResult;  // Alias for compatibility

struct ValidationError {
    std::string field_path;
    std::string error_code;
    std::string error_message;
    std::string suggested_fix;
    std::string constraint_id;
    std::string severity;  // "error", "warning", "info"
};

struct ValidationResult {
    bool is_valid;
    std::vector<ValidationError> errors;
    std::vector<ValidationError> warnings;
    std::string summary;
    double compliance_score;
    std::string validation_timestamp;

    ValidationResult() : is_valid(true), compliance_score(0.0) {}

    void addError(const std::string& field_path, const std::string& error_code,
                  const std::string& error_message, const std::string& suggested_fix = "",
                  const std::string& constraint_id = "") {
        ValidationError error;
        error.field_path = field_path;
        error.error_code = error_code;
        error.error_message = error_message;
        error.suggested_fix = suggested_fix;
        error.constraint_id = constraint_id;
        error.severity = "error";
        errors.push_back(error);
        is_valid = false;
    }

    void addWarning(const std::string& field_path, const std::string& error_code,
                    const std::string& error_message, const std::string& suggested_fix = "",
                    const std::string& constraint_id = "") {
        ValidationError warning;
        warning.field_path = field_path;
        warning.error_code = error_code;
        warning.error_message = error_message;
        warning.suggested_fix = suggested_fix;
        warning.constraint_id = constraint_id;
        warning.severity = "warning";
        warnings.push_back(warning);
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
};

class Puzzle71ConfigValidator {
public:
    static bool validate_config_file(const std::string& config_file, ValidationResult& result) {
        // TODO: Implement actual file loading and validation
        return false; // Return false to make tests fail initially
    }

    static bool validate_config_string(const std::string& config_string, ValidationResult& result) {
        // TODO: Implement actual string validation
        return false; // Return false to make tests fail initially
    }

    static std::string generate_validation_report(const ValidationResult& result) {
        // TODO: Implement actual report generation
        return "Validation report not yet implemented";
    }

    static bool passes_critical_validations(const ValidationResult& result) {
        // TODO: Implement actual critical validation logic
        return result.is_valid;
    }
};

namespace validation_utils {
    bool quick_validate(const std::string& config_file) {
        // TODO: Implement quick validation
        return false;
    }

    bool validate_constitutional_compliance(const std::string& config_file) {
        // TODO: Implement constitutional validation
        return false;
    }

    bool validate_performance_targets(const std::string& config_file) {
        // TODO: Implement performance validation
        return false;
    }
}

} // namespace config
} // namespace keyhunt

using namespace keyhunt::config;
using namespace std::chrono;

class ConfigValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = std::filesystem::temp_directory_path() / "config_validation_test";
        std::filesystem::create_directories(test_dir);

        // Create test configuration files
        valid_config_path = create_valid_config_file();
        invalid_config_path = create_invalid_config_file();
        missing_fields_path = create_missing_fields_config();
        v5_5_violation_path = create_v5_5_violation_config();
        performance_below_min_path = create_performance_below_min_config();
        schema_violation_path = create_schema_violation_config();
        malformed_yaml_path = create_malformed_yaml_config();
    }

    void TearDown() override {
        // Clean up test files
        std::filesystem::remove_all(test_dir);
    }

    std::string create_valid_config_file() {
        const std::string path = (test_dir / "valid_test_config.yaml").string();
        std::ofstream file(path);

        file << R"(
project:
  name: "Puzzle71 Test"
  version: "1.0.0"

constitutional:
  version: "5.5"
  static_configuration_only: true
  no_runtime_device_queries: true
  deterministic_replay_required: true

deterministic_config:
  launch_config:
    turing:
      block_size: 256
      grid_size: 65535
      shared_memory_size: 32768
      registers_per_thread: 32
    ampere:
      block_size: 256
      grid_size: 65535
      shared_memory_size: 49152
      registers_per_thread: 32

  performance:
    memory_efficiency_target: 95.0
    gpu_utilization_target: 80.0
    synchronization_overhead_max: 50.0
    occupancy_target: 50.0

ecc_operations:
  batch_size: 1000
  use_montgomery: true
  precision_target: 1e-10
  memory_layout: "structure_of_arrays"
  alignment: 128

techdebt_repair:
  core_repairs:
    enable_ecc_operations_fixed: true
    enable_legacy_adapter_fixed: true
    enable_static_launch_config: true
    enable_config_validator: true

validation:
  ecc_validation:
    enabled: true
    precision_threshold: 1e-10
    max_validation_cases: 100000
    failure_tolerance: 0

performance:
  memory_efficiency_target: 95.0
  gpu_utilization_target: 80.0
)";

        file.close();
        return path;
    }

    std::string create_invalid_config_file() {
        const std::string path = (test_dir / "invalid_test_config.yaml").string();
        std::ofstream file(path);

        file << R"(
project:
  name: ""  # Invalid empty name
  version: "invalid.version"  # Invalid version format

constitutional:
  version: "5.0"  # Wrong version - should be 5.5
  static_configuration_only: false  # Should be true
  no_runtime_device_queries: false  # Should be true

deterministic_config:
  launch_config:
    turing:
      block_size: 0  # Invalid
      grid_size: -1  # Invalid
      shared_memory_size: 100000000  # Too large
      registers_per_thread: 100  # Too many

  performance:
    memory_efficiency_target: 50.0  # Below 90% requirement
    gpu_utilization_target: 30.0  # Below 70% requirement
    synchronization_overhead_max: 150.0  # Above 50% limit
    occupancy_target: 200.0  # Invalid >100%

ecc_operations:
  batch_size: -1  # Invalid negative
  precision_target: 1e-5  # Too loose
  memory_layout: "invalid_layout"  # Invalid option
  alignment: 7  # Not power of 2

validation:
  ecc_validation:
    enabled: true
    precision_threshold: 1e-5  # Too loose
    failure_tolerance: 10  # Should be 0
)";

        file.close();
        return path;
    }

    std::string create_missing_fields_config() {
        const std::string path = (test_dir / "missing_fields_config.yaml").string();
        std::ofstream file(path);

        file << R"(
project:
  name: "Puzzle71 Test"
  # Missing version field

constitutional:
  version: "5.5"
  # Missing other constitutional fields

deterministic_config:
  # Missing launch_config
  performance:
    memory_efficiency_target: 95.0
    # Missing other performance fields

ecc_operations:
  batch_size: 1000
  # Missing most ecc_operations fields

# Missing entire techdebt_repair section
# Missing validation section
)";

        file.close();
        return path;
    }

    std::string create_v5_5_violation_config() {
        const std::string path = (test_dir / "v5_5_violation_config.yaml").string();
        std::ofstream file(path);

        file << R"(
project:
  name: "Puzzle71 Test"
  version: "1.0.0"

constitutional:
  version: "5.5"
  static_configuration_only: false  # VIOLATION: Must be true
  no_runtime_device_queries: false  # VIOLATION: Must be true
  deterministic_replay_required: false  # VIOLATION: Must be true

deterministic_config:
  launch_config:
    # VIOLATION: Runtime device queries should not be used
    auto_detect_gpu: true
    auto_configure_launch_params: true

  performance:
    memory_efficiency_target: 95.0
    gpu_utilization_target: 80.0

ecc_operations:
  batch_size: 1000
  use_montgomery: true
  # VIOLATION: Crypto reimplementation not allowed
  custom_crypto_implementation: true
  bypass_cpu_reference: true

validation:
  ecc_validation:
    enabled: false  # VIOLATION: Should be enabled
    precision_threshold: 1e-8  # VIOLATION: Should be 1e-10
    failure_tolerance: 5  # VIOLATION: Should be 0

# VIOLATION: Missing required techdebt_repair section
)";

        file.close();
        return path;
    }

    std::string create_performance_below_min_config() {
        const std::string path = (test_dir / "performance_below_min.yaml").string();
        std::ofstream file(path);

        file << R"(
project:
  name: "Performance Below Min Test"
  version: "1.0.0"

constitutional:
  version: "5.5"
  static_configuration_only: true
  no_runtime_device_queries: true
  deterministic_replay_required: true

deterministic_config:
  launch_config:
    turing:
      block_size: 64  # Too small
      grid_size: 100  # Too small
      shared_memory_size: 1024  # Too small
      registers_per_thread: 80  # Too many

  performance:
    memory_efficiency_target: 85.0  # Below 90% requirement
    gpu_utilization_target: 65.0   # Below 70% requirement
    synchronization_overhead_max: 75.0  # Above 50% limit
    occupancy_target: 40.0  # Below 50% requirement

ecc_operations:
  batch_size: 100  # Too small
  precision_target: 1e-8  # Below 1e-10 requirement
  memory_layout: "array_of_structures"  # Wrong layout
  alignment: 64  # Not optimal

validation:
  ecc_validation:
    enabled: false  # Should be true
    precision_threshold: 1e-8  # Too loose
    max_validation_cases: 1000  # Too small
    failure_tolerance: 10  # Should be 0
)";

        file.close();
        return path;
    }

    std::string create_schema_violation_config() {
        const std::string path = (test_dir / "schema_violation.yaml").string();
        std::ofstream file(path);

        file << R"(
project:
  name: "Schema Violation Test"
  version: "1.0.0"

constitutional:
  version: "5.5"
  static_configuration_only: true
  no_runtime_device_queries: true
  deterministic_replay_required: true

deterministic_config:
  launch_config:
    turing:
      block_size: 256
      grid_size: 65535
      shared_memory_size: 32768
      registers_per_thread: 32

  performance:
    memory_efficiency_target: 95.0
    gpu_utilization_target: 80.0
    synchronization_overhead_max: 50.0
    occupancy_target: 50.0

ecc_operations:
  batch_size: 1000
  precision_target: 1e-10
  memory_layout: "structure_of_arrays"
  alignment: 128

# Unknown field that should trigger schema validation error
unknown_field_section:
  some_parameter: "invalid"
  another_invalid:
    nested_value: true

# Wrong field types
deterministic_config:
  launch_config:
    turing:
      block_size: "not_a_number"  # Should be integer
      grid_size: null  # Should be integer
      shared_memory_size: 49152.5  # Should be integer
      registers_per_thread: [32]  # Should be integer, not array

performance:
  memory_efficiency_target: "high"  # Should be number
  gpu_utilization_target: null  # Should be number

ecc_operations:
  batch_size: "large"  # Should be integer
  precision_target: [1e-10]  # Should be number
  use_montgomery: "yes"  # Should be boolean
)";

        file.close();
        return path;
    }

    std::string create_malformed_yaml_config() {
        const std::string path = (test_dir / "malformed.yaml").string();
        std::ofstream file(path);

        file << R"(
project:
  name: "Malformed YAML Test"
  version: "1.0.0"

# Unclosed string will cause YAML parsing error
invalid_field: "This string is not closed

constitutional:
  version: "5.5"
  static_configuration_only: true
  no_runtime_device_queries: true
  deterministic_replay_required: true

# Invalid indentation
  invalid_indentation: true
    nested_invalid: false

# Invalid YAML syntax
invalid_list: [item1, item2
missing_bracket: {key: value

# Invalid mapping syntax
invalid_mapping:
key_without_colon "no colon here"
another_key: : "colon at wrong position"
)";

        file.close();
        return path;
    }

    std::filesystem::path test_dir;
    std::string valid_config_path;
    std::string invalid_config_path;
    std::string missing_fields_path;
    std::string v5_5_violation_path;
    std::string performance_below_min_path;
    std::string schema_violation_path;
    std::string malformed_yaml_path;
};

// ============================================================================
// CORE CONFIGURATION VALIDATION TESTS (TDD - These should FAIL initially)
// ============================================================================

// Test: Valid configuration passes all validations
TEST_F(ConfigValidationTest, ValidConfigurationPassesAllValidations) {
    // This test will FAIL until Puzzle71ConfigValidator::validate_config_file is implemented
    EnhancedValidationResult result;
    bool validation_passed = Puzzle71ConfigValidator::validate_config_file(valid_config_path, result);

    EXPECT_TRUE(validation_passed) << "Valid configuration should pass validation";
    EXPECT_TRUE(result.is_valid) << "Validation result should be valid";
    EXPECT_TRUE(result.errors.empty()) << "Should have no validation errors";
    EXPECT_EQ(result.compliance_score, 1.0) << "Should have perfect compliance score";
    EXPECT_TRUE(result.summary.find("passed") != std::string::npos)
        << "Summary should indicate validation passed";

    // Verify all required sections are detected as present
    bool has_project_section = false;
    bool has_constitutional_section = false;
    bool has_deterministic_config_section = false;
    bool has_ecc_operations_section = false;
    bool has_validation_section = false;

    // These will be false until validation implementation detects sections properly
    EXPECT_TRUE(has_project_section) << "Should detect project section";
    EXPECT_TRUE(has_constitutional_section) << "Should detect constitutional section";
    EXPECT_TRUE(has_deterministic_config_section) << "Should detect deterministic config section";
    EXPECT_TRUE(has_ecc_operations_section) << "Should detect ECC operations section";
    EXPECT_TRUE(has_validation_section) << "Should detect validation section";
}

// Test: Invalid configuration fails with detailed errors
TEST_F(ConfigValidationTest, InvalidConfigurationFailsWithDetailedErrors) {
    // This test will FAIL until proper validation is implemented
    EnhancedValidationResult result;
    bool validation_passed = Puzzle71ConfigValidator::validate_config_file(invalid_config_path, result);

    EXPECT_FALSE(validation_passed) << "Invalid configuration should fail validation";
    EXPECT_FALSE(result.is_valid) << "Validation result should be invalid";
    EXPECT_FALSE(result.errors.empty()) << "Should have validation errors";
    EXPECT_LT(result.compliance_score, 1.0) << "Should have less than perfect compliance score";
    EXPECT_TRUE(result.summary.find("failed") != std::string::npos)
        << "Summary should indicate validation failed";

    // Verify specific errors are detected
    bool has_empty_name_error = false;
    bool has_invalid_version_error = false;
    bool has_wrong_constitutional_version = false;
    bool has_invalid_block_size_error = false;
    bool has_low_efficiency_target_error = false;
    bool has_invalid_precision_error = false;

    // These will be false until error detection is implemented
    for (const auto& error : result.errors) {
        if (error.field_path == "project.name" &&
            error.error_message.find("empty") != std::string::npos) {
            has_empty_name_error = true;
        }
        if (error.field_path == "project.version" &&
            error.error_message.find("format") != std::string::npos) {
            has_invalid_version_error = true;
        }
        if (error.field_path == "constitutional.version" &&
            error.error_message.find("5.5") != std::string::npos) {
            has_wrong_constitutional_version = true;
        }
        if (error.field_path == "deterministic_config.launch_config.turing.block_size" &&
            error.error_message.find("invalid") != std::string::npos) {
            has_invalid_block_size_error = true;
        }
        if (error.field_path == "deterministic_config.performance.memory_efficiency_target" &&
            error.error_message.find("below") != std::string::npos) {
            has_low_efficiency_target_error = true;
        }
        if (error.field_path == "ecc_operations.precision_target" &&
            error.error_message.find("precision") != std::string::npos) {
            has_invalid_precision_error = true;
        }
    }

    EXPECT_TRUE(has_empty_name_error) << "Should detect empty project name";
    EXPECT_TRUE(has_invalid_version_error) << "Should detect invalid version format";
    EXPECT_TRUE(has_wrong_constitutional_version) << "Should detect wrong constitutional version";
    EXPECT_TRUE(has_invalid_block_size_error) << "Should detect invalid block size";
    EXPECT_TRUE(has_low_efficiency_target_error) << "Should detect low efficiency target";
    EXPECT_TRUE(has_invalid_precision_error) << "Should detect invalid precision target";
}

// Test: Missing required fields are detected
TEST_F(ConfigValidationTest, MissingRequiredFieldsAreDetected) {
    // This test will FAIL until missing field detection is implemented
    EnhancedValidationResult result;
    bool validation_passed = Puzzle71ConfigValidator::validate_config_file(missing_fields_path, result);

    EXPECT_FALSE(validation_passed) << "Configuration with missing fields should fail validation";
    EXPECT_FALSE(result.is_valid) << "Validation result should be invalid";

    // Verify specific missing fields are detected
    std::vector<std::string> expected_missing_fields = {
        "project.version",
        "constitutional.static_configuration_only",
        "constitutional.no_runtime_device_queries",
        "constitutional.deterministic_replay_required",
        "deterministic_config.launch_config",
        "techdebt_repair",
        "validation"
    };

    for (const auto& expected_field : expected_missing_fields) {
        bool field_detected_as_missing = false;
        for (const auto& error : result.errors) {
            if (error.field_path == expected_field &&
                error.error_message.find("Missing") != std::string::npos) {
                field_detected_as_missing = true;
                break;
            }
        }
        EXPECT_TRUE(field_detected_as_missing)
            << "Should detect missing field: " << expected_field;
    }

    EXPECT_GT(result.errors.size(), 5) << "Should detect multiple missing fields";
}

// Test: Constitutional v5.5 violations are detected
TEST_F(ConfigValidationTest, ConstitutionalViolationsAreDetected) {
    // This test will FAIL until constitutional validation is implemented
    EnhancedValidationResult result;
    bool validation_passed = Puzzle71ConfigValidator::validate_config_file(v5_5_violation_path, result);

    EXPECT_FALSE(validation_passed) << "Constitutional violations should fail validation";
    EXPECT_FALSE(result.is_valid) << "Validation result should be invalid";

    // Verify specific constitutional violations are detected
    bool has_static_config_violation = false;
    bool has_runtime_query_violation = false;
    bool has_deterministic_violation = false;
    bool has_crypto_reimpl_violation = false;
    bool has_validation_disabled_violation = false;
    bool has_missing_techdebt_repair_violation = false;

    for (const auto& error : result.errors) {
        if (error.field_path == "constitutional.static_configuration_only" &&
            error.error_code == "CONST_V5_5_001") {
            has_static_config_violation = true;
        }
        if (error.field_path == "constitutional.no_runtime_device_queries" &&
            error.error_code == "CONST_V5_5_002") {
            has_runtime_query_violation = true;
        }
        if (error.field_path == "constitutional.deterministic_replay_required" &&
            error.error_code == "CONST_V5_5_003") {
            has_deterministic_violation = true;
        }
        if (error.field_path == "ecc_operations.custom_crypto_implementation" &&
            error.error_code == "CONST_V5_5_004") {
            has_crypto_reimpl_violation = true;
        }
        if (error.field_path == "validation.ecc_validation.enabled" &&
            error.error_code == "CONST_V5_5_005") {
            has_validation_disabled_violation = true;
        }
        if (error.field_path == "techdebt_repair" &&
            error.error_code == "CONST_V5_5_006") {
            has_missing_techdebt_repair_violation = true;
        }
    }

    EXPECT_TRUE(has_static_config_violation) << "Should detect static_configuration_only violation";
    EXPECT_TRUE(has_runtime_query_violation) << "Should detect no_runtime_device_queries violation";
    EXPECT_TRUE(has_deterministic_violation) << "Should detect deterministic_replay_required violation";
    EXPECT_TRUE(has_crypto_reimpl_violation) << "Should detect crypto reimplementation violation";
    EXPECT_TRUE(has_validation_disabled_violation) << "Should detect disabled validation violation";
    EXPECT_TRUE(has_missing_techdebt_repair_violation) << "Should detect missing techdebt_repair section";
}

// Test: Performance below minimum requirements is detected
TEST_F(ConfigValidationTest, PerformanceBelowMinimumIsDetected) {
    // This test will FAIL until performance validation is implemented
    EnhancedValidationResult result;
    bool validation_passed = Puzzle71ConfigValidator::validate_config_file(performance_below_min_path, result);

    EXPECT_FALSE(validation_passed) << "Performance below minimum should fail validation";
    EXPECT_FALSE(result.is_valid) << "Validation result should be invalid";

    // Verify specific performance violations are detected
    bool has_low_memory_efficiency = false;
    bool has_low_gpu_utilization = false;
    bool has_high_sync_overhead = false;
    bool has_low_occupancy = false;
    bool has_loose_precision = false;
    bool has_small_batch_size = false;

    for (const auto& error : result.errors) {
        if (error.field_path == "deterministic_config.performance.memory_efficiency_target" &&
            error.error_code == "PERF_001") {
            has_low_memory_efficiency = true;
        }
        if (error.field_path == "deterministic_config.performance.gpu_utilization_target" &&
            error.error_code == "PERF_002") {
            has_low_gpu_utilization = true;
        }
        if (error.field_path == "deterministic_config.performance.synchronization_overhead_max" &&
            error.error_code == "PERF_003") {
            has_high_sync_overhead = true;
        }
        if (error.field_path == "deterministic_config.performance.occupancy_target" &&
            error.error_code == "PERF_004") {
            has_low_occupancy = true;
        }
        if (error.field_path == "ecc_operations.precision_target" &&
            error.error_code == "PERF_005") {
            has_loose_precision = true;
        }
        if (error.field_path == "ecc_operations.batch_size" &&
            error.error_code == "PERF_006") {
            has_small_batch_size = true;
        }
    }

    EXPECT_TRUE(has_low_memory_efficiency) << "Should detect low memory efficiency (< 90%)";
    EXPECT_TRUE(has_low_gpu_utilization) << "Should detect low GPU utilization (< 70%)";
    EXPECT_TRUE(has_high_sync_overhead) << "Should detect high sync overhead (> 50ms)";
    EXPECT_TRUE(has_low_occupancy) << "Should detect low occupancy (< 50%)";
    EXPECT_TRUE(has_loose_precision) << "Should detect loose precision target (> 1e-10)";
    EXPECT_TRUE(has_small_batch_size) << "Should detect small batch size (< 1000)";
}

// Test: Schema violations are detected
TEST_F(ConfigValidationTest, SchemaViolationsAreDetected) {
    // This test will FAIL until schema validation is implemented
    EnhancedValidationResult result;
    bool validation_passed = Puzzle71ConfigValidator::validate_config_file(schema_violation_path, result);

    EXPECT_FALSE(validation_passed) << "Schema violations should fail validation";
    EXPECT_FALSE(result.is_valid) << "Validation result should be invalid";

    // Verify schema violations are detected
    bool has_unknown_fields = false;
    bool has_wrong_types = false;
    bool has_null_values = false;
    bool has_invalid_yaml_structure = false;

    for (const auto& error : result.errors) {
        if (error.error_code == "SCHEMA_001") {  // Unknown field
            has_unknown_fields = true;
        }
        if (error.error_code == "SCHEMA_002") {  // Wrong type
            has_wrong_types = true;
        }
        if (error.error_code == "SCHEMA_003") {  // Null value
            has_null_values = true;
        }
        if (error.error_code == "SCHEMA_004") {  // Invalid YAML structure
            has_invalid_yaml_structure = true;
        }
    }

    EXPECT_TRUE(has_unknown_fields) << "Should detect unknown fields";
    EXPECT_TRUE(has_wrong_types) << "Should detect wrong field types";
    EXPECT_TRUE(has_null_values) << "Should detect null values where required";
}

// Test: Malformed YAML is handled gracefully
TEST_F(ConfigValidationTest, MalformedYAMLIsHandledGracefully) {
    // This test will FAIL until error handling for malformed YAML is implemented
    EnhancedValidationResult result;
    bool validation_passed = Puzzle71ConfigValidator::validate_config_file(malformed_yaml_path, result);

    EXPECT_FALSE(validation_passed) << "Malformed YAML should fail validation";
    EXPECT_FALSE(result.is_valid) << "Validation result should be invalid";

    // Should have a YAML parsing error
    bool has_yaml_parse_error = false;
    for (const auto& error : result.errors) {
        if (error.error_code == "YAML_001") {  // YAML parse error
            has_yaml_parse_error = true;
            EXPECT_FALSE(error.field_path.empty()) << "YAML error should indicate location";
            EXPECT_FALSE(error.error_message.empty()) << "YAML error should have descriptive message";
            EXPECT_FALSE(error.suggested_fix.empty()) << "YAML error should suggest fix";
            break;
        }
    }

    EXPECT_TRUE(has_yaml_parse_error) << "Should detect YAML parsing errors";
}

// ============================================================================
// CONFIGURATION LOADING AND PERFORMANCE TESTS
// ============================================================================

// Test: Configuration loading performance requirements
TEST_F(ConfigValidationTest, ConfigurationLoadingPerformance) {
    const int num_iterations = 100;
    const double max_load_time_ms = 50.0; // Should load in under 50ms

    auto start = high_resolution_clock::now();

    for (int i = 0; i < num_iterations; ++i) {
        EnhancedValidationResult result;
        bool load_success = Puzzle71ConfigValidator::validate_config_file(valid_config_path, result);
        // This will fail until implementation, but we measure the attempt
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    double avg_load_time_ms = static_cast<double>(duration.count()) / num_iterations / 1000.0;

    EXPECT_LE(avg_load_time_ms, max_load_time_ms)
        << "Average configuration load time should be under " << max_load_time_ms << "ms";
}

// Test: Configuration validation from string
TEST_F(ConfigValidationTest, ConfigurationValidationFromString) {
    // Test validation from YAML string (will FAIL until implemented)
    std::string yaml_config = R"(
project:
  name: "String Config Test"
  version: "1.0.0"

constitutional:
  version: "5.5"
  static_configuration_only: true
  no_runtime_device_queries: true
  deterministic_replay_required: true

deterministic_config:
  launch_config:
    turing:
      block_size: 256
      grid_size: 65535
      shared_memory_size: 32768
      registers_per_thread: 32

  performance:
    memory_efficiency_target: 95.0
    gpu_utilization_target: 80.0
    synchronization_overhead_max: 50.0
    occupancy_target: 50.0

ecc_operations:
  batch_size: 1000
  precision_target: 1e-10
  memory_layout: "structure_of_arrays"
  alignment: 128

validation:
  ecc_validation:
    enabled: true
    precision_threshold: 1e-10
    failure_tolerance: 0
)";

    EnhancedValidationResult result;
    bool validation_passed = Puzzle71ConfigValidator::validate_config_string(yaml_config, result);

    EXPECT_TRUE(validation_passed) << "Valid YAML string should pass validation";
    EXPECT_TRUE(result.is_valid) << "Validation result should be valid";
    EXPECT_TRUE(result.errors.empty()) << "Should have no validation errors";
}

// Test: Configuration validation report generation
TEST_F(ConfigValidationTest, ConfigurationValidationReportGeneration) {
    // Test detailed report generation (will FAIL until implemented)
    EnhancedValidationResult result;

    // Simulate validation results
    result.is_valid = false;
    result.compliance_score = 0.65;
    result.validation_timestamp = "2025-10-21T12:00:00Z";

    result.addError("project.name", "VAL_001", "Project name cannot be empty",
                   "Set a valid project name", "CONST_REQUIRED");
    result.addError("deterministic_config.performance.memory_efficiency_target", "PERF_001",
                   "Memory efficiency target 85.0% is below minimum requirement 90.0%",
                   "Increase to at least 90.0%", "CONST_PERFORMANCE");
    result.addWarning("ecc_operations.batch_size", "WARN_001",
                     "Batch size 500 is below recommended 1000 for optimal performance",
                     "Consider increasing to 1000 or higher", "");

    result.generateSummary();

    std::string report = Puzzle71ConfigValidator::generate_validation_report(result);

    EXPECT_FALSE(report.empty()) << "Validation report should not be empty";
    EXPECT_TRUE(report.find("Configuration validation failed") != std::string::npos)
        << "Report should indicate validation failure";
    EXPECT_TRUE(report.find("65.0%") != std::string::npos)
        << "Report should include compliance score";
    EXPECT_TRUE(report.find("project.name") != std::string::npos)
        << "Report should include error field paths";
    EXPECT_TRUE(report.find("Memory efficiency target") != std::string::npos)
        << "Report should include error descriptions";
    EXPECT_TRUE(report.find("Set a valid project name") != std::string::npos)
        << "Report should include suggested fixes";
}

// Test: Critical validation pass/fail determination
TEST_F(ConfigValidationTest, CriticalValidationDetermination) {
    // Test critical validation logic (will FAIL until implemented)
    EnhancedValidationResult critical_fail_result;
    critical_fail_result.is_valid = false;
    critical_fail_result.addError("constitutional.version", "CONST_V5_5_001",
                                  "Version must be 5.5", "", "");

    EnhancedValidationResult warning_only_result;
    warning_only_result.is_valid = true;
    warning_only_result.addWarning("performance.target", "WARN_001",
                                  "Target below recommended", "", "");

    EXPECT_FALSE(Puzzle71ConfigValidator::passes_critical_validations(critical_fail_result))
        << "Should fail critical validations with constitutional errors";
    EXPECT_TRUE(Puzzle71ConfigValidator::passes_critical_validations(warning_only_result))
        << "Should pass critical validations with only warnings";
}

// ============================================================================
// QUICK VALIDATION UTILITIES TESTS
// ============================================================================

// Test: Quick validation utility
TEST_F(ConfigValidationTest, QuickValidationUtility) {
    // Test quick validation utility (will FAIL until implemented)
    bool quick_valid = validation_utils::quick_validate(valid_config_path);
    EXPECT_TRUE(quick_valid) << "Quick validation should pass for valid config";

    bool quick_invalid = validation_utils::quick_validate(invalid_config_path);
    EXPECT_FALSE(quick_invalid) << "Quick validation should fail for invalid config";
}

// Test: Constitutional compliance validation utility
TEST_F(ConfigValidationTest, ConstitutionalComplianceUtility) {
    // Test constitutional compliance utility (will FAIL until implemented)
    bool compliant = validation_utils::validate_constitutional_compliance(valid_config_path);
    EXPECT_TRUE(compliant) << "Valid config should be constitutionally compliant";

    bool non_compliant = validation_utils::validate_constitutional_compliance(v5_5_violation_path);
    EXPECT_FALSE(non_compliant) << "Config with violations should not be compliant";
}

// Test: Performance targets validation utility
TEST_F(ConfigValidationTest, PerformanceTargetsValidationUtility) {
    // Test performance validation utility (will FAIL until implemented)
    bool performance_valid = validation_utils::validate_performance_targets(valid_config_path);
    EXPECT_TRUE(performance_valid) << "Valid config should have acceptable performance targets";

    bool performance_invalid = validation_utils::validate_performance_targets(performance_below_min_path);
    EXPECT_FALSE(performance_invalid) << "Config below performance minimum should fail";
}
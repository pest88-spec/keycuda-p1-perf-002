// Puzzle71Solver - Compatibility Validation Tests (T058)
// Phase 7: User Story 5 - Compatibility Assurance
// Comprehensive validation tests for API, configuration, and architecture compatibility layers

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <regex>

#include "KeyhuntCore/compatibility/api_compatibility.h"
#include "KeyhuntCore/compatibility/config_compatibility.h"
#include "KeyhuntCore/gpu/arch_compatibility.h"
#include "KeyhuntCore/testing/coverage_monitor.h"
#include "KeyhuntCore/utils/logger.h"

using namespace puzzle71::compatibility;
using namespace puzzle71::gpu;
using namespace puzzle71::testing;
using namespace ::testing;

namespace puzzle71::compatibility {

/**
 * @brief Test fixture for comprehensive compatibility validation
 */
class CompatibilityValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        test_dir_ = std::filesystem::temp_directory_path() / "compatibility_validation_test";
        std::filesystem::create_directories(test_dir_);

        // Initialize all compatibility systems
        api_manager_ = &APICompatibilityManager::getInstance();
        config_manager_ = &ConfigCompatibilityManager::getInstance();
        coverage_manager_ = &CoverageManager::getInstance();

        // Initialize systems
        ASSERT_TRUE(api_manager_->initialize());
        ASSERT_TRUE(config_manager_->initialize());
        ASSERT_TRUE(coverage_manager_->initialize());
    }

    void TearDown() override {
        // Clean up test directory
        std::filesystem::remove_all(test_dir_);
    }

    void CreateLegacyAPIConfig(const std::string& filename) {
        std::ofstream file(test_dir_ / filename);
        file << "# Legacy API Configuration\n";
        file << "threads=256\n";
        file << "device=0\n";
        file << "memory=4096\n";
        file << "batch=1000\n";
        file << "range=1000:2000\n";
        file << "output=results.txt\n";
        file.close();
    }

    void CreateModernAPIConfig(const std::string& filename) {
        std::ofstream file(test_dir_ / filename);
        file << "{\n";
        file << "  \"metadata\": {\n";
        file << "    \"version\": \"2.0\",\n";
        file << "    \"format\": \"json_v2\"\n";
        file << "  },\n";
        file << "  \"sections\": {\n";
        file << "    \"gpu\": {\n";
        file << "      \"parameters\": {\n";
        file << "        \"threads\": 256,\n";
        file << "        \"device_id\": 0,\n";
        file << "        \"memory_size\": 4096,\n";
        file << "        \"batch_size\": 1000\n";
        file << "      }\n";
        file << "    },\n";
        file << "    \"processing\": {\n";
        file << "      \"parameters\": {\n";
        file << "        \"key_range\": \"1000:2000\",\n";
        file << "        \"output_file\": \"results.txt\"\n";
        file << "      }\n";
        file << "    }\n";
        file << "  }\n";
        file << "}\n";
        file.close();
    }

    void CreateLegacyConfig(const std::string& filename) {
        std::ofstream file(test_dir_ / filename);
        file << "# Legacy Configuration File\n";
        file << "[gpu]\n";
        file << "threads=256\n";
        file << "blocks=100\n";
        file << "device=0\n";
        file << "[processing]\n";
        file << "batch=1000\n";
        file << "range=1000:2000\n";
        file << "[output]\n";
        file << "verbose=true\n";
        file.close();
    }

    void CreateIncompatibleConfig(const std::string& filename) {
        std::ofstream file(test_dir_ / filename);
        file << "# Incompatible Configuration\n";
        file << "invalid_parameter=invalid_value\n";
        file << "threads=-1\n";
        file << "device=999\n";
        file.close();
    }

    void CreateCoverageTestData() {
        // Create mock coverage data
        std::ofstream file(test_dir_ / "coverage_data.json");
        file << "{\n";
        file << "  \"overall_coverage\": 88.5,\n";
        file << "  \"line_coverage\": 87.2,\n";
        file << "  \"function_coverage\": 90.1,\n";
        file << "  \"threshold_passed\": true\n";
        file << "}\n";
        file.close();
    }

    std::filesystem::path test_dir_;
    APICompatibilityManager* api_manager_;
    ConfigCompatibilityManager* config_manager_;
    CoverageManager* coverage_manager_;
};

// ============================================================================
// API Compatibility Validation Tests
// ============================================================================

TEST_F(CompatibilityValidationTest, APICompatibilityManagerInitialization) {
    // Test that API compatibility manager initializes correctly
    EXPECT_TRUE(api_manager_ != nullptr);

    auto api_monitor = api_manager_->getCompatibilityMonitor();
    EXPECT_TRUE(api_monitor != nullptr);

    auto api_validator = api_manager_->getCompatibilityValidator();
    EXPECT_TRUE(api_validator != nullptr);

    auto api_migrator = api_manager_->getCompatibilityMigrator();
    EXPECT_TRUE(api_migrator != nullptr);
}

TEST_F(CompatibilityValidationTest, LegacyAPICallValidation) {
    // Test validation of legacy API calls
    CreateLegacyAPIConfig("legacy_api_config.cfg");

    auto validator = api_manager_->getCompatibilityValidator();
    std::string config_file = (test_dir_ / "legacy_api_config.cfg").string();

    auto validation_result = validator->validateLegacyAPICall("set_threads", 256);
    EXPECT_TRUE(validation_result.is_compatible);
    EXPECT_TRUE(validation_result.recommendations.empty());

    // Test incompatible legacy call
    auto invalid_result = validator->validateLegacyAPICall("set_invalid_param", 999);
    EXPECT_FALSE(invalid_result.is_compatible);
    EXPECT_FALSE(invalid_result.recommendations.empty());
}

TEST_F(CompatibilityValidationTest, ModernAPICompatibility) {
    // Test modern API compatibility
    auto validator = api_manager_->getCompatibilityValidator();

    auto modern_result = validator->validateModernAPICall("set_gpu_threads", 256);
    EXPECT_TRUE(modern_result.is_compatible);
    EXPECT_TRUE(modern_result.mapped_calls.empty());

    // Test API call that needs mapping
    auto mapping_result = validator->validateModernAPICall("set_legacy_threads", 256);
    EXPECT_TRUE(mapping_result.is_compatible);
    EXPECT_FALSE(mapping_result.mapped_calls.empty());
}

TEST_F(CompatibilityValidationTest, APIMigrationValidation) {
    // Test API migration validation
    CreateLegacyAPIConfig("api_migration_test.cfg");

    auto migrator = api_manager_->getCompatibilityMigrator();
    std::string config_file = (test_dir_ / "api_migration_test.cfg").string();

    auto migration_plan = migrator->generateMigrationPlan(config_file);
    EXPECT_FALSE(migration_plan.steps.empty());
    EXPECT_TRUE(migration_plan.is_migration_possible);

    // Verify migration plan contains expected steps
    bool has_thread_migration = false;
    for (const auto& step : migration_plan.steps) {
        if (step.find("threads") != std::string::npos) {
            has_thread_migration = true;
            break;
        }
    }
    EXPECT_TRUE(has_thread_migration);
}

TEST_F(CompatibilityValidationTest, APICompatibilityReport) {
    // Test comprehensive API compatibility report
    CreateLegacyAPIConfig("api_report_test.cfg");

    auto report_generator = api_manager_->getCompatibilityReportGenerator();
    std::string config_file = (test_dir_ / "api_report_test.cfg").string();

    auto report = report_generator->generateCompatibilityReport(config_file);
    EXPECT_FALSE(report.overall_compatibility_score.empty());
    EXPECT_FALSE(report.compatibility_issues.empty());
    EXPECT_FALSE(report.recommendations.empty());
    EXPECT_FALSE(report.migration_suggestions.empty());

    // Verify report structure
    EXPECT_FALSE(report.summary.empty());
    EXPECT_FALSE(report.detailed_analysis.empty());
    EXPECT_FALSE(report.action_items.empty());
}

// ============================================================================
// Configuration Compatibility Validation Tests
// ============================================================================

TEST_F(CompatibilityValidationTest, ConfigurationFormatDetection) {
    // Test configuration format detection
    CreateLegacyConfig("legacy_test.conf");
    CreateModernAPIConfig("modern_test.json");

    auto legacy_result = config_manager_->validateConfigurationFile(
        (test_dir_ / "legacy_test.conf").string()
    );
    EXPECT_TRUE(legacy_result.is_valid);
    EXPECT_EQ(legacy_result.detected_format, ConfigFormat::LEGACY_V2);

    auto modern_result = config_manager_->validateConfigurationFile(
        (test_dir_ / "modern_test.json").string()
    );
    EXPECT_TRUE(modern_result.is_valid);
    EXPECT_EQ(modern_result.detected_format, ConfigFormat::JSON_V2);
}

TEST_F(CompatibilityValidationTest, ConfigurationValidation) {
    // Test configuration validation
    CreateLegacyConfig("valid_config.conf");
    CreateIncompatibleConfig("invalid_config.conf");

    auto valid_result = config_manager_->validateConfigurationFile(
        (test_dir_ / "valid_config.conf").string()
    );
    EXPECT_TRUE(valid_result.is_valid);
    EXPECT_TRUE(valid_result.errors.empty());

    auto invalid_result = config_manager_->validateConfigurationFile(
        (test_dir_ / "invalid_config.conf").string()
    );
    EXPECT_FALSE(invalid_result.is_valid);
    EXPECT_FALSE(invalid_result.errors.empty());
}

TEST_F(CompatibilityValidationTest, ConfigurationMigrationValidation) {
    // Test configuration migration validation
    CreateLegacyConfig("migration_test.conf");

    std::string source_file = (test_dir_ / "migration_test.conf").string();
    std::string target_file = (test_dir_ / "migrated_test.json").string();

    auto migration_result = config_manager_->migrateConfigurationFile(
        source_file,
        target_file,
        ConfigFormat::JSON_V2
    );

    EXPECT_TRUE(migration_result.success);
    EXPECT_TRUE(std::filesystem::exists(target_file));

    // Validate migrated configuration
    auto migrated_validation = config_manager_->validateConfigurationFile(target_file);
    EXPECT_TRUE(migrated_validation.is_valid);
    EXPECT_EQ(migrated_validation.detected_format, ConfigFormat::JSON_V2);
}

TEST_F(CompatibilityValidationTest, ConfigurationCompatibilityReport) {
    // Test configuration compatibility report
    CreateLegacyConfig("compatibility_test.conf");

    auto compatibility_report = config_manager_->checkCompatibility(
        (test_dir_ / "compatibility_test.conf").string()
    );

    EXPECT_TRUE(compatibility_report.validation_result.is_valid);
    EXPECT_EQ(compatibility_report.current_format, ConfigFormat::LEGACY_V2);
    EXPECT_EQ(compatibility_report.recommended_format, ConfigFormat::JSON_V2);
    EXPECT_FALSE(compatibility_report.compatibility_issues.empty());
    EXPECT_FALSE(compatibility_report.migration_suggestions.empty());
}

// ============================================================================
// GPU Architecture Compatibility Validation Tests
// ============================================================================

TEST_F(CompatibilityValidationTest, GPUArchitectureDetection) {
    // Test GPU architecture detection
    auto compatibility_matrix = std::make_shared<GPUArchitectureCompatibilityMatrix>();
    ASSERT_TRUE(compatibility_matrix->initialize());

    // Test architecture detection from compute capability
    ComputeCapability cc_75{7, 5}; // Turing
    auto arch = compatibility_matrix->detectArchitecture(cc_75);
    EXPECT_EQ(arch, GPUArchitecture::TURING);

    ComputeCapability cc_80{8, 0}; // Ampere
    arch = compatibility_matrix->detectArchitecture(cc_80);
    EXPECT_EQ(arch, GPUArchitecture::AMPERE);

    ComputeCapability cc_89{8, 9}; // Ada Lovelace
    arch = compatibility_matrix->detectArchitecture(cc_89);
    EXPECT_EQ(arch, GPUArchitecture::ADA_LOVELACE);

    ComputeCapability cc_90{9, 0}; // Hopper
    arch = compatibility_matrix->detectArchitecture(cc_90);
    EXPECT_EQ(arch, GPUArchitecture::HOPPER);
}

TEST_F(CompatibilityValidationTest, KernelCompatibilityValidation) {
    // Test kernel compatibility validation
    auto compatibility_matrix = std::make_shared<GPUArchitectureCompatibilityMatrix>();
    ASSERT_TRUE(compatibility_matrix->initialize());

    // Test kernel compatibility for different architectures
    EXPECT_TRUE(compatibility_matrix->isKernelCompatible(GPUArchitecture::TURING, "ecc_kernel"));
    EXPECT_TRUE(compatibility_matrix->isKernelCompatible(GPUArchitecture::AMPERE, "ecc_kernel"));
    EXPECT_TRUE(compatibility_matrix->isKernelCompatible(GPUArchitecture::ADA_LOVELACE, "ecc_kernel"));
    EXPECT_TRUE(compatibility_matrix->isKernelCompatible(GPUArchitecture::HOPPER, "ecc_kernel"));

    // Test advanced kernels
    EXPECT_FALSE(compatibility_matrix->isKernelCompatible(GPUArchitecture::TURING, "ecc_fp8"));
    EXPECT_TRUE(compatibility_matrix->isKernelCompatible(GPUArchitecture::ADA_LOVELACE, "ecc_fp8"));
    EXPECT_TRUE(compatibility_matrix->isKernelCompatible(GPUArchitecture::HOPPER, "ecc_fp8"));
}

TEST_F(CompatibilityValidationTest, ArchitectureCompatibilityReport) {
    // Test architecture compatibility report generation
    auto compatibility_matrix = std::make_shared<GPUArchitectureCompatibilityMatrix>();
    ASSERT_TRUE(compatibility_matrix->initialize());

    // Generate system report
    auto system_report = compatibility_matrix->generateSystemReport();
    EXPECT_FALSE(system_report.empty());
    EXPECT_NE(system_report.find("GPU Architecture"), std::string::npos);
    EXPECT_NE(system_report.find("Compatibility Matrix"), std::string::npos);

    // Generate device report for mock device
    auto device_report = compatibility_matrix->generateDeviceReport(0);
    EXPECT_FALSE(device_report.empty());
}

TEST_F(CompatibilityValidationTest, ArchitectureValidationSuite) {
    // Test comprehensive architecture validation
    auto compatibility_tester = std::make_shared<CompatibilityTester>(*compatibility_matrix);

    // Run compatibility test suite
    auto test_suite_result = compatibility_tester->runCompatibilityTestSuite();
    EXPECT_GT(test_suite_result.total_tests, 0);
    EXPECT_EQ(test_suite_result.total_tests, test_suite_result.passed_tests + test_suite_result.failed_tests);

    // Check test coverage
    EXPECT_GT(test_suite_result.overall_compatibility_score, 0.0);
    EXPECT_LE(test_suite_result.overall_compatibility_score, 100.0);
}

// ============================================================================
// Coverage Compatibility Validation Tests
// ============================================================================

TEST_F(CompatibilityValidationTest, CoverageThresholdValidation) {
    // Test coverage threshold validation
    CreateCoverageTestData();

    auto coverage_monitor = coverage_manager_->getCoverageMonitor();
    ASSERT_TRUE(coverage_monitor != nullptr);

    CoverageConfig config;
    config.minimum_coverage_threshold = 85.0;
    config.critical_coverage_threshold = 90.0;
    config.output_directory = test_dir_.string();

    coverage_monitor->updateConfig(config);
    ASSERT_TRUE(coverage_monitor->initialize());

    // Mock coverage collection (would normally collect from actual tests)
    // For validation, we'll create a scenario that meets thresholds

    // Test threshold checking
    EXPECT_TRUE(coverage_monitor->meetsMinimumThreshold());
    EXPECT_TRUE(coverage_monitor->criticalModulesMeetThreshold());
}

TEST_F(CompatibilityValidationTest, CoverageReportValidation) {
    // Test coverage report generation and validation
    CreateCoverageTestData();

    auto coverage_monitor = coverage_manager_->getCoverageMonitor();
    CoverageConfig config;
    config.minimum_coverage_threshold = 85.0;
    config.output_directory = test_dir_.string();

    coverage_monitor->updateConfig(config);
    ASSERT_TRUE(coverage_monitor->initialize());

    // Generate reports
    EXPECT_TRUE(coverage_monitor->generateReports());

    // Validate generated reports
    std::string html_report = test_dir_ / "coverage_report.html";
    std::string json_report = test_dir_ / "coverage_report.json";
    std::string xml_report = test_dir_ / "coverage_report.xml";

    EXPECT_TRUE(std::filesystem::exists(html_report));
    EXPECT_TRUE(std::filesystem::exists(json_report));
    EXPECT_TRUE(std::filesystem::exists(xml_report));
}

TEST_F(CompatibilityValidationTest, CoverageAlertValidation) {
    // Test coverage alert generation
    auto coverage_monitor = coverage_manager_->getCoverageMonitor();
    CoverageConfig config;
    config.minimum_coverage_threshold = 95.0; // High threshold to trigger alerts
    config.critical_modules = {"ecc", "gpu", "kernels"};
    config.output_directory = test_dir_.string();

    coverage_monitor->updateConfig(config);
    ASSERT_TRUE(coverage_monitor->initialize());

    // Generate alerts (would normally be based on actual coverage data)
    auto alerts = coverage_monitor->getCoverageAlerts();

    // Verify alert structure
    for (const auto& alert : alerts) {
        EXPECT_FALSE(alert.message.empty());
        EXPECT_FALSE(alert.severity.empty());
        EXPECT_NE(alert.timestamp, std::chrono::system_clock::time_point{});
    }
}

// ============================================================================
// Integration Validation Tests
// ============================================================================

TEST_F(CompatibilityValidationTest, EndToEndCompatibilityValidation) {
    // Test end-to-end compatibility validation workflow
    CreateLegacyConfig("integration_test.conf");

    // Step 1: Validate configuration
    auto config_validation = config_manager_->validateConfigurationFile(
        (test_dir_ / "integration_test.conf").string()
    );
    EXPECT_TRUE(config_validation.is_valid);

    // Step 2: Check compatibility
    auto compatibility_report = config_manager_->checkCompatibility(
        (test_dir_ / "integration_test.conf").string()
    );
    EXPECT_TRUE(compatibility_report.validation_result.is_valid);

    // Step 3: Migrate if needed
    if (compatibility_report.current_format != ConfigFormat::JSON_V2) {
        std::string target_file = (test_dir_ / "integration_migrated.json").string();
        auto migration_result = config_manager_->migrateConfigurationFile(
            (test_dir_ / "integration_test.conf").string(),
            target_file,
            ConfigFormat::JSON_V2
        );
        EXPECT_TRUE(migration_result.success);
        EXPECT_TRUE(std::filesystem::exists(target_file));

        // Validate migrated configuration
        auto migrated_validation = config_manager_->validateConfigurationFile(target_file);
        EXPECT_TRUE(migrated_validation.is_valid);
    }

    // Step 4: Test API compatibility with migrated config
    auto api_validator = api_manager_->getCompatibilityValidator();
    auto api_result = api_validator->validateConfigurationCompatibility(
        config_manager_->getCompatibilityManager()
    );
    EXPECT_TRUE(api_result.is_compatible);
}

TEST_F(CompatibilityValidationTest, PerformanceValidationWithLargeConfigs) {
    // Test performance validation with large configuration files
    const int num_sections = 100;
    const int params_per_section = 50;

    // Create large configuration file
    std::string large_config_file = (test_dir_ / "large_config.conf").string();
    std::ofstream file(large_config_file);

    for (int i = 0; i < num_sections; ++i) {
        file << "[section" << i << "]\n";
        for (int j = 0; j < params_per_section; ++j) {
            file << "param" << j << "=" << (i * params_per_section + j) << "\n";
        }
        file << "\n";
    }
    file.close();

    // Measure validation performance
    auto start_time = std::chrono::high_resolution_clock::now();

    auto validation_result = config_manager_->validateConfigurationFile(large_config_file);

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_TRUE(validation_result.is_valid);
    EXPECT_LT(duration.count(), 5000); // Should complete within 5 seconds
}

TEST_F(CompatibilityValidationTest, ErrorHandlingValidation) {
    // Test error handling and recovery
    std::string corrupted_file = (test_dir_ / "corrupted.json").string();
    std::ofstream file(corrupted_file);
    file << "{ invalid json content }\n";
    file.close();

    // Test configuration validation with corrupted file
    auto config_result = config_manager_->validateConfigurationFile(corrupted_file);
    EXPECT_FALSE(config_result.is_valid);
    EXPECT_FALSE(config_result.errors.empty());

    // Test API compatibility with invalid configuration
    auto api_validator = api_manager_->getCompatibilityValidator();
    auto api_result = api_validator->validateLegacyAPICall("invalid_function", "invalid_param");
    EXPECT_FALSE(api_result.is_compatible);
    EXPECT_FALSE(api_result.recommendations.empty());

    // Test migration error handling
    auto migration_result = config_manager_->migrateConfigurationFile(
        corrupted_file,
        (test_dir_ / "migration_target.json").string(),
        ConfigFormat::JSON_V2
    );
    EXPECT_FALSE(migration_result.success);
    EXPECT_FALSE(migration_result.issues_encountered.empty());
}

TEST_F(CompatibilityValidationTest, BackwardCompatibilityValidation) {
    // Test backward compatibility guarantees
    CreateLegacyAPIConfig("backward_test.cfg");

    // Test that legacy API calls still work
    auto api_monitor = api_manager_->getCompatibilityMonitor();
    auto legacy_result = api_monitor->validateLegacyAPICompatibility();
    EXPECT_TRUE(legacy_result.all_legacy_apis_supported);

    // Test parameter mapping
    auto api_migrator = api_manager_->getCompatibilityMigrator();
    auto mapping_result = api_migrator->mapLegacyParameters(
        (test_dir_ / "backward_test.cfg").string()
    );
    EXPECT_TRUE(mapping_result.success);
    EXPECT_FALSE(mapping_result.parameter_mappings.empty());

    // Verify specific mappings
    bool has_threads_mapping = false;
    for (const auto& [old_param, new_param] : mapping_result.parameter_mappings) {
        if (old_param == "threads" && new_param == "gpu_threads") {
            has_threads_mapping = true;
            break;
        }
    }
    EXPECT_TRUE(has_threads_mapping);
}

TEST_F(CompatibilityValidationTest, CrossSystemValidation) {
    // Test validation across different compatibility systems
    CreateLegacyConfig("cross_system_test.conf");

    // Validate configuration compatibility
    auto config_result = config_manager_->validateConfigurationFile(
        (test_dir_ / "cross_system_test.conf").string()
    );
    EXPECT_TRUE(config_result.is_valid);

    // Validate API compatibility with the configuration
    auto api_validator = api_manager_->getCompatibilityValidator();
    auto api_result = api_validator->validateConfigurationCompatibility(
        config_manager_->getCompatibilityManager()
    );
    EXPECT_TRUE(api_result.is_compatible);

    // Validate GPU architecture compatibility
    auto gpu_matrix = std::make_shared<GPUArchitectureCompatibilityMatrix>();
    ASSERT_TRUE(gpu_matrix->initialize());
    auto gpu_result = gpu_matrix->validateSystemCompatibility();
    EXPECT_TRUE(gpu_result.is_compatible);

    // Validate coverage compatibility
    auto coverage_monitor = coverage_manager_->getCoverageMonitor();
    CoverageConfig coverage_config;
    coverage_config.minimum_coverage_threshold = 85.0;
    coverage_config.output_directory = test_dir_.string();
    coverage_monitor->updateConfig(coverage_config);
    ASSERT_TRUE(coverage_monitor->initialize());

    auto coverage_result = coverage_monitor->validateSystemRequirements();
    EXPECT_TRUE(coverage_result);
}

// ============================================================================
// Regression Validation Tests
// ============================================================================

TEST_F(CompatibilityValidationTest, RegressionDetection) {
    // Test regression detection in compatibility layers

    // Create baseline configurations
    CreateLegacyConfig("baseline_v1.conf");
    CreateModernAPIConfig("baseline_v2.json");

    // Load and validate both versions
    auto v1_result = config_manager_->validateConfigurationFile(
        (test_dir_ / "baseline_v1.conf").string()
    );
    auto v2_result = config_manager_->validateConfigurationFile(
        (test_dir_ / "baseline_v2.json").string()
    );

    EXPECT_TRUE(v1_result.is_valid);
    EXPECT_TRUE(v2_result.is_valid);

    // Compare configurations for regressions
    auto config_diff = config_utils::compareConfigSections(
        config_manager_->loadConfiguration((test_dir_ / "baseline_v1.conf").string()).sections,
        config_manager_->loadConfiguration((test_dir_ / "baseline_v2.json").string()).sections
    );

    // Should have differences (format migration) but no regressions
    EXPECT_TRUE(config_diff.modified_parameters.empty() ||
                std::all_of(config_diff.modified_parameters.begin(),
                             config_diff.modified_parameters.end(),
                             [](const std::string& param) {
                                 return param.find("metadata") != std::string::npos ||
                                        param.find("sections") != std::string::npos;
                             }));
}

TEST_F(CompatibilityValidationTest, VersionCompatibilityMatrix) {
    // Test version compatibility matrix
    std::vector<ConfigFormat> supported_formats = {
        ConfigFormat::LEGACY_V1, ConfigFormat::LEGACY_V2,
        ConfigFormat::JSON_V1, ConfigFormat::JSON_V2,
        ConfigFormat::YAML_V1, ConfigFormat::TOML_V1
    };

    for (const auto& source_format : supported_formats) {
        for (const auto& target_format : supported_formats) {
            // Test compatibility between all format combinations
            auto compatibility = config_manager_->validateMigrationCompatibility(
                source_format, target_format
            );

            // Should always be possible to migrate (even if it's a no-op)
            EXPECT_TRUE(compatibility.is_valid)
                << "Migration from " << static_cast<int>(source_format)
                << " to " << static_cast<int>(target_format) << " should be valid";
        }
    }
}

// ============================================================================
// Stress Validation Tests
// ============================================================================

TEST_F(CompatibilityValidationTest, StressTestMultipleConfigurations) {
    // Test stress validation with multiple configuration files
    const int num_configs = 50;

    // Create multiple configuration files
    std::vector<std::string> config_files;
    for (int i = 0; i < num_configs; ++i) {
        std::string filename = "stress_test_" + std::to_string(i) + ".conf";
        CreateLegacyConfig(filename);
        config_files.push_back((test_dir_ / filename).string());
    }

    // Validate all configurations
    int valid_configs = 0;
    int invalid_configs = 0;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (const auto& config_file : config_files) {
        auto result = config_manager_->validateConfigurationFile(config_file);
        if (result.is_valid) {
            valid_configs++;
        } else {
            invalid_configs++;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_EQ(valid_configs, num_configs);
    EXPECT_EQ(invalid_configs, 0);
    EXPECT_LT(duration.count(), 10000); // Should complete within 10 seconds
}

TEST_F(CompatibilityValidationTest, StressTestAPICompatibility) {
    // Test stress validation of API compatibility
    const int num_api_calls = 1000;

    auto api_validator = api_manager_->getCompatibilityValidator();
    int compatible_calls = 0;
    int incompatible_calls = 0;

    for (int i = 0; i < num_api_calls; ++i) {
        // Test various API calls
        std::string function_name = "set_threads";
        auto result = api_validator->validateLegacyAPICall(function_name, i % 1000);

        if (result.is_compatible) {
            compatible_calls++;
        } else {
            incompatible_calls++;
        }
    }

    // Most calls should be compatible
    EXPECT_GT(compatible_calls, incompatible_calls);
    EXPECT_EQ(compatible_calls + incompatible_calls, num_api_calls);
}

// ============================================================================
// Mock Test Scenarios
// ============================================================================

class MockCompatibilityValidatorTest : public CompatibilityValidationTest {
protected:
    void SetUp() override {
        CompatibilityValidationTest::SetUp();

        // Create mock validation scenarios
        CreateMockScenarios();
    }

    void CreateMockScenarios() {
        // Mock scenario 1: Perfect compatibility
        std::ofstream perfect_file(test_dir_ / "perfect_scenario.json");
        perfect_file << R"({
            "metadata": {"version": "2.0", "format": "json_v2"},
            "sections": {
                "gpu": {"parameters": {"threads": 256, "device": 0}},
                "processing": {"parameters": {"batch": 1000}}
            }
        })";
        perfect_file.close();

        // Mock scenario 2: Partial compatibility
        std::ofstream partial_file(test_dir_ / "partial_scenario.json");
        partial_file << R"({
            "metadata": {"version": "2.0", "format": "json_v2"},
            "sections": {
                "gpu": {"parameters": {"threads": 256, "invalid_param": "value"}},
                "processing": {"parameters": {"batch": 0}}
            }
        })";
        partial_file.close();

        // Mock scenario 3: Incompatible
        std::ofstream incompatible_file(test_dir_ / "incompatible_scenario.json");
        incompatible_file << "{ invalid json }";
        incompatible_file.close();
    }
};

TEST_F(MockCompatibilityValidatorTest, PerfectCompatibilityScenario) {
    auto result = config_manager_->validateConfigurationFile(
        (test_dir_ / "perfect_scenario.json").string()
    );

    EXPECT_TRUE(result.is_valid);
    EXPECT_TRUE(result.errors.empty());
    EXPECT_EQ(result.detected_format, ConfigFormat::JSON_V2);
}

TEST_F(MockCompatibilityValidatorTest, PartialCompatibilityScenario) {
    auto result = config_manager_->validateConfigurationFile(
        (test_dir_ / "partial_scenario.json").string()
    );

    // JSON syntax should be valid but parameter validation may fail
    EXPECT_TRUE(result.is_valid || !result.warnings.empty());
}

TEST_F(MockCompatibilityValidatorTest, IncompatibleScenario) {
    auto result = config_manager_->validateConfigurationFile(
        (test_dir_ / "incompatible_scenario.json").string()
    );

    EXPECT_FALSE(result.is_valid);
    EXPECT_FALSE(result.errors.empty());
}

// ============================================================================
// Comprehensive Compatibility Report Test
// ============================================================================

TEST_F(CompatibilityValidationTest, ComprehensiveCompatibilityReport) {
    // Generate comprehensive compatibility report
    CreateLegacyConfig("report_test.conf");
    CreateModernAPIConfig("report_test_modern.json");

    // Collect data from all compatibility systems
    std::ostringstream report;
    report << "=== COMPREHENSIVE COMPATIBILITY REPORT ===\n\n";

    // Configuration compatibility
    auto config_report = config_manager_->checkCompatibility(
        (test_dir_ / "report_test.conf").string()
    );
    report << "CONFIGURATION COMPATIBILITY:\n";
    report << "  Current Format: " << static_cast<int>(config_report.current_format) << "\n";
    report << "  Recommended Format: " << static_cast<int>(config_report.recommended_format) << "\n";
    report << "  Issues: " << config_report.compatibility_issues.size() << "\n";
    report << "  Suggestions: " << config_report.migration_suggestions.size() << "\n\n";

    // API compatibility
    auto api_validator = api_manager_->getCompatibilityValidator();
    auto api_report = api_validator->generateOverallCompatibilityReport();
    report << "API COMPATIBILITY:\n";
    report << "  Overall Score: " << api_report.overall_compatibility_score << "\n";
    report << "  Compatible APIs: " << api_report.compatible_apis.size() << "\n";
    report << "  Deprecated APIs: " << api_report.deprecated_apis.size() << "\n\n";

    // GPU architecture compatibility
    auto gpu_matrix = std::make_shared<GPUArchitectureCompatibilityMatrix>();
    gpu_matrix->initialize();
    auto gpu_report = gpu_matrix->generateSystemReport();
    report << "GPU ARCHITECTURE COMPATIBILITY:\n";
    report << "  Available Devices: " << gpu_matrix->getAvailableDevices().size() << "\n";
    report << "  Compatible Architectures: All supported\n\n";

    // Coverage compatibility
    auto coverage_monitor = coverage_manager_->getCoverageMonitor();
    CoverageConfig coverage_config;
    coverage_config.minimum_coverage_threshold = 85.0;
    coverage_config.output_directory = test_dir_.string();
    coverage_monitor->updateConfig(coverage_config);
    coverage_monitor->initialize();

    report << "COVERAGE COMPATIBILITY:\n";
    report << "  Minimum Threshold: " << coverage_config.minimum_coverage_threshold << "%\n";
    report << "  Critical Threshold: " << coverage_config.critical_coverage_threshold << "%\n\n";

    // Summary
    report << "=== SUMMARY ===\n";
    bool all_compatible = config_report.validation_result.is_valid &&
                        api_report.overall_compatibility_score > 80.0 &&
                        !gpu_report.empty();

    report << "Overall Compatibility: " << (all_compatible ? "PASS" : "NEEDS ATTENTION") << "\n";

    // Save report
    std::ofstream report_file(test_dir_ / "comprehensive_compatibility_report.txt");
    report_file << report.str();
    report_file.close();

    EXPECT_TRUE(std::filesystem::exists(test_dir_ / "comprehensive_compatibility_report.txt"));
    EXPECT_FALSE(report.str().empty());
}

} // namespace puzzle71::compatibility
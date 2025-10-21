// Puzzle71 Technical Debt Repair - SHA-256 Baseline Validator Test
// Task: T058 [P] [US3] SHA-256 Protected Baseline and Result Validation Test
// Phase: Phase 4B - User Story 3 Integration Testing and Validation System
//
// This test validates that the SHA-256 protected baseline validator works correctly
// and provides cryptographic integrity protection for all validation artifacts.

#include <gtest/gtest.h>
#include <cuda_runtime.h>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>

// Include the SHA-256 baseline validator
#include "sha256_baseline_validator.h"

using namespace puzzle71::validation;

class SHA256BaselineValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize CUDA device
        cudaError_t err = cudaSetDevice(0);
        ASSERT_EQ(cudaSuccess, err) << "Failed to set CUDA device";

        // Initialize SHA-256 baseline validator
        validator_ = std::make_unique<SHA256BaselineValidator>();

        // Configure for testing
        std::string baseline_dir = "test_sha256_baselines/";
        std::string validation_dir = "test_sha256_validations/";

        bool init_result = validator_->initialize(baseline_dir, validation_dir);
        ASSERT_TRUE(init_result) << "SHA-256 baseline validator initialization should succeed";

        // Clean up any existing test directories
        std::filesystem::remove_all(baseline_dir);
        std::filesystem::remove_all(validation_dir);

        // Re-initialize after cleanup
        init_result = validator_->initialize(baseline_dir, validation_dir);
        ASSERT_TRUE(init_result) << "SHA-256 baseline validator re-initialization should succeed";
    }

    void TearDown() override {
        validator_.reset();
        cudaDeviceReset();

        // Clean up test directories
        std::filesystem::remove_all("test_sha256_baselines/");
        std::filesystem::remove_all("test_sha256_validations/");
    }

    std::unique_ptr<SHA256BaselineValidator> validator_;
};

// Test 1: SHA-256 Hash Functionality
TEST_F(SHA256BaselineValidatorTest, T058_SHA256HashFunctionality) {
    // Test basic SHA-256 hash computation
    std::string test_data = "Hello, Puzzle71!";
    std::vector<unsigned char> hash = SHA256Hash::computeHash(test_data);

    EXPECT_EQ(hash.size(), SHA256Hash::HASH_SIZE) << "SHA-256 hash should be 32 bytes";
    EXPECT_FALSE(hash.empty()) << "Hash should not be empty";

    // Test hash consistency
    std::vector<unsigned char> hash2 = SHA256Hash::computeHash(test_data);
    EXPECT_EQ(hash, hash2) << "Same data should produce same hash";

    // Test hash string conversion
    std::string hash_string = SHA256Hash::hashToString(hash);
    EXPECT_EQ(hash_string.length(), 64) << "Hash string should be 64 hex characters";
    EXPECT_NE(hash_string.find("0x"), 0) << "Hash string should not contain 0x prefix";

    // Test reverse conversion
    std::vector<unsigned char> hash_from_string = SHA256Hash::stringToHash(hash_string);
    EXPECT_EQ(hash, hash_from_string) << "Round-trip hash conversion should be consistent";

    // Test hash verification
    bool verification_result = SHA256Hash::verifyHash(test_data, hash);
    EXPECT_TRUE(verification_result) << "Hash verification should succeed for correct data";

    // Test hash verification failure
    std::string corrupted_data = "Hello, Puzzle71!";
    corrupted_data.back() = 'X'; // Corrupt last character
    bool corrupted_verification = SHA256Hash::verifyHash(corrupted_data, hash);
    EXPECT_FALSE(corrupted_verification) << "Hash verification should fail for corrupted data";

    std::cout << "SHA-256 hash functionality: PASSED" << std::endl;
    std::cout << "  Hash size: " << hash.size() << " bytes" << std::endl;
    std::cout << "  Hash string: " << hash_string.substr(0, 16) << "..." << std::endl;
}

// Test 2: Baseline Creation and Saving
TEST_F(SHA256BaselineValidatorTest, T058_BaselineCreationAndSaving) {
    // Create test data
    std::string test_name = "ECC_Performance_Test";
    std::vector<unsigned char> test_data = {0x01, 0x02, 0x03, 0x04, 0x05};
    std::map<std::string, double> performance_metrics = {
        {"operations_per_second", 1500.0},
        {"memory_efficiency", 92.5},
        {"gpu_utilization", 85.0},
        {"precision_error", 1e-11}
    };

    // Create baseline
    BaselineEntry baseline;
    bool create_result = validator_->createBaseline(test_name, test_data, performance_metrics, baseline);

    EXPECT_TRUE(create_result) << "Baseline creation should succeed";
    EXPECT_EQ(baseline.test_name, test_name) << "Baseline test name should match";
    EXPECT_EQ(baseline.version, "1.0") << "Baseline version should be set";
    EXPECT_FALSE(baseline.data_hash.empty()) << "Data hash should be computed";
    EXPECT_FALSE(baseline.metadata_hash.empty()) << "Metadata hash should be computed";
    EXPECT_FALSE(baseline.signature_hash.empty()) << "Signature hash should be computed";
    EXPECT_EQ(baseline.performance_metrics.size(), 4) << "Performance metrics should be stored";

    // Save baseline
    bool save_result = validator_->saveBaseline(baseline);
    EXPECT_TRUE(save_result) << "Baseline saving should succeed";

    // Verify file was created
    std::string baseline_file = "test_sha256_baselines/" + test_name + ".baseline";
    EXPECT_TRUE(std::filesystem::exists(baseline_file)) << "Baseline file should exist";

    // Verify baseline is accessible
    bool has_baseline = validator_->hasBaseline(test_name);
    EXPECT_TRUE(has_baseline) << "Validator should report having the baseline";

    std::cout << "Baseline creation and saving: PASSED" << std::endl;
    std::cout << "  Test name: " << baseline.test_name << std::endl;
    std::cout << "  Version: " << baseline.version << std::endl;
    std::cout << "  Metrics count: " << baseline.performance_metrics.size() << std::endl;
}

// Test 3: Baseline Loading and Verification
TEST_F(SHA256BaselineValidatorTest, T058_BaselineLoadingAndVerification) {
    // First create and save a baseline
    std::string test_name = "Deterministic_Replay_Test";
    std::vector<unsigned char> test_data = {0x10, 0x20, 0x30, 0x40};
    std::map<std::string, double> performance_metrics = {
        {"determinism_rate", 100.0},
        {"reproduction_time_ms", 25.5},
        {"integrity_score", 1.0}
    };

    BaselineEntry original_baseline;
    validator_->createBaseline(test_name, test_data, performance_metrics, original_baseline);
    validator_->saveBaseline(original_baseline);

    // Load the baseline
    BaselineEntry loaded_baseline;
    bool load_result = validator_->loadBaseline(test_name, loaded_baseline);

    EXPECT_TRUE(load_result) << "Baseline loading should succeed";
    EXPECT_EQ(loaded_baseline.test_name, test_name) << "Loaded test name should match";
    EXPECT_EQ(loaded_baseline.version, original_baseline.version) << "Loaded version should match";
    EXPECT_EQ(loaded_baseline.data_hash, original_baseline.data_hash) << "Loaded data hash should match";
    EXPECT_EQ(loaded_baseline.metadata_hash, original_baseline.metadata_hash) << "Loaded metadata hash should match";
    EXPECT_EQ(loaded_baseline.signature_hash, original_baseline.signature_hash) << "Loaded signature hash should match";
    EXPECT_EQ(loaded_baseline.performance_metrics.size(), original_baseline.performance_metrics.size()) << "Loaded metrics count should match";

    // Verify loaded metrics
    for (const auto& [key, value] : original_baseline.performance_metrics) {
        auto it = loaded_baseline.performance_metrics.find(key);
        EXPECT_TRUE(it != loaded_baseline.performance_metrics.end()) << "Metric should exist: " << key;
        if (it != loaded_baseline.performance_metrics.end()) {
            EXPECT_DOUBLE_EQ(it->second, value) << "Metric value should match: " << key;
        }
    }

    // Verify baseline integrity
    bool integrity_result = validator_->verifyBaselineIntegrity(test_name);
    EXPECT_TRUE(integrity_result) << "Baseline integrity verification should succeed";

    std::cout << "Baseline loading and verification: PASSED" << std::endl;
    std::cout << "  Loaded baseline integrity: " << (integrity_result ? "VALID" : "INVALID") << std::endl;
    std::cout << "  Metrics verified: " << loaded_baseline.performance_metrics.size() << std::endl;
}

// Test 4: Validation Against Baseline
TEST_F(SHA256BaselineValidatorTest, T058_ValidationAgainstBaseline) {
    // Create baseline with known data and metrics
    std::string test_name = "Constitutional_Compliance_Test";
    std::vector<unsigned char> baseline_data = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    std::map<std::string, double> baseline_metrics = {
        {"compliance_score", 100.0},
        {"principles_satisfied", 6.0},
        {"violations_detected", 0.0},
        {"performance_impact", 2.5}
    };

    BaselineEntry baseline;
    validator_->createBaseline(test_name, baseline_data, baseline_metrics, baseline);
    validator_->saveBaseline(baseline);

    // Test validation with identical data (should pass)
    ValidationResult identical_result;
    std::map<std::string, double> identical_metrics = baseline_metrics; // Same metrics
    bool identical_validation = validator_->validateAgainstBaseline(
        test_name, baseline_data, identical_metrics, identical_result);

    EXPECT_TRUE(identical_validation) << "Validation with identical data should pass";
    EXPECT_TRUE(identical_result.passed) << "Result should be marked as passed";
    EXPECT_TRUE(identical_result.error_message.empty()) << "No error message should be present";
    EXPECT_GT(identical_result.execution_time_ms, 0.0) << "Execution time should be recorded";

    // Test validation with slight performance improvement (should pass)
    ValidationResult improved_result;
    std::map<std::string, double> improved_metrics = baseline_metrics;
    improved_metrics["compliance_score"] = 101.0; // Slight improvement
    improved_metrics["performance_impact"] = 2.3; // Better performance

    bool improved_validation = validator_->validateAgainstBaseline(
        test_name, baseline_data, improved_metrics, improved_result);

    EXPECT_TRUE(improved_validation) << "Validation with improvement should pass";
    EXPECT_TRUE(improved_result.passed) << "Result should be marked as passed";

    // Test validation with performance regression (should fail)
    ValidationResult regression_result;
    std::map<std::string, double> regressed_metrics = baseline_metrics;
    regressed_metrics["compliance_score"] = 95.0; // Regression
    regressed_metrics["performance_impact"] = 5.0; // Worse performance

    bool regression_validation = validator_->validateAgainstBaseline(
        test_name, baseline_data, regressed_metrics, regression_result);

    EXPECT_FALSE(regression_validation) << "Validation with regression should fail";
    EXPECT_FALSE(regression_result.passed) << "Result should be marked as failed";
    EXPECT_FALSE(regression_result.error_message.empty()) << "Error message should be present";

    // Test validation with corrupted data (should fail)
    ValidationResult corrupted_result;
    std::vector<unsigned char> corrupted_data = baseline_data;
    corrupted_data[0] ^= 0xFF; // Corrupt first byte

    bool corrupted_validation = validator_->validateAgainstBaseline(
        test_name, corrupted_data, regressed_metrics, corrupted_result);

    EXPECT_FALSE(corrupted_validation) << "Validation with corrupted data should fail";
    EXPECT_FALSE(corrupted_result.passed) << "Result should be marked as failed";

    std::cout << "Validation against baseline: PASSED" << std::endl;
    std::cout << "  Identical data validation: " << (identical_validation ? "PASSED" : "FAILED") << std::endl;
    std::cout << "  Improved metrics validation: " << (improved_validation ? "PASSED" : "FAILED") << std::endl;
    std::cout << "  Regressed metrics validation: " << (regression_validation ? "FAILED" : "PASSED") << std::endl;
    std::cout << "  Corrupted data validation: " << (corrupted_validation ? "FAILED" : "PASSED") << std::endl;
}

// Test 5: Batch Validation Operations
TEST_F(SHA256BaselineValidatorTest, T058_BatchValidationOperations) {
    // Create multiple baselines
    std::vector<std::string> test_names = {
        "ECC_Validation_Test",
        "Memory_Efficiency_Test",
        "GPU_Utilization_Test"
    };

    std::map<std::string, std::vector<unsigned char>> test_data_map;
    std::map<std::string, std::map<std::string, double>> metrics_map;

    for (size_t i = 0; i < test_names.size(); ++i) {
        std::vector<unsigned char> data = {
            static_cast<unsigned char>(0x10 + i),
            static_cast<unsigned char>(0x20 + i),
            static_cast<unsigned char>(0x30 + i),
            static_cast<unsigned char>(0x40 + i)
        };

        std::map<std::string, double> metrics = {
            {"test_metric", 100.0 + i * 10.0},
            {"efficiency", 90.0 + i * 2.0},
            {"throughput", 1000.0 + i * 500.0}
        };

        // Create and save baseline
        BaselineEntry baseline;
        validator_->createBaseline(test_names[i], data, metrics, baseline);
        validator_->saveBaseline(baseline);

        test_data_map[test_names[i]] = data;
        metrics_map[test_names[i]] = metrics;
    }

    // Perform batch validation with identical metrics (should all pass)
    std::vector<ValidationResult> results;
    bool batch_validation = validator_->validateBatchAgainstBaselines(
        test_data_map, metrics_map, results);

    EXPECT_TRUE(batch_validation) << "Batch validation should succeed";
    EXPECT_EQ(results.size(), test_names.size()) << "Should have results for all tests";

    size_t passed_count = 0;
    for (const auto& result : results) {
        if (result.passed) {
            passed_count++;
        }
    }

    EXPECT_EQ(passed_count, test_names.size()) << "All batch validation tests should pass";

    // Perform batch validation with some regressions
    std::map<std::string, std::map<std::string, double>> regressed_metrics_map = metrics_map;
    // Add regression to first test
    regressed_metrics_map[test_names[0]]["test_metric"] = 80.0; // 20% regression

    std::vector<ValidationResult> regressed_results;
    bool regressed_batch_validation = validator_->validateBatchAgainstBaselines(
        test_data_map, regressed_metrics_map, regressed_results);

    EXPECT_FALSE(regressed_batch_validation) << "Batch validation with regressions should fail";
    EXPECT_EQ(regressed_results.size(), test_names.size()) << "Should have results for all tests";

    size_t regressed_passed_count = 0;
    for (const auto& result : regressed_results) {
        if (result.passed) {
            regressed_passed_count++;
        }
    }

    EXPECT_LT(regressed_passed_count, test_names.size()) << "Some tests should fail due to regression";

    std::cout << "Batch validation operations: PASSED" << std::endl;
    std::cout << "  Identical batch validation: " << (batch_validation ? "PASSED" : "FAILED") << std::endl;
    std::cout << "  Regressed batch validation: " << (regressed_batch_validation ? "FAILED" : "PASSED") << std::endl;
    std::cout << "  Tests passed (identical): " << passed_count << "/" << test_names.size() << std::endl;
    std::cout << "  Tests passed (regressed): " << regressed_passed_count << "/" << test_names.size() << std::endl;
}

// Test 6: Baseline Integrity Verification
TEST_F(SHA256BaselineValidatorTest, T058_BaselineIntegrityVerification) {
    // Create and save multiple baselines
    std::vector<std::string> test_names = {
        "Integrity_Test_1",
        "Integrity_Test_2",
        "Integrity_Test_3"
    };

    for (const auto& test_name : test_names) {
        std::vector<unsigned char> data = {0xFF, 0xEE, 0xDD, 0xCC};
        std::map<std::string, double> metrics = {{"test_value", 42.0}};

        BaselineEntry baseline;
        validator_->createBaseline(test_name, data, metrics, baseline);
        validator_->saveBaseline(baseline);
    }

    // Verify all baselines integrity
    std::vector<std::string> corrupted_baselines;
    bool integrity_check = validator_->verifyAllBaselinesIntegrity(corrupted_baselines);

    EXPECT_TRUE(integrity_check) << "All baselines integrity check should pass";
    EXPECT_TRUE(corrupted_baselines.empty()) << "No baselines should be corrupted";
    EXPECT_EQ(validator_->getBaselineCount(), test_names.size()) << "All baselines should be accessible";

    // Verify individual baseline integrity
    for (const auto& test_name : test_names) {
        bool individual_integrity = validator_->verifyBaselineIntegrity(test_name);
        EXPECT_TRUE(individual_integrity) << "Individual baseline integrity should pass: " << test_name;
    }

    // Test baseline count and names
    size_t baseline_count = validator_->getBaselineCount();
    EXPECT_EQ(baseline_count, test_names.size()) << "Baseline count should match expected";

    std::vector<std::string> baseline_names = validator_->getBaselineNames();
    EXPECT_EQ(baseline_names.size(), test_names.size()) << "Baseline names count should match";

    // Verify all test names are present
    for (const auto& expected_name : test_names) {
        bool found = std::find(baseline_names.begin(), baseline_names.end(), expected_name) != baseline_names.end();
        EXPECT_TRUE(found) << "Expected baseline name should be found: " << expected_name;
    }

    std::cout << "Baseline integrity verification: PASSED" << std::endl;
    std::cout << "  Total baselines: " << baseline_count << std::endl;
    std::cout << "  Corrupted baselines: " << corrupted_baselines.size() << std::endl;
    std::cout << "  Individual integrity checks: " << test_names.size() << " PASSED" << std::endl;
}

// Test 7: Report Generation
TEST_F(SHA256BaselineValidatorTest, T058_ReportGeneration) {
    // Create baseline for report testing
    std::string test_name = "Report_Generation_Test";
    std::vector<unsigned char> test_data = {0x01, 0x02, 0x03, 0x04};
    std::map<std::string, double> performance_metrics = {
        {"report_metric_1", 123.45},
        {"report_metric_2", 67.89},
        {"report_metric_3", 99.99}
    };

    BaselineEntry baseline;
    validator_->createBaseline(test_name, test_data, performance_metrics, baseline);
    validator_->saveBaseline(baseline);

    // Generate baseline report
    std::string baseline_report;
    bool baseline_report_result = validator_->generateBaselineReport(test_name, baseline_report);

    EXPECT_TRUE(baseline_report_result) << "Baseline report generation should succeed";
    EXPECT_FALSE(baseline_report.empty()) << "Baseline report should not be empty";
    EXPECT_GT(baseline_report.length(), 200) << "Baseline report should be comprehensive";

    // Verify report contains expected sections
    EXPECT_TRUE(baseline_report.find("Baseline Report for: " + test_name) != std::string::npos)
        << "Report should contain baseline title";
    EXPECT_TRUE(baseline_report.find("Version: ") != std::string::npos)
        << "Report should contain version information";
    EXPECT_TRUE(baseline_report.find("Performance Metrics:") != std::string::npos)
        << "Report should contain performance metrics section";
    EXPECT_TRUE(baseline_report.find("Data Hash:") != std::string::npos)
        << "Report should contain data hash";
    EXPECT_TRUE(baseline_report.find("Signature Hash:") != std::string::npos)
        << "Report should contain signature hash";

    // Generate validation result for report testing
    ValidationResult validation_result;
    validator_->validateAgainstBaseline(test_name, test_data, performance_metrics, validation_result);

    std::string validation_report;
    bool validation_report_result = validator_->generateValidationReport(validation_result, validation_report);

    EXPECT_TRUE(validation_report_result) << "Validation report generation should succeed";
    EXPECT_FALSE(validation_report.empty()) << "Validation report should not be empty";
    EXPECT_GT(validation_report.length(), 200) << "Validation report should be comprehensive";

    // Verify validation report contains expected sections
    EXPECT_TRUE(validation_report.find("Validation Report for: " + test_name) != std::string::npos)
        << "Report should contain validation title";
    EXPECT_TRUE(validation_report.find("Status: ") != std::string::npos)
        << "Report should contain status information";
    EXPECT_TRUE(validation_report.find("Execution Time: ") != std::string::npos)
        << "Report should contain execution time";
    EXPECT_TRUE(validation_report.find("Result Hash:") != std::string::npos)
        << "Report should contain result hash";
    EXPECT_TRUE(validation_report.find("Integrity Hash:") != std::string::npos)
        << "Report should contain integrity hash";

    // Generate comprehensive integrity report
    std::string integrity_report;
    bool integrity_report_result = validator_->generateIntegrityReport(integrity_report);

    EXPECT_TRUE(integrity_report_result) << "Integrity report generation should succeed";
    EXPECT_FALSE(integrity_report.empty()) << "Integrity report should not be empty";
    EXPECT_GT(integrity_report.length(), 300) << "Integrity report should be comprehensive";

    // Verify integrity report contains expected sections
    EXPECT_TRUE(integrity_report.find("Comprehensive Baseline Integrity Report") != std::string::npos)
        << "Report should contain title";
    EXPECT_TRUE(integrity_report.find("Baseline Statistics:") != std::string::npos)
        << "Report should contain statistics section";
    EXPECT_TRUE(integrity_report.find("Integrity Status:") != std::string::npos)
        << "Report should contain integrity status";

    std::cout << "Report generation: PASSED" << std::endl;
    std::cout << "  Baseline report length: " << baseline_report.length() << " characters" << std::endl;
    std::cout << "  Validation report length: " << validation_report.length() << " characters" << std::endl;
    std::cout << "  Integrity report length: " << integrity_report.length() << " characters" << std::endl;
}

// Test 8: Performance Regression Detection
TEST_F(SHA256BaselineValidatorTest, T058_PerformanceRegressionDetection) {
    // Create baseline with known performance value
    std::string test_name = "Performance_Regression_Test";
    std::vector<unsigned char> test_data = {0x01, 0x02, 0x03};
    std::map<std::string, double> baseline_metrics = {
        {"throughput_ops_per_sec", 1000.0},
        {"latency_ms", 10.0},
        {"efficiency_percentage", 95.0}
    };

    BaselineEntry baseline;
    validator_->createBaseline(test_name, test_data, baseline_metrics, baseline);
    validator_->saveBaseline(baseline);

    // Test with no regression (same values)
    bool is_regression;
    double regression_percentage;

    bool no_regression_result = validator_->detectPerformanceRegression(
        test_name, 1000.0, 1000.0, 5.0, is_regression, regression_percentage);

    EXPECT_TRUE(no_regression_result) << "Regression detection should succeed";
    EXPECT_FALSE(is_regression) << "No regression should be detected";
    EXPECT_DOUBLE_EQ(regression_percentage, 0.0) << "Regression percentage should be zero";

    // Test with slight improvement (should not be regression)
    bool improvement_result = validator_->detectPerformanceRegression(
        test_name, 1050.0, 1000.0, 5.0, is_regression, regression_percentage);

    EXPECT_TRUE(improvement_result) << "Regression detection should succeed";
    EXPECT_FALSE(is_regression) << "Improvement should not be detected as regression";
    EXPECT_GT(regression_percentage, 0.0) << "Regression percentage should be positive (improvement)";

    // Test with slight regression (within tolerance - should not be detected)
    bool tolerance_result = validator_->detectPerformanceRegression(
        test_name, 960.0, 1000.0, 5.0, is_regression, regression_percentage);

    EXPECT_TRUE(tolerance_result) << "Regression detection should succeed";
    EXPECT_FALSE(is_regression) << "Small regression within tolerance should not be detected";
    EXPECT_LT(regression_percentage, 0.0) << "Regression percentage should be negative";
    EXPECT_GT(regression_percentage, -5.0) << "Regression should be within 5% tolerance";

    // Test with significant regression (should be detected)
    bool significant_regression_result = validator_->detectPerformanceRegression(
        test_name, 850.0, 1000.0, 5.0, is_regression, regression_percentage);

    EXPECT_TRUE(significant_regression_result) << "Regression detection should succeed";
    EXPECT_TRUE(is_regression) << "Significant regression should be detected";
    EXPECT_LT(regression_percentage, -5.0) << "Regression percentage should exceed 5% tolerance";

    std::cout << "Performance regression detection: PASSED" << std::endl;
    std::cout << "  No regression detection: " << (no_regression_result ? "PASSED" : "FAILED") << std::endl;
    std::cout << "  Improvement detection: " << (improvement_result ? "PASSED" : "FAILED") << std::endl;
    std::cout << "  Tolerance detection: " << (tolerance_result ? "PASSED" : "FAILED") << std::endl;
    std::cout << "  Significant regression detection: " << (significant_regression_result ? "PASSED" : "FAILED") << std::endl;
    std::cout << "  Last regression percentage: " << std::fixed << std::setprecision(2) << regression_percentage << "%" << std::endl;
}

// Test 9: Baseline Monitor (Basic Functionality)
TEST_F(SHA256BaselineValidatorTest, T058_BaselineMonitorBasicFunctionality) {
    // Create a baseline for monitoring
    std::string test_name = "Monitor_Test";
    std::vector<unsigned char> test_data = {0x01, 0x02, 0x03};
    std::map<std::string, double> metrics = {{"monitor_metric", 50.0}};

    BaselineEntry baseline;
    validator_->createBaseline(test_name, test_data, metrics, baseline);
    validator_->saveBaseline(baseline);

    // Create and initialize monitor
    auto monitor = std::make_unique<BaselineIntegrityMonitor>();
    bool monitor_init = monitor->initialize(validator_, 1); // 1 second interval for testing

    EXPECT_TRUE(monitor_init) << "Monitor initialization should succeed";
    EXPECT_FALSE(monitor->isMonitoringActive()) << "Monitor should not be active initially";

    // Start monitoring
    bool start_result = monitor->startMonitoring();
    EXPECT_TRUE(start_result) << "Monitor start should succeed";
    EXPECT_TRUE(monitor->isMonitoringActive()) << "Monitor should be active after start";

    // Wait a bit for monitoring to perform check
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // Stop monitoring
    bool stop_result = monitor->stopMonitoring();
    EXPECT_TRUE(stop_result) << "Monitor stop should succeed";
    EXPECT_FALSE(monitor->isMonitoringActive()) << "Monitor should not be active after stop";

    // Verify monitoring statistics
    EXPECT_GT(monitor->getTotalChecksPerformed(), 0) << "Monitor should have performed checks";

    // Perform manual integrity check
    std::vector<std::string> corrupted_baselines;
    bool manual_check = monitor->performIntegrityCheck(corrupted_baselines);

    EXPECT_TRUE(manual_check) << "Manual integrity check should succeed";
    EXPECT_TRUE(corrupted_baselines.empty()) << "No baselines should be corrupted";

    std::cout << "Baseline monitor basic functionality: PASSED" << std::endl;
    std::cout << "  Monitor initialization: " << (monitor_init ? "PASSED" : "FAILED") << std::endl;
    std::cout << "  Monitor start/stop: " << (start_result && stop_result ? "PASSED" : "FAILED") << std::endl;
    std::cout << "  Total checks performed: " << monitor->getTotalChecksPerformed() << std::endl;
    std::cout << "  Manual integrity check: " << (manual_check ? "PASSED" : "FAILED") << std::endl;
}

// SHA-256 Baseline Validator Constants Validation
TEST(SHA256BaselineValidatorConstants, T058_ConstantsValidation) {
    // Verify SHA-256 baseline validator constants are properly defined
    EXPECT_GT(baseline_constants::DEFAULT_PERFORMANCE_TOLERANCE, 0.0) << "Performance tolerance should be positive";
    EXPECT_LT(baseline_constants::DEFAULT_PERFORMANCE_TOLERANCE, 100.0) << "Performance tolerance should be reasonable";
    EXPECT_GT(baseline_constants::DEFAULT_MONITORING_INTERVAL, 0) << "Monitoring interval should be positive";
    EXPECT_GT(baseline_constants::MAX_BASELINE_SIZE_BYTES, 0) << "Max baseline size should be positive";
    EXPECT_GT(baseline_constants::MAX_BASELINE_AGE_DAYS, 0) << "Max baseline age should be positive";
    EXPECT_GT(baseline_constants::BASELINE_CACHE_SIZE, 0) << "Baseline cache size should be positive";
    EXPECT_GT(baseline_constants::SIGNIFICANCE_THRESHOLD, 0.0) << "Significance threshold should be positive";
    EXPECT_GT(baseline_constants::INTEGRITY_CHECK_RETRIES, 0) << "Integrity check retries should be positive";

    std::cout << "SHA-256 baseline validator constants validation: PASSED" << std::endl;
    std::cout << "  All " << 8 << " constants properly defined" << std::endl;
}
// Puzzle71 Technical Debt Repair - Integration Testing Framework Validation
// Task: T057 [P] [US3] Integration Testing Framework Test
// Phase: Phase 4B - User Story 3 Integration Testing and Validation System
//
// This test validates that the integration testing framework works correctly
// and can coordinate all validation frameworks together.

#include <gtest/gtest.h>
#include <cuda_runtime.h>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>

// Include the integration testing framework
#include "integration_testing_framework.h"

using namespace puzzle71::integration;

class IntegrationTestingFrameworkTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize CUDA device
        cudaError_t err = cudaSetDevice(0);
        ASSERT_EQ(cudaSuccess, err) << "Failed to set CUDA device";

        // Initialize integration testing framework
        framework_ = std::make_unique<IntegrationTestingFramework>();

        // Configure for testing
        IntegrationTestConfig config;
        config.batch_size = 1000;                     // Smaller batch for testing
        config.performance_tolerance = 0.10;          // 10% tolerance for testing
        config.precision_tolerance = 1e-8;            // Relaxed precision for testing
        config.enable_deterministic_tests = true;
        config.enableconstitutional_tests = true;
        config.enable_performance_tests = true;
        config.verbose_output = false;                  // Reduce test output
        config.test_output_dir = "test_integration_output/";
        config.baseline_dir = "test_integration_baselines/";

        bool init_result = framework_->initialize(config);
        ASSERT_TRUE(init_result) << "Integration testing framework initialization should succeed";

        // Clean up any existing test directories
        std::filesystem::remove_all(config.test_output_dir);
        std::filesystem::remove_all(config.baseline_dir);
    }

    void TearDown() override {
        framework_.reset();
        cudaDeviceReset();

        // Clean up test directories
        std::filesystem::remove_all("test_integration_output/");
        std::filesystem::remove_all("test_integration_baselines/");
    }

    std::unique_ptr<IntegrationTestingFramework> framework_;
};

// Test 1: Framework Initialization
TEST_F(IntegrationTestingFrameworkTest, T057_FrameworkInitialization) {
    // Framework should be properly initialized
    EXPECT_TRUE(framework_ != nullptr) << "Integration framework should be created";
    EXPECT_FALSE(framework_->hasErrors()) << "Framework should not have initialization errors";

    // Configuration should be accessible
    const auto& config = framework_->getConfig();
    EXPECT_EQ(config.batch_size, 1000) << "Batch size should be configured correctly";
    EXPECT_TRUE(config.enable_deterministic_tests) << "Deterministic tests should be enabled";
    EXPECT_TRUE(config.enableconstitutional_tests) << "Constitutional tests should be enabled";
    EXPECT_TRUE(config.enable_performance_tests) << "Performance tests should be enabled";

    // Validation frameworks should be available
    EXPECT_TRUE(framework_->getECCFramework() != nullptr) << "ECC framework should be available";
    EXPECT_TRUE(framework_->getDeterministicFramework() != nullptr) << "Deterministic framework should be available";
    EXPECT_TRUE(framework_->getConstitutionalFramework() != nullptr) << "Constitutional framework should be available";

    std::cout << "Integration testing framework initialization: PASSED" << std::endl;
}

// Test 2: Complete System Integration Test
TEST_F(IntegrationTestingFrameworkTest, T057_CompleteSystemIntegration) {
    IntegrationTestResult result;

    // Run complete system integration test
    bool success = framework_->testCompleteSystemIntegration(result);

    EXPECT_TRUE(success) << "Complete system integration test should pass";
    EXPECT_TRUE(result.passed) << "Integration test result should be marked as passed";
    EXPECT_GT(result.execution_time_ms, 0.0) << "Test should have execution time";
    EXPECT_TRUE(result.error_message.empty()) << "No error message should be present";

    // Performance metrics should be collected
    EXPECT_FALSE(result.performance_metrics.empty()) << "Performance metrics should be collected";

    // Check for expected performance metrics
    bool has_ecc_ops = result.performance_metrics.find("ecc_operations_per_second") != result.performance_metrics.end();
    bool has_gpu_util = result.performance_metrics.find("gpu_utilization") != result.performance_metrics.end();
    bool has_memory_eff = result.performance_metrics.find("memory_efficiency") != result.performance_metrics.end();

    EXPECT_TRUE(has_ecc_ops) << "ECC operations per second metric should be present";
    EXPECT_TRUE(has_gpu_util) << "GPU utilization metric should be present";
    EXPECT_TRUE(has_memory_eff) << "Memory efficiency metric should be present";

    std::cout << "Complete system integration test: PASSED" << std::endl;
    std::cout << "  Execution time: " << std::fixed << std::setprecision(2)
              << result.execution_time_ms << " ms" << std::endl;
    std::cout << "  Performance metrics collected: " << result.performance_metrics.size() << std::endl;
}

// Test 3: ECC Integration Tests
TEST_F(IntegrationTestingFrameworkTest, T057_ECCIntegrationTests) {
    std::vector<IntegrationTestResult> results;

    // Run ECC integration tests
    bool success = framework_->runECCIntegrationTests(results);

    EXPECT_TRUE(success) << "ECC integration tests should pass";
    EXPECT_EQ(results.size(), 3) << "Should run exactly 3 ECC integration tests";

    // Verify each test passed
    for (const auto& result : results) {
        EXPECT_TRUE(result.passed) << "ECC integration test should pass: " << result.test_name;
        EXPECT_GT(result.execution_time_ms, 0.0) << "Test should have execution time: " << result.test_name;
        EXPECT_TRUE(result.error_message.empty()) << "No error message for test: " << result.test_name;
    }

    // Verify test names
    std::vector<std::string> expected_test_names = {
        "ECCOperationsWithCPUReference",
        "ECCPrecisionAtScale",
        "ECCPerformanceConsistency"
    };

    for (const auto& expected_name : expected_test_names) {
        bool found = std::any_of(results.begin(), results.end(),
                                [&expected_name](const IntegrationTestResult& result) {
                                    return result.test_name == expected_name;
                                });
        EXPECT_TRUE(found) << "Expected ECC test should be found: " << expected_name;
    }

    std::cout << "ECC integration tests: PASSED (" << results.size() << " tests)" << std::endl;
}

// Test 4: Deterministic Replay Integration Tests
TEST_F(IntegrationTestingFrameworkTest, T057_DeterministicReplayIntegrationTests) {
    std::vector<IntegrationTestResult> results;

    // Run deterministic replay integration tests
    bool success = framework_->runDeterministicReplayIntegrationTests(results);

    EXPECT_TRUE(success) << "Deterministic replay integration tests should pass";
    EXPECT_EQ(results.size(), 3) << "Should run exactly 3 deterministic replay integration tests";

    // Verify each test passed
    for (const auto& result : results) {
        EXPECT_TRUE(result.passed) << "Deterministic replay integration test should pass: " << result.test_name;
        EXPECT_GT(result.execution_time_ms, 0.0) << "Test should have execution time: " << result.test_name;
        EXPECT_TRUE(result.error_message.empty()) << "No error message for test: " << result.test_name;
    }

    // Verify test names
    std::vector<std::string> expected_test_names = {
        "DeterministicReplayAcrossDevices",
        "DeterministicReplayWithStateCapture",
        "DeterministicReplayIntegrityProtection"
    };

    for (const auto& expected_name : expected_test_names) {
        bool found = std::any_of(results.begin(), results.end(),
                                [&expected_name](const IntegrationTestResult& result) {
                                    return result.test_name == expected_name;
                                });
        EXPECT_TRUE(found) << "Expected deterministic replay test should be found: " << expected_name;
    }

    std::cout << "Deterministic replay integration tests: PASSED (" << results.size() << " tests)" << std::endl;
}

// Test 5: Constitutional Compliance Integration Tests
TEST_F(IntegrationTestingFrameworkTest, T057_ConstitutionalComplianceIntegrationTests) {
    std::vector<IntegrationTestResult> results;

    // Run constitutional compliance integration tests
    bool success = framework_->runConstitutionalComplianceIntegrationTests(results);

    EXPECT_TRUE(success) << "Constitutional compliance integration tests should pass";
    EXPECT_EQ(results.size(), 3) << "Should run exactly 3 constitutional compliance integration tests";

    // Verify each test passed
    for (const auto& result : results) {
        EXPECT_TRUE(result.passed) << "Constitutional compliance integration test should pass: " << result.test_name;
        EXPECT_GT(result.execution_time_ms, 0.0) << "Test should have execution time: " << result.test_name;
        EXPECT_TRUE(result.error_message.empty()) << "No error message for test: " << result.test_name;
    }

    // Verify test names
    std::vector<std::string> expected_test_names = {
        "AllConstitutionalPrinciples",
        "ConstitutionalDriftDetection",
        "ConstitutionalReporting"
    };

    for (const auto& expected_name : expected_test_names) {
        bool found = std::any_of(results.begin(), results.end(),
                                [&expected_name](const IntegrationTestResult& result) {
                                    return result.test_name == expected_name;
                                });
        EXPECT_TRUE(found) << "Expected constitutional compliance test should be found: " << expected_name;
    }

    std::cout << "Constitutional compliance integration tests: PASSED (" << results.size() << " tests)" << std::endl;
}

// Test 6: Performance Regression Integration Tests
TEST_F(IntegrationTestingFrameworkTest, T057_PerformanceRegressionIntegrationTests) {
    std::vector<IntegrationTestResult> results;

    // Run performance regression integration tests
    bool success = framework_->runPerformanceRegressionIntegrationTests(results);

    EXPECT_TRUE(success) << "Performance regression integration tests should pass";
    EXPECT_EQ(results.size(), 3) << "Should run exactly 3 performance regression integration tests";

    // Verify each test passed
    for (const auto& result : results) {
        EXPECT_TRUE(result.passed) << "Performance regression integration test should pass: " << result.test_name;
        EXPECT_GT(result.execution_time_ms, 0.0) << "Test should have execution time: " << result.test_name;
        EXPECT_TRUE(result.error_message.empty()) << "No error message for test: " << result.test_name;
    }

    // Verify test names
    std::vector<std::string> expected_test_names = {
        "PerformanceBaselineComparison",
        "PerformanceAcrossArchitectures",
        "PerformanceMemoryEfficiency"
    };

    for (const auto& expected_name : expected_test_names) {
        bool found = std::any_of(results.begin(), results.end(),
                                [&expected_name](const IntegrationTestResult& result) {
                                    return result.test_name == expected_name;
                                });
        EXPECT_TRUE(found) << "Expected performance regression test should be found: " << expected_name;
    }

    std::cout << "Performance regression integration tests: PASSED (" << results.size() << " tests)" << std::endl;
}

// Test 7: All Integration Tests Combined
TEST_F(IntegrationTestingFrameworkTest, T057_AllIntegrationTestsCombined) {
    std::vector<IntegrationTestResult> results;

    // Run all integration tests
    bool success = framework_->runAllIntegrationTests(results);

    EXPECT_TRUE(success) << "All integration tests should pass";

    // Should run all individual test categories plus complete system integration
    size_t expected_min_tests = 13; // 3 + 3 + 3 + 3 + 1 (complete system) = 13
    EXPECT_GE(results.size(), expected_min_tests) << "Should run at least " << expected_min_tests << " tests";

    // Calculate success rate
    size_t passed_count = 0;
    for (const auto& result : results) {
        if (result.passed) {
            passed_count++;
        }
    }

    double success_rate = (static_cast<double>(passed_count) / results.size()) * 100.0;
    EXPECT_DOUBLE_EQ(success_rate, 100.0) << "All integration tests should pass (100% success rate)";

    // Verify no error messages
    for (const auto& result : results) {
        EXPECT_TRUE(result.error_message.empty()) << "No error messages should be present in any test: "
                                                  << result.test_name;
    }

    std::cout << "All integration tests combined: PASSED" << std::endl;
    std::cout << "  Total tests: " << results.size() << std::endl;
    std::cout << "  Passed: " << passed_count << std::endl;
    std::cout << "  Success rate: " << std::fixed << std::setprecision(1) << success_rate << "%" << std::endl;
}

// Test 8: Integration Report Generation
TEST_F(IntegrationTestingFrameworkTest, T057_IntegrationReportGeneration) {
    std::vector<IntegrationTestResult> results;

    // Run a subset of tests for report generation
    bool success = framework_->runECCIntegrationTests(results);
    ASSERT_TRUE(success) << "ECC integration tests should pass for report generation";

    // Generate integration report
    std::string report;
    bool report_success = framework_->generateIntegrationReport(results, report);

    EXPECT_TRUE(report_success) << "Integration report generation should succeed";
    EXPECT_FALSE(report.empty()) << "Generated report should not be empty";
    EXPECT_GT(report.length(), 1000) << "Report should be comprehensive";

    // Verify report contains expected sections
    EXPECT_TRUE(report.find("Integration Test Report") != std::string::npos) << "Report should contain header";
    EXPECT_TRUE(report.find("Test Summary") != std::string::npos) << "Report should contain test summary";
    EXPECT_TRUE(report.find("Individual Test Results") != std::string::npos) << "Report should contain individual results";
    EXPECT_TRUE(report.find("Performance Summary") != std::string::npos) << "Report should contain performance summary";

    // Verify test results are included
    for (const auto& result : results) {
        EXPECT_TRUE(report.find(result.test_name) != std::string::npos)
            << "Report should contain test name: " << result.test_name;
    }

    std::cout << "Integration report generation: PASSED" << std::endl;
    std::cout << "  Report length: " << report.length() << " characters" << std::endl;
}

// Test 9: Baseline Management
TEST_F(IntegrationTestingFrameworkTest, T057_BaselineManagement) {
    std::vector<IntegrationTestResult> results;

    // Run tests to get baseline data
    bool success = framework_->runECCIntegrationTests(results);
    ASSERT_TRUE(success) << "ECC integration tests should pass for baseline testing";

    // Save baselines
    std::string baseline_dir = "test_integration_baselines/";
    bool save_success = framework_->saveBaselines(baseline_dir);
    EXPECT_TRUE(save_success) << "Baseline saving should succeed";

    // Verify baseline files were created
    EXPECT_TRUE(std::filesystem::exists(baseline_dir)) << "Baseline directory should exist";

    size_t baseline_files = 0;
    for (const auto& entry : std::filesystem::directory_iterator(baseline_dir)) {
        if (entry.path().extension() == ".baseline") {
            baseline_files++;
        }
    }
    EXPECT_GT(baseline_files, 0) << "Baseline files should be created";

    // Load baselines
    bool load_success = framework_->loadBaselines(baseline_dir);
    EXPECT_TRUE(load_success) << "Baseline loading should succeed";

    std::cout << "Baseline management: PASSED" << std::endl;
    std::cout << "  Baseline files created: " << baseline_files << std::endl;
}

// Test 10: Framework Statistics
TEST_F(IntegrationTestingFrameworkTest, T057_FrameworkStatistics) {
    std::vector<IntegrationTestResult> results;

    // Run tests to get statistics
    bool success = framework_->runAllIntegrationTests(results);
    ASSERT_TRUE(success) << "All integration tests should pass for statistics testing";

    // Test statistics calculations
    size_t total = framework_->getTotalTestCount();
    size_t passed = framework_->getPassedTestCount(results);
    size_t failed = framework_->getFailedTestCount(results);
    double success_rate = framework_->getOverallSuccessRate(results);

    EXPECT_GT(total, 0) << "Total test count should be positive";
    EXPECT_EQ(passed, results.size()) << "All tests should be marked as passed";
    EXPECT_EQ(failed, 0) << "No tests should be marked as failed";
    EXPECT_DOUBLE_EQ(success_rate, 100.0) << "Success rate should be 100%";

    EXPECT_EQ(passed + failed, results.size()) << "Passed + failed should equal total results";

    std::cout << "Framework statistics: PASSED" << std::endl;
    std::cout << "  Available tests: " << total << std::endl;
    std::cout << "  Executed tests: " << results.size() << std::endl;
    std::cout << "  Passed: " << passed << std::endl;
    std::cout << "  Failed: " << failed << std::endl;
    std::cout << "  Success rate: " << std::fixed << std::setprecision(1) << success_rate << "%" << std::endl;
}

// Integration Testing Framework Constants Validation
TEST(IntegrationTestingFrameworkConstants, T057_ConstantsValidation) {
    // Verify integration testing framework constants are properly defined
    EXPECT_GT(integration_constants::DEFAULT_PERFORMANCE_TOLERANCE, 0.0) << "Performance tolerance should be positive";
    EXPECT_LT(integration_constants::DEFAULT_PERFORMANCE_TOLERANCE, 1.0) << "Performance tolerance should be reasonable";
    EXPECT_GT(integration_constants::DEFAULT_PRECISION_TOLERANCE, 0.0) << "Precision tolerance should be positive";
    EXPECT_GT(integration_constants::DEFAULT_BATCH_SIZE, 0) << "Default batch size should be positive";
    EXPECT_GT(integration_constants::DEFAULT_NUM_ITERATIONS, 0) << "Default iterations should be positive";
    EXPECT_GT(integration_constants::DEFAULT_WARMUP_TIME_MS, 0.0) << "Warmup time should be positive";
    EXPECT_GT(integration_constants::MAX_TEST_TIME_MS, 0.0) << "Max test time should be positive";
    EXPECT_GT(integration_constants::MAX_INTEGRATION_TEST_TIME_MS, 0.0) << "Max integration test time should be positive";

    // Verify performance thresholds
    EXPECT_GT(integration_constants::MIN_ECC_THROUGHPUT_OPS_PER_SEC, 0.0) << "ECC throughput threshold should be positive";
    EXPECT_GT(integration_constants::MIN_MEMORY_EFFICIENCY_PERCENTAGE, 0.0) << "Memory efficiency threshold should be positive";
    EXPECT_LE(integration_constants::MIN_MEMORY_EFFICIENCY_PERCENTAGE, 100.0) << "Memory efficiency threshold should not exceed 100%";
    EXPECT_GT(integration_constants::MIN_GPU_UTILIZATION_PERCENTAGE, 0.0) << "GPU utilization threshold should be positive";
    EXPECT_LE(integration_constants::MIN_GPU_UTILIZATION_PERCENTAGE, 100.0) << "GPU utilization threshold should not exceed 100%";
    EXPECT_GT(integration_constants::MAX_SYNCHRONIZATION_OVERHEAD_PERCENTAGE, 0.0) << "Max sync overhead should be positive";
    EXPECT_LE(integration_constants::MAX_SYNCHRONIZATION_OVERHEAD_PERCENTAGE, 100.0) << "Max sync overhead should not exceed 100%";

    std::cout << "Integration testing framework constants validation: PASSED" << std::endl;
    std::cout << "  All " << 12 << " constants properly defined" << std::endl;
}
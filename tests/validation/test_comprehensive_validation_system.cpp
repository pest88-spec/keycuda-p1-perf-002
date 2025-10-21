// Puzzle71 Technical Debt Repair - Comprehensive Validation System Test
// Task: T059 [P] [US3] Comprehensive Validation System Integration Test
// Phase: Phase 4B - User Story 3 Integration Testing and Validation System
//
// This test validates that the comprehensive validation system correctly integrates
// and orchestrates all validation frameworks with proper CI/CD integration.

#include <gtest/gtest.h>
#include <cuda_runtime.h>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

// Include all validation frameworks
#include "comprehensive_validation_system.h"
#include "ecc_validation_framework.h"
#include "deterministic_replay_framework.h"
#include "constitutional_compliance_framework.h"
#include "integration_testing_framework.h"
#include "sha256_baseline_validator.h"

using namespace puzzle71::validation;

class ComprehensiveValidationSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize CUDA device
        cudaError_t err = cudaSetDevice(0);
        ASSERT_EQ(cudaSuccess, err) << "Failed to set CUDA device";

        // Create comprehensive validation system
        validation_system_ = std::make_unique<ComprehensiveValidationSystem>();

        // Configure for testing
        ValidationSystemConfig config;
        config.enable_ecc_validation = true;
        config.enable_deterministic_replay = true;
        config.enable_constitutional_compliance = true;
        config.enable_integration_testing = true;
        config.enable_baseline_validation = true;
        config.enable_performance_analysis = true;
        config.enable_telemetry_collection = true;
        config.max_validation_time_seconds = 300; // 5 minutes for testing
        config.parallel_execution = true;
        config.continue_on_framework_failure = true;

        // Initialize the system
        ASSERT_TRUE(validation_system_->initialize(config))
            << "Failed to initialize comprehensive validation system";
    }

    void TearDown() override {
        // Cleanup test artifacts
        validation_system_->shutdown();

        // Remove test directories if they exist
        std::filesystem::remove_all("test_validation_output");
        std::filesystem::remove_all("test_ci_reports");
        std::filesystem::remove_all("test_baselines");
        std::filesystem::remove_all("test_telemetry");
    }

    // Helper method to create test validation data
    std::vector<unsigned char> createTestData(size_t size = 1024) {
        std::vector<unsigned char> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);

        for (size_t i = 0; i < size; ++i) {
            data[i] = static_cast<unsigned char>(dis(gen));
        }
        return data;
    }

    // Helper method to validate system status
    void validateSystemStatus(ValidationSystemStatus expected_status) {
        auto status = validation_system_->getSystemStatus();
        EXPECT_EQ(status, expected_status)
            << "System status mismatch. Expected: " << static_cast<int>(expected_status)
            << ", Actual: " << static_cast<int>(status);
    }

    std::unique_ptr<ComprehensiveValidationSystem> validation_system_;
};

// Test system initialization and configuration
TEST_F(ComprehensiveValidationSystemTest, SystemInitialization) {
    // Test that system initializes correctly
    EXPECT_NE(validation_system_, nullptr);
    validateSystemStatus(ValidationSystemStatus::READY);

    // Test framework availability
    EXPECT_TRUE(validation_system_->isFrameworkAvailable("ecc_validation"));
    EXPECT_TRUE(validation_system_->isFrameworkAvailable("deterministic_replay"));
    EXPECT_TRUE(validation_system_->isFrameworkAvailable("constitutional_compliance"));
    EXPECT_TRUE(validation_system_->isFrameworkAvailable("integration_testing"));
    EXPECT_TRUE(validation_system_->isFrameworkAvailable("baseline_validation"));

    // Test system statistics
    auto stats = validation_system_->getFrameworkStatistics();
    EXPECT_EQ(stats.size(), 5); // All 5 frameworks should be available
}

// Test individual framework validation
TEST_F(ComprehensiveValidationSystemTest, IndividualFrameworkValidation) {
    // Test ECC validation framework
    EXPECT_TRUE(validation_system_->validateFramework("ecc_validation"));

    // Test deterministic replay framework
    EXPECT_TRUE(validation_system_->validateFramework("deterministic_replay"));

    // Test constitutional compliance framework
    EXPECT_TRUE(validation_system_->validateFramework("constitutional_compliance"));

    // Test integration testing framework
    EXPECT_TRUE(validation_system_->validateFramework("integration_testing"));

    // Test baseline validation framework
    EXPECT_TRUE(validation_system_->validateFramework("baseline_validation"));

    // Validate invalid framework name
    EXPECT_FALSE(validation_system_->validateFramework("invalid_framework"));
}

// Test comprehensive validation orchestration
TEST_F(ComprehensiveValidationSystemTest, ComprehensiveValidationOrchestration) {
    // Run comprehensive validation
    ComprehensiveValidationResult result;
    ASSERT_TRUE(validation_system_->runComprehensiveValidation(result))
        << "Failed to run comprehensive validation";

    // Check overall result
    EXPECT_TRUE(result.overall_passed) << "Comprehensive validation should pass";
    EXPECT_TRUE(result.validation_completed) << "Validation should be completed";
    EXPECT_GT(result.total_execution_time_ms, 0) << "Execution time should be positive";

    // Check individual framework results
    EXPECT_TRUE(result.ecc_validation_passed) << "ECC validation should pass";
    EXPECT_TRUE(result.deterministic_replay_passed) << "Deterministic replay should pass";
    EXPECT_TRUE(result.constitutional_compliance_passed) << "Constitutional compliance should pass";
    EXPECT_TRUE(result.integration_testing_passed) << "Integration testing should pass";
    EXPECT_TRUE(result.baseline_validation_passed) << "Baseline validation should pass";

    // Verify no critical errors
    EXPECT_TRUE(result.critical_errors.empty()) << "Should have no critical errors";

    // Check framework integrity
    EXPECT_FALSE(result.framework_integrity_status.empty())
        << "Framework integrity status should be populated";
}

// Test CI integration functionality
TEST_F(ComprehensiveValidationSystemTest, CIIntegration) {
    // Run validation with CI integration
    ComprehensiveValidationResult result;
    ASSERT_TRUE(validation_system_->runComprehensiveValidation(result));

    // Generate CI reports
    std::string json_report, junit_report, html_report;
    ASSERT_TRUE(validation_system_->generateCIReports(result, json_report, junit_report, html_report))
        << "Failed to generate CI reports";

    // Validate JSON report
    EXPECT_FALSE(json_report.empty()) << "JSON report should not be empty";
    EXPECT_NE(json_report.find("\"overall_passed\":"), std::string::npos)
        << "JSON report should contain overall_passed field";
    EXPECT_NE(json_report.find("\"execution_time_ms\":"), std::string::npos)
        << "JSON report should contain execution time";

    // Validate JUnit report
    EXPECT_FALSE(junit_report.empty()) << "JUnit report should not be empty";
    EXPECT_NE(junit_report.find("<?xml"), std::string::npos)
        << "JUnit report should be valid XML";
    EXPECT_NE(junit_report.find("<testsuite"), std::string::npos)
        << "JUnit report should contain testsuite element";

    // Validate HTML report
    EXPECT_FALSE(html_report.empty()) << "HTML report should not be empty";
    EXPECT_NE(html_report.find("<html"), std::string::npos)
        << "HTML report should be valid HTML";
    EXPECT_NE(html_report.find("<head>"), std::string::npos)
        << "HTML report should contain head element";
}

// Test timeout and cancellation functionality
TEST_F(ComprehensiveValidationSystemTest, TimeoutAndCancellation) {
    // Configure with short timeout for testing
    ValidationSystemConfig config;
    config.enable_ecc_validation = true;
    config.enable_deterministic_replay = false; // Disable to speed up
    config.enable_constitutional_compliance = false; // Disable to speed up
    config.enable_integration_testing = false; // Disable to speed up
    config.enable_baseline_validation = false; // Disable to speed up
    config.max_validation_time_seconds = 1; // 1 second timeout

    // Reinitialize with short timeout
    validation_system_->shutdown();
    ASSERT_TRUE(validation_system_->initialize(config));

    // Run validation (should timeout)
    ComprehensiveValidationResult result;
    EXPECT_TRUE(validation_system_->runComprehensiveValidation(result))
        << "Validation should complete but with timeout status";

    // Check that timeout was handled
    EXPECT_TRUE(result.validation_completed || result.critical_errors.size() > 0)
        << "Validation should either complete or have timeout errors";
}

// Test performance analysis and regression detection
TEST_F(ComprehensiveValidationSystemTest, PerformanceAnalysis) {
    // Run validation to generate performance data
    ComprehensiveValidationResult result;
    ASSERT_TRUE(validation_system_->runComprehensiveValidation(result));

    // Check performance metrics
    EXPECT_FALSE(result.aggregate_performance_metrics.empty())
        << "Should have aggregated performance metrics";

    // Check for performance regressions and improvements
    EXPECT_FALSE(result.performance_regressions.empty() || result.performance_improvements.empty())
        << "Should detect either regressions or improvements";

    // Validate constitutional compliance metrics
    EXPECT_FALSE(result.constitutional_compliance_summary.empty())
        << "Should have constitutional compliance summary";
}

// Test telemetry collection
TEST_F(ComprehensiveValidationSystemTest, TelemetryCollection) {
    // Enable telemetry collection
    ComprehensiveValidationResult result;
    ASSERT_TRUE(validation_system_->runComprehensiveValidation(result));

    // Check that telemetry was collected
    EXPECT_FALSE(result.telemetry_data.empty())
        << "Telemetry data should be collected";
}

// Test baseline management integration
TEST_F(ComprehensiveValidationSystemTest, BaselineManagementIntegration) {
    // Create test baseline data
    std::vector<unsigned char> test_data = createTestData(2048);
    BaselineEntry baseline;

    // Initialize baseline validator
    auto baseline_validator = validation_system_->getBaselineValidator();
    ASSERT_NE(baseline_validator, nullptr) << "Baseline validator should be available";

    // Create baseline
    std::map<std::string, double> metrics = {{"throughput", 1000.0}, {"latency", 10.5}};
    EXPECT_TRUE(baseline_validator->createBaseline("test_baseline", test_data, metrics, baseline))
        << "Should create test baseline";

    // Run validation that includes baseline verification
    ComprehensiveValidationResult result;
    ASSERT_TRUE(validation_system_->runComprehensiveValidation(result));

    // Verify baseline validation was performed
    EXPECT_TRUE(result.baseline_validation_passed)
        << "Baseline validation should pass";
}

// Test progress monitoring
TEST_F(ComprehensiveValidationSystemTest, ProgressMonitoring) {
    // Start validation in background to test progress monitoring
    stdatomic<bool> validation_completed{false};
    ComprehensiveValidationResult result;

    std::thread validation_thread([&]() {
        EXPECT_TRUE(validation_system_->runComprehensiveValidation(result));
        validation_completed = true;
    });

    // Monitor progress for a short time
    auto start_time = std::chrono::steady_clock::now();
    while (!validation_completed &&
           std::chrono::steady_clock::now() - start_time < std::chrono::seconds(5)) {

        double progress = validation_system_->getValidationProgress();
        EXPECT_GE(progress, 0.0) << "Progress should be non-negative";
        EXPECT_LE(progress, 1.0) << "Progress should not exceed 1.0";

        // Check if validation is in progress
        bool in_progress = validation_system_->isValidationInProgress();
        if (!in_progress) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Wait for completion
    if (validation_thread.joinable()) {
        validation_thread.join();
    }

    // Final progress should be 1.0 (complete)
    EXPECT_DOUBLE_EQ(validation_system_->getValidationProgress(), 1.0)
        << "Final progress should be 1.0";
}

// Test error handling and recovery
TEST_F(ComprehensiveValidationSystemTest, ErrorHandlingAndRecovery) {
    // Test with invalid configuration
    ValidationSystemConfig invalid_config;
    invalid_config.enable_ecc_validation = false;
    invalid_config.enable_deterministic_replay = false;
    invalid_config.enable_constitutional_compliance = false;
    invalid_config.enable_integration_testing = false;
    invalid_config.enable_baseline_validation = false;

    // Reinitialize with invalid config
    validation_system_->shutdown();
    EXPECT_TRUE(validation_system_->initialize(invalid_config))
        << "Should initialize even with all frameworks disabled";

    // Run validation (should complete quickly with no frameworks)
    ComprehensiveValidationResult result;
    EXPECT_TRUE(validation_system_->runComprehensiveValidation(result))
        << "Validation should complete even with no frameworks enabled";

    // Restore valid configuration
    ASSERT_TRUE(validation_system_->initialize(ValidationSystemConfig{}))
        << "Should restore to valid configuration";
}

// Test multi-GPU support
TEST_F(ComprehensiveValidationSystemTest, MultiGPUSupport) {
    // Check available GPUs
    int device_count = 0;
    cudaError_t err = cudaGetDeviceCount(&device_count);
    ASSERT_EQ(cudaSuccess, err) << "Failed to get device count";

    if (device_count > 1) {
        // Test with multiple GPUs
        for (int device = 0; device < device_count; ++device) {
            validation_system_->selectGPUDevice(device);

            ComprehensiveValidationResult result;
            EXPECT_TRUE(validation_system_->runComprehensiveValidation(result))
                << "Validation should work on GPU " << device;
        }
    } else {
        // Single GPU system - test device selection
        validation_system_->selectGPUDevice(0);
        ComprehensiveValidationResult result;
        EXPECT_TRUE(validation_system_->runComprehensiveValidation(result))
            << "Validation should work on single GPU";
    }
}

// Test concurrent validation execution
TEST_F(ComprehensiveValidationSystemTest, ConcurrentValidation) {
    // Test running multiple validations concurrently (if supported)
    if (validation_system_->supportsConcurrentExecution()) {
        std::vector<std::future<ComprehensiveValidationResult>> futures;

        // Start multiple validation runs
        for (int i = 0; i < 3; ++i) {
            std::promise<ComprehensiveValidationResult> promise;
            futures.push_back(promise.get_future());

            std::thread thread([this, promise = std::move(promise)]() mutable {
                ComprehensiveValidationResult result;
                EXPECT_TRUE(validation_system_->runComprehensiveValidation(result));
                promise.set_value(std::move(result));
            });
            thread.detach();
        }

        // Wait for all validations to complete
        for (auto& future : futures) {
            auto result = future.get();
            EXPECT_TRUE(result.validation_completed)
                << "Concurrent validation should complete";
        }
    }
}

// Test system diagnostics
TEST_F(ComprehensiveValidationSystemTest, SystemDiagnostics) {
    // Generate system diagnostics
    std::string diagnostics;
    EXPECT_TRUE(validation_system_->generateDiagnostics(diagnostics))
        << "Should generate system diagnostics";

    EXPECT_FALSE(diagnostics.empty()) << "Diagnostics should not be empty";
    EXPECT_NE(diagnostics.find("System Status:"), std::string::npos)
        << "Diagnostics should contain system status";
    EXPECT_NE(diagnostics.find("Framework Statistics:"), std::string::npos)
        << "Diagnostics should contain framework statistics";
}

// Performance benchmark test
TEST_F(ComprehensiveValidationSystemTest, PerformanceBenchmark) {
    const int num_iterations = 5;
    std::vector<double> execution_times;

    for (int i = 0; i < num_iterations; ++i) {
        auto start_time = std::chrono::high_resolution_clock::now();

        ComprehensiveValidationResult result;
        EXPECT_TRUE(validation_system_->runComprehensiveValidation(result))
            << "Validation should complete in iteration " << i;

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        execution_times.push_back(duration.count());
    }

    // Calculate statistics
    double total_time = 0;
    for (double time : execution_times) {
        total_time += time;
    }
    double average_time = total_time / num_iterations;

    // Performance should be reasonable (less than 2 minutes average for testing)
    EXPECT_LT(average_time, 120000.0) << "Average validation time should be reasonable";

    // Variance should not be too high (consistent performance)
    double variance = 0;
    for (double time : execution_times) {
        variance += (time - average_time) * (time - average_time);
    }
    variance /= num_iterations;
    double std_deviation = std::sqrt(variance);

    EXPECT_LT(std_deviation, average_time * 0.5)
        << "Performance should be relatively consistent";
}

// Integration test with all frameworks enabled
TEST_F(ComprehensiveValidationSystemTest, FullIntegrationTest) {
    // Enable all frameworks and features
    ValidationSystemConfig full_config;
    full_config.enable_ecc_validation = true;
    full_config.enable_deterministic_replay = true;
    full_config.enable_constitutional_compliance = true;
    full_config.enable_integration_testing = true;
    full_config.enable_baseline_validation = true;
    full_config.enable_performance_analysis = true;
    full_config.enable_telemetry_collection = true;
    full_config.parallel_execution = true;
    full_config.continue_on_framework_failure = true;

    // Reinitialize with full configuration
    validation_system_->shutdown();
    ASSERT_TRUE(validation_system_->initialize(full_config));

    // Run comprehensive validation
    ComprehensiveValidationResult result;
    ASSERT_TRUE(validation_system_->runComprehensiveValidation(result))
        << "Full comprehensive validation should succeed";

    // Validate all components worked together
    EXPECT_TRUE(result.overall_passed) << "Overall validation should pass";
    EXPECT_TRUE(result.validation_completed) << "Validation should complete";
    EXPECT_GT(result.total_execution_time_ms, 0) << "Should have execution time";

    // All framework results should be populated
    EXPECT_TRUE(result.ecc_validation_passed) << "ECC validation should pass";
    EXPECT_TRUE(result.deterministic_replay_passed) << "Deterministic replay should pass";
    EXPECT_TRUE(result.constitutional_compliance_passed) << "Constitutional compliance should pass";
    EXPECT_TRUE(result.integration_testing_passed) << "Integration testing should pass";
    EXPECT_TRUE(result.baseline_validation_passed) << "Baseline validation should pass";

    // Should have comprehensive metrics and analysis
    EXPECT_FALSE(result.aggregate_performance_metrics.empty())
        << "Should have performance metrics";
    EXPECT_FALSE(result.constitutional_compliance_summary.empty())
        << "Should have constitutional compliance summary";
    EXPECT_FALSE(result.telemetry_data.empty())
        << "Should have telemetry data";
}
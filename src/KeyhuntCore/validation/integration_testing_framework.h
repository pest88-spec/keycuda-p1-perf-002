// Puzzle71 Technical Debt Repair - Integration Testing Framework
// Task: T057 [P] [US3] Implement automated performance regression detection
// Phase: Phase 4B - User Story 3 Integration Testing and Validation System
//
// This framework provides comprehensive integration testing capabilities
// to validate the complete technical debt repair system functionality.

#pragma once

#include <cuda_runtime.h>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <functional>
#include <map>

// Include the validation frameworks
#include "ecc_validation_simple.h"
#include "deterministic_replay_simple.h"
#include "constitutional_compliance_simple.h"

namespace puzzle71 {
namespace integration {

// Integration test result structure
struct IntegrationTestResult {
    std::string test_name;
    bool passed;
    double execution_time_ms;
    std::string error_message;
    std::map<std::string, double> performance_metrics;

    IntegrationTestResult() : passed(false), execution_time_ms(0.0) {}
};

// Integration test suite configuration
struct IntegrationTestConfig {
    size_t batch_size = 10000;                    // Default batch size for tests
    double performance_tolerance = 0.05;          // 5% performance tolerance
    double precision_tolerance = 1e-10;           // ECC precision tolerance
    int num_gpu_devices = -1;                     // Test all available GPUs
    bool enable_deterministic_tests = true;       // Enable deterministic replay tests
    bool enableconstitutional_tests = true;        // Enable constitutional compliance tests
    bool enable_performance_tests = true;         // Enable performance regression tests
    std::string test_output_dir = "test_output/"; // Output directory for test artifacts
    std::string baseline_dir = "baselines/";      // Directory for baseline files

    // GPU device filters
    std::vector<int> target_gpu_devices;          // Specific GPU devices to test (empty = all)

    // Test configuration
    bool verbose_output = false;                  // Enable verbose logging
    bool generate_reports = true;                  // Generate comprehensive reports
    bool save_artifacts = true;                    // Save test artifacts for analysis
};

// Integration test framework main class
class IntegrationTestingFramework {
public:
    IntegrationTestingFramework();
    ~IntegrationTestingFramework();

    // Initialize the framework
    bool initialize(const IntegrationTestConfig& config = IntegrationTestConfig{});

    // Core integration test execution
    bool runAllIntegrationTests(std::vector<IntegrationTestResult>& results);
    bool runECCIntegrationTests(std::vector<IntegrationTestResult>& results);
    bool runDeterministicReplayIntegrationTests(std::vector<IntegrationTestResult>& results);
    bool runConstitutionalComplianceIntegrationTests(std::vector<IntegrationTestResult>& results);
    bool runPerformanceRegressionIntegrationTests(std::vector<IntegrationTestResult>& results);

    // Cross-framework integration tests
    bool testECCWithDeterministicReplay(IntegrationTestResult& result);
    bool testConstitutionalComplianceWithECC(IntegrationTestResult& result);
    bool testPerformanceWithDeterministicReplay(IntegrationTestResult& result);
    bool testCompleteSystemIntegration(IntegrationTestResult& result);

    // Multi-GPU integration testing
    bool testMultiGPUConsistency(std::vector<IntegrationTestResult>& results);
    bool testCrossDeviceDeterministicReplay(std::vector<IntegrationTestResult>& results);

    // Configuration and baseline management
    bool loadBaselines(const std::string& baseline_dir);
    bool saveBaselines(const std::string& baseline_dir);
    bool compareWithBaselines(const std::vector<IntegrationTestResult>& results,
                             std::vector<IntegrationTestResult>& regressions);

    // Report generation
    bool generateIntegrationReport(const std::vector<IntegrationTestResult>& results,
                                  std::string& report);
    bool generatePerformanceReport(const std::vector<IntegrationTestResult>& results,
                                   std::string& report);
    bool generateComplianceReport(const std::vector<IntegrationTestResult>& results,
                                 std::string& report);

    // Validation framework access
    std::shared_ptr<puzzle71::validation::ECCValidationFramework> getECCFramework() { return ecc_framework_; }
    std::shared_ptr<puzzle71::validation::DeterministicReplayFramework> getDeterministicFramework() { return deterministic_framework_; }
    std::shared_ptr<puzzle71::validation::ConstitutionalComplianceFramework> getConstitutionalFramework() { return constitutional_framework_; }

    // Configuration access
    const IntegrationTestConfig& getConfig() const { return config_; }
    void setConfig(const IntegrationTestConfig& config) { config_ = config; }

    // Error handling and status
    std::string getLastError() const { return last_error_; }
    bool hasErrors() const { return !last_error_.empty(); }

    // Utility methods
    size_t getTotalTestCount() const;
    size_t getPassedTestCount(const std::vector<IntegrationTestResult>& results) const;
    size_t getFailedTestCount(const std::vector<IntegrationTestResult>& results) const;
    double getOverallSuccessRate(const std::vector<IntegrationTestResult>& results) const;

private:
    // Internal test execution helpers
    bool executeECCValidationTest(const std::string& test_name,
                                 std::function<bool()> test_function,
                                 IntegrationTestResult& result);
    bool executeDeterministicReplayTest(const std::string& test_name,
                                       std::function<bool()> test_function,
                                       IntegrationTestResult& result);
    bool executeConstitutionalTest(const std::string& test_name,
                                  std::function<bool()> test_function,
                                  IntegrationTestResult& result);
    bool executePerformanceTest(const std::string& test_name,
                              std::function<bool(std::map<std::string, double>&)> test_function,
                              IntegrationTestResult& result);

    // GPU device management
    bool initializeGPUDevices();
    std::vector<int> getAvailableGPUDevices();
    bool setGPUDevice(int device_id);

    // Test data generation
    std::vector<std::vector<unsigned char>> generateTestBatch(size_t batch_size, size_t data_size = 32);
    bool saveTestArtifacts(const std::string& test_name, const std::vector<std::vector<unsigned char>>& data);
    bool loadTestArtifacts(const std::string& test_name, std::vector<std::vector<unsigned char>>& data);

    // Performance measurement
    std::chrono::high_resolution_clock::time_point startTiming();
    double endTiming(const std::chrono::high_resolution_clock::time_point& start_time);

    // Report generation helpers
    std::string generateTestSummary(const std::vector<IntegrationTestResult>& results);
    std::string generatePerformanceSummary(const std::vector<IntegrationTestResult>& results);
    std::string generateComplianceSummary(const std::vector<IntegrationTestResult>& results);
    std::string generateErrorReport(const std::vector<IntegrationTestResult>& results);

    // Error handling
    void setError(const std::string& error);
    void clearError();

    // Baseline management helpers
    struct BaselineData {
        std::string test_name;
        std::map<std::string, double> performance_metrics;
        double execution_time_ms;
        std::string timestamp;
    };

    bool saveBaselineData(const BaselineData& baseline, const std::string& filepath);
    bool loadBaselineData(const std::string& filepath, BaselineData& baseline);
    bool compareWithBaseline(const IntegrationTestResult& result, const BaselineData& baseline,
                           bool& is_regression, double& regression_percentage);

private:
    // Configuration
    IntegrationTestConfig config_;

    // Validation frameworks
    std::shared_ptr<puzzle71::validation::ECCValidationFramework> ecc_framework_;
    std::shared_ptr<puzzle71::validation::DeterministicReplayFramework> deterministic_framework_;
    std::shared_ptr<puzzle71::validation::ConstitutionalComplianceFramework> constitutional_framework_;

    // GPU device management
    std::vector<int> available_gpu_devices_;
    int current_gpu_device_;

    // Baseline data
    std::map<std::string, BaselineData> baseline_data_;

    // Error handling
    std::string last_error_;

    // Framework state
    bool initialized_;

    // CUDA resources
    std::vector<cudaStream_t> cuda_streams_;
};

// Integration test factory for creating specific test types
namespace integration_tests {

    // ECC integration tests
    bool testECCOperationsWithCPUReference(IntegrationTestingFramework* framework,
                                           IntegrationTestResult& result);
    bool testECCPrecisionAtScale(IntegrationTestingFramework* framework,
                                IntegrationTestResult& result);
    bool testECCPerformanceConsistency(IntegrationTestingFramework* framework,
                                      IntegrationTestResult& result);

    // Deterministic replay integration tests
    bool testDeterministicReplayAcrossDevices(IntegrationTestingFramework* framework,
                                              IntegrationTestResult& result);
    bool testDeterministicReplayWithStateCapture(IntegrationTestingFramework* framework,
                                                 IntegrationTestResult& result);
    bool testDeterministicReplayIntegrityProtection(IntegrationTestingFramework* framework,
                                                    IntegrationTestResult& result);

    // Constitutional compliance integration tests
    bool testAllConstitutionalPrinciples(IntegrationTestingFramework* framework,
                                         IntegrationTestResult& result);
    bool testConstitutionalDriftDetection(IntegrationTestingFramework* framework,
                                         IntegrationTestResult& result);
    bool testConstitutionalReporting(IntegrationTestingFramework* framework,
                                    IntegrationTestResult& result);

    // Performance regression integration tests
    bool testPerformanceBaselineComparison(IntegrationTestingFramework* framework,
                                          IntegrationTestResult& result);
    bool testPerformanceAcrossArchitectures(IntegrationTestingFramework* framework,
                                           IntegrationTestResult& result);
    bool testPerformanceMemoryEfficiency(IntegrationTestingFramework* framework,
                                        IntegrationTestResult& result);

    // System-wide integration tests
    bool testCompleteValidationPipeline(IntegrationTestingFramework* framework,
                                       IntegrationTestResult& result);
    bool testSystemUnderLoad(IntegrationTestingFramework* framework,
                             IntegrationTestResult& result);
    bool testSystemRecovery(IntegrationTestingFramework* framework,
                            IntegrationTestResult& result);
}

// Utility functions for integration testing
namespace integration_utils {

    // Test data generation
    std::vector<std::vector<unsigned char>> generateDeterministicTestData(size_t batch_size,
                                                                          unsigned int seed = 12345);
    std::vector<std::vector<unsigned char>> generateRandomTestData(size_t batch_size,
                                                                   size_t data_size = 32);

    // Performance analysis
    double calculatePerformanceRegression(double current_value, double baseline_value);
    bool isPerformanceRegressing(double current_value, double baseline_value, double tolerance);
    std::string formatPerformanceMetric(const std::string& metric_name, double value, const std::string& unit = "");

    // Report formatting
    std::string formatTestResult(const IntegrationTestResult& result);
    std::string formatPerformanceMetrics(const std::map<std::string, double>& metrics);
    std::string generateTimestamp();
    std::string generateTestReportHeader(const std::string& test_suite_name);

    // File I/O helpers
    bool ensureDirectoryExists(const std::string& dir_path);
    bool writeTextFile(const std::string& filepath, const std::string& content);
    bool readTextFile(const std::string& filepath, std::string& content);
    bool appendToTextFile(const std::string& filepath, const std::string& content);
}

// Integration test constants
namespace integration_constants {
    constexpr double DEFAULT_PERFORMANCE_TOLERANCE = 0.05;      // 5%
    constexpr double DEFAULT_PRECISION_TOLERANCE = 1e-10;
    constexpr size_t DEFAULT_BATCH_SIZE = 10000;
    constexpr int DEFAULT_NUM_ITERATIONS = 5;
    constexpr double DEFAULT_WARMUP_TIME_MS = 1000.0;          // 1 second warmup

    // Test timeouts
    constexpr double MAX_TEST_TIME_MS = 60000.0;               // 60 seconds
    constexpr double MAX_INTEGRATION_TEST_TIME_MS = 300000.0;   // 5 minutes

    // Performance thresholds
    constexpr double MIN_ECC_THROUGHPUT_OPS_PER_SEC = 1000.0;
    constexpr double MIN_MEMORY_EFFICIENCY_PERCENTAGE = 90.0;
    constexpr double MIN_GPU_UTILIZATION_PERCENTAGE = 70.0;
    constexpr double MAX_SYNCHRONIZATION_OVERHEAD_PERCENTAGE = 50.0;
}

} // namespace integration
} // namespace puzzle71
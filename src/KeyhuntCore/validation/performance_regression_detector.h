// Puzzle71 Technical Debt Repair - Automated Performance Regression Detection System
// Task: T057 [P] [US3] Implement automated performance regression detection
// Phase: Phase 4 - User Story 3 Integration Testing and Validation System

#pragma once

#include <cuda_runtime.h>
#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <chrono>
#include <functional>
#include <map>

namespace puzzle71 {
namespace validation {

// Performance baseline data structure
struct PerformanceBaseline {
    std::string benchmark_id;                // Unique benchmark identifier
    std::string gpu_device;                   // GPU device name
    std::string cuda_driver_version;          // CUDA driver version
    std::string build_configuration;          // Build configuration details
    double baseline_throughput_mkeys_per_sec; // Baseline throughput
    double baseline_memory_efficiency;       // Baseline memory efficiency
    double baseline_gpu_utilization;         // Baseline GPU utilization
    double baseline_execution_time_ms;       // Baseline execution time
    std::chrono::system_clock::time_point timestamp; // Baseline creation time
    std::vector<uint8_t> sha256_digest;      // SHA-256 digest for integrity

    // Performance statistics
    double mean_throughput;                   // Mean throughput
    double std_deviation_throughput;          // Standard deviation
    size_t sample_count;                      // Number of samples
    std::vector<double> sample_data;          // Individual sample data
};

// Performance regression result
struct PerformanceRegressionResult {
    std::string benchmark_id;                 // Benchmark identifier
    double current_throughput;                // Current measured throughput
    double baseline_throughput;               // Baseline throughput
    double regression_percentage;              // Performance regression percentage
    bool regression_detected;                 // Whether regression is detected
    double z_score;                          // Statistical significance (Z-score)
    double confidence_interval;               // Confidence interval for result
    std::string regression_severity;          // Severity level (CRITICAL, HIGH, MEDIUM, LOW)
    std::string regression_details;           // Detailed regression information
    std::vector<std::string> affected_metrics; // List of affected performance metrics
    std::chrono::system_clock::time_point detection_time; // Detection timestamp
};

// Performance metric types
enum class PerformanceMetric {
    THROUGHPUT_MKEYS_PER_SEC,
    MEMORY_EFFICIENCY_PERCENT,
    GPU_UTILIZATION_PERCENT,
    EXECUTION_TIME_MS,
    MEMORY_BANDWIDTH_GB_PER_SEC,
    POWER_CONSUMPTION_WATTS
};

// Regression detection configuration
struct RegressionDetectionConfig {
    double regression_threshold_percentage = 5.0;   // Regression threshold
    double statistical_significance_level = 0.05; // Statistical significance (p-value)
    size_t minimum_sample_count = 20;               // Minimum samples for detection
    double z_score_threshold = 2.0;                // Z-score threshold for significance
    bool enable_sha256_validation = true;           // Enable SHA-256 integrity checks
    bool enable_statistical_analysis = true;       // Enable statistical analysis
    bool enable_trend_analysis = true;              // Enable trend analysis
    size_t max_baseline_age_days = 30;              // Maximum baseline age
    std::vector<PerformanceMetric> monitored_metrics; // Metrics to monitor

    RegressionDetectionConfig() {
        monitored_metrics = {
            PerformanceMetric::THROUGHPUT_MKEYS_PER_SEC,
            PerformanceMetric::MEMORY_EFFICIENCY_PERCENT,
            PerformanceMetric::GPU_UTILIZATION_PERCENT
        };
    }
};

// Automated performance regression detector
class PerformanceRegressionDetector {
public:
    PerformanceRegressionDetector();
    ~PerformanceRegressionDetector();

    // Initialize the detector
    bool initialize(const RegressionDetectionConfig& config = RegressionDetectionConfig{});

    // Load existing performance baselines
    bool loadBaselines(const std::string& baseline_directory = "baselines/");

    // Save performance baselines
    bool saveBaselines(const std::string& baseline_directory = "baselines/");

    // Create new performance baseline
    bool createBaseline(const std::string& benchmark_id,
                        std::function<double()> benchmark_function,
                        size_t sample_count = 50);

    // Detect performance regression for a specific benchmark
    bool detectRegression(const std::string& benchmark_id,
                          std::function<double()> benchmark_function,
                          PerformanceRegressionResult& result);

    // Batch regression detection for multiple benchmarks
    bool detectRegressionsBatch(const std::vector<std::string>& benchmark_ids,
                                 const std::vector<std::function<double()>>& benchmark_functions,
                                 std::vector<PerformanceRegressionResult>& results);

    // Validate baseline integrity with SHA-256
    bool validateBaselineIntegrity(const std::string& benchmark_id);

    // Update existing baseline (for legitimate performance improvements)
    bool updateBaseline(const std::string& benchmark_id,
                        std::function<double()> benchmark_function,
                        const std::string& justification);

    // Generate regression detection report
    bool generateRegressionReport(std::string& report);

    // Get list of available baselines
    std::vector<std::string> getAvailableBaselines() const;

    // Get regression statistics
    bool getRegressionStatistics(size_t& total_detections,
                                 size_t& critical_regressions,
                                 size_t& high_regressions,
                                 double& average_regression_percentage);

    // Check if detector is ready for regression detection
    bool isReady() const { return initialized_ && !baselines_.empty(); }

    // Get last error
    std::string getLastError() const { return last_error_; }

private:
    // Internal helper methods
    bool executeBenchmarkSamples(std::function<double()> benchmark_function,
                                  size_t sample_count,
                                  std::vector<double>& samples);

    bool calculateStatistics(const std::vector<double>& samples,
                             double& mean,
                             double& std_deviation);

    bool performStatisticalTest(const std::vector<double>& baseline_samples,
                                const std::vector<double>& current_samples,
                                double& z_score,
                                double& p_value);

    bool calculateRegressionSeverity(double regression_percentage,
                                    double z_score,
                                    std::string& severity);

    bool computeSHA256Digest(const std::vector<uint8_t>& data, std::vector<uint8_t>& digest);

    bool serializeBaseline(const PerformanceBaseline& baseline, std::vector<uint8_t>& data);
    bool deserializeBaseline(const std::vector<uint8_t>& data, PerformanceBaseline& baseline);

    std::string getBaselineFilePath(const std::string& benchmark_id) const;

    void setError(const std::string& error);
    void clearError();

private:
    bool initialized_;
    std::string last_error_;
    RegressionDetectionConfig config_;

    // Baseline storage
    std::map<std::string, PerformanceBaseline> baselines_;
    std::string baseline_directory_;

    // CUDA resources
    int cuda_device_id_;
    cudaStream_t cuda_stream_;

    // Statistics
    std::vector<PerformanceRegressionResult> detection_history_;
    size_t total_detections_;
    size_t critical_regressions_;
    size_t high_regressions_;
};

// Utility functions for performance regression detection
namespace performance_regression_utils {

    // Statistical analysis utilities
    double calculateMean(const std::vector<double>& data);
    double calculateStandardDeviation(const std::vector<double>& data, double mean);
    double calculateZScore(double value, double mean, double std_deviation);
    double calculatePValue(double z_score);

    // Performance comparison utilities
    double calculatePercentageChange(double baseline, double current);
    bool isSignificantRegression(double regression_percentage, double z_score,
                                double threshold_percentage, double z_threshold);

    // Trend analysis utilities
    enum class TrendDirection {
        IMPROVING,
        STABLE,
        DEGRADING
    };

    TrendDirection analyzeTrend(const std::vector<double>& historical_data);
    double calculateTrendSlope(const std::vector<double>& data);

    // SHA-256 integrity utilities
    bool computeSHA256(const std::vector<uint8_t>& data, std::vector<uint8_t>& hash);
    bool verifySHA256Digest(const std::vector<uint8_t>& data, const std::vector<uint8_t>& expected_hash);

    // File I/O utilities
    bool readBinaryFile(const std::string& file_path, std::vector<uint8_t>& data);
    bool writeBinaryFile(const std::string& file_path, const std::vector<uint8_t>& data);

    // String utilities
    std::string formatThroughput(double throughput_mkeys_per_sec);
    std::string formatPercentage(double percentage);
    std::string formatDuration(double milliseconds);
    std::string getCurrentTimestamp();

    // Report generation utilities
    std::string generateJSONReport(const std::vector<PerformanceRegressionResult>& results);
    std::string generateMarkdownReport(const std::vector<PerformanceRegressionResult>& results);
    std::string generateCSVReport(const std::vector<PerformanceRegressionResult>& results);

    // Performance benchmark utilities
    class BenchmarkTimer {
    public:
        BenchmarkTimer();
        void start();
        void stop();
        double getElapsedMilliseconds() const;
        double getElapsedSeconds() const;

    private:
        std::chrono::high_resolution_clock::time_point start_time_;
        std::chrono::high_resolution_clock::time_point end_time_;
        bool running_;
    };

    // GPU performance measurement utilities
    bool measureGPUThroughput(std::function<void()> kernel_function,
                              double& throughput_mkeys_per_sec);
    bool measureGPUMemoryEfficiency(double& efficiency_percentage);
    bool measureGPUUtilization(double& utilization_percentage);

    // Baseline management utilities
    bool createBaselineDirectory(const std::string& directory);
    bool validateBaselineDirectory(const std::string& directory);
    std::vector<std::string> discoverBaselineFiles(const std::string& directory);

    // Configuration utilities
    RegressionDetectionConfig loadConfigurationFromFile(const std::string& config_file);
    bool saveConfigurationToFile(const RegressionDetectionConfig& config,
                                 const std::string& config_file);
}

// Constants for performance regression detection (Constitutional v5.5 compliance)
namespace performance_regression_constants {
    constexpr double ZERO_REGRESSION_TOLERANCE = 0.0;        // Zero tolerance for regression
    constexpr double DEFAULT_REGRESSION_THRESHOLD = 5.0;      // 5% regression threshold
    constexpr double CRITICAL_REGRESSION_THRESHOLD = 15.0;     // 15% critical regression
    constexpr double HIGH_REGRESSION_THRESHOLD = 10.0;         // 10% high regression
    constexpr size_t MINIMUM_SAMPLE_COUNT = 20;                // Minimum 20 samples
    constexpr double STATISTICAL_SIGNIFICANCE_LEVEL = 0.05;    // 95% confidence level
    constexpr double Z_SCORE_THRESHOLD = 2.0;                   // 2 sigma threshold
    constexpr size_t MAX_BASELINE_AGE_DAYS = 30;               // 30 days max baseline age
    constexpr double PERFORMANCE_IMPROVEMENT_THRESHOLD = -2.0; // Performance improvement threshold
}

} // namespace validation
} // namespace puzzle71
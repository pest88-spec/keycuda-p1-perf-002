/**
 * @file performance_regression.cuh
 * @brief Performance regression detection system for Puzzle71 Technical Debt Repair
 *
 * This file implements a comprehensive performance regression detection system that
 * continuously monitors GPU performance metrics and identifies performance degradations.
 * The system focuses on:
 *
 * - Automated performance baseline management and tracking
 * - Statistical regression detection with configurable thresholds
 * - Multi-dimensional performance analysis and correlation
 * - Real-time performance monitoring and alerting
 * - Historical trend analysis and performance prediction
 * - Integration with Nsight Compute profiling and telemetry systems
 * - Automated regression report generation and distribution
 * - Performance impact assessment and root cause analysis
 * - CI/CD pipeline integration for continuous regression testing
 *
 * @author Puzzle71 Technical Debt Repair System
 * @version 5.5
 * @date 2025-10-21
 * @copyright Constitutional Compliance v5.5
 */

#pragma once

#include <cuda_runtime.h>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <chrono>
#include <atomic>
#include <functional>
#include <fstream>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include "performance_telemetry.cuh"
#include "nsight_profiler.cuh"
#include "adaptive_gpu_utilization.cuh"

namespace keyhunt {
namespace performance {
namespace regression {

// ============================================================================
// REGRESSION DETECTION CONFIGURATION CONSTANTS
// ============================================================================

/**
 * Regression detection configuration
 */
constexpr uint32_t DEFAULT_BASELINE_SAMPLES = 10;
constexpr uint32_t MIN_BASELINE_SAMPLES = 5;
constexpr uint32_t MAX_BASELINE_SAMPLES = 100;
constexpr double DEFAULT_REGRESSION_THRESHOLD = 5.0;    // 5% regression threshold
constexpr double STRICT_REGRESSION_THRESHOLD = 2.0;     // 2% for strict checking
constexpr double DEFAULT_CONFIDENCE_LEVEL = 0.95;        // 95% confidence interval
constexpr uint32_t REGRESSION_WINDOW_SIZE = 20;          // 20 samples for trend analysis
constexpr std::chrono::hours DEFAULT_BASELINE_EXPIRY(24); // 24 hours baseline expiry
constexpr std::chrono::minutes DEFAULT_MONITORING_INTERVAL(5); // 5 minutes monitoring

/**
 * Performance metric categories for regression detection
 */
enum class RegressionMetricCategory : uint32_t {
    THROUGHPUT = 0,           // Operations per second
    LATENCY = 1,             // Execution time
    UTILIZATION = 2,         // GPU utilization percentage
    MEMORY_EFFICIENCY = 3,    // Memory bandwidth utilization
    CACHE_PERFORMANCE = 4,    // Cache hit rates
    POWER_EFFICIENCY = 5,    // Performance per watt
    OCCUPANCY = 6,           // Thread block occupancy
    SYNCHRONIZATION = 7,     // Synchronization overhead
    CUSTOM = 8               // Custom user-defined metrics
};

/**
 * Regression severity levels
 */
enum class RegressionSeverity : uint32_t {
    INFO = 0,        // Informational change
    MINOR = 1,       // Minor regression (<5%)
    MODERATE = 2,     // Moderate regression (5-15%)
    MAJOR = 3,       // Major regression (15-30%)
    CRITICAL = 4     // Critical regression (>30%)
};

/**
 * Regression detection result
 */
struct alignas(64) RegressionResult {
    std::string metric_name;
    std::string kernel_name;
    std::string device_name;
    RegressionMetricCategory category;
    RegressionSeverity severity;

    // Current and baseline values
    double current_value;
    double baseline_value;
    double regression_percentage;

    // Statistical analysis
    double confidence_level;
    double statistical_significance;
    double trend_slope;
    double variance_ratio;

    // Context information
    std::chrono::system_clock::time_point detection_time;
    uint32_t sample_count;
    uint32_t window_size;

    // Root cause analysis
    std::vector<std::string> potential_causes;
    std::vector<std::string> recommended_actions;
    std::string root_cause_hypothesis;

    // Impact assessment
    double performance_impact_score;
    double user_experience_impact;
    double system_resource_impact;

    RegressionResult() : category(RegressionMetricCategory::THROUGHPUT), severity(RegressionSeverity::INFO),
                        current_value(0.0), baseline_value(0.0), regression_percentage(0.0),
                        confidence_level(0.0), statistical_significance(0.0),
                        trend_slope(0.0), variance_ratio(0.0),
                        sample_count(0), window_size(0),
                        performance_impact_score(0.0), user_experience_impact(0.0),
                        system_resource_impact(0.0) {}
};

/**
 * Performance baseline entry
 */
struct PerformanceBaseline {
    std::string metric_name;
    std::string kernel_name;
    std::string device_name;
    RegressionMetricCategory category;

    // Baseline statistics
    double mean_value;
    double std_deviation;
    double min_value;
    double max_value;
    double median_value;
    double percentile_95;

    // Sample information
    uint32_t sample_count;
    std::chrono::system_clock::time_point creation_time;
    std::chrono::system_clock::time_point last_update;
    std::chrono::hours validity_period;

    // Quality metrics
    double confidence_interval_width;
    double coefficient_of_variation;
    bool is_stable;
    bool is_outlier_filtered;

    // Configuration
    double regression_threshold;
    double confidence_level;

    PerformanceBaseline() : mean_value(0.0), std_deviation(0.0), min_value(0.0), max_value(0.0),
                        median_value(0.0), percentile_95(0.0), sample_count(0),
                        validity_period(DEFAULT_BASELINE_EXPIRY),
                        confidence_interval_width(0.0), coefficient_of_variation(0.0),
                        is_stable(false), is_outlier_filtered(false),
                        regression_threshold(DEFAULT_REGRESSION_THRESHOLD),
                        confidence_level(DEFAULT_CONFIDENCE_LEVEL) {}
};

/**
 * Regression detection configuration
 */
struct RegressionDetectionConfig {
    // Thresholds and sensitivity
    double default_regression_threshold;
    double strict_regression_threshold;
    double confidence_level;
    double minimum_sample_size;
    uint32_t window_size;
    std::chrono::hours baseline_expiry;

    // Metric categories to monitor
    std::vector<RegressionMetricCategory> monitored_categories;
    std::vector<std::string> custom_metrics;

    // Statistical analysis settings
    bool enable_statistical_testing;
    bool enable_trend_analysis;
    bool enable_outlier_detection;
    bool enable_multivariate_analysis;

    // Alerting and reporting
    bool enable_real_time_alerts;
    bool enable_periodic_reports;
    std::string report_format; // "json", "csv", "html"
    std::string output_directory;

    // Integration settings
    bool integrate_with_profiler;
    bool integrate_with_telemetry;
    bool integrate_with_adaptive_system;

    RegressionDetectionConfig() : default_regression_threshold(DEFAULT_REGRESSION_THRESHOLD),
                              strict_regression_threshold(STRICT_REGRESSION_THRESHOLD),
                              confidence_level(DEFAULT_CONFIDENCE_LEVEL),
                              minimum_sample_size(MIN_BASELINE_SAMPLES),
                              window_size(REGRESSION_WINDOW_SIZE),
                              baseline_expiry(DEFAULT_BASELINE_EXPIRY),
                              enable_statistical_testing(true),
                              enable_trend_analysis(true),
                              enable_outlier_detection(true),
                              enable_multivariate_analysis(false),
                              enable_real_time_alerts(true),
                              enable_periodic_reports(false),
                              report_format("json"),
                              integrate_with_profiler(true),
                              integrate_with_telemetry(true),
                              integrate_with_adaptive_system(true) {}
};

/**
 * Regression alert structure
 */
struct RegressionAlert {
    std::string alert_id;
    std::chrono::system_clock::time_point timestamp;
    RegressionResult regression_result;
    std::string alert_message;
    std::vector<std::string> notification_channels;
    bool is_resolved;
    std::chrono::system_clock::time_point resolution_time;

    RegressionAlert() : alert_id(""), is_resolved(false) {}
};

/**
 * Performance metric definition
 */
struct PerformanceMetric {
    std::string name;
    RegressionMetricCategory category;
    std::function<double(const PerformanceMetrics&)> extractor;
    std::string unit;
    bool higher_is_better; // Whether higher values indicate better performance
    double weight;        // Weight in overall performance score

    PerformanceMetric(const std::string& metric_name, RegressionMetricCategory cat,
                      std::function<double(const PerformanceMetrics&)> extract,
                      const std::string& metric_unit, bool higher_better, double metric_weight)
        : name(metric_name), category(cat), extractor(extract), unit(metric_unit),
          higher_is_better(higher_better), weight(metric_weight) {}
};

// ============================================================================
// PERFORMANCE BASELINE MANAGER
// ============================================================================

/**
 * Performance baseline management system
 */
class PerformanceBaselineManager {
private:
    std::map<std::string, PerformanceBaseline> baselines_;
    std::mutex baselines_mutex_;

    // Configuration
    RegressionDetectionConfig config_;

    // Statistics
    std::atomic<uint64_t> total_baselines_created_;
    std::atomic<uint64_t> total_baselines_updated_;
    std::atomic<uint64_t> expired_baselines_removed_;

public:
    /**
     * Constructor for baseline manager
     */
    explicit PerformanceBaselineManager(const RegressionDetectionConfig& config);

    /**
     * Create a new performance baseline
     */
    bool createBaseline(const std::string& metric_name,
                       const std::string& kernel_name,
                       const std::string& device_name,
                       RegressionMetricCategory category,
                       const std::vector<double>& samples);

    /**
     * Update existing baseline with new data
     */
    bool updateBaseline(const std::string& baseline_key,
                       const std::vector<double>& new_samples);

    /**
     * Get baseline for comparison
     */
    PerformanceBaseline getBaseline(const std::string& baseline_key) const;

    /**
     * Check if baseline exists and is valid
     */
    bool hasValidBaseline(const std::string& baseline_key) const;

    /**
     * Remove expired baselines
     */
    uint32_t removeExpiredBaselines();

    /**
     * Get all baselines for a specific kernel
     */
    std::vector<PerformanceBaseline> getKernelBaselines(const std::string& kernel_name);

    /**
     * Export baselines to file
     */
    bool exportBaselines(const std::string& filename, const std::string& format = "json");

    /**
     * Import baselines from file
     */
    bool importBaselines(const std::string& filename, const std::string& format = "json");

    /**
     * Get baseline statistics
     */
    void getBaselineStatistics(uint64_t& total_created, uint64_t& total_updated,
                                uint64_t& expired_removed) const;

private:
    /**
     * Calculate baseline statistics from samples
     */
    PerformanceBaseline calculateBaselineStats(const std::vector<double>& samples,
                                               const std::string& metric_name,
                                               const std::string& kernel_name,
                                               const std::string& device_name,
                                               RegressionMetricCategory category);

    /**
     * Validate baseline quality and stability
     */
    bool validateBaseline(const PerformanceBaseline& baseline);

    /**
     * Generate baseline key for storage
     */
    std::string generateBaselineKey(const std::string& metric_name,
                                   const std::string& kernel_name,
                                   const std::string& device_name);

    /**
     * Clean and filter outlier samples
     */
    std::vector<double> filterOutliers(const std::vector<double>& samples);
};

// ============================================================================
// STATISTICAL ANALYSIS ENGINE
// ============================================================================

/**
 * Statistical analysis engine for regression detection
 */
class StatisticalAnalysisEngine {
private:
    std::mt19937_64 random_generator_;
    std::normal_distribution<double> normal_distribution_;

    // Configuration
    double confidence_level_;
    bool enable_mann_kendall_test_;
    bool enable_welch_t_test_;
    bool enable_levene_test_;

public:
    /**
     * Constructor for statistical analysis engine
     */
    explicit StatisticalAnalysisEngine(double confidence_level = DEFAULT_CONFIDENCE_LEVEL);

    /**
     * Perform two-sample t-test for regression detection
     */
    double performTTest(const std::vector<double>& baseline_samples,
                         const std::vector<double>& current_samples);

    /**
     * Perform Mann-Whitney U test (non-parametric)
     */
    double performMannWhitneyTest(const std::vector<double>& baseline_samples,
                                   const std::vector<double>& current_samples);

    /**
     * Perform Kolmogorov-Smirnov test
     */
    double performKSTest(const std::vector<double>& baseline_samples,
                       const std::vector<double>& current_samples);

    /**
     * Calculate confidence interval
     */
    std::pair<double, double> calculateConfidenceInterval(const std::vector<double>& samples,
                                                        double confidence_level);

    /**
     * Detect outliers using statistical methods
     */
    std::vector<size_t> detectOutliers(const std::vector<double>& samples);

    /**
     * Calculate effect size (Cohen's d)
     */
    double calculateEffectSize(const std::vector<double>& baseline_samples,
                               const std::vector<double>& current_samples);

    /**
     * Perform trend analysis
     */
    double calculateTrend(const std::vector<double>& time_series_samples);

    /**
     * Calculate correlation coefficient
     */
    double calculateCorrelation(const std::vector<double>& series1,
                                const std::vector<double>& series2);

private:
    /**
     * Calculate sample mean
     */
    double calculateMean(const std::vector<double>& samples);

    /**
     * Calculate sample standard deviation
     */
    double calculateStdDev(const std::vector<double>& samples, double mean);

    /**
     * Calculate sample median
     */
    double calculateMedian(std::vector<double> samples);

    /**
     * Perform permutation test for p-value calculation
     */
    double performPermutationTest(const std::vector<double>& baseline_samples,
                                   const std::vector<double>& current_samples,
                                   uint32_t permutations);
};

// ============================================================================
// REGRESSION DETECTION ENGINE
// ============================================================================

/**
 * Main regression detection engine
 */
class RegressionDetectionEngine {
private:
    std::unique_ptr<PerformanceBaselineManager> baseline_manager_;
    std::unique_ptr<StatisticalAnalysisEngine> statistical_engine_;

    // Configuration
    RegressionDetectionConfig config_;

    // Metric definitions
    std::vector<PerformanceMetric> defined_metrics_;

    // Detection state
    std::atomic<bool> detection_active_;
    std::mutex detection_mutex_;
    std::condition_variable detection_cv_;

    // Alert management
    std::vector<RegressionAlert> active_alerts_;
    std::mutex alerts_mutex_;

    // Performance monitoring integration
    std::shared_ptr<PerformanceTelemetryCollector> telemetry_collector_;
    std::shared_ptr<NsightProfilerManager> profiler_manager_;
    std::shared_ptr<gpu::adaptive::AdaptiveGPUUtilizationCoordinator> adaptive_coordinator_;

public:
    /**
     * Constructor for regression detection engine
     */
    explicit RegressionDetectionEngine(
        const RegressionDetectionConfig& config = RegressionDetectionConfig(),
        std::shared_ptr<PerformanceTelemetryCollector> telemetry = nullptr,
        std::shared_ptr<NsightProfilerManager> profiler = nullptr,
        std::shared_ptr<gpu::adaptive::AdaptiveGPUUtilizationCoordinator> adaptive = nullptr
    );

    /**
     * Initialize the regression detection system
     */
    bool initialize();

    /**
     * Start regression detection monitoring
     */
    bool startDetection();

    /**
     * Stop regression detection monitoring
     */
    void stopDetection();

    /**
     * Add custom metric definition
     */
    void addMetric(const PerformanceMetric& metric);

    /**
     * Perform regression analysis for current metrics
     */
    std::vector<RegressionResult> analyzeRegression(
        const std::vector<PerformanceMetrics>& current_metrics);

    /**
     * Perform regression analysis for specific kernel
     */
    std::vector<RegressionResult> analyzeKernelRegression(
        const std::string& kernel_name,
        const PerformanceMetrics& current_metrics);

    /**
     * Check for performance regression against baselines
     */
    std::vector<RegressionResult> checkRegression(
        const std::map<std::string, double>& current_values);

    /**
     * Update performance baselines with new data
     */
    bool updateBaselines(const std::vector<PerformanceMetrics>& new_metrics);

    /**
     * Get active regression alerts
     */
    std::vector<RegressionAlert> getActiveAlerts();

    /**
     * Resolve regression alert
     */
    bool resolveAlert(const std::string& alert_id);

    /**
     * Generate regression report
     */
    std::string generateRegressionReport(const std::chrono::system_clock::time_point& start_time,
                                         const std::chrono::system_clock::time_point& end_time);

    /**
     * Export regression data to file
     */
    bool exportRegressionData(const std::string& filename, const std::string& format = "json");

    /**
     * Get detection statistics
     */
    void getDetectionStatistics(uint64_t& total_detections, uint64_t& active_alerts,
                                double& average_regression_percentage) const;

private:
    /**
     * Initialize default metrics
     */
    void initializeDefaultMetrics();

    /**
     * Detection thread function
     */
    void detectionThread();

    /**
     * Perform single metric regression analysis
     */
    RegressionResult analyzeMetricRegression(const PerformanceMetric& metric,
                                             const PerformanceMetrics& current_metrics);

    /**
     * Assess regression severity
     */
    RegressionSeverity assessRegressionSeverity(double regression_percentage,
                                              RegressionMetricCategory category);

    /**
     * Generate alert message
     */
    std::string generateAlertMessage(const RegressionResult& result);

    /**
     * Analyze root causes for regression
     */
    void analyzeRootCauses(RegressionResult& result);

    /**
     * Generate recommended actions
     */
    void generateRecommendedActions(RegressionResult& result);

    /**
     * Check if alert should be generated
     */
    bool shouldGenerateAlert(const RegressionResult& result);

    /**
     * Send alert notification
     */
    void sendAlert(const RegressionAlert& alert);
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Convert regression category to string
 */
inline const char* regressionCategoryToString(RegressionMetricCategory category) {
    switch (category) {
        case RegressionMetricCategory::THROUGHPUT: return "Throughput";
        case RegressionMetricCategory::LATENCY: return "Latency";
        case RegressionMetricCategory::UTILIZATION: return "Utilization";
        case RegressionMetricCategory::MEMORY_EFFICIENCY: return "Memory Efficiency";
        case RegressionMetricCategory::CACHE_PERFORMANCE: return "Cache Performance";
        case RegressionMetricCategory::POWER_EFFICIENCY: return "Power Efficiency";
        case RegressionMetricCategory::OCCUPANCY: return "Occupancy";
        case RegressionMetricCategory::SYNCHRONIZATION: return "Synchronization";
        case RegressionMetricCategory::CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

/**
 * Convert regression severity to string
 */
inline const char* regressionSeverityToString(RegressionSeverity severity) {
    switch (severity) {
        case RegressionSeverity::INFO: return "Info";
        case RegressionSeverity::MINOR: return "Minor";
        case RegressionSeverity::MODERATE: return "Moderate";
        case RegressionSeverity::MAJOR: return "Major";
        case RegressionSeverity::CRITICAL: return "Critical";
        default: return "Unknown";
    }
}

/**
 * Create default regression detection configuration
 */
RegressionDetectionConfig createDefaultRegressionConfig();

/**
 * Validate regression result integrity
 */
bool validateRegressionResult(const RegressionResult& result);

/**
 * Check if regression exceeds threshold
 */
bool exceedsRegressionThreshold(const RegressionResult& result, double threshold);

/**
 * Format regression result for output
 */
std::string formatRegressionResult(const RegressionResult& result, const std::string& format);

} // namespace regression
} // namespace performance
} // namespace keyhunt
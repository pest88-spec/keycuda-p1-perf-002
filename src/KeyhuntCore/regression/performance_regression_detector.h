// Puzzle71Solver - Automated Performance Regression Detection (T048)
// Phase 6: User Story 4 - Performance Monitoring
// Statistical analysis system for performance regression detection with baseline management

#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>

#include "monitoring/real_time_monitor.h"
#include "optimization/automated_optimizer.h"

namespace puzzle71::regression {

/**
 * @brief Regression detection severity levels
 */
enum class RegressionSeverity {
    INFO,        // Minor performance change (5-10%)
    WARNING,     // Moderate regression (10-20%)
    ERROR,       // Significant regression (20-50%)
    CRITICAL     // Severe regression (>50%)
};

/**
 * @brief Regression detection status
 */
enum class RegressionStatus {
    NORMAL,      // No regression detected
    MONITORING,  // Potential regression, monitoring closely
    DETECTED,    // Regression confirmed
    RESOLVED,    // Regression has been resolved
    IGNORED      // Regression acknowledged but ignored
};

/**
 * @brief Performance baseline data
 */
struct PerformanceBaseline {
    std::string id;
    std::string name;
    std::string description;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;

    // Baseline metrics
    double throughput_mean{0.0};
    double throughput_stddev{0.0};
    double throughput_median{0.0};
    double throughput_p95{0.0};
    double throughput_p99{0.0};

    double latency_mean{0.0};
    double latency_stddev{0.0};
    double latency_p95{0.0};
    double latency_p99{0.0};

    double gpu_utilization_mean{0.0};
    double memory_utilization_mean{0.0};
    double power_usage_mean{0.0};
    double temperature_mean{0.0};

    // Sample information
    size_t sample_count{0};
    std::chrono::seconds collection_duration{0};
    std::string device_info;
    std::string configuration_hash;

    // Validation settings
    double acceptable_variance{0.05};      // 5% variance acceptable
    double warning_threshold{0.10};       // 10% degradation warning
    double error_threshold{0.20};         // 20% degradation error
    double critical_threshold{0.50};       // 50% degradation critical

    /**
     * @brief Validate baseline quality
     */
    bool isValid() const;

    /**
     * @brief Calculate baseline age
     */
    std::chrono::seconds getAge() const;

    /**
     * @brief Check if baseline is expired
     */
    bool isExpired() const;

    /**
     * @brief Serialize baseline to JSON
     */
    std::string toJson() const;

    /**
     * @brief Deserialize baseline from JSON
     */
    static PerformanceBaseline fromJson(const std::string& json);
};

/**
 * @brief Regression detection result
 */
struct RegressionResult {
    std::string id;
    std::chrono::system_clock::time_point detected_at;
    RegressionSeverity severity{RegressionSeverity::INFO};
    RegressionStatus status{RegressionStatus::NORMAL};

    // Comparison metrics
    std::string baseline_id;
    std::string metric_name;
    double baseline_value{0.0};
    double current_value{0.0};
    double percentage_change{0.0};
    double z_score{0.0};
    double p_value{1.0};

    // Statistical analysis
    bool is_statistically_significant{false};
    double confidence_level{0.95};
    std::string test_method{"z_test"};

    // Context information
    std::vector<std::string> affected_metrics;
    std::map<std::string, double> secondary_changes;
    std::vector<std::string> potential_causes;
    std::string environmental_notes;

    // Resolution tracking
    std::chrono::system_clock::time_point resolved_at;
    std::string resolution_method;
    std::string resolution_notes;

    /**
     * @brief Check if regression is active
     */
    bool isActive() const;

    /**
     * @brief Get severity as string
     */
    std::string getSeverityString() const;

    /**
     * @brief Get status as string
     */
    std::string getStatusString() const;

    /**
     * @brief Serialize result to JSON
     */
    std::string toJson() const;

    /**
     * @brief Deserialize result from JSON
     */
    static RegressionResult fromJson(const std::string& json);
};

/**
 * @brief Regression detection configuration
 */
struct RegressionDetectionConfig {
    // Detection thresholds
    double min_sample_size{30};                    // Minimum samples for detection
    double significance_level{0.05};               // Statistical significance (p-value)
    double minimum_change_threshold{0.05};          // Minimum change to consider (5%)
    std::chrono::seconds baseline_max_age{86400};  // Maximum baseline age (24 hours)

    // Detection methods
    bool enable_statistical_tests{true};
    bool enable_trend_analysis{true};
    bool enable_anomaly_detection{true};
    bool enable_multivariate_analysis{false};      // Complex: multiple metrics

    // Alerting settings
    bool enable_alerts{true};
    RegressionSeverity minimum_alert_severity{RegressionSeverity::WARNING};
    std::chrono::seconds alert_cooldown{300};      // 5 minutes between alerts
    std::vector<std::string> alert_recipients;

    // Baseline management
    bool auto_update_baselines{false};
    std::chrono::seconds baseline_update_interval{3600};  // 1 hour
    size_t max_baselines_per_metric{10};

    // Noise reduction
    bool enable_smoothing{true};
    std::chrono::seconds smoothing_window{60};      // 1 minute smoothing
    double outlier_threshold{3.0};                 // 3 sigma outlier detection

    // Performance optimization
    size_t max_concurrent_tests{10};
    std::chrono::seconds test_timeout{30};
    bool enable_caching{true};
    std::chrono::seconds cache_ttl{300};           // 5 minutes cache TTL
};

/**
 * @brief Statistical test methods
 */
enum class StatisticalTest {
    Z_TEST,          // Z-test for large samples
    T_TEST,          // Student's t-test for small samples
    WILCOXON,        // Wilcoxon signed-rank test (non-parametric)
    MANN_WHITNEY,    // Mann-Whitney U test
    CHI_SQUARE,      // Chi-square test
    ANOVA            // Analysis of variance
};

/**
 * @brief Time series analysis result
 */
struct TimeSeriesAnalysis {
    double trend_slope{0.0};
    double trend_intercept{0.0};
    double correlation_coefficient{0.0};
    double mean_absolute_error{0.0};
    double root_mean_square_error{0.0};

    bool has_trend{false};
    bool is_seasonal{false};
    std::vector<double> seasonality_pattern;

    std::vector<double> residuals;
    std::vector<double> forecast_values;
    double forecast_accuracy{0.0};
};

/**
 * @brief Performance regression detector
 *
 * Comprehensive system for detecting performance regressions using statistical
 * analysis, baseline management, and automated alerting.
 */
class PerformanceRegressionDetector {
public:
    explicit PerformanceRegressionDetector(const RegressionDetectionConfig& config = {});
    ~PerformanceRegressionDetector();

    // Baseline management
    std::string createBaseline(
        const std::string& name,
        const std::vector<puzzle71::monitoring::PerformanceMetrics>& metrics,
        const std::string& description = ""
    );

    bool updateBaseline(
        const std::string& baseline_id,
        const std::vector<puzzle71::monitoring::PerformanceMetrics>& metrics
    );

    bool removeBaseline(const std::string& baseline_id);
    std::vector<PerformanceBaseline> getBaselines() const;
    PerformanceBaseline getBaseline(const std::string& baseline_id) const;
    bool setPrimaryBaseline(const std::string& metric_name, const std::string& baseline_id);

    // Regression detection
    std::vector<RegressionResult> detectRegressions(
        const puzzle71::monitoring::PerformanceMetrics& current_metrics
    );

    std::vector<RegressionResult> detectRegressions(
        const std::vector<puzzle71::monitoring::PerformanceMetrics>& metrics_series
    );

    RegressionResult detectRegression(
        const std::string& metric_name,
        double current_value,
        const std::string& baseline_id = ""
    );

    // Continuous monitoring
    void startMonitoring(std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor);
    void stopMonitoring();
    bool isMonitoring() const { return monitoring_active_.load(); }

    // Alert management
    void setAlertCallback(std::function<void(const RegressionResult&)> callback);
    std::vector<RegressionResult> getActiveRegressions() const;
    void acknowledgeRegression(const std::string& regression_id);
    void resolveRegression(const std::string& regression_id, const std::string& resolution_method);

    // Configuration
    void updateConfiguration(const RegressionDetectionConfig& config);
    RegressionDetectionConfig getConfiguration() const { return config_; }

    // Analysis and reporting
    TimeSeriesAnalysis analyzeTimeSeries(
        const std::vector<std::pair<std::chrono::system_clock::time_point, double>>& data
    );

    std::string generateRegressionReport() const;
    bool exportRegressions(const std::string& filename) const;
    bool importBaselines(const std::string& filename);

    // Statistics and health
    struct DetectionStatistics {
        size_t total_detections{0};
        size_t active_regressions{0};
        size_t resolved_regressions{0};
        std::map<RegressionSeverity, size_t> severity_counts;
        std::map<std::string, size_t> metric_counts;
        std::chrono::system_clock::time_point last_detection;
        double false_positive_rate{0.0};
    };

    DetectionStatistics getStatistics() const;

    // Health check
    bool isHealthy() const;
    std::vector<std::string> getHealthIssues() const;

private:
    RegressionDetectionConfig config_;
    mutable std::mutex config_mutex_;

    // Baseline storage
    std::map<std::string, PerformanceBaseline> baselines_;
    std::map<std::string, std::string> primary_baselines_;  // metric_name -> baseline_id
    mutable std::mutex baselines_mutex_;

    // Regression storage
    std::map<std::string, RegressionResult> regressions_;
    std::vector<RegressionResult> regression_history_;
    mutable std::mutex regressions_mutex_;

    // Monitoring
    std::atomic<bool> monitoring_active_{false};
    std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor_;
    std::unique_ptr<std::thread> monitoring_thread_;

    // Alert system
    std::function<void(const RegressionResult&)> alert_callback_;
    std::map<std::string, std::chrono::system_clock::time_point> last_alert_times_;
    mutable std::mutex alerts_mutex_;

    // Data collection
    std::vector<puzzle71::monitoring::PerformanceMetrics> collected_metrics_;
    mutable std::mutex metrics_mutex_;

    // Caching
    std::map<std::string, std::pair<RegressionResult, std::chrono::steady_clock::time_point>> result_cache_;
    mutable std::mutex cache_mutex_;

    // Private methods
    void monitoringLoop();
    void processMetrics();

    // Statistical methods
    double calculateZScore(double sample_mean, double population_mean, double population_stddev, size_t sample_size);
    double calculateTStatistic(double sample_mean, double population_mean, double sample_stddev, size_t sample_size);
    double calculatePValue(double test_statistic, size_t degrees_of_freedom, StatisticalTest test_type);
    bool performStatisticalTest(double current_value, const PerformanceBaseline& baseline, RegressionResult& result);

    // Time series analysis
    TimeSeriesAnalysis performTimeSeriesAnalysis(const std::vector<double>& values);
    double calculateTrend(const std::vector<double>& values);
    std::vector<double> smoothData(const std::vector<double>& values, std::chrono::seconds window);

    // Baseline management
    void cleanupExpiredBaselines();
    void autoUpdateBaselines();
    PerformanceBaseline calculateBaseline(const std::vector<puzzle71::monitoring::PerformanceMetrics>& metrics);

    // Regression management
    void storeRegression(const RegressionResult& result);
    void updateRegressionStatus(const std::string& regression_id, RegressionStatus status);
    bool shouldAlert(const RegressionResult& result) const;

    // Alert system
    void triggerAlert(const RegressionResult& result);
    std::string formatAlertMessage(const RegressionResult& result) const;

    // Data validation
    bool validateMetrics(const std::vector<puzzle71::monitoring::PerformanceMetrics>& metrics) const;
    bool validateBaseline(const PerformanceBaseline& baseline) const;
    std::vector<double> removeOutliers(const std::vector<double>& values) const;

    // Utility methods
    std::string generateRegressionId() const;
    std::chrono::system_clock::time_point getCurrentTime() const;
    std::string formatTimestamp(const std::chrono::system_clock::time_point& timestamp) const;
};

/**
 * @brief Factory for creating regression detectors
 */
class RegressionDetectorFactory {
public:
    /**
     * @brief Create detector with default configuration
     */
    static std::unique_ptr<PerformanceRegressionDetector> create();

    /**
     * @brief Create detector with custom configuration
     */
    static std::unique_ptr<PerformanceRegressionDetector> create(const RegressionDetectionConfig& config);

    /**
     * @brief Create detector for high-sensitivity monitoring
     */
    static std::unique_ptr<PerformanceRegressionDetector> createHighSensitivityDetector();

    /**
     * @brief Create detector for production monitoring
     */
    static std::unique_ptr<PerformanceRegressionDetector> createProductionDetector();

    /**
     * @brief Create detector for development monitoring
     */
    static std::unique_ptr<PerformanceRegressionDetector> createDevelopmentDetector();

    /**
     * @brief Create configuration templates
     */
    static RegressionDetectionConfig createDefaultConfig();
    static RegressionDetectionConfig createHighSensitivityConfig();
    static RegressionDetectionConfig createProductionConfig();
    static RegressionDetectionConfig createDevelopmentConfig();
};

/**
 * @brief Regression alert manager
 */
class RegressionAlertManager {
public:
    explicit RegressionAlertManager(const RegressionDetectionConfig& config = {});

    // Alert channels
    void addEmailRecipient(const std::string& email);
    void addSlackWebhook(const std::string& webhook_url);
    void setDiscordWebhook(const std::string& webhook_url);
    void enableConsoleLogging(bool enabled);

    // Alert sending
    bool sendAlert(const RegressionResult& regression);
    bool sendRegressionSummary(const std::vector<RegressionResult>& regressions);
    bool sendResolutionNotification(const RegressionResult& regression);

    // Alert templates
    void setEmailTemplate(const std::string& template_content);
    void setSlackTemplate(const std::string& template_content);
    void setDiscordTemplate(const std::string& template_content);

    // Rate limiting
    void setRateLimit(std::chrono::seconds min_interval);
    bool canSendAlert(const std::string& regression_id) const;

private:
    RegressionDetectionConfig config_;
    std::vector<std::string> email_recipients_;
    std::vector<std::string> slack_webhooks_;
    std::string discord_webhook_;
    bool console_logging_enabled_{true};

    std::string email_template_;
    std::string slack_template_;
    std::string discord_template_;

    std::map<std::string, std::chrono::system_clock::time_point> last_sent_times_;
    std::chrono::seconds rate_limit_{std::chrono::seconds(300)};  // 5 minutes default

    // Alert sending methods
    bool sendEmailAlert(const RegressionResult& regression);
    bool sendSlackAlert(const RegressionResult& regression);
    bool sendDiscordAlert(const RegressionResult& regression);
    void logConsoleAlert(const RegressionResult& regression);

    // Template processing
    std::string processTemplate(const std::string& template_content, const RegressionResult& regression) const;
};

/**
 * @brief Regression detection utilities
 */
namespace regression_utils {

/**
 * @brief Performance metrics comparison
 */
struct MetricsComparison {
    double throughput_change{0.0};
    double latency_change{0.0};
    double utilization_change{0.0};
    double memory_change{0.0};
    double power_change{0.0};
    double temperature_change{0.0};

    bool has_regression{false};
    RegressionSeverity max_severity{RegressionSeverity::INFO};
    std::vector<std::string> regressed_metrics;
};

MetricsComparison compareMetrics(
    const puzzle71::monitoring::PerformanceMetrics& current,
    const PerformanceBaseline& baseline
);

/**
 * @brief Performance trend analysis
 */
enum class TrendDirection {
    IMPROVING,
    STABLE,
    DEGRADING,
    UNKNOWN
};

struct TrendAnalysis {
    TrendDirection direction{TrendDirection::UNKNOWN};
    double slope{0.0};
    double confidence{0.0};
    std::chrono::seconds period{0};
    std::vector<double> data_points;
};

TrendAnalysis analyzeTrend(
    const std::vector<std::pair<std::chrono::system_clock::time_point, double>>& data,
    std::chrono::seconds analysis_window
);

/**
 * @brief Baseline validation
 */
struct BaselineValidation {
    bool is_valid{false};
    size_t sample_count{0};
    double coefficient_of_variation{0.0};
    double statistical_power{0.0};
    std::vector<std::string> issues;
    std::vector<std::string> recommendations;
};

BaselineValidation validateBaseline(
    const PerformanceBaseline& baseline,
    const RegressionDetectionConfig& config
);

/**
 * @brief Performance impact assessment
 */
struct ImpactAssessment {
    double performance_impact{0.0};           // Overall performance impact percentage
    double user_experience_impact{0.0};       // Estimated impact on user experience
    double business_impact{0.0};               // Estimated business impact
    std::chrono::seconds time_to_resolution{0}; // Estimated time to resolve
    std::vector<std::string> affected_features;   // Features impacted by regression
    std::vector<std::string> recommended_actions; // Recommended actions
};

ImpactAssessment assessImpact(const RegressionResult& regression);

/**
 * @brief Regression pattern detection
 */
struct RegressionPattern {
    std::string pattern_type;
    double frequency{0.0};
    std::vector<std::chrono::system_clock::time_point> occurrences;
    std::map<std::string, double> average_impacts;
    std::string likely_cause;
};

std::vector<RegressionPattern> detectRegressionPatterns(
    const std::vector<RegressionResult>& regression_history
);

} // namespace regression_utils

} // namespace puzzle71::regression
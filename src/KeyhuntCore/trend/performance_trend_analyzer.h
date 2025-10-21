// Puzzle71Solver - Performance Trend Analysis (T050)
// Phase 6: User Story 4 - Performance Monitoring
// Advanced trend analysis system with predictive analytics and anomaly detection

#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

#include "monitoring/real_time_monitor.h"
#include "regression/performance_regression_detector.h"

namespace puzzle71::trend {

/**
 * @brief Trend analysis algorithms
 */
enum class TrendAlgorithm {
    LINEAR_REGRESSION,      // Simple linear regression
    POLYNOMIAL_REGRESSION,  // Polynomial curve fitting
    EXPONENTIAL_SMOOTHING,  // Exponential moving average
    ARIMA,                  // AutoRegressive Integrated Moving Average
    SEASONAL_DECOMPOSITION, // Seasonal trend decomposition
    LOESS,                  // Locally Estimated Scatterplot Smoothing
    NEURAL_NETWORK,         // Neural network prediction
    ENSEMBLE                // Ensemble of multiple methods
};

/**
 * @brief Trend direction types
 */
enum class TrendDirection {
    IMPROVING,      // Performance improving over time
    DEGRADING,      // Performance degrading over time
    STABLE,         // Performance stable
    VOLATILE,       // High volatility, unclear direction
    SEASONAL,       // Seasonal patterns present
    UNKNOWN         // Insufficient data or unclear pattern
};

/**
 * @brief Trend confidence levels
 */
enum class TrendConfidence {
    VERY_LOW,       // < 50% confidence
    LOW,            // 50-65% confidence
    MEDIUM,         // 65-80% confidence
    HIGH,           // 80-90% confidence
    VERY_HIGH       // > 90% confidence
};

/**
 * @brief Anomaly detection methods
 */
enum class AnomalyMethod {
    STATISTICAL_ZSCORE,     // Z-score based detection
    IQR,                    // Interquartile range method
    ISOLATION_FOREST,       // Isolation forest algorithm
    LOCAL_OUTLIER_FACTOR,   // LOF anomaly detection
    ONE_CLASS_SVM,          // One-class SVM
    AUTOENCODER,            // Autoencoder neural network
    MOVING_AVERAGE,         // Moving average deviation
    THRESHOLD_BASED         // Fixed threshold method
};

/**
 * @brief Performance trend data point
 */
struct TrendDataPoint {
    std::chrono::system_clock::time_point timestamp;
    double value{0.0};
    std::string metric_name;
    std::map<std::string, std::string> context;
    double weight{1.0};
    bool is_anomaly{false};
    double anomaly_score{0.0};

    /**
     * @brief Get timestamp as Unix epoch
     */
    double getTimestamp() const;

    /**
     * @brief Serialize to JSON
     */
    std::string toJson() const;

    /**
     * @brief Deserialize from JSON
     */
    static TrendDataPoint fromJson(const std::string& json);
};

/**
 * @brief Trend analysis result
 */
struct TrendResult {
    std::string metric_name;
    TrendDirection direction{TrendDirection::UNKNOWN};
    TrendConfidence confidence{TrendConfidence::VERY_LOW};
    TrendAlgorithm algorithm{TrendAlgorithm::LINEAR_REGRESSION};

    // Trend parameters
    double slope{0.0};                     // Rate of change
    double intercept{0.0};                 // Baseline value
    double correlation_coefficient{0.0};   // R-squared value
    double mean_squared_error{0.0};        // MSE

    // Statistical measures
    double mean_value{0.0};
    double std_deviation{0.0};
    double variance{0.0};
    std::vector<double> confidence_intervals;

    // Predictive values
    double next_value_prediction{0.0};
    double prediction_confidence{0.0};
    std::vector<double> future_predictions;
    std::vector<double> prediction_intervals;

    // Anomaly information
    size_t anomaly_count{0};
    double anomaly_percentage{0.0};
    std::vector<TrendDataPoint> anomalies;

    // Time range
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    size_t data_point_count{0};

    /**
     * @brief Get trend direction as string
     */
    std::string getDirectionString() const;

    /**
     * @brief Get confidence as string
     */
    std::string getConfidenceString() const;

    /**
     * @brief Check if trend is statistically significant
     */
    bool isStatisticallySignificant(double significance_level = 0.05) const;

    /**
     * @brief Serialize result to JSON
     */
    std::string toJson() const;

    /**
     * @brief Deserialize result from JSON
     */
    static TrendResult fromJson(const std::string& json);
};

/**
 * @brief Anomaly detection result
 */
struct AnomalyResult {
    std::string metric_name;
    AnomalyMethod method{AnomalyMethod::STATISTICAL_ZSCORE};
    std::vector<TrendDataPoint> anomalies;

    // Detection parameters
    double threshold{3.0};                 // Z-score threshold or similar
    double sensitivity{0.5};               // Detection sensitivity
    size_t min_anomaly_window{1};          // Minimum consecutive points

    // Statistics
    size_t total_points{0};
    size_t anomaly_points{0};
    double anomaly_rate{0.0};
    double average_anomaly_score{0.0};
    std::vector<double> anomaly_scores;

    // Clustering information
    size_t anomaly_clusters{0};
    std::vector<std::vector<size_t>> cluster_indices;
    std::vector<double> cluster_severity;

    /**
     * @brief Check if point is anomaly
     */
    bool isAnomaly(size_t point_index) const;

    /**
     * @brief Get anomaly severity
     */
    double getAnomalySeverity(size_t point_index) const;

    /**
     * @brief Serialize to JSON
     */
    std::string toJson() const;

    /**
     * @brief Deserialize from JSON
     */
    static AnomalyResult fromJson(const std::string& json);
};

/**
 * @brief Seasonal analysis result
 */
struct SeasonalResult {
    std::string metric_name;
    bool has_seasonality{false};
    std::chrono::seconds season_period{0};
    double seasonal_strength{0.0};

    // Seasonal components
    std::vector<double> seasonal_pattern;
    std::vector<double> trend_component;
    std::vector<double> residual_component;

    // Decomposition statistics
    double explained_variance{0.0};
    double seasonal_variance{0.0};
    double trend_variance{0.0};
    double residual_variance{0.0};

    // Peak/valley information
    std::vector<std::chrono::system_clock::time_point> peak_times;
    std::vector<std::chrono::system_clock::time_point> valley_times;
    std::vector<double> peak_values;
    std::vector<double> valley_values;

    /**
     * @brief Predict value at specific time
     */
    double predictValue(std::chrono::system_clock::time_point time) const;

    /**
     * @brief Get next peak time
     */
    std::chrono::system_clock::time_point getNextPeak(
        std::chrono::system_clock::time_point from_time
    ) const;

    /**
     * @brief Serialize to JSON
     */
    std::string toJson() const;

    /**
     * @brief Deserialize from JSON
     */
    static SeasonalResult fromJson(const std::string& json);
};

/**
 * @brief Trend analysis configuration
 */
struct TrendAnalysisConfig {
    // Data requirements
    size_t min_data_points{10};             // Minimum points for analysis
    size_t max_data_points{10000};          // Maximum points to process
    std::chrono::hours data_retention{720}; // 30 days default retention

    // Algorithm selection
    TrendAlgorithm primary_algorithm{TrendAlgorithm::ENSEMBLE};
    std::vector<TrendAlgorithm> ensemble_methods;
    double ensemble_weights{0.0};           // Auto-weight ensemble

    // Analysis parameters
    double significance_level{0.05};        // Statistical significance
    double confidence_level{0.95};          // Confidence intervals
    double outlier_threshold{3.0};          // Outlier detection threshold
    bool enable_seasonal_analysis{true};
    bool enable_anomaly_detection{true};

    // Prediction settings
    size_t prediction_horizon{10};          // Number of future points
    double prediction_confidence{0.8};      // Minimum prediction confidence
    bool enable_uncertainty_estimation{true};

    // Anomaly detection
    AnomalyMethod anomaly_method{AnomalyMethod::ISOLATION_FOREST};
    double anomaly_sensitivity{0.5};
    size_t min_anomaly_cluster{2};
    bool enable_realtime_detection{true};

    // Performance optimization
    size_t thread_count{4};
    bool enable_caching{true};
    std::chrono::seconds cache_ttl{300};    // 5 minutes cache

    // Output settings
    bool include_detailed_stats{true};
    bool include_confidence_intervals{true};
    bool include_predictions{true};
    std::string output_format{"json"};

    /**
     * @brief Validate configuration
     */
    bool isValid() const;

    /**
     * @brief Serialize to JSON
     */
    std::string toJson() const;

    /**
     * @brief Deserialize from JSON
     */
    static TrendAnalysisConfig fromJson(const std::string& json);
};

/**
 * @brief Trend analyzer interface
 */
class TrendAnalyzer {
public:
    virtual ~TrendAnalyzer() = default;

    /**
     * @brief Analyze trend in data
     */
    virtual TrendResult analyzeTrend(
        const std::vector<TrendDataPoint>& data,
        const TrendAnalysisConfig& config = {}
    ) = 0;

    /**
     * @brief Predict future values
     */
    virtual std::vector<double> predictFuture(
        const std::vector<TrendDataPoint>& data,
        size_t horizon,
        const TrendAnalysisConfig& config = {}
    ) = 0;

    /**
     * @brief Get algorithm name
     */
    virtual std::string getAlgorithmName() const = 0;

    /**
     * @brief Check if algorithm supports prediction
     */
    virtual bool supportsPrediction() const = 0;
};

/**
 * @brief Linear regression trend analyzer
 */
class LinearRegressionAnalyzer : public TrendAnalyzer {
public:
    TrendResult analyzeTrend(
        const std::vector<TrendDataPoint>& data,
        const TrendAnalysisConfig& config = {}
    ) override;

    std::vector<double> predictFuture(
        const std::vector<TrendDataPoint>& data,
        size_t horizon,
        const TrendAnalysisConfig& config = {}
    ) override;

    std::string getAlgorithmName() const override;
    bool supportsPrediction() const override;

private:
    std::pair<double, double> calculateLinearRegression(const std::vector<double>& x, const std::vector<double>& y);
    double calculateCorrelation(const std::vector<double>& x, const std::vector<double>& y);
    std::vector<double> calculateConfidenceIntervals(const std::vector<double>& x, const std::vector<double>& y, double confidence_level);
};

/**
 * @brief Exponential smoothing analyzer
 */
class ExponentialSmoothingAnalyzer : public TrendAnalyzer {
public:
    explicit ExponentialSmoothingAnalyzer(double alpha = 0.3, double beta = 0.1, double gamma = 0.1);

    TrendResult analyzeTrend(
        const std::vector<TrendDataPoint>& data,
        const TrendAnalysisConfig& config = {}
    ) override;

    std::vector<double> predictFuture(
        const std::vector<TrendDataPoint>& data,
        size_t horizon,
        const TrendAnalysisConfig& config = {}
    ) override;

    std::string getAlgorithmName() const override;
    bool supportsPrediction() const override;

    void setSmoothingParameters(double alpha, double beta = 0.0, double gamma = 0.0);

private:
    double alpha_{0.3};  // Level smoothing
    double beta_{0.1};   // Trend smoothing
    double gamma_{0.1};  // Seasonal smoothing

    struct HoltWintersState {
        double level{0.0};
        double trend{0.0};
        std::vector<double> seasonal;
        size_t season_length{0};
    };

    HoltWintersState initializeHoltWinters(const std::vector<double>& data, size_t season_length);
    std::vector<double> applyHoltWinters(const std::vector<double>& data, const HoltWintersState& initial);
};

/**
 * @brief ARIMA time series analyzer
 */
class ARIMAAnalyzer : public TrendAnalyzer {
public:
    explicit ARIMAAnalyzer(int p = 1, int d = 1, int q = 1);

    TrendResult analyzeTrend(
        const std::vector<TrendDataPoint>& data,
        const TrendAnalysisConfig& config = {}
    ) override;

    std::vector<double> predictFuture(
        const std::vector<TrendDataPoint>& data,
        size_t horizon,
        const TrendAnalysisConfig& config = {}
    ) override;

    std::string getAlgorithmName() const override;
    bool supportsPrediction() const override;

    void setARIMAOrder(int p, int d, int q);

private:
    int p_{1};  // AutoRegressive order
    int d_{1};  // Integrated order
    int q_{1};  // Moving Average order

    std::vector<double> difference(const std::vector<double>& data, int order = 1);
    std::vector<double> autoRegressive(const std::vector<double>& data, int order);
    std::vector<double> movingAverage(const std::vector<double>& residuals, int order);
    std::vector<double> invertDifference(const std::vector<double>& diff_data, const std::vector<double>& original, int order);
};

/**
 * @brief Anomaly detector interface
 */
class AnomalyDetector {
public:
    virtual ~AnomalyDetector() = default;

    /**
     * @brief Detect anomalies in data
     */
    virtual AnomalyResult detectAnomalies(
        const std::vector<TrendDataPoint>& data,
        const TrendAnalysisConfig& config = {}
    ) = 0;

    /**
     * @brief Get detector name
     */
    virtual std::string getDetectorName() const = 0;
};

/**
 * @brief Statistical Z-score anomaly detector
 */
class ZScoreAnomalyDetector : public AnomalyDetector {
public:
    explicit ZScoreAnomalyDetector(double threshold = 3.0);

    AnomalyResult detectAnomalies(
        const std::vector<TrendDataPoint>& data,
        const TrendAnalysisConfig& config = {}
    ) override;

    std::string getDetectorName() const override;
    void setThreshold(double threshold);

private:
    double threshold_{3.0};
    std::vector<double> calculateZScores(const std::vector<double>& values);
};

/**
 * @brief Isolation forest anomaly detector
 */
class IsolationForestAnomalyDetector : public AnomalyDetector {
public:
    explicit IsolationForestAnomalyDetector(size_t n_estimators = 100, double contamination = 0.1);

    AnomalyResult detectAnomalies(
        const std::vector<TrendDataPoint>& data,
        const TrendAnalysisConfig& config = {}
    ) override;

    std::string getDetectorName() const override;

private:
    size_t n_estimators_{100};
    double contamination_{0.1};

    struct IsolationTree {
        std::vector<std::vector<size_t>> children;
        std::vector<double> split_values;
        std::vector<int> split_features;
        std::vector<int> tree_depth;
    };

    IsolationTree buildIsolationTree(const std::vector<std::vector<double>>& data, int max_depth);
    double calculatePathLength(const std::vector<double>& point, const IsolationTree& tree);
    std::vector<double> calculateAnomalyScores(const std::vector<std::vector<double>>& data);
};

/**
 * @brief Seasonal decomposition analyzer
 */
class SeasonalDecomposer {
public:
    explicit SeasonalDecomposer(std::chrono::seconds season_period = std::chrono::hours(24));

    SeasonalResult decompose(
        const std::vector<TrendDataPoint>& data,
        const TrendAnalysisConfig& config = {}
    );

    void setSeasonPeriod(std::chrono::seconds period);
    std::chrono::seconds getSeasonPeriod() const;

private:
    std::chrono::seconds season_period_{std::chrono::hours(24)};

    std::vector<double> extractTrend(const std::vector<double>& data, size_t window_size);
    std::vector<double> extractSeasonal(const std::vector<double>& data, const std::vector<double>& trend, size_t period);
    std::vector<double> extractResidual(const std::vector<double>& data, const std::vector<double>& trend, const std::vector<double>& seasonal);
    std::vector<size_t> findPeaks(const std::vector<double>& data);
    std::vector<size_t> findValleys(const std::vector<double>& data);
};

/**
 * @brief Performance trend analysis engine
 *
 * Comprehensive system for analyzing performance trends, detecting anomalies,
 * identifying seasonal patterns, and predicting future performance.
 */
class PerformanceTrendAnalyzer {
public:
    explicit PerformanceTrendAnalyzer(const TrendAnalysisConfig& config = {});
    ~PerformanceTrendAnalyzer();

    // Data management
    void addDataPoint(const TrendDataPoint& point);
    void addDataPoints(const std::vector<TrendDataPoint>& points);
    std::vector<TrendDataPoint> getData(const std::string& metric_name,
                                       std::chrono::system_clock::time_point start = {},
                                       std::chrono::system_clock::time_point end = {}) const;

    // Trend analysis
    TrendResult analyzeTrend(const std::string& metric_name);
    std::map<std::string, TrendResult> analyzeAllMetrics();
    TrendResult analyzeCustomTrend(const std::vector<TrendDataPoint>& data);

    // Anomaly detection
    AnomalyResult detectAnomalies(const std::string& metric_name);
    std::map<std::string, AnomalyResult> detectAllAnomalies();

    // Seasonal analysis
    SeasonalResult analyzeSeasonality(const std::string& metric_name);
    std::map<std::string, SeasonalResult> analyzeAllSeasonality();

    // Prediction
    std::vector<double> predictPerformance(const std::string& metric_name, size_t horizon = 10);
    std::map<std::string, std::vector<double>> predictAllMetrics(size_t horizon = 10);

    // Continuous monitoring
    void startRealTimeAnalysis(std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor);
    void stopRealTimeAnalysis();
    bool isAnalyzing() const { return analysis_active_.load(); }

    // Configuration
    void updateConfiguration(const TrendAnalysisConfig& config);
    TrendAnalysisConfig getConfiguration() const { return config_; }

    // Alert integration
    void setAnomalyAlertCallback(std::function<void(const std::string&, const AnomalyResult&)> callback);
    void setTrendAlertCallback(std::function<void(const std::string&, const TrendResult&)> callback);

    // Statistics and reporting
    struct AnalysisStatistics {
        size_t total_metrics{0};
        size_t metrics_with_trends{0};
        size_t metrics_with_anomalies{0};
        size_t metrics_with_seasonality{0};
        std::map<TrendDirection, size_t> trend_directions;
        std::map<TrendConfidence, size_t> confidence_levels;
        std::chrono::system_clock::time_point last_analysis;
        double average_prediction_accuracy{0.0};
    };

    AnalysisStatistics getStatistics() const;
    std::string generateTrendReport() const;
    bool exportAnalysisResults(const std::string& filename) const;

    // Health check
    bool isHealthy() const;
    std::vector<std::string> getHealthIssues() const;

private:
    TrendAnalysisConfig config_;
    mutable std::mutex config_mutex_;

    // Data storage
    std::map<std::string, std::vector<TrendDataPoint>> metric_data_;
    mutable std::mutex data_mutex_;

    // Analyzers
    std::map<TrendAlgorithm, std::unique_ptr<TrendAnalyzer>> trend_analyzers_;
    std::map<AnomalyMethod, std::unique_ptr<AnomalyDetector>> anomaly_detectors_;
    std::unique_ptr<SeasonalDecomposer> seasonal_decomposer_;

    // Real-time analysis
    std::atomic<bool> analysis_active_{false};
    std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor_;
    std::unique_ptr<std::thread> analysis_thread_;

    // Alert callbacks
    std::function<void(const std::string&, const AnomalyResult&)> anomaly_alert_callback_;
    std::function<void(const std::string&, const TrendResult&)> trend_alert_callback_;

    // Caching
    std::map<std::string, std::pair<TrendResult, std::chrono::steady_clock::time_point>> trend_cache_;
    std::map<std::string, std::pair<AnomalyResult, std::chrono::steady_clock::time_point>> anomaly_cache_;
    mutable std::mutex cache_mutex_;

    // Analysis results
    std::map<std::string, TrendResult> latest_trends_;
    std::map<std::string, AnomalyResult> latest_anomalies_;
    std::map<std::string, SeasonalResult> latest_seasonal_;
    mutable std::mutex results_mutex_;

    // Statistics
    mutable std::mutex stats_mutex_;
    AnalysisStatistics statistics_;

    // Private methods
    void analysisLoop();
    void processRealTimeData();

    TrendResult performTrendAnalysis(const std::string& metric_name, const std::vector<TrendDataPoint>& data);
    AnomalyResult performAnomalyDetection(const std::string& metric_name, const std::vector<TrendDataPoint>& data);
    SeasonalResult performSeasonalAnalysis(const std::string& metric_name, const std::vector<TrendDataPoint>& data);

    void cleanupOldData();
    void updateStatistics();
    void triggerAnomalyAlert(const std::string& metric_name, const AnomalyResult& result);
    void triggerTrendAlert(const std::string& metric_name, const TrendResult& result);

    // Utility methods
    std::vector<double> extractValues(const std::vector<TrendDataPoint>& points);
    std::vector<double> extractTimestamps(const std::vector<TrendDataPoint>& points);
    std::chrono::system_clock::time_point getCurrentTime() const;
    std::string formatTimestamp(const std::chrono::system_clock::time_point& timestamp) const;
    std::string escapeJsonString(const std::string& str) const;

    // Factory methods
    void initializeAnalyzers();
    std::unique_ptr<TrendAnalyzer> createTrendAnalyzer(TrendAlgorithm algorithm);
    std::unique_ptr<AnomalyDetector> createAnomalyDetector(AnomalyMethod method);
};

/**
 * @brief Trend analysis factory
 */
class TrendAnalyzerFactory {
public:
    /**
     * @brief Create analyzer with default configuration
     */
    static std::unique_ptr<PerformanceTrendAnalyzer> create();

    /**
     * @brief Create analyzer with custom configuration
     */
    static std::unique_ptr<PerformanceTrendAnalyzer> create(const TrendAnalysisConfig& config);

    /**
     * @brief Create analyzer for real-time monitoring
     */
    static std::unique_ptr<PerformanceTrendAnalyzer> createRealTimeAnalyzer(
        std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor
    );

    /**
     * @brief Create analyzer for high-frequency analysis
     */
    static std::unique_ptr<PerformanceTrendAnalyzer> createHighFrequencyAnalyzer();

    /**
     * @brief Create analyzer for long-term trend analysis
     */
    static std::unique_ptr<PerformanceTrendAnalyzer> createLongTermAnalyzer();

    /**
     * @brief Create configuration templates
     */
    static TrendAnalysisConfig createDefaultConfig();
    static TrendAnalysisConfig createRealTimeConfig();
    static TrendAnalysisConfig createHighFrequencyConfig();
    static TrendAnalysisConfig createLongTermConfig();
    static TrendAnalysisConfig createResearchConfig();
};

/**
 * @brief Trend analysis utilities
 */
namespace trend_utils {

/**
 * @brief Data preprocessing utilities
 */
std::vector<TrendDataPoint> removeOutliers(const std::vector<TrendDataPoint>& data, double threshold = 3.0);
std::vector<TrendDataPoint> smoothData(const std::vector<TrendDataPoint>& data, size_t window_size = 5);
std::vector<TrendDataPoint> resampleData(const std::vector<TrendDataPoint>& data, std::chrono::seconds interval);
std::vector<TrendDataPoint> fillMissingData(const std::vector<TrendDataPoint>& data, std::chrono::seconds max_gap);

/**
 * @brief Statistical utilities
 */
double calculateMean(const std::vector<double>& values);
double calculateStdDev(const std::vector<double>& values);
double calculateVariance(const std::vector<double>& values);
double calculateCorrelation(const std::vector<double>& x, const std::vector<double>& y);
double calculateR2(const std::vector<double>& observed, const std::vector<double>& predicted);
std::vector<double> calculateMovingAverage(const std::vector<double>& values, size_t window_size);
std::vector<double> calculateExponentialMovingAverage(const std::vector<double>& values, double alpha);

/**
 * @brief Trend strength metrics
 */
double calculateTrendStrength(const TrendResult& result);
double calculateVolatility(const std::vector<TrendDataPoint>& data);
double calculateSeasonalStrength(const SeasonalResult& result);
double calculateAnomalyRate(const AnomalyResult& result);

/**
 * @brief Prediction accuracy metrics
 */
double calculateMAE(const std::vector<double>& actual, const std::vector<double>& predicted);
double calculateRMSE(const std::vector<double>& actual, const std::vector<double>& predicted);
double calculateMAPE(const std::vector<double>& actual, const std::vector<double>& predicted);
double calculateForecastBias(const std::vector<double>& actual, const std::vector<double>& predicted);

/**
 * @brief Time series utilities
 */
std::vector<std::pair<std::chrono::system_clock::time_point, double>>
createUniformTimeSeries(const std::vector<TrendDataPoint>& data);
std::chrono::seconds estimateOptimalInterval(const std::vector<TrendDataPoint>& data);
std::vector<size_t> findChangePoints(const std::vector<TrendDataPoint>& data);
std::vector<std::vector<TrendDataPoint>> segmentByTrend(const std::vector<TrendDataPoint>& data);

/**
 * @brief Visualization utilities
 */
std::string generateTrendPlot(const std::vector<TrendDataPoint>& data, const TrendResult& result);
std::string generateAnomalyPlot(const std::vector<TrendDataPoint>& data, const AnomalyResult& result);
std::string generateSeasonalPlot(const std::vector<TrendDataPoint>& data, const SeasonalResult& result);
std::string generatePredictionPlot(const std::vector<TrendDataPoint>& historical,
                                   const std::vector<double>& predicted);

} // namespace trend_utils

} // namespace puzzle71::trend
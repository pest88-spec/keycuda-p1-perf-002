// Puzzle71Solver - Performance Trend Analysis Main Implementation (T050)
// Phase 6: User Story 4 - Performance Monitoring
// Main PerformanceTrendAnalyzer class implementation

#include "performance_trend_analyzer.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>

namespace puzzle71::trend {

// ============================================================================
// PerformanceTrendAnalyzer Implementation
// ============================================================================

PerformanceTrendAnalyzer::PerformanceTrendAnalyzer(const TrendAnalysisConfig& config)
    : config_(config) {
    initializeAnalyzers();
    seasonal_decomposer_ = std::make_unique<SeasonalDecomposer>();
}

PerformanceTrendAnalyzer::~PerformanceTrendAnalyzer() {
    stopRealTimeAnalysis();
}

void PerformanceTrendAnalyzer::initializeAnalyzers() {
    // Initialize trend analyzers
    trend_analyzers_[TrendAlgorithm::LINEAR_REGRESSION] =
        std::make_unique<LinearRegressionAnalyzer>();
    trend_analyzers_[TrendAlgorithm::EXPONENTIAL_SMOOTHING] =
        std::make_unique<ExponentialSmoothingAnalyzer>();
    trend_analyzers_[TrendAlgorithm::ARIMA] =
        std::make_unique<ARIMAAnalyzer>();

    // Initialize anomaly detectors
    anomaly_detectors_[AnomalyMethod::STATISTICAL_ZSCORE] =
        std::make_unique<ZScoreAnomalyDetector>();
    anomaly_detectors_[AnomalyMethod::ISOLATION_FOREST] =
        std::make_unique<IsolationForestAnomalyDetector>();
}

void PerformanceTrendAnalyzer::addDataPoint(const TrendDataPoint& point) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    metric_data_[point.metric_name].push_back(point);

    // Sort by timestamp to maintain order
    auto& data = metric_data_[point.metric_name];
    std::sort(data.begin(), data.end(),
              [](const TrendDataPoint& a, const TrendDataPoint& b) {
                  return a.timestamp < b.timestamp;
              });

    // Limit data size
    if (data.size() > config_.max_data_points) {
        data.erase(data.begin(), data.begin() + (data.size() - config_.max_data_points));
    }
}

void PerformanceTrendAnalyzer::addDataPoints(const std::vector<TrendDataPoint>& points) {
    for (const auto& point : points) {
        addDataPoint(point);
    }
}

std::vector<TrendDataPoint> PerformanceTrendAnalyzer::getData(
    const std::string& metric_name,
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end
) const {
    std::lock_guard<std::mutex> lock(data_mutex_);

    auto it = metric_data_.find(metric_name);
    if (it == metric_data_.end()) return {};

    const auto& data = it->second;
    std::vector<TrendDataPoint> filtered;

    for (const auto& point : data) {
        if ((!start.time_since_epoch().count() || point.timestamp >= start) &&
            (!end.time_since_epoch().count() || point.timestamp <= end)) {
            filtered.push_back(point);
        }
    }

    return filtered;
}

TrendResult PerformanceTrendAnalyzer::analyzeTrend(const std::string& metric_name) {
    // Check cache first
    if (config_.enable_caching) {
        std::lock_guard<std::mutex> cache_lock(cache_mutex_);
        auto it = trend_cache_.find(metric_name);
        if (it != trend_cache_.end()) {
            auto now = std::chrono::steady_clock::now();
            if (now - it->second.second < std::chrono::duration_cast<std::chrono::steady_clock::duration>(config_.cache_ttl)) {
                return it->second.first;
            }
        }
    }

    auto data = getData(metric_name);
    if (data.size() < config_.min_data_points) {
        TrendResult result;
        result.metric_name = metric_name;
        result.direction = TrendDirection::UNKNOWN;
        result.confidence = TrendConfidence::VERY_LOW;
        return result;
    }

    // Perform trend analysis
    TrendResult result = performTrendAnalysis(metric_name, data);

    // Cache result
    if (config_.enable_caching) {
        std::lock_guard<std::mutex> cache_lock(cache_mutex_);
        trend_cache_[metric_name] = {result, std::chrono::steady_clock::now()};
    }

    // Store latest result
    {
        std::lock_guard<std::mutex> results_lock(results_mutex_);
        latest_trends_[metric_name] = result;
    }

    // Trigger alert if needed
    if (trend_alert_callback_) {
        triggerTrendAlert(metric_name, result);
    }

    return result;
}

std::map<std::string, TrendResult> PerformanceTrendAnalyzer::analyzeAllMetrics() {
    std::map<std::string, TrendResult> results;

    std::lock_guard<std::mutex> lock(data_mutex_);
    for (const auto& [metric_name, _] : metric_data_) {
        results[metric_name] = analyzeTrend(metric_name);
    }

    return results;
}

TrendResult PerformanceTrendAnalyzer::analyzeCustomTrend(const std::vector<TrendDataPoint>& data) {
    if (data.empty()) {
        TrendResult result;
        result.direction = TrendDirection::UNKNOWN;
        result.confidence = TrendConfidence::VERY_LOW;
        return result;
    }

    std::string metric_name = data[0].metric_name;
    return performTrendAnalysis(metric_name, data);
}

AnomalyResult PerformanceTrendAnalyzer::detectAnomalies(const std::string& metric_name) {
    // Check cache first
    if (config_.enable_caching) {
        std::lock_guard<std::mutex> cache_lock(cache_mutex_);
        auto it = anomaly_cache_.find(metric_name);
        if (it != anomaly_cache_.end()) {
            auto now = std::chrono::steady_clock::now();
            if (now - it->second.second < std::chrono::duration_cast<std::chrono::steady_clock::duration>(config_.cache_ttl)) {
                return it->second.first;
            }
        }
    }

    auto data = getData(metric_name);
    if (data.empty()) {
        AnomalyResult result;
        result.metric_name = metric_name;
        return result;
    }

    // Perform anomaly detection
    AnomalyResult result = performAnomalyDetection(metric_name, data);

    // Cache result
    if (config_.enable_caching) {
        std::lock_guard<std::mutex> cache_lock(cache_mutex_);
        anomaly_cache_[metric_name] = {result, std::chrono::steady_clock::now()};
    }

    // Store latest result
    {
        std::lock_guard<std::mutex> results_lock(results_mutex_);
        latest_anomalies_[metric_name] = result;
    }

    // Trigger alert if needed
    if (anomaly_alert_callback_ && result.anomaly_points > 0) {
        triggerAnomalyAlert(metric_name, result);
    }

    return result;
}

std::map<std::string, AnomalyResult> PerformanceTrendAnalyzer::detectAllAnomalies() {
    std::map<std::string, AnomalyResult> results;

    std::lock_guard<std::mutex> lock(data_mutex_);
    for (const auto& [metric_name, _] : metric_data_) {
        results[metric_name] = detectAnomalies(metric_name);
    }

    return results;
}

SeasonalResult PerformanceTrendAnalyzer::analyzeSeasonality(const std::string& metric_name) {
    auto data = getData(metric_name);
    if (data.size() < 2 * seasonal_decomposer_->getSeasonPeriod().count()) {
        SeasonalResult result;
        result.metric_name = metric_name;
        result.has_seasonality = false;
        return result;
    }

    // Perform seasonal analysis
    SeasonalResult result = performSeasonalAnalysis(metric_name, data);

    // Store latest result
    {
        std::lock_guard<std::mutex> results_lock(results_mutex_);
        latest_seasonal_[metric_name] = result;
    }

    return result;
}

std::map<std::string, SeasonalResult> PerformanceTrendAnalyzer::analyzeAllSeasonality() {
    std::map<std::string, SeasonalResult> results;

    std::lock_guard<std::mutex> lock(data_mutex_);
    for (const auto& [metric_name, _] : metric_data_) {
        results[metric_name] = analyzeSeasonality(metric_name);
    }

    return results;
}

std::vector<double> PerformanceTrendAnalyzer::predictPerformance(const std::string& metric_name, size_t horizon) {
    auto data = getData(metric_name);
    if (data.size() < config_.min_data_points) return {};

    // Get the best analyzer for this data
    auto analyzer_it = trend_analyzers_.find(config_.primary_algorithm);
    if (analyzer_it == trend_analyzers_.end() || !analyzer_it->second->supportsPrediction()) {
        // Fall back to linear regression
        analyzer_it = trend_analyzers_.find(TrendAlgorithm::LINEAR_REGRESSION);
    }

    if (analyzer_it != trend_analyzers_.end()) {
        return analyzer_it->second->predictFuture(data, horizon, config_);
    }

    return {};
}

std::map<std::string, std::vector<double>> PerformanceTrendAnalyzer::predictAllMetrics(size_t horizon) {
    std::map<std::string, std::vector<double>> predictions;

    std::lock_guard<std::mutex> lock(data_mutex_);
    for (const auto& [metric_name, _] : metric_data_) {
        predictions[metric_name] = predictPerformance(metric_name, horizon);
    }

    return predictions;
}

void PerformanceTrendAnalyzer::startRealTimeAnalysis(
    std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor
) {
    if (analysis_active_.load()) return;

    monitor_ = monitor;
    analysis_active_.store(true);
    analysis_thread_ = std::make_unique<std::thread>(&PerformanceTrendAnalyzer::analysisLoop, this);
}

void PerformanceTrendAnalyzer::stopRealTimeAnalysis() {
    if (!analysis_active_.load()) return;

    analysis_active_.store(false);
    if (analysis_thread_ && analysis_thread_->joinable()) {
        analysis_thread_->join();
    }
    analysis_thread_.reset();
    monitor_.reset();
}

void PerformanceTrendAnalyzer::updateConfiguration(const TrendAnalysisConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
}

void PerformanceTrendAnalyzer::setAnomalyAlertCallback(
    std::function<void(const std::string&, const AnomalyResult&)> callback
) {
    anomaly_alert_callback_ = callback;
}

void PerformanceTrendAnalyzer::setTrendAlertCallback(
    std::function<void(const std::string&, const TrendResult&)> callback
) {
    trend_alert_callback_ = callback;
}

PerformanceTrendAnalyzer::AnalysisStatistics PerformanceTrendAnalyzer::getStatistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return statistics_;
}

std::string PerformanceTrendAnalyzer::generateTrendReport() const {
    std::ostringstream oss;
    oss << "Performance Trend Analysis Report\n";
    oss << "=================================\n\n";

    auto statistics = getStatistics();
    oss << "Analysis Statistics:\n";
    oss << "  Total Metrics: " << statistics.total_metrics << "\n";
    oss << "  Metrics with Trends: " << statistics.metrics_with_trends << "\n";
    oss << "  Metrics with Anomalies: " << statistics.metrics_with_anomalies << "\n";
    oss << "  Metrics with Seasonality: " << statistics.metrics_with_seasonality << "\n";
    oss << "  Average Prediction Accuracy: " << std::fixed << std::setprecision(2)
        << (statistics.average_prediction_accuracy * 100) << "%\n\n";

    // Trend direction breakdown
    oss << "Trend Direction Breakdown:\n";
    for (const auto& [direction, count] : statistics.trend_directions) {
        std::string direction_name;
        switch (direction) {
            case TrendDirection::IMPROVING: direction_name = "Improving"; break;
            case TrendDirection::DEGRADING: direction_name = "Degrading"; break;
            case TrendDirection::STABLE: direction_name = "Stable"; break;
            case TrendDirection::VOLATILE: direction_name = "Volatile"; break;
            case TrendDirection::SEASONAL: direction_name = "Seasonal"; break;
            default: direction_name = "Unknown"; break;
        }
        oss << "  " << direction_name << ": " << count << "\n";
    }
    oss << "\n";

    // Confidence level breakdown
    oss << "Confidence Level Breakdown:\n";
    for (const auto& [confidence, count] : statistics.confidence_levels) {
        std::string confidence_name;
        switch (confidence) {
            case TrendConfidence::VERY_LOW: confidence_name = "Very Low"; break;
            case TrendConfidence::LOW: confidence_name = "Low"; break;
            case TrendConfidence::MEDIUM: confidence_name = "Medium"; break;
            case TrendConfidence::HIGH: confidence_name = "High"; break;
            case TrendConfidence::VERY_HIGH: confidence_name = "Very High"; break;
            default: confidence_name = "Unknown"; break;
        }
        oss << "  " << confidence_name << ": " << count << "\n";
    }
    oss << "\n";

    // Detailed metric results
    std::lock_guard<std::mutex> results_lock(results_mutex_);
    if (!latest_trends_.empty()) {
        oss << "Detailed Trend Results:\n";
        for (const auto& [metric_name, result] : latest_trends_) {
            oss << "  " << metric_name << ":\n";
            oss << "    Direction: " << result.getDirectionString() << "\n";
            oss << "    Confidence: " << result.getConfidenceString() << "\n";
            oss << "    Slope: " << std::fixed << std::setprecision(6) << result.slope << "\n";
            oss << "    Correlation: " << std::fixed << std::setprecision(3) << result.correlation_coefficient << "\n";
            oss << "    Data Points: " << result.data_point_count << "\n";

            // Check for anomalies
            auto anomaly_it = latest_anomalies_.find(metric_name);
            if (anomaly_it != latest_anomalies_.end() && anomaly_it->second.anomaly_points > 0) {
                oss << "    Anomalies: " << anomaly_it->second.anomaly_points
                    << " (" << std::fixed << std::setprecision(1)
                    << (anomaly_it->second.anomaly_rate * 100) << "%)\n";
            }
            oss << "\n";
        }
    }

    return oss.str();
}

bool PerformanceTrendAnalyzer::exportAnalysisResults(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return false;

    file << "{\n";
    file << "  \"timestamp\": \"" << formatTimestamp(getCurrentTime()) << "\",\n";
    file << "  \"statistics\": {\n";

    auto statistics = getStatistics();
    file << "    \"total_metrics\": " << statistics.total_metrics << ",\n";
    file << "    \"metrics_with_trends\": " << statistics.metrics_with_trends << ",\n";
    file << "    \"metrics_with_anomalies\": " << statistics.metrics_with_anomalies << ",\n";
    file << "    \"metrics_with_seasonality\": " << statistics.metrics_with_seasonality << ",\n";
    file << "    \"average_prediction_accuracy\": " << statistics.average_prediction_accuracy << "\n";
    file << "  },\n";

    // Export trends
    file << "  \"trends\": {\n";
    std::lock_guard<std::mutex> results_lock(results_mutex_);
    bool first_trend = true;
    for (const auto& [metric_name, result] : latest_trends_) {
        if (!first_trend) file << ",\n";
        file << "    \"" << escapeJsonString(metric_name) << "\": " << result.toJson();
        first_trend = false;
    }
    file << "\n  },\n";

    // Export anomalies
    file << "  \"anomalies\": {\n";
    bool first_anomaly = true;
    for (const auto& [metric_name, result] : latest_anomalies_) {
        if (!first_anomaly) file << ",\n";
        file << "    \"" << escapeJsonString(metric_name) << "\": " << result.toJson();
        first_anomaly = false;
    }
    file << "\n  },\n";

    // Export seasonal analysis
    file << "  \"seasonality\": {\n";
    bool first_seasonal = true;
    for (const auto& [metric_name, result] : latest_seasonal_) {
        if (!first_seasonal) file << ",\n";
        file << "    \"" << escapeJsonString(metric_name) << "\": " << result.toJson();
        first_seasonal = false;
    }
    file << "\n  }\n";

    file << "}\n";
    return true;
}

bool PerformanceTrendAnalyzer::isHealthy() const {
    // Check if analyzer is in healthy state
    if (!config_.isValid()) return false;

    // Check if real-time analysis is running properly
    if (analysis_active_.load() && !monitor_) return false;

    return true;
}

std::vector<std::string> PerformanceTrendAnalyzer::getHealthIssues() const {
    std::vector<std::string> issues;

    if (!config_.isValid()) {
        issues.push_back("Invalid configuration");
    }

    if (analysis_active_.load() && !monitor_) {
        issues.push_back("Real-time analysis active but no monitor connected");
    }

    std::lock_guard<std::mutex> lock(data_mutex_);
    if (metric_data_.empty()) {
        issues.push_back("No data available for analysis");
    }

    return issues;
}

// ============================================================================
// Private Methods
// ============================================================================

void PerformanceTrendAnalyzer::analysisLoop() {
    while (analysis_active_.load()) {
        try {
            processRealTimeData();
            cleanupOldData();
            updateStatistics();
        } catch (const std::exception& e) {
            // Log error and continue
            std::cerr << "Error in trend analysis loop: " << e.what() << std::endl;
        }

        // Sleep for analysis interval
        std::this_thread::sleep_for(std::chrono::seconds(30)); // 30-second intervals
    }
}

void PerformanceTrendAnalyzer::processRealTimeData() {
    if (!monitor_) return;

    // Get recent metrics from monitor
    auto recent_metrics = monitor_->getRecentMetrics(std::chrono::minutes(5));

    // Convert to trend data points
    std::vector<TrendDataPoint> new_points;
    for (const auto& metric : recent_metrics) {
        TrendDataPoint point;
        point.timestamp = metric.timestamp;
        point.metric_name = metric.name;
        point.value = metric.throughput; // Simplified - use throughput as primary metric
        new_points.push_back(point);
    }

    if (!new_points.empty()) {
        addDataPoints(new_points);
    }
}

TrendResult PerformanceTrendAnalyzer::performTrendAnalysis(
    const std::string& metric_name, const std::vector<TrendDataPoint>& data
) {
    TrendResult result;
    result.metric_name = metric_name;

    // Preprocess data
    auto clean_data = trend_utils::removeOutliers(data, config_.outlier_threshold);
    if (clean_data.size() < config_.min_data_points) {
        clean_data = data; // Use original data if too many outliers removed
    }

    // Select analyzer based on configuration
    auto analyzer_it = trend_analyzers_.find(config_.primary_algorithm);
    if (analyzer_it != trend_analyzers_.end()) {
        result = analyzer_it->second->analyzeTrend(clean_data, config_);
    } else {
        // Fall back to linear regression
        result = trend_analyzers_[TrendAlgorithm::LINEAR_REGRESSION]->analyzeTrend(clean_data, config_);
    }

    // Generate predictions if requested
    if (config_.include_predictions && analyzer_it->second->supportsPrediction()) {
        result.future_predictions = analyzer_it->second->predictFuture(
            clean_data, config_.prediction_horizon, config_
        );

        if (!result.future_predictions.empty()) {
            result.next_value_prediction = result.future_predictions[0];
        }
    }

    return result;
}

AnomalyResult PerformanceTrendAnalyzer::performAnomalyDetection(
    const std::string& metric_name, const std::vector<TrendDataPoint>& data
) {
    AnomalyResult result;
    result.metric_name = metric_name;

    auto detector_it = anomaly_detectors_.find(config_.anomaly_method);
    if (detector_it != anomaly_detectors_.end()) {
        result = detector_it->second->detectAnomalies(data, config_);
    } else {
        // Fall back to Z-score detection
        result = anomaly_detectors_[AnomalyMethod::STATISTICAL_ZSCORE]->detectAnomalies(data, config_);
    }

    return result;
}

SeasonalResult PerformanceTrendAnalyzer::performSeasonalAnalysis(
    const std::string& metric_name, const std::vector<TrendDataPoint>& data
) {
    auto result = seasonal_decomposer_->decompose(data, config_);
    result.metric_name = metric_name;
    return result;
}

void PerformanceTrendAnalyzer::cleanupOldData() {
    if (config_.data_retention.count() == 0) return;

    auto cutoff_time = getCurrentTime() - config_.data_retention;

    std::lock_guard<std::mutex> lock(data_mutex_);
    for (auto& [metric_name, data] : metric_data_) {
        auto it = std::remove_if(data.begin(), data.end(),
                                [cutoff_time](const TrendDataPoint& point) {
                                    return point.timestamp < cutoff_time;
                                });
        data.erase(it, data.end());
    }
}

void PerformanceTrendAnalyzer::updateStatistics() {
    std::lock_guard<std::mutex> stats_lock(stats_mutex_);

    statistics_.total_metrics = metric_data_.size();
    statistics_.metrics_with_trends = latest_trends_.size();
    statistics_.metrics_with_anomalies = 0;
    statistics_.metrics_with_seasonality = 0;
    statistics_.trend_directions.clear();
    statistics_.confidence_levels.clear();

    for (const auto& [metric_name, trend] : latest_trends_) {
        statistics_.trend_directions[trend.direction]++;
        statistics_.confidence_levels[trend.confidence]++;
    }

    for (const auto& [metric_name, anomaly] : latest_anomalies_) {
        if (anomaly.anomaly_points > 0) {
            statistics_.metrics_with_anomalies++;
        }
    }

    for (const auto& [metric_name, seasonal] : latest_seasonal_) {
        if (seasonal.has_seasonality) {
            statistics_.metrics_with_seasonality++;
        }
    }

    statistics_.last_analysis = getCurrentTime();
}

void PerformanceTrendAnalyzer::triggerAnomalyAlert(const std::string& metric_name, const AnomalyResult& result) {
    if (anomaly_alert_callback_) {
        anomaly_alert_callback_(metric_name, result);
    }
}

void PerformanceTrendAnalyzer::triggerTrendAlert(const std::string& metric_name, const TrendResult& result) {
    if (trend_alert_callback_) {
        trend_alert_callback_(metric_name, result);
    }
}

std::chrono::system_clock::time_point PerformanceTrendAnalyzer::getCurrentTime() const {
    return std::chrono::system_clock::now();
}

std::string PerformanceTrendAnalyzer::formatTimestamp(
    const std::chrono::system_clock::time_point& timestamp
) const {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S UTC");
    return oss.str();
}

std::string PerformanceTrendAnalyzer::escapeJsonString(const std::string& str) const {
    std::string escaped;
    for (char c : str) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c; break;
        }
    }
    return escaped;
}

// ============================================================================
// TrendAnalyzerFactory Implementation
// ============================================================================

std::unique_ptr<PerformanceTrendAnalyzer> TrendAnalyzerFactory::create() {
    return std::make_unique<PerformanceTrendAnalyzer>(createDefaultConfig());
}

std::unique_ptr<PerformanceTrendAnalyzer> TrendAnalyzerFactory::create(const TrendAnalysisConfig& config) {
    return std::make_unique<PerformanceTrendAnalyzer>(config);
}

std::unique_ptr<PerformanceTrendAnalyzer> TrendAnalyzerFactory::createRealTimeAnalyzer(
    std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor
) {
    auto analyzer = std::make_unique<PerformanceTrendAnalyzer>(createRealTimeConfig());
    analyzer->startRealTimeAnalysis(monitor);
    return analyzer;
}

std::unique_ptr<PerformanceTrendAnalyzer> TrendAnalyzerFactory::createHighFrequencyAnalyzer() {
    return std::make_unique<PerformanceTrendAnalyzer>(createHighFrequencyConfig());
}

std::unique_ptr<PerformanceTrendAnalyzer> TrendAnalyzerFactory::createLongTermAnalyzer() {
    return std::make_unique<PerformanceTrendAnalyzer>(createLongTermConfig());
}

TrendAnalysisConfig TrendAnalyzerFactory::createDefaultConfig() {
    TrendAnalysisConfig config;
    config.min_data_points = 20;
    config.max_data_points = 5000;
    config.data_retention = std::chrono::hours(168); // 7 days
    config.primary_algorithm = TrendAlgorithm::ENSEMBLE;
    config.significance_level = 0.05;
    config.confidence_level = 0.95;
    config.outlier_threshold = 3.0;
    config.enable_seasonal_analysis = true;
    config.enable_anomaly_detection = true;
    config.prediction_horizon = 10;
    config.prediction_confidence = 0.8;
    config.anomaly_method = AnomalyMethod::ISOLATION_FOREST;
    config.anomaly_sensitivity = 0.5;
    config.enable_realtime_detection = true;
    config.thread_count = 4;
    config.enable_caching = true;
    config.cache_ttl = std::chrono::seconds(300);
    config.include_detailed_stats = true;
    config.include_confidence_intervals = true;
    config.include_predictions = true;
    return config;
}

TrendAnalysisConfig TrendAnalyzerFactory::createRealTimeConfig() {
    auto config = createDefaultConfig();
    config.min_data_points = 10;
    config.data_retention = std::chrono::hours(24); // 1 day
    config.prediction_horizon = 5;
    config.cache_ttl = std::chrono::seconds(60); // 1 minute cache
    config.enable_realtime_detection = true;
    return config;
}

TrendAnalysisConfig TrendAnalyzerFactory::createHighFrequencyConfig() {
    auto config = createDefaultConfig();
    config.min_data_points = 50;
    config.max_data_points = 10000;
    config.data_retention = std::chrono::hours(12); // 12 hours
    config.prediction_horizon = 20;
    config.cache_ttl = std::chrono::seconds(30); // 30 second cache
    config.thread_count = 8;
    return config;
}

TrendAnalysisConfig TrendAnalyzerFactory::createLongTermConfig() {
    auto config = createDefaultConfig();
    config.min_data_points = 100;
    config.max_data_points = 10000;
    config.data_retention = std::chrono::hours(2160); // 90 days
    config.prediction_horizon = 30;
    config.enable_seasonal_analysis = true;
    config.cache_ttl = std::chrono::seconds(1800); // 30 minute cache
    return config;
}

TrendAnalysisConfig TrendAnalyzerFactory::createResearchConfig() {
    auto config = createDefaultConfig();
    config.min_data_points = 200;
    config.max_data_points = 50000;
    config.data_retention = std::chrono::hours(4320); // 180 days
    config.prediction_horizon = 50;
    config.significance_level = 0.01; // Higher significance
    config.confidence_level = 0.99;
    config.enable_seasonal_analysis = true;
    config.enable_anomaly_detection = true;
    config.include_detailed_stats = true;
    config.include_confidence_intervals = true;
    config.include_predictions = true;
    return config;
}

} // namespace puzzle71::trend
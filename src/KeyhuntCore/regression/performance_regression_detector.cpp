// Puzzle71Solver - Automated Performance Regression Detection Implementation (T048)
// Phase 6: User Story 4 - Performance Monitoring
// Statistical analysis system for performance regression detection with baseline management

#include "performance_regression_detector.h"
#include "utils/logger.h"
#include "utils/json_serializer.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <numeric>

namespace puzzle71::regression {

// PerformanceBaseline implementation
bool PerformanceBaseline::isValid() const {
    return sample_count >= 30 &&  // Minimum sample size
           throughput_stddev > 0 &&   // Valid standard deviation
           acceptable_variance > 0 &&
           !name.empty() &&
           !created_at.time_since_epoch().count() == 0;
}

std::chrono::seconds PerformanceBaseline::getAge() const {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - created_at);
}

bool PerformanceBaseline::isExpired() const {
    return std::chrono::system_clock::now() > expires_at;
}

std::string PerformanceBaseline::toJson() const {
    std::ostringstream json;
    json << std::fixed << std::setprecision(4);

    json << "{\n";
    json << "  \"id\": \"" << id << "\",\n";
    json << "  \"name\": \"" << name << "\",\n";
    json << "  \"description\": \"" << description << "\",\n";
    json << "  \"created_at\": " << std::chrono::duration_cast<std::chrono::seconds>(
            created_at.time_since_epoch()).count() << ",\n";
    json << "  \"expires_at\": " << std::chrono::duration_cast<std::chrono::seconds>(
            expires_at.time_since_epoch()).count() << ",\n";

    json << "  \"throughput\": {\n";
    json << "    \"mean\": " << throughput_mean << ",\n";
    json << "    \"stddev\": " << throughput_stddev << ",\n";
    json << "    \"median\": " << throughput_median << ",\n";
    json << "    \"p95\": " << throughput_p95 << ",\n";
    json << "    \"p99\": " << throughput_p99 << "\n";
    json << "  },\n";

    json << "  \"latency\": {\n";
    json << "    \"mean\": " << latency_mean << ",\n";
    json << "    \"stddev\": " << latency_stddev << ",\n";
    json << "    \"p95\": " << latency_p95 << ",\n";
    json << "    \"p99\": " << latency_p99 << "\n";
    json << "  },\n";

    json << "  \"system\": {\n";
    json << "    \"gpu_utilization_mean\": " << gpu_utilization_mean << ",\n";
    json << "    \"memory_utilization_mean\": " << memory_utilization_mean << ",\n";
    json << "    \"power_usage_mean\": " << power_usage_mean << ",\n";
    json << "    \"temperature_mean\": " << temperature_mean << "\n";
    json << "  },\n";

    json << "  \"sample_info\": {\n";
    json << "    \"sample_count\": " << sample_count << ",\n";
    json << "    \"collection_duration_seconds\": " << collection_duration.count() << ",\n";
    json << "    \"device_info\": \"" << device_info << "\",\n";
    json << "    \"configuration_hash\": \"" << configuration_hash << "\"\n";
    json << "  },\n";

    json << "  \"validation\": {\n";
    json << "    \"acceptable_variance\": " << acceptable_variance << ",\n";
    json << "    \"warning_threshold\": " << warning_threshold << ",\n";
    json << "    \"error_threshold\": " << error_threshold << ",\n";
    json << "    \"critical_threshold\": " << critical_threshold << "\n";
    json << "  }\n";
    json << "}";

    return json.str();
}

PerformanceBaseline PerformanceBaseline::fromJson(const std::string& json) {
    PerformanceBaseline baseline;

    // Simplified JSON parsing - integrate with existing JSON serializer
    try {
        // Parse basic fields
        auto name_pos = json.find("\"name\":");
        if (name_pos != std::string::npos) {
            auto start = json.find("\"", name_pos + 8) + 1;
            auto end = json.find("\"", start);
            baseline.name = json.substr(start, end - start);
        }

        // Parse throughput metrics
        auto throughput_pos = json.find("\"throughput\":");
        if (throughput_pos != std::string::npos) {
            auto mean_pos = json.find("\"mean\":", throughput_pos);
            if (mean_pos != std::string::npos) {
                auto colon = json.find(":", mean_pos) + 1;
                auto end = json.find(",", colon);
                baseline.throughput_mean = std::stod(json.substr(colon, end - colon));
            }

            auto stddev_pos = json.find("\"stddev\":", throughput_pos);
            if (stddev_pos != std::string::npos) {
                auto colon = json.find(":", stddev_pos) + 1;
                auto end = json.find(",", colon);
                baseline.throughput_stddev = std::stod(json.substr(colon, end - colon));
            }
        }

        // Parse other fields similarly...

    } catch (const std::exception& e) {
        Logger::error("Failed to parse PerformanceBaseline from JSON: {}", e.what());
    }

    return baseline;
}

// RegressionResult implementation
bool RegressionResult::isActive() const {
    return status == RegressionStatus::DETECTED || status == RegressionStatus::MONITORING;
}

std::string RegressionResult::getSeverityString() const {
    switch (severity) {
        case RegressionSeverity::INFO: return "INFO";
        case RegressionSeverity::WARNING: return "WARNING";
        case RegressionSeverity::ERROR: return "ERROR";
        case RegressionSeverity::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

std::string RegressionResult::getStatusString() const {
    switch (status) {
        case RegressionStatus::NORMAL: return "NORMAL";
        case RegressionStatus::MONITORING: return "MONITORING";
        case RegressionStatus::DETECTED: return "DETECTED";
        case RegressionStatus::RESOLVED: return "RESOLVED";
        case RegressionStatus::IGNORED: return "IGNORED";
        default: return "UNKNOWN";
    }
}

std::string RegressionResult::toJson() const {
    std::ostringstream json;
    json << std::fixed << std::setprecision(4);

    json << "{\n";
    json << "  \"id\": \"" << id << "\",\n";
    json << "  \"detected_at\": " << std::chrono::duration_cast<std::chrono::seconds>(
            detected_at.time_since_epoch()).count() << ",\n";
    json << "  \"severity\": \"" << getSeverityString() << "\",\n";
    json << "  \"status\": \"" << getStatusString() << "\",\n";

    json << "  \"comparison\": {\n";
    json << "    \"baseline_id\": \"" << baseline_id << "\",\n";
    json << "    \"metric_name\": \"" << metric_name << "\",\n";
    json << "    \"baseline_value\": " << baseline_value << ",\n";
    json << "    \"current_value\": " << current_value << ",\n";
    json << "    \"percentage_change\": " << percentage_change << ",\n";
    json << "    \"z_score\": " << z_score << ",\n";
    json << "    \"p_value\": " << p_value << "\n";
    json << "  },\n";

    json << "  \"statistical_analysis\": {\n";
    json << "    \"is_statistically_significant\": " << (is_statistically_significant ? "true" : "false") << ",\n";
    json << "    \"confidence_level\": " << confidence_level << ",\n";
    json << "    \"test_method\": \"" << test_method << "\"\n";
    json << "  },\n";

    json << "  \"affected_metrics\": [";
    for (size_t i = 0; i < affected_metrics.size(); ++i) {
        if (i > 0) json << ", ";
        json << "\"" << affected_metrics[i] << "\"";
    }
    json << "],\n";

    json << "  \"secondary_changes\": {\n";
    bool first = true;
    for (const auto& [metric, change] : secondary_changes) {
        if (!first) json << ",\n";
        json << "    \"" << metric << "\": " << change;
        first = false;
    }
    json << "\n  },\n";

    json << "  \"potential_causes\": [";
    for (size_t i = 0; i < potential_causes.size(); ++i) {
        if (i > 0) json << ", ";
        json << "\"" << potential_causes[i] << "\"";
    }
    json << "],\n";

    json << "  \"environmental_notes\": \"" << environmental_notes << "\",\n";

    if (status == RegressionStatus::RESOLVED) {
        json << "  \"resolution\": {\n";
        json << "    \"resolved_at\": " << std::chrono::duration_cast<std::chrono::seconds>(
                resolved_at.time_since_epoch()).count() << ",\n";
        json << "    \"method\": \"" << resolution_method << "\",\n";
        json << "    \"notes\": \"" << resolution_notes << "\"\n";
        json << "  },\n";
    }

    json << "}";

    return json.str();
}

RegressionResult RegressionResult::fromJson(const std::string& json) {
    RegressionResult result;

    // Simplified JSON parsing - integrate with existing JSON serializer
    try {
        // Parse basic fields
        auto id_pos = json.find("\"id\":");
        if (id_pos != std::string::npos) {
            auto start = json.find("\"", id_pos + 6) + 1;
            auto end = json.find("\"", start);
            result.id = json.substr(start, end - start);
        }

        // Parse severity
        auto severity_pos = json.find("\"severity\":");
        if (severity_pos != std::string::npos) {
            auto start = json.find("\"", severity_pos + 11) + 1;
            auto end = json.find("\"", start);
            std::string severity = json.substr(start, end - start);

            if (severity == "INFO") result.severity = RegressionSeverity::INFO;
            else if (severity == "WARNING") result.severity = RegressionSeverity::WARNING;
            else if (severity == "ERROR") result.severity = RegressionSeverity::ERROR;
            else if (severity == "CRITICAL") result.severity = RegressionSeverity::CRITICAL;
        }

        // Parse other fields similarly...

    } catch (const std::exception& e) {
        Logger::error("Failed to parse RegressionResult from JSON: {}", e.what());
    }

    return result;
}

// PerformanceRegressionDetector implementation
PerformanceRegressionDetector::PerformanceRegressionDetector(const RegressionDetectionConfig& config)
    : config_(config) {

    Logger::info("Performance Regression Detector initialized with {} confidence level",
                 1.0 - config_.significance_level);
}

PerformanceRegressionDetector::~PerformanceRegressionDetector() {
    stopMonitoring();
}

std::string PerformanceRegressionDetector::createBaseline(
    const std::string& name,
    const std::vector<puzzle71::monitoring::PerformanceMetrics>& metrics,
    const std::string& description) {

    if (!validateMetrics(metrics)) {
        throw std::invalid_argument("Invalid metrics for baseline creation");
    }

    auto baseline = calculateBaseline(metrics);
    baseline.name = name;
    baseline.description = description;

    std::lock_guard<std::mutex> lock(baselines_mutex_);
    baseline.id = generateRegressionId();
    baselines_[baseline.id] = baseline;

    Logger::info("Created baseline '{}' with {} samples", name, metrics.size());
    return baseline.id;
}

bool PerformanceRegressionDetector::updateBaseline(
    const std::string& baseline_id,
    const std::vector<puzzle71::monitoring::PerformanceMetrics>& metrics) {

    if (!validateMetrics(metrics)) {
        Logger::error("Invalid metrics for baseline update");
        return false;
    }

    std::lock_guard<std::mutex> lock(baselines_mutex_);
    auto it = baselines_.find(baseline_id);
    if (it == baselines_.end()) {
        Logger::error("Baseline not found: {}", baseline_id);
        return false;
    }

    auto new_baseline = calculateBaseline(metrics);
    new_baseline.id = baseline_id;
    new_baseline.name = it->second.name;
    new_baseline.description = it->second.description;
    new_baseline.created_at = it->second.created_at;

    baselines_[baseline_id] = new_baseline;

    Logger::info("Updated baseline '{}' with {} samples", baseline_id, metrics.size());
    return true;
}

bool PerformanceRegressionDetector::removeBaseline(const std::string& baseline_id) {
    std::lock_guard<std::mutex> lock(baselines_mutex_);

    auto it = baselines_.find(baseline_id);
    if (it == baselines_.end()) {
        return false;
    }

    // Remove from primary baselines mapping
    for (auto primary_it = primary_baselines_.begin(); primary_it != primary_baselines_.end();) {
        if (primary_it->second == baseline_id) {
            primary_it = primary_baselines_.erase(primary_it);
        } else {
            ++primary_it;
        }
    }

    baselines_.erase(it);
    Logger::info("Removed baseline: {}", baseline_id);
    return true;
}

std::vector<PerformanceBaseline> PerformanceRegressionDetector::getBaselines() const {
    std::lock_guard<std::mutex> lock(baselines_mutex_);

    std::vector<PerformanceBaseline> baselines;
    for (const auto& [id, baseline] : baselines_) {
        baselines.push_back(baseline);
    }

    return baselines;
}

PerformanceBaseline PerformanceRegressionDetector::getBaseline(const std::string& baseline_id) const {
    std::lock_guard<std::mutex> lock(baselines_mutex_);

    auto it = baselines_.find(baseline_id);
    if (it != baselines_.end()) {
        return it->second;
    }

    return PerformanceBaseline{};
}

bool PerformanceRegressionDetector::setPrimaryBaseline(const std::string& metric_name, const std::string& baseline_id) {
    std::lock_guard<std::mutex> lock(baselines_mutex_);

    auto it = baselines_.find(baseline_id);
    if (it == baselines_.end()) {
        Logger::error("Baseline not found: {}", baseline_id);
        return false;
    }

    primary_baselines_[metric_name] = baseline_id;
    Logger::info("Set primary baseline for '{}' to {}", metric_name, baseline_id);
    return true;
}

std::vector<RegressionResult> PerformanceRegressionDetector::detectRegressions(
    const puzzle71::monitoring::PerformanceMetrics& current_metrics) {

    std::vector<RegressionResult> results;

    // Check throughput regression
    std::string baseline_id = getPrimaryBaselineId("throughput");
    if (!baseline_id.empty()) {
        auto result = detectRegression("throughput", current_metrics.keys_per_second, baseline_id);
        if (result.status != RegressionStatus::NORMAL) {
            results.push_back(result);
        }
    }

    // Check latency regression
    baseline_id = getPrimaryBaselineId("latency");
    if (!baseline_id.empty()) {
        auto result = detectRegression("latency", current_metrics.kernel_execution_time_ms, baseline_id);
        if (result.status != RegressionStatus::NORMAL) {
            results.push_back(result);
        }
    }

    // Check GPU utilization regression
    baseline_id = getPrimaryBaselineId("gpu_utilization");
    if (!baseline_id.empty()) {
        auto result = detectRegression("gpu_utilization", current_metrics.gpu_utilization_percent, baseline_id);
        if (result.status != RegressionStatus::NORMAL) {
            results.push_back(result);
        }
    }

    // Check memory utilization regression
    baseline_id = getPrimaryBaselineId("memory_utilization");
    if (!baseline_id.empty()) {
        auto result = detectRegression("memory_utilization", current_metrics.memory_utilization_percent, baseline_id);
        if (result.status != RegressionStatus::NORMAL) {
            results.push_back(result);
        }
    }

    // Store and alert on regressions
    for (const auto& result : results) {
        storeRegression(result);
        if (shouldAlert(result)) {
            triggerAlert(result);
        }
    }

    return results;
}

std::vector<RegressionResult> PerformanceRegressionDetector::detectRegressions(
    const std::vector<puzzle71::monitoring::PerformanceMetrics>& metrics_series) {

    std::vector<RegressionResult> results;

    if (metrics_series.size() < config_.min_sample_size) {
        Logger::warn("Insufficient data for regression detection: {} samples (required: {})",
                     metrics_series.size(), config_.min_sample_size);
        return results;
    }

    // Create temporary baseline from older data and compare with newer data
    size_t split_point = metrics_series.size() / 2;
    std::vector<puzzle71::monitoring::PerformanceMetrics> baseline_data(
        metrics_series.begin(), metrics_series.begin() + split_point);
    std::vector<puzzle71::monitoring::PerformanceMetrics> current_data(
        metrics_series.begin() + split_point, metrics_series.end());

    // Calculate baseline from older data
    auto baseline = calculateBaseline(baseline_data);

    // Check regressions in newer data
    for (const auto& metrics : current_data) {
        auto current_results = detectRegressions(metrics);
        results.insert(results.end(), current_results.begin(), current_results.end());
    }

    return results;
}

RegressionResult PerformanceRegressionDetector::detectRegression(
    const std::string& metric_name,
    double current_value,
    const std::string& baseline_id) {

    RegressionResult result;
    result.id = generateRegressionId();
    result.detected_at = getCurrentTime();
    result.metric_name = metric_name;
    result.current_value = current_value;

    // Find appropriate baseline
    std::string effective_baseline_id = baseline_id;
    if (effective_baseline_id.empty()) {
        effective_baseline_id = getPrimaryBaselineId(metric_name);
    }

    if (effective_baseline_id.empty()) {
        result.status = RegressionStatus::NORMAL;
        return result;
    }

    std::lock_guard<std::mutex> lock(baselines_mutex_);
    auto baseline_it = baselines_.find(effective_baseline_id);
    if (baseline_it == baselines_.end()) {
        result.status = RegressionStatus::NORMAL;
        return result;
    }

    const auto& baseline = baseline_it->second;
    result.baseline_id = effective_baseline_id;

    // Get baseline statistics for this metric
    double baseline_mean = 0.0;
    double baseline_stddev = 0.0;
    double warning_threshold = 0.0;
    double error_threshold = 0.0;
    double critical_threshold = 0.0;

    if (metric_name == "throughput") {
        baseline_mean = baseline.throughput_mean;
        baseline_stddev = baseline.throughput_stddev;
        warning_threshold = baseline.warning_threshold;
        error_threshold = baseline.error_threshold;
        critical_threshold = baseline.critical_threshold;
    } else if (metric_name == "latency") {
        baseline_mean = baseline.latency_mean;
        baseline_stddev = baseline.latency_stddev;
        warning_threshold = baseline.warning_threshold;
        error_threshold = baseline.error_threshold;
        critical_threshold = baseline.critical_threshold;
    } else if (metric_name == "gpu_utilization") {
        baseline_mean = baseline.gpu_utilization_mean;
        baseline_stddev = 5.0;  // Estimated stddev for utilization
        warning_threshold = baseline.warning_threshold;
        error_threshold = baseline.error_threshold;
        critical_threshold = baseline.critical_threshold;
    } else if (metric_name == "memory_utilization") {
        baseline_mean = baseline.memory_utilization_mean;
        baseline_stddev = 5.0;  // Estimated stddev for utilization
        warning_threshold = baseline.warning_threshold;
        error_threshold = baseline.error_threshold;
        critical_threshold = baseline.critical_threshold;
    }

    result.baseline_value = baseline_mean;

    // Calculate percentage change
    if (baseline_mean != 0) {
        result.percentage_change = ((current_value - baseline_mean) / baseline_mean) * 100.0;
    }

    // Perform statistical test
    if (performStatisticalTest(current_value, baseline, result)) {
        // Determine severity based on percentage change
        double abs_change = std::abs(result.percentage_change);

        if (abs_change >= critical_threshold * 100) {
            result.severity = RegressionSeverity::CRITICAL;
            result.status = RegressionStatus::DETECTED;
        } else if (abs_change >= error_threshold * 100) {
            result.severity = RegressionSeverity::ERROR;
            result.status = RegressionStatus::DETECTED;
        } else if (abs_change >= warning_threshold * 100) {
            result.severity = RegressionSeverity::WARNING;
            result.status = RegressionStatus::MONITORING;
        } else if (abs_change >= config_.minimum_change_threshold * 100) {
            result.severity = RegressionSeverity::INFO;
            result.status = RegressionStatus::MONITORING;
        } else {
            result.status = RegressionStatus::NORMAL;
        }

        // For performance metrics, lower values are better for latency
        // but higher values are better for throughput
        bool is_improvement = false;
        if (metric_name == "throughput" || metric_name == "gpu_utilization") {
            is_improvement = result.percentage_change > 0;
        } else if (metric_name == "latency" || metric_name == "memory_utilization") {
            is_improvement = result.percentage_change < 0;
        }

        if (is_improvement && result.status != RegressionStatus::NORMAL) {
            // This is actually an improvement, not a regression
            result.status = RegressionStatus::NORMAL;
            result.potential_causes.push_back("Performance improvement detected");
        } else if (!is_improvement && result.status != RegressionStatus::NORMAL) {
            // This is a regression
            result.potential_causes.push_back("Performance degradation detected");
            if (metric_name == "throughput") {
                result.potential_causes.push_back("Possible algorithmic inefficiency");
                result.potential_causes.push_back("Hardware resource constraints");
            } else if (metric_name == "latency") {
                result.potential_causes.push_back("Increased computational complexity");
                result.potential_causes.push_back("Memory access pattern changes");
            }
        }
    }

    return result;
}

void PerformanceRegressionDetector::startMonitoring(
    std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor) {

    if (monitoring_active_.load()) {
        Logger::warn("Regression monitoring is already active");
        return;
    }

    monitor_ = monitor;
    monitoring_active_.store(true);

    monitoring_thread_ = std::make_unique<std::thread>(&PerformanceRegressionDetector::monitoringLoop, this);

    Logger::info("Started regression monitoring");
}

void PerformanceRegressionDetector::stopMonitoring() {
    if (!monitoring_active_.load()) {
        return;
    }

    monitoring_active_.store(false);

    if (monitoring_thread_ && monitoring_thread_->joinable()) {
        monitoring_thread_->join();
    }

    Logger::info("Stopped regression monitoring");
}

void PerformanceRegressionDetector::setAlertCallback(std::function<void(const RegressionResult&)> callback) {
    alert_callback_ = callback;
}

std::vector<RegressionResult> PerformanceRegressionDetector::getActiveRegressions() const {
    std::lock_guard<std::mutex> lock(regressions_mutex_);

    std::vector<RegressionResult> active;
    for (const auto& [id, regression] : regressions_) {
        if (regression.isActive()) {
            active.push_back(regression);
        }
    }

    return active;
}

void PerformanceRegressionDetector::acknowledgeRegression(const std::string& regression_id) {
    std::lock_guard<std::mutex> lock(regressions_mutex_);

    auto it = regressions_.find(regression_id);
    if (it != regressions_.end()) {
        it->second.status = RegressionStatus::MONITORING;
        Logger::info("Acknowledged regression: {}", regression_id);
    }
}

void PerformanceRegressionDetector::resolveRegression(
    const std::string& regression_id, const std::string& resolution_method) {

    std::lock_guard<std::mutex> lock(regressions_mutex_);

    auto it = regressions_.find(regression_id);
    if (it != regressions_.end()) {
        it->second.status = RegressionStatus::RESOLVED;
        it->second.resolved_at = getCurrentTime();
        it->second.resolution_method = resolution_method;

        Logger::info("Resolved regression: {} with method: {}", regression_id, resolution_method);
    }
}

void PerformanceRegressionDetector::updateConfiguration(const RegressionDetectionConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
    Logger::info("Updated regression detection configuration");
}

TimeSeriesAnalysis PerformanceRegressionDetector::analyzeTimeSeries(
    const std::vector<std::pair<std::chrono::system_clock::time_point, double>>& data) {

    TimeSeriesAnalysis analysis;

    if (data.size() < 2) {
        return analysis;
    }

    // Extract values
    std::vector<double> values;
    for (const auto& [timestamp, value] : data) {
        values.push_back(value);
    }

    // Calculate trend using linear regression
    size_t n = values.size();
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;

    for (size_t i = 0; i < n; ++i) {
        double x = static_cast<double>(i);
        double y = values[i];
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }

    analysis.trend_slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
    analysis.trend_intercept = (sum_y - analysis.trend_slope * sum_x) / n;

    // Calculate correlation coefficient
    double mean_x = sum_x / n;
    double mean_y = sum_y / n;
    double sum_numerator = 0, sum_x_sq = 0, sum_y_sq = 0;

    for (size_t i = 0; i < n; ++i) {
        double x = static_cast<double>(i);
        double y = values[i];
        sum_numerator += (x - mean_x) * (y - mean_y);
        sum_x_sq += (x - mean_x) * (x - mean_x);
        sum_y_sq += (y - mean_y) * (y - mean_y);
    }

    if (sum_x_sq > 0 && sum_y_sq > 0) {
        analysis.correlation_coefficient = sum_numerator / std::sqrt(sum_x_sq * sum_y_sq);
    }

    // Calculate forecast errors
    analysis.forecast_values.reserve(n);
    double sum_abs_error = 0, sum_sq_error = 0;

    for (size_t i = 0; i < n; ++i) {
        double forecast = analysis.trend_intercept + analysis.trend_slope * static_cast<double>(i);
        analysis.forecast_values.push_back(forecast);

        double error = std::abs(values[i] - forecast);
        sum_abs_error += error;
        sum_sq_error += error * error;
    }

    analysis.mean_absolute_error = sum_abs_error / n;
    analysis.root_mean_square_error = std::sqrt(sum_sq_error / n);

    // Determine if trend is significant
    analysis.has_trend = std::abs(analysis.correlation_coefficient) > 0.3;
    analysis.is_seasonal = false;  // TODO: Implement seasonality detection

    return analysis;
}

std::string PerformanceRegressionDetector::generateRegressionReport() const {
    std::lock_guard<std::mutex> lock(regressions_mutex_);

    std::ostringstream report;
    report << "Performance Regression Detection Report\n";
    report << "=====================================\n\n";

    auto stats = getStatistics();
    report << "Summary:\n";
    report << "  Total detections: " << stats.total_detections << "\n";
    report << "  Active regressions: " << stats.active_regressions << "\n";
    report << "  Resolved regressions: " << stats.resolved_regressions << "\n\n";

    report << "Severity breakdown:\n";
    for (const auto& [severity, count] : stats.severity_counts) {
        report << "  " << severity << ": " << count << "\n";
    }
    report << "\n";

    report << "Active regressions:\n";
    for (const auto& [id, regression] : regressions_) {
        if (regression.isActive()) {
            report << "  " << regression.metric_name << ": " << regression.getSeverityString();
            report << " (" << std::fixed << std::setprecision(2) << regression.percentage_change << "%)\n";
        }
    }

    return report.str();
}

bool PerformanceRegressionDetector::exportRegressions(const std::string& filename) const {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            Logger::error("Failed to open file for regression export: {}", filename);
            return false;
        }

        file << "{\n";
        file << "  \"regressions\": [\n";

        std::lock_guard<std::mutex> lock(regressions_mutex_);
        bool first = true;
        for (const auto& [id, regression] : regressions_) {
            if (!first) file << ",\n";
            file << regression.toJson();
            first = false;
        }

        file << "\n  ],\n";
        file << "  \"statistics\": " << getStatisticsToJson() << "\n";
        file << "}\n";

        file.close();
        Logger::info("Exported {} regressions to {}", regressions_.size(), filename);
        return true;

    } catch (const std::exception& e) {
        Logger::error("Failed to export regressions: {}", e.what());
        return false;
    }
}

PerformanceRegressionDetector::DetectionStatistics PerformanceRegressionDetector::getStatistics() const {
    std::lock_guard<std::mutex> lock(regressions_mutex_);

    DetectionStatistics stats;
    stats.total_detections = regressions_.size();
    stats.last_detection = std::chrono::system_clock::time_point{}; // Will be updated below

    for (const auto& [id, regression] : regressions_) {
        if (regression.isActive()) {
            stats.active_regressions++;
        }
        if (regression.status == RegressionStatus::RESOLVED) {
            stats.resolved_regressions++;
        }

        stats.severity_counts[regression.getSeverityString()]++;
        stats.metric_counts[regression.metric_name]++;

        if (regression.detected_at > stats.last_detection) {
            stats.last_detection = regression.detected_at;
        }
    }

    // Calculate false positive rate (simplified)
    if (stats.total_detections > 0) {
        stats.false_positive_rate = static_cast<double>(stats.resolved_regressions) / stats.total_detections;
    }

    return stats;
}

bool PerformanceRegressionDetector::isHealthy() const {
    auto stats = getStatistics();
    return stats.active_regressions == 0 && stats.total_detections < 100; // Arbitrary threshold
}

std::vector<std::string> PerformanceRegressionDetector::getHealthIssues() const {
    std::vector<std::string> issues;

    auto stats = getStatistics();
    if (stats.active_regressions > 0) {
        issues.push_back(std::to_string(stats.active_regressions) + " active regressions detected");
    }

    if (stats.false_positive_rate > 0.5) {
        issues.push_back("High false positive rate: " + std::to_string(stats.false_positive_rate * 100) + "%");
    }

    std::lock_guard<std::mutex> lock(baselines_mutex_);
    if (baselines_.empty()) {
        issues.push_back("No performance baselines configured");
    }

    return issues;
}

void PerformanceRegressionDetector::monitoringLoop() {
    Logger::debug("Regression monitoring loop started");

    while (monitoring_active_.load()) {
        try {
            if (monitor_ && monitor_->isMonitoring()) {
                auto current_metrics = monitor_->getCurrentMetrics();
                auto regressions = detectRegressions(current_metrics);

                // Store metrics for time series analysis
                {
                    std::lock_guard<std::mutex> lock(metrics_mutex_);
                    collected_metrics_.push_back(current_metrics);

                    // Keep only recent metrics
                    const size_t max_metrics = 1000;
                    if (collected_metrics_.size() > max_metrics) {
                        collected_metrics_.erase(collected_metrics_.begin(),
                                               collected_metrics_.begin() + (collected_metrics_.size() - max_metrics));
                    }
                }
            }

            // Periodic maintenance tasks
            cleanupExpiredBaselines();
            if (config_.auto_update_baselines) {
                autoUpdateBaselines();
            }

        } catch (const std::exception& e) {
            Logger::error("Error in regression monitoring loop: {}", e.what());
        }

        std::this_thread::sleep_for(std::chrono::seconds(10)); // Check every 10 seconds
    }

    Logger::debug("Regression monitoring loop ended");
}

void PerformanceRegressionDetector::processMetrics() {
    // This method can be called for batch processing of collected metrics
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    if (collected_metrics_.size() >= config_.min_sample_size) {
        auto regressions = detectRegressions(collected_metrics_);

        for (const auto& regression : regressions) {
            if (shouldAlert(regression)) {
                triggerAlert(regression);
            }
        }
    }
}

double PerformanceRegressionDetector::calculateZScore(
    double sample_mean, double population_mean, double population_stddev, size_t sample_size) {

    if (population_stddev == 0) return 0.0;

    // For large samples, use population standard deviation
    // For small samples, we could use sample standard deviation with correction
    return (sample_mean - population_mean) / (population_stddev / std::sqrt(static_cast<double>(sample_size)));
}

double PerformanceRegressionDetector::calculateTStatistic(
    double sample_mean, double population_mean, double sample_stddev, size_t sample_size) {

    if (sample_size < 2 || sample_stddev == 0) return 0.0;

    return (sample_mean - population_mean) / (sample_stddev / std::sqrt(static_cast<double>(sample_size)));
}

double PerformanceRegressionDetector::calculatePValue(
    double test_statistic, size_t degrees_of_freedom, StatisticalTest test_type) {

    // Simplified p-value calculation
    // In production, use proper statistical libraries

    if (test_type == StatisticalTest::Z_TEST) {
        // Two-tailed test for z-score
        double abs_z = std::abs(test_statistic);
        if (abs_z < 1.96) return 0.1;      // p > 0.05
        if (abs_z < 2.58) return 0.01;     // p > 0.01
        return 0.001;                       // p < 0.01
    } else if (test_type == StatisticalTest::T_TEST) {
        // Simplified t-test p-value
        double abs_t = std::abs(test_statistic);
        if (abs_t < 2.0) return 0.1;        // p > 0.05 for df > 30
        if (abs_t < 2.7) return 0.01;       // p > 0.01
        return 0.001;                       // p < 0.01
    }

    return 1.0; // Default: not significant
}

bool PerformanceRegressionDetector::performStatisticalTest(
    double current_value, const PerformanceBaseline& baseline, RegressionResult& result) {

    double baseline_mean = 0.0;
    double baseline_stddev = 0.0;

    // Extract appropriate baseline statistics
    if (result.metric_name == "throughput") {
        baseline_mean = baseline.throughput_mean;
        baseline_stddev = baseline.throughput_stddev;
    } else if (result.metric_name == "latency") {
        baseline_mean = baseline.latency_mean;
        baseline_stddev = baseline.latency_stddev;
    }

    // Ensure we have valid baseline statistics
    if (baseline_stddev <= 0) {
        Logger::warn("Invalid baseline standard deviation for metric: {}", result.metric_name);
        return false;
    }

    // Choose appropriate statistical test
    StatisticalTest test_type = StatisticalTest::Z_TEST; // Default for large samples
    double test_statistic = 0.0;
    size_t degrees_of_freedom = baseline.sample_count - 1;

    if (baseline.sample_count >= 30) {
        // Z-test for large samples
        test_statistic = calculateZScore(current_value, baseline_mean, baseline_stddev, baseline.sample_count);
        test_type = StatisticalTest::Z_TEST;
    } else {
        // T-test for small samples
        test_statistic = calculateTStatistic(current_value, baseline_mean, baseline_stddev, baseline.sample_count);
        test_type = StatisticalTest::T_TEST;
    }

    result.z_score = test_statistic;
    result.p_value = calculatePValue(test_statistic, degrees_of_freedom, test_type);
    result.is_statistically_significant = result.p_value < config_.significance_level;
    result.confidence_level = 1.0 - config_.significance_level;

    // Set test method string
    switch (test_type) {
        case StatisticalTest::Z_TEST:
            result.test_method = "z_test";
            break;
        case StatisticalTest::T_TEST:
            result.test_method = "t_test";
            break;
        default:
            result.test_method = "unknown";
            break;
    }

    return result.is_statistically_significant;
}

TimeSeriesAnalysis PerformanceRegressionDetector::performTimeSeriesAnalysis(const std::vector<double>& values) {
    TimeSeriesAnalysis analysis;

    if (values.size() < 2) {
        return analysis;
    }

    // Simple linear regression
    size_t n = values.size();
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;

    for (size_t i = 0; i < n; ++i) {
        double x = static_cast<double>(i);
        double y = values[i];
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }

    analysis.trend_slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
    analysis.trend_intercept = (sum_y - analysis.trend_slope * sum_x) / n;

    // Calculate correlation
    double mean_x = sum_x / n;
    double mean_y = sum_y / n;
    double sum_numerator = 0, sum_x_sq = 0, sum_y_sq = 0;

    for (size_t i = 0; i < n; ++i) {
        double x = static_cast<double>(i);
        double y = values[i];
        sum_numerator += (x - mean_x) * (y - mean_y);
        sum_x_sq += (x - mean_x) * (x - mean_x);
        sum_y_sq += (y - mean_y) * (y - mean_y);
    }

    if (sum_x_sq > 0 && sum_y_sq > 0) {
        analysis.correlation_coefficient = sum_numerator / std::sqrt(sum_x_sq * sum_y_sq);
    }

    analysis.has_trend = std::abs(analysis.correlation_coefficient) > 0.3;

    return analysis;
}

double PerformanceRegressionDetector::calculateTrend(const std::vector<double>& values) {
    if (values.size() < 2) return 0.0;

    size_t n = values.size();
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;

    for (size_t i = 0; i < n; ++i) {
        double x = static_cast<double>(i);
        double y = values[i];
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }

    return (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
}

std::vector<double> PerformanceRegressionDetector::smoothData(
    const std::vector<double>& values, std::chrono::seconds window) {

    if (values.empty() || window.count() == 0) {
        return values;
    }

    std::vector<double> smoothed;
    size_t window_size = std::min(static_cast<size_t>(window.count()), values.size());

    for (size_t i = 0; i < values.size(); ++i) {
        size_t start = (i >= window_size) ? i - window_size + 1 : 0;
        size_t end = i + 1;

        double sum = 0.0;
        for (size_t j = start; j < end; ++j) {
            sum += values[j];
        }

        smoothed.push_back(sum / (end - start));
    }

    return smoothed;
}

void PerformanceRegressionDetector::cleanupExpiredBaselines() {
    std::lock_guard<std::mutex> lock(baselines_mutex_);

    auto now = std::chrono::system_clock::now();
    std::vector<std::string> expired_ids;

    for (const auto& [id, baseline] : baselines_) {
        if (baseline.isExpired()) {
            expired_ids.push_back(id);
        }
    }

    for (const auto& id : expired_ids) {
        baselines_.erase(id);
        Logger::info("Removed expired baseline: {}", id);
    }
}

void PerformanceRegressionDetector::autoUpdateBaselines() {
    // This would implement automatic baseline updates based on recent performance data
    // For now, it's a placeholder for the concept
    Logger::debug("Auto-updating baselines");
}

PerformanceBaseline PerformanceRegressionDetector::calculateBaseline(
    const std::vector<puzzle71::monitoring::PerformanceMetrics>& metrics) {

    PerformanceBaseline baseline;
    baseline.created_at = getCurrentTime();
    baseline.expires_at = baseline.created_at + std::chrono::hours(24); // 24 hour expiry
    baseline.sample_count = metrics.size();

    if (metrics.empty()) {
        return baseline;
    }

    // Calculate throughput statistics
    std::vector<double> throughput_values;
    for (const auto& metric : metrics) {
        throughput_values.push_back(metric.keys_per_second);
    }

    if (!throughput_values.empty()) {
        std::sort(throughput_values.begin(), throughput_values.end());

        baseline.throughput_mean = std::accumulate(throughput_values.begin(), throughput_values.end(), 0.0) / throughput_values.size();

        baseline.throughput_median = throughput_values[throughput_values.size() / 2];
        baseline.throughput_p95 = throughput_values[static_cast<size_t>(throughput_values.size() * 0.95)];
        baseline.throughput_p99 = throughput_values[static_cast<size_t>(throughput_values.size() * 0.99)];

        // Calculate standard deviation
        double variance = 0.0;
        for (double value : throughput_values) {
            variance += (value - baseline.throughput_mean) * (value - baseline.throughput_mean);
        }
        baseline.throughput_stddev = std::sqrt(variance / throughput_values.size());
    }

    // Calculate latency statistics
    std::vector<double> latency_values;
    for (const auto& metric : metrics) {
        latency_values.push_back(metric.kernel_execution_time_ms);
    }

    if (!latency_values.empty()) {
        std::sort(latency_values.begin(), latency_values.end());

        baseline.latency_mean = std::accumulate(latency_values.begin(), latency_values.end(), 0.0) / latency_values.size();
        baseline.latency_p95 = latency_values[static_cast<size_t>(latency_values.size() * 0.95)];
        baseline.latency_p99 = latency_values[static_cast<size_t>(latency_values.size() * 0.99)];

        // Calculate standard deviation
        double variance = 0.0;
        for (double value : latency_values) {
            variance += (value - baseline.latency_mean) * (value - baseline.latency_mean);
        }
        baseline.latency_stddev = std::sqrt(variance / latency_values.size());
    }

    // Calculate system resource averages
    double gpu_util_sum = 0, mem_util_sum = 0, power_sum = 0, temp_sum = 0;
    for (const auto& metric : metrics) {
        gpu_util_sum += metric.gpu_utilization_percent;
        mem_util_sum += metric.memory_utilization_percent;
        power_sum += metric.power_usage_watts;
        temp_sum += metric.temperature_celsius;
    }

    baseline.gpu_utilization_mean = gpu_util_sum / metrics.size();
    baseline.memory_utilization_mean = mem_util_sum / metrics.size();
    baseline.power_usage_mean = power_sum / metrics.size();
    baseline.temperature_mean = temp_sum / metrics.size();

    // Set default validation thresholds
    baseline.acceptable_variance = config_.minimum_change_threshold;
    baseline.warning_threshold = config_.minimum_change_threshold * 2;
    baseline.error_threshold = config_.minimum_change_threshold * 4;
    baseline.critical_threshold = config_.minimum_change_threshold * 10;

    return baseline;
}

void PerformanceRegressionDetector::storeRegression(const RegressionResult& result) {
    std::lock_guard<std::mutex> lock(regressions_mutex_);

    regressions_[result.id] = result;
    regression_history_.push_back(result);

    // Keep history manageable
    if (regression_history_.size() > 1000) {
        regression_history_.erase(regression_history_.begin(),
                                regression_history_.begin() + (regression_history_.size() - 1000));
    }

    Logger::debug("Stored regression: {} - {} ({})",
                 result.metric_name, result.getSeverityString(), result.status);
}

void PerformanceRegressionDetector::updateRegressionStatus(const std::string& regression_id, RegressionStatus status) {
    std::lock_guard<std::mutex> lock(regressions_mutex_);

    auto it = regressions_.find(regression_id);
    if (it != regressions_.end()) {
        it->second.status = status;
    }
}

bool PerformanceRegressionDetector::shouldAlert(const RegressionResult& result) const {
    if (!config_.enable_alerts) {
        return false;
    }

    // Check severity threshold
    if (static_cast<int>(result.severity) < static_cast<int>(config_.minimum_alert_severity)) {
        return false;
    }

    // Check cooldown
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(alerts_mutex_);

    auto it = last_alert_times_.find(result.metric_name);
    if (it != last_alert_times_.end()) {
        auto time_since_last = now - it->second;
        if (time_since_last < config_.alert_cooldown) {
            return false;
        }
    }

    return true;
}

void PerformanceRegressionDetector::triggerAlert(const RegressionResult& result) {
    // Update last alert time
    {
        std::lock_guard<std::mutex> lock(alerts_mutex_);
        last_alert_times_[result.metric_name] = std::chrono::steady_clock::now();
    }

    // Call custom callback if provided
    if (alert_callback_) {
        alert_callback_(result);
    }

    // Log the alert
    Logger::warn("Regression detected: {} - {} ({}% change)",
                 result.metric_name, result.getSeverityString(), result.percentage_change);

    // TODO: Implement additional alert channels (email, Slack, etc.)
}

std::string PerformanceRegressionDetector::generateRegressionId() const {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);

    return "reg_" + std::to_string(timestamp) + "_" + std::to_string(dis(gen));
}

std::chrono::system_clock::time_point PerformanceRegressionDetector::getCurrentTime() const {
    return std::chrono::system_clock::now();
}

std::string PerformanceRegressionDetector::formatTimestamp(
    const std::chrono::system_clock::time_point& timestamp) const {

    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string PerformanceRegressionDetector::getPrimaryBaselineId(const std::string& metric_name) const {
    std::lock_guard<std::mutex> lock(baselines_mutex_);

    auto it = primary_baselines_.find(metric_name);
    if (it != primary_baselines_.end()) {
        return it->second;
    }

    return "";
}

std::string PerformanceRegressionDetector::getStatisticsToJson() const {
    auto stats = getStatistics();

    std::ostringstream json;
    json << std::fixed << std::setprecision(4);
    json << "{\n";
    json << "  \"total_detections\": " << stats.total_detections << ",\n";
    json << "  \"active_regressions\": " << stats.active_regressions << ",\n";
    json << "  \"resolved_regressions\": " << stats.resolved_regressions << ",\n";
    json << "  \"false_positive_rate\": " << stats.false_positive_rate << "\n";
    json << "}";

    return json.str();
}

// RegressionDetectorFactory implementation
std::unique_ptr<PerformanceRegressionDetector> RegressionDetectorFactory::create() {
    return std::make_unique<PerformanceRegressionDetector>(createDefaultConfig());
}

std::unique_ptr<PerformanceRegressionDetector> RegressionDetectorFactory::create(const RegressionDetectionConfig& config) {
    return std::make_unique<PerformanceRegressionDetector>(config);
}

std::unique_ptr<PerformanceRegressionDetector> RegressionDetectorFactory::createHighSensitivityDetector() {
    return std::make_unique<PerformanceRegressionDetector>(createHighSensitivityConfig());
}

std::unique_ptr<PerformanceRegressionDetector> RegressionDetectorFactory::createProductionDetector() {
    return std::make_unique<PerformanceRegressionDetector>(createProductionConfig());
}

std::unique_ptr<PerformanceRegressionDetector> RegressionDetectorFactory::createDevelopmentDetector() {
    return std::make_unique<PerformanceRegressionDetector>(createDevelopmentConfig());
}

RegressionDetectionConfig RegressionDetectorFactory::createDefaultConfig() {
    RegressionDetectionConfig config;
    config.min_sample_size = 30;
    config.significance_level = 0.05;
    config.minimum_change_threshold = 0.05;
    config.enable_statistical_tests = true;
    config.enable_alerts = true;
    config.minimum_alert_severity = RegressionSeverity::WARNING;
    return config;
}

RegressionDetectionConfig RegressionDetectorFactory::createHighSensitivityConfig() {
    auto config = createDefaultConfig();
    config.significance_level = 0.1;        // Less stringent
    config.minimum_change_threshold = 0.02; // 2% threshold
    config.minimum_alert_severity = RegressionSeverity::INFO;
    return config;
}

RegressionDetectionConfig RegressionDetectorFactory::createProductionConfig() {
    auto config = createDefaultConfig();
    config.significance_level = 0.01;       // More stringent
    config.minimum_change_threshold = 0.1;  // 10% threshold
    config.minimum_alert_severity = RegressionSeverity::ERROR;
    config.enable_multivariate_analysis = false; // Keep it simple for production
    return config;
}

RegressionDetectionConfig RegressionDetectorFactory::createDevelopmentConfig() {
    auto config = createDefaultConfig();
    config.minimum_change_threshold = 0.15; // 15% threshold (more forgiving)
    config.minimum_alert_severity = RegressionSeverity::WARNING;
    config.auto_update_baselines = true;
    return config;
}

// Regression utilities implementation
namespace regression_utils {

MetricsComparison compareMetrics(
    const puzzle71::monitoring::PerformanceMetrics& current,
    const PerformanceBaseline& baseline) {

    MetricsComparison comparison;

    // Calculate percentage changes
    if (baseline.throughput_mean > 0) {
        comparison.throughput_change = ((current.keys_per_second - baseline.throughput_mean) / baseline.throughput_mean) * 100.0;
    }

    if (baseline.latency_mean > 0) {
        comparison.latency_change = ((current.kernel_execution_time_ms - baseline.latency_mean) / baseline.latency_mean) * 100.0;
    }

    if (baseline.gpu_utilization_mean > 0) {
        comparison.utilization_change = ((current.gpu_utilization_percent - baseline.gpu_utilization_mean) / baseline.gpu_utilization_mean) * 100.0;
    }

    if (baseline.memory_utilization_mean > 0) {
        comparison.memory_change = ((current.memory_utilization_percent - baseline.memory_utilization_mean) / baseline.memory_utilization_mean) * 100.0;
    }

    // Determine if there's a regression
    const double regression_threshold = 10.0; // 10% threshold for regression

    bool throughput_regression = comparison.throughput_change < -regression_threshold;
    bool latency_regression = comparison.latency_change > regression_threshold;
    bool utilization_regression = comparison.utilization_change < -regression_threshold;
    bool memory_regression = comparison.memory_change > regression_threshold;

    comparison.has_regression = throughput_regression || latency_regression || utilization_regression || memory_regression;

    // Determine severity
    double max_change = std::max({
        std::abs(comparison.throughput_change),
        std::abs(comparison.latency_change),
        std::abs(comparison.utilization_change),
        std::abs(comparison.memory_change)
    });

    if (max_change >= 50.0) {
        comparison.max_severity = RegressionSeverity::CRITICAL;
    } else if (max_change >= 20.0) {
        comparison.max_severity = RegressionSeverity::ERROR;
    } else if (max_change >= 10.0) {
        comparison.max_severity = RegressionSeverity::WARNING;
    } else {
        comparison.max_severity = RegressionSeverity::INFO;
    }

    // Track regressed metrics
    if (throughput_regression) comparison.regressed_metrics.push_back("throughput");
    if (latency_regression) comparison.regressed_metrics.push_back("latency");
    if (utilization_regression) comparison.regressed_metrics.push_back("gpu_utilization");
    if (memory_regression) comparison.regressed_metrics.push_back("memory_utilization");

    return comparison;
}

TrendAnalysis analyzeTrend(
    const std::vector<std::pair<std::chrono::system_clock::time_point, double>>& data,
    std::chrono::seconds analysis_window) {

    TrendAnalysis analysis;
    analysis.period = analysis_window;

    if (data.empty()) {
        return analysis;
    }

    // Filter data within analysis window
    auto cutoff_time = std::chrono::system_clock::now() - analysis_window;
    std::vector<std::pair<std::chrono::system_clock::time_point, double>> filtered_data;

    for (const auto& [timestamp, value] : data) {
        if (timestamp >= cutoff_time) {
            filtered_data.push_back({timestamp, value});
        }
    }

    if (filtered_data.size() < 2) {
        return analysis;
    }

    // Extract values for trend calculation
    std::vector<double> values;
    for (const auto& [timestamp, value] : filtered_data) {
        values.push_back(value);
    }

    // Calculate trend using linear regression
    size_t n = values.size();
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;

    for (size_t i = 0; i < n; ++i) {
        double x = static_cast<double>(i);
        double y = values[i];
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }

    analysis.slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
    analysis.confidence = std::min(1.0, static_cast<double>(n) / 30.0); // More samples = higher confidence
    analysis.data_points = values;

    // Determine trend direction
    if (std::abs(analysis.slope) < 0.01) {
        analysis.direction = TrendDirection::STABLE;
    } else if (analysis.slope > 0) {
        analysis.direction = TrendDirection::IMPROVING;
    } else {
        analysis.direction = TrendDirection::DEGRADING;
    }

    return analysis;
}

BaselineValidation validateBaseline(
    const PerformanceBaseline& baseline,
    const RegressionDetectionConfig& config) {

    BaselineValidation validation;

    validation.sample_count = baseline.sample_count;
    validation.is_valid = baseline.isValid();

    if (!validation.is_valid) {
        validation.issues.push_back("Baseline does not meet minimum requirements");
        validation.recommendations.push_back("Collect more samples before creating baseline");
    }

    // Calculate coefficient of variation
    if (baseline.throughput_mean > 0) {
        validation.coefficient_of_variation = baseline.throughput_stddev / baseline.throughput_mean;

        if (validation.coefficient_of_variation > 0.3) {
            validation.issues.push_back("High variance in baseline data");
            validation.recommendations.push_back("Consider collecting more samples or investigating performance instability");
        }
    }

    // Calculate statistical power (simplified)
    double effect_size = 0.1; // 10% effect size
    double n = baseline.sample_count;
    validation.statistical_power = 1.0 - std::exp(-effect_size * effect_size * n / 8.0);

    if (validation.statistical_power < 0.8) {
        validation.issues.push_back("Low statistical power");
        validation.recommendations.push_back("Increase sample size for better detection capability");
    }

    return validation;
}

ImpactAssessment assessImpact(const RegressionResult& regression) {
    ImpactAssessment assessment;

    // Calculate overall performance impact based on percentage change
    assessment.performance_impact = std::abs(regression.percentage_change);

    // Estimate user experience impact (heuristic)
    if (regression.metric_name == "throughput") {
        assessment.user_experience_impact = assessment.performance_impact * 0.8;
    } else if (regression.metric_name == "latency") {
        assessment.user_experience_impact = assessment.performance_impact * 1.2;
    } else {
        assessment.user_experience_impact = assessment.performance_impact * 0.5;
    }

    // Estimate business impact (heuristic)
    assessment.business_impact = assessment.performance_impact * 0.6;

    // Estimate time to resolution based on severity
    switch (regression.severity) {
        case RegressionSeverity::INFO:
            assessment.time_to_resolution = std::chrono::hours(1);
            break;
        case RegressionSeverity::WARNING:
            assessment.time_to_resolution = std::chrono::hours(4);
            break;
        case RegressionSeverity::ERROR:
            assessment.time_to_resolution = std::chrono::hours(24);
            break;
        case RegressionSeverity::CRITICAL:
            assessment.time_to_resolution = std::chrono::hours(72);
            break;
    }

    // Add affected features
    assessment.affected_features.push_back(regression.metric_name);

    // Add recommended actions
    if (regression.severity >= RegressionSeverity::ERROR) {
        assessment.recommended_actions.push_back("Immediate investigation required");
        assessment.recommended_actions.push_back("Consider rollback if recent changes made");
    }

    if (regression.metric_name == "throughput") {
        assessment.recommended_actions.push_back("Check algorithm efficiency");
        assessment.recommended_actions.push_back("Review resource utilization");
    } else if (regression.metric_name == "latency") {
        assessment.recommended_actions.push_back("Profile kernel execution");
        assessment.recommended_actions.push_back("Check for memory access patterns");
    }

    return assessment;
}

std::vector<RegressionPattern> detectRegressionPatterns(
    const std::vector<RegressionResult>& regression_history) {

    std::vector<RegressionPattern> patterns;

    // Group regressions by metric name
    std::map<std::string, std::vector<RegressionResult>> metrics_regressions;
    for (const auto& regression : regression_history) {
        metrics_regressions[regression.metric_name].push_back(regression);
    }

    // Analyze each metric for patterns
    for (const auto& [metric_name, regressions] : metrics_regressions) {
        if (regressions.size() < 3) continue; // Need at least 3 occurrences

        RegressionPattern pattern;
        pattern.pattern_type = "recurrent_regression";
        pattern.frequency = static_cast<double>(regressions.size()) / regression_history.size();
        pattern.likely_cause = metric_name + " performance instability";

        // Calculate occurrence times
        for (const auto& regression : regressions) {
            pattern.occurrences.push_back(regression.detected_at);
        }

        // Calculate average impact
        double total_impact = 0.0;
        for (const auto& regression : regressions) {
            total_impact += std::abs(regression.percentage_change);
        }
        pattern.average_impacts[metric_name] = total_impact / regressions.size();

        patterns.push_back(pattern);
    }

    return patterns;
}

} // namespace regression_utils

} // namespace puzzle71::regression
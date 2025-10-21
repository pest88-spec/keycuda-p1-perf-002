// Puzzle71Solver - Performance Trend Analysis (T050)
// Phase 6: User Story 4 - Performance Monitoring
// Advanced trend analysis system with predictive analytics and anomaly detection

#include "performance_trend_analyzer.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <random>
#include <numeric>
#include <queue>

namespace puzzle71::trend {

// ============================================================================
// TrendDataPoint Implementation
// ============================================================================

double TrendDataPoint::getTimestamp() const {
    return std::chrono::duration<double>(timestamp.time_since_epoch()).count();
}

std::string TrendDataPoint::toJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"timestamp\":" << getTimestamp() << ","
        << "\"value\":" << value << ","
        << "\"metric_name\":\"" << metric_name << "\","
        << "\"weight\":" << weight << ","
        << "\"is_anomaly\":" << (is_anomaly ? "true" : "false") << ","
        << "\"anomaly_score\":" << anomaly_score;

    if (!context.empty()) {
        oss << ",\"context\":{";
        bool first = true;
        for (const auto& [key, value] : context) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":\"" << value << "\"";
            first = false;
        }
        oss << "}";
    }

    oss << "}";
    return oss.str();
}

TrendDataPoint TrendDataPoint::fromJson(const std::string& json) {
    // Simplified JSON parsing - in production, use a proper JSON library
    TrendDataPoint point;
    // TODO: Implement proper JSON parsing
    return point;
}

// ============================================================================
// TrendResult Implementation
// ============================================================================

std::string TrendResult::getDirectionString() const {
    switch (direction) {
        case TrendDirection::IMPROVING: return "improving";
        case TrendDirection::DEGRADING: return "degrading";
        case TrendDirection::STABLE: return "stable";
        case TrendDirection::VOLATILE: return "volatile";
        case TrendDirection::SEASONAL: return "seasonal";
        case TrendDirection::UNKNOWN: return "unknown";
        default: return "unknown";
    }
}

std::string TrendResult::getConfidenceString() const {
    switch (confidence) {
        case TrendConfidence::VERY_LOW: return "very_low";
        case TrendConfidence::LOW: return "low";
        case TrendConfidence::MEDIUM: return "medium";
        case TrendConfidence::HIGH: return "high";
        case TrendConfidence::VERY_HIGH: return "very_high";
        default: return "very_low";
    }
}

bool TrendResult::isStatisticallySignificant(double significance_level) const {
    return std::abs(correlation_coefficient) > significance_level;
}

std::string TrendResult::toJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"metric_name\":\"" << metric_name << "\","
        << "\"direction\":\"" << getDirectionString() << "\","
        << "\"confidence\":\"" << getConfidenceString() << "\","
        << "\"algorithm\":\"" << static_cast<int>(algorithm) << "\","
        << "\"slope\":" << slope << ","
        << "\"intercept\":" << intercept << ","
        << "\"correlation_coefficient\":" << correlation_coefficient << ","
        << "\"mean_squared_error\":" << mean_squared_error << ","
        << "\"mean_value\":" << mean_value << ","
        << "\"std_deviation\":" << std_deviation << ","
        << "\"next_value_prediction\":" << next_value_prediction << ","
        << "\"prediction_confidence\":" << prediction_confidence << ","
        << "\"anomaly_count\":" << anomaly_count << ","
        << "\"anomaly_percentage\":" << anomaly_percentage << ","
        << "\"data_point_count\":" << data_point_count
        << "}";
    return oss.str();
}

TrendResult TrendResult::fromJson(const std::string& json) {
    // Simplified JSON parsing - in production, use a proper JSON library
    TrendResult result;
    // TODO: Implement proper JSON parsing
    return result;
}

// ============================================================================
// AnomalyResult Implementation
// ============================================================================

bool AnomalyResult::isAnomaly(size_t point_index) const {
    if (point_index >= anomaly_scores.size()) return false;
    return anomaly_scores[point_index] > threshold;
}

double AnomalyResult::getAnomalySeverity(size_t point_index) const {
    if (point_index >= anomaly_scores.size()) return 0.0;
    return anomaly_scores[point_index];
}

std::string AnomalyResult::toJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"metric_name\":\"" << metric_name << "\","
        << "\"method\":\"" << static_cast<int>(method) << "\","
        << "\"threshold\":" << threshold << ","
        << "\"sensitivity\":" << sensitivity << ","
        << "\"total_points\":" << total_points << ","
        << "\"anomaly_points\":" << anomaly_points << ","
        << "\"anomaly_rate\":" << anomaly_rate << ","
        << "\"average_anomaly_score\":" << average_anomaly_score << ","
        << "\"anomaly_clusters\":" << anomaly_clusters
        << "}";
    return oss.str();
}

AnomalyResult AnomalyResult::fromJson(const std::string& json) {
    // Simplified JSON parsing - in production, use a proper JSON library
    AnomalyResult result;
    // TODO: Implement proper JSON parsing
    return result;
}

// ============================================================================
// SeasonalResult Implementation
// ============================================================================

double SeasonalResult::predictValue(std::chrono::system_clock::time_point time) const {
    if (!has_seasonality || seasonal_pattern.empty()) return 0.0;

    auto duration = std::chrono::duration_cast<std::chrono::seconds>(time - start_time);
    size_t period_pos = (duration.count() % season_period.count()) % seasonal_pattern.size();

    // Simple model: trend + seasonal
    double trend_value = trend_component.empty() ? 0.0 : trend_component.back();
    double seasonal_value = seasonal_pattern[period_pos];

    return trend_value + seasonal_value;
}

std::chrono::system_clock::time_point SeasonalResult::getNextPeak(
    std::chrono::system_clock::time_point from_time
) const {
    if (peak_times.empty()) return from_time;

    // Find first peak after from_time
    for (const auto& peak_time : peak_times) {
        if (peak_time > from_time) {
            return peak_time;
        }
    }

    // If no peak found, estimate next peak based on season period
    auto last_peak = peak_times.back();
    return last_peak + season_period;
}

std::string SeasonalResult::toJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"metric_name\":\"" << metric_name << "\","
        << "\"has_seasonality\":" << (has_seasonality ? "true" : "false") << ","
        << "\"season_period\":" << season_period.count() << ","
        << "\"seasonal_strength\":" << seasonal_strength << ","
        << "\"explained_variance\":" << explained_variance << ","
        << "\"peak_count\":" << peak_times.size() << ","
        << "\"valley_count\":" << valley_times.size()
        << "}";
    return oss.str();
}

SeasonalResult SeasonalResult::fromJson(const std::string& json) {
    // Simplified JSON parsing - in production, use a proper JSON library
    SeasonalResult result;
    // TODO: Implement proper JSON parsing
    return result;
}

// ============================================================================
// TrendAnalysisConfig Implementation
// ============================================================================

bool TrendAnalysisConfig::isValid() const {
    return min_data_points > 0 &&
           max_data_points > min_data_points &&
           data_retention.count() > 0 &&
           prediction_horizon > 0 &&
           prediction_confidence > 0.0 && prediction_confidence <= 1.0 &&
           significance_level > 0.0 && significance_level < 1.0 &&
           confidence_level > 0.0 && confidence_level < 1.0;
}

std::string TrendAnalysisConfig::toJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"min_data_points\":" << min_data_points << ","
        << "\"max_data_points\":" << max_data_points << ","
        << "\"data_retention\":" << data_retention.count() << ","
        << "\"primary_algorithm\":" << static_cast<int>(primary_algorithm) << ","
        << "\"significance_level\":" << significance_level << ","
        << "\"confidence_level\":" << confidence_level << ","
        << "\"outlier_threshold\":" << outlier_threshold << ","
        << "\"enable_seasonal_analysis\":" << (enable_seasonal_analysis ? "true" : "false") << ","
        << "\"enable_anomaly_detection\":" << (enable_anomaly_detection ? "true" : "false") << ","
        << "\"prediction_horizon\":" << prediction_horizon << ","
        << "\"prediction_confidence\":" << prediction_confidence << ","
        << "\"anomaly_method\":" << static_cast<int>(anomaly_method) << ","
        << "\"anomaly_sensitivity\":" << anomaly_sensitivity << ","
        << "\"enable_realtime_detection\":" << (enable_realtime_detection ? "true" : "false") << ","
        << "\"thread_count\":" << thread_count << ","
        << "\"enable_caching\":" << (enable_caching ? "true" : "false") << ","
        << "\"cache_ttl\":" << cache_ttl.count()
        << "}";
    return oss.str();
}

TrendAnalysisConfig TrendAnalysisConfig::fromJson(const std::string& json) {
    // Simplified JSON parsing - in production, use a proper JSON library
    TrendAnalysisConfig config;
    // TODO: Implement proper JSON parsing
    return config;
}

// ============================================================================
// LinearRegressionAnalyzer Implementation
// ============================================================================

TrendResult LinearRegressionAnalyzer::analyzeTrend(
    const std::vector<TrendDataPoint>& data,
    const TrendAnalysisConfig& config
) {
    TrendResult result;

    if (data.size() < config.min_data_points) {
        result.direction = TrendDirection::UNKNOWN;
        result.confidence = TrendConfidence::VERY_LOW;
        return result;
    }

    // Extract timestamps and values
    std::vector<double> x, y;
    std::transform(data.begin(), data.end(), std::back_inserter(x),
                   [](const TrendDataPoint& p) { return p.getTimestamp(); });
    std::transform(data.begin(), data.end(), std::back_inserter(y),
                   [](const TrendDataPoint& p) { return p.value; });

    // Calculate linear regression
    auto [slope, intercept] = calculateLinearRegression(x, y);
    result.slope = slope;
    result.intercept = intercept;

    // Calculate correlation coefficient
    result.correlation_coefficient = calculateCorrelation(x, y);

    // Calculate statistical measures
    result.mean_value = trend_utils::calculateMean(y);
    result.std_deviation = trend_utils::calculateStdDev(y);
    result.variance = result.std_deviation * result.std_deviation;

    // Calculate confidence intervals
    result.confidence_intervals = calculateConfidenceIntervals(x, y, config.confidence_level);

    // Determine trend direction
    if (std::abs(slope) < 0.01 * result.mean_value) {
        result.direction = TrendDirection::STABLE;
    } else if (slope > 0) {
        result.direction = TrendDirection::IMPROVING;
    } else {
        result.direction = TrendDirection::DEGRADING;
    }

    // Determine confidence level
    double r_squared = result.correlation_coefficient * result.correlation_coefficient;
    if (r_squared > 0.9) {
        result.confidence = TrendConfidence::VERY_HIGH;
    } else if (r_squared > 0.8) {
        result.confidence = TrendConfidence::HIGH;
    } else if (r_squared > 0.6) {
        result.confidence = TrendConfidence::MEDIUM;
    } else if (r_squared > 0.4) {
        result.confidence = TrendConfidence::LOW;
    } else {
        result.confidence = TrendConfidence::VERY_LOW;
    }

    // Set algorithm and data info
    result.algorithm = TrendAlgorithm::LINEAR_REGRESSION;
    result.start_time = data.front().timestamp;
    result.end_time = data.back().timestamp;
    result.data_point_count = data.size();

    // Calculate MSE
    double mse_sum = 0.0;
    for (size_t i = 0; i < x.size(); ++i) {
        double predicted = slope * x[i] + intercept;
        mse_sum += std::pow(y[i] - predicted, 2);
    }
    result.mean_squared_error = mse_sum / x.size();

    return result;
}

std::vector<double> LinearRegressionAnalyzer::predictFuture(
    const std::vector<TrendDataPoint>& data,
    size_t horizon,
    const TrendAnalysisConfig& config
) {
    if (data.empty()) return {};

    std::vector<double> x, y;
    std::transform(data.begin(), data.end(), std::back_inserter(x),
                   [](const TrendDataPoint& p) { return p.getTimestamp(); });
    std::transform(data.begin(), data.end(), std::back_inserter(y),
                   [](const TrendDataPoint& p) { return p.value; });

    auto [slope, intercept] = calculateLinearRegression(x, y);

    std::vector<double> predictions;
    double last_timestamp = x.back();
    double time_step = (x.back() - x.front()) / (x.size() - 1);

    for (size_t i = 1; i <= horizon; ++i) {
        double future_timestamp = last_timestamp + time_step * i;
        double prediction = slope * future_timestamp + intercept;
        predictions.push_back(prediction);
    }

    return predictions;
}

std::string LinearRegressionAnalyzer::getAlgorithmName() const {
    return "Linear Regression";
}

bool LinearRegressionAnalyzer::supportsPrediction() const {
    return true;
}

std::pair<double, double> LinearRegressionAnalyzer::calculateLinearRegression(
    const std::vector<double>& x, const std::vector<double>& y
) {
    if (x.size() != y.size() || x.empty()) return {0.0, 0.0};

    size_t n = x.size();
    double sum_x = std::accumulate(x.begin(), x.end(), 0.0);
    double sum_y = std::accumulate(y.begin(), y.end(), 0.0);
    double sum_xy = 0.0, sum_x2 = 0.0;

    for (size_t i = 0; i < n; ++i) {
        sum_xy += x[i] * y[i];
        sum_x2 += x[i] * x[i];
    }

    double denominator = n * sum_x2 - sum_x * sum_x;
    if (std::abs(denominator) < 1e-10) return {0.0, sum_y / n};

    double slope = (n * sum_xy - sum_x * sum_y) / denominator;
    double intercept = (sum_y - slope * sum_x) / n;

    return {slope, intercept};
}

double LinearRegressionAnalyzer::calculateCorrelation(
    const std::vector<double>& x, const std::vector<double>& y
) {
    if (x.size() != y.size() || x.size() < 2) return 0.0;

    size_t n = x.size();
    double mean_x = trend_utils::calculateMean(x);
    double mean_y = trend_utils::calculateMean(y);

    double sum_xy = 0.0, sum_x2 = 0.0, sum_y2 = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double dx = x[i] - mean_x;
        double dy = y[i] - mean_y;
        sum_xy += dx * dy;
        sum_x2 += dx * dx;
        sum_y2 += dy * dy;
    }

    double denominator = std::sqrt(sum_x2 * sum_y2);
    return denominator < 1e-10 ? 0.0 : sum_xy / denominator;
}

std::vector<double> LinearRegressionAnalyzer::calculateConfidenceIntervals(
    const std::vector<double>& x, const std::vector<double>& y, double confidence_level
) {
    // Simplified confidence interval calculation
    std::vector<double> intervals;
    if (x.size() < 3) return intervals;

    double mse = trend_utils::calculateRMSE(y, y) / std::sqrt(x.size());
    double t_value = 1.96; // Approximate for 95% confidence

    for (size_t i = 0; i < x.size(); ++i) {
        intervals.push_back(t_value * mse);
    }

    return intervals;
}

// ============================================================================
// ExponentialSmoothingAnalyzer Implementation
// ============================================================================

ExponentialSmoothingAnalyzer::ExponentialSmoothingAnalyzer(double alpha, double beta, double gamma)
    : alpha_(alpha), beta_(beta), gamma_(gamma) {}

TrendResult ExponentialSmoothingAnalyzer::analyzeTrend(
    const std::vector<TrendDataPoint>& data,
    const TrendAnalysisConfig& config
) {
    TrendResult result;

    if (data.size() < config.min_data_points) {
        result.direction = TrendDirection::UNKNOWN;
        result.confidence = TrendConfidence::VERY_LOW;
        return result;
    }

    std::vector<double> values;
    std::transform(data.begin(), data.end(), std::back_inserter(values),
                   [](const TrendDataPoint& p) { return p.value; });

    // Apply Holt-Winters exponential smoothing
    auto state = initializeHoltWinters(values, 24); // Assume 24-period seasonality
    auto smoothed = applyHoltWinters(values, state);

    // Calculate trend from smoothed values
    std::vector<double> timestamps;
    std::transform(data.begin(), data.end(), std::back_inserter(timestamps),
                   [](const TrendDataPoint& p) { return p.getTimestamp(); });

    auto [slope, intercept] = LinearRegressionAnalyzer::calculateLinearRegression(timestamps, smoothed);

    result.slope = slope;
    result.intercept = intercept;
    result.mean_value = trend_utils::calculateMean(values);
    result.std_deviation = trend_utils::calculateStdDev(values);
    result.algorithm = TrendAlgorithm::EXPONENTIAL_SMOOTHING;
    result.start_time = data.front().timestamp;
    result.end_time = data.back().timestamp;
    result.data_point_count = data.size();

    // Determine direction and confidence
    if (std::abs(slope) < 0.01 * result.mean_value) {
        result.direction = TrendDirection::STABLE;
    } else if (slope > 0) {
        result.direction = TrendDirection::IMPROVING;
    } else {
        result.direction = TrendDirection::DEGRADING;
    }

    result.confidence = TrendConfidence::MEDIUM; // Default confidence

    return result;
}

std::vector<double> ExponentialSmoothingAnalyzer::predictFuture(
    const std::vector<TrendDataPoint>& data,
    size_t horizon,
    const TrendAnalysisConfig& config
) {
    if (data.empty()) return {};

    std::vector<double> values;
    std::transform(data.begin(), data.end(), std::back_inserter(values),
                   [](const TrendDataPoint& p) { return p.value; });

    auto state = initializeHoltWinters(values, 24);
    std::vector<double> predictions;

    for (size_t i = 0; i < horizon; ++i) {
        double next_value = state.level + state.trend;
        if (!state.seasonal.empty()) {
            size_t season_idx = i % state.seasonal.size();
            next_value *= state.seasonal[season_idx];
        }
        predictions.push_back(next_value);

        // Update state for next prediction
        state.level = alpha_ * next_value + (1 - alpha_) * (state.level + state.trend);
        state.trend = beta_ * (state.level - state.level + state.trend) + (1 - beta_) * state.trend;
    }

    return predictions;
}

std::string ExponentialSmoothingAnalyzer::getAlgorithmName() const {
    return "Exponential Smoothing";
}

bool ExponentialSmoothingAnalyzer::supportsPrediction() const {
    return true;
}

void ExponentialSmoothingAnalyzer::setSmoothingParameters(double alpha, double beta, double gamma) {
    alpha_ = alpha;
    beta_ = beta;
    gamma_ = gamma;
}

ExponentialSmoothingAnalyzer::HoltWintersState ExponentialSmoothingAnalyzer::initializeHoltWinters(
    const std::vector<double>& data, size_t season_length
) {
    HoltWintersState state;

    if (data.empty()) return state;

    state.level = data[0];
    state.season_length = season_length;

    if (data.size() > season_length) {
        // Calculate initial trend
        state.trend = (data[season_length] - data[0]) / season_length;

        // Calculate initial seasonal components
        state.seasonal.resize(season_length);
        std::vector<double> season_averages(season_length, 0.0);

        for (size_t i = 0; i < data.size(); ++i) {
            size_t season_idx = i % season_length;
            season_averages[season_idx] += data[i];
        }

        for (size_t i = 0; i < season_length; ++i) {
            season_averages[i] /= (data.size() / season_length);
            state.seasonal[i] = season_averages[i] / state.level;
        }
    } else {
        state.trend = 0.0;
        state.seasonal.resize(season_length, 1.0);
    }

    return state;
}

std::vector<double> ExponentialSmoothingAnalyzer::applyHoltWinters(
    const std::vector<double>& data, const HoltWintersState& initial
) {
    std::vector<double> smoothed;
    if (data.empty()) return smoothed;

    HoltWintersState state = initial;

    for (size_t i = 0; i < data.size(); ++i) {
        // Update level
        double seasonal_factor = 1.0;
        if (!state.seasonal.empty()) {
            size_t season_idx = i % state.seasonal.size();
            seasonal_factor = state.seasonal[season_idx];
        }

        double new_level = alpha_ * (data[i] / seasonal_factor) + (1 - alpha_) * (state.level + state.trend);
        double new_trend = beta_ * (new_level - state.level) + (1 - beta_) * state.trend;

        // Update seasonal component
        if (!state.seasonal.empty()) {
            size_t season_idx = i % state.seasonal.size();
            double new_seasonal = gamma_ * (data[i] / new_level) + (1 - gamma_) * state.seasonal[season_idx];
            state.seasonal[season_idx] = new_seasonal;
        }

        state.level = new_level;
        state.trend = new_trend;

        smoothed.push_back(state.level + state.trend);
    }

    return smoothed;
}

// ============================================================================
// ARIMAAnalyzer Implementation
// ============================================================================

ARIMAAnalyzer::ARIMAAnalyzer(int p, int d, int q) : p_(p), d_(d), q_(q) {}

TrendResult ARIMAAnalyzer::analyzeTrend(
    const std::vector<TrendDataPoint>& data,
    const TrendAnalysisConfig& config
) {
    TrendResult result;

    if (data.size() < config.min_data_points) {
        result.direction = TrendDirection::UNKNOWN;
        result.confidence = TrendConfidence::VERY_LOW;
        return result;
    }

    std::vector<double> values;
    std::transform(data.begin(), data.end(), std::back_inserter(values),
                   [](const TrendDataPoint& p) { return p.value; });

    // Apply differencing to make series stationary
    std::vector<double> diff_values = difference(values, d_);

    // Fit AR model
    std::vector<double> ar_coeffs = autoRegressive(diff_values, p_);

    // Fit MA model
    std::vector<double> ma_coeffs = movingAverage(diff_values, q_);

    // Calculate predictions on original data
    std::vector<double> predictions;
    for (size_t i = 0; i < values.size(); ++i) {
        double pred = values[i]; // Simplified prediction
        predictions.push_back(pred);
    }

    // Calculate trend statistics
    std::vector<double> timestamps;
    std::transform(data.begin(), data.end(), std::back_inserter(timestamps),
                   [](const TrendDataPoint& p) { return p.getTimestamp(); });

    auto [slope, intercept] = LinearRegressionAnalyzer::calculateLinearRegression(timestamps, values);

    result.slope = slope;
    result.intercept = intercept;
    result.mean_value = trend_utils::calculateMean(values);
    result.std_deviation = trend_utils::calculateStdDev(values);
    result.algorithm = TrendAlgorithm::ARIMA;
    result.start_time = data.front().timestamp;
    result.end_time = data.back().timestamp;
    result.data_point_count = data.size();

    // Determine direction
    if (std::abs(slope) < 0.01 * result.mean_value) {
        result.direction = TrendDirection::STABLE;
    } else if (slope > 0) {
        result.direction = TrendDirection::IMPROVING;
    } else {
        result.direction = TrendDirection::DEGRADING;
    }

    result.confidence = TrendConfidence::HIGH; // ARIMA typically provides good confidence

    return result;
}

std::vector<double> ARIMAAnalyzer::predictFuture(
    const std::vector<TrendDataPoint>& data,
    size_t horizon,
    const TrendAnalysisConfig& config
) {
    if (data.empty()) return {};

    std::vector<double> values;
    std::transform(data.begin(), data.end(), std::back_inserter(values),
                   [](const TrendDataPoint& p) { return p.value; });

    // Simple ARIMA prediction (simplified implementation)
    std::vector<double> predictions;
    double last_value = values.back();
    double trend = values.size() > 1 ? (values.back() - values[values.size() - 2]) : 0.0;

    for (size_t i = 0; i < horizon; ++i) {
        double prediction = last_value + trend;
        predictions.push_back(prediction);
        last_value = prediction;
    }

    return predictions;
}

std::string ARIMAAnalyzer::getAlgorithmName() const {
    return "ARIMA";
}

bool ARIMAAnalyzer::supportsPrediction() const {
    return true;
}

void ARIMAAnalyzer::setARIMAOrder(int p, int d, int q) {
    p_ = p;
    d_ = d;
    q_ = q;
}

std::vector<double> ARIMAAnalyzer::difference(const std::vector<double>& data, int order) {
    if (order <= 0 || data.empty()) return data;

    std::vector<double> result = data;
    for (int d = 0; d < order; ++d) {
        std::vector<double> temp;
        for (size_t i = 1; i < result.size(); ++i) {
            temp.push_back(result[i] - result[i-1]);
        }
        result = temp;
        if (result.empty()) break;
    }

    return result;
}

std::vector<double> ARIMAAnalyzer::autoRegressive(const std::vector<double>& data, int order) {
    std::vector<double> coeffs(order, 0.0);
    if (data.size() <= static_cast<size_t>(order)) return coeffs;

    // Simplified AR coefficient calculation using Yule-Walker equations
    std::vector<double> autocov(order + 1);
    for (int lag = 0; lag <= order; ++lag) {
        double sum = 0.0;
        for (size_t i = lag; i < data.size(); ++i) {
            sum += data[i] * data[i - lag];
        }
        autocov[lag] = sum / data.size();
    }

    // Solve Yule-Walker equations (simplified)
    for (int i = 0; i < order; ++i) {
        if (std::abs(autocov[0]) > 1e-10) {
            coeffs[i] = autocov[i + 1] / autocov[0];
        }
    }

    return coeffs;
}

std::vector<double> ARIMAAnalyzer::movingAverage(const std::vector<double>& residuals, int order) {
    std::vector<double> coeffs(order, 0.0);
    if (residuals.size() <= static_cast<size_t>(order)) return coeffs;

    // Simple MA coefficient estimation
    for (int i = 0; i < order; ++i) {
        coeffs[i] = 1.0 / (order + 1);
    }

    return coeffs;
}

// ============================================================================
// ZScoreAnomalyDetector Implementation
// ============================================================================

ZScoreAnomalyDetector::ZScoreAnomalyDetector(double threshold) : threshold_(threshold) {}

AnomalyResult ZScoreAnomalyDetector::detectAnomalies(
    const std::vector<TrendDataPoint>& data,
    const TrendAnalysisConfig& config
) {
    AnomalyResult result;
    result.method = AnomalyMethod::STATISTICAL_ZSCORE;
    result.threshold = threshold_;
    result.total_points = data.size();

    if (data.empty()) return result;

    std::vector<double> values;
    std::transform(data.begin(), data.end(), std::back_inserter(values),
                   [](const TrendDataPoint& p) { return p.value; });

    // Calculate Z-scores
    std::vector<double> z_scores = calculateZScores(values);
    result.anomaly_scores = z_scores;

    // Identify anomalies
    std::vector<size_t> anomaly_indices;
    for (size_t i = 0; i < z_scores.size(); ++i) {
        if (std::abs(z_scores[i]) > threshold_) {
            anomaly_indices.push_back(i);
            result.anomalies.push_back(data[i]);
        }
    }

    result.anomaly_points = anomaly_indices.size();
    result.anomaly_rate = static_cast<double>(result.anomaly_points) / data.size();

    // Calculate average anomaly score
    if (!anomaly_indices.empty()) {
        double sum_scores = 0.0;
        for (size_t idx : anomaly_indices) {
            sum_scores += std::abs(z_scores[idx]);
        }
        result.average_anomaly_score = sum_scores / anomaly_indices.size();
    }

    // Cluster anomalies (simplified)
    if (!anomaly_indices.empty()) {
        result.anomaly_clusters = 1;
        result.cluster_indices.push_back(anomaly_indices);
        result.cluster_severity.push_back(result.average_anomaly_score);
    }

    return result;
}

std::string ZScoreAnomalyDetector::getDetectorName() const {
    return "Z-Score Anomaly Detector";
}

void ZScoreAnomalyDetector::setThreshold(double threshold) {
    threshold_ = threshold;
}

std::vector<double> ZScoreAnomalyDetector::calculateZScores(const std::vector<double>& values) {
    if (values.empty()) return {};

    double mean = trend_utils::calculateMean(values);
    double std_dev = trend_utils::calculateStdDev(values);

    std::vector<double> z_scores;
    for (double value : values) {
        double z_score = std_dev > 0.0 ? (value - mean) / std_dev : 0.0;
        z_scores.push_back(z_score);
    }

    return z_scores;
}

// ============================================================================
// IsolationForestAnomalyDetector Implementation
// ============================================================================

IsolationForestAnomalyDetector::IsolationForestAnomalyDetector(size_t n_estimators, double contamination)
    : n_estimators_(n_estimators), contamination_(contamination) {}

AnomalyResult IsolationForestAnomalyDetector::detectAnomalies(
    const std::vector<TrendDataPoint>& data,
    const TrendAnalysisConfig& config
) {
    AnomalyResult result;
    result.method = AnomalyMethod::ISOLATION_FOREST;
    result.total_points = data.size();

    if (data.empty()) return result;

    // Prepare data matrix
    std::vector<std::vector<double>> data_matrix;
    for (const auto& point : data) {
        data_matrix.push_back({point.getTimestamp(), point.value});
    }

    // Calculate anomaly scores
    std::vector<double> scores = calculateAnomalyScores(data_matrix);
    result.anomaly_scores = scores;

    // Determine threshold based on contamination rate
    std::vector<double> sorted_scores = scores;
    std::sort(sorted_scores.begin(), sorted_scores.end(), std::greater<double>());

    size_t anomaly_count = static_cast<size_t>(contamination_ * data.size());
    double threshold = anomaly_count > 0 ? sorted_scores[anomaly_count - 1] : 0.5;
    result.threshold = threshold;

    // Identify anomalies
    std::vector<size_t> anomaly_indices;
    for (size_t i = 0; i < scores.size(); ++i) {
        if (scores[i] > threshold) {
            anomaly_indices.push_back(i);
            result.anomalies.push_back(data[i]);
        }
    }

    result.anomaly_points = anomaly_indices.size();
    result.anomaly_rate = static_cast<double>(result.anomaly_points) / data.size();

    if (!anomaly_indices.empty()) {
        double sum_scores = 0.0;
        for (size_t idx : anomaly_indices) {
            sum_scores += scores[idx];
        }
        result.average_anomaly_score = sum_scores / anomaly_indices.size();
    }

    // Cluster anomalies (simplified)
    if (!anomaly_indices.empty()) {
        result.anomaly_clusters = 1;
        result.cluster_indices.push_back(anomaly_indices);
        result.cluster_severity.push_back(result.average_anomaly_score);
    }

    return result;
}

std::string IsolationForestAnomalyDetector::getDetectorName() const {
    return "Isolation Forest Anomaly Detector";
}

std::vector<double> IsolationForestAnomalyDetector::calculateAnomalyScores(
    const std::vector<std::vector<double>>& data
) {
    if (data.empty()) return {};

    std::vector<double> scores(data.size(), 0.0);

    // Build multiple isolation trees and average path lengths
    for (size_t tree_idx = 0; tree_idx < n_estimators_; ++tree_idx) {
        int max_depth = static_cast<int>(std::log2(data.size()));
        auto tree = buildIsolationTree(data, max_depth);

        for (size_t i = 0; i < data.size(); ++i) {
            double path_length = calculatePathLength(data[i], tree);
            scores[i] += path_length;
        }
    }

    // Average path lengths and convert to anomaly scores
    for (double& score : scores) {
        score /= n_estimators_;
        // Convert to anomaly score (higher means more anomalous)
        score = std::exp(-score / averagePathLength(data.size()));
    }

    return scores;
}

IsolationForestAnomalyDetector::IsolationTree IsolationForestAnomalyDetector::buildIsolationTree(
    const std::vector<std::vector<double>>& data, int max_depth
) {
    IsolationTree tree;

    // Simplified isolation tree construction
    std::queue<std::tuple<std::vector<size_t>, int, size_t>> queue;
    queue.push({std::vector<size_t>(data.size()), 0, 0}); // indices, depth, node_id

    while (!queue.empty()) {
        auto [indices, depth, node_id] = queue.front();
        queue.pop();

        if (depth >= max_depth || indices.size() <= 1) {
            // Leaf node
            tree.tree_depth.push_back(depth);
            continue;
        }

        // Random split
        size_t feature = 0; // Simplified: always use first feature
        std::vector<double> feature_values;
        for (size_t idx : indices) {
            feature_values.push_back(data[idx][feature]);
        }

        std::uniform_int_distribution<size_t> dist(0, feature_values.size() - 1);
        size_t split_idx = dist(rng_);
        double split_value = feature_values[split_idx];

        // Split data
        std::vector<size_t> left_indices, right_indices;
        for (size_t idx : indices) {
            if (data[idx][feature] < split_value) {
                left_indices.push_back(idx);
            } else {
                right_indices.push_back(idx);
            }
        }

        // Store split information
        tree.split_values.push_back(split_value);
        tree.split_features.push_back(static_cast<int>(feature));
        tree.tree_depth.push_back(depth);

        // Add children
        size_t left_child = tree.children.size();
        size_t right_child = left_child + 1;
        tree.children.push_back({left_child, right_child});

        queue.push({left_indices, depth + 1, left_child});
        queue.push({right_indices, depth + 1, right_child});
    }

    return tree;
}

double IsolationForestAnomalyDetector::calculatePathLength(
    const std::vector<double>& point, const IsolationTree& tree
) {
    // Simplified path length calculation
    size_t node_id = 0;
    size_t depth = 0;

    while (node_id < tree.tree_depth.size()) {
        depth = tree.tree_depth[node_id];

        if (node_id >= tree.split_features.size()) {
            break; // Leaf node
        }

        int feature = tree.split_features[node_id];
        double split_value = tree.split_values[node_id];

        if (feature < static_cast<int>(point.size()) && point[feature] < split_value) {
            node_id = tree.children[node_id][0];
        } else {
            node_id = tree.children[node_id][1];
        }
    }

    return static_cast<double>(depth);
}

double IsolationForestAnomalyDetector::averagePathLength(size_t n) {
    if (n <= 1) return 0.0;

    // Harmonic number approximation
    double H_n = 0.0;
    for (size_t i = 1; i <= n; ++i) {
        H_n += 1.0 / i;
    }

    return 2.0 * H_n - 1.0;
}

// ============================================================================
// SeasonalDecomposer Implementation
// ============================================================================

SeasonalDecomposer::SeasonalDecomposer(std::chrono::seconds season_period)
    : season_period_(season_period) {}

SeasonalResult SeasonalDecomposer::decompose(
    const std::vector<TrendDataPoint>& data,
    const TrendAnalysisConfig& config
) {
    SeasonalResult result;

    if (data.size() < 2 * season_period_.count()) {
        result.has_seasonality = false;
        return result;
    }

    std::vector<double> values;
    std::transform(data.begin(), data.end(), std::back_inserter(values),
                   [](const TrendDataPoint& p) { return p.value; });

    size_t period = season_period_.count();

    // Extract trend using moving average
    result.trend_component = extractTrend(values, period);

    // Extract seasonal component
    result.seasonal_component = extractSeasonal(values, result.trend_component, period);

    // Extract residual component
    result.residual_component = extractResidual(values, result.trend_component, result.seasonal_component);

    // Calculate variance components
    double total_variance = trend_utils::calculateVariance(values);
    double seasonal_variance = trend_utils::calculateVariance(result.seasonal_component);
    double trend_variance = trend_utils::calculateVariance(result.trend_component);
    double residual_variance = trend_utils::calculateVariance(result.residual_component);

    result.seasonal_strength = total_variance > 0.0 ? seasonal_variance / total_variance : 0.0;
    result.explained_variance = total_variance > 0.0 ? (seasonal_variance + trend_variance) / total_variance : 0.0;

    result.seasonal_variance = seasonal_variance;
    result.trend_variance = trend_variance;
    result.residual_variance = residual_variance;

    // Find peaks and valleys
    std::vector<size_t> peak_indices = findPeaks(result.seasonal_component);
    std::vector<size_t> valley_indices = findValleys(result.seasonal_component);

    result.has_seasonality = result.seasonal_strength > 0.1; // 10% threshold
    result.season_period = season_period_;

    // Convert indices to timestamps
    for (size_t idx : peak_indices) {
        if (idx < data.size()) {
            result.peak_times.push_back(data[idx].timestamp);
            result.peak_values.push_back(values[idx]);
        }
    }

    for (size_t idx : valley_indices) {
        if (idx < data.size()) {
            result.valley_times.push_back(data[idx].timestamp);
            result.valley_values.push_back(values[idx]);
        }
    }

    // Create seasonal pattern
    if (result.has_seasonality && !result.seasonal_component.empty()) {
        result.seasonal_pattern.resize(period);
        for (size_t i = 0; i < period; ++i) {
            double sum = 0.0;
            size_t count = 0;
            for (size_t j = i; j < result.seasonal_component.size(); j += period) {
                sum += result.seasonal_component[j];
                count++;
            }
            result.seasonal_pattern[i] = count > 0 ? sum / count : 0.0;
        }
    }

    return result;
}

void SeasonalDecomposer::setSeasonPeriod(std::chrono::seconds period) {
    season_period_ = period;
}

std::chrono::seconds SeasonalDecomposer::getSeasonPeriod() const {
    return season_period_;
}

std::vector<double> SeasonalDecomposer::extractTrend(const std::vector<double>& data, size_t window_size) {
    if (data.size() < window_size) return data;

    std::vector<double> trend;
    size_t half_window = window_size / 2;

    for (size_t i = 0; i < data.size(); ++i) {
        size_t start = (i >= half_window) ? i - half_window : 0;
        size_t end = std::min(i + half_window + 1, data.size());

        double sum = 0.0;
        for (size_t j = start; j < end; ++j) {
            sum += data[j];
        }
        trend.push_back(sum / (end - start));
    }

    return trend;
}

std::vector<double> SeasonalDecomposer::extractSeasonal(
    const std::vector<double>& data, const std::vector<double>& trend, size_t period
) {
    if (data.size() != trend.size() || data.size() < period) {
        return std::vector<double>(data.size(), 0.0);
    }

    std::vector<double> seasonal(data.size(), 0.0);
    std::vector<double> seasonal_sum(period, 0.0);
    std::vector<size_t> seasonal_count(period, 0);

    // Remove trend and calculate seasonal components
    for (size_t i = 0; i < data.size(); ++i) {
        double detrended = data[i] - trend[i];
        size_t season_idx = i % period;
        seasonal_sum[season_idx] += detrended;
        seasonal_count[season_idx]++;
    }

    // Average seasonal components
    for (size_t i = 0; i < period; ++i) {
        if (seasonal_count[i] > 0) {
            seasonal_sum[i] /= seasonal_count[i];
        }
    }

    // Apply seasonal components
    for (size_t i = 0; i < data.size(); ++i) {
        size_t season_idx = i % period;
        seasonal[i] = seasonal_sum[season_idx];
    }

    return seasonal;
}

std::vector<double> SeasonalDecomposer::extractResidual(
    const std::vector<double>& data,
    const std::vector<double>& trend,
    const std::vector<double>& seasonal
) {
    if (data.size() != trend.size() || data.size() != seasonal.size()) {
        return std::vector<double>(data.size(), 0.0);
    }

    std::vector<double> residual(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        residual[i] = data[i] - trend[i] - seasonal[i];
    }

    return residual;
}

std::vector<size_t> SeasonalDecomposer::findPeaks(const std::vector<double>& data) {
    std::vector<size_t> peaks;

    for (size_t i = 1; i < data.size() - 1; ++i) {
        if (data[i] > data[i-1] && data[i] > data[i+1]) {
            peaks.push_back(i);
        }
    }

    return peaks;
}

std::vector<size_t> SeasonalDecomposer::findValleys(const std::vector<double>& data) {
    std::vector<size_t> valleys;

    for (size_t i = 1; i < data.size() - 1; ++i) {
        if (data[i] < data[i-1] && data[i] < data[i+1]) {
            valleys.push_back(i);
        }
    }

    return valleys;
}

// ============================================================================
// Trend Analysis Utilities Implementation
// ============================================================================

namespace trend_utils {

std::vector<TrendDataPoint> removeOutliers(const std::vector<TrendDataPoint>& data, double threshold) {
    if (data.empty()) return data;

    std::vector<double> values;
    std::transform(data.begin(), data.end(), std::back_inserter(values),
                   [](const TrendDataPoint& p) { return p.value; });

    double mean = calculateMean(values);
    double std_dev = calculateStdDev(values);

    std::vector<TrendDataPoint> filtered;
    for (const auto& point : data) {
        double z_score = std_dev > 0.0 ? (point.value - mean) / std_dev : 0.0;
        if (std::abs(z_score) <= threshold) {
            filtered.push_back(point);
        }
    }

    return filtered;
}

std::vector<TrendDataPoint> smoothData(const std::vector<TrendDataPoint>& data, size_t window_size) {
    if (data.size() < window_size) return data;

    std::vector<TrendDataPoint> smoothed;
    size_t half_window = window_size / 2;

    for (size_t i = 0; i < data.size(); ++i) {
        size_t start = (i >= half_window) ? i - half_window : 0;
        size_t end = std::min(i + half_window + 1, data.size());

        double sum = 0.0;
        for (size_t j = start; j < end; ++j) {
            sum += data[j].value;
        }

        TrendDataPoint smoothed_point = data[i];
        smoothed_point.value = sum / (end - start);
        smoothed.push_back(smoothed_point);
    }

    return smoothed;
}

double calculateMean(const std::vector<double>& values) {
    if (values.empty()) return 0.0;

    double sum = std::accumulate(values.begin(), values.end(), 0.0);
    return sum / values.size();
}

double calculateStdDev(const std::vector<double>& values) {
    if (values.size() < 2) return 0.0;

    double mean = calculateMean(values);
    double sum_sq_diff = 0.0;

    for (double value : values) {
        double diff = value - mean;
        sum_sq_diff += diff * diff;
    }

    return std::sqrt(sum_sq_diff / (values.size() - 1));
}

double calculateVariance(const std::vector<double>& values) {
    double std_dev = calculateStdDev(values);
    return std_dev * std_dev;
}

double calculateCorrelation(const std::vector<double>& x, const std::vector<double>& y) {
    return LinearRegressionAnalyzer::calculateCorrelation(x, y);
}

double calculateR2(const std::vector<double>& observed, const std::vector<double>& predicted) {
    if (observed.size() != predicted.size() || observed.empty()) return 0.0;

    double obs_mean = calculateMean(observed);
    double ss_res = 0.0, ss_tot = 0.0;

    for (size_t i = 0; i < observed.size(); ++i) {
        double residual = observed[i] - predicted[i];
        double total_diff = observed[i] - obs_mean;
        ss_res += residual * residual;
        ss_tot += total_diff * total_diff;
    }

    return ss_tot > 0.0 ? 1.0 - (ss_res / ss_tot) : 0.0;
}

std::vector<double> calculateMovingAverage(const std::vector<double>& values, size_t window_size) {
    if (values.size() < window_size) return values;

    std::vector<double> moving_avg;
    double window_sum = 0.0;

    for (size_t i = 0; i < values.size(); ++i) {
        window_sum += values[i];

        if (i >= window_size) {
            window_sum -= values[i - window_size];
        }

        if (i >= window_size - 1) {
            moving_avg.push_back(window_sum / window_size);
        }
    }

    return moving_avg;
}

double calculateMAE(const std::vector<double>& actual, const std::vector<double>& predicted) {
    if (actual.size() != predicted.size() || actual.empty()) return 0.0;

    double sum_abs_error = 0.0;
    for (size_t i = 0; i < actual.size(); ++i) {
        sum_abs_error += std::abs(actual[i] - predicted[i]);
    }

    return sum_abs_error / actual.size();
}

double calculateRMSE(const std::vector<double>& actual, const std::vector<double>& predicted) {
    if (actual.size() != predicted.size() || actual.empty()) return 0.0;

    double sum_sq_error = 0.0;
    for (size_t i = 0; i < actual.size(); ++i) {
        double error = actual[i] - predicted[i];
        sum_sq_error += error * error;
    }

    return std::sqrt(sum_sq_error / actual.size());
}

} // namespace trend_utils

} // namespace puzzle71::trend
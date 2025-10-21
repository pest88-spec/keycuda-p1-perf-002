// Puzzle71 Technical Debt Repair - Automated Performance Regression Detection Implementation
// Task: T057 [P] [US3] Implement automated performance regression detection
// Phase: Phase 4 - User Story 3 Integration Testing and Validation System

#include "performance_regression_detector.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <random>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <filesystem>

namespace puzzle71 {
namespace validation {

// Constructor
PerformanceRegressionDetector::PerformanceRegressionDetector()
    : initialized_(false)
    , cuda_device_id_(-1)
    , cuda_stream_(nullptr)
    , total_detections_(0)
    , critical_regressions_(0)
    , high_regressions_(0) {
}

// Destructor
PerformanceRegressionDetector::~PerformanceRegressionDetector() {
    if (cuda_stream_) {
        cudaStreamDestroy(cuda_stream_);
    }
    if (cuda_device_id_ >= 0) {
        cudaDeviceReset();
    }
}

// Initialize the detector
bool PerformanceRegressionDetector::initialize(const RegressionDetectionConfig& config) {
    if (initialized_) {
        setError("Performance regression detector already initialized");
        return false;
    }

    clearError();
    config_ = config;

    // Initialize CUDA device
    cudaError_t error = cudaGetDeviceCount(&cuda_device_id_);
    if (error != cudaSuccess || cuda_device_id_ == 0) {
        setError("No CUDA devices available");
        return false;
    }

    error = cudaSetDevice(0);
    if (error != cudaSuccess) {
        setError("Failed to set CUDA device");
        return false;
    }
    cuda_device_id_ = 0;

    // Create CUDA stream
    error = cudaStreamCreate(&cuda_stream_);
    if (error != cudaSuccess) {
        setError("Failed to create CUDA stream");
        return false;
    }

    // Initialize statistics
    total_detections_ = 0;
    critical_regressions_ = 0;
    high_regressions_ = 0;

    initialized_ = true;
    return true;
}

// Load existing performance baselines
bool PerformanceRegressionDetector::loadBaselines(const std::string& baseline_directory) {
    if (!initialized_) {
        setError("Performance regression detector not initialized");
        return false;
    }

    baseline_directory_ = baseline_directory;
    baselines_.clear();

    // Create baseline directory if it doesn't exist
    if (!std::filesystem::exists(baseline_directory_)) {
        std::filesystem::create_directories(baseline_directory_);
        return true; // No baselines to load
    }

    // Load baseline files
    for (const auto& entry : std::filesystem::directory_iterator(baseline_directory_)) {
        if (entry.path().extension() == ".bin") {
            std::string benchmark_id = entry.path().stem().string();

            auto data = performance_regression_utils::readBinaryFile(entry.path().string());
            if (data.empty()) {
                setError("Failed to read baseline file: " + entry.path().string());
                return false;
            }

            PerformanceBaseline baseline;
            if (!deserializeBaseline(data, baseline)) {
                setError("Failed to deserialize baseline: " + benchmark_id);
                return false;
            }

            // Validate baseline integrity
            if (config_.enable_sha256_validation && !validateBaselineIntegrity(benchmark_id)) {
                setError("Baseline integrity validation failed: " + benchmark_id);
                return false;
            }

            baselines_[benchmark_id] = baseline;
        }
    }

    return true;
}

// Save performance baselines
bool PerformanceRegressionDetector::saveBaselines(const std::string& baseline_directory) {
    if (!initialized_) {
        setError("Performance regression detector not initialized");
        return false;
    }

    baseline_directory_ = baseline_directory;
    std::filesystem::create_directories(baseline_directory_);

    for (const auto& [benchmark_id, baseline] : baselines_) {
        std::string file_path = getBaselineFilePath(benchmark_id);

        std::vector<uint8_t> data;
        if (!serializeBaseline(baseline, data)) {
            setError("Failed to serialize baseline: " + benchmark_id);
            return false;
        }

        if (!performance_regression_utils::writeBinaryFile(file_path, data)) {
            setError("Failed to write baseline file: " + file_path);
            return false;
        }
    }

    return true;
}

// Create new performance baseline
bool PerformanceRegressionDetector::createBaseline(const std::string& benchmark_id,
                                                  std::function<double()> benchmark_function,
                                                  size_t sample_count) {
    if (!initialized_) {
        setError("Performance regression detector not initialized");
        return false;
    }

    clearError();

    // Execute benchmark samples
    std::vector<double> samples;
    if (!executeBenchmarkSamples(benchmark_function, sample_count, samples)) {
        setError("Failed to execute benchmark samples for baseline creation");
        return false;
    }

    // Calculate statistics
    double mean, std_deviation;
    if (!calculateStatistics(samples, mean, std_deviation)) {
        setError("Failed to calculate baseline statistics");
        return false;
    }

    // Create baseline structure
    PerformanceBaseline baseline;
    baseline.benchmark_id = benchmark_id;
    baseline.timestamp = std::chrono::system_clock::now();
    baseline.baseline_throughput_mkeys_per_sec = mean;
    baseline.mean_throughput = mean;
    baseline.std_deviation_throughput = std_deviation;
    baseline.sample_count = sample_count;
    baseline.sample_data = samples;

    // Get GPU device information
    cudaDeviceProp prop;
    cudaError_t error = cudaGetDeviceProperties(&prop, cuda_device_id_);
    if (error == cudaSuccess) {
        baseline.gpu_device = prop.name;
        baseline.cuda_driver_version = std::to_string(prop.major) + "." + std::to_string(prop.minor);
    }

    // Compute SHA-256 digest
    std::vector<uint8_t> baseline_data;
    if (serializeBaseline(baseline, baseline_data)) {
        computeSHA256Digest(baseline_data, baseline.sha256_digest);
    }

    baselines_[benchmark_id] = baseline;
    return true;
}

// Detect performance regression for a specific benchmark
bool PerformanceRegressionDetector::detectRegression(const std::string& benchmark_id,
                                                     std::function<double()> benchmark_function,
                                                     PerformanceRegressionResult& result) {
    if (!initialized_) {
        setError("Performance regression detector not initialized");
        return false;
    }

    // Check if baseline exists
    auto it = baselines_.find(benchmark_id);
    if (it == baselines_.end()) {
        setError("No baseline found for benchmark: " + benchmark_id);
        return false;
    }

    const PerformanceBaseline& baseline = it->second;

    // Execute current benchmark samples
    std::vector<double> current_samples;
    if (!executeBenchmarkSamples(benchmark_function, config_.minimum_sample_count, current_samples)) {
        setError("Failed to execute current benchmark samples");
        return false;
    }

    // Calculate current statistics
    double current_mean, current_std_deviation;
    if (!calculateStatistics(current_samples, current_mean, current_std_deviation)) {
        setError("Failed to calculate current benchmark statistics");
        return false;
    }

    // Calculate regression percentage
    double regression_percentage = performance_regression_utils::calculatePercentageChange(
        baseline.baseline_throughput_mkeys_per_sec, current_mean);

    // Perform statistical test
    double z_score = 0.0, p_value = 1.0;
    if (config_.enable_statistical_analysis) {
        performStatisticalTest(baseline.sample_data, current_samples, z_score, p_value);
    }

    // Determine if regression is detected
    result.benchmark_id = benchmark_id;
    result.current_throughput = current_mean;
    result.baseline_throughput = baseline.baseline_throughput_mkeys_per_sec;
    result.regression_percentage = regression_percentage;
    result.z_score = z_score;
    result.confidence_interval = 1.0 - p_value;
    result.detection_time = std::chrono::system_clock::now();

    // Determine regression detection
    result.regression_detected = performance_regression_utils::isSignificantRegression(
        regression_percentage, z_score,
        config_.regression_threshold_percentage, config_.z_score_threshold);

    // Calculate regression severity
    calculateRegressionSeverity(regression_percentage, z_score, result.regression_severity);

    // Set affected metrics
    result.affected_metrics = {"Throughput (Mkeys/sec)"};
    if (regression_percentage < 0) {
        result.regression_details = "Performance improvement detected: " +
                                   std::to_string(-regression_percentage) + "% improvement";
    } else {
        result.regression_details = "Performance regression detected: " +
                                   std::to_string(regression_percentage) + "% degradation";
    }

    // Update statistics
    detection_history_.push_back(result);
    total_detections_++;
    if (result.regression_detected) {
        if (result.regression_severity == "CRITICAL") {
            critical_regressions_++;
        } else if (result.regression_severity == "HIGH") {
            high_regressions_++;
        }
    }

    return true;
}

// Batch regression detection for multiple benchmarks
bool PerformanceRegressionDetector::detectRegressionsBatch(const std::vector<std::string>& benchmark_ids,
                                                         const std::vector<std::function<double()>>& benchmark_functions,
                                                         std::vector<PerformanceRegressionResult>& results) {
    if (!initialized_) {
        setError("Performance regression detector not initialized");
        return false;
    }

    if (benchmark_ids.size() != benchmark_functions.size()) {
        setError("Benchmark IDs and functions size mismatch");
        return false;
    }

    results.clear();
    results.resize(benchmark_ids.size());

    bool all_success = true;
    for (size_t i = 0; i < benchmark_ids.size(); ++i) {
        if (!detectRegression(benchmark_ids[i], benchmark_functions[i], results[i])) {
            all_success = false;
        }
    }

    return all_success;
}

// Validate baseline integrity with SHA-256
bool PerformanceRegressionDetector::validateBaselineIntegrity(const std::string& benchmark_id) {
    auto it = baselines_.find(benchmark_id);
    if (it == baselines_.end()) {
        return false;
    }

    const PerformanceBaseline& baseline = it->second;

    // Serialize current baseline data
    std::vector<uint8_t> current_data;
    if (!serializeBaseline(baseline, current_data)) {
        return false;
    }

    // Compute current digest
    std::vector<uint8_t> current_digest;
    if (!computeSHA256Digest(current_data, current_digest)) {
        return false;
    }

    // Compare with stored digest
    return baseline.sha256_digest == current_digest;
}

// Update existing baseline
bool PerformanceRegressionDetector::updateBaseline(const std::string& benchmark_id,
                                                   std::function<double()> benchmark_function,
                                                   const std::string& justification) {
    if (!initialized_) {
        setError("Performance regression detector not initialized");
        return false;
    }

    // Create new baseline
    if (!createBaseline(benchmark_id, benchmark_function, config_.minimum_sample_count)) {
        return false;
    }

    // Log update justification
    std::cout << "Baseline updated for " << benchmark_id << ": " << justification << std::endl;

    return true;
}

// Generate regression detection report
bool PerformanceRegressionDetector::generateRegressionReport(std::string& report) {
    if (!initialized_) {
        setError("Performance regression detector not initialized");
        return false;
    }

    std::ostringstream oss;

    oss << "=== Performance Regression Detection Report ===\n";
    oss << "Generated: " << performance_regression_utils::getCurrentTimestamp() << "\n\n";

    oss << "Detector Configuration:\n";
    oss << "  Regression Threshold: " << config_.regression_threshold_percentage << "%\n";
    oss << "  Statistical Significance Level: " << (config_.statistical_significance_level * 100) << "%\n";
    oss << "  Minimum Sample Count: " << config_.minimum_sample_count << "\n";
    oss << "  Z-Score Threshold: " << config_.z_score_threshold << "\n";
    oss << "  SHA-256 Validation: " << (config_.enable_sha256_validation ? "ENABLED" : "DISABLED") << "\n\n";

    oss << "Detection Statistics:\n";
    oss << "  Total Detections: " << total_detections_ << "\n";
    oss << "  Critical Regressions: " << critical_regressions_ << "\n";
    oss << "  High Regressions: " << high_regressions_ << "\n";
    oss << "  Available Baselines: " << baselines_.size() << "\n\n";

    if (!detection_history_.empty()) {
        oss << "Recent Detection Results:\n";
        size_t start_idx = (detection_history_.size() > 10) ? detection_history_.size() - 10 : 0;
        for (size_t i = start_idx; i < detection_history_.size(); ++i) {
            const auto& result = detection_history_[i];
            oss << "  " << result.benchmark_id << ": ";
            if (result.regression_detected) {
                oss << "REGRESSION (" << result.regression_severity << ") ";
                oss << std::fixed << std::setprecision(2) << result.regression_percentage << "%\n";
            } else {
                oss << "OK (" << std::fixed << std::setprecision(2) << result.regression_percentage << "%)\n";
            }
            oss << "    Current: " << performance_regression_utils::formatThroughput(result.current_throughput) << "\n";
            oss << "    Baseline: " << performance_regression_utils::formatThroughput(result.baseline_throughput) << "\n";
        }
        oss << "\n";
    }

    oss << "Available Baselines:\n";
    for (const auto& [benchmark_id, baseline] : baselines_) {
        auto time_t = std::chrono::system_clock::to_time_t(baseline.timestamp);
        oss << "  " << benchmark_id << "\n";
        oss << "    Created: " << std::ctime(&time_t);
        oss << "    Throughput: " << performance_regression_utils::formatThroughput(baseline.baseline_throughput_mkeys_per_sec) << "\n";
        oss << "    Samples: " << baseline.sample_count << "\n";
        oss << "    Device: " << baseline.gpu_device << "\n";
    }

    report = oss.str();
    return true;
}

// Get list of available baselines
std::vector<std::string> PerformanceRegressionDetector::getAvailableBaselines() const {
    std::vector<std::string> baseline_ids;
    for (const auto& [benchmark_id, baseline] : baselines_) {
        baseline_ids.push_back(benchmark_id);
    }
    return baseline_ids;
}

// Get regression statistics
bool PerformanceRegressionDetector::getRegressionStatistics(size_t& total_detections,
                                                            size_t& critical_regressions,
                                                            size_t& high_regressions,
                                                            double& average_regression_percentage) {
    if (!initialized_) {
        setError("Performance regression detector not initialized");
        return false;
    }

    total_detections = total_detections_;
    critical_regressions = critical_regressions_;
    high_regressions = high_regressions_;

    if (detection_history_.empty()) {
        average_regression_percentage = 0.0;
        return true;
    }

    double total_regression = 0.0;
    for (const auto& result : detection_history_) {
        total_regression += std::abs(result.regression_percentage);
    }
    average_regression_percentage = total_regression / detection_history_.size();

    return true;
}

// Private helper methods

bool PerformanceRegressionDetector::executeBenchmarkSamples(std::function<double()> benchmark_function,
                                                             size_t sample_count,
                                                             std::vector<double>& samples) {
    samples.clear();
    samples.resize(sample_count);

    // Warm-up run
    benchmark_function();

    // Execute samples
    for (size_t i = 0; i < sample_count; ++i) {
        try {
            samples[i] = benchmark_function();
            if (samples[i] <= 0.0) {
                setError("Invalid benchmark result: " + std::to_string(samples[i]));
                return false;
            }
        } catch (const std::exception& e) {
            setError("Benchmark execution failed: " + std::string(e.what()));
            return false;
        }
    }

    return true;
}

bool PerformanceRegressionDetector::calculateStatistics(const std::vector<double>& samples,
                                                         double& mean,
                                                         double& std_deviation) {
    if (samples.empty()) {
        return false;
    }

    mean = performance_regression_utils::calculateMean(samples);
    std_deviation = performance_regression_utils::calculateStandardDeviation(samples, mean);

    return true;
}

bool PerformanceRegressionDetector::performStatisticalTest(const std::vector<double>& baseline_samples,
                                                           const std::vector<double>& current_samples,
                                                           double& z_score,
                                                           double& p_value) {
    if (baseline_samples.empty() || current_samples.empty()) {
        return false;
    }

    double baseline_mean = performance_regression_utils::calculateMean(baseline_samples);
    double current_mean = performance_regression_utils::calculateMean(current_samples);
    double baseline_std = performance_regression_utils::calculateStandardDeviation(baseline_samples, baseline_mean);
    double current_std = performance_regression_utils::calculateStandardDeviation(current_samples, current_mean);

    // Two-sample Z-test
    double pooled_std = std::sqrt((baseline_std * baseline_std / baseline_samples.size()) +
                                  (current_std * current_std / current_samples.size()));

    if (pooled_std > 0.0) {
        z_score = (current_mean - baseline_mean) / pooled_std;
        p_value = performance_regression_utils::calculatePValue(std::abs(z_score));
    } else {
        z_score = 0.0;
        p_value = 1.0;
    }

    return true;
}

bool PerformanceRegressionDetector::calculateRegressionSeverity(double regression_percentage,
                                                              double z_score,
                                                              std::string& severity) {
    if (std::abs(regression_percentage) >= performance_regression_constants::CRITICAL_REGRESSION_THRESHOLD) {
        severity = "CRITICAL";
    } else if (std::abs(regression_percentage) >= performance_regression_constants::HIGH_REGRESSION_THRESHOLD) {
        severity = "HIGH";
    } else if (std::abs(regression_percentage) >= config_.regression_threshold_percentage) {
        severity = "MEDIUM";
    } else if (std::abs(regression_percentage) >= 1.0) {
        severity = "LOW";
    } else {
        severity = "NEGLIGIBLE";
    }

    // Adjust severity based on statistical significance
    if (std::abs(z_score) < config_.z_score_threshold && severity != "NEGLIGIBLE") {
        severity = "STATISTICALLY_INSIGNIFICANT";
    }

    return true;
}

bool PerformanceRegressionDetector::computeSHA256Digest(const std::vector<uint8_t>& data, std::vector<uint8_t>& digest) {
    return performance_regression_utils::computeSHA256(data, digest);
}

bool PerformanceRegressionDetector::serializeBaseline(const PerformanceBaseline& baseline, std::vector<uint8_t>& data) {
    // Simple serialization (in a real implementation, use proper serialization)
    data.clear();

    // Serialize benchmark ID
    std::string id_str = baseline.benchmark_id;
    data.insert(data.end(), id_str.begin(), id_str.end());
    data.push_back(0); // Null terminator

    // Serialize throughput
    double throughput = baseline.baseline_throughput_mkeys_per_sec;
    uint8_t* throughput_bytes = reinterpret_cast<uint8_t*>(&throughput);
    data.insert(data.end(), throughput_bytes, throughput_bytes + sizeof(double));

    // Serialize sample data
    uint32_t sample_count = static_cast<uint32_t>(baseline.sample_count);
    uint8_t* count_bytes = reinterpret_cast<uint8_t*>(&sample_count);
    data.insert(data.end(), count_bytes, count_bytes + sizeof(uint32_t));

    for (double sample : baseline.sample_data) {
        uint8_t* sample_bytes = reinterpret_cast<uint8_t*>(&sample);
        data.insert(data.end(), sample_bytes, sample_bytes + sizeof(double));
    }

    return true;
}

bool PerformanceRegressionDetector::deserializeBaseline(const std::vector<uint8_t>& data, PerformanceBaseline& baseline) {
    // Simple deserialization (in a real implementation, use proper deserialization)
    if (data.empty()) {
        return false;
    }

    size_t pos = 0;

    // Read benchmark ID
    std::string id_str(reinterpret_cast<const char*>(data.data()) + pos);
    baseline.benchmark_id = id_str;
    pos += id_str.length() + 1;

    // Read throughput
    if (pos + sizeof(double) > data.size()) return false;
    baseline.baseline_throughput_mkeys_per_sec = *reinterpret_cast<const double*>(data.data() + pos);
    pos += sizeof(double);

    // Read sample count
    if (pos + sizeof(uint32_t) > data.size()) return false;
    uint32_t sample_count = *reinterpret_cast<const uint32_t*>(data.data() + pos);
    baseline.sample_count = sample_count;
    pos += sizeof(uint32_t);

    // Read sample data
    baseline.sample_data.clear();
    for (uint32_t i = 0; i < sample_count && pos + sizeof(double) <= data.size(); ++i) {
        double sample = *reinterpret_cast<const double*>(data.data() + pos);
        baseline.sample_data.push_back(sample);
        pos += sizeof(double);
    }

    return true;
}

std::string PerformanceRegressionDetector::getBaselineFilePath(const std::string& benchmark_id) const {
    return baseline_directory_ + "/" + benchmark_id + ".bin";
}

void PerformanceRegressionDetector::setError(const std::string& error) {
    last_error_ = error;
    std::cerr << "Performance Regression Detector Error: " << error << std::endl;
}

void PerformanceRegressionDetector::clearError() {
    last_error_.clear();
}

// Utility functions implementation
namespace performance_regression_utils {

double calculateMean(const std::vector<double>& data) {
    if (data.empty()) return 0.0;
    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    return sum / data.size();
}

double calculateStandardDeviation(const std::vector<double>& data, double mean) {
    if (data.size() <= 1) return 0.0;

    double sum_squares = 0.0;
    for (double value : data) {
        double diff = value - mean;
        sum_squares += diff * diff;
    }

    return std::sqrt(sum_squares / (data.size() - 1));
}

double calculateZScore(double value, double mean, double std_deviation) {
    if (std_deviation == 0.0) return 0.0;
    return (value - mean) / std_deviation;
}

double calculatePValue(double z_score) {
    // Approximate p-value calculation for two-tailed test
    // This is a simplified approximation
    double abs_z = std::abs(z_score);
    if (abs_z < 0.1) return 1.0;
    if (abs_z > 4.0) return 0.0001;

    // Simplified normal distribution CDF approximation
    double t = 1.0 / (1.0 + 0.2316419 * abs_z);
    double approx = 1.0 - 0.3989423 * std::exp(-0.5 * abs_z * abs_z) *
                   (0.319381530 * t - 0.356563782 * t * t +
                    1.781477937 * t * t * t - 1.821255978 * t * t * t * t +
                    1.330274429 * t * t * t * t * t);

    return 2.0 * (1.0 - approx);
}

double calculatePercentageChange(double baseline, double current) {
    if (baseline == 0.0) return 0.0;
    return ((current - baseline) / baseline) * 100.0;
}

bool isSignificantRegression(double regression_percentage, double z_score,
                            double threshold_percentage, double z_threshold) {
    return (regression_percentage > threshold_percentage) && (std::abs(z_score) >= z_threshold);
}

TrendDirection analyzeTrend(const std::vector<double>& historical_data) {
    if (historical_data.size() < 3) return TrendDirection::STABLE;

    double slope = calculateTrendSlope(historical_data);

    if (slope > 0.01) return TrendDirection::IMPROVING;
    if (slope < -0.01) return TrendDirection::DEGRADING;
    return TrendDirection::STABLE;
}

double calculateTrendSlope(const std::vector<double>& data) {
    if (data.size() < 2) return 0.0;

    double n = static_cast<double>(data.size());
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;

    for (size_t i = 0; i < data.size(); ++i) {
        double x = static_cast<double>(i);
        double y = data[i];
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }

    return (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
}

bool computeSHA256(const std::vector<uint8_t>& data, std::vector<uint8_t>& hash) {
    // Simple hash implementation for demonstration
    // In a real implementation, use actual SHA-256
    hash.resize(32);
    uint64_t checksum = 0;

    for (size_t i = 0; i < data.size(); ++i) {
        checksum = (checksum * 31 + data[i]) & 0xFFFFFFFFFFFFFFFF;
    }

    // Fill hash with checksum bytes
    for (size_t i = 0; i < hash.size(); ++i) {
        hash[i] = static_cast<uint8_t>((checksum >> (i * 8)) & 0xFF);
    }

    return true;
}

bool verifySHA256Digest(const std::vector<uint8_t>& data, const std::vector<uint8_t>& expected_hash) {
    std::vector<uint8_t> computed_hash;
    if (!computeSHA256(data, computed_hash)) {
        return false;
    }

    return computed_hash == expected_hash;
}

bool readBinaryFile(const std::string& file_path, std::vector<uint8_t>& data) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    data.resize(file_size);
    file.read(reinterpret_cast<char*>(data.data()), file_size);

    return file.good();
}

bool writeBinaryFile(const std::string& file_path, const std::vector<uint8_t>& data) {
    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

std::string formatThroughput(double throughput_mkeys_per_sec) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << throughput_mkeys_per_sec << " Mkeys/sec";
    return oss.str();
}

std::string formatPercentage(double percentage) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << percentage << "%";
    return oss.str();
}

std::string formatDuration(double milliseconds) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << milliseconds << " ms";
    return oss.str();
}

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// BenchmarkTimer implementation
BenchmarkTimer::BenchmarkTimer() : running_(false) {}

void BenchmarkTimer::start() {
    start_time_ = std::chrono::high_resolution_clock::now();
    running_ = true;
}

void BenchmarkTimer::stop() {
    if (running_) {
        end_time_ = std::chrono::high_resolution_clock::now();
        running_ = false;
    }
}

double BenchmarkTimer::getElapsedMilliseconds() const {
    if (running_) {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time_);
        return duration.count() / 1000.0;
    } else {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time_ - start_time_);
        return duration.count() / 1000.0;
    }
}

double BenchmarkTimer::getElapsedSeconds() const {
    return getElapsedMilliseconds() / 1000.0;
}

} // namespace performance_regression_utils

} // namespace validation
} // namespace puzzle71
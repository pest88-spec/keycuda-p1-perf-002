/**
 * @file performance_monitor.h
 * @brief Performance monitoring and telemetry collection system
 *
 * This header defines the performance monitoring and telemetry collection
 * system for tracking GPU performance metrics during benchmark execution
 * and technical debt repair validation.
 *
 * Requirements Addressed:
 * - T017: Create base performance measurement and telemetry collection
 * - Performance benchmark API contract implementation
 * - Constitutional compliance validation for performance metrics
 *
 * @author Puzzle71 Technical Debt Repair System
 * @version 5.5
 * @date 2025-10-20
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

namespace puzzle71 {
namespace monitoring {

/**
 * @brief Performance metric types
 *
 * Corresponds to the benchmark types defined in the performance API contract.
 */
enum class MetricType {
    MEMORY_EFFICIENCY = 0,    ///< Memory access efficiency (percentage)
    GPU_UTILIZATION = 1,      ///< GPU utilization rate (percentage)
    THROUGHPUT = 2,           ///< Operations per second
    LATENCY = 3,              ///< Operation latency (microseconds)
    SYNCHRONIZATION_OVERHEAD = 4, ///< Sync overhead percentage
    OCCUPANCY = 5,            ///< SM occupancy percentage
    MEMORY_BANDWIDTH = 6,     ///< Memory bandwidth (GB/s)
    POWER_CONSUMPTION = 7     ///< Power consumption (watts)
};

/**
 * @brief Performance measurement unit
 */
enum class MetricUnit {
    PERCENTAGE = 0,           ///< Percentage (0-100%)
    OPERATIONS_PER_SECOND = 1, ///< Ops/s throughput
    MICROSECONDS = 2,         ///< Time in microseconds
    GIGABYTES_PER_SECOND = 3, ///< Memory bandwidth
    WATTS = 4,               ///< Power consumption
    COUNT = 5                ///< Raw count
};

/**
 * @brief Performance metric status
 */
enum class MetricStatus {
    PASS = 0,                ///< Metric meets or exceeds target
    WARNING = 1,             ///< Metric below target but acceptable
    FAIL = 2,                ///< Metric below acceptable threshold
    UNKNOWN = 3              ///< Metric not measured or invalid
};

/**
 * @brief Single performance measurement
 */
struct PerformanceMeasurement {
    MetricType type;
    MetricUnit unit;
    double value;
    double baseline_value;
    double target_threshold;
    MetricStatus status;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> metadata;

    /**
     * @brief Get variance from baseline as percentage
     * @return Variance percentage (positive = better, negative = worse)
     */
    double get_variance_percentage() const {
        if (baseline_value == 0.0) return 0.0;
        return ((value - baseline_value) / baseline_value) * 100.0;
    }

    /**
     * @brief Serialize measurement to JSON
     * @return JSON representation
     */
    nlohmann::json to_json() const;

    /**
     * @brief Load measurement from JSON
     * @param j JSON object
     * @return True if loading successful
     */
    bool from_json(const nlohmann::json& j);
};

/**
 * @brief Performance benchmark configuration
 */
struct BenchmarkConfiguration {
    std::string benchmark_type;
    int gpu_device_id;
    int test_duration_seconds;
    int warmup_iterations;
    int measurement_iterations;
    std::map<std::string, int> configuration;
    std::string baseline_id;
    std::map<std::string, double> performance_thresholds;

    /**
     * @brief Validate configuration
     * @return True if configuration is valid
     */
    bool is_valid() const;

    /**
     * @brief Serialize configuration to JSON
     * @return JSON representation
     */
    nlohmann::json to_json() const;

    /**
     * @brief Load configuration from JSON
     * @param j JSON object
     * @return True if loading successful
     */
    bool from_json(const nlohmann::json& j);
};

/**
 * @brief Benchmark execution session
 */
struct BenchmarkSession {
    std::string session_id;
    BenchmarkConfiguration config;
    std::chrono::system_clock::time_point started_at;
    std::chrono::system_clock::time_point completed_at;
    std::string status;
    std::vector<PerformanceMeasurement> measurements;
    std::map<std::string, double> detailed_metrics;
    std::string error_message;

    /**
     * @brief Calculate session duration
     * @return Duration in seconds
     */
    double get_duration_seconds() const {
        if (status == "running") {
            auto now = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - started_at);
            return duration.count();
        }
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(completed_at - started_at);
        return duration.count();
    }

    /**
     * @brief Get overall session status
     * @return Overall status based on measurements
     */
    MetricStatus get_overall_status() const;

    /**
     * @brief Serialize session to JSON
     * @return JSON representation
     */
    nlohmann::json to_json() const;

    /**
     * @brief Load session from JSON
     * @param j JSON object
     * @return True if loading successful
     */
    bool from_json(const nlohmann::json& j);
};

/**
 * @brief Performance monitor main class
 *
 * Provides comprehensive performance monitoring and telemetry collection
 * with real-time measurement, baseline comparison, and regression detection.
 */
class PerformanceMonitor {
public:
    /**
     * @brief Constructor
     * @param storage_dir Directory for storing performance data
     */
    explicit PerformanceMonitor(const std::string& storage_dir);

    /**
     * @brief Destructor
     */
    ~PerformanceMonitor();

    /**
     * @brief Start a new benchmark session
     * @param config Benchmark configuration
     * @return Session ID for tracking
     */
    std::string start_benchmark(const BenchmarkConfiguration& config);

    /**
     * @brief Record a performance measurement
     * @param session_id Session identifier
     * @param measurement Performance measurement to record
     * @return True if recording successful
     */
    bool record_measurement(const std::string& session_id,
                           const PerformanceMeasurement& measurement);

    /**
     * @brief Complete benchmark session
     * @param session_id Session identifier
     * @return True if completion successful
     */
    bool complete_benchmark(const std::string& session_id);

    /**
     * @brief Get benchmark session
     * @param session_id Session identifier
     * @return Pointer to session or nullptr if not found
     */
    const BenchmarkSession* get_session(const std::string& session_id) const;

    /**
     * @brief Get all benchmark sessions
     * @return List of all sessions
     */
    std::vector<BenchmarkSession> get_all_sessions() const;

    /**
     * @brief Get active benchmark sessions
     * @return List of active sessions
     */
    std::vector<BenchmarkSession> get_active_sessions() const;

    /**
     * @brief Measure current GPU metrics
     * @param gpu_device_id GPU device ID
     * @return Map of metric types to current values
     */
    std::map<MetricType, double> measure_current_metrics(int gpu_device_id);

    /**
     * @brief Compare session against baseline
     * @param session_id Current session ID
     * @param baseline_session_id Baseline session ID
     * @param threshold_percentage Comparison threshold
     * @return Comparison result as JSON
     */
    nlohmann::json compare_against_baseline(const std::string& session_id,
                                           const std::string& baseline_session_id,
                                           double threshold_percentage = 5.0);

    /**
     * @brief Detect performance regressions
     * @param session_id Session to analyze
     * @return List of detected regressions
     */
    std::vector<std::string> detect_regressions(const std::string& session_id);

    /**
     * @brief Generate performance report
     * @param session_id Session ID
     * @param format Report format (json, markdown, csv)
     * @return Formatted report string
     */
    std::string generate_report(const std::string& session_id,
                               const std::string& format = "json") const;

    /**
     * @brief Export performance data
     * @param output_file Output file path
     * @param session_ids List of session IDs to export (empty = all)
     * @param format Export format
     * @return True if export successful
     */
    bool export_data(const std::string& output_file,
                    const std::vector<std::string>& session_ids = {},
                    const std::string& format = "json") const;

    /**
     * @brief Import performance data
     * @param input_file Input file path
     * @param format Import format
     * @return True if import successful
     */
    bool import_data(const std::string& input_file,
                    const std::string& format = "json");

    /**
     * @brief Get performance trends over time
     * @param metric_type Metric type to analyze
     * @param days Number of days to analyze
     * @return Trend analysis data
     */
    std::map<std::string, double> get_performance_trends(MetricType metric_type,
                                                         int days = 30) const;

    /**
     * @brief Validate performance against constitutional requirements
     * @param session_id Session ID to validate
     * @return Validation result with specific violations
     */
    std::map<std::string, bool> validate_constitutional_compliance(
        const std::string& session_id) const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

    /**
     * @brief Get file path for performance data
     * @param filename Base filename
     * @return Full file path
     */
    std::string get_data_path(const std::string& filename) const;

    /**
     * @brief Ensure storage directory exists
     */
    void ensure_storage_directory() const;

    /**
     * @brief Generate unique session ID
     * @return Unique session identifier
     */
    std::string generate_session_id() const;

    /**
     * @brief Convert metric type to string
     * @param type Metric type
     * @return String representation
     */
    std::string metric_type_to_string(MetricType type) const;

    /**
     * @brief Convert string to metric type
     * @param str Metric type string
     * @return Metric type enum value
     */
    MetricType string_to_metric_type(const std::string& str) const;

    /**
     * @brief Measure GPU memory efficiency
     * @param gpu_device_id GPU device ID
     * @return Memory efficiency percentage
     */
    double measure_memory_efficiency(int gpu_device_id);

    /**
     * @brief Measure GPU utilization
     * @param gpu_device_id GPU device ID
     * @return GPU utilization percentage
     */
    double measure_gpu_utilization(int gpu_device_id);

    /**
     * @brief Measure throughput for specific operation
     * @param operation_type Type of operation
     * @param duration_ms Measurement duration in milliseconds
     * @return Throughput in operations per second
     */
    double measure_throughput(const std::string& operation_type, int duration_ms);

    /**
     * @brief Measure synchronization overhead
     * @param gpu_device_id GPU device ID
     * @return Synchronization overhead percentage
     */
    double measure_synchronization_overhead(int gpu_device_id);
};

/**
 * @brief RAII helper for benchmark sessions
 *
 * Automatically manages benchmark session lifecycle within a scope.
 */
class BenchmarkSessionGuard {
public:
    /**
     * @brief Constructor - starts benchmark session
     * @param monitor Monitor instance
     * @param config Benchmark configuration
     */
    BenchmarkSessionGuard(PerformanceMonitor& monitor,
                         const BenchmarkConfiguration& config);

    /**
     * @brief Destructor - completes session and records final metrics
     */
    ~BenchmarkSessionGuard();

    /**
     * @brief Get session ID
     * @return Session ID
     */
    const std::string& session_id() const { return session_id_; }

    /**
     * @brief Record a measurement
     * @param measurement Performance measurement
     * @return True if recording successful
     */
    bool record_measurement(const PerformanceMeasurement& measurement);

    /**
     * @brief Get session summary
     * @return Session summary
     */
    std::string get_summary() const;

private:
    PerformanceMonitor& monitor_;
    std::string session_id_;
    bool session_active_;
};

/**
 * @brief Performance measurement utilities
 */
namespace performance_utils {
    /**
     * @brief Get default performance thresholds
     * @return Map of metric types to threshold values
     */
    std::map<MetricType, double> get_default_thresholds();

    /**
     * @brief Validate measurement against thresholds
     * @param measurement Performance measurement
     * @param thresholds Threshold values to validate against
     * @return Validation status
     */
    MetricStatus validate_measurement(const PerformanceMeasurement& measurement,
                                     const std::map<MetricType, double>& thresholds);

    /**
     * @brief Calculate statistical summary
     * @param measurements List of measurements
     * @return Statistical summary (mean, std dev, min, max)
     */
    std::map<std::string, double> calculate_statistics(
        const std::vector<PerformanceMeasurement>& measurements);

    /**
     * @brief Detect outliers in measurements
     * @param measurements List of measurements
     * @param threshold Standard deviation threshold for outlier detection
     * @return Indices of outlier measurements
     */
    std::vector<size_t> detect_outliers(
        const std::vector<PerformanceMeasurement>& measurements,
        double threshold = 2.0);
}

} // namespace monitoring
} // namespace puzzle71
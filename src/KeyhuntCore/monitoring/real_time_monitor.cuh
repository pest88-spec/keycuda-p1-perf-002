// Puzzle71Solver - Real-Time Performance Monitoring System (T045)
// Phase 6: User Story 4 - Performance Monitoring
// Comprehensive real-time telemetry collection and performance monitoring

#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>
#include <functional>

#include <cuda_runtime.h>
#include "core/uint256.h"

namespace puzzle71::monitoring {

/**
 * @brief Real-time performance metrics structure
 *
 * Contains all performance metrics collected in real-time during execution
 * for monitoring, optimization, and alerting purposes.
 */
struct PerformanceMetrics {
    // Timestamps
    std::chrono::high_resolution_clock::time_point timestamp;
    std::chrono::duration<double, std::milli> execution_time_ms;

    // Throughput metrics
    double keys_per_second{0.0};
    double candidates_per_second{0.0};
    double batches_per_second{0.0};
    std::uint64_t total_keys_processed{0};
    std::uint64_t total_candidates_found{0};

    // GPU utilization metrics
    double gpu_utilization_percent{0.0};
    double memory_utilization_percent{0.0};
    std::uint64_t memory_used_bytes{0};
    std::uint64_t memory_total_bytes{0};
    double memory_bandwidth_utilization_percent{0.0};

    // Temperature and power metrics
    double temperature_celsius{0.0};
    double power_usage_watts{0.0};
    double power_limit_watts{0.0};

    // Kernel execution metrics
    std::string kernel_name;
    std::uint32_t grid_size{0};
    std::uint32_t block_size{0};
    std::uint32_t points_per_thread{0};
    double kernel_execution_time_ms{0.0};

    // Error and warning metrics
    std::uint32_t cuda_errors{0};
    std::uint32_t kernel_launch_failures{0};
    std::uint32_t memory_allocation_failures{0};
    std::vector<std::string> warning_messages;

    // Efficiency metrics
    double occupancy_ratio{0.0};
    double memory_efficiency{0.0};
    double compute_efficiency{0.0};

    /**
     * @brief Serialize metrics to JSON
     */
    std::string toJson() const;

    /**
     * @brief Deserialize metrics from JSON
     */
    static PerformanceMetrics fromJson(const std::string& json);

    /**
     * @brief Calculate performance score (0-100)
     */
    double calculatePerformanceScore() const;

    /**
     * @brief Check if metrics indicate performance issues
     */
    bool hasPerformanceIssues() const;
};

/**
 * @brief Performance alert configuration
 */
struct AlertConfiguration {
    enum class Level {
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    Level level{Level::INFO};
    std::string message_template;
    std::function<bool(const PerformanceMetrics&)> condition;
    std::chrono::seconds cooldown{std::chrono::seconds(60)};

    // Alert throttling
    std::chrono::steady_clock::time_point last_triggered;
    std::uint32_t trigger_count{0};
    std::uint32_t max_triggers_per_hour{10};
};

/**
 * @brief Real-time monitoring configuration
 */
struct MonitoringConfiguration {
    // Collection settings
    std::chrono::milliseconds collection_interval{std::chrono::milliseconds(100)};
    std::chrono::seconds retention_period{std::chrono::seconds(3600)};
    size_t max_metrics_history{10000};

    // GPU monitoring
    bool enable_gpu_monitoring{true};
    bool enable_temperature_monitoring{true};
    bool enable_power_monitoring{true};
    bool enable_memory_monitoring{true};

    // Performance thresholds
    double min_throughput_threshold{1000000.0};  // 1M keys/s
    double max_temperature_threshold{85.0};      // 85°C
    double max_memory_utilization_threshold{95.0}; // 95%
    double min_gpu_utilization_threshold{50.0};   // 50%

    // Alert settings
    bool enable_alerts{true};
    bool enable_email_alerts{false};
    bool enable_log_alerts{true};
    std::string alert_log_file{"performance_alerts.log"};

    // Export settings
    bool enable_json_export{true};
    bool enable_csv_export{false};
    bool enable_prometheus_export{false};
    std::chrono::seconds export_interval{std::chrono::seconds(60)};
    std::string export_directory{"telemetry"};
};

/**
 * @brief Real-time performance monitor
 *
 * Provides comprehensive real-time monitoring of GPU performance metrics,
 * including throughput, utilization, temperature, power consumption, and
 * automated alerting for performance issues.
 */
class RealTimeMonitor {
public:
    explicit RealTimeMonitor(int device_id,
                           const MonitoringConfiguration& config = MonitoringConfiguration{});
    ~RealTimeMonitor();

    // Core monitoring operations
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const { return monitoring_active_.load(); }

    // Metrics collection
    PerformanceMetrics getCurrentMetrics() const;
    std::vector<PerformanceMetrics> getMetricsHistory(std::chrono::seconds duration) const;
    std::vector<PerformanceMetrics> getMetricsHistory(size_t count) const;

    // Alert management
    void addAlert(std::shared_ptr<AlertConfiguration> alert);
    void removeAlert(const std::string& alert_id);
    void clearAlerts();
    std::vector<std::string> getActiveAlerts() const;

    // Manual metric recording
    void recordKernelExecution(const std::string& kernel_name,
                             std::uint32_t grid_size,
                             std::uint32_t block_size,
                             std::uint32_t points_per_thread,
                             std::chrono::duration<double, std::milli> execution_time);

    void recordThroughputMetrics(std::uint64_t keys_processed,
                                std::uint64_t candidates_found,
                                std::chrono::duration<double, std::milli> duration);

    void recordError(const std::string& error_type, const std::string& message);

    // Export and reporting
    bool exportToJson(const std::string& filename) const;
    bool exportToCsv(const std::string& filename) const;
    std::string generatePerformanceReport() const;

    // Statistics and analysis
    struct Statistics {
        double average_throughput{0.0};
        double peak_throughput{0.0};
        double average_gpu_utilization{0.0};
        double peak_temperature{0.0};
        std::chrono::duration<double, std::milli> total_execution_time{0};
        std::uint64_t total_errors{0};
        std::uint64_t total_alerts_triggered{0};
    };

    Statistics getStatistics() const;

    // Configuration management
    void updateConfiguration(const MonitoringConfiguration& config);
    MonitoringConfiguration getConfiguration() const { return config_; }

    // Health check
    bool isHealthy() const;
    std::vector<std::string> getHealthIssues() const;

private:
    int device_id_;
    MonitoringConfiguration config_;
    std::atomic<bool> monitoring_active_{false};
    std::unique_ptr<std::thread> monitoring_thread_;

    // Metrics storage
    mutable std::mutex metrics_mutex_;
    std::vector<PerformanceMetrics> metrics_history_;
    PerformanceMetrics current_metrics_;

    // Alert system
    mutable std::mutex alerts_mutex_;
    std::unordered_map<std::string, std::shared_ptr<AlertConfiguration>> alerts_;
    std::vector<std::string> active_alerts_;

    // Synchronization
    mutable std::mutex state_mutex_;
    std::condition_variable stop_condition_;

    // GPU monitoring
    cudaDeviceProp device_properties_;
    std::chrono::steady_clock::time_point start_time_;

    // Private methods
    void monitoringLoop();
    void collectMetrics();
    void processAlerts();
    void cleanupOldMetrics();

    // GPU monitoring helpers
    double getGpuUtilization() const;
    double getMemoryUtilization() const;
    double getTemperature() const;
    double getPowerUsage() const;
    double getMemoryBandwidthUtilization() const;

    // Alert helpers
    void triggerAlert(const std::string& alert_id, const PerformanceMetrics& metrics);
    bool shouldTriggerAlert(const AlertConfiguration& alert) const;

    // Export helpers
    void exportMetrics() const;
    void exportToJsonInternal() const;
    void exportToCsvInternal() const;
    void exportToPrometheusInternal() const;

    // Statistics helpers
    Statistics calculateStatistics() const;
    void updateStatistics(const PerformanceMetrics& metrics);

    // Built-in alerts
    void setupDefaultAlerts();
    void createLowThroughputAlert();
    void createHighTemperatureAlert();
    void createHighMemoryUtilizationAlert();
    void createLowGpuUtilizationAlert();
    void createErrorRateAlert();
};

/**
 * @brief Factory for creating real-time monitors
 */
class MonitorFactory {
public:
    /**
     * @brief Create monitor for specific GPU device
     */
    static std::unique_ptr<RealTimeMonitor> create(int device_id);

    /**
     * @brief Create monitor with custom configuration
     */
    static std::unique_ptr<RealTimeMonitor> create(
        int device_id,
        const MonitoringConfiguration& config
    );

    /**
     * @brief Create high-performance monitoring configuration
     */
    static MonitoringConfiguration createHighPerformanceConfig();

    /**
     * @brief Create development monitoring configuration
     */
    static MonitoringConfiguration createDevelopmentConfig();

    /**
     * @brief Create production monitoring configuration
     */
    static MonitoringConfiguration createProductionConfig();
};

/**
 * @brief Performance monitoring utilities
 */
namespace monitoring_utils {

/**
 * @brief Convert performance score to grade
 */
std::string scoreToGrade(double score);

/**
 * @brief Format throughput for display
 */
std::string formatThroughput(double keys_per_second);

/**
 * @brief Format memory size for display
 */
std::string formatMemorySize(std::uint64_t bytes);

/**
 * @brief Format duration for display
 */
std::string formatDuration(std::chrono::duration<double, std::milli> duration);

/**
 * @brief Calculate performance trend
 */
enum class Trend {
    IMPROVING,
    STABLE,
    DEGRADING
};

Trend calculateTrend(const std::vector<PerformanceMetrics>& metrics);

/**
 * @brief Detect performance anomalies
 */
struct AnomalyDetection {
    bool is_anomaly{false};
    double anomaly_score{0.0};
    std::string anomaly_type;
    std::string description;
};

AnomalyDetection detectAnomalies(const std::vector<PerformanceMetrics>& metrics);

/**
 * @brief Generate performance recommendations
 */
std::vector<std::string> generateRecommendations(const PerformanceMetrics& metrics);

/**
 * @brief Compare performance against baseline
 */
struct BaselineComparison {
    double throughput_ratio{1.0};
    double efficiency_ratio{1.0};
    bool meets_baseline{true};
    std::vector<std::string> issues;
};

BaselineComparison compareWithBaseline(const PerformanceMetrics& current,
                                     const PerformanceMetrics& baseline);

} // namespace monitoring_utils

} // namespace puzzle71::monitoring
// Puzzle71Solver - Automated Performance Benchmarking (T051)
// Phase 6: User Story 4 - Performance Monitoring
// Comprehensive automated benchmarking system with CI integration and regression detection

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
#include <future>

#include "monitoring/real_time_monitor.h"
#include "regression/performance_regression_detector.h"

namespace puzzle71::benchmark {

/**
 * @brief Benchmark execution modes
 */
enum class BenchmarkMode {
    SCHEDULED,          // Run on schedule
    ON_DEMAND,          // Run when requested
    CONTINUOUS,         // Run continuously
    CI_TRIGGERED,       // Triggered by CI/CD
    PERFORMANCE_GATE    // Performance gate validation
};

/**
 * @brief Benchmark status types
 */
enum class BenchmarkStatus {
    PENDING,            // Waiting to start
    RUNNING,            // Currently executing
    COMPLETED,          // Finished successfully
    FAILED,             // Failed with error
    CANCELLED,          // Cancelled by user
    TIMEOUT,            // Exceeded time limit
    REGRESSION_FAILED   // Failed due to performance regression
};

/**
 * @brief Benchmark severity levels
 */
enum class BenchmarkSeverity {
    INFO,               // Informational benchmark
    MINOR,              // Minor performance impact
    MAJOR,              // Significant performance impact
    CRITICAL            // Critical performance issue
};

/**
 * @brief Benchmark execution configuration
 */
struct BenchmarkConfig {
    // Execution parameters
    BenchmarkMode mode{BenchmarkMode::ON_DEMAND};
    std::chrono::seconds duration{300};        // 5 minutes default
    std::chrono::seconds warmup_time{30};      // 30 seconds warmup
    std::chrono::seconds timeout{900};         // 15 minutes timeout
    size_t sample_count{20};                   // Number of samples to collect
    std::chrono::seconds sample_interval{15};  // Interval between samples

    // Performance targets
    double min_throughput{0.0};                // Minimum required throughput
    double max_latency{0.0};                   // Maximum allowed latency
    double min_gpu_utilization{0.0};           // Minimum GPU utilization
    double max_memory_usage{0.0};              // Maximum memory usage (MB)

    // Regression detection
    bool enable_regression_detection{true};
    double regression_threshold{0.05};          // 5% regression threshold
    std::string baseline_file;                  // Baseline to compare against
    std::vector<std::string> baseline_files;    // Multiple baselines for ensemble

    // GPU configuration
    std::vector<int> gpu_devices;               // GPU devices to use (-1 for all)
    size_t threads_per_gpu{1};                  // Threads per GPU
    std::map<std::string, std::string> gpu_parameters;

    // Output configuration
    std::string output_directory{"./benchmark_results"};
    bool enable_detailed_logging{true};
    bool enable_profiling{false};
    bool enable_gpu_profiling{false};
    std::vector<std::string> output_formats{"json", "csv"};

    // Notification settings
    bool enable_notifications{true};
    std::vector<std::string> notification_channels;
    BenchmarkSeverity min_notification_severity{BenchmarkSeverity::MAJOR};

    // CI/CD integration
    bool ci_mode{false};
    std::string ci_environment;                // github, gitlab, jenkins, etc.
    std::map<std::string, std::string> ci_metadata;
    bool upload_artifacts{false};
    std::string artifact_url;

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
    static BenchmarkConfig fromJson(const std::string& json);
};

/**
 * @brief Benchmark metrics data point
 */
struct BenchmarkDataPoint {
    std::chrono::system_clock::time_point timestamp;
    double throughput{0.0};                    // Keys per second
    double latency{0.0};                       // Average latency (ms)
    double gpu_utilization{0.0};               // GPU utilization (%)
    double memory_usage{0.0};                  // Memory usage (MB)
    double power_usage{0.0};                   // Power usage (W)
    double temperature{0.0};                   // Temperature (°C)

    int device_id{-1};
    std::string device_name;
    std::map<std::string, double> custom_metrics;

    /**
     * @brief Serialize to JSON
     */
    std::string toJson() const;

    /**
     * @brief Deserialize from JSON
     */
    static BenchmarkDataPoint fromJson(const std::string& json);
};

/**
 * @brief Benchmark execution result
 */
struct BenchmarkResult {
    std::string id;
    std::string name;
    std::string description;
    BenchmarkConfig config;
    BenchmarkStatus status{BenchmarkStatus::PENDING};
    BenchmarkSeverity severity{BenchmarkSeverity::INFO};

    // Timing information
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::chrono::seconds duration{0};
    std::chrono::seconds actual_duration{0};

    // Execution environment
    std::string hostname;
    std::string os_info;
    std::string cuda_version;
    std::string driver_version;
    std::vector<std::string> gpu_info;

    // Collected metrics
    std::vector<BenchmarkDataPoint> data_points;
    size_t total_samples{0};
    size_t successful_samples{0};

    // Statistical analysis
    struct Statistics {
        double throughput_mean{0.0};
        double throughput_stddev{0.0};
        double throughput_min{0.0};
        double throughput_max{0.0};
        double throughput_p50{0.0};
        double throughput_p95{0.0};
        double throughput_p99{0.0};

        double latency_mean{0.0};
        double latency_stddev{0.0};
        double latency_p95{0.0};
        double latency_p99{0.0};

        double gpu_utilization_mean{0.0};
        double memory_usage_mean{0.0};
        double power_usage_mean{0.0};
        double temperature_mean{0.0};

        double stability_score{0.0};           // Performance stability (0-1)
        double efficiency_score{0.0};          // Resource efficiency (0-1)
    } statistics;

    // Regression analysis
    bool regression_detected{false};
    double regression_percentage{0.0};
    std::vector<std::string> regressed_metrics;
    std::map<std::string, double> baseline_comparison;

    // Performance targets
    bool targets_met{true};
    std::vector<std::string> failed_targets;
    std::map<std::string, double> target_achievement;

    // Error information
    std::string error_message;
    std::vector<std::string> warnings;
    std::string stack_trace;

    // Output files
    std::string results_file;
    std::string log_file;
    std::string profiling_file;
    std::vector<std::string> artifact_files;

    /**
     * @brief Get status as string
     */
    std::string getStatusString() const;

    /**
     * @brief Get severity as string
     */
    std::string getSeverityString() const;

    /**
     * @brief Check if benchmark was successful
     */
    bool isSuccessful() const;

    /**
     * @brief Get summary message
     */
    std::string getSummary() const;

    /**
     * @brief Serialize to JSON
     */
    std::string toJson() const;

    /**
     * @brief Deserialize from JSON
     */
    static BenchmarkResult fromJson(const std::string& json);
};

/**
 * @brief Baseline management
 */
struct BaselineData {
    std::string id;
    std::string name;
    std::string description;
    std::chrono::system_clock::time_point created_at;
    std::string git_commit;
    std::string build_configuration;
    std::vector<std::string> tags;

    BenchmarkResult::Statistics statistics;
    std::map<std::string, double> target_values;
    std::string hardware_signature;
    std::string software_signature;

    /**
     * @brief Check if baseline matches current environment
     */
    bool matchesEnvironment(const std::string& current_signature) const;

    /**
     * @brief Serialize to JSON
     */
    std::string toJson() const;

    /**
     * @brief Deserialize from JSON
     */
    static BaselineData fromJson(const std::string& json);
};

/**
 * @brief Benchmark scheduler
 */
class BenchmarkScheduler {
public:
    explicit BenchmarkScheduler();
    ~BenchmarkScheduler();

    /**
     * @brief Schedule benchmark
     */
    std::string scheduleBenchmark(
        const std::string& name,
        const BenchmarkConfig& config,
        std::chrono::system_clock::time_point when = {}
    );

    /**
     * @brief Cancel scheduled benchmark
     */
    bool cancelBenchmark(const std::string& benchmark_id);

    /**
     * @brief Get scheduled benchmarks
     */
    std::vector<std::string> getScheduledBenchmarks() const;

    /**
     * @brief Start scheduler
     */
    void start();

    /**
     * @brief Stop scheduler
     */
    void stop();

    /**
     * @brief Check if scheduler is running
     */
    bool isRunning() const { return scheduler_active_.load(); }

private:
    struct ScheduledBenchmark {
        std::string id;
        std::string name;
        BenchmarkConfig config;
        std::chrono::system_clock::time_point scheduled_time;
        bool recurring{false};
        std::chrono::seconds recurring_interval;
    };

    std::map<std::string, ScheduledBenchmark> scheduled_benchmarks_;
    mutable std::mutex scheduled_mutex_;

    std::atomic<bool> scheduler_active_{false};
    std::unique_ptr<std::thread> scheduler_thread_;
    std::condition_variable scheduler_cv_;

    void schedulerLoop();
    void executeScheduledBenchmark(const ScheduledBenchmark& benchmark);
    std::string generateBenchmarkId() const;
};

/**
 * @brief Benchmark executor
 */
class BenchmarkExecutor {
public:
    explicit BenchmarkExecutor();
    ~BenchmarkExecutor();

    /**
     * @brief Execute benchmark
     */
    std::future<BenchmarkResult> executeBenchmark(
        const std::string& name,
        const BenchmarkConfig& config
    );

    /**
     * @brief Execute benchmark synchronously
     */
    BenchmarkResult executeBenchmarkSync(
        const std::string& name,
        const BenchmarkConfig& config
    );

    /**
     * @brief Cancel running benchmark
     */
    bool cancelBenchmark(const std::string& execution_id);

    /**
     * @brief Get running benchmarks
     */
    std::vector<std::string> getRunningBenchmarks() const;

    /**
     * @brief Set progress callback
     */
    void setProgressCallback(std::function<void(const std::string&, double)> callback);

    /**
     * @brief Set data callback
     */
    void setDataCallback(std::function<void(const std::string&, const BenchmarkDataPoint&)> callback);

private:
    struct RunningBenchmark {
        std::string id;
        std::string name;
        BenchmarkConfig config;
        std::future<BenchmarkResult> future;
        std::atomic<bool> cancelled{false};
        std::thread execution_thread;
    };

    std::map<std::string, std::unique_ptr<RunningBenchmark>> running_benchmarks_;
    mutable std::mutex running_mutex_;

    std::function<void(const std::string&, double)> progress_callback_;
    std::function<void(const std::string&, const BenchmarkDataPoint&)> data_callback_;

    BenchmarkResult executeBenchmarkInternal(
        const std::string& id,
        const std::string& name,
        const BenchmarkConfig& config
    );

    std::vector<BenchmarkDataPoint> collectMetrics(
        const BenchmarkConfig& config,
        std::atomic<bool>& cancelled
    );

    BenchmarkResult::Statistics calculateStatistics(const std::vector<BenchmarkDataPoint>& data);

    std::string generateExecutionId() const;
};

/**
 * @brief Baseline manager
 */
class BaselineManager {
public:
    explicit BaselineManager(const std::string& baseline_directory = "./baselines");
    ~BaselineManager() = default;

    /**
     * @brief Create baseline from benchmark result
     */
    std::string createBaseline(
        const std::string& name,
        const BenchmarkResult& result,
        const std::vector<std::string>& tags = {}
    );

    /**
     * @brief Get baseline by ID
     */
    BaselineData getBaseline(const std::string& baseline_id) const;

    /**
     * @brief Get baseline by name
     */
    BaselineData getBaselineByName(const std::string& name) const;

    /**
     * @brief Get all baselines
     */
    std::vector<BaselineData> getAllBaselines() const;

    /**
     * @brief Get matching baseline for current environment
     */
    BaselineData getMatchingBaseline(const std::string& hardware_signature) const;

    /**
     * @brief Update baseline
     */
    bool updateBaseline(const std::string& baseline_id, const BaselineData& baseline);

    /**
     * @brief Delete baseline
     */
    bool deleteBaseline(const std::string& baseline_id);

    /**
     * @brief Export baselines
     */
    bool exportBaselines(const std::string& filename) const;

    /**
     * @brief Import baselines
     */
    bool importBaselines(const std::string& filename);

    /**
     * @brief Validate baseline integrity
     */
    bool validateBaseline(const std::string& baseline_id) const;

    /**
     * @brief Get baseline statistics
     */
    struct BaselineStats {
        size_t total_baselines{0};
        size_t valid_baselines{0};
        std::map<std::string, size_t> baseline_counts;
        std::chrono::system_clock::time_point oldest_baseline;
        std::chrono::system_clock::time_point newest_baseline;
    };

    BaselineStats getStatistics() const;

private:
    std::string baseline_directory_;
    mutable std::mutex baselines_mutex_;

    std::string generateBaselineId() const;
    std::string getBaselinePath(const std::string& baseline_id) const;
    std::string getHardwareSignature() const;
    std::string getSoftwareSignature() const;
    bool saveBaseline(const BaselineData& baseline) const;
    BaselineData loadBaseline(const std::string& baseline_id) const;
};

/**
 * @brief Regression detector for benchmarks
 */
class BenchmarkRegressionDetector {
public:
    explicit BenchmarkRegressionDetector(double threshold = 0.05);
    ~BenchmarkRegressionDetector() = default;

    /**
     * @brief Detect regressions compared to baseline
     */
    bool detectRegressions(
        const BenchmarkResult& result,
        const BaselineData& baseline,
        std::vector<std::string>& regressed_metrics
    );

    /**
     * @brief Calculate regression percentage
     */
    double calculateRegressionPercentage(
        const BenchmarkResult& result,
        const BaselineData& baseline
    );

    /**
     * @brief Check if regression is significant
     */
    bool isRegressionSignificant(
        double current_value,
        double baseline_value,
        double threshold
    );

    /**
     * @brief Set regression threshold
     */
    void setThreshold(double threshold) { threshold_ = threshold; }

    /**
     * @brief Get regression threshold
     */
    double getThreshold() const { return threshold_; }

private:
    double threshold_;

    std::vector<std::pair<std::string, std::function<double(const BenchmarkResult::Statistics&)>>>
        getMetricExtractors() const;
};

/**
 * @brief Automated benchmarking system
 *
 * Comprehensive system for automated performance benchmarking with CI integration,
 * baseline management, regression detection, and continuous monitoring.
 */
class AutomatedBenchmark {
public:
    explicit AutomatedBenchmark(const std::string& config_file = "");
    ~AutomatedBenchmark();

    // Configuration
    bool loadConfiguration(const std::string& config_file);
    bool saveConfiguration(const std::string& config_file) const;
    BenchmarkConfig getDefaultConfiguration() const;

    // Benchmark execution
    std::string runBenchmark(
        const std::string& name,
        const BenchmarkConfig& config = {}
    );

    std::string runScheduledBenchmark(
        const std::string& name,
        const BenchmarkConfig& config,
        std::chrono::system_clock::time_point when = {}
    );

    bool cancelBenchmark(const std::string& benchmark_id);
    BenchmarkResult getBenchmarkResult(const std::string& benchmark_id) const;

    // Batch operations
    std::vector<std::string> runBenchmarkSuite(
        const std::vector<std::pair<std::string, BenchmarkConfig>>& benchmarks
    );

    // Baseline management
    std::string createBaselineFromResult(
        const std::string& benchmark_id,
        const std::string& baseline_name,
        const std::vector<std::string>& tags = {}
    );

    BaselineData getBaseline(const std::string& baseline_id) const;
    std::vector<BaselineData> getAllBaselines() const;

    // Continuous monitoring
    void startContinuousMonitoring(const BenchmarkConfig& config);
    void stopContinuousMonitoring();
    bool isMonitoring() const { return monitoring_active_.load(); }

    // CI/CD integration
    bool runCIBenchmark(const std::string& environment = "");
    bool runPerformanceGate(const std::string& baseline_id = "");
    bool uploadResults(const std::string& benchmark_id, const std::string& url = "");

    // Reporting and analytics
    std::string generateReport(const std::string& benchmark_id) const;
    std::string generateTrendReport(const std::vector<std::string>& benchmark_ids) const;
    std::string generateComparisonReport(
        const std::vector<std::string>& benchmark_ids,
        const std::string& baseline_id
    ) const;

    // Statistics
    struct SystemStatistics {
        size_t total_benchmarks{0};
        size_t successful_benchmarks{0};
        size_t failed_benchmarks{0};
        size_t running_benchmarks{0};
        size_t scheduled_benchmarks{0};
        std::map<BenchmarkStatus, size_t> status_counts;
        std::map<BenchmarkSeverity, size_t> severity_counts;
        std::chrono::system_clock::time_point last_benchmark;
        double average_throughput{0.0};
        double regression_rate{0.0};
    };

    SystemStatistics getSystemStatistics() const;

    // Health check
    bool isHealthy() const;
    std::vector<std::string> getHealthIssues() const;

    // Callbacks
    void setBenchmarkStartCallback(std::function<void(const std::string&)> callback);
    void setBenchmarkCompleteCallback(std::function<void(const std::string&, const BenchmarkResult&)> callback);
    void setRegressionDetectedCallback(std::function<void(const std::string&, const std::vector<std::string>&)> callback);

private:
    BenchmarkConfig default_config_;
    std::unique_ptr<BenchmarkScheduler> scheduler_;
    std::unique_ptr<BenchmarkExecutor> executor_;
    std::unique_ptr<BaselineManager> baseline_manager_;
    std::unique_ptr<BenchmarkRegressionDetector> regression_detector_;

    // Result storage
    std::map<std::string, BenchmarkResult> benchmark_results_;
    mutable std::mutex results_mutex_;

    // Continuous monitoring
    std::atomic<bool> monitoring_active_{false};
    std::string monitoring_benchmark_id_;
    std::unique_ptr<std::thread> monitoring_thread_;

    // Callbacks
    std::function<void(const std::string&)> benchmark_start_callback_;
    std::function<void(const std::string&, const BenchmarkResult&)> benchmark_complete_callback_;
    std::function<void(const std::string&, const std::vector<std::string>&)> regression_detected_callback_;

    // Configuration
    std::string config_file_;
    mutable std::mutex config_mutex_;

    // Private methods
    void initializeComponents();
    void monitoringLoop(const BenchmarkConfig& config);
    void handleBenchmarkComplete(const std::string& benchmark_id, const BenchmarkResult& result);
    void handleRegressionDetected(const std::string& benchmark_id, const std::vector<std::string>& metrics);
    void updateSystemStatistics();
    std::string generateBenchmarkId() const;
};

/**
 * @brief Benchmark factory
 */
class BenchmarkFactory {
public:
    /**
     * @brief Create automated benchmark system
     */
    static std::unique_ptr<AutomatedBenchmark> create();

    /**
     * @brief Create with configuration file
     */
    static std::unique_ptr<AutomatedBenchmark> create(const std::string& config_file);

    /**
     * @brief Create CI-optimized benchmark system
     */
    static std::unique_ptr<AutomatedBenchmark> createCIBenchmark();

    /**
     * @brief Create development benchmark system
     */
    static std::unique_ptr<AutomatedBenchmark> createDevelopmentBenchmark();

    /**
     * @brief Create production benchmark system
     */
    static std::unique_ptr<AutomatedBenchmark> createProductionBenchmark();

    /**
     * @brief Create benchmark configuration templates
     */
    static BenchmarkConfig createQuickConfig();          // 5-minute quick test
    static BenchmarkConfig createStandardConfig();       // 15-minute standard test
    static BenchmarkConfig createComprehensiveConfig();  // 1-hour comprehensive test
    static BenchmarkConfig createCIConfig();             // CI-optimized config
    static BenchmarkConfig createPerformanceGateConfig(); // Performance gate config
};

/**
 * @brief Benchmark utilities
 */
namespace benchmark_utils {

/**
 * @brief Environment detection
 */
struct SystemInfo {
    std::string hostname;
    std::string os_name;
    std::string os_version;
    std::string cpu_model;
    size_t cpu_cores{0};
    size_t memory_total{0};
    std::vector<std::string> gpu_devices;
    std::string cuda_version;
    std::string driver_version;
};

SystemInfo getSystemInfo();
std::string getHardwareSignature(const SystemInfo& info);
std::string getSoftwareSignature(const SystemInfo& info);

/**
 * @brief GPU monitoring utilities
 */
struct GPUMetrics {
    int device_id{-1};
    std::string name;
    double utilization{0.0};
    size_t memory_used{0};
    size_t memory_total{0};
    double power_usage{0.0};
    double temperature{0.0};
    double clock_frequency{0.0};
};

std::vector<GPUMetrics> getGPUMetrics();
GPUMetrics getGPUMetrics(int device_id);

/**
 * @brief Performance calculation utilities
 */
double calculateThroughput(const std::vector<BenchmarkDataPoint>& data);
double calculateStabilityScore(const std::vector<BenchmarkDataPoint>& data);
double calculateEfficiencyScore(const BenchmarkResult& result);
std::vector<double> calculateMovingAverage(const std::vector<double>& values, size_t window_size);

/**
 * @brief Report generation utilities
 */
std::string generateHTMLReport(const BenchmarkResult& result);
std::string generateMarkdownReport(const BenchmarkResult& result);
std::string generateCSVReport(const std::vector<BenchmarkDataPoint>& data);
std::string generateJSONReport(const BenchmarkResult& result);

/**
 * @brief File I/O utilities
 */
bool saveResultToFile(const BenchmarkResult& result, const std::string& filename);
BenchmarkResult loadResultFromFile(const std::string& filename);
std::vector<BenchmarkResult> loadResultsFromDirectory(const std::string& directory);
bool compressResults(const std::vector<std::string>& files, const std::string& output);

/**
 * @brief Validation utilities
 */
bool validateBenchmarkResult(const BenchmarkResult& result);
bool validateBaseline(const BaselineData& baseline);
bool validateConfiguration(const BenchmarkConfig& config);
std::vector<std::string> getResultValidationErrors(const BenchmarkResult& result);

} // namespace benchmark_utils

} // namespace puzzle71::benchmark
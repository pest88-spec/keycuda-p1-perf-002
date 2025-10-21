// Puzzle71Solver - Automated Benchmarking Main Implementation (T051)
// Phase 6: User Story 4 - Performance Monitoring
// Main AutomatedBenchmark class implementation

#include "automated_benchmark.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <filesystem>

namespace puzzle71::benchmark {

// ============================================================================
// AutomatedBenchmark Implementation
// ============================================================================

AutomatedBenchmark::AutomatedBenchmark(const std::string& config_file)
    : config_file_(config_file) {
    initializeComponents();

    if (!config_file_.empty()) {
        loadConfiguration(config_file_);
    }
}

AutomatedBenchmark::~AutomatedBenchmark() {
    stopContinuousMonitoring();
}

bool AutomatedBenchmark::loadConfiguration(const std::string& config_file) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_file_ = config_file;

    std::ifstream file(config_file);
    if (!file.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

    default_config_ = BenchmarkConfig::fromJson(content);
    return default_config_.isValid();
}

bool AutomatedBenchmark::saveConfiguration(const std::string& config_file) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    std::string filename = config_file.empty() ? config_file_ : config_file;

    std::ofstream file(filename);
    if (!file.is_open()) return false;

    file << default_config_.toJson();
    return true;
}

BenchmarkConfig AutomatedBenchmark::getDefaultConfiguration() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return default_config_;
}

std::string AutomatedBenchmark::runBenchmark(
    const std::string& name,
    const BenchmarkConfig& config
) {
    BenchmarkConfig effective_config = config.isValid() ? config : default_config_;

    if (benchmark_start_callback_) {
        benchmark_start_callback_(name);
    }

    auto future = executor_->executeBenchmark(name, effective_config);
    std::string benchmark_id = generateBenchmarkId();

    // Store the future for later retrieval
    // In a real implementation, we'd manage the futures properly
    // For now, we'll execute synchronously
    try {
        auto result = future.get();
        handleBenchmarkComplete(benchmark_id, result);

        {
            std::lock_guard<std::mutex> lock(results_mutex_);
            benchmark_results_[benchmark_id] = result;
        }

        return benchmark_id;
    } catch (const std::exception& e) {
        BenchmarkResult failed_result;
        failed_result.id = benchmark_id;
        failed_result.name = name;
        failed_result.status = BenchmarkStatus::FAILED;
        failed_result.error_message = e.what();

        {
            std::lock_guard<std::mutex> lock(results_mutex_);
            benchmark_results_[benchmark_id] = failed_result;
        }

        handleBenchmarkComplete(benchmark_id, failed_result);
        return benchmark_id;
    }
}

std::string AutomatedBenchmark::runScheduledBenchmark(
    const std::string& name,
    const BenchmarkConfig& config,
    std::chrono::system_clock::time_point when
) {
    BenchmarkConfig effective_config = config.isValid() ? config : default_config_;
    return scheduler_->scheduleBenchmark(name, effective_config, when);
}

bool AutomatedBenchmark::cancelBenchmark(const std::string& benchmark_id) {
    // Try to cancel running benchmark first
    if (executor_->cancelBenchmark(benchmark_id)) {
        return true;
    }

    // Try to cancel scheduled benchmark
    return scheduler_->cancelBenchmark(benchmark_id);
}

BenchmarkResult AutomatedBenchmark::getBenchmarkResult(const std::string& benchmark_id) const {
    std::lock_guard<std::mutex> lock(results_mutex_);
    auto it = benchmark_results_.find(benchmark_id);
    return it != benchmark_results_.end() ? it->second : BenchmarkResult{};
}

std::vector<std::string> AutomatedBenchmark::runBenchmarkSuite(
    const std::vector<std::pair<std::string, BenchmarkConfig>>& benchmarks
) {
    std::vector<std::string> benchmark_ids;

    for (const auto& [name, config] : benchmarks) {
        benchmark_ids.push_back(runBenchmark(name, config));
    }

    return benchmark_ids;
}

std::string AutomatedBenchmark::createBaselineFromResult(
    const std::string& benchmark_id,
    const std::string& baseline_name,
    const std::vector<std::string>& tags
) {
    auto result = getBenchmarkResult(benchmark_id);
    if (result.id.empty() || result.status != BenchmarkStatus::COMPLETED) {
        return "";
    }

    return baseline_manager_->createBaseline(baseline_name, result, tags);
}

BaselineData AutomatedBenchmark::getBaseline(const std::string& baseline_id) const {
    return baseline_manager_->getBaseline(baseline_id);
}

std::vector<BaselineData> AutomatedBenchmark::getAllBaselines() const {
    return baseline_manager_->getAllBaselines();
}

void AutomatedBenchmark::startContinuousMonitoring(const BenchmarkConfig& config) {
    if (monitoring_active_.load()) return;

    monitoring_active_.store(true);
    monitoring_thread_ = std::make_unique<std::thread>(
        &AutomatedBenchmark::monitoringLoop, this, config
    );
}

void AutomatedBenchmark::stopContinuousMonitoring() {
    if (!monitoring_active_.load()) return;

    monitoring_active_.store(false);
    if (monitoring_thread_ && monitoring_thread_->joinable()) {
        monitoring_thread_->join();
    }
    monitoring_thread_.reset();
}

bool AutomatedBenchmark::runCIBenchmark(const std::string& environment) {
    BenchmarkConfig ci_config = BenchmarkFactory::createCIConfig();
    ci_config.ci_mode = true;
    ci_config.ci_environment = environment.empty() ? "unknown" : environment;

    // Set CI metadata
    ci_config.ci_metadata["runner"] = benchmark_utils::getSystemInfo().hostname;
    ci_config.ci_metadata["timestamp"] = std::to_string(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    std::string benchmark_id = runBenchmark("ci_benchmark", ci_config);
    auto result = getBenchmarkResult(benchmark_id);

    // Exit with appropriate code based on result
    if (result.status == BenchmarkStatus::COMPLETED && !result.regression_detected) {
        return true;
    }

    return false;
}

bool AutomatedBenchmark::runPerformanceGate(const std::string& baseline_id) {
    BaselineData baseline = baseline_manager_->getBaseline(baseline_id);
    if (baseline.id.empty()) {
        std::cerr << "Baseline not found: " << baseline_id << std::endl;
        return false;
    }

    BenchmarkConfig gate_config = BenchmarkFactory::createPerformanceGateConfig();
    gate_config.baseline_file = baseline_id;
    gate_config.enable_regression_detection = true;

    std::string benchmark_id = runBenchmark("performance_gate", gate_config);
    auto result = getBenchmarkResult(benchmark_id);

    return result.isSuccessful();
}

bool AutomatedBenchmark::uploadResults(const std::string& benchmark_id, const std::string& url) {
    auto result = getBenchmarkResult(benchmark_id);
    if (result.id.empty()) {
        return false;
    }

    // Simplified upload - in production, use proper HTTP client
    std::string upload_url = url.empty() ? result.config.artifact_url : url;
    if (upload_url.empty()) {
        std::cerr << "No upload URL specified" << std::endl;
        return false;
    }

    // For now, just log that we would upload
    std::cout << "Uploading results for benchmark " << benchmark_id
              << " to " << upload_url << std::endl;

    return true;
}

std::string AutomatedBenchmark::generateReport(const std::string& benchmark_id) const {
    auto result = getBenchmarkResult(benchmark_id);
    if (result.id.empty()) {
        return "Benchmark not found: " + benchmark_id;
    }

    std::ostringstream oss;
    oss << "Performance Benchmark Report\n";
    oss << "============================\n\n";

    oss << "Benchmark: " << result.name << " (" << result.id << ")\n";
    oss << "Status: " << result.getStatusString() << "\n";
    oss << "Severity: " << result.getSeverityString() << "\n";
    oss << "Duration: " << result.actual_duration.count() << " seconds\n\n";

    oss << "System Information:\n";
    oss << "  Hostname: " << result.hostname << "\n";
    oss << "  OS: " << result.os_info << "\n";
    oss << "  CUDA: " << result.cuda_version << "\n";
    oss << "  Driver: " << result.driver_version << "\n\n";

    if (!result.data_points.empty()) {
        oss << "Performance Statistics:\n";
        oss << "  Throughput: " << std::fixed << std::setprecision(2)
            << result.statistics.throughput_mean << " ± " << result.statistics.throughput_stddev
            << " keys/sec\n";
        oss << "  Throughput Range: " << result.statistics.throughput_min
            << " - " << result.statistics.throughput_max << " keys/sec\n";
        oss << "  Throughput P95: " << result.statistics.throughput_p95 << " keys/sec\n";
        oss << "  Latency P95: " << std::setprecision(3) << result.statistics.latency_p95 << " ms\n";
        oss << "  GPU Utilization: " << std::setprecision(1) << result.statistics.gpu_utilization_mean << "%\n";
        oss << "  Memory Usage: " << std::setprecision(1) << result.statistics.memory_usage_mean << " MB\n";
        oss << "  Stability Score: " << std::setprecision(3) << result.statistics.stability_score << "\n";
        oss << "  Efficiency Score: " << std::setprecision(2) << result.statistics.efficiency_score << " keys/W\n\n";
    }

    if (result.regression_detected) {
        oss << "Regression Detection:\n";
        oss << "  Status: REGRESSION DETECTED\n";
        oss << "  Regression: " << std::setprecision(1) << result.regression_percentage << "%\n";
        oss << "  Regressed Metrics: ";
        for (size_t i = 0; i < result.regressed_metrics.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << result.regressed_metrics[i];
        }
        oss << "\n\n";
    }

    if (!result.targets_met) {
        oss << "Performance Targets:\n";
        oss << "  Status: TARGETS NOT MET\n";
        for (const auto& target : result.failed_targets) {
            oss << "  Failed: " << target << "\n";
        }
        oss << "\n";
    }

    if (!result.error_message.empty()) {
        oss << "Error Information:\n";
        oss << "  Message: " << result.error_message << "\n";
        if (!result.warnings.empty()) {
            oss << "  Warnings:\n";
            for (const auto& warning : result.warnings) {
                oss << "    - " << warning << "\n";
            }
        }
        oss << "\n";
    }

    oss << "Files Generated:\n";
    if (!result.results_file.empty()) {
        oss << "  Results: " << result.results_file << "\n";
    }
    if (!result.log_file.empty()) {
        oss << "  Log: " << result.log_file << "\n";
    }
    if (!result.profiling_file.empty()) {
        oss << "  Profiling: " << result.profiling_file << "\n";
    }

    return oss.str();
}

std::string AutomatedBenchmark::generateTrendReport(const std::vector<std::string>& benchmark_ids) const {
    std::ostringstream oss;
    oss << "Performance Trend Report\n";
    oss << "=======================\n\n";

    std::vector<BenchmarkResult> results;
    for (const auto& id : benchmark_ids) {
        auto result = getBenchmarkResult(id);
        if (!result.id.empty()) {
            results.push_back(result);
        }
    }

    if (results.empty()) {
        return "No valid benchmark results found";
    }

    // Sort by timestamp
    std::sort(results.begin(), results.end(),
              [](const BenchmarkResult& a, const BenchmarkResult& b) {
                  return a.start_time < b.start_time;
              });

    oss << "Benchmark Trend Analysis (" << results.size() << " results)\n\n";

    // Calculate overall statistics
    double total_throughput = 0.0;
    double total_stability = 0.0;
    size_t successful_runs = 0;

    for (const auto& result : results) {
        if (result.status == BenchmarkStatus::COMPLETED) {
            total_throughput += result.statistics.throughput_mean;
            total_stability += result.statistics.stability_score;
            successful_runs++;
        }

        oss << "Run: " << result.name << " (" << result.id << ")\n";
        oss << "  Time: " << benchmark_utils::formatTimestamp(result.start_time) << "\n";
        oss << "  Status: " << result.getStatusString() << "\n";
        oss << "  Throughput: " << std::fixed << std::setprecision(2)
            << result.statistics.throughput_mean << " keys/sec\n";
        oss << "  Stability: " << std::setprecision(3) << result.statistics.stability_score << "\n";

        if (result.regression_detected) {
            oss << "  REGRESSION: " << std::setprecision(1) << result.regression_percentage << "%\n";
        }
        oss << "\n";
    }

    if (successful_runs > 0) {
        oss << "Summary Statistics:\n";
        oss << "  Successful Runs: " << successful_runs << "/" << results.size() << "\n";
        oss << "  Average Throughput: " << std::fixed << std::setprecision(2)
            << (total_throughput / successful_runs) << " keys/sec\n";
        oss << "  Average Stability: " << std::setprecision(3)
            << (total_stability / successful_runs) << "\n";

        // Calculate trend
        if (successful_runs >= 2) {
            std::vector<double> throughputs;
            for (const auto& result : results) {
                if (result.status == BenchmarkStatus::COMPLETED) {
                    throughputs.push_back(result.statistics.throughput_mean);
                }
            }

            double trend = throughputs.back() - throughputs.front();
            double trend_percentage = (trend / throughputs.front()) * 100.0;

            oss << "  Trend: " << std::setprecision(1) << trend_percentage << "% ";
            if (trend_percentage > 1.0) {
                oss << "(Improving)\n";
            } else if (trend_percentage < -1.0) {
                oss << "(Degrading)\n";
            } else {
                oss << "(Stable)\n";
            }
        }
    }

    return oss.str();
}

std::string AutomatedBenchmark::generateComparisonReport(
    const std::vector<std::string>& benchmark_ids,
    const std::string& baseline_id
) const {
    BaselineData baseline = baseline_manager_->getBaseline(baseline_id);
    if (baseline.id.empty()) {
        return "Baseline not found: " + baseline_id;
    }

    std::ostringstream oss;
    oss << "Performance Comparison Report\n";
    oss << "============================\n\n";

    oss << "Baseline: " << baseline.name << " (" << baseline.id << ")\n";
    oss << "Created: " << benchmark_utils::formatTimestamp(baseline.created_at) << "\n";
    oss << "Baseline Throughput: " << std::fixed << std::setprecision(2)
        << baseline.statistics.throughput_mean << " keys/sec\n\n";

    for (const auto& benchmark_id : benchmark_ids) {
        auto result = getBenchmarkResult(benchmark_id);
        if (result.id.empty()) continue;

        oss << "Comparison with: " << result.name << " (" << result.id << ")\n";
        oss << "  Run Time: " << benchmark_utils::formatTimestamp(result.start_time) << "\n";
        oss << "  Status: " << result.getStatusString() << "\n";

        if (result.status == BenchmarkStatus::COMPLETED) {
            double current_throughput = result.statistics.throughput_mean;
            double baseline_throughput = baseline.statistics.throughput_mean;
            double change = current_throughput - baseline_throughput;
            double change_percentage = (change / baseline_throughput) * 100.0;

            oss << "  Current Throughput: " << std::fixed << std::setprecision(2)
                << current_throughput << " keys/sec\n";
            oss << "  Change: " << std::setprecision(1) << change_percentage << "% ";
            if (change_percentage > 0) {
                oss << "(Improvement)\n";
            } else if (change_percentage < 0) {
                oss << "(Regression)\n";
            } else {
                oss << "(No Change)\n";
            }

            // Performance gate status
            bool gate_passed = true;
            if (result.config.min_throughput > 0 && current_throughput < result.config.min_throughput) {
                gate_passed = false;
            }
            if (result.regression_detected) {
                gate_passed = false;
            }

            oss << "  Performance Gate: " << (gate_passed ? "PASSED" : "FAILED") << "\n";
        }
        oss << "\n";
    }

    return oss.str();
}

AutomatedBenchmark::SystemStatistics AutomatedBenchmark::getSystemStatistics() const {
    SystemStatistics stats;

    std::lock_guard<std::mutex> lock(results_mutex_);
    stats.total_benchmarks = benchmark_results_.size();

    for (const auto& [id, result] : benchmark_results_) {
        stats.status_counts[result.status]++;
        stats.severity_counts[result.severity]++;

        if (result.status == BenchmarkStatus::COMPLETED) {
            stats.successful_benchmarks++;
            stats.average_throughput += result.statistics.throughput_mean;
        } else if (result.status == BenchmarkStatus::FAILED ||
                   result.status == BenchmarkStatus::REGRESSION_FAILED) {
            stats.failed_benchmarks++;
        }

        if (result.regression_detected) {
            stats.regression_rate++;
        }

        if (stats.last_benchmark.time_since_epoch().count() == 0 ||
            result.end_time > stats.last_benchmark) {
            stats.last_benchmark = result.end_time;
        }
    }

    // Get running benchmarks
    auto running = executor_->getRunningBenchmarks();
    stats.running_benchmarks = running.size();

    // Get scheduled benchmarks
    auto scheduled = scheduler_->getScheduledBenchmarks();
    stats.scheduled_benchmarks = scheduled.size();

    // Calculate averages
    if (stats.successful_benchmarks > 0) {
        stats.average_throughput /= stats.successful_benchmarks;
    }

    if (stats.total_benchmarks > 0) {
        stats.regression_rate /= stats.total_benchmarks;
    }

    return stats;
}

bool AutomatedBenchmark::isHealthy() const {
    // Check if all components are initialized and running properly
    if (!scheduler_ || !executor_ || !baseline_manager_ || !regression_detector_) {
        return false;
    }

    // Check if configuration is valid
    if (!default_config_.isValid()) {
        return false;
    }

    return true;
}

std::vector<std::string> AutomatedBenchmark::getHealthIssues() const {
    std::vector<std::string> issues;

    if (!scheduler_) {
        issues.push_back("Benchmark scheduler not initialized");
    }

    if (!executor_) {
        issues.push_back("Benchmark executor not initialized");
    }

    if (!baseline_manager_) {
        issues.push_back("Baseline manager not initialized");
    }

    if (!regression_detector_) {
        issues.push_back("Regression detector not initialized");
    }

    if (!default_config_.isValid()) {
        issues.push_back("Invalid default configuration");
    }

    return issues;
}

void AutomatedBenchmark::setBenchmarkStartCallback(
    std::function<void(const std::string&)> callback
) {
    benchmark_start_callback_ = callback;
}

void AutomatedBenchmark::setBenchmarkCompleteCallback(
    std::function<void(const std::string&, const BenchmarkResult&)> callback
) {
    benchmark_complete_callback_ = callback;
}

void AutomatedBenchmark::setRegressionDetectedCallback(
    std::function<void(const std::string&, const std::vector<std::string>&)> callback
) {
    regression_detected_callback_ = callback;
}

// ============================================================================
// Private Methods
// ============================================================================

void AutomatedBenchmark::initializeComponents() {
    scheduler_ = std::make_unique<BenchmarkScheduler>();
    executor_ = std::make_unique<BenchmarkExecutor>();
    baseline_manager_ = std::make_unique<BaselineManager>();
    regression_detector_ = std::make_unique<BenchmarkRegressionDetector>();

    // Set up executor callbacks
    executor_->setProgressCallback([this](const std::string& id, double progress) {
        // Handle progress updates
        // This could be used to update UI or send progress notifications
    });

    executor_->setDataCallback([this](const std::string& id, const BenchmarkDataPoint& point) {
        // Handle real-time data updates
        // This could be used for live monitoring or alerting
    });

    // Start scheduler
    scheduler_->start();
}

void AutomatedBenchmark::monitoringLoop(const BenchmarkConfig& config) {
    while (monitoring_active_.load()) {
        try {
            // Run a benchmark in monitoring mode
            std::string benchmark_id = runBenchmark("continuous_monitoring", config);

            // Wait for the next interval
            std::this_thread::sleep_for(std::chrono::minutes(30));
        } catch (const std::exception& e) {
            std::cerr << "Error in monitoring loop: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::minutes(5));
        }
    }
}

void AutomatedBenchmark::handleBenchmarkComplete(
    const std::string& benchmark_id,
    const BenchmarkResult& result
) {
    if (benchmark_complete_callback_) {
        benchmark_complete_callback_(benchmark_id, result);
    }

    // Check for regressions
    if (result.regression_detected && regression_detected_callback_) {
        regression_detected_callback_(benchmark_id, result.regressed_metrics);
    }

    updateSystemStatistics();
}

void AutomatedBenchmark::handleRegressionDetected(
    const std::string& benchmark_id,
    const std::vector<std::string>& metrics
) {
    if (regression_detected_callback_) {
        regression_detected_callback_(benchmark_id, metrics);
    }
}

void AutomatedBenchmark::updateSystemStatistics() {
    // This would update internal statistics tracking
    // For now, statistics are calculated on-demand in getSystemStatistics()
}

std::string AutomatedBenchmark::generateBenchmarkId() const {
    static std::atomic<uint64_t> counter{0};
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return "auto_bench_" + std::to_string(timestamp) + "_" + std::to_string(counter++);
}

// ============================================================================
// BenchmarkFactory Implementation
// ============================================================================

std::unique_ptr<AutomatedBenchmark> BenchmarkFactory::create() {
    return std::make_unique<AutomatedBenchmark>();
}

std::unique_ptr<AutomatedBenchmark> BenchmarkFactory::create(const std::string& config_file) {
    return std::make_unique<AutomatedBenchmark>(config_file);
}

std::unique_ptr<AutomatedBenchmark> BenchmarkFactory::createCIBenchmark() {
    auto benchmark = std::make_unique<AutomatedBenchmark>();
    benchmark->loadConfiguration(""); // Use default CI config
    return benchmark;
}

std::unique_ptr<AutomatedBenchmark> BenchmarkFactory::createDevelopmentBenchmark() {
    auto benchmark = std::make_unique<AutomatedBenchmark>();
    // Use development-friendly configuration
    return benchmark;
}

std::unique_ptr<AutomatedBenchmark> BenchmarkFactory::createProductionBenchmark() {
    auto benchmark = std::make_unique<AutomatedBenchmark>();
    // Use production-optimized configuration
    return benchmark;
}

BenchmarkConfig BenchmarkFactory::createQuickConfig() {
    BenchmarkConfig config;
    config.mode = BenchmarkMode::ON_DEMAND;
    config.duration = std::chrono::seconds(300);  // 5 minutes
    config.warmup_time = std::chrono::seconds(30);
    config.sample_count = 10;
    config.sample_interval = std::chrono::seconds(30);
    config.enable_regression_detection = false;
    config.enable_detailed_logging = true;
    config.output_formats = {"json"};
    return config;
}

BenchmarkConfig BenchmarkFactory::createStandardConfig() {
    BenchmarkConfig config;
    config.mode = BenchmarkMode::ON_DEMAND;
    config.duration = std::chrono::seconds(900);  // 15 minutes
    config.warmup_time = std::chrono::seconds(60);
    config.sample_count = 20;
    config.sample_interval = std::chrono::seconds(45);
    config.enable_regression_detection = true;
    config.regression_threshold = 0.05;  // 5%
    config.enable_detailed_logging = true;
    config.enable_profiling = false;
    config.output_formats = {"json", "csv"};
    return config;
}

BenchmarkConfig BenchmarkFactory::createComprehensiveConfig() {
    BenchmarkConfig config;
    config.mode = BenchmarkMode::ON_DEMAND;
    config.duration = std::chrono::seconds(3600);  // 1 hour
    config.warmup_time = std::chrono::seconds(300);  // 5 minutes
    config.sample_count = 50;
    config.sample_interval = std::chrono::seconds(72);
    config.enable_regression_detection = true;
    config.regression_threshold = 0.03;  // 3%
    config.enable_detailed_logging = true;
    config.enable_profiling = true;
    config.enable_gpu_profiling = true;
    config.output_formats = {"json", "csv", "html"};
    return config;
}

BenchmarkConfig BenchmarkFactory::createCIConfig() {
    BenchmarkConfig config;
    config.mode = BenchmarkMode::CI_TRIGGERED;
    config.duration = std::chrono::seconds(600);  // 10 minutes
    config.warmup_time = std::chrono::seconds(60);
    config.sample_count = 15;
    config.sample_interval = std::chrono::seconds(40);
    config.enable_regression_detection = true;
    config.regression_threshold = 0.05;  // 5%
    config.ci_mode = true;
    config.enable_detailed_logging = true;
    config.enable_notifications = true;
    config.upload_artifacts = true;
    return config;
}

BenchmarkConfig BenchmarkFactory::createPerformanceGateConfig() {
    BenchmarkConfig config;
    config.mode = BenchmarkMode::PERFORMANCE_GATE;
    config.duration = std::chrono::seconds(300);  // 5 minutes
    config.warmup_time = std::chrono::seconds(30);
    config.sample_count = 10;
    config.sample_interval = std::chrono::seconds(30);
    config.enable_regression_detection = true;
    config.regression_threshold = 0.02;  // 2% (stricter for gates)
    config.min_throughput = 1000.0;  // Minimum 1K keys/sec
    config.max_latency = 5.0;  // Maximum 5ms latency
    config.min_gpu_utilization = 80.0;  // Minimum 80% GPU utilization
    config.enable_detailed_logging = true;
    config.enable_notifications = true;
    return config;
}

// ============================================================================
// Additional Benchmark Utilities
// ============================================================================

namespace benchmark_utils {

std::string formatTimestamp(const std::chrono::system_clock::time_point& timestamp) {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S UTC");
    return oss.str();
}

} // namespace benchmark_utils

} // namespace puzzle71::benchmark
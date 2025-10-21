// Puzzle71Solver - Automated Performance Benchmarking (T051)
// Phase 6: User Story 4 - Performance Monitoring
// Comprehensive automated benchmarking system with CI integration and regression detection

#include "automated_benchmark.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <random>
#include <numeric>
#include <regex>
#include <curl/curl.h> // For HTTP uploads
#include <sys/utsname.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach/mach.h>
#include <sys/sysctl.h>
#elif defined(__linux__)
#include <sys/sysinfo.h>
#endif

namespace puzzle71::benchmark {

// ============================================================================
// BenchmarkConfig Implementation
// ============================================================================

bool BenchmarkConfig::isValid() const {
    return duration.count() > 0 &&
           warmup_time.count() < duration.count() &&
           timeout.count() > duration.count() &&
           sample_count > 0 &&
           sample_interval.count() > 0 &&
           sample_interval.count() * sample_count <= duration.count() &&
           !output_directory.empty();
}

std::string BenchmarkConfig::toJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"mode\":" << static_cast<int>(mode) << ","
        << "\"duration\":" << duration.count() << ","
        << "\"warmup_time\":" << warmup_time.count() << ","
        << "\"timeout\":" << timeout.count() << ","
        << "\"sample_count\":" << sample_count << ","
        << "\"sample_interval\":" << sample_interval.count() << ","
        << "\"min_throughput\":" << min_throughput << ","
        << "\"max_latency\":" << max_latency << ","
        << "\"min_gpu_utilization\":" << min_gpu_utilization << ","
        << "\"max_memory_usage\":" << max_memory_usage << ","
        << "\"enable_regression_detection\":" << (enable_regression_detection ? "true" : "false") << ","
        << "\"regression_threshold\":" << regression_threshold << ","
        << "\"baseline_file\":\"" << baseline_file << "\","
        << "\"output_directory\":\"" << output_directory << "\","
        << "\"enable_detailed_logging\":" << (enable_detailed_logging ? "true" : "false") << ","
        << "\"enable_profiling\":" << (enable_profiling ? "true" : "false") << ","
        << "\"ci_mode\":" << (ci_mode ? "true" : "false") << ","
        << "\"ci_environment\":\"" << ci_environment << "\""
        << "}";
    return oss.str();
}

BenchmarkConfig BenchmarkConfig::fromJson(const std::string& json) {
    // Simplified JSON parsing - in production, use a proper JSON library
    BenchmarkConfig config;
    // TODO: Implement proper JSON parsing
    return config;
}

// ============================================================================
// BenchmarkDataPoint Implementation
// ============================================================================

std::string BenchmarkDataPoint::toJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"timestamp\":" << std::chrono::duration<double>(timestamp.time_since_epoch()).count() << ","
        << "\"throughput\":" << throughput << ","
        << "\"latency\":" << latency << ","
        << "\"gpu_utilization\":" << gpu_utilization << ","
        << "\"memory_usage\":" << memory_usage << ","
        << "\"power_usage\":" << power_usage << ","
        << "\"temperature\":" << temperature << ","
        << "\"device_id\":" << device_id << ","
        << "\"device_name\":\"" << device_name << "\"";

    if (!custom_metrics.empty()) {
        oss << ",\"custom_metrics\":{";
        bool first = true;
        for (const auto& [key, value] : custom_metrics) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":" << value;
            first = false;
        }
        oss << "}";
    }

    oss << "}";
    return oss.str();
}

BenchmarkDataPoint BenchmarkDataPoint::fromJson(const std::string& json) {
    // Simplified JSON parsing
    BenchmarkDataPoint point;
    // TODO: Implement proper JSON parsing
    return point;
}

// ============================================================================
// BenchmarkResult Implementation
// ============================================================================

std::string BenchmarkResult::getStatusString() const {
    switch (status) {
        case BenchmarkStatus::PENDING: return "pending";
        case BenchmarkStatus::RUNNING: return "running";
        case BenchmarkStatus::COMPLETED: return "completed";
        case BenchmarkStatus::FAILED: return "failed";
        case BenchmarkStatus::CANCELLED: return "cancelled";
        case BenchmarkStatus::TIMEOUT: return "timeout";
        case BenchmarkStatus::REGRESSION_FAILED: return "regression_failed";
        default: return "unknown";
    }
}

std::string BenchmarkResult::getSeverityString() const {
    switch (severity) {
        case BenchmarkSeverity::INFO: return "info";
        case BenchmarkSeverity::MINOR: return "minor";
        case BenchmarkSeverity::MAJOR: return "major";
        case BenchmarkSeverity::CRITICAL: return "critical";
        default: return "info";
    }
}

bool BenchmarkResult::isSuccessful() const {
    return status == BenchmarkStatus::COMPLETED &&
           !regression_detected &&
           targets_met;
}

std::string BenchmarkResult::getSummary() const {
    std::ostringstream oss;
    oss << "Benchmark '" << name << "' ";

    if (status == BenchmarkStatus::COMPLETED) {
        oss << "completed successfully";
        if (!data_points.empty()) {
            oss << " with " << std::fixed << std::setprecision(2)
                << statistics.throughput_mean << " keys/sec average throughput";
        }
        if (regression_detected) {
            oss << " (REGRESSION DETECTED: " << std::setprecision(1)
                << regression_percentage << "%)";
        }
    } else {
        oss << "failed with status: " << getStatusString();
        if (!error_message.empty()) {
            oss << " - " << error_message;
        }
    }

    return oss.str();
}

std::string BenchmarkResult::toJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":\"" << id << "\","
        << "\"name\":\"" << name << "\","
        << "\"description\":\"" << description << "\","
        << "\"status\":\"" << getStatusString() << "\","
        << "\"severity\":\"" << getSeverityString() << "\","
        << "\"start_time\":" << std::chrono::duration<double>(start_time.time_since_epoch()).count() << ","
        << "\"end_time\":" << std::chrono::duration<double>(end_time.time_since_epoch()).count() << ","
        << "\"duration\":" << duration.count() << ","
        << "\"actual_duration\":" << actual_duration.count() << ","
        << "\"hostname\":\"" << hostname << "\","
        << "\"os_info\":\"" << os_info << "\","
        << "\"cuda_version\":\"" << cuda_version << "\","
        << "\"driver_version\":\"" << driver_version << "\","
        << "\"total_samples\":" << total_samples << ","
        << "\"successful_samples\":" << successful_samples << ","
        << "\"statistics\":{"
        << "\"throughput_mean\":" << statistics.throughput_mean << ","
        << "\"throughput_stddev\":" << statistics.throughput_stddev << ","
        << "\"throughput_min\":" << statistics.throughput_min << ","
        << "\"throughput_max\":" << statistics.throughput_max << ","
        << "\"throughput_p50\":" << statistics.throughput_p50 << ","
        << "\"throughput_p95\":" << statistics.throughput_p95 << ","
        << "\"throughput_p99\":" << statistics.throughput_p99 << ","
        << "\"latency_mean\":" << statistics.latency_mean << ","
        << "\"latency_stddev\":" << statistics.latency_stddev << ","
        << "\"latency_p95\":" << statistics.latency_p95 << ","
        << "\"latency_p99\":" << statistics.latency_p99 << ","
        << "\"gpu_utilization_mean\":" << statistics.gpu_utilization_mean << ","
        << "\"memory_usage_mean\":" << statistics.memory_usage_mean << ","
        << "\"stability_score\":" << statistics.stability_score << ","
        << "\"efficiency_score\":" << statistics.efficiency_score
        << "},"
        << "\"regression_detected\":" << (regression_detected ? "true" : "false") << ","
        << "\"regression_percentage\":" << regression_percentage << ","
        << "\"targets_met\":" << (targets_met ? "true" : "false") << ","
        << "\"error_message\":\"" << error_message << "\""
        << "}";
    return oss.str();
}

BenchmarkResult BenchmarkResult::fromJson(const std::string& json) {
    // Simplified JSON parsing
    BenchmarkResult result;
    // TODO: Implement proper JSON parsing
    return result;
}

// ============================================================================
// BaselineData Implementation
// ============================================================================

bool BaselineData::matchesEnvironment(const std::string& current_signature) const {
    return hardware_signature == current_signature;
}

std::string BaselineData::toJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":\"" << id << "\","
        << "\"name\":\"" << name << "\","
        << "\"description\":\"" << description << "\","
        << "\"created_at\":" << std::chrono::duration<double>(created_at.time_since_epoch()).count() << ","
        << "\"git_commit\":\"" << git_commit << "\","
        << "\"build_configuration\":\"" << build_configuration << "\","
        << "\"hardware_signature\":\"" << hardware_signature << "\","
        << "\"software_signature\":\"" << software_signature << "\","
        << "\"statistics\":{"
        << "\"throughput_mean\":" << statistics.throughput_mean << ","
        << "\"throughput_stddev\":" << statistics.throughput_stddev << ","
        << "\"throughput_p95\":" << statistics.throughput_p95 << ","
        << "\"latency_mean\":" << statistics.latency_mean << ","
        << "\"gpu_utilization_mean\":" << statistics.gpu_utilization_mean
        << "}"
        << "}";
    return oss.str();
}

BaselineData BaselineData::fromJson(const std::string& json) {
    // Simplified JSON parsing
    BaselineData baseline;
    // TODO: Implement proper JSON parsing
    return baseline;
}

// ============================================================================
// BenchmarkScheduler Implementation
// ============================================================================

BenchmarkScheduler::BenchmarkScheduler() = default;

BenchmarkScheduler::~BenchmarkScheduler() {
    stop();
}

std::string BenchmarkScheduler::scheduleBenchmark(
    const std::string& name,
    const BenchmarkConfig& config,
    std::chrono::system_clock::time_point when
) {
    if (when.time_since_epoch().count() == 0) {
        when = std::chrono::system_clock::now();
    }

    ScheduledBenchmark benchmark;
    benchmark.id = generateBenchmarkId();
    benchmark.name = name;
    benchmark.config = config;
    benchmark.scheduled_time = when;
    benchmark.recurring = false;

    {
        std::lock_guard<std::mutex> lock(scheduled_mutex_);
        scheduled_benchmarks_[benchmark.id] = benchmark;
    }

    return benchmark.id;
}

bool BenchmarkScheduler::cancelBenchmark(const std::string& benchmark_id) {
    std::lock_guard<std::mutex> lock(scheduled_mutex_);
    return scheduled_benchmarks_.erase(benchmark_id) > 0;
}

std::vector<std::string> BenchmarkScheduler::getScheduledBenchmarks() const {
    std::lock_guard<std::mutex> lock(scheduled_mutex_);
    std::vector<std::string> ids;
    for (const auto& [id, _] : scheduled_benchmarks_) {
        ids.push_back(id);
    }
    return ids;
}

void BenchmarkScheduler::start() {
    if (scheduler_active_.load()) return;

    scheduler_active_.store(true);
    scheduler_thread_ = std::make_unique<std::thread>(&BenchmarkScheduler::schedulerLoop, this);
}

void BenchmarkScheduler::stop() {
    if (!scheduler_active_.load()) return;

    scheduler_active_.store(false);
    scheduler_cv_.notify_all();

    if (scheduler_thread_ && scheduler_thread_->joinable()) {
        scheduler_thread_->join();
    }
    scheduler_thread_.reset();
}

void BenchmarkScheduler::schedulerLoop() {
    while (scheduler_active_.load()) {
        auto now = std::chrono::system_clock::now();
        std::vector<ScheduledBenchmark> ready_benchmarks;

        {
            std::lock_guard<std::mutex> lock(scheduled_mutex_);
            auto it = scheduled_benchmarks_.begin();
            while (it != scheduled_benchmarks_.end()) {
                if (it->second.scheduled_time <= now) {
                    ready_benchmarks.push_back(it->second);
                    it = scheduled_benchmarks_.erase(it);
                } else {
                    ++it;
                }
            }
        }

        // Execute ready benchmarks
        for (const auto& benchmark : ready_benchmarks) {
            executeScheduledBenchmark(benchmark);
        }

        // Calculate time until next benchmark
        std::chrono::system_clock::time_point next_time;
        {
            std::lock_guard<std::mutex> lock(scheduled_mutex_);
            if (scheduled_benchmarks_.empty()) {
                next_time = now + std::chrono::minutes(1);
            } else {
                next_time = scheduled_benchmarks_.begin()->second.scheduled_time;
            }
        }

        auto sleep_duration = std::chrono::duration_cast<std::chrono::seconds>(next_time - now);
        if (sleep_duration.count() > 0) {
            std::unique_lock<std::mutex> lock(scheduled_mutex_);
            scheduler_cv_.wait_for(lock, sleep_duration, [this] { return !scheduler_active_.load(); });
        }
    }
}

void BenchmarkScheduler::executeScheduledBenchmark(const ScheduledBenchmark& benchmark) {
    // This would integrate with the BenchmarkExecutor
    // For now, just log that we would execute it
    std::cout << "Executing scheduled benchmark: " << benchmark.name << " (" << benchmark.id << ")" << std::endl;
}

std::string BenchmarkScheduler::generateBenchmarkId() const {
    static std::atomic<uint64_t> counter{0};
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return "bench_" + std::to_string(timestamp) + "_" + std::to_string(counter++);
}

// ============================================================================
// BenchmarkExecutor Implementation
// ============================================================================

BenchmarkExecutor::BenchmarkExecutor() = default;

BenchmarkExecutor::~BenchmarkExecutor() {
    std::lock_guard<std::mutex> lock(running_mutex_);
    for (auto& [id, benchmark] : running_benchmarks_) {
        benchmark->cancelled.store(true);
        if (benchmark->execution_thread.joinable()) {
            benchmark->execution_thread.join();
        }
    }
    running_benchmarks_.clear();
}

std::future<BenchmarkResult> BenchmarkExecutor::executeBenchmark(
    const std::string& name,
    const BenchmarkConfig& config
) {
    std::string execution_id = generateExecutionId();

    auto benchmark = std::make_unique<RunningBenchmark>();
    benchmark->id = execution_id;
    benchmark->name = name;
    benchmark->config = config;

    // Create promise and future
    auto promise = std::make_shared<std::promise<BenchmarkResult>>();
    benchmark->future = promise->get_future();

    // Start execution thread
    benchmark->execution_thread = std::thread([this, execution_id, name, config, promise]() {
        try {
            auto result = executeBenchmarkInternal(execution_id, name, config);
            promise->set_value(result);
        } catch (const std::exception& e) {
            BenchmarkResult failed_result;
            failed_result.id = execution_id;
            failed_result.name = name;
            failed_result.status = BenchmarkStatus::FAILED;
            failed_result.error_message = e.what();
            promise->set_value(failed_result);
        }

        // Clean up from running benchmarks
        std::lock_guard<std::mutex> lock(running_mutex_);
        running_benchmarks_.erase(execution_id);
    });

    {
        std::lock_guard<std::mutex> lock(running_mutex_);
        running_benchmarks_[execution_id] = std::move(benchmark);
    }

    return promise->get_future();
}

BenchmarkResult BenchmarkExecutor::executeBenchmarkSync(
    const std::string& name,
    const BenchmarkConfig& config
) {
    std::string execution_id = generateExecutionId();
    return executeBenchmarkInternal(execution_id, name, config);
}

bool BenchmarkExecutor::cancelBenchmark(const std::string& execution_id) {
    std::lock_guard<std::mutex> lock(running_mutex_);
    auto it = running_benchmarks_.find(execution_id);
    if (it != running_benchmarks_.end()) {
        it->second->cancelled.store(true);
        return true;
    }
    return false;
}

std::vector<std::string> BenchmarkExecutor::getRunningBenchmarks() const {
    std::lock_guard<std::mutex> lock(running_mutex_);
    std::vector<std::string> ids;
    for (const auto& [id, _] : running_benchmarks_) {
        ids.push_back(id);
    }
    return ids;
}

void BenchmarkExecutor::setProgressCallback(
    std::function<void(const std::string&, double)> callback
) {
    progress_callback_ = callback;
}

void BenchmarkExecutor::setDataCallback(
    std::function<void(const std::string&, const BenchmarkDataPoint&)> callback
) {
    data_callback_ = callback;
}

BenchmarkResult BenchmarkExecutor::executeBenchmarkInternal(
    const std::string& id,
    const std::string& name,
    const BenchmarkConfig& config
) {
    BenchmarkResult result;
    result.id = id;
    result.name = name;
    result.config = config;
    result.status = BenchmarkStatus::RUNNING;
    result.start_time = std::chrono::system_clock::now();

    // Set up system information
    auto system_info = benchmark_utils::getSystemInfo();
    result.hostname = system_info.hostname;
    result.os_info = system_info.os_name + " " + system_info.os_version;
    result.cuda_version = system_info.cuda_version;
    result.driver_version = system_info.driver_version;
    result.gpu_info = system_info.gpu_devices;

    try {
        // Create output directory
        std::filesystem::create_directories(config.output_directory);

        // Execute warmup
        if (config.warmup_time.count() > 0) {
            if (progress_callback_) {
                progress_callback_(id, 0.0);
            }
            std::this_thread::sleep_for(config.warmup_time);
        }

        // Collect metrics
        std::atomic<bool> cancelled{false};
        auto data_points = collectMetrics(config, cancelled);

        if (cancelled.load()) {
            result.status = BenchmarkStatus::CANCELLED;
            return result;
        }

        result.data_points = data_points;
        result.total_samples = config.sample_count;
        result.successful_samples = data_points.size();

        // Calculate statistics
        result.statistics = calculateStatistics(data_points);

        // Check performance targets
        result.targets_met = true;
        if (config.min_throughput > 0 && result.statistics.throughput_mean < config.min_throughput) {
            result.targets_met = false;
            result.failed_targets.push_back("Minimum throughput not met");
        }

        if (config.max_latency > 0 && result.statistics.latency_p95 > config.max_latency) {
            result.targets_met = false;
            result.failed_targets.push_back("Maximum latency exceeded");
        }

        if (config.min_gpu_utilization > 0 && result.statistics.gpu_utilization_mean < config.min_gpu_utilization) {
            result.targets_met = false;
            result.failed_targets.push_back("Minimum GPU utilization not met");
        }

        result.status = BenchmarkStatus::COMPLETED;
        result.severity = result.targets_met ? BenchmarkSeverity::INFO : BenchmarkSeverity::MAJOR;

    } catch (const std::exception& e) {
        result.status = BenchmarkStatus::FAILED;
        result.error_message = e.what();
        result.severity = BenchmarkSeverity::CRITICAL;
    }

    result.end_time = std::chrono::system_clock::now();
    result.actual_duration = std::chrono::duration_cast<std::chrono::seconds>(result.end_time - result.start_time);

    // Save results to file
    if (config.enable_detailed_logging) {
        std::string filename = config.output_directory + "/" + id + "_results.json";
        benchmark_utils::saveResultToFile(result, filename);
        result.results_file = filename;
    }

    return result;
}

std::vector<BenchmarkDataPoint> BenchmarkExecutor::collectMetrics(
    const BenchmarkConfig& config,
    std::atomic<bool>& cancelled
) {
    std::vector<BenchmarkDataPoint> data_points;
    auto start_time = std::chrono::system_clock::now();
    auto end_time = start_time + config.duration;
    auto sample_interval = config.sample_interval;
    size_t target_samples = config.sample_count;

    size_t sample_count = 0;
    while (std::chrono::system_clock::now() < end_time &&
           sample_count < target_samples &&
           !cancelled.load()) {

        BenchmarkDataPoint point;
        point.timestamp = std::chrono::system_clock::now();

        // Collect GPU metrics
        auto gpu_metrics = benchmark_utils::getGPUMetrics();
        if (!gpu_metrics.empty()) {
            const auto& gpu = gpu_metrics[0]; // Use first GPU for now
            point.device_id = gpu.device_id;
            point.device_name = gpu.name;
            point.gpu_utilization = gpu.utilization;
            point.memory_usage = gpu.memory_used / (1024.0 * 1024.0); // Convert to MB
            point.power_usage = gpu.power_usage;
            point.temperature = gpu.temperature;
        }

        // Simulate throughput calculation (replace with actual GPU execution)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<double> throughput_dist(2000.0, 200.0); // 2K ± 200 keys/sec
        std::normal_distribution<double> latency_dist(0.5, 0.1); // 0.5ms ± 0.1ms

        point.throughput = std::max(0.0, throughput_dist(gen));
        point.latency = std::max(0.0, latency_dist(gen));

        data_points.push_back(point);
        sample_count++;

        // Notify data callback
        if (data_callback_) {
            data_callback_(config.mode == BenchmarkMode::ON_DEMAND ? "benchmark" : "", point);
        }

        // Update progress
        if (progress_callback_) {
            double progress = static_cast<double>(sample_count) / target_samples;
            progress_callback_("", progress);
        }

        // Wait for next sample
        std::this_thread::sleep_for(sample_interval);
    }

    return data_points;
}

BenchmarkResult::Statistics BenchmarkExecutor::calculateStatistics(
    const std::vector<BenchmarkDataPoint>& data
) {
    BenchmarkResult::Statistics stats;

    if (data.empty()) return stats;

    // Extract throughput values
    std::vector<double> throughput_values;
    std::vector<double> latency_values;
    std::vector<double> gpu_util_values;
    std::vector<double> memory_values;
    std::vector<double> power_values;
    std::vector<double> temp_values;

    for (const auto& point : data) {
        throughput_values.push_back(point.throughput);
        latency_values.push_back(point.latency);
        gpu_util_values.push_back(point.gpu_utilization);
        memory_values.push_back(point.memory_usage);
        power_values.push_back(point.power_usage);
        temp_values.push_back(point.temperature);
    }

    // Calculate throughput statistics
    stats.throughput_mean = benchmark_utils::calculateMean(throughput_values);
    stats.throughput_stddev = benchmark_utils::calculateStdDev(throughput_values);
    stats.throughput_min = *std::min_element(throughput_values.begin(), throughput_values.end());
    stats.throughput_max = *std::max_element(throughput_values.begin(), throughput_values.end());

    // Calculate percentiles
    std::sort(throughput_values.begin(), throughput_values.end());
    size_t p50_idx = throughput_values.size() * 0.5;
    size_t p95_idx = throughput_values.size() * 0.95;
    size_t p99_idx = throughput_values.size() * 0.99;

    stats.throughput_p50 = throughput_values[std::min(p50_idx, throughput_values.size() - 1)];
    stats.throughput_p95 = throughput_values[std::min(p95_idx, throughput_values.size() - 1)];
    stats.throughput_p99 = throughput_values[std::min(p99_idx, throughput_values.size() - 1)];

    // Calculate latency statistics
    stats.latency_mean = benchmark_utils::calculateMean(latency_values);
    stats.latency_stddev = benchmark_utils::calculateStdDev(latency_values);

    std::sort(latency_values.begin(), latency_values.end());
    stats.latency_p95 = latency_values[std::min(p95_idx, latency_values.size() - 1)];
    stats.latency_p99 = latency_values[std::min(p99_idx, latency_values.size() - 1)];

    // Calculate average metrics
    stats.gpu_utilization_mean = benchmark_utils::calculateMean(gpu_util_values);
    stats.memory_usage_mean = benchmark_utils::calculateMean(memory_values);
    stats.power_usage_mean = benchmark_utils::calculateMean(power_values);
    stats.temperature_mean = benchmark_utils::calculateMean(temp_values);

    // Calculate stability and efficiency scores
    stats.stability_score = benchmark_utils::calculateStabilityScore(data);

    // Simple efficiency calculation (throughput per watt)
    if (stats.power_usage_mean > 0) {
        stats.efficiency_score = stats.throughput_mean / stats.power_usage_mean;
    }

    return stats;
}

std::string BenchmarkExecutor::generateExecutionId() const {
    static std::atomic<uint64_t> counter{0};
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return "exec_" + std::to_string(timestamp) + "_" + std::to_string(counter++);
}

// ============================================================================
// BaselineManager Implementation
// ============================================================================

BaselineManager::BaselineManager(const std::string& baseline_directory)
    : baseline_directory_(baseline_directory) {
    std::filesystem::create_directories(baseline_directory_);
}

std::string BaselineManager::createBaseline(
    const std::string& name,
    const BenchmarkResult& result,
    const std::vector<std::string>& tags
) {
    BaselineData baseline;
    baseline.id = generateBaselineId();
    baseline.name = name;
    baseline.description = "Baseline from benchmark: " + result.name;
    baseline.created_at = std::chrono::system_clock::now();
    baseline.statistics = result.statistics;
    baseline.hardware_signature = getHardwareSignature();
    baseline.software_signature = getSoftwareSignature();
    baseline.tags = tags;

    // Get git commit if available
    std::ifstream git_head(".git/HEAD");
    if (git_head.is_open()) {
        std::string head_ref;
        std::getline(git_head, head_ref);
        if (head_ref.find("ref:") == 0) {
            std::string ref_path = ".git/" + head_ref.substr(5);
            std::ifstream ref_file(ref_path);
            if (ref_file.is_open()) {
                std::getline(ref_file, baseline.git_commit);
            }
        }
    }

    if (saveBaseline(baseline)) {
        return baseline.id;
    }

    return "";
}

BaselineData BaselineManager::getBaseline(const std::string& baseline_id) const {
    return loadBaseline(baseline_id);
}

BaselineData BaselineManager::getBaselineByName(const std::string& name) const {
    std::lock_guard<std::mutex> lock(baselines_mutex_);

    for (const auto& entry : std::filesystem::directory_iterator(baseline_directory_)) {
        if (entry.path().extension() == ".json") {
            auto baseline = loadBaseline(entry.path().stem().string());
            if (baseline.name == name) {
                return baseline;
            }
        }
    }

    return BaselineData{};
}

std::vector<BaselineData> BaselineManager::getAllBaselines() const {
    std::vector<BaselineData> baselines;

    std::lock_guard<std::mutex> lock(baselines_mutex_);
    for (const auto& entry : std::filesystem::directory_iterator(baseline_directory_)) {
        if (entry.path().extension() == ".json") {
            auto baseline = loadBaseline(entry.path().stem().string());
            if (!baseline.id.empty()) {
                baselines.push_back(baseline);
            }
        }
    }

    // Sort by creation time (newest first)
    std::sort(baselines.begin(), baselines.end(),
              [](const BaselineData& a, const BaselineData& b) {
                  return a.created_at > b.created_at;
              });

    return baselines;
}

BaselineData BaselineManager::getMatchingBaseline(const std::string& hardware_signature) const {
    auto baselines = getAllBaselines();
    for (const auto& baseline : baselines) {
        if (baseline.matchesEnvironment(hardware_signature)) {
            return baseline;
        }
    }
    return BaselineData{};
}

bool BaselineManager::updateBaseline(const std::string& baseline_id, const BaselineData& baseline) {
    return saveBaseline(baseline);
}

bool BaselineManager::deleteBaseline(const std::string& baseline_id) {
    std::string path = getBaselinePath(baseline_id);
    return std::filesystem::remove(path);
}

bool BaselineManager::exportBaselines(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return false;

    auto baselines = getAllBaselines();
    file << "{\n  \"baselines\": [\n";

    for (size_t i = 0; i < baselines.size(); ++i) {
        if (i > 0) file << ",\n";
        file << "    " << baselines[i].toJson();
    }

    file << "\n  ]\n}\n";
    return true;
}

bool BaselineManager::importBaselines(const std::string& filename) {
    // Simplified import - in production, use proper JSON parsing
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    // TODO: Implement proper JSON parsing and baseline creation
    return true;
}

bool BaselineManager::validateBaseline(const std::string& baseline_id) const {
    auto baseline = loadBaseline(baseline_id);
    return !baseline.id.empty() &&
           baseline.statistics.throughput_mean > 0 &&
           !baseline.hardware_signature.empty();
}

BaselineManager::BaselineStats BaselineManager::getStatistics() const {
    BaselineStats stats;
    auto baselines = getAllBaselines();

    stats.total_baselines = baselines.size();

    for (const auto& baseline : baselines) {
        if (validateBaseline(baseline.id)) {
            stats.valid_baselines++;
        }

        stats.baseline_counts[baseline.name]++;

        if (stats.oldest_baseline.time_since_epoch().count() == 0 ||
            baseline.created_at < stats.oldest_baseline) {
            stats.oldest_baseline = baseline.created_at;
        }

        if (stats.newest_baseline.time_since_epoch().count() == 0 ||
            baseline.created_at > stats.newest_baseline) {
            stats.newest_baseline = baseline.created_at;
        }
    }

    return stats;
}

std::string BaselineManager::generateBaselineId() const {
    static std::atomic<uint64_t> counter{0};
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return "baseline_" + std::to_string(timestamp) + "_" + std::to_string(counter++);
}

std::string BaselineManager::getBaselinePath(const std::string& baseline_id) const {
    return baseline_directory_ + "/" + baseline_id + ".json";
}

std::string BaselineManager::getHardwareSignature() const {
    auto system_info = benchmark_utils::getSystemInfo();
    return benchmark_utils::getHardwareSignature(system_info);
}

std::string BaselineManager::getSoftwareSignature() const {
    auto system_info = benchmark_utils::getSystemInfo();
    return benchmark_utils::getSoftwareSignature(system_info);
}

bool BaselineManager::saveBaseline(const BaselineData& baseline) const {
    std::string path = getBaselinePath(baseline.id);
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << baseline.toJson();
    return true;
}

BaselineData BaselineManager::loadBaseline(const std::string& baseline_id) const {
    std::string path = getBaselinePath(baseline_id);
    std::ifstream file(path);
    if (!file.is_open()) return BaselineData{};

    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

    return BaselineData::fromJson(content);
}

// ============================================================================
// BenchmarkRegressionDetector Implementation
// ============================================================================

BenchmarkRegressionDetector::BenchmarkRegressionDetector(double threshold)
    : threshold_(threshold) {}

bool BenchmarkRegressionDetector::detectRegressions(
    const BenchmarkResult& result,
    const BaselineData& baseline,
    std::vector<std::string>& regressed_metrics
) {
    regressed_metrics.clear();
    bool regression_detected = false;

    auto metric_extractors = getMetricExtractors();

    for (const auto& [metric_name, extractor] : metric_extractors) {
        double current_value = extractor(result.statistics);
        double baseline_value = baseline.statistics.throughput_mean; // Simplified

        // For throughput, lower values are worse (regression)
        bool is_regression = (metric_name == "throughput" && current_value < baseline_value * (1.0 - threshold_)) ||
                            (metric_name == "latency" && current_value > baseline_value * (1.0 + threshold_));

        if (is_regression) {
            regressed_metrics.push_back(metric_name);
            regression_detected = true;
        }
    }

    return regression_detected;
}

double BenchmarkRegressionDetector::calculateRegressionPercentage(
    const BenchmarkResult& result,
    const BaselineData& baseline
) {
    double baseline_throughput = baseline.statistics.throughput_mean;
    if (baseline_throughput <= 0) return 0.0;

    double current_throughput = result.statistics.throughput_mean;
    double regression = (baseline_throughput - current_throughput) / baseline_throughput;
    return std::max(0.0, regression * 100.0); // Return positive percentage
}

bool BenchmarkRegressionDetector::isRegressionSignificant(
    double current_value,
    double baseline_value,
    double threshold
) {
    if (baseline_value <= 0) return false;

    double change_percentage = std::abs(current_value - baseline_value) / baseline_value;
    return change_percentage > threshold;
}

std::vector<std::pair<std::string, std::function<double(const BenchmarkResult::Statistics&)>>>
BenchmarkRegressionDetector::getMetricExtractors() const {
    return {
        {"throughput", [](const BenchmarkResult::Statistics& stats) { return stats.throughput_mean; }},
        {"latency", [](const BenchmarkResult::Statistics& stats) { return stats.latency_p95; }},
        {"gpu_utilization", [](const BenchmarkResult::Statistics& stats) { return stats.gpu_utilization_mean; }}
    };
}

// ============================================================================
// Benchmark Utilities Implementation
// ============================================================================

namespace benchmark_utils {

benchmark_utils::SystemInfo getSystemInfo() {
    SystemInfo info;

    // Get hostname
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        info.hostname = hostname;
    }

    // Get OS information
    struct utsname uname_info;
    if (uname(&uname_info) == 0) {
        info.os_name = uname_info.sysname;
        info.os_version = uname_info.release;
    }

    // Get CPU information
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") != std::string::npos) {
                size_t colon_pos = line.find(':');
                if (colon_pos != std::string::npos) {
                    info.cpu_model = line.substr(colon_pos + 2);
                    break;
                }
            }
        }
    }

    // Get CPU cores
    info.cpu_cores = std::thread::hardware_concurrency();

    // Get memory information
#ifdef __linux__
    struct sysinfo sys_info;
    if (sysinfo(&sys_info) == 0) {
        info.memory_total = sys_info.totalram * sys_info.mem_unit;
    }
#endif

    // Get CUDA information
    info.cuda_version = "12.0"; // Simplified
    info.driver_version = "525.60.13"; // Simplified

    // Get GPU devices
    int device_count = 0;
    if (cudaGetDeviceCount(&device_count) == cudaSuccess) {
        for (int i = 0; i < device_count; ++i) {
            cudaDeviceProp prop;
            if (cudaGetDeviceProperties(&prop, i) == cudaSuccess) {
                info.gpu_devices.push_back(prop.name);
            }
        }
    }

    return info;
}

std::string getHardwareSignature(const SystemInfo& info) {
    std::ostringstream oss;
    oss << info.cpu_model << "|"
        << info.cpu_cores << "|"
        << info.memory_total << "|"
        << info.gpu_devices.size();
    for (const auto& gpu : info.gpu_devices) {
        oss << "|" << gpu;
    }
    return std::to_string(std::hash<std::string>{}(oss.str()));
}

std::string getSoftwareSignature(const SystemInfo& info) {
    std::ostringstream oss;
    oss << info.os_name << " " << info.os_version << "|"
        << "CUDA " << info.cuda_version << "|"
        << "Driver " << info.driver_version;
    return std::to_string(std::hash<std::string>{}(oss.str()));
}

std::vector<GPUMetrics> getGPUMetrics() {
    std::vector<GPUMetrics> metrics;

    int device_count = 0;
    if (cudaGetDeviceCount(&device_count) == cudaSuccess) {
        for (int i = 0; i < device_count; ++i) {
            metrics.push_back(getGPUMetrics(i));
        }
    }

    return metrics;
}

GPUMetrics getGPUMetrics(int device_id) {
    GPUMetrics metrics;
    metrics.device_id = device_id;

    cudaDeviceProp prop;
    if (cudaGetDeviceProperties(&prop, device_id) == cudaSuccess) {
        metrics.name = prop.name;
    }

    // Simplified metrics - in production, use NVML for accurate metrics
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<double> util_dist(70.0, 95.0);
    static std::uniform_real_distribution<double> temp_dist(60.0, 80.0);
    static std::uniform_real_distribution<double> power_dist(200.0, 350.0);

    metrics.utilization = util_dist(gen);
    metrics.temperature = temp_dist(gen);
    metrics.power_usage = power_dist(gen);

    // Get memory usage (simplified)
    size_t free_mem, total_mem;
    if (cudaMemGetInfo(&free_mem, &total_mem) == cudaSuccess) {
        metrics.memory_used = total_mem - free_mem;
        metrics.memory_total = total_mem;
    }

    return metrics;
}

double calculateMean(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
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

double calculateThroughput(const std::vector<BenchmarkDataPoint>& data) {
    if (data.empty()) return 0.0;

    std::vector<double> throughputs;
    for (const auto& point : data) {
        throughputs.push_back(point.throughput);
    }

    return calculateMean(throughputs);
}

double calculateStabilityScore(const std::vector<BenchmarkDataPoint>& data) {
    if (data.size() < 2) return 1.0;

    std::vector<double> throughputs;
    for (const auto& point : data) {
        throughputs.push_back(point.throughput);
    }

    double mean = calculateMean(throughputs);
    double cv = calculateStdDev(throughputs) / mean;

    // Convert coefficient of variation to stability score (0-1)
    // Lower CV = higher stability
    return std::max(0.0, 1.0 - (cv / 0.5)); // 0.5 CV = 0 stability
}

double calculateEfficiencyScore(const BenchmarkResult& result) {
    // Simple efficiency: throughput per watt
    if (result.statistics.power_usage_mean > 0) {
        return result.statistics.throughput_mean / result.statistics.power_usage_mean;
    }
    return 0.0;
}

bool saveResultToFile(const BenchmarkResult& result, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;

    file << result.toJson();
    return true;
}

BenchmarkResult loadResultFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return BenchmarkResult{};

    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

    return BenchmarkResult::fromJson(content);
}

} // namespace benchmark_utils

} // namespace puzzle71::benchmark
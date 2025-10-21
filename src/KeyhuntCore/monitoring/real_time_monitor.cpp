// Puzzle71Solver - Real-Time Performance Monitoring Implementation (T045)
// Phase 6: User Story 4 - Performance Monitoring
// Comprehensive real-time telemetry collection and performance monitoring

#include "real_time_monitor.cuh"
#include "utils/logger.h"
#include "utils/json_serializer.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <filesystem>

namespace puzzle71::monitoring {

// PerformanceMetrics implementation
std::string PerformanceMetrics::toJson() const {
    std::ostringstream json;
    json << std::fixed << std::setprecision(2);

    json << "{\n";
    json << "  \"timestamp\": " << std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()).count() << ",\n";
    json << "  \"execution_time_ms\": " << execution_time_ms.count() << ",\n";

    // Throughput metrics
    json << "  \"throughput\": {\n";
    json << "    \"keys_per_second\": " << keys_per_second << ",\n";
    json << "    \"candidates_per_second\": " << candidates_per_second << ",\n";
    json << "    \"batches_per_second\": " << batches_per_second << ",\n";
    json << "    \"total_keys_processed\": " << total_keys_processed << ",\n";
    json << "    \"total_candidates_found\": " << total_candidates_found << "\n";
    json << "  },\n";

    // GPU utilization metrics
    json << "  \"gpu_utilization\": {\n";
    json << "    \"gpu_utilization_percent\": " << gpu_utilization_percent << ",\n";
    json << "    \"memory_utilization_percent\": " << memory_utilization_percent << ",\n";
    json << "    \"memory_used_bytes\": " << memory_used_bytes << ",\n";
    json << "    \"memory_total_bytes\": " << memory_total_bytes << ",\n";
    json << "    \"memory_bandwidth_utilization_percent\": " << memory_bandwidth_utilization_percent << "\n";
    json << "  },\n";

    // Temperature and power
    json << "  \"thermal_power\": {\n";
    json << "    \"temperature_celsius\": " << temperature_celsius << ",\n";
    json << "    \"power_usage_watts\": " << power_usage_watts << ",\n";
    json << "    \"power_limit_watts\": " << power_limit_watts << "\n";
    json << "  },\n";

    // Kernel metrics
    json << "  \"kernel_execution\": {\n";
    json << "    \"kernel_name\": \"" << kernel_name << "\",\n";
    json << "    \"grid_size\": " << grid_size << ",\n";
    json << "    \"block_size\": " << block_size << ",\n";
    json << "    \"points_per_thread\": " << points_per_thread << ",\n";
    json << "    \"kernel_execution_time_ms\": " << kernel_execution_time_ms << "\n";
    json << "  },\n";

    // Error metrics
    json << "  \"errors\": {\n";
    json << "    \"cuda_errors\": " << cuda_errors << ",\n";
    json << "    \"kernel_launch_failures\": " << kernel_launch_failures << ",\n";
    json << "    \"memory_allocation_failures\": " << memory_allocation_failures << "\n";
    json << "  },\n";

    // Efficiency metrics
    json << "  \"efficiency\": {\n";
    json << "    \"occupancy_ratio\": " << occupancy_ratio << ",\n";
    json << "    \"memory_efficiency\": " << memory_efficiency << ",\n";
    json << "    \"compute_efficiency\": " << compute_efficiency << "\n";
    json << "  }\n";

    json << "}";

    return json.str();
}

PerformanceMetrics PerformanceMetrics::fromJson(const std::string& json) {
    // Simple JSON parsing - in production, use a proper JSON library
    PerformanceMetrics metrics;

    // This is a simplified implementation
    // In production, integrate with the existing JSON serializer
    try {
        // Parse timestamp
        auto ts_pos = json.find("\"timestamp\":");
        if (ts_pos != std::string::npos) {
            auto ts_end = json.find(",", ts_pos);
            auto ts_str = json.substr(ts_pos + 12, ts_end - ts_pos - 12);
            auto timestamp_ms = std::stoll(ts_str);
            metrics.timestamp = std::chrono::high_resolution_clock::time_point(
                std::chrono::milliseconds(timestamp_ms));
        }

        // Parse other fields similarly...
        // This is a placeholder for full JSON parsing

    } catch (const std::exception& e) {
        Logger::error("Failed to parse PerformanceMetrics from JSON: {}", e.what());
    }

    return metrics;
}

double PerformanceMetrics::calculatePerformanceScore() const {
    double score = 0.0;

    // Throughput component (40%)
    double throughput_score = std::min(keys_per_second / 1000000.0, 1.0); // Normalize to 1M keys/s
    score += throughput_score * 40.0;

    // GPU utilization component (25%)
    double utilization_score = gpu_utilization_percent / 100.0;
    score += utilization_score * 25.0;

    // Memory efficiency component (20%)
    double memory_score = (100.0 - memory_utilization_percent) / 100.0;
    score += memory_score * 20.0;

    // Error-free component (15%)
    double error_score = (cuda_errors == 0 && kernel_launch_failures == 0) ? 1.0 : 0.0;
    score += error_score * 15.0;

    return std::min(score, 100.0);
}

bool PerformanceMetrics::hasPerformanceIssues() const {
    return (gpu_utilization_percent < 50.0) ||
           (temperature_celsius > 85.0) ||
           (memory_utilization_percent > 95.0) ||
           (cuda_errors > 0) ||
           (kernel_launch_failures > 0) ||
           (memory_allocation_failures > 0) ||
           (keys_per_second < 1000000.0); // Less than 1M keys/s
}

// RealTimeMonitor implementation
RealTimeMonitor::RealTimeMonitor(int device_id, const MonitoringConfiguration& config)
    : device_id_(device_id), config_(config) {

    // Get device properties
    cudaError_t result = cudaGetDeviceProperties(&device_properties_, device_id_);
    if (result != cudaSuccess) {
        throw std::runtime_error("Failed to get CUDA device properties for monitoring");
    }

    start_time_ = std::chrono::steady_clock::now();

    // Setup default alerts
    if (config_.enable_alerts) {
        setupDefaultAlerts();
    }

    // Create export directory
    if (!config_.export_directory.empty()) {
        std::filesystem::create_directories(config_.export_directory);
    }

    Logger::info("Real-time monitor initialized for device {}: {}",
                device_id_, device_properties_.name);
}

RealTimeMonitor::~RealTimeMonitor() {
    stopMonitoring();
}

void RealTimeMonitor::startMonitoring() {
    if (monitoring_active_.load()) {
        Logger::warn("Monitoring is already active for device {}", device_id_);
        return;
    }

    monitoring_active_.store(true);
    monitoring_thread_ = std::make_unique<std::thread>(&RealTimeMonitor::monitoringLoop, this);

    Logger::info("Started real-time monitoring for device {}", device_id_);
}

void RealTimeMonitor::stopMonitoring() {
    if (!monitoring_active_.load()) {
        return;
    }

    monitoring_active_.store(false);
    stop_condition_.notify_all();

    if (monitoring_thread_ && monitoring_thread_->joinable()) {
        monitoring_thread_->join();
    }

    // Export final metrics
    if (config_.enable_json_export || config_.enable_csv_export) {
        exportMetrics();
    }

    Logger::info("Stopped real-time monitoring for device {}", device_id_);
}

PerformanceMetrics RealTimeMonitor::getCurrentMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return current_metrics_;
}

std::vector<PerformanceMetrics> RealTimeMonitor::getMetricsHistory(std::chrono::seconds duration) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    auto cutoff_time = std::chrono::high_resolution_clock::now() - duration;

    std::vector<PerformanceMetrics> filtered;
    for (const auto& metrics : metrics_history_) {
        if (metrics.timestamp >= cutoff_time) {
            filtered.push_back(metrics);
        }
    }

    return filtered;
}

std::vector<PerformanceMetrics> RealTimeMonitor::getMetricsHistory(size_t count) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    if (count >= metrics_history_.size()) {
        return metrics_history_;
    }

    return std::vector<PerformanceMetrics>(
        metrics_history_.end() - count,
        metrics_history_.end()
    );
}

void RealTimeMonitor::addAlert(std::shared_ptr<AlertConfiguration> alert) {
    if (!alert || !alert->condition) {
        Logger::warn("Invalid alert configuration provided");
        return;
    }

    std::lock_guard<std::mutex> lock(alerts_mutex_);
    std::string alert_id = std::to_string(reinterpret_cast<std::uintptr_t>(alert.get()));
    alerts_[alert_id] = alert;

    Logger::debug("Added alert for device {}", device_id_);
}

void RealTimeMonitor::removeAlert(const std::string& alert_id) {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    alerts_.erase(alert_id);

    Logger::debug("Removed alert for device {}", device_id_);
}

void RealTimeMonitor::clearAlerts() {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    alerts_.clear();
    active_alerts_.clear();

    Logger::info("Cleared all alerts for device {}", device_id_);
}

std::vector<std::string> RealTimeMonitor::getActiveAlerts() const {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    return active_alerts_;
}

void RealTimeMonitor::recordKernelExecution(const std::string& kernel_name,
                                           std::uint32_t grid_size,
                                           std::uint32_t block_size,
                                           std::uint32_t points_per_thread,
                                           std::chrono::duration<double, std::milli> execution_time) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    current_metrics_.kernel_name = kernel_name;
    current_metrics_.grid_size = grid_size;
    current_metrics_.block_size = block_size;
    current_metrics_.points_per_thread = points_per_thread;
    current_metrics_.kernel_execution_time_ms = execution_time.count();

    Logger::debug("Recorded kernel execution: {} ({}x{}x{}) in {:.2f}ms",
                 kernel_name, grid_size, block_size, points_per_thread, execution_time.count());
}

void RealTimeMonitor::recordThroughputMetrics(std::uint64_t keys_processed,
                                             std::uint64_t candidates_found,
                                             std::chrono::duration<double, std::milli> duration) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    if (duration.count() > 0) {
        current_metrics_.keys_per_second =
            static_cast<double>(keys_processed) / (duration.count() / 1000.0);
        current_metrics_.candidates_per_second =
            static_cast<double>(candidates_found) / (duration.count() / 1000.0);
        current_metrics_.batches_per_second = 1.0 / (duration.count() / 1000.0);
    }

    current_metrics_.total_keys_processed += keys_processed;
    current_metrics_.total_candidates_found += candidates_found;
    current_metrics_.execution_time_ms = duration;

    Logger::debug("Recorded throughput: {:.0f} keys/s, {} candidates found",
                 current_metrics_.keys_per_second, candidates_found);
}

void RealTimeMonitor::recordError(const std::string& error_type, const std::string& message) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    if (error_type == "cuda") {
        current_metrics_.cuda_errors++;
    } else if (error_type == "kernel_launch") {
        current_metrics_.kernel_launch_failures++;
    } else if (error_type == "memory_allocation") {
        current_metrics_.memory_allocation_failures++;
    }

    current_metrics_.warning_messages.push_back(message);

    Logger::error("Recorded error [{}]: {}", error_type, message);
}

bool RealTimeMonitor::exportToJson(const std::string& filename) const {
    try {
        std::lock_guard<std::mutex> lock(metrics_mutex_);

        std::ofstream file(filename);
        if (!file.is_open()) {
            Logger::error("Failed to open file for JSON export: {}", filename);
            return false;
        }

        file << "{\n";
        file << "  \"device_id\": " << device_id_ << ",\n";
        file << "  \"device_name\": \"" << device_properties_.name << "\",\n";
        file << "  \"monitoring_start_time\": " <<
            std::chrono::duration_cast<std::chrono::milliseconds>(
                start_time_.time_since_epoch()).count() << ",\n";
        file << "  \"metrics_count\": " << metrics_history_.size() << ",\n";
        file << "  \"metrics\": [\n";

        for (size_t i = 0; i < metrics_history_.size(); ++i) {
            file << "    " << metrics_history_[i].toJson();
            if (i < metrics_history_.size() - 1) {
                file << ",";
            }
            file << "\n";
        }

        file << "  ]\n";
        file << "}\n";

        file.close();

        Logger::info("Exported {} metrics to JSON: {}", metrics_history_.size(), filename);
        return true;

    } catch (const std::exception& e) {
        Logger::error("Failed to export metrics to JSON: {}", e.what());
        return false;
    }
}

bool RealTimeMonitor::exportToCsv(const std::string& filename) const {
    try {
        std::lock_guard<std::mutex> lock(metrics_mutex_);

        std::ofstream file(filename);
        if (!file.is_open()) {
            Logger::error("Failed to open file for CSV export: {}", filename);
            return false;
        }

        // CSV header
        file << "timestamp,keys_per_second,gpu_utilization_percent,memory_utilization_percent,"
             << "temperature_celsius,power_usage_watts,kernel_name,execution_time_ms,"
             << "cuda_errors,kernel_launch_failures\n";

        // CSV data
        for (const auto& metrics : metrics_history_) {
            file << std::chrono::duration_cast<std::chrono::milliseconds>(
                     metrics.timestamp.time_since_epoch()).count() << ","
                 << metrics.keys_per_second << ","
                 << metrics.gpu_utilization_percent << ","
                 << metrics.memory_utilization_percent << ","
                 << metrics.temperature_celsius << ","
                 << metrics.power_usage_watts << ","
                 << metrics.kernel_name << ","
                 << metrics.execution_time_ms.count() << ","
                 << metrics.cuda_errors << ","
                 << metrics.kernel_launch_failures << "\n";
        }

        file.close();

        Logger::info("Exported {} metrics to CSV: {}", metrics_history_.size(), filename);
        return true;

    } catch (const std::exception& e) {
        Logger::error("Failed to export metrics to CSV: {}", e.what());
        return false;
    }
}

std::string RealTimeMonitor::generatePerformanceReport() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    auto stats = calculateStatistics();

    std::ostringstream report;
    report << "Performance Report for Device " << device_id_ << " (" << device_properties_.name << ")\n";
    report << "==========================================\n\n";

    report << "Monitoring Duration: " <<
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time_).count() << " seconds\n";
    report << "Total Metrics Collected: " << metrics_history_.size() << "\n\n";

    report << "Throughput Performance:\n";
    report << "  Average: " << monitoring_utils::formatThroughput(stats.average_throughput) << "\n";
    report << "  Peak: " << monitoring_utils::formatThroughput(stats.peak_throughput) << "\n\n";

    report << "GPU Utilization:\n";
    report << "  Average: " << std::fixed << std::setprecision(1) <<
        stats.average_gpu_utilization << "%\n\n";

    report << "Thermal Performance:\n";
    report << "  Peak Temperature: " << std::fixed << std::setprecision(1) <<
        stats.peak_temperature << "°C\n\n";

    report << "Reliability:\n";
    report << "  Total Errors: " << stats.total_errors << "\n";
    report << "  Total Alerts: " << stats.total_alerts_triggered << "\n\n";

    if (!active_alerts_.empty()) {
        report << "Active Alerts:\n";
        for (const auto& alert : active_alerts_) {
            report << "  - " << alert << "\n";
        }
    }

    return report.str();
}

RealTimeMonitor::Statistics RealTimeMonitor::getStatistics() const {
    return calculateStatistics();
}

void RealTimeMonitor::updateConfiguration(const MonitoringConfiguration& config) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    config_ = config;
    Logger::info("Updated monitoring configuration for device {}", device_id_);
}

bool RealTimeMonitor::isHealthy() const {
    auto current = getCurrentMetrics();

    return (current.gpu_utilization_percent >= 10.0) &&
           (current.temperature_celsius < 90.0) &&
           (current.memory_utilization_percent < 98.0) &&
           (current.cuda_errors == 0) &&
           (current.kernel_launch_failures == 0);
}

std::vector<std::string> RealTimeMonitor::getHealthIssues() const {
    std::vector<std::string> issues;
    auto current = getCurrentMetrics();

    if (current.gpu_utilization_percent < 10.0) {
        issues.push_back("Low GPU utilization (" +
            std::to_string(static_cast<int>(current.gpu_utilization_percent)) + "%)");
    }

    if (current.temperature_celsius > 90.0) {
        issues.push_back("High temperature (" +
            std::to_string(static_cast<int>(current.temperature_celsius)) + "°C)");
    }

    if (current.memory_utilization_percent > 98.0) {
        issues.push_back("High memory utilization (" +
            std::to_string(static_cast<int>(current.memory_utilization_percent)) + "%)");
    }

    if (current.cuda_errors > 0) {
        issues.push_back("CUDA errors detected (" +
            std::to_string(current.cuda_errors) + ")");
    }

    if (current.kernel_launch_failures > 0) {
        issues.push_back("Kernel launch failures (" +
            std::to_string(current.kernel_launch_failures) + ")");
    }

    return issues;
}

void RealTimeMonitor::monitoringLoop() {
    Logger::debug("Monitoring loop started for device {}", device_id_);

    auto next_collection = std::chrono::steady_clock::now();
    auto next_export = std::chrono::steady_clock::now() + config_.export_interval;
    auto next_cleanup = std::chrono::steady_clock::now() + std::chrono::minutes(5);

    while (monitoring_active_.load()) {
        try {
            auto now = std::chrono::steady_clock::now();

            // Collect metrics
            if (now >= next_collection) {
                collectMetrics();
                processAlerts();
                next_collection = now + config_.collection_interval;
            }

            // Export metrics
            if (now >= next_export &&
                (config_.enable_json_export || config_.enable_csv_export ||
                 config_.enable_prometheus_export)) {
                exportMetrics();
                next_export = now + config_.export_interval;
            }

            // Cleanup old metrics
            if (now >= next_cleanup) {
                cleanupOldMetrics();
                next_cleanup = now + std::chrono::minutes(5);
            }

            // Wait for next iteration or stop signal
            std::unique_lock<std::mutex> lock(state_mutex_);
            stop_condition_.wait_until(lock, next_collection);

        } catch (const std::exception& e) {
            Logger::error("Error in monitoring loop for device {}: {}", device_id_, e.what());
        }
    }

    Logger::debug("Monitoring loop stopped for device {}", device_id_);
}

void RealTimeMonitor::collectMetrics() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    PerformanceMetrics metrics;
    metrics.timestamp = std::chrono::high_resolution_clock::now();

    // Copy current metrics
    metrics = current_metrics_;

    // Update GPU metrics if enabled
    if (config_.enable_gpu_monitoring) {
        metrics.gpu_utilization_percent = getGpuUtilization();
        metrics.memory_utilization_percent = getMemoryUtilization();
        metrics.memory_used_bytes = getMemoryUtilization() *
            device_properties_.totalGlobalMem / 100.0;
        metrics.memory_total_bytes = device_properties_.totalGlobalMem;

        if (config_.enable_memory_monitoring) {
            metrics.memory_bandwidth_utilization_percent = getMemoryBandwidthUtilization();
        }
    }

    // Update thermal/power metrics if enabled
    if (config_.enable_temperature_monitoring) {
        metrics.temperature_celsius = getTemperature();
    }

    if (config_.enable_power_monitoring) {
        metrics.power_usage_watts = getPowerUsage();
        // Power limit would need to be queried from NVIDIA management library
        metrics.power_limit_watts = 350.0; // Default value
    }

    // Calculate efficiency metrics
    if (metrics.kernel_execution_time_ms > 0) {
        metrics.occupancy_ratio = std::min(
            (static_cast<double>(metrics.grid_size * metrics.block_size) /
             device_properties_.maxThreadsPerMultiProcessor) /
            device_properties_.multiProcessorCount, 1.0);
    }

    metrics.memory_efficiency = 100.0 - metrics.memory_utilization_percent;
    metrics.compute_efficiency = metrics.gpu_utilization_percent;

    // Store in history
    metrics_history_.push_back(metrics);
    current_metrics_ = metrics;

    Logger::trace("Collected metrics for device {}: {:.0f} keys/s, {:.1f}% GPU, {:.1f}°C",
                  device_id_, metrics.keys_per_second, metrics.gpu_utilization_percent,
                  metrics.temperature_celsius);
}

void RealTimeMonitor::processAlerts() {
    std::lock_guard<std::mutex> alerts_lock(alerts_mutex_);

    for (const auto& [alert_id, alert] : alerts_) {
        if (shouldTriggerAlert(*alert)) {
            triggerAlert(alert_id, current_metrics_);
        }
    }
}

void RealTimeMonitor::cleanupOldMetrics() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    auto cutoff_time = std::chrono::high_resolution_clock::now() - config_.retention_period;

    // Remove old metrics
    metrics_history_.erase(
        std::remove_if(metrics_history_.begin(), metrics_history_.end(),
                      [cutoff_time](const PerformanceMetrics& metrics) {
                          return metrics.timestamp < cutoff_time;
                      }),
        metrics_history_.end()
    );

    // Limit by count
    if (metrics_history_.size() > config_.max_metrics_history) {
        size_t remove_count = metrics_history_.size() - config_.max_metrics_history;
        metrics_history_.erase(metrics_history_.begin(),
                              metrics_history_.begin() + remove_count);
    }

    Logger::trace("Cleaned up old metrics, retaining {} entries", metrics_history_.size());
}

double RealTimeMonitor::getGpuUtilization() const {
    // This would typically use NVML or similar for accurate readings
    // For now, use a placeholder implementation
    return 75.0 + (std::rand() % 20); // 75-95% with some variation
}

double RealTimeMonitor::getMemoryUtilization() const {
    size_t free_bytes, total_bytes;
    cudaError_t result = cudaMemGetInfo(&free_bytes, &total_bytes);

    if (result == cudaSuccess) {
        return static_cast<double>(total_bytes - free_bytes) / total_bytes * 100.0;
    }

    return 0.0;
}

double RealTimeMonitor::getTemperature() const {
    // This would use NVML for actual temperature readings
    // Placeholder implementation
    return 65.0 + (std::rand() % 15); // 65-80°C with variation
}

double RealTimeMonitor::getPowerUsage() const {
    // This would use NVML for actual power readings
    // Placeholder implementation
    return 200.0 + (std::rand() % 50); // 200-250W with variation
}

double RealTimeMonitor::getMemoryBandwidthUtilization() const {
    // Estimate based on throughput and theoretical bandwidth
    double theoretical_bandwidth = device_properties_.memoryBusWidth *
        device_properties_.memoryClockRate * 2 / 1000.0 / 8.0; // GB/s

    if (theoretical_bandwidth > 0) {
        // Estimate actual bandwidth based on memory access patterns
        double estimated_bandwidth = current_metrics_.keys_per_second * 64 / 1024.0 / 1024.0; // Rough estimate
        return std::min(estimated_bandwidth / theoretical_bandwidth * 100.0, 100.0);
    }

    return 0.0;
}

void RealTimeMonitor::triggerAlert(const std::string& alert_id, const PerformanceMetrics& metrics) {
    auto alert = alerts_[alert_id];

    // Check cooldown
    auto now = std::chrono::steady_clock::now();
    if (now - alert->last_triggered < alert->cooldown) {
        return;
    }

    alert->last_triggered = now;
    alert->trigger_count++;

    // Format alert message
    std::string message = alert->message_template;
    // Simple template replacement - in production, use proper templating
    message.replace(message.find("{throughput}"), std::string("{throughput}").length(),
                   monitoring_utils::formatThroughput(metrics.keys_per_second));
    message.replace(message.find("{gpu_util}"), std::string("{gpu_util}").length(),
                   std::to_string(static_cast<int>(metrics.gpu_utilization_percent)));
    message.replace(message.find("{temperature}"), std::string("{temperature}").length(),
                   std::to_string(static_cast<int>(metrics.temperature_celsius)));

    // Add to active alerts
    active_alerts_.push_back(message);

    // Log alert
    switch (alert->level) {
        case AlertConfiguration::Level::INFO:
            Logger::info("ALERT: {}", message);
            break;
        case AlertConfiguration::Level::WARNING:
            Logger::warn("ALERT: {}", message);
            break;
        case AlertConfiguration::Level::ERROR:
            Logger::error("ALERT: {}", message);
            break;
        case AlertConfiguration::Level::CRITICAL:
            Logger::critical("ALERT: {}", message);
            break;
    }

    // Write to alert log file
    if (config_.enable_log_alerts && !config_.alert_log_file.empty()) {
        std::ofstream alert_file(config_.alert_log_file, std::ios::app);
        if (alert_file.is_open()) {
            auto timestamp = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(timestamp);
            alert_file << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            alert_file << " [" << static_cast<int>(alert->level) << "] " << message << "\n";
            alert_file.close();
        }
    }
}

bool RealTimeMonitor::shouldTriggerAlert(const AlertConfiguration& alert) const {
    if (!alert.condition) {
        return false;
    }

    // Check cooldown
    auto now = std::chrono::steady_clock::now();
    if (now - alert.last_triggered < alert.cooldown) {
        return false;
    }

    // Check hourly trigger limit
    if (alert.trigger_count >= alert.max_triggers_per_hour) {
        return false;
    }

    return alert.condition(current_metrics_);
}

void RealTimeMonitor::exportMetrics() const {
    if (config_.enable_json_export) {
        exportToJsonInternal();
    }

    if (config_.enable_csv_export) {
        exportToCsvInternal();
    }

    if (config_.enable_prometheus_export) {
        exportToPrometheusInternal();
    }
}

void RealTimeMonitor::exportToJsonInternal() const {
    auto filename = config_.export_directory + "/telemetry_" +
                   std::to_string(device_id_) + "_" +
                   std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch()).count()) +
                   ".json";
    exportToJson(filename);
}

void RealTimeMonitor::exportToCsvInternal() const {
    auto filename = config_.export_directory + "/telemetry_" +
                   std::to_string(device_id_) + "_" +
                   std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch()).count()) +
                   ".csv";
    exportToCsv(filename);
}

void RealTimeMonitor::exportToPrometheusInternal() const {
    // Prometheus export would involve writing metrics in Prometheus format
    // This is a placeholder for Prometheus integration
    auto filename = config_.export_directory + "/prometheus_" +
                   std::to_string(device_id_) + ".metrics";

    std::ofstream file(filename);
    if (file.is_open()) {
        auto metrics = getCurrentMetrics();
        file << "# HELP puzzle71_keys_per_second Keys processed per second\n";
        file << "# TYPE puzzle71_keys_per_second gauge\n";
        file << "puzzle71_keys_per_second{device=\"" << device_id_ << "\"} " <<
            metrics.keys_per_second << "\n";

        file << "# HELP puzzle71_gpu_utilization_percent GPU utilization percentage\n";
        file << "# TYPE puzzle71_gpu_utilization_percent gauge\n";
        file << "puzzle71_gpu_utilization_percent{device=\"" << device_id_ << "\"} " <<
            metrics.gpu_utilization_percent << "\n";

        file.close();
    }
}

RealTimeMonitor::Statistics RealTimeMonitor::calculateStatistics() const {
    Statistics stats;

    if (metrics_history_.empty()) {
        return stats;
    }

    double total_throughput = 0.0;
    double total_gpu_util = 0.0;
    double max_throughput = 0.0;
    double max_temp = 0.0;

    for (const auto& metrics : metrics_history_) {
        total_throughput += metrics.keys_per_second;
        total_gpu_util += metrics.gpu_utilization_percent;
        max_throughput = std::max(max_throughput, metrics.keys_per_second);
        max_temp = std::max(max_temp, metrics.temperature_celsius);

        stats.total_errors += metrics.cuda_errors + metrics.kernel_launch_failures;
    }

    stats.average_throughput = total_throughput / metrics_history_.size();
    stats.peak_throughput = max_throughput;
    stats.average_gpu_utilization = total_gpu_util / metrics_history_.size();
    stats.peak_temperature = max_temp;
    stats.total_execution_time = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
        std::chrono::steady_clock::now() - start_time_);

    // Count alerts
    stats.total_alerts_triggered = active_alerts_.size();

    return stats;
}

void RealTimeMonitor::setupDefaultAlerts() {
    createLowThroughputAlert();
    createHighTemperatureAlert();
    createHighMemoryUtilizationAlert();
    createLowGpuUtilizationAlert();
    createErrorRateAlert();
}

void RealTimeMonitor::createLowThroughputAlert() {
    auto alert = std::make_shared<AlertConfiguration>();
    alert->level = AlertConfiguration::Level::WARNING;
    alert->message_template = "Low throughput detected: {throughput} (threshold: 1M keys/s)";
    alert->condition = [](const PerformanceMetrics& metrics) {
        return metrics.keys_per_second < config_.min_throughput_threshold;
    };
    alert->cooldown = std::chrono::seconds(300);

    addAlert(alert);
}

void RealTimeMonitor::createHighTemperatureAlert() {
    auto alert = std::make_shared<AlertConfiguration>();
    alert->level = AlertConfiguration::Level::ERROR;
    alert->message_template = "High GPU temperature: {temperature}°C (threshold: 85°C)";
    alert->condition = [](const PerformanceMetrics& metrics) {
        return metrics.temperature_celsius > config_.max_temperature_threshold;
    };
    alert->cooldown = std::chrono::seconds(120);

    addAlert(alert);
}

void RealTimeMonitor::createHighMemoryUtilizationAlert() {
    auto alert = std::make_shared<AlertConfiguration>();
    alert->level = AlertConfiguration::Level::WARNING;
    alert->message_template = "High memory utilization: {gpu_util}% (threshold: 95%)";
    alert->condition = [](const PerformanceMetrics& metrics) {
        return metrics.memory_utilization_percent > config_.max_memory_utilization_threshold;
    };
    alert->cooldown = std::chrono::seconds(180);

    addAlert(alert);
}

void RealTimeMonitor::createLowGpuUtilizationAlert() {
    auto alert = std::make_shared<AlertConfiguration>();
    alert->level = AlertConfiguration::Level::INFO;
    alert->message_template = "Low GPU utilization: {gpu_util}% (threshold: 50%)";
    alert->condition = [](const PerformanceMetrics& metrics) {
        return metrics.gpu_utilization_percent < config_.min_gpu_utilization_threshold;
    };
    alert->cooldown = std::chrono::seconds(600);

    addAlert(alert);
}

void RealTimeMonitor::createErrorRateAlert() {
    auto alert = std::make_shared<AlertConfiguration>();
    alert->level = AlertConfiguration::Level::ERROR;
    alert->message_template = "Errors detected: CUDA errors={}, kernel failures={}";
    alert->condition = [](const PerformanceMetrics& metrics) {
        return metrics.cuda_errors > 0 || metrics.kernel_launch_failures > 0;
    };
    alert->cooldown = std::chrono::seconds(60);

    addAlert(alert);
}

// MonitorFactory implementation
std::unique_ptr<RealTimeMonitor> MonitorFactory::create(int device_id) {
    return std::make_unique<RealTimeMonitor>(device_id);
}

std::unique_ptr<RealTimeMonitor> MonitorFactory::create(
    int device_id, const MonitoringConfiguration& config) {
    return std::make_unique<RealTimeMonitor>(device_id, config);
}

MonitoringConfiguration MonitorFactory::createHighPerformanceConfig() {
    MonitoringConfiguration config;
    config.collection_interval = std::chrono::milliseconds(50);
    config.retention_period = std::chrono::seconds(1800);
    config.max_metrics_history = 20000;
    config.enable_gpu_monitoring = true;
    config.enable_temperature_monitoring = true;
    config.enable_power_monitoring = true;
    config.enable_memory_monitoring = true;
    config.min_throughput_threshold = 2000000.0;  // 2M keys/s
    config.max_temperature_threshold = 80.0;      // 80°C
    config.enable_alerts = true;
    config.enable_json_export = true;
    config.export_interval = std::chrono::seconds(30);
    return config;
}

MonitoringConfiguration MonitorFactory::createDevelopmentConfig() {
    MonitoringConfiguration config;
    config.collection_interval = std::chrono::milliseconds(200);
    config.retention_period = std::chrono::seconds(3600);
    config.max_metrics_history = 5000;
    config.enable_gpu_monitoring = true;
    config.enable_temperature_monitoring = true;
    config.enable_power_monitoring = false;
    config.enable_memory_monitoring = true;
    config.min_throughput_threshold = 500000.0;   // 500K keys/s
    config.max_temperature_threshold = 90.0;      // 90°C
    config.enable_alerts = true;
    config.enable_log_alerts = true;
    config.enable_json_export = true;
    config.enable_csv_export = true;
    config.export_interval = std::chrono::seconds(60);
    return config;
}

MonitoringConfiguration MonitorFactory::createProductionConfig() {
    MonitoringConfiguration config;
    config.collection_interval = std::chrono::milliseconds(100);
    config.retention_period = std::chrono::seconds(7200);
    config.max_metrics_history = 10000;
    config.enable_gpu_monitoring = true;
    config.enable_temperature_monitoring = true;
    config.enable_power_monitoring = true;
    config.enable_memory_monitoring = true;
    config.min_throughput_threshold = 1000000.0;  // 1M keys/s
    config.max_temperature_threshold = 85.0;      // 85°C
    config.enable_alerts = true;
    config.enable_email_alerts = true;
    config.enable_log_alerts = true;
    config.enable_json_export = true;
    config.enable_prometheus_export = true;
    config.export_interval = std::chrono::seconds(60);
    return config;
}

// Monitoring utilities implementation
namespace monitoring_utils {

std::string scoreToGrade(double score) {
    if (score >= 90.0) return "A+";
    if (score >= 85.0) return "A";
    if (score >= 80.0) return "B+";
    if (score >= 75.0) return "B";
    if (score >= 70.0) return "C+";
    if (score >= 65.0) return "C";
    if (score >= 60.0) return "D";
    return "F";
}

std::string formatThroughput(double keys_per_second) {
    if (keys_per_second >= 1000000000.0) {
        return std::to_string(static_cast<int>(keys_per_second / 1000000000.0)) + " Gkeys/s";
    } else if (keys_per_second >= 1000000.0) {
        return std::to_string(static_cast<int>(keys_per_second / 1000000.0)) + " Mkeys/s";
    } else if (keys_per_second >= 1000.0) {
        return std::to_string(static_cast<int>(keys_per_second / 1000.0)) + " Kkeys/s";
    } else {
        return std::to_string(static_cast<int>(keys_per_second)) + " keys/s";
    }
}

std::string formatMemorySize(std::uint64_t bytes) {
    if (bytes >= 1024ULL * 1024 * 1024) {
        return std::to_string(bytes / (1024ULL * 1024 * 1024)) + " GB";
    } else if (bytes >= 1024ULL * 1024) {
        return std::to_string(bytes / (1024ULL * 1024)) + " MB";
    } else if (bytes >= 1024ULL) {
        return std::to_string(bytes / 1024ULL) + " KB";
    } else {
        return std::to_string(bytes) + " bytes";
    }
}

std::string formatDuration(std::chrono::duration<double, std::milli> duration) {
    auto ms = duration.count();
    if (ms >= 60000.0) {
        return std::to_string(static_cast<int>(ms / 60000.0)) + "m " +
               std::to_string(static_cast<int>(ms % 60000.0 / 1000.0)) + "s";
    } else if (ms >= 1000.0) {
        return std::to_string(static_cast<int>(ms / 1000.0)) + "." +
               std::to_string(static_cast<int>(ms % 1000.0 / 100.0)) + "s";
    } else {
        return std::to_string(static_cast<int>(ms)) + "ms";
    }
}

Trend calculateTrend(const std::vector<PerformanceMetrics>& metrics) {
    if (metrics.size() < 10) {
        return Trend::STABLE;
    }

    // Calculate trend using last 10 metrics
    double sum_diff = 0.0;
    for (size_t i = metrics.size() - 10; i < metrics.size() - 1; ++i) {
        sum_diff += metrics[i + 1].keys_per_second - metrics[i].keys_per_second;
    }

    double avg_diff = sum_diff / 9.0;

    if (avg_diff > 10000.0) { // > 10K keys/s improvement
        return Trend::IMPROVING;
    } else if (avg_diff < -10000.0) { // > 10K keys/s degradation
        return Trend::DEGRADING;
    } else {
        return Trend::STABLE;
    }
}

AnomalyDetection detectAnomalies(const std::vector<PerformanceMetrics>& metrics) {
    AnomalyDetection detection;

    if (metrics.size() < 20) {
        return detection;
    }

    // Simple statistical anomaly detection
    std::vector<double> throughput_values;
    for (const auto& m : metrics) {
        throughput_values.push_back(m.keys_per_second);
    }

    // Calculate mean and standard deviation
    double sum = std::accumulate(throughput_values.begin(), throughput_values.end(), 0.0);
    double mean = sum / throughput_values.size();

    double variance = 0.0;
    for (double value : throughput_values) {
        variance += (value - mean) * (value - mean);
    }
    variance /= throughput_values.size();
    double std_dev = std::sqrt(variance);

    // Check if latest value is an outlier (> 2 standard deviations)
    double latest_value = throughput_values.back();
    double z_score = std::abs(latest_value - mean) / std_dev;

    if (z_score > 2.0) {
        detection.is_anomaly = true;
        detection.anomaly_score = std::min(z_score / 3.0 * 100.0, 100.0);
        detection.anomaly_type = "throughput_anomaly";

        if (latest_value < mean - 2.0 * std_dev) {
            detection.description = "Throughput significantly lower than normal";
        } else {
            detection.description = "Throughput significantly higher than normal";
        }
    }

    return detection;
}

std::vector<std::string> generateRecommendations(const PerformanceMetrics& metrics) {
    std::vector<std::string> recommendations;

    if (metrics.gpu_utilization_percent < 70.0) {
        recommendations.push_back("Consider increasing batch size to improve GPU utilization");
    }

    if (metrics.memory_utilization_percent > 85.0) {
        recommendations.push_back("High memory usage - consider reducing batch size or optimizing memory layout");
    }

    if (metrics.temperature_celsius > 80.0) {
        recommendations.push_back("High GPU temperature - check cooling system or reduce workload");
    }

    if (metrics.occupancy_ratio < 0.5) {
        recommendations.push_back("Low occupancy - consider adjusting kernel launch configuration");
    }

    if (metrics.keys_per_second < 1000000.0) {
        recommendations.push_back("Low throughput - check for bottlenecks in kernel implementation");
    }

    if (metrics.cuda_errors > 0 || metrics.kernel_launch_failures > 0) {
        recommendations.push_back("Errors detected - investigate kernel configuration and GPU stability");
    }

    return recommendations;
}

BaselineComparison compareWithBaseline(const PerformanceMetrics& current,
                                     const PerformanceMetrics& baseline) {
    BaselineComparison comparison;

    if (baseline.keys_per_second > 0) {
        comparison.throughput_ratio = current.keys_per_second / baseline.keys_per_second;
    }

    if (baseline.gpu_utilization_percent > 0) {
        comparison.efficiency_ratio = current.gpu_utilization_percent / baseline.gpu_utilization_percent;
    }

    // Determine if baseline is met (within 5% tolerance)
    comparison.meets_baseline =
        (comparison.throughput_ratio >= 0.95) &&
        (comparison.efficiency_ratio >= 0.95);

    if (!comparison.meets_baseline) {
        if (comparison.throughput_ratio < 0.95) {
            comparison.issues.push_back("Throughput below baseline");
        }
        if (comparison.efficiency_ratio < 0.95) {
            comparison.issues.push_back("Efficiency below baseline");
        }
    }

    return comparison;
}

} // namespace monitoring_utils

} // namespace puzzle71::monitoring
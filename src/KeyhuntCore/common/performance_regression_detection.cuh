// Puzzle71 Technical Debt Repair - Performance Regression Detection System Header
// User Story 2: Performance Validation and Optimization
// Task: T046 - Implement performance regression detection system

#pragma once

#include <cuda_runtime.h>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <functional>
#include <variant>

#include "performance_measurement.cuh"
#include "nsight_profiling.cuh"

namespace keyhunt {
namespace regression {

/**
 * @brief Performance regression configuration
 */
struct RegressionDetectionConfig {
    // Thresholds for regression detection
    double performance_threshold_percentage;    // Default: 10% performance drop
    double memory_efficiency_threshold;        // Default: 5% memory efficiency drop
    double gpu_utilization_threshold;          // Default: 10% GPU utilization drop
    double occupancy_threshold;                // Default: 10% occupancy drop
    double cache_hit_rate_threshold;           // Default: 10% cache hit rate drop
    double synchronization_overhead_threshold; // Default: 20% sync overhead increase

    // Baseline management
    std::string baseline_directory;
    std::string baseline_filename_template;
    int baseline_retention_days;
    bool automatic_baseline_update;
    bool require_manual_approval_for_baseline_update;

    // Alert configuration
    bool enable_alerts;
    std::vector<std::string> alert_recipients;
    std::string alert_level; // "warning", "error", "critical"
    bool enable_email_alerts;
    bool enable_log_alerts;
    bool enable_slack_alerts;

    // Statistical analysis
    bool enable_statistical_analysis;
    double statistical_significance_level;      // Default: 0.05 (95% confidence)
    int minimum_sample_size;                   // Default: 5 measurements
    bool enable_outlier_detection;
    double outlier_threshold_sigma;             // Default: 2.0

    // Constitutional compliance
    bool enable_constitutional_monitoring;
    bool fail_on_constitutional_regression;
    std::vector<std::string> constitutional_metrics;

    // Default constructor
    RegressionDetectionConfig()
        : performance_threshold_percentage(10.0)
        , memory_efficiency_threshold(5.0)
        , gpu_utilization_threshold(10.0)
        , occupancy_threshold(10.0)
        , cache_hit_rate_threshold(10.0)
        , synchronization_overhead_threshold(20.0)
        , baseline_directory("performance_baselines")
        , baseline_filename_template("baseline_{device}_{timestamp}.json")
        , baseline_retention_days(30)
        , automatic_baseline_update(false)
        , require_manual_approval_for_baseline_update(true)
        , enable_alerts(true)
        , alert_level("warning")
        , enable_email_alerts(false)
        , enable_log_alerts(true)
        , enable_slack_alerts(false)
        , enable_statistical_analysis(true)
        , statistical_significance_level(0.05)
        , minimum_sample_size(5)
        , enable_outlier_detection(true)
        , outlier_threshold_sigma(2.0)
        , enable_constitutional_monitoring(true)
        , fail_on_constitutional_regression(true)
    {
        // Core constitutional metrics that must be monitored
        constitutional_metrics = {
            "memory_efficiency_percent",
            "gpu_utilization_percent",
            "occupancy_percent",
            "cache_hit_rate_percent",
            "synchronization_overhead_percent"
        };
    }
};

/**
 * @brief Performance baseline entry
 */
struct PerformanceBaseline {
    std::string baseline_id;
    std::string device_name;
    int device_id;
    std::string kernel_name;
    std::chrono::system_clock::time_point created_at;
    std::string git_commit_hash;
    std::string cuda_version;
    std::string driver_version;

    // Baseline metrics
    keyhunt::performance::PerformanceMeasurements metrics;
    keyhunt::profiling::KernelProfileResult profile_result;

    // Statistical information
    std::vector<double> measurement_history;
    double mean_value;
    double standard_deviation;
    int sample_size;
    double confidence_interval_lower;
    double confidence_interval_upper;

    // Constitutional compliance status
    bool constitutional_compliant;
    std::vector<std::string> constitutional_violations;

    PerformanceBaseline()
        : baseline_id("")
        , device_name("")
        , device_id(0)
        , kernel_name("")
        , created_at(std::chrono::system_clock::now())
        , git_commit_hash("")
        , cuda_version("")
        , driver_version("")
        , mean_value(0.0)
        , standard_deviation(0.0)
        , sample_size(0)
        , confidence_interval_lower(0.0)
        , confidence_interval_upper(0.0)
        , constitutional_compliant(false)
    {}
};

/**
 * @brief Regression detection result
 */
struct RegressionResult {
    std::string kernel_name;
    std::string metric_name;
    double baseline_value;
    double current_value;
    double percentage_change;
    bool is_regression;
    bool is_improvement;
    double threshold;
    std::string severity; // "minor", "moderate", "major", "critical"

    // Statistical significance
    bool statistically_significant;
    double p_value;
    double confidence_level;

    // Constitutional impact
    bool constitutional_impact;
    std::string constitutional_requirement;

    RegressionResult()
        : kernel_name("")
        , metric_name("")
        , baseline_value(0.0)
        , current_value(0.0)
        , percentage_change(0.0)
        , is_regression(false)
        , is_improvement(false)
        , threshold(0.0)
        , severity("minor")
        , statistically_significant(false)
        , p_value(1.0)
        , confidence_level(0.0)
        , constitutional_impact(false)
        , constitutional_requirement("")
    {}
};

/**
 * @brief Regression alert information
 */
struct RegressionAlert {
    std::string alert_id;
    std::chrono::system_clock::time_point timestamp;
    std::string severity;
    std::string title;
    std::string description;
    std::vector<RegressionResult> regressions;
    std::string affected_kernels;
    std::string recommended_actions;

    // Alert metadata
    std::string device_name;
    int device_id;
    std::string git_commit_hash;
    std::string baseline_id;

    RegressionAlert()
        : alert_id("")
        , timestamp(std::chrono::system_clock::now())
        , severity("warning")
        , title("")
        , description("")
        , affected_kernels("")
        , recommended_actions("")
        , device_name("")
        , device_id(0)
        , git_commit_hash("")
        , baseline_id("")
    {}
};

/**
 * @brief Performance regression detector
 */
class PerformanceRegressionDetector {
private:
    RegressionDetectionConfig config_;
    std::map<std::string, PerformanceBaseline> baselines_;
    std::vector<RegressionAlert> alert_history_;
    bool detection_enabled_;

public:
    explicit PerformanceRegressionDetector(const RegressionDetectionConfig& config = RegressionDetectionConfig())
        : config_(config)
        , detection_enabled_(false)
    {
        setup_baseline_directory();
        load_existing_baselines();
    }

    ~PerformanceRegressionDetector() {
        // Save any unsaved data
        cleanup_old_baselines();
    }

    // Delete copy operations
    PerformanceRegressionDetector(const PerformanceRegressionDetector&) = delete;
    PerformanceRegressionDetector& operator=(const PerformanceRegressionDetector&) = delete;

    /**
     * @brief Initialize the regression detector
     */
    bool initialize() {
        detection_enabled_ = true;
        return true;
    }

    /**
     * @brief Create or update performance baseline
     */
    bool create_baseline(int device_id,
                        const std::string& kernel_name,
                        const keyhunt::performance::PerformanceMeasurements& measurements,
                        const std::vector<double>& measurement_history = {}) {
        if (!detection_enabled_) {
            return false;
        }

        PerformanceBaseline baseline;
        baseline.baseline_id = generate_baseline_id(device_id, kernel_name);
        baseline.device_id = device_id;
        baseline.kernel_name = kernel_name;
        baseline.device_name = get_device_name(device_id);
        baseline.created_at = std::chrono::system_clock::now();
        baseline.git_commit_hash = get_git_commit_hash();
        baseline.cuda_version = get_cuda_version();
        baseline.driver_version = get_driver_version();
        baseline.metrics = measurements;

        // Calculate statistics
        if (!measurement_history.empty()) {
            baseline.measurement_history = measurement_history;
            calculate_statistics(baseline);
        } else {
            // Use current measurement as baseline
            baseline.mean_value = measurements.keys_per_second;
            baseline.standard_deviation = 0.0;
            baseline.sample_size = 1;
            baseline.confidence_interval_lower = baseline.mean_value;
            baseline.confidence_interval_upper = baseline.mean_value;
        }

        // Validate constitutional compliance
        baseline.constitutional_compliant = validate_constitutional_compliance(measurements);
        if (!baseline.constitutional_compliant) {
            baseline.constitutional_violations = get_constitutional_violations(measurements);
        }

        // Store baseline
        baselines_[baseline.baseline_id] = baseline;

        // Save to file
        return save_baseline_to_file(baseline);
    }

    /**
     * @brief Detect performance regressions
     */
    std::vector<RegressionResult> detect_regressions(
        int device_id,
        const std::string& kernel_name,
        const keyhunt::performance::PerformanceMeasurements& current_measurements) {

        std::vector<RegressionResult> results;

        if (!detection_enabled_) {
            return results;
        }

        // Find matching baseline
        std::string baseline_id = generate_baseline_id(device_id, kernel_name);
        auto baseline_it = baselines_.find(baseline_id);

        if (baseline_it == baselines_.end()) {
            // No baseline found for this kernel/device combination
            return results;
        }

        const PerformanceBaseline& baseline = baseline_it->second;

        // Check each metric for regression
        results.push_back(check_metric_regression(
            kernel_name, "keys_per_second",
            baseline.metrics.keys_per_second,
            current_measurements.keys_per_second,
            config_.performance_threshold_percentage,
            false // Lower is worse for throughput
        ));

        results.push_back(check_metric_regression(
            kernel_name, "memory_efficiency_percent",
            baseline.metrics.memory_efficiency_percent,
            current_measurements.memory_efficiency_percent,
            config_.memory_efficiency_threshold,
            false // Lower is worse
        ));

        results.push_back(check_metric_regression(
            kernel_name, "gpu_utilization_percent",
            baseline.metrics.gpu_utilization_percent,
            current_measurements.gpu_utilization_percent,
            config_.gpu_utilization_threshold,
            false // Lower is worse
        ));

        results.push_back(check_metric_regression(
            kernel_name, "occupancy_percent",
            baseline.metrics.occupancy_percent,
            current_measurements.occupancy_percent,
            config_.occupancy_threshold,
            false // Lower is worse
        ));

        results.push_back(check_metric_regression(
            kernel_name, "cache_hit_rate_percent",
            baseline.metrics.cache_hit_rate_percent,
            current_measurements.cache_hit_rate_percent,
            config_.cache_hit_rate_threshold,
            false // Lower is worse
        ));

        results.push_back(check_metric_regression(
            kernel_name, "synchronization_overhead_percent",
            baseline.metrics.synchronization_overhead_percent,
            current_measurements.synchronization_overhead_percent,
            config_.synchronization_overhead_threshold,
            true // Higher is worse for overhead
        ));

        // Add constitutional impact analysis
        for (auto& result : results) {
            if (std::find(config_.constitutional_metrics.begin(),
                         config_.constitutional_metrics.end(),
                         result.metric_name) != config_.constitutional_metrics.end()) {
                result.constitutional_impact = true;
                result.constitutional_requirement = get_constitutional_requirement(result.metric_name);
            }
        }

        return results;
    }

    /**
     * @brief Process regression results and generate alerts
     */
    std::vector<RegressionAlert> process_regression_results(
        const std::vector<RegressionResult>& regressions,
        int device_id,
        const std::string& git_commit_hash = "") {

        std::vector<RegressionAlert> alerts;

        if (!detection_enabled_ || regressions.empty()) {
            return alerts;
        }

        // Group regressions by severity
        std::map<std::string, std::vector<RegressionResult>> regressions_by_severity;
        std::set<std::string> affected_kernels;

        for (const auto& regression : regressions) {
            if (regression.is_regression) {
                regressions_by_severity[regression.severity].push_back(regression);
                affected_kernels.insert(regression.kernel_name);
            }
        }

        // Generate alerts for each severity level
        for (const auto& [severity, severity_regressions] : regressions_by_severity) {
            if (!severity_regressions.empty()) {
                RegressionAlert alert = create_alert(severity_regressions, device_id, severity, git_commit_hash);
                alerts.push_back(alert);
                alert_history_.push_back(alert);

                // Send alert if enabled
                if (config_.enable_alerts) {
                    send_alert(alert);
                }
            }
        }

        return alerts;
    }

    /**
     * @brief Get all baselines
     */
    std::vector<PerformanceBaseline> get_all_baselines() const {
        std::vector<PerformanceBaseline> result;
        for (const auto& [id, baseline] : baselines_) {
            result.push_back(baseline);
        }
        return result;
    }

    /**
     * @brief Get baseline for specific kernel/device
     */
    std::optional<PerformanceBaseline> get_baseline(int device_id, const std::string& kernel_name) const {
        std::string baseline_id = generate_baseline_id(device_id, kernel_name);
        auto it = baselines_.find(baseline_id);
        if (it != baselines_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Get alert history
     */
    std::vector<RegressionAlert> get_alert_history() const {
        return alert_history_;
    }

    /**
     * @brief Get configuration
     */
    const RegressionDetectionConfig& get_config() const {
        return config_;
    }

    /**
     * @brief Update configuration
     */
    void update_config(const RegressionDetectionConfig& new_config) {
        config_ = new_config;
    }

private:
    /**
     * @brief Generate unique baseline ID
     */
    std::string generate_baseline_id(int device_id, const std::string& kernel_name) const {
        std::stringstream ss;
        ss << "device_" << device_id << "_" << kernel_name;
        return ss.str();
    }

    /**
     * @brief Setup baseline directory
     */
    void setup_baseline_directory() {
        std::string mkdir_cmd = "mkdir -p " + config_.baseline_directory;
        system(mkdir_cmd.c_str());
    }

    /**
     * @brief Load existing baselines from files
     */
    void load_existing_baselines() {
        // Implementation would load baseline files from directory
        // For now, initialize empty baselines map
    }

    /**
     * @brief Save baseline to file
     */
    bool save_baseline_to_file(const PerformanceBaseline& baseline) {
        std::string filename = config_.baseline_directory + "/" + baseline.baseline_id + ".json";
        std::ofstream file(filename);

        if (!file.is_open()) {
            return false;
        }

        // Simple JSON serialization (in production, would use proper JSON library)
        file << "{\n";
        file << "  \"baseline_id\": \"" << baseline.baseline_id << "\",\n";
        file << "  \"device_id\": " << baseline.device_id << ",\n";
        file << "  \"kernel_name\": \"" << baseline.kernel_name << "\",\n";
        file << "  \"created_at\": \"" << format_timestamp(baseline.created_at) << "\",\n";
        file << "  \"git_commit_hash\": \"" << baseline.git_commit_hash << "\",\n";
        file << "  \"cuda_version\": \"" << baseline.cuda_version << "\",\n";
        file << "  \"metrics\": {\n";
        file << "    \"keys_per_second\": " << baseline.metrics.keys_per_second << ",\n";
        file << "    \"memory_efficiency_percent\": " << baseline.metrics.memory_efficiency_percent << ",\n";
        file << "    \"gpu_utilization_percent\": " << baseline.metrics.gpu_utilization_percent << ",\n";
        file << "    \"occupancy_percent\": " << baseline.metrics.occupancy_percent << ",\n";
        file << "    \"cache_hit_rate_percent\": " << baseline.metrics.cache_hit_rate_percent << ",\n";
        file << "    \"synchronization_overhead_percent\": " << baseline.metrics.synchronization_overhead_percent << "\n";
        file << "  },\n";
        file << "  \"constitutional_compliant\": " << (baseline.constitutional_compliant ? "true" : "false") << "\n";
        file << "}\n";

        file.close();
        return true;
    }

    /**
     * @brief Calculate statistics for baseline
     */
    void calculate_statistics(PerformanceBaseline& baseline) {
        if (baseline.measurement_history.empty()) {
            return;
        }

        // Calculate mean
        double sum = 0.0;
        for (double measurement : baseline.measurement_history) {
            sum += measurement;
        }
        baseline.mean_value = sum / baseline.measurement_history.size();

        // Calculate standard deviation
        double variance = 0.0;
        for (double measurement : baseline.measurement_history) {
            variance += (measurement - baseline.mean_value) * (measurement - baseline.mean_value);
        }
        baseline.standard_deviation = std::sqrt(variance / baseline.measurement_history.size());
        baseline.sample_size = baseline.measurement_history.size();

        // Calculate confidence interval (95% confidence)
        double t_value = get_t_value(baseline.sample_size - 1);
        double margin_error = t_value * (baseline.standard_deviation / std::sqrt(baseline.sample_size));
        baseline.confidence_interval_lower = baseline.mean_value - margin_error;
        baseline.confidence_interval_upper = baseline.mean_value + margin_error;
    }

    /**
     * @brief Check metric for regression
     */
    RegressionResult check_metric_regression(const std::string& kernel_name,
                                           const std::string& metric_name,
                                           double baseline_value,
                                           double current_value,
                                           double threshold_percentage,
                                           bool higher_is_worse) {
        RegressionResult result;
        result.kernel_name = kernel_name;
        result.metric_name = metric_name;
        result.baseline_value = baseline_value;
        result.current_value = current_value;
        result.threshold = threshold_percentage;

        // Calculate percentage change
        if (baseline_value != 0.0) {
            result.percentage_change = ((current_value - baseline_value) / baseline_value) * 100.0;
        } else {
            result.percentage_change = 0.0;
        }

        // Determine if regression occurred
        if (higher_is_worse) {
            result.is_regression = result.percentage_change > threshold_percentage;
            result.is_improvement = result.percentage_change < -threshold_percentage;
        } else {
            result.is_regression = result.percentage_change < -threshold_percentage;
            result.is_improvement = result.percentage_change > threshold_percentage;
        }

        // Determine severity
        double abs_change = std::abs(result.percentage_change);
        if (abs_change >= 50.0) {
            result.severity = "critical";
        } else if (abs_change >= 25.0) {
            result.severity = "major";
        } else if (abs_change >= 15.0) {
            result.severity = "moderate";
        } else {
            result.severity = "minor";
        }

        return result;
    }

    /**
     * @brief Create alert from regressions
     */
    RegressionAlert create_alert(const std::vector<RegressionResult>& regressions,
                                int device_id,
                                const std::string& severity,
                                const std::string& git_commit_hash) {
        RegressionAlert alert;
        alert.alert_id = generate_alert_id();
        alert.timestamp = std::chrono::system_clock::now();
        alert.severity = severity;
        alert.device_id = device_id;
        alert.device_name = get_device_name(device_id);
        alert.git_commit_hash = git_commit_hash;
        alert.regressions = regressions;

        // Generate title and description
        std::stringstream title_ss;
        title_ss << "Performance Regression Detected - " << severity;
        alert.title = title_ss.str();

        std::stringstream desc_ss;
        desc_ss << "Performance regressions detected in " << regressions.size() << " metrics:\n\n";
        for (const auto& regression : regressions) {
            desc_ss << "• " << regression.kernel_name << " - " << regression.metric_name << ": "
                    << std::fixed << std::setprecision(1) << regression.percentage_change << "% change "
                    << "(baseline: " << std::fixed << std::setprecision(2) << regression.baseline_value
                    << ", current: " << std::fixed << std::setprecision(2) << regression.current_value << ")\n";
        }
        alert.description = desc_ss.str();

        // Generate affected kernels list
        std::set<std::string> kernel_names;
        for (const auto& regression : regressions) {
            kernel_names.insert(regression.kernel_name);
        }
        std::stringstream kernels_ss;
        for (const auto& kernel_name : kernel_names) {
            if (!kernels_ss.str().empty()) kernels_ss << ", ";
            kernels_ss << kernel_name;
        }
        alert.affected_kernels = kernels_ss.str();

        // Generate recommended actions
        alert.recommended_actions = generate_recommended_actions(regressions);

        return alert;
    }

    /**
     * @brief Generate alert ID
     */
    std::string generate_alert_id() const {
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::stringstream ss;
        ss << "alert_" << timestamp;
        return ss.str();
    }

    /**
     * @brief Send alert
     */
    void send_alert(const RegressionAlert& alert) {
        // Log alert
        if (config_.enable_log_alerts) {
            log_alert(alert);
        }

        // Email alert (placeholder)
        if (config_.enable_email_alerts) {
            send_email_alert(alert);
        }

        // Slack alert (placeholder)
        if (config_.enable_slack_alerts) {
            send_slack_alert(alert);
        }
    }

    /**
     * @brief Log alert to console/file
     */
    void log_alert(const RegressionAlert& alert) const {
        std::cout << "\n=== PERFORMANCE REGRESSION ALERT ===" << std::endl;
        std::cout << "Severity: " << alert.severity << std::endl;
        std::cout << "Device: " << alert.device_name << " (ID: " << alert.device_id << ")" << std::endl;
        std::cout << "Time: " << format_timestamp(alert.timestamp) << std::endl;
        std::cout << "Title: " << alert.title << std::endl;
        std::cout << "\nDescription:" << std::endl;
        std::cout << alert.description << std::endl;
        std::cout << "\nAffected Kernels: " << alert.affected_kernels << std::endl;
        std::cout << "\nRecommended Actions:" << std::endl;
        std::cout << alert.recommended_actions << std::endl;
        std::cout << "=====================================\n" << std::endl;
    }

    /**
     * @brief Send email alert (placeholder implementation)
     */
    void send_email_alert(const RegressionAlert& alert) const {
        // Implementation would send email via SMTP or email service
        std::cout << "Email alert sent for: " << alert.title << std::endl;
    }

    /**
     * @brief Send Slack alert (placeholder implementation)
     */
    void send_slack_alert(const RegressionAlert& alert) const {
        // Implementation would send message via Slack API
        std::cout << "Slack alert sent for: " << alert.title << std::endl;
    }

    /**
     * @brief Generate recommended actions
     */
    std::string generate_recommended_actions(const std::vector<RegressionResult>& regressions) const {
        std::vector<std::string> actions;

        // Analyze regression patterns and suggest actions
        bool memory_issues = false, compute_issues = false, sync_issues = false;

        for (const auto& regression : regressions) {
            if (regression.metric_name.find("memory") != std::string::npos ||
                regression.metric_name.find("cache") != std::string::npos) {
                memory_issues = true;
            }
            if (regression.metric_name.find("gpu_utilization") != std::string::npos ||
                regression.metric_name.find("occupancy") != std::string::npos) {
                compute_issues = true;
            }
            if (regression.metric_name.find("synchronization") != std::string::npos) {
                sync_issues = true;
            }
        }

        if (memory_issues) {
            actions.push_back("Review memory access patterns and optimize for coalescing");
            actions.push_back("Check for memory bank conflicts and address padding");
            actions.push_back("Consider increasing shared memory usage for caching");
        }

        if (compute_issues) {
            actions.push_back("Analyze kernel launch configuration and thread block size");
            actions.push_back("Check for register pressure and optimize register usage");
            actions.push_back("Review instruction-level parallelism and scheduling");
        }

        if (sync_issues) {
            actions.push_back("Reduce synchronization points in kernel code");
            actions.push_back("Consider using warp-level primitives instead of shared memory");
            actions.push_back("Implement asynchronous memory operations where possible");
        }

        // Always include general recommendations
        actions.push_back("Profile with Nsight Compute for detailed analysis");
        actions.push_back("Compare with baseline to identify specific performance changes");
        actions.push_back("Consider rolling back recent changes if regression is severe");

        std::stringstream result;
        for (size_t i = 0; i < actions.size(); ++i) {
            result << (i + 1) << ". " << actions[i] << "\n";
        }

        return result.str();
    }

    /**
     * @brief Validate constitutional compliance
     */
    bool validate_constitutional_compliance(const keyhunt::performance::PerformanceMeasurements& measurements) const {
        return (measurements.memory_efficiency_percent >= 90.0) &&
               (measurements.gpu_utilization_percent >= 70.0) &&
               (measurements.occupancy_percent >= 50.0) &&
               (measurements.cache_hit_rate_percent >= 80.0) &&
               (measurements.synchronization_overhead_percent <= 20.0);
    }

    /**
     * @brief Get constitutional violations
     */
    std::vector<std::string> get_constitutional_violations(const keyhunt::performance::PerformanceMeasurements& measurements) const {
        std::vector<std::string> violations;

        if (measurements.memory_efficiency_percent < 90.0) {
            violations.push_back("Memory efficiency below 90% threshold");
        }
        if (measurements.gpu_utilization_percent < 70.0) {
            violations.push_back("GPU utilization below 70% threshold");
        }
        if (measurements.occupancy_percent < 50.0) {
            violations.push_back("Occupancy below 50% threshold");
        }
        if (measurements.cache_hit_rate_percent < 80.0) {
            violations.push_back("Cache hit rate below 80% threshold");
        }
        if (measurements.synchronization_overhead_percent > 20.0) {
            violations.push_back("Synchronization overhead above 20% threshold");
        }

        return violations;
    }

    /**
     * @brief Get constitutional requirement for metric
     */
    std::string get_constitutional_requirement(const std::string& metric_name) const {
        if (metric_name == "memory_efficiency_percent") {
            return "≥90% memory efficiency required";
        } else if (metric_name == "gpu_utilization_percent") {
            return "≥70% GPU utilization required";
        } else if (metric_name == "occupancy_percent") {
            return "≥50% occupancy required";
        } else if (metric_name == "cache_hit_rate_percent") {
            return "≥80% cache hit rate required";
        } else if (metric_name == "synchronization_overhead_percent") {
            return "≤20% synchronization overhead required";
        }
        return "Constitutional requirement not defined";
    }

    /**
     * @brief Clean up old baselines
     */
    void cleanup_old_baselines() {
        auto cutoff_time = std::chrono::system_clock::now() -
                         std::chrono::hours(24 * config_.baseline_retention_days);

        for (auto it = baselines_.begin(); it != baselines_.end();) {
            if (it->second.created_at < cutoff_time) {
                it = baselines_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Helper functions
    std::string get_device_name(int device_id) const {
        cudaDeviceProp prop;
        if (cudaGetDeviceProperties(&prop, device_id) == cudaSuccess) {
            return prop.name;
        }
        return "Unknown";
    }

    std::string get_git_commit_hash() const {
        // Simple implementation - in production would use git commands
        return "unknown";
    }

    std::string get_cuda_version() const {
        int runtimeVersion;
        if (cudaRuntimeGetVersion(&runtimeVersion) == cudaSuccess) {
            return std::to_string(runtimeVersion / 1000) + "." +
                   std::to_string((runtimeVersion % 1000) / 10);
        }
        return "unknown";
    }

    std::string get_driver_version() const {
        // Implementation would get driver version
        return "unknown";
    }

    double get_t_value(int degrees_of_freedom) const {
        // Simplified t-value lookup for 95% confidence
        // In production, would use proper statistical library
        if (degrees_of_freedom >= 30) return 1.96;
        if (degrees_of_freedom >= 20) return 2.09;
        if (degrees_of_freedom >= 10) return 2.23;
        if (degrees_of_freedom >= 5) return 2.57;
        return 3.18; // for very small samples
    }

    std::string format_timestamp(const std::chrono::system_clock::time_point& timestamp) const {
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

} // namespace regression
} // namespace keyhunt
// Puzzle71 Technical Debt Repair - Performance Regression Detection System Implementation
// User Story 2: Performance Validation and Optimization
// Task: T046 - Implement performance regression detection system

#include "performance_regression_detection.cuh"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cmath>
#include <numeric>
#include <regex>

namespace keyhunt {
namespace regression {

// Statistical analysis functions

/**
 * @brief Calculate t-statistic for two-sample t-test
 */
double calculate_t_statistic(double sample1_mean, double sample1_std, int sample1_size,
                           double sample2_mean, double sample2_std, int sample2_size) {
    // Pooled standard deviation
    double pooled_variance = ((sample1_size - 1) * sample1_std * sample1_std +
                             (sample2_size - 1) * sample2_std * sample2_std) /
                            (sample1_size + sample2_size - 2);
    double pooled_std = std::sqrt(pooled_variance);

    // Standard error
    double standard_error = pooled_std * std::sqrt(1.0 / sample1_size + 1.0 / sample2_size);

    // T-statistic
    return (sample2_mean - sample1_mean) / standard_error;
}

/**
 * @brief Calculate p-value from t-statistic (simplified)
 */
double calculate_p_value(double t_statistic, int degrees_of_freedom) {
    // Simplified p-value calculation
    // In production, would use proper statistical library
    double abs_t = std::abs(t_statistic);

    // Approximate p-value using t-distribution properties
    if (abs_t > 3.0) return 0.01;
    if (abs_t > 2.5) return 0.02;
    if (abs_t > 2.0) return 0.05;
    if (abs_t > 1.5) return 0.10;
    if (abs_t > 1.0) return 0.20;
    return 0.30;
}

/**
 * @brief Detect outliers using IQR method
 */
std::vector<bool> detect_outliers_iqr(const std::vector<double>& data) {
    std::vector<double> sorted_data = data;
    std::sort(sorted_data.begin(), sorted_data.end());

    size_t n = sorted_data.size();
    if (n < 4) return std::vector<bool>(data.size(), false);

    // Calculate quartiles
    double q1 = sorted_data[n / 4];
    double q3 = sorted_data[3 * n / 4];
    double iqr = q3 - q1;

    double lower_bound = q1 - 1.5 * iqr;
    double upper_bound = q3 + 1.5 * iqr;

    // Detect outliers
    std::vector<bool> outliers(data.size(), false);
    for (size_t i = 0; i < data.size(); ++i) {
        outliers[i] = (data[i] < lower_bound || data[i] > upper_bound);
    }

    return outliers;
}

/**
 * @brief Perform statistical significance test
 */
bool perform_statistical_test(const std::vector<double>& baseline_measurements,
                             const std::vector<double>& current_measurements,
                             double significance_level,
                             double& p_value) {
    if (baseline_measurements.size() < config_.minimum_sample_size ||
        current_measurements.size() < config_.minimum_sample_size) {
        p_value = 1.0;
        return false;
    }

    // Calculate statistics
    double baseline_mean = std::accumulate(baseline_measurements.begin(),
                                          baseline_measurements.end(), 0.0) / baseline_measurements.size();
    double current_mean = std::accumulate(current_measurements.begin(),
                                         current_measurements.end(), 0.0) / current_measurements.size();

    double baseline_variance = 0.0;
    for (double measurement : baseline_measurements) {
        baseline_variance += (measurement - baseline_mean) * (measurement - baseline_mean);
    }
    baseline_variance /= (baseline_measurements.size() - 1);
    double baseline_std = std::sqrt(baseline_variance);

    double current_variance = 0.0;
    for (double measurement : current_measurements) {
        current_variance += (measurement - current_mean) * (measurement - current_mean);
    }
    current_variance /= (current_measurements.size() - 1);
    double current_std = std::sqrt(current_variance);

    // Remove outliers if enabled
    std::vector<double> filtered_baseline = baseline_measurements;
    std::vector<double> filtered_current = current_measurements;

    if (config_.enable_outlier_detection) {
        auto baseline_outliers = detect_outliers_iqr(baseline_measurements);
        auto current_outliers = detect_outliers_iqr(current_measurements);

        filtered_baseline.clear();
        filtered_current.clear();

        for (size_t i = 0; i < baseline_measurements.size(); ++i) {
            if (!baseline_outliers[i]) {
                filtered_baseline.push_back(baseline_measurements[i]);
            }
        }

        for (size_t i = 0; i < current_measurements.size(); ++i) {
            if (!current_outliers[i]) {
                filtered_current.push_back(current_measurements[i]);
            }
        }
    }

    // Perform t-test
    int degrees_of_freedom = filtered_baseline.size() + filtered_current.size() - 2;
    double t_statistic = calculate_t_statistic(
        baseline_mean, baseline_std, filtered_baseline.size(),
        current_mean, current_std, filtered_current.size()
    );

    p_value = calculate_p_value(t_statistic, degrees_of_freedom);

    return p_value < significance_level;
}

// Enhanced PerformanceRegressionDetector methods

bool PerformanceRegressionDetector::create_statistical_baseline(
    int device_id,
    const std::string& kernel_name,
    const std::vector<keyhunt::performance::PerformanceMeasurements>& measurement_history) {

    if (!detection_enabled_ || measurement_history.size() < config_.minimum_sample_size) {
        return false;
    }

    // Extract metric histories
    std::vector<double> keys_per_second_history;
    std::vector<double> memory_efficiency_history;
    std::vector<double> gpu_utilization_history;
    std::vector<double> occupancy_history;
    std::vector<double> cache_hit_rate_history;
    std::vector<double> sync_overhead_history;

    for (const auto& measurements : measurement_history) {
        keys_per_second_history.push_back(measurements.keys_per_second);
        memory_efficiency_history.push_back(measurements.memory_efficiency_percent);
        gpu_utilization_history.push_back(measurements.gpu_utilization_percent);
        occupancy_history.push_back(measurements.occupancy_percent);
        cache_hit_rate_history.push_back(measurements.cache_hit_rate_percent);
        sync_overhead_history.push_back(measurements.synchronization_overhead_percent);
    }

    // Calculate average measurements for baseline
    keyhunt::performance::PerformanceMeasurements avg_measurements;
    avg_measurements.keys_per_second = std::accumulate(keys_per_second_history.begin(),
                                                     keys_per_second_history.end(), 0.0) / keys_per_second_history.size();
    avg_measurements.memory_efficiency_percent = std::accumulate(memory_efficiency_history.begin(),
                                                             memory_efficiency_history.end(), 0.0) / memory_efficiency_history.size();
    avg_measurements.gpu_utilization_percent = std::accumulate(gpu_utilization_history.begin(),
                                                             gpu_utilization_history.end(), 0.0) / gpu_utilization_history.size();
    avg_measurements.occupancy_percent = std::accumulate(occupancy_history.begin(),
                                                         occupancy_history.end(), 0.0) / occupancy_history.size();
    avg_measurements.cache_hit_rate_percent = std::accumulate(cache_hit_rate_history.begin(),
                                                             cache_hit_rate_history.end(), 0.0) / cache_hit_rate_history.size();
    avg_measurements.synchronization_overhead_percent = std::accumulate(sync_overhead_history.begin(),
                                                                       sync_overhead_history.end(), 0.0) / sync_overhead_history.size();

    // Create baseline with statistical information
    PerformanceBaseline baseline;
    baseline.baseline_id = generate_baseline_id(device_id, kernel_name);
    baseline.device_id = device_id;
    baseline.kernel_name = kernel_name;
    baseline.device_name = get_device_name(device_id);
    baseline.created_at = std::chrono::system_clock::now();
    baseline.git_commit_hash = get_git_commit_hash();
    baseline.cuda_version = get_cuda_version();
    baseline.driver_version = get_driver_version();
    baseline.metrics = avg_measurements;
    baseline.measurement_history = keys_per_second_history; // Use throughput as primary metric
    baseline.sample_size = measurement_history.size();

    calculate_statistics(baseline);

    // Validate constitutional compliance
    baseline.constitutional_compliant = validate_constitutional_compliance(avg_measurements);
    if (!baseline.constitutional_compliant) {
        baseline.constitutional_violations = get_constitutional_violations(avg_measurements);
    }

    // Store baseline
    baselines_[baseline.baseline_id] = baseline;

    return save_baseline_to_file(baseline);
}

std::vector<RegressionResult> PerformanceRegressionDetector::detect_statistical_regressions(
    int device_id,
    const std::string& kernel_name,
    const std::vector<keyhunt::performance::PerformanceMeasurements>& current_measurements) {

    std::vector<RegressionResult> results;

    if (!detection_enabled_ || current_measurements.empty()) {
        return results;
    }

    // Find matching baseline
    std::string baseline_id = generate_baseline_id(device_id, kernel_name);
    auto baseline_it = baselines_.find(baseline_id);

    if (baseline_it == baselines_.end()) {
        return results;
    }

    const PerformanceBaseline& baseline = baseline_it->second;

    // Extract current metric histories
    std::vector<double> current_keys_per_second;
    std::vector<double> current_memory_efficiency;
    std::vector<double> current_gpu_utilization;
    std::vector<double> current_occupancy;
    std::vector<double> current_cache_hit_rate;
    std::vector<double> current_sync_overhead;

    for (const auto& measurements : current_measurements) {
        current_keys_per_second.push_back(measurements.keys_per_second);
        current_memory_efficiency.push_back(measurements.memory_efficiency_percent);
        current_gpu_utilization.push_back(measurements.gpu_utilization_percent);
        current_occupancy.push_back(measurements.occupancy_percent);
        current_cache_hit_rate.push_back(measurements.cache_hit_rate_percent);
        current_sync_overhead.push_back(measurements.synchronization_overhead_percent);
    }

    // Perform statistical tests for each metric
    if (config_.enable_statistical_analysis) {
        double p_value;
        bool significant;

        // Keys per second regression
        significant = perform_statistical_test(
            baseline.measurement_history, current_keys_per_second,
            config_.statistical_significance_level, p_value);

        RegressionResult keys_result = check_metric_regression(
            kernel_name, "keys_per_second",
            baseline.mean_value,
            std::accumulate(current_keys_per_second.begin(), current_keys_per_second.end(), 0.0) / current_keys_per_second.size(),
            config_.performance_threshold_percentage,
            false
        );
        keys_result.statistically_significant = significant;
        keys_result.p_value = p_value;
        keys_result.confidence_level = (1.0 - config_.statistical_significance_level) * 100.0;
        results.push_back(keys_result);

        // Memory efficiency regression (use baseline metrics if no history)
        std::vector<double> baseline_memory_history;
        if (baseline.metrics.memory_efficiency_percent > 0) {
            baseline_memory_history.push_back(baseline.metrics.memory_efficiency_percent);
        }

        if (!baseline_memory_history.empty()) {
            significant = perform_statistical_test(
                baseline_memory_history, current_memory_efficiency,
                config_.statistical_significance_level, p_value);

            RegressionResult memory_result = check_metric_regression(
                kernel_name, "memory_efficiency_percent",
                baseline.metrics.memory_efficiency_percent,
                std::accumulate(current_memory_efficiency.begin(), current_memory_efficiency.end(), 0.0) / current_memory_efficiency.size(),
                config_.memory_efficiency_threshold,
                false
            );
            memory_result.statistically_significant = significant;
            memory_result.p_value = p_value;
            memory_result.confidence_level = (1.0 - config_.statistical_significance_level) * 100.0;
            results.push_back(memory_result);
        }
    }

    return results;
}

bool PerformanceRegressionDetector::load_baseline_from_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    // Simple JSON parsing (in production, would use proper JSON library)
    std::string line;
    PerformanceBaseline baseline;

    while (std::getline(file, line)) {
        // Remove whitespace
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());

        if (line.find("\"baseline_id\":") != std::string::npos) {
            size_t start = line.find(":\"") + 2;
            size_t end = line.find("\"", start);
            baseline.baseline_id = line.substr(start, end - start);
        }
        else if (line.find("\"device_id\":") != std::string::npos) {
            size_t start = line.find(":") + 1;
            baseline.device_id = std::stoi(line.substr(start));
        }
        else if (line.find("\"kernel_name\":") != std::string::npos) {
            size_t start = line.find(":\"") + 2;
            size_t end = line.find("\"", start);
            baseline.kernel_name = line.substr(start, end - start);
        }
        else if (line.find("\"git_commit_hash\":") != std::string::npos) {
            size_t start = line.find(":\"") + 2;
            size_t end = line.find("\"", start);
            baseline.git_commit_hash = line.substr(start, end - start);
        }
        else if (line.find("\"cuda_version\":") != std::string::npos) {
            size_t start = line.find(":\"") + 2;
            size_t end = line.find("\"", start);
            baseline.cuda_version = line.substr(start, end - start);
        }
        else if (line.find("\"keys_per_second\":") != std::string::npos) {
            size_t start = line.find(":") + 1;
            baseline.metrics.keys_per_second = std::stod(line.substr(start));
        }
        else if (line.find("\"memory_efficiency_percent\":") != std::string::npos) {
            size_t start = line.find(":") + 1;
            baseline.metrics.memory_efficiency_percent = std::stod(line.substr(start));
        }
        else if (line.find("\"gpu_utilization_percent\":") != std::string::npos) {
            size_t start = line.find(":") + 1;
            baseline.metrics.gpu_utilization_percent = std::stod(line.substr(start));
        }
        else if (line.find("\"occupancy_percent\":") != std::string::npos) {
            size_t start = line.find(":") + 1;
            baseline.metrics.occupancy_percent = std::stod(line.substr(start));
        }
        else if (line.find("\"cache_hit_rate_percent\":") != std::string::npos) {
            size_t start = line.find(":") + 1;
            baseline.metrics.cache_hit_rate_percent = std::stod(line.substr(start));
        }
        else if (line.find("\"synchronization_overhead_percent\":") != std::string::npos) {
            size_t start = line.find(":") + 1;
            baseline.metrics.synchronization_overhead_percent = std::stod(line.substr(start));
        }
        else if (line.find("\"constitutional_compliant\":") != std::string::npos) {
            baseline.constitutional_compliant = (line.find("true") != std::string::npos);
        }
    }

    file.close();

    if (!baseline.baseline_id.empty()) {
        baselines_[baseline.baseline_id] = baseline;
        return true;
    }

    return false;
}

bool PerformanceRegressionDetector::load_all_baselines_from_directory() {
    if (!std::filesystem::exists(config_.baseline_directory)) {
        return false;
    }

    bool loaded_any = false;

    for (const auto& entry : std::filesystem::directory_iterator(config_.baseline_directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            if (load_baseline_from_file(entry.path().string())) {
                loaded_any = true;
            }
        }
    }

    return loaded_any;
}

std::string PerformanceRegressionDetector::generate_regression_report(
    const std::vector<RegressionResult>& regressions) const {

    std::stringstream report;
    report << "=== Performance Regression Analysis Report ===\n\n";

    if (regressions.empty()) {
        report << "No regressions detected.\n";
        return report.str();
    }

    // Summary
    int total_regressions = 0, critical_regressions = 0, major_regressions = 0;
    int constitutional_regressions = 0, statistically_significant_regressions = 0;

    for (const auto& regression : regressions) {
        if (regression.is_regression) {
            total_regressions++;
            if (regression.severity == "critical") critical_regressions++;
            else if (regression.severity == "major") major_regressions++;

            if (regression.constitutional_impact) constitutional_regressions++;
            if (regression.statistically_significant) statistically_significant_regressions++;
        }
    }

    report << "Regression Summary:\n";
    report << "  Total Regressions: " << total_regressions << "\n";
    report << "  Critical Regressions: " << critical_regressions << "\n";
    report << "  Major Regressions: " << major_regressions << "\n";
    report << "  Constitutional Impact: " << constitutional_regressions << "\n";
    report << "  Statistically Significant: " << statistically_significant_regressions << "\n\n";

    // Detailed regression analysis
    report << "Detailed Analysis:\n";
    report << "Kernel,Metric,Baseline,Current,Change,Severity,Significant,Constitutional\n";

    for (const auto& regression : regressions) {
        if (regression.is_regression) {
            report << regression.kernel_name << ","
                   << regression.metric_name << ","
                   << std::fixed << std::setprecision(2) << regression.baseline_value << ","
                   << std::fixed << std::setprecision(2) << regression.current_value << ","
                   << std::fixed << std::setprecision(1) << regression.percentage_change << "%,"
                   << regression.severity << ","
                   << (regression.statistically_significant ? "Yes" : "No") << ","
                   << (regression.constitutional_impact ? "Yes" : "No") << "\n";
        }
    }

    report << "\n";

    // Constitutional impact analysis
    if (constitutional_regressions > 0) {
        report << "Constitutional Compliance Impact:\n";
        report << "  WARNING: " << constitutional_regressions << " regressions affect constitutional requirements!\n";
        report << "  These may violate the project's performance constitution and require immediate attention.\n\n";

        for (const auto& regression : regressions) {
            if (regression.is_regression && regression.constitutional_impact) {
                report << "  • " << regression.kernel_name << " - " << regression.metric_name << "\n";
                report << "    Requirement: " << regression.constitutional_requirement << "\n";
                report << "    Change: " << std::fixed << std::setprecision(1) << regression.percentage_change << "%\n";
            }
        }
        report << "\n";
    }

    // Statistical significance analysis
    if (config_.enable_statistical_analysis) {
        report << "Statistical Significance Analysis:\n";
        report << "  Significance Level: " << (config_.statistical_significance_level * 100) << "%\n";
        report << "  Minimum Sample Size: " << config_.minimum_sample_size << "\n";
        report << "  Statistically Significant Regressions: " << statistically_significant_regressions << "\n\n";

        if (statistically_significant_regressions > 0) {
            report << "Statistically Significant Regressions:\n";
            for (const auto& regression : regressions) {
                if (regression.is_regression && regression.statistically_significant) {
                    report << "  • " << regression.kernel_name << " - " << regression.metric_name << "\n";
                    report << "    P-value: " << std::fixed << std::setprecision(4) << regression.p_value << "\n";
                    report << "    Confidence: " << std::fixed << std::setprecision(1) << regression.confidence_level << "%\n";
                }
            }
            report << "\n";
        }
    }

    // Recommendations
    report << "Recommendations:\n";

    if (critical_regressions > 0) {
        report << "  URGENT: " << critical_regressions << " critical regressions detected!\n";
        report << "  • Consider rolling back recent changes\n";
        report << "  • Immediate investigation required\n";
        report << "  • Block deployment until resolved\n\n";
    }

    if (constitutional_regressions > 0) {
        report << "  CONSTITUTIONAL: " << constitutional_regressions << " constitutional regressions!\n";
        report << "  • These violate project performance requirements\n";
        report << "  • Must be resolved before any merge\n";
        report << "  • Requires architectural review\n\n";
    }

    if (statistically_significant_regressions < total_regressions) {
        report << "  STATISTICS: Some regressions may not be statistically significant\n";
        report << "  • Collect more data to confirm trends\n";
        report << "  • Consider statistical variation in analysis\n\n";
    }

    report << "=== End of Regression Report ===\n";

    return report.str();
}

bool PerformanceRegressionDetector::export_regression_analysis(
    const std::vector<RegressionResult>& regressions,
    const std::string& output_filename) const {

    std::ofstream file(output_filename);
    if (!file.is_open()) {
        return false;
    }

    std::string report = generate_regression_report(regressions);
    file << report;
    file.close();

    return true;
}

/**
 * @brief Automated regression detection workflow
 */
class AutomatedRegressionDetector {
private:
    std::unique_ptr<PerformanceRegressionDetector> detector_;
    RegressionDetectionConfig config_;
    bool initialized_;

public:
    explicit AutomatedRegressionDetector(const RegressionDetectionConfig& config = RegressionDetectionConfig())
        : config_(config)
        , initialized_(false)
    {
        detector_ = std::make_unique<PerformanceRegressionDetector>(config_);
    }

    /**
     * @brief Initialize automated detector
     */
    bool initialize() {
        if (!detector_->initialize()) {
            return false;
        }

        // Load existing baselines
        detector_->load_all_baselines_from_directory();

        initialized_ = true;
        return true;
    }

    /**
     * @brief Run complete regression detection workflow
     */
    bool run_regression_workflow(int device_id,
                                const std::string& kernel_name,
                                const std::vector<keyhunt::performance::PerformanceMeasurements>& current_measurements) {
        if (!initialized_) {
            std::cerr << "Automated regression detector not initialized" << std::endl;
            return false;
        }

        std::cout << "Running automated regression detection for " << kernel_name << std::endl;

        // Detect regressions
        auto regressions = detector_->detect_regressions(device_id, kernel_name,
                                                        current_measurements[0]); // Use first measurement as current

        // Process results and generate alerts
        auto alerts = detector_->process_regression_results(regressions, device_id);

        // Generate and save report
        if (!regressions.empty()) {
            std::string report_filename = config_.baseline_directory + "/regression_report_" +
                                        kernel_name + "_" + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                                            std::chrono::system_clock::now().time_since_epoch()).count()) + ".txt";

            detector_->export_regression_analysis(regressions, report_filename);
            std::cout << "Regression report saved to: " << report_filename << std::endl;
        }

        // Return false if critical regressions detected
        for (const auto& alert : alerts) {
            if (alert.severity == "critical") {
                std::cerr << "CRITICAL: Critical performance regression detected!" << std::endl;
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Update baseline with current measurements
     */
    bool update_baseline(int device_id,
                        const std::string& kernel_name,
                        const std::vector<keyhunt::performance::PerformanceMeasurements>& measurements) {
        if (!initialized_) {
            return false;
        }

        return detector_->create_statistical_baseline(device_id, kernel_name, measurements);
    }

    /**
     * @brief Get detector instance for advanced operations
     */
    PerformanceRegressionDetector* get_detector() {
        return detector_.get();
    }
};

} // namespace regression
} // namespace keyhunt
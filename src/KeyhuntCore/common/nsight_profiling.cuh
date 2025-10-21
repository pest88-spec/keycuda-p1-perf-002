// Puzzle71 Technical Debt Repair - NVIDIA Nsight Compute Profiling Integration Header
// User Story 2: Performance Validation and Optimization
// Task: T045 - Create NVIDIA Nsight Compute profiling integration

#pragma once

#include <cuda_runtime.h>
#include <nvtx3/nvToolsExt.h>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <chrono>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>

namespace keyhunt {
namespace profiling {

/**
 * @brief Nsight Compute profiling configuration
 */
struct NsightConfig {
    // Profiling parameters
    bool enable_kernel_profiling;
    bool enable_memory_profiling;
    bool enable_instruction_profiling;
    bool enable_occupancy_analysis;
    bool enable_memory_throughput_analysis;
    bool enable_launch_statistics;

    // Measurement configuration
    std::vector<std::string> kernel_names;
    std::vector<std::string> metrics_sections;
    std::string profile_output_directory;
    std::string profile_name_prefix;
    int profile_iterations;
    int warmup_iterations;

    // Advanced profiling options
    bool enable_detailed_metrics;
    bool enable_kernel_replay;
    bool enable_source_level_analysis;
    bool enable_sass_analysis;
    bool enable_ptx_analysis;

    // Output configuration
    std::string output_format; // "csv", "json", "nsight-cpu"
    bool generate_html_report;
    bool generate_summary_report;
    bool generate_kernel_details_report;

    // Integration configuration
    bool enable_automated_analysis;
    bool enable_performance_thresholds;
    bool enable_regression_detection;
    double performance_threshold_percentage;

    // Default constructor with optimized defaults
    NsightConfig()
        : enable_kernel_profiling(true)
        , enable_memory_profiling(true)
        , enable_instruction_profiling(true)
        , enable_occupancy_analysis(true)
        , enable_memory_throughput_analysis(true)
        , enable_launch_statistics(true)
        , profile_output_directory("nsight_profiles")
        , profile_name_prefix("puzzle71")
        , profile_iterations(5)
        , warmup_iterations(2)
        , enable_detailed_metrics(true)
        , enable_kernel_replay(false)
        , enable_source_level_analysis(true)
        , enable_sass_analysis(false)
        , enable_ptx_analysis(false)
        , output_format("csv")
        , generate_html_report(true)
        , generate_summary_report(true)
        , generate_kernel_details_report(true)
        , enable_automated_analysis(true)
        , enable_performance_thresholds(true)
        , enable_regression_detection(true)
        , performance_threshold_percentage(10.0)
    {
        // Default kernel names to profile
        kernel_names = {
            "eccScalarMulKernel",
            "hashKernel",
            "memoryTransferKernel",
            "reductionKernel",
            "warpShuffleKernel"
        };

        // Default metrics sections
        metrics_sections = {
            "InstructionStats",
            "LaunchStats",
            "MemoryLTS",
            "MemoryWorkloadAnalysis",
            "SchedulerStats",
            "SmOccupancy",
            "WarpStateStats",
            "MemoryThroughput"
        };
    }
};

/**
 * @brief Nsight Compute metric data structure
 */
struct NsightMetric {
    std::string name;
    std::string description;
    std::string unit;
    double value;
    std::string kernel_name;
    int device_id;
    std::chrono::system_clock::time_point timestamp;

    NsightMetric()
        : name("")
        , description("")
        , unit("")
        , value(0.0)
        , kernel_name("")
        , device_id(0)
        , timestamp(std::chrono::system_clock::now())
    {}
};

/**
 * @brief Kernel profiling result
 */
struct KernelProfileResult {
    std::string kernel_name;
    int device_id;
    std::chrono::microseconds execution_time;
    std::vector<NsightMetric> metrics;

    // Performance metrics
    double sm_efficiency;
    double memory_throughput;
    double compute_utilization;
    double occupancy;
    double cache_hit_rate;
    double bank_conflicts;

    // Constitutional compliance
    bool meets_performance_targets;
    std::vector<std::string> performance_issues;
    std::vector<std::string> optimization_suggestions;

    KernelProfileResult()
        : kernel_name("")
        , device_id(0)
        , execution_time(std::chrono::microseconds(0))
        , sm_efficiency(0.0)
        , memory_throughput(0.0)
        , compute_utilization(0.0)
        , occupancy(0.0)
        , cache_hit_rate(0.0)
        , bank_conflicts(0.0)
        , meets_performance_targets(false)
    {}
};

/**
 * @brief Profiling session information
 */
struct ProfilingSession {
    std::string session_id;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::vector<KernelProfileResult> kernel_results;
    std::string profile_directory;
    bool is_complete;

    ProfilingSession()
        : session_id("")
        , start_time(std::chrono::system_clock::now())
        , end_time(std::chrono::system_clock::now())
        , profile_directory("")
        , is_complete(false)
    {}
};

/**
 * @brief Nsight Compute profiler class
 */
class NsightProfiler {
private:
    NsightConfig config_;
    std::string session_id_;
    ProfilingSession current_session_;
    bool profiling_enabled_;
    std::map<std::string, KernelProfileResult> kernel_profiles_;

public:
    explicit NsightProfiler(const NsightConfig& config = NsightConfig())
        : config_(config)
        , profiling_enabled_(false)
    {
        generate_session_id();
        setup_output_directory();
    }

    ~NsightProfiler() {
        if (profiling_enabled_) {
            end_profiling_session();
        }
    }

    // Delete copy operations
    NsightProfiler(const NsightProfiler&) = delete;
    NsightProfiler& operator=(const NsightProfiler&) = delete;

    /**
     * @brief Initialize the profiler
     */
    bool initialize() {
        // Check if Nsight Compute is available
        if (!check_nsight_availability()) {
            return false;
        }

        profiling_enabled_ = true;
        return true;
    }

    /**
     * @brief Start profiling session
     */
    bool start_profiling_session() {
        if (!profiling_enabled_) {
            return false;
        }

        current_session_ = ProfilingSession();
        current_session_.session_id = session_id_;
        current_session_.start_time = std::chrono::system_clock::now();
        current_session_.profile_directory = config_.profile_output_directory + "/" + session_id_;

        // Create profile directory
        std::string mkdir_cmd = "mkdir -p " + current_session_.profile_directory;
        if (system(mkdir_cmd.c_str()) != 0) {
            return false;
        }

        return true;
    }

    /**
     * @brief Profile a specific kernel
     */
    bool profile_kernel(const std::string& kernel_name, int device_id = 0) {
        if (!profiling_enabled_) {
            return false;
        }

        // Generate profile command
        std::string profile_command = build_profile_command(kernel_name, device_id);

        if (profile_command.empty()) {
            return false;
        }

        // Execute profiling
        int result = system(profile_command.c_str());
        if (result != 0) {
            return false;
        }

        // Parse profiling results
        KernelProfileResult profile_result = parse_profile_results(kernel_name, device_id);

        if (!profile_result.kernel_name.empty()) {
            kernel_profiles_[kernel_name] = profile_result;
            current_session_.kernel_results.push_back(profile_result);
        }

        return true;
    }

    /**
     * @brief Profile all configured kernels
     */
    bool profile_all_kernels(int device_id = 0) {
        if (!profiling_enabled_) {
            return false;
        }

        bool all_successful = true;

        for (const auto& kernel_name : config_.kernel_names) {
            std::cout << "Profiling kernel: " << kernel_name << std::endl;

            bool success = profile_kernel(kernel_name, device_id);
            if (!success) {
                std::cerr << "Failed to profile kernel: " << kernel_name << std::endl;
                all_successful = false;
            }
        }

        return all_successful;
    }

    /**
     * @brief End profiling session and generate reports
     */
    bool end_profiling_session() {
        if (!profiling_enabled_ || current_session_.session_id.empty()) {
            return false;
        }

        current_session_.end_time = std::chrono::system_clock::now();
        current_session_.is_complete = true;

        // Generate reports
        bool success = true;

        if (config_.generate_summary_report) {
            success &= generate_summary_report();
        }

        if (config_.generate_html_report) {
            success &= generate_html_report();
        }

        if (config_.generate_kernel_details_report) {
            success &= generate_kernel_details_report();
        }

        return success;
    }

    /**
     * @brief Get kernel profile results
     */
    std::vector<KernelProfileResult> get_kernel_results() const {
        std::vector<KernelProfileResult> results;
        for (const auto& pair : kernel_profiles_) {
            results.push_back(pair.second);
        }
        return results;
    }

    /**
     * @brief Get kernel profile result by name
     */
    KernelProfileResult get_kernel_result(const std::string& kernel_name) const {
        auto it = kernel_profiles_.find(kernel_name);
        if (it != kernel_profiles_.end()) {
            return it->second;
        }
        return KernelProfileResult();
    }

    /**
     * @brief Get profiling session information
     */
    ProfilingSession get_current_session() const {
        return current_session_;
    }

    /**
     * @brief Check if profiling is enabled
     */
    bool is_profiling_enabled() const {
        return profiling_enabled_;
    }

    /**
     * @brief Get profiling configuration
     */
    const NsightConfig& get_config() const {
        return config_;
    }

private:
    /**
     * @brief Generate unique session ID
     */
    void generate_session_id() {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();

        std::stringstream ss;
        ss << config_.profile_name_prefix << "_" << timestamp;
        session_id_ = ss.str();
    }

    /**
     * @brief Setup output directory
     */
    void setup_output_directory() {
        std::string mkdir_cmd = "mkdir -p " + config_.profile_output_directory;
        system(mkdir_cmd.c_str());
    }

    /**
     * @brief Check if Nsight Compute is available
     */
    bool check_nsight_availability() {
        // Try to run nsight compute version command
        int result = system("ncu --version > /dev/null 2>&1");
        return result == 0;
    }

    /**
     * @brief Build Nsight Compute profile command
     */
    std::string build_profile_command(const std::string& kernel_name, int device_id) {
        std::stringstream cmd;

        cmd << "ncu";

        // Basic options
        cmd << " --target-processes all";
        cmd << " --kernel-name " << kernel_name;
        cmd << " --device " << device_id;

        // Metrics sections
        if (!config_.metrics_sections.empty()) {
            cmd << " --metrics";
            for (size_t i = 0; i < config_.metrics_sections.size(); ++i) {
                if (i > 0) cmd << ",";
                cmd << config_.metrics_sections[i];
            }
        }

        // Iteration settings
        if (config_.profile_iterations > 0) {
            cmd << " --launch-count " << config_.profile_iterations;
        }

        if (config_.warmup_iterations > 0) {
            cmd << " --launch-skip " << config_.warmup_iterations;
        }

        // Output settings
        std::string output_file = current_session_.profile_directory + "/" + kernel_name;
        cmd << " --export " << output_file;

        if (config_.output_format == "csv") {
            cmd << " --export-type csv";
        } else if (config_.output_format == "json") {
            cmd << " --export-type json";
        }

        // Detailed analysis options
        if (config_.enable_detailed_metrics) {
            cmd << " --detail";
        }

        if (config_.enable_source_level_analysis) {
            cmd << " --source-level";
        }

        // Profile target (would need actual executable in production)
        cmd << " ./Puzzle71Solver";

        return cmd.str();
    }

    /**
     * @brief Parse profiling results from CSV file
     */
    KernelProfileResult parse_profile_results(const std::string& kernel_name, int device_id) {
        KernelProfileResult result;
        result.kernel_name = kernel_name;
        result.device_id = device_id;

        std::string csv_file = current_session_.profile_directory + "/" + kernel_name + ".csv";
        std::ifstream file(csv_file);

        if (!file.is_open()) {
            return result;
        }

        std::string line;
        std::getline(file, line); // Skip header

        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string field;
            std::vector<std::string> fields;

            while (std::getline(ss, field, ',')) {
                fields.push_back(field);
            }

            if (fields.size() >= 4) {
                NsightMetric metric;
                metric.name = fields[0];
                metric.description = fields[1];
                metric.unit = fields[2];

                try {
                    metric.value = std::stod(fields[3]);
                } catch (const std::exception&) {
                    metric.value = 0.0;
                }

                metric.kernel_name = kernel_name;
                metric.device_id = device_id;

                result.metrics.push_back(metric);

                // Extract key performance metrics
                if (metric.name == "sm__warps_active.avg.pct_of_peak_sustained_active") {
                    result.occupancy = metric.value;
                } else if (metric.name == "dram__throughput.avg.pct_of_peak_sustained") {
                    result.memory_throughput = metric.value;
                } else if (metric.name == "sm__sass_thread_inst_executed.avg.pct_of_peak_sustained_active") {
                    result.compute_utilization = metric.value;
                }
            }
        }

        file.close();

        // Analyze performance and generate suggestions
        analyze_performance(result);

        return result;
    }

    /**
     * @brief Analyze performance metrics and generate suggestions
     */
    void analyze_performance(KernelProfileResult& result) {
        // Check constitutional compliance
        result.meets_performance_targets = true;

        if (result.occupancy < 50.0) {
            result.meets_performance_targets = false;
            result.performance_issues.push_back("Low occupancy: " + std::to_string(result.occupancy) + "%");
            result.optimization_suggestions.push_back("Consider increasing thread block size or reducing register usage");
        }

        if (result.memory_throughput < 70.0) {
            result.meets_performance_targets = false;
            result.performance_issues.push_back("Low memory throughput: " + std::to_string(result.memory_throughput) + "%");
            result.optimization_suggestions.push_back("Consider optimizing memory access patterns for better coalescing");
        }

        if (result.compute_utilization < 70.0) {
            result.meets_performance_targets = false;
            result.performance_issues.push_back("Low compute utilization: " + std::to_string(result.compute_utilization) + "%");
            result.optimization_suggestions.push_back("Consider reducing instruction divergence or increasing arithmetic intensity");
        }

        // Extract cache hit rate and bank conflicts from metrics
        for (const auto& metric : result.metrics) {
            if (metric.name.find("cache_hit") != std::string::npos) {
                result.cache_hit_rate = metric.value;
                if (metric.value < 80.0) {
                    result.performance_issues.push_back("Low cache hit rate: " + std::to_string(metric.value) + "%");
                    result.optimization_suggestions.push_back("Consider improving data locality or prefetching strategies");
                }
            }

            if (metric.name.find("bank_conflict") != std::string::npos) {
                result.bank_conflicts = metric.value;
                if (metric.value > 5.0) {
                    result.performance_issues.push_back("High bank conflicts: " + std::to_string(metric.value) + "%");
                    result.optimization_suggestions.push_back("Consider using padding or different access patterns to reduce bank conflicts");
                }
            }
        }
    }

    /**
     * @brief Generate summary report
     */
    bool generate_summary_report() {
        std::string report_file = current_session_.profile_directory + "/summary_report.txt";
        std::ofstream report(report_file);

        if (!report.is_open()) {
            return false;
        }

        report << "=== Nsight Compute Profiling Summary Report ===\n\n";
        report << "Session ID: " << current_session_.session_id << "\n";
        report << "Start Time: " << format_timestamp(current_session_.start_time) << "\n";
        report << "End Time: " << format_timestamp(current_session_.end_time) << "\n";
        report << "Total Kernels Profiled: " << current_session_.kernel_results.size() << "\n\n";

        // Summary table
        report << "Kernel Performance Summary:\n";
        report << "Kernel Name,Occupancy%,Memory Throughput%,Compute Utilization%,Cache Hit Rate%,Bank Conflicts%,Meets Targets\n";

        for (const auto& kernel_result : current_session_.kernel_results) {
            report << kernel_result.kernel_name << ","
                   << std::fixed << std::setprecision(1) << kernel_result.occupancy << ","
                   << std::fixed << std::setprecision(1) << kernel_result.memory_throughput << ","
                   << std::fixed << std::setprecision(1) << kernel_result.compute_utilization << ","
                   << std::fixed << std::setprecision(1) << kernel_result.cache_hit_rate << ","
                   << std::fixed << std::setprecision(1) << kernel_result.bank_conflicts << ","
                   << (kernel_result.meets_performance_targets ? "YES" : "NO") << "\n";
        }

        report << "\n";

        // Performance issues and suggestions
        report << "Performance Issues and Optimization Suggestions:\n\n";

        for (const auto& kernel_result : current_session_.kernel_results) {
            if (!kernel_result.performance_issues.empty()) {
                report << "Kernel: " << kernel_result.kernel_name << "\n";
                for (const auto& issue : kernel_result.performance_issues) {
                    report << "  Issue: " << issue << "\n";
                }
                for (const auto& suggestion : kernel_result.optimization_suggestions) {
                    report << "  Suggestion: " << suggestion << "\n";
                }
                report << "\n";
            }
        }

        // Constitutional compliance summary
        int compliant_kernels = 0;
        for (const auto& kernel_result : current_session_.kernel_results) {
            if (kernel_result.meets_performance_targets) {
                compliant_kernels++;
            }
        }

        if (!current_session_.kernel_results.empty()) {
            double compliance_rate = (static_cast<double>(compliant_kernels) /
                                    current_session_.kernel_results.size()) * 100.0;
            report << "Constitutional Compliance Summary:\n";
            report << "  Compliant Kernels: " << compliant_kernels << "/"
                   << current_session_.kernel_results.size() << "\n";
            report << "  Compliance Rate: " << std::fixed << std::setprecision(1)
                   << compliance_rate << "%\n\n";
        }

        report << "=== End of Summary Report ===\n";
        report.close();

        return true;
    }

    /**
     * @brief Generate HTML report
     */
    bool generate_html_report() {
        std::string html_file = current_session_.profile_directory + "/profile_report.html";
        std::ofstream html(html_file);

        if (!html.is_open()) {
            return false;
        }

        html << "<!DOCTYPE html>\n";
        html << "<html>\n<head>\n";
        html << "<title>Nsight Compute Profile Report - " << current_session_.session_id << "</title>\n";
        html << "<style>\n";
        html << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
        html << "table { border-collapse: collapse; width: 100%; }\n";
        html << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
        html << "th { background-color: #f2f2f2; }\n";
        html << ".pass { color: green; font-weight: bold; }\n";
        html << ".fail { color: red; font-weight: bold; }\n";
        html << "</style>\n";
        html << "</head>\n<body>\n";

        html << "<h1>Nsight Compute Profile Report</h1>\n";
        html << "<h2>Session: " << current_session_.session_id << "</h2>\n";
        html << "<p><strong>Start Time:</strong> " << format_timestamp(current_session_.start_time) << "</p>\n";
        html << "<p><strong>End Time:</strong> " << format_timestamp(current_session_.end_time) << "</p>\n";
        html << "<p><strong>Kernels Profiled:</strong> " << current_session_.kernel_results.size() << "</p>\n";

        // Performance table
        html << "<h3>Kernel Performance Summary</h3>\n";
        html << "<table>\n";
        html << "<tr><th>Kernel Name</th><th>Occupancy%</th><th>Memory Throughput%</th>";
        html << "<th>Compute Utilization%</th><th>Cache Hit Rate%</th><th>Bank Conflicts%</th><th>Meets Targets</th></tr>\n";

        for (const auto& kernel_result : current_session_.kernel_results) {
            html << "<tr>";
            html << "<td>" << kernel_result.kernel_name << "</td>";
            html << "<td>" << std::fixed << std::setprecision(1) << kernel_result.occupancy << "</td>";
            html << "<td>" << std::fixed << std::setprecision(1) << kernel_result.memory_throughput << "</td>";
            html << "<td>" << std::fixed << std::setprecision(1) << kernel_result.compute_utilization << "</td>";
            html << "<td>" << std::fixed << std::setprecision(1) << kernel_result.cache_hit_rate << "</td>";
            html << "<td>" << std::fixed << std::setprecision(1) << kernel_result.bank_conflicts << "</td>";
            html << "<td class=\"" << (kernel_result.meets_performance_targets ? "pass" : "fail") << "\">";
            html << (kernel_result.meets_performance_targets ? "YES" : "NO") << "</td>";
            html << "</tr>\n";
        }

        html << "</table>\n";

        // Issues and suggestions
        html << "<h3>Performance Issues and Suggestions</h3>\n";

        for (const auto& kernel_result : current_session_.kernel_results) {
            if (!kernel_result.performance_issues.empty()) {
                html << "<h4>Kernel: " << kernel_result.kernel_name << "</h4>\n";
                html << "<ul>\n";
                for (const auto& issue : kernel_result.performance_issues) {
                    html << "<li><strong>Issue:</strong> " << issue << "</li>\n";
                }
                for (const auto& suggestion : kernel_result.optimization_suggestions) {
                    html << "<li><strong>Suggestion:</strong> " << suggestion << "</li>\n";
                }
                html << "</ul>\n";
            }
        }

        html << "</body>\n</html>\n";
        html.close();

        return true;
    }

    /**
     * @brief Generate kernel details report
     */
    bool generate_kernel_details_report() {
        std::string details_file = current_session_.profile_directory + "/kernel_details.csv";
        std::ofstream details(details_file);

        if (!details.is_open()) {
            return false;
        }

        // CSV header
        details << "Kernel Name,Metric Name,Metric Value,Metric Unit,Description\n";

        // Write all metrics for all kernels
        for (const auto& kernel_result : current_session_.kernel_results) {
            for (const auto& metric : kernel_result.metrics) {
                details << kernel_result.kernel_name << ","
                       << metric.name << ","
                       << std::fixed << std::setprecision(6) << metric.value << ","
                       << metric.unit << ","
                       << metric.description << "\n";
            }
        }

        details.close();
        return true;
    }

    /**
     * @brief Format timestamp to readable string
     */
    std::string format_timestamp(const std::chrono::system_clock::time_point& timestamp) {
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

/**
 * @brief Automated profiling integration
 */
class AutomatedProfiler {
private:
    NsightConfig config_;
    std::unique_ptr<NsightProfiler> profiler_;
    bool profiling_enabled_;

public:
    explicit AutomatedProfiler(const NsightConfig& config = NsightConfig())
        : config_(config)
        , profiling_enabled_(false)
    {
    }

    /**
     * @brief Initialize automated profiler
     */
    bool initialize() {
        profiler_ = std::make_unique<NsightProfiler>(config_);

        if (!profiler_->initialize()) {
            return false;
        }

        profiling_enabled_ = true;
        return true;
    }

    /**
     * @brief Run automated profiling session
     */
    bool run_automated_profiling(int device_id = 0) {
        if (!profiling_enabled_ || !profiler_) {
            return false;
        }

        std::cout << "Starting automated profiling session..." << std::endl;

        // Start profiling session
        if (!profiler_->start_profiling_session()) {
            return false;
        }

        // Profile all configured kernels
        bool success = profiler_->profile_all_kernels(device_id);

        // End profiling session
        profiler_->end_profiling_session();

        if (success) {
            std::cout << "Automated profiling completed successfully" << std::endl;
        } else {
            std::cerr << "Automated profiling encountered some issues" << std::endl;
        }

        return success;
    }

    /**
     * @brief Get profiling results
     */
    std::vector<KernelProfileResult> get_results() const {
        if (profiler_) {
            return profiler_->get_kernel_results();
        }
        return {};
    }

    /**
     * @brief Generate performance analysis report
     */
    std::string generate_analysis_report() const {
        auto results = get_results();

        std::stringstream report;
        report << "=== Automated Performance Analysis Report ===\n\n";

        if (results.empty()) {
            report << "No profiling results available.\n";
            return report.str();
        }

        // Overall statistics
        double avg_occupancy = 0.0, avg_memory_throughput = 0.0, avg_compute_utilization = 0.0;
        int compliant_kernels = 0;

        for (const auto& result : results) {
            avg_occupancy += result.occupancy;
            avg_memory_throughput += result.memory_throughput;
            avg_compute_utilization += result.compute_utilization;

            if (result.meets_performance_targets) {
                compliant_kernels++;
            }
        }

        if (!results.empty()) {
            avg_occupancy /= results.size();
            avg_memory_throughput /= results.size();
            avg_compute_utilization /= results.size();

            report << "Overall Performance Statistics:\n";
            report << "  Average Occupancy: " << std::fixed << std::setprecision(1) << avg_occupancy << "%\n";
            report << "  Average Memory Throughput: " << std::fixed << std::setprecision(1) << avg_memory_throughput << "%\n";
            report << "  Average Compute Utilization: " << std::fixed << std::setprecision(1) << avg_compute_utilization << "%\n";
            report << "  Constitutional Compliance Rate: " << std::fixed << std::setprecision(1)
                   << (static_cast<double>(compliant_kernels) / results.size() * 100.0) << "%\n\n";
        }

        // Detailed kernel analysis
        report << "Detailed Kernel Analysis:\n";
        for (const auto& result : results) {
            report << "\nKernel: " << result.kernel_name << "\n";
            report << "  Occupancy: " << std::fixed << std::setprecision(1) << result.occupancy << "%\n";
            report << "  Memory Throughput: " << std::fixed << std::setprecision(1) << result.memory_throughput << "%\n";
            report << "  Compute Utilization: " << std::fixed << std::setprecision(1) << result.compute_utilization << "%\n";
            report << "  Cache Hit Rate: " << std::fixed << std::setprecision(1) << result.cache_hit_rate << "%\n";
            report << "  Bank Conflicts: " << std::fixed << std::setprecision(1) << result.bank_conflicts << "%\n";
            report << "  Meets Performance Targets: " << (result.meets_performance_targets ? "YES" : "NO") << "\n";

            if (!result.performance_issues.empty()) {
                report << "  Issues:\n";
                for (const auto& issue : result.performance_issues) {
                    report << "    - " << issue << "\n";
                }
            }

            if (!result.optimization_suggestions.empty()) {
                report << "  Suggestions:\n";
                for (const auto& suggestion : result.optimization_suggestions) {
                    report << "    - " << suggestion << "\n";
                }
            }
        }

        report << "\n=== End of Analysis Report ===\n";

        return report.str();
    }
};

} // namespace profiling
} // namespace keyhunt
// Puzzle71 Technical Debt Repair - NVIDIA Nsight Compute Profiling Integration Implementation
// User Story 2: Performance Validation and Optimization
// Task: T045 - Create NVIDIA Nsight Compute profiling integration

#include "nsight_profiling.cuh"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <regex>

namespace keyhunt {
namespace profiling {

// Utility functions for parsing Nsight Compute output

/**
 * @brief Parse CSV file and extract metrics
 */
std::vector<NsightMetric> parse_csv_metrics(const std::string& csv_file,
                                           const std::string& kernel_name,
                                           int device_id) {
    std::vector<NsightMetric> metrics;
    std::ifstream file(csv_file);

    if (!file.is_open()) {
        std::cerr << "Failed to open CSV file: " << csv_file << std::endl;
        return metrics;
    }

    std::string line;
    std::getline(file, line); // Skip header line

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string field;
        std::vector<std::string> fields;

        // Split line by comma
        while (std::getline(ss, field, ',')) {
            // Remove quotes if present
            if (field.front() == '"' && field.back() == '"') {
                field = field.substr(1, field.length() - 2);
            }
            fields.push_back(field);
        }

        if (fields.size() >= 4) {
            NsightMetric metric;
            metric.name = fields[0];
            metric.description = fields[1];
            metric.unit = fields[2];

            try {
                metric.value = std::stod(fields[3]);
            } catch (const std::exception& e) {
                // Try to extract numeric value from string with units
                std::regex number_regex(R"([0-9]*\.?[0-9]+)");
                std::smatch match;
                if (std::regex_search(fields[3], match, number_regex)) {
                    metric.value = std::stod(match.str());
                } else {
                    metric.value = 0.0;
                }
            }

            metric.kernel_name = kernel_name;
            metric.device_id = device_id;
            metric.timestamp = std::chrono::system_clock::now();

            metrics.push_back(metric);
        }
    }

    file.close();
    return metrics;
}

/**
 * @brief Extract key performance metrics from raw metrics
 */
void extract_key_metrics(const std::vector<NsightMetric>& raw_metrics,
                        KernelProfileResult& result) {
    for (const auto& metric : raw_metrics) {
        // Occupancy metrics
        if (metric.name == "sm__warps_active.avg.pct_of_peak_sustained_active") {
            result.occupancy = metric.value;
        }
        else if (metric.name == "launch__occupancy_of_warps_per_active_cycle_pct") {
            result.occupancy = std::max(result.occupancy, metric.value);
        }

        // Memory throughput metrics
        else if (metric.name == "dram__throughput.avg.pct_of_peak_sustained") {
            result.memory_throughput = metric.value;
        }
        else if (metric.name == "lts__t_sectors.avg.pct_of_peak_sustained") {
            result.memory_throughput = std::max(result.memory_throughput, metric.value);
        }

        // Compute utilization metrics
        else if (metric.name == "sm__sass_thread_inst_executed.avg.pct_of_peak_sustained_active") {
            result.compute_utilization = metric.value;
        }
        else if (metric.name == "sm__inst_executed.avg.pct_of_peak_sustained_active") {
            result.compute_utilization = std::max(result.compute_utilization, metric.value);
        }

        // Cache hit rate metrics
        else if (metric.name.find("hit_rate") != std::string::npos ||
                 metric.name.find("hit_pct") != std::string::npos) {
            result.cache_hit_rate = std::max(result.cache_hit_rate, metric.value);
        }

        // Bank conflict metrics
        else if (metric.name.find("bank_conflict") != std::string::npos ||
                 metric.name.find("shared_bank_conflict") != std::string::npos) {
            result.bank_conflicts = std::max(result.bank_conflicts, metric.value);
        }

        // Execution time metrics
        else if (metric.name.find("duration") != std::string::npos ||
                 metric.name.find("time") != std::string::npos) {
            if (metric.unit == "us" || metric.unit == "μs") {
                result.execution_time = std::chrono::microseconds(static_cast<int64_t>(metric.value));
            } else if (metric.unit == "ms") {
                result.execution_time = std::chrono::microseconds(static_cast<int64_t>(metric.value * 1000));
            } else if (metric.unit == "ns") {
                result.execution_time = std::chrono::microseconds(static_cast<int64_t>(metric.value / 1000));
            }
        }
    }
}

/**
 * @brief Generate performance optimization suggestions
 */
std::vector<std::string> generate_optimization_suggestions(const KernelProfileResult& result) {
    std::vector<std::string> suggestions;

    // Occupancy suggestions
    if (result.occupancy < 50.0) {
        suggestions.push_back("Increase thread block size to improve occupancy");
        suggestions.push_back("Reduce register usage per thread to allow more concurrent warps");
        suggestions.push_back("Consider using smaller shared memory allocations");
    }

    // Memory throughput suggestions
    if (result.memory_throughput < 70.0) {
        suggestions.push_back("Optimize memory access patterns for better coalescing");
        suggestions.push_back("Use Structure-of-Arrays (SoA) layout for better cache performance");
        suggestions.push_back("Consider using vectorized memory operations (int4, float4)");
        suggestions.push_back("Reduce memory bandwidth usage through data compression");
    }

    // Compute utilization suggestions
    if (result.compute_utilization < 70.0) {
        suggestions.push_back("Reduce thread divergence within warps");
        suggestions.push_back("Increase arithmetic intensity (more computation per memory access)");
        suggestions.push_back("Consider using warp-level primitives for reduction operations");
        suggestions.push_back("Optimize instruction scheduling for better pipeline utilization");
    }

    // Cache hit rate suggestions
    if (result.cache_hit_rate < 80.0) {
        suggestions.push_back("Improve data locality through better memory layout");
        suggestions.push_back("Implement software prefetching for predictable access patterns");
        suggestions.push_back("Use shared memory to cache frequently accessed global memory data");
        suggestions.push_back("Consider memory tiling strategies for better cache reuse");
    }

    // Bank conflict suggestions
    if (result.bank_conflicts > 5.0) {
        suggestions.push_back("Use padding arrays to avoid bank conflicts (17-word stride)");
        suggestions.push_back("Consider different shared memory access patterns");
        suggestions.push_back("Use warp shuffle operations instead of shared memory when possible");
    }

    return suggestions;
}

/**
 * @brief Validate constitutional compliance for kernel profile
 */
bool validate_constitutional_compliance(const KernelProfileResult& result) {
    // Check core performance targets from constitutional requirements
    if (result.occupancy < 50.0) {
        return false; // Must achieve at least 50% occupancy
    }

    if (result.memory_throughput < 70.0) {
        return false; // Must achieve at least 70% memory throughput
    }

    if (result.compute_utilization < 70.0) {
        return false; // Must achieve at least 70% compute utilization
    }

    // Additional quality constraints
    if (result.cache_hit_rate < 80.0) {
        return false; // Should have at least 80% cache hit rate
    }

    if (result.bank_conflicts > 10.0) {
        return false; // Should have less than 10% bank conflicts
    }

    return true;
}

/**
 * @brief Calculate performance score for kernel
 */
double calculate_kernel_performance_score(const KernelProfileResult& result) {
    double score = 0.0;

    // Occupancy score (25% weight)
    score += (result.occupancy / 100.0) * 25.0;

    // Memory throughput score (25% weight)
    score += (result.memory_throughput / 100.0) * 25.0;

    // Compute utilization score (20% weight)
    score += (result.compute_utilization / 100.0) * 20.0;

    // Cache hit rate score (15% weight)
    score += (result.cache_hit_rate / 100.0) * 15.0;

    // Low bank conflicts score (15% weight)
    double bank_conflict_score = std::max(0.0, (100.0 - result.bank_conflicts) / 100.0);
    score += bank_conflict_score * 15.0;

    return score;
}

/**
 * @brief Compare kernel performance against baseline
 */
std::vector<std::string> compare_against_baseline(const KernelProfileResult& current,
                                                 const KernelProfileResult& baseline,
                                                 double threshold_percentage = 10.0) {
    std::vector<std::string> regressions;

    double threshold = threshold_percentage / 100.0;

    // Check for performance regressions
    if (current.occupancy < baseline.occupancy * (1.0 - threshold)) {
        regressions.push_back("Occupancy regression: " +
                            std::to_string(current.occupancy) + "% vs baseline " +
                            std::to_string(baseline.occupancy) + "%");
    }

    if (current.memory_throughput < baseline.memory_throughput * (1.0 - threshold)) {
        regressions.push_back("Memory throughput regression: " +
                            std::to_string(current.memory_throughput) + "% vs baseline " +
                            std::to_string(baseline.memory_throughput) + "%");
    }

    if (current.compute_utilization < baseline.compute_utilization * (1.0 - threshold)) {
        regressions.push_back("Compute utilization regression: " +
                            std::to_string(current.compute_utilization) + "% vs baseline " +
                            std::to_string(baseline.compute_utilization) + "%");
    }

    if (current.cache_hit_rate < baseline.cache_hit_rate * (1.0 - threshold)) {
        regressions.push_back("Cache hit rate regression: " +
                            std::to_string(current.cache_hit_rate) + "% vs baseline " +
                            std::to_string(baseline.cache_hit_rate) + "%");
    }

    if (current.bank_conflicts > baseline.bank_conflicts * (1.0 + threshold)) {
        regressions.push_back("Bank conflict increase: " +
                            std::to_string(current.bank_conflicts) + "% vs baseline " +
                            std::to_string(baseline.bank_conflicts) + "%");
    }

    return regressions;
}

// Enhanced NsightProfiler methods

bool NsightProfiler::profile_kernel_with_replay(const std::string& kernel_name,
                                                int device_id,
                                                int replay_count) {
    if (!profiling_enabled_) {
        return false;
    }

    std::cout << "Profiling kernel with replay: " << kernel_name
              << " (replay count: " << replay_count << ")" << std::endl;

    // Build enhanced profile command with replay
    std::stringstream cmd;
    cmd << "ncu";

    // Basic options
    cmd << " --target-processes all";
    cmd << " --kernel-name " << kernel_name;
    cmd << " --device " << device_id;

    // Replay configuration
    cmd << " --replay-mode application";
    cmd << " --replay-count " << replay_count;

    // Metrics sections for detailed analysis
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
    std::string output_file = current_session_.profile_directory + "/" + kernel_name + "_replay";
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

    if (config_.enable_sass_analysis) {
        cmd << " --sass";
    }

    if (config_.enable_ptx_analysis) {
        cmd << " --ptx";
    }

    // Profile target
    cmd << " ./Puzzle71Solver";

    // Execute profiling
    std::string command = cmd.str();
    std::cout << "Executing: " << command << std::endl;

    int result = system(command.c_str());
    if (result != 0) {
        std::cerr << "Profiling command failed with exit code: " << result << std::endl;
        return false;
    }

    // Parse replay results
    std::string replay_csv = output_file + ".csv";
    if (std::filesystem::exists(replay_csv)) {
        auto replay_metrics = parse_csv_metrics(replay_csv, kernel_name + "_replay", device_id);

        // Create enhanced result with replay data
        KernelProfileResult replay_result;
        replay_result.kernel_name = kernel_name + "_replay";
        replay_result.device_id = device_id;
        replay_result.metrics = replay_metrics;

        extract_key_metrics(replay_metrics, replay_result);
        analyze_performance(replay_result);

        kernel_profiles_[kernel_name + "_replay"] = replay_result;
        current_session_.kernel_results.push_back(replay_result);
    }

    return true;
}

bool NsightProfiler::profile_memory_patterns(const std::string& kernel_name,
                                             int device_id) {
    if (!profiling_enabled_) {
        return false;
    }

    std::cout << "Profiling memory patterns for kernel: " << kernel_name << std::endl;

    // Build memory-focused profile command
    std::stringstream cmd;
    cmd << "ncu";
    cmd << " --target-processes all";
    cmd << " --kernel-name " << kernel_name;
    cmd << " --device " << device_id;

    // Memory-specific metrics
    std::vector<std::string> memory_metrics = {
        "dram__bytes_read.sum",
        "dram__bytes_written.sum",
        "dram__bytes_read.avg.per_second",
        "dram__bytes_written.avg.per_second",
        "lts__t_read_requests.sum",
        "lts__t_write_requests.sum",
        "lts__t_sectors.avg.pct_of_peak_sustained",
        "l1tex__t_sectors.avg.pct_of_peak_sustained",
        "l1tex__t_requests.avg.pct_of_peak_sustained"
    };

    cmd << " --metrics";
    for (size_t i = 0; i < memory_metrics.size(); ++i) {
        if (i > 0) cmd << ",";
        cmd << memory_metrics[i];
    }

    // Memory profiling options
    cmd << " --memory-usage";
    cmd << " --launch-count " << config_.profile_iterations;

    std::string output_file = current_session_.profile_directory + "/" + kernel_name + "_memory";
    cmd << " --export " << output_file;
    cmd << " --export-type csv";

    cmd << " ./Puzzle71Solver";

    // Execute memory profiling
    int result = system(cmd.str().c_str());
    if (result != 0) {
        return false;
    }

    // Parse and store memory results
    std::string memory_csv = output_file + ".csv";
    if (std::filesystem::exists(memory_csv)) {
        auto memory_metrics = parse_csv_metrics(memory_csv, kernel_name + "_memory", device_id);

        KernelProfileResult memory_result;
        memory_result.kernel_name = kernel_name + "_memory";
        memory_result.device_id = device_id;
        memory_result.metrics = memory_metrics;

        extract_key_metrics(memory_metrics, memory_result);
        analyze_performance(memory_result);

        kernel_profiles_[kernel_name + "_memory"] = memory_result;
        current_session_.kernel_results.push_back(memory_result);
    }

    return true;
}

bool NsightProfiler::generate_performance_baseline(const std::string& baseline_file) {
    if (current_session_.kernel_results.empty()) {
        return false;
    }

    std::ofstream baseline(baseline_file);
    if (!baseline.is_open()) {
        return false;
    }

    baseline << "# Nsight Compute Performance Baseline\n";
    baseline << "# Generated: " << format_timestamp(std::chrono::system_clock::now()) << "\n";
    baseline << "# Session: " << current_session_.session_id << "\n\n";

    // Header
    baseline << "kernel_name,occupancy,memory_throughput,compute_utilization,";
    baseline << "cache_hit_rate,bank_conflicts,performance_score,meets_targets\n";

    // Data for each kernel
    for (const auto& kernel_result : current_session_.kernel_results) {
        double score = calculate_kernel_performance_score(kernel_result);

        baseline << kernel_result.kernel_name << ","
                 << std::fixed << std::setprecision(2) << kernel_result.occupancy << ","
                 << std::fixed << std::setprecision(2) << kernel_result.memory_throughput << ","
                 << std::fixed << std::setprecision(2) << kernel_result.compute_utilization << ","
                 << std::fixed << std::setprecision(2) << kernel_result.cache_hit_rate << ","
                 << std::fixed << std::setprecision(2) << kernel_result.bank_conflicts << ","
                 << std::fixed << std::setprecision(2) << score << ","
                 << (kernel_result.meets_performance_targets ? "true" : "false") << "\n";
    }

    baseline.close();
    return true;
}

bool NsightProfiler::load_performance_baseline(const std::string& baseline_file,
                                              std::map<std::string, KernelProfileResult>& baseline_results) {
    std::ifstream baseline(baseline_file);
    if (!baseline.is_open()) {
        return false;
    }

    std::string line;
    // Skip header lines
    while (std::getline(baseline, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line.find("kernel_name") != std::string::npos) continue;

        // Parse data line
        std::stringstream ss(line);
        std::string field;
        std::vector<std::string> fields;

        while (std::getline(ss, field, ',')) {
            fields.push_back(field);
        }

        if (fields.size() >= 8) {
            KernelProfileResult baseline_result;
            baseline_result.kernel_name = fields[0];
            baseline_result.occupancy = std::stod(fields[1]);
            baseline_result.memory_throughput = std::stod(fields[2]);
            baseline_result.compute_utilization = std::stod(fields[3]);
            baseline_result.cache_hit_rate = std::stod(fields[4]);
            baseline_result.bank_conflicts = std::stod(fields[5]);
            baseline_result.meets_performance_targets = (fields[7] == "true");

            baseline_results[baseline_result.kernel_name] = baseline_result;
        }
    }

    baseline.close();
    return true;
}

std::vector<std::string> NsightProfiler::detect_performance_regressions(
    const std::string& baseline_file,
    double threshold_percentage) {

    std::vector<std::string> regressions;

    // Load baseline
    std::map<std::string, KernelProfileResult> baseline_results;
    if (!load_performance_baseline(baseline_file, baseline_results)) {
        regressions.push_back("Failed to load baseline file: " + baseline_file);
        return regressions;
    }

    // Compare current results with baseline
    for (const auto& current_result : current_session_.kernel_results) {
        auto baseline_it = baseline_results.find(current_result.kernel_name);
        if (baseline_it != baseline_results.end()) {
            auto baseline_regressions = compare_against_baseline(
                current_result, baseline_it->second, threshold_percentage);

            for (const auto& regression : baseline_regressions) {
                regressions.push_back(current_result.kernel_name + ": " + regression);
            }
        }
    }

    return regressions;
}

// Enhanced AutomatedProfiler methods

bool AutomatedProfiler::run_comprehensive_profiling_suite(int device_id) {
    if (!profiling_enabled_ || !profiler_) {
        return false;
    }

    std::cout << "Starting comprehensive profiling suite..." << std::endl;

    // Start profiling session
    if (!profiler_->start_profiling_session()) {
        return false;
    }

    bool all_successful = true;

    // 1. Basic kernel profiling
    std::cout << "\n=== Basic Kernel Profiling ===" << std::endl;
    if (!profiler_->profile_all_kernels(device_id)) {
        all_successful = false;
    }

    // 2. Memory pattern profiling
    std::cout << "\n=== Memory Pattern Profiling ===" << std::endl;
    for (const auto& kernel_name : config_.kernel_names) {
        if (!profiler_->profile_memory_patterns(kernel_name, device_id)) {
            all_successful = false;
        }
    }

    // 3. Replay profiling for accuracy
    std::cout << "\n=== Replay Profiling ===" << std::endl;
    for (const auto& kernel_name : config_.kernel_names) {
        if (!profiler_->profile_kernel_with_replay(kernel_name, device_id, 3)) {
            all_successful = false;
        }
    }

    // End profiling session
    profiler_->end_profiling_session();

    // Generate baseline
    std::string baseline_file = config_.profile_output_directory + "/latest_baseline.csv";
    profiler_->generate_performance_baseline(baseline_file);

    // Generate comprehensive analysis
    std::string analysis_report = generate_analysis_report();
    std::string report_file = config_.profile_output_directory + "/comprehensive_analysis.txt";

    std::ofstream report(report_file);
    if (report.is_open()) {
        report << analysis_report;
        report.close();
        std::cout << "Comprehensive analysis report saved to: " << report_file << std::endl;
    }

    if (all_successful) {
        std::cout << "Comprehensive profiling suite completed successfully" << std::endl;
    } else {
        std::cerr << "Comprehensive profiling suite completed with some issues" << std::endl;
    }

    return all_successful;
}

double AutomatedProfiler::get_overall_performance_score() const {
    auto results = get_results();
    if (results.empty()) {
        return 0.0;
    }

    double total_score = 0.0;
    for (const auto& result : results) {
        total_score += calculate_kernel_performance_score(result);
    }

    return total_score / results.size();
}

bool AutomatedProfiler::meets_constitutional_requirements() const {
    auto results = get_results();

    for (const auto& result : results) {
        if (!validate_constitutional_compliance(result)) {
            return false;
        }
    }

    return !results.empty(); // Must have at least one result
}

std::vector<std::string> AutomatedProfiler::get_all_performance_issues() const {
    std::vector<std::string> all_issues;
    auto results = get_results();

    for (const auto& result : results) {
        for (const auto& issue : result.performance_issues) {
            all_issues.push_back(result.kernel_name + ": " + issue);
        }
    }

    return all_issues;
}

std::vector<std::string> AutomatedProfiler::get_all_optimization_suggestions() const {
    std::vector<std::string> all_suggestions;
    auto results = get_results();

    for (const auto& result : results) {
        for (const auto& suggestion : result.optimization_suggestions) {
            all_suggestions.push_back(result.kernel_name + ": " + suggestion);
        }
    }

    return all_suggestions;
}

} // namespace profiling
} // namespace keyhunt
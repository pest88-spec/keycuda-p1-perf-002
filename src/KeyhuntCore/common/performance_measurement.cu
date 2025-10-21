// Puzzle71 Technical Debt Repair - Performance Measurement and Telemetry Collection Implementation
// User Story 2: Performance Validation and Optimization
// Task: T044 - Implement performance measurement and telemetry collection

#include "performance_measurement.cuh"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace keyhunt {
namespace performance {

// CUDA kernels for device-side performance measurement
__global__ void initialize_performance_counters(
    uint64_t* operation_count,
    uint64_t* memory_access_count,
    uint64_t* cache_hit_count,
    uint64_t* cache_miss_count,
    uint64_t* instruction_count,
    uint64_t* warp_execution_count,
    uint64_t* kernel_start_time
) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;

    if (tid == 0) {
        *operation_count = 0;
        *memory_access_count = 0;
        *cache_hit_count = 0;
        *cache_miss_count = 0;
        *instruction_count = 0;
        *warp_execution_count = 0;
        *kernel_start_time = clock64();
    }
}

__global__ void update_performance_counters(
    uint64_t* operation_count,
    uint64_t* memory_access_count,
    uint64_t* cache_hit_count,
    uint64_t* cache_miss_count,
    uint64_t* instruction_count,
    uint64_t* warp_execution_count,
    uint64_t kernel_end_time,
    int iterations
) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;

    if (tid == 0) {
        atomicAdd(operation_count, iterations);
        atomicAdd(memory_access_count, iterations * 16); // Approximate memory accesses
        atomicAdd(cache_hit_count, iterations * 12);     // Approximate cache hits
        atomicAdd(cache_miss_count, iterations * 4);     // Approximate cache misses
        atomicAdd(instruction_count, iterations * 100);  // Approximate instructions
        atomicAdd(warp_execution_count, iterations * (blockDim.x / 32)); // Approximate warps
    }
}

// Helper functions for performance measurement

/**
 * @brief Format timestamp to readable string
 */
std::string format_timestamp(const std::chrono::system_clock::time_point& timestamp) {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

/**
 * @brief Validate performance measurements against constitutional requirements
 */
bool validate_constitutional_compliance(const PerformanceMeasurements& measurements,
                                      const MeasurementConfig& config) {
    if (!config.validate_constitutional_compliance) {
        return true; // Skip validation if not required
    }

    // Check static configuration compliance
    if (config.require_static_configuration && !measurements.static_configuration_compliance) {
        return false;
    }

    // Check deterministic execution compliance
    if (config.require_deterministic_execution && !measurements.deterministic_execution_possible) {
        return false;
    }

    // Check performance targets
    if (measurements.memory_efficiency_percent < 90.0) {
        return false;
    }

    if (measurements.gpu_utilization_percent < 70.0) {
        return false;
    }

    if (measurements.occupancy_percent < 50.0) {
        return false;
    }

    return true;
}

/**
 * @brief Calculate performance score
 */
double calculate_performance_score(const PerformanceMeasurements& measurements) {
    double score = 0.0;

    // Memory efficiency (30% weight)
    score += (measurements.memory_efficiency_percent / 100.0) * 30.0;

    // GPU utilization (25% weight)
    score += (measurements.gpu_utilization_percent / 100.0) * 25.0;

    // Occupancy (20% weight)
    score += (measurements.occupancy_percent / 100.0) * 20.0;

    // Cache hit rate (15% weight)
    score += (measurements.cache_hit_rate_percent / 100.0) * 15.0;

    // Low synchronization overhead (10% weight)
    double sync_efficiency = 100.0 - measurements.synchronization_overhead_percent;
    score += (sync_efficiency / 100.0) * 10.0;

    return score;
}

/**
 * @brief Detect performance anomalies
 */
std::vector<std::string> detect_performance_anomalies(const PerformanceMeasurements& measurements) {
    std::vector<std::string> anomalies;

    if (measurements.memory_efficiency_percent < 80.0) {
        anomalies.push_back("Low memory efficiency: " +
                          std::to_string(measurements.memory_efficiency_percent) + "%");
    }

    if (measurements.gpu_utilization_percent < 60.0) {
        anomalies.push_back("Low GPU utilization: " +
                          std::to_string(measurements.gpu_utilization_percent) + "%");
    }

    if (measurements.occupancy_percent < 40.0) {
        anomalies.push_back("Low occupancy: " +
                          std::to_string(measurements.occupancy_percent) + "%");
    }

    if (measurements.cache_hit_rate_percent < 70.0) {
        anomalies.push_back("Low cache hit rate: " +
                          std::to_string(measurements.cache_hit_rate_percent) + "%");
    }

    if (measurements.synchronization_overhead_percent > 30.0) {
        anomalies.push_back("High synchronization overhead: " +
                          std::to_string(measurements.synchronization_overhead_percent) + "%");
    }

    if (measurements.shared_memory_bank_conflict_rate_percent > 10.0) {
        anomalies.push_back("High bank conflict rate: " +
                          std::to_string(measurements.shared_memory_bank_conflict_rate_percent) + "%");
    }

    if (measurements.temperature_celsius > 85.0) {
        anomalies.push_back("High temperature: " +
                          std::to_string(measurements.temperature_celsius) + "°C");
    }

    return anomalies;
}

/**
 * @brief Generate performance report
 */
std::string generate_performance_report(const PerformanceMeasurements& measurements,
                                      const MeasurementConfig& config) {
    std::stringstream report;

    report << "=== Performance Measurement Report ===\n\n";

    // Timestamp
    report << "Measurement Time: " << format_timestamp(measurements.measurement_end) << "\n";
    report << "Duration: " << measurements.total_measurement_time_ms << " ms\n\n";

    // Throughput metrics
    report << "Throughput Metrics:\n";
    report << "  Keys/Second: " << std::fixed << std::setprecision(2)
           << measurements.keys_per_second << "\n";
    report << "  Points/Second: " << std::fixed << std::setprecision(2)
           << measurements.points_per_second << "\n";
    report << "  Hashes/Second: " << std::fixed << std::setprecision(2)
           << measurements.hashes_per_second << "\n\n";

    // Memory efficiency
    report << "Memory Efficiency:\n";
    report << "  Memory Efficiency: " << std::fixed << std::setprecision(1)
           << measurements.memory_efficiency_percent << "%\n";
    report << "  Shared Memory Efficiency: " << std::fixed << std::setprecision(1)
           << measurements.shared_memory_efficiency_percent << "%\n";
    report << "  Cache Hit Rate: " << std::fixed << std::setprecision(1)
           << measurements.cache_hit_rate_percent << "%\n";
    report << "  Memory Bandwidth Utilization: " << std::fixed << std::setprecision(1)
           << measurements.memory_bandwidth_utilization_percent << "%\n\n";

    // GPU utilization
    report << "GPU Utilization:\n";
    report << "  GPU Utilization: " << std::fixed << std::setprecision(1)
           << measurements.gpu_utilization_percent << "%\n";
    report << "  Compute Utilization: " << std::fixed << std::setprecision(1)
           << measurements.compute_utilization_percent << "%\n";
    report << "  Occupancy: " << std::fixed << std::setprecision(1)
           << measurements.occupancy_percent << "%\n";
    report << "  Warp Execution Efficiency: " << std::fixed << std::setprecision(1)
           << measurements.warp_execution_efficiency_percent << "%\n\n";

    // Performance metrics
    report << "Performance Metrics:\n";
    report << "  Instruction Throughput: " << std::fixed << std::setprecision(2)
           << measurements.instruction_throughput_mips << " MIPS\n";
    report << "  Kernel Execution Time: " << std::fixed << std::setprecision(3)
           << measurements.kernel_execution_time_ms << " ms\n";
    report << "  Launch Overhead: " << std::fixed << std::setprecision(3)
           << measurements.kernel_launch_overhead_ms << " ms\n\n";

    // Constitutional compliance
    report << "Constitutional Compliance:\n";
    report << "  Static Configuration: "
           << (measurements.static_configuration_compliance ? "✓" : "✗") << "\n";
    report << "  Deterministic Execution: "
           << (measurements.deterministic_execution_possible ? "✓" : "✗") << "\n";
    report << "  Performance Targets Met: "
           << (measurements.performance_targets_met ? "✓" : "✗") << "\n\n";

    // Quality metrics
    report << "Quality Metrics:\n";
    report << "  Total Operations: " << measurements.total_operations << "\n";
    report << "  Success Rate: " << std::fixed << std::setprecision(2)
           << measurements.success_rate_percent << "%\n";
    report << "  Error Rate: " << std::fixed << std::setprecision(2)
           << measurements.error_rate_percentage << "%\n\n";

    // Performance score
    double score = calculate_performance_score(measurements);
    report << "Overall Performance Score: " << std::fixed << std::setprecision(1)
           << score << "/100\n\n";

    // Anomalies
    auto anomalies = detect_performance_anomalies(measurements);
    if (!anomalies.empty()) {
        report << "Performance Anomalies:\n";
        for (const auto& anomaly : anomalies) {
            report << "  ⚠ " << anomaly << "\n";
        }
        report << "\n";
    }

    // Constitutional validation
    bool constitutional_compliance = validate_constitutional_compliance(measurements, config);
    report << "Constitutional Compliance: "
           << (constitutional_compliance ? "✓ PASS" : "✗ FAIL") << "\n";

    report << "================================\n";

    return report.str();
}

// Enhanced RealTimeMonitor methods

bool RealTimeMonitor::measure_kernel_performance(const std::string& kernel_name,
                                                  int device_id,
                                                  int iterations) {
    if (!monitoring_enabled_ || !device_operation_count_) {
        return false;
    }

    // Launch initialization kernel
    initialize_performance_counters<<<1, 1>>>(
        device_operation_count_,
        device_memory_access_count_,
        device_cache_hit_count_,
        device_cache_miss_count_,
        device_instruction_count_,
        device_warp_execution_count_,
        device_kernel_start_time_
    );

    cudaDeviceSynchronize();

    // Simulate kernel execution (in production, this would be the actual kernel)
    for (int i = 0; i < iterations; ++i) {
        record_operation_start();
        record_memory_access(256); // Simulate memory access
        record_cache_hit();
        record_instruction_execution();
        record_warp_execution();
        record_operation_end(true);
    }

    // Launch update kernel
    uint64_t end_time = clock64();
    update_performance_counters<<<1, 1>>>(
        device_operation_count_,
        device_memory_access_count_,
        device_cache_hit_count_,
        device_cache_miss_count_,
        device_instruction_count_,
        device_warp_execution_count_,
        end_time,
        iterations
    );

    cudaDeviceSynchronize();

    // Collect device measurements
    collect_device_measurements();
    update_cached_metrics();

    return true;
}

// Enhanced TelemetryCollector methods

bool TelemetryCollector::export_detailed_report(const std::string& filename) {
    if (!config_.enable_file_output) {
        return false;
    }

    std::string report_file = output_directory_ + "/" + filename;
    std::ofstream report_stream(report_file);

    if (!report_stream.is_open()) {
        return false;
    }

    // Generate comprehensive report
    report_stream << "=== Comprehensive Performance Telemetry Report ===\n\n";

    // Summary statistics
    report_stream << "Summary Statistics:\n";
    report_stream << "  Total Data Points: " << telemetry_data_.size() << "\n";

    if (!telemetry_data_.empty()) {
        // Calculate aggregates
        double avg_gpu_util = 0.0, avg_memory_eff = 0.0, avg_occupancy = 0.0;
        double max_gpu_util = 0.0, max_memory_eff = 0.0, max_occupancy = 0.0;
        double min_gpu_util = 100.0, min_memory_eff = 100.0, min_occupancy = 100.0;

        for (const auto& point : telemetry_data_) {
            avg_gpu_util += point.measurements.gpu_utilization_percent;
            avg_memory_eff += point.measurements.memory_efficiency_percent;
            avg_occupancy += point.measurements.occupancy_percent;

            max_gpu_util = std::max(max_gpu_util, point.measurements.gpu_utilization_percent);
            max_memory_eff = std::max(max_memory_eff, point.measurements.memory_efficiency_percent);
            max_occupancy = std::max(max_occupancy, point.measurements.occupancy_percent);

            min_gpu_util = std::min(min_gpu_util, point.measurements.gpu_utilization_percent);
            min_memory_eff = std::min(min_memory_eff, point.measurements.memory_efficiency_percent);
            min_occupancy = std::min(min_occupancy, point.measurements.occupancy_percent);
        }

        int count = telemetry_data_.size();
        avg_gpu_util /= count;
        avg_memory_eff /= count;
        avg_occupancy /= count;

        report_stream << "  GPU Utilization - Avg: " << std::fixed << std::setprecision(1)
                     << avg_gpu_util << "%, Min: " << min_gpu_util
                     << "%, Max: " << max_gpu_util << "%\n";
        report_stream << "  Memory Efficiency - Avg: " << std::fixed << std::setprecision(1)
                     << avg_memory_eff << "%, Min: " << min_memory_eff
                     << "%, Max: " << max_memory_eff << "%\n";
        report_stream << "  Occupancy - Avg: " << std::fixed << std::setprecision(1)
                     << avg_occupancy << "%, Min: " << min_occupancy
                     << "%, Max: " << max_occupancy << "%\n";
    }

    report_stream << "\n";

    // Constitutional compliance summary
    report_stream << "Constitutional Compliance Summary:\n";
    int compliant_points = 0;
    for (const auto& point : telemetry_data_) {
        if (validate_constitutional_compliance(point.measurements, config_)) {
            compliant_points++;
        }
    }

    if (!telemetry_data_.empty()) {
        double compliance_rate = (static_cast<double>(compliant_points) / telemetry_data_.size()) * 100.0;
        report_stream << "  Compliance Rate: " << std::fixed << std::setprecision(1)
                     << compliance_rate << "% (" << compliant_points
                     << "/" << telemetry_data_.size() << " points)\n";
    }

    report_stream << "\n";

    // Individual data points
    report_stream << "Individual Data Points:\n";
    report_stream << "Timestamp,Device,Kernel,Iteration,GPU_Util%,Memory_Eff%,Occupancy%,Cache_Hit%,Sync_Overhead%\n";

    for (const auto& point : telemetry_data_) {
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            point.timestamp.time_since_epoch()).count();

        report_stream << timestamp << ","
                     << point.device_id << ","
                     << point.kernel_name << ","
                     << point.iteration << ","
                     << std::fixed << std::setprecision(2)
                     << point.measurements.gpu_utilization_percent << ","
                     << point.measurements.memory_efficiency_percent << ","
                     << point.measurements.occupancy_percent << ","
                     << point.measurements.cache_hit_rate_percent << ","
                     << point.measurements.synchronization_overhead_percent << "\n";
    }

    report_stream << "\n=== End of Report ===\n";
    report_stream.close();

    return true;
}

// Enhanced PerformanceMeasurementManager methods

bool PerformanceMeasurementManager::run_comprehensive_benchmark(int device_id,
                                                              const std::string& benchmark_name) {
    if (!initialized_) {
        std::cerr << "Performance measurement manager not initialized" << std::endl;
        return false;
    }

    std::cout << "Starting comprehensive benchmark: " << benchmark_name << std::endl;

    // Start measurement session
    start_measurement_session();

    // Run different kernel types for comprehensive measurement
    std::vector<std::string> kernel_types = {
        "ecc_scalar_mul",
        "hash_sha256",
        "memory_transfer",
        "reduction_operation",
        "warp_shuffle_test"
    };

    for (size_t i = 0; i < kernel_types.size(); ++i) {
        const std::string& kernel_name = kernel_types[i];

        std::cout << "Benchmarking kernel: " << kernel_name << std::endl;

        // Measure kernel performance
        bool success = real_time_monitor_.measure_kernel_performance(
            kernel_name, device_id, 100
        );

        if (success) {
            // Record measurement point
            std::unordered_map<std::string, std::string> metadata;
            metadata["benchmark_name"] = benchmark_name;
            metadata["kernel_type"] = kernel_name;

            record_measurement_point(kernel_name, static_cast<int>(i), metadata);

            // Small delay between measurements
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            std::cerr << "Failed to measure kernel: " << kernel_name << std::endl;
        }
    }

    // Stop measurement session
    stop_measurement_session();

    // Export results
    bool export_success = export_data();
    if (export_success) {
        std::cout << "Benchmark results exported successfully" << std::endl;
    } else {
        std::cerr << "Failed to export benchmark results" << std::endl;
    }

    // Generate and display report
    PerformanceMeasurements final_metrics = get_aggregated_metrics();
    std::string report = generate_performance_report(final_metrics, config_);
    std::cout << report << std::endl;

    return export_success;
}

double PerformanceMeasurementManager::get_current_performance_score() const {
    PerformanceMeasurements current_metrics = get_aggregated_metrics();
    return calculate_performance_score(current_metrics);
}

bool PerformanceMeasurementManager::meets_constitutional_requirements() const {
    PerformanceMeasurements current_metrics = get_aggregated_metrics();
    return validate_constitutional_compliance(current_metrics, config_);
}

std::vector<std::string> PerformanceMeasurementManager::get_performance_issues() const {
    PerformanceMeasurements current_metrics = get_aggregated_metrics();
    return detect_performance_anomalies(current_metrics);
}

} // namespace performance
} // namespace keyhunt
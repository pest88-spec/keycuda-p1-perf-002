/**
 * @file performance_validator.h
 * @brief Performance metrics validation system for constitutional compliance
 *
 * Validates constitutional requirements for performance metrics:
 * - Memory efficiency: 90%+ (from 15.6% baseline)
 * - GPU occupancy: ≥80% (from 25% baseline)
 * - Throughput improvements: 2.5-3× over baseline
 *
 * @author Puzzle71Solver CUDA Team
 * @date 2025-10-19
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>
#include <chrono>
#include <cstdint>

namespace keyhunt {
namespace validation {

/**
 * @brief Configuration for performance validation
 */
struct PerformanceValidatorConfig {
    bool enable_logging = true;
    bool run_full_benchmark = true;  // 10-minute sustained benchmark
    bool generate_detailed_report = true;
    bool enable_profiling = true;

    // Benchmark configuration
    struct BenchmarkConfig {
        size_t duration_seconds = 600;  // 10 minutes for constitution validation
        size_t warmup_seconds = 30;
        size_t sampling_interval_ms = 1000;
        double stability_threshold = 95.0;  // % stability requirement
    } benchmark_config;

    // Nsight Compute configuration
    struct NsightConfig {
        std::string executable = "ncu";
        std::vector<std::string> metrics = {
            "sm__warps_active.avg.pct_of_peak_sustained_active",
            "sm__warps_active.avg.pct_of_peak_sustained_active",
            "dram__throughput.avg.pct_of_peak_sustained",
            "lts__throughput_avg_bytes_read.global.pct_of_peak_sustained",
            "lts__throughput_avg_bytes_written.global.pct_of_peak_sustained"
        };
        std::vector<std::string> kernels = {"eccKernel", "hashKernel", "compareKernel"};
    } nsight_config;

    // Telemetry configuration
    struct TelemetryConfig {
        std::string executable = "nvidia-smi";
        size_t sampling_interval_ms = 1000;
        std::vector<std::string> metrics = {
            "utilization.gpu",
            "utilization.memory",
            "power.draw",
            "temperature.gpu"
        };
    } telemetry_config;
};

/**
 * @brief GPU information
 */
struct GPUInfo {
    std::string identifier;
    std::string name;
    std::string architecture;
    std::string compute_capability;
    uint64_t total_memory_mb;
    std::string driver_version;
    std::string cuda_version;
};

/**
 * @brief Baseline performance metrics (from technical debt analysis)
 */
struct BaselineMetrics {
    std::string gpu_name;
    std::string architecture;
    std::string compute_capability;
    double baseline_throughput;  // Mkeys/s
    double baseline_memory_efficiency;  // %
    double baseline_gpu_occupancy;  // %
    uint64_t baseline_memory_bandwidth_mbps;  // MB/s
    double baseline_register_usage;  // registers/thread
};

/**
 * @brief Target performance metrics (constitution requirements)
 */
struct TargetMetrics {
    std::string gpu_name;
    double target_throughput_min;  // Mkeys/s
    double target_throughput_max;  // Mkeys/s
    double target_memory_efficiency;  // %
    double target_gpu_occupancy;  // %
    double target_register_usage_max;  // registers/thread
};

/**
 * @brief Measured performance metrics
 */
struct PerformanceMetrics {
    std::string gpu_identifier;
    std::string benchmark_timestamp;

    // Throughput metrics
    double throughput_mkeys_per_sec = 0.0;
    double throughput_improvement_factor = 0.0;
    double throughput_stability = 0.0;  // % stability over benchmark duration

    // Memory performance metrics
    double memory_efficiency_percentage = 0.0;
    double memory_bandwidth_utilization_mbps = 0.0;
    double bank_conflict_percentage = 0.0;
    double memory_efficiency_improvement = 0.0;  // improvement over baseline

    // GPU utilization metrics
    double gpu_occupancy_percentage = 0.0;
    double sm_efficiency_percentage = 0.0;
    double warp_efficiency_percentage = 0.0;
    double gpu_occupancy_improvement = 0.0;  // improvement over baseline

    // Resource usage metrics
    double average_registers_per_thread = 0.0;
    double max_registers_per_thread = 0;

    // System metrics
    double average_gpu_utilization = 0.0;
    double average_power_usage_watts = 0.0;
    double average_temperature_celsius = 0.0;

    // Quality metrics
    double measurement_quality_score = 0.0;
    std::chrono::milliseconds benchmark_duration_ms{0};
};

/**
 * @brief Validation results for each constitutional requirement
 */
struct ValidationResults {
    // Individual compliance flags
    bool memory_efficiency_compliance = false;
    bool gpu_occupancy_compliance = false;
    bool throughput_improvement_compliance = false;
    bool register_usage_compliance = false;
    bool stability_compliance = false;
    bool measurement_quality_compliance = false;

    // Individual compliance scores (0-100%)
    double memory_efficiency_score = 0.0;
    double gpu_occupancy_score = 0.0;
    double throughput_improvement_score = 0.0;
    double register_usage_score = 0.0;
    double stability_score = 0.0;
};

/**
 * @brief Complete validation result
 */
struct ValidationResult {
    std::string gpu_identifier;
    std::string validation_timestamp;
    bool success = true;
    std::string error_message;

    GPUInfo gpu_info;
    PerformanceMetrics measured_metrics;
    ValidationResults validation_results;

    double overall_compliance = 0.0;
    bool is_constitution_compliant = false;
};

/**
 * @brief Global validation metrics
 */
struct ValidationMetrics {
    size_t total_validations = 0;
    size_t successful_validations = 0;
    size_t failed_validations = 0;
    size_t constitution_compliant_validations = 0;

    double average_throughput_improvement = 0.0;
    double average_memory_efficiency = 0.0;
    double average_gpu_occupancy = 0.0;

    std::chrono::milliseconds total_benchmark_time{0};
};

// Forward declarations for collector classes
class NsightComputeCollector;
class TelemetryCollector;
class BenchmarkRunner;

/**
 * @brief Performance metrics validation system
 *
 * Validates constitutional requirements for performance metrics including
 * memory efficiency, GPU occupancy, and throughput improvements.
 */
class PerformanceValidator {
public:
    /**
     * @brief Constructor
     * @param config Validation configuration
     */
    explicit PerformanceValidator(const PerformanceValidatorConfig& config = {});

    /**
     * @brief Destructor
     */
    ~PerformanceValidator();

    // Delete copy constructor and assignment operator
    PerformanceValidator(const PerformanceValidator&) = delete;
    PerformanceValidator& operator=(const PerformanceValidator&) = delete;

    /**
     * @brief Validate performance compliance for a specific GPU
     * @param gpu_identifier GPU identifier (can be empty for auto-detection)
     * @return Complete validation results
     */
    ValidationResult validatePerformanceCompliance(const std::string& gpu_identifier = "");

    /**
     * @brief Generate compliance report
     * @param result Validation results
     * @return Formatted compliance report in Markdown format
     */
    std::string generateComplianceReport(const ValidationResult& result);

    /**
     * @brief Get global validation metrics
     * @return Current validation metrics
     */
    const ValidationMetrics& getMetrics() const { return metrics_; }

    /**
     * @brief Reset global metrics
     */
    void resetMetrics() { metrics_ = ValidationMetrics{}; }

    /**
     * @brief Create validator with default configuration
     * @param config Optional configuration override
     * @return Unique pointer to validator instance
     */
    static std::unique_ptr<PerformanceValidator> create(
        const PerformanceValidatorConfig& config = {});

    /**
     * @brief Quick validation check (pass/fail only, short benchmark)
     * @param gpu_identifier GPU identifier
     * @return True if constitution compliant
     */
    static bool quickValidate(const std::string& gpu_identifier = "");

    /**
     * @brief Full validation with detailed report
     * @param gpu_identifier GPU identifier
     * @param report_output Output string for the report
     * @return True if constitution compliant
     */
    static bool fullValidate(const std::string& gpu_identifier,
                            std::string& report_output);

    // Constitution compliance thresholds
    static constexpr double CONSTITUTION_MEMORY_EFFICIENCY_THRESHOLD = 90.0;
    static constexpr double CONSTITUTION_GPU_OCCUPANCY_THRESHOLD = 80.0;
    static constexpr double CONSTITUTION_THROUGHPUT_IMPROVEMENT_MIN = 2.5;
    static constexpr double CONSTITUTION_THROUGHPUT_IMPROVEMENT_MAX = 3.0;

private:
    // Configuration and state
    PerformanceValidatorConfig config_;
    ValidationMetrics metrics_;

    // Performance collectors
    std::unique_ptr<NsightComputeCollector> nsight_collector_;
    std::unique_ptr<TelemetryCollector> telemetry_collector_;
    std::unique_ptr<BenchmarkRunner> benchmark_runner_;

    // Static data
    static const std::map<std::string, BaselineMetrics> BASELINE_PERFORMANCE;
    static const std::map<std::string, TargetMetrics> TARGET_PERFORMANCE;

    /**
     * @brief Reset metrics to initial state
     */
    void resetMetrics();

    /**
     * @brief Setup performance collectors
     */
    void setupCollectors();

    /**
     * @brief Detect GPU capabilities
     * @param gpu_identifier GPU identifier (can be empty for auto-detection)
     * @return GPU information
     */
    GPUInfo detectGPUCapabilities(const std::string& gpu_identifier);

    /**
     * @brief Get baseline metrics for GPU
     * @param gpu_info GPU information
     * @return Baseline metrics if available
     */
    std::optional<BaselineMetrics> getBaselineMetrics(const GPUInfo& gpu_info);

    /**
     * @brief Get target metrics for GPU
     * @param gpu_info GPU information
     * @return Target metrics if available
     */
    std::optional<TargetMetrics> getTargetMetrics(const GPUInfo& gpu_info);

    /**
     * @brief Run performance benchmarks
     * @param gpu_info GPU information
     * @param baseline Baseline metrics
     * @param target Target metrics
     * @return Measured performance metrics
     */
    PerformanceMetrics runPerformanceBenchmarks(
        const GPUInfo& gpu_info,
        const BaselineMetrics& baseline,
        const TargetMetrics& target);

    /**
     * @brief Validate constitution requirements
     * @param measured Measured metrics
     * @param baseline Baseline metrics
     * @param target Target metrics
     * @return Validation results
     */
    ValidationResults validateConstitutionRequirements(
        const PerformanceMetrics& measured,
        const BaselineMetrics& baseline,
        const TargetMetrics& target);

    // Individual validation methods
    bool validateMemoryEfficiency(const PerformanceMetrics& measured);
    bool validateGPUOccupancy(const PerformanceMetrics& measured);
    bool validateThroughputImprovement(const PerformanceMetrics& measured,
                                       const BaselineMetrics& baseline);
    bool validateRegisterUsage(const PerformanceMetrics& measured);
    bool validateStability(const PerformanceMetrics& measured);
    bool validateMeasurementQuality(const PerformanceMetrics& measured);

    // Score calculation methods
    double calculateMemoryEfficiencyScore(const PerformanceMetrics& measured);
    double calculateGPUOccupancyScore(const PerformanceMetrics& measured);
    double calculateThroughputImprovementScore(const PerformanceMetrics& measured,
                                               const BaselineMetrics& baseline);
    double calculateRegisterUsageScore(const PerformanceMetrics& measured);
    double calculateStabilityScore(const PerformanceMetrics& measured);

    /**
     * @brief Calculate overall compliance score
     * @param results Individual validation results
     * @return Overall compliance score (0-100%)
     */
    double calculateOverallCompliance(const ValidationResults& results);

    /**
     * @brief Check if results meet all constitution requirements
     * @param results Validation results
     * @return True if constitution compliant
     */
    bool isConstitutionCompliant(const ValidationResults& results);

    /**
     * @brief Update global metrics with new validation result
     * @param result Validation result
     */
    void updateGlobalMetrics(const ValidationResult& result);

    /**
     * @brief Calculate measurement quality score
     * @param metrics Performance metrics
     * @return Quality score (0-100%)
     */
    double calculateMeasurementQuality(const PerformanceMetrics& metrics);

    // Utility methods
    std::string executeCommand(const std::string& command);
    std::string trim(const std::string& str);
    std::string determineArchitecture(const std::string& compute_capability);
    std::string generateGPUIdentifier(const GPUInfo& info);
    std::string getCurrentTimestamp();

    /**
     * @brief Generate summary report to console
     */
    void generateSummaryReport() const;
};

/**
 * @brief Mock Nsight Compute collector for testing
 */
class NsightComputeCollector {
public:
    struct MemoryResult {
        double global_load_efficiency = 0.0;
        double bandwidth_utilization = 0.0;
        double bank_conflict_percentage = 0.0;
    };

    struct OccupancyResult {
        double occupancy_percentage = 0.0;
        double sm_efficiency = 0.0;
        double warp_efficiency = 0.0;
    };

    struct RegisterResult {
        double average_registers = 0.0;
        double max_registers = 0;
    };

    explicit NsightComputeCollector(const PerformanceValidatorConfig::NsightConfig& config)
        : config_(config) {}

    MemoryResult analyzeMemoryEfficiency(const std::string& gpu_identifier);
    OccupancyResult analyzeGPUOccupancy(const std::string& gpu_identifier);
    RegisterResult analyzeRegisterUsage(const std::string& gpu_identifier);

private:
    PerformanceValidatorConfig::NsightConfig config_;
};

/**
 * @brief Mock telemetry collector for real-time monitoring
 */
class TelemetryCollector {
public:
    struct TelemetryData {
        double average_utilization = 0.0;
        double average_power = 0.0;
        double average_temperature = 0.0;
    };

    explicit TelemetryCollector(const PerformanceValidatorConfig::TelemetryConfig& config)
        : config_(config) {}

    TelemetryData collectDuringBenchmark(const std::string& gpu_identifier,
                                        size_t duration_seconds);

private:
    PerformanceValidatorConfig::TelemetryConfig config_;
};

/**
 * @brief Mock benchmark runner for sustained testing
 */
class BenchmarkRunner {
public:
    struct ThroughputResult {
        double average_throughput = 0.0;
        double stability_percentage = 0.0;
    };

    explicit BenchmarkRunner(const PerformanceValidatorConfig::BenchmarkConfig& config)
        : config_(config) {}

    ThroughputResult runSustainedBenchmark(const std::string& gpu_identifier,
                                          size_t duration_seconds);

private:
    PerformanceValidatorConfig::BenchmarkConfig config_;
};

} // namespace validation
} // namespace keyhunt
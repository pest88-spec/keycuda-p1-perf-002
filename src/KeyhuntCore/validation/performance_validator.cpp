/**
 * @file performance_validator.cpp
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

#include "performance_validator.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <chrono>
#include <regex>

namespace keyhunt {
namespace validation {

// Constitution compliance thresholds
const double PerformanceValidator::CONSTITUTION_MEMORY_EFFICIENCY_THRESHOLD = 90.0;
const double PerformanceValidator::CONSTITUTION_GPU_OCCUPANCY_THRESHOLD = 80.0;
const double PerformanceValidator::CONSTITUTION_THROUGHPUT_IMPROVEMENT_MIN = 2.5;
const double PerformanceValidator::CONSTITUTION_THROUGHPUT_IMPROVEMENT_MAX = 3.0;

// Baseline performance values (from technical debt analysis)
const std::map<std::string, BaselineMetrics> PerformanceValidator::BASELINE_PERFORMANCE = {
    {"RTX_2080_Ti", {
        .gpu_name = "RTX 2080 Ti",
        .architecture = "Turing",
        .compute_capability = "7.5",
        .baseline_throughput = 279.0,  // Mkeys/s
        .baseline_memory_efficiency = 15.6,  // %
        .baseline_gpu_occupancy = 25.0,  // %
        .baseline_memory_bandwidth_mbps = 616000,  // MB/s
        .baseline_register_usage = 75  // average registers/thread
    }},
    {"RTX_3090", {
        .gpu_name = "RTX 3090",
        .architecture = "Ampere",
        .compute_capability = "8.6",
        .baseline_throughput = 558.0,  // Mkeys/s (estimated)
        .baseline_memory_efficiency = 15.6,  // % (same baseline)
        .baseline_gpu_occupancy = 25.0,  // % (same baseline)
        .baseline_memory_bandwidth_mbps = 936000,  // MB/s
        .baseline_register_usage = 75  // average registers/thread
    }},
    {"H20", {
        .gpu_name = "H20",
        .architecture = "Hopper",
        .compute_capability = "9.0",
        .baseline_throughput = 950.0,  // Mkeys/s (estimated)
        .baseline_memory_efficiency = 15.6,  // % (same baseline)
        .baseline_gpu_occupancy = 25.0,  // % (same baseline)
        .baseline_memory_bandwidth_mbps = 4096000,  // MB/s
        .baseline_register_usage = 75  // average registers/thread
    }},
    {"A100", {
        .gpu_name = "A100",
        .architecture = "Hopper",
        .compute_capability = "8.0",
        .baseline_throughput = 1100.0,  // Mkeys/s (estimated)
        .baseline_memory_efficiency = 15.6,  // % (same baseline)
        .baseline_gpu_occupancy = 25.0,  // % (same baseline)
        .baseline_memory_bandwidth_mbps = 2039000,  // MB/s
        .baseline_register_usage = 75  // average registers/thread
    }}
};

// Target performance values (constitution requirements)
const std::map<std::string, TargetMetrics> PerformanceValidator::TARGET_PERFORMANCE = {
    {"RTX_2080_Ti", {
        .gpu_name = "RTX 2080 Ti",
        .target_throughput_min = 700.0,   // Mkeys/s (2.5× improvement)
        .target_throughput_max = 837.0,   // Mkeys/s (3× improvement)
        .target_memory_efficiency = 90.0,  // %
        .target_gpu_occupancy = 80.0,      // %
        .target_register_usage_max = 40    // registers/thread
    }},
    {"RTX_3090", {
        .gpu_name = "RTX 3090",
        .target_throughput_min = 1400.0,  // Mkeys/s (2.5× improvement)
        .target_throughput_max = 1674.0,  // Mkeys/s (3× improvement)
        .target_memory_efficiency = 90.0,  // %
        .target_gpu_occupancy = 80.0,      // %
        .target_register_usage_max = 40    // registers/thread
    }},
    {"H20", {
        .gpu_name = "H20",
        .target_throughput_min = 2400.0,  // Mkeys/s (2.5× improvement)
        .target_throughput_max = 2850.0,  // Mkeys/s (3× improvement)
        .target_memory_efficiency = 90.0,  // %
        .target_gpu_occupancy = 80.0,      // %
        .target_register_usage_max = 40    // registers/thread
    }},
    {"A100", {
        .gpu_name = "A100",
        .target_throughput_min = 2800.0,  // Mkeys/s (2.5× improvement)
        .target_throughput_max = 3300.0,  // Mkeys/s (3× improvement)
        .target_memory_efficiency = 90.0,  // %
        .target_gpu_occupancy = 80.0,      // %
        .target_register_usage_max = 40    // registers/thread
    }}
};

PerformanceValidator::PerformanceValidator(const PerformanceValidatorConfig& config)
    : config_(config) {

    // Initialize validation metrics
    resetMetrics();

    // Setup performance collectors
    setupCollectors();

    LOG_INFO("PerformanceValidator initialized with constitution compliance validation");
}

PerformanceValidator::~PerformanceValidator() {
    if (config_.enable_logging) {
        generateSummaryReport();
    }
}

void PerformanceValidator::resetMetrics() {
    metrics_.total_validations = 0;
    metrics_.successful_validations = 0;
    metrics_.failed_validations = 0;
    metrics_.constitution_compliant_validations = 0;
    metrics_.average_throughput_improvement = 0.0;
    metrics_.average_memory_efficiency = 0.0;
    metrics_.average_gpu_occupancy = 0.0;
    metrics_.total_benchmark_time = std::chrono::milliseconds{0};
}

void PerformanceValidator::setupCollectors() {
    // Initialize Nsight Compute collector
    nsight_collector_ = std::make_unique<NsightComputeCollector>(config_.nsight_config);

    // Initialize telemetry collector
    telemetry_collector_ = std::make_unique<TelemetryCollector>(config_.telemetry_config);

    // Initialize benchmark runner
    benchmark_runner_ = std::make_unique<BenchmarkRunner>(config_.benchmark_config);

    LOG_DEBUG("Performance collectors initialized");
}

ValidationResult PerformanceValidator::validatePerformanceCompliance(const std::string& gpu_identifier) {
    LOG_INFO("Starting performance compliance validation for: " + gpu_identifier);

    ValidationResult result;
    result.gpu_identifier = gpu_identifier;
    result.validation_timestamp = getCurrentTimestamp();
    result.success = true;

    try {
        // Detect GPU capabilities
        auto gpu_info = detectGPUCapabilities(gpu_identifier);
        result.gpu_info = gpu_info;

        // Get baseline metrics for this GPU
        auto baseline = getBaselineMetrics(gpu_info);
        if (!baseline) {
            result.success = false;
            result.error_message = "No baseline metrics available for GPU: " + gpu_identifier;
            return result;
        }

        // Get target metrics for this GPU
        auto target = getTargetMetrics(gpu_info);
        if (!target) {
            result.success = false;
            result.error_message = "No target metrics available for GPU: " + gpu_identifier;
            return result;
        }

        // Run performance benchmarks
        auto performance_metrics = runPerformanceBenchmarks(gpu_info, *baseline, *target);
        result.measured_metrics = performance_metrics;

        // Validate against constitution requirements
        result.validation_results = validateConstitutionRequirements(performance_metrics, *baseline, *target);

        // Calculate overall compliance
        result.overall_compliance = calculateOverallCompliance(result.validation_results);
        result.is_constitution_compliant = isConstitutionCompliant(result.validation_results);

        // Update global metrics
        updateGlobalMetrics(result);

        LOG_INFO("Performance compliance validation completed: " +
                 std::string(result.is_constitution_compliant ? "COMPLIANT" : "NON-COMPLIANT") +
                 " (Score: " + std::to_string(result.overall_compliance) + "%)");

    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("Performance validation failed: ") + e.what();
        LOG_ERROR("Performance validation failed: " + std::string(e.what()));
    }

    return result;
}

GPUInfo PerformanceValidator::detectGPUCapabilities(const std::string& gpu_identifier) {
    LOG_DEBUG("Detecting GPU capabilities for: " + gpu_identifier);

    GPUInfo info;

    // Use nvidia-smi to get GPU information
    std::string command = "nvidia-smi --query-gpu=name,compute_cap,memory.total --format=csv,noheader,nounits";
    if (!gpu_identifier.empty()) {
        command += " -i " + gpu_identifier;
    }

    std::string output = executeCommand(command);
    std::istringstream iss(output);
    std::string line;

    if (std::getline(iss, line)) {
        std::istringstream line_stream(line);
        std::string name, compute_cap, memory_total;

        if (std::getline(line_stream, name, ',') &&
            std::getline(line_stream, compute_cap, ',') &&
            std::getline(line_stream, memory_total)) {

            // Trim whitespace
            name = trim(name);
            compute_cap = trim(compute_cap);
            memory_total = trim(memory_total);

            info.name = name;
            info.compute_capability = compute_cap;
            info.total_memory_mb = std::stoull(memory_total);

            // Determine architecture
            info.architecture = determineArchitecture(compute_cap);

            // Generate standard identifier
            info.identifier = generateGPUIdentifier(info);

            LOG_DEBUG("GPU detected: " + info.name + " (" + info.architecture + " " + compute_cap + ")");
        }
    }

    if (info.name.empty()) {
        throw std::runtime_error("Failed to detect GPU capabilities for: " + gpu_identifier);
    }

    return info;
}

std::optional<BaselineMetrics> PerformanceValidator::getBaselineMetrics(const GPUInfo& gpu_info) {
    auto it = BASELINE_PERFORMANCE.find(gpu_info.identifier);
    if (it != BASELINE_PERFORMANCE.end()) {
        return it->second;
    }

    // Try fuzzy matching by GPU name patterns
    for (const auto& [identifier, baseline] : BASELINE_PERFORMANCE) {
        if (gpu_info.name.find(baseline.gpu_name) != std::string::npos ||
            baseline.gpu_name.find(gpu_info.name) != std::string::npos) {
            LOG_INFO("Using fuzzy-matched baseline: " + identifier + " for " + gpu_info.name);
            return baseline;
        }
    }

    return std::nullopt;
}

std::optional<TargetMetrics> PerformanceValidator::getTargetMetrics(const GPUInfo& gpu_info) {
    auto it = TARGET_PERFORMANCE.find(gpu_info.identifier);
    if (it != TARGET_PERFORMANCE.end()) {
        return it->second;
    }

    // Try fuzzy matching by GPU name patterns
    for (const auto& [identifier, target] : TARGET_PERFORMANCE) {
        if (gpu_info.name.find(target.gpu_name) != std::string::npos ||
            target.gpu_name.find(gpu_info.name) != std::string::npos) {
            LOG_INFO("Using fuzzy-matched target: " + identifier + " for " + gpu_info.name);
            return target;
        }
    }

    return std::nullopt;
}

PerformanceMetrics PerformanceValidator::runPerformanceBenchmarks(
    const GPUInfo& gpu_info,
    const BaselineMetrics& baseline,
    const TargetMetrics& target) {

    LOG_INFO("Running performance benchmarks for: " + gpu_info.name);

    PerformanceMetrics metrics;
    metrics.gpu_identifier = gpu_info.identifier;
    metrics.benchmark_timestamp = getCurrentTimestamp();

    auto start_time = std::chrono::high_resolution_clock::now();

    try {
        // Run sustained benchmark (10 minutes as per constitution)
        LOG_DEBUG("Starting 10-minute sustained benchmark...");
        auto throughput_result = benchmark_runner_->runSustainedBenchmark(
            gpu_info.identifier, 600); // 10 minutes

        metrics.throughput_mkeys_per_sec = throughput_result.average_throughput;
        metrics.throughput_stability = throughput_result.stability_percentage;

        // Run memory efficiency analysis
        LOG_DEBUG("Running memory efficiency analysis...");
        auto memory_result = nsight_collector_->analyzeMemoryEfficiency(gpu_info.identifier);

        metrics.memory_efficiency_percentage = memory_result.global_load_efficiency;
        metrics.memory_bandwidth_utilization_mbps = memory_result.bandwidth_utilization;
        metrics.bank_conflict_percentage = memory_result.bank_conflict_percentage;

        // Run GPU occupancy analysis
        LOG_DEBUG("Running GPU occupancy analysis...");
        auto occupancy_result = nsight_collector_->analyzeGPUOccupancy(gpu_info.identifier);

        metrics.gpu_occupancy_percentage = occupancy_result.occupancy_percentage;
        metrics.sm_efficiency_percentage = occupancy_result.sm_efficiency;
        metrics.warp_efficiency_percentage = occupancy_result.warp_efficiency;

        // Run register usage analysis
        LOG_DEBUG("Running register usage analysis...");
        auto register_result = nsight_collector_->analyzeRegisterUsage(gpu_info.identifier);

        metrics.average_registers_per_thread = register_result.average_registers;
        metrics.max_registers_per_thread = register_result.max_registers;

        // Collect real-time telemetry during benchmark
        LOG_DEBUG("Collecting real-time telemetry...");
        auto telemetry_data = telemetry_collector_->collectDuringBenchmark(
            gpu_info.identifier, 600); // 10 minutes

        metrics.average_gpu_utilization = telemetry_data.average_utilization;
        metrics.average_power_usage_watts = telemetry_data.average_power;
        metrics.average_temperature_celsius = telemetry_data.average_temperature;

        // Calculate improvements over baseline
        metrics.throughput_improvement_factor = metrics.throughput_mkeys_per_sec / baseline.baseline_throughput;
        metrics.memory_efficiency_improvement = metrics.memory_efficiency_percentage - baseline.baseline_memory_efficiency;
        metrics.gpu_occupancy_improvement = metrics.gpu_occupancy_percentage - baseline.baseline_gpu_occupancy;

        // Validate measurement quality
        metrics.measurement_quality_score = calculateMeasurementQuality(metrics);

        auto end_time = std::chrono::high_resolution_clock::now();
        metrics.benchmark_duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);

        LOG_INFO("Benchmark completed: " + std::to_string(metrics.throughput_mkeys_per_sec) +
                 " Mkeys/s, " + std::to_string(metrics.throughput_improvement_factor) + "× improvement");

    } catch (const std::exception& e) {
        LOG_ERROR("Benchmark failed: " + std::string(e.what()));
        throw;
    }

    return metrics;
}

ValidationResults PerformanceValidator::validateConstitutionRequirements(
    const PerformanceMetrics& measured,
    const BaselineMetrics& baseline,
    const TargetMetrics& target) {

    LOG_DEBUG("Validating constitution requirements...");

    ValidationResults results;

    // Validate memory efficiency requirement (≥90%)
    results.memory_efficiency_compliance = validateMemoryEfficiency(measured);

    // Validate GPU occupancy requirement (≥80%)
    results.gpu_occupancy_compliance = validateGPUOccupancy(measured);

    // Validate throughput improvement requirement (2.5-3×)
    results.throughput_improvement_compliance = validateThroughputImprovement(measured, baseline);

    // Validate register usage requirement (≤40 registers/thread)
    results.register_usage_compliance = validateRegisterUsage(measured);

    // Validate stability requirements
    results.stability_compliance = validateStability(measured);

    // Validate measurement quality
    results.measurement_quality_compliance = validateMeasurementQuality(measured);

    // Calculate individual compliance scores
    results.memory_efficiency_score = calculateMemoryEfficiencyScore(measured);
    results.gpu_occupancy_score = calculateGPUOccupancyScore(measured);
    results.throughput_improvement_score = calculateThroughputImprovementScore(measured, baseline);
    results.register_usage_score = calculateRegisterUsageScore(measured);
    results.stability_score = calculateStabilityScore(measured);

    LOG_DEBUG("Constitution validation completed");

    return results;
}

bool PerformanceValidator::validateMemoryEfficiency(const PerformanceMetrics& measured) {
    return measured.memory_efficiency_percentage >= CONSTITUTION_MEMORY_EFFICIENCY_THRESHOLD;
}

bool PerformanceValidator::validateGPUOccupancy(const PerformanceMetrics& measured) {
    return measured.gpu_occupancy_percentage >= CONSTITUTION_GPU_OCCUPANCY_THRESHOLD;
}

bool PerformanceValidator::validateThroughputImprovement(const PerformanceMetrics& measured,
                                                        const BaselineMetrics& baseline) {
    return measured.throughput_improvement_factor >= CONSTITUTION_THROUGHPUT_IMPROVEMENT_MIN &&
           measured.throughput_improvement_factor <= CONSTITUTION_THROUGHPUT_IMPROVEMENT_MAX;
}

bool PerformanceValidator::validateRegisterUsage(const PerformanceMetrics& measured) {
    return measured.average_registers_per_thread <= 40.0; // Constitution requirement
}

bool PerformanceValidator::validateStability(const PerformanceMetrics& measured) {
    // Require ≥95% stability over sustained benchmark
    return measured.throughput_stability >= 95.0;
}

bool PerformanceValidator::validateMeasurementQuality(const PerformanceMetrics& measured) {
    // Require measurement quality score ≥80%
    return measured.measurement_quality_score >= 80.0;
}

double PerformanceValidator::calculateMemoryEfficiencyScore(const PerformanceMetrics& measured) {
    return std::min(100.0, measured.memory_efficiency_percentage);
}

double PerformanceValidator::calculateGPUOccupancyScore(const PerformanceMetrics& measured) {
    return std::min(100.0, measured.gpu_occupancy_percentage);
}

double PerformanceValidator::calculateThroughputImprovementScore(const PerformanceMetrics& measured,
                                                               const BaselineMetrics& baseline) {
    double improvement = measured.throughput_improvement_factor;
    double target_min = CONSTITUTION_THROUGHPUT_IMPROVEMENT_MIN;
    double target_max = CONSTITUTION_THROUGHPUT_IMPROVEMENT_MAX;

    if (improvement < target_min) {
        return 0.0; // Failed requirement
    } else if (improvement > target_max) {
        // Exceeding target is good but cap the score
        return 100.0;
    } else {
        // Linear interpolation between min and max targets
        return ((improvement - target_min) / (target_max - target_min)) * 100.0;
    }
}

double PerformanceValidator::calculateRegisterUsageScore(const PerformanceMetrics& measured) {
    double usage = measured.average_registers_per_thread;
    double target = 40.0;

    if (usage <= target) {
        return 100.0;
    } else {
        // Penalty for exceeding target
        return std::max(0.0, 100.0 - (usage - target) * 5.0);
    }
}

double PerformanceValidator::calculateStabilityScore(const PerformanceMetrics& measured) {
    return measured.throughput_stability;
}

double PerformanceValidator::calculateOverallCompliance(const ValidationResults& results) {
    // Weighted average of all compliance scores
    double weights[] = {
        0.25, // Memory efficiency
        0.25, // GPU occupancy
        0.30, // Throughput improvement (most important)
        0.10, // Register usage
        0.10  // Stability
    };

    double scores[] = {
        results.memory_efficiency_score,
        results.gpu_occupancy_score,
        results.throughput_improvement_score,
        results.register_usage_score,
        results.stability_score
    };

    double overall_score = 0.0;
    for (int i = 0; i < 5; ++i) {
        overall_score += weights[i] * scores[i];
    }

    return overall_score;
}

bool PerformanceValidator::isConstitutionCompliant(const ValidationResults& results) {
    // All requirements must be met for constitution compliance
    return results.memory_efficiency_compliance &&
           results.gpu_occupancy_compliance &&
           results.throughput_improvement_compliance &&
           results.register_usage_compliance &&
           results.stability_compliance &&
           results.measurement_quality_compliance;
}

void PerformanceValidator::updateGlobalMetrics(const ValidationResult& result) {
    metrics_.total_validations++;

    if (result.success) {
        metrics_.successful_validations++;

        if (result.is_constitution_compliant) {
            metrics_.constitution_compliant_validations++;
        }

        // Update running averages
        size_t successful = metrics_.successful_validations;
        double throughput_imp = result.measured_metrics.throughput_improvement_factor;
        double memory_eff = result.measured_metrics.memory_efficiency_percentage;
        double gpu_occ = result.measured_metrics.gpu_occupancy_percentage;

        metrics_.average_throughput_improvement =
            (metrics_.average_throughput_improvement * (successful - 1) + throughput_imp) / successful;
        metrics_.average_memory_efficiency =
            (metrics_.average_memory_efficiency * (successful - 1) + memory_eff) / successful;
        metrics_.average_gpu_occupancy =
            (metrics_.average_gpu_occupancy * (successful - 1) + gpu_occ) / successful;

        metrics_.total_benchmark_time += result.measured_metrics.benchmark_duration_ms;
    } else {
        metrics_.failed_validations++;
    }
}

double PerformanceValidator::calculateMeasurementQuality(const PerformanceMetrics& metrics) {
    double quality_score = 100.0;

    // Penalize high variability
    if (metrics.throughput_stability < 95.0) {
        quality_score -= (95.0 - metrics.throughput_stability) * 2.0;
    }

    // Penalize low GPU utilization
    if (metrics.average_gpu_utilization < 90.0) {
        quality_score -= (90.0 - metrics.average_gpu_utilization);
    }

    // Penalize extreme temperatures
    if (metrics.average_temperature_celsius > 85.0) {
        quality_score -= (metrics.average_temperature_celsius - 85.0) * 2.0;
    }

    return std::max(0.0, quality_score);
}

std::string PerformanceValidator::generateComplianceReport(const ValidationResult& result) {
    std::ostringstream report;

    report << "# Constitution Compliance Report: Performance Metrics\n\n";
    report << "**GPU**: " << result.gpu_info.name << " (" << result.gpu_info.architecture << " " << result.gpu_info.compute_capability << ")\n";
    report << "**Validation Date**: " << result.validation_timestamp << "\n";
    report << "**Overall Status**: " << (result.is_constitution_compliant ? "✅ COMPLIANT" : "❌ NON-COMPLIANT") << "\n";
    report << "**Compliance Score**: " << std::fixed << std::setprecision(1) << result.overall_compliance << "%\n\n";

    // Executive Summary
    report << "## Executive Summary\n\n";
    report << "**Constitution Compliance**: " << (result.is_constitution_compliant ? "✅ PASSED" : "❌ FAILED") << "\n\n";

    if (result.is_constitution_compliant) {
        report << "🎉 **ALL CONSTITUTIONAL REQUIREMENTS MET**\n\n";
        report << "The system meets or exceeds all constitutional performance requirements:\n";
        report << "- ✅ Memory efficiency: " << std::setprecision(1) << result.measured_metrics.memory_efficiency_percentage << "% (≥90% required)\n";
        report << "- ✅ GPU occupancy: " << std::setprecision(1) << result.measured_metrics.gpu_occupancy_percentage << "% (≥80% required)\n";
        report << "- ✅ Throughput improvement: " << std::setprecision(2) << result.measured_metrics.throughput_improvement_factor << "× (2.5-3× required)\n";
        report << "- ✅ Register usage: " << std::setprecision(1) << result.measured_metrics.average_registers_per_thread << " (≤40 required)\n";
        report << "- ✅ Stability: " << std::setprecision(1) << result.measured_metrics.throughput_stability << "% (≥95% required)\n";
    } else {
        report << "❌ **CONSTITUTIONAL VIOLATIONS DETECTED**\n\n";
        report << "The following constitutional requirements are not met:\n";

        if (!result.validation_results.memory_efficiency_compliance) {
            report << "- ❌ Memory efficiency: " << std::setprecision(1) << result.measured_metrics.memory_efficiency_percentage << "% (≥90% required)\n";
        }
        if (!result.validation_results.gpu_occupancy_compliance) {
            report << "- ❌ GPU occupancy: " << std::setprecision(1) << result.measured_metrics.gpu_occupancy_percentage << "% (≥80% required)\n";
        }
        if (!result.validation_results.throughput_improvement_compliance) {
            report << "- ❌ Throughput improvement: " << std::setprecision(2) << result.measured_metrics.throughput_improvement_factor << "× (2.5-3× required)\n";
        }
        if (!result.validation_results.register_usage_compliance) {
            report << "- ❌ Register usage: " << std::setprecision(1) << result.measured_metrics.average_registers_per_thread << " (≤40 required)\n";
        }
        if (!result.validation_results.stability_compliance) {
            report << "- ❌ Stability: " << std::setprecision(1) << result.measured_metrics.throughput_stability << "% (≥95% required)\n";
        }
    }

    report << "\n";

    // Performance Metrics
    report << "## Performance Metrics\n\n";
    report << "### Throughput Performance\n";
    report << "- **Measured Throughput**: " << std::setprecision(1) << result.measured_metrics.throughput_mkeys_per_sec << " Mkeys/s\n";
    report << "- **Improvement Factor**: " << std::setprecision(2) << result.measured_metrics.throughput_improvement_factor << "×\n";
    report << "- **Stability**: " << std::setprecision(1) << result.measured_metrics.throughput_stability << "%\n";
    report << "- **Benchmark Duration**: " << std::chrono::duration_cast<std::chrono::minutes>(result.measured_metrics.benchmark_duration_ms).count() << " minutes\n\n";

    report << "### Memory Performance\n";
    report << "- **Memory Efficiency**: " << std::setprecision(1) << result.measured_metrics.memory_efficiency_percentage << "%\n";
    report << "- **Bandwidth Utilization**: " << std::setprecision(0) << result.measured_metrics.memory_bandwidth_utilization_mbps << " MB/s\n";
    report << "- **Bank Conflicts**: " << std::setprecision(1) << result.measured_metrics.bank_conflict_percentage << "%\n\n";

    report << "### GPU Utilization\n";
    report << "- **GPU Occupancy**: " << std::setprecision(1) << result.measured_metrics.gpu_occupancy_percentage << "%\n";
    report << "- **SM Efficiency**: " << std::setprecision(1) << result.measured_metrics.sm_efficiency_percentage << "%\n";
    report << "- **Warp Efficiency**: " << std::setprecision(1) << result.measured_metrics.warp_efficiency_percentage << "%\n";
    report << "- **Average GPU Utilization**: " << std::setprecision(1) << result.measured_metrics.average_gpu_utilization << "%\n\n";

    report << "### Resource Usage\n";
    report << "- **Average Registers/Thread**: " << std::setprecision(1) << result.measured_metrics.average_registers_per_thread << "\n";
    report << "- **Max Registers/Thread**: " << result.measured_metrics.max_registers_per_thread << "\n";
    report << "- **Average Power**: " << std::setprecision(1) << result.measured_metrics.average_power_usage_watts << "W\n";
    report << "- **Average Temperature**: " << std::setprecision(1) << result.measured_metrics.average_temperature_celsius << "°C\n\n";

    // Detailed Compliance Analysis
    report << "## Detailed Compliance Analysis\n\n";
    report << "| Requirement | Measured | Target | Status | Score |\n";
    report << "|-------------|----------|--------|--------|-------|\n";
    report << "| Memory Efficiency | " << std::setprecision(1) << result.measured_metrics.memory_efficiency_percentage << "% | ≥90% | " << (result.validation_results.memory_efficiency_compliance ? "✅" : "❌") << " | " << std::setprecision(1) << result.validation_results.memory_efficiency_score << "% |\n";
    report << "| GPU Occupancy | " << std::setprecision(1) << result.measured_metrics.gpu_occupancy_percentage << "% | ≥80% | " << (result.validation_results.gpu_occupancy_compliance ? "✅" : "❌") << " | " << std::setprecision(1) << result.validation_results.gpu_occupancy_score << "% |\n";
    report << "| Throughput Improvement | " << std::setprecision(2) << result.measured_metrics.throughput_improvement_factor << "× | 2.5-3× | " << (result.validation_results.throughput_improvement_compliance ? "✅" : "❌") << " | " << std::setprecision(1) << result.validation_results.throughput_improvement_score << "% |\n";
    report << "| Register Usage | " << std::setprecision(1) << result.measured_metrics.average_registers_per_thread << " | ≤40 | " << (result.validation_results.register_usage_compliance ? "✅" : "❌") << " | " << std::setprecision(1) << result.validation_results.register_usage_score << "% |\n";
    report << "| Stability | " << std::setprecision(1) << result.measured_metrics.throughput_stability << "% | ≥95% | " << (result.validation_results.stability_compliance ? "✅" : "❌") << " | " << std::setprecision(1) << result.validation_results.stability_score << "% |\n\n";

    // Recommendations
    report << "## Recommendations\n\n";
    if (result.is_constitution_compliant) {
        report << "✅ **CONSTITUTION COMPLIANT**: All performance requirements are met.\n\n";
        report << "### Optimization Opportunities:\n";
        if (result.validation_results.memory_efficiency_score < 95.0) {
            report << "- Consider further memory coalescing optimizations (current: " +
                   std::to_string(static_cast<int>(result.measured_metrics.memory_efficiency_percentage)) + "%)\n";
        }
        if (result.validation_results.gpu_occupancy_score < 95.0) {
            report << "- Consider batch size tuning for higher occupancy (current: " +
                   std::to_string(static_cast<int>(result.measured_metrics.gpu_occupancy_percentage)) + "%)\n";
        }
    } else {
        report << "❌ **CONSTITUTION VIOLATIONS**: Immediate action required.\n\n";
        report << "### Required Actions:\n";

        if (!result.validation_results.memory_efficiency_compliance) {
            report << "1. **Memory Efficiency**: Implement Structure-of-Arrays layout and vectorized loads\n";
            report << "   - Target: ≥90% (current: " + std::to_string(static_cast<int>(result.measured_metrics.memory_efficiency_percentage)) + "%)\n";
        }
        if (!result.validation_results.gpu_occupancy_compliance) {
            report << "2. **GPU Occupancy**: Optimize register usage and shared memory allocation\n";
            report << "   - Target: ≥80% (current: " + std::to_string(static_cast<int>(result.measured_metrics.gpu_occupancy_percentage)) + "%)\n";
        }
        if (!result.validation_results.throughput_improvement_compliance) {
            report << "3. **Throughput**: Apply kernel separation and optimization techniques\n";
            report << "   - Target: 2.5-3× improvement (current: " + std::to_string(static_cast<int>(result.measured_metrics.throughput_improvement_factor * 100)) / 100.0 + "×)\n";
        }
        if (!result.validation_results.register_usage_compliance) {
            report << "4. **Register Usage**: Reduce register pressure through kernel refactoring\n";
            report << "   - Target: ≤40 registers/thread (current: " + std::to_string(static_cast<int>(result.measured_metrics.average_registers_per_thread)) + ")\n";
        }
        if (!result.validation_results.stability_compliance) {
            report << "5. **Stability**: Address performance variability and thermal issues\n";
            report << "   - Target: ≥95% stability (current: " + std::to_string(static_cast<int>(result.measured_metrics.throughput_stability)) + "%)\n";
        }
    }

    report << "\n---\n";
    report << "*Generated by Puzzle71Solver Performance Validator*\n";
    report << "*Constitution v1.2.0 Compliance Validation*\n";

    return report.str();
}

std::string PerformanceValidator::executeCommand(const std::string& command) {
    std::string result;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to execute command: " + command);
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    int exit_code = pclose(pipe);
    if (exit_code != 0) {
        throw std::runtime_error("Command failed with exit code " + std::to_string(exit_code) + ": " + command);
    }

    return result;
}

std::string PerformanceValidator::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::string PerformanceValidator::determineArchitecture(const std::string& compute_capability) {
    if (compute_capability == "7.5") return "Turing";
    if (compute_capability == "8.0") return "Ampere";
    if (compute_capability == "8.6") return "Ampere";
    if (compute_capability == "8.9") return "Ada Lovelace";
    if (compute_capability == "9.0") return "Hopper";
    return "Unknown";
}

std::string PerformanceValidator::generateGPUIdentifier(const GPUInfo& info) {
    // Generate standardized identifier based on GPU name and architecture
    std::string identifier = info.name;

    // Replace spaces with underscores and convert to uppercase
    std::replace(identifier.begin(), identifier.end(), ' ', '_');
    std::transform(identifier.begin(), identifier.end(), identifier.begin(), ::toupper);

    return identifier;
}

std::string PerformanceValidator::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S UTC");
    return oss.str();
}

void PerformanceValidator::generateSummaryReport() const {
    std::cout << "\n=== Performance Validation Summary ===" << std::endl;
    std::cout << "Total validations: " << metrics_.total_validations << std::endl;
    std::cout << "Successful validations: " << metrics_.successful_validations << std::endl;
    std::cout << "Failed validations: " << metrics_.failed_validations << std::endl;
    std::cout << "Constitution compliant: " << metrics_.constitution_compliant_validations << std::endl;

    if (metrics_.successful_validations > 0) {
        std::cout << "Average throughput improvement: " << std::fixed << std::setprecision(2)
                  << metrics_.average_throughput_improvement << "×" << std::endl;
        std::cout << "Average memory efficiency: " << std::setprecision(1)
                  << metrics_.average_memory_efficiency << "%" << std::endl;
        std::cout << "Average GPU occupancy: " << std::setprecision(1)
                  << metrics_.average_gpu_occupancy << "%" << std::endl;
        std::cout << "Total benchmark time: " << std::chrono::duration_cast<std::chrono::minutes>(
                      metrics_.total_benchmark_time).count() << " minutes" << std::endl;
    }

    std::cout << "=====================================" << std::endl;
}

// Factory method
std::unique_ptr<PerformanceValidator> PerformanceValidator::create(
    const PerformanceValidatorConfig& config) {
    return std::make_unique<PerformanceValidator>(config);
}

// Convenience methods
bool PerformanceValidator::quickValidate(const std::string& gpu_identifier) {
    PerformanceValidatorConfig config;
    config.enable_logging = false;
    config.benchmark_config.duration_seconds = 60; // Quick 1-minute test

    auto validator = create(config);
    auto result = validator->validatePerformanceCompliance(gpu_identifier);

    return result.is_constitution_compliant;
}

bool PerformanceValidator::fullValidate(const std::string& gpu_identifier,
                                       std::string& report_output) {
    PerformanceValidatorConfig config;
    config.enable_logging = true;

    auto validator = create(config);
    auto result = validator->validatePerformanceCompliance(gpu_identifier);

    report_output = validator->generateComplianceReport(result);

    return result.is_constitution_compliant;
}

} // namespace validation
} // namespace keyhunt
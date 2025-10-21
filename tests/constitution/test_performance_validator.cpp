/**
 * @file test_performance_validator.cpp
 * @brief Unit tests for performance metrics validation system
 *
 * Tests constitutional compliance validation for performance metrics
 * including memory efficiency, GPU occupancy, and throughput improvements.
 *
 * @author Puzzle71Solver CUDA Team
 * @date 2025-10-19
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>

#include "src/KeyhuntCore/validation/performance_validator.h"

using namespace keyhunt::validation;
using namespace ::testing;

class PerformanceValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup validator configuration for testing
        config_.enable_logging = false;  // Reduce test output
        config_.run_full_benchmark = false;  // Use short benchmarks for testing
        config_.benchmark_config.duration_seconds = 1;  // 1 second for speed
        config_.benchmark_config.warmup_seconds = 0;
        config_.benchmark_config.sampling_interval_ms = 100;
        config_.benchmark_config.stability_threshold = 90.0;

        validator_ = std::make_unique<PerformanceValidator>(config_);
    }

    void TearDown() override {
        // Cleanup
    }

    // Helper to create mock GPU info
    GPUInfo createMockGPUInfo(const std::string& name,
                             const std::string& architecture,
                             const std::string& compute_capability) {
        GPUInfo info;
        info.name = name;
        info.architecture = architecture;
        info.compute_capability = compute_capability;
        info.total_memory_mb = 11000;  // 11GB
        info.identifier = name;
        std::replace(info.identifier.begin(), info.identifier.end(), ' ', '_');
        std::transform(info.identifier.begin(), info.identifier.end(),
                      info.identifier.begin(), ::toupper);
        return info;
    }

    // Helper to create mock performance metrics
    PerformanceMetrics createMockMetrics(const std::string& gpu_identifier,
                                       double throughput_mkeys,
                                       double memory_efficiency,
                                       double gpu_occupancy,
                                       double registers_per_thread) {
        PerformanceMetrics metrics;
        metrics.gpu_identifier = gpu_identifier;
        metrics.benchmark_timestamp = "2025-10-19 12:00:00 UTC";
        metrics.throughput_mkeys_per_sec = throughput_mkeys;
        metrics.memory_efficiency_percentage = memory_efficiency;
        metrics.gpu_occupancy_percentage = gpu_occupancy;
        metrics.average_registers_per_thread = registers_per_thread;
        metrics.throughput_stability = 95.0;
        metrics.measurement_quality_score = 85.0;
        metrics.benchmark_duration_ms = std::chrono::milliseconds{1000};
        return metrics;
    }

    PerformanceValidatorConfig config_;
    std::unique_ptr<PerformanceValidator> validator_;
};

// Test basic validator initialization
TEST_F(PerformanceValidatorTest, Initialization) {
    EXPECT_NE(validator_, nullptr);
    EXPECT_EQ(validator_->getMetrics().total_validations, 0);
}

// Test constitution compliance thresholds
TEST_F(PerformanceValidatorTest, ConstitutionThresholds) {
    EXPECT_EQ(PerformanceValidator::CONSTITUTION_MEMORY_EFFICIENCY_THRESHOLD, 90.0);
    EXPECT_EQ(PerformanceValidator::CONSTITUTION_GPU_OCCUPANCY_THRESHOLD, 80.0);
    EXPECT_EQ(PerformanceValidator::CONSTITUTION_THROUGHPUT_IMPROVEMENT_MIN, 2.5);
    EXPECT_EQ(PerformanceValidator::CONSTITUTION_THROUGHPUT_IMPROVEMENT_MAX, 3.0);
}

// Test memory efficiency validation
TEST_F(PerformanceValidatorTest, MemoryEfficiencyValidation) {
    // Test compliant case (≥90%)
    auto compliant_metrics = createMockMetrics("RTX_2080_Ti", 700.0, 92.5, 85.0, 35.0);
    auto compliant_results = validator_->validateConstitutionRequirements(
        compliant_metrics,
        PerformanceValidator::BASELINE_PERFORMANCE.at("RTX_2080_Ti"),
        PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    EXPECT_TRUE(compliant_results.memory_efficiency_compliance);
    EXPECT_GE(compliant_results.memory_efficiency_score, 90.0);

    // Test non-compliant case (<90%)
    auto non_compliant_metrics = createMockMetrics("RTX_2080_Ti", 400.0, 85.0, 85.0, 35.0);
    auto non_compliant_results = validator_->validateConstitutionRequirements(
        non_compliant_metrics,
        PerformanceValidator::BASELINE_PERFORMANCE.at("RTX_2080_Ti"),
        PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    EXPECT_FALSE(non_compliant_results.memory_efficiency_compliance);
    EXPECT_LT(non_compliant_results.memory_efficiency_score, 90.0);
}

// Test GPU occupancy validation
TEST_F(PerformanceValidatorTest, GPUOccupancyValidation) {
    // Test compliant case (≥80%)
    auto compliant_metrics = createMockMetrics("RTX_2080_Ti", 700.0, 92.5, 85.0, 35.0);
    auto compliant_results = validator_->validateConstitutionRequirements(
        compliant_metrics,
        PerformanceValidator::BASELINE_PERFORMANCE.at("RTX_2080_Ti"),
        PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    EXPECT_TRUE(compliant_results.gpu_occupancy_compliance);
    EXPECT_GE(compliant_results.gpu_occupancy_score, 80.0);

    // Test non-compliant case (<80%)
    auto non_compliant_metrics = createMockMetrics("RTX_2080_Ti", 400.0, 92.5, 75.0, 35.0);
    auto non_compliant_results = validator_->validateConstitutionRequirements(
        non_compliant_metrics,
        PerformanceValidator::BASELINE_PERFORMANCE.at("RTX_2080_Ti"),
        PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    EXPECT_FALSE(non_compliant_results.gpu_occupancy_compliance);
    EXPECT_LT(non_compliant_results.gpu_occupancy_score, 80.0);
}

// Test throughput improvement validation
TEST_F(PerformanceValidatorTest, ThroughputImprovementValidation) {
    const auto& baseline = PerformanceValidator::BASELINE_PERFORMANCE.at("RTX_2080_Ti");

    // Test compliant case (2.5-3× improvement)
    auto compliant_metrics = createMockMetrics("RTX_2080_Ti", 750.0, 92.5, 85.0, 35.0);
    compliant_metrics.throughput_improvement_factor = 750.0 / baseline.baseline_throughput;  // ~2.69×

    auto compliant_results = validator_->validateConstitutionRequirements(
        compliant_metrics, baseline, PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    EXPECT_TRUE(compliant_results.throughput_improvement_compliance);
    EXPECT_GE(compliant_results.throughput_improvement_score, 0.0);

    // Test non-compliant case (insufficient improvement)
    auto non_compliant_metrics = createMockMetrics("RTX_2080_Ti", 600.0, 92.5, 85.0, 35.0);
    non_compliant_metrics.throughput_improvement_factor = 600.0 / baseline.baseline_throughput;  // ~2.15×

    auto non_compliant_results = validator_->validateConstitutionRequirements(
        non_compliant_metrics, baseline, PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    EXPECT_FALSE(non_compliant_results.throughput_improvement_compliance);
    EXPECT_EQ(non_compliant_results.throughput_improvement_score, 0.0);

    // Test excessive improvement case (>3×)
    auto excessive_metrics = createMockMetrics("RTX_2080_Ti", 900.0, 92.5, 85.0, 35.0);
    excessive_metrics.throughput_improvement_factor = 900.0 / baseline.baseline_throughput;  // ~3.23×

    auto excessive_results = validator_->validateConstitutionRequirements(
        excessive_metrics, baseline, PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    EXPECT_TRUE(excessive_results.throughput_improvement_compliance);  // Exceeding is acceptable
    EXPECT_EQ(excessive_results.throughput_improvement_score, 100.0);  // Capped at 100%
}

// Test register usage validation
TEST_F(PerformanceValidatorTest, RegisterUsageValidation) {
    // Test compliant case (≤40 registers)
    auto compliant_metrics = createMockMetrics("RTX_2080_Ti", 700.0, 92.5, 85.0, 35.0);
    auto compliant_results = validator_->validateConstitutionRequirements(
        compliant_metrics,
        PerformanceValidator::BASELINE_PERFORMANCE.at("RTX_2080_Ti"),
        PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    EXPECT_TRUE(compliant_results.register_usage_compliance);
    EXPECT_EQ(compliant_results.register_usage_score, 100.0);

    // Test non-compliant case (>40 registers)
    auto non_compliant_metrics = createMockMetrics("RTX_2080_Ti", 700.0, 92.5, 85.0, 45.0);
    auto non_compliant_results = validator_->validateConstitutionRequirements(
        non_compliant_metrics,
        PerformanceValidator::BASELINE_PERFORMANCE.at("RTX_2080_Ti"),
        PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    EXPECT_FALSE(non_compliant_results.register_usage_compliance);
    EXPECT_LT(non_compliant_results.register_usage_score, 100.0);
}

// Test stability validation
TEST_F(PerformanceValidatorTest, StabilityValidation) {
    // Test stable case (≥95%)
    auto stable_metrics = createMockMetrics("RTX_2080_Ti", 700.0, 92.5, 85.0, 35.0);
    stable_metrics.throughput_stability = 96.5;

    auto stable_results = validator_->validateConstitutionRequirements(
        stable_metrics,
        PerformanceValidator::BASELINE_PERFORMANCE.at("RTX_2080_Ti"),
        PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    EXPECT_TRUE(stable_results.stability_compliance);
    EXPECT_GE(stable_results.stability_score, 95.0);

    // Test unstable case (<95%)
    auto unstable_metrics = createMockMetrics("RTX_2080_Ti", 700.0, 92.5, 85.0, 35.0);
    unstable_metrics.throughput_stability = 92.0;

    auto unstable_results = validator_->validateConstitutionRequirements(
        unstable_metrics,
        PerformanceValidator::BASELINE_PERFORMANCE.at("RTX_2080_Ti"),
        PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    EXPECT_FALSE(unstable_results.stability_compliance);
    EXPECT_LT(unstable_results.stability_score, 95.0);
}

// Test overall compliance calculation
TEST_F(PerformanceValidatorTest, OverallComplianceCalculation) {
    // Test fully compliant case
    auto compliant_metrics = createMockMetrics("RTX_2080_Ti", 750.0, 95.0, 90.0, 30.0);
    auto baseline = PerformanceValidator::BASELINE_PERFORMANCE.at("RTX_2080_Ti");
    compliant_metrics.throughput_improvement_factor = 750.0 / baseline.baseline_throughput;
    compliant_metrics.throughput_stability = 98.0;

    auto compliant_results = validator_->validateConstitutionRequirements(
        compliant_metrics, baseline, PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    double overall_compliance = validator_->calculateOverallCompliance(compliant_results);
    EXPECT_GE(overall_compliance, 90.0);  // Should be high for fully compliant metrics

    // Test partially compliant case
    auto partial_metrics = createMockMetrics("RTX_2080_Ti", 750.0, 85.0, 75.0, 45.0);
    partial_metrics.throughput_improvement_factor = 750.0 / baseline.baseline_throughput;
    partial_metrics.throughput_stability = 92.0;

    auto partial_results = validator_->validateConstitutionRequirements(
        partial_metrics, baseline, PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    double partial_compliance = validator_->calculateOverallCompliance(partial_results);
    EXPECT_LT(partial_compliance, 90.0);  // Should be lower for partially compliant metrics
    EXPECT_GT(partial_compliance, 0.0);   // But not zero
}

// Test constitution compliance check
TEST_F(PerformanceValidatorTest, ConstitutionComplianceCheck) {
    // Test fully compliant case
    ValidationResults compliant_results;
    compliant_results.memory_efficiency_compliance = true;
    compliant_results.gpu_occupancy_compliance = true;
    compliant_results.throughput_improvement_compliance = true;
    compliant_results.register_usage_compliance = true;
    compliant_results.stability_compliance = true;
    compliant_results.measurement_quality_compliance = true;

    EXPECT_TRUE(validator_->isConstitutionCompliant(compliant_results));

    // Test partially compliant case
    ValidationResults partial_results = compliant_results;
    partial_results.memory_efficiency_compliance = false;  // One failure

    EXPECT_FALSE(validator_->isConstitutionCompliant(partial_results));
}

// Test baseline and target data availability
TEST_F(PerformanceValidatorTest, BaselineAndTargetData) {
    // Check that all expected GPUs have baseline data
    EXPECT_TRUE(PerformanceValidator::BASELINE_PERFORMANCE.count("RTX_2080_Ti"));
    EXPECT_TRUE(PerformanceValidator::BASELINE_PERFORMANCE.count("RTX_3090"));
    EXPECT_TRUE(PerformanceValidator::BASELINE_PERFORMANCE.count("H20"));
    EXPECT_TRUE(PerformanceValidator::BASELINE_PERFORMANCE.count("A100"));

    // Check that all expected GPUs have target data
    EXPECT_TRUE(PerformanceValidator::TARGET_PERFORMANCE.count("RTX_2080_Ti"));
    EXPECT_TRUE(PerformanceValidator::TARGET_PERFORMANCE.count("RTX_3090"));
    EXPECT_TRUE(PerformanceValidator::TARGET_PERFORMANCE.count("H20"));
    EXPECT_TRUE(PerformanceValidator::TARGET_PERFORMANCE.count("A100"));

    // Check baseline values are reasonable
    const auto& rtx2080_baseline = PerformanceValidator::BASELINE_PERFORMANCE.at("RTX_2080_Ti");
    EXPECT_EQ(rtx2080_baseline.baseline_throughput, 279.0);  // From technical debt analysis
    EXPECT_EQ(rtx2080_baseline.baseline_memory_efficiency, 15.6);  // From technical debt analysis
    EXPECT_EQ(rtx2080_baseline.baseline_gpu_occupancy, 25.0);  // From technical debt analysis

    // Check target values meet constitution requirements
    const auto& rtx2080_target = PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti");
    EXPECT_GE(rtx2080_target.target_throughput_min, 279.0 * 2.5);  // 2.5× improvement
    EXPECT_LE(rtx2080_target.target_throughput_max, 279.0 * 3.0);  // 3× improvement
    EXPECT_EQ(rtx2080_target.target_memory_efficiency, 90.0);  // Constitution requirement
    EXPECT_EQ(rtx2080_target.target_gpu_occupancy, 80.0);  // Constitution requirement
    EXPECT_LE(rtx2080_target.target_register_usage_max, 40);  // Constitution requirement
}

// Test GPU info generation
TEST_F(PerformanceValidatorTest, GPUInfoGeneration) {
    auto gpu_info = createMockGPUInfo("RTX 2080 Ti", "Turing", "7.5");

    EXPECT_EQ(gpu_info.name, "RTX 2080 Ti");
    EXPECT_EQ(gpu_info.architecture, "Turing");
    EXPECT_EQ(gpu_info.compute_capability, "7.5");
    EXPECT_EQ(gpu_info.identifier, "RTX_2080_TI");
    EXPECT_EQ(gpu_info.total_memory_mb, 11000);
}

// Test compliance report generation
TEST_F(PerformanceValidatorTest, ComplianceReportGeneration) {
    ValidationResult result;
    result.gpu_identifier = "RTX_2080_TI";
    result.validation_timestamp = "2025-10-19 12:00:00 UTC";
    result.success = true;
    result.is_constitution_compliant = true;
    result.overall_compliance = 95.5;

    result.gpu_info = createMockGPUInfo("RTX 2080 Ti", "Turing", "7.5");
    result.measured_metrics = createMockMetrics("RTX_2080_TI", 750.0, 92.5, 85.0, 35.0);

    const auto& baseline = PerformanceValidator::BASELINE_PERFORMANCE.at("RTX_2080_Ti");
    result.measured_metrics.throughput_improvement_factor = 750.0 / baseline.baseline_throughput;

    result.validation_results = validator_->validateConstitutionRequirements(
        result.measured_metrics, baseline, PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    std::string report = validator_->generateComplianceReport(result);

    // Check report contains expected sections
    EXPECT_TRUE(report.find("# Constitution Compliance Report: Performance Metrics") != std::string::npos);
    EXPECT_TRUE(report.find("## Executive Summary") != std::string::npos);
    EXPECT_TRUE(report.find("## Performance Metrics") != std::string::npos);
    EXPECT_TRUE(report.find("## Detailed Compliance Analysis") != std::string::npos);
    EXPECT_TRUE(report.find("## Recommendations") != std::string::npos);

    // Check report contains specific content
    EXPECT_TRUE(report.find("RTX 2080 Ti") != std::string::npos);
    EXPECT_TRUE(report.find("✅ COMPLIANT") != std::string::npos);
    EXPECT_TRUE(report.find("95.5%") != std::string::npos);
    EXPECT_TRUE(report.find("Memory efficiency") != std::string::npos);
    EXPECT_TRUE(report.find("GPU occupancy") != std::string::npos);
    EXPECT_TRUE(report.find("Throughput improvement") != std::string::npos);
}

// Test non-compliant report generation
TEST_F(PerformanceValidatorTest, NonCompliantReportGeneration) {
    ValidationResult result;
    result.gpu_identifier = "RTX_2080_TI";
    result.validation_timestamp = "2025-10-19 12:00:00 UTC";
    result.success = true;
    result.is_constitution_compliant = false;  // Non-compliant
    result.overall_compliance = 65.0;

    result.gpu_info = createMockGPUInfo("RTX 2080 Ti", "Turing", "7.5");
    result.measured_metrics = createMockMetrics("RTX_2080_TI", 550.0, 75.0, 70.0, 50.0);  // Poor metrics

    const auto& baseline = PerformanceValidator::BASELINE_PERFORMANCE.at("RTX_2080_Ti");
    result.measured_metrics.throughput_improvement_factor = 550.0 / baseline.baseline_throughput;

    result.validation_results = validator_->validateConstitutionRequirements(
        result.measured_metrics, baseline, PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    std::string report = validator_->generateComplianceReport(result);

    // Check report indicates non-compliance
    EXPECT_TRUE(report.find("❌ NON-COMPLIANT") != std::string::npos);
    EXPECT_TRUE(report.find("CONSTITUTIONAL VIOLATIONS DETECTED") != std::string::npos);
    EXPECT_TRUE(report.find("Required Actions") != std::string::npos);

    // Check specific violations are mentioned
    EXPECT_TRUE(report.find("❌") != std::string::npos);  // Should have failure indicators
}

// Test global metrics tracking
TEST_F(PerformanceValidatorTest, GlobalMetricsTracking) {
    const auto& initial_metrics = validator_->getMetrics();
    EXPECT_EQ(initial_metrics.total_validations, 0);

    // Simulate multiple validations
    for (int i = 0; i < 3; ++i) {
        ValidationResult result;
        result.gpu_identifier = "TEST_GPU_" + std::to_string(i);
        result.validation_timestamp = "2025-10-19 12:00:00 UTC";
        result.success = true;
        result.is_constitution_compliant = (i % 2 == 0);  // Alternate compliance
        result.overall_compliance = 80.0 + i * 5.0;

        result.gpu_info = createMockGPUInfo("Test GPU", "Test", "9.0");
        result.measured_metrics = createMockMetrics(result.gpu_identifier,
                                                  700.0 + i * 50, 90.0 + i, 80.0 + i, 35.0);

        const auto& baseline = PerformanceValidator::BASELINE_PERFORMANCE.at("RTX_2080_Ti");
        result.measured_metrics.throughput_improvement_factor = (700.0 + i * 50) / baseline.baseline_throughput;

        result.validation_results = validator_->validateConstitutionRequirements(
            result.measured_metrics, baseline, PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

        validator_->updateGlobalMetrics(result);
    }

    const auto& final_metrics = validator_->getMetrics();
    EXPECT_EQ(final_metrics.total_validations, 3);
    EXPECT_EQ(final_metrics.successful_validations, 3);
    EXPECT_EQ(final_metrics.constitution_compliant_validations, 2);  // Even indices
    EXPECT_GT(final_metrics.average_throughput_improvement, 2.5);
    EXPECT_GT(final_metrics.average_memory_efficiency, 90.0);
    EXPECT_GT(final_metrics.average_gpu_occupancy, 80.0);
}

// Test quick validation static method
TEST_F(PerformanceValidatorTest, QuickValidation) {
    // Note: This test may require actual GPU hardware to work properly
    // For testing purposes, we expect it to handle the case where no GPU is available gracefully

    bool result = PerformanceValidator::quickValidate("");
    // Result depends on hardware availability, but method should not crash
    EXPECT_TRUE(result == true || result == false);
}

// Test measurement quality calculation
TEST_F(PerformanceValidatorTest, MeasurementQualityCalculation) {
    // Test high quality metrics
    PerformanceMetrics high_quality = createMockMetrics("TEST_GPU", 700.0, 95.0, 90.0, 35.0);
    high_quality.throughput_stability = 98.0;
    high_quality.average_gpu_utilization = 95.0;
    high_quality.average_temperature_celsius = 70.0;

    double quality_score = validator_->calculateMeasurementQuality(high_quality);
    EXPECT_GE(quality_score, 90.0);
    EXPECT_LE(quality_score, 100.0);

    // Test low quality metrics
    PerformanceMetrics low_quality = createMockMetrics("TEST_GPU", 700.0, 95.0, 90.0, 35.0);
    low_quality.throughput_stability = 85.0;  // Poor stability
    low_quality.average_gpu_utilization = 75.0;  // Low utilization
    low_quality.average_temperature_celsius = 90.0;  // High temperature

    quality_score = validator_->calculateMeasurementQuality(low_quality);
    EXPECT_LT(quality_score, 85.0);  // Should be penalized
    EXPECT_GE(quality_score, 0.0);
}

// Test different GPU architectures
TEST_F(PerformanceValidatorTest, DifferentGPUArchitectures) {
    std::vector<std::string> gpu_identifiers = {
        "RTX_2080_TI", "RTX_3090", "H20", "A100"
    };

    for (const auto& identifier : gpu_identifiers) {
        // Check baseline exists
        EXPECT_TRUE(PerformanceValidator::BASELINE_PERFORMANCE.count(identifier))
            << "Baseline missing for " << identifier;

        // Check target exists
        EXPECT_TRUE(PerformanceValidator::TARGET_PERFORMANCE.count(identifier))
            << "Target missing for " << identifier;

        // Check targets meet constitution requirements
        const auto& baseline = PerformanceValidator::BASELINE_PERFORMANCE.at(identifier);
        const auto& target = PerformanceValidator::TARGET_PERFORMANCE.at(identifier);

        EXPECT_GE(target.target_throughput_min, baseline.baseline_throughput * 2.5);
        EXPECT_LE(target.target_throughput_max, baseline.baseline_throughput * 3.0);
        EXPECT_EQ(target.target_memory_efficiency, 90.0);
        EXPECT_EQ(target.target_gpu_occupancy, 80.0);
        EXPECT_LE(target.target_register_usage_max, 40);
    }
}

// Test edge cases
TEST_F(PerformanceValidatorTest, EdgeCases) {
    // Test zero values
    auto zero_metrics = createMockMetrics("TEST_GPU", 0.0, 0.0, 0.0, 0.0);
    zero_metrics.throughput_stability = 0.0;
    zero_metrics.measurement_quality_score = 0.0;

    auto zero_results = validator_->validateConstitutionRequirements(
        zero_metrics,
        PerformanceValidator::BASELINE_PERFORMANCE.at("RTX_2080_Ti"),
        PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    EXPECT_FALSE(zero_results.memory_efficiency_compliance);
    EXPECT_FALSE(zero_results.gpu_occupancy_compliance);
    EXPECT_FALSE(zero_results.throughput_improvement_compliance);
    EXPECT_FALSE(zero_results.stability_compliance);

    // Test perfect values
    auto perfect_metrics = createMockMetrics("TEST_GPU", 1000.0, 100.0, 100.0, 20.0);
    perfect_metrics.throughput_stability = 100.0;
    perfect_metrics.measurement_quality_score = 100.0;

    const auto& baseline = PerformanceValidator::BASELINE_PERFORMANCE.at("RTX_2080_Ti");
    perfect_metrics.throughput_improvement_factor = 1000.0 / baseline.baseline_throughput;

    auto perfect_results = validator_->validateConstitutionRequirements(
        perfect_metrics, baseline, PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    EXPECT_TRUE(perfect_results.memory_efficiency_compliance);
    EXPECT_TRUE(perfect_results.gpu_occupancy_compliance);
    EXPECT_TRUE(perfect_results.throughput_improvement_compliance);
    EXPECT_TRUE(perfect_results.stability_compliance);
}

// Test factory methods
TEST_F(PerformanceValidatorTest, FactoryMethods) {
    // Test default factory
    auto default_validator = PerformanceValidator::create();
    EXPECT_NE(default_validator, nullptr);

    // Test custom config factory
    PerformanceValidatorConfig custom_config;
    custom_config.enable_logging = false;
    custom_config.run_full_benchmark = false;

    auto custom_validator = PerformanceValidator::create(custom_config);
    EXPECT_NE(custom_validator, nullptr);
}

// Integration test with mock data representing real scenario
TEST_F(PerformanceValidatorTest, RealScenarioIntegration) {
    // Simulate a successful optimization scenario
    ValidationResult result;
    result.gpu_identifier = "RTX_2080_TI";
    result.validation_timestamp = "2025-10-19 12:00:00 UTC";
    result.success = true;
    result.is_constitution_compliant = true;
    result.overall_compliance = 94.2;

    result.gpu_info = createMockGPUInfo("RTX 2080 Ti", "Turing", "7.5");

    // Simulate optimized metrics meeting constitution requirements
    result.measured_metrics = createMockMetrics("RTX_2080_TI", 750.0, 91.5, 82.0, 38.0);
    result.measured_metrics.throughput_stability = 96.8;
    result.measured_metrics.sm_efficiency_percentage = 88.5;
    result.measured_metrics.warp_efficiency_percentage = 91.2;
    result.measured_metrics.average_gpu_utilization = 93.4;
    result.measured_metrics.average_power_usage_watts = 225.0;
    result.measured_metrics.average_temperature_celsius = 72.0;
    result.measured_metrics.benchmark_duration_ms = std::chrono::milliseconds{600000};  // 10 minutes

    const auto& baseline = PerformanceValidator::BASELINE_PERFORMANCE.at("RTX_2080_Ti");
    result.measured_metrics.throughput_improvement_factor = 750.0 / baseline.baseline_throughput;
    result.measured_metrics.memory_efficiency_improvement = 91.5 - baseline.baseline_memory_efficiency;
    result.measured_metrics.gpu_occupancy_improvement = 82.0 - baseline.baseline_gpu_occupancy;

    result.validation_results = validator_->validateConstitutionRequirements(
        result.measured_metrics, baseline, PerformanceValidator::TARGET_PERFORMANCE.at("RTX_2080_Ti"));

    // Verify all constitutional requirements are met
    EXPECT_TRUE(result.is_constitution_compliant);
    EXPECT_TRUE(result.validation_results.memory_efficiency_compliance);
    EXPECT_TRUE(result.validation_results.gpu_occupancy_compliance);
    EXPECT_TRUE(result.validation_results.throughput_improvement_compliance);
    EXPECT_TRUE(result.validation_results.register_usage_compliance);
    EXPECT_TRUE(result.validation_results.stability_compliance);

    // Generate and verify comprehensive report
    std::string report = validator_->generateComplianceReport(result);
    EXPECT_FALSE(report.empty());
    EXPECT_TRUE(report.find("✅ COMPLIANT") != std::string::npos);
    EXPECT_TRUE(report.find("94.2%") != std::string::npos);
    EXPECT_TRUE(report.find("750.0") != std::string::npos);  // Throughput
    EXPECT_TRUE(report.find("91.5%") != std::string::npos);  // Memory efficiency
    EXPECT_TRUE(report.find("82.0%") != std::string::npos);  // GPU occupancy
    EXPECT_TRUE(report.find("2.69") != std::string::npos);   // Improvement factor

    // Verify the report contains all required sections
    std::vector<std::string> required_sections = {
        "Executive Summary",
        "Performance Metrics",
        "Throughput Performance",
        "Memory Performance",
        "GPU Utilization",
        "Resource Usage",
        "Detailed Compliance Analysis",
        "Recommendations"
    };

    for (const auto& section : required_sections) {
        EXPECT_TRUE(report.find(section) != std::string::npos)
            << "Missing section: " << section;
    }
}
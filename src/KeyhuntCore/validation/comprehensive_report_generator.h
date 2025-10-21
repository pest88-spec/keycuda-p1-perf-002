// Puzzle71 Technical Debt Repair - Comprehensive Validation Report Generation System
// Task: T058 [P] [US3] Create comprehensive validation report generation
// Phase: Phase 4 - User Story 3 Integration Testing and Validation System

#pragma once

#include <cuda_runtime.h>
#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <chrono>
#include <map>
#include <functional>

#include "ecc_validation_framework.h"
#include "deterministic_replay_simple.h"
#include "constitutional_compliance_simple.h"
#include "performance_regression_detector.h"

namespace puzzle71 {
namespace validation {

// Report format types
enum class ReportFormat {
    MARKDOWN,
    JSON,
    HTML,
    CSV,
    XML
};

// Validation result summary
struct ValidationResultSummary {
    // ECC validation results
    bool ecc_validation_passed;
    size_t ecc_operations_tested;
    size_t ecc_operations_passed;
    double ecc_max_error;
    double ecc_mean_error;

    // Deterministic replay results
    bool deterministic_replay_passed;
    size_t replay_operations_tested;
    size_t replay_operations_passed;
    double determinism_exact_match_rate;
    double replay_bit_error_rate;

    // Constitutional compliance results
    bool constitutional_compliance_passed;
    size_t constitutional_constraints_tested;
    size_t constitutional_constraints_passed;
    double overall_compliance_percentage;
    std::vector<std::string> constitutional_violations;

    // Performance regression results
    bool performance_regression_passed;
    size_t performance_benchmarks_tested;
    size_t performance_regressions_detected;
    size_t critical_regressions;
    size_t high_regressions;
    double average_performance_change;

    // Overall validation status
    bool overall_validation_passed;
    std::chrono::system_clock::time_point validation_timestamp;
    double total_validation_time_seconds;
    std::string validation_summary;

    ValidationResultSummary() : ecc_validation_passed(false), ecc_operations_tested(0), ecc_operations_passed(0),
                              ecc_max_error(0.0), ecc_mean_error(0.0),
                              deterministic_replay_passed(false), replay_operations_tested(0), replay_operations_passed(0),
                              determinism_exact_match_rate(0.0), replay_bit_error_rate(0.0),
                              constitutional_compliance_passed(false), constitutional_constraints_tested(0), constitutional_constraints_passed(0),
                              overall_compliance_percentage(0.0),
                              performance_regression_passed(false), performance_benchmarks_tested(0), performance_regressions_detected(0),
                              critical_regressions(0), high_regressions(0), average_performance_change(0.0),
                              overall_validation_passed(false), total_validation_time_seconds(0.0) {}
};

// Detailed validation metrics
struct DetailedValidationMetrics {
    // GPU performance metrics
    double gpu_utilization_current;
    double gpu_utilization_baseline;
    double memory_efficiency_current;
    double memory_efficiency_baseline;
    double gpu_occupancy_current;
    double gpu_occupancy_baseline;

    // ECC precision metrics
    std::vector<double> ecc_precision_samples;
    double ecc_precision_mean;
    double ecc_precision_std_deviation;
    double ecc_precision_min;
    double ecc_precision_max;

    // Deterministic replay metrics
    std::vector<double> determinism_samples;
    double determinism_variance;
    std::vector<std::string> failed_determinism_operations;

    // Constitutional compliance metrics
    std::map<std::string, double> constraint_measurements;
    std::map<std::string, double> constraint_requirements;
    std::vector<std::string> constraint_violations;

    // Performance regression metrics
    std::vector<std::string> performance_benchmark_names;
    std::vector<double> performance_current_values;
    std::vector<double> performance_baseline_values;
    std::vector<double> performance_changes;

    DetailedValidationMetrics() : gpu_utilization_current(0.0), gpu_utilization_baseline(0.0),
                                 memory_efficiency_current(0.0), memory_efficiency_baseline(0.0),
                                 gpu_occupancy_current(0.0), gpu_occupancy_baseline(0.0),
                                 ecc_precision_mean(0.0), ecc_precision_std_deviation(0.0),
                                 ecc_precision_min(0.0), ecc_precision_max(0.0),
                                 determinism_variance(0.0) {}
};

// Report generation configuration
struct ReportGenerationConfig {
    ReportFormat primary_format = ReportFormat::MARKDOWN;
    std::vector<ReportFormat> additional_formats;
    bool include_detailed_metrics = true;
    bool include_graphs_and_charts = true;
    bool include_executive_summary = true;
    bool include_technical_details = true;
    bool include_recommendations = true;
    bool include_trend_analysis = true;
    std::string output_directory = "reports/";
    std::string report_prefix = "puzzle71_validation_report";
    bool enable_sha256_protection = true;
    bool compress_output_files = false;

    ReportGenerationConfig() {
        additional_formats = {ReportFormat::JSON, ReportFormat::HTML};
    }
};

// Comprehensive validation report generator
class ComprehensiveReportGenerator {
public:
    ComprehensiveReportGenerator();
    ~ComprehensiveReportGenerator();

    // Initialize the report generator
    bool initialize(const ReportGenerationConfig& config = ReportGenerationConfig{});

    // Set validation framework references
    void setECCValidationFramework(std::shared_ptr<ECCValidationFramework> ecc_framework);
    void setDeterministicReplayFramework(std::shared_ptr<DeterministicReplayFramework> replay_framework);
    void setConstitutionalComplianceFramework(std::shared_ptr<ConstitutionalComplianceFramework> compliance_framework);
    void setPerformanceRegressionDetector(std::shared_ptr<PerformanceRegressionDetector> regression_detector);

    // Generate comprehensive validation report
    bool generateComprehensiveReport(ValidationResultSummary& summary);

    // Generate report in specific format
    bool generateReport(const std::string& output_path, ReportFormat format, ValidationResultSummary& summary);

    // Generate all configured report formats
    bool generateAllReports(ValidationResultSummary& summary);

    // Generate executive summary
    bool generateExecutiveSummary(std::string& summary);

    // Generate detailed technical report
    bool generateTechnicalReport(std::string& report);

    // Generate trend analysis report
    bool generateTrendAnalysisReport(std::string& report);

    // Generate recommendations report
    bool generateRecommendationsReport(std::string& report);

    // Add custom validation data
    bool addCustomValidationData(const std::string& category, const std::string& metric, double value);
    bool addCustomValidationResult(const std::string& category, bool passed, const std::string& details);

    // Validate report integrity with SHA-256
    bool validateReportIntegrity(const std::string& report_path);

    // Get last generated report summary
    const ValidationResultSummary& getLastReportSummary() const { return last_report_summary_; }

    // Get generation statistics
    bool getGenerationStatistics(size_t& total_reports_generated,
                                 std::chrono::system_clock::time_point& last_generation_time);

    // Check if generator is ready
    bool isReady() const { return initialized_; }

    // Get last error
    std::string getLastError() const { return last_error_; }

private:
    // Internal report generation methods
    bool generateMarkdownReport(const std::string& output_path, ValidationResultSummary& summary);
    bool generateJSONReport(const std::string& output_path, ValidationResultSummary& summary);
    bool generateHTMLReport(const std::string& output_path, ValidationResultSummary& summary);
    bool generateCSVReport(const std::string& output_path, ValidationResultSummary& summary);

    // Data collection methods
    bool collectECCValidationData(ValidationResultSummary& summary, DetailedValidationMetrics& metrics);
    bool collectDeterministicReplayData(ValidationResultSummary& summary, DetailedValidationMetrics& metrics);
    bool collectConstitutionalComplianceData(ValidationResultSummary& summary, DetailedValidationMetrics& metrics);
    bool collectPerformanceRegressionData(ValidationResultSummary& summary, DetailedValidationMetrics& metrics);

    // Report section generation methods
    std::string generateExecutiveSummarySection(const ValidationResultSummary& summary);
    std::string generateValidationOverviewSection(const ValidationResultSummary& summary);
    std::string generateECCValidationSection(const ValidationResultSummary& summary, const DetailedValidationMetrics& metrics);
    std::string generateDeterministicReplaySection(const ValidationResultSummary& summary, const DetailedValidationMetrics& metrics);
    std::string generateConstitutionalComplianceSection(const ValidationResultSummary& summary, const DetailedValidationMetrics& metrics);
    std::string generatePerformanceRegressionSection(const ValidationResultSummary& summary, const DetailedValidationMetrics& metrics);
    std::string generateTrendAnalysisSection(const DetailedValidationMetrics& metrics);
    std::string generateRecommendationsSection(const ValidationResultSummary& summary, const DetailedValidationMetrics& metrics);

    // Utility methods
    std::string formatValidationStatus(bool passed);
    std::string formatPercentage(double value);
    std::string formatThroughput(double throughput);
    std::string getCurrentTimestamp();
    std::string generateReportFileName(ReportFormat format);
    bool computeSHA256Digest(const std::string& content, std::vector<uint8_t>& digest);
    bool writeReportWithIntegrity(const std::string& content, const std::string& file_path);

    void setError(const std::string& error);
    void clearError();

private:
    bool initialized_;
    std::string last_error_;
    ReportGenerationConfig config_;

    // Validation framework references
    std::shared_ptr<ECCValidationFramework> ecc_framework_;
    std::shared_ptr<DeterministicReplayFramework> replay_framework_;
    std::shared_ptr<ConstitutionalComplianceFramework> compliance_framework_;
    std::shared_ptr<PerformanceRegressionDetector> regression_detector_;

    // Custom validation data
    std::map<std::string, std::map<std::string, double>> custom_validation_data_;
    std::map<std::string, std::pair<bool, std::string>> custom_validation_results_;

    // Generation statistics
    ValidationResultSummary last_report_summary_;
    size_t total_reports_generated_;
    std::chrono::system_clock::time_point last_generation_time_;
};

// Utility functions for report generation
namespace report_generation_utils {

    // Template rendering utilities
    std::string renderMarkdownTemplate(const std::string& template_content,
                                        const std::map<std::string, std::string>& variables);

    std::string renderJSONTemplate(const std::map<std::string, std::string>& data);
    std::string renderHTMLTemplate(const std::string& template_content,
                                   const std::map<std::string, std::string>& variables);

    // Data visualization utilities
    std::string generateMarkdownChart(const std::vector<std::pair<std::string, double>>& data,
                                      const std::string& title,
                                      const std::string& chart_type = "bar");

    std::string generateHTMLChart(const std::vector<std::pair<std::string, double>>& data,
                                  const std::string& title,
                                  const std::string& chart_type = "bar");

    // String formatting utilities
    std::string formatNumber(double value, int precision = 2);
    std::string formatScientific(double value);
    std::string formatDuration(double milliseconds);
    std::string formatFileSize(size_t bytes);

    // Color coding utilities for console output
    std::string colorizeText(const std::string& text, const std::string& color);
    std::string getValidationColor(bool passed);

    // Table generation utilities
    std::string generateMarkdownTable(const std::vector<std::vector<std::string>>& rows,
                                         const std::vector<std::string>& headers = {});

    std::string generateHTMLTable(const std::vector<std::vector<std::string>>& rows,
                                    const std::vector<std::string>& headers = {});

    // Graph generation utilities
    enum class GraphType {
        LINE_GRAPH,
        BAR_GRAPH,
        PIE_CHART,
        SCATTER_PLOT
    };

    std::string generateGraphDataJSON(const std::vector<std::pair<std::string, double>>& data,
                                        GraphType type);

    // File I/O utilities
    bool writeTextFile(const std::string& file_path, const std::string& content);
    bool readTextFile(const std::string& file_path, std::string& content);
    bool appendTextFile(const std::string& file_path, const std::string& content);

    // Compression utilities
    bool compressFile(const std::string& file_path);
    bool decompressFile(const std::string& file_path);

    // Date and time utilities
    std::string formatDate(std::chrono::system_clock::time_point time_point, const std::string& format = "%Y-%m-%d");
    std::string formatTime(std::chrono::system_clock::time_point time_point, const std::string& format = "%H:%M:%S");

    // Validation status utilities
    enum class ValidationStatus {
        PASSED,
        FAILED,
        WARNING,
        SKIPPED,
        UNKNOWN
    };

    ValidationStatus determineOverallStatus(const ValidationResultSummary& summary);
    std::string statusToString(ValidationStatus status);
    std::string statusToIcon(ValidationStatus status);

    // Recommendation generation utilities
    struct Recommendation {
        std::string category;
        std::string priority;
        std::string description;
        std::string action_item;
    };

    std::vector<Recommendation> generateRecommendations(const ValidationResultSummary& summary,
                                                          const DetailedValidationMetrics& metrics);

    std::string formatRecommendations(const std::vector<Recommendation>& recommendations);

    // Trend analysis utilities
    struct TrendData {
        std::vector<std::chrono::system_clock::time_point> timestamps;
        std::vector<double> values;
        std::string metric_name;
        std::string unit;
    };

    TrendData analyzeTrend(const std::vector<std::pair<std::chrono::system_clock::time_point, double>>& data,
                           const std::string& metric_name,
                           const std::string& unit);

    std::string formatTrendAnalysis(const TrendData& trend);

    // Integrity verification utilities
    bool verifyFileIntegrity(const std::string& file_path, const std::vector<uint8_t>& expected_hash);
    std::vector<uint8_t> computeFileHash(const std::string& file_path);

    // Report comparison utilities
    struct ReportComparison {
        std::string baseline_report_path;
        std::string current_report_path;
        std::vector<std::string> metric_changes;
        std::vector<std::string> new_issues;
        std::vector<std::string> resolved_issues;
    };

    ReportComparison compareReports(const std::string& baseline_path,
                                   const std::string& current_path);

    std::string formatReportComparison(const ReportComparison& comparison);
}

// Constants for report generation
namespace report_generation_constants {
    constexpr size_t MAX_REPORT_SIZE_MB = 10;              // Maximum report size in MB
    constexpr size_t MAX_CUSTOM_DATA_POINTS = 1000;        // Maximum custom data points
    constexpr double VALIDATION_PASS_THRESHOLD = 95.0;     // 95% pass threshold for overall validation
    constexpr double CRITICAL_FAILURE_THRESHOLD = 80.0;   // 80% threshold for critical failure
    constexpr int DEFAULT_REPORT_PRECISION = 2;            // Default decimal precision
    constexpr size_t MAX_TABLE_ROWS = 100;                 // Maximum table rows for performance
}

} // namespace validation
} // namespace puzzle71
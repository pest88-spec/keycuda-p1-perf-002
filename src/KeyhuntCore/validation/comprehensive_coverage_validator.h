// Puzzle71 Technical Debt Repair - Comprehensive Coverage Validator
// Phase 5: User Story 4 - Complete System Migration and Quality Assurance
// Task: T076 - Verify comprehensive test coverage achieved for all critical code paths
// Implements comprehensive validation of test coverage across all critical code paths

#pragma once

#include <cuda_runtime.h>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <filesystem>
#include <regex>
#include <fstream>
#include <chrono>
#include <unordered_set>
#include <algorithm>
#include <cmath>

#include "src/KeyhuntCore/testing/test_coverage_framework.h"

namespace puzzle71 {
namespace validation {

// Comprehensive coverage validation constants (T076 specific)
constexpr double CRITICAL_PATH_COVERAGE_THRESHOLD = 95.0;     // 95% critical path coverage required
constexpr double OVERALL_COVERAGE_THRESHOLD = 90.0;            // 90% overall coverage required
constexpr double BRANCH_COVERAGE_THRESHOLD = 85.0;             // 85% branch coverage required
constexpr double FUNCTION_COVERAGE_THRESHOLD = 95.0;           // 95% function coverage required
constexpr double CONDITION_COVERAGE_THRESHOLD = 80.0;          // 80% condition coverage required
constexpr int MAX_UNCOVERED_CRITICAL_PATHS = 0;                 // Zero uncovered critical paths allowed
constexpr int MAX_UNCOVERED_CRITICAL_FUNCTIONS = 0;             // Zero uncovered critical functions allowed
constexpr int MAX_UNCOVERED_CRITICAL_BRANCHES = 5;              // Maximum 5 uncovered critical branches

// Critical path categories
enum class CriticalPathCategory {
    KERNEL_EXECUTION,        // GPU kernel execution paths
    ECC_OPERATIONS,          // ECC operation paths
    MEMORY_MANAGEMENT,       // Memory management paths
    ERROR_HANDLING,          // Error handling paths
    CONFIGURATION_LOADING,   // Configuration loading paths
    GPU_INITIALIZATION,      // GPU initialization paths
    RESULT_VALIDATION,       // Result validation paths
    PERFORMANCE_CRITICAL     // Performance-critical paths
};

// Coverage validation types
enum class CoverageValidationType {
    LINE_COVERAGE,           // Line coverage validation
    BRANCH_COVERAGE,         // Branch coverage validation
    FUNCTION_COVERAGE,       // Function coverage validation
    CONDITION_COVERAGE,      // Condition coverage validation
    PATH_COVERAGE,           // Path coverage validation
    CRITICAL_PATH_COVERAGE,  // Critical path coverage validation
    INTEGRATION_COVERAGE,    // Integration test coverage validation
    REGRESSION_COVERAGE      // Regression test coverage validation
};

// Critical code path analysis result
struct CriticalCodePath {
    std::string path_name;
    std::string filepath;
    int start_line = 0;
    int end_line = 0;
    CriticalPathCategory category;
    std::string description;
    std::vector<std::string> test_files_covering;
    std::vector<std::string> functions_in_path;
    std::vector<std::string> branches_in_path;
    double coverage_percentage = 0.0;
    bool is_covered = false;
    bool is_critical = true;
    std::string risk_assessment;

    CriticalCodePath() : start_line(0), end_line(0), category(CriticalPathCategory::KERNEL_EXECUTION),
                        coverage_percentage(0.0), is_covered(false), is_critical(true) {}
};

// Comprehensive coverage validation result
struct ComprehensiveCoverageValidationResult {
    bool critical_path_coverage_achieved = false;
    bool overall_coverage_threshold_met = false;
    bool branch_coverage_threshold_met = false;
    bool function_coverage_threshold_met = false;
    bool condition_coverage_threshold_met = false;
    bool integration_coverage_adequate = false;
    bool regression_coverage_complete = false;

    // Coverage metrics
    int total_critical_paths = 0;
    int covered_critical_paths = 0;
    int uncovered_critical_paths = 0;
    int total_critical_functions = 0;
    int covered_critical_functions = 0;
    int uncovered_critical_functions = 0;
    int total_critical_branches = 0;
    int covered_critical_branches = 0;
    int uncovered_critical_branches = 0;

    // Coverage percentages
    double critical_path_coverage_percentage = 0.0;
    double overall_line_coverage_percentage = 0.0;
    double branch_coverage_percentage = 0.0;
    double function_coverage_percentage = 0.0;
    double condition_coverage_percentage = 0.0;
    double integration_coverage_percentage = 0.0;
    double regression_coverage_percentage = 0.0;

    // Detailed analysis results
    std::vector<CriticalCodePath> critical_paths;
    std::map<CriticalPathCategory, std::vector<CriticalCodePath>> paths_by_category;
    std::map<std::string, std::vector<std::string>> test_to_path_mapping;
    std::map<std::string, double> file_coverage_scores;

    // Missing coverage analysis
    std::vector<std::string> uncovered_critical_files;
    std::vector<std::string> uncovered_critical_functions;
    std::vector<std::string> uncovered_critical_branches;
    std::map<std::string, std::vector<std::string>> coverage_gaps;

    // Test quality metrics
    int total_tests = 0;
    int passing_tests = 0;
    int failing_tests = 0;
    int integration_tests = 0;
    int unit_tests = 0;
    int performance_tests = 0;
    double test_effectiveness_score = 0.0;

    // Risk assessment
    std::vector<std::string> high_risk_uncovered_areas;
    std::vector<std::string> medium_risk_uncovered_areas;
    std::vector<std::string> low_risk_uncovered_areas;
    double overall_risk_score = 0.0;

    // Recommendations and actions
    std::vector<std::string> coverage_improvement_recommendations;
    std::vector<std::string> critical_test_recommendations;
    std::vector<std::string> integration_gaps;
    std::vector<std::string> regression_test_recommendations;

    // Validation metadata
    std::chrono::milliseconds analysis_duration;
    int total_files_analyzed = 0;
    int total_lines_analyzed = 0;
    std::string analysis_version;
    std::string validation_timestamp;

    ComprehensiveCoverageValidationResult() : critical_path_coverage_achieved(false),
                                              overall_coverage_threshold_met(false),
                                              branch_coverage_threshold_met(false),
                                              function_coverage_threshold_met(false),
                                              condition_coverage_threshold_met(false),
                                              integration_coverage_adequate(false),
                                              regression_coverage_complete(false),
                                              total_critical_paths(0),
                                              covered_critical_paths(0),
                                              uncovered_critical_paths(0),
                                              total_critical_functions(0),
                                              covered_critical_functions(0),
                                              uncovered_critical_functions(0),
                                              total_critical_branches(0),
                                              covered_critical_branches(0),
                                              uncovered_critical_branches(0),
                                              critical_path_coverage_percentage(0.0),
                                              overall_line_coverage_percentage(0.0),
                                              branch_coverage_percentage(0.0),
                                              function_coverage_percentage(0.0),
                                              condition_coverage_percentage(0.0),
                                              integration_coverage_percentage(0.0),
                                              regression_coverage_percentage(0.0),
                                              total_tests(0),
                                              passing_tests(0),
                                              failing_tests(0),
                                              integration_tests(0),
                                              unit_tests(0),
                                              performance_tests(0),
                                              test_effectiveness_score(0.0),
                                              overall_risk_score(0.0),
                                              analysis_duration(0),
                                              total_files_analyzed(0),
                                              total_lines_analyzed(0),
                                              analysis_version("1.0") {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        validation_timestamp = std::ctime(&time_t);
    }
};

/**
 * @brief Comprehensive coverage validator for T076
 *
 * This class provides comprehensive validation of test coverage across all
 * critical code paths, ensuring thorough testing of system functionality.
 */
class ComprehensiveCoverageValidator {
public:
    ComprehensiveCoverageValidator();
    ~ComprehensiveCoverageValidator();

    // Initialization and configuration
    bool initialize(const std::string& project_root = ".");
    bool configure(double critical_path_threshold = CRITICAL_PATH_COVERAGE_THRESHOLD,
                   double overall_threshold = OVERALL_COVERAGE_THRESHOLD,
                   double branch_threshold = BRANCH_COVERAGE_THRESHOLD,
                   double function_threshold = FUNCTION_COVERAGE_THRESHOLD,
                   double condition_threshold = CONDITION_COVERAGE_THRESHOLD,
                   bool strict_critical_path_validation = true);
    void shutdown();

    // Main validation methods
    bool validateCriticalPathCoverage(ComprehensiveCoverageValidationResult& result);
    bool validateOverallCoverageThresholds(ComprehensiveCoverageValidationResult& result);
    bool validateBranchCoverage(ComprehensiveCoverageValidationResult& result);
    bool validateFunctionCoverage(ComprehensiveCoverageValidationResult& result);
    bool validateConditionCoverage(ComprehensiveCoverageValidationResult& result);
    bool validateIntegrationCoverage(ComprehensiveCoverageValidationResult& result);
    bool validateRegressionCoverage(ComprehensiveCoverageValidationResult& result);

    // Comprehensive validation
    bool validateComprehensiveTestCoverage(ComprehensiveCoverageValidationResult& result);

    // Critical path analysis
    bool identifyCriticalPaths(std::vector<CriticalCodePath>& critical_paths);
    bool analyzeCriticalPathCoverage(const std::vector<CriticalCodePath>& paths,
                                    ComprehensiveCoverageValidationResult& result);
    bool validateCriticalPathCompleteness(const std::vector<CriticalCodePath>& paths,
                                          ComprehensiveCoverageValidationResult& result);

    // Category-specific analysis
    bool validateKernelExecutionPaths(ComprehensiveCoverageValidationResult& result);
    bool validateECCOperationPaths(ComprehensiveCoverageValidationResult& result);
    bool validateMemoryManagementPaths(ComprehensiveCoverageValidationResult& result);
    bool validateErrorHandlingPaths(ComprehensiveCoverageValidationResult& result);
    bool validateConfigurationLoadingPaths(ComprehensiveCoverageValidationResult& result);
    bool validateGPUInitializationPaths(ComprehensiveCoverageValidationResult& result);
    bool validateResultValidationPaths(ComprehensiveCoverageValidationResult& result);
    bool validatePerformanceCriticalPaths(ComprehensiveCoverageValidationResult& result);

    // Test coverage measurement
    bool measureTestCoverage(TestCoverageAnalysisResult& coverage_result);
    bool analyzeTestEffectiveness(ComprehensiveCoverageValidationResult& result);
    bool validateTestQuality(ComprehensiveCoverageValidationResult& result);

    // Integration test validation
    bool identifyIntegrationPoints(std::vector<std::string>& integration_points);
    bool validateIntegrationTestCoverage(const std::vector<std::string>& integration_points,
                                         ComprehensiveCoverageValidationResult& result);
    bool analyzeSystemIntegrationCoverage(ComprehensiveCoverageValidationResult& result);

    // Regression test validation
    bool identifyRegressionRisks(std::vector<std::string>& regression_risks);
    bool validateRegressionTestCoverage(const std::vector<std::string>& regression_risks,
                                       ComprehensiveCoverageValidationResult& result);
    bool analyzeRegressionTestEffectiveness(ComprehensiveCoverageValidationResult& result);

    // Coverage gap analysis
    bool identifyCoverageGaps(const ComprehensiveCoverageValidationResult& coverage_result,
                              std::vector<std::string>& coverage_gaps);
    bool analyzeMissingCriticalPaths(ComprehensiveCoverageValidationResult& result);
    bool prioritizeCoverageImprovements(ComprehensiveCoverageValidationResult& result);

    // Risk assessment
    bool assessUncoveredCodeRisk(const ComprehensiveCoverageValidationResult& result,
                                  ComprehensiveCoverageValidationResult& risk_result);
    bool calculateRiskScores(ComprehensiveCoverageValidationResult& result);
    bool generateRiskMitigationPlan(const ComprehensiveCoverageValidationResult& result,
                                   std::string& mitigation_plan);

    // Reporting and validation
    bool generateComprehensiveCoverageReport(const ComprehensiveCoverageValidationResult& result,
                                            std::string& report);
    bool generateCriticalPathReport(const ComprehensiveCoverageValidationResult& result,
                                    std::string& report);
    bool generateRiskAssessmentReport(const ComprehensiveCoverageValidationResult& result,
                                      std::string& report);

    // Compliance scoring
    bool calculateCriticalPathScore(const ComprehensiveCoverageValidationResult& result,
                                    double& score);
    bool calculateOverallCoverageScore(const ComprehensiveCoverageValidationResult& result,
                                       double& score);
    bool calculateTestQualityScore(const ComprehensiveCoverageValidationResult& result,
                                   double& score);

    // Recommendations and improvement
    bool generateCoverageImprovementPlan(const ComprehensiveCoverageValidationResult& result,
                                         std::string& improvement_plan);
    bool suggestCriticalTestCases(const ComprehensiveCoverageValidationResult& result,
                                   std::vector<std::string>& test_cases);
    bool recommendIntegrationTests(const ComprehensiveCoverageValidationResult& result,
                                   std::vector<std::string>& integration_tests);

    // Configuration and status
    void setStrictCriticalPathValidation(bool strict) { strict_critical_path_validation_ = strict; }
    bool isStrictCriticalPathValidation() const { return strict_critical_path_validation_; }

    void setCriticalPathThreshold(double threshold) { critical_path_threshold_ = threshold; }
    double getCriticalPathThreshold() const { return critical_path_threshold_; }

    void setProjectRoot(const std::string& root) { project_root_ = root; }
    std::string getProjectRoot() const { return project_root_; }

    // Statistics and monitoring
    size_t getTotalPathsAnalyzed() const { return total_paths_analyzed_; }
    size_t getTotalFilesAnalyzed() const { return total_files_analyzed_; }
    std::chrono::milliseconds getTotalAnalysisTime() const { return total_analysis_time_; }

    // Error handling
    std::string getLastError() const { return last_error_; }
    bool hasErrors() const { return !last_error_.empty(); }

private:
    // Internal analysis methods
    bool analyzeFileForCriticalPaths(const std::string& filepath,
                                    std::vector<CriticalCodePath>& paths);
    bool extractCriticalFunctions(const std::string& content,
                                  std::vector<std::string>& functions);
    bool extractCriticalBranches(const std::string& content,
                                 std::vector<std::string>& branches);
    bool categorizeCriticalPath(const std::string& content,
                                CriticalPathCategory& category);

    // Critical path detection helpers
    bool detectKernelExecutionPaths(const std::string& content,
                                    std::vector<CriticalCodePath>& paths);
    bool detectECCOperationPaths(const std::string& content,
                                 std::vector<CriticalCodePath>& paths);
    bool detectMemoryManagementPaths(const std::string& content,
                                     std::vector<CriticalCodePath>& paths);
    bool detectErrorHandlingPaths(const std::string& content,
                                  std::vector<CriticalCodePath>& paths);
    bool detectConfigurationPaths(const std::string& content,
                                  std::vector<CriticalCodePath>& paths);
    bool detectGPUInitializationPaths(const std::string& content,
                                      std::vector<CriticalCodePath>& paths);
    bool detectResultValidationPaths(const std::string& content,
                                     std::vector<CriticalCodePath>& paths);
    bool detectPerformanceCriticalPaths(const std::string& content,
                                        std::vector<CriticalCodePath>& paths);

    // Test coverage analysis helpers
    bool mapTestsToCriticalPaths(const std::vector<std::string>& test_files,
                                 const std::vector<CriticalCodePath>& paths,
                                 std::map<std::string, std::vector<std::string>>& mapping);
    bool calculatePathCoverage(const CriticalCodePath& path,
                               const std::vector<std::string>& covering_tests,
                               double& coverage_percentage);
    bool validateTestCompleteness(const std::vector<std::string>& test_files,
                                  const std::vector<CriticalCodePath>& paths);

    // Integration analysis helpers
    bool identifyModuleDependencies(std::map<std::string, std::vector<std::string>>& dependencies);
    bool analyzeCrossModuleCoverage(std::map<std::string, double>& module_coverage);
    bool validateSystemLevelTestCoverage(ComprehensiveCoverageValidationResult& result);

    // Risk assessment helpers
    double calculatePathRisk(const CriticalCodePath& path);
    double calculateFunctionRisk(const std::string& function_name);
    double calculateComplexityRisk(const std::string& filepath);
    void categorizeRiskLevel(double risk_score, std::vector<std::string>& risk_category);

    // Report generation helpers
    std::string generateExecutiveSummary(const ComprehensiveCoverageValidationResult& result);
    std::string generateCriticalPathAnalysis(const ComprehensiveCoverageValidationResult& result);
    std::string generateCoverageBreakdown(const ComprehensiveCoverageValidationResult& result);
    std::string generateRiskAnalysis(const ComprehensiveCoverageValidationResult& result);
    std::string generateRecommendationsSection(const ComprehensiveCoverageValidationResult& result);

    // File system helpers
    std::vector<std::string> findSourceFiles(const std::string& directory);
    std::vector<std::string> findTestFiles(const std::string& directory);
    bool isSourceFile(const std::string& filepath);
    bool isTestFile(const std::string& filepath);
    bool shouldAnalyzeFile(const std::string& filepath);

    // Pattern matching helpers
    std::vector<std::regex> getCriticalPathPatterns(CriticalPathCategory category);
    bool matchesCriticalPathPattern(const std::string& line, CriticalPathCategory& category);
    bool extractFunctionSignature(const std::string& line, std::string& signature);

    // Error handling
    void setError(const std::string& error);
    void clearError();

private:
    std::string project_root_;
    double critical_path_threshold_;
    double overall_threshold_;
    double branch_threshold_;
    double function_threshold_;
    double condition_threshold_;
    bool strict_critical_path_validation_;
    bool initialized_;

    // Analysis statistics
    size_t total_paths_analyzed_;
    size_t total_files_analyzed_;
    std::chrono::milliseconds total_analysis_time_;

    // Configuration
    std::vector<std::string> source_extensions_;
    std::vector<std::string> test_extensions_;
    std::vector<std::string> exclude_directories_;
    std::vector<std::string> exclude_patterns_;

    // Critical path patterns
    std::map<CriticalPathCategory, std::vector<std::regex>> critical_path_patterns_;
    std::map<CriticalPathCategory, std::vector<std::string>> category_keywords_;

    // Integration with test coverage framework
    std::unique_ptr<TestCoverageFramework> test_coverage_framework_;

    // Analysis cache
    std::map<std::string, std::vector<CriticalCodePath>> file_critical_paths_cache_;
    std::map<std::string, TestCoverageAnalysisResult> test_coverage_cache_;

    // Error handling
    std::string last_error_;
};

/**
 * @brief Utility functions for comprehensive coverage validation
 */
namespace comprehensive_coverage_validation_utils {

    // Quick validation functions
    bool quickCriticalPathCheck(const std::string& project_root);
    bool validateOverallCoverage(const std::string& project_root);
    bool checkIntegrationCoverage(const std::string& project_root);

    // Analysis utilities
    std::vector<std::string> findUncoveredCriticalPaths(const std::string& project_root);
    std::vector<std::string> findMissingIntegrationTests(const std::string& project_root);
    std::vector<std::string> findRegressionTestGaps(const std::string& project_root);

    // Metrics calculation
    double calculateCriticalPathCoverage(const std::string& project_root);
    std::map<std::string, double> calculateModuleCoverageScores(const std::string& project_root);
    std::vector<std::string> getLowCoverageCriticalPaths(const std::string& project_root,
                                                         double threshold = 90.0);

    // Risk assessment utilities
    std::vector<std::string> assessCoverageRisks(const std::string& project_root);
    double calculateOverallRiskScore(const std::string& project_root);
    std::vector<std::string> getHighRiskUncoveredAreas(const std::string& project_root);

    // Validation utilities
    bool validateCriticalPathRequirements(const std::string& project_root);
    bool validateIntegrationTestRequirements(const std::string& project_root);
    bool validateRegressionTestRequirements(const std::string& project_root);

    // Coverage improvement utilities
    bool suggestCoverageImprovements(const std::string& project_root,
                                     std::vector<std::string>& suggestions);
    bool generateTestPlan(const std::string& project_root,
                          const ComprehensiveCoverageValidationResult& result,
                          std::string& test_plan);
    bool runComprehensiveCoverageValidation(const std::string& project_root,
                                           ComprehensiveCoverageValidationResult& result);

    // Reporting utilities
    bool generateCoverageDashboard(const std::string& project_root, std::string& dashboard);
    bool generateRiskMitigationReport(const std::string& project_root, std::string& report);
    bool createCoverageQualityBadge(const ComprehensiveCoverageValidationResult& result,
                                    std::string& badge_svg);
}

} // namespace validation
} // namespace puzzle71
// Puzzle71 Technical Debt Repair - Unified Module Validator
// Phase 5: User Story 4 - Complete System Migration and Quality Assurance
// Task: T074 - Validate all kernels use unified modules (no legacy paths remain)
// Implements comprehensive validation of unified module usage across all kernel implementations

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

namespace puzzle71 {
namespace validation {

// Unified module validation constants
constexpr double UNIFIED_MODULE_USAGE_THRESHOLD = 100.0;  // 100% unified module usage required
constexpr int MAX_LEGACY_FUNCTION_CALLS = 0;              // Zero legacy function calls allowed
constexpr int MAX_LEGACY_INCLUDES = 0;                    // Zero legacy includes allowed
constexpr int MAX_LEGACY_VARIABLES = 0;                   // Zero legacy variables allowed
constexpr int MAX_LEGACY_MACROS = 0;                      // Zero legacy macros allowed

// Unified module types
enum class UnifiedModuleType {
    CANDIDATE_SCANNER,      // UnifiedCandidateScanner module
    ECC_OPERATIONS,         // ECCOperationsFixed module
    LEGACY_ADAPTER,         // LegacyAdapterFixed module
    MEMORY_ACCESS,          // OptimizedMemoryAccess module
    GPU_EXECUTOR,           // GPUExecutor module
    BATCH_PLANNER,          // BatchPlanner module
    RESULT_EMITTER,         // ResultEmitter module
    HASH_UTILS,             // Unified hash utilities module
    CONFIG_MANAGER,         // Configuration manager module
    KERNEL_LAUNCHER         // Kernel launcher module
};

// Kernel validation types
enum class KernelValidationType {
    UNIFIED_MODULE_USAGE,   // Unified module usage validation
    LEGACY_CODE_REMOVAL,    // Legacy code removal validation
    FUNCTION_CALL_ANALYSIS, // Function call analysis
    INCLUDE_DEPENDENCY,     // Include dependency validation
    VARIABLE_USAGE,         // Variable usage validation
    MACRO_USAGE,           // Macro usage validation
    NAMESPACE_USAGE,       // Namespace usage validation
    TEMPLATE_USAGE         // Template usage validation
};

// Unified module usage metrics
struct UnifiedModuleUsageMetrics {
    // Module usage counts
    std::map<UnifiedModuleType, int> module_usage_counts;
    std::map<UnifiedModuleType, double> module_usage_percentages;

    // Legacy detection metrics
    int legacy_function_calls = 0;
    int legacy_includes = 0;
    int legacy_variables = 0;
    int legacy_macros = 0;
    int legacy_namespaces = 0;
    int legacy_templates = 0;

    // Overall metrics
    int total_function_calls = 0;
    int total_includes = 0;
    int total_variables = 0;
    int total_macros = 0;
    double overall_unified_usage_percentage = 0.0;

    // Kernel-specific metrics
    std::map<std::string, std::map<UnifiedModuleType, int>> kernel_module_usage;
    std::map<std::string, std::vector<std::string>> kernel_legacy_patterns;

    UnifiedModuleUsageMetrics() : legacy_function_calls(0), legacy_includes(0),
                                 legacy_variables(0), legacy_macros(0), legacy_namespaces(0),
                                 legacy_templates(0), total_function_calls(0), total_includes(0),
                                 total_variables(0), total_macros(0),
                                 overall_unified_usage_percentage(0.0) {}
};

// Kernel validation result
struct KernelValidationResult {
    bool all_kernels_use_unified_modules = false;
    bool no_legacy_code_paths_remain = false;
    bool complete_migration_achieved = false;
    bool unified_module_compliance_met = false;

    // Detailed validation results
    std::map<std::string, bool> kernel_validation_status;
    std::vector<std::string> compliant_kernels;
    std::vector<std::string> non_compliant_kernels;
    std::vector<std::string> partially_compliant_kernels;

    // Legacy analysis results
    std::map<std::string, std::vector<std::string>> legacy_patterns_found;
    std::map<std::string, std::vector<std::string>> legacy_functions_found;
    std::map<std::string, std::vector<std::string>> legacy_includes_found;

    // Unified module usage analysis
    UnifiedModuleUsageMetrics usage_metrics;
    std::map<std::string, UnifiedModuleUsageMetrics> kernel_specific_metrics;

    // Compliance scores
    std::map<std::string, double> kernel_compliance_scores;
    double overall_compliance_score = 0.0;

    // Validation details
    std::map<std::string, std::vector<std::string>> validation_issues;
    std::map<std::string, std::vector<std::string>> recommendations;
    std::vector<std::string> blocking_issues;

    // Analysis metadata
    std::chrono::milliseconds analysis_duration;
    int total_kernels_analyzed = 0;
    int total_files_analyzed = 0;
    int total_lines_analyzed = 0;
    std::string analysis_version;

    KernelValidationResult() : all_kernels_use_unified_modules(false), no_legacy_code_paths_remain(false),
                             complete_migration_achieved(false), unified_module_compliance_met(false),
                             overall_compliance_score(0.0), analysis_duration(0),
                             total_kernels_analyzed(0), total_files_analyzed(0), total_lines_analyzed(0),
                             analysis_version("1.0") {}
};

// Kernel file analysis result
struct KernelFileAnalysisResult {
    std::string filepath;
    std::string kernel_name;

    // Unified module usage
    std::map<UnifiedModuleType, int> module_usage;
    std::vector<std::string> unified_module_includes;
    std::vector<std::string> unified_module_function_calls;

    // Legacy detection
    std::vector<std::string> legacy_patterns;
    std::vector<std::string> legacy_function_calls;
    std::vector<std::string> legacy_includes;
    std::vector<std::string> legacy_variables;
    std::vector<std::string> legacy_macros;

    // Code analysis
    int total_lines = 0;
    int kernel_functions_count = 0;
    int unified_module_calls = 0;
    int legacy_code_blocks = 0;

    // Compliance metrics
    double unified_usage_percentage = 0.0;
    bool is_compliant = false;
    std::vector<std::string> compliance_issues;
    std::vector<std::string> recommendations;

    KernelFileAnalysisResult() : total_lines(0), kernel_functions_count(0),
                                 unified_module_calls(0), legacy_code_blocks(0),
                                 unified_usage_percentage(0.0), is_compliant(false) {}
};

/**
 * @brief Unified module validator for T074
 *
 * This class provides comprehensive validation of unified module usage across
 * all kernel implementations, ensuring complete migration from legacy code paths.
 */
class UnifiedModuleValidator {
public:
    UnifiedModuleValidator();
    ~UnifiedModuleValidator();

    // Initialization and configuration
    bool initialize(const std::string& project_root = ".");
    bool configure(double unified_usage_threshold = UNIFIED_MODULE_USAGE_THRESHOLD,
                   bool strict_legacy_mode = true,
                   bool enable_detailed_analysis = true);
    void shutdown();

    // Main validation methods
    bool validateAllKernelsUseUnifiedModules(KernelValidationResult& result);
    bool validateNoLegacyCodePathsRemain(KernelValidationResult& result);
    bool validateCompleteMigrationAchieved(KernelValidationResult& result);

    // Kernel-specific validation
    bool validateKernelFile(const std::string& kernel_filepath, KernelFileAnalysisResult& result);
    bool validateKernelFunction(const std::string& kernel_name, const std::string& function_content,
                                KernelFileAnalysisResult& result);
    bool validateKernelCompliance(const std::string& kernel_name, double& compliance_score);

    // Unified module usage analysis
    bool analyzeUnifiedModuleUsage(const std::string& content, const std::string& filepath,
                                   UnifiedModuleUsageMetrics& metrics);
    bool detectUnifiedModulePatterns(const std::string& content, std::vector<std::string>& patterns);
    bool calculateUnifiedUsagePercentage(const UnifiedModuleUsageMetrics& metrics, double& percentage);

    // Legacy code detection
    bool detectLegacyPatterns(const std::string& content, std::vector<std::string>& patterns);
    bool detectLegacyFunctionCalls(const std::string& content, std::vector<std::string>& functions);
    bool detectLegacyIncludes(const std::string& content, std::vector<std::string>& includes);
    bool detectLegacyVariables(const std::string& content, std::vector<std::string>& variables);
    bool detectLegacyMacros(const std::string& content, std::vector<std::string>& macros);

    // Module-specific validation
    bool validateCandidateScannerUsage(const std::string& content, KernelFileAnalysisResult& result);
    bool validateECCOperationsUsage(const std::string& content, KernelFileAnalysisResult& result);
    bool validateLegacyAdapterUsage(const std::string& content, KernelFileAnalysisResult& result);
    bool validateMemoryAccessUsage(const std::string& content, KernelFileAnalysisResult& result);
    bool validateGPUExecutorUsage(const std::string& content, KernelFileAnalysisResult& result);

    // Compliance validation
    bool validateUnifiedModuleCompliance(KernelValidationResult& result);
    bool validateLegacyRemovalCompliance(KernelValidationResult& result);
    bool validateMigrationCompleteness(KernelValidationResult& result);

    // File system analysis
    bool findKernelFiles(std::vector<std::string>& kernel_files);
    bool analyzeKernelDirectory(const std::string& directory, KernelValidationResult& result);
    bool isKernelFile(const std::string& filepath);
    bool shouldAnalyzeFile(const std::string& filepath);

    // Pattern matching helpers
    bool matchesUnifiedModulePattern(const std::string& line, UnifiedModuleType& module_type);
    bool matchesLegacyPattern(const std::string& line, std::string& pattern_type);
    bool extractFunctionCalls(const std::string& content, std::vector<std::string>& function_calls);
    bool extractIncludes(const std::string& content, std::vector<std::string>& includes);

    // Reporting and validation
    bool generateValidationReport(const KernelValidationResult& result, std::string& report);
    bool generateKernelSpecificReport(const std::string& kernel_name,
                                     const KernelFileAnalysisResult& result, std::string& report);
    bool generateComplianceSummary(const KernelValidationResult& result, std::string& summary);

    // Recommendations and improvement
    bool generateMigrationRecommendations(const KernelValidationResult& result,
                                          std::vector<std::string>& recommendations);
    bool generateComplianceImprovementPlan(const KernelValidationResult& result,
                                           std::string& improvement_plan);

    // Configuration and status
    void setStrictLegacyMode(bool strict) { strict_legacy_mode_ = strict; }
    bool isStrictLegacyMode() const { return strict_legacy_mode_; }

    void setDetailedAnalysis(bool detailed) { detailed_analysis_ = detailed; }
    bool isDetailedAnalysis() const { return detailed_analysis_; }

    void setProjectRoot(const std::string& root) { project_root_ = root; }
    std::string getProjectRoot() const { return project_root_; }

    // Statistics and monitoring
    size_t getTotalKernelsAnalyzed() const { return total_kernels_analyzed_; }
    size_t getTotalFilesAnalyzed() const { return total_files_analyzed_; }
    std::chrono::milliseconds getTotalAnalysisTime() const { return total_analysis_time_; }

    // Error handling
    std::string getLastError() const { return last_error_; }
    bool hasErrors() const { return !last_error_.empty(); }

private:
    // Internal analysis methods
    bool analyzeKernelContent(const std::string& content, const std::string& filepath,
                             KernelFileAnalysisResult& result);
    bool validateKernelFunctionCalls(const std::string& content, KernelFileAnalysisResult& result);
    bool validateKernelIncludes(const std::string& content, KernelFileAnalysisResult& result);
    bool validateKernelVariables(const std::string& content, KernelFileAnalysisResult& result);

    // Pattern detection helpers
    bool detectCandidateScannerPattern(const std::string& line);
    bool detectECCOperationsPattern(const std::string& line);
    bool detectLegacyAdapterPattern(const std::string& line);
    bool detectMemoryAccessPattern(const std::string& line);
    bool detectGPUExecutorPattern(const std::string& line);

    // Legacy pattern detection helpers
    bool detectLegacyFunctionPattern(const std::string& line);
    bool detectLegacyIncludePattern(const std::string& line);
    bool detectLegacyVariablePattern(const std::string& line);
    bool detectLegacyMacroPattern(const std::string& line);
    bool detectLegacyNamespacePattern(const std::string& line);

    // Module usage calculation helpers
    void updateModuleUsageMetrics(const std::string& content, UnifiedModuleType module_type,
                                 UnifiedModuleUsageMetrics& metrics);
    double calculateModuleComplianceScore(const KernelFileAnalysisResult& result);
    double calculateOverallComplianceScore(const std::vector<KernelFileAnalysisResult>& results);

    // Validation helper methods
    bool validateKernelComplianceRequirements(const KernelFileAnalysisResult& result);
    bool checkForLegacyCodeBlocks(const std::string& content);
    bool validateUnifiedModuleImports(const std::vector<std::string>& includes);

    // Report generation helpers
    std::string generateExecutiveSummary(const KernelValidationResult& result);
    std::string generateDetailedAnalysis(const KernelValidationResult& result);
    std::string generateKernelBreakdown(const KernelValidationResult& result);
    std::string generateLegacyAnalysis(const KernelValidationResult& result);
    std::string generateRecommendationsSection(const KernelValidationResult& result);

    // File system helpers
    std::vector<std::string> findSourceFiles(const std::string& directory);
    std::vector<std::string> findKernelFilesInDirectory(const std::string& directory);
    std::string extractKernelNameFromPath(const std::string& filepath);
    bool isCUDAFile(const std::string& filepath);

    // Error handling
    void setError(const std::string& error);
    void clearError();

private:
    std::string project_root_;
    double unified_usage_threshold_;
    bool strict_legacy_mode_;
    bool detailed_analysis_;
    bool initialized_;

    // Analysis statistics
    size_t total_kernels_analyzed_;
    size_t total_files_analyzed_;
    std::chrono::milliseconds total_analysis_time_;

    // Configuration
    std::vector<std::string> kernel_extensions_;
    std::vector<std::string> exclude_directories_;
    std::vector<std::string> exclude_patterns_;

    // Unified module patterns
    std::map<UnifiedModuleType, std::vector<std::regex>> unified_module_patterns_;
    std::map<UnifiedModuleType, std::vector<std::string>> unified_module_names_;

    // Legacy pattern detection
    std::vector<std::regex> legacy_function_patterns_;
    std::vector<std::regex> legacy_include_patterns_;
    std::vector<std::regex> legacy_variable_patterns_;
    std::vector<std::regex> legacy_macro_patterns_;
    std::vector<std::regex> legacy_namespace_patterns_;

    // Validation cache
    std::map<std::string, KernelFileAnalysisResult> kernel_analysis_cache_;
    std::map<std::string, UnifiedModuleUsageMetrics> usage_metrics_cache_;

    // Error handling
    std::string last_error_;
};

/**
 * @brief Utility functions for unified module validation
 */
namespace unified_module_validation_utils {

    // Quick validation functions
    bool quickUnifiedModuleCheck(const std::string& project_root);
    bool validateNoLegacyCode(const std::string& project_root);
    bool checkCompleteMigration(const std::string& project_root);

    // Analysis utilities
    std::vector<std::string> findKernelsWithLegacyCode(const std::string& project_root);
    std::vector<std::string> findKernelsWithoutUnifiedModules(const std::string& project_root);
    std::vector<std::string> findLegacyCodePatterns(const std::string& project_root);

    // Metrics calculation
    double calculateUnifiedModuleUsage(const std::string& project_root);
    std::map<std::string, double> calculateKernelComplianceScores(const std::string& project_root);
    std::vector<std::string> getNonCompliantKernels(const std::string& project_root,
                                                    double threshold = 95.0);

    // Validation utilities
    bool validateKernelCompliance(const std::string& kernel_filepath);
    bool validateUnifiedModuleImports(const std::string& kernel_filepath);
    bool validateLegacyCodeRemoval(const std::string& kernel_filepath);

    // Compliance improvement utilities
    bool suggestUnifiedModuleImprovements(const std::string& project_root,
                                          std::vector<std::string>& suggestions);
    bool generateMigrationPlan(const std::string& project_root,
                              const KernelValidationResult& result,
                              std::string& plan);
    bool runComprehensiveValidation(const std::string& project_root,
                                    KernelValidationResult& result);

    // Reporting utilities
    bool generateMigrationReport(const std::string& project_root, std::string& report);
    bool generateComplianceDashboard(const std::string& project_root, std::string& dashboard);
    bool createComplianceBadge(const KernelValidationResult& result, std::string& badge_svg);
}

} // namespace validation
} // namespace puzzle71
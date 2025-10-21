// Puzzle71 Technical Debt Repair - Architectural Compliance Validator
// Phase 5: User Story 4 - Complete System Migration and Quality Assurance
// Task: T075 - Confirm architectural compliance tests show zero code duplication
// Implements comprehensive architectural compliance validation with zero code duplication verification

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

#include "src/KeyhuntCore/architecture/architectural_compliance_framework.h"

namespace puzzle71 {
namespace validation {

// Architectural compliance validation constants (T075 specific)
constexpr double ZERO_DUPLICATION_THRESHOLD = 0.0;           // Zero code duplication required
constexpr double ADAPTER_PATTERN_THRESHOLD = 100.0;          // All duplications must be adapter patterns
constexpr int MAX_NON_ADAPTER_DUPLICATIONS = 0;              // Zero non-adapter duplications allowed
constexpr double STRUCTURAL_SIMILARITY_THRESHOLD = 0.1;      // Maximum 10% structural similarity
constexpr double LOGICAL_SIMILARITY_THRESHOLD = 0.05;        // Maximum 5% logical similarity
constexpr int MIN_CODE_BLOCKS_FOR_ANALYSIS = 10;             // Minimum blocks for meaningful analysis

// Duplication analysis types
enum class DuplicationType {
    EXACT_DUPLICATION,        // Identical code blocks
    STRUCTURAL_DUPLICATION,   // Similar structure/logic
    LOGICAL_DUPLICATION,      // Functionally equivalent
    ADAPTER_PATTERN,          // Intentional adapter pattern
    TEMPLATE_DUPLICATION,     // Template-based duplication
    CONFIGURATION_DUPLICATION // Configuration-based duplication
};

// Adapter pattern validation types
enum class AdapterPatternType {
    LEGACY_ADAPTER,           // Legacy system adapter
    INTERFACE_ADAPTER,        // Interface adapter
    PROTOCOL_ADAPTER,         // Protocol adapter
    DATA_FORMAT_ADAPTER,      // Data format adapter
    API_ADAPTER,              // API adapter
    PLATFORM_ADAPTER          // Platform adapter
};

// Code block analysis result
struct CodeBlockAnalysisResult {
    std::string filepath;
    int start_line = 0;
    int end_line = 0;
    std::string content;
    std::string normalized_content;
    std::vector<std::string> tokens;
    std::string hash_signature;
    std::vector<std::string> function_names;
    std::vector<std::string> variable_names;
    std::vector<std::string> include_files;

    // Duplication metrics
    std::vector<std::string> exact_duplicates;
    std::vector<std::string> structural_duplicates;
    std::vector<std::string> logical_duplicates;
    bool is_adapter_pattern = false;
    AdapterPatternType adapter_type = AdapterPatternType::LEGACY_ADAPTER;

    CodeBlockAnalysisResult() : start_line(0), end_line(0), is_adapter_pattern(false) {}
};

// Zero duplication validation result
struct ZeroDuplicationValidationResult {
    bool zero_code_duplication_achieved = false;
    bool all_duplications_are_adapter_patterns = false;
    bool adapter_pattern_validation_passed = false;
    bool structural_similarity_within_threshold = false;
    bool logical_similarity_within_threshold = false;

    // Overall compliance metrics
    int total_code_blocks_analyzed = 0;
    int exact_duplicates_count = 0;
    int structural_duplicates_count = 0;
    int logical_duplicates_count = 0;
    int adapter_pattern_instances = 0;
    int non_adapter_duplicates = 0;

    // Duplication percentages
    double exact_duplication_percentage = 0.0;
    double structural_duplication_percentage = 0.0;
    double logical_duplication_percentage = 0.0;
    double overall_duplication_percentage = 0.0;
    double adapter_pattern_percentage = 0.0;

    // Similarity metrics
    double average_structural_similarity = 0.0;
    double maximum_structural_similarity = 0.0;
    double average_logical_similarity = 0.0;
    double maximum_logical_similarity = 0.0;

    // Detailed analysis results
    std::vector<CodeBlockAnalysisResult> code_blocks;
    std::map<std::string, std::vector<std::string>> duplication_groups;
    std::map<std::string, AdapterPatternType> adapter_patterns;
    std::vector<std::string> non_compliant_files;

    // Violations and recommendations
    std::vector<std::string> duplication_violations;
    std::vector<std::string> adapter_pattern_violations;
    std::vector<std::string> structural_violations;
    std::vector<std::string> logical_violations;
    std::vector<std::string> recommendations;

    // Analysis metadata
    std::chrono::milliseconds analysis_duration;
    int total_files_analyzed = 0;
    int total_lines_analyzed = 0;
    std::string analysis_version;

    ZeroDuplicationValidationResult() : zero_code_duplication_achieved(false),
                                      all_duplications_are_adapter_patterns(false),
                                      adapter_pattern_validation_passed(false),
                                      structural_similarity_within_threshold(false),
                                      logical_similarity_within_threshold(false),
                                      total_code_blocks_analyzed(0),
                                      exact_duplicates_count(0),
                                      structural_duplicates_count(0),
                                      logical_duplicates_count(0),
                                      adapter_pattern_instances(0),
                                      non_adapter_duplicates(0),
                                      exact_duplication_percentage(0.0),
                                      structural_duplication_percentage(0.0),
                                      logical_duplication_percentage(0.0),
                                      overall_duplication_percentage(0.0),
                                      adapter_pattern_percentage(0.0),
                                      average_structural_similarity(0.0),
                                      maximum_structural_similarity(0.0),
                                      average_logical_similarity(0.0),
                                      maximum_logical_similarity(0.0),
                                      analysis_duration(0),
                                      total_files_analyzed(0),
                                      total_lines_analyzed(0),
                                      analysis_version("1.0") {}
};

/**
 * @brief Architectural compliance validator for T075
 *
 * This class provides comprehensive validation of architectural compliance with
 * focus on zero code duplication verification and adapter pattern validation.
 */
class ArchitecturalComplianceValidator {
public:
    ArchitecturalComplianceValidator();
    ~ArchitecturalComplianceValidator();

    // Initialization and configuration
    bool initialize(const std::string& project_root = ".");
    bool configure(double zero_duplication_threshold = ZERO_DUPLICATION_THRESHOLD,
                   double adapter_pattern_threshold = ADAPTER_PATTERN_THRESHOLD,
                   double structural_similarity_threshold = STRUCTURAL_SIMILARITY_THRESHOLD,
                   double logical_similarity_threshold = LOGICAL_SIMILARITY_THRESHOLD,
                   bool strict_adapter_validation = true);
    void shutdown();

    // Main validation methods
    bool validateZeroCodeDuplication(ZeroDuplicationValidationResult& result);
    bool validateAllDuplicationsAreAdapterPatterns(ZeroDuplicationValidationResult& result);
    bool validateAdapterPatternCompliance(ZeroDuplicationValidationResult& result);
    bool validateStructuralSimilarity(ZeroDuplicationValidationResult& result);
    bool validateLogicalSimilarity(ZeroDuplicationValidationResult& result);

    // Comprehensive architectural compliance validation
    bool validateArchitecturalComplianceWithZeroDuplication(ArchitecturalComplianceResult& result);

    // Code block analysis
    bool extractCodeBlocks(const std::string& filepath, std::vector<CodeBlockAnalysisResult>& blocks);
    bool analyzeCodeBlock(const std::string& content, const std::string& filepath,
                         CodeBlockAnalysisResult& result);
    bool normalizeCodeBlock(const CodeBlockAnalysisResult& input, CodeBlockAnalysisResult& normalized);

    // Duplication detection
    bool detectExactDuplicates(std::vector<CodeBlockAnalysisResult>& blocks,
                               std::map<std::string, std::vector<std::string>>& duplicates);
    bool detectStructuralDuplicates(std::vector<CodeBlockAnalysisResult>& blocks,
                                   std::map<std::string, std::vector<std::string>>& duplicates);
    bool detectLogicalDuplicates(std::vector<CodeBlockAnalysisResult>& blocks,
                                std::map<std::string, std::vector<std::string>>& duplicates);

    // Adapter pattern validation
    bool detectAdapterPatterns(const std::vector<CodeBlockAnalysisResult>& blocks,
                              std::map<std::string, AdapterPatternType>& patterns);
    bool validateAdapterPatternImplementation(const CodeBlockAnalysisResult& block,
                                             AdapterPatternType& pattern_type);
    bool isIntentionalAdapterPattern(const std::string& content, const std::string& context);

    // Similarity analysis
    bool calculateStructuralSimilarity(const CodeBlockAnalysisResult& block1,
                                      const CodeBlockAnalysisResult& block2,
                                      double& similarity_score);
    bool calculateLogicalSimilarity(const CodeBlockAnalysisResult& block1,
                                   const CodeBlockAnalysisResult& block2,
                                   double& similarity_score);
    bool calculateOverallSimilarity(const std::vector<CodeBlockAnalysisResult>& blocks,
                                   double& average_similarity, double& maximum_similarity);

    // Token and signature analysis
    bool extractTokens(const std::string& content, std::vector<std::string>& tokens);
    bool generateHashSignature(const std::string& content, std::string& signature);
    bool normalizeTokens(std::vector<std::string>& tokens);

    // File system analysis
    bool findSourceFiles(std::vector<std::string>& source_files);
    bool analyzeFileStructure(const std::string& filepath,
                             std::vector<CodeBlockAnalysisResult>& blocks);
    bool shouldAnalyzeFile(const std::string& filepath);
    bool isSourceFile(const std::string& filepath);

    // Reporting and validation
    bool generateZeroDuplicationReport(const ZeroDuplicationValidationResult& result,
                                       std::string& report);
    bool generateAdapterPatternReport(const ZeroDuplicationValidationResult& result,
                                      std::string& report);
    bool generateSimilarityReport(const ZeroDuplicationValidationResult& result,
                                  std::string& report);

    // Compliance scoring
    bool calculateZeroDuplicationScore(const ZeroDuplicationValidationResult& result,
                                       double& score);
    bool calculateAdapterPatternScore(const ZeroDuplicationValidationResult& result,
                                      double& score);
    bool calculateOverallComplianceScore(const ZeroDuplicationValidationResult& result,
                                         double& score);

    // Recommendations and improvement
    bool generateDuplicationEliminationPlan(const ZeroDuplicationValidationResult& result,
                                           std::string& elimination_plan);
    bool generateAdapterPatternRecommendations(const ZeroDuplicationValidationResult& result,
                                               std::vector<std::string>& recommendations);
    bool suggestRefactoringStrategies(const std::vector<CodeBlockAnalysisResult>& duplicated_blocks,
                                      std::vector<std::string>& strategies);

    // Configuration and status
    void setStrictAdapterValidation(bool strict) { strict_adapter_validation_ = strict; }
    bool isStrictAdapterValidation() const { return strict_adapter_validation_; }

    void setZeroDuplicationThreshold(double threshold) { zero_duplication_threshold_ = threshold; }
    double getZeroDuplicationThreshold() const { return zero_duplication_threshold_; }

    void setProjectRoot(const std::string& root) { project_root_ = root; }
    std::string getProjectRoot() const { return project_root_; }

    // Statistics and monitoring
    size_t getTotalBlocksAnalyzed() const { return total_blocks_analyzed_; }
    size_t getTotalFilesAnalyzed() const { return total_files_analyzed_; }
    std::chrono::milliseconds getTotalAnalysisTime() const { return total_analysis_time_; }

    // Error handling
    std::string getLastError() const { return last_error_; }
    bool hasErrors() const { return !last_error_.empty(); }

private:
    // Internal analysis methods
    bool analyzeCodeBlockStructure(const std::string& content, CodeBlockAnalysisResult& result);
    bool extractFunctionNames(const std::string& content, std::vector<std::string>& functions);
    bool extractVariableNames(const std::string& content, std::vector<std::string>& variables);
    bool extractIncludeFiles(const std::string& content, std::vector<std::string>& includes);

    // Duplication detection helpers
    bool calculateTokenSimilarity(const std::vector<std::string>& tokens1,
                                  const std::vector<std::string>& tokens2,
                                  double& similarity);
    bool calculateStructureSimilarity(const std::vector<std::string>& functions1,
                                      const std::vector<std::string>& functions2,
                                      double& similarity);
    bool isExactDuplicate(const CodeBlockAnalysisResult& block1,
                          const CodeBlockAnalysisResult& block2);
    bool isStructuralDuplicate(const CodeBlockAnalysisResult& block1,
                               const CodeBlockAnalysisResult& block2,
                               double& similarity);
    bool isLogicalDuplicate(const CodeBlockAnalysisResult& block1,
                            const CodeBlockAnalysisResult& block2,
                            double& similarity);

    // Adapter pattern detection helpers
    bool detectLegacyAdapterPattern(const std::string& content);
    bool detectInterfaceAdapterPattern(const std::string& content);
    bool detectProtocolAdapterPattern(const std::string& content);
    bool detectDataFormatAdapterPattern(const std::string& content);
    bool detectAPIAdapterPattern(const std::string& content);
    bool detectPlatformAdapterPattern(const std::string& content);

    // Similarity calculation helpers
    double calculateJaccardSimilarity(const std::vector<std::string>& set1,
                                     const std::vector<std::string>& set2);
    double calculateCosineSimilarity(const std::vector<std::string>& vec1,
                                    const std::vector<std::string>& vec2);
    double calculateLevenshteinSimilarity(const std::string& str1, const std::string& str2);
    double calculateEditDistance(const std::string& str1, const std::string& str2);

    // Refactoring analysis helpers
    bool identifyRefactoringOpportunities(const CodeBlockAnalysisResult& block,
                                         std::vector<std::string>& opportunities);
    bool suggestCommonExtraction(const std::vector<CodeBlockAnalysisResult>& blocks,
                                 std::string& extraction_plan);
    bool suggestTemplateRefactoring(const std::vector<CodeBlockAnalysisResult>& blocks,
                                    std::string& template_plan);

    // Report generation helpers
    std::string generateExecutiveSummary(const ZeroDuplicationValidationResult& result);
    std::string generateDuplicationAnalysis(const ZeroDuplicationValidationResult& result);
    std::string generateAdapterPatternAnalysis(const ZeroDuplicationValidationResult& result);
    std::string generateSimilarityAnalysis(const ZeroDuplicationValidationResult& result);
    std::string generateRecommendationsSection(const ZeroDuplicationValidationResult& result);

    // File system helpers
    std::vector<std::string> findFilesInDirectory(const std::string& directory,
                                                  const std::vector<std::string>& extensions);
    bool readFileContent(const std::string& filepath, std::string& content);
    bool isExcludedFile(const std::string& filepath);

    // Error handling
    void setError(const std::string& error);
    void clearError();

private:
    std::string project_root_;
    double zero_duplication_threshold_;
    double adapter_pattern_threshold_;
    double structural_similarity_threshold_;
    double logical_similarity_threshold_;
    bool strict_adapter_validation_;
    bool initialized_;

    // Analysis statistics
    size_t total_blocks_analyzed_;
    size_t total_files_analyzed_;
    std::chrono::milliseconds total_analysis_time_;

    // Configuration
    std::vector<std::string> source_extensions_;
    std::vector<std::string> exclude_directories_;
    std::vector<std::string> exclude_patterns_;
    std::vector<std::string> exclude_files_;

    // Analysis patterns
    std::vector<std::regex> function_patterns_;
    std::vector<std::regex> variable_patterns_;
    std::vector<std::regex> include_patterns_;
    std::vector<std::regex> adapter_patterns_;

    // Adapter pattern detection patterns
    std::map<AdapterPatternType, std::vector<std::regex>> adapter_type_patterns_;

    // Analysis cache
    std::map<std::string, std::vector<CodeBlockAnalysisResult>> file_blocks_cache_;
    std::map<std::string, std::string> file_content_cache_;
    std::map<std::string, std::string> signature_cache_;

    // Error handling
    std::string last_error_;
};

/**
 * @brief Utility functions for architectural compliance validation
 */
namespace architectural_compliance_validation_utils {

    // Quick validation functions
    bool quickZeroDuplicationCheck(const std::string& project_root);
    bool validateAdapterPatternUsage(const std::string& project_root);
    bool checkStructuralSimilarity(const std::string& project_root);

    // Analysis utilities
    std::vector<std::string> findExactDuplicates(const std::string& project_root);
    std::vector<std::string> findStructuralDuplicates(const std::string& project_root);
    std::vector<std::string> findLogicalDuplicates(const std::string& project_root);
    std::vector<std::string> findAdapterPatterns(const std::string& project_root);

    // Metrics calculation
    double calculateOverallDuplicationPercentage(const std::string& project_root);
    std::map<std::string, double> calculateFileDuplicationScores(const std::string& project_root);
    std::vector<std::string> getHighDuplicationFiles(const std::string& project_root,
                                                      double threshold = 5.0);

    // Validation utilities
    bool validateZeroDuplicationRequirement(const std::string& project_root);
    bool validateAdapterPatternCompliance(const std::string& project_root);
    bool validateSimilarityThresholds(const std::string& project_root);

    // Compliance improvement utilities
    bool suggestDuplicationElimination(const std::string& project_root,
                                       std::vector<std::string>& suggestions);
    bool generateRefactoringPlan(const std::string& project_root,
                                 const ZeroDuplicationValidationResult& result,
                                 std::string& plan);
    bool runComprehensiveArchitecturalValidation(const std::string& project_root,
                                                  ZeroDuplicationValidationResult& result);

    // Reporting utilities
    bool generateArchitecturalComplianceReport(const std::string& project_root,
                                               std::string& report);
    bool generateDuplicationDashboard(const std::string& project_root, std::string& dashboard);
    bool createComplianceBadge(const ZeroDuplicationValidationResult& result,
                               std::string& badge_svg);
}

} // namespace validation
} // namespace puzzle71
// Puzzle71 Technical Debt Repair - Code Duplication Validator
// Phase 5: User Story 4 - Complete System Migration and Quality Assurance
// Task: T070 - Verify zero code duplication elimination through adapter pattern
// Implements comprehensive code duplication detection and validation system

#pragma once

#include <cuda_runtime.h>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <set>
#include <filesystem>
#include <regex>
#include <fstream>
#include <unordered_set>
#include <algorithm>

namespace puzzle71 {
namespace migration {

// Code duplication validation constants
constexpr double CODE_DUPLICATION_THRESHOLD = 5.0;      // Code duplication threshold (%)
constexpr int MAX_DUPLICATE_BLOCKS = 5;                 // Maximum duplicate blocks allowed
constexpr int MIN_SIMILARITY_THRESHOLD = 80;            // Minimum similarity for duplication (%)
constexpr int MIN_BLOCK_SIZE_LINES = 5;                 // Minimum block size to check
constexpr int MAX_FUNCTION_SIZE_LINES = 50;             // Maximum function size for analysis

// Duplication types
enum class DuplicationType {
    EXACT_DUPLICATE,      // Identical code blocks
    STRUCTURAL_SIMILAR,   // Similar structure with different details
    LOGICAL_SIMILAR,      // Same logic with different implementation
    ADAPTER_PATTERN,      // Intentional adapter pattern duplication
    REFACTORED_DUPLICATE  // Previously duplicated but now unified
};

// Code block representation
struct CodeBlock {
    std::string filepath;
    int start_line;
    int end_line;
    std::string content;
    std::string normalized_content;  // Content with variables/strings normalized
    size_t hash;                     // Hash of normalized content
    int size_lines;
    std::vector<std::string> tokens;

    CodeBlock() : start_line(0), end_line(0), size_lines(0), hash(0) {}
};

// Duplication result
struct DuplicationResult {
    CodeBlock block1;
    CodeBlock block2;
    DuplicationType type;
    double similarity_percentage;
    std::string description;
    bool is_intentional_adapter;     // True if this is intentional adapter pattern

    DuplicationResult() : type(DuplicationType::EXACT_DUPLICATE),
                         similarity_percentage(0.0), is_intentional_adapter(false) {}
};

// Overall code duplication analysis result
struct CodeDuplicationAnalysisResult {
    std::vector<DuplicationResult> duplications;
    std::map<DuplicationType, int> duplication_counts;
    std::map<std::string, int> file_duplication_counts;

    // Statistics
    int total_blocks_analyzed = 0;
    int total_duplications_found = 0;
    double overall_duplication_percentage = 0.0;
    double adapter_pattern_percentage = 0.0;
    size_t total_code_lines = 0;
    size_t duplicated_lines = 0;

    // Performance metrics
    std::chrono::milliseconds analysis_duration;

    CodeDuplicationAnalysisResult() : total_blocks_analyzed(0), total_duplications_found(0),
                                     overall_duplication_percentage(0.0), adapter_pattern_percentage(0.0),
                                     total_code_lines(0), duplicated_lines(0) {}
};

// Adapter pattern validation result
struct AdapterPatternValidationResult {
    bool adapter_pattern_implemented = false;
    bool code_duplication_eliminated = false;
    bool unified_interfaces_present = false;
    bool legacy_abstraction_successful = false;

    // Adapter pattern metrics
    int adapter_classes_found = 0;
    int unified_interfaces_found = 0;
    int legacy_wrappers_found = 0;
    double adapter_effectiveness_score = 0.0;

    // Detailed findings
    std::vector<std::string> adapter_classes;
    std::vector<std::string> unified_interfaces;
    std::vector<std::string> legacy_abstractions;
    std::vector<std::string> remaining_duplications;

    std::chrono::system_clock::time_point validation_timestamp;
};

/**
 * @brief Code duplication validator for T070
 *
 * This class provides comprehensive code duplication detection and validation
 * to ensure that the adapter pattern has successfully eliminated code duplication.
 */
class CodeDuplicationValidator {
public:
    CodeDuplicationValidator();
    ~CodeDuplicationValidator();

    // Initialization and configuration
    bool initialize(const std::string& project_root = ".");
    bool configure(double duplication_threshold = CODE_DUPLICATION_THRESHOLD,
                   bool enable_adapter_pattern_detection = true);
    void shutdown();

    // Main analysis methods
    bool analyzeCodeDuplication(CodeDuplicationAnalysisResult& result);
    bool validateAdapterPattern(AdapterPatternValidationResult& result);
    bool verifyZeroCodeDuplication(CodeDuplicationAnalysisResult& result);

    // Specialized analysis methods
    bool detectExactDuplicates(CodeDuplicationAnalysisResult& result);
    bool detectStructuralDuplicates(CodeDuplicationAnalysisResult& result);
    bool detectAdapterPatternDuplications(CodeDuplicationAnalysisResult& result);
    bool analyzeFunctionalDuplication(CodeDuplicationAnalysisResult& result);

    // File-level analysis
    bool analyzeFile(const std::string& filepath, std::vector<CodeBlock>& blocks);
    bool extractFunctions(const std::string& content, const std::string& filepath,
                         std::vector<CodeBlock>& functions);
    bool extractCodeBlocks(const std::string& content, const std::string& filepath,
                          std::vector<CodeBlock>& blocks);

    // Similarity calculation
    double calculateSimilarity(const CodeBlock& block1, const CodeBlock& block2);
    double calculateStructuralSimilarity(const CodeBlock& block1, const CodeBlock& block2);
    double calculateLogicalSimilarity(const CodeBlock& block1, const CodeBlock& block2);
    std::string normalizeCode(const std::string& code, bool normalize_variables = true,
                             bool normalize_strings = true, bool normalize_numbers = true);

    // Adapter pattern detection
    bool detectAdapterClasses(const std::string& content, const std::string& filepath,
                             std::vector<std::string>& adapter_classes);
    bool detectUnifiedInterfaces(const std::string& content, const std::string& filepath,
                                 std::vector<std::string>& interfaces);
    bool detectLegacyAbstractions(const std::string& content, const std::string& filepath,
                                  std::vector<std::string>& abstractions);

    // Reporting
    bool generateDuplicationReport(const CodeDuplicationAnalysisResult& result, std::string& report);
    bool generateAdapterPatternReport(const AdapterPatternValidationResult& result, std::string& report);
    bool generateSummaryReport(const CodeDuplicationAnalysisResult& dup_result,
                              const AdapterPatternValidationResult& adapter_result,
                              std::string& report);

    // Configuration and status
    void setDuplicationThreshold(double threshold) { duplication_threshold_ = threshold; }
    double getDuplicationThreshold() const { return duplication_threshold_; }

    void setProjectRoot(const std::string& root) { project_root_ = root; }
    std::string getProjectRoot() const { return project_root_; }

    // Statistics
    size_t getTotalBlocksAnalyzed() const { return total_blocks_analyzed_; }
    size_t getTotalDuplicationsFound() const { return total_duplications_found_; }
    std::chrono::milliseconds getTotalAnalysisTime() const { return total_analysis_time_; }

    // Error handling
    std::string getLastError() const { return last_error_; }
    bool hasErrors() const { return !last_error_.empty(); }

private:
    // Internal analysis methods
    std::vector<std::string> tokenizeCode(const std::string& code);
    std::string extractFunctionSignature(const std::string& function_code);
    std::vector<std::string> findFunctionBodies(const std::string& content);
    bool isAdapterPattern(const std::string& class_content);
    bool isUnifiedInterface(const std::string& class_content);
    bool isLegacyAbstraction(const std::string& class_content);

    // Hash and comparison helpers
    size_t calculateBlockHash(const CodeBlock& block);
    bool blocksAreSimilar(const CodeBlock& block1, const CodeBlock& block2, double threshold);
    std::vector<CodeBlock> findSimilarBlocks(const CodeBlock& target_block,
                                           const std::vector<CodeBlock>& blocks,
                                           double threshold);

    // File system helpers
    std::vector<std::string> findSourceFiles(const std::string& directory);
    bool isSourceFile(const std::string& filepath);
    bool shouldAnalyzeFile(const std::string& filepath);

    // Normalization helpers
    std::string normalizeVariables(const std::string& code);
    std::string normalizeStrings(const std::string& code);
    std::string normalizeNumbers(const std::string& code);
    std::string normalizeWhitespace(const std::string& code);
    std::string extractControlStructures(const std::string& code);

    // Report generation helpers
    std::string generateDuplicationSummary(const CodeDuplicationAnalysisResult& result);
    std::string generateDuplicationDetails(const CodeDuplicationAnalysisResult& result);
    std::string generateAdapterPatternSummary(const AdapterPatternValidationResult& result);
    std::string generateRecommendations(const CodeDuplicationAnalysisResult& dup_result,
                                       const AdapterPatternValidationResult& adapter_result);

    // Error handling
    void setError(const std::string& error);
    void clearError();

private:
    std::string project_root_;
    double duplication_threshold_;
    bool enable_adapter_pattern_detection_;
    bool initialized_;

    // Analysis statistics
    size_t total_blocks_analyzed_;
    size_t total_duplications_found_;
    std::chrono::milliseconds total_analysis_time_;

    // Configuration
    std::vector<std::string> source_extensions_;
    std::vector<std::string> exclude_directories_;
    std::vector<std::string> exclude_patterns_;

    // Patterns for detection
    std::vector<std::regex> adapter_class_patterns_;
    std::vector<std::regex> interface_patterns_;
    std::vector<std::regex> legacy_patterns_;

    // Cache for performance
    std::map<std::string, std::vector<CodeBlock>> file_blocks_cache_;
    std::map<size_t, std::vector<CodeBlock>> hash_blocks_cache_;

    // Error handling
    std::string last_error_;
};

/**
 * @brief Utility functions for code duplication validation
 */
namespace duplication_utils {

    // Quick validation functions
    bool quickDuplicationCheck(const std::string& project_root);
    bool validateAdapterPatternImplementation(const std::string& project_root);
    bool countCodeDuplicates(const std::string& project_root, int& duplicate_count);

    // Analysis utilities
    std::vector<std::string> findDuplicateFunctions(const std::string& project_root);
    std::vector<std::string> findSimilarClasses(const std::string& project_root);
    std::vector<std::string> findDuplicateIncludes(const std::string& project_root);

    // Refactoring suggestions
    bool suggestRefactoring(const CodeDuplicationAnalysisResult& result,
                           std::vector<std::string>& suggestions);
    bool generateRefactoringPlan(const std::string& project_root,
                                 const CodeDuplicationAnalysisResult& result,
                                 std::string& plan);

    // Metrics calculation
    double calculateDuplicationDensity(const CodeDuplicationAnalysisResult& result);
    double calculateAdapterPatternEffectiveness(const AdapterPatternValidationResult& result);
    std::map<std::string, double> calculateFileDuplicationMetrics(const std::string& project_root);

    // Comparison utilities
    bool functionsAreEquivalent(const std::string& func1, const std::string& func2);
    bool classesAreSimilar(const std::string& class1, const std::string& class2);
    double calculateCodeSimilarity(const std::string& code1, const std::string& code2);
}

} // namespace migration
} // namespace puzzle71
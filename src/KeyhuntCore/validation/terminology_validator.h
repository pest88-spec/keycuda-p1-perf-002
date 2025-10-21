/**
 * @file terminology_validator.h
 * @brief Terminology consistency validator for constitutional compliance
 *
 * Validates that "unified modules" terminology is used in lowercase across
 * all codebase modules per Constitution v1.2.0 requirements.
 *
 * @author Puzzle71Solver CUDA Team
 * @date 2025-10-19
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <regex>
#include <memory>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>

namespace keyhunt {
namespace validation {

/**
 * @brief Configuration for terminology validation
 */
struct TerminologyValidatorConfig {
    bool enable_logging = true;
    bool generate_corrections = true;
    bool fail_fast = false;
    bool check_comments = false;  // Don't check comments by default
    bool check_strings = false;   // Don't check string literals by default
    std::vector<std::string> additional_patterns = {};
    std::vector<std::string> exclude_directories = {};
    std::vector<std::string> exclude_files = {};
};

/**
 * @brief Severity levels for terminology violations
 */
enum class ViolationSeverity {
    INFO = 0,
    WARNING = 1,
    ERROR = 2
};

/**
 * @brief Individual terminology violation
 */
struct TerminologyViolation {
    int line_number;
    size_t column_start;
    size_t column_end;
    std::string invalid_text;
    std::string corrected_text;
    std::string line_content;
    std::string rule_name;
    ViolationSeverity severity;
};

/**
 * @brief Suggested correction for a terminology violation
 */
struct TerminologyCorrection {
    std::string file_path;
    int line_number;
    size_t column_start;
    size_t column_end;
    std::string original_text;
    std::string corrected_text;
    bool auto_applicable;
    bool requires_review;
};

/**
 * @brief Results for a single file validation
 */
struct FileValidationResult {
    std::string file_path;
    bool success = true;
    std::string error_message;
    std::vector<TerminologyViolation> violations;
};

/**
 * @brief Complete validation results for a project
 */
struct ValidationResult {
    bool success = true;
    std::string project_root;
    double compliance_score = 0.0;
    std::string error_message;
    std::vector<FileValidationResult> file_results;
    std::vector<TerminologyCorrection> corrections;
};

/**
 * @brief Validation metrics
 */
struct ValidationMetrics {
    size_t total_files_scanned = 0;
    size_t files_with_violations = 0;
    size_t total_violations = 0;
    size_t total_lines_processed = 0;
    size_t total_corrections = 0;
    std::chrono::milliseconds validation_time{0};
};

/**
 * @brief Pattern information for validation
 */
struct ValidationPattern {
    std::regex pattern;
    std::string invalid;
    std::string correct;
};

/**
 * @brief Terminology consistency validator
 *
 * Ensures constitutional compliance by validating that "unified modules"
 * terminology is consistently used in lowercase across all codebase modules.
 */
class TerminologyValidator {
public:
    /**
     * @brief Constructor
     * @param config Validation configuration
     */
    explicit TerminologyValidator(const TerminologyValidatorConfig& config = {});

    /**
     * @brief Destructor
     */
    ~TerminologyValidator();

    // Delete copy constructor and assignment operator
    TerminologyValidator(const TerminologyValidator&) = delete;
    TerminologyValidator& operator=(const TerminologyValidator&) = delete;

    /**
     * @brief Validate entire project for terminology compliance
     * @param project_root Root directory of the project
     * @return Complete validation results
     */
    ValidationResult validateProject(const std::string& project_root);

    /**
     * @brief Validate a single file
     * @param file_path Path to the file to validate
     * @return File validation results
     */
    FileValidationResult validateFile(const std::string& file_path);

    /**
     * @brief Apply suggested corrections to files
     * @param corrections List of corrections to apply
     * @return True if corrections were applied successfully
     */
    bool applyCorrections(const std::vector<TerminologyCorrection>& corrections);

    /**
     * @brief Generate compliance report
     * @param result Validation results
     * @return Formatted compliance report in Markdown format
     */
    std::string generateComplianceReport(const ValidationResult& result);

    /**
     * @brief Check if validation results meet constitution requirements
     * @param result Validation results
     * @return True if constitution compliant (100% compliance)
     */
    bool validateConstitutionCompliance(const ValidationResult& result);

    /**
     * @brief Get validation metrics
     * @return Current validation metrics
     */
    const ValidationMetrics& getMetrics() const { return metrics_; }

    /**
     * @brief Create validator with default configuration
     * @param config Optional configuration override
     * @return Unique pointer to validator instance
     */
    static std::unique_ptr<TerminologyValidator> create(
        const TerminologyValidatorConfig& config = {});

    /**
     * @brief Quick validation check (pass/fail only)
     * @param project_root Root directory of the project
     * @return True if constitution compliant
     */
    static bool quickValidate(const std::string& project_root);

    /**
     * @brief Validate and automatically fix violations
     * @param project_root Root directory of the project
     * @return True if all violations were fixed
     */
    static bool validateAndFix(const std::string& project_root);

private:
    // Configuration and state
    TerminologyValidatorConfig config_;
    ValidationMetrics metrics_;
    std::vector<ValidationPattern> invalid_patterns_;
    std::vector<std::string> skip_directories_;
    std::vector<std::string> skip_files_;

    // Static validation data
    static const std::map<std::string, std::string> VALIDATION_PATTERNS;
    static const std::vector<std::string> ALLOWED_CONTEXTS;
    static const std::vector<std::string> FILE_EXTENSIONS_TO_CHECK;

    /**
     * @brief Setup validation patterns
     */
    void setupValidationPatterns();

    /**
     * @brief Setup file filters
     */
    void setupFileFilters();

    /**
     * @brief Collect all files to validate
     * @param project_root Root directory
     * @return List of file paths to validate
     */
    std::vector<std::string> collectFiles(const std::string& project_root);

    /**
     * @brief Check if directory should be skipped
     * @param dir_name Directory name
     * @return True if should skip
     */
    bool shouldSkipDirectory(const std::string& dir_name) const;

    /**
     * @brief Check if file should be validated
     * @param file_path File path
     * @return True if should validate
     */
    bool shouldCheckFile(const std::filesystem::path& file_path) const;

    /**
     * @brief Check if term is in allowed context
     * @param position Position in line
     * @param line Full line content
     * @param invalid_text The invalid text found
     * @return True if allowed context
     */
    bool isAllowedContext(size_t position, const std::string& line,
                         const std::string& invalid_text) const;

    /**
     * @brief Get surrounding context for validation
     * @param position Position in line
     * @param line Full line content
     * @return Surrounding context string
     */
    std::string getSurroundingContext(size_t position, const std::string& line) const;

    /**
     * @brief Check if position is within a comment
     * @param position Position in line
     * @param line Full line content
     * @return True if in comment
     */
    bool isInComment(size_t position, const std::string& line) const;

    /**
     * @brief Check if position is within a string literal
     * @param position Position in line
     * @param line Full line content
     * @return True if in string literal
     */
    bool isInStringLiteral(size_t position, const std::string& line) const;

    /**
     * @brief Determine violation severity
     * @param invalid_text The invalid text found
     * @return Severity level
     */
    ViolationSeverity determineSeverity(const std::string& invalid_text) const;

    /**
     * @brief Generate corrections from file results
     * @param file_results Results from file validation
     * @return List of suggested corrections
     */
    std::vector<TerminologyCorrection> generateCorrections(
        const std::vector<FileValidationResult>& file_results);

    /**
     * @brief Check if correction can be automatically applied
     * @param violation The violation to correct
     * @return True if auto-applicable
     */
    bool isAutoApplicable(const TerminologyViolation& violation) const;

    /**
     * @brief Calculate compliance score percentage
     * @param result Validation results
     * @return Compliance score (0-100)
     */
    double calculateComplianceScore(const ValidationResult& result) const;

    /**
     * @brief Apply corrections to a single file
     * @param file_path File path
     * @param corrections Corrections for this file
     * @return True if applied successfully
     */
    bool applyCorrectionsToFile(const std::string& file_path,
                               const std::vector<TerminologyCorrection>& corrections);

    /**
     * @brief Calculate position in file content from line/column
     * @param content File content
     * @param line_number Line number (1-based)
     * @param column Column number (0-based)
     * @return Byte position in content
     */
    size_t calculatePositionInContent(const std::string& content,
                                     int line_number, int column) const;

    /**
     * @brief Generate summary report to console
     */
    void generateSummaryReport() const;

    /**
     * @brief Convert severity to string
     * @param severity Severity level
     * @return String representation
     */
    std::string severityToString(ViolationSeverity severity) const;

    /**
     * @brief Get current timestamp string
     * @return Formatted timestamp
     */
    std::string getCurrentTimestamp() const;
};

} // namespace validation
} // namespace keyhunt
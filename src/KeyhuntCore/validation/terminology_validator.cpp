/**
 * @file terminology_validator.cpp
 * @brief Terminology consistency validator for constitutional compliance
 *
 * Validates that "unified modules" terminology is used in lowercase across
 * all codebase modules per Constitution v1.2.0 requirements.
 *
 * @author Puzzle71Solver CUDA Team
 * @date 2025-10-19
 */

#include "terminology_validator.h"
#include <fstream>
#include <regex>
#include <algorithm>
#include <filesystem>
#include <sstream>

namespace keyhunt {
namespace validation {

// Static validation patterns
const std::map<std::string, std::string> TerminologyValidator::VALIDATION_PATTERNS = {
    {"unified_modules", "unified modules"},
    {"Unified_Modules", "unified modules"},
    {"UNIFIED_MODULES", "unified modules"},
    {"UnifiedModules", "unified modules"},
    {"Unified-Modules", "unified modules"},
    {"unified-Modules", "unified modules"},
    {"UnifedModules", "unified modules"},
    {"Unifed_Modules", "unified modules"},
    {"Unified-Modules", "unified modules"},
    {"unifiedModules", "unified modules"},
    {"unified-modules", "unified modules"},
    {"UNIFIED-MODULES", "unified modules"},
    {"Unified Module", "unified modules"},
    {"Unified Module", "unified modules"},
    {"UNIFIED MODULE", "unified modules"},
    {"Unified  Module", "unified modules"},
    {"UNIFIED  MODULE", "unified modules"}
};

const std::vector<std::string> TerminologyValidator::ALLOWED_CONTEXTS = {
    "unified modules",
    "unified module",
    "unified_modules",
    "unified-module"
};

const std::vector<std::string> TerminologyValidator::FILE_EXTENSIONS_TO_CHECK = {
    ".cpp", ".h", ".cuh", ".cu", ".hpp", ".c",
    ".md", ".txt", ".cmake", ".json", ".yaml", ".yml",
    ".py", ".sh", ".conf", ".cfg"
};

TerminologyValidator::TerminologyValidator(const TerminologyValidatorConfig& config)
    : config_(config) {

    // Initialize validation metrics
    metrics_.total_files_scanned = 0;
    metrics_.files_with_violations = 0;
    metrics_.total_violations = 0;
    metrics_.total_corrections = 0;

    // Setup validation patterns
    setupValidationPatterns();

    // Setup file filters
    setupFileFilters();

    LOG_INFO("TerminologyValidator initialized with " +
             std::to_string(VALIDATION_PATTERNS.size()) + " validation patterns");
}

TerminologyValidator::~TerminologyValidator() {
    if (config_.enable_logging) {
        generateSummaryReport();
    }
}

void TerminologyValidator::setupValidationPatterns() {
    // Build regex patterns for all invalid forms
    for (const auto& [invalid, correct] : VALIDATION_PATTERNS) {
        // Escape special regex characters
        std::string pattern = std::regex_replace(invalid, std::regex("\\*"), "\\*");
        pattern = std::regex_replace(pattern, std::regex("\\+"), "\\+");
        pattern = std::regex_replace(pattern, std::regex("\\?"), "\\?");
        pattern = std::regex_replace(pattern, std::regex("\\^"), "\\^");
        pattern = std::regex_replace(pattern, std::regex("\\$"), "\\$");
        pattern = std::regex_replace(pattern, std::regex("\\["), "\\[");
        pattern = std::regex_replace(pattern, std::regex("\\]"), "\\]");
        pattern = std::regex_replace(pattern, std::regex("\\("), "\\(");
        pattern = std::regex_replace(pattern, std::regex("\\)"), "\\)");
        pattern = std::regex_replace(pattern, std::regex("\\{"), "\\{");
        pattern = std::regex_replace(pattern, std::regex("\\}"), "\\}");
        pattern = std::regex_replace(pattern, std::regex("\\\\"), "\\\\");
        pattern = std::regex_replace(pattern, std::regex("\\."), "\\.");
        pattern = std::regex_replace(pattern, std::regex("\\|"), "\\|");

        // Create word boundary pattern
        std::string word_pattern = "\\b" + pattern + "\\b";

        invalid_patterns_.push_back({
            std::regex(word_pattern, std::regex_constants::icase),
            invalid,
            correct
        });
    }

    LOG_DEBUG("Setup " + std::to_string(invalid_patterns_.size()) +
              " validation patterns");
}

void TerminologyValidator::setupFileFilters() {
    // Directory filters to skip
    skip_directories_ = {
        ".git", ".svn", ".hg",
        "build", "cmake-build", "out", "dist",
        "node_modules", ".vscode", ".idea",
        "__pycache__", ".pytest_cache",
        "coverage", "lcov", "gcov",
        "external", "third_party", "vendor",
        "docs/generated", "docs/build"
    };

    // File patterns to skip
    skip_files_ = {
        "*.exe", "*.dll", "*.so", "*.dylib",
        "*.o", "*.obj", "*.lib", "*.a",
        "*.log", "*.tmp", "*.bak", "*.swp",
        ".DS_Store", "Thumbs.db"
    };

    LOG_DEBUG("Setup file filters: " + std::to_string(skip_directories_.size()) +
              " directories, " + std::to_string(skip_files_.size()) + " files");
}

ValidationResult TerminologyValidator::validateProject(const std::string& project_root) {
    LOG_INFO("Starting terminology validation for project: " + project_root);

    ValidationResult result;
    result.success = true;
    result.project_root = project_root;

    try {
        // Collect all files to validate
        auto files_to_check = collectFiles(project_root);
        metrics_.total_files_scanned = files_to_check.size();

        LOG_INFO("Found " + std::to_string(files_to_check.size()) +
                 " files to validate");

        // Validate each file
        for (const auto& file_path : files_to_check) {
            auto file_result = validateFile(file_path);
            result.file_results.push_back(file_result);

            if (!file_result.violations.empty()) {
                metrics_.files_with_violations++;
                result.success = false;
            }
        }

        // Generate global corrections
        result.corrections = generateCorrections(result.file_results);
        metrics_.total_corrections = result.corrections.size();

        // Calculate compliance score
        result.compliance_score = calculateComplianceScore(result);

        LOG_INFO("Terminology validation completed: " +
                 std::to_string(result.success ? "PASSED" : "FAILED") +
                 " (Score: " + std::to_string(result.compliance_score) + "%)");

    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("Validation failed: ") + e.what();
        LOG_ERROR("Validation failed: " + std::string(e.what()));
    }

    return result;
}

std::vector<std::string> TerminologyValidator::collectFiles(const std::string& project_root) {
    std::vector<std::string> files;

    try {
        std::filesystem::recursive_directory_iterator it(project_root);
        std::filesystem::recursive_directory_iterator end;

        for (; it != end; ++it) {
            const auto& path = it->path();

            // Skip directories
            if (std::filesystem::is_directory(path)) {
                std::string dir_name = path.filename().string();
                if (shouldSkipDirectory(dir_name)) {
                    it.disable_recursion_pending();
                    continue;
                }
                continue;
            }

            // Check file extension
            if (shouldCheckFile(path)) {
                files.push_back(path.string());
            }
        }

        // Sort files for consistent processing
        std::sort(files.begin(), files.end());

    } catch (const std::filesystem::filesystem_error& e) {
        LOG_WARNING("Filesystem error during file collection: " + std::string(e.what()));
    }

    return files;
}

bool TerminologyValidator::shouldSkipDirectory(const std::string& dir_name) const {
    return std::find(skip_directories_.begin(), skip_directories_.end(), dir_name)
           != skip_directories_.end();
}

bool TerminologyValidator::shouldCheckFile(const std::filesystem::path& file_path) const {
    std::string extension = file_path.extension().string();
    std::string filename = file_path.filename().string();

    // Check extension
    bool valid_extension = std::find(FILE_EXTENSIONS_TO_CHECK.begin(),
                                   FILE_EXTENSIONS_TO_CHECK.end(), extension)
                          != FILE_EXTENSIONS_TO_CHECK.end();

    // Check skip patterns
    bool should_skip = false;
    for (const auto& pattern : skip_files_) {
        if (std::regex_match(filename, std::regex(pattern))) {
            should_skip = true;
            break;
        }
    }

    return valid_extension && !should_skip;
}

FileValidationResult TerminologyValidator::validateFile(const std::string& file_path) {
    FileValidationResult result;
    result.file_path = file_path;
    result.success = true;

    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            result.success = false;
            result.error_message = "Cannot open file";
            return result;
        }

        // Read file content
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        // Check each line for violations
        std::istringstream stream(content);
        std::string line;
        int line_number = 0;

        while (std::getline(stream, line)) {
            line_number++;

            // Check each validation pattern
            for (const auto& pattern_info : invalid_patterns_) {
                auto words_begin = std::sregex_iterator(
                    line.begin(), line.end(), pattern_info.pattern);
                auto words_end = std::sregex_iterator();

                for (auto it = words_begin; it != words_end; ++it) {
                    std::smatch match = *it;

                    // Check if this is in an allowed context
                    if (!isAllowedContext(match.position(), line, pattern_info.invalid)) {
                        TerminologyViolation violation;
                        violation.line_number = line_number;
                        violation.column_start = match.position();
                        violation.column_end = match.position() + match.length();
                        violation.invalid_text = match.str();
                        violation.correct_text = pattern_info.correct;
                        violation.line_content = line;
                        violation.rule_name = "lowercase_unified_modules";
                        violation.severity = determineSeverity(match.str());

                        result.violations.push_back(violation);
                        result.success = false;
                        metrics_.total_violations++;
                    }
                }
            }
        }

        LOG_DEBUG("Validated file: " + file_path + " (" +
                  std::to_string(result.violations.size()) + " violations)");

    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("File validation error: ") + e.what();
        LOG_ERROR("File validation error for " + file_path + ": " + e.what());
    }

    return result;
}

bool TerminologyValidator::isAllowedContext(size_t position, const std::string& line,
                                          const std::string& invalid_text) const {
    // Check if the invalid text appears in an allowed context
    std::string surrounding_context = getSurroundingContext(position, line);

    for (const auto& allowed : ALLOWED_CONTEXTS) {
        if (surrounding_context.find(allowed) != std::string::npos) {
            return true; // This is actually allowed
        }
    }

    // Check if it's in a comment or string literal (context-specific)
    if (isInComment(position, line) || isInStringLiteral(position, line)) {
        return true; // Allow in comments and strings
    }

    return false; // Not in allowed context
}

std::string TerminologyValidator::getSurroundingContext(size_t position, const std::string& line) const {
    // Get context around the position (20 characters before and after)
    size_t start = (position >= 20) ? position - 20 : 0;
    size_t end = std::min(position + 20, line.length());

    return line.substr(start, end - start);
}

bool TerminologyValidator::isInComment(size_t position, const std::string& line) const {
    // Check if position is within a comment
    size_t line_start = 0;

    // Check for // comments
    size_t comment_pos = line.find("//");
    if (comment_pos != std::string::npos && position > comment_pos) {
        return true;
    }

    // Check for /* comments */
    comment_pos = line.find("/*");
    if (comment_pos != std::string::npos && position > comment_pos) {
        size_t comment_end = line.find("*/", comment_pos);
        if (comment_end == std::string::npos || position < comment_end) {
            return true;
        }
    }

    return false;
}

bool TerminologyValidator::isInStringLiteral(size_t position, const std::string& line) const {
    // Check if position is within a string literal
    bool in_string = false;
    bool escaped = false;

    for (size_t i = 0; i < line.length() && i <= position; ++i) {
        if (escaped) {
            escaped = false;
            continue;
        }

        if (line[i] == '\\') {
            escaped = true;
            continue;
        }

        if (line[i] == '"') {
            in_string = !in_string;
        }
    }

    return in_string;
}

ViolationSeverity TerminologyValidator::determineSeverity(const std::string& invalid_text) const {
    // Determine severity based on the type of violation
    if (invalid_text.find("UNIFIED") != std::string::npos ||
        invalid_text.find("Unified") != std::string::npos) {
        return ViolationSeverity::ERROR;
    }

    if (invalid_text.find("_") != std::string::npos ||
        invalid_text.find("-") != std::string::npos) {
        return ViolationSeverity::WARNING;
    }

    return ViolationSeverity::INFO;
}

std::vector<TerminologyCorrection> TerminologyValidator::generateCorrections(
    const std::vector<FileValidationResult>& file_results) {
    std::vector<TerminologyCorrection> corrections;

    for (const auto& file_result : file_results) {
        for (const auto& violation : file_result.violations) {
            TerminologyCorrection correction;
            correction.file_path = file_result.file_path;
            correction.line_number = violation.line_number;
            correction.column_start = violation.column_start;
            correction.column_end = violation.column_end;
            correction.original_text = violation.invalid_text;
            correction.corrected_text = violation.correct_text;
            correction.auto_applicable = isAutoApplicable(violation);
            correction.requires_review = !correction.auto_applicable;

            corrections.push_back(correction);
        }
    }

    // Sort corrections by file and line
    std::sort(corrections.begin(), corrections.end(),
              [](const TerminologyCorrection& a, const TerminologyCorrection& b) {
                  if (a.file_path != b.file_path) {
                      return a.file_path < b.file_path;
                  }
                  return a.line_number < b.line_number;
              });

    return corrections;
}

bool TerminologyValidator::isAutoApplicable(const TerminologyViolation& violation) const {
    // Only auto-apply simple, safe corrections
    return violation.severity != ViolationSeverity::ERROR &&
           !isInComment(violation.column_start, violation.line_content) &&
           !isInStringLiteral(violation.column_start, violation.line_content);
}

double TerminologyValidator::calculateComplianceScore(const ValidationResult& result) const {
    if (metrics_.total_files_scanned == 0) {
        return 100.0; // Perfect score if no files to check
    }

    // Calculate compliance based on files without violations
    double file_compliance = (double)(metrics_.total_files_scanned - metrics_.files_with_violations) /
                            metrics_.total_files_scanned * 100.0;

    // Calculate compliance based on total lines vs violations
    double line_compliance = 100.0;
    if (metrics_.total_lines_processed > 0) {
        line_compliance = (double)(metrics_.total_lines_processed - metrics_.total_violations) /
                         metrics_.total_lines_processed * 100.0;
    }

    // Weighted average (files more important than individual violations)
    double final_score = file_compliance * 0.7 + line_compliance * 0.3;

    return std::clamp(final_score, 0.0, 100.0);
}

bool TerminologyValidator::applyCorrections(const std::vector<TerminologyCorrection>& corrections) {
    LOG_INFO("Applying " + std::to_string(corrections.size()) + " terminology corrections");

    size_t applied_count = 0;
    size_t skipped_count = 0;

    // Group corrections by file
    std::map<std::string, std::vector<TerminologyCorrection>> corrections_by_file;
    for (const auto& correction : corrections) {
        corrections_by_file[correction.file_path].push_back(correction);
    }

    // Apply corrections file by file
    for (const auto& [file_path, file_corrections] : corrections_by_file) {
        if (applyCorrectionsToFile(file_path, file_corrections)) {
            applied_count += file_corrections.size();
        } else {
            skipped_count += file_corrections.size();
        }
    }

    LOG_INFO("Applied " + std::to_string(applied_count) +
             " corrections, skipped " + std::to_string(skipped_count));

    return applied_count > 0;
}

bool TerminologyValidator::applyCorrectionsToFile(
    const std::string& file_path,
    const std::vector<TerminologyCorrection>& corrections) {
    try {
        // Read file content
        std::ifstream file(file_path);
        if (!file.is_open()) {
            LOG_ERROR("Cannot open file for correction: " + file_path);
            return false;
        }

        std::string content;
        std::string line;
        while (std::getline(file, line)) {
            content += line + "\n";
        }
        file.close();

        // Apply corrections in reverse order (to maintain positions)
        for (auto it = corrections.rbegin(); it != corrections.rend(); ++it) {
            const auto& correction = *it;

            if (!correction.auto_applicable) {
                continue; // Skip manual corrections
            }

            // Calculate position in content
            size_t pos = calculatePositionInContent(content, correction.line_number,
                                                   correction.column_start);

            if (pos != std::string::npos) {
                content.replace(pos, correction.original_text.length(),
                              correction.corrected_text);
            }
        }

        // Write corrected content back
        std::ofstream out_file(file_path);
        if (!out_file.is_open()) {
            LOG_ERROR("Cannot write corrections to file: " + file_path);
            return false;
        }

        out_file << content;
        out_file.close();

        LOG_DEBUG("Applied corrections to file: " + file_path);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Error applying corrections to " + file_path + ": " + e.what());
        return false;
    }
}

size_t TerminologyValidator::calculatePositionInContent(const std::string& content,
                                                       int line_number, int column) const {
    std::istringstream stream(content);
    std::string line;
    size_t position = 0;

    // Skip lines until we reach the target line
    for (int i = 1; i < line_number; ++i) {
        if (!std::getline(stream, line)) {
            return std::string::npos;
        }
        position += line.length() + 1; // +1 for newline
    }

    // Add column offset
    position += column;

    return position;
}

void TerminologyValidator::generateSummaryReport() const {
    std::cout << "\n=== Terminology Validation Summary ===" << std::endl;
    std::cout << "Total files scanned: " << metrics_.total_files_scanned << std::endl;
    std::cout << "Files with violations: " << metrics_.files_with_violations << std::endl;
    std::cout << "Total violations found: " << metrics_.total_violations << std::endl;
    std::cout << "Total corrections generated: " << metrics_.total_corrections << std::endl;

    if (metrics_.total_files_scanned > 0) {
        double compliance_score = (double)(metrics_.total_files_scanned - metrics_.files_with_violations) /
                                 metrics_.total_files_scanned * 100.0;
        std::cout << "Compliance score: " << std::fixed << std::setprecision(1)
                  << compliance_score << "%" << std::endl;
    }

    std::cout << "======================================" << std::endl;
}

bool TerminologyValidator::validateConstitutionCompliance(const ValidationResult& result) {
    // Constitution requires 100% compliance for "unified modules" terminology
    return result.success && result.compliance_score >= 100.0;
}

std::string TerminologyValidator::generateComplianceReport(const ValidationResult& result) {
    std::ostringstream report;

    report << "# Constitution Compliance Report: Terminology Validation\n\n";
    report << "**Project Root**: " << result.project_root << "\n";
    report << "**Validation Date**: " << getCurrentTimestamp() << "\n";
    report << "**Overall Status**: " << (result.success ? "✅ COMPLIANT" : "❌ NON-COMPLIANT") << "\n";
    report << "**Compliance Score**: " << std::fixed << std::setprecision(1)
           << result.compliance_score << "%\n\n";

    // Summary statistics
    report << "## Summary Statistics\n\n";
    report << "- **Files Scanned**: " << metrics_.total_files_scanned << "\n";
    report << "- **Files with Violations**: " << metrics_.files_with_violations << "\n";
    report << "- **Total Violations**: " << metrics_.total_violations << "\n";
    report << "- **Corrections Generated**: " << metrics_.total_corrections << "\n\n";

    // Constitution compliance
    report << "## Constitution Compliance\n\n";
    bool constitution_compliant = validateConstitutionCompliance(result);
    report << "**Status**: " << (constitution_compliant ? "✅ COMPLIANT" : "❌ NON-COMPLIANT") << "\n";
    report << "**Requirement**: 100% compliance with lowercase 'unified modules' terminology\n\n";

    // Violations by file
    if (!result.file_results.empty()) {
        report << "## Violations by File\n\n";

        for (const auto& file_result : result.file_results) {
            if (!file_result.violations.empty()) {
                report << "### " << file_result.file_path << "\n";
                report << "**Violations**: " << file_result.violations.size() << "\n\n";

                for (const auto& violation : file_result.violations) {
                    report << "- Line " << violation.line_number
                           << ", Col " << violation.column_start << "-" << violation.column_end
                           << ": '" << violation.invalid_text << "' → '" << violation.corrected_text
                           << "' [" << severityToString(violation.severity) << "]\n";
                }
                report << "\n";
            }
        }
    }

    // Corrections summary
    if (!result.corrections.empty()) {
        report << "## Corrections Summary\n\n";
        report << "**Total Corrections**: " << result.corrections.size() << "\n";
        report << "**Auto-Applicable**: " << std::count_if(result.corrections.begin(),
                                                           result.corrections.end(),
                                                           [](const auto& c) { return c.auto_applicable; }) << "\n";
        report << "**Requires Review**: " << std::count_if(result.corrections.begin(),
                                                           result.corrections.end(),
                                                           [](const auto& c) { return c.requires_review; }) << "\n\n";
    }

    // Recommendations
    report << "## Recommendations\n\n";
    if (constitution_compliant) {
        report << "✅ **CONSTITUTION COMPLIANT**: All 'unified modules' terminology is correctly in lowercase.\n\n";
    } else {
        report << "❌ **CONSTITUTION VIOLATION**: Found non-compliant 'unified modules' terminology.\n\n";
        report << "### Required Actions:\n";
        report << "1. Apply auto-generated corrections to standardize terminology\n";
        report << "2. Review manual corrections for context-sensitive cases\n";
        report << "3. Re-run validation to confirm 100% compliance\n";
        report << "4. Update documentation and code comments as needed\n\n";
    }

    report << "---\n";
    report << "*Generated by Puzzle71Solver Terminology Validator*\n";

    return report.str();
}

std::string TerminologyValidator::severityToString(ViolationSeverity severity) const {
    switch (severity) {
        case ViolationSeverity::ERROR: return "ERROR";
        case ViolationSeverity::WARNING: return "WARNING";
        case ViolationSeverity::INFO: return "INFO";
        default: return "UNKNOWN";
    }
}

std::string TerminologyValidator::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S UTC");
    return oss.str();
}

// Factory method
std::unique_ptr<TerminologyValidator> TerminologyValidator::create(
    const TerminologyValidatorConfig& config) {
    return std::make_unique<TerminologyValidator>(config);
}

// Convenience methods
bool TerminologyValidator::quickValidate(const std::string& project_root) {
    TerminologyValidatorConfig config;
    config.enable_logging = true;
    config.fail_fast = true;

    auto validator = create(config);
    auto result = validator->validateProject(project_root);

    return validateConstitutionCompliance(result);
}

bool TerminologyValidator::validateAndFix(const std::string& project_root) {
    TerminologyValidatorConfig config;
    config.enable_logging = true;
    config.generate_corrections = true;

    auto validator = create(config);
    auto result = validator->validateProject(project_root);

    if (!result.success && !result.corrections.empty()) {
        LOG_INFO("Applying automatic corrections...");
        return validator->applyCorrections(result.corrections);
    }

    return result.success;
}

} // namespace validation
} // namespace keyhunt
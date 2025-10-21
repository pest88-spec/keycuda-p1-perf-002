// Puzzle71 Technical Debt Repair - Legacy Code Removal Framework
// Phase 5: User Story 4 - Complete System Migration and Quality Assurance
// Task: T069 - Remove all legacy code paths and deprecated functions
// Implements comprehensive legacy code detection and validation system

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

namespace puzzle71 {
namespace migration {

// Legacy removal validation constants
constexpr int MAX_LEGACY_FILES = 0;                     // Zero legacy files allowed
constexpr int MAX_LEGACY_FUNCTIONS = 0;                  // Zero legacy functions allowed
constexpr int MAX_LEGACY_CLASSES = 0;                   // Zero legacy classes allowed
constexpr double MAX_LEGACY_CODE_PERCENTAGE = 0.0;       // Zero legacy code percentage allowed
constexpr double MIN_UNIFIED_MODULE_USAGE = 95.0;      // Minimum unified module usage (%)
constexpr int MAX_DEPRECATED_API_USAGE = 0;              // Zero deprecated API usage

// Legacy code patterns to detect
namespace legacy_patterns {
    // Function patterns
    const std::vector<std::regex> LEGACY_FUNCTION_PATTERNS = {
        std::regex(R"(\blegacy_\w+)"),                    // Functions starting with "legacy_"
        std::regex(R"(\bold_\w+)"),                       // Functions starting with "old_"
        std::regex(R"(\bdeprecated_\w+)"),                // Functions starting with "deprecated_"
        std::regex(R"(\b__legacy\w+)"),                   // Functions with "__legacy" prefix
        std::regex(R"(\b_v\d+_.*\()")                    // Versioned functions like _v1_, _v2_
    };

    // Class/Struct patterns
    const std::vector<std::regex> LEGACY_CLASS_PATTERNS = {
        std::regex(R"(\bclass\s+Legacy\w+)"),             // Classes starting with "Legacy"
        std::regex(R"(\bstruct\s+Legacy\w+)"),            // Structs starting with "Legacy"
        std::regex(R"(\bclass\s+Old\w+)"),                // Classes starting with "Old"
        std::regex(R"(\bstruct\s+Old\w+)"),               // Structs starting with "Old"
        std::regex(R"(\bclass\s+Deprecated\w+)"),         // Classes starting with "Deprecated"
        std::regex(R"(\bstruct\s+Deprecated\w+)")          // Structs starting with "Deprecated"
    };

    // File patterns
    const std::vector<std::regex> LEGACY_FILE_PATTERNS = {
        std::regex(R"(legacy_\w+\.(cpp|cu|h|hpp|cuh))"),  // Files starting with "legacy_"
        std::regex(R"(old_\w+\.(cpp|cu|h|hpp|cuh))"),      // Files starting with "old_"
        std::regex(R"(deprecated_\w+\.(cpp|cu|h|hpp|cuh))"), // Files starting with "deprecated_"
        std::regex(R"(_v\d+_.*\.(cpp|cu|h|hpp|cuh))"),     // Versioned files like _v1_, _v2_
        std::regex(R"(\w+_legacy\.(cpp|cu|h|hpp|cuh))")    // Files ending with "_legacy"
    };

    // Include patterns
    const std::vector<std::regex> LEGACY_INCLUDE_PATTERNS = {
        std::regex(R"(#include\s+[\"<]legacy_\w+[\">])"),  // Legacy includes
        std::regex(R"(#include\s+[\"<]old_\w+[\">])"),      // Old includes
        std::regex(R"(#include\s+[\"<]deprecated_\w+[\">])") // Deprecated includes
    };

    // Comment patterns indicating legacy code
    const std::vector<std::regex> LEGACY_COMMENT_PATTERNS = {
        std::regex(R"(//\s*TODO:\s*Remove\s+legacy)"),       // Comments about removing legacy
        std::regex(R"(//\s*FIXME:\s*Legacy)"),               // FIXME comments about legacy
        std::regex(R"(//\s*DEPRECATED:)"),                    // DEPRECATED comments
        std::regex(R"(/\*\*[^*]*DEPRECATED[^*]*\*/)"),       // Deprecated documentation
        std::regex(R"(/\*[^*]*legacy[^*]*\*/)")              // Legacy comments
    };
}

// Legacy code detection results
struct LegacyCodeDetectionResult {
    std::vector<std::string> legacy_files;
    std::vector<std::string> legacy_functions;
    std::vector<std::string> legacy_classes;
    std::vector<std::string> legacy_includes;
    std::vector<std::string> legacy_comments;
    std::vector<std::string> deprecated_api_usage;

    // Statistics
    int total_files_scanned = 0;
    int total_lines_scanned = 0;
    double legacy_code_percentage = 0.0;
    double unified_module_usage_percentage = 0.0;

    // Timing
    std::chrono::milliseconds scan_duration;

    LegacyCodeDetectionResult() : total_files_scanned(0), total_lines_scanned(0),
                                 legacy_code_percentage(0.0),
                                 unified_module_usage_percentage(0.0) {}
};

// Migration validation result
struct MigrationValidationResult {
    bool migration_complete = false;
    bool zero_legacy_code = false;
    bool full_unified_module_usage = false;
    bool no_deprecated_apis = false;
    bool constitutional_compliance = false;

    // Detailed results
    LegacyCodeDetectionResult detection_result;
    std::map<std::string, bool> compliance_checks;
    std::vector<std::string> recommendations;
    std::vector<std::string> blocking_issues;

    // Overall metrics
    double overall_migration_score = 0.0;
    std::chrono::system_clock::time_point validation_timestamp;

    MigrationValidationResult() : validation_timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Main legacy code removal framework class
 *
 * This framework provides comprehensive detection and validation of legacy code
 * removal as part of the Phase 5 migration to unified modules.
 */
class LegacyRemovalFramework {
public:
    LegacyRemovalFramework();
    ~LegacyRemovalFramework();

    // Initialization and configuration
    bool initialize(const std::string& project_root = ".");
    bool configure(bool strict_mode = true, bool generate_reports = true);
    void shutdown();

    // Legacy code detection
    bool detectLegacyCode(LegacyCodeDetectionResult& result);
    bool scanDirectory(const std::string& directory, LegacyCodeDetectionResult& result);
    bool scanFile(const std::string& filepath, LegacyCodeDetectionResult& result);

    // Migration validation
    bool validateMigration(MigrationValidationResult& result);
    bool validateZeroLegacyCode(MigrationValidationResult& result);
    bool validateUnifiedModuleUsage(MigrationValidationResult& result);
    bool validateNoDeprecatedAPIs(MigrationValidationResult& result);

    // Constitutional compliance validation
    bool validateConstitutionalCompliance(MigrationValidationResult& result);
    bool validateStaticConfigurationCompliance(MigrationValidationResult& result);
    bool validateDeterministicBehavior(MigrationValidationResult& result);
    bool validateBitLevelAccuracy(MigrationValidationResult& result);

    // Reporting and analysis
    bool generateMigrationReport(const MigrationValidationResult& result, std::string& report);
    bool generateDetailedLegacyReport(const LegacyCodeDetectionResult& result, std::string& report);
    bool generateComplianceReport(const MigrationValidationResult& result, std::string& report);

    // Automated remediation suggestions
    bool suggestRemediation(const LegacyCodeDetectionResult& detection_result,
                           std::vector<std::string>& suggestions);
    bool generateMigrationPlan(const LegacyCodeDetectionResult& detection_result,
                              std::string& migration_plan);

    // Configuration and status
    void setStrictMode(bool strict) { strict_mode_ = strict; }
    bool isStrictMode() const { return strict_mode_; }

    void setProjectRoot(const std::string& root) { project_root_ = root; }
    std::string getProjectRoot() const { return project_root_; }

    // Statistics and monitoring
    size_t getTotalFilesScanned() const { return total_files_scanned_; }
    size_t getTotalLinesScanned() const { return total_lines_scanned_; }
    std::chrono::milliseconds getTotalScanTime() const { return total_scan_time_; }

    // Error handling
    std::string getLastError() const { return last_error_; }
    bool hasErrors() const { return !last_error_.empty(); }

private:
    // Internal detection methods
    bool detectLegacyFunctions(const std::string& content, const std::string& filename,
                              LegacyCodeDetectionResult& result);
    bool detectLegacyClasses(const std::string& content, const std::string& filename,
                            LegacyCodeDetectionResult& result);
    bool detectLegacyIncludes(const std::string& content, const std::string& filename,
                             LegacyCodeDetectionResult& result);
    bool detectLegacyComments(const std::string& content, const std::string& filename,
                             LegacyCodeDetectionResult& result);

    // Pattern matching helpers
    bool matchesAnyPattern(const std::string& text,
                          const std::vector<std::regex>& patterns);
    std::vector<std::string> findMatches(const std::string& content,
                                        const std::regex& pattern);

    // File system helpers
    std::vector<std::string> findSourceFiles(const std::string& directory);
    bool isSourceFile(const std::string& filepath);
    bool shouldSkipFile(const std::string& filepath);

    // Analysis helpers
    double calculateLegacyCodePercentage(const LegacyCodeDetectionResult& result);
    double calculateUnifiedModuleUsage(const LegacyCodeDetectionResult& result);
    std::map<std::string, int> countLinesOfCode(const std::string& filepath);

    // Report generation helpers
    std::string generateSummarySection(const MigrationValidationResult& result);
    std::string generateLegacyCodeSection(const LegacyCodeDetectionResult& result);
    std::string generateComplianceSection(const MigrationValidationResult& result);
    std::string generateRecommendationsSection(const MigrationValidationResult& result);

    // Error handling
    void setError(const std::string& error);
    void clearError();

private:
    std::string project_root_;
    bool strict_mode_;
    bool generate_reports_;
    bool initialized_;

    // Scanning statistics
    size_t total_files_scanned_;
    size_t total_lines_scanned_;
    std::chrono::milliseconds total_scan_time_;

    // Configuration
    std::vector<std::string> source_extensions_;
    std::vector<std::string> exclude_directories_;
    std::vector<std::string> exclude_patterns_;

    // Cache for performance
    std::map<std::string, LegacyCodeDetectionResult> scan_cache_;
    std::chrono::system_clock::time_point last_scan_time_;

    // Error handling
    std::string last_error_;
};

/**
 * @brief Utility functions for legacy code removal validation
 */
namespace legacy_removal_utils {

    // Quick validation functions
    bool quickLegacyCodeCheck(const std::string& project_root);
    bool validateNoLegacyFiles(const std::string& project_root);
    bool validateNoLegacyFunctions(const std::string& project_root);
    bool validateUnifiedModuleUsage(const std::string& project_root, double threshold = 95.0);

    // Analysis utilities
    std::vector<std::string> findDuplicateFunctions(const std::string& project_root);
    std::vector<std::string> findUnusedIncludes(const std::string& project_root);
    std::vector<std::string> findDeadCode(const std::string& project_root);

    // Migration utilities
    bool createMigrationBackup(const std::string& project_root, const std::string& backup_path);
    bool validateMigrationIntegrity(const std::string& project_root, const std::string& backup_path);
    bool rollbackMigration(const std::string& project_root, const std::string& backup_path);

    // Reporting utilities
    bool generateMigrationSummary(const std::string& project_root, std::string& summary);
    bool exportMigrationResults(const MigrationValidationResult& result,
                               const std::string& export_path);
}

} // namespace migration
} // namespace puzzle71
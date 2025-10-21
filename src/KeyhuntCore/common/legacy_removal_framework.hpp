// Puzzle71 Technical Debt Repair - Legacy Code Removal Validation Framework
// User Story 3: Complete System Migration and Quality Assurance
// TDD Implementation: This framework makes T067 legacy removal validation tests pass (GREEN phase)

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <fstream>
#include <regex>
#include <filesystem>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <json/json.hpp>

namespace keyhunt {
namespace migration {

// Legacy removal validation metrics and thresholds
struct LegacyRemovalMetrics {
    // Legacy file metrics
    int total_legacy_files = 0;
    std::vector<std::string> legacy_files_found;
    std::map<std::string, std::vector<std::string>> legacy_file_patterns;

    // Legacy function metrics
    int total_legacy_functions = 0;
    std::vector<std::string> legacy_functions_found;
    std::map<std::string, std::vector<std::string>> legacy_function_patterns;

    // Legacy class metrics
    int total_legacy_classes = 0;
    std::vector<std::string> legacy_classes_found;
    std::map<std::string, std::vector<std::string>> legacy_class_patterns;

    // Unified module adoption metrics
    std::vector<std::string> required_modules;
    std::vector<std::string> adopted_modules;
    std::vector<std::string> non_adopted_modules;
    double unified_module_adoption_rate = 0.0;

    // Deprecated API usage metrics
    int total_deprecated_api_usage = 0;
    std::vector<std::string> deprecated_usage_found;
    std::map<std::string, int> deprecated_api_counts;

    // Legacy code percentage
    double legacy_code_percentage = 0.0;
    int total_code_blocks = 0;
    int legacy_code_blocks = 0;

    // Migration completeness metrics
    std::map<std::string, bool> completeness_status;
    double overall_completeness_percentage = 0.0;
    std::vector<std::string> incomplete_aspects;

    // Legacy references metrics
    std::vector<std::string> legacy_references_found;
    std::map<std::string, std::vector<std::string>> legacy_reference_types;

    // Migration quality assessment
    std::map<std::string, double> quality_metrics;
    double overall_quality_score = 0.0;
    std::vector<std::string> quality_issues;

    // Constitutional compliance
    bool meets_legacy_elimination_requirements = false;
    bool meets_unified_module_adoption_requirements = false;
    bool meets_deprecated_api_removal_requirements = false;
    bool meets_static_configuration_requirements = false;
};

// Migration violation information
struct MigrationViolation {
    std::string category;
    std::string description;
    std::string file_location;
    int severity;
    std::string recommendation;
    std::string violation_type;  // legacy, deprecated, incomplete, quality
};

// Legacy removal validation framework
class LegacyRemovalFramework {
public:
    LegacyRemovalFramework();
    ~LegacyRemovalFramework();

    // Framework lifecycle
    bool initialize();
    bool shutdown();

    // Core validation methods
    bool scanForLegacyFiles(std::vector<std::string>& legacy_files);
    bool scanForLegacyFunctions(std::vector<std::string>& legacy_functions);
    bool scanForLegacyClasses(std::vector<std::string>& legacy_classes);
    bool validateUnifiedModuleAdoption(std::vector<std::string>& non_adopted_modules);
    bool checkDeprecatedAPIUsage(std::vector<std::string>& deprecated_usage);
    bool calculateLegacyCodePercentage(double& legacy_percentage);
    bool validateMigrationCompleteness(std::map<std::string, bool>& completeness_status);
    bool generateMigrationReport(std::string& report);
    bool verifyNoLegacyReferences(std::vector<std::string>& legacy_references);
    bool assessMigrationQuality(std::map<std::string, double>& quality_metrics);

    // Advanced analysis methods
    bool analyzeLegacyPatterns(std::map<std::string, std::vector<std::string>>& patterns);
    bool validateModuleReplacement(std::map<std::string, std::string>& replacement_map);
    bool checkAPIConsistency(std::vector<std::string>& consistency_issues);
    bool analyzeMigrationImpact(std::map<std::string, double>& impact_metrics);

    // Comprehensive validation
    bool validateAllMigrationRequirements(std::vector<MigrationViolation>& violations);
    bool generateDetailedReport(nlohmann::json& report);

    // Configuration
    void setCodebaseRoot(const std::string& root_path);
    void setMigrationRequirements(const std::map<std::string, double>& requirements);
    void setRequiredModules(const std::vector<std::string>& modules);
    void enableVerboseLogging(bool enable) { verbose_logging_ = enable; }

private:
    // Internal analysis methods
    std::vector<std::string> scanAllFiles(const std::string& extension = "");
    std::vector<std::string> scanSourceFiles();
    std::vector<std::string> scanHeaderFiles();

    // Legacy detection methods
    std::vector<std::string> detectLegacyFilesByPattern();
    std::vector<std::string> detectLegacyFunctionsByPattern();
    std::vector<std::string> detectLegacyClassesByPattern();
    std::vector<std::string> detectLegacyContent(const std::string& file_path);

    // Pattern matching methods
    bool matchesLegacyPattern(const std::string& content, const std::vector<std::regex>& patterns);
    std::vector<std::regex> getLegacyFilePatterns();
    std::vector<std::regex> getLegacyFunctionPatterns();
    std::vector<std::regex> getLegacyClassPatterns();

    // Module adoption analysis
    std::vector<std::string> getRequiredUnifiedModules();
    std::vector<std::string> detectModuleUsage(const std::string& module_name);
    bool isModuleFullyAdopted(const std::string& module_name);
    double calculateModuleAdoptionRate();

    // Deprecated API detection
    std::vector<std::string> getDeprecatedAPIList();
    std::vector<std::string> detectDeprecatedAPIUsage(const std::string& file_path);
    int countDeprecatedAPIUsage(const std::string& content);

    // Legacy code percentage calculation
    int analyzeCodeBlocks(const std::string& file_path);
    int countLegacyCodeBlocks(const std::vector<std::string>& files);
    double calculateLegacyPercentage(int legacy_blocks, int total_blocks);

    // Migration completeness validation
    std::vector<std::string> getMigrationAspects();
    bool validateAspect(const std::string& aspect);
    double calculateCompletenessPercentage();

    // Legacy reference analysis
    std::vector<std::string> findLegacyIncludes(const std::string& file_path);
    std::vector<std::string> findLegacyFunctionCalls(const std::string& file_path);
    std::vector<std::string> findLegacyTypeUsage(const std::string& file_path);

    // Quality assessment methods
    double assessCodeConsistency();
    double assessArchitecturalCompliance();
    double assessTestMigration();
    double assessDocumentationCompleteness();
    double assessBuildSystemUpdate();

    // Report generation
    void generateLegacyAnalysisReport(nlohmann::json& report, const LegacyRemovalMetrics& metrics);
    void generateModuleAdoptionReport(nlohmann::json& report, const LegacyRemovalMetrics& metrics);
    void generateQualityAssessmentReport(nlohmann::json& report, const LegacyRemovalMetrics& metrics);
    void generateMigrationCompletenessReport(nlohmann::json& report, const LegacyRemovalMetrics& metrics);

    // Configuration and state
    std::string codebase_root_;
    std::map<std::string, double> migration_requirements_;
    std::vector<std::string> required_modules_;
    bool verbose_logging_;
    bool initialized_;

    // Analysis cache
    std::vector<std::string> source_files_;
    std::vector<std::string> header_files_;
    LegacyRemovalMetrics last_metrics_;

    // Legacy removal constants (from constitutional v5.5)
    static constexpr double DEFAULT_MAX_LEGACY_FILES = 0.0;
    static constexpr double DEFAULT_MAX_LEGACY_FUNCTIONS = 0.0;
    static constexpr double DEFAULT_MAX_LEGACY_CLASSES = 0.0;
    static constexpr double DEFAULT_MAX_LEGACY_CODE_PERCENTAGE = 0.0;
    static constexpr double DEFAULT_MIN_UNIFIED_MODULE_USAGE = 95.0;
    static constexpr double DEFAULT_MAX_DEPRECATED_API_USAGE = 0.0;
    static constexpr double DEFAULT_MIN_QUALITY_SCORE = 8.5;
    static constexpr double DEFAULT_MIN_COMPLETENESS_PERCENTAGE = 100.0;

    // Legacy patterns
    const std::vector<std::string> LEGACY_FILE_PATTERNS = {
        "legacy_", "_old", "_deprecated", "backup_", "original_", "temp_", "test_"
    };

    const std::vector<std::string> LEGACY_FUNCTION_PATTERNS = {
        "legacy_", "_old_", "deprecated_", "temp_func", "backup_", "original_"
    };

    const std::vector<std::string> LEGACY_CLASS_PATTERNS = {
        "Legacy", "Old", "Deprecated", "Temp", "Backup", "Original"
    };

    const std::vector<std::string> DEPRECATED_API_PATTERNS = {
        "old_", "legacy_", "deprecated_", "temp_", "backup_"
    };

    // Utility methods
    void logVerbose(const std::string& message);
    std::string getCurrentTimestamp();
    std::vector<std::string> splitLines(const std::string& content);
    std::string trimWhitespace(const std::string& str);
    bool isCommentLine(const std::string& line);
    std::string extractFileName(const std::string& file_path);
    std::string extractFunctionName(const std::string& line);
    std::string extractClassName(const std::string& line);
};

} // namespace migration
} // namespace keyhunt
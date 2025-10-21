// Puzzle71 Technical Debt Repair - Architectural Compliance Validation Framework
// User Story 3: Complete System Migration and Quality Assurance
// TDD Implementation: This framework makes T065 architectural compliance tests pass (GREEN phase)

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
namespace architecture {

// Architectural compliance metrics and thresholds
struct ArchitecturalMetrics {
    double code_duplication_percentage = 0.0;
    int duplicated_blocks = 0;
    int legacy_blocks_remaining = 0;
    std::vector<std::string> legacy_files;
    std::vector<std::string> missing_modules;
    std::map<std::string, int> layer_violations;
    std::vector<std::string> naming_violations;
    std::vector<std::string> dependency_violations;
    std::vector<std::string> pattern_violations;
    std::map<std::string, double> code_organization_metrics;
    double compliance_score = 0.0;
    double static_configuration_queries = 0.0;
    double deterministic_operations = 0.0;
    double runtime_overhead = 0.0;
};

// Compliance violation information
struct ComplianceViolation {
    std::string category;
    std::string description;
    std::string file_location;
    int severity;
    std::string recommendation;
};

// Architectural compliance validation framework
class ArchitecturalComplianceFramework {
public:
    ArchitecturalComplianceFramework();
    ~ArchitecturalComplianceFramework();

    // Framework lifecycle
    bool initialize();
    bool shutdown();

    // Core validation methods
    bool validateCodeDuplication(double& duplication_percentage, int& duplicated_blocks);
    bool validateLegacyCodeRemoval(int& legacy_blocks_remaining, std::vector<std::string>& legacy_files);
    bool validateUnifiedModuleUsage(std::vector<std::string>& missing_modules);
    bool validateArchitecturalLayers(std::map<std::string, int>& layer_violations);
    bool validateNamingConventions(std::vector<std::string>& naming_violations);
    bool validateModuleDependencies(std::vector<std::string>& dependency_violations);
    bool validateConformanceToDesignPatterns(std::vector<std::string>& pattern_violations);
    bool checkCodeOrganizationMetrics(std::map<std::string, double>& metrics);

    // Compliance scoring and reporting
    bool calculateArchitecturalComplianceScore(double& compliance_score);
    bool generateArchitecturalReport(std::string& report);
    bool generateDetailedReport(nlohmann::json& report);

    // Batch validation
    bool validateAllConstraints(std::vector<ComplianceViolation>& violations);

    // Configuration
    void setCodebaseRoot(const std::string& root_path);
    void setThresholds(const std::map<std::string, double>& thresholds);
    void enableVerboseLogging(bool enable) { verbose_logging_ = enable; }

private:
    // Internal analysis methods
    std::vector<std::string> scanSourceFiles(const std::string& extension = ".cpp");
    std::vector<std::string> scanHeaderFiles(const std::string& extension = ".hpp");

    // Code duplication analysis
    struct CodeBlock {
        std::string content;
        std::string file_path;
        int line_number;
        std::string hash;
    };

    std::vector<CodeBlock> extractCodeBlocks(const std::string& file_path);
    std::string generateBlockHash(const std::string& content);
    std::map<std::string, std::vector<CodeBlock>> findDuplicateBlocks(const std::vector<CodeBlock>& blocks);
    double calculateDuplicationPercentage(int total_blocks, int duplicated_blocks);

    // Legacy code detection
    std::vector<std::string> detectLegacyFiles();
    std::vector<std::string> detectLegacyFunctions(const std::string& file_path);
    std::vector<std::string> detectLegacyClasses(const std::string& file_path);
    bool isLegacyPattern(const std::string& content);

    // Module usage analysis
    std::vector<std::string> getRequiredModules();
    std::vector<std::string> findMissingModules(const std::vector<std::string>& required_modules);
    bool isModuleUsed(const std::string& module_name);

    // Architectural layer validation
    enum class ArchitecturalLayer {
        PRESENTATION,
        BUSINESS,
        DATA_ACCESS,
        INFRASTRUCTURE
    };

    ArchitecturalLayer identifyLayer(const std::string& file_path);
    std::map<std::string, int> analyzeLayerViolations();

    // Naming convention validation
    std::vector<std::string> analyzeNamingViolations(const std::string& file_path);
    bool isValidSnakeCase(const std::string& name);
    bool isValidPascalCase(const std::string& name);
    bool isValidCamelCase(const std::string& name);

    // Dependency analysis
    struct Dependency {
        std::string from_file;
        std::string to_file;
        std::string dependency_type;
    };

    std::vector<Dependency> analyzeDependencies();
    std::vector<std::string> detectCircularDependencies(const std::vector<Dependency>& dependencies);
    std::vector<std::string> detectMissingDependencies(const std::vector<Dependency>& dependencies);
    std::vector<std::string> detectInvalidDependencies(const std::vector<Dependency>& dependencies);

    // Design pattern validation
    std::vector<std::string> validateAdapterPattern();
    std::vector<std::string> validateSingletonPattern();
    std::vector<std::string> validateFactoryPattern();
    std::vector<std::string> validateObserverPattern();

    // Code organization metrics
    double calculateCyclomaticComplexity(const std::string& file_path);
    double calculateCoupling(const std::string& file_path);
    double calculateCohesion(const std::string& file_path);
    std::map<std::string, double> analyzeCodeOrganization();

    // Configuration and state
    std::string codebase_root_;
    std::map<std::string, double> thresholds_;
    bool verbose_logging_;
    bool initialized_;

    // Analysis cache
    std::vector<std::string> source_files_;
    std::vector<std::string> header_files_;
    std::map<std::string, std::vector<CodeBlock>> file_code_blocks_;
    std::map<std::string, std::vector<Dependency>> file_dependencies_;

    // Compliance constants (from constitutional v5.5)
    static constexpr double DEFAULT_CODE_DUPLICATION_THRESHOLD = 5.0;
    static constexpr int DEFAULT_MAX_LEGACY_BLOCKS = 0;
    static constexpr double DEFAULT_ARCHITECTURAL_COMPLIANCE_SCORE = 95.0;
    static constexpr int DEFAULT_MAX_NAMING_VIOLATIONS = 5;
    static constexpr int DEFAULT_MAX_DEPENDENCY_VIOLATIONS = 3;

    // Utility methods
    void logVerbose(const std::string& message);
    std::string getCurrentTimestamp();
    std::vector<std::string> splitLines(const std::string& content);
    std::string trimWhitespace(const std::string& str);
    bool isCommentLine(const std::string& line);
};

} // namespace architecture
} // namespace keyhunt
// Puzzle71Solver - Configuration Compatibility Layer (T057)
// Phase 7: User Story 5 - Compatibility Assurance
// Comprehensive configuration compatibility and migration system for legacy config files

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>
#include <optional>
#include <functional>

namespace puzzle71::compatibility {

/**
 * @brief Configuration format versions
 */
enum class ConfigFormat {
    UNKNOWN = 0,
    LEGACY_V1 = 1,      // Original text-based config format
    LEGACY_V2 = 2,      // Enhanced text format with sections
    JSON_V1 = 3,        // First JSON format
    JSON_V2 = 4,        // Current JSON format with validation
    YAML_V1 = 5,        // YAML format support
    TOML_V1 = 6         // TOML format support
};

/**
 * @brief Configuration validation result
 */
struct ValidationResult {
    bool is_valid{false};
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::vector<std::string> suggestions;
    ConfigFormat detected_format{ConfigFormat::UNKNOWN};
    std::string format_description;

    /**
     * @brief Check if validation passed
     */
    bool isValid() const { return is_valid && errors.empty(); }

    /**
     * @brief Get severity level
     */
    std::string getSeverityLevel() const;

    /**
     * @brief Serialize to JSON
     */
    std::string toJson() const;
};

/**
 * @brief Configuration migration result
 */
struct MigrationResult {
    bool success{false};
    ConfigFormat source_format{ConfigFormat::UNKNOWN};
    ConfigFormat target_format{ConfigFormat::UNKNOWN};
    std::string source_file;
    std::string target_file;
    std::vector<std::string> migrations_applied;
    std::vector<std::string> issues_encountered;
    std::map<std::string, std::string> parameter_mappings;
    ValidationResult validation_result;

    /**
     * @brief Check if migration was successful
     */
    bool isSuccessful() const { return success && issues_encountered.empty(); }

    /**
     * @brief Get migration summary
     */
    std::string getSummary() const;

    /**
     * @brief Serialize to JSON
     */
    std::string toJson() const;
};

/**
 * @brief Configuration parameter definition
 */
struct ConfigParameter {
    std::string name;
    std::string description;
    std::variant<bool, int, double, std::string> default_value;
    std::variant<bool, int, double, std::string> current_value;
    std::string section;
    bool is_required{false};
    bool is_deprecated{false};
    std::string deprecated_version;
    std::string replacement_parameter;
    std::vector<std::string> valid_values;
    std::function<bool(const std::variant<bool, int, double, std::string>&)> validator;

    /**
     * @brief Check if parameter has valid value
     */
    bool isValid() const;

    /**
     * @brief Get parameter value as string
     */
    std::string getValueAsString() const;

    /**
     * @brief Set parameter value from string
     */
    bool setValueFromString(const std::string& value);

    /**
     * @brief Serialize to JSON
     */
    std::string toJson() const;
};

/**
 * @brief Configuration section definition
 */
struct ConfigSection {
    std::string name;
    std::string description;
    std::vector<ConfigParameter> parameters;
    bool is_required{false};
    bool is_deprecated{false};
    std::string deprecated_version;
    std::string replacement_section;

    /**
     * @brief Find parameter by name
     */
    std::optional<ConfigParameter> findParameter(const std::string& name) const;

    /**
     * @brief Add parameter
     */
    void addParameter(const ConfigParameter& parameter);

    /**
     * @brief Serialize to JSON
     */
    std::string toJson() const;
};

/**
 * @brief Legacy configuration parser
 */
class LegacyConfigParser {
public:
    LegacyConfigParser();
    ~LegacyConfigParser() = default;

    /**
     * @brief Parse legacy text configuration
     */
    struct ParseResult {
        bool success{false};
        std::map<std::string, ConfigSection> sections;
        std::vector<std::string> parse_errors;
        std::vector<std::string> parse_warnings;
        ConfigFormat detected_format{ConfigFormat::UNKNOWN};
    };

    ParseResult parseLegacyConfig(const std::string& config_file);
    ParseResult parseLegacyV1Config(const std::string& config_content);
    ParseResult parseLegacyV2Config(const std::string& config_content);

    /**
     * @brief Detect configuration format
     */
    ConfigFormat detectConfigFormat(const std::string& config_file);
    ConfigFormat detectConfigFormatFromContent(const std::string& content);

private:
    std::map<std::string, std::string> legacy_parameter_mappings_;
    std::vector<std::string> known_sections_;
    std::vector<std::string> known_parameters_;

    void initializeLegacyMappings();
    ParseResult parseKeyValuePairs(const std::string& content);
    ParseResult parseSectionedFormat(const std::string& content);
    std::string normalizeParameterName(const std::string& name);
    std::string extractSectionName(const std::string& line);
    bool isCommentLine(const std::string& line);
    bool isEmptyLine(const std::string& line);
};

/**
 * @brief Modern configuration parser (JSON/YAML/TOML)
 */
class ModernConfigParser {
public:
    ModernConfigParser();
    ~ModernConfigParser() = default;

    /**
     * @brief Parse modern configuration formats
     */
    struct ParseResult {
        bool success{false};
        nlohmann::json config_data;
        std::vector<std::string> parse_errors;
        std::vector<std::string> parse_warnings;
        ConfigFormat detected_format{ConfigFormat::UNKNOWN};
    };

    ParseResult parseJsonConfig(const std::string& config_file);
    ParseResult parseYamlConfig(const std::string& config_file);
    ParseResult parseTomlConfig(const std::string& config_file);
    ParseResult parseModernConfig(const std::string& config_file);

    /**
     * @brief Convert JSON to config sections
     */
    std::map<std::string, ConfigSection> jsonToSections(const nlohmann::json& json_data);

private:
    bool validateJsonStructure(const nlohmann::json& json_data);
    ConfigSection jsonToSection(const std::string& section_name, const nlohmann::json& section_data);
    ConfigParameter jsonToParameter(const std::string& name, const nlohmann::json& param_data);
};

/**
 * @brief Configuration migrator
 */
class ConfigMigrator {
public:
    ConfigMigrator();
    ~ConfigMigrator() = default;

    /**
     * @brief Migrate configuration between formats
     */
    MigrationResult migrateConfiguration(
        const std::string& source_file,
        const std::string& target_file,
        ConfigFormat target_format = ConfigFormat::JSON_V2
    );

    /**
     * @brief Migrate legacy to modern format
     */
    MigrationResult migrateLegacyToModern(
        const std::string& legacy_file,
        const std::string& modern_file
    );

    /**
     * @brief Upgrade configuration version
     */
    MigrationResult upgradeConfiguration(
        const std::string& config_file,
        ConfigFormat target_format
    );

    /**
     * @brief Validate migration compatibility
     */
    ValidationResult validateMigrationCompatibility(
        ConfigFormat source_format,
        ConfigFormat target_format
    );

    /**
     * @brief Generate migration plan
     */
    struct MigrationPlan {
        std::vector<std::string> steps;
        std::vector<std::string> required_changes;
        std::vector<std::string> potential_issues;
        std::map<std::string, std::string> parameter_renames;
        std::vector<std::string> deprecated_parameters;
        bool requires_manual_intervention{false};
    };

    MigrationPlan generateMigrationPlan(
        ConfigFormat source_format,
        ConfigFormat target_format,
        const std::map<std::string, ConfigSection>& source_sections
    );

private:
    LegacyConfigParser legacy_parser_;
    ModernConfigParser modern_parser_;

    void initializeMigrationRules();
    MigrationResult applyMigrationRules(
        const std::map<std::string, ConfigSection>& source_sections,
        ConfigFormat target_format
    );

    std::string sectionsToJson(const std::map<std::string, ConfigSection>& sections);
    std::string sectionsToYaml(const std::map<std::string, ConfigSection>& sections);
    std::string sectionsToToml(const std::map<std::string, ConfigSection>& sections);

    bool validateTargetFormat(ConfigFormat format);
    std::string getDefaultExtension(ConfigFormat format);
};

/**
 * @brief Configuration validator
 */
class ConfigValidator {
public:
    ConfigValidator();
    ~ConfigValidator() = default;

    /**
     * @brief Validate configuration format
     */
    ValidationResult validateConfigFormat(const std::string& config_file);

    /**
     * @brief Validate configuration content
     */
    ValidationResult validateConfigContent(
        const std::map<std::string, ConfigSection>& sections,
        ConfigFormat format = ConfigFormat::UNKNOWN
    );

    /**
     * @brief Validate configuration schema
     */
    ValidationResult validateConfigSchema(
        const nlohmann::json& config_data,
        const std::string& schema_file = ""
    );

    /**
     * @brief Validate parameter values
     */
    ValidationResult validateParameterValues(
        const std::map<std::string, ConfigSection>& sections
    );

    /**
     * @brief Check for deprecated parameters
     */
    ValidationResult checkDeprecatedParameters(
        const std::map<std::string, ConfigSection>& sections
    );

    /**
     * @brief Validate required parameters
     */
    ValidationResult validateRequiredParameters(
        const std::map<std::string, ConfigSection>& sections
    );

    /**
     * @brief Generate validation report
     */
    std::string generateValidationReport(const ValidationResult& result);

private:
    std::map<std::string, ConfigParameter> known_parameters_;
    std::map<std::string, ConfigSection> known_sections_;
    std::map<std::string, std::function<bool(const std::string&)>> validators_;

    void initializeKnownParameters();
    void initializeValidators();
    bool isValidParameterValue(const ConfigParameter& param);
    std::vector<std::string> getMissingRequiredParameters(
        const std::map<std::string, ConfigSection>& sections
    );
};

/**
 * @brief Configuration compatibility manager
 */
class ConfigCompatibilityManager {
public:
    ConfigCompatibilityManager();
    ~ConfigCompatibilityManager() = default;

    /**
     * @brief Initialize compatibility system
     */
    bool initialize();

    /**
     * @brief Load configuration with automatic format detection
     */
    struct LoadResult {
        bool success{false};
        std::map<std::string, ConfigSection> sections;
        ConfigFormat detected_format{ConfigFormat::UNKNOWN};
        ValidationResult validation_result;
        MigrationResult migration_result;
        bool was_migrated{false};
    };

    LoadResult loadConfiguration(const std::string& config_file);

    /**
     * @brief Save configuration in specified format
     */
    bool saveConfiguration(
        const std::map<std::string, ConfigSection>& sections,
        const std::string& output_file,
        ConfigFormat format = ConfigFormat::JSON_V2
    );

    /**
     * @brief Migrate configuration file
     */
    MigrationResult migrateConfigurationFile(
        const std::string& source_file,
        const std::string& target_file,
        ConfigFormat target_format = ConfigFormat::JSON_V2
    );

    /**
     * @brief Validate configuration file
     */
    ValidationResult validateConfigurationFile(const std::string& config_file);

    /**
     * @brief Check configuration compatibility
     */
    struct CompatibilityReport {
        bool is_compatible{false};
        ConfigFormat current_format{ConfigFormat::UNKNOWN};
        ConfigFormat recommended_format{ConfigFormat::JSON_V2};
        std::vector<std::string> compatibility_issues;
        std::vector<std::string> migration_suggestions;
        ValidationResult validation_result;
        MigrationPlan migration_plan;
    };

    CompatibilityReport checkCompatibility(const std::string& config_file);

    /**
     * @brief Auto-migrate configuration
     */
    MigrationResult autoMigrateConfiguration(
        const std::string& config_file,
        bool create_backup = true
    );

    /**
     * @brief Generate configuration template
     */
    bool generateConfigurationTemplate(
        const std::string& template_file,
        ConfigFormat format = ConfigFormat::JSON_V2
    );

    /**
     * @brief Convert configuration between formats
     */
    bool convertConfigurationFormat(
        const std::string& input_file,
        const std::string& output_file,
        ConfigFormat target_format
    );

    /**
     * @brief Get supported formats
     */
    std::vector<ConfigFormat> getSupportedFormats() const;

    /**
     * @brief Get format description
     */
    std::string getFormatDescription(ConfigFormat format) const;

    /**
     * @brief Get format from file extension
     */
    ConfigFormat getFormatFromExtension(const std::string& file_extension) const;

    /**
     * @brief Get recommended format
     */
    ConfigFormat getRecommendedFormat() const { return ConfigFormat::JSON_V2; }

private:
    LegacyConfigParser legacy_parser_;
    ModernConfigParser modern_parser_;
    ConfigMigrator migrator_;
    ConfigValidator validator_;

    bool initialized_{false};
    std::map<ConfigFormat, std::string> format_extensions_;
    std::map<ConfigFormat, std::string> format_descriptions_;

    void initializeFormatMappings();
    std::string createBackupFile(const std::string& original_file);
    bool isValidConfigFile(const std::string& config_file);
    std::string detectConfigFileFormat(const std::string& config_file);
};

/**
 * @brief Configuration utilities
 */
namespace config_utils {

/**
 * @brief Convert format enum to string
 */
std::string formatToString(ConfigFormat format);

/**
 * @brief Convert string to format enum
 */
ConfigFormat stringToFormat(const std::string& format_str);

/**
 * @brief Get file extension for format
 */
std::string getExtensionForFormat(ConfigFormat format);

/**
 * @brief Merge configuration sections
 */
std::map<std::string, ConfigSection> mergeConfigSections(
    const std::map<std::string, ConfigSection>& base,
    const std::map<std::string, ConfigSection>& overlay,
    bool overwrite_existing = true
);

/**
 * @brief Compare configuration sections
 */
struct ConfigDiff {
    std::vector<std::string> added_parameters;
    std::vector<std::string> removed_parameters;
    std::vector<std::string> modified_parameters;
    std::vector<std::string> added_sections;
    std::vector<std::string> removed_sections;
};

ConfigDiff compareConfigSections(
    const std::map<std::string, ConfigSection>& old_config,
    const std::map<std::string, ConfigSection>& new_config
);

/**
 * @brief Validate configuration file path
 */
bool isValidConfigPath(const std::string& config_path);

/**
 * @brief Create configuration backup
 */
bool createConfigurationBackup(const std::string& config_file, std::string& backup_path);

/**
 * @brief Restore configuration from backup
 */
bool restoreConfigurationFromBackup(const std::string& backup_file, const std::string& target_file);

/**
 * @brief Normalize configuration value
 */
std::string normalizeConfigValue(const std::string& value, const std::string& parameter_type);

/**
 * @brief Escape configuration value
 */
std::string escapeConfigValue(const std::string& value, ConfigFormat format);

/**
 * @brief Parse boolean value
 */
bool parseBoolValue(const std::string& value, bool default_value = false);

/**
 * @brief Parse integer value
 */
int parseIntValue(const std::string& value, int default_value = 0);

/**
 * @brief Parse double value
 */
double parseDoubleValue(const std::string& value, double default_value = 0.0);

} // namespace config_utils

/**
 * @brief Global configuration compatibility manager
 */
class ConfigCompatibilitySystem {
public:
    static ConfigCompatibilitySystem& getInstance();

    /**
     * @brief Initialize system
     */
    bool initialize();

    /**
     * @brief Get compatibility manager
     */
    std::shared_ptr<ConfigCompatibilityManager> getCompatibilityManager();

    /**
     * @brief Quick load configuration
     */
    ConfigCompatibilityManager::LoadResult quickLoadConfig(const std::string& config_file);

    /**
     * @brief Quick validate configuration
     */
    ValidationResult quickValidateConfig(const std::string& config_file);

    /**
     * @brief Quick migrate configuration
     */
    MigrationResult quickMigrateConfig(
        const std::string& config_file,
        ConfigFormat target_format = ConfigFormat::JSON_V2
    );

    /**
     * @brief Generate system report
     */
    std::string generateSystemReport();

private:
    ConfigCompatibilitySystem() = default;
    ~ConfigCompatibilitySystem() = default;

    std::shared_ptr<ConfigCompatibilityManager> manager_;
    bool initialized_{false};
};

} // namespace puzzle71::compatibility
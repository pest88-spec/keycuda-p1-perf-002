// Puzzle71Solver - Configuration Compatibility Layer Implementation (T057)
// Phase 7: User Story 5 - Compatibility Assurance
// Comprehensive configuration compatibility and migration system for legacy config files

#include "config_compatibility.h"
#include "../utils/logger.h"
#include "../utils/file_io.h"
#include "../utils/json_serializer.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <regex>
#include <iomanip>
#include <ctime>

// YAML and TOML support (optional)
#ifdef PUZZLE71_HAS_YAML
#include <yaml-cpp/yaml.h>
#endif

#ifdef PUZZLE71_HAS_TOML
#include <toml.hpp>
#endif

namespace puzzle71::compatibility {

// ============================================================================
// ValidationResult Implementation
// ============================================================================

std::string ValidationResult::getSeverityLevel() const {
    if (!errors.empty()) return "error";
    if (!warnings.empty()) return "warning";
    if (!suggestions.empty()) return "info";
    return "success";
}

std::string ValidationResult::toJson() const {
    nlohmann::json j;
    j["is_valid"] = is_valid;
    j["errors"] = errors;
    j["warnings"] = warnings;
    j["suggestions"] = suggestions;
    j["detected_format"] = static_cast<int>(detected_format);
    j["format_description"] = format_description;
    j["severity_level"] = getSeverityLevel();
    return j.dump(4);
}

// ============================================================================
// MigrationResult Implementation
// ============================================================================

std::string MigrationResult::getSummary() const {
    std::ostringstream oss;
    oss << "Migration Summary:\n";
    oss << "  Success: " << (success ? "Yes" : "No") << "\n";
    oss << "  Source Format: " << static_cast<int>(source_format) << "\n";
    oss << "  Target Format: " << static_cast<int>(target_format) << "\n";
    oss << "  Source File: " << source_file << "\n";
    oss << "  Target File: " << target_file << "\n";
    oss << "  Migrations Applied: " << migrations_applied.size() << "\n";
    oss << "  Issues Encountered: " << issues_encountered.size() << "\n";
    return oss.str();
}

std::string MigrationResult::toJson() const {
    nlohmann::json j;
    j["success"] = success;
    j["source_format"] = static_cast<int>(source_format);
    j["target_format"] = static_cast<int>(target_format);
    j["source_file"] = source_file;
    j["target_file"] = target_file;
    j["migrations_applied"] = migrations_applied;
    j["issues_encountered"] = issues_encountered;
    j["parameter_mappings"] = parameter_mappings;
    j["validation_result"] = nlohmann::json::parse(validation_result.toJson());
    return j.dump(4);
}

// ============================================================================
// ConfigParameter Implementation
// ============================================================================

bool ConfigParameter::isValid() const {
    if (validator) {
        return validator(current_value);
    }

    // Default validation
    if (is_required) {
        if (std::holds_alternative<std::string>(current_value)) {
            return !std::get<std::string>(current_value).empty();
        }
        return true;
    }

    return true;
}

std::string ConfigParameter::getValueAsString() const {
    if (std::holds_alternative<bool>(current_value)) {
        return std::get<bool>(current_value) ? "true" : "false";
    } else if (std::holds_alternative<int>(current_value)) {
        return std::to_string(std::get<int>(current_value));
    } else if (std::holds_alternative<double>(current_value)) {
        return std::to_string(std::get<double>(current_value));
    } else if (std::holds_alternative<std::string>(current_value)) {
        return std::get<std::string>(current_value);
    }
    return "";
}

bool ConfigParameter::setValueFromString(const std::string& value) {
    try {
        if (std::holds_alternative<bool>(default_value)) {
            current_value = config_utils::parseBoolValue(value);
        } else if (std::holds_alternative<int>(default_value)) {
            current_value = std::stoi(value);
        } else if (std::holds_alternative<double>(default_value)) {
            current_value = std::stod(value);
        } else {
            current_value = value;
        }
        return true;
    } catch (...) {
        return false;
    }
}

std::string ConfigParameter::toJson() const {
    nlohmann::json j;
    j["name"] = name;
    j["description"] = description;
    j["section"] = section;
    j["is_required"] = is_required;
    j["is_deprecated"] = is_deprecated;
    j["deprecated_version"] = deprecated_version;
    j["replacement_parameter"] = replacement_parameter;
    j["valid_values"] = valid_values;
    j["current_value"] = getValueAsString();
    j["is_valid"] = isValid();
    return j.dump(4);
}

// ============================================================================
// ConfigSection Implementation
// ============================================================================

std::optional<ConfigParameter> ConfigSection::findParameter(const std::string& name) const {
    auto it = std::find_if(parameters.begin(), parameters.end(),
        [&name](const ConfigParameter& param) {
            return param.name == name;
        });

    if (it != parameters.end()) {
        return *it;
    }
    return std::nullopt;
}

void ConfigSection::addParameter(const ConfigParameter& parameter) {
    parameters.push_back(parameter);
}

std::string ConfigSection::toJson() const {
    nlohmann::json j;
    j["name"] = name;
    j["description"] = description;
    j["is_required"] = is_required;
    j["is_deprecated"] = is_deprecated;
    j["deprecated_version"] = deprecated_version;
    j["replacement_section"] = replacement_section;

    j["parameters"] = nlohmann::json::array();
    for (const auto& param : parameters) {
        j["parameters"].push_back(nlohmann::json::parse(param.toJson()));
    }

    return j.dump(4);
}

// ============================================================================
// LegacyConfigParser Implementation
// ============================================================================

LegacyConfigParser::LegacyConfigParser() {
    initializeLegacyMappings();
}

void LegacyConfigParser::initializeLegacyMappings() {
    // Initialize known parameter mappings for legacy configurations
    legacy_parameter_mappings_ = {
        {"threads", "gpu_threads"},
        {"blocks", "gpu_blocks"},
        {"device", "gpu_device_id"},
        {"memory", "gpu_memory_size"},
        {"batch", "batch_size"},
        {"range", "key_range"},
        {"targets", "target_addresses"},
        {"output", "output_file"},
        {"verbose", "verbosity_level"},
        {"debug", "debug_mode"},
        {"seed", "random_seed"},
        {"checkpoints", "enable_checkpoints"},
        {"checkpoint_file", "checkpoint_path"},
        {"timeout", "operation_timeout"},
        {"retries", "max_retries"}
    };

    known_sections_ = {
        "gpu", "cpu", "memory", "ecc", "scan", "compare",
        "output", "logging", "debugging", "performance", "network"
    };

    known_parameters_ = {
        "gpu_threads", "gpu_blocks", "gpu_device_id", "gpu_memory_size",
        "batch_size", "key_range", "target_addresses", "output_file",
        "verbosity_level", "debug_mode", "random_seed", "enable_checkpoints",
        "checkpoint_path", "operation_timeout", "max_retries"
    };
}

LegacyConfigParser::ParseResult LegacyConfigParser::parseLegacyConfig(const std::string& config_file) {
    ParseResult result;

    if (!std::filesystem::exists(config_file)) {
        result.parse_errors.push_back("Configuration file not found: " + config_file);
        return result;
    }

    try {
        std::ifstream file(config_file);
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        result.detected_format = detectConfigFormatFromContent(content);

        switch (result.detected_format) {
            case ConfigFormat::LEGACY_V1:
                result = parseLegacyV1Config(content);
                break;
            case ConfigFormat::LEGACY_V2:
                result = parseLegacyV2Config(content);
                break;
            default:
                result.parse_errors.push_back("Unknown or unsupported legacy configuration format");
                break;
        }

        if (result.success) {
            LOG_INFO("Successfully parsed legacy configuration: " << config_file);
            LOG_INFO("Detected format: " << static_cast<int>(result.detected_format));
        }

    } catch (const std::exception& e) {
        result.parse_errors.push_back("Error reading configuration file: " + std::string(e.what()));
    }

    return result;
}

ConfigFormat LegacyConfigParser::detectConfigFormat(const std::string& config_file) {
    if (!std::filesystem::exists(config_file)) {
        return ConfigFormat::UNKNOWN;
    }

    try {
        std::ifstream file(config_file);
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        return detectConfigFormatFromContent(content);
    } catch (...) {
        return ConfigFormat::UNKNOWN;
    }
}

ConfigFormat LegacyConfigParser::detectConfigFormatFromContent(const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    bool has_sections = false;
    bool has_json_structure = false;
    int key_value_pairs = 0;
    int lines_with_equals = 0;

    while (std::getline(stream, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || isCommentLine(line)) {
            continue;
        }

        // Check for JSON structure
        if (line.find("{") != std::string::npos || line.find("\"") != std::string::npos) {
            has_json_structure = true;
        }

        // Check for sections (format: [section_name])
        if (line.front() == '[' && line.back() == ']') {
            has_sections = true;
            continue;
        }

        // Check for key=value pairs
        if (line.find('=') != std::string::npos) {
            lines_with_equals++;
            auto equal_pos = line.find('=');
            if (equal_pos > 0 && equal_pos < line.length() - 1) {
                key_value_pairs++;
            }
        }
    }

    // Determine format
    if (has_json_structure) {
        return ConfigFormat::JSON_V1; // Could be JSON_V1 or JSON_V2
    } else if (has_sections) {
        return ConfigFormat::LEGACY_V2; // Sectioned format
    } else if (key_value_pairs > 0) {
        return ConfigFormat::LEGACY_V1; // Simple key=value format
    }

    return ConfigFormat::UNKNOWN;
}

LegacyConfigParser::ParseResult LegacyConfigParser::parseLegacyV1Config(const std::string& content) {
    ParseResult result;
    result.detected_format = ConfigFormat::LEGACY_V1;

    try {
        result.sections = parseKeyValuePairs(content);
        result.success = !result.sections.empty();

        if (!result.success) {
            result.parse_errors.push_back("No valid configuration parameters found");
        }

    } catch (const std::exception& e) {
        result.parse_errors.push_back("Error parsing V1 config: " + std::string(e.what()));
    }

    return result;
}

LegacyConfigParser::ParseResult LegacyConfigParser::parseLegacyV2Config(const std::string& content) {
    ParseResult result;
    result.detected_format = ConfigFormat::LEGACY_V2;

    try {
        result.sections = parseSectionedFormat(content);
        result.success = !result.sections.empty();

        if (!result.success) {
            result.parse_errors.push_back("No valid configuration sections found");
        }

    } catch (const std::exception& e) {
        result.parse_errors.push_back("Error parsing V2 config: " + std::string(e.what()));
    }

    return result;
}

std::map<std::string, ConfigSection> LegacyConfigParser::parseKeyValuePairs(const std::string& content) {
    std::map<std::string, ConfigSection> sections;
    ConfigSection default_section;
    default_section.name = "default";
    default_section.description = "Default configuration parameters";

    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || isCommentLine(line)) {
            continue;
        }

        // Parse key=value pair
        auto equal_pos = line.find('=');
        if (equal_pos != std::string::npos && equal_pos > 0 && equal_pos < line.length() - 1) {
            std::string key = line.substr(0, equal_pos);
            std::string value = line.substr(equal_pos + 1);

            // Trim key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            // Remove quotes if present
            if (value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }

            // Normalize parameter name
            key = normalizeParameterName(key);

            // Create parameter
            ConfigParameter param;
            param.name = key;
            param.section = "default";
            param.setValueFromString(value);

            default_section.addParameter(param);
        } else {
            // Invalid line - add warning
            LOG_WARN("Invalid configuration line: " << line);
        }
    }

    if (!default_section.parameters.empty()) {
        sections["default"] = default_section;
    }

    return sections;
}

std::map<std::string, ConfigSection> LegacyConfigParser::parseSectionedFormat(const std::string& content) {
    std::map<std::string, ConfigSection> sections;
    ConfigSection* current_section = nullptr;

    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || isCommentLine(line)) {
            continue;
        }

        // Check for section header
        if (line.front() == '[' && line.back() == ']') {
            std::string section_name = extractSectionName(line);

            ConfigSection section;
            section.name = section_name;
            sections[section_name] = section;
            current_section = &sections[section_name];
            continue;
        }

        // Parse key=value pair within section
        if (current_section != nullptr) {
            auto equal_pos = line.find('=');
            if (equal_pos != std::string::npos && equal_pos > 0 && equal_pos < line.length() - 1) {
                std::string key = line.substr(0, equal_pos);
                std::string value = line.substr(equal_pos + 1);

                // Trim key and value
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                // Remove quotes if present
                if (value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.length() - 2);
                }

                // Normalize parameter name
                key = normalizeParameterName(key);

                // Create parameter
                ConfigParameter param;
                param.name = key;
                param.section = current_section->name;
                param.setValueFromString(value);

                current_section->addParameter(param);
            }
        }
    }

    return sections;
}

std::string LegacyConfigParser::normalizeParameterName(const std::string& name) {
    // Convert to lowercase
    std::string normalized = name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);

    // Replace known legacy parameters
    auto it = legacy_parameter_mappings_.find(normalized);
    if (it != legacy_parameter_mappings_.end()) {
        return it->second;
    }

    return normalized;
}

std::string LegacyConfigParser::extractSectionName(const std::string& line) {
    // Extract section name from [section_name]
    if (line.front() == '[' && line.back() == ']') {
        return line.substr(1, line.length() - 2);
    }
    return "";
}

bool LegacyConfigParser::isCommentLine(const std::string& line) {
    return line.empty() || line.front() == '#' || line.front() == ';' ||
           (line.length() >= 2 && line.substr(0, 2) == "//");
}

// ============================================================================
// ModernConfigParser Implementation
// ============================================================================

ModernConfigParser::ModernConfigParser() = default;

ModernConfigParser::ParseResult ModernConfigParser::parseJsonConfig(const std::string& config_file) {
    ParseResult result;
    result.detected_format = ConfigFormat::JSON_V1;

    try {
        std::ifstream file(config_file);
        nlohmann::json json_data;
        file >> json_data;
        file.close();

        // Validate JSON structure
        if (!validateJsonStructure(json_data)) {
            result.parse_errors.push_back("Invalid JSON configuration structure");
            return result;
        }

        result.config_data = json_data;
        result.success = true;

        // Try to determine if it's JSON_V2
        if (json_data.contains("version") || json_data.contains("metadata")) {
            result.detected_format = ConfigFormat::JSON_V2;
        }

    } catch (const nlohmann::json::exception& e) {
        result.parse_errors.push_back("JSON parsing error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        result.parse_errors.push_back("Error reading JSON file: " + std::string(e.what()));
    }

    return result;
}

#ifdef PUZZLE71_HAS_YAML
ModernConfigParser::ParseResult ModernConfigParser::parseYamlConfig(const std::string& config_file) {
    ParseResult result;
    result.detected_format = ConfigFormat::YAML_V1;

    try {
        YAML::Node yaml_data = YAML::LoadFile(config_file);

        // Convert YAML to JSON
        result.config_data = nlohmann::json::parse(YAML::Dump(yaml_data));
        result.success = true;

    } catch (const YAML::Exception& e) {
        result.parse_errors.push_back("YAML parsing error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        result.parse_errors.push_back("Error reading YAML file: " + std::string(e.what()));
    }

    return result;
}
#else
ModernConfigParser::ParseResult ModernConfigParser::parseYamlConfig(const std::string& config_file) {
    ParseResult result;
    result.detected_format = ConfigFormat::YAML_V1;
    result.parse_errors.push_back("YAML support not compiled in");
    return result;
}
#endif

#ifdef PUZZLE71_HAS_TOML
ModernConfigParser::ParseResult ModernConfigParser::parseTomlConfig(const std::string& config_file) {
    ParseResult result;
    result.detected_format = ConfigFormat::TOML_V1;

    try {
        auto toml_data = toml::parse_file(config_file);

        // Convert TOML to JSON
        result.config_data = nlohmann::json::parse(toml::to_string(toml_data));
        result.success = true;

    } catch (const toml::parse_error& e) {
        result.parse_errors.push_back("TOML parsing error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        result.parse_errors.push_back("Error reading TOML file: " + std::string(e.what()));
    }

    return result;
}
#else
ModernConfigParser::ParseResult ModernConfigParser::parseTomlConfig(const std::string& config_file) {
    ParseResult result;
    result.detected_format = ConfigFormat::TOML_V1;
    result.parse_errors.push_back("TOML support not compiled in");
    return result;
}
#endif

ModernConfigParser::ParseResult ModernConfigParser::parseModernConfig(const std::string& config_file) {
    std::string extension = std::filesystem::path(config_file).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == ".json") {
        return parseJsonConfig(config_file);
    } else if (extension == ".yaml" || extension == ".yml") {
        return parseYamlConfig(config_file);
    } else if (extension == ".toml") {
        return parseTomlConfig(config_file);
    } else {
        // Try JSON as default
        return parseJsonConfig(config_file);
    }
}

bool ModernConfigParser::validateJsonStructure(const nlohmann::json& json_data) {
    // Basic validation - check if it's an object
    if (!json_data.is_object()) {
        return false;
    }

    // Could add more specific validation rules here
    return true;
}

std::map<std::string, ConfigSection> ModernConfigParser::jsonToSections(const nlohmann::json& json_data) {
    std::map<std::string, ConfigSection> sections;

    if (json_data.is_object()) {
        for (auto& [section_name, section_data] : json_data.items()) {
            if (section_data.is_object()) {
                ConfigSection section = jsonToSection(section_name, section_data);
                sections[section_name] = section;
            }
        }
    }

    return sections;
}

ConfigSection ModernConfigParser::jsonToSection(const std::string& section_name, const nlohmann::json& section_data) {
    ConfigSection section;
    section.name = section_name;

    if (section_data.contains("description")) {
        section.description = section_data["description"];
    }

    if (section_data.contains("parameters") && section_data["parameters"].is_object()) {
        for (auto& [param_name, param_value] : section_data["parameters"].items()) {
            ConfigParameter param = jsonToParameter(param_name, param_value);
            param.section = section_name;
            section.addParameter(param);
        }
    } else {
        // Treat all direct key-value pairs as parameters
        for (auto& [param_name, param_value] : section_data.items()) {
            if (param_name != "description") {
                ConfigParameter param = jsonToParameter(param_name, param_value);
                param.section = section_name;
                section.addParameter(param);
            }
        }
    }

    return section;
}

ConfigParameter ModernConfigParser::jsonToParameter(const std::string& name, const nlohmann::json& param_data) {
    ConfigParameter param;
    param.name = name;

    if (param_data.is_string()) {
        param.current_value = param_data.get<std::string>();
    } else if (param_data.is_number_integer()) {
        param.current_value = param_data.get<int>();
    } else if (param_data.is_number_float()) {
        param.current_value = param_data.get<double>();
    } else if (param_data.is_boolean()) {
        param.current_value = param_data.get<bool>();
    } else if (param_data.is_object()) {
        // Complex parameter with metadata
        if (param_data.contains("value")) {
            if (param_data["value"].is_string()) {
                param.current_value = param_data["value"].get<std::string>();
            } else if (param_data["value"].is_number_integer()) {
                param.current_value = param_data["value"].get<int>();
            } else if (param_data["value"].is_number_float()) {
                param.current_value = param_data["value"].get<double>();
            } else if (param_data["value"].is_boolean()) {
                param.current_value = param_data["value"].get<bool>();
            }
        }

        if (param_data.contains("description")) {
            param.description = param_data["description"];
        }

        if (param_data.contains("default")) {
            if (param_data["default"].is_string()) {
                param.default_value = param_data["default"].get<std::string>();
            } else if (param_data["default"].is_number_integer()) {
                param.default_value = param_data["default"].get<int>();
            } else if (param_data["default"].is_number_float()) {
                param.default_value = param_data["default"].get<double>();
            } else if (param_data["default"].is_boolean()) {
                param.default_value = param_data["default"].get<bool>();
            }
        }

        if (param_data.contains("required")) {
            param.is_required = param_data["required"];
        }

        if (param_data.contains("deprecated")) {
            param.is_deprecated = param_data["deprecated"];
        }

        if (param_data.contains("replacement")) {
            param.replacement_parameter = param_data["replacement"];
        }
    }

    return param;
}

// ============================================================================
// ConfigMigrator Implementation
// ============================================================================

ConfigMigrator::ConfigMigrator() {
    initializeMigrationRules();
}

void ConfigMigrator::initializeMigrationRules() {
    // Migration rules would be initialized here
    // This is a placeholder for the actual implementation
}

ConfigMigrator::MigrationResult ConfigMigrator::migrateConfiguration(
    const std::string& source_file,
    const std::string& target_file,
    ConfigFormat target_format) {

    MigrationResult result;
    result.source_file = source_file;
    result.target_file = target_file;
    result.target_format = target_format;

    try {
        // Detect source format
        result.source_format = legacy_parser_.detectConfigFormat(source_file);

        if (result.source_format == ConfigFormat::UNKNOWN) {
            result.issues_encountered.push_back("Cannot detect source configuration format");
            return result;
        }

        // Parse source configuration
        std::map<std::string, ConfigSection> source_sections;

        if (result.source_format == ConfigFormat::LEGACY_V1 || result.source_format == ConfigFormat::LEGACY_V2) {
            auto parse_result = legacy_parser_.parseLegacyConfig(source_file);
            if (!parse_result.success) {
                result.issues_encountered.insert(result.issues_encountered.end(),
                                               parse_result.parse_errors.begin(),
                                               parse_result.parse_errors.end());
                return result;
            }
            source_sections = parse_result.sections;
        } else {
            // Parse modern format
            auto parse_result = modern_parser_.parseModernConfig(source_file);
            if (!parse_result.success) {
                result.issues_encountered.insert(result.issues_encountered.end(),
                                               parse_result.parse_errors.begin(),
                                               parse_result.parse_errors.end());
                return result;
            }
            source_sections = modern_parser_.jsonToSections(parse_result.config_data);
        }

        // Apply migration rules
        result = applyMigrationRules(source_sections, target_format);

        // Generate target configuration file
        std::string migrated_content;
        switch (target_format) {
            case ConfigFormat::JSON_V2:
                migrated_content = sectionsToJson(source_sections);
                break;
            case ConfigFormat::YAML_V1:
                migrated_content = sectionsToYaml(source_sections);
                break;
            case ConfigFormat::TOML_V1:
                migrated_content = sectionsToToml(source_sections);
                break;
            default:
                result.issues_encountered.push_back("Unsupported target format: " + std::to_string(static_cast<int>(target_format)));
                return result;
        }

        // Write target file
        std::ofstream target_stream(target_file);
        target_stream << migrated_content;
        target_stream.close();

        result.success = true;
        result.migrations_applied.push_back("Migrated from " +
                                           config_utils::formatToString(result.source_format) +
                                           " to " +
                                           config_utils::formatToString(target_format));

    } catch (const std::exception& e) {
        result.issues_encountered.push_back("Migration error: " + std::string(e.what()));
        result.success = false;
    }

    return result;
}

ConfigMigrator::MigrationResult ConfigMigrator::migrateLegacyToModern(
    const std::string& legacy_file,
    const std::string& modern_file) {

    return migrateConfiguration(legacy_file, modern_file, ConfigFormat::JSON_V2);
}

ConfigMigrator::MigrationResult ConfigMigrator::applyMigrationRules(
    const std::map<std::string, ConfigSection>& source_sections,
    ConfigFormat target_format) {

    MigrationResult result;
    result.success = true;

    // Apply parameter migrations
    for (const auto& [section_name, section] : source_sections) {
        for (const auto& param : section.parameters) {
            // Check for deprecated parameters
            if (param.is_deprecated) {
                result.migrations_applied.push_back("Deprecated parameter found: " + param.name);
                if (!param.replacement_parameter.empty()) {
                    result.parameter_mappings[param.name] = param.replacement_parameter;
                    result.migrations_applied.push_back("Parameter mapping: " + param.name + " -> " + param.replacement_parameter);
                }
            }

            // Check for parameter value migrations
            // This would contain specific migration logic
        }
    }

    return result;
}

std::string ConfigMigrator::sectionsToJson(const std::map<std::string, ConfigSection>& sections) {
    nlohmann::json json_data;

    // Add metadata
    json_data["metadata"] = {
        {"version", "2.0"},
        {"format", "json"},
        {"generated_at", std::put_time(std::gmtime(&std::time_t{}), "%Y-%m-%dT%H:%M:%SZ")},
        {"generator", "Puzzle71Solver ConfigMigrator"}
    };

    // Add sections
    for (const auto& [section_name, section] : sections) {
        nlohmann::json section_json;

        if (!section.description.empty()) {
            section_json["description"] = section.description;
        }

        nlohmann::json parameters_json;
        for (const auto& param : section.parameters) {
            if (std::holds_alternative<std::string>(param.current_value)) {
                parameters_json[param.name] = std::get<std::string>(param.current_value);
            } else if (std::holds_alternative<int>(param.current_value)) {
                parameters_json[param.name] = std::get<int>(param.current_value);
            } else if (std::holds_alternative<double>(param.current_value)) {
                parameters_json[param.name] = std::get<double>(param.current_value);
            } else if (std::holds_alternative<bool>(param.current_value)) {
                parameters_json[param.name] = std::get<bool>(param.current_value);
            }
        }

        section_json["parameters"] = parameters_json;
        json_data["sections"][section_name] = section_json;
    }

    return json_data.dump(4);
}

std::string ConfigMigrator::sectionsToYaml(const std::map<std::string, ConfigSection>& sections) {
    // This would implement YAML generation
    // For now, return a basic YAML structure
    std::ostringstream yaml;
    yaml << "# Puzzle71Solver Configuration\n";
    yaml << "# Generated by ConfigMigrator\n";
    yaml << "# Generated at: " << std::put_time(std::gmtime(&std::time_t{}), "%Y-%m-%dT%H:%M:%SZ") << "\n\n";

    for (const auto& [section_name, section] : sections) {
        yaml << section_name << ":\n";

        if (!section.description.empty()) {
            yaml << "  # " << section.description << "\n";
        }

        for (const auto& param : section.parameters) {
            yaml << "  " << param.name << ": " << param.getValueAsString() << "\n";
        }
        yaml << "\n";
    }

    return yaml.str();
}

std::string ConfigMigrator::sectionsToToml(const std::map<std::string, ConfigSection>& sections) {
    // This would implement TOML generation
    // For now, return a basic TOML structure
    std::ostringstream toml;
    toml << "# Puzzle71Solver Configuration\n";
    toml << "# Generated by ConfigMigrator\n";
    toml << "# Generated at: " << std::put_time(std::gmtime(&std::time_t{}), "%Y-%m-%dT%H:%M:%SZ") << "\n\n";

    for (const auto& [section_name, section] : sections) {
        toml << "[" << section_name << "]\n";

        if (!section.description.empty()) {
            toml << "# " << section.description << "\n";
        }

        for (const auto& param : section.parameters) {
            toml << param.name << " = \"" << param.getValueAsString() << "\"\n";
        }
        toml << "\n";
    }

    return toml.str();
}

// ============================================================================
// ConfigValidator Implementation
// ============================================================================

ConfigValidator::ConfigValidator() {
    initializeKnownParameters();
    initializeValidators();
}

void ConfigValidator::initializeKnownParameters() {
    // Initialize known parameters with their definitions
    // This is a subset - in a real implementation this would be comprehensive

    ConfigParameter gpu_threads;
    gpu_threads.name = "gpu_threads";
    gpu_threads.description = "Number of GPU threads to use";
    gpu_threads.default_value = 256;
    gpu_threads.is_required = false;
    known_parameters_["gpu_threads"] = gpu_threads;

    ConfigParameter batch_size;
    batch_size.name = "batch_size";
    batch_size.description = "Batch size for processing";
    batch_size.default_value = 1000;
    batch_size.is_required = false;
    known_parameters_["batch_size"] = batch_size;

    // Add more known parameters as needed
}

void ConfigValidator::initializeValidators() {
    // Initialize parameter validators
    validators_["gpu_threads"] = [](const std::string& value) {
        try {
            int threads = std::stoi(value);
            return threads > 0 && threads <= 1024;
        } catch (...) {
            return false;
        }
    };

    validators_["batch_size"] = [](const std::string& value) {
        try {
            int batch = std::stoi(value);
            return batch > 0 && batch <= 100000;
        } catch (...) {
            return false;
        }
    };
}

ValidationResult ConfigValidator::validateConfigFormat(const std::string& config_file) {
    ValidationResult result;

    if (!std::filesystem::exists(config_file)) {
        result.errors.push_back("Configuration file not found: " + config_file);
        result.is_valid = false;
        return result;
    }

    // Detect format
    LegacyConfigParser legacy_parser;
    ConfigFormat detected_format = legacy_parser.detectConfigFormat(config_file);
    result.detected_format = detected_format;

    switch (detected_format) {
        case ConfigFormat::LEGACY_V1:
            result.format_description = "Legacy V1 (key=value format)";
            break;
        case ConfigFormat::LEGACY_V2:
            result.format_description = "Legacy V2 (sectioned format)";
            break;
        case ConfigFormat::JSON_V1:
            result.format_description = "JSON V1 format";
            break;
        case ConfigFormat::JSON_V2:
            result.format_description = "JSON V2 format (current)";
            break;
        case ConfigFormat::YAML_V1:
            result.format_description = "YAML format";
            break;
        case ConfigFormat::TOML_V1:
            result.format_description = "TOML format";
            break;
        default:
            result.format_description = "Unknown format";
            result.errors.push_back("Unknown configuration format");
            result.is_valid = false;
            return result;
    }

    // Try to parse the configuration
    try {
        if (detected_format == ConfigFormat::LEGACY_V1 || detected_format == ConfigFormat::LEGACY_V2) {
            auto parse_result = legacy_parser.parseLegacyConfig(config_file);
            if (!parse_result.success) {
                result.errors.insert(result.errors.end(),
                                   parse_result.parse_errors.begin(),
                                   parse_result.parse_errors.end());
                result.is_valid = false;
            } else {
                result.is_valid = true;
            }
        } else {
            ModernConfigParser modern_parser;
            auto parse_result = modern_parser.parseModernConfig(config_file);
            if (!parse_result.success) {
                result.errors.insert(result.errors.end(),
                                   parse_result.parse_errors.begin(),
                                   parse_result.parse_errors.end());
                result.is_valid = false;
            } else {
                result.is_valid = true;
            }
        }
    } catch (const std::exception& e) {
        result.errors.push_back("Exception during validation: " + std::string(e.what()));
        result.is_valid = false;
    }

    if (result.is_valid) {
        result.suggestions.push_back("Configuration format is valid and supported");
    } else {
        result.suggestions.push_back("Consider migrating to JSON V2 format for better compatibility");
    }

    return result;
}

std::string ConfigValidator::generateValidationReport(const ValidationResult& result) {
    std::ostringstream report;
    report << "Configuration Validation Report\n";
    report << "================================\n\n";
    report << "Status: " << (result.is_valid ? "VALID" : "INVALID") << "\n";
    report << "Detected Format: " << result.format_description << "\n";
    report << "Severity Level: " << result.getSeverityLevel() << "\n\n";

    if (!result.errors.empty()) {
        report << "Errors (" << result.errors.size() << "):\n";
        for (const auto& error : result.errors) {
            report << "  ❌ " << error << "\n";
        }
        report << "\n";
    }

    if (!result.warnings.empty()) {
        report << "Warnings (" << result.warnings.size() << "):\n";
        for (const auto& warning : result.warnings) {
            report << "  ⚠️  " << warning << "\n";
        }
        report << "\n";
    }

    if (!result.suggestions.empty()) {
        report << "Suggestions (" << result.suggestions.size() << "):\n";
        for (const auto& suggestion : result.suggestions) {
            report << "  💡 " << suggestion << "\n";
        }
        report << "\n";
    }

    return report.str();
}

// ============================================================================
// ConfigCompatibilityManager Implementation
// ============================================================================

ConfigCompatibilityManager::ConfigCompatibilityManager() {
    initializeFormatMappings();
}

bool ConfigCompatibilityManager::initialize() {
    if (initialized_) {
        return true;
    }

    try {
        // Initialize parsers
        // (They're already initialized in their constructors)

        initialized_ = true;
        LOG_INFO("Configuration compatibility manager initialized successfully");
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize configuration compatibility manager: " << e.what());
        return false;
    }
}

void ConfigCompatibilityManager::initializeFormatMappings() {
    format_extensions_[ConfigFormat::LEGACY_V1] = ".cfg";
    format_extensions_[ConfigFormat::LEGACY_V2] = ".conf";
    format_extensions_[ConfigFormat::JSON_V1] = ".json";
    format_extensions_[ConfigFormat::JSON_V2] = ".json";
    format_extensions_[ConfigFormat::YAML_V1] = ".yaml";
    format_extensions_[ConfigFormat::TOML_V1] = ".toml";

    format_descriptions_[ConfigFormat::LEGACY_V1] = "Legacy V1 - Simple key=value format";
    format_descriptions_[ConfigFormat::LEGACY_V2] = "Legacy V2 - Sectioned key=value format";
    format_descriptions_[ConfigFormat::JSON_V1] = "JSON V1 - Basic JSON format";
    format_descriptions_[ConfigFormat::JSON_V2] = "JSON V2 - Enhanced JSON with metadata (Recommended)";
    format_descriptions_[ConfigFormat::YAML_V1] = "YAML - Human-readable data serialization format";
    format_descriptions_[ConfigFormat::TOML_V1] = "TOML - Tom's Obvious, Minimal Language";
}

ConfigCompatibilityManager::LoadResult ConfigCompatibilityManager::loadConfiguration(const std::string& config_file) {
    LoadResult result;

    if (!initialized_) {
        result.validation_result.errors.push_back("Configuration compatibility manager not initialized");
        return result;
    }

    if (!std::filesystem::exists(config_file)) {
        result.validation_result.errors.push_back("Configuration file not found: " + config_file);
        return result;
    }

    try {
        // Detect format
        ConfigFormat detected_format = legacy_parser_.detectConfigFormat(config_file);
        result.detected_format = detected_format;

        // Parse configuration
        if (detected_format == ConfigFormat::LEGACY_V1 || detected_format == ConfigFormat::LEGACY_V2) {
            auto parse_result = legacy_parser_.parseLegacyConfig(config_file);
            if (parse_result.success) {
                result.sections = parse_result.sections;
                result.success = true;
            } else {
                result.validation_result.errors.insert(result.validation_result.errors.end(),
                                                      parse_result.parse_errors.begin(),
                                                      parse_result.parse_errors.end());
            }
        } else {
            auto parse_result = modern_parser_.parseModernConfig(config_file);
            if (parse_result.success) {
                result.sections = modern_parser_.jsonToSections(parse_result.config_data);
                result.success = true;
            } else {
                result.validation_result.errors.insert(result.validation_result.errors.end(),
                                                      parse_result.parse_errors.begin(),
                                                      parse_result.parse_errors.end());
            }
        }

        // Validate configuration
        if (result.success) {
            result.validation_result = validator_.validateConfigContent(result.sections, detected_format);

            // Check if migration is needed
            if (detected_format != ConfigFormat::JSON_V2) {
                result.was_migrated = true;
                // Auto-migrate to JSON V2 would happen here
            }
        }

    } catch (const std::exception& e) {
        result.validation_result.errors.push_back("Exception loading configuration: " + std::string(e.what()));
        result.success = false;
    }

    return result;
}

bool ConfigCompatibilityManager::saveConfiguration(
    const std::map<std::string, ConfigSection>& sections,
    const std::string& output_file,
    ConfigFormat format) {

    if (!initialized_) {
        LOG_ERROR("Configuration compatibility manager not initialized");
        return false;
    }

    try {
        std::string content;

        switch (format) {
            case ConfigFormat::JSON_V2:
                content = migrator_.sectionsToJson(sections);
                break;
            case ConfigFormat::YAML_V1:
                content = migrator_.sectionsToYaml(sections);
                break;
            case ConfigFormat::TOML_V1:
                content = migrator_.sectionsToToml(sections);
                break;
            default:
                LOG_ERROR("Unsupported format for saving: " << static_cast<int>(format));
                return false;
        }

        std::ofstream file(output_file);
        file << content;
        file.close();

        LOG_INFO("Configuration saved successfully: " << output_file);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Error saving configuration: " << e.what());
        return false;
    }
}

ConfigCompatibilityManager::CompatibilityReport ConfigCompatibilityManager::checkCompatibility(const std::string& config_file) {
    CompatibilityReport report;

    if (!initialized_) {
        report.compatibility_issues.push_back("Configuration compatibility manager not initialized");
        return report;
    }

    // Validate configuration
    report.validation_result = validator_.validateConfigFormat(config_file);
    report.current_format = report.validation_result.detected_format;

    if (!report.validation_result.is_valid) {
        report.compatibility_issues.insert(report.compatibility_issues.end(),
                                          report.validation_result.errors.begin(),
                                          report.validation_result.errors.end());
        report.is_compatible = false;
    } else {
        report.is_compatible = true;
    }

    // Check if format is current
    if (report.current_format != ConfigFormat::JSON_V2) {
        report.compatibility_issues.push_back("Using outdated configuration format");
        report.migration_suggestions.push_back("Migrate to JSON V2 format for better compatibility and features");
        report.recommended_format = ConfigFormat::JSON_V2;
    }

    // Generate migration plan
    if (!report.is_compatible || report.current_format != ConfigFormat::JSON_V2) {
        report.migration_plan = migrator_.generateMigrationPlan(
            report.current_format,
            report.recommended_format,
            {}
        );
    }

    return report;
}

MigrationResult ConfigCompatibilityManager::autoMigrateConfiguration(
    const std::string& config_file,
    bool create_backup) {

    MigrationResult result;
    result.source_file = config_file;

    if (!initialized_) {
        result.issues_encountered.push_back("Configuration compatibility manager not initialized");
        return result;
    }

    try {
        // Create backup if requested
        std::string backup_file;
        if (create_backup) {
            backup_file = createBackupFile(config_file);
            if (backup_file.empty()) {
                result.issues_encountered.push_back("Failed to create backup file");
            } else {
                result.migrations_applied.push_back("Created backup: " + backup_file);
            }
        }

        // Generate target filename
        std::string target_file = config_file;
        if (target_file.find_last_of('.') != std::string::npos) {
            target_file = target_file.substr(0, target_file.find_last_of('.')) + "_migrated.json";
        } else {
            target_file += "_migrated.json";
        }
        result.target_file = target_file;

        // Perform migration
        result = migrator_.migrateConfiguration(config_file, target_file, ConfigFormat::JSON_V2);

        if (result.success) {
            result.migrations_applied.push_back("Auto-migration completed successfully");
            LOG_INFO("Configuration auto-migrated successfully: " << config_file << " -> " << target_file);
        }

    } catch (const std::exception& e) {
        result.issues_encountered.push_back("Auto-migration failed: " + std::string(e.what()));
        result.success = false;
    }

    return result;
}

std::string ConfigCompatibilityManager::createBackupFile(const std::string& original_file) {
    try {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::gmtime(&time_t);

        std::ostringstream timestamp;
        timestamp << std::put_time(&tm, "%Y%m%d_%H%M%S");

        std::filesystem::path original_path(original_file);
        std::string backup_file = original_path.stem().string() + "_backup_" + timestamp.str() + original_path.extension().string();
        backup_file = original_path.parent_path() / backup_file;

        std::filesystem::copy_file(original_file, backup_file);
        return backup_file;

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to create backup file: " << e.what());
        return "";
    }
}

std::vector<ConfigFormat> ConfigCompatibilityManager::getSupportedFormats() const {
    return {
        ConfigFormat::LEGACY_V1,
        ConfigFormat::LEGACY_V2,
        ConfigFormat::JSON_V1,
        ConfigFormat::JSON_V2,
        ConfigFormat::YAML_V1,
        ConfigFormat::TOML_V1
    };
}

std::string ConfigCompatibilityManager::getFormatDescription(ConfigFormat format) const {
    auto it = format_descriptions_.find(format);
    if (it != format_descriptions_.end()) {
        return it->second;
    }
    return "Unknown format";
}

ConfigFormat ConfigCompatibilityManager::getFormatFromExtension(const std::string& file_extension) const {
    for (const auto& [format, ext] : format_extensions_) {
        if (ext == file_extension) {
            return format;
        }
    }
    return ConfigFormat::UNKNOWN;
}

// ============================================================================
// Utility Functions Implementation
// ============================================================================

namespace config_utils {

std::string formatToString(ConfigFormat format) {
    switch (format) {
        case ConfigFormat::LEGACY_V1: return "legacy_v1";
        case ConfigFormat::LEGACY_V2: return "legacy_v2";
        case ConfigFormat::JSON_V1: return "json_v1";
        case ConfigFormat::JSON_V2: return "json_v2";
        case ConfigFormat::YAML_V1: return "yaml_v1";
        case ConfigFormat::TOML_V1: return "toml_v1";
        default: return "unknown";
    }
}

ConfigFormat stringToFormat(const std::string& format_str) {
    if (format_str == "legacy_v1") return ConfigFormat::LEGACY_V1;
    if (format_str == "legacy_v2") return ConfigFormat::LEGACY_V2;
    if (format_str == "json_v1") return ConfigFormat::JSON_V1;
    if (format_str == "json_v2") return ConfigFormat::JSON_V2;
    if (format_str == "yaml_v1") return ConfigFormat::YAML_V1;
    if (format_str == "toml_v1") return ConfigFormat::TOML_V1;
    return ConfigFormat::UNKNOWN;
}

std::string getExtensionForFormat(ConfigFormat format) {
    switch (format) {
        case ConfigFormat::LEGACY_V1: return ".cfg";
        case ConfigFormat::LEGACY_V2: return ".conf";
        case ConfigFormat::JSON_V1:
        case ConfigFormat::JSON_V2: return ".json";
        case ConfigFormat::YAML_V1: return ".yaml";
        case ConfigFormat::TOML_V1: return ".toml";
        default: return "";
    }
}

bool parseBoolValue(const std::string& value, bool default_value) {
    std::string lower_value = value;
    std::transform(lower_value.begin(), lower_value.end(), lower_value.begin(), ::tolower);

    if (lower_value == "true" || lower_value == "yes" || lower_value == "1" || lower_value == "on") {
        return true;
    } else if (lower_value == "false" || lower_value == "no" || lower_value == "0" || lower_value == "off") {
        return false;
    }

    return default_value;
}

int parseIntValue(const std::string& value, int default_value) {
    try {
        return std::stoi(value);
    } catch (...) {
        return default_value;
    }
}

double parseDoubleValue(const std::string& value, double default_value) {
    try {
        return std::stod(value);
    } catch (...) {
        return default_value;
    }
}

bool isValidConfigPath(const std::string& config_path) {
    return std::filesystem::exists(config_path) &&
           std::filesystem::is_regular_file(config_path);
}

} // namespace config_utils

// ============================================================================
// ConfigCompatibilitySystem Implementation
// ============================================================================

ConfigCompatibilitySystem& ConfigCompatibilitySystem::getInstance() {
    static ConfigCompatibilitySystem instance;
    return instance;
}

bool ConfigCompatibilitySystem::initialize() {
    if (initialized_) {
        return true;
    }

    manager_ = std::make_shared<ConfigCompatibilityManager>();
    if (!manager_->initialize()) {
        return false;
    }

    initialized_ = true;
    LOG_INFO("Configuration compatibility system initialized");
    return true;
}

std::shared_ptr<ConfigCompatibilityManager> ConfigCompatibilitySystem::getCompatibilityManager() {
    if (!initialized_) {
        initialize();
    }
    return manager_;
}

ConfigCompatibilityManager::LoadResult ConfigCompatibilitySystem::quickLoadConfig(const std::string& config_file) {
    if (!initialized_) {
        initialize();
    }
    return manager_->loadConfiguration(config_file);
}

ValidationResult ConfigCompatibilitySystem::quickValidateConfig(const std::string& config_file) {
    if (!initialized_) {
        initialize();
    }
    return manager_->validateConfigurationFile(config_file);
}

MigrationResult ConfigCompatibilitySystem::quickMigrateConfig(
    const std::string& config_file,
    ConfigFormat target_format) {

    if (!initialized_) {
        initialize();
    }
    return manager_->migrateConfigurationFile(config_file, "", target_format);
}

std::string ConfigCompatibilitySystem::generateSystemReport() {
    if (!initialized_) {
        return "Configuration compatibility system not initialized";
    }

    std::ostringstream report;
    report << "Configuration Compatibility System Report\n";
    report << "========================================\n\n";
    report << "System Status: " << (initialized_ ? "INITIALIZED" : "NOT INITIALIZED") << "\n";
    report << "Supported Formats: " << manager_->getSupportedFormats().size() << "\n\n";

    report << "Supported Formats:\n";
    for (const auto& format : manager_->getSupportedFormats()) {
        report << "  - " << config_utils::formatToString(format)
                << " (" << manager_->getFormatDescription(format) << ")\n";
    }
    report << "\n";

    report << "Recommended Format: " << config_utils::formatToString(manager_->getRecommendedFormat()) << "\n\n";

    return report.str();
}

} // namespace puzzle71::compatibility
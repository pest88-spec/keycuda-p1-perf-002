// Puzzle71Solver - Configuration Compatibility Tests (T057)
// Phase 7: User Story 5 - Compatibility Assurance
// Comprehensive tests for configuration compatibility and migration system

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "KeyhuntCore/compatibility/config_compatibility.h"
#include "KeyhuntCore/utils/logger.h"

using namespace puzzle71::compatibility;
using namespace ::testing;

namespace puzzle71::compatibility {

class ConfigCompatibilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        test_dir_ = std::filesystem::temp_directory_path() / "config_compatibility_test";
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        // Clean up test directory
        std::filesystem::remove_all(test_dir_);
    }

    void CreateLegacyV1Config(const std::string& filename) {
        std::ofstream file(test_dir_ / filename);
        file << "# Puzzle71Solver Configuration (Legacy V1)\n";
        file << "# Generated for testing\n\n";
        file << "threads=256\n";
        file << "blocks=100\n";
        file << "device=0\n";
        file << "memory=4096\n";
        file << "batch=1000\n";
        file << "range=1000:2000\n";
        file << "targets=addresses.txt\n";
        file << "output=results.txt\n";
        file << "verbose=true\n";
        file << "debug=false\n";
        file.close();
    }

    void CreateLegacyV2Config(const std::string& filename) {
        std::ofstream file(test_dir_ / filename);
        file << "# Puzzle71Solver Configuration (Legacy V2)\n";
        file << "# Generated for testing\n\n";
        file << "[gpu]\n";
        file << "threads=256\n";
        file << "blocks=100\n";
        file << "device=0\n";
        file << "memory=4096\n\n";
        file << "[processing]\n";
        file << "batch=1000\n";
        file << "range=1000:2000\n\n";
        file << "[output]\n";
        file << "targets=addresses.txt\n";
        file << "output=results.txt\n";
        file << "verbose=true\n";
        file << "debug=false\n";
        file.close();
    }

    void CreateJsonV1Config(const std::string& filename) {
        std::ofstream file(test_dir_ / filename);
        file << "{\n";
        file << "  \"threads\": 256,\n";
        file << "  \"blocks\": 100,\n";
        file << "  \"device\": 0,\n";
        file << "  \"memory\": 4096,\n";
        file << "  \"batch\": 1000,\n";
        file << "  \"range\": \"1000:2000\",\n";
        file << "  \"targets\": \"addresses.txt\",\n";
        file << "  \"output\": \"results.txt\",\n";
        file << "  \"verbose\": true,\n";
        file << "  \"debug\": false\n";
        file << "}\n";
        file.close();
    }

    void CreateJsonV2Config(const std::string& filename) {
        std::ofstream file(test_dir_ / filename);
        file << "{\n";
        file << "  \"metadata\": {\n";
        file << "    \"version\": \"2.0\",\n";
        file << "    \"format\": \"json_v2\",\n";
        file << "    \"generated_at\": \"2025-01-19T10:00:00Z\",\n";
        file << "    \"generator\": \"Puzzle71Solver\"\n";
        file << "  },\n";
        file << "  \"sections\": {\n";
        file << "    \"gpu\": {\n";
        file << "      \"parameters\": {\n";
        file << "        \"threads\": 256,\n";
        file << "        \"blocks\": 100,\n";
        file << "        \"device\": 0,\n";
        file << "        \"memory\": 4096\n";
        file << "      }\n";
        file << "    },\n";
        file << "    \"processing\": {\n";
        file << "      \"parameters\": {\n";
        file << "        \"batch\": 1000,\n";
        file << "        \"range\": \"1000:2000\"\n";
        file << "      }\n";
        file << "    },\n";
        file << "    \"output\": {\n";
        file << "      \"parameters\": {\n";
        file << "        \"targets\": \"addresses.txt\",\n";
        file << "        \"output\": \"results.txt\",\n";
        file << "        \"verbose\": true,\n";
        file << "        \"debug\": false\n";
        file << "      }\n";
        file << "    }\n";
        file << "  }\n";
        file << "}\n";
        file.close();
    }

    void CreateYamlConfig(const std::string& filename) {
        std::ofstream file(test_dir_ / filename);
        file << "# Puzzle71Solver Configuration (YAML)\n";
        file << "metadata:\n";
        file << "  version: \"2.0\"\n";
        file << "  format: \"yaml_v1\"\n";
        file << "  generated_at: \"2025-01-19T10:00:00Z\"\n";
        file << "\n";
        file << "sections:\n";
        file << "  gpu:\n";
        file << "    parameters:\n";
        file << "      threads: 256\n";
        file << "      blocks: 100\n";
        file << "      device: 0\n";
        file << "      memory: 4096\n";
        file << "  processing:\n";
        file << "    parameters:\n";
        file << "      batch: 1000\n";
        file << "      range: \"1000:2000\"\n";
        file.close();
    }

    void CreateTomlConfig(const std::string& filename) {
        std::ofstream file(test_dir_ / filename);
        file << "# Puzzle71Solver Configuration (TOML)\n";
        file << "[metadata]\n";
        file << "version = \"2.0\"\n";
        file << "format = \"toml_v1\"\n";
        file << "generated_at = \"2025-01-19T10:00:00Z\"\n";
        file << "\n";
        file << "[sections.gpu.parameters]\n";
        file << "threads = 256\n";
        file << "blocks = 100\n";
        file << "device = 0\n";
        file << "memory = 4096\n";
        file << "\n";
        file << "[sections.processing.parameters]\n";
        file << "batch = 1000\n";
        file << "range = \"1000:2000\"\n";
        file.close();
    }

    std::filesystem::path test_dir_;
};

// Test ValidationResult functionality
TEST_F(ConfigCompatibilityTest, ValidationResultInitialization) {
    ValidationResult result;

    EXPECT_FALSE(result.is_valid);
    EXPECT_TRUE(result.errors.empty());
    EXPECT_TRUE(result.warnings.empty());
    EXPECT_TRUE(result.suggestions.empty());
    EXPECT_EQ(result.detected_format, ConfigFormat::UNKNOWN);
    EXPECT_TRUE(result.format_description.empty());
}

TEST_F(ConfigCompatibilityTest, ValidationResultValidation) {
    ValidationResult result;
    result.is_valid = true;

    EXPECT_TRUE(result.isValid());

    // Add an error
    result.errors.push_back("Test error");
    EXPECT_FALSE(result.isValid());

    // Clear errors and add warning
    result.errors.clear();
    result.warnings.push_back("Test warning");
    EXPECT_TRUE(result.isValid());
}

TEST_F(ConfigCompatibilityTest, ValidationResultSeverityLevel) {
    ValidationResult result;

    // No issues
    EXPECT_EQ(result.getSeverityLevel(), "success");

    // Only suggestions
    result.suggestions.push_back("Test suggestion");
    EXPECT_EQ(result.getSeverityLevel(), "info");

    // Only warnings
    result.warnings.push_back("Test warning");
    EXPECT_EQ(result.getSeverityLevel(), "warning");

    // Only errors
    result.warnings.clear();
    result.errors.push_back("Test error");
    EXPECT_EQ(result.getSeverityLevel(), "error");
}

TEST_F(ConfigCompatibilityTest, ValidationResultJsonSerialization) {
    ValidationResult result;
    result.is_valid = true;
    result.detected_format = ConfigFormat::JSON_V2;
    result.format_description = "JSON V2 format";
    result.suggestions.push_back("Consider using JSON V2");

    std::string json = result.toJson();

    EXPECT_NE(json.find("\"is_valid\":true"), std::string::npos);
    EXPECT_NE(json.find("\"detected_format\":4"), std::string::npos);
    EXPECT_NE(json.find("\"format_description\":\"JSON V2 format\""), std::string::npos);
    EXPECT_NE(json.find("\"suggestions\":[\"Consider using JSON V2\"]"), std::string::npos);
}

// Test ConfigParameter functionality
TEST_F(ConfigCompatibilityTest, ConfigParameterInitialization) {
    ConfigParameter param;
    param.name = "test_param";
    param.description = "Test parameter";
    param.section = "test_section";

    EXPECT_EQ(param.name, "test_param");
    EXPECT_EQ(param.description, "Test parameter");
    EXPECT_EQ(param.section, "test_section");
    EXPECT_FALSE(param.is_required);
    EXPECT_FALSE(param.is_deprecated);
}

TEST_F(ConfigCompatibilityTest, ConfigParameterBoolValue) {
    ConfigParameter param;
    param.name = "bool_param";
    param.current_value = true;
    param.default_value = false;

    EXPECT_EQ(param.getValueAsString(), "true");
    EXPECT_TRUE(param.setValueFromString("false"));
    EXPECT_EQ(param.getValueAsString(), "false");
}

TEST_F(ConfigCompatibilityTest, ConfigParameterIntValue) {
    ConfigParameter param;
    param.name = "int_param";
    param.current_value = 42;
    param.default_value = 10;

    EXPECT_EQ(param.getValueAsString(), "42");
    EXPECT_TRUE(param.setValueFromString("100"));
    EXPECT_EQ(param.getValueAsString(), "100");
}

TEST_F(ConfigCompatibilityTest, ConfigParameterDoubleValue) {
    ConfigParameter param;
    param.name = "double_param";
    param.current_value = 3.14;
    param.default_value = 1.0;

    EXPECT_EQ(param.getValueAsString(), "3.140000");
    EXPECT_TRUE(param.setValueFromString("2.718"));
    EXPECT_EQ(param.getValueAsString(), "2.718000");
}

TEST_F(ConfigCompatibilityTest, ConfigParameterStringValue) {
    ConfigParameter param;
    param.name = "string_param";
    param.current_value = std::string("test_value");
    param.default_value = std::string("default");

    EXPECT_EQ(param.getValueAsString(), "test_value");
    EXPECT_TRUE(param.setValueFromString("new_value"));
    EXPECT_EQ(param.getValueAsString(), "new_value");
}

// Test ConfigSection functionality
TEST_F(ConfigCompatibilityTest, ConfigSectionInitialization) {
    ConfigSection section;
    section.name = "test_section";
    section.description = "Test section";

    EXPECT_EQ(section.name, "test_section");
    EXPECT_EQ(section.description, "Test section");
    EXPECT_TRUE(section.parameters.empty());
    EXPECT_FALSE(section.is_required);
    EXPECT_FALSE(section.is_deprecated);
}

TEST_F(ConfigCompatibilityTest, ConfigSectionParameterOperations) {
    ConfigSection section;
    section.name = "test_section";

    ConfigParameter param1;
    param1.name = "param1";
    param1.current_value = 42;

    ConfigParameter param2;
    param2.name = "param2";
    param2.current_value = std::string("test");

    section.addParameter(param1);
    section.addParameter(param2);

    EXPECT_EQ(section.parameters.size(), 2);

    // Find parameter
    auto found = section.findParameter("param1");
    EXPECT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "param1");

    auto not_found = section.findParameter("nonexistent");
    EXPECT_FALSE(not_found.has_value());
}

// Test LegacyConfigParser
TEST_F(ConfigCompatibilityTest, LegacyConfigParserFormatDetection) {
    LegacyConfigParser parser;

    // Test Legacy V1 detection
    CreateLegacyV1Config("test_v1.cfg");
    ConfigFormat format = parser.detectConfigFormat((test_dir_ / "test_v1.cfg").string());
    EXPECT_EQ(format, ConfigFormat::LEGACY_V1);

    // Test Legacy V2 detection
    CreateLegacyV2Config("test_v2.conf");
    format = parser.detectConfigFormat((test_dir_ / "test_v2.conf").string());
    EXPECT_EQ(format, ConfigFormat::LEGACY_V2);

    // Test JSON detection
    CreateJsonV1Config("test.json");
    format = parser.detectConfigFormat((test_dir_ / "test.json").string());
    EXPECT_EQ(format, ConfigFormat::JSON_V1);
}

TEST_F(ConfigCompatibilityTest, LegacyConfigParserV1Parsing) {
    LegacyConfigParser parser;
    CreateLegacyV1Config("test_v1.cfg");

    auto result = parser.parseLegacyV1Config("# Test content\nthreads=256\nverbose=true");

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.detected_format, ConfigFormat::LEGACY_V1);
    EXPECT_FALSE(result.sections.empty());

    // Check default section was created
    auto default_section = result.sections.find("default");
    EXPECT_NE(default_section, result.sections.end());

    // Check parameters
    const auto& section = default_section->second;
    EXPECT_GT(section.parameters.size(), 0);

    // Find specific parameters
    bool found_threads = false;
    bool found_verbose = false;

    for (const auto& param : section.parameters) {
        if (param.name == "gpu_threads") {
            found_threads = true;
            EXPECT_EQ(param.getValueAsString(), "256");
        }
        if (param.name == "verbosity_level") {
            found_verbose = true;
            EXPECT_EQ(param.getValueAsString(), "true");
        }
    }

    EXPECT_TRUE(found_threads);
    EXPECT_TRUE(found_verbose);
}

TEST_F(ConfigCompatibilityTest, LegacyConfigParserV2Parsing) {
    LegacyConfigParser parser;
    CreateLegacyV2Config("test_v2.conf");

    auto result = parser.parseLegacyV2Config("[gpu]\nthreads=256\n[output]\nverbose=true");

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.detected_format, ConfigFormat::LEGACY_V2);
    EXPECT_FALSE(result.sections.empty());

    // Check sections were created
    auto gpu_section = result.sections.find("gpu");
    auto output_section = result.sections.find("output");

    EXPECT_NE(gpu_section, result.sections.end());
    EXPECT_NE(output_section, result.sections.end());

    // Check GPU section parameters
    if (gpu_section != result.sections.end()) {
        bool found_threads = false;
        for (const auto& param : gpu_section->second.parameters) {
            if (param.name == "threads") {
                found_threads = true;
                EXPECT_EQ(param.getValueAsString(), "256");
                break;
            }
        }
        EXPECT_TRUE(found_threads);
    }
}

// Test ModernConfigParser
TEST_F(ConfigCompatibilityTest, ModernConfigParserJsonParsing) {
    ModernConfigParser parser;
    CreateJsonV2Config("test.json");

    auto result = parser.parseJsonConfig((test_dir_ / "test.json").string());

    EXPECT_TRUE(result.success);
    EXPECT_NE(result.detected_format, ConfigFormat::UNKNOWN);
    EXPECT_FALSE(result.config_data.empty());

    // Check JSON structure
    EXPECT_TRUE(result.config_data.contains("metadata"));
    EXPECT_TRUE(result.config_data.contains("sections"));
}

TEST_F(ConfigCompatibilityTest, ModernConfigParserJsonToSections) {
    ModernConfigParser parser;

    // Create test JSON data
    nlohmann::json test_json;
    test_json["metadata"]["version"] = "2.0";
    test_json["sections"]["gpu"]["parameters"]["threads"] = 256;
    test_json["sections"]["gpu"]["parameters"]["blocks"] = 100;
    test_json["sections"]["output"]["parameters"]["verbose"] = true;

    auto sections = parser.jsonToSections(test_json);

    EXPECT_EQ(sections.size(), 2);

    // Check GPU section
    auto gpu_section = sections.find("gpu");
    EXPECT_NE(gpu_section, sections.end());

    if (gpu_section != sections.end()) {
        const auto& section = gpu_section->second;
        EXPECT_EQ(section.name, "gpu");
        EXPECT_EQ(section.parameters.size(), 2);

        // Find threads parameter
        auto threads_param = section.findParameter("threads");
        EXPECT_TRUE(threads_param.has_value());
        EXPECT_EQ(threads_param->getValueAsString(), "256");
    }

    // Check output section
    auto output_section = sections.find("output");
    EXPECT_NE(output_section, sections.end());

    if (output_section != sections.end()) {
        const auto& section = output_section->second;
        EXPECT_EQ(section.name, "output");
        EXPECT_EQ(section.parameters.size(), 1);

        // Find verbose parameter
        auto verbose_param = section.findParameter("verbose");
        EXPECT_TRUE(verbose_param.has_value());
        EXPECT_EQ(verbose_param->getValueAsString(), "true");
    }
}

// Test ConfigMigrator
class ConfigMigratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = std::filesystem::temp_directory_path() / "config_migrator_test";
        std::filesystem::create_directories(test_dir_);
        migrator_ = std::make_unique<ConfigMigrator>();
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
    }

    void CreateLegacyConfig(const std::string& filename) {
        std::ofstream file(test_dir_ / filename);
        file << "threads=256\n";
        file << "blocks=100\n";
        file << "verbose=true\n";
        file.close();
    }

    std::filesystem::path test_dir_;
    std::unique_ptr<ConfigMigrator> migrator_;
};

TEST_F(ConfigMigratorTest, MigrationResultInitialization) {
    MigrationResult result;

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.source_format, ConfigFormat::UNKNOWN);
    EXPECT_EQ(result.target_format, ConfigFormat::UNKNOWN);
    EXPECT_TRUE(result.migrations_applied.empty());
    EXPECT_TRUE(result.issues_encountered.empty());
}

TEST_F(ConfigMigratorTest, MigrationResultSuccessCheck) {
    MigrationResult result;
    result.success = true;

    EXPECT_TRUE(result.isSuccessful());

    // Add issues
    result.issues_encountered.push_back("Test issue");
    EXPECT_FALSE(result.isSuccessful());
}

TEST_F(ConfigMigratorTest, LegacyToModernMigration) {
    CreateLegacyConfig("legacy.cfg");

    std::string source_file = (test_dir_ / "legacy.cfg").string();
    std::string target_file = (test_dir_ / "modern.json").string();

    auto result = migrator_->migrateLegacyToModern(source_file, target_file);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.source_format, ConfigFormat::LEGACY_V1);
    EXPECT_EQ(result.target_format, ConfigFormat::JSON_V2);
    EXPECT_FALSE(result.migrations_applied.empty());
    EXPECT_TRUE(std::filesystem::exists(target_file));
}

TEST_F(ConfigMigratorTest, ConfigurationMigration) {
    CreateLegacyConfig("legacy.cfg");

    std::string source_file = (test_dir_ / "legacy.cfg").string();
    std::string target_file = (test_dir_ / "migrated.json").string();

    auto result = migrator_->migrateConfiguration(
        source_file,
        target_file,
        ConfigFormat::JSON_V2
    );

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.source_format, ConfigFormat::LEGACY_V1);
    EXPECT_EQ(result.target_format, ConfigFormat::JSON_V2);
    EXPECT_TRUE(std::filesystem::exists(target_file));

    // Verify target file contains expected content
    std::ifstream file(target_file);
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();

    EXPECT_NE(content.find("\"metadata\""), std::string::npos);
    EXPECT_NE(content.find("\"sections\""), std::string::npos);
    EXPECT_NE(content.find("\"version\": \"2.0\""), std::string::npos);
}

TEST_F(ConfigMigratorTest, MigrationPlanGeneration) {
    std::map<std::string, ConfigSection> source_sections;

    ConfigSection gpu_section;
    gpu_section.name = "gpu";
    ConfigParameter threads_param;
    threads_param.name = "threads";
    threads_param.current_value = 256;
    gpu_section.addParameter(threads_param);
    source_sections["gpu"] = gpu_section;

    auto plan = migrator_->generateMigrationPlan(
        ConfigFormat::LEGACY_V1,
        ConfigFormat::JSON_V2,
        source_sections
    );

    EXPECT_FALSE(plan.steps.empty());
    EXPECT_FALSE(plan.required_changes.empty());
    EXPECT_FALSE(plan.parameter_renames.empty());
}

// Test ConfigValidator
class ConfigValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = std::filesystem::temp_directory_path() / "config_validator_test";
        std::filesystem::create_directories(test_dir_);
        validator_ = std::make_unique<ConfigValidator>();
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
    }

    void CreateValidConfig(const std::string& filename) {
        std::ofstream file(test_dir_ / filename);
        file << "{\n";
        file << "  \"gpu_threads\": 256,\n";
        file << "  \"batch_size\": 1000\n";
        file << "}\n";
        file.close();
    }

    void CreateInvalidConfig(const std::string& filename) {
        std::ofstream file(test_dir_ / filename);
        file << "{\n";
        file << "  \"gpu_threads\": -1,\n";  // Invalid: negative value
        file << "  \"batch_size\": 0\n";      // Invalid: zero value
        file << "}\n";
        file.close();
    }

    std::filesystem::path test_dir_;
    std::unique_ptr<ConfigValidator> validator_;
};

TEST_F(ConfigValidatorTest, ConfigFormatValidation) {
    CreateValidConfig("valid.json");

    auto result = validator_->validateConfigFormat((test_dir_ / "valid.json").string());

    EXPECT_TRUE(result.is_valid);
    EXPECT_TRUE(result.errors.empty());
    EXPECT_NE(result.detected_format, ConfigFormat::UNKNOWN);
    EXPECT_FALSE(result.format_description.empty());
}

TEST_F(ConfigValidatorTest, ConfigValidationWithInvalidFile) {
    std::string invalid_file = (test_dir_ / "nonexistent.json").string();

    auto result = validator_->validateConfigFormat(invalid_file);

    EXPECT_FALSE(result.is_valid);
    EXPECT_FALSE(result.errors.empty());
    EXPECT_TRUE(result.errors[0].find("not found") != std::string::npos);
}

TEST_F(ConfigValidatorTest, ParameterValidation) {
    std::map<std::string, ConfigSection> sections;

    ConfigSection gpu_section;
    gpu_section.name = "gpu";

    ConfigParameter threads_param;
    threads_param.name = "gpu_threads";
    threads_param.current_value = 256;
    threads_param.section = "gpu";
    gpu_section.addParameter(threads_param);

    ConfigParameter batch_param;
    batch_param.name = "batch_size";
    batch_param.current_value = 1000;
    batch_param.section = "gpu";
    gpu_section.addParameter(batch_param);

    sections["gpu"] = gpu_section;

    auto result = validator_->validateParameterValues(sections);

    EXPECT_TRUE(result.is_valid);
    EXPECT_TRUE(result.errors.empty());
}

// Test ConfigCompatibilityManager
class ConfigCompatibilityManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = std::filesystem::temp_directory_path() / "config_manager_test";
        std::filesystem::create_directories(test_dir_);
        manager_ = std::make_unique<ConfigCompatibilityManager>();
        EXPECT_TRUE(manager_->initialize());
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
    }

    void CreateLegacyConfig(const std::string& filename) {
        std::ofstream file(test_dir_ / filename);
        file << "threads=256\n";
        file << "blocks=100\n";
        file << "verbose=true\n";
        file.close();
    }

    void CreateModernConfig(const std::string& filename) {
        std::ofstream file(test_dir_ / filename);
        file << "{\n";
        file << "  \"metadata\": {\n";
        file << "    \"version\": \"2.0\",\n";
        file << "    \"format\": \"json_v2\"\n";
        file << "  },\n";
        file << "  \"sections\": {\n";
        file << "    \"gpu\": {\n";
        file << "      \"parameters\": {\n";
        file << "        \"threads\": 256,\n";
        file << "        \"blocks\": 100\n";
        file << "      }\n";
        file << "    }\n";
        file << "  }\n";
        file << "}\n";
        file.close();
    }

    std::filesystem::path test_dir_;
    std::unique_ptr<ConfigCompatibilityManager> manager_;
};

TEST_F(ConfigCompatibilityManagerTest, ManagerInitialization) {
    auto new_manager = std::make_unique<ConfigCompatibilityManager>();
    EXPECT_TRUE(new_manager->initialize());
}

TEST_F(ConfigCompatibilityManagerTest, LoadLegacyConfiguration) {
    CreateLegacyConfig("legacy.cfg");

    auto result = manager_->loadConfiguration((test_dir_ / "legacy.cfg").string());

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.detected_format, ConfigFormat::LEGACY_V1);
    EXPECT_FALSE(result.sections.empty());

    // Check sections were parsed
    auto default_section = result.sections.find("default");
    EXPECT_NE(default_section, result.sections.end());
}

TEST_F(ConfigCompatibilityManagerTest, LoadModernConfiguration) {
    CreateModernConfig("modern.json");

    auto result = manager_->loadConfiguration((test_dir_ / "modern.json").string());

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.detected_format, ConfigFormat::JSON_V2);
    EXPECT_FALSE(result.sections.empty());

    // Check sections were parsed
    auto gpu_section = result.sections.find("gpu");
    EXPECT_NE(gpu_section, result.sections.end());

    if (gpu_section != result.sections.end()) {
        const auto& section = gpu_section->second;
        EXPECT_EQ(section.name, "gpu");
        EXPECT_GT(section.parameters.size(), 0);
    }
}

TEST_F(ConfigCompatibilityManagerTest, SaveConfiguration) {
    CreateLegacyConfig("legacy.cfg");

    // Load configuration
    auto load_result = manager_->loadConfiguration((test_dir_ / "legacy.cfg").string());
    ASSERT_TRUE(load_result.success);

    // Save in JSON V2 format
    std::string target_file = (test_dir_ / "saved.json").string();
    bool save_success = manager_->saveConfiguration(
        load_result.sections,
        target_file,
        ConfigFormat::JSON_V2
    );

    EXPECT_TRUE(save_success);
    EXPECT_TRUE(std::filesystem::exists(target_file));

    // Verify saved content
    std::ifstream file(target_file);
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();

    EXPECT_NE(content.find("\"metadata\""), std::string::npos);
    EXPECT_NE(content.find("\"sections\""), std::string::npos);
}

TEST_F(ConfigCompatibilityManagerTest, ConfigurationMigration) {
    CreateLegacyConfig("legacy.cfg");

    std::string source_file = (test_dir_ / "legacy.cfg").string();
    std::string target_file = (test_dir_ / "migrated.json").string();

    auto result = manager_->migrateConfigurationFile(
        source_file,
        target_file,
        ConfigFormat::JSON_V2
    );

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.source_format, ConfigFormat::LEGACY_V1);
    EXPECT_EQ(result.target_format, ConfigFormat::JSON_V2);
    EXPECT_TRUE(std::filesystem::exists(target_file));
}

TEST_F(ConfigCompatibilityManagerTest, ConfigurationCompatibilityCheck) {
    CreateLegacyConfig("legacy.cfg");

    auto report = manager_->checkCompatibility((test_dir_ / "legacy.cfg").string());

    EXPECT_TRUE(report.validation_result.is_valid);
    EXPECT_EQ(report.current_format, ConfigFormat::LEGACY_V1);
    EXPECT_EQ(report.recommended_format, ConfigFormat::JSON_V2);

    // Should suggest migration since it's not JSON V2
    EXPECT_FALSE(report.compatibility_issues.empty());
    EXPECT_FALSE(report.migration_suggestions.empty());
}

TEST_F(ConfigCompatibilityManagerTest, AutoMigrateConfiguration) {
    CreateLegacyConfig("legacy.cfg");

    std::string source_file = (test_dir_ / "legacy.cfg").string();

    auto result = manager_->autoMigrateConfiguration(source_file, false);

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.target_file.empty());
    EXPECT_TRUE(std::filesystem::exists(result.target_file));

    // Verify migration was applied
    EXPECT_FALSE(result.migrations_applied.empty());
}

TEST_F(ConfigCompatibilityManagerTest, SupportedFormats) {
    auto formats = manager_->getSupportedFormats();

    EXPECT_GT(formats.size(), 0);

    // Check that all expected formats are included
    std::vector<ConfigFormat> expected_formats = {
        ConfigFormat::LEGACY_V1,
        ConfigFormat::LEGACY_V2,
        ConfigFormat::JSON_V1,
        ConfigFormat::JSON_V2,
        ConfigFormat::YAML_V1,
        ConfigFormat::TOML_V1
    };

    for (const auto& expected : expected_formats) {
        auto it = std::find(formats.begin(), formats.end(), expected);
        EXPECT_NE(it, formats.end()) << "Expected format not found: " << static_cast<int>(expected);
    }
}

TEST_F(ConfigCompatibilityManagerTest, FormatDescriptions) {
    // Test JSON V2 format
    auto description = manager_->getFormatDescription(ConfigFormat::JSON_V2);
    EXPECT_FALSE(description.empty());
    EXPECT_NE(description.find("JSON V2"), std::string::npos);

    // Test legacy format
    description = manager_->getFormatDescription(ConfigFormat::LEGACY_V1);
    EXPECT_FALSE(description.empty());
    EXPECT_NE(description.find("Legacy V1"), std::string::npos);
}

TEST_F(ConfigCompatibilityManagerTest, FormatFromExtension) {
    EXPECT_EQ(manager_->getFormatFromExtension(".json"), ConfigFormat::JSON_V2);
    EXPECT_EQ(manager_->getFormatFromExtension(".cfg"), ConfigFormat::LEGACY_V1);
    EXPECT_EQ(manager_->getFormatFromExtension(".conf"), ConfigFormat::LEGACY_V2);
    EXPECT_EQ(manager_->getFormatFromExtension(".yaml"), ConfigFormat::YAML_V1);
    EXPECT_EQ(manager_->getFormatFromExtension(".toml"), ConfigFormat::TOML_V1);
    EXPECT_EQ(manager_->getFormatFromExtension(".unknown"), ConfigFormat::UNKNOWN);
}

// Test utility functions
class ConfigUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup common test data
    }
};

TEST_F(ConfigUtilsTest, FormatToStringConversion) {
    EXPECT_EQ(config_utils::formatToString(ConfigFormat::LEGACY_V1), "legacy_v1");
    EXPECT_EQ(config_utils::formatToString(ConfigFormat::JSON_V2), "json_v2");
    EXPECT_EQ(config_utils::formatToString(ConfigFormat::YAML_V1), "yaml_v1");
    EXPECT_EQ(config_utils::formatToString(ConfigFormat::UNKNOWN), "unknown");
}

TEST_F(ConfigUtilsTest, StringToFormatConversion) {
    EXPECT_EQ(config_utils::stringToFormat("legacy_v1"), ConfigFormat::LEGACY_V1);
    EXPECT_EQ(config_utils::stringToFormat("json_v2"), ConfigFormat::JSON_V2);
    EXPECT_EQ(config_utils::stringToFormat("yaml_v1"), ConfigFormat::YAML_V1);
    EXPECT_EQ(config_utils::stringToFormat("unknown"), ConfigFormat::UNKNOWN);
}

TEST_F(ConfigUtilsTest, ExtensionForFormat) {
    EXPECT_EQ(config_utils::getExtensionForFormat(ConfigFormat::JSON_V2), ".json");
    EXPECT_EQ(config_utils::getExtensionForFormat(ConfigFormat::LEGACY_V1), ".cfg");
    EXPECT_EQ(config_utils::getExtensionForFormat(ConfigFormat::YAML_V1), ".yaml");
    EXPECT_EQ(config_utils::getExtensionForFormat(ConfigFormat::UNKNOWN), "");
}

TEST_F(ConfigUtilsTest, BoolValueParsing) {
    EXPECT_TRUE(config_utils::parseBoolValue("true"));
    EXPECT_TRUE(config_utils::parseBoolValue("yes"));
    EXPECT_TRUE(config_utils::parseBoolValue("1"));
    EXPECT_TRUE(config_utils::parseBoolValue("on"));

    EXPECT_FALSE(config_utils::parseBoolValue("false"));
    EXPECT_FALSE(config_utils::parseBoolValue("no"));
    EXPECT_FALSE(config_utils::parseBoolValue("0"));
    EXPECT_FALSE(config_utils::parseBoolValue("off"));

    // Default values
    EXPECT_TRUE(config_utils::parseBoolValue("invalid", true));
    EXPECT_FALSE(config_utils::parseBoolValue("invalid", false));
}

TEST_F(ConfigUtilsTest, IntValueParsing) {
    EXPECT_EQ(config_utils::parseIntValue("42"), 42);
    EXPECT_EQ(config_utils::parseIntValue("-100"), -100);
    EXPECT_EQ(config_utils::parseIntValue("0"), 0);

    // Default values
    EXPECT_EQ(config_utils::parseIntValue("invalid", 10), 10);
    EXPECT_EQ(config_utils::parseIntValue("abc", -1), -1);
}

TEST_F(ConfigUtilsTest, DoubleValueParsing) {
    EXPECT_DOUBLE_EQ(config_utils::parseDoubleValue("3.14"), 3.14);
    EXPECT_DOUBLE_EQ(config_utils::parseDoubleValue("-2.718"), -2.718);
    EXPECT_DOUBLE_EQ(config_utils::parseDoubleValue("0.0"), 0.0);

    // Default values
    EXPECT_DOUBLE_EQ(config_utils::parseDoubleValue("invalid", 1.0), 1.0);
    EXPECT_DOUBLE_EQ(config_utils::parseDoubleValue("abc", -1.0), -1.0);
}

TEST_F(ConfigUtilsTest, ConfigPathValidation) {
    // This would require creating actual test files
    // For now, test with non-existent paths
    EXPECT_FALSE(config_utils::isValidConfigPath("/nonexistent/path/config.json"));
}

// Test ConfigCompatibilitySystem
class ConfigCompatibilitySystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        system_ = &ConfigCompatibilitySystem::getInstance();
        EXPECT_TRUE(system_->initialize());
    }

    ConfigCompatibilitySystem* system_;
};

TEST_F(ConfigCompatibilitySystemTest, SingletonPattern) {
    ConfigCompatibilitySystem& system1 = ConfigCompatibilitySystem::getInstance();
    ConfigCompatibilitySystem& system2 = ConfigCompatibilitySystem::getInstance();

    EXPECT_EQ(&system1, &system2);
}

TEST_F(ConfigCompatibilitySystemTest, SystemInitialization) {
    EXPECT_TRUE(system_ != nullptr);

    auto manager = system_->getCompatibilityManager();
    EXPECT_TRUE(manager != nullptr);
}

TEST_F(ConfigCompatibilitySystemTest, SystemReportGeneration) {
    std::string report = system_->generateSystemReport();

    EXPECT_FALSE(report.empty());
    EXPECT_NE(report.find("Configuration Compatibility System Report"), std::string::npos);
    EXPECT_NE(report.find("INITIALIZED"), std::string::npos);
    EXPECT_NE(report.find("Supported Formats:"), std::string::npos);
}

// Integration tests
class ConfigCompatibilityIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = std::filesystem::temp_directory_path() / "config_integration_test";
        std::filesystem::create_directories(test_dir_);

        manager_ = std::make_unique<ConfigCompatibilityManager>();
        EXPECT_TRUE(manager_->initialize());
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
    }

    void CreateComplexLegacyConfig(const std::string& filename) {
        std::ofstream file(test_dir_ / filename);
        file << "# Puzzle71Solver Complex Configuration\n";
        file << "# Legacy V2 format with multiple sections\n\n";
        file << "[gpu]\n";
        file << "threads=256\n";
        file << "blocks=100\n";
        file << "device=0\n";
        file << "memory=4096\n\n";
        file << "[processing]\n";
        file << "batch=1000\n";
        file << "range=1000:2000\n";
        file << "algorithm=secp256k1\n\n";
        file << "[output]\n";
        file << "targets=addresses.txt\n";
        file << "results=results.txt\n";
        file << "verbose=true\n";
        file << "debug=false\n";
        file << "log_level=info\n\n";
        file << "[network]\n";
        file << "host=localhost\n";
        file << "port=8080\n";
        file << "timeout=30\n";
        file.close();
    }

    std::filesystem::path test_dir_;
    std::unique_ptr<ConfigCompatibilityManager> manager_;
};

TEST_F(ConfigCompatibilityIntegrationTest, EndToEndLegacyToModernMigration) {
    // Create complex legacy configuration
    CreateComplexLegacyConfig("complex.conf");

    std::string source_file = (test_dir_ / "complex.conf").string();

    // Load and validate configuration
    auto load_result = manager_->loadConfiguration(source_file);
    ASSERT_TRUE(load_result.success);
    EXPECT_EQ(load_result.detected_format, ConfigFormat::LEGACY_V2);

    // Check compatibility
    auto compatibility_report = manager_->checkCompatibility(source_file);
    EXPECT_TRUE(compatibility_report.validation_result.is_valid);
    EXPECT_NE(compatibility_report.current_format, ConfigFormat::JSON_V2);

    // Perform migration
    std::string target_file = (test_dir_ / "complex_migrated.json").string();
    auto migration_result = manager_->migrateConfigurationFile(
        source_file,
        target_file,
        ConfigFormat::JSON_V2
    );

    ASSERT_TRUE(migration_result.success);
    EXPECT_TRUE(std::filesystem::exists(target_file));

    // Load migrated configuration
    auto reload_result = manager_->loadConfiguration(target_file);
    ASSERT_TRUE(reload_result.success);
    EXPECT_EQ(reload_result.detected_format, ConfigFormat::JSON_V2);

    // Verify all sections were migrated
    std::vector<std::string> expected_sections = {"gpu", "processing", "output", "network"};
    for (const auto& section_name : expected_sections) {
        auto section = reload_result.sections.find(section_name);
        EXPECT_NE(section, reload_result.sections.end()) << "Section not found: " << section_name;
    }

    // Verify specific parameters were migrated correctly
    auto gpu_section = reload_result.sections.find("gpu");
    if (gpu_section != reload_result.sections.end()) {
        bool found_threads = false;
        bool found_blocks = false;

        for (const auto& param : gpu_section->second.parameters) {
            if (param.name == "threads") {
                found_threads = true;
                EXPECT_EQ(param.getValueAsString(), "256");
            }
            if (param.name == "blocks") {
                found_blocks = true;
                EXPECT_EQ(param.getValueAsString(), "100");
            }
        }

        EXPECT_TRUE(found_threads);
        EXPECT_TRUE(found_blocks);
    }
}

TEST_F(ConfigCompatibilityIntegrationTest, MultiFormatRoundTrip) {
    CreateComplexLegacyConfig("roundtrip.conf");

    std::string original_file = (test_dir_ / "roundtrip.conf").string();
    std::string json_file = (test_dir_ / "roundtrip.json").string();
    std::string yaml_file = (test_dir_ / "roundtrip.yaml").string();
    std::string toml_file = (test_dir_ / "roundtrip.toml").string();

    // Load original configuration
    auto original_result = manager_->loadConfiguration(original_file);
    ASSERT_TRUE(original_result.success);

    // Convert to JSON
    auto json_migration = manager_->migrateConfigurationFile(
        original_file,
        json_file,
        ConfigFormat::JSON_V2
    );
    ASSERT_TRUE(json_migration.success);

    // Convert JSON to YAML
    auto json_load = manager_->loadConfiguration(json_file);
    ASSERT_TRUE(json_load.success);
    ASSERT_TRUE(manager_->saveConfiguration(json_load.sections, yaml_file, ConfigFormat::YAML_V1));

    // Convert YAML to TOML
    auto yaml_load = manager_->loadConfiguration(yaml_file);
    ASSERT_TRUE(yaml_load.success);
    ASSERT_TRUE(manager_->saveConfiguration(yaml_load.sections, toml_file, ConfigFormat::TOML_V1));

    // Load final TOML and compare with original
    auto final_result = manager_->loadConfiguration(toml_file);
    ASSERT_TRUE(final_result.success);

    // Compare sections count
    EXPECT_EQ(original_result.sections.size(), final_result.sections.size());

    // Compare key parameters
    std::vector<std::pair<std::string, std::string>> key_params = {
        {"gpu", "threads"}, {"gpu", "blocks"}, {"processing", "batch"}, {"output", "verbose"}
    };

    for (const auto& [section_name, param_name] : key_params) {
        auto orig_section = original_result.sections.find(section_name);
        auto final_section = final_result.sections.find(section_name);

        ASSERT_NE(orig_section, original_result.sections.end());
        ASSERT_NE(final_section, final_result.sections.end());

        auto orig_param = orig_section->second.findParameter(param_name);
        auto final_param = final_section->second.findParameter(param_name);

        ASSERT_TRUE(orig_param.has_value());
        ASSERT_TRUE(final_param.has_value());

        EXPECT_EQ(orig_param->getValueAsString(), final_param->getValueAsString())
            << "Parameter value mismatch: " << section_name << "." << param_name;
    }
}

TEST_F(ConfigCompatibilityIntegrationTest, ErrorHandlingAndRecovery) {
    // Create invalid configuration file
    std::string invalid_file = (test_dir_ / "invalid.json").string();
    std::ofstream file(invalid_file);
    file << "{ invalid json content }\n";
    file.close();

    // Try to load invalid configuration
    auto load_result = manager_->loadConfiguration(invalid_file);
    EXPECT_FALSE(load_result.success);
    EXPECT_FALSE(load_result.validation_result.is_valid);
    EXPECT_FALSE(load_result.validation_result.errors.empty());

    // Try to validate invalid file
    auto validation_result = manager_->validateConfigurationFile(invalid_file);
    EXPECT_FALSE(validation_result.is_valid);
    EXPECT_FALSE(validation_result.errors.empty());

    // Check compatibility report
    auto compatibility_report = manager_->checkCompatibility(invalid_file);
    EXPECT_FALSE(compatibility_report.validation_result.is_valid);
    EXPECT_FALSE(compatibility_report.is_compatible);

    // Migration should fail for invalid file
    std::string target_file = (test_dir_ / "migration_target.json").string();
    auto migration_result = manager_->migrateConfigurationFile(
        invalid_file,
        target_file,
        ConfigFormat::JSON_V2
    );

    EXPECT_FALSE(migration_result.success);
    EXPECT_FALSE(migration_result.issues_encountered.empty());
}

TEST_F(ConfigCompatibilityIntegrationTest, PerformanceWithLargeConfigurations) {
    // Create a large configuration file
    std::string large_config_file = (test_dir_ / "large.conf").string();
    std::ofstream file(large_config_file);

    file << "[gpu]\n";
    for (int i = 0; i < 1000; ++i) {
        file << "param" << i << "=" << i << "\n";
    }

    file << "[processing]\n";
    for (int i = 0; i < 1000; ++i) {
        file << "process_param" << i << "=" << (i * 2) << "\n";
    }

    file.close();

    // Measure loading time
    auto start_time = std::chrono::high_resolution_clock::now();

    auto load_result = manager_->loadConfiguration(large_config_file);

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_TRUE(load_result.success);
    EXPECT_LT(duration.count(), 1000); // Should load within 1 second

    // Check that all parameters were loaded
    EXPECT_EQ(load_result.sections.size(), 2);

    auto gpu_section = load_result.sections.find("gpu");
    if (gpu_section != load_result.sections.end()) {
        EXPECT_EQ(gpu_section->second.parameters.size(), 1000);
    }

    auto processing_section = load_result.sections.find("processing");
    if (processing_section != load_result.sections.end()) {
        EXPECT_EQ(processing_section->second.parameters.size(), 1000);
    }
}

} // namespace puzzle71::compatibility
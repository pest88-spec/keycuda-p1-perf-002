/**
 * @file test_terminology_validator.cpp
 * @brief Unit tests for terminology consistency validator
 *
 * Tests constitutional compliance validation for "unified modules"
 * terminology consistency across codebase modules.
 *
 * @author Puzzle71Solver CUDA Team
 * @date 2025-10-19
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>

#include "src/KeyhuntCore/validation/terminology_validator.h"

using namespace keyhunt::validation;
using namespace ::testing;

class TerminologyValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary test directory
        test_dir_ = std::filesystem::temp_directory_path() / "terminology_validator_test";
        std::filesystem::create_directories(test_dir_);

        // Setup validator configuration
        config_.enable_logging = false;  // Reduce test output
        config_.generate_corrections = true;
        config_.fail_fast = false;
        config_.check_comments = true;
        config_.check_strings = true;

        validator_ = std::make_unique<TerminologyValidator>(config_);
    }

    void TearDown() override {
        // Cleanup test directory
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    // Helper to create test files
    std::string createTestFile(const std::string& filename,
                              const std::string& content) {
        std::filesystem::path file_path = test_dir_ / filename;
        std::filesystem::create_directories(file_path.parent_path());

        std::ofstream file(file_path);
        file << content;
        file.close();

        return file_path.string();
    }

    // Helper to create test directory structure
    void createTestDirectoryStructure() {
        // Create subdirectories
        std::filesystem::create_directories(test_dir_ / "src");
        std::filesystem::create_directories(test_dir_ / "docs");
        std::filesystem::create_directories(test_dir_ / "tests");
        std::filesystem::create_directories(test_dir_ / "build");
        std::filesystem::create_directories(test_dir_ / "external");
    }

    std::filesystem::path test_dir_;
    TerminologyValidatorConfig config_;
    std::unique_ptr<TerminologyValidator> validator_;
};

// Test basic validator initialization
TEST_F(TerminologyValidatorTest, Initialization) {
    EXPECT_NE(validator_, nullptr);
    EXPECT_TRUE(validator_->getMetrics().total_files_scanned == 0);
}

// Test validation of clean file (no violations)
TEST_F(TerminologyValidatorTest, CleanFileValidation) {
    const std::string content = R"(
#include <iostream>

// This file uses proper terminology
// We work with unified modules throughout
int main() {
    // Unified modules provide consistent functionality
    std::cout << "Using unified modules approach" << std::endl;
    return 0;
}
)";

    std::string file_path = createTestFile("clean.cpp", content);
    auto result = validator_->validateFile(file_path);

    EXPECT_TRUE(result.success) << "Clean file should pass validation";
    EXPECT_TRUE(result.violations.empty()) << "No violations should be found";
    EXPECT_EQ(result.file_path, file_path);
}

// Test detection of uppercase violations
TEST_F(TerminologyValidatorTest, DetectUppercaseViolations) {
    const std::string content = R"(
#include <iostream>

// This file has violations
// UNIFIED_MODULES should be "unified modules"
int main() {
    // UnifiedModules is incorrect
    std::cout << "Using UnifiedModules approach" << std::endl;
    return 0;
}
)";

    std::string file_path = createTestFile("violations.cpp", content);
    auto result = validator_->validateFile(file_path);

    EXPECT_FALSE(result.success) << "File with violations should fail validation";
    EXPECT_EQ(result.violations.size(), 2) << "Should find 2 violations";

    // Check first violation (UNIFIED_MODULES)
    EXPECT_EQ(result.violations[0].line_number, 4);
    EXPECT_EQ(result.violations[0].invalid_text, "UNIFIED_MODULES");
    EXPECT_EQ(result.violations[0].corrected_text, "unified modules");
    EXPECT_EQ(result.violations[0].severity, ViolationSeverity::ERROR);

    // Check second violation (UnifiedModules)
    EXPECT_EQ(result.violations[1].line_number, 7);
    EXPECT_EQ(result.violations[1].invalid_text, "UnifiedModules");
    EXPECT_EQ(result.violations[1].corrected_text, "unified modules");
}

// Test detection of underscore violations
TEST_F(TerminologyValidatorTest, DetectUnderscoreViolations) {
    const std::string content = R"(
// File with underscore violations
// unified_modules should be "unified modules"
// Unified_Modules should be "unified modules"

void function() {
    int unified_modules_var = 0;  // Variable names are OK
}
)";

    std::string file_path = createTestFile("underscores.cpp", content);
    auto result = validator_->validateFile(file_path);

    EXPECT_FALSE(result.success) << "File with violations should fail validation";
    EXPECT_EQ(result.violations.size(), 2) << "Should find 2 violations";

    // Check that variable names are not flagged
    for (const auto& violation : result.violations) {
        EXPECT_NE(violation.invalid_text, "unified_modules_var")
            << "Variable names should not be flagged as violations";
    }
}

// Test detection of hyphen violations
TEST_F(TerminologyValidatorTest, DetectHyphenViolations) {
    const std::string content = R"(
// File with hyphen violations
// unified-modules should be "unified modules"
// Unified-Modules should be "unified modules"
)";

    std::string file_path = createTestFile("hyphens.cpp", content);
    auto result = validator_->validateFile(file_path);

    EXPECT_FALSE(result.success) << "File with violations should fail validation";
    EXPECT_EQ(result.violations.size(), 2) << "Should find 2 violations";

    for (const auto& violation : result.violations) {
        EXPECT_EQ(violation.corrected_text, "unified modules");
    }
}

// Test that comments are skipped when configured
TEST_F(TerminologyValidatorTest, SkipCommentsWhenConfigured) {
    config_.check_comments = false;
    validator_ = std::make_unique<TerminologyValidator>(config_);

    const std::string content = R"(
// This comment has Unified_Modules but should be ignored
int main() {
    return 0;  // UNIFIED_MODULES in comment
}
)";

    std::string file_path = createTestFile("comments.cpp", content);
    auto result = validator_->validateFile(file_path);

    EXPECT_TRUE(result.success) << "Comments should be ignored when configured";
    EXPECT_TRUE(result.violations.empty()) << "No violations should be found";
}

// Test that string literals are skipped when configured
TEST_F(TerminologyValidatorTest, SkipStringLiteralsWhenConfigured) {
    config_.check_strings = false;
    validator_ = std::make_unique<TerminologyValidator>(config_);

    const std::string content = R"(
#include <iostream>
int main() {
    std::cout << "Unified_Modules in string" << std::endl;
    std::string text = "UNIFIED_MODULES text";
    return 0;
}
)";

    std::string file_path = createTestFile("strings.cpp", content);
    auto result = validator_->validateFile(file_path);

    EXPECT_TRUE(result.success) << "String literals should be ignored when configured";
    EXPECT_TRUE(result.violations.empty()) << "No violations should be found";
}

// Test project-level validation
TEST_F(TerminologyValidatorTest, ProjectValidation) {
    createTestDirectoryStructure();

    // Create multiple test files
    createTestFile("src/good.cpp", R"(
// Proper terminology
// Using unified modules consistently
int main() { return 0; }
)");

    createTestFile("src/bad.cpp", R"(
// Bad terminology
// Using UNIFIED_MODULES incorrectly
void func() { }
)");

    createTestFile("docs/readme.md", R"(
# Documentation
This project uses UnifiedModules for consistency.
)");

    createTestFile("tests/test.cpp", R"(
// Test file
// unified-modules used incorrectly
TEST() { EXPECT_TRUE(true); }
)");

    auto result = validator_->validateProject(test_dir_.string());

    EXPECT_FALSE(result.success) << "Project with violations should fail validation";
    EXPECT_EQ(result.file_results.size(), 4) << "Should process 4 files";

    // Count violations
    int total_violations = 0;
    for (const auto& file_result : result.file_results) {
        total_violations += file_result.violations.size();
    }
    EXPECT_EQ(total_violations, 3) << "Should find 3 total violations";

    // Check compliance score calculation
    EXPECT_LT(result.compliance_score, 100.0) << "Compliance score should be < 100%";
    EXPECT_GT(result.compliance_score, 0.0) << "Compliance score should be > 0%";
}

// Test correction generation
TEST_F(TerminologyValidatorTest, CorrectionGeneration) {
    const std::string content = R"(
// File with violations
// UNIFIED_MODULES and Unified_Modules
int main() { return 0; }
)";

    std::string file_path = createTestFile("corrections.cpp", content);
    auto result = validator_->validateProject(test_dir_.string());

    EXPECT_FALSE(result.success) << "File with violations should fail validation";
    EXPECT_FALSE(result.corrections.empty()) << "Corrections should be generated";

    // Check correction details
    for (const auto& correction : result.corrections) {
        EXPECT_EQ(correction.file_path, file_path);
        EXPECT_EQ(correction.corrected_text, "unified modules");
        EXPECT_FALSE(correction.original_text.empty());
        EXPECT_GT(correction.line_number, 0);
    }
}

// Test correction application
TEST_F(TerminologyValidatorTest, CorrectionApplication) {
    const std::string original_content = R"(
// File with violations
// UNIFIED_MODULES should be corrected
)";

    std::string file_path = createTestFile("apply_corrections.cpp", original_content);
    auto result = validator_->validateProject(test_dir_.string());

    EXPECT_FALSE(result.success) << "File with violations should fail validation";
    EXPECT_FALSE(result.corrections.empty()) << "Corrections should be generated";

    // Apply corrections
    bool applied = validator_->applyCorrections(result.corrections);
    EXPECT_TRUE(applied) << "Corrections should be applied successfully";

    // Verify corrections were applied
    std::ifstream file(file_path);
    std::string corrected_content;
    std::string line;
    while (std::getline(file, line)) {
        corrected_content += line + "\n";
    }
    file.close();

    EXPECT_TRUE(corrected_content.find("unified modules") != std::string::npos)
        << "Corrected text should be present";
    EXPECT_TRUE(corrected_content.find("UNIFIED_MODULES") == std::string::npos)
        << "Original violation should be removed";
}

// Test constitution compliance validation
TEST_F(TerminologyValidatorTest, ConstitutionCompliance) {
    // Test compliant case
    createTestFile("compliant.cpp", R"(
// Proper terminology
// Using unified modules consistently
)");

    auto result = validator_->validateProject(test_dir_.string());
    bool compliant = validator_->validateConstitutionCompliance(result);

    EXPECT_TRUE(compliant) << "Compliant project should pass constitution validation";
    EXPECT_EQ(result.compliance_score, 100.0) << "Compliant project should have 100% score";

    // Test non-compliant case
    createTestFile("non_compliant.cpp", R"(
// Bad terminology
// Using UNIFIED_MODULES incorrectly
)");

    result = validator_->validateProject(test_dir_.string());
    compliant = validator_->validateConstitutionCompliance(result);

    EXPECT_FALSE(compliant) << "Non-compliant project should fail constitution validation";
    EXPECT_LT(result.compliance_score, 100.0) << "Non-compliant project should have < 100% score";
}

// Test compliance report generation
TEST_F(TerminologyValidatorTest, ComplianceReportGeneration) {
    createTestFile("report_test.cpp", R"(
// Mixed content
// Good: unified modules
// Bad: UNIFIED_MODULES
)");

    auto result = validator_->validateProject(test_dir_.string());
    std::string report = validator_->generateComplianceReport(result);

    // Check report contains expected sections
    EXPECT_TRUE(report.find("# Constitution Compliance Report") != std::string::npos)
        << "Report should have title";
    EXPECT_TRUE(report.find("## Summary Statistics") != std::string::npos)
        << "Report should have summary section";
    EXPECT_TRUE(report.find("## Constitution Compliance") != std::string::npos)
        << "Report should have constitution section";
    EXPECT_TRUE(report.find("## Violations by File") != std::string::npos)
        << "Report should have violations section";
    EXPECT_TRUE(report.find("## Recommendations") != std::string::npos)
        << "Report should have recommendations section";

    // Check report contains specific content
    EXPECT_TRUE(report.find("UNIFIED_MODULES") != std::string::npos)
        << "Report should mention specific violation";
    EXPECT_TRUE(report.find("unified modules") != std::string::npos)
        << "Report should mention correction";
}

// Test file filtering
TEST_F(TerminologyValidatorTest, FileFiltering) {
    createTestDirectoryStructure();

    // Create files in different directories
    createTestFile("src/test.cpp", R"(
// UNIFIED_MODULES in source
)");
    createTestFile("build/test.cpp", R"(
// UNIFIED_MODULES in build directory (should be skipped)
)");
    createTestFile("external/test.cpp", R"(
// UNIFIED_MODULES in external directory (should be skipped)
)");

    auto result = validator_->validateProject(test_dir_.string());

    // Should only process files not in skip directories
    EXPECT_EQ(result.file_results.size(), 1) << "Should only process 1 file";
    EXPECT_EQ(result.file_results[0].file_path, (test_dir_ / "src" / "test.cpp").string())
        << "Should process only src directory file";
}

// Test quick validation static method
TEST_F(TerminologyValidatorTest, QuickValidation) {
    // Test compliant case
    createTestFile("quick_compliant.cpp", R"(
// Using unified modules properly
)");

    bool compliant = TerminologyValidator::quickValidate(test_dir_.string());
    EXPECT_TRUE(compliant) << "Compliant project should pass quick validation";

    // Test non-compliant case
    createTestFile("quick_non_compliant.cpp", R"(
// Using UNIFIED_MODULES incorrectly
)");

    compliant = TerminologyValidator::quickValidate(test_dir_.string());
    EXPECT_FALSE(compliant) << "Non-compliant project should fail quick validation";
}

// Test validate and fix static method
TEST_F(TerminologyValidatorTest, ValidateAndFix) {
    const std::string content_with_violations = R"(
// Using UNIFIED_MODULES incorrectly
)";

    std::string file_path = createTestFile("fix_test.cpp", content_with_violations);

    bool fixed = TerminologyValidator::validateAndFix(test_dir_.string());
    EXPECT_TRUE(fixed) << "Should be able to fix violations automatically";

    // Verify the fix was applied
    std::ifstream file(file_path);
    std::string fixed_content;
    std::string line;
    while (std::getline(file, line)) {
        fixed_content += line + "\n";
    }
    file.close();

    EXPECT_TRUE(fixed_content.find("unified modules") != std::string::npos)
        << "Fixed content should contain corrected terminology";
    EXPECT_TRUE(fixed_content.find("UNIFIED_MODULES") == std::string::npos)
        << "Fixed content should not contain violations";
}

// Test severity determination
TEST_F(TerminologyValidatorTest, SeverityDetermination) {
    const std::string content = R"(
// Different severity violations
// ERROR: UNIFIED_MODULES (all caps)
// WARNING: Unified_Modules (underscores)
// INFO: unified-modules (hyphens)
)";

    std::string file_path = createTestFile("severity_test.cpp", content);
    auto result = validator_->validateFile(file_path);

    EXPECT_FALSE(result.success) << "File with violations should fail validation";
    EXPECT_EQ(result.violations.size(), 3) << "Should find 3 violations";

    // Check severity levels
    bool found_error = false, found_warning = false, found_info = false;
    for (const auto& violation : result.violations) {
        if (violation.severity == ViolationSeverity::ERROR) found_error = true;
        if (violation.severity == ViolationSeverity::WARNING) found_warning = true;
        if (violation.severity == ViolationSeverity::INFO) found_info = true;
    }

    EXPECT_TRUE(found_error) << "Should find ERROR severity violation";
    EXPECT_TRUE(found_warning) << "Should find WARNING severity violation";
    EXPECT_TRUE(found_info) << "Should find INFO severity violation";
}

// Test metrics tracking
TEST_F(TerminologyValidatorTest, MetricsTracking) {
    // Create multiple files with violations
    createTestFile("metrics1.cpp", R"(
// UNIFIED_MODULES violation 1
)");
    createTestFile("metrics2.cpp", R"(
// UNIFIED_MODULES violation 2
)");
    createTestFile("metrics3.cpp", R"(
// No violations here
)");

    auto result = validator_->validateProject(test_dir_.string());
    const auto& metrics = validator_->getMetrics();

    EXPECT_EQ(metrics.total_files_scanned, 3) << "Should scan 3 files";
    EXPECT_EQ(metrics.files_with_violations, 2) << "Should find violations in 2 files";
    EXPECT_EQ(metrics.total_violations, 2) << "Should find 2 total violations";
    EXPECT_GT(metrics.total_corrections, 0) << "Should generate corrections";
}

// Test edge cases
TEST_F(TerminologyValidatorTest, EdgeCases) {
    // Test empty file
    std::string empty_file = createTestFile("empty.cpp", "");
    auto result = validator_->validateFile(empty_file);
    EXPECT_TRUE(result.success) << "Empty file should pass validation";

    // Test file with only allowed terminology
    std::string allowed_file = createTestFile("allowed.cpp", R"(
// unified modules
// unified module
// unified_modules
// unified-module
)");
    result = validator_->validateFile(allowed_file);
    EXPECT_TRUE(result.success) << "File with only allowed terminology should pass validation";

    // Test file with mixed case in comments (should be flagged if comments are checked)
    std::string mixed_file = createTestFile("mixed.cpp", R"(
// This is fine: unified modules
// This is not: UnifiedModules
int main() { return 0; }  // This is not: UNIFIED_MODULES
)");
    result = validator_->validateFile(mixed_file);
    EXPECT_FALSE(result.success) << "Mixed file should fail validation when comments are checked";
}

// Performance test with larger codebase
TEST_F(TerminologyValidatorTest, PerformanceTest) {
    // Create multiple files to test performance
    for (int i = 0; i < 10; ++i) {
        std::ostringstream filename;
        filename << "perf_test_" << i << ".cpp";

        std::ostringstream content;
        content << "// File " << i << "\n";
        content << "// Some files have UNIFIED_MODULES violations\n";
        content << "void func" << i << "() { }\n";

        createTestFile(filename.str(), content.str());
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    auto result = validator_->validateProject(test_dir_.string());
    auto end_time = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    // Should complete reasonably quickly (adjust threshold as needed)
    EXPECT_LT(duration.count(), 5000) << "Validation should complete in under 5 seconds";
    EXPECT_EQ(result.file_results.size(), 10) << "Should process all 10 files";
    EXPECT_FALSE(result.success) << "Should detect violations";
}

// Integration test with real project structure
TEST_F(TerminologyValidatorTest, RealProjectStructureIntegration) {
    createTestDirectoryStructure();

    // Create realistic project files
    createTestFile("src/main.cpp", R"(
#include <iostream>
// Main entry point using unified modules
int main() {
    std::cout << "Welcome to UnifiedModules system" << std::endl;
    return 0;
}
)");

    createTestFile("src/unified_modules.h", R"(
// Header for UNIFIED_MODULES functionality
#pragma once
class UnifiedModules {
public:
    void process();
};
)");

    createTestFile("docs/README.md", R"(
# Project Documentation

This project uses unified-modules for consistency.

## Architecture

We implement UNIFIED_MODULES pattern for code reuse.
)");

    createTestFile("tests/test_unified_modules.cpp", R"(
#include <gtest/gtest.h>
// Test cases for unified-modules functionality
TEST(UnifiedModulesTest, BasicFunctionality) {
    EXPECT_TRUE(true);
}
)");

    auto result = validator_->validateProject(test_dir_.string());

    // Should find violations across multiple files
    EXPECT_FALSE(result.success) << "Real project with violations should fail validation";

    int total_violations = 0;
    for (const auto& file_result : result.file_results) {
        total_violations += file_result.violations.size();
    }
    EXPECT_GT(total_violations, 0) << "Should find multiple violations";

    // Generate and verify compliance report
    std::string report = validator_->generateComplianceReport(result);
    EXPECT_FALSE(report.empty()) << "Should generate compliance report";
    EXPECT_TRUE(report.find("UNIFIED_MODULES") != std::string::npos)
        << "Report should mention violations";
    EXPECT_TRUE(report.find("unified modules") != std::string::npos)
        << "Report should mention corrections";
}
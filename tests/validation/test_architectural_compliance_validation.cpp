// Puzzle71 Technical Debt Repair - Architectural Compliance Validation Tests
// Phase 5: User Story 4 - Complete System Migration and Quality Assurance
// Task: T075 - Confirm architectural compliance tests show zero code duplication
// TDD test implementation for architectural compliance validation with zero code duplication

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <algorithm>

#include "src/KeyhuntCore/validation/architectural_compliance_validator.h"

using namespace puzzle71::validation;
using ::testing::Return;
using ::testing::_;
using ::testing::Contains;
using ::testing::Not;
using ::testing::UnorderedElementsAre;

class ArchitecturalComplianceValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        validator_ = std::make_unique<ArchitecturalComplianceValidator>();
        test_directory_ = std::filesystem::temp_directory_path() / "puzzle71_architectural_test";
        std::filesystem::create_directories(test_directory_);
    }

    void TearDown() override {
        validator_.reset();
        std::filesystem::remove_all(test_directory_);
    }

    void createTestSourceFile(const std::string& filename, const std::string& content) {
        std::ofstream file(test_directory_ / filename);
        file << content;
        file.close();
    }

    void createDuplicateFiles() {
        // Create exact duplicate files
        createTestSourceFile("duplicate1.cpp", R"(
#include <iostream>
#include <vector>

class Calculator {
public:
    int add(int a, int b) {
        return a + b;
    }

    int multiply(int a, int b) {
        return a * b;
    }
};
)");

        createTestSourceFile("duplicate2.cpp", R"(
#include <iostream>
#include <vector>

class Calculator {
public:
    int add(int a, int b) {
        return a + b;
    }

    int multiply(int a, int b) {
        return a * b;
    }
};
)");

        // Create structural duplicate
        createTestSourceFile("structural1.cpp", R"(
#include <iostream>

class Processor {
public:
    int process(int a, int b) {
        int temp = a + b;
        return temp * 2;
    }
};
)");

        createTestSourceFile("structural2.cpp", R"(
#include <iostream>

class Handler {
public:
    int handle(int x, int y) {
        int result = x + y;
        return result * 2;
    }
};
)");

        // Create adapter pattern file
        createTestSourceFile("adapter.cpp", R"(
#include <iostream>

// Legacy interface
class LegacySystem {
public:
    void oldOperation(int param) {
        std::cout << "Legacy operation: " << param << std::endl;
    }
};

// New interface
class ModernSystem {
public:
    void newOperation(int value) {
        std::cout << "Modern operation: " << value << std::endl;
    }
};

// Adapter pattern implementation
class LegacyAdapter : public ModernSystem {
private:
    LegacySystem* legacy_;

public:
    LegacyAdapter(LegacySystem* legacy) : legacy_(legacy) {}

    void newOperation(int value) override {
        // Adapting the interface
        int adapted_param = value * 2;
        legacy_->oldOperation(adapted_param);
    }
};
)");

        // Create unique file
        createTestSourceFile("unique.cpp", R"(
#include <iostream>

class UniqueClass {
public:
    void uniqueMethod() {
        std::cout << "This is a unique implementation" << std::endl;
        for (int i = 0; i < 10; ++i) {
            std::cout << "Processing item: " << i << std::endl;
        }
    }
};
)");
    }

    std::unique_ptr<ArchitecturalComplianceValidator> validator_;
    std::filesystem::path test_directory_;
};

// Test 1: Zero Code Duplication Validation
TEST_F(ArchitecturalComplianceValidationTest, ZeroCodeDuplicationValidation) {
    // Create files with no duplication
    createTestSourceFile("unique1.cpp", R"(
#include <iostream>

class UniqueCalculator1 {
public:
    int add(int a, int b) {
        return a + b + 1; // Unique implementation
    }
};
)");

    createTestSourceFile("unique2.cpp", R"(
#include <iostream>

class UniqueCalculator2 {
public:
    int add(int a, int b) {
        return a + b + 2; // Different implementation
    }
};
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(0.0, 100.0, 0.1, 0.05, true));

    // Validate zero code duplication
    ZeroDuplicationValidationResult result;
    ASSERT_TRUE(validator_->validateZeroCodeDuplication(result));

    // Verify zero duplication
    EXPECT_TRUE(result.zero_code_duplication_achieved);
    EXPECT_EQ(result.exact_duplicates_count, 0);
    EXPECT_EQ(result.structural_duplicates_count, 0);
    EXPECT_EQ(result.logical_duplicates_count, 0);
    EXPECT_EQ(result.overall_duplication_percentage, 0.0);
}

// Test 2: Exact Duplication Detection
TEST_F(ArchitecturalComplianceValidationTest, ExactDuplicationDetection) {
    createDuplicateFiles();

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(0.0, 100.0, 0.1, 0.05, true));

    // Validate zero code duplication (should fail due to exact duplicates)
    ZeroDuplicationValidationResult result;
    ASSERT_TRUE(validator_->validateZeroCodeDuplication(result));

    // Verify exact duplication detection
    EXPECT_FALSE(result.zero_code_duplication_achieved);
    EXPECT_GT(result.exact_duplicates_count, 0);
    EXPECT_GT(result.overall_duplication_percentage, 0.0);
    EXPECT_FALSE(result.duplication_groups.empty());

    // Check for specific duplicate files
    bool found_duplicate_group = false;
    for (const auto& group : result.duplication_groups) {
        if (group.second.size() >= 2) {
            found_duplicate_group = true;
            break;
        }
    }
    EXPECT_TRUE(found_duplicate_group);
}

// Test 3: Adapter Pattern Validation
TEST_F(ArchitecturalComplianceValidationTest, AdapterPatternValidation) {
    createTestSourceFile("legacy_adapter.cpp", R"(
#include <iostream>

// Legacy system interface
class OldSystem {
public:
    void legacyMethod(int param) {
        std::cout << "Legacy: " << param << std::endl;
    }
};

// Modern system interface
class NewSystem {
public:
    virtual void modernMethod(int value) = 0;
};

// Adapter pattern implementation
class SystemAdapter : public NewSystem {
private:
    OldSystem* old_system_;

public:
    SystemAdapter(OldSystem* old) : old_system_(old) {}

    void modernMethod(int value) override {
        // Adapt interface call
        int adapted_value = value + 100;
        old_system_->legacyMethod(adapted_value);
    }
};
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(0.0, 100.0, 0.1, 0.05, true));

    // Validate adapter patterns
    ZeroDuplicationValidationResult result;
    ASSERT_TRUE(validator_->validateAdapterPatternCompliance(result));

    // Verify adapter pattern detection
    EXPECT_GT(result.adapter_pattern_instances, 0);
    EXPECT_TRUE(result.adapter_pattern_validation_passed);
    EXPECT_FALSE(result.adapter_patterns.empty());

    // Check for legacy adapter pattern
    bool found_legacy_adapter = false;
    for (const auto& pattern : result.adapter_patterns) {
        if (pattern.second == AdapterPatternType::LEGACY_ADAPTER) {
            found_legacy_adapter = true;
            break;
        }
    }
    EXPECT_TRUE(found_legacy_adapter);
}

// Test 4: All Duplications Are Adapter Patterns Validation
TEST_F(ArchitecturalComplianceValidationTest, AllDuplicationsAreAdapterPatterns) {
    // Create files with only adapter pattern duplications
    createTestSourceFile("adapter1.cpp", R"(
// Interface adapter pattern
class InterfaceAdapter {
public:
    void adapt() {
        // Common adapter logic
        transformData();
        callAdaptee();
    }
};
)");

    createTestSourceFile("adapter2.cpp", R"(
// Interface adapter pattern - similar structure but different adapter
class AnotherAdapter {
public:
    void adapt() {
        // Common adapter logic - intentional similarity
        processData();
        invokeTarget();
    }
};
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(0.0, 100.0, 0.1, 0.05, true));

    // Validate that all duplications are adapter patterns
    ZeroDuplicationValidationResult result;
    ASSERT_TRUE(validator_->validateAllDuplicationsAreAdapterPatterns(result));

    // Verify adapter pattern validation
    EXPECT_TRUE(result.all_duplications_are_adapter_patterns);
    EXPECT_EQ(result.non_adapter_duplicates, 0);
    EXPECT_GT(result.adapter_pattern_percentage, 0.0);
}

// Test 5: Structural Similarity Validation
TEST_F(ArchitecturalComplianceValidationTest, StructuralSimilarityValidation) {
    // Create files with low structural similarity
    createTestSourceFile("structurally_unique1.cpp", R"(
#include <iostream>

class DataProcessor {
public:
    void execute() {
        for (int i = 0; i < 100; ++i) {
            processData(i);
        }
    }

private:
    void processData(int value) {
        std::cout << value * 2 << std::endl;
    }
};
)");

    createTestSourceFile("structurally_unique2.cpp", R"(
#include <vector>

class StringHandler {
public:
    void manipulate() {
        std::string text = "Hello";
        for (char c : text) {
            transform(c);
        }
    }

private:
    void transform(char character) {
        char result = std::toupper(character);
        std::cout << result << std::endl;
    }
};
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(0.0, 100.0, 0.1, 0.05, true));

    // Validate structural similarity
    ZeroDuplicationValidationResult result;
    ASSERT_TRUE(validator_->validateStructuralSimilarity(result));

    // Verify structural similarity within threshold
    EXPECT_TRUE(result.structural_similarity_within_threshold);
    EXPECT_LE(result.maximum_structural_similarity, structural_similarity_threshold_);
    EXPECT_LE(result.average_structural_similarity, structural_similarity_threshold_);
}

// Test 6: Logical Similarity Validation
TEST_F(ArchitecturalComplianceValidationTest, LogicalSimilarityValidation) {
    // Create files with different logical implementations
    createTestSourceFile("logical_unique1.cpp", R"(
#include <iostream>

class MathCalculator {
public:
    int compute(int a, int b) {
        return a + b * 2 - a;
    }
};
)");

    createTestSourceFile("logical_unique2.cpp", R"(
#include <iostream>

class StringCalculator {
public:
    int calculate(const std::string& s) {
        int sum = 0;
        for (char c : s) {
            sum += c - '0';
        }
        return sum;
    }
};
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(0.0, 100.0, 0.1, 0.05, true));

    // Validate logical similarity
    ZeroDuplicationValidationResult result;
    ASSERT_TRUE(validator_->validateLogicalSimilarity(result));

    // Verify logical similarity within threshold
    EXPECT_TRUE(result.logical_similarity_within_threshold);
    EXPECT_LE(result.maximum_logical_similarity, logical_similarity_threshold_);
    EXPECT_LE(result.average_logical_similarity, logical_similarity_threshold_);
}

// Test 7: Comprehensive Architectural Compliance Validation
TEST_F(ArchitecturalComplianceValidationTest, ComprehensiveArchitecturalComplianceValidation) {
    // Create a mix of files: some unique, some adapter patterns
    createTestSourceFile("unique_file.cpp", R"(
#include <iostream>

class UniqueImplementation {
public:
    void process() {
        std::cout << "Unique processing logic" << std::endl;
    }
};
)");

    createTestSourceFile("adapter_pattern.cpp", R"(
#include <iostream>

class InterfaceAdapter {
public:
    void adapt() {
        // Adapter pattern implementation
        transformData();
        callTargetSystem();
    }
};
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(0.0, 100.0, 0.1, 0.05, true));

    // Perform comprehensive validation
    ArchitecturalComplianceResult result;
    ASSERT_TRUE(validator_->validateArchitecturalComplianceWithZeroDuplication(result));

    // Verify comprehensive compliance
    EXPECT_TRUE(result.architectural_principles_met);
    EXPECT_TRUE(result.design_patterns_compliant);
    EXPECT_GE(result.overall_architectural_score, 95.0);
}

// Test 8: Code Block Extraction and Analysis
TEST_F(ArchitecturalComplianceValidationTest, CodeBlockExtractionAndAnalysis) {
    // Create a file with multiple code blocks
    createTestSourceFile("multi_block.cpp", R"(
#include <iostream>

class FirstClass {
public:
    void method1() {
        std::cout << "Method 1" << std::endl;
    }
};

class SecondClass {
public:
    void method2() {
        std::cout << "Method 2" << std::endl;
    }
};

void standaloneFunction() {
    std::cout << "Standalone function" << std::endl;
}
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));

    // Extract code blocks
    std::vector<CodeBlockAnalysisResult> blocks;
    ASSERT_TRUE(validator_->extractCodeBlocks((test_directory_ / "multi_block.cpp").string(), blocks));

    // Verify code block extraction
    EXPECT_GT(blocks.size(), 0);

    // Check that blocks have expected properties
    for (const auto& block : blocks) {
        EXPECT_FALSE(block.content.empty());
        EXPECT_GT(block.start_line, 0);
        EXPECT_GE(block.end_line, block.start_line);
        EXPECT_FALSE(block.hash_signature.empty());
    }
}

// Test 9: Token Analysis and Normalization
TEST_F(ArchitecturalComplianceValidationTest, TokenAnalysisAndNormalization) {
    // Create test file with various code elements
    createTestSourceFile("token_test.cpp", R"(
#include <iostream>
#include <vector>

class TokenTest {
private:
    int variable1;
    std::string variable2;

public:
    void testMethod(int param) {
        std::vector<int> vec;
        for (int i = 0; i < param; ++i) {
            vec.push_back(i * 2);
        }
    }
};
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));

    // Extract code blocks and analyze tokens
    std::vector<CodeBlockAnalysisResult> blocks;
    ASSERT_TRUE(validator_->extractCodeBlocks((test_directory_ / "token_test.cpp").string(), blocks));

    ASSERT_GT(blocks.size(), 0);
    const auto& block = blocks[0];

    // Verify token extraction
    EXPECT_GT(block.tokens.size(), 0);
    EXPECT_GT(block.function_names.size(), 0);
    EXPECT_GT(block.variable_names.size(), 0);

    // Verify normalization
    CodeBlockAnalysisResult normalized;
    ASSERT_TRUE(validator_->normalizeCodeBlock(block, normalized));
    EXPECT_FALSE(normalized.normalized_content.empty());
    EXPECT_GT(normalized.tokens.size(), 0);
}

// Test 10: Similarity Calculation Accuracy
TEST_F(ArchitecturalComplianceValidationTest, SimilarityCalculationAccuracy) {
    // Create similar but not identical files
    createTestSourceFile("similar1.cpp", R"(
class SimilarClass1 {
public:
    int calculate(int a, int b) {
        return a + b + 1;
    }
};
)");

    createTestSourceFile("similar2.cpp", R"(
class SimilarClass2 {
public:
    int compute(int x, int y) {
        return x + y + 2;
    }
};
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));

    // Extract code blocks
    std::vector<CodeBlockAnalysisResult> blocks1, blocks2;
    ASSERT_TRUE(validator_->extractCodeBlocks((test_directory_ / "similar1.cpp").string(), blocks1));
    ASSERT_TRUE(validator_->extractCodeBlocks((test_directory_ / "similar2.cpp").string(), blocks2));

    ASSERT_GT(blocks1.size(), 0);
    ASSERT_GT(blocks2.size(), 0);

    // Calculate similarities
    double structural_similarity, logical_similarity;
    ASSERT_TRUE(validator_->calculateStructuralSimilarity(blocks1[0], blocks2[0], structural_similarity));
    ASSERT_TRUE(validator_->calculateLogicalSimilarity(blocks1[0], blocks2[0], logical_similarity));

    // Verify similarity calculations
    EXPECT_GE(structural_similarity, 0.0);
    EXPECT_LE(structural_similarity, 1.0);
    EXPECT_GE(logical_similarity, 0.0);
    EXPECT_LE(logical_similarity, 1.0);
}

// Test 11: Report Generation
TEST_F(ArchitecturalComplianceValidationTest, ReportGeneration) {
    createTestSourceFile("report_test.cpp", R"(
#include <iostream>

class ReportTestClass {
public:
    void testMethod() {
        std::cout << "Test method for report generation" << std::endl;
    }
};
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(0.0, 100.0, 0.1, 0.05, true));

    // Validate and generate reports
    ZeroDuplicationValidationResult result;
    ASSERT_TRUE(validator_->validateZeroCodeDuplication(result));

    std::string zero_duplication_report;
    ASSERT_TRUE(validator_->generateZeroDuplicationReport(result, zero_duplication_report));

    std::string adapter_pattern_report;
    ASSERT_TRUE(validator_->generateAdapterPatternReport(result, adapter_pattern_report));

    std::string similarity_report;
    ASSERT_TRUE(validator_->generateSimilarityReport(result, similarity_report));

    // Verify report content
    EXPECT_THAT(zero_duplication_report, Contains("Zero Duplication Validation Report"));
    EXPECT_THAT(zero_duplication_report, Contains("Executive Summary"));
    EXPECT_THAT(zero_duplication_report, Contains("Duplication Analysis"));

    EXPECT_THAT(adapter_pattern_report, Contains("Adapter Pattern Report"));
    EXPECT_THAT(adapter_pattern_report, Contains("Pattern Analysis"));

    EXPECT_THAT(similarity_report, Contains("Similarity Analysis Report"));
    EXPECT_THAT(similarity_report, Contains("Structural Similarity"));
    EXPECT_THAT(similarity_report, Contains("Logical Similarity"));
}

// Test 12: Performance and Scalability Test
TEST_F(ArchitecturalComplianceValidationTest, PerformanceAndScalabilityTest) {
    // Create multiple test files
    for (int i = 0; i < 20; ++i) {
        std::string content = R"(
#include <iostream>

class TestClass)" + std::to_string(i) + R"( {
public:
    void method)" + std::to_string(i) + R"(() {
        std::cout << "Method )" + std::to_string(i) + R"(" << std::endl;
    }
};
)";
        createTestSourceFile("test_file_" + std::to_string(i) + ".cpp", content);
    }

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(0.0, 100.0, 0.1, 0.05, true));

    // Measure validation performance
    auto start_time = std::chrono::high_resolution_clock::now();

    ZeroDuplicationValidationResult result;
    ASSERT_TRUE(validator_->validateZeroCodeDuplication(result));

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Verify performance and results
    EXPECT_LT(duration.count(), 10000); // Should complete within 10 seconds
    EXPECT_EQ(result.total_files_analyzed, 20);
    EXPECT_GT(result.total_code_blocks_analyzed, 0);
    EXPECT_TRUE(result.zero_code_duplication_achieved); // All files are unique
}

// Test 13: Error Handling and Edge Cases
TEST_F(ArchitecturalComplianceValidationTest, ErrorHandlingAndEdgeCases) {
    // Test with non-existent directory
    EXPECT_FALSE(validator_->initialize("/non/existent/directory"));

    // Test with empty directory
    std::filesystem::path empty_dir = test_directory_ / "empty";
    std::filesystem::create_directories(empty_dir);
    ASSERT_TRUE(validator_->initialize(empty_dir.string()));

    ZeroDuplicationValidationResult result;
    EXPECT_FALSE(validator_->validateZeroCodeDuplication(result)); // Should fail with no files

    // Test with empty file
    createTestSourceFile("empty.cpp", "");
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));

    std::vector<CodeBlockAnalysisResult> blocks;
    EXPECT_TRUE(validator_->extractCodeBlocks((test_directory_ / "empty.cpp").string(), blocks));
    EXPECT_TRUE(blocks.empty()); // Should not extract blocks from empty file
}

// Test 14: Configuration and Threshold Testing
TEST_F(ArchitecturalComplianceValidationTest, ConfigurationAndThresholdTesting) {
    // Create files with some similarity
    createTestSourceFile("threshold_test1.cpp", R"(
class ThresholdTest {
public:
    void test() {
        std::cout << "Test" << std::endl;
    }
};
)");

    createTestSourceFile("threshold_test2.cpp", R"(
class ThresholdTest {
public:
    void test() {
        std::cout << "Test" << std::endl;
    }
};
)");

    // Test with strict threshold (0%)
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(0.0, 100.0, 0.0, 0.0, true));

    ZeroDuplicationValidationResult strict_result;
    ASSERT_TRUE(validator_->validateZeroCodeDuplication(strict_result));
    EXPECT_FALSE(strict_result.zero_code_duplication_achieved); // Should fail with strict threshold

    // Test with lenient threshold (100%)
    ASSERT_TRUE(validator_->configure(100.0, 100.0, 1.0, 1.0, false));

    ZeroDuplicationValidationResult lenient_result;
    ASSERT_TRUE(validator_->validateZeroCodeDuplication(lenient_result));
    EXPECT_TRUE(lenient_result.zero_code_duplication_achieved); // Should pass with lenient threshold
}

// Test 15: Utility Functions Test
TEST_F(ArchitecturalComplianceValidationTest, UtilityFunctionsTest) {
    // Create unique files
    createTestSourceFile("utility_test1.cpp", R"(
class UtilityTest1 {
public:
    void uniqueMethod() {}
};
)");

    createTestSourceFile("utility_test2.cpp", R"(
class UtilityTest2 {
public:
    void anotherUniqueMethod() {}
};
)");

    // Test utility functions
    EXPECT_TRUE(architectural_compliance_validation_utils::quickZeroDuplicationCheck(test_directory_.string()));
    EXPECT_TRUE(architectural_compliance_validation_utils::checkStructuralSimilarity(test_directory_.string()));

    // Test metrics calculation
    double duplication_percentage = architectural_compliance_validation_utils::calculateOverallDuplicationPercentage(test_directory_.string());
    EXPECT_EQ(duplication_percentage, 0.0); // No duplicates

    // Test file duplication scores
    std::map<std::string, double> file_scores = architectural_compliance_validation_utils::calculateFileDuplicationScores(test_directory_.string());
    EXPECT_EQ(file_scores.size(), 2); // Two files
    for (const auto& score : file_scores) {
        EXPECT_EQ(score.second, 0.0); // No duplicates in any file
    }
}

// Main function for running tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
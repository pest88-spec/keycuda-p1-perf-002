// Puzzle71 Technical Debt Repair - Unified Module Validation Tests
// Phase 5: User Story 4 - Complete System Migration and Quality Assurance
// Task: T074 - Validate all kernels use unified modules (no legacy paths remain)
// TDD test implementation for unified module validation framework

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <memory>

#include "src/KeyhuntCore/validation/unified_module_validator.h"

using namespace puzzle71::validation;
using ::testing::Return;
using ::testing::_;
using ::testing::Contains;
using ::testing::Not;

class UnifiedModuleValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        validator_ = std::make_unique<UnifiedModuleValidator>();
        test_directory_ = std::filesystem::temp_directory_path() / "puzzle71_unified_test";
        std::filesystem::create_directories(test_directory_);
    }

    void TearDown() override {
        validator_.reset();
        std::filesystem::remove_all(test_directory_);
    }

    void createTestKernelFile(const std::string& filename, const std::string& content) {
        std::ofstream file(test_directory_ / filename);
        file << content;
        file.close();
    }

    std::unique_ptr<UnifiedModuleValidator> validator_;
    std::filesystem::path test_directory_;
};

// Test 1: Unified Module Usage Validation
TEST_F(UnifiedModuleValidationTest, UnifiedModuleUsageValidation) {
    // Create a kernel file with unified modules
    createTestKernelFile("test_kernel.cu", R"(
#include "src/KeyhuntCore/scanning/unified_candidate_scanner.h"
#include "src/KeyhuntCore/ecc/ecc_operations_fixed.h"
#include "src/KeyhuntCore/memory/optimized_memory_access.h"

extern "C" __global__ void unifiedKernelTest(
    UnifiedCandidateScanner* scanner,
    ECCOperationsFixed* ecc_ops,
    OptimizedMemoryAccess* memory_access
) {
    // Use unified modules
    uint32_t candidate;
    if (scanner->getNextCandidate(threadIdx.x, blockDim.x, candidate)) {
        secp256k1_pubkey pubkey;
        if (ecc_ops->computePublicKey(candidate, pubkey)) {
            uint8_t hash160[20];
            if (memory_access->computeHash160(pubkey, hash160)) {
                // Process result
            }
        }
    }
}
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(100.0, true, true));

    // Validate kernel file
    KernelFileAnalysisResult result;
    ASSERT_TRUE(validator_->validateKernelFile((test_directory_ / "test_kernel.cu").string(), result));

    // Verify unified module usage
    EXPECT_GT(result.unified_usage_percentage, 90.0);
    EXPECT_TRUE(result.is_compliant);
    EXPECT_TRUE(result.legacy_patterns.empty());
    EXPECT_GT(result.unified_module_function_calls.size(), 0);
}

// Test 2: Legacy Code Detection
TEST_F(UnifiedModuleValidationTest, LegacyCodeDetection) {
    // Create a kernel file with legacy patterns
    createTestKernelFile("legacy_kernel.cu", R"(
#include "legacy_ecc_operations.h"
#include "old_memory_access.h"

extern "C" __global__ void legacyKernelTest() {
    // Use legacy function calls
    legacy_compute_private_key();
    old_memory_allocation();
    deprecated_hash_calculation();

    // Use legacy variables
    static int legacy_counter = 0;
    LEGACY_MACRO_DEFINITION();
}
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(100.0, true, true));

    // Validate kernel file
    KernelFileAnalysisResult result;
    ASSERT_TRUE(validator_->validateKernelFile((test_directory_ / "legacy_kernel.cu").string(), result));

    // Verify legacy pattern detection
    EXPECT_LT(result.unified_usage_percentage, 100.0);
    EXPECT_FALSE(result.is_compliant);
    EXPECT_GT(result.legacy_patterns.size(), 0);
    EXPECT_GT(result.legacy_includes.size(), 0);
    EXPECT_GT(result.legacy_function_calls.size(), 0);
    EXPECT_GT(result.legacy_macros.size(), 0);
}

// Test 3: Complete Migration Validation
TEST_F(UnifiedModuleValidationTest, CompleteMigrationValidation) {
    // Create multiple kernel files
    createTestKernelFile("kernel1.cu", R"(
#include "src/KeyhuntCore/scanning/unified_candidate_scanner.h"
#include "src/KeyhuntCore/ecc/ecc_operations_fixed.h"

extern "C" __global__ void kernel1(
    UnifiedCandidateScanner* scanner,
    ECCOperationsFixed* ecc_ops
) {
    uint32_t candidate;
    scanner->getNextCandidate(threadIdx.x, blockDim.x, candidate);
    ecc_ops->computePublicKey(candidate, candidate);
}
)");

    createTestKernelFile("kernel2.cu", R"(
#include "src/KeyhuntCore/memory/optimized_memory_access.h"
#include "src/KeyhuntCore/execution/gpu_executor.h"

extern "C" __global__ void kernel2(
    OptimizedMemoryAccess* memory_access,
    GPUExecutor* executor
) {
    memory_access->allocateSharedMemory();
    executor->executeKernel();
}
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(100.0, true, true));

    // Validate complete migration
    KernelValidationResult result;
    ASSERT_TRUE(validator_->validateCompleteMigrationAchieved(result));

    // Verify complete migration
    EXPECT_TRUE(result.complete_migration_achieved);
    EXPECT_TRUE(result.all_kernels_use_unified_modules);
    EXPECT_TRUE(result.no_legacy_code_paths_remain);
    EXPECT_TRUE(result.unified_module_compliance_met);
    EXPECT_TRUE(result.non_compliant_kernels.empty());
    EXPECT_EQ(result.usage_metrics.legacy_function_calls, 0);
}

// Test 4: All Kernels Use Unified Modules Validation
TEST_F(UnifiedModuleValidationTest, AllKernelsUseUnifiedModules) {
    // Create a mix of compliant and non-compliant kernels
    createTestKernelFile("compliant_kernel.cu", R"(
#include "src/KeyhuntCore/scanning/unified_candidate_scanner.h"

extern "C" __global__ void compliantKernel(UnifiedCandidateScanner* scanner) {
    uint32_t candidate;
    scanner->getNextCandidate(threadIdx.x, blockDim.x, candidate);
}
)");

    createTestKernelFile("non_compliant_kernel.cu", R"(
#include "legacy_scanner.h"

extern "C" __global__ void nonCompliantKernel() {
    legacy_function_call();
    old_memory_operation();
}
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(100.0, true, true));

    // Validate all kernels
    KernelValidationResult result;
    ASSERT_TRUE(validator_->validateAllKernelsUseUnifiedModules(result));

    // Verify validation results
    EXPECT_EQ(result.total_kernels_analyzed, 2);
    EXPECT_EQ(result.compliant_kernels.size(), 1);
    EXPECT_EQ(result.non_compliant_kernels.size(), 1);
    EXPECT_THAT(result.compliant_kernels, Contains("compliant_kernel"));
    EXPECT_THAT(result.non_compliant_kernels, Contains("non_compliant_kernel"));
    EXPECT_LT(result.overall_compliance_score, 100.0);
}

// Test 5: No Legacy Code Paths Remain Validation
TEST_F(UnifiedModuleValidationTest, NoLegacyCodePathsRemain) {
    // Create kernels with various legacy patterns
    createTestKernelFile("kernel_with_legacy.cu", R"(
#include "legacy_header.h"
#include "deprecated_functions.h"

extern "C" __global__ void kernelWithLegacy() {
    // Legacy function calls
    legacy_compute_function();
    old_memory_allocator();
    deprecated_hash_function();

    // Legacy variables
    static int legacy_variable = 0;
    old_struct_type old_instance;

    // Legacy macros
    LEGACY_MACRO_CALL();
    DEPRECATED_DEFINITION();
}
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(100.0, true, true));

    // Validate no legacy code paths
    KernelValidationResult result;
    ASSERT_TRUE(validator_->validateNoLegacyCodePathsRemain(result));

    // Verify legacy code detection
    EXPECT_FALSE(result.no_legacy_code_paths_remain);
    EXPECT_GT(result.usage_metrics.legacy_function_calls, 0);
    EXPECT_GT(result.usage_metrics.legacy_includes, 0);
    EXPECT_GT(result.usage_metrics.legacy_variables, 0);
    EXPECT_GT(result.usage_metrics.legacy_macros, 0);
    EXPECT_FALSE(result.legacy_patterns_found.empty());
}

// Test 6: Module-Specific Usage Validation
TEST_F(UnifiedModuleValidationTest, ModuleSpecificUsageValidation) {
    // Create kernel using specific unified modules
    createTestKernelFile("specific_modules_kernel.cu", R"(
#include "src/KeyhuntCore/scanning/unified_candidate_scanner.h"
#include "src/KeyhuntCore/ecc/ecc_operations_fixed.h"
#include "src/KeyhuntCore/adapters/legacy_adapter_fixed.h"
#include "src/KeyhuntCore/memory/optimized_memory_access.h"
#include "src/KeyhuntCore/execution/gpu_executor.h"
#include "src/KeyhuntCore/planning/batch_planner.h"

extern "C" __global__ void specificModulesKernel(
    UnifiedCandidateScanner* scanner,
    ECCOperationsFixed* ecc_ops,
    LegacyAdapterFixed* adapter,
    OptimizedMemoryAccess* memory_access,
    GPUExecutor* executor,
    BatchPlanner* planner
) {
    // Use all unified modules
    scanner->getNextCandidate(threadIdx.x, blockDim.x, candidate);
    ecc_ops->computePublicKey(candidate, pubkey);
    adapter->adaptLegacyInterface(old_interface, new_interface);
    memory_access->optimizeMemoryLayout();
    executor->launchKernel();
    planner->planBatchExecution();
}
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(100.0, true, true));

    // Validate kernel file
    KernelFileAnalysisResult result;
    ASSERT_TRUE(validator_->validateKernelFile((test_directory_ / "specific_modules_kernel.cu").string(), result));

    // Verify module usage
    EXPECT_GT(result.module_usage.size(), 4); // Should use multiple modules
    EXPECT_TRUE(result.is_compliant);
    EXPECT_EQ(result.unified_usage_percentage, 100.0);

    // Check for specific modules
    bool has_candidate_scanner = false;
    bool has_ecc_operations = false;
    bool has_legacy_adapter = false;
    bool has_memory_access = false;

    for (const auto& module_usage : result.module_usage) {
        switch (module_usage.first) {
            case UnifiedModuleType::CANDIDATE_SCANNER:
                has_candidate_scanner = true;
                break;
            case UnifiedModuleType::ECC_OPERATIONS:
                has_ecc_operations = true;
                break;
            case UnifiedModuleType::LEGACY_ADAPTER:
                has_legacy_adapter = true;
                break;
            case UnifiedModuleType::MEMORY_ACCESS:
                has_memory_access = true;
                break;
        }
    }

    EXPECT_TRUE(has_candidate_scanner);
    EXPECT_TRUE(has_ecc_operations);
    EXPECT_TRUE(has_legacy_adapter);
    EXPECT_TRUE(has_memory_access);
}

// Test 7: Validation Report Generation
TEST_F(UnifiedModuleValidationTest, ValidationReportGeneration) {
    // Create test kernels
    createTestKernelFile("report_test_kernel.cu", R"(
#include "src/KeyhuntCore/scanning/unified_candidate_scanner.h"
#include "legacy_header.h"

extern "C" __global__ void reportTestKernel(UnifiedCandidateScanner* scanner) {
    uint32_t candidate;
    scanner->getNextCandidate(threadIdx.x, blockDim.x, candidate);
    legacy_function_call(); // This should be detected as legacy
}
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(95.0, true, true));

    // Validate and generate report
    KernelValidationResult result;
    ASSERT_TRUE(validator_->validateAllKernelsUseUnifiedModules(result));

    std::string report;
    ASSERT_TRUE(validator_->generateValidationReport(result, report));

    // Verify report content
    EXPECT_THAT(report, Contains("Unified Module Validation Report"));
    EXPECT_THAT(report, Contains("Executive Summary"));
    EXPECT_THAT(report, Contains("Detailed Analysis"));
    EXPECT_THAT(report, Contains("Kernel Breakdown"));
    EXPECT_THAT(report, Contains("Legacy Code Analysis"));
    EXPECT_THAT(report, Contains("Recommendations"));
    EXPECT_THAT(report, Contains("report_test_kernel"));
}

// Test 8: Performance and Scalability Test
TEST_F(UnifiedModuleValidationTest, PerformanceAndScalabilityTest) {
    // Create multiple kernel files to test scalability
    for (int i = 0; i < 10; ++i) {
        std::string kernel_content = R"(
#include "src/KeyhuntCore/scanning/unified_candidate_scanner.h"
#include "src/KeyhuntCore/ecc/ecc_operations_fixed.h"

extern "C" __global__ void performanceTestKernel)" + std::to_string(i) + R"((
    UnifiedCandidateScanner* scanner,
    ECCOperationsFixed* ecc_ops
) {
    uint32_t candidate;
    scanner->getNextCandidate(threadIdx.x, blockDim.x, candidate);
    ecc_ops->computePublicKey(candidate, candidate);
}
)";
        createTestKernelFile("performance_kernel_" + std::to_string(i) + ".cu", kernel_content);
    }

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(100.0, true, true));

    // Measure validation performance
    auto start_time = std::chrono::high_resolution_clock::now();

    KernelValidationResult result;
    ASSERT_TRUE(validator_->validateAllKernelsUseUnifiedModules(result));

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Verify performance and results
    EXPECT_LT(duration.count(), 5000); // Should complete within 5 seconds
    EXPECT_EQ(result.total_kernels_analyzed, 10);
    EXPECT_EQ(result.compliant_kernels.size(), 10);
    EXPECT_TRUE(result.non_compliant_kernels.empty());
    EXPECT_EQ(result.overall_compliance_score, 100.0);
}

// Test 9: Error Handling and Edge Cases
TEST_F(UnifiedModuleValidationTest, ErrorHandlingAndEdgeCases) {
    // Test with non-existent directory
    EXPECT_FALSE(validator_->initialize("/non/existent/directory"));

    // Test with empty directory
    std::filesystem::path empty_dir = test_directory_ / "empty";
    std::filesystem::create_directories(empty_dir);
    ASSERT_TRUE(validator_->initialize(empty_dir.string()));

    KernelValidationResult result;
    EXPECT_FALSE(validator_->validateAllKernelsUseUnifiedModules(result)); // Should fail with no kernels

    // Test with invalid file content
    createTestKernelFile("invalid.cu", "This is not valid CUDA code");
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));

    KernelFileAnalysisResult file_result;
    EXPECT_TRUE(validator_->validateKernelFile((test_directory_ / "invalid.cu").string(), file_result));
    EXPECT_EQ(file_result.total_lines, 1); // Should still count the line
}

// Test 10: Configuration and Threshold Testing
TEST_F(UnifiedModuleValidationTest, ConfigurationAndThresholdTesting) {
    // Create a partially compliant kernel
    createTestKernelFile("partial_kernel.cu", R"(
#include "src/KeyhuntCore/scanning/unified_candidate_scanner.h"
#include "legacy_header.h"

extern "C" __global__ void partialKernel(UnifiedCandidateScanner* scanner) {
    uint32_t candidate;
    scanner->getNextCandidate(threadIdx.x, blockDim.x, candidate);
    legacy_function_call(); // 50% unified, 50% legacy
}
)");

    // Test with strict threshold (100%)
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(100.0, true, true));

    KernelFileAnalysisResult result;
    ASSERT_TRUE(validator_->validateKernelFile((test_directory_ / "partial_kernel.cu").string(), result));

    EXPECT_LT(result.unified_usage_percentage, 100.0);
    EXPECT_FALSE(result.is_compliant);

    // Test with lenient threshold (50%)
    ASSERT_TRUE(validator_->configure(50.0, false, true));
    ASSERT_TRUE(validator_->validateKernelFile((test_directory_ / "partial_kernel.cu").string(), result));

    EXPECT_GT(result.unified_usage_percentage, 50.0);
    EXPECT_TRUE(result.is_compliant);
}

// Test 11: Utility Functions Test
TEST_F(UnifiedModuleValidationTest, UtilityFunctionsTest) {
    // Create compliant kernel
    createTestKernelFile("utility_test_kernel.cu", R"(
#include "src/KeyhuntCore/scanning/unified_candidate_scanner.h"

extern "C" __global__ void utilityTestKernel(UnifiedCandidateScanner* scanner) {
    uint32_t candidate;
    scanner->getNextCandidate(threadIdx.x, blockDim.x, candidate);
}
)");

    // Test utility functions
    EXPECT_TRUE(unified_module_validation_utils::quickUnifiedModuleCheck(test_directory_.string()));
    EXPECT_TRUE(unified_module_validation_utils::validateNoLegacyCode(test_directory_.string()));
    EXPECT_TRUE(unified_module_validation_utils::checkCompleteMigration(test_directory_.string()));

    // Test with legacy code
    createTestKernelFile("legacy_utility_kernel.cu", R"(
#include "legacy_header.h"

extern "C" __global__ void legacyUtilityKernel() {
    legacy_function_call();
}
)");

    EXPECT_FALSE(unified_module_validation_utils::validateNoLegacyCode(test_directory_.string()));
    EXPECT_FALSE(unified_module_validation_utils::checkCompleteMigration(test_directory_.string()));
}

// Test 12: Cache and Performance Optimization Test
TEST_F(UnifiedModuleValidationTest, CacheAndPerformanceOptimizationTest) {
    // Create test kernel
    createTestKernelFile("cache_test_kernel.cu", R"(
#include "src/KeyhuntCore/scanning/unified_candidate_scanner.h"

extern "C" __global__ void cacheTestKernel(UnifiedCandidateScanner* scanner) {
    uint32_t candidate;
    scanner->getNextCandidate(threadIdx.x, blockDim.x, candidate);
}
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(100.0, true, true));

    std::string kernel_path = (test_directory_ / "cache_test_kernel.cu").string();

    // First validation (should populate cache)
    auto start1 = std::chrono::high_resolution_clock::now();
    KernelFileAnalysisResult result1;
    ASSERT_TRUE(validator_->validateKernelFile(kernel_path, result1));
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1);

    // Second validation (should use cache)
    auto start2 = std::chrono::high_resolution_clock::now();
    KernelFileAnalysisResult result2;
    ASSERT_TRUE(validator_->validateKernelFile(kernel_path, result2));
    auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2);

    // Verify cache functionality
    EXPECT_EQ(result1.filepath, result2.filepath);
    EXPECT_EQ(result1.kernel_name, result2.kernel_name);
    EXPECT_EQ(result1.unified_usage_percentage, result2.unified_usage_percentage);
    EXPECT_EQ(result1.is_compliant, result2.is_compliant);

    // Second validation should be faster (due to caching)
    EXPECT_LT(duration2.count(), duration1.count());
}

// Main function for running tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
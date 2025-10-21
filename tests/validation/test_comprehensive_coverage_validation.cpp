// Puzzle71 Technical Debt Repair - Comprehensive Coverage Validation Tests
// Phase 5: User Story 4 - Complete System Migration and Quality Assurance
// Task: T076 - Verify comprehensive test coverage achieved for all critical code paths
// TDD test implementation for comprehensive coverage validation

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <algorithm>

#include "src/KeyhuntCore/validation/comprehensive_coverage_validator.h"

using namespace puzzle71::validation;
using ::testing::Return;
using ::testing::_;
using ::testing::Contains;
using ::testing::Not;
using ::testing::UnorderedElementsAre;
using ::testing::WhenSorted;

class ComprehensiveCoverageValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        validator_ = std::make_unique<ComprehensiveCoverageValidator>();
        test_directory_ = std::filesystem::temp_directory_path() / "puzzle71_coverage_test";
        std::filesystem::create_directories(test_directory_);

        // Create source and test directories
        std::filesystem::create_directories(test_directory_ / "src");
        std::filesystem::create_directories(test_directory_ / "tests");
    }

    void TearDown() override {
        validator_.reset();
        std::filesystem::remove_all(test_directory_);
    }

    void createSourceFile(const std::string& filename, const std::string& content) {
        std::ofstream file(test_directory_ / "src" / filename);
        file << content;
        file.close();
    }

    void createTestFile(const std::string& filename, const std::string& content) {
        std::ofstream file(test_directory_ / "tests" / filename);
        file << content;
        file.close();
    }

    void createCriticalSourceFiles() {
        // Kernel execution critical paths
        createSourceFile("kernel_executor.cu", R"(
#include <cuda_runtime.h>

extern "C" __global__ void criticalKernelFunction(uint32_t* data, int size) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < size) {
        // Critical kernel execution path
        data[tid] = data[tid] * 2 + tid;

        // Error handling path
        if (data[tid] == 0) {
            return; // Critical error handling
        }

        // Memory management path
        __syncthreads();
    }
}

extern "C" __global__ void anotherCriticalKernel(float* input, float* output) {
    int idx = threadIdx.x;

    // Critical computation path
    float temp = input[idx] * 3.14159f;
    output[idx] = sinf(temp) + cosf(temp);

    // Performance critical path
    for (int i = 0; i < 100; ++i) {
        output[idx] += temp * i;
    }
}
)");

        // ECC operations critical paths
        createSourceFile("ecc_operations.cpp", R"(
#include "secp256k1.h"

class CriticalECCOperations {
public:
    // Critical ECC computation path
    bool computePublicKey(const uint32_t* private_key, uint32_t* public_key) {
        if (!private_key || !public_key) {
            return false; // Critical error handling
        }

        // Core ECC computation
        for (int i = 0; i < 8; ++i) {
            public_key[i] = private_key[i] * 2 + i;
        }

        return validatePublicKey(public_key);
    }

    // Critical validation path
    bool validatePublicKey(const uint32_t* public_key) {
        if (!public_key) {
            return false;
        }

        // Complex validation logic
        uint32_t sum = 0;
        for (int i = 0; i < 8; ++i) {
            sum += public_key[i];
        }

        return (sum % 256) != 0;
    }

    // Critical memory management path
    bool allocateECCMemory() {
        void* memory = malloc(1024);
        if (!memory) {
            return false; // Critical allocation failure
        }

        // Initialize memory
        memset(memory, 0, 1024);

        free(memory);
        return true;
    }
};
)");

        // Memory management critical paths
        createSourceFile("memory_manager.cpp", R"(
#include <cuda_runtime.h>
#include <cstring>

class CriticalMemoryManager {
private:
    void* device_memory_;
    size_t memory_size_;

public:
    // Critical memory allocation path
    bool allocateDeviceMemory(size_t size) {
        if (size == 0) {
            return false; // Critical error handling
        }

        cudaError_t result = cudaMalloc(&device_memory_, size);
        if (result != cudaSuccess) {
            return false; // Critical CUDA error handling
        }

        memory_size_ = size;
        return true;
    }

    // Critical memory copy path
    bool copyToDevice(const void* host_data, size_t size) {
        if (!host_data || size > memory_size_) {
            return false; // Critical parameter validation
        }

        cudaError_t result = cudaMemcpy(device_memory_, host_data, size, cudaMemcpyHostToDevice);
        if (result != cudaSuccess) {
            return false; // Critical copy error handling
        }

        return true;
    }

    // Critical memory deallocation path
    void deallocateDeviceMemory() {
        if (device_memory_) {
            cudaFree(device_memory_);
            device_memory_ = nullptr;
            memory_size_ = 0;
        }
    }
};
)");

        // Configuration loading critical paths
        createSourceFile("config_loader.cpp", R"(
#include <fstream>
#include <json/json.h>

class CriticalConfigLoader {
public:
    // Critical configuration loading path
    bool loadConfiguration(const std::string& config_file) {
        std::ifstream file(config_file);
        if (!file.is_open()) {
            return false; // Critical file loading error
        }

        Json::Value root;
        Json::Reader reader;

        if (!reader.parse(file, root)) {
            return false; // Critical JSON parsing error
        }

        // Validate configuration
        if (!validateConfiguration(root)) {
            return false; // Critical configuration validation error
        }

        return applyConfiguration(root);
    }

    // Critical configuration validation path
    bool validateConfiguration(const Json::Value& config) {
        if (!config.isMember("gpu_memory") || !config.isMember("batch_size")) {
            return false; // Critical configuration missing
        }

        int gpu_memory = config["gpu_memory"].asInt();
        int batch_size = config["batch_size"].asInt();

        if (gpu_memory <= 0 || batch_size <= 0) {
            return false; // Critical invalid configuration
        }

        return true;
    }

    // Critical configuration application path
    bool applyConfiguration(const Json::Value& config) {
        try {
            // Apply GPU memory configuration
            int gpu_memory = config["gpu_memory"].asInt();
            setGPUMemoryLimit(gpu_memory);

            // Apply batch size configuration
            int batch_size = config["batch_size"].asInt();
            setBatchSize(batch_size);

            return true;
        } catch (...) {
            return false; // Critical configuration application error
        }
    }

private:
    void setGPUMemoryLimit(int memory) {}
    void setBatchSize(int size) {}
};
)");
    }

    void createComprehensiveTestFiles() {
        // Test file covering kernel execution paths
        createTestFile("test_kernel_execution.cpp", R"(
#include <gtest/gtest.h>
#include "kernel_executor.cu"

TEST(KernelExecutionTest, CriticalKernelFunctionTest) {
    const int size = 1024;
    uint32_t* data = new uint32_t[size];
    uint32_t* device_data;

    // Initialize test data
    for (int i = 0; i < size; ++i) {
        data[i] = i;
    }

    // Allocate device memory
    cudaMalloc(&device_data, size * sizeof(uint32_t));
    cudaMemcpy(device_data, data, size * sizeof(uint32_t), cudaMemcpyHostToDevice);

    // Launch critical kernel
    criticalKernelFunction<<<1, size>>>(device_data, size);
    cudaDeviceSynchronize();

    // Copy results back
    cudaMemcpy(data, device_data, size * sizeof(uint32_t), cudaMemcpyDeviceToHost);

    // Validate results
    for (int i = 0; i < size; ++i) {
        EXPECT_EQ(data[i], i * 2 + i);
    }

    // Test error handling path
    uint32_t* null_data = nullptr;
    criticalKernelFunction<<<1, 1>>>(null_data, 0);

    cudaFree(device_data);
    delete[] data;
}

TEST(KernelExecutionTest, AnotherCriticalKernelTest) {
    const int size = 256;
    float* input = new float[size];
    float* output = new float[size];
    float* device_input, * device_output;

    // Initialize test data
    for (int i = 0; i < size; ++i) {
        input[i] = i * 0.1f;
    }

    // Allocate device memory
    cudaMalloc(&device_input, size * sizeof(float));
    cudaMalloc(&device_output, size * sizeof(float));

    cudaMemcpy(device_input, input, size * sizeof(float), cudaMemcpyHostToDevice);

    // Launch critical kernel
    anotherCriticalKernel<<<1, size>>>(device_input, device_output);
    cudaDeviceSynchronize();

    // Copy results back
    cudaMemcpy(output, device_output, size * sizeof(float), cudaMemcpyDeviceToHost);

    // Validate results
    for (int i = 0; i < size; ++i) {
        float expected = sinf(input[i] * 3.14159f) + cosf(input[i] * 3.14159f);
        EXPECT_NEAR(output[i], expected, 1e-6f);
    }

    cudaFree(device_input);
    cudaFree(device_output);
    delete[] input;
    delete[] output;
}
)");

        // Test file covering ECC operations
        createTestFile("test_ecc_operations.cpp", R"(
#include <gtest/gtest.h>
#include "ecc_operations.cpp"

TEST(ECCOperationsTest, ComputePublicKeyTest) {
    CriticalECCOperations ecc;

    // Test normal computation path
    uint32_t private_key[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint32_t public_key[8] = {0};

    bool result = ecc.computePublicKey(private_key, public_key);
    EXPECT_TRUE(result);

    // Validate computed public key
    bool valid = ecc.validatePublicKey(public_key);
    EXPECT_TRUE(valid);

    // Test error handling path - null private key
    result = ecc.computePublicKey(nullptr, public_key);
    EXPECT_FALSE(result);

    // Test error handling path - null public key
    result = ecc.computePublicKey(private_key, nullptr);
    EXPECT_FALSE(result);
}

TEST(ECCOperationsTest, ValidatePublicKeyTest) {
    CriticalECCOperations ecc;

    // Test valid public key
    uint32_t valid_key[8] = {2, 4, 6, 8, 10, 12, 14, 16};
    bool result = ecc.validatePublicKey(valid_key);
    EXPECT_TRUE(result);

    // Test invalid public key (sum divisible by 256)
    uint32_t invalid_key[8] = {32, 32, 32, 32, 32, 32, 32, 32}; // sum = 256
    result = ecc.validatePublicKey(invalid_key);
    EXPECT_FALSE(result);

    // Test null public key
    result = ecc.validatePublicKey(nullptr);
    EXPECT_FALSE(result);
}

TEST(ECCOperationsTest, MemoryAllocationTest) {
    CriticalECCOperations ecc;

    // Test successful allocation
    bool result = ecc.allocateECCMemory();
    EXPECT_TRUE(result);

    // Note: Memory allocation failure testing would require mocking
}
)");

        // Test file covering memory management
        createTestFile("test_memory_manager.cpp", R"(
#include <gtest/gtest.h>
#include "memory_manager.cpp"

TEST(MemoryManagerTest, AllocateDeviceMemoryTest) {
    CriticalMemoryManager manager;

    // Test successful allocation
    bool result = manager.allocateDeviceMemory(1024);
    EXPECT_TRUE(result);

    // Test allocation with zero size
    CriticalMemoryManager manager2;
    result = manager2.allocateDeviceMemory(0);
    EXPECT_FALSE(result);

    manager.deallocateDeviceMemory();
}

TEST(MemoryManagerTest, CopyToDeviceTest) {
    CriticalMemoryManager manager;

    // Allocate device memory first
    ASSERT_TRUE(manager.allocateDeviceMemory(1024));

    // Test successful copy
    uint32_t host_data[256];
    for (int i = 0; i < 256; ++i) {
        host_data[i] = i;
    }

    bool result = manager.copyToDevice(host_data, 256 * sizeof(uint32_t));
    EXPECT_TRUE(result);

    // Test copy with null host data
    result = manager.copyToDevice(nullptr, 256);
    EXPECT_FALSE(result);

    // Test copy with size larger than allocated
    result = manager.copyToDevice(host_data, 2048);
    EXPECT_FALSE(result);

    manager.deallocateDeviceMemory();
}

TEST(MemoryManagerTest, DeallocateDeviceMemoryTest) {
    CriticalMemoryManager manager;

    // Test deallocation after allocation
    ASSERT_TRUE(manager.allocateDeviceMemory(1024));
    manager.deallocateDeviceMemory();

    // Should not crash when called twice
    manager.deallocateDeviceMemory();
}
)");

        // Test file covering configuration loading
        createTestFile("test_config_loader.cpp", R"(
#include <gtest/gtest.h>
#include "config_loader.cpp"
#include <fstream>
#include <sstream>

TEST(ConfigLoaderTest, LoadValidConfigurationTest) {
    // Create temporary valid config file
    std::string valid_config = R"({
    "gpu_memory": 2048,
    "batch_size": 1024,
    "additional_settings": {
        "optimization_level": 3
    }
})";

    std::ofstream config_file("test_config.json");
    config_file << valid_config;
    config_file.close();

    CriticalConfigLoader loader;
    bool result = loader.loadConfiguration("test_config.json");
    EXPECT_TRUE(result);

    // Clean up
    std::remove("test_config.json");
}

TEST(ConfigLoaderTest, LoadInvalidConfigurationTest) {
    // Create temporary invalid config file
    std::string invalid_config = R"({
    "gpu_memory": -1,
    "batch_size": 0
})";

    std::ofstream config_file("invalid_config.json");
    config_file << invalid_config;
    config_file.close();

    CriticalConfigLoader loader;
    bool result = loader.loadConfiguration("invalid_config.json");
    EXPECT_FALSE(result);

    // Clean up
    std::remove("invalid_config.json");
}

TEST(ConfigLoaderTest, LoadNonExistentFileTest) {
    CriticalConfigLoader loader;
    bool result = loader.loadConfiguration("non_existent_config.json");
    EXPECT_FALSE(result);
}

TEST(ConfigLoaderTest, LoadMalformedJsonTest) {
    // Create malformed JSON file
    std::string malformed_config = R"({
    "gpu_memory": 2048,
    "batch_size": 1024,
    // This comment makes it invalid JSON
})";

    std::ofstream config_file("malformed_config.json");
    config_file << malformed_config;
    config_file.close();

    CriticalConfigLoader loader;
    bool result = loader.loadConfiguration("malformed_config.json");
    EXPECT_FALSE(result);

    // Clean up
    std::remove("malformed_config.json");
}
)");

        // Integration test file
        createTestFile("test_integration.cpp", R"(
#include <gtest/gtest.h>
#include "ecc_operations.cpp"
#include "memory_manager.cpp"

TEST(IntegrationTest, ECCWithMemoryIntegration) {
    CriticalECCOperations ecc;
    CriticalMemoryManager manager;

    // Allocate device memory
    ASSERT_TRUE(manager.allocateDeviceMemory(1024));

    // Test ECC operations with memory
    uint32_t private_key[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint32_t public_key[8] = {0};

    bool result = ecc.computePublicKey(private_key, public_key);
    EXPECT_TRUE(result);

    // Copy public key to device memory
    result = manager.copyToDevice(public_key, 8 * sizeof(uint32_t));
    EXPECT_TRUE(result);

    manager.deallocateDeviceMemory();
}

TEST(IntegrationTest, ConfigurationSystemIntegration) {
    // Test integration between configuration and other components
    std::string config = R"({
    "gpu_memory": 4096,
    "batch_size": 2048
})";

    std::ofstream config_file("integration_config.json");
    config_file << config;
    config_file.close();

    CriticalConfigLoader loader;
    ASSERT_TRUE(loader.loadConfiguration("integration_config.json"));

    // Configuration should be applied and ready for use by other components

    std::remove("integration_config.json");
}
)");
    }

    std::unique_ptr<ComprehensiveCoverageValidator> validator_;
    std::filesystem::path test_directory_;
};

// Test 1: Critical Path Coverage Validation
TEST_F(ComprehensiveCoverageValidationTest, CriticalPathCoverageValidation) {
    createCriticalSourceFiles();
    createComprehensiveTestFiles();

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(95.0, 90.0, 85.0, 95.0, 80.0, true));

    // Validate critical path coverage
    ComprehensiveCoverageValidationResult result;
    ASSERT_TRUE(validator_->validateCriticalPathCoverage(result));

    // Verify critical path coverage
    EXPECT_TRUE(result.critical_path_coverage_achieved);
    EXPECT_GE(result.critical_path_coverage_percentage, 95.0);
    EXPECT_EQ(result.uncovered_critical_paths, 0);
    EXPECT_GT(result.total_critical_paths, 0);
    EXPECT_EQ(result.total_critical_paths, result.covered_critical_paths);
}

// Test 2: Overall Coverage Threshold Validation
TEST_F(ComprehensiveCoverageValidationTest, OverallCoverageThresholdValidation) {
    createCriticalSourceFiles();
    createComprehensiveTestFiles();

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(95.0, 90.0, 85.0, 95.0, 80.0, true));

    // Validate overall coverage thresholds
    ComprehensiveCoverageValidationResult result;
    ASSERT_TRUE(validator_->validateOverallCoverageThresholds(result));

    // Verify overall coverage thresholds
    EXPECT_TRUE(result.overall_coverage_threshold_met);
    EXPECT_GE(result.overall_line_coverage_percentage, 90.0);
    EXPECT_GE(result.function_coverage_percentage, 95.0);
    EXPECT_GE(result.branch_coverage_percentage, 85.0);
    EXPECT_GE(result.condition_coverage_percentage, 80.0);
}

// Test 3: Branch Coverage Validation
TEST_F(ComprehensiveCoverageValidationTest, BranchCoverageValidation) {
    createCriticalSourceFiles();
    createComprehensiveTestFiles();

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(95.0, 90.0, 85.0, 95.0, 80.0, true));

    // Validate branch coverage
    ComprehensiveCoverageValidationResult result;
    ASSERT_TRUE(validator_->validateBranchCoverage(result));

    // Verify branch coverage
    EXPECT_TRUE(result.branch_coverage_threshold_met);
    EXPECT_GE(result.branch_coverage_percentage, 85.0);
    EXPECT_LE(result.uncovered_critical_branches, 5);
    EXPECT_GT(result.total_critical_branches, 0);
}

// Test 4: Function Coverage Validation
TEST_F(ComprehensiveCoverageValidationTest, FunctionCoverageValidation) {
    createCriticalSourceFiles();
    createComprehensiveTestFiles();

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(95.0, 90.0, 85.0, 95.0, 80.0, true));

    // Validate function coverage
    ComprehensiveCoverageValidationResult result;
    ASSERT_TRUE(validator_->validateFunctionCoverage(result));

    // Verify function coverage
    EXPECT_TRUE(result.function_coverage_threshold_met);
    EXPECT_GE(result.function_coverage_percentage, 95.0);
    EXPECT_EQ(result.uncovered_critical_functions, 0);
    EXPECT_GT(result.total_critical_functions, 0);
}

// Test 5: Condition Coverage Validation
TEST_F(ComprehensiveCoverageValidationTest, ConditionCoverageValidation) {
    createCriticalSourceFiles();
    createComprehensiveTestFiles();

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(95.0, 90.0, 85.0, 95.0, 80.0, true));

    // Validate condition coverage
    ComprehensiveCoverageValidationResult result;
    ASSERT_TRUE(validator_->validateConditionCoverage(result));

    // Verify condition coverage
    EXPECT_TRUE(result.condition_coverage_threshold_met);
    EXPECT_GE(result.condition_coverage_percentage, 80.0);
}

// Test 6: Integration Coverage Validation
TEST_F(ComprehensiveCoverageValidationTest, IntegrationCoverageValidation) {
    createCriticalSourceFiles();
    createComprehensiveTestFiles();

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(95.0, 90.0, 85.0, 95.0, 80.0, true));

    // Validate integration coverage
    ComprehensiveCoverageValidationResult result;
    ASSERT_TRUE(validator_->validateIntegrationCoverage(result));

    // Verify integration coverage
    EXPECT_TRUE(result.integration_coverage_adequate);
    EXPECT_GE(result.integration_coverage_percentage, 70.0); // Integration coverage typically lower
    EXPECT_GT(result.integration_tests, 0);
}

// Test 7: Regression Coverage Validation
TEST_F(ComprehensiveCoverageValidationTest, RegressionCoverageValidation) {
    createCriticalSourceFiles();
    createComprehensiveTestFiles();

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(95.0, 90.0, 85.0, 95.0, 80.0, true));

    // Validate regression coverage
    ComprehensiveCoverageValidationResult result;
    ASSERT_TRUE(validator_->validateRegressionCoverage(result));

    // Verify regression coverage
    EXPECT_TRUE(result.regression_coverage_complete);
    EXPECT_GE(result.regression_coverage_percentage, 80.0);
}

// Test 8: Comprehensive Coverage Validation
TEST_F(ComprehensiveCoverageValidationTest, ComprehensiveCoverageValidation) {
    createCriticalSourceFiles();
    createComprehensiveTestFiles();

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(95.0, 90.0, 85.0, 95.0, 80.0, true));

    // Perform comprehensive validation
    ComprehensiveCoverageValidationResult result;
    ASSERT_TRUE(validator_->validateComprehensiveTestCoverage(result));

    // Verify comprehensive coverage
    EXPECT_TRUE(result.critical_path_coverage_achieved);
    EXPECT_TRUE(result.overall_coverage_threshold_met);
    EXPECT_TRUE(result.branch_coverage_threshold_met);
    EXPECT_TRUE(result.function_coverage_threshold_met);
    EXPECT_TRUE(result.condition_coverage_threshold_met);
    EXPECT_TRUE(result.integration_coverage_adequate);
    EXPECT_TRUE(result.regression_coverage_complete);

    // Verify metrics
    EXPECT_GE(result.overall_line_coverage_percentage, 90.0);
    EXPECT_GE(result.branch_coverage_percentage, 85.0);
    EXPECT_GE(result.function_coverage_percentage, 95.0);
    EXPECT_GE(result.condition_coverage_percentage, 80.0);
}

// Test 9: Critical Path Identification
TEST_F(ComprehensiveCoverageValidationTest, CriticalPathIdentification) {
    createCriticalSourceFiles();

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));

    // Identify critical paths
    std::vector<CriticalCodePath> critical_paths;
    ASSERT_TRUE(validator_->identifyCriticalPaths(critical_paths));

    // Verify critical path identification
    EXPECT_GT(critical_paths.size(), 0);

    // Check for different categories
    std::set<CriticalPathCategory> categories_found;
    for (const auto& path : critical_paths) {
        categories_found.insert(path.category);
        EXPECT_FALSE(path.path_name.empty());
        EXPECT_FALSE(path.filepath.empty());
        EXPECT_GE(path.start_line, 0);
        EXPECT_GE(path.end_line, path.start_line);
    }

    // Should find multiple categories
    EXPECT_GT(categories_found.size(), 2);
}

// Test 10: Category-Specific Validation
TEST_F(ComprehensiveCoverageValidationTest, CategorySpecificValidation) {
    createCriticalSourceFiles();
    createComprehensiveTestFiles();

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(95.0, 90.0, 85.0, 95.0, 80.0, true));

    ComprehensiveCoverageValidationResult result;

    // Test each category
    EXPECT_TRUE(validator_->validateKernelExecutionPaths(result));
    EXPECT_TRUE(validator_->validateECCOperationPaths(result));
    EXPECT_TRUE(validator_->validateMemoryManagementPaths(result));
    EXPECT_TRUE(validator_->validateErrorHandlingPaths(result));
    EXPECT_TRUE(validator_->validateConfigurationLoadingPaths(result));
    EXPECT_TRUE(validator_->validateGPUInitializationPaths(result));
    EXPECT_TRUE(validator_->validateResultValidationPaths(result));
    EXPECT_TRUE(validator_->validatePerformanceCriticalPaths(result));
}

// Test 11: Coverage Gap Analysis
TEST_F(ComprehensiveCoverageValidationTest, CoverageGapAnalysis) {
    // Create source files but minimal test coverage
    createCriticalSourceFiles();
    createTestFile("minimal_test.cpp", R"(
#include <gtest/gtest.h>

TEST(MinimalTest, BasicTest) {
    EXPECT_EQ(1, 1);
}
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(95.0, 90.0, 85.0, 95.0, 80.0, true));

    // Validate coverage (should identify gaps)
    ComprehensiveCoverageValidationResult result;
    ASSERT_TRUE(validator_->validateComprehensiveTestCoverage(result));

    // Should identify coverage gaps
    EXPECT_FALSE(result.critical_path_coverage_achieved);
    EXPECT_GT(result.uncovered_critical_paths, 0);
    EXPECT_GT(result.uncovered_critical_functions, 0);
    EXPECT_FALSE(result.coverage_gaps.empty());

    // Analyze coverage gaps
    std::vector<std::string> gaps;
    ASSERT_TRUE(validator_->identifyCoverageGaps(result, gaps));
    EXPECT_GT(gaps.size(), 0);
}

// Test 12: Risk Assessment
TEST_F(ComprehensiveCoverageValidationTest, RiskAssessment) {
    // Create source files with partial test coverage
    createCriticalSourceFiles();
    createTestFile("partial_test.cpp", R"(
#include <gtest/gtest.h>
#include "ecc_operations.cpp"

TEST(PartialTest, SomeECCOperations) {
    CriticalECCOperations ecc;

    uint32_t private_key[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint32_t public_key[8] = {0};

    // Test only happy path
    bool result = ecc.computePublicKey(private_key, public_key);
    EXPECT_TRUE(result);
}
)");

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(95.0, 90.0, 85.0, 95.0, 80.0, true));

    // Validate and assess risk
    ComprehensiveCoverageValidationResult result;
    ASSERT_TRUE(validator_->validateComprehensiveTestCoverage(result));
    ASSERT_TRUE(validator_->assessUncoveredCodeRisk(result, result));

    // Should identify some risk due to incomplete coverage
    EXPECT_GT(result.overall_risk_score, 0.0);
    EXPECT_FALSE(result.high_risk_uncovered_areas.empty() ||
                result.medium_risk_uncovered_areas.empty());
}

// Test 13: Report Generation
TEST_F(ComprehensiveCoverageValidationTest, ReportGeneration) {
    createCriticalSourceFiles();
    createComprehensiveTestFiles();

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(95.0, 90.0, 85.0, 95.0, 80.0, true));

    // Validate and generate reports
    ComprehensiveCoverageValidationResult result;
    ASSERT_TRUE(validator_->validateComprehensiveTestCoverage(result));

    std::string comprehensive_report;
    ASSERT_TRUE(validator_->generateComprehensiveCoverageReport(result, comprehensive_report));

    std::string critical_path_report;
    ASSERT_TRUE(validator_->generateCriticalPathReport(result, critical_path_report));

    std::string risk_assessment_report;
    ASSERT_TRUE(validator_->generateRiskAssessmentReport(result, risk_assessment_report));

    // Verify report content
    EXPECT_THAT(comprehensive_report, Contains("Comprehensive Coverage Validation Report"));
    EXPECT_THAT(comprehensive_report, Contains("Executive Summary"));
    EXPECT_THAT(comprehensive_report, Contains("Critical Path Analysis"));
    EXPECT_THAT(comprehensive_report, Contains("Coverage Breakdown"));
    EXPECT_THAT(comprehensive_report, Contains("Risk Assessment"));

    EXPECT_THAT(critical_path_report, Contains("Critical Path Coverage Report"));
    EXPECT_THAT(risk_assessment_report, Contains("Risk Assessment Report"));
}

// Test 14: Performance and Scalability Test
TEST_F(ComprehensiveCoverageValidationTest, PerformanceAndScalabilityTest) {
    // Create many source files
    for (int i = 0; i < 10; ++i) {
        std::string content = R"(
#include <cuda_runtime.h>

class TestClass)" + std::to_string(i) + R"( {
public:
    void criticalMethod() {
        // Critical computation
        for (int j = 0; j < 100; ++j) {
            int result = j * 2;
            if (result > 50) {
                // Critical branch
                result = result / 2;
            }
        }
    }

    bool errorHandlingMethod(int param) {
        if (param < 0) {
            return false; // Critical error handling
        }
        return true;
    }
};
)";
        createSourceFile("test_source_" + std::to_string(i) + ".cpp", content);

        // Create corresponding test files
        std::string test_content = R"(
#include <gtest/gtest.h>
#include "test_source_)" + std::to_string(i) + R"(.cpp"

TEST(TestClass)" + std::to_string(i) + R"(Test, CriticalMethodTest) {
    TestClass)" + std::to_string(i) + R"( obj;
    obj.criticalMethod();
}

TEST(TestClass)" + std::to_string(i) + R"(Test, ErrorHandlingTest) {
    TestClass)" + std::to_string(i) + R"( obj;
    EXPECT_TRUE(obj.errorHandlingMethod(10));
    EXPECT_FALSE(obj.errorHandlingMethod(-1));
}
)";
        createTestFile("test_source_" + std::to_string(i) + "_test.cpp", test_content);
    }

    // Initialize validator
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(95.0, 90.0, 85.0, 95.0, 80.0, true));

    // Measure validation performance
    auto start_time = std::chrono::high_resolution_clock::now();

    ComprehensiveCoverageValidationResult result;
    ASSERT_TRUE(validator_->validateComprehensiveTestCoverage(result));

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Verify performance and results
    EXPECT_LT(duration.count(), 15000); // Should complete within 15 seconds
    EXPECT_EQ(result.total_files_analyzed, 20); // 10 source + 10 test files
    EXPECT_GT(result.total_tests, 0);
}

// Test 15: Configuration and Threshold Testing
TEST_F(ComprehensiveCoverageValidationTest, ConfigurationAndThresholdTesting) {
    createCriticalSourceFiles();
    createComprehensiveTestFiles();

    // Test with strict thresholds
    ASSERT_TRUE(validator_->initialize(test_directory_.string()));
    ASSERT_TRUE(validator_->configure(100.0, 95.0, 90.0, 100.0, 85.0, true));

    ComprehensiveCoverageValidationResult strict_result;
    ASSERT_TRUE(validator_->validateComprehensiveTestCoverage(strict_result));

    // Some tests might fail with strict thresholds
    if (!strict_result.critical_path_coverage_achieved) {
        EXPECT_LT(strict_result.critical_path_coverage_percentage, 100.0);
    }

    // Test with lenient thresholds
    ASSERT_TRUE(validator_->configure(80.0, 70.0, 60.0, 80.0, 50.0, false));

    ComprehensiveCoverageValidationResult lenient_result;
    ASSERT_TRUE(validator_->validateComprehensiveTestCoverage(lenient_result));

    // Should pass with lenient thresholds
    EXPECT_TRUE(lenient_result.critical_path_coverage_achieved);
    EXPECT_TRUE(lenient_result.overall_coverage_threshold_met);
}

// Test 16: Utility Functions Test
TEST_F(ComprehensiveCoverageValidationTest, UtilityFunctionsTest) {
    createCriticalSourceFiles();
    createComprehensiveTestFiles();

    // Test utility functions
    EXPECT_TRUE(comprehensive_coverage_validation_utils::quickCriticalPathCheck(test_directory_.string()));
    EXPECT_TRUE(comprehensive_coverage_validation_utils::validateOverallCoverage(test_directory_.string()));
    EXPECT_TRUE(comprehensive_coverage_validation_utils::checkIntegrationCoverage(test_directory_.string()));

    // Test metrics calculation
    double critical_coverage = comprehensive_coverage_validation_utils::calculateCriticalPathCoverage(test_directory_.string());
    EXPECT_GE(critical_coverage, 0.0);
    EXPECT_LE(critical_coverage, 100.0);

    // Test file coverage scores
    std::map<std::string, double> file_scores = comprehensive_coverage_validation_utils::calculateModuleCoverageScores(test_directory_.string());
    EXPECT_GT(file_scores.size(), 0);

    // Test risk assessment
    std::vector<std::string> risks = comprehensive_coverage_validation_utils::assessCoverageRisks(test_directory_.string());
    // With good coverage, should have minimal risks
    EXPECT_TRUE(risks.size() <= 10);
}

// Main function for running tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
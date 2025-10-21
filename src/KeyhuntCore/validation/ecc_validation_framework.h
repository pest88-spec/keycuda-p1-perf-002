// Puzzle71 Technical Debt Repair - ECC Validation Framework
// Task: T054 [P] [US3] Create ECC operation validation framework with CPU reference comparison
// Phase: Phase 4 - User Story 3 Integration Testing and Validation System

#pragma once

#include <cuda_runtime.h>
#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <cmath>
#include <secp256k1.h>

namespace puzzle71 {
namespace validation {

// ECC validation result structure
struct ECCValidationResult {
    bool is_valid;
    double max_error;
    double mean_error;
    double relative_error;
    size_t operations_tested;
    size_t operations_passed;
    std::vector<size_t> failed_indices;
    std::string error_details;

    ECCValidationResult() : is_valid(false), max_error(0.0), mean_error(0.0),
                           relative_error(0.0), operations_tested(0), operations_passed(0) {}
};

// ECC operation types for validation
enum class ECCOperationType {
    PUBLIC_KEY_GENERATION,
    SCALAR_MULTIPLICATION,
    POINT_ADDITION,
    POINT_DOUBLING,
    BATCH_INVERSION,
    COMPLETE_ECDSA
};

// ECC validation configuration
struct ECCValidationConfig {
    size_t batch_size = 10000;           // Number of operations per batch
    double precision_tolerance = 1e-10;  // Constitutional v5.5 requirement
    size_t max_iterations = 100000;      // Maximum validation iterations
    bool enable_performance_testing = true;
    bool enable_deterministic_testing = true;
    size_t random_seed = 12345;           // Reproducible testing seed

    // Performance thresholds
    double min_throughput_mkeys_per_sec = 500.0;  // Minimum throughput
    double max_execution_time_ms = 10000.0;       // Maximum execution time
};

// GPU ECC operation state for deterministic replay
struct GPUECCState {
    std::vector<uint8_t> device_memory_snapshot;
    std::vector<uint32_t> register_states;
    uint64_t kernel_launch_params[8];
    uint32_t thread_grid_dims[3];
    uint32_t thread_block_dims[3];
    uint64_t checksum;

    GPUECCState() : checksum(0) {
        memset(kernel_launch_params, 0, sizeof(kernel_launch_params));
        memset(thread_grid_dims, 0, sizeof(thread_grid_dims));
        memset(thread_block_dims, 0, sizeof(thread_block_dims));
    }
};

// Main ECC validation framework class
class ECCValidationFramework {
public:
    ECCValidationFramework();
    ~ECCValidationFramework();

    // Initialization and cleanup
    bool initialize(const ECCValidationConfig& config = ECCValidationConfig{});
    void cleanup();

    // Core validation methods
    bool validatePublicKeyGeneration(ECCValidationResult& result);
    bool validateScalarMultiplication(ECCValidationResult& result);
    bool validatePointAddition(ECCValidationResult& result);
    bool validatePointDoubling(ECCValidationResult& result);
    bool validateBatchInversion(ECCValidationResult& result);
    bool validateCompleteECDSA(ECCValidationResult& result);

    // Comprehensive validation
    bool validateAllECCOperations(std::vector<ECCValidationResult>& results);
    bool performConstitutionalComplianceValidation(ECCValidationResult& result);

    // Deterministic replay capabilities
    bool captureECCOperationState(ECCOperationType operation, GPUECCState& state);
    bool replayECCOperationFromState(const GPUECCState& state, std::vector<uint8_t>& result);
    bool validateECCDeterministicReplay(ECCOperationType operation);

    // Performance validation
    bool measureECCPerformance(ECCOperationType operation, double& throughput_mkeys_per_sec);
    bool validatePerformanceRequirements(ECCOperationType operation, ECCValidationResult& result);

    // Large-scale validation (10,000+ operations)
    bool performLargeScaleValidation(ECCOperationType operation, size_t operation_count,
                                   ECCValidationResult& result);

    // CPU reference implementation methods
    bool initializeCPUReference();
    void cleanupCPUReference();

    // Report generation
    bool generateECCValidationReport(std::string& report);
    bool generatePerformanceReport(std::string& report);

    // Configuration management
    void setValidationConfig(const ECCValidationConfig& config);
    const ECCValidationConfig& getValidationConfig() const;

    // Status and diagnostics
    bool isInitialized() const { return initialized_; }
    std::string getLastError() const { return last_error_; }
    size_t getTotalValidationsPerformed() const { return total_validations_; }

private:
    // Internal helper methods
    bool generateTestPrivateKeys(std::vector<std::vector<uint8_t>>& private_keys);
    bool computeCPUPublicKeys(const std::vector<std::vector<uint8_t>>& private_keys,
                             std::vector<std::vector<uint8_t>>& public_keys);
    bool computeGPUPublicKeys(const std::vector<std::vector<uint8_t>>& private_keys,
                             std::vector<std::vector<uint8_t>>& public_keys);

    bool compareECCResults(const std::vector<std::vector<uint8_t>>& cpu_results,
                          const std::vector<std::vector<uint8_t>>& gpu_results,
                          ECCValidationResult& result);

    bool computeScalarMultiplyCPU(const std::vector<uint8_t>& scalar,
                                 const std::vector<uint8_t>& point,
                                 std::vector<uint8_t>& result);
    bool computeScalarMultiplyGPU(const std::vector<uint8_t>& scalar,
                                 const std::vector<uint8_t>& point,
                                 std::vector<uint8_t>& result);

    bool computePointAdditionCPU(const std::vector<uint8_t>& point1,
                                const std::vector<uint8_t>& point2,
                                std::vector<uint8_t>& result);
    bool computePointAdditionGPU(const std::vector<uint8_t>& point1,
                                const std::vector<uint8_t>& point2,
                                std::vector<uint8_t>& result);

    bool computePointDoublingCPU(const std::vector<uint8_t>& point,
                                std::vector<uint8_t>& result);
    bool computePointDoublingGPU(const std::vector<uint8_t>& point,
                                std::vector<uint8_t>& result);

    // Performance measurement helpers
    bool measureKernelExecutionTime(cudaEvent_t start, cudaEvent_t stop, double& time_ms);
    bool calculateThroughput(size_t operations_count, double time_ms, double& throughput_mkeys_per_sec);

    // Error handling
    void setError(const std::string& error);
    void clearError();

    // CUDA device management
    bool initializeCUDADevice();
    void cleanupCUDADevice();

    // Memory management
    bool allocateGPUMemory(size_t size, void** device_ptr);
    bool freeGPUMemory(void* device_ptr);
    bool copyToDevice(const void* host_data, size_t size, void* device_ptr);
    bool copyFromDevice(const void* device_ptr, size_t size, void* host_data);

private:
    // Configuration and state
    ECCValidationConfig config_;
    bool initialized_;
    std::string last_error_;
    size_t total_validations_;

    // CUDA resources
    int cuda_device_id_;
    cudaStream_t cuda_stream_;
    cudaEvent_t timing_start_;
    cudaEvent_t timing_stop_;

    // CPU reference (libsecp256k1)
    secp256k1_context* secp_context_;
    bool cpu_reference_initialized_;

    // GPU memory buffers
    void* device_private_keys_;
    void* device_public_keys_;
    void* device_intermediate_results_;
    size_t buffer_size_;

    // Deterministic replay state
    GPUECCState captured_state_;
    bool state_captured_;

    // Performance tracking
    std::vector<double> performance_history_;
    std::vector<ECCValidationResult> validation_history_;
};

// Utility functions for ECC validation
namespace ecc_validation_utils {

    // Conversion utilities
    bool compressPublicKey(const std::vector<uint8_t>& uncompressed_pubkey,
                          std::vector<uint8_t>& compressed_pubkey);
    bool decompressPublicKey(const std::vector<uint8_t>& compressed_pubkey,
                            std::vector<uint8_t>& uncompressed_pubkey);

    // Validation utilities
    bool isValidPrivateKey(const std::vector<uint8_t>& private_key);
    bool isValidPublicKey(const std::vector<uint8_t>& public_key);
    bool isValidCurvePoint(const std::vector<uint8_t>& point);

    // Error calculation utilities
    double calculateMaxError(const std::vector<std::vector<uint8_t>>& reference,
                           const std::vector<std::vector<uint8_t>>& test);
    double calculateMeanError(const std::vector<std::vector<uint8_t>>& reference,
                            const std::vector<std::vector<uint8_t>>& test);
    double calculateRelativeError(const std::vector<std::vector<uint8_t>>& reference,
                                const std::vector<std::vector<uint8_t>>& test);

    // Random data generation for testing
    bool generateSecureRandomPrivateKeys(size_t count,
                                       std::vector<std::vector<uint8_t>>& private_keys,
                                       uint32_t seed = 0);
    bool generateKnownTestVectors(std::vector<std::vector<uint8_t>>& test_cases);

    // CUDA error handling
    const char* getCUDAErrorString(cudaError_t error);
    bool checkCUDAError(cudaError_t error, const std::string& operation);
}

// Constants for ECC validation (Constitutional v5.5 compliance)
namespace ecc_validation_constants {
    constexpr double CONSTITUTIONAL_PRECISION_TOLERANCE = 1e-10;
    constexpr double CONSTITUTIONAL_MIN_THROUGHPUT_MKEYS_PER_SEC = 500.0;
    constexpr size_t CONSTITUTIONAL_MIN_BATCH_SIZE = 10000;
    constexpr double CONSTITUTIONAL_MAX_ERROR_RATE = 0.0;  // Zero tolerance for errors
    constexpr size_t CONSTITUTIONAL_MAX_VALIDATION_TIME_MS = 30000;  // 30 seconds max

    // secp256k1 curve constants
    constexpr size_t PRIVATE_KEY_SIZE = 32;
    constexpr size_t COMPRESSED_PUBLIC_KEY_SIZE = 33;
    constexpr size_t UNCOMPRESSED_PUBLIC_KEY_SIZE = 65;
    constexpr size_t COORDINATE_SIZE = 32;

    // Validation thresholds
    constexpr size_t MIN_OPERATIONS_FOR_STATISTICAL_SIGNIFICANCE = 1000;
    constexpr double MAX_PERFORMANCE_VARIANCE_PERCENT = 10.0;
    constexpr size_t MAX_FAILED_OPERATIONS = 0;  // Zero tolerance
}

} // namespace validation
} // namespace puzzle71
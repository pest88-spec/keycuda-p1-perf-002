// Puzzle71Solver - Deterministic Replay Framework Header
// Implements deterministic replay validation system for GPU operations for T055

#pragma once

#include <cuda_runtime.h>
#include <vector>
#include <string>
#include <cstdint>
#include <chrono>
#include <memory>

namespace keyhunt {
namespace validation {

/**
 * Deterministic state capture structure
 * Represents the complete state of a GPU operation for replay
 */
struct DeterministicState {
    std::vector<uint8_t> memory_data;       // GPU memory state
    size_t memory_size;                      // Size of memory data
    std::vector<uint64_t> registers;         // GPU register state
    size_t register_count;                   // Number of registers
    uint64_t checksum;                       // State checksum for validation
    std::string operation_id;                 // Unique operation identifier
    std::chrono::time_point<std::chrono::high_resolution_clock> capture_time; // Capture timestamp
    std::vector<uint8_t> kernel_parameters; // Kernel launch parameters
    dim3 grid_dim;                          // Grid dimensions
    dim3 block_dim;                         // Block dimensions
    size_t shared_memory_size;               // Shared memory size
    cudaStream_t stream_id;                  // CUDA stream identifier

    DeterministicState() : memory_size(0), register_count(0), checksum(0),
                          shared_memory_size(0), stream_id(0) {}
};

/**
 * Replay result structure
 * Contains results from deterministic replay operations
 */
struct ReplayResult {
    bool deterministic;                      // Whether replay was deterministic
    double bit_error_rate;                  // Bit error rate (percentage)
    double performance_variance_ms;          // Performance variance in milliseconds
    std::vector<uint8_t> result_data;       // Result data from replay
    std::chrono::microseconds execution_time; // Execution time
    std::string operation_id;                // Associated operation ID
    bool validation_passed;                  // Whether validation passed
    std::string error_message;               // Error message if validation failed

    ReplayResult() : deterministic(false), bit_error_rate(0.0),
                     performance_variance_ms(0.0), validation_passed(false) {}
};

/**
 * Deterministic Replay Framework
 *
 * Provides comprehensive deterministic replay validation for GPU operations
 * ensuring consistent and reproducible results across multiple executions.
 *
 * Features:
 * - State capture and restore functionality
 * - 100% determinism validation
 * - Performance consistency monitoring
 * - Multi-GPU cross-validation
 * - Comprehensive reporting
 * - Constitutional compliance validation
 */
class DeterministicReplayFramework {
public:
    /**
     * Constructor
     */
    DeterministicReplayFramework();

    /**
     * Destructor
     */
    virtual ~DeterministicReplayFramework();

    /**
     * Initialize the deterministic replay framework
     * @return true if initialization successful
     */
    virtual bool initialize();

    /**
     * Capture deterministic state for an operation
     * @param operation_id Unique identifier for the operation
     * @param state Output captured state
     * @return true if state capture successful
     */
    virtual bool captureDeterministicState(const std::string& operation_id,
                                          DeterministicState& state);

    /**
     * Replay operation from captured state
     * @param state Previously captured state
     * @param result Output result from replay
     * @return true if replay successful
     */
    virtual bool replayFromState(const DeterministicState& state,
                                 std::vector<uint8_t>& result);

    /**
     * Validate deterministic replay for an operation
     * @param operation_id Operation to validate
     * @return true if operation is deterministic
     */
    virtual bool validateDeterministicReplay(const std::string& operation_id);

    /**
     * Capture and replay multiple operations in batch
     * @param operation_ids Vector of operation identifiers
     * @param results Output vector of replay results
     * @return true if batch replay successful
     */
    virtual bool captureAndReplayBatch(const std::vector<std::string>& operation_ids,
                                       std::vector<ReplayResult>& results);

    /**
     * Get determinism metrics
     * @param exact_match_rate Output exact match rate (percentage)
     * @param bit_error_rate Output bit error rate (percentage)
     * @param performance_variance Output performance variance (percentage)
     * @return true if metrics available
     */
    virtual bool getDeterminismMetrics(double& exact_match_rate,
                                       double& bit_error_rate,
                                       double& performance_variance);

    /**
     * Generate comprehensive determinism report
     * @param report Output string containing formatted report
     * @return true if report generated successfully
     */
    virtual bool generateDeterminismReport(std::string& report);

    /**
     * Validate determinism across multiple GPU devices
     * @param device_ids Vector of GPU device IDs to test
     * @return true if multi-GPU determinism validated
     */
    virtual bool validateMultiGpuDeterminism(const std::vector<int>& device_ids);

    /**
     * Test determinism across different platforms
     * @param platforms Vector of platform identifiers
     * @return true if cross-platform determinism validated
     */
    virtual bool testCrossPlatformDeterminism(const std::vector<std::string>& platforms);

    // Constitutional compliance constants
    static constexpr double DETERMINISM_TOLERANCE = 0.0;            // Must be exactly 0.0
    static constexpr int REPLAY_ITERATIONS = 10;                     // Number of replay iterations
    static constexpr double PERFORMANCE_VARIANCE_TOLERANCE = 5.0;   // Performance variance tolerance (%)
    static constexpr int COMPLEX_WORKLOAD_SIZE = 50000;             // Size of complex workloads

private:
    // Internal state
    bool initialized_;
    int current_device_id_;
    std::vector<DeterministicState> captured_states_;
    std::vector<ReplayResult> replay_results_;

    // Determinism metrics
    struct DeterminismMetrics {
        int total_operations;
        int deterministic_operations;
        double total_bit_errors;
        double total_performance_variance;
        double exact_match_rate;
        double bit_error_rate;
        double performance_variance;
        std::vector<double> performance_samples;
    } metrics_;

    // Helper methods
    bool initializeCuda();
    bool captureMemoryState(DeterministicState& state);
    bool captureRegisterState(DeterministicState& state);
    bool restoreMemoryState(const DeterministicState& state);
    bool restoreRegisterState(const DeterministicState& state);
    uint64_t calculateStateChecksum(const DeterministicState& state);
    double compareResults(const std::vector<uint8_t>& result1, const std::vector<uint8_t>& result2);
    void updateMetrics(const ReplayResult& result);
    void resetMetrics();

    // GPU operation simulation
    std::vector<uint8_t> simulateGPUOperation(const DeterministicState& state);
    std::vector<uint8_t> simulateECCOperation(const std::vector<uint8_t>& input_data, size_t data_size);
    std::vector<uint8_t> simulateHashOperation(const std::vector<uint8_t>& input_data, size_t data_size);
    std::vector<uint8_t> simulateMemoryOperation(const std::vector<uint8_t>& input_data, size_t data_size);

    // Operation type detection
    enum class OperationType {
        ECC_OPERATION,
        HASH_OPERATION,
        MEMORY_OPERATION,
        UNKNOWN_OPERATION
    };

    OperationType detectOperationType(const std::string& operation_id);
    std::vector<uint8_t> generateOperationData(OperationType type, size_t size);
};

} // namespace validation
} // namespace keyhunt
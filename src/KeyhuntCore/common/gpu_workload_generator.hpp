// Puzzle71Solver - GPU Workload Generator Header
// Implements various GPU workloads for utilization testing

#pragma once

#include <cuda_runtime.h>
#include <vector>

namespace keyhunt {
namespace performance {

/**
 * GPU Workload Generator
 *
 * Generates various types of GPU workloads for utilization testing including:
 * - ECC (Elliptic Curve Cryptography) workloads
 * - Memory-intensive workloads
 * - Compute-intensive workloads
 * - Mixed workloads (balanced compute and memory)
 *
 * Features:
 * - Asynchronous workload execution
 * - Stream-based execution for concurrency
 * - Event-based completion tracking
 * - Configurable workload intensity
 * - Multi-workload management
 */
class GPUWorkloadGenerator {
public:
    /**
     * Constructor
     * @param device_id GPU device ID for workload generation
     */
    explicit GPUWorkloadGenerator(int device_id = 0);

    /**
     * Destructor
     */
    virtual ~GPUWorkloadGenerator() = default;

    /**
     * Generate ECC (Elliptic Curve Cryptography) workload
     * Simulates elliptic curve point multiplication and addition operations
     * @param operations Number of ECC operations to perform
     * @return true if workload generated successfully
     */
    bool generateECCWorkload(size_t operations);

    /**
     * Generate memory-intensive workload
     * Focuses on memory access patterns, bandwidth utilization, and caching
     * @param data_size Size of data set for memory operations
     * @return true if workload generated successfully
     */
    bool generateMemoryIntensiveWorkload(size_t data_size);

    /**
     * Generate compute-intensive workload
     * Focuses on arithmetic operations and ALU utilization
     * @param iterations Number of compute iterations
     * @return true if workload generated successfully
     */
    bool generateComputeIntensiveWorkload(int iterations);

    /**
     * Generate mixed workload (balanced compute and memory)
     * Combines both computational and memory operations
     * @param ops Number of operations
     * @param data_size Size of data set
     * @return true if workload generated successfully
     */
    bool generateMixedWorkload(size_t ops, size_t data_size);

    /**
     * Wait for all generated workloads to complete
     * @return true if all workloads completed successfully
     */
    bool waitForCompletion();

private:
    int device_id_;
    std::vector<cudaStream_t> streams_;
    std::vector<cudaEvent_t> completion_events_;

    // Helper methods
    cudaStream_t createStream();
    void recordCompletionEvent(cudaStream_t stream);
    bool waitForEvents();

    // CUDA kernel declarations
    static void eccWorkloadKernel(int operations);
    static void memoryIntensiveKernel(int data_size);
    static void computeIntensiveKernel(int iterations);
    static void mixedWorkloadKernel(int ops, int data_size);
};

} // namespace performance
} // namespace keyhunt
// Puzzle71Solver - GPU Utilization Analyzer Header
// Implements comprehensive GPU utilization measurement and analysis for T049

#pragma once

#include <cuda_runtime.h>
#include <nvml.h>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <string>
#include <utility>

namespace keyhunt {
namespace performance {

/**
 * GPU Utilization Analyzer
 *
 * Provides comprehensive measurement and analysis of GPU utilization metrics
 * including compute utilization, memory controller utilization, SM utilization,
 * occupancy, warp execution efficiency, and instruction throughput.
 *
 * Features:
 * - Real-time utilization monitoring
 * - NVML integration for detailed metrics
 * - CUDA-based fallback measurements
 * - Continuous monitoring with time series data
 * - Comprehensive utilization reporting
 * - Constitutional compliance validation
 *
 * Constitutional v5.5 Compliance:
 * - Static configuration only (no runtime device queries for configuration)
 * - Performance targets: GPU utilization ≥70%, occupancy ≥65%
 * - Monitoring intervals: 100ms sampling, 5s measurements
 */
class GPUUtilizationAnalyzer {
public:
    /**
     * Constructor
     * @param device_id GPU device ID to analyze
     */
    explicit GPUUtilizationAnalyzer(int device_id = 0);

    /**
     * Destructor
     */
    ~GPUUtilizationAnalyzer();

    /**
     * Initialize analyzer for specific GPU device
     * @param device_id GPU device ID
     * @return true if initialization successful
     */
    bool initialize(int device_id);

    /**
     * Measure overall GPU utilization percentage
     * @param utilization_percent Output parameter for utilization percentage
     * @return true if measurement successful
     */
    bool measureGPUUtilization(double& utilization_percent);

    /**
     * Measure compute utilization (GPU core utilization)
     * @param compute_utilization Output parameter for compute utilization percentage
     * @return true if measurement successful
     */
    bool measureComputeUtilization(double& compute_utilization);

    /**
     * Measure memory controller utilization
     * @param mem_utilization Output parameter for memory controller utilization percentage
     * @return true if measurement successful
     */
    bool measureMemoryControllerUtilization(double& mem_utilization);

    /**
     * Measure Streaming Multiprocessor (SM) utilization
     * @param sm_utilization Output parameter for SM utilization percentage
     * @return true if measurement successful
     */
    bool measureSMUtilization(double& sm_utilization);

    /**
     * Measure kernel execution occupancy
     * @param occupancy_percent Output parameter for occupancy percentage
     * @return true if measurement successful
     */
    bool measureOccupancy(double& occupancy_percent);

    /**
     * Measure warp execution efficiency
     * @param efficiency Output parameter for warp efficiency percentage
     * @return true if measurement successful
     */
    bool measureWarpExecutionEfficiency(double& efficiency);

    /**
     * Measure instruction throughput (instructions per second)
     * @param throughput Output parameter for instruction throughput
     * @return true if measurement successful
     */
    bool measureInstructionThroughput(double& throughput);

    /**
     * Start continuous monitoring of GPU utilization
     * @return true if monitoring started successfully
     */
    bool startContinuousMonitoring();

    /**
     * Stop continuous monitoring
     * @return true if monitoring stopped successfully
     */
    bool stopContinuousMonitoring();

    /**
     * Get utilization time series data from continuous monitoring
     * @param time_series Output vector of (timestamp, utilization) pairs
     * @return true if data available
     */
    bool getUtilizationTimeSeries(std::vector<std::pair<double, double>>& time_series);

    /**
     * Generate comprehensive utilization report
     * @param report Output string containing formatted report
     * @return true if report generated successfully
     */
    bool generateUtilizationReport(std::string& report);

    // Constitutional compliance constants
    static constexpr double GPU_UTILIZATION_MINIMUM = 70.0;    // Must exceed 70%
    static constexpr double GPU_UTILIZATION_TARGET = 85.0;     // Target 85%
    static constexpr double OCCUPANCY_TARGET = 65.0;            // Target 65% occupancy
    static constexpr int MEASUREMENT_DURATION_MS = 5000;       // 5 second measurements
    static constexpr int SAMPLING_INTERVAL_MS = 100;           // 100ms sampling

private:
    // Device and monitoring state
    int device_id_;
    nvmlDevice_t nvml_device_;
    nvmlReturn_t nvml_result_;
    std::atomic<bool> monitoring_active_;
    std::thread monitoring_thread_;

    // Monitoring parameters
    int sampling_interval_ms_;
    int measurement_duration_ms_;
    std::vector<std::pair<double, double>> utilization_time_series_;

    // Private methods
    void monitoringLoop();
    bool measureBasicGPUUtilization(double& utilization_percent);
    bool measureBasicComputeUtilization(double& compute_utilization);
    bool measureBasicMemoryUtilization(double& mem_utilization);
    int getOptimalBlockCount(int block_size);

    // CUDA kernels for measurement
    static void testOccupancyKernel();
    static void instructionThroughputKernel(int iterations);
    static void basicWorkloadKernel(int iterations);
};

/**
 * GPU Workload Generator
 *
 * Generates various types of GPU workloads for utilization testing
 * including ECC workloads, memory-intensive workloads, compute-intensive
 * workloads, and mixed workloads.
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
     * @param operations Number of ECC operations to perform
     * @return true if workload generated successfully
     */
    bool generateECCWorkload(size_t operations);

    /**
     * Generate memory-intensive workload
     * @param data_size Size of data set for memory operations
     * @return true if workload generated successfully
     */
    bool generateMemoryIntensiveWorkload(size_t data_size);

    /**
     * Generate compute-intensive workload
     * @param iterations Number of compute iterations
     * @return true if workload generated successfully
     */
    bool generateComputeIntensiveWorkload(int iterations);

    /**
     * Generate mixed workload (balanced compute and memory)
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
};

} // namespace performance
} // namespace keyhunt
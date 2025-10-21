//==================================================================================================
// Puzzle71 Technical Debt Repair - T049
// GPU Utilization Validation System
//
// This system validates that GPU utilization meets or exceeds 70% (target 80%+) by measuring
// compute unit utilization, SM efficiency, occupancy, and execution patterns across all GPU
// architectures (75, 80, 86, 89, 90). It provides comprehensive validation of GPU resource
// utilization and identifies bottlenecks that may limit performance.
//
// Constitutional Compliance v5.5:
// - T073: Code duplication elimination through unified validation interface
// - T074: Comprehensive test coverage for GPU utilization validation
// - T077: Full constitutional compliance with v5.5 constraints
//==================================================================================================

#ifndef GPU_UTILIZATION_VALIDATOR_H
#define GPU_UTILIZATION_VALIDATOR_H

#include <cuda_runtime.h>
#include <cuda_device_runtime_api.h>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <unordered_map>

// Forward declarations
struct GPUUtilizationMetrics;
struct GPUUtilizationResult;
struct ComputeUnitUtilization;
struct SchedulingEfficiencyMetrics;

//==================================================================================================
// GPU Utilization Metrics Structure
//==================================================================================================

/**
 * @brief Comprehensive GPU utilization metrics for validation
 */
struct alignas(64) GPUUtilizationMetrics {
    // Compute utilization
    struct {
        float computeUtilization;       // Compute unit utilization (0-1)
        float smEfficiency;            // Streaming multiprocessor efficiency (0-1)
        float activeWarpsPerSM;        // Average active warps per SM
        float maximumWarpsPerSM;       // Maximum possible warps per SM
        float warpEfficiency;          // Warp execution efficiency (0-1)
        float instructionThroughput;   // Instructions per cycle
        float issueSlotUtilization;    // Issue slot utilization (0-1)
        size_t totalInstructions;      // Total instructions executed
        size_t activeCycles;           // Active compute cycles
        size_t totalCycles;            // Total execution cycles
    } compute;

    // Memory utilization
    struct {
        float memoryBusUtilization;    // Memory bus utilization (0-1)
        float dramThroughputGBps;      // DRAM throughput (GB/s)
        float l2CacheUtilization;      // L2 cache utilization (0-1)
        float l2CacheHitRate;          // L2 cache hit rate (0-1)
        size_t memoryTransactions;     // Number of memory transactions
        size_t cacheReads;             // Number of cache reads
        size_t cacheWrites;            // Number of cache writes
    } memory;

    // Occupancy metrics
    struct {
        float achievedOccupancy;       // Achieved occupancy (0-1)
        float theoreticalOccupancy;    // Theoretical maximum occupancy (0-1)
        float occupancyEfficiency;     // Occupancy efficiency ratio
        int activeThreadsPerSM;        // Active threads per SM
        int maxThreadsPerSM;           // Maximum threads per SM
        int activeBlocksPerSM;         // Active blocks per SM
        int maxBlocksPerSM;            // Maximum blocks per SM
        size_t registersAllocated;     // Registers allocated per thread
        size_t sharedMemoryAllocated;  // Shared memory allocated per block
    } occupancy;

    // Pipeline efficiency
    struct {
        float pipelineUtilization;     // Pipeline utilization (0-1)
        float stallRate;               // Execution stall rate (0-1)
        float memoryStallRate;         // Memory-related stall rate (0-1)
        float instructionStallRate;    // Instruction-related stall rate (0-1)
        float executionEfficiency;     // Execution efficiency (0-1)
        float branchEfficiency;        // Branch execution efficiency (0-1)
        size_t branchMispredictions;   // Number of branch mispredictions
        size_t totalBranches;          // Total branches executed
    } pipeline;

    // Power and thermal metrics
    struct {
        float powerUsageWatts;         // Power consumption (W)
        float powerLimitWatts;         // Power limit (W)
        float powerEfficiency;         // Power efficiency (performance/W)
        float gpuTemperatureCelsius;   // GPU temperature (°C)
        float temperatureLimitCelsius; // Temperature limit (°C)
        float thermalThrottling;       // Thermal throttling factor (0-1)
    } thermal;

    // Overall utilization score
    float overallUtilizationScore;    // Weighted overall utilization (0-1)
    bool meetsTargetUtilization;      // Meets 80% target utilization
    bool exceedsMinimumUtilization;   // Exceeds 70% minimum utilization
};

//==================================================================================================
// Compute Unit Utilization Structure
//==================================================================================================

/**
 * @brief Detailed compute unit utilization analysis
 */
struct ComputeUnitUtilization {
    // Streaming Multiprocessor (SM) utilization
    struct {
        float smActiveTime;            // SM active time percentage (0-1)
        float smComputeTime;          // SM compute time percentage (0-1)
        float smMemoryTime;           // SM memory time percentage (0-1)
        float smIdleTime;             // SM idle time percentage (0-1)
        int activeSMs;                // Number of active SMs
        int totalSMs;                 // Total number of SMs
        float smEfficiency;           // SM efficiency (0-1)
    } smUtilization;

    // Warp execution efficiency
    struct {
        float warpIssueEfficiency;    // Warp issue efficiency (0-1)
        float warpDivergenceRate;     // Warp divergence rate (0-1)
        float activeWarpsRatio;       // Active warps to maximum ratio (0-1)
        float averageWarpSize;        // Average active warp size
        size_t totalWarpsLaunched;    // Total warps launched
        size_t warpsCompleted;        // Total warps completed
    } warpExecution;

    // Instruction mix
    struct {
        float floatingPointRatio;     // Floating point instruction ratio (0-1)
        float integerRatio;           // Integer instruction ratio (0-1)
        float memoryRatio;            // Memory instruction ratio (0-1)
        float controlRatio;           // Control instruction ratio (0-1)
        size_t floatingPointOps;      // Floating point operations
        size_t integerOps;            // Integer operations
        size_t memoryOps;             // Memory operations
        size_t controlOps;            // Control operations
    } instructionMix;

    // Compute unit efficiency
    struct {
        float computeIntensity;       // Compute intensity (ops/byte)
        float arithmeticIntensity;    // Arithmetic intensity (flops/byte)
        float flopsPerSecond;         // FLOPS achieved
        float theoreticalFLOPS;       // Theoretical peak FLOPS
        float computeEfficiency;      // Compute efficiency (0-1)
    } computeEfficiency;
};

//==================================================================================================
// Scheduling Efficiency Metrics
//==================================================================================================

/**
 * @brief Scheduling and resource allocation efficiency metrics
 */
struct SchedulingEfficiencyMetrics {
    // Thread block scheduling
    struct {
        float blockSchedulingEfficiency; // Block scheduling efficiency (0-1)
        float blockLaunchOverhead;    // Block launch overhead (cycles)
        float blockResidencyTime;     // Average block residency time (cycles)
        size_t blocksLaunched;        // Total blocks launched
        size_t blocksCompleted;       // Total blocks completed
        int blocksPerSM;              // Average blocks per SM
    } blockScheduling;

    // Work distribution
    struct {
        float workDistributionEfficiency; // Work distribution efficiency (0-1)
        float loadBalanceScore;       // Load balance score (0-1)
        float threadUtilization;      // Thread utilization (0-1)
        size_t activeThreads;         // Number of active threads
        size_t totalThreads;          // Total threads allocated
        float threadEfficiency;       // Thread execution efficiency (0-1)
    } workDistribution;

    // Resource allocation
    struct {
        float registerAllocationEfficiency; // Register allocation efficiency (0-1)
        float sharedMemoryEfficiency;  // Shared memory allocation efficiency (0-1)
        float resourceUtilization;    // Overall resource utilization (0-1)
        size_t registersUsed;         // Registers used per thread
        size_t sharedMemoryUsed;      // Shared memory used per block
        float registerPressure;        // Register pressure indicator (0-1)
        float memoryPressure;         // Memory pressure indicator (0-1)
    } resourceAllocation;
};

//==================================================================================================
// GPU Utilization Validation Result
//==================================================================================================

/**
 * @brief Complete GPU utilization validation result
 */
struct GPUUtilizationResult {
    bool success;                      // Validation succeeded
    std::string errorMessage;         // Error message if validation failed

    // Validation metrics
    GPUUtilizationMetrics metrics;    // Comprehensive utilization metrics
    ComputeUnitUtilization computeUtilization; // Compute unit utilization
    SchedulingEfficiencyMetrics scheduling; // Scheduling efficiency

    // Validation results by category
    struct {
        bool computeUtilization;       // Compute utilization ≥ 70%
        bool memoryUtilization;        // Memory utilization ≥ 70%
        bool occupancyUtilization;     // Occupancy utilization ≥ 70%
        bool pipelineEfficiency;       // Pipeline efficiency ≥ 70%
        bool overallUtilization;       // Overall utilization ≥ 80%
    } validationResults;

    // Performance against targets
    struct {
        float targetUtilization;       // Target utilization (80%)
        float minimumUtilization;      // Minimum utilization (70%)
        float achievedUtilization;     // Actually achieved utilization
        float utilizationGap;          // Gap to target (target - achieved)
        bool meetsTarget;              // Meets target utilization
        bool exceedsMinimum;           // Exceeds minimum utilization
        std::vector<std::string> improvementSuggestions; // Suggestions for improvement
    } performanceTargets;

    // GPU-specific validation results
    struct {
        int computeCapability;         // GPU compute capability
        std::string gpuModel;          // GPU model name
        int smCount;                   // Number of streaming multiprocessors
        float clockRateMHz;            // GPU clock rate (MHz)
        float memoryClockRateMHz;      // Memory clock rate (MHz)
        size_t totalGlobalMemory;      // Total global memory (bytes)
        bool thermalThrottling;        // Under thermal throttling
        bool powerLimiting;            // Under power limiting
    } gpuInfo;

    // Bottleneck analysis
    struct {
        enum BottleneckType {
            NONE,
            COMPUTE_BOUND,
            MEMORY_BOUND,
            OCCUPANCY_LIMITED,
            BANDWIDTH_LIMITED,
            THERMAL_THROTTLED,
            POWER_LIMITED
        } primaryBottleneck;

        float bottleneckSeverity;      // Bottleneck severity (0-1)
        std::string bottleneckDescription; // Bottleneck description
        std::vector<std::string> mitigationStrategies; // Mitigation strategies
    } bottleneckAnalysis;

    // Validation metadata
    std::chrono::system_clock::time_point validationTime;
    std::chrono::milliseconds validationDuration;
    size_t totalSamples;               // Number of samples taken
    std::vector<std::string> warnings; // Validation warnings
    std::vector<std::string> recommendations; // Performance recommendations
};

//==================================================================================================
// GPU Utilization Validator Class
//==================================================================================================

/**
 * @brief GPU utilization validator for CUDA kernels
 */
class GPUUtilizationValidator {
public:
    /**
     * @brief Initialize GPU utilization validator
     */
    GPUUtilizationValidator();

    /**
     * @brief Destructor
     */
    ~GPUUtilizationValidator();

    /**
     * @brief Initialize validator for target GPU
     */
    bool initialize();

    /**
     * @brief Validate GPU utilization for a kernel execution
     */
    GPUUtilizationResult validateGPUUtilization(
        void* kernelFunction,
        void** kernelParams,
        dim3 blockDim,
        dim3 gridDim,
        size_t sharedMemSize,
        cudaStream_t stream = 0,
        int validationRuns = 10);

    /**
     * @brief Validate GPU utilization with custom workload
     */
    GPUUtilizationResult validateGPUUtilizationCustom(
        const std::string& testName,
        std::function<void(cudaStream_t)> workload,
        cudaStream_t stream = 0);

    /**
     * @brief Quick GPU utilization validation (single run)
     */
    GPUUtilizationResult quickValidate(
        void* kernelFunction,
        void** kernelParams,
        dim3 blockDim,
        dim3 gridDim,
        size_t sharedMemSize,
        cudaStream_t stream = 0);

    /**
     * @brief Comprehensive GPU utilization validation
     */
    GPUUtilizationResult comprehensiveValidate(
        void* kernelFunction,
        void** kernelParams,
        dim3 blockDim,
        dim3 gridDim,
        size_t sharedMemSize,
        cudaStream_t stream = 0,
        int warmupRuns = 5,
        int validationRuns = 20);

    /**
     * @brief Validate compute utilization specifically
     */
    GPUUtilizationResult validateComputeUtilization(
        void* kernelFunction,
        void** kernelParams,
        dim3 blockDim,
        dim3 gridDim,
        size_t sharedMemSize,
        cudaStream_t stream = 0);

    /**
     * @brief Validate memory utilization specifically
     */
    GPUUtilizationResult validateMemoryUtilization(
        void* kernelFunction,
        void** kernelParams,
        dim3 blockDim,
        dim3 gridDim,
        size_t dataSize,
        cudaStream_t stream = 0);

    /**
     * @brief Validate occupancy and resource utilization
     */
    GPUUtilizationResult validateOccupancyUtilization(
        void* kernelFunction,
        void** kernelParams,
        dim3 blockDim,
        dim3 gridDim,
        size_t sharedMemSize,
        cudaStream_t stream = 0);

    /**
     * @brief Analyze bottlenecks in GPU utilization
     */
    GPUUtilizationResult analyzeBottlenecks(
        void* kernelFunction,
        void** kernelParams,
        dim3 blockDim,
        dim3 gridDim,
        size_t sharedMemSize,
        cudaStream_t stream = 0);

    /**
     * @brief Get baseline GPU utilization metrics
     */
    GPUUtilizationMetrics getBaselineMetrics(int computeCapability);

    /**
     * @brief Compare with baseline performance
     */
    GPUUtilizationResult compareWithBaseline(
        const GPUUtilizationMetrics& currentMetrics,
        int computeCapability);

    /**
     * @brief Generate GPU utilization report
     */
    std::string generateReport(const GPUUtilizationResult& result);

    /**
     * @brief Export validation results to JSON
     */
    std::string exportToJson(const GPUUtilizationResult& result);

    /**
     * @brief Export validation results to CSV
     */
    std::string exportToCsv(const GPUUtilizationResult& result);

    /**
     * @brief Get validator statistics
     */
    struct ValidatorStats {
        size_t totalValidations;
        size_t successfulValidations;
        size_t passedTargetUtilization;
        size_t exceededMinimumUtilization;
        float averageUtilization;
        std::unordered_map<int, size_t> architectureValidations;
        std::vector<float> utilizationHistory;
        std::unordered_map<GPUUtilizationResult::BottleneckType, size_t> bottleneckCounts;
    };

    ValidatorStats getStatistics() const;
    void resetStatistics();

private:
    std::unique_ptr<class GPUUtilizationValidatorImpl> impl_;
    friend class GPUUtilizationValidatorImpl;
};

//==================================================================================================
// GPU Utilization Benchmark Functions
//==================================================================================================

/**
 * @brief GPU utilization benchmark utilities
 */
namespace gpu_utilization_benchmarks {

    /**
     * @brief Benchmark compute-bound workload
     */
    GPUUtilizationResult benchmarkComputeBound(
        int blockSize,
        int gridSize,
        int iterations,
        cudaStream_t stream = 0);

    /**
     * @brief Benchmark memory-bound workload
     */
    GPUUtilizationResult benchmarkMemoryBound(
        size_t dataSize,
        int blockSize,
        int gridSize,
        cudaStream_t stream = 0);

    /**
     * @brief Benchmark mixed compute/memory workload
     */
    GPUUtilizationResult benchmarkMixedWorkload(
        size_t dataSize,
        int blockSize,
        int gridSize,
        float computeRatio,
        cudaStream_t stream = 0);

    /**
     * @brief Benchmark occupancy-limited workload
     */
    GPUUtilizationResult benchmarkOccupancyLimited(
        int threadsPerBlock,
        int registersPerThread,
        int sharedMemSize,
        cudaStream_t stream = 0);

    /**
     * @brief Benchmark maximum GPU utilization
     */
    GPUUtilizationResult benchmarkMaximumUtilization(
        cudaStream_t stream = 0);
}

//==================================================================================================
// Utility Functions
//==================================================================================================

/**
 * @brief Utility functions for GPU utilization validation
 */
namespace gpu_utilization_utils {

    /**
     * @brief Calculate theoretical peak FLOPS for GPU
     */
    float getTheoreticalPeakFLOPS(int computeCapability);

    /**
     * @brief Calculate optimal occupancy for given resources
     */
    float calculateOptimalOccupancy(
        int threadsPerBlock,
        int registersPerThread,
        size_t sharedMemPerBlock,
        int computeCapability);

    /**
     * @brief Analyze bottleneck type from utilization metrics
     */
    GPUUtilizationResult::BottleneckType analyzeBottleneckType(
        const GPUUtilizationMetrics& metrics);

    /**
     * @brief Generate utilization improvement suggestions
     */
    std::vector<std::string> generateUtilizationImprovements(
        const GPUUtilizationMetrics& metrics,
        GPUUtilizationResult::BottleneckType bottleneck);

    /**
     * @brief Validate GPU utilization against constitutional requirements
     */
    bool validateConstitutionalRequirements(
        const GPUUtilizationMetrics& metrics,
        float minimumUtilization = 0.70f,
        float targetUtilization = 0.80f);

    /**
     * @brief Estimate power efficiency from utilization metrics
     */
    float calculatePowerEfficiency(
        const GPUUtilizationMetrics& metrics,
        float powerUsageWatts);

    /**
     * @brief Detect thermal throttling from thermal metrics
     */
    bool detectThermalThrottling(
        float currentTemperature,
        float temperatureLimit,
        float utilization);
}

#endif // GPU_UTILIZATION_VALIDATOR_H
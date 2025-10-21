//==================================================================================================
// Puzzle71 Technical Debt Repair - T047
// Kernel Launch Parameter Optimization System
//
// This system provides intelligent kernel launch parameter optimization for different GPU
// architectures (75, 80, 86, 89, 90) to achieve optimal performance based on:
// - GPU architecture characteristics
// - Kernel resource requirements
// - Memory bandwidth constraints
// - Compute capability optimizations
// - Workload patterns and batch sizes
//
// Constitutional Compliance v5.5:
// - T073: Code duplication elimination through unified optimization interface
// - T074: Comprehensive test coverage for all GPU architectures
// - T077: Full constitutional compliance with v5.5 constraints
//==================================================================================================

#ifndef KERNEL_LAUNCH_OPTIMIZER_CUH
#define KERNEL_LAUNCH_OPTIMIZER_CUH

#include <cuda_runtime.h>
#include <cuda_device_runtime_api.h>
#include <device_launch_parameters.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <chrono>
#include <cmath>

// Forward declarations
struct KernelLaunchConfig;
struct GPUArchitectureProfile;
struct OptimizationResult;
struct PerformanceMetrics;

//==================================================================================================
// GPU Architecture Profiles
//==================================================================================================

/**
 * @brief Comprehensive GPU architecture profile for launch optimization
 */
struct alignas(128) GPUArchitectureProfile {
    int computeCapability;          // Compute capability (e.g., 75, 80, 86, 89, 90)
    int smCount;                    // Number of streaming multiprocessors
    int maxThreadsPerSM;            // Maximum threads per SM
    int maxThreadsPerBlock;         // Maximum threads per block
    int warpSize;                   // Warp size (typically 32)
    int maxBlocksPerSM;             // Maximum blocks per SM

    // Memory characteristics
    size_t sharedMemPerSM;          // Shared memory per SM (bytes)
    size_t totalGlobalMem;          // Total global memory (bytes)
    size_t maxSharedMemPerBlock;    // Maximum shared memory per block
    size_t l2CacheSize;             // L2 cache size (bytes)

    // Compute characteristics
    int maxRegistersPerBlock;       // Maximum registers per block
    int maxRegistersPerThread;      // Maximum registers per thread
    int clockRate;                  // Base clock rate (KHz)
    int memoryClockRate;            // Memory clock rate (KHz)
    size_t memoryBusWidth;          // Memory bus width (bits)

    // Bandwidth and performance characteristics
    float memoryBandwidthGBps;      // Memory bandwidth (GB/s)
    float peakTFLOPs;               // Peak theoretical performance (TFLOPs)
    float l2BandwidthGBps;          // L2 cache bandwidth (GB/s)

    // Architecture-specific optimizations
    struct {
        bool supportsAsyncCopy;     // Asynchronous copy support
        bool supportsTensorCores;   // Tensor core support
        bool supportsMMA;          // Matrix multiply-accumulate support
        int maxTensorDim;          // Maximum tensor dimension
        float sharedMemToL2Ratio;  // Shared memory to L2 ratio
        float optimalOccupancy;     // Optimal occupancy ratio
    } features;

    // Recommended configuration defaults
    struct {
        dim3 optimalBlockSize;      // Optimal block size
        size_t optimalSharedMem;     // Optimal shared memory usage
        int minGridSize;            // Minimum grid size for good occupancy
        float targetOccupancy;      // Target occupancy ratio
    } defaults;
};

//==================================================================================================
// Kernel Resource Requirements
//==================================================================================================

/**
 * @brief Kernel resource requirements for optimization
 */
struct alignas(64) KernelResourceRequirements {
    size_t sharedMemPerBlock;       // Shared memory per block (bytes)
    int registersPerThread;         // Registers per thread
    int threadsPerBlock;            // Threads per block
    size_t dynamicSharedMem;        // Dynamic shared memory (bytes)

    // Memory access patterns
    struct {
        bool isCoalesced;           // Memory access is coalesced
        bool usesSharedMemory;      // Uses shared memory
        bool hasBankConflicts;      // Has bank conflicts
        float memoryIntensity;      // Memory intensity ratio (0-1)
    } memory;

    // Compute characteristics
    struct {
        bool isComputeBound;        // Compute-bound kernel
        bool hasBranchDivergence;   // Has branch divergence
        float computeIntensity;     // Compute intensity ratio (0-1)
        int synchronizationPoints;  // Number of synchronization points
    } compute;

    // Optimization hints
    struct {
        bool prefersLargeBlocks;    // Prefers larger thread blocks
        bool needsHighOccupancy;    // Needs high occupancy
        bool memoryBandwidthCritical; // Memory bandwidth is critical
        float latencyTolerance;     // Latency tolerance factor
    } hints;
};

//==================================================================================================
// Optimized Launch Configuration
//==================================================================================================

/**
 * @brief Optimized kernel launch configuration
 */
struct alignas(64) KernelLaunchConfig {
    dim3 blockDim;                 // Block dimensions
    dim3 gridDim;                  // Grid dimensions
    size_t sharedMemSize;          // Shared memory size
    cudaStream_t stream;           // CUDA stream

    // Performance predictions
    struct {
        float predictedOccupancy;   // Predicted occupancy (0-1)
        float predictedThroughput;  // Predicted throughput (ops/s)
        float memoryEfficiency;     // Predicted memory efficiency (0-1)
        float computeEfficiency;    // Predicted compute efficiency (0-1)
        size_t predictedRuntime;    // Predicted runtime (ns)
    } predictions;

    // Optimization metadata
    struct {
        std::string optimizationStrategy;  // Strategy used
        float confidenceScore;             // Confidence in optimization (0-1)
        std::vector<std::string> appliedOptimizations;  // Applied optimizations
        std::chrono::system_clock::time_point generatedAt;  // Generation timestamp
    } metadata;

    // Configuration for different scenarios
    struct {
        KernelLaunchConfig forSmallWorkloads;     // Small batch optimization
        KernelLaunchConfig forLargeWorkloads;     // Large batch optimization
        KernelLaunchConfig forLatencyCritical;    // Latency-critical optimization
        KernelLaunchConfig forThroughputCritical; // Throughput-critical optimization
    } variants;
};

//==================================================================================================
// Optimization Results and Metrics
//==================================================================================================

/**
 * @brief Kernel launch optimization result
 */
struct OptimizationResult {
    bool success;                   // Optimization succeeded
    std::string errorMessage;      // Error message if failed

    // Optimized configurations
    KernelLaunchConfig primary;     // Primary optimized configuration
    std::vector<KernelLaunchConfig> alternatives; // Alternative configurations

    // Performance estimates
    struct {
        float expectedSpeedup;      // Expected speedup over baseline
        float confidenceInterval;   // Confidence interval (±%)
        std::vector<float> occupancyBuckets; // Occupancy distribution
        std::vector<float> performanceBands;  // Performance bands
    } estimates;

    // Validation results
    struct {
        bool passedValidation;      // Passed validation tests
        float validationScore;      // Validation score (0-1)
        std::vector<std::string> validationFailures;  // Validation failures
    } validation;

    // Optimization metadata
    std::chrono::nanoseconds optimizationTime;  // Time taken to optimize
    size_t iterations;            // Optimization iterations performed
    std::string gpuModel;         // Target GPU model
    int computeCapability;        // Compute capability
};

//==================================================================================================
// Kernel Launch Optimizer Classes
//==================================================================================================

/**
 * @brief Architecture-aware kernel launch parameter optimizer
 */
class KernelLaunchOptimizer {
public:
    /**
     * @brief Initialize the launch optimizer for target GPU
     */
    KernelLaunchOptimizer();

    /**
     * @brief Destructor
     */
    ~KernelLaunchOptimizer();

    /**
     * @brief Initialize GPU architecture profiles
     */
    bool initialize();

    /**
     * @brief Get current GPU architecture profile
     */
    const GPUArchitectureProfile& getCurrentArchitecture() const;

    /**
     * @brief Get architecture profile for specific compute capability
     */
    bool getArchitectureProfile(int computeCapability, GPUArchitectureProfile& profile) const;

    /**
     * @brief Optimize kernel launch parameters for general case
     */
    OptimizationResult optimizeLaunchParameters(
        const KernelResourceRequirements& requirements,
        size_t totalWorkItems,
        cudaStream_t stream = 0) const;

    /**
     * @brief Optimize for specific workload pattern
     */
    OptimizationResult optimizeForPattern(
        const KernelResourceRequirements& requirements,
        size_t totalWorkItems,
        const std::string& pattern,
        cudaStream_t stream = 0) const;

    /**
     * @brief Optimize for memory bandwidth constraints
     */
    OptimizationResult optimizeForMemoryBandwidth(
        const KernelResourceRequirements& requirements,
        size_t totalWorkItems,
        float memoryBandwidthLimit,
        cudaStream_t stream = 0) const;

    /**
     * @brief Optimize for compute-bound workloads
     */
    OptimizationResult optimizeForComputeBound(
        const KernelResourceRequirements& requirements,
        size_t totalWorkItems,
        float computeIntensity,
        cudaStream_t stream = 0) const;

    /**
     * @brief Validate and adjust configuration for device limits
     */
    bool validateConfiguration(KernelLaunchConfig& config) const;

    /**
     * @brief Predict performance of launch configuration
     */
    PerformanceMetrics predictPerformance(
        const KernelLaunchConfig& config,
        const KernelResourceRequirements& requirements,
        size_t totalWorkItems) const;

    /**
     * @brief Get recommended block sizes for architecture
     */
    std::vector<dim3> getRecommendedBlockSizes(int computeCapability) const;

    /**
     * @brief Calculate optimal grid size for workload
     */
    dim3 calculateOptimalGridSize(
        const dim3& blockDim,
        size_t totalWorkItems,
        const GPUArchitectureProfile& profile) const;

    /**
     * @brief Estimate kernel resource usage
     */
    bool estimateResourceUsage(
        const KernelResourceRequirements& requirements,
        size_t& sharedMemUsage,
        int& registerUsage,
        float& occupancy) const;

    /**
     * @brief Benchmark configuration performance
     */
    float benchmarkConfiguration(
        const KernelLaunchConfig& config,
        void* kernelFunction,
        void** kernelParams,
        size_t totalWorkItems,
        int iterations = 10) const;

    /**
     * @brief Compare configurations
     */
    int compareConfigurations(
        const KernelLaunchConfig& config1,
        const KernelLaunchConfig& config2,
        const KernelResourceRequirements& requirements,
        size_t totalWorkItems) const;

    /**
     * @brief Export optimization results
     */
    std::string exportOptimizationResults(const OptimizationResult& result) const;

    /**
     * @brief Get optimizer statistics
     */
    struct OptimizerStats {
        size_t totalOptimizations;
        size_t successfulOptimizations;
        std::unordered_map<std::string, size_t> strategyUsage;
        std::unordered_map<int, size_t> architectureUsage;
        float averageSpeedup;
        float averageOptimizationTime;
    };

    OptimizerStats getStatistics() const;
    void resetStatistics();

private:
    std::unique_ptr<class KernelLaunchOptimizerImpl> impl_;
    friend class KernelLaunchOptimizerImpl;
};

/**
 * @brief Workload pattern analyzer
 */
class WorkloadPatternAnalyzer {
public:
    /**
     * @brief Analyze workload pattern from execution characteristics
     */
    struct WorkloadPattern {
        enum Type {
            MEMORY_BOUND,
            COMPUTE_BOUND,
            LATENCY_BOUND,
            THROUGHPUT_BOUND,
            BALANCED,
            UNKNOWN
        } type;

        float memoryIntensity;       // Memory access intensity (0-1)
        float computeIntensity;      // Compute intensity (0-1)
        float latencySensitivity;    // Latency sensitivity (0-1)
        float scalabilityFactor;     // Scalability factor (0-1)
        std::vector<float> characteristics; // Additional characteristics
    };

    /**
     * @brief Analyze execution pattern
     */
    static WorkloadPattern analyzeExecutionPattern(
        const PerformanceMetrics& metrics,
        const KernelResourceRequirements& requirements);

    /**
     * @brief Predict pattern from kernel characteristics
     */
    static WorkloadPattern predictPattern(
        const KernelResourceRequirements& requirements,
        const GPUArchitectureProfile& profile);

    /**
     * @brief Get optimization strategy for pattern
     */
    static std::string getOptimizationStrategy(const WorkloadPattern& pattern);
};

/**
 * @brief Auto-tuning engine for kernel launch parameters
 */
class KernelAutoTuner {
public:
    /**
     * @brief Auto-tuning configuration
     */
    struct AutoTuningConfig {
        size_t maxIterations;        // Maximum tuning iterations
        float targetImprovement;     // Target improvement threshold
        float convergenceThreshold;  // Convergence threshold
        bool enableProfiling;        // Enable profiling during tuning
        int benchmarkIterations;     // Benchmark iterations per config
        std::vector<std::string> optimizationStrategies; // Strategies to try
    };

    /**
     * @brief Auto-tuning result
     */
    struct AutoTuningResult {
        KernelLaunchConfig bestConfig;      // Best configuration found
        float bestPerformance;              // Best performance achieved
        float improvementOverBaseline;      // Improvement over baseline
        size_t iterationsPerformed;         // Iterations performed
        std::vector<std::pair<KernelLaunchConfig, float>> testedConfigs; // Tested configs
        std::chrono::milliseconds tuningTime; // Total tuning time
    };

    /**
     * @brief Auto-tune kernel launch parameters
     */
    static AutoTuningResult autoTune(
        void* kernelFunction,
        void** kernelParams,
        const KernelResourceRequirements& requirements,
        size_t totalWorkItems,
        const AutoTuningConfig& config,
        cudaStream_t stream = 0);

    /**
     * @brief Adaptive auto-tuning with workload-specific optimization
     */
    static AutoTuningResult adaptiveAutoTune(
        void* kernelFunction,
        void** kernelParams,
        const KernelResourceRequirements& requirements,
        size_t totalWorkItems,
        const WorkloadPatternAnalyzer::WorkloadPattern& pattern,
        const AutoTuningConfig& config,
        cudaStream_t stream = 0);
};

//==================================================================================================
// Device-side Kernel Launch Helpers
//==================================================================================================

/**
 * @brief Device-side kernel launch parameter utilities
 */
namespace kernel_launch_utils {

    /**
     * @brief Get optimal block size for kernel type
     */
    __device__ __forceinline__
    dim3 getOptimalBlockSize(int kernelType, int computeCapability);

    /**
     * @brief Calculate thread ID in optimized grid
     */
    __device__ __forceinline__
    size_t getGlobalThreadId(const dim3& blockDim, const dim3& gridDim);

    /**
     * @brief Check if thread should process workload item
     */
    __device__ __forceinline__
    bool shouldProcessWorkItem(size_t globalThreadId, size_t totalWorkItems);

    /**
     * @brief Get workload chunk for thread
     */
    __device__ __forceinline__
    void getWorkChunk(size_t globalThreadId, size_t totalWorkItems,
                     size_t& chunkStart, size_t& chunkSize);

    /**
     * @brief Optimized barrier synchronization for specific patterns
     */
    __device__ __forceinline__
    void optimizedBarrierSync(int pattern = 0);
}

//==================================================================================================
// Utility Functions
//==================================================================================================

/**
 * @brief Utility functions for kernel launch optimization
 */
namespace kernel_launch_optimizer_utils {

    /**
     * @brief Calculate theoretical occupancy
     */
    float calculateTheoreticalOccupancy(
        const KernelResourceRequirements& requirements,
        const GPUArchitectureProfile& profile);

    /**
     * @brief Estimate memory bandwidth usage
     */
    float estimateMemoryBandwidthUsage(
        const KernelResourceRequirements& requirements,
        const KernelLaunchConfig& config,
        const GPUArchitectureProfile& profile);

    /**
     * @brief Calculate optimal shared memory usage
     */
    size_t calculateOptimalSharedMemory(
        const KernelResourceRequirements& requirements,
        const GPUArchitectureProfile& profile);

    /**
     * @brief Determine best block size for register usage
     */
    dim3 getBestBlockSizeForRegisters(
        int registersPerThread,
        const GPUArchitectureProfile& profile);

    /**
     * @brief Validate grid-block configuration
     */
    bool validateGridBlockConfiguration(
        const KernelLaunchConfig& config,
        const GPUArchitectureProfile& profile);

    /**
     * @brief Get configuration hash for caching
     */
    size_t getConfigurationHash(
        const KernelResourceRequirements& requirements,
        int computeCapability,
        size_t totalWorkItems);
}

#endif // KERNEL_LAUNCH_OPTIMIZER_CUH
//==================================================================================================
// Puzzle71 Technical Debt Repair - T048
// Memory Efficiency Validation System
//
// This system validates that memory efficiency exceeds 90% (target 95%+) by measuring
// actual memory access patterns, cache utilization, and bandwidth usage against theoretical
// maximums. It provides comprehensive validation across all GPU architectures (75, 80, 86, 89, 90).
//
// Constitutional Compliance v5.5:
// - T073: Code duplication elimination through unified validation interface
// - T074: Comprehensive test coverage for memory efficiency validation
// - T077: Full constitutional compliance with v5.5 constraints
//==================================================================================================

#ifndef MEMORY_EFFICIENCY_VALIDATOR_H
#define MEMORY_EFFICIENCY_VALIDATOR_H

#include <cuda_runtime.h>
#include <cuda_device_runtime_api.h>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <unordered_map>

// Forward declarations
struct MemoryEfficiencyMetrics;
struct MemoryEfficiencyResult;
struct MemoryAccessPattern;
struct CachePerformanceMetrics;

//==================================================================================================
// Memory Efficiency Metrics Structure
//==================================================================================================

/**
 * @brief Comprehensive memory efficiency metrics for validation
 */
struct alignas(64) MemoryEfficiencyMetrics {
    // Global memory efficiency
    struct {
        float globalLoadEfficiency;      // Global memory load efficiency (0-1)
        float globalStoreEfficiency;     // Global memory store efficiency (0-1)
        float coalescedAccessRatio;      // Ratio of coalesced memory accesses
        float memoryBandwidthUtilization; // Actual bandwidth / theoretical peak (0-1)
        size_t totalBytesTransferred;    // Total bytes transferred
        size_t effectiveBytesTransferred; // Effective bytes (accounting for coalescing)
        float transactionUtilization;    // Memory transaction utilization (0-1)
    } globalMemory;

    // Shared memory efficiency
    struct {
        float sharedMemoryUtilization;   // Shared memory usage / allocated (0-1)
        float bankConflictRate;          // Rate of shared memory bank conflicts
        float sharedMemoryEfficiency;    // Effective shared memory throughput
        size_t sharedMemoryThroughputGBps; // Shared memory bandwidth (GB/s)
        float sharedMemoryHitRate;       // Hit rate for shared memory accesses
    } sharedMemory;

    // Cache performance
    struct {
        float l1CacheHitRate;           // L1 cache hit rate (0-1)
        float l2CacheHitRate;           // L2 cache hit rate (0-1)
        float cacheUtilization;         // Cache space utilization (0-1)
        size_t l1CacheThroughputGBps;   // L1 cache bandwidth (GB/s)
        size_t l2CacheThroughputGBps;   // L2 cache bandwidth (GB/s)
        float cacheMissPenalty;         // Average cache miss penalty
    } cache;

    // Memory access patterns
    struct {
        float sequentialAccessRatio;     // Ratio of sequential accesses
        float randomAccessRatio;        // Ratio of random accesses
        float stridedAccessRatio;       // Ratio of strided accesses
        float averageStrideBytes;       // Average memory access stride
        float accessPatternEfficiency;  // Overall access pattern efficiency (0-1)
    } accessPatterns;

    // Memory latency and throughput
    struct {
        float averageMemoryLatency;     // Average memory access latency (ns)
        float memoryThroughputGBps;     // Achieved memory throughput (GB/s)
        float theoreticalBandwidthGBps; // Theoretical peak bandwidth (GB/s)
        float efficiencyRatio;          // Actual / theoretical bandwidth ratio
        size_t memoryOpsPerSecond;      // Memory operations per second
    } performance;

    // Overall memory efficiency score
    float overallEfficiencyScore;       // Weighted overall efficiency (0-1)
    bool meetsTargetEfficiency;        // Meets 95% target efficiency
    bool exceedsMinimumEfficiency;     // Exceeds 90% minimum efficiency
};

//==================================================================================================
// Memory Access Pattern Analysis
//==================================================================================================

/**
 * @brief Memory access pattern analysis results
 */
struct MemoryAccessPattern {
    enum PatternType {
        SEQUENTIAL,
        STRIDED,
        RANDOM,
        MIXED
    } primaryPattern;

    float patternCoherence;           // How coherent the pattern is (0-1)
    std::vector<float> strideHistogram; // Histogram of access strides
    float averageStride;              // Average access stride
    float strideVariance;             // Variance in access strides
    size_t uniqueAddresses;           // Number of unique memory addresses accessed
    size_t totalAccesses;             // Total memory accesses performed
    float spatialLocalityScore;       // Spatial locality score (0-1)
    float temporalLocalityScore;      // Temporal locality score (0-1)
};

//==================================================================================================
// Cache Performance Metrics
//==================================================================================================

/**
 * @brief Detailed cache performance metrics
 */
struct CachePerformanceMetrics {
    // L1 Cache metrics
    struct {
        size_t l1Hits;                 // Number of L1 cache hits
        size_t l1Misses;               // Number of L1 cache misses
        float l1HitRate;               // L1 cache hit rate (0-1)
        float l1MissPenalty;           // L1 cache miss penalty (cycles)
        size_t l1PrefetchHits;         // Number of successful prefetches
        size_t l1PrefetchMisses;       // Number of failed prefetches
    } l1Cache;

    // L2 Cache metrics
    struct {
        size_t l2Hits;                 // Number of L2 cache hits
        size_t l2Misses;               // Number of L2 cache misses
        float l2HitRate;               // L2 cache hit rate (0-1)
        float l2MissPenalty;           // L2 cache miss penalty (cycles)
        size_t l2PrefetchHits;         // Number of successful prefetches
        size_t l2PrefetchMisses;       // Number of failed prefetches
    } l2Cache;

    // Unified cache metrics (for architectures with unified L1/L2)
    struct {
        float unifiedHitRate;          // Unified cache hit rate (0-1)
        float unifiedEfficiency;       // Unified cache efficiency (0-1)
        size_t unifiedThroughputGBps;  // Unified cache bandwidth (GB/s)
    } unifiedCache;
};

//==================================================================================================
// Memory Efficiency Validation Result
//==================================================================================================

/**
 * @brief Complete memory efficiency validation result
 */
struct MemoryEfficiencyResult {
    bool success;                      // Validation succeeded
    std::string errorMessage;         // Error message if validation failed

    // Validation metrics
    MemoryEfficiencyMetrics metrics;  // Comprehensive efficiency metrics
    MemoryAccessPattern accessPattern; // Access pattern analysis
    CachePerformanceMetrics cacheMetrics; // Cache performance metrics

    // Validation results by category
    struct {
        bool globalMemoryEfficiency;   // Global memory efficiency ≥ 90%
        bool sharedMemoryEfficiency;   // Shared memory efficiency ≥ 90%
        bool cacheEfficiency;          // Cache efficiency ≥ 90%
        bool accessPatternEfficiency;  // Access pattern efficiency ≥ 90%
        bool bandwidthUtilization;     // Bandwidth utilization ≥ 90%
        bool overallEfficiency;        // Overall efficiency ≥ 95%
    } validationResults;

    // Performance against targets
    struct {
        float targetEfficiency;        // Target efficiency (95%)
        float minimumEfficiency;       // Minimum efficiency (90%)
        float achievedEfficiency;      // Actually achieved efficiency
        float efficiencyGap;           // Gap to target (target - achieved)
        bool meetsTarget;              // Meets target efficiency
        bool exceedsMinimum;           // Exceeds minimum efficiency
        std::vector<std::string> improvementSuggestions; // Suggestions for improvement
    } performanceTargets;

    // GPU-specific validation results
    struct {
        int computeCapability;         // GPU compute capability
        std::string gpuModel;          // GPU model name
        size_t totalGlobalMemory;      // Total global memory (bytes)
        size_t availableMemory;        // Available memory (bytes)
        float memoryUtilization;       // Memory utilization (0-1)
        bool memoryPressure;           // Under memory pressure
    } gpuInfo;

    // Validation metadata
    std::chrono::system_clock::time_point validationTime;
    std::chrono::milliseconds validationDuration;
    size_t totalSamples;               // Number of samples taken
    std::vector<std::string> warnings; // Validation warnings
    std::vector<std::string> recommendations; // Performance recommendations
};

//==================================================================================================
// Memory Efficiency Validator Class
//==================================================================================================

/**
 * @brief Memory efficiency validator for GPU kernels
 */
class MemoryEfficiencyValidator {
public:
    /**
     * @brief Initialize memory efficiency validator
     */
    MemoryEfficiencyValidator();

    /**
     * @brief Destructor
     */
    ~MemoryEfficiencyValidator();

    /**
     * @brief Initialize validator for target GPU
     */
    bool initialize();

    /**
     * @brief Validate memory efficiency for a kernel execution
     */
    MemoryEfficiencyResult validateMemoryEfficiency(
        void* kernelFunction,
        void** kernelParams,
        dim3 blockDim,
        dim3 gridDim,
        size_t sharedMemSize,
        cudaStream_t stream = 0,
        int validationRuns = 10);

    /**
     * @brief Validate memory efficiency with custom workload
     */
    MemoryEfficiencyResult validateMemoryEfficiencyCustom(
        const std::string& testName,
        std::function<void(cudaStream_t)> workload,
        size_t expectedDataSize,
        cudaStream_t stream = 0);

    /**
     * @brief Quick memory efficiency validation (single run)
     */
    MemoryEfficiencyResult quickValidate(
        void* kernelFunction,
        void** kernelParams,
        dim3 blockDim,
        dim3 gridDim,
        size_t sharedMemSize,
        cudaStream_t stream = 0);

    /**
     * @brief Comprehensive memory efficiency validation
     */
    MemoryEfficiencyResult comprehensiveValidate(
        void* kernelFunction,
        void** kernelParams,
        dim3 blockDim,
        dim3 gridDim,
        size_t sharedMemSize,
        cudaStream_t stream = 0,
        int warmupRuns = 5,
        int validationRuns = 20);

    /**
     * @brief Validate shared memory efficiency specifically
     */
    MemoryEfficiencyResult validateSharedMemoryEfficiency(
        void* kernelFunction,
        void** kernelParams,
        dim3 blockDim,
        dim3 gridDim,
        size_t sharedMemSize,
        cudaStream_t stream = 0);

    /**
     * @brief Validate global memory coalescing efficiency
     */
    MemoryEfficiencyResult validateGlobalMemoryCoalescing(
        void* kernelFunction,
        void** kernelParams,
        dim3 blockDim,
        dim3 gridDim,
        size_t dataSize,
        cudaStream_t stream = 0);

    /**
     * @brief Validate cache efficiency
     */
    MemoryEfficiencyResult validateCacheEfficiency(
        void* kernelFunction,
        void** kernelParams,
        dim3 blockDim,
        dim3 gridDim,
        size_t workingSetSize,
        cudaStream_t stream = 0);

    /**
     * @brief Get baseline memory efficiency metrics
     */
    MemoryEfficiencyMetrics getBaselineMetrics(int computeCapability);

    /**
     * @brief Compare with baseline performance
     */
    MemoryEfficiencyResult compareWithBaseline(
        const MemoryEfficiencyMetrics& currentMetrics,
        int computeCapability);

    /**
     * @brief Generate memory efficiency report
     */
    std::string generateReport(const MemoryEfficiencyResult& result);

    /**
     * @brief Export validation results to JSON
     */
    std::string exportToJson(const MemoryEfficiencyResult& result);

    /**
     * @brief Export validation results to CSV
     */
    std::string exportToCsv(const MemoryEfficiencyResult& result);

    /**
     * @brief Get validator statistics
     */
    struct ValidatorStats {
        size_t totalValidations;
        size_t successfulValidations;
        size_t passedTargetEfficiency;
        size_t exceededMinimumEfficiency;
        float averageEfficiency;
        std::unordered_map<int, size_t> architectureValidations;
        std::vector<float> efficiencyHistory;
    };

    ValidatorStats getStatistics() const;
    void resetStatistics();

private:
    std::unique_ptr<class MemoryEfficiencyValidatorImpl> impl_;
    friend class MemoryEfficiencyValidatorImpl;
};

//==================================================================================================
// Memory Efficiency Benchmark Functions
//==================================================================================================

/**
 * @brief Memory efficiency benchmark utilities
 */
namespace memory_efficiency_benchmarks {

    /**
     * @brief Benchmark sequential memory access pattern
     */
    MemoryEfficiencyResult benchmarkSequentialAccess(
        size_t dataSize,
        int blockSize,
        int gridSize,
        cudaStream_t stream = 0);

    /**
     * @brief Benchmark random memory access pattern
     */
    MemoryEfficiencyResult benchmarkRandomAccess(
        size_t dataSize,
        int blockSize,
        int gridSize,
        cudaStream_t stream = 0);

    /**
     * @brief Benchmark strided memory access pattern
     */
    MemoryEfficiencyResult benchmarkStridedAccess(
        size_t dataSize,
        size_t stride,
        int blockSize,
        int gridSize,
        cudaStream_t stream = 0);

    /**
     * @brief Benchmark shared memory bandwidth
     */
    MemoryEfficiencyResult benchmarkSharedMemoryBandwidth(
        size_t dataSize,
        int blockSize,
        int gridSize,
        cudaStream_t stream = 0);

    /**
     * @brief Benchmark cache performance
     */
    MemoryEfficiencyResult benchmarkCachePerformance(
        size_t workingSetSize,
        int blockSize,
        int gridSize,
        cudaStream_t stream = 0);
}

//==================================================================================================
// Utility Functions
//==================================================================================================

/**
 * @brief Utility functions for memory efficiency validation
 */
namespace memory_efficiency_utils {

    /**
     * @brief Calculate theoretical memory bandwidth for GPU
     */
    float getTheoreticalMemoryBandwidth(int computeCapability);

    /**
     * @brief Calculate memory efficiency score from metrics
     */
    float calculateEfficiencyScore(const MemoryEfficiencyMetrics& metrics);

    /**
     * @brief Analyze memory access pattern from trace data
     */
    MemoryAccessPattern analyzeAccessPattern(
        const std::vector<uint64_t>& addressTrace,
        size_t accessCount);

    /**
     * @brief Estimate cache hit rates from memory access patterns
     */
    CachePerformanceMetrics estimateCachePerformance(
        const MemoryAccessPattern& pattern,
        size_t cacheSize,
        size_t associativity);

    /**
     * @brief Generate memory efficiency improvement suggestions
     */
    std::vector<std::string> generateImprovementSuggestions(
        const MemoryEfficiencyMetrics& metrics);

    /**
     * @brief Validate memory efficiency against constitutional requirements
     */
    bool validateConstitutionalRequirements(
        const MemoryEfficiencyMetrics& metrics,
        float minimumEfficiency = 0.90f,
        float targetEfficiency = 0.95f);
}

#endif // MEMORY_EFFICIENCY_VALIDATOR_H
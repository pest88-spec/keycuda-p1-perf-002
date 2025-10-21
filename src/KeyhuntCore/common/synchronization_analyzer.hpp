// Puzzle71Solver - Synchronization Overhead Analyzer Header
// Implements comprehensive synchronization overhead measurement and analysis for T050

#pragma once

#include <cuda_runtime.h>
#include <vector>
#include <string>
#include <map>

namespace keyhunt {
namespace performance {

/**
 * Synchronization Overhead Analyzer
 *
 * Provides comprehensive measurement and analysis of synchronization overhead
 * including basic synchronization, kernel synchronization, atomic operations,
 * warp synchronization, and various optimization techniques.
 *
 * Features:
 * - Baseline synchronization overhead measurement
 * - Multiple synchronization pattern analysis
 * - Optimization effectiveness measurement
 * - Constitutional compliance validation (≥50% overhead reduction)
 * - Comprehensive reporting with recommendations
 *
 * Constitutional v5.5 Compliance:
 * - Synchronization overhead must be reduced to 50% or less of baseline
 * - Performance targets: ≥50% reduction through optimizations
 * - Real-time overhead measurement and analysis
 */
class SynchronizationAnalyzer {
public:
    /**
     * Optimization types for synchronization overhead reduction
     */
    enum class OptimizationType {
        BASIC = 0,
        WARP_SHUFFLE = 1,
        SHARED_MEMORY_OPTIMIZATION = 2,
        ATOMIC_AGGREGATION = 3,
        ASYNC_STREAMS = 4
    };

    /**
     * Constructor
     * @param device_id GPU device ID to analyze
     */
    explicit SynchronizationAnalyzer(int device_id = 0);

    /**
     * Destructor
     */
    ~SynchronizationAnalyzer();

    /**
     * Initialize analyzer for specific GPU device
     * @param device_id GPU device ID
     * @return true if initialization successful
     */
    bool initialize(int device_id);

    /**
     * Measure basic synchronization overhead
     * @param overhead_ns Output parameter for overhead in nanoseconds
     * @return true if measurement successful
     */
    bool measureSynchronizationOverhead(double& overhead_ns);

    /**
     * Measure kernel synchronization overhead (including __syncthreads())
     * @param overhead_ns Output parameter for overhead in nanoseconds
     * @return true if measurement successful
     */
    bool measureKernelSynchronizationOverhead(double& overhead_ns);

    /**
     * Measure atomic operation overhead
     * @param overhead_ns Output parameter for overhead in nanoseconds
     * @return true if measurement successful
     */
    bool measureAtomicOperationOverhead(double& overhead_ns);

    /**
     * Measure warp-level synchronization overhead
     * @param overhead_ns Output parameter for overhead in nanoseconds
     * @return true if measurement successful
     */
    bool measureWarpSynchronizationOverhead(double& overhead_ns);

    /**
     * Establish baseline synchronization overhead
     * @return true if baseline established successfully
     */
    bool establishBaseline();

    /**
     * Measure optimized synchronization overhead
     * @param overhead_ns Output parameter for overhead in nanoseconds
     * @param type Type of optimization to test
     * @return true if measurement successful
     */
    bool measureOptimizedSynchronizationOverhead(double& overhead_ns, OptimizationType type);

    /**
     * Calculate overhead reduction percentage
     * @param baseline_ns Baseline overhead in nanoseconds
     * @param optimized_ns Optimized overhead in nanoseconds
     * @return Reduction percentage
     */
    double calculateOverheadReduction(double baseline_ns, double optimized_ns);

    /**
     * Generate comprehensive synchronization overhead report
     * @param report Output string containing formatted report
     * @return true if report generated successfully
     */
    bool generateSynchronizationReport(std::string& report);

    // Accessors
    double getBaselineSynchronizationOverhead() const { return baseline_sync_overhead_ns_; }
    double getBaselineStandardDeviation() const { return baseline_std_dev_ns_; }

    // Constitutional compliance constants
    static constexpr double TARGET_REDUCTION_PERCENTAGE = 50.0;  // Must achieve ≥50% reduction
    static constexpr int BASELINE_SAMPLES = 10;                  // Number of baseline samples

private:
    int device_id_;
    double baseline_sync_overhead_ns_;
    double baseline_std_dev_ns_;

    // Specific optimization measurement methods
    bool measureWarpShuffleOverhead(double& overhead_ns);
    bool measureSharedMemoryOptimizedOverhead(double& overhead_ns);
    bool measureAtomicAggregationOverhead(double& overhead_ns);
    bool measureAsyncStreamOverhead(double& overhead_ns);

    // CUDA kernel declarations
    static void synchronizationKernel();
    static void atomicKernel(int* counter);
    static void warpSyncKernel();
    static void warpShuffleKernel();
    static void sharedMemoryOptimizedKernel();
    static void atomicAggregationKernel(int* counter);
    static void asyncKernel();
};

} // namespace performance
} // namespace keyhunt
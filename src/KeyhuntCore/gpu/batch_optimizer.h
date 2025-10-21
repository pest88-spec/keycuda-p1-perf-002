// Puzzle71Solver - Adaptive Batch Sizing System Header (T032)
// Optimizes batch sizes for maximum GPU utilization

#pragma once

#include "compute/gpu/batch_planner.h"
#include "KeyhuntCore/gpu/architecture_detector.hpp"

#include <cuda_runtime.h>
#include <chrono>
#include <cstdint>
#include <iosfwd>
#include <vector>

namespace keyhunt {
namespace gpu {

/**
 * @brief Performance metrics for adaptive batch optimization
 */
struct PerformanceMetrics {
    std::uint64_t batch_size;
    double throughput_keys_per_sec;
    double gpu_utilization; // 0.0 to 1.0
    std::chrono::steady_clock::time_point timestamp;
};

/**
 * @brief Adaptive batch size optimizer
 *
 * This class implements adaptive batch sizing that:
 * - Analyzes GPU architecture to determine optimal batch sizes
 * - Monitors performance metrics to adaptively adjust batch sizes
 * - Targets ≥80% GPU utilization for maximum throughput
 * - Handles memory constraints gracefully
 * - Learns from historical performance data
 *
 * The optimizer uses a feedback loop:
 * 1. Start with architecture-specific defaults
 * 2. Monitor performance (throughput, GPU utilization)
 * 3. Adjust batch size to maximize throughput while maintaining high utilization
 * 4. Repeat and refine
 *
 * Target performance improvements:
 * - GPU utilization: 25% → ≥80%
 * - Throughput: 2.5-3× improvement through optimal batching
 */
class BatchOptimizer {
public:
    /**
     * @brief Construct adaptive batch optimizer
     * @param device_id CUDA device ID
     */
    explicit BatchOptimizer(int device_id);

    /**
     * @brief Optimize batch configuration for current workload
     *
     * @param target_keys Desired number of keys (0 = optimize for throughput)
     * @return Optimized batch configuration
     */
    puzzle71::gpu::BatchConfig optimizeBatch(std::uint64_t target_keys = 0);

    /**
     * @brief Record performance metrics for adaptive learning
     *
     * @param batch_size Batch size used
     * @param throughput_keys_per_sec Achieved throughput
     * @param gpu_utilization GPU utilization (0.0-1.0)
     */
    void recordPerformance(
        std::uint64_t batch_size,
        double throughput_keys_per_sec,
        double gpu_utilization
    );

    /**
     * @brief Get current optimal batch size
     * @return Optimal batch size in keys
     */
    std::uint64_t getOptimalBatchSize() const;

    /**
     * @brief Get architecture information
     */
    const ArchitectureInfo& getArchitectureInfo() const;

    /**
     * @brief Reset optimizer to default configuration
     */
    void reset();

    /**
     * @brief Print current optimization status
     * @param os Output stream
     */
    void printStatus(std::ostream& os) const;

private:
    void initializeDefaults();
    void updateOptimalBatchSize();

    int device_id_;
    cudaDeviceProp device_props_;
    ArchitectureDetector arch_detector_;
    ArchitectureInfo arch_info_;

    // Batch size parameters (in keys)
    std::uint64_t default_batch_size_;
    std::uint64_t min_batch_size_;
    std::uint64_t max_batch_size_;
    std::uint64_t current_batch_size_;
    std::uint64_t optimal_batch_size_;

    // Points per thread parameters
    int default_points_per_thread_;
    int min_points_per_thread_;
    int max_points_per_thread_;
    int current_points_per_thread_;

    // Performance history for adaptive optimization
    std::vector<PerformanceMetrics> performance_history_;
};

} // namespace gpu
} // namespace keyhunt

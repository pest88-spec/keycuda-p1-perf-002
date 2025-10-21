// Puzzle71Solver - Adaptive Batch Sizing System (T032)
// Optimizes batch sizes for maximum GPU utilization across different architectures

#include "batch_optimizer.h"
#include "compute/gpu/batch_planner.h"
#include "KeyhuntCore/gpu/architecture_detector.hpp"

#include <cuda_runtime.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace keyhunt {
namespace gpu {

/**
 * @brief Construct adaptive batch optimizer
 *
 * @param device_id CUDA device ID to optimize for
 */
BatchOptimizer::BatchOptimizer(int device_id)
    : device_id_(device_id),
      arch_detector_(device_id),
      current_batch_size_(0),
      optimal_batch_size_(0),
      performance_history_() {

    // Get device properties
    cudaError_t err = cudaGetDeviceProperties(&device_props_, device_id_);
    if (err != cudaSuccess) {
        throw std::runtime_error("Failed to get CUDA device properties: " +
                                 std::string(cudaGetErrorString(err)));
    }

    // Detect GPU architecture
    arch_info_ = arch_detector_.detectArchitecture();

    // Initialize with architecture-specific defaults
    initializeDefaults();
}

/**
 * @brief Initialize architecture-specific batch sizing defaults
 */
void BatchOptimizer::initializeDefaults() {
    // Architecture-specific default batch sizes (keys per batch)
    // These are starting points for adaptive optimization

    if (arch_info_.compute_capability >= 90) {
        // Hopper (sm_90+): H20, H100
        // Target: Very large batches for maximum throughput
        default_batch_size_ = 163'840'000; // 163M keys
        min_batch_size_ = 50'000'000;      // 50M keys
        max_batch_size_ = 268'435'456;     // 268M keys (kMaxKeysPerBatch)

    } else if (arch_info_.compute_capability >= 80) {
        // Ampere/Ada (sm_80-89): A100, RTX 3090, RTX 4090
        // Target: Large batches with good occupancy
        default_batch_size_ = 81'920'000;  // 82M keys
        min_batch_size_ = 25'000'000;      // 25M keys
        max_batch_size_ = 134'217'728;     // 134M keys

    } else if (arch_info_.compute_capability >= 75) {
        // Turing (sm_75): RTX 2080 Ti, Titan RTX
        // Target: Moderate batches for balanced performance
        default_batch_size_ = 40'960'000;  // 41M keys
        min_batch_size_ = 10'000'000;      // 10M keys
        max_batch_size_ = 67'108'864;      // 67M keys

    } else {
        // Older architectures: Conservative defaults
        default_batch_size_ = 20'480'000;  // 20M keys
        min_batch_size_ = 5'000'000;       // 5M keys
        max_batch_size_ = 33'554'432;      // 33M keys
    }

    current_batch_size_ = default_batch_size_;
    optimal_batch_size_ = default_batch_size_;

    // Points per thread optimization
    // Higher compute capability → more points per thread for better GPU utilization
    if (arch_info_.compute_capability >= 90) {
        default_points_per_thread_ = 1024;
        min_points_per_thread_ = 256;
        max_points_per_thread_ = 2048;
    } else if (arch_info_.compute_capability >= 80) {
        default_points_per_thread_ = 512;
        min_points_per_thread_ = 128;
        max_points_per_thread_ = 1024;
    } else {
        default_points_per_thread_ = 256;
        min_points_per_thread_ = 64;
        max_points_per_thread_ = 512;
    }

    current_points_per_thread_ = default_points_per_thread_;
}

/**
 * @brief Optimize batch configuration for current workload
 *
 * Uses adaptive algorithm considering:
 * - GPU architecture capabilities
 * - Available GPU memory
 * - Historical performance data
 * - Target GPU utilization (≥80%)
 *
 * @param target_keys Desired number of keys to process (0 = optimize for throughput)
 * @return Optimized batch configuration
 */
puzzle71::gpu::BatchConfig BatchOptimizer::optimizeBatch(std::uint64_t target_keys) {
    puzzle71::gpu::BatchConfig config;

    // Step 1: Determine target batch size
    std::uint64_t batch_size = 0;
    if (target_keys > 0) {
        // User specified exact key count
        batch_size = std::min(target_keys, static_cast<std::uint64_t>(max_batch_size_));
    } else {
        // Optimize for maximum throughput
        batch_size = optimal_batch_size_ > 0 ? optimal_batch_size_ : default_batch_size_;
    }

    // Step 2: Check available GPU memory
    size_t free_memory = 0, total_memory = 0;
    cudaMemGetInfo(&free_memory, &total_memory);

    // Calculate memory requirements per key
    // Each key needs: X coordinate (32 bytes) + Y coordinate (32 bytes) + metadata (~16 bytes)
    constexpr size_t bytes_per_key = 80;
    size_t required_memory = batch_size * bytes_per_key;

    // Safety margin: use at most 70% of free memory
    size_t available_memory = static_cast<size_t>(free_memory * 0.70);

    if (required_memory > available_memory) {
        // Reduce batch size to fit in available memory
        std::uint64_t memory_limited_batch = available_memory / bytes_per_key;
        batch_size = std::min(batch_size, memory_limited_batch);
        batch_size = std::max(batch_size, static_cast<std::uint64_t>(min_batch_size_));
    }

    // Step 3: Calculate optimal grid/block configuration
    int points_per_thread = current_points_per_thread_;

    // Calculate number of threads needed
    std::uint64_t total_points = batch_size;
    std::uint64_t num_threads = (total_points + points_per_thread - 1) / points_per_thread;

    // Optimize block size for GPU architecture
    int block_size = 256; // Default: works well for most architectures

    if (arch_info_.compute_capability >= 90) {
        // Hopper: 256 threads/block for 8-10 blocks/SM
        block_size = 256;
    } else if (arch_info_.compute_capability >= 80) {
        // Ampere/Ada: 256-384 threads/block
        block_size = 256;
    } else {
        // Turing: 256-512 threads/block
        block_size = 256;
    }

    // Calculate grid size to fully utilize all SMs
    int sm_count = device_props_.multiProcessorCount;
    int max_threads_per_sm = device_props_.maxThreadsPerMultiProcessor;
    int max_blocks_per_sm = max_threads_per_sm / block_size;

    // Target occupancy: aim for high block count per SM
    int target_blocks_per_sm = std::min(max_blocks_per_sm,
        arch_info_.compute_capability >= 90 ? 16 :
        arch_info_.compute_capability >= 80 ? 12 : 8);

    int optimal_blocks = sm_count * target_blocks_per_sm;

    // Calculate actual grid size based on workload
    std::uint64_t required_blocks = (num_threads + block_size - 1) / block_size;
    int grid_size = static_cast<int>(std::min<std::uint64_t>(required_blocks, optimal_blocks));

    // Ensure grid size doesn't exceed device limits
    grid_size = std::min(grid_size, device_props_.maxGridSize[0]);
    grid_size = std::max(grid_size, 1);

    // Step 4: Populate configuration
    config.grid = dim3(grid_size, 1, 1);
    config.block = dim3(block_size, 1, 1);
    config.points_per_thread = points_per_thread;
    config.keys_total = static_cast<std::uint64_t>(grid_size) *
                       static_cast<std::uint64_t>(block_size) *
                       static_cast<std::uint64_t>(points_per_thread);

    // Update current batch size
    current_batch_size_ = config.keys_total;

    return config;
}

/**
 * @brief Record performance metrics for adaptive optimization
 *
 * The optimizer learns from historical performance to improve future
 * batch size selections.
 *
 * @param batch_size Batch size used
 * @param throughput_keys_per_sec Achieved throughput (keys/second)
 * @param gpu_utilization GPU utilization percentage (0.0-1.0)
 */
void BatchOptimizer::recordPerformance(
    std::uint64_t batch_size,
    double throughput_keys_per_sec,
    double gpu_utilization
) {
    PerformanceMetrics metrics;
    metrics.batch_size = batch_size;
    metrics.throughput_keys_per_sec = throughput_keys_per_sec;
    metrics.gpu_utilization = gpu_utilization;
    metrics.timestamp = std::chrono::steady_clock::now();

    // Add to history (keep last 100 samples)
    performance_history_.push_back(metrics);
    if (performance_history_.size() > 100) {
        performance_history_.erase(performance_history_.begin());
    }

    // Adaptive optimization: find batch size that maximizes throughput
    // while maintaining ≥80% GPU utilization
    updateOptimalBatchSize();
}

/**
 * @brief Update optimal batch size based on performance history
 */
void BatchOptimizer::updateOptimalBatchSize() {
    if (performance_history_.empty()) {
        return;
    }

    // Find configuration with best throughput and ≥80% GPU utilization
    double best_throughput = 0.0;
    std::uint64_t best_batch_size = default_batch_size_;

    for (const auto& metrics : performance_history_) {
        // Only consider samples with good GPU utilization
        if (metrics.gpu_utilization >= 0.80) {
            if (metrics.throughput_keys_per_sec > best_throughput) {
                best_throughput = metrics.throughput_keys_per_sec;
                best_batch_size = metrics.batch_size;
            }
        }
    }

    // Update optimal batch size if we found a better configuration
    if (best_throughput > 0.0) {
        optimal_batch_size_ = best_batch_size;

        // Adaptive adjustment: try slightly larger batches to explore performance space
        // This implements a simple gradient ascent for batch size optimization
        if (best_batch_size < max_batch_size_) {
            // Try 10% larger batch next time
            std::uint64_t experimental_size = static_cast<std::uint64_t>(best_batch_size * 1.10);
            experimental_size = std::min(experimental_size, static_cast<std::uint64_t>(max_batch_size_));
            current_batch_size_ = experimental_size;
        }
    }
}

/**
 * @brief Get current optimal batch size
 */
std::uint64_t BatchOptimizer::getOptimalBatchSize() const {
    return optimal_batch_size_ > 0 ? optimal_batch_size_ : default_batch_size_;
}

/**
 * @brief Get architecture information
 */
const ArchitectureInfo& BatchOptimizer::getArchitectureInfo() const {
    return arch_info_;
}

/**
 * @brief Reset optimizer to default configuration
 */
void BatchOptimizer::reset() {
    current_batch_size_ = default_batch_size_;
    optimal_batch_size_ = default_batch_size_;
    current_points_per_thread_ = default_points_per_thread_;
    performance_history_.clear();
}

/**
 * @brief Print current optimization status
 */
void BatchOptimizer::printStatus(std::ostream& os) const {
    os << "=== Batch Optimizer Status ===" << std::endl;
    os << "GPU: " << device_props_.name << " (sm_" << device_props_.major
       << device_props_.minor << ")" << std::endl;
    os << "SMs: " << device_props_.multiProcessorCount << std::endl;
    os << "Default batch size: " << default_batch_size_ << " keys" << std::endl;
    os << "Optimal batch size: " << optimal_batch_size_ << " keys" << std::endl;
    os << "Current batch size: " << current_batch_size_ << " keys" << std::endl;
    os << "Points per thread: " << current_points_per_thread_ << std::endl;
    os << "Performance samples: " << performance_history_.size() << std::endl;

    if (!performance_history_.empty()) {
        const auto& latest = performance_history_.back();
        os << "Latest throughput: " << (latest.throughput_keys_per_sec / 1e6) << " Mkeys/s" << std::endl;
        os << "Latest GPU utilization: " << (latest.gpu_utilization * 100) << "%" << std::endl;
    }
}

} // namespace gpu
} // namespace keyhunt

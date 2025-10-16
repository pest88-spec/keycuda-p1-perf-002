#pragma once

#include "compute/shards/shard_walker.h"
#include "puzzle71_kernel.h"

#include <cuda_runtime.h>

#include <cstddef>
#include <cstdint>
#include <optional>

namespace puzzle71::gpu {

struct BatchConfig {
    dim3 grid{1, 1, 1};
    dim3 block{32, 1, 1};
    int points_per_thread{1};
    std::uint64_t keys_total{0};
};

constexpr std::uint64_t kMaxKeysPerBatch = 1ULL << 28;          // 268,435,456 keys
constexpr std::uint64_t kMaxThreadsPerBatch = 1ULL << 20;        // 1,048,576 threads
constexpr std::size_t kMaxCandidateBuffer = 4096;                // bounded result slots

/**
 * @brief 计算最优批量大小 (P1-PERF-002: 自适应批量配置)
 *
 * 根据GPU内存容量动态计算最优批量大小,替代硬编码的kMaxKeysPerBatch
 *
 * 内存估算:
 * - 每个key: 32字节(私钥) + 64字节(公钥) + 20字节(hash160) = 116字节
 * - 预留20%内存给其他用途
 *
 * @param gpu_memory_bytes GPU总内存(字节)
 * @return 最优批量大小(keys数量)
 */
inline std::uint64_t ComputeOptimalBatchSize(std::uint64_t gpu_memory_bytes) {
    // 预留20%内存给其他用途(kernel、临时缓冲等)
    constexpr double kMemoryReserveRatio = 0.20;
    std::uint64_t available_memory = static_cast<std::uint64_t>(
        gpu_memory_bytes * (1.0 - kMemoryReserveRatio)
    );

    // 每个key的内存占用估算 (字节)
    // 私钥: 32字节, 公钥: 64字节, hash160: 20字节
    constexpr std::uint64_t kBytesPerKey = 116;

    std::uint64_t optimal_batch = available_memory / kBytesPerKey;

    // 限制在合理范围内
    // 最小: 1M keys, 最大: 4B keys (或硬编码上限)
    constexpr std::uint64_t kMinBatchSize = 1ULL << 20;  // 1M keys
    constexpr std::uint64_t kMaxBatchSize = 1ULL << 32;  // 4B keys

    if (optimal_batch < kMinBatchSize) {
        optimal_batch = kMinBatchSize;
    }
    if (optimal_batch > kMaxBatchSize) {
        optimal_batch = kMaxBatchSize;
    }

    return optimal_batch;
}

class BatchPlanner {
public:
    explicit BatchPlanner(int device_id);

    BatchConfig Plan(const shards::ShardWalker& walker,
                     std::uint64_t desired_keys_hint = 1'048'576) const;

    void SetDeterministicLaunchConfig(const puzzle71::kernel::KernelLaunchConfig& config);

    // Phase A optimization: Remove artificial PPT limit to unlock higher batch sizes
    // Previous limit (64) capped H20 performance at 440 Mkeys/s
    // New limit (1024) enables 163M+ key batches for 2-3 Gkeys/s target
    static constexpr int kMaxPointsPerThread = 1024;

private:
    int device_id_{0};
    cudaDeviceProp props_{};
    std::optional<puzzle71::kernel::KernelLaunchConfig> deterministic_launch_;
};

std::uint64_t ComputeThreadCount(dim3 grid, dim3 block);
void ClampBatchConfig(BatchConfig& cfg, std::uint64_t keys_limit);

}  // namespace puzzle71::gpu

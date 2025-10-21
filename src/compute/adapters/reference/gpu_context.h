#pragma once

#include "compute/shards/shard_walker.h"
#include "compute/gpu/batch_planner.h"
#include "compute/gpu/gpu_executor.h"
#include "compute/gpu/separated_kernel_executor.h"
#include "scheduler/range_scheduler.h"

#include <array>

#include <optional>
#include <utility>

namespace puzzle71::reference_adapter {

struct GpuContext {
    shards::ShardWalker walker;
    gpu::BatchPlanner planner;
    gpu::GpuExecutor executor;
    std::unique_ptr<gpu::SeparatedKernelExecutor> separated_executor;
    bool use_separated_kernels{false};

    GpuContext(shards::ShardWalker w,
               gpu::BatchPlanner p,
               gpu::GpuExecutor&& e)
        : walker(std::move(w)),
          planner(std::move(p)),
          executor(std::move(e)) {}

    GpuContext(shards::ShardWalker w,
               gpu::BatchPlanner p,
               gpu::GpuExecutor&& e,
               std::unique_ptr<gpu::SeparatedKernelExecutor> sep_e)
        : walker(std::move(w)),
          planner(std::move(p)),
          executor(std::move(e)),
          separated_executor(std::move(sep_e)),
          use_separated_kernels(true) {}
};

GpuContext BuildGpuContext(const scheduler::Shard& shard,
                           const std::array<std::uint32_t, 5>& target_hash,
                           bool compressed,
                           bool verbose,
                           bool use_separated_kernels = false);

}  // namespace puzzle71::reference_adapter

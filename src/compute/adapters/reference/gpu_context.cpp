#include "compute/adapters/reference/gpu_context.h"

namespace puzzle71::reference_adapter {

GpuContext BuildGpuContext(const scheduler::Shard& shard,
                           const std::array<std::uint32_t, 5>& target_hash,
                           bool compressed,
                           bool verbose,
                           bool use_separated_kernels) {
    shards::ShardWalker walker(shard.start, shard.end);
    gpu::BatchPlanner planner(static_cast<int>(shard.device_id));
    gpu::GpuExecutor executor(static_cast<int>(shard.device_id), compressed, target_hash, verbose);

    if (use_separated_kernels) {
        // Create high-performance separated kernel executor
        auto separated_executor = gpu::SeparatedKernelExecutorFactory::CreateHighPerformance(
            static_cast<int>(shard.device_id),
            compressed,
            target_hash,
            verbose
        );
        return GpuContext(std::move(walker), std::move(planner), std::move(executor), std::move(separated_executor));
    } else {
        return GpuContext(std::move(walker), std::move(planner), std::move(executor));
    }
}

}  // namespace puzzle71::reference_adapter

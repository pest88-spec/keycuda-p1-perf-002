// Puzzle71Solver - GPU Memory Pool Management Implementation
// Advanced memory pool system for efficient GPU memory allocation (T033)

#include "gpu_memory_pool.cuh"
#include <algorithm>
#include <chrono>
#include <thread>
#include <cstring>

namespace keyhunt {
namespace memory {

// Implementation of MemoryPoolBase protected method
AllocationTier MemoryPoolBase::get_allocation_tier(size_t size) {
    if (size < 1024) return AllocationTier::SMALL;
    if (size < 1024 * 1024) return AllocationTier::MEDIUM;
    if (size < 64 * 1024 * 1024) return AllocationTier::LARGE;
    return AllocationTier::HUGE;
}

size_t MemoryPoolBase::align_size(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

void* MemoryPoolBase::allocate_from_device(size_t size) {
    void* device_ptr;
    cudaError_t result = cudaMalloc(&device_ptr, size);
    if (result != cudaSuccess) {
        throw std::runtime_error("CUDA malloc failed: " + std::string(cudaGetErrorString(result)));
    }
    return device_ptr;
}

// Implementation of TieredMemoryPool protected methods
MemoryBlock* TieredMemoryPool::find_free_block(size_t size) {
    AllocationTier tier = get_allocation_tier(size);
    return find_free_block_in_tier(tier, size);
}

// Implementation of MemoryPoolBase protected method (accessing protected member)
void MemoryPoolBase::set_configuration(const MemoryPoolConfig& config) {
    config_ = config;
}

/**
 * @brief Memory pool performance benchmark
 */
class MemoryPoolBenchmark {
public:
    struct BenchmarkResult {
        double pool_throughput_mb_per_sec;
        double cuda_direct_throughput_mb_per_sec;
        double speedup_factor;
        double average_allocation_time_us;
        double average_deallocation_time_us;
        size_t total_operations;
        double hit_ratio;
        double memory_efficiency;
    };

    /**
     * @brief Run comprehensive memory pool benchmark
     */
    static BenchmarkResult run_benchmark(
        size_t num_operations = 10000,
        size_t min_allocation_size = 64,
        size_t max_allocation_size = 1024 * 1024,
        bool enable_pool = true
    ) {
        BenchmarkResult result{};

        // Initialize memory pool if enabled
        std::unique_ptr<TieredMemoryPool> pool;
        if (enable_pool) {
            MemoryPoolConfig config;
            config.initial_pool_size_mb = 512;
            config.max_pool_size_mb = 2048;
            config.enable_statistics = true;
            pool = std::make_unique<TieredMemoryPool>(config);
        }

        // Generate random allocation sizes
        std::vector<size_t> allocation_sizes;
        std::vector<void*> pointers;
        allocation_sizes.reserve(num_operations);
        pointers.reserve(num_operations);

        // Seed random number generator
        std::srand(42);

        for (size_t i = 0; i < num_operations; ++i) {
            size_t size = min_allocation_size + (std::rand() % (max_allocation_size - min_allocation_size));
            allocation_sizes.push_back(size);
        }

        // Benchmark allocations
        auto start_time = std::chrono::high_resolution_clock::now();

        if (enable_pool) {
            // Pool allocations
            for (size_t i = 0; i < num_operations; ++i) {
                void* ptr = pool->allocate(allocation_sizes[i], "benchmark");
                pointers.push_back(ptr);

                // Initialize memory to ensure allocation is real
                if (ptr) {
                    cudaMemset(ptr, 0, allocation_sizes[i]);
                }
            }

            auto alloc_end_time = std::chrono::high_resolution_clock::now();
            auto alloc_duration = std::chrono::duration_cast<std::chrono::microseconds>(alloc_end_time - start_time);

            // Benchmark deallocations
            auto dealloc_start_time = std::chrono::high_resolution_clock::now();

            for (void* ptr : pointers) {
                pool->deallocate(ptr);
            }

            auto dealloc_end_time = std::chrono::high_resolution_clock::now();
            auto dealloc_duration = std::chrono::duration_cast<std::chrono::microseconds>(dealloc_end_time - dealloc_start_time);

            // Calculate statistics
            result.average_allocation_time_us = static_cast<double>(alloc_duration.count()) / num_operations;
            result.average_deallocation_time_us = static_cast<double>(dealloc_duration.count()) / num_operations;

            // Get pool statistics
            auto stats = pool->get_statistics();
            result.hit_ratio = stats.hit_ratio;
            result.memory_efficiency = 1.0 - stats.fragmentation_ratio;

        } else {
            // Direct CUDA allocations
            for (size_t i = 0; i < num_operations; ++i) {
                void* ptr;
                cudaMalloc(&ptr, allocation_sizes[i]);
                pointers.push_back(ptr);

                if (ptr) {
                    cudaMemset(ptr, 0, allocation_sizes[i]);
                }
            }

            auto alloc_end_time = std::chrono::high_resolution_clock::now();
            auto alloc_duration = std::chrono::duration_cast<std::chrono::microseconds>(alloc_end_time - start_time);

            // Direct CUDA deallocations
            auto dealloc_start_time = std::chrono::high_resolution_clock::now();

            for (void* ptr : pointers) {
                cudaFree(ptr);
            }

            auto dealloc_end_time = std::chrono::high_resolution_clock::now();
            auto dealloc_duration = std::chrono::duration_cast<std::chrono::microseconds>(dealloc_end_time - dealloc_start_time);

            result.average_allocation_time_us = static_cast<double>(alloc_duration.count()) / num_operations;
            result.average_deallocation_time_us = static_cast<double>(dealloc_duration.count()) / num_operations;

            result.hit_ratio = 0.0; // No pool hit ratio for direct allocations
            result.memory_efficiency = 1.0; // Assume perfect efficiency for direct allocations
        }

        auto total_end_time = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(total_end_time - start_time);

        // Calculate total bytes allocated
        size_t total_bytes = 0;
        for (size_t size : allocation_sizes) {
            total_bytes += size;
        }

        // Calculate throughput (MB/s)
        double total_time_seconds = total_duration.count() / 1000000.0;
        result.total_operations = num_operations;

        if (enable_pool) {
            result.pool_throughput_mb_per_sec = (total_bytes / (1024.0 * 1024.0)) / total_time_seconds;
        } else {
            result.cuda_direct_throughput_mb_per_sec = (total_bytes / (1024.0 * 1024.0)) / total_time_seconds;
        }

        // Calculate speedup factor (need both pool and direct results)
        if (enable_pool && result.cuda_direct_throughput_mb_per_sec > 0) {
            result.speedup_factor = result.pool_throughput_mb_per_sec / result.cuda_direct_throughput_mb_per_sec;
        } else {
            result.speedup_factor = 1.0;
        }

        return result;
    }

    /**
     * @brief Run comparative benchmark (pool vs direct CUDA)
     */
    static std::pair<BenchmarkResult, BenchmarkResult> run_comparative_benchmark(
        size_t num_operations = 10000,
        size_t min_allocation_size = 64,
        size_t max_allocation_size = 1024 * 1024
    ) {
        // Run pool benchmark
        auto pool_result = run_benchmark(num_operations, min_allocation_size, max_allocation_size, true);

        // Run direct CUDA benchmark
        auto direct_result = run_benchmark(num_operations, min_allocation_size, max_allocation_size, false);

        // Copy direct results to pool result for comparison
        pool_result.cuda_direct_throughput_mb_per_sec = direct_result.cuda_direct_throughput_mb_per_sec;
        pool_result.speedup_factor = pool_result.pool_throughput_mb_per_sec / direct_result.cuda_direct_throughput_mb_per_sec;

        return {pool_result, direct_result};
    }
};

/**
 * @brief Memory pool diagnostic tools
 */
class MemoryPoolDiagnostics {
public:
    struct DiagnosticReport {
        bool is_healthy;
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
        std::vector<std::string> recommendations;
        MemoryPoolStats stats;
    };

    /**
     * @brief Perform comprehensive diagnostic on memory pool
     */
    static DiagnosticReport diagnose_pool(MemoryPoolBase& pool) {
        DiagnosticReport report;
        report.stats = pool.get_statistics();
        report.is_healthy = true;

        // Check hit ratio
        if (report.stats.hit_ratio < 0.8) {
            report.warnings.push_back("Low pool hit ratio: " + std::to_string(report.stats.hit_ratio * 100) + "%");
            report.recommendations.push_back("Consider increasing initial pool size");
        }

        // Check fragmentation
        if (report.stats.fragmentation_ratio > 0.3) {
            report.warnings.push_back("High fragmentation ratio: " + std::to_string(report.stats.fragmentation_ratio * 100) + "%");
            report.recommendations.push_back("Enable automatic defragmentation");
        }

        // Check memory usage
        if (report.stats.total_allocated_bytes > 0) {
            double utilization = static_cast<double>(report.stats.total_used_bytes) / report.stats.total_allocated_bytes;
            if (utilization < 0.5) {
                report.warnings.push_back("Low memory utilization: " + std::to_string(utilization * 100) + "%");
                report.recommendations.push_back("Reduce pool size or enable aggressive garbage collection");
            } else if (utilization > 0.95) {
                report.warnings.push_back("High memory utilization: " + std::to_string(utilization * 100) + "%");
                report.recommendations.push_back("Increase pool size to avoid allocation failures");
            }
        }

        // Check allocation/deallocation times
        if (report.stats.average_allocation_time_ms > 1.0) {
            report.warnings.push_back("Slow allocation time: " + std::to_string(report.stats.average_allocation_time_ms) + "ms");
            report.recommendations.push_back("Check for memory pressure or pool size issues");
        }

        if (report.stats.average_deallocation_time_ms > 0.5) {
            report.warnings.push_back("Slow deallocation time: " + std::to_string(report.stats.average_deallocation_time_ms) + "ms");
            report.recommendations.push_back("Optimize garbage collection settings");
        }

        // Check for allocation/deallocation balance
        if (report.stats.allocation_count > report.stats.deallocation_count + 1000) {
            report.warnings.push_back("Memory leak detected: " +
                             std::to_string(report.stats.allocation_count - report.stats.deallocation_count) + " unfreed allocations");
            report.is_healthy = false;
            report.errors.push_back("CRITICAL: Memory leak detected");
        }

        // Performance recommendations
        if (report.stats.hit_ratio < memory_pool_performance::TARGET_HIT_RATIO) {
            report.recommendations.push_back("Increase pool size to improve hit ratio");
        }

        if (report.stats.fragmentation_ratio > memory_pool_performance::TARGET_FRAGMENTATION_RATIO) {
            report.recommendations.push_back("Enable automatic defragmentation");
        }

        if (report.warnings.empty() && report.errors.empty()) {
            report.recommendations.push_back("Memory pool is operating optimally");
        }

        return report;
    }

    /**
     * @brief Print diagnostic report
     */
    static void print_diagnostic_report(const DiagnosticReport& report) {
        printf("\n=== Memory Pool Diagnostic Report ===\n");
        printf("Health Status: %s\n", report.is_healthy ? "HEALTHY" : "UNHEALTHY");

        if (!report.errors.empty()) {
            printf("\nERRORS:\n");
            for (const auto& error : report.errors) {
                printf("  - %s\n", error.c_str());
            }
        }

        if (!report.warnings.empty()) {
            printf("\nWARNINGS:\n");
            for (const auto& warning : report.warnings) {
                printf("  - %s\n", warning.c_str());
            }
        }

        if (!report.recommendations.empty()) {
            printf("\nRECOMMENDATIONS:\n");
            for (const auto& recommendation : report.recommendations) {
                printf("  - %s\n", recommendation.c_str());
            }
        }

        printf("\nSTATISTICS:\n");
        printf("  Total Allocated: %zu MB\n", report.stats.total_allocated_bytes / (1024 * 1024));
        printf("  Total Used: %zu MB\n", report.stats.total_used_bytes / (1024 * 1024));
        printf("  Peak Usage: %zu MB\n", report.stats.peak_usage_bytes / (1024 * 1024));
        printf("  Hit Ratio: %.2f%%\n", report.stats.hit_ratio * 100);
        printf("  Fragmentation: %.2f%%\n", report.stats.fragmentation_ratio * 100);
        printf("  Average Alloc Time: %.3f ms\n", report.stats.average_allocation_time_ms);
        printf("  Average Dealloc Time: %.3f ms\n", report.stats.average_deallocation_time_ms);
        printf("  Total Allocations: %zu\n", report.stats.allocation_count);
        printf("  Total Deallocations: %zu\n", report.stats.deallocation_count);
        printf("=======================================\n\n");
    }
};

/**
 * @brief Memory pool factory with presets
 */
class MemoryPoolFactory {
public:
    /**
     * @brief Create high-performance memory pool
     */
    static std::unique_ptr<TieredMemoryPool> create_high_performance_pool() {
        MemoryPoolConfig config;
        config.initial_pool_size_mb = 2048;
        config.max_pool_size_mb = 8192;
        config.min_block_size_bytes = 64;
        config.max_block_size_bytes = 256 * 1024 * 1024;
        config.enable_garbage_collection = true;
        config.gc_interval = std::chrono::seconds(60);
        config.gc_threshold_ratio = 0.2;
        config.enable_defragmentation = true;
        config.defrag_interval = std::chrono::seconds(600);
        config.enable_statistics = true;
        config.enable_debug_logging = false;

        return std::make_unique<TieredMemoryPool>(config);
    }

    /**
     * @brief Create memory conservative pool
     */
    static std::unique_ptr<TieredMemoryPool> create_memory_conservative_pool() {
        MemoryPoolConfig config;
        config.initial_pool_size_mb = 256;
        config.max_pool_size_mb = 1024;
        config.min_block_size_bytes = 128;
        config.max_block_size_bytes = 64 * 1024 * 1024;
        config.enable_garbage_collection = true;
        config.gc_interval = std::chrono::seconds(30);
        config.gc_threshold_ratio = 0.1;
        config.enable_defragmentation = true;
        config.defrag_interval = std::chrono::seconds(300);
        config.enable_statistics = true;
        config.enable_debug_logging = false;

        return std::make_unique<TieredMemoryPool>(config);
    }

    /**
     * @brief Create debug pool with extensive logging
     */
    static std::unique_ptr<TieredMemoryPool> create_debug_pool() {
        MemoryPoolConfig config;
        config.initial_pool_size_mb = 512;
        config.max_pool_size_mb = 2048;
        config.min_block_size_bytes = 64;
        config.max_block_size_bytes = 128 * 1024 * 1024;
        config.enable_garbage_collection = true;
        config.gc_interval = std::chrono::seconds(15);
        config.gc_threshold_ratio = 0.15;
        config.enable_defragmentation = true;
        config.defrag_interval = std::chrono::seconds(120);
        config.enable_statistics = true;
        config.enable_debug_logging = true;

        return std::make_unique<TieredMemoryPool>(config);
    }

    /**
     * @brief Create pool optimized for small allocations
     */
    static std::unique_ptr<TieredMemoryPool> create_small_allocation_pool() {
        MemoryPoolConfig config;
        config.initial_pool_size_mb = 1024;
        config.max_pool_size_mb = 4096;
        config.min_block_size_bytes = 32;
        config.max_block_size_bytes = 1024 * 1024;
        config.allocation_alignment_bytes = 64;
        config.enable_garbage_collection = true;
        config.gc_interval = std::chrono::seconds(45);
        config.gc_threshold_ratio = 0.25;
        config.enable_defragmentation = true;
        config.defrag_interval = std::chrono::seconds(240);
        config.enable_statistics = true;
        config.enable_debug_logging = false;

        return std::make_unique<TieredMemoryPool>(config);
    }

    /**
     * @brief Create pool optimized for large allocations
     */
    static std::unique_ptr<TieredMemoryPool> create_large_allocation_pool() {
        MemoryPoolConfig config;
        config.initial_pool_size_mb = 4096;
        config.max_pool_size_mb = 16384;
        config.min_block_size_bytes = 1024;
        config.max_block_size_bytes = 512 * 1024 * 1024;
        config.allocation_alignment_bytes = 512;
        config.enable_garbage_collection = true;
        config.gc_interval = std::chrono::seconds(120);
        config.gc_threshold_ratio = 0.1;
        config.enable_defragmentation = false; // Less effective for large allocations
        config.enable_statistics = true;
        config.enable_debug_logging = false;

        return std::make_unique<TieredMemoryPool>(config);
    }
};

/**
 * @brief Integration utilities for CUDA kernels
 */
class CudaMemoryPoolIntegration {
public:
    /**
     * @brief Allocate memory for CUDA kernel with automatic pool management
     */
    template<typename T>
    static T* allocate_for_kernel(size_t count, const std::string& tag = "kernel") {
        auto& pool = MemoryPoolManager::get_default_pool();
        return static_cast<T*>(pool.allocate(count * sizeof(T), tag));
    }

    /**
     * @brief Allocate memory with specific pool for CUDA kernel
     */
    template<typename T>
    static T* allocate_for_kernel(const std::string& pool_name, size_t count, const std::string& tag = "kernel") {
        auto* pool = MemoryPoolManager::get_pool(pool_name);
        if (!pool) {
            throw std::runtime_error("Pool not found: " + pool_name);
        }
        return static_cast<T*>(pool->allocate(count * sizeof(T), tag));
    }

    /**
     * @brief Deallocate memory from CUDA kernel
     */
    template<typename T>
    static void deallocate_from_kernel(T* ptr) {
        auto& pool = MemoryPoolManager::get_default_pool();
        pool.deallocate(ptr);
    }

    /**
     * @brief Initialize memory pool for CUDA operations
     */
    static void initialize_cuda_pool(const MemoryPoolConfig& config = MemoryPoolConfig()) {
        // Set CUDA device before pool initialization
        int device_id;
        cudaError_t result = cudaGetDevice(&device_id);
        if (result != cudaSuccess) {
            throw std::runtime_error("Failed to get CUDA device");
        }

        MemoryPoolManager::initialize(config);
    }

    /**
     * @brief Get memory pool statistics for CUDA operations
     */
    static MemoryPoolStats get_cuda_pool_stats() {
        return MemoryPoolManager::get_default_pool().get_statistics();
    }

    /**
     * @brief Perform CUDA-aware garbage collection
     */
    static void cuda_garbage_collect() {
        cudaDeviceSynchronize(); // Ensure all CUDA operations complete
        MemoryPoolManager::get_default_pool().garbage_collect();
    }
};

} // namespace memory
} // namespace keyhunt
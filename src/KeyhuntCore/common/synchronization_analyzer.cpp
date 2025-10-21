// Puzzle71Solver - Synchronization Overhead Analyzer
// Implements comprehensive synchronization overhead measurement and analysis for T050

#include "synchronization_analyzer.hpp"
#include "logging_utils.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <cmath>

using json = nlohmann::json;
using namespace std::chrono;

namespace keyhunt {
namespace performance {

SynchronizationAnalyzer::SynchronizationAnalyzer(int device_id)
    : device_id_(device_id), baseline_sync_overhead_ns_(1000000) { // 1ms default baseline

    // Initialize CUDA device
    cudaError_t err = cudaSetDevice(device_id_);
    if (err != cudaSuccess) {
        log_error("Failed to set CUDA device " + std::to_string(device_id_) + ": " +
                 std::string(cudaGetErrorString(err)));
        throw std::runtime_error("CUDA device initialization failed");
    }

    log_info("Synchronization analyzer initialized for device " + std::to_string(device_id_));
}

SynchronizationAnalyzer::~SynchronizationAnalyzer() {
    // Cleanup
}

bool SynchronizationAnalyzer::initialize(int device_id) {
    device_id_ = device_id;

    cudaError_t err = cudaSetDevice(device_id_);
    if (err != cudaSuccess) {
        log_error("Failed to set CUDA device " + std::to_string(device_id_) + ": " +
                 std::string(cudaGetErrorString(err)));
        return false;
    }

    log_info("Synchronization analyzer initialized for device " + std::to_string(device_id_));
    return true;
}

bool SynchronizationAnalyzer::measureSynchronizationOverhead(double& overhead_ns) {
    // Measure synchronization overhead using CUDA events
    cudaEvent_t start, stop;
    cudaError_t err = cudaEventCreate(&start);
    if (err != cudaSuccess) return false;

    err = cudaEventCreate(&stop);
    if (err != cudaSuccess) {
        cudaEventDestroy(start);
        return false;
    }

    // Measure pure synchronization overhead
    cudaEventRecord(start);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float milliseconds = 0.0f;
    err = cudaEventElapsedTime(&milliseconds, start, stop);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    if (err != cudaSuccess) {
        log_error("Failed to measure synchronization overhead: " + std::string(cudaGetErrorString(err)));
        return false;
    }

    overhead_ns = static_cast<double>(milliseconds) * 1000000.0; // Convert to nanoseconds
    return true;
}

bool SynchronizationAnalyzer::measureKernelSynchronizationOverhead(double& overhead_ns) {
    // Measure kernel synchronization overhead (time spent in __syncthreads())
    const int block_size = 256;
    const int num_blocks = 64;

    cudaEvent_t start, stop;
    cudaError_t err = cudaEventCreate(&start);
    if (err != cudaSuccess) return false;

    err = cudaEventCreate(&stop);
    if (err != cudaSuccess) {
        cudaEventDestroy(start);
        return false;
    }

    // Launch kernel with heavy synchronization
    cudaEventRecord(start);
    synchronizationKernel<<<num_blocks, block_size>>>();
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float milliseconds = 0.0f;
    err = cudaEventElapsedTime(&milliseconds, start, stop);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    if (err != cudaSuccess) {
        log_error("Failed to measure kernel synchronization overhead: " + std::string(cudaGetErrorString(err)));
        return false;
    }

    overhead_ns = static_cast<double>(milliseconds) * 1000000.0; // Convert to nanoseconds
    return true;
}

bool SynchronizationAnalyzer::measureAtomicOperationOverhead(double& overhead_ns) {
    // Measure atomic operation overhead
    const int block_size = 256;
    const int num_blocks = 64;

    // Allocate shared counter
    int* counter_d;
    cudaError_t err = cudaMalloc(&counter_d, sizeof(int));
    if (err != cudaSuccess) return false;

    // Initialize counter to 0
    err = cudaMemset(counter_d, 0, sizeof(int));
    if (err != cudaSuccess) {
        cudaFree(counter_d);
        return false;
    }

    cudaEvent_t start, stop;
    err = cudaEventCreate(&start);
    if (err != cudaSuccess) {
        cudaFree(counter_d);
        return false;
    }

    err = cudaEventCreate(&stop);
    if (err != cudaSuccess) {
        cudaEventDestroy(start);
        cudaFree(counter_d);
        return false;
    }

    // Launch kernel with heavy atomic operations
    cudaEventRecord(start);
    atomicKernel<<<num_blocks, block_size>>>(counter_d);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float milliseconds = 0.0f;
    err = cudaEventElapsedTime(&milliseconds, start, stop);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);
    cudaFree(counter_d);

    if (err != cudaSuccess) {
        log_error("Failed to measure atomic operation overhead: " + std::string(cudaGetErrorString(err)));
        return false;
    }

    overhead_ns = static_cast<double>(milliseconds) * 1000000.0; // Convert to nanoseconds
    return true;
}

bool SynchronizationAnalyzer::measureWarpSynchronizationOverhead(double& overhead_ns) {
    // Measure warp-level synchronization overhead
    const int block_size = 256;
    const int num_blocks = 64;

    cudaEvent_t start, stop;
    cudaError_t err = cudaEventCreate(&start);
    if (err != cudaSuccess) return false;

    err = cudaEventCreate(&stop);
    if (err != cudaSuccess) {
        cudaEventDestroy(start);
        return false;
    }

    // Launch kernel with warp-level synchronization
    cudaEventRecord(start);
    warpSyncKernel<<<num_blocks, block_size>>>();
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float milliseconds = 0.0f;
    err = cudaEventElapsedTime(&milliseconds, start, stop);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    if (err != cudaSuccess) {
        log_error("Failed to measure warp synchronization overhead: " + std::string(cudaGetErrorString(err)));
        return false;
    }

    overhead_ns = static_cast<double>(milliseconds) * 1000000.0; // Convert to nanoseconds
    return true;
}

bool SynchronizationAnalyzer::establishBaseline() {
    log_info("Establishing synchronization overhead baseline...");

    std::vector<double> measurements;
    const int num_samples = 10;

    for (int i = 0; i < num_samples; ++i) {
        double overhead = 0.0;
        if (measureSynchronizationOverhead(overhead)) {
            measurements.push_back(overhead);
        }
    }

    if (measurements.empty()) {
        log_error("Failed to collect baseline measurements");
        return false;
    }

    // Calculate average baseline
    double sum = std::accumulate(measurements.begin(), measurements.end(), 0.0);
    baseline_sync_overhead_ns_ = sum / measurements.size();

    // Calculate standard deviation
    double variance = 0.0;
    for (double measurement : measurements) {
        variance += (measurement - baseline_sync_overhead_ns_) * (measurement - baseline_sync_overhead_ns_);
    }
    variance /= measurements.size();
    baseline_std_dev_ns_ = sqrt(variance);

    log_info("Baseline synchronization overhead: " + std::to_string(baseline_sync_overhead_ns_) + " ns (±" + std::to_string(baseline_std_dev_ns_) + " ns)");

    return true;
}

bool SynchronizationAnalyzer::measureOptimizedSynchronizationOverhead(double& overhead_ns, OptimizationType type) {
    switch (type) {
        case OptimizationType::WARP_SHUFFLE:
            return measureWarpShuffleOverhead(overhead_ns);
        case OptimizationType::SHARED_MEMORY_OPTIMIZATION:
            return measureSharedMemoryOptimizedOverhead(overhead_ns);
        case OptimizationType::ATOMIC_AGGREGATION:
            return measureAtomicAggregationOverhead(overhead_ns);
        case OptimizationType::ASYNC_STREAMS:
            return measureAsyncStreamOverhead(overhead_ns);
        default:
            return measureSynchronizationOverhead(overhead_ns);
    }
}

bool SynchronizationAnalyzer::measureWarpShuffleOverhead(double& overhead_ns) {
    // Measure overhead when using warp shuffle instead of shared memory
    const int block_size = 256;
    const int num_blocks = 64;

    cudaEvent_t start, stop;
    cudaError_t err = cudaEventCreate(&start);
    if (err != cudaSuccess) return false;

    err = cudaEventCreate(&stop);
    if (err != cudaSuccess) {
        cudaEventDestroy(start);
        return false;
    }

    // Launch kernel using warp shuffle operations
    cudaEventRecord(start);
    warpShuffleKernel<<<num_blocks, block_size>>>();
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float milliseconds = 0.0f;
    err = cudaEventElapsedTime(&milliseconds, start, stop);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    if (err != cudaSuccess) {
        log_error("Failed to measure warp shuffle overhead: " + std::string(cudaGetErrorString(err)));
        return false;
    }

    overhead_ns = static_cast<double>(milliseconds) * 1000000.0;
    return true;
}

bool SynchronizationAnalyzer::measureSharedMemoryOptimizedOverhead(double& overhead_ns) {
    // Measure overhead with shared memory optimizations
    const int block_size = 256;
    const int num_blocks = 64;

    cudaEvent_t start, stop;
    cudaError_t err = cudaEventCreate(&start);
    if (err != cudaSuccess) return false;

    err = cudaEventCreate(&stop);
    if (err != cudaSuccess) {
        cudaEventDestroy(start);
        return false;
    }

    // Launch kernel with optimized shared memory access
    cudaEventRecord(start);
    sharedMemoryOptimizedKernel<<<num_blocks, block_size>>>();
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float milliseconds = 0.0f;
    err = cudaEventElapsedTime(&milliseconds, start, stop);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    if (err != cudaSuccess) {
        log_error("Failed to measure shared memory optimized overhead: " + std::string(cudaGetErrorString(err)));
        return false;
    }

    overhead_ns = static_cast<double>(milliseconds) * 1000000.0;
    return true;
}

bool SynchronizationAnalyzer::measureAtomicAggregationOverhead(double& overhead_ns) {
    // Measure overhead with atomic operation aggregation
    const int block_size = 256;
    const int num_blocks = 64;

    // Allocate shared counter for aggregated atomic operations
    int* counter_d;
    cudaError_t err = cudaMalloc(&counter_d, sizeof(int));
    if (err != cudaSuccess) return false;

    err = cudaMemset(counter_d, 0, sizeof(int));
    if (err != cudaSuccess) {
        cudaFree(counter_d);
        return false;
    }

    cudaEvent_t start, stop;
    err = cudaEventCreate(&start);
    if (err != cudaSuccess) {
        cudaFree(counter_d);
        return false;
    }

    err = cudaEventCreate(&stop);
    if (err != cudaSuccess) {
        cudaEventDestroy(start);
        cudaFree(counter_d);
        return false;
    }

    // Launch kernel with aggregated atomic operations
    cudaEventRecord(start);
    atomicAggregationKernel<<<num_blocks, block_size>>>(counter_d);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float milliseconds = 0.0f;
    err = cudaEventElapsedTime(&milliseconds, start, stop);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);
    cudaFree(counter_d);

    if (err != cudaSuccess) {
        log_error("Failed to measure atomic aggregation overhead: " + std::string(cudaGetErrorString(err)));
        return false;
    }

    overhead_ns = static_cast<double>(milliseconds) * 1000000.0;
    return true;
}

bool SynchronizationAnalyzer::measureAsyncStreamOverhead(double& overhead_ns) {
    // Measure overhead when using asynchronous streams
    cudaStream_t stream1, stream2;
    cudaError_t err = cudaStreamCreate(&stream1);
    if (err != cudaSuccess) return false;

    err = cudaStreamCreate(&stream2);
    if (err != cudaSuccess) {
        cudaStreamDestroy(stream1);
        return false;
    }

    cudaEvent_t start, stop;
    err = cudaEventCreate(&start);
    if (err != cudaSuccess) {
        cudaStreamDestroy(stream1);
        cudaStreamDestroy(stream2);
        return false;
    }

    err = cudaEventCreate(&stop);
    if (err != cudaSuccess) {
        cudaEventDestroy(start);
        cudaStreamDestroy(stream1);
        cudaStreamDestroy(stream2);
        return false;
    }

    const int block_size = 256;
    const int num_blocks = 32;

    // Launch kernels on different streams asynchronously
    cudaEventRecord(start);
    asyncKernel<<<num_blocks, block_size, 0, stream1>>>();
    asyncKernel<<<num_blocks, block_size, 0, stream2>>>();
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float milliseconds = 0.0f;
    err = cudaEventElapsedTime(&milliseconds, start, stop);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);
    cudaStreamDestroy(stream1);
    cudaStreamDestroy(stream2);

    if (err != cudaSuccess) {
        log_error("Failed to measure async stream overhead: " + std::string(cudaGetErrorString(err)));
        return false;
    }

    overhead_ns = static_cast<double>(milliseconds) * 1000000.0;
    return true;
}

double SynchronizationAnalyzer::calculateOverheadReduction(double baseline_ns, double optimized_ns) {
    if (baseline_ns <= 0.0) return 0.0;
    return ((baseline_ns - optimized_ns) / baseline_ns) * 100.0;
}

bool SynchronizationAnalyzer::generateSynchronizationReport(std::string& report) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    oss << "=== Synchronization Overhead Analysis Report ===\n";
    oss << "Device ID: " << device_id_ << "\n";
    oss << "Baseline Synchronization Overhead: " << baseline_sync_overhead_ns_ << " ns\n";
    oss << "Baseline Standard Deviation: " << baseline_std_dev_ns_ << " ns\n\n";

    // Measure different synchronization overheads
    double sync_overhead = 0.0, kernel_sync_overhead = 0.0, atomic_overhead = 0.0, warp_sync_overhead = 0.0;

    bool sync_success = measureSynchronizationOverhead(sync_overhead);
    bool kernel_sync_success = measureKernelSynchronizationOverhead(kernel_sync_overhead);
    bool atomic_success = measureAtomicOperationOverhead(atomic_overhead);
    bool warp_sync_success = measureWarpSynchronizationOverhead(warp_sync_overhead);

    oss << "Current Synchronization Overheads:\n";
    if (sync_success) {
        double reduction = calculateOverheadReduction(baseline_sync_overhead_ns_, sync_overhead);
        oss << "  Basic Synchronization: " << sync_overhead << " ns (" << reduction << "% reduction from baseline)\n";
    }

    if (kernel_sync_success) {
        oss << "  Kernel Synchronization: " << kernel_sync_overhead << " ns\n";
    }

    if (atomic_success) {
        oss << "  Atomic Operations: " << atomic_overhead << " ns\n";
    }

    if (warp_sync_success) {
        oss << "  Warp Synchronization: " << warp_sync_overhead << " ns\n";
    }

    // Measure optimized synchronization overheads
    oss << "\nOptimized Synchronization Overheads:\n";

    double warp_shuffle_overhead = 0.0;
    if (measureWarpShuffleOverhead(warp_shuffle_overhead)) {
        double reduction = calculateOverheadReduction(baseline_sync_overhead_ns_, warp_shuffle_overhead);
        oss << "  Warp Shuffle Optimization: " << warp_shuffle_overhead << " ns (" << reduction << "% reduction from baseline)\n";
    }

    double shared_mem_overhead = 0.0;
    if (measureSharedMemoryOptimizedOverhead(shared_mem_overhead)) {
        double reduction = calculateOverheadReduction(baseline_sync_overhead_ns_, shared_mem_overhead);
        oss << "  Shared Memory Optimization: " << shared_mem_overhead << " ns (" << reduction << "% reduction from baseline)\n";
    }

    double atomic_agg_overhead = 0.0;
    if (measureAtomicAggregationOverhead(atomic_agg_overhead)) {
        double reduction = calculateOverheadReduction(baseline_sync_overhead_ns_, atomic_agg_overhead);
        oss << "  Atomic Aggregation: " << atomic_agg_overhead << " ns (" << reduction << "% reduction from baseline)\n";
    }

    double async_stream_overhead = 0.0;
    if (measureAsyncStreamOverhead(async_stream_overhead)) {
        double reduction = calculateOverheadReduction(baseline_sync_overhead_ns_, async_stream_overhead);
        oss << "  Async Streams: " << async_stream_overhead << " ns (" << reduction << "% reduction from baseline)\n";
    }

    // Performance assessment
    oss << "\nPerformance Assessment:\n";

    const double TARGET_REDUCTION = 50.0; // Constitutional requirement: 50% reduction

    if (sync_success) {
        double reduction = calculateOverheadReduction(baseline_sync_overhead_ns_, sync_overhead);
        bool target_met = reduction >= TARGET_REDUCTION;
        oss << "  Basic Synchronization: " << (target_met ? "✅ PASS" : "❌ FAIL");
        oss << " (" << reduction << "% reduction vs " << TARGET_REDUCTION << "% target)\n";
    }

    // Check if any optimization meets the target
    std::vector<std::pair<std::string, double>> optimizations;

    if (warp_shuffle_overhead > 0) {
        double reduction = calculateOverheadReduction(baseline_sync_overhead_ns_, warp_shuffle_overhead);
        optimizations.emplace_back("Warp Shuffle", reduction);
    }

    if (shared_mem_overhead > 0) {
        double reduction = calculateOverheadReduction(baseline_sync_overhead_ns_, shared_mem_overhead);
        optimizations.emplace_back("Shared Memory", reduction);
    }

    if (atomic_agg_overhead > 0) {
        double reduction = calculateOverheadReduction(baseline_sync_overhead_ns_, atomic_agg_overhead);
        optimizations.emplace_back("Atomic Aggregation", reduction);
    }

    if (async_stream_overhead > 0) {
        double reduction = calculateOverheadReduction(baseline_sync_overhead_ns_, async_stream_overhead);
        optimizations.emplace_back("Async Streams", reduction);
    }

    if (!optimizations.empty()) {
        auto best_optimization = std::max_element(optimizations.begin(), optimizations.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });

        bool target_met = best_optimization->second >= TARGET_REDUCTION;
        oss << "  Best Optimization (" << best_optimization->first << "): " << (target_met ? "✅ PASS" : "❌ FAIL");
        oss << " (" << best_optimization->second << "% reduction vs " << TARGET_REDUCTION << "% target)\n";
    }

    // Recommendations
    oss << "\nRecommendations:\n";

    if (sync_success) {
        double reduction = calculateOverheadReduction(baseline_sync_overhead_ns_, sync_overhead);
        if (reduction < TARGET_REDUCTION) {
            oss << "  - Basic synchronization overhead reduction below target. Consider optimizations:\n";

            if (warp_shuffle_overhead > 0 && warp_shuffle_overhead < sync_overhead) {
                double imp = calculateOverheadReduction(sync_overhead, warp_shuffle_overhead);
                oss << "    * Use warp shuffle operations (" << imp << "% improvement)\n";
            }

            if (shared_mem_overhead > 0 && shared_mem_overhead < sync_overhead) {
                double imp = calculateOverheadReduction(sync_overhead, shared_mem_overhead);
                oss << "    * Optimize shared memory access patterns (" << imp << "% improvement)\n";
            }
        }
    }

    if (atomic_success && atomic_overhead > baseline_sync_overhead_ns_ * 2) {
        oss << "  - Atomic operation overhead is high. Consider aggregation strategies.\n";
    }

    if (warp_sync_success) {
        oss << "  - Warp synchronization measured. Consider using shuffle instructions for better performance.\n";
    }

    report = oss.str();
    return true;
}

// CUDA kernel implementations

__global__ void SynchronizationAnalyzer::synchronizationKernel() {
    __shared__ int shared_data[256];

    // Initialize shared memory
    shared_data[threadIdx.x] = threadIdx.x;
    __syncthreads();

    // Perform operations with synchronization
    for (int i = 0; i < 100; ++i) {
        // Some computation
        int value = shared_data[threadIdx.x] * i;

        // Synchronize to ensure data consistency
        __syncthreads();

        // Update shared memory
        shared_data[(threadIdx.x + 1) % 256] = value;

        // Another synchronization
        __syncthreads();
    }
}

__global__ void SynchronizationAnalyzer::atomicKernel(int* counter) {
    // Perform atomic operations
    for (int i = 0; i < 100; ++i) {
        atomicAdd(counter, 1);
    }
}

__global__ void SynchronizationAnalyzer::warpSyncKernel() {
    // Use warp shuffle operations instead of shared memory
    int value = threadIdx.x;

    for (int i = 0; i < 100; ++i) {
        // Broadcast within warp
        value = __shfl_sync(0xFFFFFFFF, value, 0);

        // Shuffle down
        value = __shfl_down_sync(0xFFFFFFFF, value, 1);

        // Shuffle up
        value = __shfl_up_sync(0xFFFFFFFF, value, 1);
    }
}

__global__ void SynchronizationAnalyzer::warpShuffleKernel() {
    // Demonstrate warp shuffle optimization
    int value = threadIdx.x;
    int sum = 0;

    // Butterfly reduction using shuffle (no shared memory needed)
    for (int stride = 16; stride > 0; stride /= 2) {
        value += __shfl_xor_sync(0xFFFFFFFF, value, stride);
    }

    sum = value;
}

__global__ void SynchronizationAnalyzer::sharedMemoryOptimizedKernel() {
    // Optimized shared memory access with reduced synchronization
    __shared__ int shared_data[256];

    // Bank-conflict free access pattern
    int padded_idx = threadIdx.x + (threadIdx.x / 32); // Avoid bank conflicts

    // Initialize
    shared_data[padded_idx % 256] = threadIdx.x;
    __syncthreads();

    // Reduce synchronization points
    for (int i = 0; i < 50; ++i) {
        // Do more work between synchronizations
        int value = shared_data[padded_idx % 256] * i;
        value = sqrt(value) + i;

        // Only synchronize when necessary
        if (i % 10 == 0) {
            __syncthreads();
            shared_data[padded_idx % 256] = value;
            __syncthreads();
        }
    }
}

__global__ void SynchronizationAnalyzer::atomicAggregationKernel(int* counter) {
    // Aggregate atomic operations to reduce contention
    __shared__ int local_counter;

    if (threadIdx.x == 0) {
        local_counter = 0;
    }
    __syncthreads();

    // Each thread increments local counter
    for (int i = 0; i < 100; ++i) {
        atomicAdd(&local_counter, 1);
    }

    __syncthreads();

    // Only one thread performs global atomic operation
    if (threadIdx.x == 0) {
        atomicAdd(counter, local_counter);
    }
}

__global__ void SynchronizationAnalyzer::asyncKernel() {
    // Simple kernel for async execution
    int result = 0;
    for (int i = 0; i < 1000; ++i) {
        result += i * threadIdx.x;
    }
}

} // namespace performance
} // namespace keyhunt
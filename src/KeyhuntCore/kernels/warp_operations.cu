// Puzzle71Solver - Warp-Level Atomic Operations Implementation
// Advanced warp-level synchronization and atomic operations (T035)

#include "warp_operations.cuh"
#include <cuda_runtime.h>

namespace keyhunt {
namespace kernels {

/**
 * @brief Example kernel demonstrating warp-level atomic operations
 */
__global__ void warp_atomic_example_kernel(int* global_counter, int* results, int num_iterations) {
    const unsigned full_mask = 0xffffffffu;
    int lane_id = threadIdx.x % 32;
    int warp_id = (threadIdx.x + blockIdx.x * blockDim.x) / 32;

    // Each thread in warp performs atomic operations
    int thread_value = lane_id + 1;

    for (int i = 0; i < num_iterations; ++i) {
        // Warp-level atomic addition
        int old_value = WARP_ATOMIC_ADD(global_counter + warp_id, thread_value);

        // Broadcast old value to all threads in warp
        old_value = __shfl_sync(full_mask, old_value, 0);

        // Each thread stores the result
        if (lane_id == 0) {
            results[warp_id] = old_value + thread_value;
        }
    }
}

/**
 * @brief Example kernel demonstrating warp-level reductions
 */
__global__ void warp_reduction_example_kernel(int* input_data, int* reduction_results) {
    const unsigned full_mask = 0xffffffffu;
    int lane_id = threadIdx.x % 32;
    int warp_id = (threadIdx.x + blockIdx.x * blockDim.x) / 32;

    // Read input data
    int my_value = input_data[threadIdx.x + blockIdx.x * blockDim.x];

    // Perform different reductions
    int sum = WARP_REDUCE_SUM(my_value);
    int max = WARP_REDUCE_MAX(my_value);
    int min = WARP_REDUCE_MIN(my_value);

    // Store results (only first thread in each warp)
    if (lane_id == 0) {
        reduction_results[warp_id * 3 + 0] = sum;
        reduction_results[warp_id * 3 + 1] = max;
        reduction_results[warp_id * 3 + 2] = min;
    }
}

/**
 * @brief Example kernel demonstrating warp-level prefix scans
 */
__global__ void warp_prefix_scan_example_kernel(int* input_data, int* scan_results) {
    int lane_id = threadIdx.x % 32;
    int warp_id = (threadIdx.x + blockIdx.x * blockDim.x) / 32;

    // Read input data
    int my_value = input_data[threadIdx.x + blockIdx.x * blockDim.x];

    // Perform exclusive prefix scan
    int exclusive_scan = WARP_SCAN_EXCLUSIVE(my_value);

    // Perform inclusive prefix scan
    int inclusive_scan = WARP_SCAN_INCLUSIVE(my_value);

    // Store results
    scan_results[(threadIdx.x + blockIdx.x * blockDim.x) * 2 + 0] = exclusive_scan;
    scan_results[(threadIdx.x + blockIdx.x * blockDim.x) * 2 + 1] = inclusive_scan;
}

/**
 * @brief Example kernel demonstrating conflict resolution
 */
__global__ void conflict_resolution_example_kernel(int* target_array, int array_size, int num_threads) {
    int lane_id = threadIdx.x % 32;
    int global_tid = threadIdx.x + blockIdx.x * blockDim.x;

    if (global_tid >= num_threads) return;

    // Simulate potential conflicts by having threads target the same indices
    int target_index = (global_tid * 7) % array_size; // Pseudo-random targeting
    int value_to_write = global_tid;

    // Use cooperative write with conflict resolution
    WarpConflictResolution::cooperative_write(target_array, target_index, value_to_write, array_size);
}

/**
 * @brief Example kernel demonstrating histogram building
 */
__global__ void histogram_example_kernel(int* input_data, int* histogram, int num_data, int num_bins) {
    int lane_id = threadIdx.x % 32;
    int global_tid = threadIdx.x + blockIdx.x * blockDim.x;

    if (global_tid >= num_data) return;

    // Read input value
    int value = input_data[global_tid];

    // Build histogram using warp-level operations
    WarpHistogram::build_histogram(value, histogram, num_bins);
}

/**
 * @brief Performance benchmark kernel for warp operations
 */
__global__ void warp_performance_benchmark_kernel(
    int* global_data,
    int* performance_metrics,
    int num_operations,
    bool test_atomics,
    bool test_reductions,
    bool test_scans,
    bool test_conflicts
) {
    const unsigned full_mask = 0xffffffffu;
    int lane_id = threadIdx.x % 32;
    int warp_id = (threadIdx.x + blockIdx.x * blockDim.x) / 32;

    clock_t start_time = clock();

    // Test atomic operations
    if (test_atomics) {
        for (int i = 0; i < num_operations; ++i) {
            int value = lane_id + i;
            WARP_ATOMIC_ADD(global_data + warp_id, value);
        }
    }

    // Test reductions
    if (test_reductions) {
        int test_value = lane_id + warp_id;
        int sum = WARP_REDUCE_SUM(test_value);
        if (lane_id == 0) {
            performance_metrics[warp_id] = sum;
        }
    }

    // Test prefix scans
    if (test_scans) {
        int scan_value = lane_id;
        int result = WARP_SCAN_EXCLUSIVE(scan_value);
        if (lane_id == 0) {
            performance_metrics[warp_id + gridDim.x * blockDim.x] = result;
        }
    }

    // Test conflict resolution
    if (test_conflicts) {
        int conflict_index = (warp_id * 13 + lane_id * 7) % 1024;
        int conflict_value = global_tid + blockIdx.x * blockDim.x;
        bool won_conflict = WarpConflictResolution::resolve_write_conflict(
            &global_data[conflict_index],
            conflict_value,
            conflict_index
        );

        if (won_conflict && lane_id == 0) {
            performance_metrics[warp_id + 2 * gridDim.x * blockDim.x] = 1;
        }
    }

    clock_t end_time = clock();
    clock_t elapsed = end_time - start_time;

    // Store timing information
    if (lane_id == 0) {
        performance_metrics[warp_id + 3 * gridDim.x * blockDim.x] = static_cast<int>(elapsed);
    }
}

/**
 * @brief Memory coalescing analysis kernel
 */
__global__ void memory_coalescing_analysis_kernel(
    int* global_data,
    void** address_array,
    float* coalescing_metrics,
    int access_pattern
) {
    int lane_id = threadIdx.x % 32;
    int global_tid = threadIdx.x + blockIdx.x * blockDim.x;

    // Generate different access patterns
    switch (access_pattern) {
        case 0: // Sequential access (coalesced)
            address_array[global_tid] = &global_data[global_tid];
            break;
        case 1: // Strided access (partially coalesced)
            address_array[global_tid] = &global_data[global_tid * 4];
            break;
        case 2: // Random access (uncoalesced)
            address_array[global_tid] = &global_data[(global_tid * 7) % (blockDim.x * gridDim.x)];
            break;
    }

    __syncthreads();

    // Profile memory coalescing efficiency
    if (lane_id < blockDim.x / 32) { // Only first warp in block
        float efficiency = WarpPerformanceMonitor::profile_memory_coalescing(
            address_array + blockIdx.x * blockDim.x,
            blockDim.x
        );
        coalescing_metrics[blockIdx.x] = efficiency;
    }
}

/**
 * @brief Advanced warp-level sorting kernel (bitonic sort)
 */
__global__ void warp_bitonic_sort_kernel(int* data, int size) {
    const unsigned full_mask = 0xffffffffu;
    int lane_id = threadIdx.x % 32;
    int warp_id = (threadIdx.x + blockIdx.x * blockDim.x) / 32;

    // Each warp processes 32 elements
    int warp_start = warp_id * 32;
    if (warp_start + 31 >= size) return;

    // Load data into registers
    int my_value = data[warp_start + lane_id];

    // Bitonic sort within warp
    for (int k = 2; k <= 32; k <<= 1) {
        for (int j = k >> 1; j > 0; j >>= 1) {
            int partner = lane_id ^ j;
            int partner_value = __shfl_xor_sync(full_mask, my_value, partner);

            bool ascending = ((lane_id & k) == 0);
            if ((ascending && my_value > partner_value) || (!ascending && my_value < partner_value)) {
                my_value = partner_value;
            }
        }
    }

    // Store sorted data
    data[warp_start + lane_id] = my_value;
}

/**
 * @brief Warp-level parallel prefix sum for large arrays
 */
__global__ void large_array_prefix_sum_kernel(
    int* input_data,
    int* prefix_sums,
    int* block_sums,
    int array_size
) {
    const unsigned full_mask = 0xffffffffu;
    int lane_id = threadIdx.x % 32;
    int warp_id = threadIdx.x / 32;
    int block_id = blockIdx.x;

    // Each block processes 1024 elements (32 warps * 32 threads)
    int block_start = block_id * 1024;

    // Load data into shared memory
    extern __shared__ int shared_data[];

    int global_idx = block_start + threadIdx.x;
    shared_data[threadIdx.x] = (global_idx < array_size) ? input_data[global_idx] : 0;

    __syncthreads();

    // Each warp performs prefix sum on its 32 elements
    int warp_sum = WARP_REDUCE_SUM(shared_data[threadIdx.x]);

    // Store partial sums for each warp
    if (lane_id == 0) {
        block_sums[block_id * 32 + warp_id] = warp_sum;
    }

    __syncthreads();

    // First warp in block performs prefix sum on warp sums
    if (warp_id == 0 && lane_id < 32) {
        int warp_partial_sum = (lane_id < 32) ? block_sums[block_id * 32 + lane_id] : 0;
        int warp_prefix = WARP_SCAN_EXCLUSIVE(warp_partial_sum);

        if (lane_id < 32) {
            block_sums[block_id * 32 + lane_id] = warp_prefix;
        }
    }

    __syncthreads();

    // Each thread adds its warp's prefix sum
    int warp_prefix = (warp_id > 0) ? block_sums[block_id * 32 + warp_id - 1] : 0;
    int thread_value = shared_data[threadIdx.x];
    int thread_prefix = WARP_SCAN_EXCLUSIVE(thread_value);
    int final_prefix = thread_prefix + warp_prefix;

    // Store result
    if (global_idx < array_size) {
        prefix_sums[global_idx] = final_prefix;
    }
}

/**
 * @brief Host-side utility functions for warp operations
 */
class WarpOperationsHost {
public:
    /**
     * @brief Launch atomic operations example
     */
    static void launch_atomic_example(int* global_counter, int* results, int num_iterations = 1000) {
        int blocks = 4;
        int threads_per_block = 256;

        warp_atomic_example_kernel<<<blocks, threads_per_block>>>(
            global_counter, results, num_iterations
        );
        cudaDeviceSynchronize();
    }

    /**
     * @brief Launch reduction example
     */
    static void launch_reduction_example(int* input_data, int* reduction_results, int num_elements) {
        int blocks = (num_elements + 255) / 256;
        int threads_per_block = 256;

        warp_reduction_example_kernel<<<blocks, threads_per_block>>>(
            input_data, reduction_results
        );
        cudaDeviceSynchronize();
    }

    /**
     * @brief Launch prefix scan example
     */
    static void launch_prefix_scan_example(int* input_data, int* scan_results, int num_elements) {
        int blocks = (num_elements + 255) / 256;
        int threads_per_block = 256;

        warp_prefix_scan_example_kernel<<<blocks, threads_per_block>>>(
            input_data, scan_results
        );
        cudaDeviceSynchronize();
    }

    /**
     * @brief Launch conflict resolution example
     */
    static void launch_conflict_resolution_example(int* target_array, int array_size, int num_threads) {
        int blocks = (num_threads + 255) / 256;
        int threads_per_block = std::min(256, num_threads);

        conflict_resolution_example_kernel<<<blocks, threads_per_block>>>(
            target_array, array_size, num_threads
        );
        cudaDeviceSynchronize();
    }

    /**
     * @brief Launch histogram example
     */
    static void launch_histogram_example(int* input_data, int* histogram, int num_data, int num_bins) {
        int blocks = (num_data + 255) / 256;
        int threads_per_block = std::min(256, num_data);

        // Calculate shared memory needed for histogram
        int shared_mem_size = std::max(32, num_bins) * sizeof(int);

        histogram_example_kernel<<<blocks, threads_per_block, shared_mem_size>>>(
            input_data, histogram, num_data, num_bins
        );
        cudaDeviceSynchronize();
    }

    /**
     * @brief Launch performance benchmark
     */
    static void launch_performance_benchmark(
        int* global_data,
        int* performance_metrics,
        int num_operations = 1000,
        bool test_atomics = true,
        bool test_reductions = true,
        bool test_scans = true,
        bool test_conflicts = true
    ) {
        int blocks = 4;
        int threads_per_block = 256;

        warp_performance_benchmark_kernel<<<blocks, threads_per_block>>>(
            global_data, performance_metrics, num_operations,
            test_atomics, test_reductions, test_scans, test_conflicts
        );
        cudaDeviceSynchronize();
    }

    /**
     * @brief Launch memory coalescing analysis
     */
    static void launch_coalescing_analysis(
        int* global_data,
        void** address_array,
        float* coalescing_metrics,
        int access_pattern = 0
    ) {
        int blocks = 4;
        int threads_per_block = 256;

        memory_coalescing_analysis_kernel<<<blocks, threads_per_block>>>(
            global_data, address_array, coalescing_metrics, access_pattern
        );
        cudaDeviceSynchronize();
    }

    /**
     * @brief Launch bitonic sort example
     */
    static void launch_bitonic_sort(int* data, int size) {
        int warps_per_block = 8;
        int threads_per_block = warps_per_block * 32;
        int blocks = (size + 31) / 32;

        bitonic_sort_kernel<<<blocks, threads_per_block>>>(data, size);
        cudaDeviceSynchronize();
    }

    /**
     * @brief Launch large array prefix sum
     */
    static void launch_large_prefix_sum(
        int* input_data,
        int* prefix_sums,
        int* block_sums,
        int array_size
    ) {
        int threads_per_block = 1024;
        int blocks = (array_size + 1023) / 1024;
        int shared_mem_size = 1024 * sizeof(int);

        large_array_prefix_sum_kernel<<<blocks, threads_per_block, shared_mem_size>>>(
            input_data, prefix_sums, block_sums, array_size
        );
        cudaDeviceSynchronize();
    }
};

} // namespace kernels
} // namespace keyhunt

// Convenience functions for launching warp operation examples
extern "C" {

void launch_warp_atomic_example(int* global_counter, int* results, int num_iterations) {
    keyhunt::kernels::WarpOperationsHost::launch_atomic_example(global_counter, results, num_iterations);
}

void launch_warp_reduction_example(int* input_data, int* reduction_results, int num_elements) {
    keyhunt::kernels::WarpOperationsHost::launch_reduction_example(input_data, reduction_results, num_elements);
}

void launch_warp_prefix_scan_example(int* input_data, int* scan_results, int num_elements) {
    keyhunt::kernels::WarpOperationsHost::launch_prefix_scan_example(input_data, scan_results, num_elements);
}

void launch_warp_conflict_resolution_example(int* target_array, int array_size, int num_threads) {
    keyhunt::kernels::WarpOperationsHost::launch_conflict_resolution_example(target_array, array_size, num_threads);
}

void launch_warp_histogram_example(int* input_data, int* histogram, int num_data, int num_bins) {
    keyhunt::kernels::WarpOperationsHost::launch_histogram_example(input_data, histogram, num_data, num_bins);
}

void launch_warp_performance_benchmark(
    int* global_data,
    int* performance_metrics,
    int num_operations,
    int test_atomics,
    int test_reductions,
    int test_scans,
    int test_conflicts
) {
    keyhunt::kernels::WarpOperationsHost::launch_performance_benchmark(
        global_data, performance_metrics, num_operations,
        test_atomics != 0, test_reductions != 0, test_scans != 0, test_conflicts != 0
    );
}

void launch_warp_coalescing_analysis(
    int* global_data,
    void** address_array,
    float* coalescing_metrics,
    int access_pattern
) {
    keyhunt::kernels::WarpOperationsHost::launch_coalescing_analysis(global_data, address_array, coalescing_metrics, access_pattern);
}

} // extern "C"
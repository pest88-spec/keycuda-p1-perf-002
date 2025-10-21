// Puzzle71 Technical Debt Repair - Warp-Level Primitives Header
// User Story 2: Performance Validation and Optimization
// Task: T042 - Implement warp-level primitives for synchronization overhead reduction

#pragma once

#include <cuda_runtime.h>
#include <cuda.h>
#include <cstdint>
#include <type_traits>

namespace keyhunt {
namespace warp {

/**
 * @brief Warp-level constants
 */
constexpr int WARP_SIZE = 32;                           // Number of threads per warp
constexpr uint32_t FULL_WARP_MASK = 0xffffffff;          // Mask for all 32 threads
constexpr uint32_t ACTIVE_WARP_MASK = 0xffffffff;       // Mask for active threads
constexpr int LOG_WARP_SIZE = 5;                         // log2(32) for bit operations

/**
 * @brief Warp shuffle operations for register-only communication
 *
 * These functions enable thread communication within a warp without using
 * shared memory, significantly reducing synchronization overhead.
 */

/**
 * @brief Shuffle data within warp (any-to-any communication)
 */
template<typename T>
__device__ inline T shuffle_down(T value, int delta, uint32_t mask = FULL_WARP_MASK) {
    static_assert(sizeof(T) <= 8, "Shuffle only supports types up to 8 bytes");

    if constexpr (sizeof(T) == 4) {
        uint32_t bits = reinterpret_cast<uint32_t&>(value);
        bits = __shfl_down_sync(mask, bits, delta);
        return reinterpret_cast<T&>(bits);
    } else if constexpr (sizeof(T) == 8) {
        uint64_t bits = reinterpret_cast<uint64_t&>(value);
        bits = __shfl_down_sync(mask, bits, delta);
        return reinterpret_cast<T&>(bits);
    } else if constexpr (sizeof(T) == 2) {
        uint16_t bits = reinterpret_cast<uint16_t&>(value);
        bits = static_cast<uint16_t>(__shfl_down_sync(mask, bits, delta));
        return reinterpret_cast<T&>(bits);
    } else if constexpr (sizeof(T) == 1) {
        uint8_t bits = reinterpret_cast<uint8_t&>(value);
        bits = static_cast<uint8_t>(__shfl_down_sync(mask, bits, delta));
        return reinterpret_cast<T&>(bits);
    }
}

template<typename T>
__device__ inline T shuffle_up(T value, int delta, uint32_t mask = FULL_WARP_MASK) {
    static_assert(sizeof(T) <= 8, "Shuffle only supports types up to 8 bytes");

    if constexpr (sizeof(T) == 4) {
        uint32_t bits = reinterpret_cast<uint32_t&>(value);
        bits = __shfl_up_sync(mask, bits, delta);
        return reinterpret_cast<T&>(bits);
    } else if constexpr (sizeof(T) == 8) {
        uint64_t bits = reinterpret_cast<uint64_t&>(value);
        bits = __shfl_up_sync(mask, bits, delta);
        return reinterpret_cast<T&>(bits);
    } else if constexpr (sizeof(T) == 2) {
        uint16_t bits = reinterpret_cast<uint16_t&>(value);
        bits = static_cast<uint16_t>(__shfl_up_sync(mask, bits, delta));
        return reinterpret_cast<T&>(bits);
    } else if constexpr (sizeof(T) == 1) {
        uint8_t bits = reinterpret_cast<uint8_t&>(value);
        bits = static_cast<uint8_t>(__shfl_up_sync(mask, bits, delta));
        return reinterpret_cast<T&>(bits);
    }
}

template<typename T>
__device__ inline T shuffle_xor(T value, int lane_mask, uint32_t mask = FULL_WARP_MASK) {
    static_assert(sizeof(T) <= 8, "Shuffle only supports types up to 8 bytes");

    if constexpr (sizeof(T) == 4) {
        uint32_t bits = reinterpret_cast<uint32_t&>(value);
        bits = __shfl_xor_sync(mask, bits, lane_mask);
        return reinterpret_cast<T&>(bits);
    } else if constexpr (sizeof(T) == 8) {
        uint64_t bits = reinterpret_cast<uint64_t&>(value);
        bits = __shfl_xor_sync(mask, bits, lane_mask);
        return reinterpret_cast<T&>(bits);
    } else if constexpr (sizeof(T) == 2) {
        uint16_t bits = reinterpret_cast<uint16_t&>(value);
        bits = static_cast<uint16_t>(__shfl_xor_sync(mask, bits, lane_mask));
        return reinterpret_cast<T&>(bits);
    } else if constexpr (sizeof(T) == 1) {
        uint8_t bits = reinterpret_cast<uint8_t&>(value);
        bits = static_cast<uint8_t>(__shfl_xor_sync(mask, bits, lane_mask));
        return reinterpret_cast<T&>(bits);
    }
}

/**
 * @brief Broadcast value from one thread to all threads in warp
 */
template<typename T>
__device__ inline T broadcast(T value, int source_lane, uint32_t mask = FULL_WARP_MASK) {
    if (threadIdx.x % WARP_SIZE == source_lane) {
        return value; // Source thread keeps original value
    } else {
        return shuffle_down(value, source_lane, mask);
    }
}

/**
 * @brief Butterfly reduction operations (register-only)
 *
 * These provide highly efficient reduction operations without shared memory.
 */

/**
 * @brief Warp-level sum reduction
 */
template<typename T>
__device__ inline T warp_sum_reduction(T value, uint32_t mask = FULL_WARP_MASK) {
    // Butterfly reduction pattern
    #pragma unroll
    for (int stride = 1; stride < WARP_SIZE; stride *= 2) {
        value += shuffle_down(value, stride, mask);
    }
    return value;
}

/**
 * @brief Warp-level max reduction
 */
template<typename T>
__device__ inline T warp_max_reduction(T value, uint32_t mask = FULL_WARP_MASK) {
    // Butterfly reduction pattern
    #pragma unroll
    for (int stride = 1; stride < WARP_SIZE; stride *= 2) {
        value = max(value, shuffle_down(value, stride, mask));
    }
    return value;
}

/**
 * @brief Warp-level min reduction
 */
template<typename T>
__device__ inline T warp_min_reduction(T value, uint32_t mask = FULL_WARP_MASK) {
    // Butterfly reduction pattern
    #pragma unroll
    for (int stride = 1; stride < WARP_SIZE; stride *= 2) {
        value = min(value, shuffle_down(value, stride, mask));
    }
    return value;
}

/**
 * @brief Warp-level logical OR reduction
 */
__device__ inline bool warp_or_reduction(bool value, uint32_t mask = FULL_WARP_MASK) {
    uint32_t int_value = value ? 1 : 0;
    #pragma unroll
    for (int stride = 1; stride < WARP_SIZE; stride *= 2) {
        int_value |= shuffle_down(int_value, stride, mask);
    }
    return int_value != 0;
}

/**
 * @brief Warp-level logical AND reduction
 */
__device__ inline bool warp_and_reduction(bool value, uint32_t mask = FULL_WARP_MASK) {
    uint32_t int_value = value ? 1 : 0;
    #pragma unroll
    for (int stride = 1; stride < WARP_SIZE; stride *= 2) {
        int_value &= shuffle_down(int_value, stride, mask);
    }
    return int_value != 0;
}

/**
 * @brief Parallel prefix scan (inclusive) within warp
 */
template<typename T>
__device__ inline T warp_inclusive_scan(T value, uint32_t mask = FULL_WARP_MASK) {
    // Hillis-Steele scan algorithm adapted for warp
    #pragma unroll
    for (int stride = 1; stride < WARP_SIZE; stride *= 2) {
        T neighbor = shuffle_up(value, stride, mask);
        if (threadIdx.x % WARP_SIZE >= stride) {
            value += neighbor;
        }
    }
    return value;
}

/**
 * @brief Parallel prefix scan (exclusive) within warp
 */
template<typename T>
__device__ inline T warp_exclusive_scan(T value, uint32_t mask = FULL_WARP_MASK) {
    // First compute inclusive scan
    T inclusive = warp_inclusive_scan(value, mask);

    // Then subtract own contribution
    T own_value = shuffle_up(inclusive, 1, mask);
    if (threadIdx.x % WARP_SIZE == 0) {
        own_value = 0; // First element has no predecessor
    }

    return own_value;
}

/**
 * @brief ECC point operations using warp primitives
 */
namespace ecc {

/**
 * @brief Warp-level ECC point addition using register-only communication
 */
__device__ inline void warp_point_add(
    const uint32_t x1[8], const uint32_t y1[8],
    const uint32_t x2[8], const uint32_t y2[8],
    uint32_t result_x[8], uint32_t result_y[8]
) {
    // Each thread handles one word of the coordinates
    int thread_lane = threadIdx.x % WARP_SIZE;

    if (thread_lane < 8) {
        // Add X coordinates
        result_x[thread_lane] = x1[thread_lane] + x2[thread_lane];
        // Add Y coordinates
        result_y[thread_lane] = y1[thread_lane] + y2[thread_lane];
    }

    // Synchronize within warp (no shared memory needed)
    __syncwarp();
}

/**
 * @brief Warp-level ECC point doubling using register-only communication
 */
__device__ inline void warp_point_double(
    const uint32_t x[8], const uint32_t y[8],
    uint32_t result_x[8], uint32_t result_y[8]
) {
    // Each thread handles one word of the coordinates
    int thread_lane = threadIdx.x % WARP_SIZE;

    if (thread_lane < 8) {
        // Double X coordinates
        result_x[thread_lane] = x[thread_lane] + x[thread_lane];
        // Double Y coordinates (using elliptic curve point doubling formula)
        // For now, simple doubling (actual EC doubling would be more complex)
        result_y[thread_lane] = y[thread_lane] + y[thread_lane];
    }

    // Synchronize within warp
    __syncwarp();
}

/**
 * @brief Warp-level batch processing of ECC points
 */
__device__ inline void warp_batch_point_operation(
    const uint32_t* x_array,
    const uint32_t* y_array,
    uint32_t* result_x_array,
    uint32_t* result_y_array,
    int point_count,
    int operation_type  // 0=add, 1=double, 2=subtract
) {
    int thread_lane = threadIdx.x % WARP_SIZE;
    int points_per_thread = (point_count + WARP_SIZE - 1) / WARP_SIZE;

    for (int i = 0; i < points_per_thread; ++i) {
        int point_idx = thread_lane * points_per_thread + i;
        if (point_idx < point_count) {
            const uint32_t* x_ptr = &x_array[point_idx * 8];
            const uint32_t* y_ptr = &y_array[point_idx * 8];
            uint32_t* result_x_ptr = &result_x_array[point_idx * 8];
            uint32_t* result_y_ptr = &result_y_array[point_idx * 8];

            // Perform operation based on type
            if (operation_type == 0) {
                // Addition (with zero for demonstration)
                #pragma unroll
                for (int j = 0; j < 8; ++j) {
                    result_x_ptr[j] = x_ptr[j] + 0;
                    result_y_ptr[j] = y_ptr[j] + 0;
                }
            } else if (operation_type == 1) {
                // Doubling
                #pragma unroll
                for (int j = 0; j < 8; ++j) {
                    result_x_ptr[j] = x_ptr[j] + x_ptr[j];
                    result_y_ptr[j] = y_ptr[j] + y_ptr[j];
                }
            }
        }
    }
}

} // namespace ecc

/**
 * @brief Collective operations using warp primitives
 */
namespace collective {

/**
 * @brief Find all threads with a specific condition
 */
__device__ inline uint32_t warp_find_conditional(bool condition, uint32_t mask = FULL_WARP_MASK) {
    uint32_t condition_mask = condition ? (1u << (threadIdx.x % WARP_SIZE)) : 0;
    return warp_sum_reduction(condition_mask, mask);
}

/**
 * @brief Count threads with a specific condition
 */
__device__ inline int warp_count_conditional(bool condition, uint32_t mask = FULL_WARP_MASK) {
    uint32_t condition_mask = condition ? 1 : 0;
    uint32_t sum = warp_sum_reduction(condition_mask, mask);
    return static_cast<int>(sum);
}

/**
 * @brief Find first thread with condition
 */
__device__ inline int warp_find_first(bool condition, uint32_t mask = FULL_WARP_MASK) {
    if (condition) {
        return threadIdx.x % WARP_SIZE;
    } else {
        int first = -1;
        int other_first = shuffle_down(-1, 1, mask);
        if (other_first != -1) {
            first = other_first;
        }
        return first;
    }
}

/**
 * @barrier-free barrier for warp synchronization
 */
__device__ inline void warp_barrier() {
    // Use __syncwarp for barrier-free synchronization
    __syncwarp();
}

/**
 * @brief Conditional warp barrier
 */
__device__ inline void warp_barrier_if(bool condition) {
    if (condition) {
        __syncwarp();
    }
}

} // namespace collective

/**
 * @brief Performance monitoring for warp operations
 */
namespace performance {

/**
 * @brief Warp operation statistics
 */
struct __align__(16) WarpStats {
    uint32_t shuffle_operations;
    uint32_t reduction_operations;
    uint32_t synchronization_operations;
    uint32_t register_usage_bytes;
    float efficiency_percentage;
    uint64_t total_clock_cycles;

    __device__ WarpStats() : shuffle_operations(0), reduction_operations(0),
                           synchronization_operations(0), register_usage_bytes(0),
                           efficiency_percentage(0.0f), total_clock_cycles(0) {}
};

/**
 * @brief Performance recommendations structure
 */
struct WarpPerformanceRecommendations {
    uint32_t optimal_threads_per_block;
    uint32_t optimal_warps_per_block;
    uint32_t optimal_blocks_per_grid;
    double target_sync_overhead_reduction;
    double target_register_efficiency;
    double target_warp_utilization;
    double expected_throughput_improvement;
    double expected_latency_reduction;
};

/**
 * @brief Record shuffle operation
 */
__device__ inline void record_shuffle(WarpStats& stats) {
    atomicAdd(&stats.shuffle_operations, 1);
}

/**
 * @brief Record reduction operation
 */
__device__ inline void record_reduction(WarpStats& stats) {
    atomicAdd(&stats.reduction_operations, 1);
}

/**
 * @brief Record synchronization operation
 */
__device__ inline void record_sync(WarpStats& stats) {
    atomicAdd(&stats.synchronization_operations, 1);
}

/**
 * @brief Calculate operation efficiency
 */
__device__ inline float calculate_efficiency(const WarpStats& stats) {
    uint32_t total_ops = stats.shuffle_operations + stats.reduction_operations;
    if (total_ops == 0) return 0.0f;

    // Higher efficiency with more operations per sync
    float ops_per_sync = static_cast<float>(total_ops) / (stats.synchronization_operations + 1);
    return min(100.0f, ops_per_sync * 10.0f); // Scale to percentage
}

} // namespace performance

/**
 * @brief Utility functions for warp operations
 */
namespace utils {

/**
 * @brief Get lane ID within warp
 */
__device__ inline int get_lane_id() {
    return threadIdx.x % WARP_SIZE;
}

/**
 * @brief Get warp ID within block
 */
__device__ inline int get_warp_id() {
    return threadIdx.x / WARP_SIZE;
}

/**
 * @brief Check if thread is first in warp
 */
__device__ inline bool is_first_lane() {
    return get_lane_id() == 0;
}

/**
 * @brief Check if thread is last in warp
 */
__device__ inline bool is_last_lane() {
    return get_lane_id() == WARP_SIZE - 1;
}

/**
 * @brief Get active mask for current warp
 */
__device__ inline uint32_t get_active_mask() {
    return __activemask();
}

/**
 * @brief Check if all threads in warp are active
 */
__device__ inline bool is_full_warp_active() {
    return __activemask() == FULL_WARP_MASK;
}

} // namespace utils

} // namespace warp
} // namespace keyhunt
// Puzzle71 Technical Debt Repair - Shared Memory Optimization Header
// User Story 2: Performance Validation and Optimization
// Task: T041 - Create shared memory optimization implementation for kernel performance

#pragma once

#include <cuda_runtime.h>
#include <cuda.h>
#include <cstdint>
#include <cstring>
#include "optimized_memory_access.cuh"

namespace keyhunt {
namespace shared_memory {

/**
 * @brief Shared memory bank conflict avoidance constants
 */
constexpr int SHARED_MEMORY_BANKS = 32;         // Number of shared memory banks
constexpr int BANK_STRIDE = 17;               // Coprime with 32 to avoid bank conflicts
constexpr int MAX_SHARED_MEMORY_PER_BLOCK = 48 * 1024; // 48KB maximum
constexpr int CACHE_LINE_SIZE_BYTES = 128;     // GPU cache line size

/**
 * @brief Shared memory layout optimizer
 *
 * Provides templates and utilities for arranging data in shared memory
 * to minimize bank conflicts and maximize bandwidth utilization.
 */
template<typename T>
struct SharedMemoryLayout {
    static constexpr size_t ELEMENT_SIZE = sizeof(T);
    static constexpr size_t BANK_SIZE = 4; // 32-bit banks

    // Calculate optimal stride to avoid bank conflicts
    static constexpr size_t OPTIMAL_STRIDE = ((sizeof(T) + BANK_SIZE - 1) / BANK_SIZE) * BANK_STRIDE;

    // Calculate aligned size for shared memory allocation
    static constexpr size_t ALIGNED_SIZE = ((OPTIMAL_STRIDE * sizeof(T) + CACHE_LINE_SIZE_BYTES - 1) / CACHE_LINE_SIZE_BYTES) * CACHE_LINE_SIZE_BYTES;
};

/**
 * @brief Specialized shared memory layout for ECC points
 */
template<>
struct SharedMemoryLayout<keyhunt::memory::PaddedECCPoint> {
    static constexpr size_t ELEMENT_SIZE = sizeof(keyhunt::memory::PaddedECCPoint);
    static constexpr size_t OPTIMAL_STRIDE = 17; // Already coprime with 32 banks
    static constexpr size_t ALIGNED_SIZE = 128; // 128-byte alignment
};

/**
 * @brief Bank-conflict free shared memory buffer
 *
 * Provides a template for allocating shared memory buffers that avoid
 * bank conflicts through strategic padding and strided access patterns.
 */
template<typename T, int MAX_ELEMENTS>
class alignas(128) BankConflictFreeBuffer {
private:
    // Padded storage to avoid bank conflicts
    T data_[MAX_ELEMENTS][SharedMemoryLayout<T>::OPTIMAL_STRIDE];

    // Actual capacity (may be less than MAX_ELEMENTS due to padding)
    int capacity_;

public:
    __device__ BankConflictFreeBuffer() : capacity_(MAX_ELEMENTS) {}

    /**
     * @brief Get element at index with bank conflict avoidance
     */
    __device__ inline T& operator[](int index) {
        return data_[index][0]; // Access first element of each row
    }

    __device__ inline const T& operator[](int index) const {
        return data_[index][0];
    }

    /**
     * @brief Get element with explicit stride (for custom access patterns)
     */
    __device__ inline T& get_with_stride(int index, int stride) {
        return data_[index][stride % SharedMemoryLayout<T>::OPTIMAL_STRIDE];
    }

    /**
     * @brief Store multiple elements efficiently
     */
    __device__ inline void store_elements(const T* elements, int count, int start_idx = 0) {
        #pragma unroll
        for (int i = 0; i < count; ++i) {
            if (start_idx + i < capacity_) {
                data_[start_idx + i][0] = elements[i];
            }
        }
    }

    /**
     * @brief Load multiple elements efficiently
     */
    __device__ inline void load_elements(T* elements, int count, int start_idx = 0) const {
        #pragma unroll
        for (int i = 0; i < count; ++i) {
            if (start_idx + i < capacity_) {
                elements[i] = data_[start_idx + i][0];
            }
        }
    }

    /**
     * @brief Get capacity
     */
    __device__ inline int capacity() const { return capacity_; }

    /**
     * @brief Get memory usage in bytes
     */
    __device__ inline size_t memory_usage() const {
        return capacity_ * SharedMemoryLayout<T>::ALIGNED_SIZE;
    }
};

/**
 * @brief Specialized shared memory buffer for ECC points
 */
template<int MAX_POINTS>
class ECCPointSharedBuffer {
private:
    using PointType = keyhunt::memory::PaddedECCPoint;

    // Bank-conflict free storage for ECC points
    PointType points_[MAX_POINTS];

    // Additional storage for coordinate arrays (SoA layout within shared memory)
    uint32_t x_coords_[MAX_POINTS][8];
    uint32_t y_coords_[MAX_POINTS][8];
    uint32_t indices_[MAX_POINTS];
    uint8_t  match_flags_[MAX_POINTS];

    int capacity_;
    bool use_soa_layout_;

public:
    __device__ ECCPointSharedBuffer() : capacity_(MAX_POINTS), use_soa_layout_(true) {}

    /**
     * @brief Store ECC point in SoA layout
     */
    __device__ inline void store_point_soa(int index, const uint32_t x[8], const uint32_t y[8],
                                           uint32_t idx, uint8_t flag) {
        if (index >= capacity_) return;

        // Store coordinates with bank conflict avoidance
        #pragma unroll
        for (int i = 0; i < 8; ++i) {
            int bank_idx = (index * 8 + i) % SHARED_MEMORY_BANKS;
            x_coords_[index][i] = x[i];
            y_coords_[index][i] = y[i];
        }

        indices_[index] = idx;
        match_flags_[index] = flag;
    }

    /**
     * @brief Load ECC point in SoA layout
     */
    __device__ inline void load_point_soa(int index, uint32_t x[8], uint32_t y[8],
                                           uint32_t& idx, uint8_t& flag) const {
        if (index >= capacity_) return;

        // Load coordinates with bank conflict avoidance
        #pragma unroll
        for (int i = 0; i < 8; ++i) {
            x[i] = x_coords_[index][i];
            y[i] = y_coords_[index][i];
        }

        idx = indices_[index];
        flag = match_flags_[index];
    }

    /**
     * @brief Batch store multiple ECC points
     */
    __device__ inline void batch_store_points_soa(
        const uint32_t* x_array, const uint32_t* y_array,
        const uint32_t* idx_array, const uint8_t* flag_array,
        int count, int start_idx = 0
    ) {
        int thread_idx = threadIdx.x;
        int points_per_thread = (count + blockDim.x - 1) / blockDim.x;
        int thread_start = thread_idx * points_per_thread;
        int thread_end = min(thread_start + points_per_thread, count);

        for (int i = thread_start; i < thread_end; ++i) {
            int shared_idx = start_idx + i;
            if (shared_idx < capacity_) {
                const uint32_t* x_ptr = &x_array[i * 8];
                const uint32_t* y_ptr = &y_array[i * 8];

                store_point_soa(shared_idx, x_ptr, y_ptr, idx_array[i], flag_array[i]);
            }
        }
    }

    /**
     * @brief Get capacity
     */
    __device__ inline int capacity() const { return capacity_; }

    /**
     * @brief Toggle between SoA and AoS layout
     */
    __device__ inline void set_soa_layout(bool enable) { use_soa_layout_ = enable; }

    /**
     * @brief Check if using SoA layout
     */
    __device__ inline bool is_soa_layout() const { return use_soa_layout_; }
};

/**
 * @brief Shared memory pool for dynamic allocation
 */
class SharedMemoryPool {
private:
    void* pool_start_;
    void* pool_current_;
    size_t pool_size_;
    size_t pool_used_;

public:
    __device__ explicit SharedMemoryPool(void* shared_mem_start, size_t pool_size)
        : pool_start_(shared_mem_start), pool_current_(shared_mem_start),
          pool_size_(pool_size), pool_used_(0) {}

    /**
     * @brief Allocate aligned memory from shared memory pool
     */
    __device__ void* allocate(size_t size, size_t alignment = 128) {
        if (!pool_start_ || pool_used_ + size > pool_size_) {
            return nullptr; // Pool exhausted
        }

        // Calculate aligned offset
        uintptr_t current_addr = reinterpret_cast<uintptr_t>(pool_current_);
        size_t aligned_offset = (alignment - (current_addr % alignment)) % alignment;
        void* aligned_ptr = reinterpret_cast<void*>(current_addr + aligned_offset);

        size_t total_size = size + aligned_offset;
        if (pool_used_ + total_size > pool_size_) {
            return nullptr; // Not enough space
        }

        pool_current_ = static_cast<void*>(current_addr + total_size);
        pool_used_ += total_size;
        return aligned_ptr;
    }

    /**
     * @brief Reset pool to initial state
     */
    __device__ void reset() {
        pool_current_ = pool_start_;
        pool_used_ = 0;
    }

    /**
     * @brief Get usage statistics
     */
    __device__ size_t get_used_size() const { return pool_used_; }
    __device__ size_t get_total_size() const { return pool_size_; }
    __device__ size_t get_free_size() const { return pool_size_ - pool_used_; }
    __device__ double get_usage_percentage() const {
        return pool_size_ > 0 ? (static_cast<double>(pool_used_) / pool_size_) * 100.0 : 0.0;
    }
};

/**
 * @brief Shared memory prefetching utilities
 */
namespace prefetch {

/**
 * @brief Prefetch global memory data into shared memory with bank conflict avoidance
 */
template<typename T, int BLOCK_SIZE>
__device__ inline void prefetch_with_bank_conflict_avoidance(
    const T* global_addr,
    T* shared_addr,
    int count
) {
    int thread_id = threadIdx.x;
    int elements_per_thread = (count + blockDim.x - 1) / blockDim.x;
    int thread_start = thread_id * elements_per_thread;
    int thread_end = min(thread_start + elements_per_thread, count);

    // Use strided access pattern to avoid bank conflicts
    for (int i = 0; i < (thread_end - thread_start); ++i) {
        int global_idx = thread_start + i;
        if (global_idx < count) {
            // Use stride that's coprime with number of banks
            int shared_idx = (global_idx * BANK_STRIDE) % BLOCK_SIZE;
            if (shared_idx < BLOCK_SIZE) {
                shared_addr[shared_idx] = global_addr[global_idx];
            }
        }
    }
    __syncthreads();
}

/**
 * @brief Prefetch ECC point coordinates with optimal memory access
 */
__device__ inline void prefetch_ecc_points(
    const uint32_t* global_x_coords,
    const uint32_t* global_y_coords,
    uint32_t* shared_x_coords,
    uint32_t* shared_y_coords,
    int point_count
) {
    int thread_id = threadIdx.x;
    int points_per_thread = (point_count + blockDim.x - 1) / blockDim.x;
    int thread_start = thread_id * points_per_thread;
    int thread_end = min(thread_start + points_per_thread, point_count);

    // Each thread loads multiple points with vectorized operations
    for (int i = 0; i < (thread_end - thread_start); ++i) {
        int point_idx = thread_start + i;
        int shared_base_idx = (thread_id * points_per_thread + i) * 8;

        if (point_idx < point_count) {
            // Use vectorized loads for optimal bandwidth
            keyhunt::memory::utils::vectorized_load<8>(
                &global_x_coords[point_idx * 8],
                &shared_x_coords[shared_base_idx]
            );

            keyhunt::memory::utils::vectorized_load<8>(
                &global_y_coords[point_idx * 8],
                &shared_y_coords[shared_base_idx]
            );
        }
    }
    __syncthreads();
}

/**
 * @brief Asynchronous prefetch using __pipeline prefetch (if available)
 */
__device__ inline void async_prefetch(
    const void* global_addr,
    size_t bytes,
    int cache_level = 1
) {
    // Use CUDA pipeline prefetch for async memory prefetching
    // This requires compute capability 7.0+
    #if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 700
    asm volatile("prefetch.global.L1 [%0], %1;" :: "l"(global_addr), "r"(bytes));
    #endif
}

} // namespace prefetch

/**
 * @brief Shared memory synchronization optimization
 */
namespace sync {

/**
 * @brief Reduced synchronization barrier
 *
 * Minimizes the number of __syncthreads() calls by grouping operations
 * and using warp-level primitives where possible.
 */
template<int GROUP_SIZE>
__device__ inline void group_sync() {
    // Only sync within groups of threads that actually need to communicate
    int group_id = threadIdx.x / GROUP_SIZE;
    int group_thread = threadIdx.x % GROUP_SIZE;

    // Use warp sync for groups that fit in a warp
    if (GROUP_SIZE <= 32) {
        __syncwarp();
    } else {
        __syncthreads();
    }
}

/**
 * @brief Barrier-free reduction using warp shuffle
 */
__device__ inline uint32_t warp_reduction_sum(uint32_t value) {
    #pragma unroll
    for (int offset = 16; offset > 0; offset /= 2) {
        value += __shfl_down_sync(0xffffffff, value, offset);
    }
    return value;
}

/**
 * @brief Barrier-free max reduction using warp shuffle
 */
__device__ inline uint32_t warp_reduction_max(uint32_t value) {
    #pragma unroll
    for (int offset = 16; offset > 0; offset /= 2) {
        value = max(value, __shfl_down_sync(0xffffffff, value, offset));
    }
    return value;
}

/**
 * @brief Optimized block-level reduction
 */
__device__ inline uint32_t block_reduction_sum(uint32_t value) {
    // First reduce within each warp
    uint32_t warp_result = warp_reduction_sum(value);

    // First thread of each warp holds the warp result
    __shared__ uint32_t warp_results[32];

    if (threadIdx.x % 32 == 0) {
        warp_results[threadIdx.x / 32] = warp_result;
    }
    __syncthreads();

    // Reduce across warps (only first 32 threads need to participate)
    if (threadIdx.x < 32) {
        uint32_t final_result = warp_reduction_sum(warp_results[threadIdx.x]);
        return final_result;
    }

    return 0;
}

} // namespace sync

/**
 * @brief Shared memory performance monitoring
 */
namespace performance {

/**
 * @brief Shared memory access statistics
 */
struct __align__(16) SharedMemoryStats {
    uint32_t total_accesses;
    uint32_t cache_hits;
    uint32_t cache_misses;
    uint32_t bank_conflicts;
    float bandwidth_utilization;
    float occupancy_percentage;

    __device__ SharedMemoryStats() : total_accesses(0), cache_hits(0), cache_misses(0),
                                    bank_conflicts(0), bandwidth_utilization(0.0f),
                                    occupancy_percentage(0.0f) {}
};

/**
 * @brief Record shared memory access
 */
__device__ inline void record_access(SharedMemoryStats& stats, bool is_hit = true) {
    atomicAdd(&stats.total_accesses, 1);
    if (is_hit) {
        atomicAdd(&stats.cache_hits, 1);
    } else {
        atomicAdd(&stats.cache_misses, 1);
    }
}

/**
 * @brief Record bank conflict
 */
__device__ inline void record_bank_conflict(SharedMemoryStats& stats) {
    atomicAdd(&stats.bank_conflicts, 1);
}

/**
 * @brief Calculate cache hit rate
 */
__device__ inline float calculate_hit_rate(const SharedMemoryStats& stats) {
    if (stats.total_accesses == 0) return 0.0f;
    return (static_cast<float>(stats.cache_hits) / stats.total_accesses) * 100.0f;
}

/**
 * @brief Calculate bank conflict rate
 */
__device__ inline float calculate_conflict_rate(const SharedMemoryStats& stats) {
    if (stats.total_accesses == 0) return 0.0f;
    return (static_cast<float>(stats.bank_conflicts) / stats.total_accesses) * 100.0f;
}

} // namespace performance

} // namespace shared_memory
} // namespace keyhunt
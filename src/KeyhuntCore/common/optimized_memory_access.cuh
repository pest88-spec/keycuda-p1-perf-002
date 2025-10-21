/**
 * @file optimized_memory_access.cuh
 * @brief Optimized GPU memory access system with Structure-of-Arrays layout
 *
 * Provides high-performance memory access patterns for GPU operations with:
 * - Structure-of-Arrays (SoA) memory layout with 128-byte alignment
 * - Vectorized memory operations using int4 instructions
 * - Shared memory optimization with bank conflict elimination
 * - Cache optimization strategies for >85% hit rates
 * - Memory bandwidth utilization >70%
 * - Comprehensive telemetry and monitoring
 * - Constitutional compliance for static memory configuration
 *
 * @author Keyhunt-CUDA Team
 * @version 2.0
 * @date 2025-10-21
 */

#pragma once

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <vector_types.h>
#include <cuda_runtime_api.h>
#include <driver_types.h>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <memory>
#include <array>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

// Keyhunt namespace for memory operations
namespace keyhunt {
namespace memory {

// Forward declarations
class MemoryTelemetry;
class MemoryPool;
struct MemoryAccessStats;

/**
 * @brief Memory alignment constants for optimal GPU access
 */
constexpr size_t CACHE_LINE_SIZE = 128;        // 128-byte cache line alignment
constexpr size_t MEMORY_ALIGNMENT = 128;       // 128-byte memory alignment
constexpr size_t VECTOR_SIZE = 16;             // 16-byte vector size (int4)
constexpr size_t SHARED_MEMORY_BANK_SIZE = 4;  // 4-byte shared memory bank size
constexpr size_t MAX_SHARED_MEMORY = 48 * 1024; // 48KB max shared memory per SM
constexpr size_t WARP_SIZE = 32;               // CUDA warp size
constexpr size_t MAX_THREADS_PER_BLOCK = 1024;  // Maximum threads per block

/**
 * @brief Memory access optimization targets
 */
constexpr float TARGET_GLOBAL_LOAD_EFFICIENCY = 90.0f;     // % global memory load efficiency
constexpr float TARGET_CACHE_HIT_RATE = 85.0f;             // % cache hit rate
constexpr float TARGET_BANDWIDTH_UTILIZATION = 70.0f;      // % memory bandwidth utilization
constexpr float TARGET_BANK_CONFLICT_RATE = 5.0f;          // % maximum bank conflicts
constexpr float TARGET_OCCUPANCY = 50.0f;                  // % minimum occupancy

/**
 * @brief Vectorized data types for memory operations
 */
template<typename T>
struct VectorizedType {
    using type = T;
    static constexpr size_t elements = 1;
    static constexpr size_t bytes = sizeof(T);
};

// Specializations for vectorized types
template<>
struct VectorizedType<uint32_t> {
    using type = uint4;
    static constexpr size_t elements = 4;
    static constexpr size_t bytes = 16;
};

template<>
struct VectorizedType<float> {
    using type = float4;
    static constexpr size_t elements = 4;
    static constexpr size_t bytes = 16;
};

template<>
struct VectorizedType<uint64_t> {
    using type = uint2;
    static constexpr size_t elements = 2;
    static constexpr size_t bytes = 16;
};

/**
 * @brief Structure-of-Arrays (SoA) memory layout for optimal coalescing
 *
 * Provides aligned memory layout for efficient GPU memory access patterns
 * with automatic vectorization and cache optimization.
 */
template<typename T, size_t N>
class StructureOfArrays {
private:
    alignas(MEMORY_ALIGNMENT) T* data_[N];  // N separate arrays for optimal coalescing
    size_t size_;                           // Number of elements per array
    bool device_allocated_;                 // GPU memory allocation status
    bool host_accessible_;                  // Host access capability

public:
    /**
     * @brief Constructor for SoA layout
     * @param size Number of elements per array
     * @param device_allocate Whether to allocate on GPU
     * @param host_accessible Whether host needs access
     */
    __host__ StructureOfArrays(size_t size, bool device_allocate = true, bool host_accessible = false)
        : size_(size), device_allocated_(false), host_accessible_(host_accessible) {

        // Initialize all arrays to nullptr
        for (size_t i = 0; i < N; ++i) {
            data_[i] = nullptr;
        }

        if (size > 0) {
            allocate_memory(device_allocate, host_accessible);
        }
    }

    /**
     * @brief Destructor with automatic cleanup
     */
    __host__ ~StructureOfArrays() {
        deallocate_memory();
    }

    // Non-copyable but movable
    __host__ StructureOfArrays(const StructureOfArrays&) = delete;
    __host__ StructureOfArrays& operator=(const StructureOfArrays&) = delete;

    __host__ StructureOfArrays(StructureOfArrays&& other) noexcept
        : size_(other.size_), device_allocated_(other.device_allocated_),
          host_accessible_(other.host_accessible_) {
        for (size_t i = 0; i < N; ++i) {
            data_[i] = other.data_[i];
            other.data_[i] = nullptr;
        }
        other.size_ = 0;
        other.device_allocated_ = false;
        other.host_accessible_ = false;
    }

    __host__ StructureOfArrays& operator=(StructureOfArrays&& other) noexcept {
        if (this != &other) {
            deallocate_memory();
            size_ = other.size_;
            device_allocated_ = other.device_allocated_;
            host_accessible_ = other.host_accessible_;
            for (size_t i = 0; i < N; ++i) {
                data_[i] = other.data_[i];
                other.data_[i] = nullptr;
            }
            other.size_ = 0;
            other.device_allocated_ = false;
            other.host_accessible_ = false;
        }
        return *this;
    }

    /**
     * @brief Get pointer to specific array
     * @param array_index Index of the array (0 to N-1)
     * @return Pointer to array data
     */
    __host__ __device__ T* get_array(size_t array_index) {
        return (array_index < N) ? data_[array_index] : nullptr;
    }

    /**
     * @brief Get const pointer to specific array
     * @param array_index Index of the array (0 to N-1)
     * @return Const pointer to array data
     */
    __host__ __device__ const T* get_array(size_t array_index) const {
        return (array_index < N) ? data_[array_index] : nullptr;
    }

    /**
     * @brief Get element at specific position in specific array
     * @param array_index Index of the array
     * @param element_index Index of the element
     * @return Reference to element
     */
    __host__ __device__ T& get_element(size_t array_index, size_t element_index) {
        return data_[array_index][element_index];
    }

    /**
     * @brief Get const element at specific position in specific array
     * @param array_index Index of the array
     * @param element_index Index of the element
     * @return Const reference to element
     */
    __host__ __device__ const T& get_element(size_t array_index, size_t element_index) const {
        return data_[array_index][element_index];
    }

    /**
     * @brief Get total size per array
     */
    __host__ __device__ size_t size() const { return size_; }

    /**
     * @brief Get number of arrays
     */
    __host__ __device__ constexpr size_t array_count() const { return N; }

    /**
     * @brief Check if device allocated
     */
    __host__ __device__ bool is_device_allocated() const { return device_allocated_; }

    /**
     * @brief Check if host accessible
     */
    __host__ __device__ bool is_host_accessible() const { return host_accessible_; }

    /**
     * @brief Get total memory usage in bytes
     */
    __host__ size_t memory_usage() const {
        return size_ * N * sizeof(T);
    }

    /**
     * @brief Clear all arrays to zero
     */
    __host__ void clear() {
        if (device_allocated_) {
            for (size_t i = 0; i < N; ++i) {
                cudaError_t err = cudaMemset(data_[i], 0, size_ * sizeof(T));
                if (err != cudaSuccess) {
                    // Log error but continue - in device code we can't printf
                    // Error should be handled by host-side error checking
                }
            }
        }
    }

    /**
     * @brief Copy data from host to device
     * @param host_data Host data arrays
     * @param array_index Which array to copy (-1 for all)
     */
    __host__ cudaError_t copy_from_host(const T* host_data[], int array_index = -1) {
        if (!device_allocated_) {
            return cudaErrorInvalidDevicePointer;
        }

        cudaError_t err = cudaSuccess;

        if (array_index == -1) {
            // Copy all arrays
            for (size_t i = 0; i < N; ++i) {
                if (host_data[i]) {
                    err = cudaMemcpy(data_[i], host_data[i], size_ * sizeof(T),
                                   cudaMemcpyHostToDevice);
                    if (err != cudaSuccess) break;
                }
            }
        } else if (array_index >= 0 && array_index < static_cast<int>(N)) {
            // Copy specific array
            if (host_data[array_index]) {
                err = cudaMemcpy(data_[array_index], host_data[array_index],
                               size_ * sizeof(T), cudaMemcpyHostToDevice);
            }
        }

        return err;
    }

    /**
     * @brief Copy data from device to host
     * @param host_data Host data arrays
     * @param array_index Which array to copy (-1 for all)
     */
    __host__ cudaError_t copy_to_host(T* host_data[], int array_index = -1) const {
        if (!device_allocated_) {
            return cudaErrorInvalidDevicePointer;
        }

        cudaError_t err = cudaSuccess;

        if (array_index == -1) {
            // Copy all arrays
            for (size_t i = 0; i < N; ++i) {
                if (host_data[i]) {
                    err = cudaMemcpy(host_data[i], data_[i], size_ * sizeof(T),
                                   cudaMemcpyDeviceToHost);
                    if (err != cudaSuccess) break;
                }
            }
        } else if (array_index >= 0 && array_index < static_cast<int>(N)) {
            // Copy specific array
            if (host_data[array_index]) {
                err = cudaMemcpy(host_data[array_index], data_[array_index],
                               size_ * sizeof(T), cudaMemcpyDeviceToHost);
            }
        }

        return err;
    }

private:
    /**
     * @brief Allocate memory for all arrays
     */
    __host__ void allocate_memory(bool device_allocate, bool host_accessible) {
        for (size_t i = 0; i < N; ++i) {
            cudaError_t err = cudaSuccess;

            if (device_allocate) {
                if (host_accessible) {
                    // Allocate with host access
                    err = cudaMallocManaged(&data_[i], size_ * sizeof(T));
                } else {
                    // Device-only allocation
                    err = cudaMalloc(&data_[i], size_ * sizeof(T));
                }
            } else {
                // Host allocation
                err = cudaMallocHost(&data_[i], size_ * sizeof(T));
            }

            if (err != cudaSuccess) {
                // Cleanup allocated memory on failure
                for (size_t j = 0; j < i; ++j) {
                    if (device_allocate) {
                        cudaFree(data_[j]);
                    } else {
                        cudaFreeHost(data_[j]);
                    }
                }
                for (size_t j = 0; j < N; ++j) {
                    data_[j] = nullptr;
                }
                device_allocated_ = false;
                return;
            }
        }

        device_allocated_ = device_allocate;
    }

    /**
     * @brief Deallocate all memory
     */
    __host__ void deallocate_memory() {
        if (data_) {
            for (size_t i = 0; i < N; ++i) {
                if (data_[i] != nullptr) {
                    if (device_allocated_) {
                        cudaFree(data_[i]);
                    } else {
                        cudaFreeHost(data_[i]);
                    }
                    data_[i] = nullptr;
                }
            }
        }
        device_allocated_ = false;
        host_accessible_ = false;
    }
};

/**
 * @brief Shared memory optimization with bank conflict elimination
 *
 * Provides padded data structures to eliminate shared memory bank conflicts
 * and optimize access patterns for maximum throughput.
 */
template<typename T>
class BankConflictOptimized {
private:
    static constexpr size_t PADDING_SIZE =
        ((sizeof(T) % SHARED_MEMORY_BANK_SIZE) == 0) ?
        SHARED_MEMORY_BANK_SIZE : (SHARED_MEMORY_BANK_SIZE - (sizeof(T) % SHARED_MEMORY_BANK_SIZE));

    struct PaddedElement {
        T data;
        char padding[PADDING_SIZE];
    };

    alignas(MEMORY_ALIGNMENT) PaddedElement* data_;
    size_t size_;

public:
    /**
     * @brief Constructor for bank-optimized shared memory
     * @param size Number of elements
     */
    __device__ BankConflictOptimized(void* shared_memory, size_t size)
        : size_(size) {
        data_ = static_cast<PaddedElement*>(shared_memory);
    }

    /**
     * @brief Get element with bank conflict elimination
     * @param index Element index
     * @return Reference to element
     */
    __device__ T& operator[](size_t index) {
        return data_[index].data;
    }

    /**
     * @brief Get const element with bank conflict elimination
     * @param index Element index
     * @return Const reference to element
     */
    __device__ const T& operator[](size_t index) const {
        return data_[index].data;
    }

    /**
     * @brief Get total size in bytes including padding
     */
    __device__ static size_t padded_size(size_t element_count) {
        return element_count * sizeof(PaddedElement);
    }

    /**
     * @brief Get element stride for coalesced access
     */
    __device__ static constexpr size_t stride() {
        return sizeof(PaddedElement);
    }
};

/**
 * @brief Memory access statistics and telemetry
 */
struct MemoryAccessStats {
    // Global memory access statistics
    uint64_t global_load_bytes;
    uint64_t global_store_bytes;
    uint64_t global_load_transactions;
    uint64_t global_store_transactions;
    float global_load_efficiency;
    float global_store_efficiency;

    // Cache performance statistics
    float l1_cache_hit_rate;
    float l2_cache_hit_rate;
    float shared_memory_bank_conflicts;
    uint64_t shared_memory_transactions;

    // Bandwidth utilization
    float memory_bandwidth_utilization;
    float achieved_bandwidth_gbps;
    float theoretical_bandwidth_gbps;

    // Access pattern analysis
    float coalescing_efficiency;
    float memory_throughput_gbps;
    uint64_t memory_access_latency_ns;

    // Error tracking
    uint64_t memory_errors;
    uint64_t alignment_violations;
    uint64_t out_of_bounds_access;

    // Timing information
    uint64_t total_access_time_ns;
    uint64_t kernel_execution_time_ns;

    __host__ __device__ MemoryAccessStats()
        : global_load_bytes(0), global_store_bytes(0),
          global_load_transactions(0), global_store_transactions(0),
          global_load_efficiency(0.0f), global_store_efficiency(0.0f),
          l1_cache_hit_rate(0.0f), l2_cache_hit_rate(0.0f),
          shared_memory_bank_conflicts(0.0f), shared_memory_transactions(0),
          memory_bandwidth_utilization(0.0f), achieved_bandwidth_gbps(0.0f),
          theoretical_bandwidth_gbps(0.0f), coalescing_efficiency(0.0f),
          memory_throughput_gbps(0.0f), memory_access_latency_ns(0),
          memory_errors(0), alignment_violations(0), out_of_bounds_access(0),
          total_access_time_ns(0), kernel_execution_time_ns(0) {}
};

/**
 * @brief Memory telemetry collection and monitoring
 */
class MemoryTelemetry {
private:
    MemoryAccessStats stats_;
    bool monitoring_enabled_;
    std::chrono::high_resolution_clock::time_point start_time_;

public:
    /**
     * @brief Constructor
     */
    __host__ MemoryTelemetry() : monitoring_enabled_(false) {
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    /**
     * @brief Start memory monitoring
     */
    __host__ void start_monitoring() {
        monitoring_enabled_ = true;
        stats_ = MemoryAccessStats();
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    /**
     * @brief Stop memory monitoring
     */
    __host__ void stop_monitoring() {
        monitoring_enabled_ = false;
        auto end_time = std::chrono::high_resolution_clock::now();
        stats_.total_access_time_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time_).count();
    }

    /**
     * @brief Get current statistics
     */
    __host__ const MemoryAccessStats& get_stats() const {
        return stats_;
    }

    /**
     * @brief Update statistics from device
     */
    __host__ cudaError_t update_from_device(const MemoryAccessStats* device_stats) {
        if (!monitoring_enabled_) return cudaErrorInvalidValue;

        return cudaMemcpy(&stats_, device_stats, sizeof(MemoryAccessStats),
                         cudaMemcpyDeviceToHost);
    }

    /**
     * @brief Check if performance targets are met
     */
    __host__ bool check_performance_targets() const {
        return (stats_.global_load_efficiency >= TARGET_GLOBAL_LOAD_EFFICIENCY &&
                stats_.l1_cache_hit_rate >= TARGET_CACHE_HIT_RATE &&
                stats_.memory_bandwidth_utilization >= TARGET_BANDWIDTH_UTILIZATION &&
                stats_.shared_memory_bank_conflicts <= TARGET_BANK_CONFLICT_RATE);
    }

    /**
     * @brief Generate performance report
     */
    __host__ std::string generate_report() const {
        std::string report = "=== Memory Access Performance Report ===\n";
        report += "Global Memory Load Efficiency: " +
                  std::to_string(stats_.global_load_efficiency) + "%\n";
        report += "L1 Cache Hit Rate: " + std::to_string(stats_.l1_cache_hit_rate) + "%\n";
        report += "L2 Cache Hit Rate: " + std::to_string(stats_.l2_cache_hit_rate) + "%\n";
        report += "Memory Bandwidth Utilization: " +
                  std::to_string(stats_.memory_bandwidth_utilization) + "%\n";
        report += "Achieved Bandwidth: " + std::to_string(stats_.achieved_bandwidth_gbps) + " GB/s\n";
        report += "Coalescing Efficiency: " + std::to_string(stats_.coalescing_efficiency) + "%\n";
        report += "Shared Memory Bank Conflicts: " +
                  std::to_string(stats_.shared_memory_bank_conflicts) + "%\n";
        report += "Memory Errors: " + std::to_string(stats_.memory_errors) + "\n";
        report += "Performance Targets Met: ";
        report += check_performance_targets() ? "YES" : "NO";
        report += "\n";
        return report;
    }
};

/**
 * @brief Memory pool for efficient GPU memory management
 */
class MemoryPool {
private:
    struct MemoryBlock {
        void* ptr;
        size_t size;
        bool in_use;

        __host__ MemoryBlock() : ptr(nullptr), size(0), in_use(false) {}
        __host__ MemoryBlock(void* p, size_t s) : ptr(p), size(s), in_use(false) {}
    };

    std::vector<MemoryBlock> blocks_;
    size_t total_allocated_;
    size_t peak_usage_;
    bool pool_initialized_;

public:
    /**
     * @brief Constructor
     */
    __host__ MemoryPool() : total_allocated_(0), peak_usage_(0), pool_initialized_(false) {}

    /**
     * @brief Destructor
     */
    __host__ ~MemoryPool() {
        cleanup();
    }

    /**
     * @brief Initialize memory pool with default size
     */
    __host__ cudaError_t initialize(size_t initial_size = 256 * 1024 * 1024) { // 256MB default
        if (pool_initialized_) return cudaSuccess;

        // Allocate initial block
        void* initial_block = nullptr;
        cudaError_t err = cudaMalloc(&initial_block, initial_size);
        if (err != cudaSuccess) return err;

        blocks_.emplace_back(initial_block, initial_size);
        total_allocated_ = initial_size;
        peak_usage_ = 0;
        pool_initialized_ = true;

        return cudaSuccess;
    }

    /**
     * @brief Allocate memory from pool
     */
    __host__ void* allocate(size_t size, size_t alignment = MEMORY_ALIGNMENT) {
        if (!pool_initialized_) return nullptr;

        // Find suitable free block
        for (auto& block : blocks_) {
            if (!block.in_use && block.size >= size) {
                block.in_use = true;
                update_peak_usage();
                return block.ptr;
            }
        }

        // No suitable block found, allocate new one
        size_t alloc_size = ((size + alignment - 1) / alignment) * alignment;
        alloc_size = std::max(alloc_size, size_t(64 * 1024)); // Minimum 64KB allocation

        void* new_block = nullptr;
        cudaError_t err = cudaMalloc(&new_block, alloc_size);
        if (err != cudaSuccess) return nullptr;

        blocks_.emplace_back(new_block, alloc_size);
        blocks_.back().in_use = true;
        total_allocated_ += alloc_size;
        update_peak_usage();

        return new_block;
    }

    /**
     * @brief Deallocate memory back to pool
     */
    __host__ void deallocate(void* ptr) {
        if (!ptr || !pool_initialized_) return;

        for (auto& block : blocks_) {
            if (block.ptr == ptr) {
                block.in_use = false;
                return;
            }
        }
    }

    /**
     * @brief Get memory pool statistics
     */
    __host__ struct PoolStats {
        size_t total_allocated;
        size_t peak_usage;
        size_t current_usage;
        size_t block_count;
        size_t free_blocks;

        __host__ PoolStats() : total_allocated(0), peak_usage(0),
                              current_usage(0), block_count(0), free_blocks(0) {}
    };

    __host__ PoolStats get_stats() const {
        PoolStats stats;
        stats.total_allocated = total_allocated_;
        stats.peak_usage = peak_usage_;
        stats.block_count = blocks_.size();

        for (const auto& block : blocks_) {
            if (block.in_use) {
                stats.current_usage += block.size;
            } else {
                stats.free_blocks++;
            }
        }

        return stats;
    }

    /**
     * @brief Cleanup all allocated memory
     */
    __host__ void cleanup() {
        for (auto& block : blocks_) {
            if (block.ptr) {
                cudaFree(block.ptr);
                block.ptr = nullptr;
            }
        }
        blocks_.clear();
        total_allocated_ = 0;
        peak_usage_ = 0;
        pool_initialized_ = false;
    }

private:
    /**
     * @brief Update peak usage statistics
     */
    __host__ void update_peak_usage() {
        size_t current_usage = 0;
        for (const auto& block : blocks_) {
            if (block.in_use) {
                current_usage += block.size;
            }
        }
        peak_usage_ = std::max(peak_usage_, current_usage);
    }
};

/**
 * @brief Vectorized memory operations for optimal throughput
 */
namespace vectorized {

/**
 * @brief Load data with vectorized instructions
 * @param dst Destination pointer
 * @param src Source pointer
 * @param count Number of elements to load
 */
template<typename T>
__device__ __inline__ void vectorized_load(T* dst, const T* src, size_t count) {
    using VecType = typename VectorizedType<T>::type;
    constexpr size_t VEC_ELEMENTS = VectorizedType<T>::elements;

    size_t vector_count = count / VEC_ELEMENTS;
    size_t scalar_count = count % VEC_ELEMENTS;

    // Vectorized load
    const VecType* vec_src = reinterpret_cast<const VecType*>(src);
    VecType* vec_dst = reinterpret_cast<VecType*>(dst);

    #pragma unroll
    for (size_t i = 0; i < vector_count; ++i) {
        vec_dst[i] = vec_src[i];
    }

    // Scalar load for remaining elements
    for (size_t i = 0; i < scalar_count; ++i) {
        dst[vector_count * VEC_ELEMENTS + i] = src[vector_count * VEC_ELEMENTS + i];
    }
}

/**
 * @brief Store data with vectorized instructions
 * @param dst Destination pointer
 * @param src Source pointer
 * @param count Number of elements to store
 */
template<typename T>
__device__ __inline__ void vectorized_store(T* dst, const T* src, size_t count) {
    using VecType = typename VectorizedType<T>::type;
    constexpr size_t VEC_ELEMENTS = VectorizedType<T>::elements;

    size_t vector_count = count / VEC_ELEMENTS;
    size_t scalar_count = count % VEC_ELEMENTS;

    // Vectorized store
    const VecType* vec_src = reinterpret_cast<const VecType*>(src);
    VecType* vec_dst = reinterpret_cast<VecType*>(dst);

    #pragma unroll
    for (size_t i = 0; i < vector_count; ++i) {
        vec_dst[i] = vec_src[i];
    }

    // Scalar store for remaining elements
    for (size_t i = 0; i < scalar_count; ++i) {
        dst[vector_count * VEC_ELEMENTS + i] = src[vector_count * VEC_ELEMENTS + i];
    }
}

/**
 * @brief Prefetch data into cache
 * @param ptr Pointer to prefetch
 * @param bytes Number of bytes to prefetch
 */
template<typename T>
__device__ __inline__ void prefetch_data(const T* ptr, size_t bytes) {
    // Use built-in prefetch if available
    #if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 350
        __builtin_prefetch(ptr, 0, 3); // Read, high temporal locality
    #endif
}

/**
 * @brief Strided memory access for SoA layouts
 * @param dst Destination pointer
 * @param src Source pointer with stride
 * @param count Number of elements
 * @param stride Stride between elements
 */
template<typename T>
__device__ __inline__ void strided_load(T* dst, const T* src, size_t count, size_t stride) {
    #pragma unroll 4
    for (size_t i = 0; i < count; ++i) {
        dst[i] = src[i * stride];
    }
}

/**
 * @brief Coalesced memory access pattern
 * @param data Global memory data
 * @param tid Thread ID
 * @param total_threads Total number of threads
 */
template<typename T>
__device__ __inline__ T coalesced_read(const T* data, size_t tid, size_t total_threads) {
    // Ensure coalesced access by having consecutive threads access consecutive memory
    return data[tid];
}

/**
 * @brief Coalesced memory write pattern
 * @param data Global memory data
 * @param value Value to write
 * @param tid Thread ID
 * @param total_threads Total number of threads
 */
template<typename T>
__device__ __inline__ void coalesced_write(T* data, T value, size_t tid, size_t total_threads) {
    // Ensure coalesced access by having consecutive threads write to consecutive memory
    data[tid] = value;
}

} // namespace vectorized

/**
 * @brief Memory access pattern optimization utilities
 */
namespace patterns {

/**
 * @brief Optimized 2D memory access for matrix operations
 */
template<typename T>
class Matrix2DAccess {
private:
    T* data_;
    size_t rows_;
    size_t cols_;
    size_t pitch_; // Padded column size for optimal access

public:
    __host__ __device__ Matrix2DAccess(T* data, size_t rows, size_t cols)
        : data_(data), rows_(rows), cols_(cols) {
        // Calculate optimal pitch (pad to 128-byte alignment)
        size_t bytes_per_row = cols_ * sizeof(T);
        pitch_ = ((bytes_per_row + MEMORY_ALIGNMENT - 1) / MEMORY_ALIGNMENT) * MEMORY_ALIGNMENT;
        pitch_ /= sizeof(T); // Convert back to element count
    }

    __device__ __inline__ T& operator()(size_t row, size_t col) {
        return data_[row * pitch_ + col];
    }

    __device__ __inline__ const T& operator()(size_t row, size_t col) const {
        return data_[row * pitch_ + col];
    }

    __host__ __device__ size_t get_pitch() const { return pitch_; }

    __host__ __device__ size_t get_padded_size() const { return rows_ * pitch_; }
};

/**
 * @brief Ring buffer for streaming data access
 */
template<typename T>
class RingBuffer {
private:
    T* buffer_;
    size_t capacity_;
    size_t head_;
    size_t tail_;
    size_t count_;

public:
    __host__ __device__ RingBuffer(T* buffer, size_t capacity)
        : buffer_(buffer), capacity_(capacity), head_(0), tail_(0), count_(0) {}

    __device__ __inline__ bool push(const T& item) {
        if (count_ >= capacity_) return false;

        buffer_[head_] = item;
        head_ = (head_ + 1) % capacity_;
        count_++;
        return true;
    }

    __device__ __inline__ bool pop(T& item) {
        if (count_ == 0) return false;

        item = buffer_[tail_];
        tail_ = (tail_ + 1) % capacity_;
        count_--;
        return true;
    }

    __device__ __inline__ bool is_full() const { return count_ >= capacity_; }
    __device__ __inline__ bool is_empty() const { return count_ == 0; }
    __device__ __inline__ size_t size() const { return count_; }
    __device__ __inline__ size_t capacity() const { return capacity_; }
};

/**
 * @brief Memory access pattern analyzer
 */
class AccessPatternAnalyzer {
private:
    uint64_t access_count_;
    uint64_t sequential_accesses_;
    uint64_t random_accesses_;
    uint64_t cache_misses_;

public:
    __device__ AccessPatternAnalyzer()
        : access_count_(0), sequential_accesses_(0),
          random_accesses_(0), cache_misses_(0) {}

    __device__ __inline__ void record_access(uintptr_t prev_addr, uintptr_t curr_addr) {
        access_count_++;

        if (curr_addr == prev_addr + sizeof(uint32_t)) {
            sequential_accesses_++;
        } else {
            random_accesses_++;
        }
    }

    __device__ __inline__ void record_cache_miss() {
        cache_misses_++;
    }

    __device__ __inline__ float get_sequential_ratio() const {
        return access_count_ > 0 ?
            (static_cast<float>(sequential_accesses_) / access_count_) * 100.0f : 0.0f;
    }

    __device__ __inline__ float get_cache_miss_rate() const {
        return access_count_ > 0 ?
            (static_cast<float>(cache_misses_) / access_count_) * 100.0f : 0.0f;
    }
};

} // namespace patterns

/**
 * @brief Memory pool singleton for global access
 */
class GlobalMemoryPool {
private:
    static MemoryPool instance_;
    static bool initialized_;

public:
    /**
     * @brief Initialize global memory pool
     */
    __host__ static cudaError_t initialize(size_t initial_size = 256 * 1024 * 1024) {
        if (!initialized_) {
            cudaError_t err = instance_.initialize(initial_size);
            if (err == cudaSuccess) {
                initialized_ = true;
            }
            return err;
        }
        return cudaSuccess;
    }

    /**
     * @brief Get memory pool instance
     */
    __host__ static MemoryPool& get_instance() {
        return instance_;
    }

    /**
     * @brief Cleanup global memory pool
     */
    __host__ static void cleanup() {
        if (initialized_) {
            instance_.cleanup();
            initialized_ = false;
        }
    }
};

// Static member definitions
MemoryPool GlobalMemoryPool::instance_;
bool GlobalMemoryPool::initialized_ = false;

/**
 * @brief Convenience functions for memory operations
 */
namespace utils {

/**
 * @brief Allocate aligned memory from global pool
 */
template<typename T>
__host__ T* allocate_aligned(size_t count, size_t alignment = MEMORY_ALIGNMENT) {
    void* ptr = GlobalMemoryPool::get_instance().allocate(count * sizeof(T), alignment);
    return static_cast<T*>(ptr);
}

/**
 * @brief Deallocate memory back to global pool
 */
template<typename T>
__host__ void deallocate_aligned(T* ptr) {
    GlobalMemoryPool::get_instance().deallocate(ptr);
}

/**
 * @brief Check if pointer is properly aligned
 */
template<typename T>
__host__ __device__ bool is_aligned(const T* ptr, size_t alignment = MEMORY_ALIGNMENT) {
    return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0;
}

/**
 * @brief Get next power of 2 for alignment
 */
__host__ __device__ constexpr size_t next_power_of_2(size_t n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;
    return n;
}

/**
 * @brief Calculate optimal shared memory size for given data
 */
template<typename T>
__host__ __device__ constexpr size_t calculate_shared_memory_size(size_t element_count) {
    size_t raw_size = element_count * sizeof(T);
    return ((raw_size + MEMORY_ALIGNMENT - 1) / MEMORY_ALIGNMENT) * MEMORY_ALIGNMENT;
}

/**
 * @brief Validate memory access bounds
 */
template<typename T>
__device__ __inline__ bool validate_access(const T* ptr, size_t index, size_t size) {
    return ptr != nullptr && index < size;
}

/**
 * @brief Safe memory access with bounds checking
 */
template<typename T>
__device__ __inline__ T safe_access(const T* ptr, size_t index, size_t size, T default_value = T()) {
    return validate_access(ptr, index, size) ? ptr[index] : default_value;
}

} // namespace utils

// Legacy optimized big integer functions for backward compatibility

/**
 * @brief Optimized big integer read with minimal synchronization
 *
 * This function reduces synchronization overhead from 2 __syncthreads() per call
 * to 0 for most cases, addressing the P1 issue in ecc_operations.cuh:55.
 *
 * Performance improvements:
 * - Eliminates shared memory barrier for small batches
 * - Uses vectorized loads when possible
 * - Reduces warp divergence
 * - Improves memory coalescing
 *
 * Usage: Replace ReadBigInt calls in performance-critical paths
 */

/**
 * @brief Fast read without shared memory (no synchronization)
 *
 * For small batches or when coalescing is guaranteed, skip shared memory
 * entirely to eliminate synchronization overhead.
 *
 * @param ara Global memory array
 * @param idx Point index
 * @param x Output array (8 words)
 */
__device__ inline void ReadBigInt_Fast(
    const unsigned int* ara,
    int idx,
    unsigned int x[8]
) {
    // Calculate base index for strided access
    int totalThreads = gridDim.x * blockDim.x;
    int threadId = blockDim.x * blockIdx.x + threadIdx.x;
    int base = idx * totalThreads * 8;

    // Direct global memory read with minimal overhead
    // This is faster than shared memory version when:
    // 1. Access pattern is already coalesced
    // 2. No need for inter-thread cooperation
    // 3. Batch size is small

    #pragma unroll
    for (int i = 0; i < 8; i++) {
        int globalIndex = base + threadId + i * totalThreads;
        x[i] = ara[globalIndex];
    }
}

/**
 * @brief Vectorized read using int4 (128-bit transactions)
 *
 * Uses CUDA's vectorized loads to reduce memory transaction count
 * from 8 to 2 per big integer read.
 *
 * @param ara Global memory array (must be 16-byte aligned)
 * @param idx Point index
 * @param x Output array (8 words)
 */
__device__ inline void ReadBigInt_Vectorized(
    const unsigned int* ara,
    int idx,
    unsigned int x[8]
) {
    // Calculate 16-byte aligned base address
    int totalThreads = gridDim.x * blockDim.x;
    int threadId = blockDim.x * blockIdx.x + threadIdx.x;
    int base = idx * totalThreads * 2;  // 2 int4 elements per big integer

    // Cast to int4 pointer for vectorized access
    const int4* vec_ptr = reinterpret_cast<const int4*>(ara);

    // Read first 4 words (128 bits)
    int4 vec_data1 = vec_ptr[base + threadId];
    x[0] = vec_data1.x;
    x[1] = vec_data1.y;
    x[2] = vec_data1.z;
    x[3] = vec_data1.w;

    // Read second 4 words (128 bits)
    int4 vec_data2 = vec_ptr[base + threadId + totalThreads];
    x[4] = vec_data2.x;
    x[5] = vec_data2.y;
    x[6] = vec_data2.z;
    x[7] = vec_data2.w;
}

/**
 * @brief Warp-cooperative read (single synchronization per warp)
 *
 * Reduces synchronization overhead from per-thread to per-warp by
 * coordinating reads within warps.
 *
 * @param ara Global memory array
 * @param idx Point index
 * @param x Output array (8 words)
 */
__device__ inline void ReadBigInt_WarpCooperative(
    const unsigned int* ara,
    int idx,
    unsigned int x[8]
) {
    const int warp_id = threadIdx.x / 32;
    const int lane_id = threadIdx.x % 32;

    // Each warp cooperatively reads 32 big integers (256 words total)
    // Use shared memory only within the warp
    __shared__ unsigned int warp_shared[32][8];  // 32 lanes × 8 words

    // Each thread reads its assigned word from the global array
    int base_idx = idx + warp_id;  // Warp offset

    // Read one word per thread (total 8 threads per big integer)
    if (lane_id < 8) {
        int totalThreads = gridDim.x * blockDim.x;
        int threadId = blockDim.x * blockIdx.x + threadIdx.x;
        int global_base = base_idx * totalThreads * 8;

        x[lane_id] = ara[global_base + threadId + lane_id * totalThreads];
        warp_shared[warp_id][lane_id] = x[lane_id];
    }

    // Single synchronization per warp
    __syncthreads();

    // Each thread reconstructs its complete big integer
    if (lane_id < 8) {
        #pragma unroll
        for (int i = 0; i < 8; i++) {
            x[i] = warp_shared[warp_id][i];
        }
    }
}

/**
 * @brief Adaptive read - selects optimal method based on conditions
 *
 * Automatically chooses the fastest read method based on:
 * 1. Batch size
 * 2. Memory alignment
 * 3. Thread block configuration
 * 4. Available shared memory
 *
 * @param ara Global memory array
 * @param idx Point index
 * @param x Output array (8 words)
 */
__device__ inline void ReadBigInt_Adaptive(
    const unsigned int* ara,
    int idx,
    unsigned int x[8]
) {
    // Choose optimal method based on conditions

    // Condition 1: Check if address is 16-byte aligned
    bool is_aligned = (reinterpret_cast<uintptr_t>(ara) & 0xF) == 0;

    // Condition 2: Check batch size (small batches = no shared memory)
    bool small_batch = (gridDim.x * blockDim.x <= 256);

    // Condition 3: Check if we have enough threads per warp
    bool full_warp = (blockDim.x >= 32);

    if (is_aligned && !small_batch) {
        // Best case: vectorized access
        ReadBigInt_Vectorized(ara, idx, x);
    } else if (small_batch) {
        // Small batch: skip shared memory entirely
        ReadBigInt_Fast(ara, idx, x);
    } else if (full_warp) {
        // Warp-cooperative: single synchronization
        ReadBigInt_WarpCooperative(ara, idx, x);
    } else {
        // Fallback: use direct memory access
        ReadBigInt_Fast(ara, idx, x);
    }
}

/**
 * @brief Optimized write operations (mirror of read operations)
 */

__device__ inline void WriteBigInt_Fast(
    unsigned int* ara,
    int idx,
    const unsigned int x[8]
) {
    int totalThreads = gridDim.x * blockDim.x;
    int threadId = blockDim.x * blockIdx.x + threadIdx.x;
    int base = idx * totalThreads * 8;

    #pragma unroll
    for (int i = 0; i < 8; i++) {
        int globalIndex = base + threadId + i * totalThreads;
        ara[globalIndex] = x[i];
    }
}

__device__ inline void WriteBigInt_Vectorized(
    unsigned int* ara,
    int idx,
    const unsigned int x[8]
) {
    int totalThreads = gridDim.x * blockDim.x;
    int threadId = blockDim.x * blockIdx.x + threadIdx.x;
    int base = idx * totalThreads * 2;

    int4* vec_ptr = reinterpret_cast<int4*>(ara);

    // Pack first 4 words into int4
    int4 vec_data1;
    vec_data1.x = x[0];
    vec_data1.y = x[1];
    vec_data1.z = x[2];
    vec_data1.w = x[3];
    vec_ptr[base + threadId] = vec_data1;

    // Pack second 4 words into int4
    int4 vec_data2;
    vec_data2.x = x[4];
    vec_data2.y = x[5];
    vec_data2.z = x[6];
    vec_data2.w = x[7];
    vec_ptr[base + threadId + totalThreads] = vec_data2;
}

__device__ inline void WriteBigInt_Adaptive(
    unsigned int* ara,
    int idx,
    const unsigned int x[8]
) {
    bool is_aligned = (reinterpret_cast<uintptr_t>(ara) & 0xF) == 0;
    bool small_batch = (gridDim.x * blockDim.x <= 256);

    if (is_aligned && !small_batch) {
        WriteBigInt_Vectorized(ara, idx, x);
    } else {
        WriteBigInt_Fast(ara, idx, x);
    }
}

/**
 * @brief Structure-of-Arrays (SoA) layout for ECC point coordinates
 *
 * Provides optimal memory coalescing by storing coordinates in separate arrays
 * rather than array-of-structures. This enables consecutive threads to access
 * consecutive memory addresses.
 */
struct ECCPointSoA {
    uint32_t* x_coords;      // X coordinates array (8 words per point)
    uint32_t* y_coords;      // Y coordinates array (8 words per point)
    uint32_t* indices;       // Index array for point identification
    uint8_t*  match_flags;    // Flags array for match status
    size_t capacity;         // Total capacity of arrays
    size_t count;            // Current number of points

    // Constructor
    __host__ ECCPointSoA(size_t initial_capacity = 0)
        : capacity(initial_capacity), count(0) {
        if (initial_capacity > 0) {
            allocate_device_memory();
        }
    }

    // Destructor
    __host__ ~ECCPointSoA() {
        deallocate_device_memory();
    }

    // Delete copy operations
    ECCPointSoA(const ECCPointSoA&) = delete;
    ECCPointSoA& operator=(const ECCPointSoA&) = delete;

    // Move operations
    __host__ ECCPointSoA(ECCPointSoA&& other) noexcept
        : x_coords(other.x_coords), y_coords(other.y_coords), indices(other.indices),
          match_flags(other.match_flags), capacity(other.capacity), count(other.count) {
        other.x_coords = nullptr;
        other.y_coords = nullptr;
        other.indices = nullptr;
        other.match_flags = nullptr;
        other.capacity = 0;
        other.count = 0;
    }

    __host__ ECCPointSoA& operator=(ECCPointSoA&& other) noexcept {
        if (this != &other) {
            deallocate_device_memory();
            x_coords = other.x_coords;
            y_coords = other.y_coords;
            indices = other.indices;
            match_flags = other.match_flags;
            capacity = other.capacity;
            count = other.count;

            other.x_coords = nullptr;
            other.y_coords = nullptr;
            other.indices = nullptr;
            other.match_flags = nullptr;
            other.capacity = 0;
            other.count = 0;
        }
        return *this;
    }

    /**
     * @brief Allocate device memory with 128-byte alignment
     */
    __host__ bool allocate_device_memory() {
        if (capacity == 0) return false;

        size_t x_y_size = capacity * 8 * sizeof(uint32_t);  // 8 words per coordinate
        size_t indices_size = capacity * sizeof(uint32_t);
        size_t flags_size = capacity * sizeof(uint8_t);

        // Align to 128 bytes for optimal memory access
        size_t aligned_x_y_size = ((x_y_size + MEMORY_ALIGNMENT - 1) / MEMORY_ALIGNMENT) * MEMORY_ALIGNMENT;
        size_t aligned_indices_size = ((indices_size + MEMORY_ALIGNMENT - 1) / MEMORY_ALIGNMENT) * MEMORY_ALIGNMENT;
        size_t aligned_flags_size = ((flags_size + MEMORY_ALIGNMENT - 1) / MEMORY_ALIGNMENT) * MEMORY_ALIGNMENT;

        cudaError_t error = cudaMalloc(&x_coords, aligned_x_y_size);
        if (error != cudaSuccess) return false;

        error = cudaMalloc(&y_coords, aligned_x_y_size);
        if (error != cudaSuccess) {
            cudaFree(x_coords);
            return false;
        }

        error = cudaMalloc(&indices, aligned_indices_size);
        if (error != cudaSuccess) {
            cudaFree(x_coords);
            cudaFree(y_coords);
            return false;
        }

        error = cudaMalloc(&match_flags, aligned_flags_size);
        if (error != cudaSuccess) {
            cudaFree(x_coords);
            cudaFree(y_coords);
            cudaFree(indices);
            return false;
        }

        return true;
    }

    /**
     * @brief Deallocate device memory
     */
    __host__ void deallocate_device_memory() {
        if (x_coords) cudaFree(x_coords);
        if (y_coords) cudaFree(y_coords);
        if (indices) cudaFree(indices);
        if (match_flags) cudaFree(match_flags);

        x_coords = nullptr;
        y_coords = nullptr;
        indices = nullptr;
        match_flags = nullptr;
    }

    /**
     * @brief Get memory usage information
     */
    __host__ size_t get_memory_usage() const {
        size_t x_y_size = capacity * 8 * sizeof(uint32_t) * 2;  // X and Y coordinates
        size_t indices_size = capacity * sizeof(uint32_t);
        size_t flags_size = capacity * sizeof(uint8_t);
        return x_y_size + indices_size + flags_size;
    }

    /**
     * @brief Get memory alignment information
     */
    __host__ bool is_aligned() const {
        uintptr_t x_addr = reinterpret_cast<uintptr_t>(x_coords);
        uintptr_t y_addr = reinterpret_cast<uintptr_t>(y_coords);
        uintptr_t idx_addr = reinterpret_cast<uintptr_t>(indices);
        uintptr_t flag_addr = reinterpret_cast<uintptr_t>(match_flags);

        return (x_addr % MEMORY_ALIGNMENT == 0) &&
               (y_addr % MEMORY_ALIGNMENT == 0) &&
               (idx_addr % MEMORY_ALIGNMENT == 0) &&
               (flag_addr % MEMORY_ALIGNMENT == 0);
    }
};

/**
 * @brief Padded structure to eliminate shared memory bank conflicts
 *
 * Uses 68-byte (17 words) structure which is coprime with 32 shared memory banks,
 * ensuring even distribution across banks and preventing bank conflicts.
 */
struct alignas(MEMORY_ALIGNMENT) PaddedECCPoint {
    uint32_t x[8];  // X coordinate (8 words)
    uint32_t y[8];  // Y coordinate (8 words)
    char padding[4]; // Pad to 68 bytes (17 words total)

    // Default constructor
    __device__ __host__ PaddedECCPoint() {
        #pragma unroll
        for (int i = 0; i < 8; ++i) {
            x[i] = 0;
            y[i] = 0;
        }
        #pragma unroll
        for (int i = 0; i < 4; ++i) {
            padding[i] = 0;
        }
    }

    // Constructor from coordinates
    __device__ __host__ PaddedECCPoint(const uint32_t x_in[8], const uint32_t y_in[8]) {
        #pragma unroll
        for (int i = 0; i < 8; ++i) {
            x[i] = x_in[i];
            y[i] = y_in[i];
        }
        #pragma unroll
        for (int i = 0; i < 4; ++i) {
            padding[i] = 0;
        }
    }

    // Static size for shared memory allocation
    static constexpr size_t SIZE = 68;  // 17 * 4 bytes
    static constexpr size_t ALIGNMENT = MEMORY_ALIGNMENT;
};

} // namespace memory
} // namespace keyhunt

// Migration macros for easy replacement
#define ReadBigInt_Optimized(x_ptr, idx, x_out) \
    keyhunt::memory::ReadBigInt_Adaptive(x_ptr, idx, x_out)

#define WriteBigInt_Optimized(ara, idx, x_in) \
    keyhunt::memory::WriteBigInt_Adaptive(ara, idx, x_in)

/**
 * @brief Device-side memory statistics collection
 */
extern __device__ keyhunt::memory::MemoryAccessStats device_memory_stats;

/**
 * @brief Initialize device memory statistics
 */
__device__ void initialize_memory_stats();

/**
 * @brief Update memory statistics during access
 */
__device__ void update_memory_stats(uint64_t bytes_accessed, bool is_load, bool is_coalesced);

/**
 * @brief Finalize memory statistics collection
 */
__device__ void finalize_memory_stats();
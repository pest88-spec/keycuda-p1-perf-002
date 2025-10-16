/**
 * Unified RAII Resource Management
 * 
 * Implements P2-001: Code Duplication Optimization
 * - CUDA resource RAII wrappers
 * - File handle RAII wrappers
 * - Memory management RAII wrappers
 * - Exception-safe resource management
 * - DRY principle compliance
 * 
 * @origin       https://github.com/Puzzle71Solver/Puzzle71Solver
 * @origin_path  src/utils/resource_guard.h
 * @origin_commit <current_commit>
 * @origin_license MIT
 * @extracted_date   2025-10-13
 * @extracted_by     Puzzle71Solver Team
 * @modifications    Created for P2-001 code duplication optimization
 * @spdx_license_identifier MIT
 */

#pragma once

#include "error_handling.h"
#include <cuda_runtime.h>
#include <fstream>
#include <memory>
#include <cstdio>

namespace puzzle71 {
namespace utils {

/**
 * CUDA Memory Guard
 * 
 * RAII wrapper for CUDA device memory allocation.
 * Automatically frees memory on destruction.
 * 
 * Usage:
 *   CudaMemoryGuard<float> buffer(1024);  // Allocate 1024 floats
 *   float* ptr = buffer.get();            // Get device pointer
 *   // Memory automatically freed when buffer goes out of scope
 */
template<typename T>
class CudaMemoryGuard {
public:
    /**
     * Allocate CUDA device memory
     * 
     * @param count Number of elements to allocate
     * @throws std::runtime_error if allocation fails
     */
    explicit CudaMemoryGuard(size_t count = 0)
        : ptr_(nullptr), size_(0)
    {
        if (count > 0) {
            allocate(count);
        }
    }

    /**
     * Destructor - automatically frees memory
     */
    ~CudaMemoryGuard() {
        free();
    }

    // Disable copy (move-only type)
    CudaMemoryGuard(const CudaMemoryGuard&) = delete;
    CudaMemoryGuard& operator=(const CudaMemoryGuard&) = delete;

    /**
     * Move constructor
     */
    CudaMemoryGuard(CudaMemoryGuard&& other) noexcept
        : ptr_(other.ptr_), size_(other.size_)
    {
        other.ptr_ = nullptr;
        other.size_ = 0;
    }

    /**
     * Move assignment
     */
    CudaMemoryGuard& operator=(CudaMemoryGuard&& other) noexcept {
        if (this != &other) {
            free();
            ptr_ = other.ptr_;
            size_ = other.size_;
            other.ptr_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    /**
     * Allocate or reallocate memory
     * 
     * @param count Number of elements to allocate
     * @throws std::runtime_error if allocation fails
     */
    void allocate(size_t count) {
        if (count == 0) {
            free();
            return;
        }

        // Free existing memory
        free();

        // Allocate new memory
        CUDA_CHECK(cudaMalloc(&ptr_, count * sizeof(T)));
        size_ = count;
    }

    /**
     * Free memory (called automatically by destructor)
     */
    void free() {
        if (ptr_) {
            cudaFree(ptr_);
            ptr_ = nullptr;
            size_ = 0;
        }
    }

    /**
     * Get device pointer
     */
    T* get() const { return ptr_; }

    /**
     * Get number of elements
     */
    size_t size() const { return size_; }

    /**
     * Check if memory is allocated
     */
    bool isAllocated() const { return ptr_ != nullptr; }

    /**
     * Release ownership (caller responsible for freeing)
     */
    T* release() {
        T* tmp = ptr_;
        ptr_ = nullptr;
        size_ = 0;
        return tmp;
    }

    /**
     * Reset (free memory)
     */
    void reset() {
        free();
    }

private:
    T* ptr_;
    size_t size_;
};

/**
 * CUDA Device Guard
 * 
 * RAII wrapper for CUDA device selection.
 * Automatically restores previous device on destruction.
 * 
 * Usage:
 *   {
 *       CudaDeviceGuard guard(1);  // Switch to device 1
 *       // Operations on device 1
 *   }  // Automatically restore previous device
 */
class CudaDeviceGuard {
public:
    /**
     * Switch to specified device
     * 
     * @param device_id Device ID to switch to
     * @throws std::runtime_error if device switch fails
     */
    explicit CudaDeviceGuard(int device_id)
        : previous_device_(-1)
    {
        // Get current device
        CUDA_CHECK(cudaGetDevice(&previous_device_));

        // Switch to new device
        if (device_id != previous_device_) {
            CUDA_CHECK(cudaSetDevice(device_id));
        }
    }

    /**
     * Destructor - restore previous device
     */
    ~CudaDeviceGuard() {
        if (previous_device_ >= 0) {
            cudaSetDevice(previous_device_);
        }
    }

    // Disable copy and move
    CudaDeviceGuard(const CudaDeviceGuard&) = delete;
    CudaDeviceGuard& operator=(const CudaDeviceGuard&) = delete;
    CudaDeviceGuard(CudaDeviceGuard&&) = delete;
    CudaDeviceGuard& operator=(CudaDeviceGuard&&) = delete;

private:
    int previous_device_;
};

/**
 * File Handle Guard
 * 
 * RAII wrapper for C FILE* handles.
 * Automatically closes file on destruction.
 * 
 * Usage:
 *   FileGuard file("data.bin", "rb");
 *   if (file.isOpen()) {
 *       fread(buffer, 1, size, file.get());
 *   }  // File automatically closed
 */
class FileGuard {
public:
    /**
     * Open file
     *
     * @param filename Path to file
     * @param mode File open mode ("r", "w", "rb", "wb", etc.)
     */
    FileGuard(const char* filename, const char* mode)
        : file_(nullptr)
    {
        file_ = fopen(filename, mode);
    }

    /**
     * Wrap existing FILE* pointer
     *
     * @param file FILE* pointer to wrap
     */
    explicit FileGuard(FILE* file = nullptr)
        : file_(file)
    {
    }

    /**
     * Destructor - close file
     */
    ~FileGuard() {
        close();
    }

    // Disable copy (move-only type)
    FileGuard(const FileGuard&) = delete;
    FileGuard& operator=(const FileGuard&) = delete;

    /**
     * Move constructor
     */
    FileGuard(FileGuard&& other) noexcept
        : file_(other.file_)
    {
        other.file_ = nullptr;
    }

    /**
     * Move assignment
     */
    FileGuard& operator=(FileGuard&& other) noexcept {
        if (this != &other) {
            close();
            file_ = other.file_;
            other.file_ = nullptr;
        }
        return *this;
    }

    /**
     * Close file (called automatically by destructor)
     */
    void close() {
        if (file_) {
            fclose(file_);
            file_ = nullptr;
        }
    }

    /**
     * Get FILE* pointer
     */
    FILE* get() const { return file_; }

    /**
     * Check if file is open
     */
    bool isOpen() const { return file_ != nullptr; }

    /**
     * Release ownership (caller responsible for closing)
     */
    FILE* release() {
        FILE* tmp = file_;
        file_ = nullptr;
        return tmp;
    }

    /**
     * Reset (close file)
     */
    void reset() {
        close();
    }

private:
    FILE* file_;
};

/**
 * Scoped Memory Guard
 * 
 * RAII wrapper for host memory allocation.
 * Automatically frees memory on destruction.
 * 
 * Usage:
 *   ScopedMemory<int> buffer(1024);  // Allocate 1024 ints
 *   int* ptr = buffer.get();         // Get pointer
 *   // Memory automatically freed when buffer goes out of scope
 */
template<typename T>
class ScopedMemory {
public:
    /**
     * Allocate host memory
     * 
     * @param count Number of elements to allocate
     */
    explicit ScopedMemory(size_t count = 0)
        : ptr_(nullptr), size_(0)
    {
        if (count > 0) {
            allocate(count);
        }
    }

    /**
     * Destructor - automatically frees memory
     */
    ~ScopedMemory() {
        free();
    }

    // Disable copy (move-only type)
    ScopedMemory(const ScopedMemory&) = delete;
    ScopedMemory& operator=(const ScopedMemory&) = delete;

    /**
     * Move constructor
     */
    ScopedMemory(ScopedMemory&& other) noexcept
        : ptr_(other.ptr_), size_(other.size_)
    {
        other.ptr_ = nullptr;
        other.size_ = 0;
    }

    /**
     * Move assignment
     */
    ScopedMemory& operator=(ScopedMemory&& other) noexcept {
        if (this != &other) {
            free();
            ptr_ = other.ptr_;
            size_ = other.size_;
            other.ptr_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    /**
     * Allocate or reallocate memory
     */
    void allocate(size_t count) {
        if (count == 0) {
            free();
            return;
        }

        free();
        ptr_ = new T[count];
        size_ = count;
    }

    /**
     * Free memory
     */
    void free() {
        if (ptr_) {
            delete[] ptr_;
            ptr_ = nullptr;
            size_ = 0;
        }
    }

    /**
     * Get pointer
     */
    T* get() const { return ptr_; }

    /**
     * Get number of elements
     */
    size_t size() const { return size_; }

    /**
     * Check if memory is allocated
     */
    bool isAllocated() const { return ptr_ != nullptr; }

    /**
     * Array access operator
     */
    T& operator[](size_t index) { return ptr_[index]; }
    const T& operator[](size_t index) const { return ptr_[index]; }

    /**
     * Release ownership (caller responsible for freeing)
     */
    T* release() {
        T* tmp = ptr_;
        ptr_ = nullptr;
        size_ = 0;
        return tmp;
    }

    /**
     * Reset (free memory)
     */
    void reset() {
        free();
    }

private:
    T* ptr_;
    size_t size_;
};

} // namespace utils
} // namespace puzzle71


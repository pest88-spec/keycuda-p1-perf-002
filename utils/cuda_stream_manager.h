/**
 * @file cuda_stream_manager.h
 * @brief CUDA Stream Manager for parallel kernel execution
 * 
 * P1-PERF-001: CUDA流并行化优化
 * 目标: 提升GPU利用率10-20%,实现ECC和Hash内核并行执行
 * 
 * 设计原则:
 * - RAII管理CUDA流生命周期
 * - 支持多流并行执行
 * - 异步内存传输
 * - 流同步管理
 */

#pragma once

#include <cuda_runtime.h>
#include <vector>
#include <stdexcept>
#include <string>

namespace puzzle71 {
namespace utils {

/**
 * @brief CUDA Stream Manager
 * 
 * 管理多个CUDA流,实现内核并行执行和异步内存传输
 * 
 * 使用示例:
 * ```cpp
 * CudaStreamManager streams(2);
 * 
 * // 流0: ECC kernel
 * LaunchEccKernel<<<grid, block, 0, streams.getStream(0)>>>(batch0);
 * 
 * // 流1: Hash kernel (前一批次)
 * LaunchHashKernel<<<grid, block, 0, streams.getStream(1)>>>(batch1);
 * 
 * // 同步所有流
 * streams.synchronizeAll();
 * ```
 */
class CudaStreamManager {
public:
    /**
     * @brief 构造函数,创建指定数量的CUDA流
     * @param num_streams 流数量(默认2个)
     * @throws std::runtime_error 如果流创建失败
     */
    explicit CudaStreamManager(int num_streams = 2);
    
    /**
     * @brief 析构函数,自动销毁所有CUDA流
     */
    ~CudaStreamManager();
    
    // 禁止拷贝
    CudaStreamManager(const CudaStreamManager&) = delete;
    CudaStreamManager& operator=(const CudaStreamManager&) = delete;
    
    // 允许移动
    CudaStreamManager(CudaStreamManager&& other) noexcept;
    CudaStreamManager& operator=(CudaStreamManager&& other) noexcept;
    
    /**
     * @brief 获取指定索引的CUDA流
     * @param index 流索引(0 ~ num_streams-1)
     * @return CUDA流句柄
     * @throws std::out_of_range 如果索引越界
     */
    cudaStream_t getStream(int index) const;
    
    /**
     * @brief 同步所有CUDA流
     * @throws std::runtime_error 如果同步失败
     */
    void synchronizeAll();
    
    /**
     * @brief 同步指定CUDA流
     * @param index 流索引
     * @throws std::runtime_error 如果同步失败
     * @throws std::out_of_range 如果索引越界
     */
    void synchronizeStream(int index);
    
    /**
     * @brief 获取流数量
     * @return 流数量
     */
    int getNumStreams() const { return static_cast<int>(streams_.size()); }
    
    /**
     * @brief 检查所有流是否已完成
     * @return true 如果所有流已完成
     */
    bool allStreamsCompleted() const;
    
    /**
     * @brief 检查指定流是否已完成
     * @param index 流索引
     * @return true 如果流已完成
     * @throws std::out_of_range 如果索引越界
     */
    bool isStreamCompleted(int index) const;
    
private:
    std::vector<cudaStream_t> streams_;  ///< CUDA流数组
    
    /**
     * @brief 检查CUDA错误
     * @param err CUDA错误码
     * @param msg 错误消息
     * @throws std::runtime_error 如果有错误
     */
    void checkCudaError(cudaError_t err, const char* msg) const;
};

/**
 * @brief CUDA Stream Guard (RAII wrapper for single stream)
 * 
 * 单个CUDA流的RAII封装,自动管理流生命周期
 * 
 * 使用示例:
 * ```cpp
 * {
 *     CudaStreamGuard stream;
 *     LaunchKernel<<<grid, block, 0, stream.get()>>>(data);
 *     stream.synchronize();
 * } // 流自动销毁
 * ```
 */
class CudaStreamGuard {
public:
    /**
     * @brief 构造函数,创建CUDA流
     * @throws std::runtime_error 如果流创建失败
     */
    CudaStreamGuard();
    
    /**
     * @brief 析构函数,自动销毁CUDA流
     */
    ~CudaStreamGuard();
    
    // 禁止拷贝
    CudaStreamGuard(const CudaStreamGuard&) = delete;
    CudaStreamGuard& operator=(const CudaStreamGuard&) = delete;
    
    // 允许移动
    CudaStreamGuard(CudaStreamGuard&& other) noexcept;
    CudaStreamGuard& operator=(CudaStreamGuard&& other) noexcept;
    
    /**
     * @brief 获取CUDA流句柄
     * @return CUDA流句柄
     */
    cudaStream_t get() const { return stream_; }
    
    /**
     * @brief 同步CUDA流
     * @throws std::runtime_error 如果同步失败
     */
    void synchronize();
    
    /**
     * @brief 检查流是否已完成
     * @return true 如果流已完成
     */
    bool isCompleted() const;
    
private:
    cudaStream_t stream_;  ///< CUDA流句柄
    
    /**
     * @brief 检查CUDA错误
     * @param err CUDA错误码
     * @param msg 错误消息
     * @throws std::runtime_error 如果有错误
     */
    void checkCudaError(cudaError_t err, const char* msg) const;
};

} // namespace utils
} // namespace puzzle71


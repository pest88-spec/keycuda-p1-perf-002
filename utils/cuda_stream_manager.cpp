/**
 * @file cuda_stream_manager.cpp
 * @brief CUDA Stream Manager implementation
 * 
 * P1-PERF-001: CUDA流并行化优化实现
 */

#include "cuda_stream_manager.h"
#include <sstream>

namespace puzzle71 {
namespace utils {

// ============================================================================
// CudaStreamManager Implementation
// ============================================================================

CudaStreamManager::CudaStreamManager(int num_streams) {
    if (num_streams <= 0) {
        throw std::invalid_argument("Number of streams must be positive");
    }
    
    streams_.reserve(num_streams);
    
    for (int i = 0; i < num_streams; ++i) {
        cudaStream_t stream;
        cudaError_t err = cudaStreamCreate(&stream);
        
        if (err != cudaSuccess) {
            // 清理已创建的流
            for (auto s : streams_) {
                cudaStreamDestroy(s);
            }
            
            std::ostringstream oss;
            oss << "Failed to create CUDA stream " << i << ": " << cudaGetErrorString(err);
            throw std::runtime_error(oss.str());
        }
        
        streams_.push_back(stream);
    }
}

CudaStreamManager::~CudaStreamManager() {
    for (auto stream : streams_) {
        if (stream != nullptr) {
            cudaStreamDestroy(stream);
        }
    }
}

CudaStreamManager::CudaStreamManager(CudaStreamManager&& other) noexcept
    : streams_(std::move(other.streams_)) {
    other.streams_.clear();
}

CudaStreamManager& CudaStreamManager::operator=(CudaStreamManager&& other) noexcept {
    if (this != &other) {
        // 销毁当前流
        for (auto stream : streams_) {
            if (stream != nullptr) {
                cudaStreamDestroy(stream);
            }
        }
        
        // 移动资源
        streams_ = std::move(other.streams_);
        other.streams_.clear();
    }
    return *this;
}

cudaStream_t CudaStreamManager::getStream(int index) const {
    if (index < 0 || index >= static_cast<int>(streams_.size())) {
        std::ostringstream oss;
        oss << "Stream index " << index << " out of range [0, " << streams_.size() << ")";
        throw std::out_of_range(oss.str());
    }
    return streams_[index];
}

void CudaStreamManager::synchronizeAll() {
    for (size_t i = 0; i < streams_.size(); ++i) {
        cudaError_t err = cudaStreamSynchronize(streams_[i]);
        if (err != cudaSuccess) {
            std::ostringstream oss;
            oss << "Failed to synchronize stream " << i << ": " << cudaGetErrorString(err);
            throw std::runtime_error(oss.str());
        }
    }
}

void CudaStreamManager::synchronizeStream(int index) {
    if (index < 0 || index >= static_cast<int>(streams_.size())) {
        std::ostringstream oss;
        oss << "Stream index " << index << " out of range [0, " << streams_.size() << ")";
        throw std::out_of_range(oss.str());
    }
    
    cudaError_t err = cudaStreamSynchronize(streams_[index]);
    checkCudaError(err, "Failed to synchronize stream");
}

bool CudaStreamManager::allStreamsCompleted() const {
    for (auto stream : streams_) {
        cudaError_t err = cudaStreamQuery(stream);
        if (err == cudaErrorNotReady) {
            return false;
        } else if (err != cudaSuccess) {
            // 其他错误,抛出异常
            std::ostringstream oss;
            oss << "Failed to query stream: " << cudaGetErrorString(err);
            throw std::runtime_error(oss.str());
        }
    }
    return true;
}

bool CudaStreamManager::isStreamCompleted(int index) const {
    if (index < 0 || index >= static_cast<int>(streams_.size())) {
        std::ostringstream oss;
        oss << "Stream index " << index << " out of range [0, " << streams_.size() << ")";
        throw std::out_of_range(oss.str());
    }
    
    cudaError_t err = cudaStreamQuery(streams_[index]);
    if (err == cudaErrorNotReady) {
        return false;
    } else if (err != cudaSuccess) {
        std::ostringstream oss;
        oss << "Failed to query stream: " << cudaGetErrorString(err);
        throw std::runtime_error(oss.str());
    }
    return true;
}

void CudaStreamManager::checkCudaError(cudaError_t err, const char* msg) const {
    if (err != cudaSuccess) {
        std::ostringstream oss;
        oss << msg << ": " << cudaGetErrorString(err);
        throw std::runtime_error(oss.str());
    }
}

// ============================================================================
// CudaStreamGuard Implementation
// ============================================================================

CudaStreamGuard::CudaStreamGuard() : stream_(nullptr) {
    cudaError_t err = cudaStreamCreate(&stream_);
    checkCudaError(err, "Failed to create CUDA stream");
}

CudaStreamGuard::~CudaStreamGuard() {
    if (stream_ != nullptr) {
        cudaStreamDestroy(stream_);
    }
}

CudaStreamGuard::CudaStreamGuard(CudaStreamGuard&& other) noexcept
    : stream_(other.stream_) {
    other.stream_ = nullptr;
}

CudaStreamGuard& CudaStreamGuard::operator=(CudaStreamGuard&& other) noexcept {
    if (this != &other) {
        if (stream_ != nullptr) {
            cudaStreamDestroy(stream_);
        }
        stream_ = other.stream_;
        other.stream_ = nullptr;
    }
    return *this;
}

void CudaStreamGuard::synchronize() {
    cudaError_t err = cudaStreamSynchronize(stream_);
    checkCudaError(err, "Failed to synchronize stream");
}

bool CudaStreamGuard::isCompleted() const {
    cudaError_t err = cudaStreamQuery(stream_);
    if (err == cudaErrorNotReady) {
        return false;
    } else if (err != cudaSuccess) {
        std::ostringstream oss;
        oss << "Failed to query stream: " << cudaGetErrorString(err);
        throw std::runtime_error(oss.str());
    }
    return true;
}

void CudaStreamGuard::checkCudaError(cudaError_t err, const char* msg) const {
    if (err != cudaSuccess) {
        std::ostringstream oss;
        oss << msg << ": " << cudaGetErrorString(err);
        throw std::runtime_error(oss.str());
    }
}

} // namespace utils
} // namespace puzzle71


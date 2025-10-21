/**
 * @file hash_kernel.cu
 * @brief Hash计算和地址比对专用CUDA Kernel（优化寄存器使用）
 * 
 * P0-C002修复：将Hash计算从puzzle71_kernel.cu分离
 * 目标：寄存器使用 ≤40个/线程
 * 
 * 参考：
 * - NVIDIA CUDA Samples: SHA256/RIPEMD160优化
 * - 铁笼协议v5.0: ZERO-TOLERANCE-PERFORMANCE原则
 */

#include "hash_kernel.h"
#include "compare/kernels/hash160_fused.h"
#include "utils/endianness.h"
#include "compute/gpu/device_buffers.h"
#include "compute/gpu/device_results.h"
#include "cudaMath/secp256k1.cuh"

// UNIFIED MODULES: Using existing unified modules for T071 migration
#include "../KeyhuntCore/common/result_emitter.cuh"
#include "../KeyhuntCore/common/hash_utils.cuh"
#include "../KeyhuntCore/common/ecc_operations.cuh"

#include <cuda_runtime.h>

using puzzle71::gpu::DeviceCandidate;
using puzzle71::gpu::DeviceResultBuffer;

namespace {

// 全局结果缓冲区
__device__ DeviceResultBuffer g_result_buffer;

/**
 * @brief finalizeDigest now uses unified implementation from hash_utils.cuh (T038: Updated to camelCase)
 * This eliminates 14 lines of code duplication and ensures consistency
 */

/**
 * @brief emitCandidate now uses unified implementation from result_emitter.cuh (T038: Updated to camelCase)
 * This eliminates 60 lines of code duplication and ensures consistency
 *
 * The unified implementation combines the best features from both original versions:
 * - Advanced overflow detection (from puzzle71_kernel.cu)
 * - Optimized warp-level communication (from hash_kernel.cu)
 * - Comprehensive result metadata
 * - Streamlined memory access patterns
 */

}  // namespace

namespace puzzle71::kernels {

/**
 * @brief Hash计算和地址比对Kernel（寄存器优化版）
 * 
 * 功能：
 * 1. 读取ECC计算后的公钥坐标
 * 2. 计算Hash160（SHA256 + RIPEMD160）
 * 3. 比对目标地址
 * 4. 发射匹配的候选
 * 
 * 寄存器使用分析：
 * - x[8], y[8]: 16个寄存器
 * - digest[5]: 5个寄存器
 * - SHA256状态: ~10个寄存器（优化后）
 * - RIPEMD160状态: ~10个寄存器（优化后）
 * - 其他变量: ~5个寄存器
 * 总计: ~40个寄存器 ✅
 * 
 * @param pointsPerThread 每个线程处理的点数量
 * @param compression 压缩类型（UNCOMPRESSED/COMPRESSED/BOTH）
 * @param xPtr X坐标数组指针
 * @param yPtr Y坐标数组指针
 */
__global__ void __launch_bounds__(256) HashKernel(
    int pointsPerThread,
    int compression
) {
    // 在device代码中获取指针
    unsigned int* xPtr = ec::getXPtr();
    unsigned int* yPtr = ec::getYPtr();

    const bool check_uncompressed =
        (compression == PointCompressionType::UNCOMPRESSED) ||
        (compression == PointCompressionType::BOTH);
    const bool check_compressed =
        (compression == PointCompressionType::COMPRESSED) ||
        (compression == PointCompressionType::BOTH);

    for (int i = 0; i < pointsPerThread; ++i) {
        unsigned int x[8];
        keyhunt::common::ReadBigInt(xPtr, i, x);

        // 检查未压缩地址 - Using unified hash operations (T071)
        if (check_uncompressed) {
            unsigned int y[8];
            std::uint32_t digest[5]{};

            keyhunt::common::ReadBigInt(yPtr, i, y);
            puzzle71::compare::Hash160Uncompressed(x, y, digest);

            bool match = puzzle71::compare::HashMatchesTarget(digest);
            keyhunt::common::emitCandidate(match, i, false, x, y, digest);
        }

        // 检查压缩地址 - Using unified hash operations (T071)
        if (check_compressed) {
            std::uint32_t digest[5]{};
            unsigned int y_parity = keyhunt::common::ReadLSW(yPtr, i);

            puzzle71::compare::Hash160Compressed(x, y_parity, digest);

            bool match = puzzle71::compare::HashMatchesTarget(digest);

            // 只有匹配时才读取完整的Y坐标
            unsigned int y_full[8]{};
            if (match) {
                keyhunt::common::ReadBigInt(yPtr, i, y_full);
            }

            keyhunt::common::emitCandidate(match, i, true, x, y_full, digest);
        }
    }
}

/**
 * @brief 启动Hash Kernel的辅助函数
 * 
 * @param gridDim Grid维度
 * @param blockDim Block维度
 * @param pointsPerThread 每个线程处理的点数量
 * @param compression 压缩类型
 * @param xPtr X坐标数组指针
 * @param yPtr Y坐标数组指针
 * @param stream CUDA流（可选）
 * @return cudaError_t 错误码
 */
cudaError_t LaunchHashKernel(
    dim3 gridDim,
    dim3 blockDim,
    int pointsPerThread,
    int compression,
    cudaStream_t stream
) {
    if (stream == nullptr) {
        HashKernel<<<gridDim, blockDim>>>(pointsPerThread, compression);
    } else {
        HashKernel<<<gridDim, blockDim, 0, stream>>>(pointsPerThread, compression);
    }

    return cudaGetLastError();
}

/**
 * @brief 设置结果缓冲区
 * 
 * @param buffer 结果缓冲区指针
 * @return cudaError_t 错误码
 */
cudaError_t SetResultBuffer(const DeviceResultBuffer& buffer) {
    return cudaMemcpyToSymbol(
        g_result_buffer,
        &buffer,
        sizeof(DeviceResultBuffer)
    );
}

}  // namespace puzzle71::kernels


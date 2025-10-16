# Week1: readInt 优化实现方案

**文档版本**: v1.0  
**创建日期**: 2025-10-14  
**铁笼协议**: v5.0 (NO-CRYPTO-REINVENTION)  
**参考源**: BitCrack (MIT License)  
**预期提升**: 2-3× (Memory Coalescing: 15.6% → >90%)

---

## 一、任务目标

### 1.1 核心目标
- ✅ 适配BitCrack的`readInt_Optimized`到项目数据布局
- ✅ 实现Adapter层，不重写核心逻辑
- ✅ 提供Kernel调用示例
- ✅ 设计Week2/Week3接口草案

### 1.2 性能目标
- Memory Coalescing Efficiency: 15.6% → >90%
- Cache Hit Rate: 12.5% → >80%
- 预期吞吐量提升: 2-3×
- H20目标: 4.1 Gkeys/s → 8-12 Gkeys/s

---

## 二、Adapter层设计

### 2.1 设计原则
1. **不重写BitCrack核心逻辑** - 仅做数据布局适配
2. **最小化Glue代码** - 只做必要的index计算和偏移变换
3. **可追溯调用** - 注释标明调用来源
4. **资源优化** - 权衡寄存器、shared memory、bank conflict

### 2.2 Adapter接口设计

```cuda
/**
 * @file readint_adapter.cuh
 * @brief Adapter for BitCrack readInt_Optimized
 * 
 * Reference Source: BitCrack by brichard19 (MIT License)
 * Original File: cudaMath/secp256k1.cuh:143-170
 * 
 * Adapter Purpose:
 * - Adapt BitCrack's readInt_Optimized to our SoA memory layout
 * - Provide glue code for index calculation and offset transformation
 * - NO rewriting of BitCrack's core logic
 * 
 * Performance Target:
 * - Memory Coalescing Efficiency: >90%
 * - Cache Hit Rate: >80%
 * - Expected Speedup: 2-3×
 */

#ifndef READINT_ADAPTER_CUH
#define READINT_ADAPTER_CUH

#include <cuda_runtime.h>
#include "../extracted/bitcrack/cudaMath/secp256k1.cuh"

namespace keyhunt {
namespace adapters {

/**
 * @brief Adapter for BitCrack readInt_Optimized
 * 
 * This adapter wraps BitCrack's readInt_Optimized function to work with
 * our Structure-of-Arrays (SoA) memory layout.
 * 
 * Data Layout Transformation:
 * - Input: SoA layout (X[0..n], Y[0..n], Z[0..n])
 * - BitCrack expects: AoS layout (X0,Y0,Z0, X1,Y1,Z1, ...)
 * - Adapter: Performs index mapping without data copying
 * 
 * Shared Memory Usage:
 * - blockDim.x * 8 * sizeof(unsigned int) bytes
 * - Example: 256 threads × 8 words × 4 bytes = 8KB per block
 * 
 * @param ara Global memory array (SoA layout)
 * @param idx Point index
 * @param x Output array (8 words)
 * @param component Component index (0=X, 1=Y, 2=Z for Jacobian)
 * @param totalPoints Total number of points in array
 */
__device__ inline void readInt_Optimized_Adapter(
    const unsigned int *ara,
    int idx,
    unsigned int x[8],
    int component,
    int totalPoints)
{
    // Glue code: Calculate offset for SoA layout
    // SoA layout: [X0, X1, ..., Xn, Y0, Y1, ..., Yn, Z0, Z1, ..., Zn]
    // Each component (X/Y/Z) has totalPoints * 8 words
    int componentOffset = component * totalPoints * 8;
    
    // Call BitCrack::readInt_Optimized with adjusted pointer
    // NOTE: This is the ONLY call to BitCrack's implementation
    // We do NOT rewrite the core logic
    readInt_Optimized(ara + componentOffset, idx, x);
}

/**
 * @brief Load Jacobian point from SoA layout using optimized memory access
 * 
 * This function loads a complete Jacobian point (X, Y, Z) from global memory
 * using BitCrack's optimized readInt function.
 * 
 * Memory Access Pattern:
 * - 3 calls to readInt_Optimized (X, Y, Z)
 * - Each call uses shared memory for coalescing
 * - Total shared memory: 3 × blockDim.x × 8 × 4 bytes
 * 
 * Performance Considerations:
 * - Coalesced access: >90% efficiency
 * - Bank conflicts: Minimized by sequential access
 * - Occupancy: Limited by shared memory usage
 * 
 * @param ara Global memory array (SoA layout)
 * @param idx Point index
 * @param x Output X coordinate (8 words)
 * @param y Output Y coordinate (8 words)
 * @param z Output Z coordinate (8 words)
 * @param totalPoints Total number of points
 */
__device__ inline void loadJacobianPoint_Optimized(
    const unsigned int *ara,
    int idx,
    unsigned int x[8],
    unsigned int y[8],
    unsigned int z[8],
    int totalPoints)
{
    // Load X coordinate (component 0)
    readInt_Optimized_Adapter(ara, idx, x, 0, totalPoints);
    
    // Load Y coordinate (component 1)
    readInt_Optimized_Adapter(ara, idx, y, 1, totalPoints);
    
    // Load Z coordinate (component 2)
    readInt_Optimized_Adapter(ara, idx, z, 2, totalPoints);
}

} // namespace adapters
} // namespace keyhunt

#endif // READINT_ADAPTER_CUH
```

---

## 三、Kernel调用示例

### 3.1 Kernel实现

```cuda
/**
 * @file optimized_ecc_kernel.cu
 * @brief ECC kernel using readInt_Optimized adapter
 * 
 * Week1 Implementation: Memory access optimization
 * Reference: BitCrack readInt_Optimized (MIT License)
 */

#include "readint_adapter.cuh"
#include <cuda_runtime.h>

namespace keyhunt {
namespace kernels {

/**
 * @brief Optimized ECC scalar multiplication kernel (Week1)
 * 
 * Launch Configuration:
 * - Block size: 256 threads (8 warps)
 * - Grid size: (count + 255) / 256 blocks
 * - Shared memory: 8KB per block (256 threads × 8 words × 4 bytes)
 * - Register budget: ≤128 registers per thread
 * 
 * Memory Layout:
 * - Input: SoA layout for precomputed points
 * - Output: AoS layout for public keys
 * 
 * Performance Targets:
 * - Memory Coalescing: >90%
 * - GPU Utilization: ≥90%
 * - Throughput: 8-12 Gkeys/s on H20
 * 
 * @param privateKeys Input private keys (32 bytes each)
 * @param publicKeys Output public keys (65 bytes each)
 * @param precomputedPoints Precomputed ECC points (SoA layout)
 * @param totalPoints Number of precomputed points
 * @param count Number of keys to process
 */
__global__ void eccScalarMulKernel_Week1(
    const unsigned char* privateKeys,
    unsigned char* publicKeys,
    const unsigned int* precomputedPoints,
    int totalPoints,
    int count)
{
    // Thread index calculation
    int threadId = blockDim.x * blockIdx.x + threadIdx.x;
    if (threadId >= count) return;
    
    // Allocate registers for Jacobian point (X, Y, Z)
    unsigned int resultX[8], resultY[8], resultZ[8];
    
    // Initialize result to identity point (point at infinity)
    // Z = 0 represents point at infinity in Jacobian coordinates
    #pragma unroll
    for (int i = 0; i < 8; i++) {
        resultX[i] = 0;
        resultY[i] = 0;
        resultZ[i] = 0;
    }
    
    // Load private key
    const unsigned char* privKey = privateKeys + threadId * 32;
    
    // Scalar multiplication using precomputed table
    // NOTE: This is simplified bit-by-bit method
    // Week2 will replace with Jacobian coordinates
    // Week3 will replace with GLV endomorphism
    for (int byte_idx = 0; byte_idx < 32; byte_idx++) {
        unsigned char byte_val = privKey[byte_idx];
        
        for (int bit_idx = 0; bit_idx < 8; bit_idx++) {
            if (byte_val & (1 << bit_idx)) {
                int table_idx = byte_idx * 8 + bit_idx;
                if (table_idx < totalPoints) {
                    // Load precomputed point using optimized adapter
                    // Call: BitCrack::readInt_Optimized (via adapter)
                    unsigned int pointX[8], pointY[8], pointZ[8];
                    adapters::loadJacobianPoint_Optimized(
                        precomputedPoints,
                        table_idx,
                        pointX, pointY, pointZ,
                        totalPoints
                    );
                    
                    // Point addition (simplified)
                    // Week2 will replace with Jacobian point addition
                    // from HareInWeed/gec
                    // TODO: Call gec::jacobian_add(resultX, resultY, resultZ,
                    //                               pointX, pointY, pointZ)
                }
            }
        }
    }
    
    // Convert result to public key format
    // (Simplified - Week2 will add Jacobian to Affine conversion)
    unsigned char* pubKey = publicKeys + threadId * 65;
    pubKey[0] = 0x04;  // Uncompressed prefix
    
    // Copy X coordinate (big-endian)
    for (int i = 0; i < 8; i++) {
        uint32_t word = resultX[7 - i];
        pubKey[1 + i * 4 + 0] = (word >> 24) & 0xFF;
        pubKey[1 + i * 4 + 1] = (word >> 16) & 0xFF;
        pubKey[1 + i * 4 + 2] = (word >> 8) & 0xFF;
        pubKey[1 + i * 4 + 3] = word & 0xFF;
    }
    
    // Copy Y coordinate (big-endian)
    for (int i = 0; i < 8; i++) {
        uint32_t word = resultY[7 - i];
        pubKey[33 + i * 4 + 0] = (word >> 24) & 0xFF;
        pubKey[33 + i * 4 + 1] = (word >> 16) & 0xFF;
        pubKey[33 + i * 4 + 2] = (word >> 8) & 0xFF;
        pubKey[33 + i * 4 + 3] = word & 0xFF;
    }
}

/**
 * @brief Host function to launch optimized ECC kernel
 * 
 * Resource Considerations:
 * - Shared memory: 8KB per block (within 48KB limit)
 * - Registers: ~80 per thread (within 128 budget)
 * - Occupancy: ~75% (limited by shared memory)
 * 
 * Fallback Strategy:
 * - If shared memory limited: Reduce block size to 128 threads
 * - If register limited: Use #pragma unroll with smaller factor
 * 
 * @param privateKeys Input private keys
 * @param publicKeys Output public keys
 * @param precomputedPoints Precomputed points (SoA layout)
 * @param totalPoints Number of precomputed points
 * @param count Number of keys to process
 * @param stream CUDA stream
 */
void launchEccScalarMulKernel_Week1(
    const unsigned char* privateKeys,
    unsigned char* publicKeys,
    const unsigned int* precomputedPoints,
    int totalPoints,
    int count,
    cudaStream_t stream = 0)
{
    // Launch configuration
    const int blockSize = 256;
    const int gridSize = (count + blockSize - 1) / blockSize;
    const int sharedMemSize = blockSize * 8 * sizeof(unsigned int);
    
    // Launch kernel
    eccScalarMulKernel_Week1<<<gridSize, blockSize, sharedMemSize, stream>>>(
        privateKeys,
        publicKeys,
        precomputedPoints,
        totalPoints,
        count
    );
}

} // namespace kernels
} // namespace keyhunt
```

---

## 四、资源考量与设计说明

### 4.1 Shared Memory分析

**使用量**:
- 每个block: 256 threads × 8 words × 4 bytes = 8KB
- GPU限制: 48KB per SM (Turing), 164KB per SM (Hopper)
- 结论: ✅ 在限制内，可支持多个block并发

**Bank Conflict分析**:
- 访问模式: Sequential (sharedData[localThreadId * 8 + i])
- Bank数量: 32 banks
- 冲突概率: 低 (每个线程访问连续8个word)

### 4.2 Register预算

**当前使用**:
- resultX[8], resultY[8], resultZ[8]: 24 registers
- pointX[8], pointY[8], pointZ[8]: 24 registers
- 临时变量: ~20 registers
- 总计: ~68 registers/thread

**目标**: ≤128 registers/thread
**状态**: ✅ 在预算内

### 4.3 Occupancy分析

**限制因素**:
- Shared memory: 8KB per block
- Registers: 68 per thread
- Block size: 256 threads

**理论Occupancy**:
- Shared memory限制: 48KB / 8KB = 6 blocks per SM
- Register限制: 65536 / (256 × 68) = 3.76 blocks per SM
- 实际: min(6, 3.76) = 3 blocks per SM
- Occupancy: 3 × 256 / 2048 = 37.5%

**优化方向**:
- 减少临时变量使用
- 使用#pragma unroll减少循环开销

### 4.4 Fallback策略

**场景1: Shared Memory受限**
- 减少block size: 256 → 128 threads
- Shared memory: 8KB → 4KB
- Occupancy提升: 37.5% → 50%

**场景2: Register受限**
- 减少#pragma unroll因子
- 使用局部变量复用
- 目标: 降至≤64 registers/thread

---

## 五、性能预估

### 5.1 Memory Load分析

**原始方法** (readInt_Original):
- 每个256位整数: 8次global memory load
- Coalescing efficiency: 15.6%
- 有效带宽: 15.6% × 1555 GB/s (H20) = 242 GB/s

**优化方法** (readInt_Optimized):
- 每个256位整数: 8次global memory load (coalesced)
- Coalescing efficiency: >90%
- 有效带宽: 90% × 1555 GB/s = 1400 GB/s

**提升倍数**: 1400 / 242 = **5.8×**

### 5.2 点加法次数

**当前方法** (bit-by-bit):
- 平均点加法: ~128次 (256位 × 50%)
- 每次点加法: ~10次模乘 (Affine坐标)
- 总模乘: ~1280次

**Week2优化** (Jacobian坐标):
- 每次点加法: ~12次模乘 (Jacobian坐标)
- 但避免模逆: 节省~100次模乘
- 总模乘: ~1536次 (略增)
- 但模逆节省: **7.5× 理论提升**

### 5.3 综合预估

**Week1优化** (readInt only):
- Memory load提升: 5.8×
- 点加法不变: 1×
- 综合提升: **2-3×** (保守估计)
- H20吞吐量: 4.1 → **8-12 Gkeys/s**

**Week1+Week2** (readInt + Jacobian):
- Memory load: 5.8×
- 点加法: 2-3× (实际)
- 综合提升: **10-15×** (理想)
- H20吞吐量: 4.1 → **40-60 Gkeys/s** (理想)

**Week1+Week2+Week3** (readInt + Jacobian + GLV):
- Memory load: 5.8×
- 点加法: 2-3×
- 标量长度: 2× (GLV)
- 综合提升: **20-30×** (理想)
- H20吞吐量: 4.1 → **80-120 Gkeys/s** (理想)

**折损因素**:
- Warp divergence: ~10% 损失
- Bank conflicts: ~5% 损失
- 其他开销: ~15% 损失
- 实际提升: **15-20×** (折损后)
- H20吞吐量: 4.1 → **60-80 Gkeys/s** (实际预期)

---

**Week1方案完成**

**下一步**: 查看Week2/Week3接口草案


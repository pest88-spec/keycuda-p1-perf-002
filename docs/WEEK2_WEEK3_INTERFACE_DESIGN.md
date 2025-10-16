# Week2/Week3 接口设计草案

**文档版本**: v1.0  
**创建日期**: 2025-10-14  
**铁笼协议**: v5.0 (NO-CRYPTO-REINVENTION)  
**目的**: 设计后续模块的调用/适配接口

---

## 一、Week2: Jacobian坐标系统

### 1.1 参考源
- **主要参考**: HareInWeed/gec (MIT License)
- **文件**: `include/gec/curve/jacobian_curve.hpp`
- **备选参考**: bitcoin-core/secp256k1, gECC

### 1.2 核心功能

**Jacobian坐标表示**:
```
Affine: (x, y)
Jacobian: (X, Y, Z) where x = X/Z², y = Y/Z³
```

**优势**:
- 避免模逆运算 (最昂贵的操作)
- 点加法: 12次模乘 vs Affine的10次模乘+1次模逆
- 理论提升: 7.5× (模逆 ≈ 80次模乘)

### 1.3 接口设计

```cuda
/**
 * @file jacobian_adapter.cuh
 * @brief Adapter for HareInWeed/gec Jacobian coordinates
 * 
 * Reference Source: HareInWeed/gec (MIT License)
 * Original File: include/gec/curve/jacobian_curve.hpp
 * 
 * Adapter Purpose:
 * - Adapt gec's Jacobian point operations to CUDA
 * - Provide GPU-optimized wrappers
 * - NO rewriting of core Jacobian math
 */

#ifndef JACOBIAN_ADAPTER_CUH
#define JACOBIAN_ADAPTER_CUH

#include <cuda_runtime.h>

namespace keyhunt {
namespace adapters {

/**
 * @brief Jacobian point structure (256-bit coordinates)
 * 
 * Represents point (X, Y, Z) on secp256k1 curve
 * Affine coordinates: x = X/Z², y = Y/Z³
 */
struct JacobianPoint {
    unsigned int X[8];  // X coordinate (256-bit)
    unsigned int Y[8];  // Y coordinate (256-bit)
    unsigned int Z[8];  // Z coordinate (256-bit)
};

/**
 * @brief Jacobian point addition: R = P + Q
 * 
 * Reference: HareInWeed/gec::jacobian_add
 * Algorithm: Standard Jacobian addition formula
 * 
 * Cost: 12 modular multiplications + 4 modular squarings
 * 
 * Formula:
 *   U1 = X1*Z2², U2 = X2*Z1²
 *   S1 = Y1*Z2³, S2 = Y2*Z1³
 *   H = U2 - U1, R = S2 - S1
 *   X3 = R² - H³ - 2*U1*H²
 *   Y3 = R*(U1*H² - X3) - S1*H³
 *   Z3 = Z1*Z2*H
 * 
 * @param result Output point R = P + Q
 * @param P First input point
 * @param Q Second input point
 */
__device__ void jacobian_add(
    JacobianPoint& result,
    const JacobianPoint& P,
    const JacobianPoint& Q);

/**
 * @brief Jacobian point doubling: R = 2*P
 * 
 * Reference: HareInWeed/gec::jacobian_double
 * Algorithm: Standard Jacobian doubling formula
 * 
 * Cost: 4 modular multiplications + 6 modular squarings
 * 
 * Formula:
 *   S = 4*X*Y²
 *   M = 3*X² + a*Z⁴ (a=0 for secp256k1)
 *   X' = M² - 2*S
 *   Y' = M*(S - X') - 8*Y⁴
 *   Z' = 2*Y*Z
 * 
 * @param result Output point R = 2*P
 * @param P Input point
 */
__device__ void jacobian_double(
    JacobianPoint& result,
    const JacobianPoint& P);

/**
 * @brief Convert Jacobian to Affine coordinates
 * 
 * Reference: HareInWeed/gec::jacobian_to_affine
 * 
 * Cost: 1 modular inverse + 3 modular multiplications
 * 
 * Formula:
 *   z_inv = Z^(-1) mod p
 *   z_inv_sq = z_inv²
 *   x = X * z_inv_sq
 *   y = Y * z_inv_sq * z_inv
 * 
 * @param x Output affine x coordinate
 * @param y Output affine y coordinate
 * @param P Input Jacobian point
 */
__device__ void jacobian_to_affine(
    unsigned int x[8],
    unsigned int y[8],
    const JacobianPoint& P);

/**
 * @brief Scalar multiplication using Jacobian coordinates
 * 
 * Reference: HareInWeed/gec::scalar_mul
 * Algorithm: Double-and-add with Jacobian coordinates
 * 
 * Pseudocode:
 *   R = O (point at infinity)
 *   for i from 255 down to 0:
 *     R = 2*R (jacobian_double)
 *     if bit i of k is 1:
 *       R = R + G (jacobian_add)
 *   return R
 * 
 * Cost: 256 doublings + ~128 additions (average)
 * 
 * @param result Output point R = k*G
 * @param k Scalar (256-bit)
 * @param G Base point (Jacobian)
 */
__device__ void jacobian_scalar_mul(
    JacobianPoint& result,
    const unsigned int k[8],
    const JacobianPoint& G);

} // namespace adapters
} // namespace keyhunt

#endif // JACOBIAN_ADAPTER_CUH
```

### 1.4 Kernel集成示例

```cuda
/**
 * @brief ECC kernel using Jacobian coordinates (Week2)
 */
__global__ void eccScalarMulKernel_Week2(
    const unsigned char* privateKeys,
    unsigned char* publicKeys,
    const unsigned int* basePointG,  // Jacobian base point
    int count)
{
    int threadId = blockDim.x * blockIdx.x + threadIdx.x;
    if (threadId >= count) return;
    
    // Load private key
    unsigned int k[8];
    const unsigned char* privKey = privateKeys + threadId * 32;
    for (int i = 0; i < 8; i++) {
        k[i] = (privKey[i*4+0] << 24) |
               (privKey[i*4+1] << 16) |
               (privKey[i*4+2] << 8) |
               (privKey[i*4+3]);
    }
    
    // Load base point G (Jacobian)
    adapters::JacobianPoint G;
    // Call: BitCrack::readInt_Optimized (via Week1 adapter)
    adapters::loadJacobianPoint_Optimized(
        basePointG, 0, G.X, G.Y, G.Z, 1);
    
    // Scalar multiplication: R = k*G
    // Call: HareInWeed/gec::jacobian_scalar_mul
    adapters::JacobianPoint result;
    adapters::jacobian_scalar_mul(result, k, G);
    
    // Convert to Affine coordinates
    // Call: HareInWeed/gec::jacobian_to_affine
    unsigned int x[8], y[8];
    adapters::jacobian_to_affine(x, y, result);
    
    // Output public key
    unsigned char* pubKey = publicKeys + threadId * 65;
    pubKey[0] = 0x04;
    for (int i = 0; i < 8; i++) {
        pubKey[1 + i*4 + 0] = (x[7-i] >> 24) & 0xFF;
        pubKey[1 + i*4 + 1] = (x[7-i] >> 16) & 0xFF;
        pubKey[1 + i*4 + 2] = (x[7-i] >> 8) & 0xFF;
        pubKey[1 + i*4 + 3] = x[7-i] & 0xFF;
    }
    for (int i = 0; i < 8; i++) {
        pubKey[33 + i*4 + 0] = (y[7-i] >> 24) & 0xFF;
        pubKey[33 + i*4 + 1] = (y[7-i] >> 16) & 0xFF;
        pubKey[33 + i*4 + 2] = (y[7-i] >> 8) & 0xFF;
        pubKey[33 + i*4 + 3] = y[7-i] & 0xFF;
    }
}
```

---

## 二、Week3: GLV Endomorphism

### 2.1 参考源
- **主要参考**: secp256k1-zkp (MIT License)
- **文件**: `src/scalar_impl.h:407-450`
- **备选参考**: VanitySearch, gECC

### 2.2 核心原理

**GLV Endomorphism**:
```
secp256k1曲线具有高效endomorphism φ:
φ(x, y) = (β*x, y) where β³ ≡ 1 (mod p)

性质: φ(P) = λ*P where λ² + λ + 1 ≡ 0 (mod n)

标量分解: k = k1 + k2*λ (mod n)
其中 k1, k2 是128位标量

计算: k*P = k1*P + k2*φ(P)
```

**优势**:
- 标量长度减半: 256位 → 128位
- 迭代次数减半: 256次 → 128次
- 理论提升: 1.5-2×

### 2.3 接口设计

```cuda
/**
 * @file glv_adapter.cuh
 * @brief Adapter for secp256k1-zkp GLV endomorphism
 * 
 * Reference Source: secp256k1-zkp (MIT License)
 * Original File: src/scalar_impl.h:407-450
 * 
 * Adapter Purpose:
 * - Adapt secp256k1-zkp's scalar_split_lambda to CUDA
 * - Provide GPU-optimized GLV decomposition
 * - NO rewriting of core GLV math
 */

#ifndef GLV_ADAPTER_CUH
#define GLV_ADAPTER_CUH

#include <cuda_runtime.h>
#include "jacobian_adapter.cuh"

namespace keyhunt {
namespace adapters {

/**
 * @brief GLV scalar decomposition: k = k1 + k2*λ
 * 
 * Reference: secp256k1-zkp::scalar_split_lambda
 * Algorithm: Lattice-based scalar decomposition
 * 
 * Constants:
 *   λ = 0x5363ad4cc05c30e0a5261c028812645a122e22ea20816678df02967c1b23bd72
 *   β = 0x7ae96a2b657c07106e64479eac3434e99cf0497512f58995c1396c28719501ee
 * 
 * Properties:
 *   |k1|, |k2| ≤ √n ≈ 2^128
 *   k ≡ k1 + k2*λ (mod n)
 * 
 * @param k1 Output: First 128-bit scalar
 * @param k2 Output: Second 128-bit scalar
 * @param k Input: 256-bit scalar
 */
__device__ void glv_scalar_split(
    unsigned int k1[4],  // 128-bit
    unsigned int k2[4],  // 128-bit
    const unsigned int k[8]);  // 256-bit

/**
 * @brief Apply endomorphism: φ(P) = (β*x, y)
 * 
 * Reference: secp256k1-zkp::ge_mul_lambda
 * 
 * Formula:
 *   φ(x, y) = (β*x mod p, y)
 *   where β = 0x7ae96a2b657c07106e64479eac3434e99cf0497512f58995c1396c28719501ee
 * 
 * @param result Output: φ(P)
 * @param P Input: Jacobian point
 */
__device__ void glv_apply_endomorphism(
    JacobianPoint& result,
    const JacobianPoint& P);

/**
 * @brief GLV-based scalar multiplication: R = k*P
 * 
 * Reference: secp256k1-zkp::ecmult
 * Algorithm: Simultaneous double-and-add with GLV
 * 
 * Pseudocode:
 *   (k1, k2) = glv_scalar_split(k)
 *   Q = φ(P)  // Apply endomorphism
 *   R = O
 *   for i from 127 down to 0:
 *     R = 2*R
 *     if bit i of k1 is 1: R = R + P
 *     if bit i of k2 is 1: R = R + Q
 *   return R
 * 
 * Cost: 128 doublings + ~128 additions (vs 256 doublings)
 * Expected speedup: 1.5-2×
 * 
 * @param result Output: R = k*P
 * @param k Scalar (256-bit)
 * @param P Base point (Jacobian)
 */
__device__ void glv_scalar_mul(
    JacobianPoint& result,
    const unsigned int k[8],
    const JacobianPoint& P);

} // namespace adapters
} // namespace keyhunt

#endif // GLV_ADAPTER_CUH
```

### 2.4 Kernel集成示例

```cuda
/**
 * @brief ECC kernel using GLV endomorphism (Week3)
 */
__global__ void eccScalarMulKernel_Week3(
    const unsigned char* privateKeys,
    unsigned char* publicKeys,
    const unsigned int* basePointG,
    int count)
{
    int threadId = blockDim.x * blockIdx.x + threadIdx.x;
    if (threadId >= count) return;
    
    // Load private key
    unsigned int k[8];
    const unsigned char* privKey = privateKeys + threadId * 32;
    for (int i = 0; i < 8; i++) {
        k[i] = (privKey[i*4+0] << 24) |
               (privKey[i*4+1] << 16) |
               (privKey[i*4+2] << 8) |
               (privKey[i*4+3]);
    }
    
    // Load base point G (Jacobian)
    // Call: BitCrack::readInt_Optimized (Week1)
    adapters::JacobianPoint G;
    adapters::loadJacobianPoint_Optimized(
        basePointG, 0, G.X, G.Y, G.Z, 1);
    
    // GLV scalar multiplication: R = k*G
    // Call: secp256k1-zkp::glv_scalar_mul
    // Internally calls:
    //   - secp256k1-zkp::scalar_split_lambda
    //   - HareInWeed/gec::jacobian_add
    //   - HareInWeed/gec::jacobian_double
    adapters::JacobianPoint result;
    adapters::glv_scalar_mul(result, k, G);
    
    // Convert to Affine
    // Call: HareInWeed/gec::jacobian_to_affine (Week2)
    unsigned int x[8], y[8];
    adapters::jacobian_to_affine(x, y, result);
    
    // Output public key (same as Week2)
    unsigned char* pubKey = publicKeys + threadId * 65;
    pubKey[0] = 0x04;
    // ... (same as Week2)
}
```

---

## 三、模块组合调用方案

### 3.1 调用层次结构

```
Week3 Kernel (最终版本)
├── Week1: readInt_Optimized (BitCrack)
│   └── 用于加载预计算点和基点
├── Week2: Jacobian坐标 (HareInWeed/gec)
│   ├── jacobian_add (点加法)
│   ├── jacobian_double (点倍)
│   └── jacobian_to_affine (坐标转换)
└── Week3: GLV Endomorphism (secp256k1-zkp)
    ├── glv_scalar_split (标量分解)
    ├── glv_apply_endomorphism (endomorphism应用)
    └── glv_scalar_mul (GLV标量乘法)
        ├── 调用 jacobian_add (Week2)
        └── 调用 jacobian_double (Week2)
```

### 3.2 数据流

```
Input: Private Key (256-bit)
  ↓
Week3: GLV Scalar Split
  ↓
k1 (128-bit), k2 (128-bit)
  ↓
Week1: Load Base Point G (readInt_Optimized)
  ↓
G (Jacobian)
  ↓
Week3: Apply Endomorphism → Q = φ(G)
  ↓
Week2: Simultaneous Scalar Mul
  R = k1*G + k2*Q (using jacobian_add/double)
  ↓
Week2: Jacobian to Affine
  ↓
Output: Public Key (x, y)
```

### 3.3 性能组合预估

| 阶段 | 优化 | 提升倍数 | 累积提升 |
|------|------|---------|---------|
| Baseline | - | 1× | 1× |
| Week1 | readInt_Optimized | 2-3× | 2-3× |
| Week2 | Jacobian坐标 | 2-3× | 4-9× |
| Week3 | GLV Endomorphism | 1.5-2× | 6-18× |

**保守估计**: 10-15× (考虑折损)
**H20吞吐量**: 4.1 Gkeys/s → **40-60 Gkeys/s**

---

## 四、实现优先级与依赖关系

### 4.1 实现顺序

1. **Week1** (独立模块)
   - 实现readInt_Optimized adapter
   - 测试memory coalescing效率
   - 验证性能提升 (2-3×)

2. **Week2** (依赖Week1)
   - 实现Jacobian adapter
   - 集成Week1的readInt_Optimized
   - 测试点加法/点倍性能
   - 验证CPU-GPU一致性

3. **Week3** (依赖Week1+Week2)
   - 实现GLV adapter
   - 集成Week1+Week2
   - 测试GLV标量分解
   - 验证最终性能 (10-15×)

### 4.2 测试策略

**Week1测试**:
- Memory coalescing efficiency ≥90%
- Cache hit rate ≥80%
- 吞吐量提升 2-3×

**Week2测试**:
- CPU-GPU一致性 (vs bitcoin-core/secp256k1)
- 点加法/点倍正确性
- 吞吐量提升 4-9× (累积)

**Week3测试**:
- GLV标量分解正确性
- CPU-GPU一致性 (vs secp256k1-zkp)
- 吞吐量提升 10-15× (累积)

---

## 五、资源预算

### 5.1 Shared Memory

| 模块 | 使用量 | 说明 |
|------|--------|------|
| Week1 | 8KB | readInt_Optimized缓存 |
| Week2 | 0KB | 使用寄存器 |
| Week3 | 0KB | 使用寄存器 |
| **总计** | **8KB** | 在48KB限制内 ✅ |

### 5.2 Register预算

| 模块 | 寄存器数 | 说明 |
|------|---------|------|
| Week1 | 20 | 临时变量 |
| Week2 | 48 | Jacobian点 (3×8×2) |
| Week3 | 24 | GLV分解 (k1, k2) |
| **总计** | **92** | 在128限制内 ✅ |

### 5.3 Occupancy预估

- Shared memory限制: 48KB / 8KB = 6 blocks/SM
- Register限制: 65536 / (256 × 92) = 2.78 blocks/SM
- 实际: min(6, 2.78) = 2 blocks/SM
- Occupancy: 2 × 256 / 2048 = **25%**

**优化方向**:
- 减少寄存器使用 (目标: ≤64/thread)
- 提升occupancy至50%+

---

**Week2/Week3接口设计完成**

**下一步**: 生成完整执行计划


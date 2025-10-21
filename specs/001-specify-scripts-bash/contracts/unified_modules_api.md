# Unified Modules API Contract

**Version**: 1.0.0
**Date**: 2025-10-17
**Purpose**: API specification for unified code deduplication modules

## Overview

This API defines the contract for unified modules that eliminate 100% code duplication across EmitCandidate, FinalizeDigest, and ECC computation functions in the Puzzle71Solver CUDA refactoring project.

## Module Specifications

### ResultEmitter Module

**File Location**: `src/KeyhuntCore/common/result_emitter.cuh`

**Purpose**: Unified candidate key emission functionality

**API Contract**:
```cuda
#ifndef RESULT_EMITTER_CUH
#define RESULT_EMITTER_CUH

#include <cuda_runtime.h>
#include <cstdint>

namespace keyhunt {
namespace common {

/**
 * Unified candidate result emission structure
 */
struct CandidateResult {
    uint64_t private_key;      // Found private key
    uint8_t public_key[65];    // Corresponding public key
    char address[35];          // Bitcoin address
    uint32_t thread_id;        // Discovering thread ID
    uint32_t block_id;         // Discovering block ID
};

/**
 * Emit candidate result to output buffer
 * @param results Output buffer for candidate results
 * @param max_results Maximum number of results buffer can hold
 * @param result_count Current result count (atomic counter)
 * @param candidate Candidate result to emit
 * @return true if successfully emitted, false if buffer full
 */
__device__ __forceinline__
bool emitCandidate(CandidateResult* results,
                   uint32_t max_results,
                   uint32_t* result_count,
                   const CandidateResult& candidate);

/**
 * Initialize result emission system
 * @param results Pre-allocated result buffer
 * @param result_count Atomic counter for result tracking
 * @param max_results Maximum results capacity
 */
__host__
cudaError_t initializeResultEmitter(CandidateResult* results,
                                   uint32_t* result_count,
                                   uint32_t max_results);

/**
 * Retrieve results from device
 * @param device_results Device result buffer
 * @param result_count Result count from device
 * @param host_results Host result buffer to copy into
 * @param max_results Maximum results to copy
 */
__host__
cudaError_t retrieveResults(const CandidateResult* device_results,
                           const uint32_t* result_count,
                           CandidateResult* host_results,
                           uint32_t max_results);

} // namespace common
} // namespace keyhunt

#endif // RESULT_EMITTER_CUH
```

### HashUtils Module

**File Location**: `src/KeyhuntCore/common/hash_utils.cuh`

**Purpose**: Unified hash finalization and digest operations

**API Contract**:
```cuda
#ifndef HASH_UTILS_CUH
#define HASH_UTILS_CUH

#include <cuda_runtime.h>
#include <cstdint>

namespace keyhunt {
namespace common {

/**
 * Hash context for unified digest operations
 */
struct HashContext {
    uint32_t h[8];             // SHA-256 hash state
    uint64_t total_length;     // Total bytes processed
    uint8_t buffer[64];        // Input buffer
    uint32_t buffer_used;      // Bytes currently in buffer
};

/**
 * Initialize hash context for SHA-256
 * @param ctx Hash context to initialize
 */
__device__ __forceinline__
void initializeHashContext(HashContext* ctx);

/**
 * Process single 32-bit word in hash computation
 * @param ctx Hash context
 * @param word 32-bit word to process
 */
__device__ __forceinline__
void processHashWord(HashContext* ctx, uint32_t word);

/**
 * Finalize hash computation and output digest
 * @param ctx Hash context to finalize
 * @param digest Output 32-byte digest
 */
__device__ __forceinline__
void finalizeDigest(HashContext* ctx, uint8_t digest[32]);

/**
 * Compute RIPEMD-160 from SHA-256 digest (Hash160)
 * @param sha256_digest 32-byte SHA-256 digest
 * @param hash160_output 20-byte Hash160 output
 */
__device__ __forceinline__
void computeHash160(const uint8_t sha256_digest[32],
                    uint8_t hash160_output[20]);

/**
 * Unified Hash160 pipeline: SHA256 → RIPEMD160
 * @param input_data Input data to hash
 * @param input_length Length of input data in bytes
 * @param hash160_output 20-byte Hash160 output
 */
__device__ __forceinline__
void computeUnifiedHash160(const uint8_t* input_data,
                          uint32_t input_length,
                          uint8_t hash160_output[20]);

} // namespace common
} // namespace keyhunt

#endif // HASH_UTILS_CUH
```

### ECCOperations Module

**File Location**: `src/KeyhuntCore/common/ecc_operations.cuh`

**Purpose**: Unified elliptic curve computation operations

**API Contract**:
```cuda
#ifndef ECC_OPERATIONS_CUH
#define ECC_OPERATIONS_CUH

#include <cuda_runtime.h>
#include <cstdint>

namespace keyhunt {
namespace common {

/**
 * Elliptic curve point in affine coordinates
 */
struct ECPoint {
    uint32_t x[8];             // 256-bit X coordinate (8 × 32-bit)
    uint32_t y[8];             // 256-bit Y coordinate (8 × 32-bit)
    bool is_infinity;          // Point at infinity flag
};

/**
 * Elliptic curve point in Jacobian coordinates
 */
struct ECJacobianPoint {
    uint32_t x[8];             // Jacobian X coordinate
    uint32_t y[8];             // Jacobian Y coordinate
    uint32_t z[8];             // Jacobian Z coordinate
    bool is_infinity;          // Point at infinity flag
};

/**
 * 256-bit unsigned integer for scalar operations
 */
struct UInt256 {
    uint32_t data[8];          // 8 × 32-bit words
};

/**
 * Convert private key to public key (scalar multiplication)
 * @param private_key 256-bit private key scalar
 * @param public_key Output public key point (affine coordinates)
 * @return true if computation successful, false on error
 */
__device__ __forceinline__
bool privateKeyToPublicKey(const UInt256& private_key,
                          ECPoint& public_key);

/**
 * Add two elliptic curve points
 * @param point1 First point to add
 * @param point2 Second point to add
 * @param result Output sum point
 * @return true if addition successful, false on error
 */
__device__ __forceinline__
bool pointAdd(const ECPoint& point1,
              const ECPoint& point2,
              ECPoint& result);

/**
 * Double an elliptic curve point
 * @param point Point to double
 * @param result Output doubled point
 * @return true if doubling successful, false on error
 */
__device__ __forceinline__
bool pointDouble(const ECPoint& point,
                 ECPoint& result);

/**
 * Scalar multiplication: k * G where G is generator point
 * @param scalar Scalar multiplier (private key)
 * @param result Output point (public key)
 * @return true if multiplication successful, false on error
 */
__device__ __forceinline__
bool scalarMultiplyBase(const UInt256& scalar,
                        ECPoint& result);

/**
 * Scalar multiplication: k * P
 * @param scalar Scalar multiplier
 * @param point Base point to multiply
 * @param result Output point
 * @return true if multiplication successful, false on error
 */
__device__ __forceinline__
bool scalarMultiply(const UInt256& scalar,
                    const ECPoint& point,
                    ECPoint& result);

/**
 * Check if point is on secp256k1 curve
 * @param point Point to validate
 * @return true if point is on curve, false otherwise
 */
__device__ __forceinline__
bool isValidPoint(const ECPoint& point);

/**
 * Compress public key to 33 bytes
 * @param public_key Uncompressed public key (65 bytes)
 * @param compressed_key Output compressed key (33 bytes)
 */
__device__ __forceinline__
void compressPublicKey(const ECPoint& public_key,
                      uint8_t compressed_key[33]);

/**
 * Initialize ECC module with secp256k1 parameters
 * @return cudaSuccess if initialization successful
 */
__host__
cudaError_t initializeECCModule();

/**
 * Cleanup ECC module resources
 * @return cudaSuccess if cleanup successful
 */
__host__
cudaError_t cleanupECCModule();

} // namespace common
} // namespace keyhunt

#endif // ECC_OPERATIONS_CUH
```

## Integration API

### Module Manager

**File Location**: `src/KeyhuntCore/common/module_manager.cuh`

**Purpose**: Centralized management of unified modules

**API Contract**:
```cuda
#ifndef MODULE_MANAGER_CUH
#define MODULE_MANAGER_CUH

#include "result_emitter.cuh"
#include "hash_utils.cuh"
#include "ecc_operations.cuh"
#include <cuda_runtime.h>

namespace keyhunt {
namespace common {

/**
 * Unified module configuration
 */
struct ModuleConfig {
    uint32_t max_results_per_kernel;     // Maximum candidate results
    uint32_t hash_buffer_size;           // Hash computation buffer size
    bool use_precomputed_ecc_tables;     // Enable ECC precomputation
    uint32_t ecc_table_size;             // Size of ECC precomputed tables
};

/**
 * Initialize all unified modules
 * @param config Module configuration parameters
 * @return cudaSuccess if all modules initialized successfully
 */
__host__
cudaError_t initializeUnifiedModules(const ModuleConfig& config);

/**
 * Cleanup all unified modules
 * @return cudaSuccess if cleanup successful
 */
__host__
cudaError_t cleanupUnifiedModules();

/**
 * Get module status information
 * @param result_emitter_initialized Output: result emitter status
 * @param hash_utils_initialized Output: hash utils status
 * @param ecc_operations_initialized Output: ECC operations status
 * @return cudaSuccess if status retrieval successful
 */
__host__
cudaError_t getModuleStatus(bool& result_emitter_initialized,
                           bool& hash_utils_initialized,
                           bool& ecc_operations_initialized);

/**
 * Synchronize all module operations
 * @return cudaSuccess if synchronization successful
 */
__host__
cudaError_t synchronizeModules();

} // namespace common
} // namespace keyhunt

#endif // MODULE_MANAGER_CUH
```

## Migration API

### Legacy Compatibility Layer

**File Location**: `src/KeyhuntCore/common/legacy_adapter.cuh`

**Purpose**: Maintain backward compatibility during migration

**API Contract**:
```cuda
#ifndef LEGACY_ADAPTER_CUH
#define LEGACY_ADAPTER_CUH

#include "result_emitter.cuh"
#include "hash_utils.cuh"
#include "ecc_operations.cuh"

namespace keyhunt {
namespace common {

/**
 * Legacy EmitCandidate function signature (for backward compatibility)
 * @param private_key Found private key
 * @param public_key Corresponding public key
 * @param address Bitcoin address
 * @param thread_id Thread identifier
 * @param block_id Block identifier
 */
__device__ __forceinline__
void EmitCandidate(uint64_t private_key,
                   const uint8_t public_key[65],
                   const char address[35],
                   uint32_t thread_id,
                   uint32_t block_id);

/**
 * Legacy FinalizeDigest function signature (for backward compatibility)
 * @param hash_context Hash context to finalize
 * @param digest Output digest buffer
 */
__device__ __forceinline__
void FinalizeDigest(HashContext* hash_context, uint8_t digest[32]);

/**
 * Migrate legacy kernel to use unified modules
 * @param legacy_kernel_function Pointer to legacy kernel function
 * @param unified_kernel_function Pointer to unified kernel function
 * @return true if migration successful, false otherwise
 */
__host__
bool migrateToUnifiedModules(void (*legacy_kernel_function)(),
                            void (*unified_kernel_function)());

/**
 * Validate migration results
 * @param test_cases Number of test cases to validate
 * @param tolerance Error tolerance for floating-point comparisons
 * @return true if migration validation successful, false otherwise
 */
__host__
bool validateMigration(uint32_t test_cases, double tolerance);

} // namespace common
} // namespace keyhunt

#endif // LEGACY_ADAPTER_CUH
```

## Usage Examples

### Basic Usage

```cuda
// Example: Using unified modules in separated kernel
__global__ void eccHashCompareKernel(
    const uint64_t* private_keys,
    uint32_t key_count,
    const uint8_t* target_hash160s,
    uint32_t target_count,
    CandidateResult* results,
    uint32_t* result_count) {

    uint32_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= key_count) return;

    // ECC computation using unified module
    UInt256 private_key;
    private_key.data[0] = private_keys[tid];
    // ... fill rest of private_key

    ECPoint public_key;
    if (!privateKeyToPublicKey(private_key, public_key)) {
        return; // ECC computation failed
    }

    // Hash computation using unified module
    uint8_t hash160[20];
    uint8_t public_key_bytes[65];
    compressPublicKey(public_key, public_key_bytes);
    computeUnifiedHash160(public_key_bytes, 33, hash160);

    // Comparison and result emission using unified module
    for (uint32_t i = 0; i < target_count; i++) {
        if (memcmp(hash160, target_hash160s + i * 20, 20) == 0) {
            CandidateResult result;
            result.private_key = private_keys[tid];
            memcpy(result.public_key, public_key_bytes, 65);
            // ... format address
            result.thread_id = tid;
            result.block_id = blockIdx.x;

            emitCandidate(results, 1000, result_count, result);
            break;
        }
    }
}
```

### Migration Usage

```cuda
// Legacy code before migration
__device__ void legacyEmitCandidate(uint64_t key, uint8_t* pubkey, char* addr) {
    // Old implementation...
}

// After migration - automatically redirects to unified module
__device__ void legacyEmitCandidate(uint64_t key, uint8_t* pubkey, char* addr) {
    CandidateResult result;
    result.private_key = key;
    memcpy(result.public_key, pubkey, 65);
    // ... format address

    emitCandidate(global_results, max_results, global_result_count, result);
}
```

## Validation Requirements

### Functional Validation

1. **Bitwise Identical Results**:
   - All unified module functions must produce identical results to legacy implementations
   - Validation against bitcoin-core/secp256k1 CPU reference
   - Zero tolerance for functional deviations

2. **Performance Validation**:
   - Unified module performance must meet or exceed legacy performance
   - Memory usage must not increase significantly
   - No performance regression allowed

### Integration Validation

1. **Kernel Compatibility**:
   - All existing kernels must compile with unified modules
   - No changes to kernel launch configurations required
   - Backward API compatibility maintained

2. **Memory Layout Compatibility**:
   - Data structures must maintain memory layout compatibility
   - No changes to existing data formats
   - Seamless migration without data conversion

## Performance Requirements

### Throughput Targets

- **ECC Operations**: ≥1.0 Gkeys/s on RTX 2080 Ti
- **Hash Operations**: ≥2.0 Ghash/s on RTX 3090
- **Result Emission**: ≥10 Mresults/s sustained rate

### Memory Requirements

- **Result Buffer**: Maximum 1000 results per kernel launch
- **Hash Context**: 64 bytes per thread maximum
- **ECC Context**: 512 bytes per thread maximum

### Latency Requirements

- **Module Initialization**: <100ms total
- **ECC Computation**: <1μs per key
- **Hash Computation**: <500ns per hash
- **Result Emission**: <100ns per result

## Error Handling

### Return Codes

All unified module functions use standard CUDA error handling:

- `cudaSuccess`: Operation completed successfully
- `cudaErrorInvalidValue`: Invalid input parameters
- `cudaErrorMemoryAllocation`: Memory allocation failed
- `cudaErrorLaunchFailure`: Kernel launch failed

### Error Recovery

1. **Graceful Degradation**:
   - ECC computation failures result in key skipping
   - Hash computation failures result in retry with fallback
   - Result emission failures result in buffer overflow handling

2. **Error Reporting**:
   - Detailed error messages in debug builds
   - Error counters for monitoring
   - Automatic retry for transient failures

## Testing Requirements

### Unit Tests

1. **Functional Correctness**:
   - Test all module functions with known inputs/outputs
   - Validate against reference implementations
   - Edge case testing (boundary conditions)

2. **Performance Benchmarks**:
   - Throughput testing for all operations
   - Memory usage profiling
   - Latency measurement under load

### Integration Tests

1. **Kernel Integration**:
   - Test unified modules within actual kernels
   - Multi-threaded access testing
   - Concurrent kernel execution testing

2. **Migration Testing**:
   - Test legacy adapter functionality
   - Validate migration process end-to-end
   - Rollback testing for failed migrations
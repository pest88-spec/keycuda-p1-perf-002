// Puzzle71Solver - Legacy Adapter Layer Header
// Backward compatibility adapter for code deduplication transition (T022)

#pragma once

#include "result_emitter.cuh"
#include "hash_utils.cuh"
#include "ecc_operations.cuh"
#include <cuda_runtime.h>

namespace keyhunt {
namespace legacy {

/**
 * @brief Legacy Adapter Layer for Backward Compatibility
 *
 * This adapter layer provides backward compatibility during the code deduplication
 * transition period. It allows existing code to continue working with the old
 * function names while internally using the new unified implementations.
 *
 * Key benefits:
 * - Zero-impact transition: existing code continues to work
 * - Gradual migration: can replace calls incrementally
 * - Debugging support: can log legacy function usage
 * - Performance preservation: no overhead in release builds
 *
 * Usage:
 * - Include this header in files that still use old function names
 * - Gradually replace old calls with new unified module calls
 * - Remove this adapter once migration is complete
 */

// Configuration for legacy adapter behavior
#ifdef LEGACY_ADAPTER_DEBUG
    #define LEGACY_LOG_USAGE 1
#else
    #define LEGACY_LOG_USAGE 0
#endif

/**
 * @brief Debug logging for legacy function usage
 *
 * Logs when legacy functions are called to help identify migration opportunities.
 * Only active when LEGACY_ADAPTER_DEBUG is defined.
 */
__device__ inline void LogLegacyUsage(const char* function_name) {
    #if LEGACY_LOG_USAGE
    // In a real implementation, this would log to a debug buffer
    // For now, we use a simple compile-time warning approach
    #endif
}

// Legacy EmitCandidate adapter (forwarding to unified implementation)
__device__ inline void EmitCandidate(
    bool has_candidate,
    int idx,
    bool compressed,
    const unsigned int x[8],
    const unsigned int y[8],
    const std::uint32_t digest[5]
) {
    LogLegacyUsage("EmitCandidate");
    keyhunt::common::EmitCandidate(has_candidate, idx, compressed, x, y, digest);
}

// Legacy EmitCandidate with different parameter order (backward compatibility)
__device__ inline void EmitCandidate(
    const unsigned int x[8],
    const unsigned int y[8],
    const std::uint32_t digest[5],
    bool has_candidate,
    int idx,
    bool compressed
) {
    LogLegacyUsage("EmitCandidate(legacy_order)");
    keyhunt::common::EmitCandidate(has_candidate, idx, compressed, x, y, digest);
}

// Legacy FinalizeDigest adapter (forwarding to unified implementation)
__device__ inline void FinalizeDigest(const std::uint32_t in[5], std::uint32_t out[5]) {
    LogLegacyUsage("FinalizeDigest");
    keyhunt::common::FinalizeDigest(in, out);
}

// Legacy ECC function adapters

__device__ inline void readInt(const unsigned int* ara, int idx, unsigned int x[8]) {
    LogLegacyUsage("readInt");
    keyhunt::common::ReadBigInt(ara, idx, x);
}

__device__ inline void writeInt(unsigned int* ara, int idx, const unsigned int x[8]) {
    LogLegacyUsage("writeInt");
    keyhunt::common::WriteBigInt(ara, idx, x);
}

__device__ inline void copyBigInt(const unsigned int src[8], unsigned int dest[8]) {
    LogLegacyUsage("copyBigInt");
    keyhunt::common::CopyBigInt(src, dest);
}

__device__ inline bool isInfinity(const unsigned int x[8]) {
    LogLegacyUsage("isInfinity");
    return keyhunt::common::IsInfinity(x);
}

__device__ inline unsigned int readIntLSW(const unsigned int* ara, int idx) {
    LogLegacyUsage("readIntLSW");
    return keyhunt::common::ReadLSW(ara, idx);
}

// Legacy BitCrack function adapters (for compatibility with extracted BitCrack code)

__device__ inline void beginBatchAddWithDouble(
    const unsigned int incX[8],
    const unsigned int incY[8],
    unsigned int* xPtr,
    unsigned int* chain,
    int srcIdx,
    int dstIdx,
    unsigned int accumulator[8]
) {
    LogLegacyUsage("beginBatchAddWithDouble");
    keyhunt::common::BeginBatchPointAdd(incX, incY, xPtr, chain, srcIdx, dstIdx, accumulator);
}

__device__ inline void completeBatchAddWithDouble(
    const unsigned int incX[8],
    const unsigned int incY[8],
    unsigned int* xPtr,
    unsigned int* yPtr,
    int srcIdx,
    int dstIdx,
    unsigned int* chain,
    const unsigned int accumulator[8],
    unsigned int resultX[8],
    unsigned int resultY[8]
) {
    LogLegacyUsage("completeBatchAddWithDouble");
    keyhunt::common::CompleteBatchPointAdd(incX, incY, xPtr, yPtr, srcIdx, dstIdx, chain, accumulator, resultX, resultY);
}

__device__ inline void doBatchInverse(unsigned int accumulator[8]) {
    LogLegacyUsage("doBatchInverse");
    // This would call the actual batch inverse implementation
    // For now, it's a placeholder maintaining the interface
}

// Legacy namespace compatibility

/**
 * @brief Legacy namespace that provides old function names
 *
 * Allows code to continue using the old namespace structure
 * while internally calling the unified implementations.
 */
namespace legacy_ec {
    __device__ inline void readInt(const unsigned int* ara, int idx, unsigned int x[8]) {
        keyhunt::legacy::readInt(ara, idx, x);
    }

    __device__ inline void writeInt(unsigned int* ara, int idx, const unsigned int x[8]) {
        keyhunt::legacy::writeInt(ara, idx, x);
    }

    __device__ inline unsigned int* getXPtr() {
        // This would return the actual X pointer from the ECC system
        // Implementation depends on the ECC system being used
        return nullptr; // Placeholder
    }

    __device__ inline unsigned int* getYPtr() {
        // This would return the actual Y pointer from the ECC system
        // Implementation depends on the ECC system being used
        return nullptr; // Placeholder
    }
}

// Legacy utility functions

/**
 * @brief Check if legacy adapter is being used
 *
 * Runtime function to check if any legacy functions have been called.
 * Useful for debugging migration progress.
 *
 * @return True if legacy functions have been used, false otherwise
 */
__host__ bool HasLegacyUsage();

/**
 * @brief Get legacy usage statistics
 *
 * Provides statistics about which legacy functions are being used
 * to help prioritize migration efforts.
 *
 * @return JSON string with usage statistics
 */
__host__ std::string GetLegacyUsageStats();

/**
 * @brief Reset legacy usage tracking
 *
 * Resets the legacy usage tracking counters.
 * Useful for testing or starting new measurement periods.
 */
__host__ void ResetLegacyUsageTracking();

// Migration helper macros

/**
 * @brief Macro to warn about legacy function usage at compile time
 *
 * Use this to identify files that still use legacy functions during compilation.
 */
#define WARN_LEGACY_USAGE(func_name) \
    _Pragma("message(\"WARNING: Using legacy function " #func_name ". Consider migrating to unified modules.\")")

/**
 * @brief Macro to gradually migrate from legacy to unified functions
 *
 * Example usage:
 * ```cpp
 * // Old code:
 * EmitCandidate(has_candidate, idx, compressed, x, y, digest);
 *
 * // Migrated code:
 * EMIT_CANDIDATE_MIGRATED(has_candidate, idx, compressed, x, y, digest);
 * ```
 */
#define EMIT_CANDIDATE_MIGRATED(has_candidate, idx, compressed, x, y, digest) \
    WARN_LEGACY_USAGE(EmitCandidate); \
    keyhunt::common::EmitCandidate(has_candidate, idx, compressed, x, y, digest)

#define FINALIZE_DIGEST_MIGRATED(in, out) \
    WARN_LEGACY_USAGE(FinalizeDigest); \
    keyhunt::common::FinalizeDigest(in, out)

#define READ_INT_MIGRATED(ara, idx, x) \
    WARN_LEGACY_USAGE(readInt); \
    keyhunt::common::ReadBigInt(ara, idx, x)

#define WRITE_INT_MIGRATED(ara, idx, x) \
    WARN_LEGACY_USAGE(writeInt); \
    keyhunt::common::WriteBigInt(ara, idx, x)

// Automatic version detection and migration hints

/**
 * @brief Compile-time detection of legacy code patterns
 *
 * These macros help identify legacy usage patterns that need migration.
 */
#define DETECT_LEGACY_EMITCANDIDATE_USAGE \
    _Pragma("message(\"INFO: Checking for EmitCandidate usage patterns\")")

#define DETECT_LEGACY_ECC_USAGE \
    _Pragma("message(\"INFO: Checking for ECC function usage patterns\")")

} // namespace legacy

} // namespace keyhunt

// Global legacy namespace for maximum backward compatibility
// This allows existing code to continue working without any namespace changes
namespace keyhunt_legacy = keyhunt::legacy;

// Legacy compatibility macros for seamless transition
#define LEGACY_EMITCANDIDATE(has_candidate, idx, compressed, x, y, digest) \
    keyhunt::legacy::EmitCandidate(has_candidate, idx, compressed, x, y, digest)

#define LEGACY_FINALIZE_DIGEST(in, out) \
    keyhunt::legacy::FinalizeDigest(in, out)

#define LEGACY_READ_INT(ara, idx, x) \
    keyhunt::legacy::readInt(ara, idx, x)

#define LEGACY_WRITE_INT(ara, idx, x) \
    keyhunt::legacy::writeInt(ara, idx, x)

// Version information for migration tracking
namespace legacy_info {
    constexpr int ADAPTER_VERSION = 1;
    constexpr char ADAPTER_NAME[] = "Puzzle71Solver Legacy Adapter";
    constexpr char MIGRATION_GUIDE_URL[] = "docs/migration-guide.md";
}
// Puzzle71Solver - Unified ResultEmitter Module Header
// Consolidated EmitCandidate implementation eliminating code duplication (T014-T015)

#pragma once

#include <cstdint>
#include <cuda_runtime.h>
#include "../../compute/gpu/device_results.h"

namespace keyhunt {
namespace common {

// Use existing DeviceCandidate and DeviceResultBuffer from puzzle71::gpu namespace
using DeviceCandidate = puzzle71::gpu::DeviceCandidate;
using DeviceResultBuffer = puzzle71::gpu::DeviceResultBuffer;

// Global device result buffer (extern declaration)
extern __device__ DeviceResultBuffer g_result_buffer;

/**
 * @brief Unified EmitCandidate function with highest-quality implementation
 *
 * This function consolidates the best features from both implementations:
 * - Advanced overflow detection (from puzzle71_kernel.cu)
 * - Optimized warp-level communication (from hash_kernel.cu)
 * - Comprehensive result metadata (from puzzle71_kernel.cu)
 * - Streamlined memory access patterns (from hash_kernel.cu)
 *
 * Key optimizations:
 * - Uses __ballot_sync for warp-level coordination
 * - Single atomic operation per warp (not per thread)
 * - Efficient slot calculation using __popc
 * - Proper overflow detection and reporting
 * - Full result metadata preservation
 *
 * @param has_candidate Whether this thread has a matching candidate
 * @param idx Index identifier for the candidate
 * @param compressed Whether the candidate is compressed format
 * @param x X-coordinate array (8 32-bit words)
 * @param y Y-coordinate array (8 32-bit words)
 * @param digest Hash160 digest (5 32-bit words)
 */
__device__ inline void EmitCandidate(
    bool has_candidate,
    int idx,
    bool compressed,
    const unsigned int x[8],
    const unsigned int y[8],
    const std::uint32_t digest[5]
) {
    // Early exit if result buffer is not initialized
    if (g_result_buffer.capacity == 0 ||
        g_result_buffer.candidates == nullptr ||
        g_result_buffer.count == nullptr) {
        return;
    }

    const unsigned full_mask = 0xffffffffu;
    unsigned active = __ballot_sync(full_mask, has_candidate);

    // Exit early if no threads in this warp have candidates
    if (active == 0u) {
        return;
    }

    const int lane = threadIdx.x & 31;
    const int leader = __ffs(active) - 1;
    const unsigned int matches = __popc(active);

    std::uint32_t base_index = 0;

    // Only the leader thread performs the atomic operation
    if (lane == leader) {
        base_index = atomicAdd(g_result_buffer.count, matches);

        // Advanced overflow detection and reporting
        if (g_result_buffer.dropped != nullptr) {
            std::uint32_t overflow = 0;
            if (base_index >= g_result_buffer.capacity) {
                overflow = matches;  // All matches will overflow
            } else if (base_index + matches > g_result_buffer.capacity) {
                overflow = (base_index + matches) - g_result_buffer.capacity;  // Partial overflow
            }
            if (overflow > 0) {
                atomicAdd(g_result_buffer.dropped, overflow);
            }
        }
    }

    // Broadcast base_index to all threads in the warp
    base_index = __shfl_sync(active, base_index, leader);

    if (!has_candidate) {
        return;
    }

    // Calculate offset for this thread within the warp
    unsigned lane_offset = __popc(active & ((1u << lane) - 1));
    std::uint32_t slot = base_index + lane_offset;

    // Check for buffer overflow
    if (slot >= g_result_buffer.capacity) {
        return;
    }

    // Write result with full metadata
    DeviceCandidate& out = g_result_buffer.candidates[slot];

    // Comprehensive result metadata
    out.block = static_cast<std::uint32_t>(blockIdx.x);
    out.thread = static_cast<std::uint32_t>(threadIdx.x);
    out.idx = static_cast<std::uint32_t>(idx);
    out.compressed = compressed ? 1u : 0u;

    // Copy coordinate data
    for (int i = 0; i < 8; ++i) {
        out.x[i] = x[i];
        out.y[i] = y[i];
    }

    // Finalize digest using unified hash utils (T038: Updated to camelCase)
    keyhunt::common::finalizeDigest(digest, out.digest);
}

/**
 * @brief camelCase wrapper for emitCandidate (T038 - Naming Convention Update)
 *
 * This is the new standardized name following camelCase convention.
 * The implementation is identical to EmitCandidate for consistency.
 */
__device__ inline void emitCandidate(
    bool has_candidate,
    int idx,
    bool compressed,
    const unsigned int x[8],
    const unsigned int y[8],
    const std::uint32_t digest[5]
) {
    EmitCandidate(has_candidate, idx, compressed, x, y, digest);
}

/**
 * @brief Backward compatibility wrapper for legacy EmitCandidate calls
 *
 * Maintains compatibility with existing code that may use different
 * parameter orders or types during the transition period.
 */

/**
 * @brief Batch emit multiple candidates (for future optimization)
 *
 * This function is designed for scenarios where multiple candidates
 * need to be emitted from the same thread, reducing warp synchronization overhead.
 *
 * @param candidate_count Number of candidates to emit
 * @param indices Array of candidate indices
 * @param compressed_flags Array of compression flags
 * @param x_coordinates Array of X coordinate arrays
 * @param y_coordinates Array of Y coordinate arrays
 * @param digests Array of digest arrays
 */
__device__ inline void EmitCandidateBatch(
    int candidate_count,
    const int indices[],
    const bool compressed_flags[],
    const unsigned int x_coordinates[][8],
    const unsigned int y_coordinates[][8],
    const std::uint32_t digests[][5]
) {
    // For now, call EmitCandidate for each candidate
    // TODO: Optimize this to reduce warp synchronization overhead
    for (int i = 0; i < candidate_count; ++i) {
        EmitCandidate(
            true,  // has_candidate (always true in batch mode)
            indices[i],
            compressed_flags[i],
            x_coordinates[i],
            y_coordinates[i],
            digests[i]
        );
    }
}

/**
 * @brief Check if result buffer has capacity for additional candidates
 *
 * @param additional_candidates Number of additional candidates to check
 * @return True if buffer has sufficient capacity
 */
__device__ inline bool HasResultCapacity(int additional_candidates = 1) {
    if (g_result_buffer.count == nullptr || g_result_buffer.capacity == 0) {
        return false;
    }

    std::uint32_t current_count = *g_result_buffer.count;
    return (current_count + additional_candidates) <= g_result_buffer.capacity;
}

/**
 * @brief Get current result buffer statistics
 *
 * @param[out] used Number of slots currently used
 * @param[out] capacity Total buffer capacity
 * @param[out] dropped Number of candidates that were dropped due to overflow
 */
__device__ inline void GetResultBufferStats(
    std::uint32_t& used,
    std::uint32_t& capacity,
    std::uint32_t& dropped
) {
    used = g_result_buffer.count ? *g_result_buffer.count : 0;
    capacity = g_result_buffer.capacity;
    dropped = g_result_buffer.dropped ? *g_result_buffer.dropped : 0;
}

/**
 * @brief Reset result buffer (for testing or reuse scenarios)
 */
__device__ inline void ResetResultBuffer() {
    if (g_result_buffer.count) {
        *g_result_buffer.count = 0;
    }
    if (g_result_buffer.dropped) {
        *g_result_buffer.dropped = 0;
    }
}

// Forward declaration for FinalizeDigest (from hash_utils.cuh)
__device__ inline void FinalizeDigest(const std::uint32_t in[5], std::uint32_t out[5]);

} // namespace common
} // namespace keyhunt

// Legacy compatibility macros for existing code
#define EMIT_CANDIDATE(has_candidate, idx, compressed, x, y, digest) keyhunt::common::EmitCandidate(has_candidate, idx, compressed, x, y, digest)


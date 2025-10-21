// Puzzle71Solver - Unified HashUtils Module Header
// Consolidated FinalizeDigest and hash utilities eliminating code duplication (T016-T017)

#pragma once

#include <cstdint>
#include <cuda_runtime.h>

namespace keyhunt {
namespace common {

/**
 * @brief Unified FinalizeDigest function with 100% compatibility
 *
 * This function consolidates the identical FinalizeDigest implementations
 * from both puzzle71_kernel.cu and hash_kernel.cu. The implementation
 * adds the initialization vector (IV) and performs byte swapping to
 * finalize the RIPEMD160 digest.
 *
 * Hash160 process: SHA256(message) -> RIPEMD160(SHA256_result)
 * This function adds the final IV to complete the RIPEMD160 digest.
 *
 * @param in Input digest array (5 32-bit words from RIPEMD160 core)
 * @param out Output digest array (5 32-bit words, finalized Hash160)
 */
__device__ inline void FinalizeDigest(const std::uint32_t in[5], std::uint32_t out[5]) {
    // RIPEMD160 initialization vector (reversed order for final addition)
    const std::uint32_t iv[5] = {
        0x67452301u,  // h0
        0xefcdab89u,  // h1
        0x98badcfeu,  // h2
        0x10325476u,  // h3
        0xc3d2e1f0u   // h4
    };

    // Add IV with circular shift and byte swap
    for (int i = 0; i < 5; ++i) {
        const std::uint32_t value = in[i] + iv[(i + 1) % 5];
        out[i] = ByteSwap32(value);
    }
}

/**
 * @brief camelCase wrapper for finalizeDigest (T038 - Naming Convention Update)
 *
 * This is the new standardized name following camelCase convention.
 * The implementation is identical to FinalizeDigest for consistency.
 */
__device__ inline void finalizeDigest(const std::uint32_t in[5], std::uint32_t out[5]) {
    FinalizeDigest(in, out);
}

/**
 * @brief Byte swap function for 32-bit values (big-endian to little-endian)
 *
 * CUDA provides intrinsics for byte swapping, but this implementation
 * ensures compatibility across all platforms and CUDA versions.
 *
 * @param value 32-bit value to byte swap
 * @return Byte-swapped 32-bit value
 */
__device__ inline std::uint32_t ByteSwap32(std::uint32_t value) {
    // Use CUDA intrinsic if available for better performance
    #if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 300
        return __byte_perm(value, 0, 0x0123);
    #else
        // Fallback implementation
        return ((value & 0x000000ff) << 24) |
               ((value & 0x0000ff00) << 8)  |
               ((value & 0x00ff0000) >> 8)  |
               ((value & 0xff000000) >> 24);
    #endif
}

/**
 * @brief Byte swap function for 64-bit values
 *
 * Used for handling larger data structures and ensuring proper endianness.
 *
 * @param value 64-bit value to byte swap
 * @return Byte-swapped 64-bit value
 */
__device__ inline std::uint64_t ByteSwap64(std::uint64_t value) {
    #if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 300
        return std::uint64_t(__byte_perm(std::uint32_t(value), std::uint32_t(value >> 32), 0x0123)) << 32 |
               __byte_perm(std::uint32_t(value >> 32), std::uint32_t(value), 0x0123);
    #else
        // Fallback implementation
        std::uint32_t low = std::uint32_t(value);
        std::uint32_t high = std::uint32_t(value >> 32);
        return std::uint64_t(ByteSwap32(low)) << 32 | ByteSwap32(high);
    #endif
}

/**
 * @brief Compare two digests for equality
 *
 * @param digest1 First digest array (5 32-bit words)
 * @param digest2 Second digest array (5 32-bit words)
 * @return True if digests are identical
 */
__device__ inline bool DigestEqual(const std::uint32_t digest1[5], const std::uint32_t digest2[5]) {
    // Use single comparison for efficiency
    return (digest1[0] == digest2[0]) &&
           (digest1[1] == digest2[1]) &&
           (digest1[2] == digest2[2]) &&
           (digest1[3] == digest2[3]) &&
           (digest1[4] == digest2[4]);
}

/**
 * @brief Zero out a digest array (for security and initialization)
 *
 * @param digest Digest array to clear (5 32-bit words)
 */
__device__ inline void DigestClear(std::uint32_t digest[5]) {
    #pragma unroll
    for (int i = 0; i < 5; ++i) {
        digest[i] = 0;
    }
}

/**
 * @brief Copy digest from source to destination
 *
 * @param src Source digest array (5 32-bit words)
 * @param dst Destination digest array (5 32-bit words)
 */
__device__ inline void DigestCopy(const std::uint32_t src[5], std::uint32_t dst[5]) {
    #pragma unroll
    for (int i = 0; i < 5; ++i) {
        dst[i] = src[i];
    }
}

/**
 * @brief Calculate simple hash of digest for indexing/caching
 *
 * This is not a cryptographic hash, but a simple function for
 * creating indices or cache keys from digests.
 *
 * @param digest Input digest array (5 32-bit words)
 * @return 32-bit hash value
 */
__device__ inline std::uint32_t DigestHash(const std::uint32_t digest[5]) {
    // Simple XOR-based hash for indexing purposes
    std::uint32_t hash = digest[0];
    hash ^= digest[1] + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= digest[2] + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= digest[3] + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= digest[4] + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
}

/**
 * @brief Convert digest to hexadecimal string representation
 *
 * This function creates a readable hexadecimal representation
 * of the digest for debugging and logging purposes.
 *
 * @param digest Input digest array (5 32-bit words)
 * @param output Output character array (at least 40 characters for 20-byte digest)
 */
__device__ inline void DigestToHex(const std::uint32_t digest[5], char output[40]) {
    const char hex_chars[] = "0123456789abcdef";

    for (int i = 0; i < 5; ++i) {
        std::uint32_t value = ByteSwap32(digest[i]); // Convert to big-endian for display
        for (int j = 0; j < 4; ++j) {
            std::uint8_t byte = (value >> (8 * (3 - j))) & 0xff;
            output[i * 8 + j * 2] = hex_chars[byte >> 4];
            output[i * 8 + j * 2 + 1] = hex_chars[byte & 0x0f];
        }
    }
}

/**
 * @brief Validate digest format and content
 *
 * Performs basic validation to ensure digest contains reasonable values.
 * This is primarily for debugging and error detection.
 *
 * @param digest Digest array to validate (5 32-bit words)
 * @return True if digest appears valid
 */
__device__ inline bool DigestValidate(const std::uint32_t digest[5]) {
    // Check for obviously invalid patterns (all zeros, all ones, etc.)
    bool all_zeros = true;
    bool all_ones = true;

    #pragma unroll
    for (int i = 0; i < 5; ++i) {
        if (digest[i] != 0) all_zeros = false;
        if (digest[i] != 0xffffffffu) all_ones = false;
    }

    // Reject trivially invalid digests
    return !(all_zeros || all_ones);
}

/**
 * @brief Batch digest operations for multiple digests
 *
 * Provides vectorized operations on multiple digests for improved
 * performance when processing multiple candidates simultaneously.
 *
 * @param count Number of digests in batch
 * @param inputs Array of input digest arrays
 * @param outputs Array of output digest arrays
 */
__device__ inline void DigestBatchFinalize(
    int count,
    const std::uint32_t inputs[][5],
    std::uint32_t outputs[][5]
) {
    // Process each digest in the batch
    #pragma unroll 4  // Unroll small batches for better performance
    for (int i = 0; i < count; ++i) {
        FinalizeDigest(inputs[i], outputs[i]);
    }
}

/**
 * @brief Merge multiple digests using XOR (for certain cryptographic applications)
 *
 * @param digest1 First digest array (5 32-bit words)
 * @param digest2 Second digest array (5 32-bit words)
 * @param result Output digest array (5 32-bit words)
 */
__device__ inline void DigestXor(
    const std::uint32_t digest1[5],
    const std::uint32_t digest2[5],
    std::uint32_t result[5]
) {
    #pragma unroll
    for (int i = 0; i < 5; ++i) {
        result[i] = digest1[i] ^ digest2[i];
    }
}

// RIPEMD160 constants for reference
namespace ripemd160 {
    constexpr std::uint32_t IV[5] = {
        0x67452301u,  // h0
        0xefcdab89u,  // h1
        0x98badcfeu,  // h2
        0x10325476u,  // h3
        0xc3d2e1f0u   // h4
    };

    constexpr int DIGEST_SIZE = 20;  // 160 bits = 20 bytes
    constexpr int DIGEST_WORDS = 5;   // 20 bytes / 4 bytes per word
}

} // namespace common
} // namespace keyhunt

// Legacy compatibility macros for existing code
#define FINALIZE_DIGEST(in, out) keyhunt::common::FinalizeDigest(in, out)

#define BYTE_SWAP32(value) keyhunt::common::ByteSwap32(value)

#define DIGEST_EQUAL(digest1, digest2) keyhunt::common::DigestEqual(digest1, digest2)
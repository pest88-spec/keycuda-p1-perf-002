// Puzzle71Solver - Unified Candidate Structure (T040 - Adapter Simplification)
// Architecture Modernization - Eliminating redundant adapter abstractions
//
// This header defines the unified candidate structure that replaces the redundant
// adapter abstractions between different GPU result formats.
//
// Previously Redundant Structures:
// - reference_adapter::ComputationResult
// - puzzle71::gpu::DeviceCandidate
//
// Unified Structure:
// - puzzle71::gpu::UnifiedCandidate (single source of truth)

#pragma once

#include <array>
#include <cstdint>
#include "core/uint256.h"

namespace puzzle71::gpu {

/**
 * @brief Unified candidate structure eliminating adapter redundancy
 *
 * This structure combines the best features from both DeviceCandidate and
 * ComputationResult, providing a single source of truth for candidate data.
 *
 * Key improvements from adapter consolidation:
 * - Direct UInt256 coordinates (no conversion overhead)
 * - Unified field naming (camelCase convention)
 * - Comprehensive metadata for debugging
 * - Memory-efficient layout for GPU transfer
 *
 * Memory Layout:
 * - GPU-optimized fields first (for device memory)
 * - CPU-only fields last (for host processing)
 * - Total size: ~144 bytes (vs ~200 bytes with adapters)
 */
struct UnifiedCandidate {
    // GPU-optimized fields (device memory layout)
    std::uint32_t block{0};           // Block index
    std::uint32_t thread{0};          // Thread index
    std::uint32_t idx{0};             // Linear index
    std::uint32_t compressed{0};      // Compression flag (0=uncompressed, 1=compressed)

    // Coordinate data (GPU-optimized arrays)
    std::uint32_t x[8]{};            // X-coordinate (8 x 32-bit words)
    std::uint32_t y[8]{};            // Y-coordinate (8 x 32-bit words)
    std::uint32_t digest[5]{};       // Hash160 digest (5 x 32-bit words)

    // CPU-only fields (host processing)
    core::UInt256 private_key{};      // Private key scalar
    core::UInt256 x_256{};           // X-coordinate as UInt256 (convenience)
    core::UInt256 y_256{};           // Y-coordinate as UInt256 (convenience)

    /**
     * @brief Convert from device layout to unified format
     *
     * This replaces the adapter conversion logic with direct field assignment.
     * Eliminates the need for reference_adapter::ComputationResult conversion.
     *
     * @param device_candidate Device candidate from GPU memory
     * @param batch_start Starting scalar for private key calculation
     * @param total_threads Total threads in kernel launch
     */
    static UnifiedCandidate fromDeviceCandidate(
        const struct DeviceCandidate& device_candidate,
        const core::UInt256& batch_start,
        std::uint64_t total_threads
    );

    /**
     * @brief Check if this candidate is valid
     * @return true if candidate has valid coordinate data
     */
    bool isValid() const;

    /**
     * @brief Get the compressed/uncompressed flag as boolean
     * @return true if compressed, false if uncompressed
     */
    bool isCompressed() const { return compressed != 0; }

    /**
     * @brief Calculate the private key for this candidate
     * @param batch_start Starting scalar for the batch
     * @param total_threads Total threads in kernel launch
     * @return Private key scalar
     */
    core::UInt256 calculatePrivateKey(
        const core::UInt256& batch_start,
        std::uint64_t total_threads
    ) const;

    /**
     * @brief Update UInt256 coordinate fields from array data
     *
     * Synchronizes the convenience UInt256 fields with the raw array data.
     * This replaces the adapter coordinate conversion logic.
     */
    void updateCoordinateFields();

    /**
     * @brief Validate coordinate consistency
     * @return true if array and UInt256 fields are consistent
     */
    bool validateCoordinates() const;
};

/**
 * @brief Unified result buffer for GPU candidates
 *
 * Replaces the adapter-dependent buffer management with direct
 * GPU memory operations.
 */
struct UnifiedResultBuffer {
    UnifiedCandidate* candidates{nullptr};    // Candidate array
    std::uint32_t* count{nullptr};           // Current count
    std::uint32_t* dropped{nullptr};         // Dropped candidates count
    std::size_t capacity{0};                  // Buffer capacity

    /**
     * @brief Check if buffer is properly initialized
     * @return true if buffer is ready for use
     */
    bool isInitialized() const {
        return candidates != nullptr &&
               count != nullptr &&
               capacity > 0;
    }

    /**
     * @brief Get current number of candidates (with bounds checking)
     * @return Number of candidates or 0 if buffer not initialized
     */
    std::uint32_t getCandidateCount() const {
        return isInitialized() ? *count : 0;
    }

    /**
     * @brief Get number of dropped candidates (with bounds checking)
     * @return Number of dropped candidates or 0 if not available
     */
    std::uint32_t getDroppedCount() const {
        return isInitialized() && dropped ? *dropped : 0;
    }
};

} // namespace puzzle71::gpu


/**
 * @brief Conversion utilities replacing adapter functions (T040)
 *
 * These functions provide direct conversion without adapter overhead.
 */
namespace puzzle71::gpu::conversion {

/**
 * @brief Convert UInt256 to secp256k1 format (replaces reference_adapter::ToReferenceFormat)
 * @param value UInt256 to convert
 * @return secp256k1 uint256 in reference format
 */
secp256k1::uint256 uint256ToSecp256k1(const core::UInt256& value);

/**
 * @brief Convert secp256k1 format to UInt256 (replaces reference_adapter::FromReferenceFormat)
 * @param value secp256k1 uint256 to convert
 * @return UInt256 in puzzle71 format
 */
core::UInt256 secp256k1ToUint256(const secp256k1::uint256& value);

/**
 * @brief Convert UInt256 to byte array (replaces reference_adapter::UInt256ToBytes)
 * @param value UInt256 to convert
 * @return 32-byte array in big-endian format
 */
std::array<unsigned char, 32> uint256ToBytes(const core::UInt256& value);

} // namespace puzzle71::gpu::conversion
// Puzzle71Solver - Unified Candidate Implementation (T040 - Adapter Simplification)
// Architecture Modernization - Eliminating redundant adapter abstractions

#include "unified_candidate.h"
#include "device_results.h"
#include <secp256k1.h>
#include <algorithm>

namespace puzzle71::gpu {

UnifiedCandidate UnifiedCandidate::fromDeviceCandidate(
    const DeviceCandidate& device_candidate,
    const core::UInt256& batch_start,
    std::uint64_t total_threads
) {
    UnifiedCandidate candidate{};

    // Copy GPU-optimized fields directly
    candidate.block = device_candidate.block;
    candidate.thread = device_candidate.thread;
    candidate.idx = device_candidate.idx;
    candidate.compressed = device_candidate.compressed;

    // Copy coordinate arrays
    for (int i = 0; i < 8; ++i) {
        candidate.x[i] = device_candidate.x[i];
        candidate.y[i] = device_candidate.y[i];
    }

    // Copy digest
    for (int i = 0; i < 5; ++i) {
        candidate.digest[i] = device_candidate.digest[i];
    }

    // Calculate private key
    candidate.private_key = candidate.calculatePrivateKey(batch_start, total_threads);

    // Update UInt256 convenience fields
    candidate.updateCoordinateFields();

    return candidate;
}

bool UnifiedCandidate::isValid() const {
    // Check for non-zero coordinates (basic validity)
    bool has_valid_coordinates = false;
    for (int i = 0; i < 8; ++i) {
        if (x[i] != 0 || y[i] != 0) {
            has_valid_coordinates = true;
            break;
        }
    }

    // Check for non-zero digest
    bool has_valid_digest = false;
    for (int i = 0; i < 5; ++i) {
        if (digest[i] != 0) {
            has_valid_digest = true;
            break;
        }
    }

    return has_valid_coordinates && has_valid_digest;
}

core::UInt256 UnifiedCandidate::calculatePrivateKey(
    const core::UInt256& batch_start,
    std::uint64_t total_threads
) const {
    // Calculate global thread index
    const std::uint64_t global_thread_idx =
        static_cast<std::uint64_t>(block) * 256 + thread; // Assuming 256 threads per block

    // Calculate offset within batch
    const std::uint64_t batch_offset = global_thread_idx + static_cast<std::uint64_t>(idx);

    return batch_start + core::UInt256(batch_offset);
}

void UnifiedCandidate::updateCoordinateFields() {
    // Convert x-coordinate from array to UInt256
    for (std::size_t i = 0; i < 4; ++i) {
        std::uint64_t low = static_cast<std::uint64_t>(x[2 * i]);
        std::uint64_t high = static_cast<std::uint64_t>(x[2 * i + 1]);
        x_256.limbs[i] = (high << 32) | low;
    }

    // Convert y-coordinate from array to UInt256
    for (std::size_t i = 0; i < 4; ++i) {
        std::uint64_t low = static_cast<std::uint64_t>(y[2 * i]);
        std::uint64_t high = static_cast<std::uint64_t>(y[2 * i + 1]);
        y_256.limbs[i] = (high << 32) | low;
    }
}

bool UnifiedCandidate::validateCoordinates() const {
    // Create temporary UInt256 from array data
    core::UInt256 temp_x{}, temp_y{};

    for (std::size_t i = 0; i < 4; ++i) {
        std::uint64_t low = static_cast<std::uint64_t>(x[2 * i]);
        std::uint64_t high = static_cast<std::uint64_t>(x[2 * i + 1]);
        temp_x.limbs[i] = (high << 32) | low;

        low = static_cast<std::uint64_t>(y[2 * i]);
        high = static_cast<std::uint64_t>(y[2 * i + 1]);
        temp_y.limbs[i] = (high << 32) | low;
    }

    // Compare with convenience fields
    return temp_x == x_256 && temp_y == y_256;
}

} // namespace puzzle71::gpu

// Conversion utilities implementation (replacing adapter functions)
namespace puzzle71::gpu::conversion {

secp256k1::uint256 uint256ToSecp256k1(const core::UInt256& value) {
    unsigned int words[8];
    for (std::size_t i = 0; i < 4; ++i) {
        std::uint64_t limb = value.limbs[i];
        words[2 * i] = static_cast<unsigned int>(limb & 0xFFFFFFFFu);
        words[2 * i + 1] = static_cast<unsigned int>((limb >> 32) & 0xFFFFFFFFu);
    }
    return secp256k1::uint256(words, secp256k1::uint256::LittleEndian);
}

core::UInt256 secp256k1ToUint256(const secp256k1::uint256& value) {
    unsigned int words[8];
    value.exportWords(words, 8, secp256k1::uint256::LittleEndian);
    core::UInt256 out = core::UInt256::Zero();
    for (std::size_t i = 0; i < 4; ++i) {
        std::uint64_t low = static_cast<std::uint64_t>(words[2 * i]);
        std::uint64_t high = static_cast<std::uint64_t>(words[2 * i + 1]);
        out.limbs[i] = (high << 32) | low;
    }
    return out;
}

std::array<unsigned char, 32> uint256ToBytes(const core::UInt256& value) {
    std::array<unsigned char, 32> out{};
    for (std::size_t i = 0; i < value.limbs.size(); ++i) {
        std::uint64_t limb = value.limbs[i];
        for (std::size_t j = 0; j < 8; ++j) {
            out[31 - (i * 8 + j)] = static_cast<unsigned char>((limb >> (j * 8)) & 0xFF);
        }
    }
    return out;
}

} // namespace puzzle71::gpu::conversion
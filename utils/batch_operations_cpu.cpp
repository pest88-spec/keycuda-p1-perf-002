/**
 * Unified Batch Operations - CPU Implementation
 * 
 * Implements P2-001: Code Duplication Optimization
 * - CPU batch operations using reference implementations
 * - Adapts VanitySearch and BatchInverseAdapter
 * - DRY principle compliance
 * 
 * @origin       https://github.com/Puzzle71Solver/Puzzle71Solver
 * @origin_path  src/utils/batch_operations_cpu.cpp
 * @origin_commit <current_commit>
 * @origin_license MIT
 * @extracted_date   2025-10-13
 * @extracted_by     Puzzle71Solver Team
 * @modifications    Created for P2-001 code duplication optimization
 * @spdx_license_identifier MIT
 */

#include "batch_operations.h"
#include "../core/ECC/batch_inverse_adapter.h"
#include "../../external/VanitySearch/Int.h"
#include "../../external/VanitySearch/Point.h"
#include "../../external/VanitySearch/SECP256k1.h"

#include <stdexcept>
#include <vector>
#include <cstring>

namespace puzzle71 {
namespace utils {

namespace {
    // Helper: Convert uint32_t[8] to VanitySearch Int
    void uint32ToInt(const uint32_t* data, Int& result) {
        // VanitySearch Int uses big-endian 256-bit representation
        // uint32_t[8] is also big-endian (most significant word first)
        uint32_t bits[8];
        for (int i = 0; i < 8; i++) {
            bits[i] = data[i];
        }
        result.SetInt32Array(bits);
    }

    // Helper: Convert VanitySearch Int to uint32_t[8]
    void intToUint32(const Int& value, uint32_t* data) {
        // Extract 256-bit value as uint32_t array
        uint32_t bits[8];
        value.Get32Bytes(reinterpret_cast<unsigned char*>(bits));
        for (int i = 0; i < 8; i++) {
            data[i] = bits[i];
        }
    }

    // Helper: Convert uint32_t[8] to VanitySearch Point
    void uint32ToPoint(const uint32_t* x, const uint32_t* y, Point& result) {
        uint32ToInt(x, result.x);
        uint32ToInt(y, result.y);
        result.z.SetInt32(1);  // Affine coordinates (z=1)
    }

    // Helper: Convert VanitySearch Point to uint32_t[8]
    void pointToUint32(const Point& point, uint32_t* x, uint32_t* y) {
        intToUint32(point.x, x);
        intToUint32(point.y, y);
    }

    // Global secp256k1 instance (initialized once)
    static Secp256K1* getSecp256k1Instance() {
        static Secp256K1 instance;
        static bool initialized = false;
        if (!initialized) {
            instance.Init();
            initialized = true;
        }
        return &instance;
    }
}

/**
 * Batch point addition (CPU)
 * 
 * Computes P[i] = P[i] + Q for all i in parallel
 * Uses VanitySearch AddDirect for affine coordinates
 */
bool BatchOperations::batch_point_add_cpu(
    uint32_t* points_x,
    uint32_t* points_y,
    size_t count,
    const uint32_t* q_x,
    const uint32_t* q_y)
{
    try {
        // Validate input
        if (!points_x || !points_y || !q_x || !q_y) {
            throw std::invalid_argument("batch_point_add_cpu: null pointer");
        }
        if (count == 0) {
            return true;  // Nothing to do
        }

        // Get secp256k1 instance
        Secp256K1* secp = getSecp256k1Instance();

        // Convert Q to VanitySearch Point
        Point q;
        uint32ToPoint(q_x, q_y, q);

        // Process each point
        for (size_t i = 0; i < count; i++) {
            // Convert P[i] to VanitySearch Point
            Point p;
            uint32ToPoint(&points_x[i * 8], &points_y[i * 8], p);

            // Compute P[i] = P[i] + Q using VanitySearch AddDirect
            Point result = secp->AddDirect(p, q);

            // Convert result back to uint32_t[8]
            pointToUint32(result, &points_x[i * 8], &points_y[i * 8]);
        }

        return true;
    } catch (const std::exception& e) {
        // Log error (in production, use proper logging)
        return false;
    }
}

/**
 * Batch modular inverse (CPU)
 * 
 * Computes inv[i] = 1/values[i] mod p for all i
 * Uses BatchInverseAdapter (Montgomery batch inversion)
 */
bool BatchOperations::batch_mod_inverse_cpu(
    const uint32_t* values,
    uint32_t* inv,
    size_t count,
    const uint32_t* modulus)
{
    try {
        // Validate input
        if (!values || !inv || !modulus) {
            throw std::invalid_argument("batch_mod_inverse_cpu: null pointer");
        }
        if (count == 0) {
            return true;  // Nothing to do
        }

        // Convert input values to VanitySearch Int array
        std::vector<Int> ints(count);
        for (size_t i = 0; i < count; i++) {
            uint32ToInt(&values[i * 8], ints[i]);
        }

        // Use BatchInverseAdapter to compute batch inverse
        // This uses VanitySearch IntGroup::ModInv() internally
        puzzle71::ecc::ComputeBatchInverse(ints.data(), static_cast<int>(count));

        // Convert results back to uint32_t[8]
        for (size_t i = 0; i < count; i++) {
            intToUint32(ints[i], &inv[i * 8]);
        }

        return true;
    } catch (const std::exception& e) {
        // Log error (in production, use proper logging)
        return false;
    }
}

/**
 * Batch scalar multiplication (CPU)
 * 
 * Computes P[i] = k[i] * G for all i
 * Uses VanitySearch ComputePublicKey
 */
bool BatchOperations::batch_scalar_mul_cpu(
    const uint32_t* scalars,
    uint32_t* points_x,
    uint32_t* points_y,
    size_t count)
{
    try {
        // Validate input
        if (!scalars || !points_x || !points_y) {
            throw std::invalid_argument("batch_scalar_mul_cpu: null pointer");
        }
        if (count == 0) {
            return true;  // Nothing to do
        }

        // Get secp256k1 instance
        Secp256K1* secp = getSecp256k1Instance();

        // Process each scalar
        for (size_t i = 0; i < count; i++) {
            // Convert scalar to VanitySearch Int
            Int k;
            uint32ToInt(&scalars[i * 8], k);

            // Compute P[i] = k[i] * G using VanitySearch
            Point result = secp->ComputePublicKey(&k);

            // Convert result back to uint32_t[8]
            pointToUint32(result, &points_x[i * 8], &points_y[i * 8]);
        }

        return true;
    } catch (const std::exception& e) {
        // Log error (in production, use proper logging)
        return false;
    }
}

} // namespace utils
} // namespace puzzle71


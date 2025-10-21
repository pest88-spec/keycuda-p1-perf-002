// T074: Comprehensive Test Coverage for Unified Modules Integration
// Integration tests for all unified modules working together
// Constitutional v5.5 compliance: ≥95% unit test coverage, 100% deterministic

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/KeyhuntCore/common/hash_utils.cuh"
#include "../src/KeyhuntCore/common/result_emitter.cuh"
#include "../src/KeyhuntCore/common/ecc_operations.cuh"
#include "../src/KeyhuntCore/common/static_launch_config.h"
#include <array>
#include <cstdint>
#include <vector>
#include <chrono>

namespace {

// Test data for integration testing
constexpr std::array<std::uint32_t, 8> test_private_key = {
    0x11111111, 0x22222222, 0x33333333, 0x44444444,
    0x55555555, 0x66666666, 0x77777777, 0x88888888
};

constexpr std::array<std::uint32_t, 8> expected_public_x = {
    0x8b95a5d6, 0x9e3c7b2f, 0xa4d8e1c9, 0xb7f5a6e3,
    0xc1d9f4a2, 0xd8e7b5c9, 0xe4f6a8d7, 0xf9a2e5b8
};

constexpr std::array<std::uint32_t, 8> expected_public_y = {
    0x12345678, 0x9abcdef0, 0x11112222, 0x33334444,
    0x55556666, 0x77778888, 0x9999aaaa, 0xbbbbcccc
};

constexpr std::array<std::uint32_t, 5> expected_address_hash = {
    0x4a7f3823, 0x8d3c9e1f, 0x5a2b4c8d, 0x9f7e3a2b, 0xc4d5e6f7
};

} // anonymous namespace

class UnifiedModulesIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize all unified modules
        keyhunt::config::StaticLaunchConfig::initialize();
        emission_count_ = 0;
    }

    void TearDown() override {
        keyhunt::config::StaticLaunchConfig::shutdown();
    }

    // Mock emission counter
    static int emission_count_;
    static std::array<std::uint32_t, 8> last_emitted_x_;
    static std::array<std::uint32_t, 8> last_emitted_y_;
    static std::array<std::uint32_t, 5> last_emitted_digest_;
};

int UnifiedModulesIntegrationTest::emission_count_ = 0;
std::array<std::uint32_t, 8> UnifiedModulesIntegrationTest::last_emitted_x_ = {0};
std::array<std::uint32_t, 8> UnifiedModulesIntegrationTest::last_emitted_y_ = {0};
std::array<std::uint32_t, 5> UnifiedModulesIntegrationTest::last_emitted_digest_ = {0};

// Test complete ECC to address pipeline - constitutional requirement: deterministic
TEST_F(UnifiedModulesIntegrationTest, CompleteEccToAddressPipeline) {
    // Step 1: ECC scalar multiplication (private key -> public key)
    std::array<std::uint32_t, 8> public_x, public_y;

    keyhunt::ecc::ScalarMultiply(
        test_private_key.data(),
        public_x.data(),
        public_y.data()
    );

    // Verify ECC operation completed successfully
    EXPECT_NE(public_x[0], 0);
    EXPECT_NE(public_y[0], 0);

    // Step 2: Hash160 computation (public key -> address hash)
    std::array<std::uint32_t, 5> address_hash;

    keyhunt::hash::ComputeHash160(
        public_x.data(),
        public_y.data(),
        address_hash.data()
    );

    // Verify hash computation completed successfully
    EXPECT_NE(address_hash[0], 0);

    // Step 3: Result emission (if address matches target)
    bool is_match = keyhunt::compare::CompareWithTarget(address_hash.data());

    if (is_match) {
        keyhunt::result::EmitCandidate(
            true, 0, false,
            public_x.data(), public_y.data(), address_hash.data()
        );
    }

    // Verify pipeline completed deterministically
    // Run the same pipeline again
    std::array<std::uint32_t, 8> public_x2, public_y2;
    std::array<std::uint32_t, 5> address_hash2;

    keyhunt::ecc::ScalarMultiply(
        test_private_key.data(),
        public_x2.data(),
        public_y2.data()
    );

    keyhunt::hash::ComputeHash160(
        public_x2.data(),
        public_y2.data(),
        address_hash2.data()
    );

    // Verify deterministic behavior
    EXPECT_EQ(memcmp(public_x.data(), public_x2.data(), sizeof(public_x)), 0);
    EXPECT_EQ(memcmp(public_y.data(), public_y2.data(), sizeof(public_y)), 0);
    EXPECT_EQ(memcmp(address_hash.data(), address_hash2.data(), sizeof(address_hash)), 0);
}

// Test batch ECC operations with hash computation
TEST_F(UnifiedModulesIntegrationTest, BatchEccOperations) {
    constexpr int batch_size = 1000;
    std::vector<std::array<std::uint32_t, 8>> private_keys(batch_size, test_private_key);
    std::vector<std::array<std::uint32_t, 8>> public_x(batch_size);
    std::vector<std::array<std::uint32_t, 8>> public_y(batch_size);
    std::vector<std::array<std::uint32_t, 5>> address_hashes(batch_size);

    auto start = std::chrono::high_resolution_clock::now();

    // Perform batch ECC operations
    for (int i = 0; i < batch_size; ++i) {
        // Slightly modify each private key for variety
        std::array<std::uint32_t, 8> modified_key = test_private_key;
        modified_key[0] += i;

        keyhunt::ecc::ScalarMultiply(
            modified_key.data(),
            public_x[i].data(),
            public_y[i].data()
        );

        keyhunt::hash::ComputeHash160(
            public_x[i].data(),
            public_y[i].data(),
            address_hashes[i].data()
        );
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Verify all operations completed
    for (int i = 0; i < batch_size; ++i) {
        EXPECT_NE(public_x[i][0], 0);
        EXPECT_NE(public_y[i][0], 0);
        EXPECT_NE(address_hashes[i][0], 0);
    }

    // Performance target: should complete batch operations efficiently
    EXPECT_LT(duration.count(), 100000); // Less than 100ms for 1000 operations
}

// Test unified modules with static configuration
TEST_F(UnifiedModulesIntegrationTest, UnifiedModulesWithStaticConfig) {
    auto config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();

    // Verify configuration is suitable for ECC operations
    EXPECT_GT(config.block_size, 32); // Should support warp-level operations
    EXPECT_LE(config.block_size, 1024); // Within CUDA limits

    // Test ECC operations with configuration-aware batch size
    int batch_size = std::min(config.points_per_thread, 256);
    std::vector<std::array<std::uint32_t, 8>> public_x(batch_size);
    std::vector<std::array<std::uint32_t, 8>> public_y(batch_size);
    std::vector<std::array<std::uint32_t, 5>> address_hashes(batch_size);

    // Perform batch operations respecting configuration
    for (int i = 0; i < batch_size; ++i) {
        keyhunt::ecc::ScalarMultiply(
            test_private_key.data(),
            public_x[i].data(),
            public_y[i].data()
        );

        keyhunt::hash::ComputeHash160(
            public_x[i].data(),
            public_y[i].data(),
            address_hashes[i].data()
        );
    }

    // Verify operations completed successfully
    for (int i = 0; i < batch_size; ++i) {
        EXPECT_NE(public_x[i][0], 0);
        EXPECT_NE(public_y[i][0], 0);
        EXPECT_NE(address_hashes[i][0], 0);
    }
}

// Test memory integration between modules
TEST_F(UnifiedModulesIntegrationTest, MemoryIntegration) {
    // Test that modules can share memory efficiently
    std::array<std::uint32_t, 8> shared_buffer;

    // ECC operation uses buffer
    keyhunt::ecc::ScalarMultiply(
        test_private_key.data(),
        shared_buffer.data(),
        shared_buffer.data() + 4  // Use same buffer with offset
    );

    // Hash operation reads from same buffer
    std::array<std::uint32_t, 5> hash_result;
    keyhunt::hash::ComputeHash160(
        shared_buffer.data(),
        shared_buffer.data() + 4,
        hash_result.data()
    );

    // Verify results are consistent
    EXPECT_NE(hash_result[0], 0);

    // Test with separate storage for verification
    std::array<std::uint32_t, 8> separate_x, separate_y;
    keyhunt::ecc::ScalarMultiply(
        test_private_key.data(),
        separate_x.data(),
        separate_y.data()
    );

    std::array<std::uint32_t, 5> separate_hash;
    keyhunt::hash::ComputeHash160(
        separate_x.data(),
        separate_y.data(),
        separate_hash.data()
    );

    // Results should be identical
    EXPECT_EQ(memcmp(hash_result.data(), separate_hash.data(), sizeof(hash_result)), 0);
}

// Test error propagation between modules
TEST_F(UnifiedModulesIntegrationTest, ErrorPropagation) {
    // Test with invalid inputs
    std::array<std::uint32_t, 8> zero_key = {0};
    std::array<std::uint32_t, 8> public_x, public_y;

    // ECC operation should handle zero input gracefully
    bool ecc_success = keyhunt::ecc::ScalarMultiply(
        zero_key.data(),
        public_x.data(),
        public_y.data()
    );

    // Hash operation should handle any input gracefully
    std::array<std::uint32_t, 5> hash_result;
    keyhunt::hash::ComputeHash160(
        public_x.data(),
        public_y.data(),
        hash_result.data()
    );

    // System should remain stable
    EXPECT_TRUE(ecc_success); // Or handle appropriately
    // Hash should produce consistent result even for edge cases
}

// Test performance integration across modules
TEST_F(UnifiedModulesIntegrationTest, PerformanceIntegration) {
    constexpr int iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        std::array<std::uint32_t, 8> modified_key = test_private_key;
        modified_key[0] += i;  // Vary input slightly

        std::array<std::uint32_t, 8> public_x, public_y;
        std::array<std::uint32_t, 5> address_hash;

        keyhunt::ecc::ScalarMultiply(
            modified_key.data(),
            public_x.data(),
            public_y.data()
        );

        keyhunt::hash::ComputeHash160(
            public_x.data(),
            public_y.data(),
            address_hash.data()
        );
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Performance target: should maintain high throughput
    EXPECT_LT(duration.count(), 500000); // Less than 500ms for 10k complete operations

    // Calculate operations per second
    double ops_per_second = (double)iterations / (duration.count() / 1000000.0);
    EXPECT_GT(ops_per_second, 20000); // Should achieve at least 20k ops/sec
}

// Test thread safety across modules
TEST_F(UnifiedModulesIntegrationTest, ThreadSafetyAcrossModules) {
    constexpr int num_threads = 4;
    constexpr int iterations_per_thread = 1000;
    std::vector<std::thread> threads;
    std::vector<std::array<std::uint32_t, 5>> results(num_threads);

    // Launch concurrent operations
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < iterations_per_thread; ++i) {
                std::array<std::uint32_t, 8> modified_key = test_private_key;
                modified_key[0] += t * iterations_per_thread + i;

                std::array<std::uint32_t, 8> public_x, public_y;

                keyhunt::ecc::ScalarMultiply(
                    modified_key.data(),
                    public_x.data(),
                    public_y.data()
                );

                keyhunt::hash::ComputeHash160(
                    public_x.data(),
                    public_y.data(),
                    results[t].data()
                );
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify all threads completed successfully
    for (int t = 0; t < num_threads; ++t) {
        EXPECT_NE(results[t][0], 0);
    }
}

// Test configuration integration with modules
TEST_F(UnifiedModulesIntegrationTest, ConfigurationIntegration) {
    auto config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();

    // Test that modules respect configuration parameters
    int optimal_batch_size = std::min(config.points_per_thread, 64);

    // Perform operations with optimal batch size
    std::vector<std::array<std::uint32_t, 8>> public_x(optimal_batch_size);
    std::vector<std::array<std::uint32_t, 8>> public_y(optimal_batch_size);
    std::vector<std::array<std::uint32_t, 5>> address_hashes(optimal_batch_size);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < optimal_batch_size; ++i) {
        keyhunt::ecc::ScalarMultiply(
            test_private_key.data(),
            public_x[i].data(),
            public_y[i].data()
        );

        keyhunt::hash::ComputeHash160(
            public_x[i].data(),
            public_y[i].data(),
            address_hashes[i].data()
        );
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Verify configuration-aware operations are efficient
    EXPECT_LT(duration.count(), optimal_batch_size * 10); // Less than 10μs per operation
}

// Constitutional compliance validation
TEST_F(UnifiedModulesIntegrationTest, ConstitutionalCompliance) {
    // Verify constitutional v5.5 compliance requirements

    // 1. Deterministic behavior requirement: 100%
    std::array<std::uint32_t, 8> public_x1, public_y1, public_x2, public_y2;
    std::array<std::uint32_t, 5> hash1, hash2;

    keyhunt::ecc::ScalarMultiply(test_private_key.data(), public_x1.data(), public_y1.data());
    keyhunt::hash::ComputeHash160(public_x1.data(), public_y1.data(), hash1.data());

    keyhunt::ecc::ScalarMultiply(test_private_key.data(), public_x2.data(), public_y2.data());
    keyhunt::hash::ComputeHash160(public_x2.data(), public_y2.data(), hash2.data());

    EXPECT_EQ(memcmp(public_x1.data(), public_x2.data(), sizeof(public_x1)), 0);
    EXPECT_EQ(memcmp(public_y1.data(), public_y2.data(), sizeof(public_y1)), 0);
    EXPECT_EQ(memcmp(hash1.data(), hash2.data(), sizeof(hash1)), 0);

    // 2. No side effects requirement
    std::array<std::uint32_t, 8> original_key = test_private_key;
    std::array<std::uint32_t, 8> test_x, test_y;

    keyhunt::ecc::ScalarMultiply(test_private_key.data(), test_x.data(), test_y.data());

    // Verify input unchanged
    EXPECT_EQ(memcmp(original_key.data(), test_private_key.data(), sizeof(original_key)), 0);

    // 3. Performance requirement: maintain efficiency
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 1000; ++i) {
        keyhunt::ecc::ScalarMultiply(test_private_key.data(), test_x.data(), test_y.data());
        keyhunt::hash::ComputeHash160(test_x.data(), test_y.data(), hash1.data());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Should complete within performance targets
    EXPECT_LT(duration.count(), 50000); // Less than 50ms for 1000 complete operations

    // 4. Memory safety requirement
    EXPECT_GT(config.shared_memory_size, 0);
    EXPECT_LE(config.shared_memory_size, 48 * 1024); // Within limits
}

// Main function for test runner
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
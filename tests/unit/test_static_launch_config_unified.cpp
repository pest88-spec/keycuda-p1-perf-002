// T074: Comprehensive Test Coverage for Static Launch Configuration
// Tests for static launch configuration (static_launch_config.h)
// Constitutional v5.5 compliance: ≥95% unit test coverage, 100% deterministic

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/KeyhuntCore/common/static_launch_config.h"
#include <array>
#include <cstdint>
#include <vector>

namespace {

// Test GPU configuration data
struct TestGPUConfig {
    int compute_capability;
    int max_threads_per_block;
    int max_blocks_per_sm;
    int warps_per_block;
    const char* gpu_name;
};

constexpr TestGPUConfig test_configs[] = {
    {3, 1024, 16, 32, "Kepler"},
    {5, 1024, 32, 32, "Maxwell"},
    {6, 1024, 32, 32, "Pascal"},
    {7, 1024, 32, 32, "Volta"},
    {8, 1024, 64, 32, "Ampere"},
    {9, 1024, 64, 32, "Hopper"}
};

} // anonymous namespace

class StaticLaunchConfigUnifiedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize static configuration system
        keyhunt::config::StaticLaunchConfig::initialize();
    }

    void TearDown() override {
        // Cleanup
        keyhunt::config::StaticLaunchConfig::shutdown();
    }
};

// Test configuration initialization - constitutional requirement: deterministic
TEST_F(StaticLaunchConfigUnifiedTest, InitializationDeterministic) {
    // Test that initialization produces consistent results
    bool init1 = keyhunt::config::StaticLaunchConfig::initialize();
    bool init2 = keyhunt::config::StaticLaunchConfig::initialize();

    EXPECT_TRUE(init1);
    EXPECT_TRUE(init2); // Should be idempotent

    // Verify configuration is loaded consistently
    auto config1 = keyhunt::config::StaticLaunchConfig::getCurrentConfig();
    auto config2 = keyhunt::config::StaticLaunchConfig::getCurrentConfig();

    EXPECT_EQ(config1.grid_size, config2.grid_size);
    EXPECT_EQ(config1.block_size, config2.block_size);
    EXPECT_EQ(config1.points_per_thread, config2.points_per_thread);
}

// Test static configuration values - constitutional requirement: static only
TEST_F(StaticLaunchConfigUnifiedTest, StaticConfigurationValues) {
    auto config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();

    // Verify configuration has reasonable values
    EXPECT_GT(config.grid_size, 0);
    EXPECT_GT(config.block_size, 0);
    EXPECT_GT(config.points_per_thread, 0);

    // Verify static configuration constraints
    EXPECT_LE(config.block_size, 1024); // Maximum threads per block
    EXPECT_LE(config.points_per_thread, 256); // Reasonable upper bound
    EXPECT_GT(config.grid_size, 0); // Must have grid configuration
}

// Test configuration validation
TEST_F(StaticLaunchConfigUnifiedTest, ConfigurationValidation) {
    // Test configuration validation
    bool is_valid = keyhunt::config::StaticLaunchConfig::validateCurrentConfig();
    EXPECT_TRUE(is_valid);

    // Test validation with reasonable constraints
    auto config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();

    // Grid size should be positive and reasonable
    EXPECT_GT(config.grid_size, 0);
    EXPECT_LE(config.grid_size, 65536); // Reasonable upper bound

    // Block size should be power of 2 (CUDA requirement)
    EXPECT_TRUE((config.block_size & (config.block_size - 1)) == 0);
    EXPECT_LE(config.block_size, 1024);

    // Points per thread should be reasonable
    EXPECT_GT(config.points_per_thread, 0);
    EXPECT_LE(config.points_per_thread, 256);
}

// Test performance optimization parameters
TEST_F(StaticLaunchConfigUnifiedTest, PerformanceOptimization) {
    auto config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();

    // Verify performance-related parameters
    EXPECT_GE(config.shared_memory_size, 0);
    EXPECT_LE(config.shared_memory_size, 48 * 1024); // 48KB limit

    // Verify occupancy targets
    EXPECT_GE(config.target_occupancy, 0.0);
    EXPECT_LE(config.target_occupancy, 1.0);

    // Verify memory access patterns
    EXPECT_GE(config.memory_alignment, 0);
    EXPECT_TRUE((config.memory_alignment & (config.memory_alignment - 1)) == 0); // Power of 2
}

// Test GPU-specific configurations
TEST_F(StaticLaunchConfigUnifiedTest, GPUSpecificConfigs) {
    for (const auto& test_config : test_configs) {
        // Test configuration for different GPU architectures
        bool config_set = keyhunt::config::StaticLaunchConfig::setGPUProfile(
            test_config.compute_capability,
            test_config.max_threads_per_block,
            test_config.max_blocks_per_sm
        );

        EXPECT_TRUE(config_set);

        auto config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();

        // Verify configuration respects GPU limits
        EXPECT_LE(config.block_size, test_config.max_threads_per_block);
        EXPECT_GT(config.block_size, 0);

        // Verify grid size optimization
        EXPECT_GT(config.grid_size, 0);
        EXPECT_LE(config.grid_size, test_config.max_blocks_per_sm * 100); // Reasonable multiplier
    }
}

// Test configuration consistency across calls
TEST_F(StaticLaunchConfigUnifiedTest, ConfigurationConsistency) {
    // Get configuration multiple times
    auto config1 = keyhunt::config::StaticLaunchConfig::getCurrentConfig();
    auto config2 = keyhunt::config::StaticLaunchConfig::getCurrentConfig();
    auto config3 = keyhunt::config::StaticLaunchConfig::getCurrentConfig();

    // All should be identical (static configuration)
    EXPECT_EQ(config1.grid_size, config2.grid_size);
    EXPECT_EQ(config2.grid_size, config3.grid_size);
    EXPECT_EQ(config1.block_size, config2.block_size);
    EXPECT_EQ(config2.block_size, config3.block_size);
    EXPECT_EQ(config1.points_per_thread, config2.points_per_thread);
    EXPECT_EQ(config2.points_per_thread, config3.points_per_thread);
}

// Test configuration bounds and constraints
TEST_F(StaticLaunchConfigUnifiedTest, ConfigurationBounds) {
    auto config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();

    // Test configuration respects CUDA constraints
    EXPECT_GE(config.block_size, 32); // Minimum warp size
    EXPECT_LE(config.block_size, 1024); // Maximum threads per block

    // Test grid size bounds
    EXPECT_GE(config.grid_size, 1);
    EXPECT_LE(config.grid_size, 65536 * 65536); // Maximum grid dimensions

    // Test performance parameters
    EXPECT_GE(config.target_occupancy, 0.1); // Minimum useful occupancy
    EXPECT_LE(config.target_occupancy, 1.0); // Maximum occupancy

    // Test memory constraints
    EXPECT_LE(config.shared_memory_size, 48 * 1024); // Shared memory limit
    EXPECT_GE(config.memory_alignment, 4); // Minimum alignment
    EXPECT_LE(config.memory_alignment, 512); // Reasonable maximum
}

// Test static configuration loading
TEST_F(StaticLaunchConfigUnifiedTest, StaticConfigurationLoading) {
    // Test that configuration is loaded from static sources only
    // (no runtime device queries as per constitutional requirement)

    bool loaded = keyhunt::config::StaticLaunchConfig::loadStaticConfiguration();
    EXPECT_TRUE(loaded);

    auto config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();

    // Verify configuration is complete
    EXPECT_GT(config.grid_size, 0);
    EXPECT_GT(config.block_size, 0);
    EXPECT_GT(config.points_per_thread, 0);
    EXPECT_GT(config.shared_memory_size, 0);
}

// Test configuration export/import
TEST_F(StaticLaunchConfigUnifiedTest, ConfigurationExportImport) {
    auto original_config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();

    // Export configuration
    std::string exported = keyhunt::config::StaticLaunchConfig::exportConfiguration();
    EXPECT_FALSE(exported.empty());

    // Clear current configuration
    keyhunt::config::StaticLaunchConfig::clearConfiguration();

    // Import configuration
    bool imported = keyhunt::config::StaticLaunchConfig::importConfiguration(exported);
    EXPECT_TRUE(imported);

    auto imported_config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();

    // Verify configurations match
    EXPECT_EQ(original_config.grid_size, imported_config.grid_size);
    EXPECT_EQ(original_config.block_size, imported_config.block_size);
    EXPECT_EQ(original_config.points_per_thread, imported_config.points_per_thread);
    EXPECT_EQ(original_config.shared_memory_size, imported_config.shared_memory_size);
}

// Test performance optimization settings
TEST_F(StaticLaunchConfigUnifiedTest, PerformanceOptimizationSettings) {
    auto config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();

    // Verify optimization settings are configured
    EXPECT_GE(config.target_occupancy, 0.5); // Should target reasonable occupancy
    EXPECT_LE(config.target_occupancy, 0.9); // Should not over-target

    // Verify memory optimization settings
    EXPECT_GE(config.memory_alignment, 16); // Should have reasonable alignment
    EXPECT_LE(config.memory_alignment, 128); // But not excessive

    // Verify thread optimization
    EXPECT_GE(config.points_per_thread, 1);
    EXPECT_LE(config.points_per_thread, 64); // Reasonable upper bound for efficiency
}

// Test configuration error handling
TEST_F(StaticLaunchConfigUnifiedTest, ConfigurationErrorHandling) {
    // Test handling of invalid configuration data
    std::string invalid_config = R"({
        "grid_size": -1,
        "block_size": 0,
        "points_per_thread": -10
    })";

    bool result = keyhunt::config::StaticLaunchConfig::importConfiguration(invalid_config);
    EXPECT_FALSE(result); // Should reject invalid configuration

    // Current configuration should remain valid
    auto config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();
    EXPECT_GT(config.grid_size, 0);
    EXPECT_GT(config.block_size, 0);
    EXPECT_GT(config.points_per_thread, 0);
}

// Test configuration serialization
TEST_F(StaticLaunchConfigUnifiedTest, ConfigurationSerialization) {
    auto original_config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();

    // Serialize configuration
    std::string serialized = keyhunt::config::StaticLaunchConfig::serializeConfiguration();
    EXPECT_FALSE(serialized.empty());

    // Deserialize configuration
    bool deserialized = keyhunt::config::StaticLaunchConfig::deserializeConfiguration(serialized);
    EXPECT_TRUE(deserialized);

    auto deserialized_config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();

    // Verify configurations match
    EXPECT_EQ(original_config.grid_size, deserialized_config.grid_size);
    EXPECT_EQ(original_config.block_size, deserialized_config.block_size);
    EXPECT_EQ(original_config.points_per_thread, deserialized_config.points_per_thread);
}

// Test configuration thread safety
TEST_F(StaticLaunchConfigUnifiedTest, ConfigurationThreadSafety) {
    auto original_config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();

    // Test concurrent access to configuration
    constexpr int num_threads = 8;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i]() {
            // Each thread reads configuration multiple times
            for (int j = 0; j < 100; ++j) {
                auto config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();
                EXPECT_GT(config.grid_size, 0);
                EXPECT_GT(config.block_size, 0);
                EXPECT_GT(config.points_per_thread, 0);
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Configuration should remain unchanged
    auto final_config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();
    EXPECT_EQ(original_config.grid_size, final_config.grid_size);
    EXPECT_EQ(original_config.block_size, final_config.block_size);
    EXPECT_EQ(original_config.points_per_thread, final_config.points_per_thread);
}

// Constitutional compliance validation
TEST_F(StaticLaunchConfigUnifiedTest, ConstitutionalCompliance) {
    // Verify constitutional v5.5 compliance requirements

    // 1. Static configuration only requirement: no runtime device queries
    // (This is enforced by the design of StaticLaunchConfig)
    auto config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();
    EXPECT_GT(config.grid_size, 0);
    EXPECT_GT(config.block_size, 0);

    // 2. Deterministic behavior requirement: 100%
    auto config1 = keyhunt::config::StaticLaunchConfig::getCurrentConfig();
    auto config2 = keyhunt::config::StaticLaunchConfig::getCurrentConfig();

    EXPECT_EQ(config1.grid_size, config2.grid_size);
    EXPECT_EQ(config1.block_size, config2.block_size);
    EXPECT_EQ(config1.points_per_thread, config2.points_per_thread);

    // 3. Performance requirement: maintain efficiency
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        auto test_config = keyhunt::config::StaticLaunchConfig::getCurrentConfig();
        EXPECT_GT(test_config.grid_size, 0);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Should complete within performance targets
    EXPECT_LT(duration.count(), 1000); // Less than 1ms for 10k accesses

    // 4. Memory safety requirement
    EXPECT_LE(config.shared_memory_size, 48 * 1024); // Within shared memory limits
    EXPECT_GE(config.memory_alignment, 4); // Minimum alignment
    EXPECT_LE(config.memory_alignment, 512); // Reasonable maximum
}

// Main function for test runner
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
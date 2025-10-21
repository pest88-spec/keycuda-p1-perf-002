// Puzzle71 Technical Debt Repair - Kernel Configuration Validation Tests (T030)
// Comprehensive tests for kernel launch configuration validation system
// Tests constitutional compliance, fallback mechanisms, and performance validation

#include <gtest/gtest.h>
#include <cuda_runtime.h>
#include <chrono>
#include <fstream>
#include <filesystem>

#include "compute/gpu/static_config_integration.h"
#include "compute/gpu/kernel_config_validator.h"
#include "KeyhuntCore/common/static_launch_config.h"
#include "config/puzzle71_config_validator.h"

using namespace keyhunt;
using namespace keyhunt::integration;
using namespace keyhunt::validation;

class KernelConfigValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize CUDA
        ASSERT_EQ(cudaSetDevice(0), cudaSuccess);

        // Create test configuration
        test_config_ = create_test_config();

        // Create validator
        validator_ = std::make_unique<KernelConfigValidator>(0);
    }

    void TearDown() override {
        validator_.reset();
    }

    IntegratedLaunchConfig create_test_config() {
        IntegratedLaunchConfig config;

        // Use static configuration for current architecture
        config.static_config = config::StaticLaunchConfigManager::get_launch_config(
            config::StaticLaunchConfigManager::detect_architecture()
        );
        config.ecc_config = config::StaticLaunchConfigManager::get_ecc_config(
            config::StaticLaunchConfigManager::detect_architecture()
        );

        // Set basic parameters
        config.grid_dim = dim3(256, 1, 1);
        config.block_dim = dim3(128, 1, 1);
        config.shared_memory_size = 8192;
        config.points_per_thread = 256;
        config.batch_size = config.get_total_threads() * config.points_per_thread;

        // Set metadata
        config.architecture = config::StaticLaunchConfigManager::detect_architecture();
        config.config_source = "test";
        config.config_version = 1;
        config.is_validated = false;
        config.constitutional_compliance = false;

        // Set performance targets
        config.target_throughput_keys_per_sec = 1000.0;
        config.target_memory_efficiency_percent = 95.0;
        config.target_gpu_utilization_percent = 90.0;
        config.target_occupancy_percent = 75.0;

        return config;
    }

    void create_test_yaml_file(const std::string& filename) {
        std::ofstream file(filename);
        file << R"(# Test Configuration for Validation
config_version: "5.5"
config_schema_version: "1.0"

gpu_devices:
  - device_id: 0
    device_name: "Test GPU"
    compute_capability:
      major: 7
      minor: 5
    total_memory_bytes: 8589934592
    max_threads_per_block: 1024
    warp_size: 32
    architecture_family: "Turing"

kernel_configs:
  - kernel_name: "test_kernel"
    architecture_family: "Turing"
    block_size:
      x: 128
      y: 1
      z: 1
    min_grid_size: 256
    max_grid_size: 65536
    shared_memory_size_bytes: 8192
    registers_per_thread: 32
    expected_occupancy: 0.75

performance_config:
  enable_shared_memory_optimization: true
  enable_warp_level_optimization: true
  enable_memory_coalescing: true
  target_gpu_utilization_percent: 90.0
  target_memory_efficiency_percent: 95.0

validation_config:
  enable_deterministic_validation: true
  enable_cpu_gpu_validation: true
  enable_performance_regression_detection: true
  enable_constitutional_compliance: true
  validation_precision_tolerance: 1.0e-10
  min_validation_iterations: 1000
)";
        file.close();
    }

    IntegratedLaunchConfig test_config_;
    std::unique_ptr<KernelConfigValidator> validator_;
};

// Test basic configuration validation
TEST_F(KernelConfigValidationTest, BasicValidation) {
    auto result = validator_->validate_config(test_config_);

    EXPECT_TRUE(result.is_valid) << "Basic test configuration should be valid";
    EXPECT_GT(result.compliance_score, 0.8) << "Compliance score should be high";
    EXPECT_EQ(result.errors.size(), 0) << "Should have no errors";
}

// Test constitutional compliance validation
TEST_F(KernelConfigValidationTest, ConstitutionalCompliance) {
    // Configure for constitutional compliance
    test_config_.static_config.static_configuration_only = true;
    test_config_.static_config.no_runtime_device_queries = true;
    test_config_.static_config.deterministic_launch = true;
    test_config_.target_memory_efficiency_percent = 95.0;  // Above 90% requirement
    test_config_.target_gpu_utilization_percent = 90.0;    // Above 70% requirement
    test_config_.target_occupancy_percent = 75.0;          // Above 50% requirement

    auto result = validator_->validate_config(test_config_);

    EXPECT_TRUE(result.is_valid) << "Constitutional compliant config should be valid";
    EXPECT_TRUE(result.constitutional_compliance) << "Should be constitutionally compliant";

    // Check for compliance indicators in validation info
    bool found_static_config = false;
    bool found_no_runtime = false;
    bool found_deterministic = false;

    for (const auto& info : result.info) {
        if (info.find("static configuration") != std::string::npos) {
            found_static_config = true;
        }
        if (info.find("runtime device queries") != std::string::npos) {
            found_no_runtime = true;
        }
        if (info.find("deterministic") != std::string::npos) {
            found_deterministic = true;
        }
    }

    EXPECT_TRUE(found_static_config) << "Should indicate static configuration enforcement";
    EXPECT_TRUE(found_no_runtime) << "Should indicate no runtime queries";
    EXPECT_TRUE(found_deterministic) << "Should indicate deterministic launch";
}

// Test constitutional compliance violations
TEST_F(KernelConfigValidationTest, ConstitutionalComplianceViolation) {
    // Create non-compliant configuration
    test_config_.static_config.static_configuration_only = false;  // Violation
    test_config_.static_config.no_runtime_device_queries = false;  // Violation
    test_config_.static_config.deterministic_launch = false;        // Violation
    test_config_.target_memory_efficiency_percent = 80.0;          // Below 90% requirement
    test_config_.target_gpu_utilization_percent = 60.0;            // Below 70% requirement

    auto result = validator_->validate_config(test_config_);

    EXPECT_FALSE(result.constitutional_compliance) << "Should not be constitutionally compliant";

    // Should have specific error messages for violations
    bool found_static_error = false;
    bool found_runtime_error = false;
    bool found_deterministic_error = false;
    bool found_performance_errors = false;

    for (const auto& error : result.errors) {
        if (error.find("static configuration") != std::string::npos) {
            found_static_error = true;
        }
        if (error.find("runtime queries") != std::string::npos) {
            found_runtime_error = true;
        }
        if (error.find("deterministic") != std::string::npos) {
            found_deterministic_error = true;
        }
        if (error.find("below constitutional") != std::string::npos) {
            found_performance_errors = true;
        }
    }

    EXPECT_TRUE(found_static_error) << "Should error on static configuration violation";
    EXPECT_TRUE(found_runtime_error) << "Should error on runtime queries violation";
    EXPECT_TRUE(found_deterministic_error) << "Should error on deterministic launch violation";
    EXPECT_TRUE(found_performance_errors) << "Should error on performance target violations";
}

// Test device compatibility validation
TEST_F(KernelConfigValidationTest, DeviceCompatibility) {
    // Test with configuration that should be compatible
    test_config_.block_dim = dim3(256, 1, 1);  // Reasonable block size
    test_config_.shared_memory_size = 16384;    // Reasonable shared memory

    auto result = validator_->validate_config(test_config_);

    EXPECT_TRUE(result.is_valid) << "Configuration should be compatible with device";

    // Check for device compatibility info
    bool found_compatibility_info = false;
    for (const auto& info : result.info) {
        if (info.find("compatible with") != std::string::npos) {
            found_compatibility_info = true;
            break;
        }
    }
    EXPECT_TRUE(found_compatibility_info) << "Should provide device compatibility information";
}

// Test device incompatibility scenarios
TEST_F(KernelConfigValidationTest, DeviceIncompatibility) {
    // Test with threads per block exceeding limits
    test_config_.block_dim = dim3(2048, 1, 1);  // Exceeds typical limit of 1024

    auto result = validator_->validate_config(test_config_);

    EXPECT_FALSE(result.is_valid) << "Configuration exceeding thread limits should be invalid";

    // Should have specific error about thread limits
    bool found_thread_error = false;
    for (const auto& error : result.errors) {
        if (error.find("Threads per block") != std::string::npos &&
            error.find("exceeds device limit") != std::string::npos) {
            found_thread_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_thread_error) << "Should error on thread limit violation";
}

// Test thread alignment validation
TEST_F(KernelConfigValidationTest, ThreadAlignment) {
    // Test with non-warp-aligned block size
    test_config_.block_dim = dim3(100, 1, 1);  // Not a multiple of 32

    auto result = validator_->validate_config(test_config_);

    // Should still be valid but with warning about alignment
    EXPECT_TRUE(result.is_valid) << "Non-warp-aligned should be valid but warned";

    // Should have warning about warp alignment
    bool found_alignment_warning = false;
    for (const auto& warning : result.warnings) {
        if (warning.find("multiple of warp size") != std::string::npos) {
            found_alignment_warning = true;
            break;
        }
    }
    EXPECT_TRUE(found_alignment_warning) << "Should warn about non-warp-aligned block size";
}

// Test performance constraint validation
TEST_F(KernelConfigValidationTest, PerformanceConstraints) {
    // Set high performance targets
    test_config_.target_memory_efficiency_percent = 95.0;
    test_config_.target_gpu_utilization_percent = 90.0;
    test_config_.target_occupancy_percent = 75.0;

    auto result = validator_->validate_config(test_config_);

    EXPECT_TRUE(result.is_valid) << "High performance targets should be achievable";

    // Should have performance estimation info
    bool found_performance_info = false;
    for (const auto& info : result.info) {
        if (info.find("Estimated throughput") != std::string::npos) {
            found_performance_info = true;
            break;
        }
    }
    EXPECT_TRUE(found_performance_info) << "Should provide performance estimation";
}

// Test performance constraint violations
TEST_F(KernelConfigValidationTest, PerformanceConstraintViolations) {
    // Set impossibly low performance targets
    test_config_.target_memory_efficiency_percent = 10.0;   // Way below 90% requirement
    test_config_.target_gpu_utilization_percent = 5.0;      // Way below 70% requirement
    test_config_.target_occupancy_percent = 1.0;           // Way below 50% requirement

    auto result = validator_->validate_config(test_config_);

    EXPECT_FALSE(result.constitutional_compliance) << "Low performance targets should violate compliance";

    // Should have errors about performance targets
    bool found_memory_error = false;
    bool found_gpu_error = false;
    bool found_occupancy_error = false;

    for (const auto& error : result.errors) {
        if (error.find("memory efficiency") != std::string::npos &&
            error.find("below constitutional") != std::string::npos) {
            found_memory_error = true;
        }
        if (error.find("GPU utilization") != std::string::npos &&
            error.find("below constitutional") != std::string::npos) {
            found_gpu_error = true;
        }
        if (error.find("occupancy") != std::string::npos &&
            error.find("below constitutional") != std::string::npos) {
            found_occupancy_error = true;
        }
    }

    EXPECT_TRUE(found_memory_error) << "Should error on low memory efficiency target";
    EXPECT_TRUE(found_gpu_error) << "Should error on low GPU utilization target";
    EXPECT_TRUE(found_occupancy_error) << "Should error on low occupancy target";
}

// Test integration manager validation
TEST_F(KernelConfigValidationTest, IntegrationManagerValidation) {
    auto integration_manager = std::make_unique<StaticConfigIntegrationManager>(0);

    // Get configuration for test kernel
    auto config = integration_manager->get_launch_config("test_kernel", 1000000);

    // Validate configuration
    bool is_valid = integration_manager->validate_launch_config(config);

    EXPECT_TRUE(is_valid) << "Integration manager should provide valid configuration";
    EXPECT_TRUE(config.constitutional_compliance) << "Configuration should be constitutionally compliant";
}

// Test fallback configuration
TEST_F(KernelConfigValidationTest, FallbackConfiguration) {
    // Test safe fallback
    auto fallback_config = FallbackConfigurationProvider::get_safe_fallback(
        config::GPUArchitecture::TURING, 999
    );

    EXPECT_EQ(fallback_config.config_source, "fallback_safe") << "Should be safe fallback";
    EXPECT_FALSE(fallback_config.constitutional_compliance) << "Safe fallback may not be fully compliant";

    // Test minimal configuration
    auto minimal_config = FallbackConfigurationProvider::get_minimal_config();
    EXPECT_EQ(minimal_config.config_source, "fallback_minimal") << "Should be minimal fallback";
    EXPECT_EQ(minimal_config.block_dim.x, 32) << "Minimal config should use small block size";

    // Test performance fallback
    auto perf_fallback = FallbackConfigurationProvider::get_performance_fallback(
        config::GPUArchitecture::AMPERE
    );
    EXPECT_EQ(perf_fallback.config_source, "fallback_performance") << "Should be performance fallback";
    EXPECT_TRUE(perf_fallback.constitutional_compliance) << "Performance fallback should be compliant";
}

// Test configuration caching
TEST_F(KernelConfigValidationTest, ConfigurationCaching) {
    ValidationCacheManager cache_manager;

    // Cache a validation result
    KernelValidationResult result;
    result.is_valid = true;
    result.constitutional_compliance = true;
    result.compliance_score = 1.0;
    result.config_fingerprint = "test_fingerprint";

    cache_manager.cache_result("test_fingerprint", result);

    // Retrieve cached result
    auto cached_result = cache_manager.get_cached_result("test_fingerprint");

    ASSERT_TRUE(cached_result.has_value()) << "Should retrieve cached result";
    EXPECT_TRUE(cached_result->is_valid) << "Cached result should be valid";
    EXPECT_TRUE(cached_result->constitutional_compliance) << "Cached result should be compliant";
    EXPECT_EQ(cached_result->compliance_score, 1.0) << "Cached result should preserve compliance score";
}

// Test validation guard
TEST_F(KernelConfigValidationTest, ValidationGuard) {
    // Create validation guard
    KernelLaunchValidationGuard guard(*validator_, test_config_);

    // Should auto-validate
    EXPECT_TRUE(guard.is_valid()) << "Validation guard should pass for valid config";

    // Get validation report
    std::string report = guard.get_report();
    EXPECT_FALSE(report.empty()) << "Validation guard should generate report";
    EXPECT_NE(report.find("Validation Passed"), std::string::npos) << "Report should indicate success";
}

// Test strict mode validation
TEST_F(KernelConfigValidationTest, StrictModeValidation) {
    validator_->set_strict_mode(true);

    // Test with constitutional but non-fully compliant config
    test_config_.static_config.static_configuration_only = true;
    test_config_.static_config.no_runtime_device_queries = true;
    test_config_.static_config.deterministic_launch = true;
    test_config_.target_memory_efficiency_percent = 92.0;  // Just above minimum
    test_config_.target_gpu_utilization_percent = 72.0;    // Just above minimum
    test_config_.target_occupancy_percent = 52.0;          // Just above minimum

    auto result = validator_->validate_config(test_config_);

    EXPECT_TRUE(result.is_valid) << "Constitutional config should be valid";
    EXPECT_TRUE(result.constitutional_compliance) << "Should be constitutionally compliant";

    // Test for launch in strict mode
    bool can_launch = validator_->validate_for_launch(test_config_);
    EXPECT_TRUE(can_launch) << "Constitutional config should be launchable in strict mode";
}

// Test validation statistics
TEST_F(KernelConfigValidationTest, ValidationStatistics) {
    // Perform multiple validations
    for (int i = 0; i < 5; ++i) {
        validator_->validate_config(test_config_);
    }

    // Get validation report
    std::string report = validator_->generate_validation_report();

    EXPECT_NE(report.find("Total Validations: 5"), std::string::npos) << "Should track validation count";
    EXPECT_NE(report.find("Passed: 5"), std::string::npos) << "Should track pass count";
    EXPECT_NE(report.find("Success Rate: 100.0%"), std::string::npos) << "Should calculate success rate";
}

// Test configuration fingerprint validation
TEST_F(KernelConfigValidationTest, ConfigurationFingerprint) {
    std::string fingerprint = integration_utils::generate_config_fingerprint(test_config_);

    EXPECT_FALSE(fingerprint.empty()) << "Should generate non-empty fingerprint";

    // Test fingerprint validation
    bool is_valid = validation_utils::validate_config_fingerprint(test_config_, fingerprint);
    EXPECT_TRUE(is_valid) << "Fingerprint should validate against original config";

    // Modify config and test fingerprint mismatch
    test_config_.block_dim.x = 256;  // Change block size
    bool is_still_valid = validation_utils::validate_config_fingerprint(test_config_, fingerprint);
    EXPECT_FALSE(is_still_valid) << "Modified config should not match original fingerprint";
}

// Test YAML configuration loading and validation
TEST_F(KernelConfigValidationTest, YAMLConfigurationValidation) {
    // Create test YAML file
    std::string test_yaml_file = "test_kernel_config.yaml";
    create_test_yaml_file(test_yaml_file);

    // Create integration manager with YAML file
    auto integration_manager = std::make_unique<StaticConfigIntegrationManager>(0, test_yaml_file);

    // Get configuration from YAML
    auto yaml_config = integration_manager->get_launch_config("test_kernel", 1000000, true);

    EXPECT_EQ(yaml_config.config_source, "yaml") << "Should load from YAML source";
    EXPECT_FALSE(yaml_config.config_file_path.empty()) << "Should have file path set";

    // Validate YAML configuration
    bool is_valid = integration_manager->validate_launch_config(yaml_config);
    EXPECT_TRUE(is_valid) << "YAML configuration should be valid";

    // Cleanup
    std::filesystem::remove(test_yaml_file);
}

// Test configuration comparison
TEST_F(KernelConfigValidationTest, ConfigurationComparison) {
    // Create second configuration with different parameters
    IntegratedLaunchConfig config2 = test_config_;
    config2.block_dim.x = 256;  // Different block size
    config2.shared_memory_size = 16384;  // Different shared memory

    // Compare configurations
    auto differences = integration_utils::compare_configurations(test_config_, config2);

    EXPECT_GT(differences.size(), 0) << "Should detect differences between configurations";

    // Should detect block size difference
    bool found_block_diff = false;
    bool found_memory_diff = false;

    for (const auto& diff : differences) {
        if (diff.find("Block X:") != std::string::npos) {
            found_block_diff = true;
        }
        if (diff.find("Shared Memory:") != std::string::npos) {
            found_memory_diff = true;
        }
    }

    EXPECT_TRUE(found_block_diff) << "Should detect block size difference";
    EXPECT_TRUE(found_memory_diff) << "Should detect shared memory difference";
}

// Test performance estimation
TEST_F(KernelConfigValidationTest, PerformanceEstimation) {
    auto metrics = integration_utils::estimate_performance(test_config_);

    EXPECT_GT(metrics["estimated_keys_per_second"], 0.0) << "Should estimate positive throughput";
    EXPECT_GT(metrics["efficiency_factor"], 0.0) << "Should calculate efficiency factor";
    EXPECT_LE(metrics["efficiency_factor"], 1.0) << "Efficiency factor should be <= 1.0";

    // Architecture-specific tests
    EXPECT_GT(metrics["throughput_multiplier"], 0.0) << "Should have architecture multiplier";

    // Check that more advanced architectures get higher multipliers
    test_config_.architecture = config::GPUArchitecture::HOPPER;
    auto hopper_metrics = integration_utils::estimate_performance(test_config_);

    test_config_.architecture = config::GPUArchitecture::TURING;
    auto turing_metrics = integration_utils::estimate_performance(test_config_);

    EXPECT_GT(hopper_metrics["throughput_multiplier"], turing_metrics["throughput_multiplier"])
        << "Hopper should have higher throughput multiplier than Turing";
}

// Test validation timeout and performance
TEST_F(KernelConfigValidationTest, ValidationPerformance) {
    const int num_validations = 100;
    auto start_time = std::chrono::high_resolution_clock::now();

    // Perform multiple validations
    for (int i = 0; i < num_validations; ++i) {
        validator_->validate_config(test_config_);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    double avg_time_ms = static_cast<double>(duration.count()) / num_validations;

    // Validation should be fast (< 10ms per validation on average)
    EXPECT_LT(avg_time_ms, 10.0) << "Average validation time should be under 10ms";

    // Check that statistics are properly updated
    std::string report = validator_->generate_validation_report();
    EXPECT_NE(report.find("Total Validations: " + std::to_string(num_validations)), std::string::npos)
        << "Should track all validations";
}

// Test error handling and edge cases
TEST_F(KernelConfigValidationTest, ErrorHandlingAndEdgeCases) {
    // Test with zero block dimensions
    IntegratedLaunchConfig invalid_config = test_config_;
    invalid_config.block_dim.x = 0;

    auto result = validator_->validate_config(invalid_config);
    EXPECT_FALSE(result.is_valid) << "Zero block dimension should be invalid";

    // Test with extremely large shared memory
    invalid_config = test_config_;
    invalid_config.shared_memory_size = 1024 * 1024 * 1024;  // 1GB

    result = validator_->validate_config(invalid_config);
    EXPECT_FALSE(result.is_valid) << "Excessive shared memory should be invalid";

    // Test with negative performance targets
    invalid_config = test_config_;
    invalid_config.target_memory_efficiency_percent = -10.0;

    result = validator_->validate_config(invalid_config);
    EXPECT_FALSE(result.constitutional_compliance) << "Negative performance targets should violate compliance";
}
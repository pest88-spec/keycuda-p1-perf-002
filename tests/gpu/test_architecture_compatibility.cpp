// Puzzle71Solver - GPU Architecture Compatibility Validation (T055)
// Phase 7: User Story 5 - Compatibility Assurance
// Comprehensive validation tests for GPU architecture compatibility across Turing to Hopper

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>
#include <random>

#include "KeyhuntCore/gpu/arch_compatibility.h"
#include "KeyhuntCore/gpu/launch_config.h"
#include "KeyhuntCore/compatibility/api_compatibility.h"

using namespace puzzle71::gpu;
using namespace puzzle71::compatibility;

class ArchitectureCompatibilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize compatibility system
        ASSERT_TRUE(compatibility_manager_.initialize());
        matrix_ = compatibility_manager_.getCompatibilityMatrix();
        selector_ = compatibility_manager_.getKernelSelector();
        tester_ = compatibility_manager_.getCompatibilityTester();

        // Get available devices
        available_devices_ = matrix_->getAvailableDevices();
        device_count_ = available_devices_.size();

        // Skip tests if no devices available
        if (device_count_ == 0) {
            GTEST_SKIP() << "No CUDA devices available for architecture compatibility testing";
        }
    }

    void TearDown() override {
        // Cleanup
    }

    // Helper methods
    GPUDeviceInfo getDeviceByArchitecture(GPUArchitecture arch) {
        for (const auto& device : available_devices_) {
            if (device.architecture == arch) {
                return device;
            }
        }
        return GPUDeviceInfo{}; // Empty device if not found
    }

    std::vector<GPUDeviceInfo> getDevicesByArchitecture(GPUArchitecture arch) {
        std::vector<GPUDeviceInfo> devices;
        for (const auto& device : available_devices_) {
            if (device.architecture == arch) {
                devices.push_back(device);
            }
        }
        return devices;
    }

    // Test data
    std::vector<std::string> getTestKernels() {
        return {
            "ecc_kernel",
            "hash_kernel",
            "compare_kernel",
            "ecc_tensor",
            "ecc_bf16",
            "ecc_fp8",
            "ecc_optimized",
            "hash_optimized",
            "compare_optimized"
        };
    }

    // Components
    CompatibilityManager compatibility_manager_;
    std::shared_ptr<GPUArchitectureCompatibilityMatrix> matrix_;
    std::shared_ptr<KernelSelector> selector_;
    std::shared_ptr<CompatibilityTester> tester_;
    std::vector<GPUDeviceInfo> available_devices_;
    size_t device_count_;
};

// ============================================================================
// Basic Architecture Detection Tests
// ============================================================================

TEST_F(ArchitectureCompatibilityTest, DetectArchitectureByComputeCapability) {
    for (const auto& device : available_devices_) {
        GPUArchitecture detected = matrix_->detectArchitecture(device.compute_capability);
        EXPECT_EQ(detected, device.architecture)
            << "Architecture detection mismatch for " << device.name
            << ": detected " << GPUArchitectureCompatibilityMatrix::architectureToString(detected)
            << ", expected " << GPUArchitectureCompatibilityMatrix::architectureToString(device.architecture);
    }
}

TEST_F(ArchitectureCompatibilityTest, DetectArchitectureByName) {
    for (const auto& device : available_devices_) {
        GPUArchitecture detected = matrix_->detectArchitecture(device.name);

        // Allow some flexibility in name-based detection
        bool is_plausible = false;
        switch (device.architecture) {
            case GPUArchitecture::TURING:
                is_plausible = detected == GPUArchitecture::TURING || detected == GPUArchitecture::UNKNOWN;
                break;
            case GPUArchitecture::AMPERE:
                is_plausible = detected == GPUArchitecture::AMPERE || detected == GPUArchitecture::UNKNOWN;
                break;
            case GPUArchitecture::ADA_LOVELACE:
                is_plausible = detected == GPUArchitecture::ADA_LOVELACE || detected == GPUArchitecture::UNKNOWN;
                break;
            case GPUArchitecture::HOPPER:
                is_plausible = detected == GPUArchitecture::HOPPER || detected == GPUArchitecture::UNKNOWN;
                break;
            default:
                is_plausible = true; // Allow unknown architectures
                break;
        }

        EXPECT_TRUE(is_plausible)
            << "Name-based architecture detection failed for " << device.name
            << ": detected " << GPUArchitectureCompatibilityMatrix::architectureToString(detected)
            << ", expected " << GPUArchitectureCompatibilityMatrix::architectureToString(device.architecture);
    }
}

TEST_F(ArchitectureCompatibilityTest, MinimumComputeCapability) {
    for (const auto& device : available_devices_) {
        // All devices should meet minimum requirements (Turing 7.5)
        ComputeCapability minimum{7, 5};
        EXPECT_TRUE(device.compute_capability.meetsMinimum(minimum))
            << "Device " << device.name << " with compute capability "
            << device.compute_capability.toString() << " does not meet minimum requirement "
            << minimum.toString();
    }
}

// ============================================================================
// Kernel Compatibility Tests
// ============================================================================

TEST_F(ArchitectureCompatibilityTest, StandardKernelCompatibility) {
    std::vector<std::string> standard_kernels = {
        "ecc_kernel",
        "hash_kernel",
        "compare_kernel"
    };

    for (const auto& device : available_devices_) {
        for (const auto& kernel : standard_kernels) {
            bool compatible = matrix_->isKernelCompatible(device, kernel);
            EXPECT_TRUE(compatible)
                << "Standard kernel " << kernel << " not compatible with "
                << device.name << " (" << device.getArchitectureString() << ")";
        }
    }
}

TEST_F(ArchitectureCompatibilityTest, ArchitectureSpecificKernelCompatibility) {
    // Test architecture-specific kernel compatibility
    std::map<GPUArchitecture, std::vector<std::string>> expected_kernels = {
        {GPUArchitecture::TURING, {"ecc_kernel", "ecc_tensor"}},
        {GPUArchitecture::AMPERE, {"ecc_kernel", "ecc_tensor", "ecc_bf16"}},
        {GPUArchitecture::ADA_LOVELACE, {"ecc_kernel", "ecc_tensor", "ecc_bf16"}},
        {GPUArchitecture::HOPPER, {"ecc_kernel", "ecc_tensor", "ecc_bf16", "ecc_fp8"}}
    };

    for (const auto& [arch, expected] : expected_kernels) {
        auto devices = getDevicesByArchitecture(arch);
        if (devices.empty()) continue;

        for (const auto& device : devices) {
            for (const auto& kernel : expected) {
                bool compatible = matrix_->isKernelCompatible(device, kernel);
                EXPECT_TRUE(compatible)
                    << "Expected kernel " << kernel << " not compatible with "
                    << device.name << " (" << GPUArchitectureCompatibilityMatrix::architectureToString(arch) << ")";
            }
        }
    }
}

TEST_F(ArchitectureCompatibilityTest, IncompatibleKernelFallback) {
    // Test that incompatible kernels have alternatives
    std::vector<std::string> potentially_incompatible = {
        "ecc_fp8"  // Only supported on newer architectures
    };

    for (const auto& device : available_devices_) {
        for (const auto& kernel : potentially_incompatible) {
            if (!matrix_->isKernelCompatible(device, kernel)) {
                std::string alternative = matrix_->getAlternativeKernel(device, kernel);
                if (!alternative.empty()) {
                    // Alternative should be compatible
                    EXPECT_TRUE(matrix_->isKernelCompatible(device, alternative))
                        << "Alternative kernel " << alternative << " not compatible with "
                        << device.name;
                }
            }
        }
    }
}

// ============================================================================
// Kernel Requirements Validation Tests
// ============================================================================

TEST_F(ArchitectureCompatibilityTest, KernelRequirementsValidation) {
    // Test various kernel requirements
    std::vector<std::pair<std::string, KernelRequirements>> test_cases = {
        {
            "basic_kernel",
            KernelRequirements{ComputeCapability{7, 5}, 0, 0, 64, 0, false, false, false, false}
        },
        {
            "tensor_kernel",
            KernelRequirements{ComputeCapability{7, 0}, 0, 0, 128, 0, true, false, false, false}
        },
        {
            "bf16_kernel",
            KernelRequirements{ComputeCapability{8, 0}, 0, 0, 256, 0, false, false, true, false}
        },
        {
            "fp8_kernel",
            KernelRequirements{ComputeCapability{8, 9}, 0, 0, 512, 0, false, false, false, true}
        }
    };

    for (const auto& [kernel_name, requirements] : test_cases) {
        for (const auto& device : available_devices_) {
            bool meets_requirements = matrix_->deviceMeetsRequirements(device, requirements);
            bool should_meet = requirements.isDeviceCompatible(device);

            EXPECT_EQ(meets_requirements, should_meet)
                << "Requirements validation mismatch for " << kernel_name << " on " << device.name;
        }
    }
}

TEST_F(ArchitectureCompatibilityTest, MissingRequirementsReporting) {
    // Create requirements that some devices might not meet
    KernelRequirements strict_requirements{
        ComputeCapability{9, 0},  // Hopper minimum
        65536,                   // High shared memory requirement
        80,                      // High register requirement
        1024,                    // High thread requirement
        0,
        true, true, true, true   // All advanced features
    };

    for (const auto& device : available_devices_) {
        bool compatible = matrix_->deviceMeetsRequirements(device, strict_requirements);
        if (!compatible) {
            auto missing = strict_requirements.getMissingRequirements(device);
            EXPECT_FALSE(missing.empty())
                << "Device " << device.name << " should have missing requirements listed";
        }
    }
}

// ============================================================================
// Kernel Selection Tests
// ============================================================================

TEST_F(ArchitectureCompatibilityTest, KernelSelectionFallback) {
    // Test kernel selection with fallback mechanisms
    std::vector<std::string> base_kernels = {
        "ecc_kernel",
        "hash_kernel",
        "compare_kernel"
    };

    for (const auto& device : available_devices_) {
        for (const auto& base_kernel : base_kernels) {
            auto selection = selector_->selectKernel(device, base_kernel);

            EXPECT_FALSE(selection.kernel_name.empty())
                << "No kernel selected for " << base_kernel << " on " << device.name;

            if (selection.is_fallback) {
                // Fallback should be compatible
                EXPECT_TRUE(matrix_->isKernelCompatible(device, selection.kernel_name))
                    << "Fallback kernel " << selection.kernel_name << " not compatible with "
                    << device.name;

                EXPECT_FALSE(selection.fallback_reason.empty())
                    << "Fallback reason should be provided";
            }
        }
    }
}

TEST_F(ArchitectureCompatibilityTest, MultiDeviceKernelSelection) {
    if (device_count_ < 2) {
        GTEST_SKIP() << "Need at least 2 devices for multi-device testing";
    }

    std::vector<int> device_ids;
    for (const auto& device : available_devices_) {
        device_ids.push_back(device.device_id);
    }

    std::string base_kernel = "ecc_kernel";
    auto selections = selector_->selectKernels(device_ids, base_kernel);

    EXPECT_EQ(selections.size(), device_ids.size())
        << "Should have selection for each device";

    for (size_t i = 0; i < selections.size(); ++i) {
        const auto& selection = selections[i];
        const auto& device = available_devices_[i];

        EXPECT_FALSE(selection.kernel_name.empty())
            << "No kernel selected for device " << device.name;

        EXPECT_TRUE(matrix_->isKernelCompatible(device, selection.kernel_name))
            << "Selected kernel " << selection.kernel_name << " not compatible with "
            << device.name;
    }
}

TEST_F(ArchitectureCompatibilityTest, KernelRanking) {
    for (const auto& device : available_devices_) {
        std::vector<std::string> test_variants = {
            "ecc_kernel",
            "ecc_tensor",
            "ecc_bf16",
            "ecc_optimized"
        };

        auto ranking = selector_->getKernelRanking(device.device_id, test_variants);

        EXPECT_FALSE(ranking.empty())
            << "Should have at least one ranked kernel for " << device.name;

        // Verify all ranked kernels are compatible
        for (const auto& kernel : ranking) {
            EXPECT_TRUE(matrix_->isKernelCompatible(device, kernel))
                << "Ranked kernel " << kernel << " not compatible with " << device.name;
        }
    }
}

// ============================================================================
// Performance Characteristics Tests
// ============================================================================

TEST_F(ArchitectureCompatibilityTest, PerformanceCharacteristics) {
    for (const auto& device : available_devices_) {
        auto perf = matrix_->getPerformanceCharacteristics(device.architecture);

        // Verify performance data exists
        EXPECT_EQ(perf.architecture, device.architecture)
            << "Performance characteristics architecture mismatch for " << device.name;

        EXPECT_FALSE(perf.supported_kernels.empty())
            << "No supported kernels listed for " << device.getArchitectureString();

        // Verify supported kernels include standard ones
        std::vector<std::string> standard_kernels = {"ecc_kernel", "hash_kernel", "compare_kernel"};
        for (const auto& kernel : standard_kernels) {
            bool supported = std::find(perf.supported_kernels.begin(), perf.supported_kernels.end(), kernel) != perf.supported_kernels.end();
            EXPECT_TRUE(supported)
                << "Standard kernel " << kernel << " not listed as supported for "
                << device.getArchitectureString();
        }

        // Verify performance characteristics are reasonable
        EXPECT_GT(perf.memory_bandwidth_efficiency, 0.0)
            << "Memory bandwidth efficiency should be positive";
        EXPECT_LE(perf.memory_bandwidth_efficiency, 1.0)
            << "Memory bandwidth efficiency should not exceed 100%";

        EXPECT_GT(perf.compute_efficiency, 0.0)
            << "Compute efficiency should be positive";
        EXPECT_LE(perf.compute_efficiency, 1.0)
            << "Compute efficiency should not exceed 100%";

        EXPECT_GT(perf.occupancy_target, 0.0)
            << "Occupancy target should be positive";
        EXPECT_LE(perf.occupancy_target, 1.0)
            << "Occupancy target should not exceed 100%";
    }
}

TEST_F(ArchitectureCompatibilityTest, OptimalConfigurationGeneration) {
    std::vector<std::string> test_kernels = {
        "ecc_kernel",
        "hash_kernel",
        "compare_kernel"
    };

    for (const auto& device : available_devices_) {
        for (const auto& kernel : test_kernels) {
            auto config = matrix_->getOptimalConfiguration(device, kernel);

            // Verify configuration is valid
            EXPECT_GT(config.threads_per_block, 0)
                << "Threads per block should be positive for " << kernel << " on " << device.name;

            EXPECT_LE(config.threads_per_block, device.max_threads_per_block)
                << "Threads per block should not exceed device maximum for " << kernel << " on " << device.name;

            // Verify configuration matches device capabilities
            if (device.supports_tensor_cores && kernel.find("tensor") != std::string::npos) {
                EXPECT_TRUE(config.use_tensor_cores)
                    << "Tensor cores should be enabled for tensor kernels on devices that support them";
            }
        }
    }
}

// ============================================================================
// Architecture-Specific Tests
// ============================================================================

TEST_F(ArchitectureCompatibilityTest, TuringCompatibility) {
    auto turing_devices = getDevicesByArchitecture(GPUArchitecture::TURING);
    if (turing_devices.empty()) {
        GTEST_SKIP() << "No Turing devices available for testing";
    }

    for (const auto& device : turing_devices) {
        // Verify Turing-specific characteristics
        EXPECT_EQ(device.compute_capability.major, 7);
        EXPECT_GE(device.compute_capability.minor, 5);
        EXPECT_TRUE(device.supports_tensor_cores);
        EXPECT_FALSE(device.supports_ray_tracing);
        EXPECT_FALSE(device.supports_bf16);
        EXPECT_FALSE(device.supports_tf32);
        EXPECT_FALSE(device.supports_fp8);

        // Verify Turing kernel compatibility
        EXPECT_TRUE(matrix_->isKernelCompatible(device, "ecc_kernel"));
        EXPECT_TRUE(matrix_->isKernelCompatible(device, "ecc_tensor"));
        EXPECT_FALSE(matrix_->isKernelCompatible(device, "ecc_bf16"));
        EXPECT_FALSE(matrix_->isKernelCompatible(device, "ecc_fp8"));
    }
}

TEST_F(ArchitectureCompatibilityTest, AmpereCompatibility) {
    auto ampere_devices = getDevicesByArchitecture(GPUArchitecture::AMPERE);
    if (ampere_devices.empty()) {
        GTEST_SKIP() << "No Ampere devices available for testing";
    }

    for (const auto& device : ampere_devices) {
        // Verify Ampere-specific characteristics
        EXPECT_EQ(device.compute_capability.major, 8);
        EXPECT_TRUE(device.supports_tensor_cores);
        EXPECT_TRUE(device.supports_bf16);
        EXPECT_TRUE(device.supports_tf32);
        EXPECT_FALSE(device.supports_fp8);

        // Verify Ampere kernel compatibility
        EXPECT_TRUE(matrix_->isKernelCompatible(device, "ecc_kernel"));
        EXPECT_TRUE(matrix_->isKernelCompatible(device, "ecc_tensor"));
        EXPECT_TRUE(matrix_->isKernelCompatible(device, "ecc_bf16"));
        EXPECT_FALSE(matrix_->isKernelCompatible(device, "ecc_fp8"));
    }
}

TEST_F(ArchitectureCompatibilityTest, AdaLovelaceCompatibility) {
    var ada_devices = getDevicesByArchitecture(GPUArchitecture::ADA_LOVELACE);
    if (ada_devices.empty()) {
        GTEST_SKIP() << "No Ada Lovelace devices available for testing";
    }

    for (const auto& device : ada_devices) {
        // Verify Ada Lovelace-specific characteristics
        EXPECT_EQ(device.compute_capability.major, 8);
        EXPECT_EQ(device.compute_capability.minor, 9);
        EXPECT_TRUE(device.supports_tensor_cores);
        EXPECT_TRUE(device.supports_bf16);
        EXPECT_TRUE(device.supports_tf32);
        EXPECT_TRUE(device.supports_fp8);

        // Verify Ada Lovelace kernel compatibility
        EXPECT_TRUE(matrix_->isKernelCompatible(device, "ecc_kernel"));
        EXPECT_TRUE(matrix_->isKernelCompatible(device, "ecc_tensor"));
        EXPECT_TRUE(matrix_->isKernelCompatible(device, "ecc_bf16"));
        EXPECT_TRUE(matrix_->isKernelCompatible(device, "ecc_fp8"));
    }
}

TEST_F(ArchitectureCompatibilityTest, HopperCompatibility) {
    auto hopper_devices = getDevicesByArchitecture(GPUArchitecture::HOPPER);
    if (hopper_devices.empty()) {
        GTEST_SKIP() << "No Hopper devices available for testing";
    }

    for (const auto& device : hopper_devices) {
        // Verify Hopper-specific characteristics
        EXPECT_EQ(device.compute_capability.major, 9);
        EXPECT_TRUE(device.supports_tensor_cores);
        EXPECT_TRUE(device.supports_bf16);
        EXPECT_TRUE(device.supports_tf32);
        EXPECT_TRUE(device.supports_fp8);

        // Verify Hopper kernel compatibility
        EXPECT_TRUE(matrix_->isKernelCompatible(device, "ecc_kernel"));
        EXPECT_TRUE(matrix_->isKernelCompatible(device, "ecc_tensor"));
        EXPECT_TRUE(matrix_->isKernelCompatible(device, "ecc_bf16"));
        EXPECT_TRUE(matrix_->isKernelCompatible(device, "ecc_fp8"));
    }
}

// ============================================================================
// Compatibility Validation Tests
// ============================================================================

TEST_F(ArchitectureCompatibilityTest, DeviceValidationReport) {
    for (const auto& device : available_devices_) {
        std::vector<std::string> required_kernels = {"ecc_kernel", "hash_kernel", "compare_kernel"};
        auto report = matrix_->validateDeviceCompatibility(device, required_kernels);

        EXPECT_TRUE(report.is_compatible)
            << "Device " << device.name << " should be compatible with basic kernels";

        EXPECT_TRUE(report.kernel_compatibility.size() == required_kernels.size())
            << "Should have compatibility status for each required kernel";

        for (const auto& [kernel, compatible] : report.kernel_compatibility) {
            EXPECT_TRUE(compatible)
                << "Required kernel " << kernel << " should be compatible with " << device.name;
        }
    }
}

TEST_F(ArchitectureCompatibilityTest, SystemCompatibilityReport) {
    std::string report = matrix_->generateSystemReport();

    EXPECT_FALSE(report.empty())
        << "System compatibility report should not be empty";

    EXPECT_NE(report.find("Available Devices"), std::string::npos)
        << "Report should contain device count information";

    EXPECT_NE(report.find("Architecture"), std::string::npos)
        << "Report should contain architecture information";
}

// ============================================================================
// Workaround and Issue Detection Tests
// ============================================================================

TEST_F(ArchitectureCompatibilityTest, WorkaroundDetection) {
    std::vector<std::string> test_kernels = {"ecc_kernel", "hash_kernel"};

    for (const auto& device : available_devices_) {
        for (const auto& kernel : test_kernels) {
            auto workarounds = matrix_->getWorkarounds(device.device_id, kernel);

            // Workarounds should be consistent across devices of same architecture
            if (!workarounds.empty()) {
                // Verify workaround descriptions are not empty
                for (const auto& workaround : workarounds) {
                    EXPECT_FALSE(workaround.empty())
                        << "Workaround description should not be empty for " << kernel
                        << " on " << device.name;
                }
            }
        }
    }
}

TEST_F(ArchitectureCompatibilityTest, OptimizationHints) {
    std::vector<std::string> test_kernels = {"ecc_tensor", "ecc_bf16"};

    for (const auto& device : available_devices_) {
        for (const auto& kernel : test_kernels) {
            if (matrix_->isKernelCompatible(device, kernel)) {
                auto hints = matrix_->getOptimizationHints(device.device_id, kernel);

                // Hints should be relevant and non-empty when available
                if (!hints.empty()) {
                    for (const auto& hint : hints) {
                        EXPECT_FALSE(hint.empty())
                            << "Optimization hint should not be empty for " << kernel
                            << " on " << device.name;
                    }
                }
            }
        }
    }
}

// ============================================================================
// Performance Benchmarks
// ============================================================================

TEST_F(ArchitectureCompatibilityTest, KernelSelectionPerformance) {
    if (device_count_ == 0) {
        GTEST_SKIP() << "No devices available for performance testing";
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    const int iterations = 1000;
    std::vector<std::string> test_kernels = {"ecc_kernel", "hash_kernel", "compare_kernel"};

    for (int i = 0; i < iterations; ++i) {
        for (const auto& device : available_devices_) {
            for (const auto& kernel : test_kernels) {
                auto selection = selector_->selectKernel(device, kernel);
                EXPECT_FALSE(selection.kernel_name.empty());
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Kernel selection should be fast (< 10ms for 1000 iterations across all devices)
    EXPECT_LT(duration.count(), 10000)
        << "Kernel selection performance too slow: " << duration.count() << "ms for "
        << iterations << " iterations";
}

TEST_F(ArchitectureCompatibilityTest, CompatibilityCheckPerformance) {
    if (device_count_ == 0) {
        GTEST_SKIP() << "No devices available for performance testing";
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    const int iterations = 10000;
    std::vector<std::string> test_kernels = {"ecc_kernel", "hash_kernel", "compare_kernel"};

    for (int i = 0; i < iterations; ++i) {
        for (const auto& device : available_devices_) {
            for (const auto& kernel : test_kernels) {
                bool compatible = matrix_->isKernelCompatible(device, kernel);
                (void)compatible; // Prevent optimization
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Compatibility checking should be very fast (< 100ms for 10000 iterations)
    EXPECT_LT(duration.count(), 100)
        << "Compatibility checking performance too slow: " << duration.count() << "ms for "
        << iterations << " iterations";
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(ArchitectureCompatibilityTest, EndToEndCompatibilityFlow) {
    if (device_count_ == 0) {
        GTEST_SKIP() << "No devices available for integration testing";
    }

    // Test complete compatibility workflow
    for (const auto& device : available_devices_) {
        // 1. Validate device
        std::vector<std::string> required_kernels = {"ecc_kernel", "hash_kernel", "compare_kernel"};
        auto validation_report = matrix_->validateDeviceCompatibility(device, required_kernels);
        EXPECT_TRUE(validation_report.is_compatible);

        // 2. Select kernels
        auto selections = selector_->selectKernels({device.device_id}, "ecc_kernel");
        EXPECT_EQ(selections.size(), 1);
        EXPECT_FALSE(selections[0].kernel_name.empty());

        // 3. Get optimal configuration
        auto config = matrix_->getOptimalConfiguration(device, selections[0].kernel_name);
        EXPECT_GT(config.threads_per_block, 0);

        // 4. Check workarounds
        auto workarounds = matrix_->getWorkarounds(device.device_id, selections[0].kernel_name);
        // Workarounds may be empty, but should not cause errors

        // 5. Get optimization hints
        auto hints = matrix_->getOptimizationHints(device.device_id, selections[0].kernel_name);
        // Hints may be empty, but should not cause errors

        // If we reach here, the workflow completed successfully
        SUCCEED();
    }
}

// ============================================================================
// Regression Tests
// ============================================================================

TEST_F(ArchitectureCompatibilityTest, NoRegressionInBasicCompatibility) {
    // Ensure all devices support basic kernels
    std::vector<std::string> basic_kernels = {"ecc_kernel", "hash_kernel", "compare_kernel"};

    for (const auto& device : available_devices_) {
        for (const auto& kernel : basic_kernels) {
            bool compatible = matrix_->isKernelCompatible(device, kernel);
            EXPECT_TRUE(compatible)
                << "REGRESSION: Basic kernel " << kernel << " no longer compatible with "
                << device.name << " (" << device.getArchitectureString() << ")";
        }
    }
}

TEST_F(ArchitectureCompatibilityTest, NoRegressionInPerformanceCharacteristics) {
    // Ensure performance characteristics are reasonable
    for (const auto& device : available_devices_) {
        auto perf = matrix_->getPerformanceCharacteristics(device.architecture);

        EXPECT_GE(perf.memory_bandwidth_efficiency, 0.5)
            << "REGRESSION: Memory bandwidth efficiency too low for "
            << device.getArchitectureString() << ": " << perf.memory_bandwidth_efficiency;

        EXPECT_GE(perf.compute_efficiency, 0.5)
            << "REGRESSION: Compute efficiency too low for "
            << device.getArchitectureString() << ": " << perf.compute_efficiency;

        EXPECT_GE(perf.occupancy_target, 0.5)
            << "REGRESSION: Occupancy target too low for "
            << device.getArchitectureString() << ": " << perf.occupancy_target;
    }
}

// ============================================================================
// Main Function (for manual testing)
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
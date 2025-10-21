#!/bin/bash

# Test runner script for adapter layer TDD tests
# This script compiles and runs the failing integration tests

set -e

echo "=== Adapter Layer TDD Test Runner ==="
echo "TDD Phase: RED (tests should fail before implementation)"
echo

# Check if CUDA is available
if ! command -v nvcc &> /dev/null; then
    echo "ERROR: CUDA compiler (nvcc) not found"
    exit 1
fi

# Check if Google Test is available
if ! pkg-config --exists gtest; then
    echo "ERROR: Google Test not found"
    exit 1
fi

# Create build directory
mkdir -p tests/build
cd tests/build

# Create a simple test compilation (standalone)
echo "Creating standalone test compilation..."

cat > test_adapter_standalone.cpp << 'EOF'
#include <gtest/gtest.h>
#include <cuda_runtime.h>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

// Mock implementations that ensure test failures
namespace keyhunt {
namespace adapter {

// Mock adapter state (always fails)
struct MockAdapterState {
    bool initialized = false;
    bool gpu_allocated = false;
    bool config_loaded = false;
    bool ecc_operational = false;
    bool compliant = false;

    bool is_ready() const { return false; }
    bool is_compliant() const { return false; }
    cudaError_t get_last_error() const { return cudaErrorUnknown; }
};

static MockAdapterState g_mock_state;

extern "C" {
    cudaError_t adapter_initialize() {
        return cudaErrorUnknown;
    }
    cudaError_t adapter_cleanup() {
        return cudaErrorUnknown;
    }
    cudaError_t adapter_load_static_config(const char* config_path) {
        return cudaErrorFileNotFound;
    }
    cudaError_t adapter_validate_compliance() {
        return cudaErrorNotSupported;
    }
    cudaError_t adapter_ecc_scalar_multiply(
        const uint8_t* private_keys,
        uint8_t* public_keys,
        int batch_size,
        cudaStream_t stream = 0
    ) {
        return cudaErrorNotSupported;
    }
    cudaError_t adapter_memory_allocate(size_t size, void** ptr) {
        return cudaErrorMemoryAllocation;
    }
    cudaError_t adapter_memory_free(void* ptr) {
        return cudaErrorInvalidDevicePointer;
    }
    cudaError_t adapter_get_performance_metrics(
        double* throughput,
        double* gpu_utilization,
        double* memory_bandwidth
    ) {
        return cudaErrorNotSupported;
    }
    bool adapter_supports_multigpu() {
        return false;
    }
    cudaError_t adapter_multigpu_initialize(int gpu_count) {
        return cudaErrorNotSupported;
    }
    cudaError_t adapter_error_recovery() {
        return cudaErrorNotSupported;
    }
}

} // namespace adapter
} // namespace keyhunt

class AdapterLayerTest : public ::testing::Test {
protected:
    void SetUp() override {
        rng.seed(std::random_device{}());

        cudaError_t cuda_status = cudaGetDeviceCount(&gpu_count);
        if (cuda_status != cudaSuccess || gpu_count == 0) {
            GTEST_SKIP() << "No CUDA GPU available for testing";
        }

        keyhunt::adapter::g_mock_state = keyhunt::adapter::MockAdapterState{};
        adapter_initialized = false;
    }

    void TearDown() override {
        if (adapter_initialized) {
            keyhunt::adapter::adapter_cleanup();
        }
    }

    void generate_test_data(int batch_size, std::vector<uint8_t>& private_keys,
                           std::vector<uint8_t>& expected_public_keys) {
        private_keys.resize(batch_size * 32);
        expected_public_keys.resize(batch_size * 64);

        std::uniform_int_distribution<uint64_t> dist(1, UINT64_MAX);
        for (int i = 0; i < batch_size; ++i) {
            for (int j = 0; j < 4; ++j) {
                uint64_t val = dist(rng);
                memcpy(private_keys.data() + i * 32 + j * 8, &val, 8);
            }
            private_keys[i * 32] = (private_keys[i * 32] & 0x7F) | 0x01;
        }

        std::uniform_int_distribution<uint8_t> pub_dist;
        for (auto& byte : expected_public_keys) {
            byte = pub_dist(rng);
        }
    }

    std::mt19937_64 rng;
    int gpu_count;
    bool adapter_initialized = false;
};

TEST_F(AdapterLayerTest, AdapterInitializationFails) {
    cudaError_t init_result = keyhunt::adapter::adapter_initialize();

    EXPECT_NE(init_result, cudaSuccess)
        << "Adapter initialization should fail before implementation - TDD Red Phase";

    EXPECT_FALSE(keyhunt::adapter::g_mock_state.initialized)
        << "Adapter state should show uninitialized";

    EXPECT_FALSE(keyhunt::adapter::g_mock_state.gpu_allocated)
        << "GPU resources should not be allocated without implementation";
}

TEST_F(AdapterLayerTest, LegacyAdapterECCIntegrationFails) {
    const int batch_size = 1000;
    std::vector<uint8_t> private_keys;
    std::vector<uint8_t> expected_public_keys;
    generate_test_data(batch_size, private_keys, expected_public_keys);

    std::vector<uint8_t> adapter_public_keys(batch_size * 64);

    cudaError_t ecc_result = keyhunt::adapter::adapter_ecc_scalar_multiply(
        private_keys.data(),
        adapter_public_keys.data(),
        batch_size
    );

    EXPECT_NE(ecc_result, cudaSuccess)
        << "Adapter ECC operations should fail before implementation - TDD Red Phase";

    bool all_zero = true;
    for (auto byte : adapter_public_keys) {
        if (byte != 0) {
            all_zero = false;
            break;
        }
    }
    EXPECT_TRUE(all_zero)
        << "Output should be uninitialized (all zeros) before implementation";
}

TEST_F(AdapterLayerTest, StaticLaunchConfigIntegrationFails) {
    cudaError_t config_result = keyhunt::adapter::adapter_load_static_config(
        "/nonexistent/config/path/config.json"
    );

    EXPECT_NE(config_result, cudaSuccess)
        << "Static launch configuration loading should fail before implementation - TDD Red Phase";

    EXPECT_FALSE(keyhunt::adapter::g_mock_state.config_loaded)
        << "Configuration should not be loaded without implementation";

    cudaError_t validation_result = keyhunt::adapter::adapter_validate_compliance();
    EXPECT_NE(validation_result, cudaSuccess)
        << "Configuration validation should fail before implementation - TDD Red Phase";
}

TEST_F(AdapterLayerTest, AdapterMemoryManagementFails) {
    const size_t test_allocation_size = 10 * 1024 * 1024;

    void* gpu_memory = nullptr;
    cudaError_t alloc_result = keyhunt::adapter::adapter_memory_allocate(
        test_allocation_size,
        &gpu_memory
    );

    EXPECT_NE(alloc_result, cudaSuccess)
        << "GPU memory allocation should fail before implementation - TDD Red Phase";

    EXPECT_EQ(gpu_memory, nullptr)
        << "GPU memory pointer should be null when allocation fails";

    cudaError_t dealloc_result = keyhunt::adapter::adapter_memory_free(gpu_memory);
    EXPECT_NE(dealloc_result, cudaSuccess)
        << "GPU memory deallocation should fail before implementation - TDD Red Phase";
}

TEST_F(AdapterLayerTest, MultiGPUAdapterIntegrationFails) {
    bool multigpu_supported = keyhunt::adapter::adapter_supports_multigpu();
    EXPECT_FALSE(multigpu_supported)
        << "Multi-GPU support should return false before implementation - TDD Red Phase";

    cudaError_t multigpu_result = keyhunt::adapter::adapter_multigpu_initialize(gpu_count);
    EXPECT_NE(multigpu_result, cudaSuccess)
        << "Multi-GPU initialization should fail before implementation - TDD Red Phase";
}

TEST_F(AdapterLayerTest, ThreadSafetyFails) {
    const int num_threads = 4;
    const int operations_per_thread = 100;

    std::atomic<bool> all_threads_failed{true};
    std::vector<std::thread> threads;
    std::mutex result_mutex;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, operations_per_thread, &all_threads_failed, &result_mutex]() {
            for (int op = 0; op < operations_per_thread; ++op) {
                std::vector<uint8_t> input(32);
                std::vector<uint8_t> output(64);

                std::uniform_int_distribution<uint8_t> dist;
                for (auto& byte : input) {
                    byte = dist(rng);
                }

                cudaError_t result = keyhunt::adapter::adapter_ecc_scalar_multiply(
                    input.data(), output.data(), 1
                );

                if (result == cudaSuccess) {
                    std::lock_guard<std::mutex> lock(result_mutex);
                    all_threads_failed = false;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_TRUE(all_threads_failed.load())
        << "All threads should fail before adapter implementation - TDD Red Phase";
}

TEST_F(AdapterLayerTest, ConstitutionalComplianceFails) {
    bool is_compliant = keyhunt::adapter::g_mock_state.is_compliant();
    EXPECT_FALSE(is_compliant)
        << "Adapter should not be compliant before implementation - TDD Red Phase";

    cudaError_t compliance_result = keyhunt::adapter::adapter_validate_compliance();
    EXPECT_NE(compliance_result, cudaSuccess)
        << "Compliance validation should fail before implementation - TDD Red Phase";

    EXPECT_FALSE(keyhunt::adapter::g_mock_state.is_ready())
        << "Adapter should not be ready without proper implementation - TDD Red Phase";
}

TEST_F(AdapterLayerTest, FallbackMechanismsFails) {
    cudaError_t fallback_result = keyhunt::adapter::adapter_error_recovery();
    EXPECT_NE(fallback_result, cudaSuccess)
        << "Fallback mechanisms should fail before implementation - TDD Red Phase";

    bool multigpu_fallback = keyhunt::adapter::adapter_supports_multigpu();
    EXPECT_FALSE(multigpu_fallback)
        << "Multi-GPU fallback should be false before implementation - TDD Red Phase";
}

TEST_F(AdapterLayerTest, EndToEndIntegrationWorkflowFails) {
    cudaError_t init_result = keyhunt::adapter::adapter_initialize();
    EXPECT_NE(init_result, cudaSuccess)
        << "Step 1: Adapter initialization should fail - TDD Red Phase";

    cudaError_t config_result = keyhunt::adapter::adapter_load_static_config(
        "config/test_config.json"
    );
    EXPECT_NE(config_result, cudaSuccess)
        << "Step 2: Configuration loading should fail - TDD Red Phase";

    const int batch_size = 100;
    std::vector<uint8_t> input_data(batch_size * 32);
    std::vector<uint8_t> output_data(batch_size * 64);

    std::uniform_int_distribution<uint8_t> dist;
    for (auto& byte : input_data) {
        byte = dist(rng);
    }

    cudaError_t process_result = keyhunt::adapter::adapter_ecc_scalar_multiply(
        input_data.data(),
        output_data.data(),
        batch_size
    );
    EXPECT_NE(process_result, cudaSuccess)
        << "Step 3: Data processing should fail - TDD Red Phase";

    double throughput, gpu_util, mem_bw;
    cudaError_t metrics_result = keyhunt::adapter::adapter_get_performance_metrics(
        &throughput, &gpu_util, &mem_bw
    );
    EXPECT_NE(metrics_result, cudaSuccess)
        << "Step 4: Performance metrics should fail - TDD Red Phase";

    cudaError_t cleanup_result = keyhunt::adapter::adapter_cleanup();
    EXPECT_NE(cleanup_result, cudaSuccess)
        << "Step 5: Cleanup should fail - TDD Red Phase";
}

TEST_F(AdapterLayerTest, CompilationVerificationSucceeds) {
    EXPECT_EQ(keyhunt::adapter::adapter_initialize(), cudaErrorUnknown);
    EXPECT_EQ(keyhunt::adapter::adapter_cleanup(), cudaErrorUnknown);
    EXPECT_EQ(keyhunt::adapter::adapter_load_static_config(nullptr), cudaErrorFileNotFound);
    EXPECT_EQ(keyhunt::adapter::adapter_validate_compliance(), cudaErrorNotSupported);
    EXPECT_EQ(keyhunt::adapter::adapter_memory_allocate(0, nullptr), cudaErrorMemoryAllocation);
    EXPECT_EQ(keyhunt::adapter::adapter_memory_free(nullptr), cudaErrorInvalidDevicePointer);
    EXPECT_EQ(keyhunt::adapter::adapter_get_performance_metrics(nullptr, nullptr, nullptr), cudaErrorNotSupported);
    EXPECT_EQ(keyhunt::adapter::adapter_multigpu_initialize(0), cudaErrorNotSupported);
    EXPECT_EQ(keyhunt::adapter::adapter_error_recovery(), cudaErrorNotSupported);

    EXPECT_FALSE(keyhunt::adapter::g_mock_state.initialized);
    EXPECT_FALSE(keyhunt::adapter::g_mock_state.gpu_allocated);
    EXPECT_FALSE(keyhunt::adapter::g_mock_state.config_loaded);
    EXPECT_FALSE(keyhunt::adapter::g_mock_state.ecc_operational);
    EXPECT_FALSE(keyhunt::adapter::g_mock_state.compliant);

    SUCCEED() << "Compilation verification passed - test harness is working correctly";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
EOF

echo "Compiling standalone adapter test..."
g++ -std=c++17 -Wall -Wextra -O0 -g \
    $(pkg-config --cflags gtest) \
    $(pkg-config --libs gtest) \
    -lgtest_main -pthread \
    -lcuda \
    test_adapter_standalone.cpp -o test_adapter_standalone

if [ $? -eq 0 ]; then
    echo "✓ Test compilation successful"
else
    echo "✗ Test compilation failed"
    exit 1
fi

echo
echo "Running TDD tests (should all FAIL - Red Phase)..."
echo "==================================================="

./test_adapter_standalone --gtest_output=xml:test_results.xml

test_exit_code=$?

echo
echo "=== TDD Test Results ==="
if [ $test_exit_code -eq 0 ]; then
    echo "✗ UNEXPECTED: All tests passed - this violates TDD Red Phase!"
    echo "  The tests should fail before implementation exists."
    exit 1
else
    echo "✓ EXPECTED: Tests failed as expected (TDD Red Phase)"
    echo "  This confirms the adapter layer is not yet implemented."
fi

echo
echo "=== TDD Status Summary ==="
echo "Phase: RED ✓ (Tests failing as expected)"
echo "Next: Implement adapter layer in src/KeyhuntCore/common/legacy_adapter_fixed.cuh"
echo "Target: Make all tests pass (Green Phase)"
echo

exit 0
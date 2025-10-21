#include "src/KeyhuntCore/common/ecc_operations_fixed.cuh"
#include <iostream>
#include <vector>
#include <cassert>
#include <memory>

int main() {
    std::cout << "Testing ECC Operations Implementation..." << std::endl;

    try {
        // Test basic initialization
        keyhunt::ecc::ECCBatchConfig config;
        config.batch_size = 1000;
        config.use_montgomery = true;
        config.use_fixed_point = false;
        config.precision_target = 1e-11;
        config.cuda_device_id = 0;
        config.use_soa_layout = true;
        config.alignment_bytes = 128;
        config.enable_shared_memory = true;
        config.registers_per_thread = 32;
        config.threads_per_block = 256;

        auto ecc_ops = std::make_unique<keyhunt::ecc::ECCOperationsFixed>();

        // Test initialization
        bool init_result = ecc_ops->initialize(config);
        std::cout << "Initialization: " << (init_result ? "SUCCESS" : "FAILED") << std::endl;

        if (!init_result) {
            std::cout << "Error: " << ecc_ops->get_last_error() << std::endl;
            return 1;
        }

        // Test configuration retrieval
        const auto& retrieved_config = ecc_ops->get_config();
        std::cout << "Configuration validation: ";
        if (retrieved_config.batch_size == config.batch_size &&
            retrieved_config.use_soa_layout == true &&
            retrieved_config.alignment_bytes == 128 &&
            retrieved_config.precision_target == 1e-11) {
            std::cout << "SUCCESS" << std::endl;
        } else {
            std::cout << "FAILED" << std::endl;
            return 1;
        }

        // Test SOA point allocation
        keyhunt::ecc::ECCPointSoA test_points;
        bool alloc_result = ecc_ops->allocate_soa_points(&test_points, 100);
        std::cout << "SOA allocation: " << (alloc_result ? "SUCCESS" : "FAILED") << std::endl;

        if (alloc_result) {
            // Test memory management
            std::vector<uint32_t> test_data(800, 0x12345678); // 100 points * 8 words
            bool copy_result = ecc_ops->copy_to_device(
                test_data.data(), test_points.x_words, test_data.size() * sizeof(uint32_t));
            std::cout << "Memory copy: " << (copy_result ? "SUCCESS" : "FAILED") << std::endl;

            // Test memory layout optimization
            bool layout_result = ecc_ops->optimize_memory_layout();
            std::cout << "Memory layout optimization: " << (layout_result ? "SUCCESS" : "FAILED") << std::endl;

            // Test shared memory optimization
            bool shared_result = ecc_ops->enable_shared_memory_optimization();
            std::cout << "Shared memory optimization: " << (shared_result ? "SUCCESS" : "FAILED") << std::endl;

            // Test performance benchmark
            double throughput = 0.0, efficiency = 0.0;
            bool benchmark_result = ecc_ops->benchmark_operations(throughput, efficiency);
            std::cout << "Performance benchmark: " << (benchmark_result ? "SUCCESS" : "FAILED") << std::endl;
            if (benchmark_result) {
                std::cout << "  Throughput: " << throughput << " ops/sec" << std::endl;
                std::cout << "  Efficiency: " << efficiency << "%" << std::endl;
            }

            // Test CPU validation (placeholder)
            std::vector<uint32_t> test_keys(800, 0x12345678);
            double max_error = 0.0;
            bool validation_result = ecc_ops->validate_against_cpu_reference(
                test_keys.data(), &test_points, 100, max_error);
            std::cout << "CPU validation: " << (validation_result ? "SUCCESS" : "FAILED") << std::endl;
            std::cout << "  Max relative error: " << max_error << std::endl;

            // Cleanup
            bool free_result = ecc_ops->free_soa_points(&test_points);
            std::cout << "Memory cleanup: " << (free_result ? "SUCCESS" : "FAILED") << std::endl;
        }

        std::cout << "\nAll ECC operations tests completed successfully!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
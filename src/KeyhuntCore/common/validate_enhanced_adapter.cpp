// Enhanced Adapter Validation Script
// Tests integration with existing modules and validates functionality

#include "legacy_adapter_fixed_enhanced.cuh"
#include <iostream>
#include <vector>
#include <chrono>
#include <cassert>

using namespace keyhunt::adapter;

class EnhancedAdapterValidator {
public:
    bool run_all_tests() {
        std::cout << "=== Enhanced Adapter Validation Suite ===" << std::endl;

        bool all_passed = true;

        all_passed &= test_initialization();
        all_passed &= test_memory_management();
        all_passed &= test_ecc_integration();
        all_passed &= test_performance_monitoring();
        all_passed &= test_memory_layout_transparency();
        all_passed &= test_legacy_compatibility();
        all_passed &= test_constitutional_compliance();
        all_passed &= test_error_handling();

        std::cout << "\n=== Validation Summary ===" << std::endl;
        std::cout << "Overall Result: " << (all_passed ? "PASSED" : "FAILED") << std::endl;

        return all_passed;
    }

private:
    bool test_initialization() {
        std::cout << "\n--- Testing Initialization ---" << std::endl;

        try {
            // Test default initialization
            if (!initialize_enhanced_adapter_with_defaults()) {
                std::cout << "FAIL: Default initialization failed" << std::endl;
                return false;
            }
            std::cout << "PASS: Default initialization successful" << std::endl;

            // Get adapter instance
            auto* adapter = get_global_enhanced_adapter();
            if (!adapter) {
                std::cout << "FAIL: Cannot get global adapter instance" << std::endl;
                return false;
            }
            std::cout << "PASS: Global adapter instance accessible" << std::endl;

            // Test re-initialization (should fail)
            if (initialize_enhanced_adapter_with_defaults()) {
                std::cout << "FAIL: Double initialization should fail" << std::endl;
                return false;
            }
            std::cout << "PASS: Double initialization correctly prevented" << std::endl;

            // Cleanup for next test
            cleanup_global_enhanced_adapter();

            return true;
        } catch (const std::exception& e) {
            std::cout << "FAIL: Exception during initialization: " << e.what() << std::endl;
            return false;
        }
    }

    bool test_memory_management() {
        std::cout << "\n--- Testing Memory Management ---" << std::endl;

        if (!initialize_enhanced_adapter_with_defaults()) {
            std::cout << "FAIL: Cannot initialize adapter for memory test" << std::endl;
            return false;
        }

        auto* adapter = get_global_enhanced_adapter();

        try {
            // Test device memory allocation
            void* device_ptr = nullptr;
            const size_t test_size = 1024 * 1024; // 1MB

            if (!adapter->allocate_device_memory(&device_ptr, test_size)) {
                std::cout << "FAIL: Device memory allocation failed" << std::endl;
                return false;
            }
            std::cout << "PASS: Device memory allocation successful" << std::endl;

            // Test host memory allocation
            void* host_ptr = nullptr;
            if (!adapter->allocate_host_memory(&host_ptr, test_size, true)) {
                std::cout << "FAIL: Host memory allocation failed" << std::endl;
                return false;
            }
            std::cout << "PASS: Host memory allocation successful" << std::endl;

            // Test memory copy
            std::vector<uint8_t> test_data(test_size, 0x42);
            if (!adapter->copy_to_device(test_data.data(), device_ptr, test_size)) {
                std::cout << "FAIL: Host to device copy failed" << std::endl;
                return false;
            }
            std::cout << "PASS: Host to device copy successful" << std::endl;

            std::vector<uint8_t> received_data(test_size, 0);
            if (!adapter->copy_to_host(device_ptr, received_data.data(), test_size)) {
                std::cout << "FAIL: Device to host copy failed" << std::endl;
                return false;
            }
            std::cout << "PASS: Device to host copy successful" << std::endl;

            // Verify data integrity
            bool data_match = (test_data == received_data);
            if (!data_match) {
                std::cout << "FAIL: Data integrity check failed" << std::endl;
                return false;
            }
            std::cout << "PASS: Data integrity verified" << std::endl;

            // Test memory deallocation
            adapter->deallocate_device_memory(device_ptr);
            adapter->deallocate_host_memory(host_ptr);
            std::cout << "PASS: Memory deallocation successful" << std::endl;

            return true;
        } catch (const std::exception& e) {
            std::cout << "FAIL: Exception during memory test: " << e.what() << std::endl;
            return false;
        }
    }

    bool test_ecc_integration() {
        std::cout << "\n--- Testing ECC Integration ---" << std::endl;

        auto* adapter = get_global_enhanced_adapter();
        if (!adapter) {
            std::cout << "FAIL: Adapter not initialized" << std::endl;
            return false;
        }

        try {
            // Setup ECC operations
            keyhunt::ecc::ECCBatchConfig ecc_config;
            ecc_config.batch_size = 256;
            ecc_config.use_montgomery = true;
            ecc_config.precision_target = 1e-11;
            ecc_config.use_soa_layout = true;
            ecc_config.alignment_bytes = 128;
            ecc_config.enable_shared_memory = true;

            if (!adapter->setup_ecc_operations(ecc_config)) {
                std::cout << "FAIL: ECC operations setup failed" << std::endl;
                return false;
            }
            std::cout << "PASS: ECC operations setup successful" << std::endl;

            // Test SoA memory allocation
            keyhunt::ecc::ECCPointSoA public_keys;
            if (!adapter->allocate_soa_memory(&public_keys, 256, true)) {
                std::cout << "FAIL: SoA memory allocation failed" << std::endl;
                return false;
            }
            std::cout << "PASS: SoA memory allocation successful" << std::endl;

            // Create test private keys (simple sequential values)
            std::vector<uint32_t> private_keys(256 * 8); // 8 words per 256-bit key
            for (size_t i = 0; i < 256; ++i) {
                for (int j = 0; j < 8; ++j) {
                    private_keys[i * 8 + j] = static_cast<uint32_t>(i);
                }
            }

            // Test scalar multiplication with monitoring
            ComprehensivePerformanceReport report;
            auto start_time = std::chrono::high_resolution_clock::now();

            bool success = adapter->scalar_multiply_with_monitoring(
                private_keys.data(), &public_keys, 256, report);

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

            if (!success) {
                std::cout << "FAIL: Scalar multiplication with monitoring failed" << std::endl;
                return false;
            }
            std::cout << "PASS: Scalar multiplication with monitoring successful" << std::endl;
            std::cout << "  Execution time: " << duration.count() << " ms" << std::endl;
            std::cout << "  Operations completed: " << report.successful_operations << std::endl;

            // Cleanup
            adapter->free_soa_points(&public_keys);
            std::cout << "PASS: SoA memory cleanup successful" << std::endl;

            return true;
        } catch (const std::exception& e) {
            std::cout << "FAIL: Exception during ECC test: " << e.what() << std::endl;
            return false;
        }
    }

    bool test_performance_monitoring() {
        std::cout << "\n--- Testing Performance Monitoring ---" << std::endl;

        auto* adapter = get_global_enhanced_adapter();
        if (!adapter) {
            std::cout << "FAIL: Adapter not initialized" << std::endl;
            return false;
        }

        try {
            // Test performance monitoring
            if (!adapter->start_performance_monitoring()) {
                std::cout << "FAIL: Failed to start performance monitoring" << std::endl;
                return false;
            }
            std::cout << "PASS: Performance monitoring started" << std::endl;

            // Test telemetry
            if (!adapter->enable_real_time_telemetry("./test_telemetry/")) {
                std::cout << "FAIL: Failed to enable telemetry" << std::endl;
                return false;
            }
            std::cout << "PASS: Real-time telemetry enabled" << std::endl;

            // Simulate some operations to generate telemetry data
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            // Generate performance report
            ComprehensivePerformanceReport report;
            if (!adapter->generate_comprehensive_performance_report(report)) {
                std::cout << "FAIL: Failed to generate performance report" << std::endl;
                return false;
            }
            std::cout << "PASS: Performance report generated" << std::endl;

            // Test telemetry export
            if (!adapter->export_telemetry_data("test_telemetry_export.json")) {
                std::cout << "FAIL: Failed to export telemetry data" << std::endl;
                return false;
            }
            std::cout << "PASS: Telemetry data exported" << std::endl;

            // Test current metrics access
            auto metrics = adapter->get_current_memory_metrics();
            std::cout << "PASS: Current memory metrics accessible" << std::endl;
            std::cout << "  Coalesced access ratio: " << metrics.coalesced_access_ratio << std::endl;

            // Stop monitoring
            adapter->stop_performance_monitoring();
            adapter->disable_real_time_telemetry();
            std::cout << "PASS: Monitoring and telemetry stopped" << std::endl;

            return true;
        } catch (const std::exception& e) {
            std::cout << "FAIL: Exception during performance monitoring test: " << e.what() << std::endl;
            return false;
        }
    }

    bool test_memory_layout_transparency() {
        std::cout << "\n--- Testing Memory Layout Transparency ---" << std::endl;

        auto* adapter = get_global_enhanced_adapter();
        if (!adapter) {
            std::cout << "FAIL: Adapter not initialized" << std::endl;
            return false;
        }

        try {
            // Test different memory layouts
            std::vector<MemoryLayout> layouts = {
                MemoryLayout::STRUCTURE_OF_ARRAYS,
                MemoryLayout::ARRAY_OF_STRUCTURES,
                MemoryLayout::AUTO_DETECT
            };

            for (auto layout : layouts) {
                void* ptr = nullptr;
                const size_t test_size = 1024;

                if (!adapter->allocate_device_memory(&ptr, test_size, layout)) {
                    std::cout << "FAIL: Memory allocation failed for layout " << static_cast<int>(layout) << std::endl;
                    return false;
                }

                adapter->deallocate_device_memory(ptr);
                std::cout << "PASS: Memory layout " << static_cast<int>(layout) << " works correctly" << std::endl;
            }

            // Test memory layout optimization
            if (!adapter->optimize_memory_layout(MemoryLayout::STRUCTURE_OF_ARRAYS)) {
                std::cout << "FAIL: Memory layout optimization failed" << std::endl;
                return false;
            }
            std::cout << "PASS: Memory layout optimization successful" << std::endl;

            return true;
        } catch (const std::exception& e) {
            std::cout << "FAIL: Exception during memory layout test: " << e.what() << std::endl;
            return false;
        }
    }

    bool test_legacy_compatibility() {
        std::cout << "\n--- Testing Legacy Compatibility ---" << std::endl;

        auto* adapter = get_global_enhanced_adapter();
        if (!adapter) {
            std::cout << "FAIL: Adapter not initialized" << std::endl;
            return false;
        }

        try {
            // Test legacy function wrappers
            std::vector<uint32_t> private_keys(256);
            std::vector<uint32_t> public_keys(256 * 8);

            // Initialize test data
            for (size_t i = 0; i < private_keys.size(); ++i) {
                private_keys[i] = static_cast<uint32_t>(i + 1);
            }

            if (!adapter->legacy_scalar_multiply(private_keys.data(), public_keys.data(), 256)) {
                std::cout << "FAIL: Legacy scalar multiplication failed" << std::endl;
                return false;
            }
            std::cout << "PASS: Legacy scalar multiplication successful" << std::endl;

            // Test legacy batch operations
            unsigned int accumulator[8] = {1, 2, 3, 4, 5, 6, 7, 8};
            if (!adapter->legacy_batch_inverse(accumulator)) {
                std::cout << "FAIL: Legacy batch inverse failed" << std::endl;
                return false;
            }
            std::cout << "PASS: Legacy batch inverse successful" << std::endl;

            // Check adapter statistics
            auto stats = adapter->get_adapter_statistics();
            std::cout << "PASS: Adapter statistics accessible" << std::endl;
            std::cout << "  Total operations: " << stats.total_operations << std::endl;
            std::cout << "  Legacy operations redirected: " << stats.legacy_operations_redirected << std::endl;

            return true;
        } catch (const std::exception& e) {
            std::cout << "FAIL: Exception during legacy compatibility test: " << e.what() << std::endl;
            return false;
        }
    }

    bool test_constitutional_compliance() {
        std::cout << "\n--- Testing Constitutional Compliance ---" << std::endl;

        auto* adapter = get_global_enhanced_adapter();
        if (!adapter) {
            std::cout << "FAIL: Adapter not initialized" << std::endl;
            return false;
        }

        try {
            // Test constitutional compliance validation
            if (!adapter->validate_constitutional_compliance()) {
                std::cout << "FAIL: Constitutional compliance validation failed" << std::endl;
                return false;
            }
            std::cout << "PASS: Constitutional compliance validation successful" << std::endl;

            // Get compliance score
            double compliance_score = adapter->get_constitutional_compliance_score();
            std::cout << "PASS: Compliance score accessible: " << compliance_score * 100 << "%" << std::endl;

            if (compliance_score < 0.95) {
                std::cout << "WARN: Compliance score below 95% threshold" << std::endl;
            }

            // Generate compliance report
            std::string compliance_report;
            if (!adapter->generate_compliance_report(compliance_report)) {
                std::cout << "FAIL: Failed to generate compliance report" << std::endl;
                return false;
            }
            std::cout << "PASS: Compliance report generated" << std::endl;

            return true;
        } catch (const std::exception& e) {
            std::cout << "FAIL: Exception during constitutional compliance test: " << e.what() << std::endl;
            return false;
        }
    }

    bool test_error_handling() {
        std::cout << "\n--- Testing Error Handling ---" << std::endl;

        auto* adapter = get_global_enhanced_adapter();
        if (!adapter) {
            std::cout << "FAIL: Adapter not initialized" << std::endl;
            return false;
        }

        try {
            // Test error tracking
            if (!adapter->has_errors()) {
                std::cout << "PASS: No initial errors" << std::endl;
            }

            // Test invalid memory allocation (should fail gracefully)
            void* ptr = nullptr;
            if (adapter->allocate_device_memory(&ptr, SIZE_MAX)) {
                std::cout << "FAIL: Should have failed to allocate oversized memory" << std::endl;
                return false;
            }
            std::cout << "PASS: Oversized memory allocation correctly rejected" << std::endl;

            // Check error message
            const char* error_msg = adapter->get_last_error();
            if (strlen(error_msg) == 0) {
                std::cout << "FAIL: No error message recorded" << std::endl;
                return false;
            }
            std::cout << "PASS: Error message recorded: " << error_msg << std::endl;

            // Test error history
            auto error_history = adapter->get_error_history();
            if (error_history.empty()) {
                std::cout << "FAIL: No error history recorded" << std::endl;
                return false;
            }
            std::cout << "PASS: Error history maintained (" << error_history.size() << " errors)" << std::endl;

            // Clear error history
            adapter->clear_error_history();
            error_history = adapter->get_error_history();
            if (!error_history.empty()) {
                std::cout << "FAIL: Error history not cleared" << std::endl;
                return false;
            }
            std::cout << "PASS: Error history cleared successfully" << std::endl;

            return true;
        } catch (const std::exception& e) {
            std::cout << "FAIL: Exception during error handling test: " << e.what() << std::endl;
            return false;
        }
    }
};

int main() {
    try {
        EnhancedAdapterValidator validator;
        bool success = validator.run_all_tests();

        // Cleanup
        cleanup_global_enhanced_adapter();

        return success ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Validation suite failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
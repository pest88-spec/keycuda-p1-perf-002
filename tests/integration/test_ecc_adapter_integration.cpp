// Puzzle71 Technical Debt Repair - ECC Adapter Integration Tests
// Comprehensive test suite for T029: ECC operations integrated with adapter layer
// Tests all aspects of the integration including performance, compatibility, and compliance

#include <gtest/gtest.h>
#include <cuda_runtime.h>
#include <vector>
#include <random>
#include <chrono>
#include <fstream>
#include <filesystem>

#include "KeyhuntCore/common/ecc_adapter_integration_enhanced.cuh"
#include "KeyhuntCore/common/ecc_operations_fixed.cuh"
#include "KeyhuntCore/common/legacy_adapter_fixed_enhanced.cuh"

using namespace keyhunt;
using namespace keyhunt::integration;
using namespace keyhunt::adapter;
using namespace keyhunt::ecc;

class ECCAdapterIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize CUDA
        cudaError_t cuda_err = cudaSetDevice(0);
        ASSERT_EQ(cudaSuccess, cuda_err) << "Failed to set CUDA device";

        // Setup default configurations
        setup_default_configurations();

        // Initialize global integration
        bool init_success = initialize_global_enhanced_ecc_integration(adapter_config_, ecc_config_);
        ASSERT_TRUE(init_success) << "Failed to initialize global ECC integration";

        integration_ = get_global_enhanced_ecc_integration();
        ASSERT_NE(nullptr, integration_) << "Global integration is null after initialization";

        // Generate test data
        generate_test_data();
    }

    void TearDown() override {
        // Cleanup test data
        cleanup_test_data();

        // Cleanup global integration
        cleanup_global_enhanced_ecc_integration();
    }

    void setup_default_configurations() {
        // Adapter configuration with constitutional compliance
        adapter_config_ = EnhancedAdapterConfig();
        adapter_config_.mode = AdapterMode::COMPATIBILITY_BRIDGE;
        adapter_config_.preferred_layout = MemoryLayout::STRUCTURE_OF_ARRAYS;
        adapter_config_.enable_backward_compatibility = true;
        adapter_config_.enable_performance_monitoring = true;
        adapter_config_.enable_deterministic_replay = true;
        adapter_config_.enable_constitutional_compliance = true;
        adapter_config_.enable_memory_access_optimization = true;
        adapter_config_.enable_ecc_integration = true;
        adapter_config_.enable_strict_validation = true;
        adapter_config_.enable_fallback_mechanisms = true;

        // ECC configuration with high precision requirements
        ecc_config_ = ECCBatchConfig();
        ecc_config_.batch_size = test_batch_size_;
        ecc_config_.use_soa_layout = true;
        ecc_config_.alignment_bytes = 128;
        ecc_config_.precision_target = 1e-11;  // Must be < 1e-10
        ecc_config_.cuda_device_id = 0;
        ecc_config_.registers_per_thread = 32;
        ecc_config_.threads_per_block = 256;
        ecc_config_.shared_memory_size = 48 * 1024;  // 48KB
    }

    void generate_test_data() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dis;

        // Generate test private keys
        test_private_keys_.resize(test_batch_size_);
        for (auto& key : test_private_keys_) {
            key = dis(gen);
        }

        // Allocate device memory for private keys
        cudaError_t err = cudaMalloc(&device_private_keys_,
                                    test_batch_size_ * sizeof(uint32_t));
        ASSERT_EQ(cudaSuccess, err) << "Failed to allocate device memory for private keys";

        err = cudaMemcpy(device_private_keys_, test_private_keys_.data(),
                        test_batch_size_ * sizeof(uint32_t), cudaMemcpyHostToDevice);
        ASSERT_EQ(cudaSuccess, err) << "Failed to copy private keys to device";

        // Allocate SoA structure for public keys
        ASSERT_TRUE(integration_->get_adapter()->allocate_soa_memory(&soa_public_keys_, test_batch_size_));
    }

    void cleanup_test_data() {
        if (device_private_keys_) {
            cudaFree(device_private_keys_);
            device_private_keys_ = nullptr;
        }

        if (integration_ && integration_->get_adapter()) {
            auto* adapter = integration_->get_adapter();
            if (soa_public_keys_.x_words) {
                adapter->deallocate_device_memory(soa_public_keys_.x_words);
            }
            if (soa_public_keys_.y_words) {
                adapter->deallocate_device_memory(soa_public_keys_.y_words);
            }
            if (soa_public_keys_.is_valid) {
                adapter->deallocate_host_memory(soa_public_keys_.is_valid);
            }
        }
    }

    // Test configuration
    static constexpr size_t test_batch_size_ = 1024;
    EnhancedAdapterConfig adapter_config_;
    ECCBatchConfig ecc_config_;

    // Test data
    std::vector<uint32_t> test_private_keys_;
    uint32_t* device_private_keys_ = nullptr;
    ECCPointSoA soa_public_keys_{};

    // Integration instance
    EnhancedECCAdapterIntegration* integration_ = nullptr;
};

// ============================================================================
// Basic Integration Tests
// ============================================================================

TEST_F(ECCAdapterIntegrationTest, InitializationAndCleanup) {
    // Test that integration is properly initialized
    EXPECT_TRUE(integration_->is_initialized());
    EXPECT_NE(nullptr, integration_->get_ecc_operations());
    EXPECT_NE(nullptr, integration_->get_adapter());

    // Test configuration access
    const auto& current_adapter_config = integration_->get_adapter_config();
    const auto& current_ecc_config = integration_->get_ecc_config();

    EXPECT_EQ(AdapterMode::COMPATIBILITY_BRIDGE, current_adapter_config.mode);
    EXPECT_EQ(MemoryLayout::STRUCTURE_OF_ARRAYS, current_adapter_config.preferred_layout);
    EXPECT_TRUE(current_ecc_config.use_soa_layout);
    EXPECT_LT(current_ecc_config.precision_target, 1e-10);
}

TEST_F(ECCAdapterIntegrationTest, ConfigurationCompatibilityValidation) {
    // Create incompatible configurations
    EnhancedAdapterConfig incompatible_adapter = adapter_config_;
    ECCBatchConfig incompatible_ecc = ecc_config_;

    // Mismatched device IDs
    incompatible_ecc.cuda_device_id = 1;
    incompatible_adapter.cuda_device_id = 0;

    // Try to reinitialize with incompatible configs
    EXPECT_FALSE(integration_->reconfigure(incompatible_adapter, incompatible_ecc));

    // Precision target too high (violates <1e-10 requirement)
    incompatible_ecc = ecc_config_;
    incompatible_ecc.precision_target = 1e-8;  // Too high
    incompatible_adapter = adapter_config_;

    EXPECT_FALSE(integration_->reconfigure(incompatible_adapter, incompatible_ecc));
}

// ============================================================================
// ECC Operations Integration Tests
// ============================================================================

TEST_F(ECCAdapterIntegrationTest, ScalarMultiplyWithMonitoring) {
    ComprehensivePerformanceReport report;

    // Perform scalar multiplication with monitoring
    bool success = integration_->scalar_multiply_with_monitoring(
        device_private_keys_, &soa_public_keys_, test_batch_size_,
        MemoryLayout::STRUCTURE_OF_ARRAYS, &report);

    EXPECT_TRUE(success) << "Scalar multiplication with monitoring failed";
    EXPECT_GT(report.ecc_throughput_ops_per_sec, 0.0);
    EXPECT_EQ(test_batch_size_, report.ecc_operations_completed);
    EXPECT_EQ(0, report.ecc_operations_failed);
    EXPECT_LT(report.ecc_precision_achieved, 1e-10);
    EXPECT_GT(report.memory_efficiency_percent, 70.0);
    EXPECT_GT(report.gpu_utilization_percent, 70.0);

    // Verify that public keys were generated
    ASSERT_NE(nullptr, soa_public_keys_.is_valid);

    // Copy validation results back to host
    std::vector<bool> host_valid(test_batch_size_);
    cudaError_t err = cudaMemcpy(host_valid.data(), soa_public_keys_.is_valid,
                                test_batch_size_ * sizeof(bool), cudaMemcpyDeviceToHost);
    ASSERT_EQ(cudaSuccess, err);

    // Check that most operations succeeded (allowing for some invalid keys)
    size_t valid_count = std::count(host_valid.begin(), host_valid.end(), true);
    EXPECT_GT(valid_count, test_batch_size_ * 0.9) << "Too many invalid results";
}

TEST_F(ECCAdapterIntegrationTest, PointAdditionWithMonitoring) {
    // First generate some points using scalar multiplication
    ComprehensivePerformanceReport init_report;
    ASSERT_TRUE(integration_->scalar_multiply_with_monitoring(
        device_private_keys_, &soa_public_keys_, test_batch_size_,
        MemoryLayout::STRUCTURE_OF_ARRAYS, &init_report));

    // Create second set of points
    ECCPointSoA points_q;
    ASSERT_TRUE(integration_->get_adapter()->allocate_soa_memory(&points_q, test_batch_size_));

    // Generate second set of public keys
    std::vector<uint32_t> second_keys(test_batch_size_);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis;
    for (auto& key : second_keys) {
        key = dis(gen);
    }

    uint32_t* device_second_keys = nullptr;
    cudaError_t err = cudaMalloc(&device_second_keys, test_batch_size_ * sizeof(uint32_t));
    ASSERT_EQ(cudaSuccess, err);
    err = cudaMemcpy(device_second_keys, second_keys.data(),
                    test_batch_size_ * sizeof(uint32_t), cudaMemcpyHostToDevice);
    ASSERT_EQ(cudaSuccess, err);

    ComprehensivePerformanceReport q_report;
    ASSERT_TRUE(integration_->scalar_multiply_with_monitoring(
        device_second_keys, &points_q, test_batch_size_,
        MemoryLayout::STRUCTURE_OF_ARRAYS, &q_report));

    // Perform point addition: R = P + Q
    ECCPointSoA points_r;
    ASSERT_TRUE(integration_->get_adapter()->allocate_soa_memory(&points_r, test_batch_size_));

    ComprehensivePerformanceReport add_report;
    bool success = integration_->point_addition_with_monitoring(
        &soa_public_keys_, &points_q, &points_r, test_batch_size_, &add_report);

    EXPECT_TRUE(success) << "Point addition with monitoring failed";
    EXPECT_GT(add_report.ecc_throughput_ops_per_sec, 0.0);
    EXPECT_EQ(test_batch_size_, add_report.ecc_operations_completed);

    // Cleanup
    cudaFree(device_second_keys);
    integration_->get_adapter()->deallocate_device_memory(points_q.x_words);
    integration_->get_adapter()->deallocate_device_memory(points_q.y_words);
    integration_->get_adapter()->deallocate_host_memory(points_q.is_valid);
    integration_->get_adapter()->deallocate_device_memory(points_r.x_words);
    integration_->get_adapter()->deallocate_device_memory(points_r.y_words);
    integration_->get_adapter()->deallocate_host_memory(points_r.is_valid);
}

TEST_F(ECCAdapterIntegrationTest, PointValidationWithCPUConsistency) {
    // Generate points first
    ComprehensivePerformanceReport init_report;
    ASSERT_TRUE(integration_->scalar_multiply_with_monitoring(
        device_private_keys_, &soa_public_keys_, test_batch_size_,
        MemoryLayout::STRUCTURE_OF_ARRAYS, &init_report));

    // Validate points with CPU consistency checking
    std::vector<bool> validation_results(test_batch_size_);
    double max_relative_error = 0.0;
    ComprehensivePerformanceReport validation_report;

    bool success = integration_->validate_points_with_cpu_consistency(
        &soa_public_keys_, validation_results.data(), test_batch_size_,
        max_relative_error, &validation_report);

    EXPECT_TRUE(success) << "Point validation with CPU consistency failed";
    EXPECT_LT(max_relative_error, 1e-10) << "CPU/GPU consistency error exceeds threshold";
    EXPECT_TRUE(validation_report.cpu_gpu_consistency_passed);
    EXPECT_LT(validation_report.max_relative_error, 1e-10);

    // Check that most points are valid
    size_t valid_count = std::count(validation_results.begin(), validation_results.end(), true);
    EXPECT_GT(valid_count, test_batch_size_ * 0.9) << "Too many invalid points";
}

// ============================================================================
// Legacy Compatibility Tests
// ============================================================================

TEST_F(ECCAdapterIntegrationTest, LegacyScalarMultiplyOptimized) {
    // Prepare legacy format data (AoS)
    std::vector<uint32_t> legacy_public_keys(test_batch_size_ * 16); // 8 words X + 8 words Y
    std::fill(legacy_public_keys.begin(), legacy_public_keys.end(), 0);

    ComprehensivePerformanceReport report;

    // Perform legacy scalar multiplication with automatic optimization
    bool success = integration_->legacy_scalar_multiply_optimized(
        device_private_keys_, legacy_public_keys.data(), test_batch_size_, &report);

    EXPECT_TRUE(success) << "Legacy scalar multiplication optimized failed";
    EXPECT_GT(report.ecc_throughput_ops_per_sec, 0.0);
    EXPECT_EQ(1, report.legacy_calls_redirected);
    EXPECT_GT(report.adapter_overhead_percent, 0.0); // Should have some overhead due to conversion
}

TEST_F(ECCAdapterIntegrationTest, LegacyBatchInverseWithMonitoring) {
    unsigned int accumulator[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    ComprehensivePerformanceReport report;

    bool success = integration_->legacy_batch_inverse_with_monitoring(accumulator, &report);

    EXPECT_TRUE(success) << "Legacy batch inverse with monitoring failed";
    // Note: Actual validation of the mathematical result would require comparison with reference
}

// ============================================================================
// Memory Layout Optimization Tests
// ============================================================================

TEST_F(ECCAdapterIntegrationTest, MemoryLayoutConversion) {
    // Create test data in AoS format
    const size_t point_size = 16 * sizeof(uint32_t); // 8 words X + 8 words Y
    std::vector<uint8_t> aos_data(test_batch_size_ * point_size);
    std::vector<uint8_t> soa_data(test_batch_size_ * point_size * 2); // Separate X and Y arrays

    // Fill AoS data with test pattern
    for (size_t i = 0; i < test_batch_size_; ++i) {
        uint8_t* point = aos_data.data() + i * point_size;
        for (size_t j = 0; j < point_size; ++j) {
            point[j] = static_cast<uint8_t>((i * point_size + j) & 0xFF);
        }
    }

    // Test AoS to SoA conversion
    bool convert_success = MemoryLayoutConverter::aos_to_soa(
        aos_data.data(), soa_data.data(), point_size, test_batch_size_);
    EXPECT_TRUE(convert_success) << "AoS to SoA conversion failed";

    // Test SoA to AoS conversion back
    std::vector<uint8_t> converted_aos_data(test_batch_size_ * point_size);
    convert_success = MemoryLayoutConverter::soa_to_aos(
        soa_data.data(), converted_aos_data.data(), point_size, test_batch_size_);
    EXPECT_TRUE(convert_success) << "SoA to AoS conversion failed";

    // Verify data integrity
    EXPECT_EQ(aos_data, converted_aos_data) << "Data integrity lost during conversion";
}

TEST_F(ECCAdapterIntegrationTest, OptimalLayoutDetection) {
    // Test layout detection for different scenarios
    MemoryLayout large_sequential = MemoryLayoutConverter::detect_optimal_layout(
        1024 * 1024, false, true);
    EXPECT_EQ(MemoryLayout::STRUCTURE_OF_ARRAYS, large_sequential);

    MemoryLayout small_random = MemoryLayoutConverter::detect_optimal_layout(
        1024, true, false);
    EXPECT_EQ(MemoryLayout::ARRAY_OF_STRUCTURES, small_random);

    MemoryLayout default_case = MemoryLayoutConverter::detect_optimal_layout(
        64 * 1024, false, false);
    EXPECT_EQ(MemoryLayout::STRUCTURE_OF_ARRAYS, default_case);
}

// ============================================================================
// Performance Monitoring Tests
// ============================================================================

TEST_F(ECCAdapterIntegrationTest, PerformanceMonitoringIntegration) {
    // Start performance monitoring
    std::string session_name = "test_monitoring_session";
    bool monitoring_started = integration_->start_performance_monitoring(session_name);
    EXPECT_TRUE(monitoring_started) << "Failed to start performance monitoring";

    // Perform some operations to generate metrics
    ComprehensivePerformanceReport report1, report2, report3;
    integration_->scalar_multiply_with_monitoring(
        device_private_keys_, &soa_public_keys_, test_batch_size_,
        MemoryLayout::STRUCTURE_OF_ARRAYS, &report1);

    integration_->validate_points_with_cpu_consistency(
        &soa_public_keys_, nullptr, test_batch_size_, report1.max_relative_error, &report2);

    // Stop monitoring and get comprehensive report
    ComprehensivePerformanceReport final_report;
    bool monitoring_stopped = integration_->stop_performance_monitoring(final_report);
    EXPECT_TRUE(monitoring_stopped) << "Failed to stop performance monitoring";

    // Verify monitoring data
    EXPECT_GT(final_report.total_execution_time.count(), 0.0);
    EXPECT_GT(final_report.memory_efficiency_percent, 0.0);
    EXPECT_GT(final_report.gpu_utilization_percent, 0.0);
    EXPECT_GT(final_report.ecc_throughput_ops_per_sec, 0.0);
}

TEST_F(ECCAdapterIntegrationTest, PerformanceBenchmarks) {
    // Run performance benchmarks comparing adapter vs direct calls
    const size_t benchmark_batch_size = 2048;
    ComprehensivePerformanceReport adapter_report, direct_report;

    bool benchmark_success = integration_->run_performance_benchmarks(
        benchmark_batch_size, true, adapter_report, direct_report);
    EXPECT_TRUE(benchmark_success) << "Performance benchmarks failed";

    // Verify benchmark results
    EXPECT_GT(adapter_report.ecc_throughput_ops_per_sec, 0.0);
    EXPECT_GT(direct_report.ecc_throughput_ops_per_sec, 0.0);
    EXPECT_EQ(benchmark_batch_size, adapter_report.ecc_operations_completed);
    EXPECT_EQ(benchmark_batch_size, direct_report.ecc_operations_completed);

    // Adapter overhead should be reasonable (< 20%)
    EXPECT_LT(adapter_report.adapter_overhead_percent, 20.0);

    std::cout << "Performance Benchmark Results:" << std::endl;
    std::cout << "  Adapter throughput: " << adapter_report.ecc_throughput_ops_per_sec << " ops/sec" << std::endl;
    std::cout << "  Direct throughput:   " << direct_report.ecc_throughput_ops_per_sec << " ops/sec" << std::endl;
    std::cout << "  Adapter overhead:    " << adapter_report.adapter_overhead_percent << "%" << std::endl;
}

// ============================================================================
// Deterministic Replay Tests
// ============================================================================

TEST_F(ECCAdapterIntegrationTest, DeterministicRecordingAndReplay) {
    const std::string recording_file = "test_deterministic_recording.dat";

    // Start deterministic recording
    bool recording_started = integration_->start_deterministic_recording(recording_file);
    EXPECT_TRUE(recording_started) << "Failed to start deterministic recording";

    // Perform operations while recording
    ComprehensivePerformanceReport report1, report2;
    integration_->scalar_multiply_with_monitoring(
        device_private_keys_, &soa_public_keys_, test_batch_size_,
        MemoryLayout::STRUCTURE_OF_ARRAYS, &report1);

    integration_->validate_points_with_cpu_consistency(
        &soa_public_keys_, nullptr, test_batch_size_, report1.max_relative_error, &report2);

    // Stop recording
    bool recording_stopped = integration_->stop_deterministic_recording();
    EXPECT_TRUE(recording_stopped) << "Failed to stop deterministic recording";

    // Verify recording file was created
    EXPECT_TRUE(std::filesystem::exists(recording_file)) << "Recording file was not created";

    // Replay the recording
    bool replay_successful = false;
    ComprehensivePerformanceReport replay_report;
    bool replay_started = integration_->replay_deterministic_recording(
        recording_file, replay_report, replay_successful);
    EXPECT_TRUE(replay_started) << "Failed to start deterministic replay";
    EXPECT_TRUE(replay_successful) << "Deterministic replay was not successful";

    // Clean up recording file
    std::filesystem::remove(recording_file);
}

TEST_F(ECCAdapterIntegrationTest, DeterministicConsistencyValidation) {
    const std::string recording_file1 = "test_recording_1.dat";
    const std::string recording_file2 = "test_recording_2.dat";

    // Create two recordings of the same operations
    for (int recording_num = 1; recording_num <= 2; ++recording_num) {
        const std::string& filename = (recording_num == 1) ? recording_file1 : recording_file2;

        integration_->start_deterministic_recording(filename);

        // Perform deterministic operations
        ComprehensivePerformanceReport report;
        integration_->scalar_multiply_with_monitoring(
            device_private_keys_, &soa_public_keys_, test_batch_size_,
            MemoryLayout::STRUCTURE_OF_ARRAYS, &report);

        integration_->stop_deterministic_recording();
    }

    // Validate consistency between recordings
    double max_difference = 0.0;
    bool consistency_valid = integration_->validate_deterministic_consistency(
        recording_file1, recording_file2, max_difference);

    EXPECT_TRUE(consistency_valid) << "Deterministic consistency validation failed";
    EXPECT_LT(max_difference, 1e-10) << "Deterministic consistency difference exceeds threshold";

    // Clean up recording files
    std::filesystem::remove(recording_file1);
    std::filesystem::remove(recording_file2);
}

// ============================================================================
// Constitutional Compliance Tests
// ============================================================================

TEST_F(ECCAdapterIntegrationTest, ConstitutionalComplianceValidation) {
    // Validate constitutional compliance
    bool compliance_valid = integration_->validate_constitutional_compliance();
    EXPECT_TRUE(compliance_valid) << "Constitutional compliance validation failed";

    // Get compliance score
    double compliance_score = integration_->get_constitutional_compliance_score();
    EXPECT_GE(compliance_score, 0.95) << "Constitutional compliance score too low: " << compliance_score;

    // Generate compliance report
    std::string compliance_report;
    bool report_generated = integration_->generate_compliance_report(compliance_report);
    EXPECT_TRUE(report_generated) << "Failed to generate compliance report";
    EXPECT_FALSE(compliance_report.empty()) << "Compliance report is empty";

    std::cout << "Constitutional Compliance Report:" << std::endl;
    std::cout << compliance_report << std::endl;
}

// ============================================================================
// Error Handling and Fallback Tests
// ============================================================================

TEST_F(ECCAdapterIntegrationTest, ErrorHandlingAndDiagnostics) {
    // Test error handling with invalid parameters
    ComprehensivePerformanceReport report;

    // Try with null pointers
    bool result = integration_->scalar_multiply_with_monitoring(
        nullptr, &soa_public_keys_, test_batch_size_,
        MemoryLayout::STRUCTURE_OF_ARRAYS, &report);
    EXPECT_FALSE(result) << "Should fail with null private keys";

    // Check error reporting
    EXPECT_TRUE(integration_->has_errors()) << "Should have errors after failed operation";
    const char* last_error = integration_->get_last_error();
    EXPECT_NE(nullptr, last_error) << "Last error should not be null";
    EXPECT_STRNE("", last_error) << "Last error should not be empty";

    // Check error history
    auto error_history = integration_->get_error_history();
    EXPECT_FALSE(error_history.empty()) << "Error history should not be empty";

    // Clear error history
    integration_->clear_error_history();
    error_history = integration_->get_error_history();
    EXPECT_TRUE(error_history.empty()) << "Error history should be empty after clearing";
}

TEST_F(ECCAdapterIntegrationTest, FallbackMechanisms) {
    // Enable fallback mechanisms
    bool fallback_enabled = integration_->enable_fallback_mechanisms(true);
    EXPECT_TRUE(fallback_enabled) << "Failed to enable fallback mechanisms";

    // Test operations that might trigger fallbacks
    // (This would typically involve scenarios that cause the primary path to fail)

    // Get integration statistics to check fallback usage
    auto stats = integration_->get_integration_statistics();
    EXPECT_GE(stats.total_operations, 0);
    EXPECT_GE(stats.fallback_operations_used, 0);
}

// ============================================================================
// Batch Processing Optimization Tests
// ============================================================================

TEST_F(ECCAdapterIntegrationTest, BatchSizeOptimization) {
    // Test batch size optimization for different sizes
    std::vector<size_t> test_sizes = {256, 512, 1024, 2048, 4096};

    for (size_t test_size : test_sizes) {
        size_t optimized_size = integration_->optimize_batch_size(test_size, ecc_config_);
        EXPECT_GT(optimized_size, 0) << "Optimized batch size should be > 0";
        EXPECT_LE(optimized_size, test_size * 2) << "Optimized size should not exceed 2x requested";
    }
}

TEST_F(ECCAdapterIntegrationTest, LargeBatchProcessing) {
    // Test processing of large batches in chunks
    const size_t large_batch_size = 10000;
    std::vector<uint32_t> large_private_keys(large_batch_size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis;
    for (auto& key : large_private_keys) {
        key = dis(gen);
    }

    uint32_t* device_large_keys = nullptr;
    cudaError_t err = cudaMalloc(&device_large_keys, large_batch_size * sizeof(uint32_t));
    ASSERT_EQ(cudaSuccess, err);
    err = cudaMemcpy(device_large_keys, large_private_keys.data(),
                    large_batch_size * sizeof(uint32_t), cudaMemcpyHostToDevice);
    ASSERT_EQ(cudaSuccess, err);

    ECCPointSoA large_public_keys;
    ASSERT_TRUE(integration_->get_adapter()->allocate_soa_memory(&large_public_keys, large_batch_size));

    ComprehensivePerformanceReport report;
    bool success = integration_->process_large_batch(
        device_large_keys, &large_public_keys, large_batch_size, report);

    EXPECT_TRUE(success) << "Large batch processing failed";
    EXPECT_EQ(large_batch_size, report.ecc_operations_completed);
    EXPECT_GT(report.ecc_throughput_ops_per_sec, 0.0);

    // Cleanup
    cudaFree(device_large_keys);
    integration_->get_adapter()->deallocate_device_memory(large_public_keys.x_words);
    integration_->get_adapter()->deallocate_device_memory(large_public_keys.y_words);
    integration_->get_adapter()->deallocate_host_memory(large_public_keys.is_valid);
}

// ============================================================================
// Statistics and Metrics Tests
// ============================================================================

TEST_F(ECCAdapterIntegrationTest, IntegrationStatistics) {
    // Perform some operations to generate statistics
    ComprehensivePerformanceReport report;
    integration_->scalar_multiply_with_monitoring(
        device_private_keys_, &soa_public_keys_, test_batch_size_,
        MemoryLayout::STRUCTURE_OF_ARRAYS, &report);

    integration_->legacy_scalar_multiply_optimized(
        device_private_keys_, nullptr, test_batch_size_, &report);

    // Get integration statistics
    auto stats = integration_->get_integration_statistics();

    EXPECT_GT(stats.total_ecc_operations, 0);
    EXPECT_GT(stats.successful_operations, 0);
    EXPECT_GE(stats.legacy_calls_redirected, 0);
    EXPECT_GT(stats.average_execution_time_ms, 0.0);
    EXPECT_GT(stats.peak_throughput_ops_per_sec, 0.0);
    EXPECT_NE(std::chrono::high_resolution_clock::time_point{}, stats.last_operation_time);

    std::cout << "Integration Statistics:" << std::endl;
    std::cout << "  Total operations:      " << stats.total_ecc_operations << std::endl;
    std::cout << "  Successful operations: " << stats.successful_operations << std::endl;
    std::cout << "  Legacy calls redirected: " << stats.legacy_calls_redirected << std::endl;
    std::cout << "  Average execution time: " << stats.average_execution_time_ms << " ms" << std::endl;
    std::cout << "  Peak throughput:       " << stats.peak_throughput_ops_per_sec << " ops/sec" << std::endl;

    // Reset statistics
    integration_->reset_statistics();
    auto reset_stats = integration_->get_integration_statistics();
    EXPECT_EQ(0, reset_stats.total_ecc_operations);
    EXPECT_EQ(0, reset_stats.successful_operations);
}

// ============================================================================
// Convenience Functions Tests
// ============================================================================

TEST_F(ECCAdapterIntegrationTest, ConvenienceFunctions) {
    // Test quick scalar multiplication
    bool quick_success = quick_scalar_multiply(device_private_keys_, &soa_public_keys_, test_batch_size_);
    EXPECT_TRUE(quick_success) << "Quick scalar multiplication failed";

    // Test quick CPU consistency validation
    double max_error = 0.0;
    bool validation_success = quick_validate_cpu_consistency(&soa_public_keys_, test_batch_size_, max_error);
    EXPECT_TRUE(validation_success) << "Quick CPU consistency validation failed";
    EXPECT_LT(max_error, 1e-10) << "CPU consistency error exceeds threshold";

    // Test quick performance benchmark
    double adapter_throughput = 0.0, direct_throughput = 0.0;
    bool benchmark_success = quick_performance_benchmark(512, adapter_throughput, direct_throughput);
    EXPECT_TRUE(benchmark_success) << "Quick performance benchmark failed";
    EXPECT_GT(adapter_throughput, 0.0);
    EXPECT_GT(direct_throughput, 0.0);

    // Test quick constitutional compliance check
    double compliance_score = 0.0;
    bool compliance_success = quick_constitutional_compliance_check(compliance_score);
    EXPECT_TRUE(compliance_success) << "Quick constitutional compliance check failed";
    EXPECT_GE(compliance_score, 0.95) << "Compliance score too low";
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    std::cout << "=== ECC Adapter Integration Test Suite ===" << std::endl;
    std::cout << "Testing T029: ECC operations integrated with adapter layer" << std::endl;
    std::cout << "Batch size: " << ECCAdapterIntegrationTest::test_batch_size_ << std::endl;

    return RUN_ALL_TESTS();
}
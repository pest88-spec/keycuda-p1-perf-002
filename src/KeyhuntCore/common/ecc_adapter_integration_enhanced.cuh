// Puzzle71 Technical Debt Repair - Enhanced ECC/Adapter Integration Layer
// Addresses T029: Comprehensive integration of ECC operations with adapter layer
// Provides seamless bridging between legacy code and optimized ECC operations
// with full performance monitoring, deterministic replay, and constitutional compliance

#pragma once

#include "ecc_operations_fixed.cuh"
#include "legacy_adapter_fixed_enhanced.cuh"
#include "../monitoring/real_time_monitor.cuh"
#include "../benchmarks/baseline_manager.hpp"
#include "../config/static_launch_config.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <string>
#include <array>
#include <fstream>
#include <sstream>

namespace keyhunt {
namespace integration {

/**
 * @brief Memory layout conversion utilities for transparent SoA/AoS support
 */
struct MemoryLayoutConverter {
    // Convert Array-of-Structures to Structure-of-Arrays
    static bool aos_to_soa(const void* aos_data, void* soa_data,
                          size_t struct_size, size_t count,
                          size_t alignment = 128);

    // Convert Structure-of-Arrays to Array-of-Structures
    static bool soa_to_aos(const void* soa_data, void* aos_data,
                          size_t struct_size, size_t count,
                          size_t alignment = 128);

    // Detect optimal layout based on access patterns
    static MemoryLayout detect_optimal_layout(size_t data_size,
                                             bool random_access,
                                             bool sequential_access);
};

/**
 * @brief Enhanced ECC operations with adapter integration
 *
 * This class provides the complete integration between ECC operations
 * and the adapter layer, offering performance monitoring, deterministic
 * replay, memory layout optimization, and constitutional compliance.
 */
class EnhancedECCAdapterIntegration {
public:
    // Constructor and destructor
    EnhancedECCAdapterIntegration();
    ~EnhancedECCAdapterIntegration();

    // Initialization and configuration
    bool initialize(const adapter::EnhancedAdapterConfig& adapter_config,
                   const ecc::ECCBatchConfig& ecc_config);
    void cleanup();
    bool reconfigure(const adapter::EnhancedAdapterConfig& new_adapter_config,
                    const ecc::ECCBatchConfig& new_ecc_config);

    // Core ECC operations with adapter integration and monitoring

    /**
     * @brief Scalar multiplication with comprehensive monitoring
     *
     * Performs batch scalar multiplication P = k * G with full adapter integration,
     * performance monitoring, memory layout optimization, and error handling.
     */
    bool scalar_multiply_with_monitoring(
        const uint32_t* private_keys,           // Input: private keys (AoS or SoA)
        ecc::ECCPointSoA* public_keys,          // Output: public keys (SoA optimized)
        size_t batch_size,
        MemoryLayout input_layout = MemoryLayout::AUTO_DETECT,
        adapter::ComprehensivePerformanceReport* report = nullptr
    );

    /**
     * @brief Point addition with monitoring and layout optimization
     */
    bool point_addition_with_monitoring(
        const ecc::ECCPointSoA* points_p,       // Input: points P
        const ecc::ECCPointSoA* points_q,       // Input: points Q
        ecc::ECCPointSoA* points_r,             // Output: points R
        size_t batch_size,
        adapter::ComprehensivePerformanceReport* report = nullptr
    );

    /**
     * @brief Point doubling with monitoring
     */
    bool point_doubling_with_monitoring(
        const ecc::ECCPointSoA* points_p,       // Input: points P
        ecc::ECCPointSoA* points_r,             // Output: points R
        size_t batch_size,
        adapter::ComprehensivePerformanceReport* report = nullptr
    );

    /**
     * @brief Batch validation with CPU/GPU consistency checking
     */
    bool validate_points_with_cpu_consistency(
        const ecc::ECCPointSoA* points,         // Input: points to validate
        bool* validation_results,               // Output: validation flags
        size_t batch_size,
        double& max_relative_error,             // Output: maximum relative error
        adapter::ComprehensivePerformanceReport* report = nullptr
    );

    // Legacy compatibility interface with automatic redirection

    /**
     * @brief Legacy scalar multiplication with automatic optimization
     *
     * Provides backward compatibility while automatically using optimized
     * ECC operations under the hood with performance monitoring.
     */
    bool legacy_scalar_multiply_optimized(
        const uint32_t* private_keys,           // Legacy AoS format
        uint32_t* public_keys,                  // Legacy AoS format
        size_t count,
        adapter::ComprehensivePerformanceReport* report = nullptr
    );

    /**
     * @brief Legacy batch inverse with monitoring
     */
    bool legacy_batch_inverse_with_monitoring(
        unsigned int accumulator[8],
        adapter::ComprehensivePerformanceReport* report = nullptr
    );

    /**
     * @brief Legacy batch point add with monitoring
     */
    bool legacy_batch_point_add_with_monitoring(
        const unsigned int incX[8],
        const unsigned int incY[8],
        unsigned int* xPtr,
        unsigned int* yPtr,
        unsigned int* chain,
        int srcIdx,
        int dstIdx,
        unsigned int accumulator[8],
        unsigned int resultX[8],
        unsigned int resultY[8],
        adapter::ComprehensivePerformanceReport* report = nullptr
    );

    // Memory layout optimization and management

    /**
     * @brief Allocate optimized memory with automatic layout detection
     */
    bool allocate_optimized_memory(
        void** device_ptr,
        size_t size,
        MemoryLayout preferred_layout = MemoryLayout::AUTO_DETECT,
        bool enable_monitoring = true
    );

    /**
     * @brief Convert memory layout transparently
     */
    bool convert_memory_layout(
        void* data,
        MemoryLayout from_layout,
        MemoryLayout to_layout,
        size_t struct_size,
        size_t count
    );

    /**
     * @brief Optimize memory access patterns for ECC operations
     */
    bool optimize_memory_access_patterns(
        const ecc::ECCPointSoA* points,
        size_t batch_size,
        adapter::MemoryAccessMetrics& metrics
    );

    // Performance monitoring and benchmarking

    /**
     * @brief Start comprehensive performance monitoring
     */
    bool start_performance_monitoring(const std::string& session_name = "ecc_integration");

    /**
     * @brief Stop performance monitoring and generate report
     */
    bool stop_performance_monitoring(adapter::ComprehensivePerformanceReport& report);

    /**
     * @brief Run performance benchmarks comparing adapter vs direct calls
     */
    bool run_performance_benchmarks(
        size_t batch_size,
        bool compare_with_direct = true,
        adapter::ComprehensivePerformanceReport& adapter_report = adapter::ComprehensivePerformanceReport(),
        adapter::ComprehensivePerformanceReport& direct_report = adapter::ComprehensivePerformanceReport()
    );

    /**
     * @brief Export performance telemetry data
     */
    bool export_telemetry_data(const std::string& filename);

    // Deterministic replay support

    /**
     * @brief Start deterministic recording for reproducibility
     */
    bool start_deterministic_recording(const std::string& recording_file);

    /**
     * @brief Stop deterministic recording
     */
    bool stop_deterministic_recording();

    /**
     * @brief Replay deterministic recording for validation
     */
    bool replay_deterministic_recording(
        const std::string& recording_file,
        adapter::ComprehensivePerformanceReport& report,
        bool& replay_successful
    );

    /**
     * @brief Validate deterministic replay consistency
     */
    bool validate_deterministic_consistency(
        const std::string& recording_file1,
        const std::string& recording_file2,
        double& max_difference
    );

    // CPU/GPU consistency validation

    /**
     * @brief Validate ECC operations against CPU reference
     */
    bool validate_against_cpu_reference(
        const uint32_t* private_keys,
        const ecc::ECCPointSoA* gpu_public_keys,
        size_t batch_size,
        double& max_relative_error,
        size_t& failed_validations
    );

    /**
     * @brief Run comprehensive CPU/GPU consistency tests
     */
    bool run_cpu_gpu_consistency_tests(
        size_t test_count = 10000,
        double& max_error,
        double& average_error,
        size_t& failed_tests
    );

    // Constitutional compliance validation

    /**
     * @brief Validate constitutional compliance through adapter
     */
    bool validate_constitutional_compliance();

    /**
     * @brief Generate constitutional compliance report
     */
    bool generate_compliance_report(std::string& report);

    /**
     * @brief Get constitutional compliance score (0.0 to 1.0)
     */
    double get_constitutional_compliance_score() const;

    // Error handling and fallback mechanisms

    /**
     * @brief Enable fallback mechanisms for ECC operations
     */
    bool enable_fallback_mechanisms(bool enable = true);

    /**
     * @brief Get last error with detailed diagnostics
     */
    const char* get_last_error() const;

    /**
     * @brief Check if any errors have occurred
     */
    bool has_errors() const;

    /**
     * @brief Get error history for debugging
     */
    std::vector<std::string> get_error_history() const;

    /**
     * @brief Clear error history
     */
    void clear_error_history();

    // Batch processing optimization

    /**
     * @brief Optimize batch size for current GPU architecture
     */
    size_t optimize_batch_size(size_t requested_size,
                              const ecc::ECCBatchConfig& config);

    /**
     * @brief Process large batches in chunks with monitoring
     */
    bool process_large_batch(
        const uint32_t* private_keys,
        ecc::ECCPointSoA* public_keys,
        size_t total_batch_size,
        adapter::ComprehensivePerformanceReport& report
    );

    // Configuration and state access

    /**
     * @brief Check if integration is initialized
     */
    bool is_initialized() const { return initialized_; }

    /**
     * @brief Get current adapter configuration
     */
    const adapter::EnhancedAdapterConfig& get_adapter_config() const { return adapter_config_; }

    /**
     * @brief Get current ECC configuration
     */
    const ecc::ECCBatchConfig& get_ecc_config() const { return ecc_config_; }

    /**
     * @brief Get current performance metrics
     */
    adapter::MemoryAccessMetrics get_current_memory_metrics() const;

    /**
     * @brief Get current throughput metrics
     */
    double get_current_throughput() const;

    /**
     * @brief Get integration statistics
     */
    struct IntegrationStatistics {
        size_t total_ecc_operations;
        size_t successful_operations;
        size_t failed_operations;
        size_t legacy_calls_redirected;
        size_t optimized_calls_direct;
        size_t fallback_operations_used;
        double average_execution_time_ms;
        double peak_throughput_ops_per_sec;
        double average_adapter_overhead_percent;
        std::chrono::high_resolution_clock::time_point last_operation_time;
    };

    IntegrationStatistics get_integration_statistics() const;
    void reset_statistics();

private:
    // Core configuration and state
    adapter::EnhancedAdapterConfig adapter_config_;
    ecc::ECCBatchConfig ecc_config_;
    bool initialized_;
    mutable std::mutex error_mutex_;
    mutable std::mutex statistics_mutex_;
    char last_error_[1024];
    std::vector<std::string> error_history_;

    // Core components
    std::unique_ptr<ecc::ECCOperationsFixed> ecc_ops_;
    std::unique_ptr<adapter::LegacyAdapterFixedEnhanced> adapter_;
    std::unique_ptr<puzzle71::monitoring::RealTimeMonitor> monitor_;
    std::unique_ptr<keyhunt::benchmarks::BaselineManager> baseline_manager_;

    // Performance tracking
    std::atomic<size_t> total_ecc_operations_;
    std::atomic<size_t> successful_operations_;
    std::atomic<size_t> failed_operations_;
    std::atomic<size_t> legacy_calls_redirected_;
    std::atomic<size_t> optimized_calls_direct_;
    std::atomic<size_t> fallback_operations_used_;
    std::atomic<double> total_execution_time_;
    std::atomic<double> peak_throughput_;
    std::atomic<double> adapter_overhead_total_;

    // Deterministic replay
    std::atomic<bool> deterministic_recording_active_;
    std::atomic<bool> deterministic_replay_active_;
    std::string deterministic_recording_file_;
    std::string deterministic_replay_file_;
    std::ofstream deterministic_recording_stream_;
    std::ifstream deterministic_replay_stream_;
    std::mutex deterministic_mutex_;

    // Performance monitoring
    std::atomic<bool> monitoring_active_;
    std::string current_monitoring_session_;
    std::chrono::high_resolution_clock::time_point session_start_time_;
    std::vector<adapter::ComprehensivePerformanceReport> performance_history_;

    // Memory layout optimization
    MemoryLayout current_layout_;
    bool layout_optimization_enabled_;
    adapter::MemoryAccessMetrics current_memory_metrics_;

    // Fallback mechanisms
    bool fallback_enabled_;
    bool use_legacy_fallback_;
    bool use_cpu_fallback_;

    // Constitutional compliance
    bool constitutional_compliance_enabled_;
    double constitutional_compliance_score_;
    std::chrono::high_resolution_clock::time_point last_compliance_check_;

    // Internal helper methods

    /**
     * @brief Setup core components for integration
     */
    bool setup_core_components();

    /**
     * @brief Validate configuration compatibility
     */
    bool validate_configuration_compatibility();

    /**
     * @brief Setup performance monitoring integration
     */
    bool setup_monitoring_integration();

    /**
     * @brief Setup deterministic replay system
     */
    bool setup_deterministic_replay_system();

    /**
     * @brief Record operation for deterministic replay
     */
    bool record_operation(const std::string& operation_name,
                         const void* input_data, size_t input_size,
                         const void* output_data, size_t output_size,
                         double execution_time_ms);

    /**
     * @brief Replay operation from recording
     */
    bool replay_operation(const std::string& operation_name,
                         void* input_data, size_t input_size,
                         void* output_data, size_t output_size,
                         double& execution_time_ms);

    /**
     * @brief Update error state
     */
    void update_error(const char* error, cudaError_t cuda_err = cudaSuccess);

    /**
     * @brief Update operation statistics
     */
    void update_operation_stats(bool success, double execution_time_ms,
                               bool legacy_call = false, bool fallback_used = false);

    /**
     * @brief Measure adapter overhead
     */
    double measure_adapter_overhead(std::function<void()> operation);

    /**
     * @brief Optimize memory layout for specific operation
     */
    bool optimize_layout_for_operation(const std::string& operation_name,
                                       void* data, size_t size);

    /**
     * @brief Validate memory access patterns
     */
    bool validate_memory_access_patterns(const void* data, size_t size,
                                        adapter::MemoryAccessMetrics& metrics);

    /**
     * @brief Check constitutional compliance requirements
     */
    bool check_constitutional_requirements();

    /**
     * @brief Update constitutional compliance score
     */
    void update_constitutional_compliance_score(bool requirement_met,
                                               double weight = 1.0);

    /**
     * @brief Generate performance report from current metrics
     */
    void generate_performance_report(adapter::ComprehensivePerformanceReport& report);

    /**
     * @brief Export telemetry data to file
     */
    bool export_telemetry_to_file(const std::string& filename,
                                 const std::vector<adapter::ComprehensivePerformanceReport>& reports);

    /**
     * @brief Import telemetry data from file
     */
    bool import_telemetry_from_file(const std::string& filename,
                                   std::vector<adapter::ComprehensivePerformanceReport>& reports);
};

/**
 * @brief Global enhanced ECC integration instance
 */
extern std::unique_ptr<EnhancedECCAdapterIntegration> g_enhanced_ecc_integration;

/**
 * @brief Initialize global enhanced ECC integration
 */
bool initialize_global_enhanced_ecc_integration(
    const adapter::EnhancedAdapterConfig& adapter_config = adapter::EnhancedAdapterConfig(),
    const ecc::ECCBatchConfig& ecc_config = ecc::ECCBatchConfig()
);

/**
 * @brief Cleanup global enhanced ECC integration
 */
void cleanup_global_enhanced_ecc_integration();

/**
 * @brief Get global enhanced ECC integration instance
 */
EnhancedECCAdapterIntegration* get_global_enhanced_ecc_integration();

/**
 * @brief Convenience functions for common operations
 */

// Quick scalar multiplication with monitoring
bool quick_scalar_multiply(const uint32_t* private_keys,
                          ecc::ECCPointSoA* public_keys,
                          size_t batch_size);

// Quick validation with CPU consistency check
bool quick_validate_cpu_consistency(const ecc::ECCPointSoA* points,
                                   size_t batch_size,
                                   double& max_error);

// Quick performance benchmark
bool quick_performance_benchmark(size_t batch_size,
                                double& adapter_throughput,
                                double& direct_throughput);

// Quick constitutional compliance check
bool quick_constitutional_compliance_check(double& compliance_score);

} // namespace integration

// Namespace aliases for backward compatibility
namespace ecc_integration = keyhunt::integration;
namespace enhanced_ecc = keyhunt::integration;

} // namespace keyhunt
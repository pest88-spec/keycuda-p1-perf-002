// Puzzle71 Technical Debt Repair - Enhanced Fixed Legacy Adapter Module
// Addresses comprehensive adapter pattern requirements with constitutional compliance
// Implements complete adapter pattern to eliminate code duplication between legacy and new implementations
// Provides backward compatibility while using optimized implementations with full monitoring support

#pragma once

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

// Core dependencies
#include "ecc_operations_fixed.cuh"
#include "ecc_adapter_integration.cuh"
#include "static_launch_config.h"
#include "../monitoring/real_time_monitor.cuh"
#include "../benchmarks/baseline_manager.hpp"

namespace keyhunt {
namespace adapter {

/**
 * @brief Memory layout enumeration for transparent SoA/AoS support
 */
enum class MemoryLayout {
    STRUCTURE_OF_ARRAYS,   // Optimized for GPU coalescing
    ARRAY_OF_STRUCTURES,   // Legacy compatibility
    HYBRID,               // Adaptive based on usage pattern
    AUTO_DETECT           // Choose optimal layout automatically
};

/**
 * @brief Adapter operation mode for different compatibility levels
 */
enum class AdapterMode {
    LEGACY_ONLY,          // Use only legacy implementations
    OPTIMIZED_ONLY,       // Use only new optimized implementations
    COMPATIBILITY_BRIDGE, // Bridge between legacy and optimized
    AUTO_SELECT,          // Automatically select best implementation
    DUAL_IMPLEMENTATION   // Run both for validation/comparison
};

/**
 * @brief Comprehensive adapter configuration with constitutional compliance
 */
struct EnhancedAdapterConfig {
    // Basic configuration
    AdapterMode mode;
    MemoryLayout preferred_layout;
    bool enable_backward_compatibility;
    bool enable_deprecation_warnings;

    // Performance optimization settings
    bool enable_memory_pooling;
    size_t pool_size_bytes;
    size_t alignment_bytes;
    bool enable_zero_copy;
    bool enable_unified_memory;
    bool enable_pinned_memory;

    // Error handling and validation
    bool enable_strict_validation;
    bool enable_detailed_logging;
    int max_retry_attempts;
    bool enable_fallback_mechanisms;

    // Constitutional compliance (v5.5 requirements)
    bool enforce_static_configuration;
    bool disable_runtime_device_queries;
    bool enable_deterministic_behavior;
    bool enable_constitutional_compliance;

    // Performance monitoring and telemetry
    bool enable_performance_monitoring;
    bool enable_real_time_telemetry;
    bool enable_deterministic_replay;
    std::string telemetry_output_path;
    std::chrono::milliseconds telemetry_interval;

    // Memory access optimization
    bool enable_memory_access_optimization;
    bool enable_coalesced_access_detection;
    bool enable_bank_conflict_elimination;
    bool enable_shared_memory_optimization;

    // Integration settings
    bool enable_ecc_integration;
    bool enable_hash_integration;
    bool enable_compare_integration;
    bool enable_unified_candidate_scanning;

    // Logging and monitoring
    bool enable_comprehensive_logging;
    bool enable_performance_logging;
    bool enable_error_logging;
    bool enable_debug_logging;

    // Default constructor with constitutional defaults
    EnhancedAdapterConfig()
        : mode(AdapterMode::COMPATIBILITY_BRIDGE)
        , preferred_layout(MemoryLayout::STRUCTURE_OF_ARRAYS)
        , enable_backward_compatibility(true)
        , enable_deprecation_warnings(true)
        , enable_memory_pooling(true)
        , pool_size_bytes(128 * 1024 * 1024)  // 128MB default
        , alignment_bytes(128)                // 128-byte alignment
        , enable_zero_copy(false)
        , enable_unified_memory(true)
        , enable_pinned_memory(true)
        , enable_strict_validation(true)
        , enable_detailed_logging(false)
        , max_retry_attempts(3)
        , enable_fallback_mechanisms(true)
        , enforce_static_configuration(true)
        , disable_runtime_device_queries(true)
        , enable_deterministic_behavior(true)
        , enable_constitutional_compliance(true)
        , enable_performance_monitoring(true)
        , enable_real_time_telemetry(true)
        , enable_deterministic_replay(true)
        , telemetry_output_path("telemetry/")
        , telemetry_interval(std::chrono::milliseconds(100))
        , enable_memory_access_optimization(true)
        , enable_coalesced_access_detection(true)
        , enable_bank_conflict_elimination(true)
        , enable_shared_memory_optimization(true)
        , enable_ecc_integration(true)
        , enable_hash_integration(true)
        , enable_compare_integration(true)
        , enable_unified_candidate_scanning(true)
        , enable_comprehensive_logging(true)
        , enable_performance_logging(true)
        , enable_error_logging(true)
        , enable_debug_logging(false)
    {}
};

/**
 * @brief Memory access pattern optimization metrics
 */
struct MemoryAccessMetrics {
    double coalesced_access_ratio;      // Target: >95%
    double bank_conflict_ratio;         // Target: <5%
    double shared_memory_efficiency;    // Target: >90%
    double memory_bandwidth_utilization; // Target: >70%
    size_t total_memory_transactions;
    size_t coalesced_transactions;
    size_t uncoalesced_transactions;
    std::chrono::duration<double, std::milli> access_time_ms;
};

/**
 * @brief Comprehensive performance report with telemetry data
 */
struct ComprehensivePerformanceReport {
    // Basic performance metrics
    double memory_efficiency_percent;
    double gpu_utilization_percent;
    double kernel_throughput_ops_per_sec;
    size_t total_memory_allocated_mb;
    size_t peak_memory_usage_mb;

    // Success/failure tracking
    int successful_kernel_launches;
    int failed_kernel_launches;
    int successful_operations;
    int failed_operations;

    // Memory access optimization metrics
    MemoryAccessMetrics memory_metrics;

    // ECC operations metrics
    double ecc_throughput_ops_per_sec;
    double ecc_precision_achieved;
    size_t ecc_operations_completed;
    size_t ecc_operations_failed;

    // Adapter-specific metrics
    double adapter_overhead_percent;
    double legacy_bridge_overhead_percent;
    size_t legacy_calls_redirected;
    size_t optimized_calls_direct;

    // Constitutional compliance metrics
    bool static_configuration_compliance;
    bool deterministic_behavior_compliance;
    bool no_runtime_queries_compliance;
    double constitutional_compliance_score;

    // Telemetry data
    std::vector<puzzle71::monitoring::PerformanceMetrics> telemetry_history;
    std::chrono::duration<double, std::milli> total_execution_time;

    // Error analysis
    std::vector<std::string> error_messages;
    std::unordered_map<std::string, int> error_counts;

    // Validation results
    bool cpu_gpu_consistency_passed;
    double max_relative_error;
    bool deterministic_replay_passed;
    bool memory_layout_optimization_passed;
};

/**
 * @brief Enhanced fixed adapter layer with comprehensive functionality
 *
 * This is the central adapter that eliminates code duplication across legacy modules
 * and provides a unified interface with constitutional compliance, performance monitoring,
 * deterministic replay support, and seamless migration capabilities.
 */
class LegacyAdapterFixedEnhanced {
public:
    // Constructor and destructor
    LegacyAdapterFixedEnhanced();
    ~LegacyAdapterFixedEnhanced();

    // Initialization and cleanup
    bool initialize(const EnhancedAdapterConfig& config);
    void cleanup();
    bool reconfigure(const EnhancedAdapterConfig& new_config);

    // Memory management interface with layout transparency
    bool allocate_device_memory(void** device_ptr, size_t size, MemoryLayout layout = MemoryLayout::AUTO_DETECT);
    bool allocate_host_memory(void** host_ptr, size_t size, bool pinned = true, MemoryLayout layout = MemoryLayout::AUTO_DETECT);
    bool allocate_soa_memory(ecc::ECCPointSoA* points, size_t size, bool optimize_layout = true);
    bool allocate_aos_memory(void** memory_ptr, size_t struct_size, size_t count, bool optimize_layout = true);
    bool copy_to_device(const void* host_ptr, void* device_ptr, size_t size, MemoryLayout layout = MemoryLayout::AUTO_DETECT);
    bool copy_to_host(const void* device_ptr, void* host_ptr, size_t size, MemoryLayout layout = MemoryLayout::AUTO_DETECT);
    void deallocate_device_memory(void* ptr);
    void deallocate_host_memory(void* ptr);
    bool optimize_memory_layout(MemoryLayout target_layout);

    // ECC operations interface with adapter integration
    bool setup_ecc_operations(const ecc::ECCBatchConfig& ecc_config);
    ecc::ECCOperationsFixed* get_ecc_operations() { return ecc_operations_.get(); }

    // Enhanced ECC operations with monitoring
    bool scalar_multiply_with_monitoring(const uint32_t* private_keys,
                                        ecc::ECCPointSoA* public_keys,
                                        size_t batch_size,
                                        ComprehensivePerformanceReport& report);

    bool point_addition_with_monitoring(const ecc::ECCPointSoA* points_p,
                                       const ecc::ECCPointSoA* points_q,
                                       ecc::ECCPointSoA* points_r,
                                       size_t batch_size,
                                       ComprehensivePerformanceReport& report);

    // Unified interface for legacy compatibility with automatic redirection
    bool legacy_scalar_multiply(const uint32_t* private_keys, uint32_t* public_keys, size_t count);
    bool legacy_point_add(const uint32_t* p1, const uint32_t* p2, uint32_t* result);
    bool legacy_point_double(const uint32_t* point, uint32_t* result);
    bool legacy_batch_inverse(unsigned int accumulator[8]);
    bool legacy_begin_batch_point_add(const unsigned int incX[8], const unsigned int incY[8],
                                     unsigned int* xPtr, unsigned int* chain, int srcIdx, int dstIdx,
                                     unsigned int accumulator[8]);
    bool legacy_complete_batch_point_add(const unsigned int incX[8], const unsigned int incY[8],
                                        unsigned int* xPtr, unsigned int* yPtr, int srcIdx, int dstIdx,
                                        unsigned int* chain, unsigned int accumulator[8],
                                        unsigned int resultX[8], unsigned int resultY[8]);

    // Performance monitoring and telemetry collection
    bool start_performance_monitoring();
    bool stop_performance_monitoring();
    bool enable_real_time_telemetry(const std::string& output_path);
    bool disable_real_time_telemetry();
    bool generate_comprehensive_performance_report(ComprehensivePerformanceReport& report);
    bool export_telemetry_data(const std::string& filename);
    bool import_telemetry_data(const std::string& filename);

    // Real-time metrics access
    MemoryAccessMetrics get_current_memory_metrics() const;
    double get_current_throughput() const;
    double get_current_gpu_utilization() const;
    size_t get_current_memory_usage() const;

    // Deterministic replay support
    bool start_deterministic_recording(const std::string& recording_file);
    bool stop_deterministic_recording();
    bool start_deterministic_replay(const std::string& recording_file);
    bool stop_deterministic_replay();
    bool validate_deterministic_replay(const std::string& recording_file1,
                                     const std::string& recording_file2,
                                     double& max_difference);

    // Constitutional compliance enforcement
    bool validate_constitutional_compliance();
    bool enforce_static_configuration();
    bool disable_runtime_device_queries();
    bool enable_deterministic_behavior();
    double get_constitutional_compliance_score() const;
    bool generate_compliance_report(std::string& report);

    // Error handling and validation with detailed diagnostics
    bool validate_system_state();
    bool validate_memory_layout();
    bool validate_ecc_integration();
    bool validate_performance_targets();
    const char* get_last_error() const;
    bool has_errors() const;
    std::vector<std::string> get_error_history() const;
    void clear_error_history();

    // Configuration access and management
    const EnhancedAdapterConfig& get_config() const { return config_; }
    bool is_initialized() const { return initialized_; }
    bool is_monitoring_active() const { return monitoring_active_; }
    bool is_telemetry_active() const { return telemetry_active_; }
    bool is_deterministic_mode() const { return deterministic_mode_; }

    // Adapter statistics and usage tracking
    struct AdapterStatistics {
        size_t total_operations;
        size_t legacy_operations_redirected;
        size_t optimized_operations_direct;
        size_t fallback_operations;
        double average_execution_time_ms;
        double peak_throughput_ops_per_sec;
        std::chrono::high_resolution_clock::time_point last_operation_time;
    };

    AdapterStatistics get_adapter_statistics() const;
    void reset_statistics();

    // Memory access pattern optimization
    bool analyze_memory_access_patterns();
    bool optimize_memory_access_patterns();
    bool enable_coalesced_access_optimization();
    bool enable_bank_conflict_elimination();
    MemoryAccessMetrics analyze_memory_efficiency();

    // Integration with other modules
    bool integrate_with_hash_module();
    bool integrate_with_compare_module();
    bool integrate_with_unified_candidate_scanner();
    bool validate_module_integration();

private:
    // Core configuration and state
    EnhancedAdapterConfig config_;
    bool initialized_;
    mutable std::mutex error_mutex_;
    mutable std::mutex statistics_mutex_;
    char last_error_[512];
    std::vector<std::string> error_history_;

    // Core components
    std::unique_ptr<ecc::ECCOperationsFixed> ecc_operations_;
    std::unique_ptr<puzzle71::monitoring::RealTimeMonitor> performance_monitor_;
    std::unique_ptr<keyhunt::benchmarks::BaselineManager> baseline_manager_;

    // Memory management
    size_t total_allocated_;
    size_t peak_allocation_;
    std::unordered_map<void*, size_t> memory_allocations_;
    MemoryLayout current_layout_;

    // Performance tracking
    std::atomic<int> kernel_launch_count_;
    std::atomic<int> kernel_success_count_;
    std::atomic<int> kernel_failure_count_;
    std::atomic<int> total_operations_;
    std::atomic<int> legacy_operations_count_;
    std::atomic<int> optimized_operations_count_;
    std::atomic<double> total_execution_time_;
    std::atomic<double> peak_throughput_;

    // Monitoring and telemetry
    std::atomic<bool> monitoring_active_;
    std::atomic<bool> telemetry_active_;
    std::string telemetry_output_path_;
    std::thread telemetry_thread_;
    std::condition_variable telemetry_cv_;
    std::mutex telemetry_mutex_;

    // Deterministic replay
    std::atomic<bool> deterministic_mode_;
    std::string deterministic_recording_file_;
    std::string deterministic_replay_file_;
    std::ofstream deterministic_recording_stream_;
    std::ifstream deterministic_replay_stream_;

    // Memory access optimization
    MemoryAccessMetrics current_memory_metrics_;
    bool memory_optimization_enabled_;

    // Constitutional compliance
    bool constitutional_compliance_enabled_;
    double constitutional_compliance_score_;
    std::chrono::high_resolution_clock::time_point last_compliance_check_;

    // Internal helper methods
    bool setup_core_components();
    bool validate_configuration();
    bool setup_performance_monitoring();
    bool setup_telemetry_system();
    bool setup_deterministic_replay();
    void update_error(const char* error);
    void record_operation_stats(bool success, double execution_time);
    void update_memory_metrics(const MemoryAccessMetrics& metrics);

    // Memory layout helpers
    bool detect_optimal_layout(size_t data_size, MemoryAccessPattern pattern);
    bool convert_memory_layout(void* data, MemoryLayout from, MemoryLayout to, size_t size);
    bool validate_layout_compatibility(MemoryLayout layout1, MemoryLayout layout2);

    // Performance optimization helpers
    bool optimize_for_current_architecture();
    bool apply_static_launch_configuration();
    bool enable_architecture_specific_optimizations();

    // Telemetry and monitoring helpers
    void telemetry_collection_loop();
    bool record_telemetry_point(const ComprehensivePerformanceReport& report);
    bool load_telemetry_history(const std::string& filename);

    // Deterministic replay helpers
    bool record_operation(const std::string& operation_name, const void* data, size_t size);
    bool replay_operation(const std::string& operation_name, void* data, size_t size);
    bool validate_recording_integrity(const std::string& recording_file);

    // Constitutional compliance helpers
    bool validate_static_configuration_compliance();
    bool validate_no_runtime_queries_compliance();
    bool validate_deterministic_behavior_compliance();
    void update_constitutional_compliance_score();
};

/**
 * @brief Global enhanced adapter instance for singleton pattern
 */
extern std::unique_ptr<LegacyAdapterFixedEnhanced> g_enhanced_adapter_instance;

/**
 * @brief Initialize global enhanced adapter instance
 */
bool initialize_global_enhanced_adapter(const EnhancedAdapterConfig& config = EnhancedAdapterConfig());

/**
 * @brief Cleanup global enhanced adapter instance
 */
void cleanup_global_enhanced_adapter();

/**
 * @brief Get global enhanced adapter instance
 */
LegacyAdapterFixedEnhanced* get_global_enhanced_adapter();

/**
 * @brief Convenience functions for common operations
 */
bool initialize_enhanced_adapter_with_defaults();
bool generate_performance_report_and_export(const std::string& filename);
bool validate_constitutional_compliance_and_report(std::string& report);
bool start_monitoring_with_telemetry(const std::string& output_path);

} // namespace adapter

// Legacy namespace compatibility for existing code
namespace legacy = keyhunt::adapter;
namespace enhanced = keyhunt::adapter;

} // namespace keyhunt

// Device-side adapter functions for compatibility
__device__ inline void log_legacy_usage(const char* function_name);
__device__ inline void record_legacy_operation(const char* operation, uint32_t thread_id);
__device__ inline bool is_optimized_path_available();
__device__ inline bool should_use_legacy_fallback();
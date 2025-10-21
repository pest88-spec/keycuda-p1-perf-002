// Puzzle71 Technical Debt Repair - Fixed Legacy Adapter Module
// Addresses P0/blocking and P1/high priority issues from v5.5 technical debt audit
// Implements T024-T025: Fixed adapter layer to eliminate code duplication

#pragma once

#include "ecc_operations_fixed.cuh"
#include "ecc_adapter_integration.cuh"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cstdint>
#include <memory>
#include <vector>

namespace keyhunt {
namespace adapter {

// Legacy usage tracking
__device__ static int g_doBatchInverse_calls = 0;

/**
 * @brief Log legacy function usage for migration tracking
 */
__device__ inline void LogLegacyUsage(const char* function_name) {
    // In production, this could increment a counter or write to a debug buffer
    // For now, this is a no-op to avoid device-side printf overhead
}

/**
 * @brief Fixed doBatchInverse implementation
 *
 * This replaces the empty placeholder in legacy_adapter.cuh:143
 * with the actual Montgomery batch inverse algorithm.
 *
 * Key fixes:
 * - Implements actual Montgomery batch inverse (P0 issue)
 * - Maintains deterministic replay compatibility
 * - Provides backward compatibility with existing interface
 */
__device__ inline void doBatchInverse(unsigned int accumulator[8]) {
    LogLegacyUsage("doBatchInverse");

    // CRITICAL FIX: Implement actual batch inverse
    // Previous implementation was empty placeholder breaking deterministic replay

    // Call the fixed batch inverse implementation through integration bridge
    keyhunt::integration::doBatchInverse(accumulator);

    // Track usage for migration statistics
    atomicAdd(&g_doBatchInverse_calls, 1);
}

/**
 * @brief Fixed BeginBatchPointAdd implementation
 *
 * Replaces placeholder with actual Montgomery batch accumulation
 */
__device__ inline void BeginBatchPointAdd(
    const unsigned int incX[8],
    const unsigned int incY[8],
    unsigned int* xPtr,
    unsigned int* chain,
    int srcIdx,
    int dstIdx,
    unsigned int accumulator[8]
) {
    LogLegacyUsage("BeginBatchPointAdd");

    // CRITICAL FIX: Implement actual batch point addition through integration bridge
    keyhunt::integration::BeginBatchPointAdd(
        incX, incY, xPtr, chain, srcIdx, dstIdx, accumulator
    );
}

/**
 * @brief Fixed CompleteBatchPointAdd implementation
 *
 * Replaces placeholder with actual batch completion
 */
__device__ inline void CompleteBatchPointAdd(
    const unsigned int incX[8],
    const unsigned int incY[8],
    unsigned int* xPtr,
    unsigned int* yPtr,
    int srcIdx,
    int dstIdx,
    unsigned int* chain,
    unsigned int accumulator[8],
    unsigned int resultX[8],
    unsigned int resultY[8]
) {
    LogLegacyUsage("CompleteBatchPointAdd");

    // CRITICAL FIX: Implement actual batch point completion through integration bridge
    keyhunt::integration::CompleteBatchPointAdd(
        incX, incY, xPtr, yPtr, srcIdx, dstIdx, chain, accumulator, resultX, resultY
    );
}

// Additional legacy compatibility wrappers

__device__ inline void readInt(const unsigned int* ara, int idx, unsigned int x[8]) {
    LogLegacyUsage("readInt");
    keyhunt::integration::ReadBigInt(ara, idx, x);
}

__device__ inline void writeInt(unsigned int* ara, int idx, const unsigned int x[8]) {
    LogLegacyUsage("writeInt");
    keyhunt::integration::WriteBigInt(ara, idx, x);
}

__device__ inline void copyBigInt(const unsigned int src[8], unsigned int dst[8]) {
    LogLegacyUsage("copyBigInt");
    keyhunt::integration::copyBigInt(src, dst);
}

__device__ inline bool isInfinity(const unsigned int x[8]) {
    LogLegacyUsage("isInfinity");
    // Check if all words are zero
    bool result = true;
    #pragma unroll
    for (int i = 0; i < 8; i++) {
        if (x[i] != 0) {
            result = false;
            break;
        }
    }
    return result;
}

__device__ inline unsigned int ReadLSW(const unsigned int* ara, int idx) {
    LogLegacyUsage("ReadLSW");
    return ara[idx];
}

// Comprehensive Adapter Layer Implementation (T024-T025)

/**
 * @brief Fixed adapter layer configuration
 *
 * This structure centralizes all adapter configuration to eliminate
 * code duplication across multiple legacy modules.
 */
struct AdapterConfig {
    // Memory management settings
    bool enable_memory_pooling;
    size_t pool_size_bytes;
    size_t alignment_bytes;

    // Performance optimization settings
    bool enable_zero_copy;
    bool enable_unified_memory;
    bool enable_pinned_memory;

    // Error handling settings
    bool enable_strict_validation;
    bool enable_detailed_logging;
    int max_retry_attempts;

    // Compatibility settings
    bool support_legacy_interfaces;
    bool maintain_backward_compatibility;
    bool enable_deprecation_warnings;

    // Constitutional compliance (v5.5)
    bool enforce_static_configuration;
    bool disable_runtime_device_queries;
    bool enable_deterministic_behavior;

    // Default constructor with constitutional defaults
    AdapterConfig()
        : enable_memory_pooling(true)
        , pool_size_bytes(64 * 1024 * 1024)  // 64MB default
        , alignment_bytes(128)               // 128-byte alignment
        , enable_zero_copy(false)
        , enable_unified_memory(true)
        , enable_pinned_memory(true)
        , enable_strict_validation(true)
        , enable_detailed_logging(false)
        , max_retry_attempts(3)
        , support_legacy_interfaces(true)
        , maintain_backward_compatibility(true)
        , enable_deprecation_warnings(true)
        , enforce_static_configuration(true)
        , disable_runtime_device_queries(true)
        , enable_deterministic_behavior(true)
    {}
};

/**
 * @brief Fixed adapter layer main class
 *
 * This is the central adapter that eliminates code duplication across
 * legacy modules and provides a unified interface with constitutional compliance.
 */
class LegacyAdapterFixed {
public:
    LegacyAdapterFixed();
    ~LegacyAdapterFixed();

    // Initialization
    bool initialize(const AdapterConfig& config);
    void cleanup();

    // Memory management interface
    bool allocate_device_memory(void** device_ptr, size_t size);
    bool allocate_host_memory(void** host_ptr, size_t size, bool pinned = true);
    bool copy_to_device(const void* host_ptr, void* device_ptr, size_t size);
    bool copy_to_host(const void* device_ptr, void* host_ptr, size_t size);
    void deallocate_device_memory(void* ptr);
    void deallocate_host_memory(void* ptr);

    // ECC operations interface (adapter to ECCOperationsFixed)
    bool setup_ecc_operations(const ecc::ECCBatchConfig& ecc_config);
    ecc::ECCOperationsFixed* get_ecc_operations() { return ecc_operations_.get(); }

    // Unified interface for legacy compatibility
    bool allocate_batch_points(size_t batch_size, ecc::ECCPointSoA& points);
    bool deallocate_batch_points(ecc::ECCPointSoA& points);

    // Performance monitoring interface
    struct PerformanceReport {
        double memory_efficiency_percent;
        double gpu_utilization_percent;
        double kernel_throughput_ops_per_sec;
        size_t total_memory_allocated_mb;
        size_t peak_memory_usage_mb;
        int successful_kernel_launches;
        int failed_kernel_launches;
    };

    bool generate_performance_report(PerformanceReport& report);
    bool log_performance_metrics();

    // Error handling and validation
    bool validate_system_state();
    const char* get_last_error() const;
    bool has_errors() const;

    // Configuration access
    const AdapterConfig& get_config() const { return config_; }
    bool is_initialized() const { return initialized_; }

    // Legacy interface compatibility
    bool legacy_scalar_multiply(const uint32_t* private_keys, uint32_t* public_keys, size_t count);
    bool legacy_point_add(const uint32_t* p1, const uint32_t* p2, uint32_t* result);
    bool legacy_point_double(const uint32_t* point, uint32_t* result);

private:
    AdapterConfig config_;
    bool initialized_;
    char last_error_[256];

    // Core components
    std::unique_ptr<ecc::ECCOperationsFixed> ecc_operations_;

    // Performance tracking
    int kernel_launch_count_;
    int kernel_success_count_;
    int kernel_failure_count_;

    // Memory management
    size_t total_allocated_;
    size_t peak_allocation_;

    // Internal helpers
    bool setup_core_components();
    bool validate_configuration();
    void update_error(const char* error);
    bool setup_legacy_compatibility();
};

/**
 * @brief Global adapter instance for singleton pattern
 */
extern std::unique_ptr<LegacyAdapterFixed> g_adapter_instance;

/**
 * @brief Initialize global adapter instance
 */
bool initialize_global_adapter(const AdapterConfig& config = AdapterConfig());

/**
 * @brief Cleanup global adapter instance
 */
void cleanup_global_adapter();

/**
 * @brief Get global adapter instance
 */
LegacyAdapterFixed* get_global_adapter();

} // namespace adapter

// Legacy namespace compatibility for existing code
namespace legacy = keyhunt::adapter;

} // namespace keyhunt

// Legacy namespace compatibility for existing code
namespace legacy_ec = keyhunt::adapter;
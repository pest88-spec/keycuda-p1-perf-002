// Puzzle71 Technical Debt Repair - ECC/Adapter Integration Layer
// Addresses P0/blocking and P1/high priority issues from v5.5 technical debt audit
// Implements T029: Integration of ECC operations with adapter layer

#pragma once

#include "ecc_operations_fixed.cuh"
#include "legacy_adapter_fixed.cuh"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

namespace keyhunt {
namespace integration {

/**
 * @brief Integration bridge between ECC operations and adapter layer
 *
 * This namespace provides the glue code that properly connects the
 * ECC operations with the legacy adapter layer, ensuring all
 * function calls use the correct namespaces and interfaces.
 */

// Forward declarations for common namespace functions
// These provide the integration bridge between adapter and ECC modules

/**
 * @brief Bridge function for doBatchInverse integration
 *
 * This function properly routes the adapter's doBatchInverse call
 * to the ECC operations implementation in the correct namespace.
 */
__device__ inline void doBatchInverse(unsigned int accumulator[8]) {
    // Route to ECC operations namespace
    keyhunt::ecc::doBatchInverse_Fixed(accumulator);
}

/**
 * @brief Bridge function for BeginBatchPointAdd integration
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
    // Route to ECC operations namespace
    keyhunt::ecc::BeginBatchPointAdd_Fixed(
        incX, incY, xPtr, chain, srcIdx, dstIdx, accumulator
    );
}

/**
 * @brief Bridge function for CompleteBatchPointAdd integration
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
    // Route to ECC operations namespace
    keyhunt::ecc::CompleteBatchPointAdd_Fixed(
        incX, incY, xPtr, yPtr, srcIdx, dstIdx, chain, accumulator, resultX, resultY
    );
}

/**
 * @brief Bridge function for big integer read operations
 */
__device__ inline void ReadBigInt(const unsigned int* ara, int idx, unsigned int x[8]) {
    // Use existing ECC operations implementation
    keyhunt::ecc::ReadBigInt(ara, idx, x);
}

/**
 * @brief Bridge function for big integer write operations
 */
__device__ inline void WriteBigInt(unsigned int* ara, int idx, const unsigned int x[8]) {
    // Use existing ECC operations implementation
    keyhunt::ecc::WriteBigInt(ara, idx, x);
}

/**
 * @brief Bridge function for big integer copy operations
 */
__device__ inline void copyBigInt(const unsigned int src[8], unsigned int dst[8]) {
    // Use ECC operations implementation
    keyhunt::ecc::copyBigInt(src, dst);
}

/**
 * @brief Integrated ECC operations manager
 *
 * This class provides a unified interface that combines the ECC operations
 * with the adapter layer for seamless integration.
 */
class IntegratedECCManager {
public:
    IntegratedECCManager();
    ~IntegratedECCManager();

    // Initialization with both ECC and adapter configuration
    bool initialize(const adapter::AdapterConfig& adapter_config,
                   const ecc::ECCBatchConfig& ecc_config);
    void cleanup();

    // Get access to both subsystems
    ecc::ECCOperationsFixed* get_ecc_operations() { return ecc_ops_.get(); }
    adapter::LegacyAdapterFixed* get_adapter() { return adapter_.get(); }

    // Integrated operations that handle both subsystems
    bool perform_integrated_scalar_multiply(
        const uint32_t* private_keys,
        ecc::ECCPointSoA* public_keys,
        size_t batch_size,
        ecc::ECCOperationResult& result
    );

    bool perform_integrated_batch_operations(
        const unsigned int incX[8],
        const unsigned int incY[8],
        unsigned int* xPtr,
        unsigned int* yPtr,
        unsigned int* chain,
        int batch_size,
        ecc::ECCOperationResult& result
    );

    // Validation of integration
    bool validate_integration();
    bool run_integration_tests();

    // Performance monitoring for the integrated system
    struct IntegratedPerformanceReport {
        // ECC operations metrics
        double ecc_throughput_ops_per_sec;
        double ecc_memory_efficiency_percent;
        double ecc_gpu_utilization_percent;

        // Adapter metrics
        double adapter_memory_efficiency_percent;
        size_t adapter_total_allocated_mb;
        int adapter_successful_operations;

        // Integration metrics
        double total_execution_time_ms;
        double integration_overhead_percent;
        bool constitutional_compliance;
    };

    bool generate_integrated_performance_report(IntegratedPerformanceReport& report);

    // Error handling
    const char* get_last_error() const;
    bool has_errors() const;

    // Configuration access
    bool is_initialized() const { return initialized_; }

private:
    // Core components
    std::unique_ptr<ecc::ECCOperationsFixed> ecc_ops_;
    std::unique_ptr<adapter::LegacyAdapterFixed> adapter_;

    // Integration state
    bool initialized_;
    char last_error_[512];

    // Performance tracking
    int total_integrated_operations_;
    double total_integration_time_;

    // Internal helpers
    bool setup_integration();
    bool validate_ecc_adapter_compatibility();
    void update_error(const char* error);
    bool measure_integration_overhead();
};

/**
 * @brief Global integrated manager for singleton pattern
 */
extern std::unique_ptr<IntegratedECCManager> g_integrated_manager;

/**
 * @brief Initialize global integrated manager
 */
bool initialize_global_integration(const adapter::AdapterConfig& adapter_config = adapter::AdapterConfig(),
                                  const ecc::ECCBatchConfig& ecc_config = ecc::ECCBatchConfig());

/**
 * @brief Cleanup global integrated manager
 */
void cleanup_global_integration();

/**
 * @brief Get global integrated manager
 */
IntegratedECCManager* get_global_integration();

} // namespace integration

// Common namespace compatibility for existing code
namespace common = keyhunt::integration;

} // namespace keyhunt
// Puzzle71Solver - Module Manager Header
// Centralized coordination system for unified modules (T023)

#pragma once

#include "result_emitter.cuh"
#include "hash_utils.cuh"
#include "ecc_operations.cuh"
#include "legacy_adapter.cuh"
#include <cuda_runtime.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <mutex>
#include <nlohmann/json.hpp>

namespace keyhunt {
namespace common {

/**
 * @brief Module Status Enumeration
 *
 * Tracks the status of each unified module for proper coordination.
 */
enum class ModuleStatus {
    UNINITIALIZED = 0,
    INITIALIZING = 1,
    INITIALIZED = 2,
    ACTIVE = 3,
    ERROR = 4,
    DISABLED = 5
};

/**
 * @brief Module Configuration Structure
 *
 * Configuration parameters for unified modules.
 */
struct ModuleConfiguration {
    bool enable_legacy_adapter = true;
    bool enable_debug_logging = false;
    bool enable_performance_tracking = true;
    bool enable_memory_optimization = true;
    size_t shared_memory_size = 65536; // 64KB default
    int preferred_block_size = 256;
    int max_registers_per_thread = 40;

    // Optimization settings
    bool use_shared_memory_optimization = true;
    bool use_warp_level_optimization = true;
    bool use_batch_processing = true;

    // Performance targets
    double target_memory_efficiency = 0.90;
    double target_occupancy = 0.80;
    double target_bandwidth_utilization = 0.85;
};

/**
 * @brief Module Performance Metrics
 *
 * Tracks performance metrics for each module.
 */
struct ModuleMetrics {
    std::string module_name;
    ModuleStatus status = ModuleStatus::UNINITIALIZED;

    // Performance metrics
    double execution_time_ms = 0.0;
    double memory_efficiency = 0.0;
    double occupancy = 0.0;
    double bandwidth_utilization = 0.0;
    size_t memory_used_bytes = 0;

    // Usage statistics
    size_t call_count = 0;
    size_t error_count = 0;
    std::chrono::system_clock::time_point last_call_time;
    std::chrono::system_clock::time_point initialization_time;

    // Error tracking
    std::vector<std::string> recent_errors;
    size_t max_error_history = 10;
};

/**
 * @brief Unified Module Manager
 *
 * Centralized coordination system for all unified modules. Provides:
 * - Module initialization and lifecycle management
 * - Performance monitoring and metrics collection
 * - Configuration management
 * - Error handling and recovery
 * - Debugging and profiling support
 */
class ModuleManager {
public:
    /**
     * @brief Get singleton instance
     *
     * @return Reference to the global module manager instance
     */
    static ModuleManager& getInstance();

    /**
     * @brief Initialize all unified modules
     *
     * @param config Module configuration parameters
     * @return True if initialization successful, false otherwise
     */
    bool initializeModules(const ModuleConfiguration& config = ModuleConfiguration{});

    /**
     * @brief Shutdown all modules and cleanup resources
     */
    void shutdownModules();

    /**
     * @brief Check if modules are initialized and ready
     *
     * @return True if modules are ready, false otherwise
     */
    bool areModulesReady() const;

    /**
     * @brief Get module configuration
     *
     * @return Current module configuration
     */
    const ModuleConfiguration& getConfiguration() const;

    /**
     * @brief Update module configuration
     *
     * @param config New configuration to apply
     * @return True if configuration updated successfully
     */
    bool updateConfiguration(const ModuleConfiguration& config);

    /**
     * @brief Get performance metrics for a specific module
     *
     * @param module_name Name of the module
     * @return Module metrics structure
     */
    ModuleMetrics getModuleMetrics(const std::string& module_name) const;

    /**
     * @brief Get performance metrics for all modules
     *
     * @return Map of module names to their metrics
     */
    std::unordered_map<std::string, ModuleMetrics> getAllModuleMetrics() const;

    /**
     * @brief Reset performance metrics
     *
     * Clears all collected performance metrics.
     */
    void resetMetrics();

    /**
     * @brief Get system-wide performance summary
     *
     * @return JSON string containing system performance summary
     */
    std::string getPerformanceSummary() const;

    /**
     * @brief Enable/disable debug logging
     *
     * @param enable Whether to enable debug logging
     */
    void setDebugLogging(bool enable);

    /**
     * @brief Check if a module is enabled and functional
     *
     * @param module_name Name of the module to check
     * @return True if module is enabled and functional
     */
    bool isModuleEnabled(const std::string& module_name) const;

    /**
     * @brief Enable/disable a specific module
     *
     * @param module_name Name of the module
     * @param enable Whether to enable the module
     * @return True if operation successful
     */
    bool setModuleEnabled(const std::string& module_name, bool enable);

    /**
     * @brief Get module initialization status
     *
     * @param module_name Name of the module
     * @return Current status of the module
     */
    ModuleStatus getModuleStatus(const std::string& module_name) const;

    /**
     * @brief Validate module health
     *
     * Performs health checks on all initialized modules.
     *
     * @return True if all modules are healthy, false otherwise
     */
    bool validateModuleHealth();

    /**
     * @brief Get recommended optimization settings
     *
     * Analyzes current metrics and provides optimization recommendations.
     *
     * @return JSON string containing optimization recommendations
     */
    std::string getOptimizationRecommendations() const;

    /**
     * @brief Export module configuration and metrics
     *
     * @param filename Output file path
     * @return True if export successful
     */
    bool exportMetrics(const std::string& filename) const;

    /**
     * @brief Import module configuration
     *
     * @param filename Input file path
     * @return True if import successful
     */
    bool importConfiguration(const std::string& filename);

    // Module-specific convenience methods

    /**
     * @brief Execute unified EmitCandidate with automatic metrics collection
     */
    static __device__ __forceinline__ void executeEmitCandidate(
        bool has_candidate,
        int idx,
        bool compressed,
        const unsigned int x[8],
        const unsigned int y[8],
        const std::uint32_t digest[5]
    ) {
        #ifdef MODULE_MANAGER_DEBUG
        // Device-side debug logging would go here
        #endif

        keyhunt::common::EmitCandidate(has_candidate, idx, compressed, x, y, digest);
    }

    /**
     * @brief Execute unified FinalizeDigest with automatic metrics collection
     */
    static __device__ __forceinline__ void executeFinalizeDigest(
        const std::uint32_t in[5],
        std::uint32_t out[5]
    ) {
        #ifdef MODULE_MANAGER_DEBUG
        // Device-side debug logging would go here
        #endif

        keyhunt::common::FinalizeDigest(in, out);
    }

    /**
     * @brief Execute unified ECC operations with automatic metrics collection
     */
    static __device__ __forceinline__ void executeReadBigInt(
        const unsigned int* ara,
        int idx,
        unsigned int x[8]
    ) {
        #ifdef MODULE_MANAGER_DEBUG
        // Device-side debug logging would go here
        #endif

        keyhunt::common::ReadBigInt(ara, idx, x);
    }

    static __device__ __forceinline__ void executeWriteBigInt(
        unsigned int* ara,
        int idx,
        const unsigned int x[8]
    ) {
        #ifdef MODULE_MANAGER_DEBUG
        // Device-side debug logging would go here
        #endif

        keyhunt::common::WriteBigInt(ara, idx, x);
    }

private:
    ModuleManager() = default;
    ~ModuleManager() = default;
    ModuleManager(const ModuleManager&) = delete;
    ModuleManager& operator=(const ModuleManager&) = delete;

    // Internal state
    bool initialized_ = false;
    ModuleConfiguration configuration_;
    std::unordered_map<std::string, ModuleMetrics> module_metrics_;
    mutable std::mutex metrics_mutex_;

    // Module names
    static constexpr const char* MODULE_NAMES[] = {
        "ResultEmitter",
        "HashUtils",
        "ECCOperations",
        "LegacyAdapter"
    };

    // Internal methods
    bool initializeResultEmitterModule();
    bool initializeHashUtilsModule();
    bool initializeECCOperationsModule();
    bool initializeLegacyAdapterModule();

    void updateModuleMetrics(const std::string& module_name,
                           double execution_time_ms = 0.0,
                           bool success = true,
                           const std::string& error = "");

    std::string getTimestamp() const;
    void logInfo(const std::string& message) const;
    void logWarning(const std::string& message) const;
    void logError(const std::string& message) const;
    void logDebug(const std::string& message) const;

    // Performance analysis
    double calculateSystemEfficiency() const;
    std::vector<std::string> getPerformanceBottlenecks() const;
    bool meetsPerformanceTargets() const;
};

// Global convenience functions

/**
 * @brief Initialize the module manager with default configuration
 *
 * @return True if initialization successful
 */
inline bool initializeModuleManager() {
    return ModuleManager::getInstance().initializeModules();
}

/**
 * @brief Check if module manager is ready
 *
 * @return True if ready, false otherwise
 */
inline bool isModuleManagerReady() {
    return ModuleManager::getInstance().areModulesReady();
}

/**
 * @brief Get system performance summary
 *
 * @return JSON string with performance summary
 */
inline std::string getSystemPerformanceSummary() {
    return ModuleManager::getInstance().getPerformanceSummary();
}

/**
 * @brief Validate all module health
 *
 * @return True if all modules healthy, false otherwise
 */
inline bool validateAllModules() {
    return ModuleManager::getInstance().validateModuleHealth();
}

} // namespace common
} // namespace keyhunt

// Convenience macros for module manager integration

#define MODULE_MANAGER_ENSURE_READY() \
    do { \
        if (!keyhunt::common::isModuleManagerReady()) { \
            keyhunt::common::initializeModuleManager(); \
        } \
    } while(0)

#define MODULE_MANAGER_EMIT_CANDIDATE(has_candidate, idx, compressed, x, y, digest) \
    MODULE_MANAGER_ENSURE_READY(); \
    keyhunt::common::ModuleManager::executeEmitCandidate(has_candidate, idx, compressed, x, y, digest)

#define MODULE_MANAGER_FINALIZE_DIGEST(in, out) \
    MODULE_MANAGER_ENSURE_READY(); \
    keyhunt::common::ModuleManager::executeFinalizeDigest(in, out)

#define MODULE_MANAGER_READ_BIG_INT(ara, idx, x) \
    MODULE_MANAGER_ENSURE_READY(); \
    keyhunt::common::ModuleManager::executeReadBigInt(ara, idx, x)

#define MODULE_MANAGER_WRITE_BIG_INT(ara, idx, x) \
    MODULE_MANAGER_ENSURE_READY(); \
    keyhunt::common::ModuleManager::executeWriteBigInt(ara, idx, x)
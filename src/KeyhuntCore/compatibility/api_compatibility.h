// Puzzle71Solver - Backward API Compatibility Layer (T053)
// Phase 7: User Story 5 - Compatibility Assurance
// Comprehensive backward compatibility layer for maintaining API compatibility

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>
#include <mutex>
#include <atomic>

#include "common/result_emitter.h"
#include "common/hash_utils.h"
#include "common/ecc_operations.h"
#include "gpu/launch_config.h"
#include "memory/soa_manager.h"

namespace puzzle71::compatibility {

/**
 * @brief Compatibility mode levels
 */
enum class CompatibilityMode {
    STRICT,          // Strict backward compatibility - no behavior changes
    LEGACY,          // Legacy mode - emulate old behavior exactly
    MODERN,          // Modern mode - use new APIs with compatibility wrappers
    HYBRID           // Hybrid mode - mix of legacy and modern behavior
};

/**
 * @brief API version information
 */
struct APIVersion {
    uint32_t major{1};
    uint32_t minor{0};
    uint32_t patch{0};
    std::string build_info;
    std::chrono::system_clock::time_point release_date;

    /**
     * @brief Get version as string
     */
    std::string toString() const;

    /**
     * @brief Compare versions
     */
    int compare(const APIVersion& other) const;

    /**
     * @brief Check if version is compatible
     */
    bool isCompatible(const APIVersion& required) const;
};

/**
 * @brief Legacy configuration structure (pre-refactoring)
 */
struct LegacyConfig {
    // Original configuration fields
    int device_id{0};
    size_t threads_per_block{256};
    size_t blocks_per_grid{0};
    size_t batch_size{1000};
    bool use_separated_kernels{false};
    bool enable_optimization{true};
    std::string kernel_mode{"default"};

    // Memory configuration
    size_t memory_pool_size{1024 * 1024 * 1024};  // 1GB
    bool use_soa_layout{false};
    int memory_alignment{16};

    // Performance configuration
    bool enable_profiling{false};
    bool enable_monitoring{false};
    int warmup_iterations{10};
    int benchmark_iterations{100};

    // Legacy fields that may have different meanings now
    int legacy_flag_1{0};
    int legacy_flag_2{0};
    std::string legacy_option_1;
    std::string legacy_option_2;

    /**
     * @brief Convert to modern configuration
     */
    puzzle71::gpu::LaunchConfig toModernConfig() const;

    /**
     * @brief Create from modern configuration
     */
    static LegacyConfig fromModernConfig(const puzzle71::gpu::LaunchConfig& modern_config);
};

/**
 * @brief Legacy result structure (pre-refactoring)
 */
struct LegacyResult {
    // Basic result fields
    bool success{false};
    std::string error_message;
    size_t keys_processed{0};
    double processing_time{0.0};
    double throughput{0.0};

    // GPU information
    int device_id{-1};
    std::string device_name;
    double gpu_utilization{0.0};
    size_t memory_used{0};

    // Result data (original format)
    std::vector<std::string> found_keys;
    std::vector<std::string> found_addresses;
    std::vector<uint32_t> match_indices;

    // Legacy fields
    int legacy_status_code{0};
    std::string legacy_status_message;
    std::map<std::string, std::string> legacy_metadata;

    /**
     * @brief Convert to modern result format
     */
    puzzle71::monitoring::PerformanceMetrics toModernMetrics() const;

    /**
     * @brief Create from modern result format
     */
    static LegacyResult fromModernMetrics(const puzzle71::monitoring::PerformanceMetrics& modern_metrics);
};

/**
 * @brief Legacy kernel parameters (pre-refactoring)
 */
struct LegacyKernelParams {
    // Basic parameters
    uint64_t start_key{0};
    uint64_t end_key{0};
    uint64_t stride{1};
    int device_id{0};

    // Target data
    std::vector<std::string> target_addresses;
    std::vector<uint8_t> target_hashes;

    // Kernel configuration
    size_t threads_per_block{256};
    size_t blocks_per_grid{0};
    size_t shared_memory_size{0};

    // Memory pointers (legacy format)
    void* d_target_data{nullptr};
    void* d_result_data{nullptr};
    void* d_working_memory{nullptr};

    // Legacy flags
    bool use_compression{false};
    bool use_fast_math{true};
    int optimization_level{3};

    /**
     * @brief Convert to modern kernel parameters
     */
    puzzle71::gpu::KernelLaunchParams toModernParams() const;

    /**
     * @brief Create from modern kernel parameters
     */
    static LegacyKernelParams fromModernParams(const puzzle71::gpu::KernelLaunchParams& modern_params);
};

/**
 * @brief Compatibility wrapper for legacy API functions
 */
class LegacyAPIWrapper {
public:
    explicit LegacyAPIWrapper(CompatibilityMode mode = CompatibilityMode::HYBRID);
    ~LegacyAPIWrapper();

    // Legacy initialization functions
    int legacy_initialize(const LegacyConfig& config);
    int legacy_shutdown();
    int legacy_reset_device(int device_id);

    // Legacy kernel execution functions
    int legacy_launch_kernel(const LegacyKernelParams& params);
    int legacy_launch_batch(const std::vector<LegacyKernelParams>& batch_params);
    int legacy_wait_for_completion();
    int legacy_cancel_execution();

    // Legacy result functions
    LegacyResult legacy_get_results();
    std::vector<LegacyResult> legacy_get_batch_results();
    int legacy_get_result_count();
    bool legacy_is_execution_complete();

    // Legacy memory management functions
    void* legacy_allocate_memory(size_t size);
    void legacy_free_memory(void* ptr);
    int legacy_copy_to_device(void* device_ptr, const void* host_ptr, size_t size);
    int legacy_copy_to_host(void* host_ptr, const void* device_ptr, size_t size);

    // Legacy configuration functions
    int legacy_set_config(const std::string& key, const std::string& value);
    std::string legacy_get_config(const std::string& key);
    int legacy_load_config_from_file(const std::string& filename);
    int legacy_save_config_to_file(const std::string& filename);

    // Legacy profiling functions
    int legacy_start_profiling();
    int legacy_stop_profiling();
    std::map<std::string, double> legacy_get_profile_data();

    // Legacy monitoring functions
    int legacy_enable_monitoring();
    int legacy_disable_monitoring();
    std::map<std::string, double> legacy_get_current_metrics();

    // Compatibility management
    void setCompatibilityMode(CompatibilityMode mode);
    CompatibilityMode getCompatibilityMode() const;

    APIVersion getAPIVersion() const;
    bool isFunctionSupported(const std::string& function_name) const;

    // Migration assistance
    std::vector<std::string> getDeprecatedFunctions() const;
    std::vector<std::string> getMigrationHints() const;
    std::string generateMigrationReport() const;

private:
    CompatibilityMode mode_;
    APIVersion current_version_;
    mutable std::mutex wrapper_mutex_;

    // Modern system components
    std::unique_ptr<puzzle71::common::ModuleManager> module_manager_;
    std::unique_ptr<puzzle71::gpu::LaunchConfigManager> launch_config_manager_;
    std::unique_ptr<puzzle71::memory::SoAManager> soa_manager_;

    // Legacy state tracking
    std::map<std::string, std::string> legacy_config_;
    std::vector<LegacyResult> pending_results_;
    std::atomic<bool> execution_active_{false};
    std::atomic<bool> monitoring_enabled_{false};

    // Compatibility mappings
    std::map<std::string, std::function<int(const std::vector<std::string>&)>> function_mappings_;
    std::map<std::string, bool> deprecated_functions_;

    // Private methods
    void initializeFunctionMappings();
    void initializeDeprecatedFunctions();

    // Function implementations
    int legacyInitializeImpl(const std::vector<std::string>& args);
    int legacyShutdownImpl(const std::vector<std::string>& args);
    int legacyLaunchKernelImpl(const std::vector<std::string>& args);
    int legacyGetResultsImpl(const std::vector<std::string>& args);

    // Conversion utilities
    LegacyConfig convertConfig(const std::map<std::string, std::string>& config_map) const;
    LegacyKernelParams convertKernelParams(const std::map<std::string, std::string>& param_map) const;

    // Error handling
    int handleCompatibilityError(const std::string& function_name, const std::string& error_message) const;
    void logCompatibilityWarning(const std::string& function_name, const std::string& warning_message) const;
};

/**
 * @brief API compatibility validator
 */
class APICompatibilityValidator {
public:
    explicit APICompatibilityValidator();
    ~APICompatibilityValidator() = default;

    /**
     * @brief Validate API call compatibility
     */
    bool validateFunctionCall(const std::string& function_name, const std::vector<std::string>& args) const;

    /**
     * @brief Validate configuration compatibility
     */
    bool validateConfiguration(const LegacyConfig& config) const;

    /**
     * @brief Validate kernel parameters compatibility
     */
    bool validateKernelParams(const LegacyKernelParams& params) const;

    /**
     * @brief Get compatibility report
     */
    struct CompatibilityReport {
        bool is_compatible{true};
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
        std::vector<std::string> recommendations;
        std::map<std::string, std::string> deprecated_alternatives;
    };

    CompatibilityReport generateReport(const LegacyConfig& config, const LegacyKernelParams& params) const;

    /**
     * @brief Check for deprecated usage
     */
    std::vector<std::string> checkDeprecatedUsage(const std::string& function_name, const std::vector<std::string>& args) const;

private:
    std::map<std::string, std::function<bool(const std::vector<std::string>&)>> validators_;
    std::map<std::string, std::vector<std::string>> deprecated_signatures_;

    void initializeValidators();
    void initializeDeprecatedSignatures();
};

/**
 * @brief Migration assistant for API transitions
 */
class APIMigrationAssistant {
public:
    explicit APIMigrationAssistant();
    ~APIMigrationAssistant() = default;

    /**
     * @brief Generate migration plan
     */
    struct MigrationStep {
        std::string description;
        std::string old_code;
        std::string new_code;
        std::string justification;
        int priority{0};  // 1=highest, 5=lowest
    };

    std::vector<MigrationStep> generateMigrationPlan(const std::string& source_file) const;

    /**
     * @brief Analyze code for compatibility issues
     */
    struct CodeAnalysis {
        std::vector<std::string> deprecated_functions;
        std::vector<std::string> incompatible_calls;
        std::vector<std::string> required_changes;
        std::map<std::string, int> complexity_scores;
    };

    CodeAnalysis analyzeCode(const std::string& source_code) const;

    /**
     * @brief Generate automated migration patches
     */
    std::string generatePatch(const std::string& original_code) const;

    /**
     * @brief Validate migration results
     */
    bool validateMigration(const std::string& original_code, const std::string& migrated_code) const;

    /**
     * @brief Get migration documentation
     */
    std::string getMigrationDocumentation() const;

private:
    std::map<std::string, std::string> function_mappings_;
    std::map<std::string, std::vector<std::string>> parameter_mappings_;

    void initializeFunctionMappings();
    void initializeParameterMappings();
};

/**
 * @brief Compatibility testing framework
 */
class CompatibilityTestFramework {
public:
    explicit CompatibilityTestFramework();
    ~CompatibilityTestFramework() = default;

    /**
     * @brief Run compatibility tests
     */
    struct TestResult {
        std::string test_name;
        bool passed{false};
        std::string error_message;
        double execution_time{0.0};
        std::map<std::string, std::string> metrics;
    };

    std::vector<TestResult> runAllTests();
    TestResult runFunctionTest(const std::string& function_name);
    TestResult runConfigurationTest(const LegacyConfig& config);
    TestResult runPerformanceTest(const LegacyKernelParams& params);

    /**
     * @brief Compare legacy vs modern behavior
     */
    struct ComparisonResult {
        std::string metric_name;
        double legacy_value{0.0};
        double modern_value{0.0};
        double difference_percentage{0.0};
        bool within_tolerance{false};
    };

    std::vector<ComparisonResult> compareImplementations(const LegacyKernelParams& params);

    /**
     * @brief Generate compatibility test report
     */
    std::string generateTestReport(const std::vector<TestResult>& results) const;

private:
    std::unique_ptr<LegacyAPIWrapper> legacy_wrapper_;
    std::unique_ptr<APICompatibilityValidator> validator_;
    std::map<std::string, std::function<TestResult()>> test_functions_;

    void initializeTestFunctions();
};

/**
 * @brief Global compatibility manager
 */
class CompatibilityManager {
public:
    static CompatibilityManager& getInstance();

    /**
     * @brief Initialize compatibility system
     */
    bool initialize(CompatibilityMode mode = CompatibilityMode::HYBRID);

    /**
     * @brief Get API wrapper
     */
    std::shared_ptr<LegacyAPIWrapper> getAPIWrapper();

    /**
     * @brief Get validator
     */
    std::shared_ptr<APICompatibilityValidator> getValidator();

    /**
     * @brief Get migration assistant
     */
    std::shared_ptr<APIMigrationAssistant> getMigrationAssistant();

    /**
     * @brief Get test framework
     */
    std::shared_ptr<CompatibilityTestFramework> getTestFramework();

    /**
     * @brief Global compatibility settings
     */
    void setGlobalCompatibilityMode(CompatibilityMode mode);
    CompatibilityMode getGlobalCompatibilityMode() const;

    /**
     * @brief Enable/disable compatibility warnings
     */
    void setWarningsEnabled(bool enabled);
    bool areWarningsEnabled() const;

    /**
     * @brief Get system compatibility report
     */
    std::string getSystemCompatibilityReport() const;

private:
    CompatibilityManager() = default;
    ~CompatibilityManager() = default;

    std::shared_ptr<LegacyAPIWrapper> api_wrapper_;
    std::shared_ptr<APICompatibilityValidator> validator_;
    std::shared_ptr<APIMigrationAssistant> migration_assistant_;
    std::shared_ptr<CompatibilityTestFramework> test_framework_;

    CompatibilityMode global_mode_{CompatibilityMode::HYBRID};
    bool warnings_enabled_{true};
    mutable std::mutex manager_mutex_;
};

} // namespace puzzle71::compatibility

// ============================================================================
// Legacy API Functions (C-style compatibility wrappers)
// ============================================================================

extern "C" {

/**
 * @brief Legacy initialization function
 * @param config Legacy configuration structure
 * @return 0 on success, non-zero on error
 */
int puzzle71_legacy_initialize(const puzzle71::compatibility::LegacyConfig* config);

/**
 * @brief Legacy kernel launch function
 * @param params Legacy kernel parameters
 * @return 0 on success, non-zero on error
 */
int puzzle71_legacy_launch_kernel(const puzzle71::compatibility::LegacyKernelParams* params);

/**
 * @brief Legacy results retrieval function
 * @param result Output buffer for results
 * @return 0 on success, non-zero on error
 */
int puzzle71_legacy_get_results(puzzle71::compatibility::LegacyResult* result);

/**
 * @brief Legacy cleanup function
 * @return 0 on success, non-zero on error
 */
int puzzle71_legacy_shutdown();

/**
 * @brief Legacy configuration function
 * @param key Configuration key
 * @param value Configuration value
 * @return 0 on success, non-zero on error
 */
int puzzle71_legacy_set_config(const char* key, const char* value);

/**
 * @brief Legacy configuration retrieval function
 * @param key Configuration key
 * @param value Output buffer for value
 * @param max_size Maximum size of output buffer
 * @return 0 on success, non-zero on error
 */
int puzzle71_legacy_get_config(const char* key, char* value, int max_size);

} // extern "C"

// ============================================================================
// Macros for compatibility checking
// ============================================================================

#define PUZZLE71_COMPATIBILITY_VERSION_MAJOR 1
#define PUZZLE71_COMPATIBILITY_VERSION_MINOR 0
#define PUZZLE71_COMPATIBILITY_VERSION_PATCH 0

/**
 * @brief Check if function is supported in current compatibility mode
 */
#define PUZZLE71_IS_FUNCTION_SUPPORTED(func_name) \
    puzzle71::compatibility::CompatibilityManager::getInstance().getAPIWrapper()->isFunctionSupported(func_name)

/**
 * @brief Emit compatibility warning for deprecated function
 */
#define PUZZLE71_COMPATIBILITY_WARNING(func_name, message) \
    do { \
        if (puzzle71::compatibility::CompatibilityManager::getInstance().areWarningsEnabled()) { \
            /* Log warning */ \
        } \
    } while(0)

/**
 * @brief Convert legacy config to modern config
 */
#define PUZZLE71_CONVERT_CONFIG(legacy_config) \
    (legacy_config).toModernConfig()

/**
 * @brief Convert modern result to legacy result
 */
#define PUZZLE71_CONVERT_RESULT(modern_result) \
    puzzle71::compatibility::LegacyResult::fromModernMetrics(modern_result)
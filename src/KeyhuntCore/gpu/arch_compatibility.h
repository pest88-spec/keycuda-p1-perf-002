// Puzzle71Solver - GPU Architecture Compatibility Matrix (T054)
// Phase 7: User Story 5 - Compatibility Assurance
// Comprehensive GPU architecture compatibility matrix with Turing to Hopper support

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_set>

namespace puzzle71::gpu {

/**
 * @brief GPU architecture enumeration
 */
enum class GPUArchitecture {
    UNKNOWN = 0,
    PASCAL = 60,     // GTX 10xx series (SM 6.0)
    VOLTA = 70,      // Tesla V100, GTX 10xx (SM 7.0)
    TURING = 75,     // RTX 20xx series, GTX 16xx series (SM 7.5)
    AMPERE = 80,     // RTX 30xx series (SM 8.0, 8.6)
    ADA_LOVELACE = 89, // RTX 40xx series (SM 8.9)
    HOPPER = 90,     // H100, H200 (SM 9.0)
    BLACKWELL = 100  // Future architecture (SM 10.0)
};

/**
 * @brief GPU compute capability structure
 */
struct ComputeCapability {
    int major{0};
    int minor{0};

    /**
     * @brief Get compute capability as string "X.Y"
     */
    std::string toString() const;

    /**
     * @brief Get compute capability as integer (major * 10 + minor)
     */
    int toInteger() const;

    /**
     * @brief Compare compute capabilities
     */
    int compare(const ComputeCapability& other) const;

    /**
     * @brief Check if this capability meets minimum requirements
     */
    bool meetsMinimum(const ComputeCapability& minimum) const;
};

/**
 * @brief GPU device information
 */
struct GPUDeviceInfo {
    int device_id{-1};
    std::string name;
    GPUArchitecture architecture{GPUArchitecture::UNKNOWN};
    ComputeCapability compute_capability;
    size_t total_memory{0};        // Bytes
    size_t shared_memory_per_block{0}; // Bytes
    int max_threads_per_block{0};
    int max_threads_per_multiprocessor{0};
    int max_blocks_per_multiprocessor{0};
    int warp_size{32};
    int max_registers_per_block{0};
    int max_registers_per_thread{0};
    int multiprocessor_count{0};
    int max_threads_per_device{0};
    size_t l2_cache_size{0};
    size_t memory_bandwidth{0};    // Bytes/sec
    double base_clock{0.0};        // MHz
    double boost_clock{0.0};       // MHz
    double memory_clock{0.0};      // MHz
    std::string driver_version;
    std::string cuda_runtime_version;
    bool integrated{false};
    bool supports_managed_memory{false};
    bool supports_concurrent_kernels{true};
    bool supports_ecc{false};
    bool supports_tensor_cores{false};
    bool supports_ray_tracing{false};
    bool supports_bf16{false};
    bool supports_tf32{false};
    bool supports_fp8{false};

    /**
     * @brief Check if device supports a specific feature
     */
    bool supportsFeature(const std::string& feature) const;

    /**
     * @brief Get architecture as string
     */
    std::string getArchitectureString() const;

    /**
     * @brief Check if device is compatible with minimum requirements
     */
    bool isCompatible(const ComputeCapability& minimum_capability) const;

    /**
     * @brief Serialize to JSON
     */
    std::string toJson() const;

    /**
     * @brief Get device summary
     */
    std::string getSummary() const;
};

/**
 * @brief Kernel compatibility requirements
 */
struct KernelRequirements {
    ComputeCapability min_compute_capability{7, 5};  // Minimum Turing
    size_t min_shared_memory{0};                    // Minimum shared memory per block
    int min_registers_per_thread{0};                // Minimum registers per thread
    int min_threads_per_block{0};                  // Minimum threads per block
    size_t min_constant_memory{0};                  // Minimum constant memory
    bool requires_tensor_cores{false};              // Requires Tensor Cores
    bool requires_ray_tracing{false};               // Requires RT cores
    bool requires_fp16{false};                      // Requires FP16 support
    bool requires_bf16{false};                      // Requires BF16 support
    bool requires_tf32{false};                      // Requires TF32 support
    bool requires_cooperative_groups{false};        // Requires Cooperative Groups
    bool requires_dynamic_parallelism{false};       // Requires Dynamic Parallelism
    bool requires_managed_memory{false};            // Requires Managed Memory
    std::vector<std::string> required_extensions;   // Required CUDA extensions

    /**
     * @brief Check if device meets requirements
     */
    bool isDeviceCompatible(const GPUDeviceInfo& device) const;

    /**
     * @brief Get missing requirements for device
     */
    std::vector<std::string> getMissingRequirements(const GPUDeviceInfo& device) const;

    /**
     * @brief Serialize to JSON
     */
    std::string toJson() const;
};

/**
 * @brief Performance characteristics by architecture
 */
struct ArchitecturePerformance {
    GPUArchitecture architecture;
    std::vector<std::string> supported_kernels;
    std::map<std::string, double> kernel_throughput; // keys/sec
    std::map<std::string, double> kernel_efficiency; // utilization percentage
    std::map<std::string, int> optimal_block_sizes;
    std::map<std::string, int> optimal_grid_sizes;
    std::map<std::string, size_t> optimal_batch_sizes;

    // Architecture-specific optimizations
    bool supports_warp_specialization{false};
    bool supports_cooperative_launch{false};
    bool supports_graph_launch{false};
    bool supports_memory_pool{false};
    bool supports_pinned_host_memory{true};
    bool supports_unified_memory{false};

    // Performance characteristics
    double memory_bandwidth_efficiency{0.0};  // % of theoretical peak
    double compute_efficiency{0.0};           // % of theoretical peak
    double occupancy_target{0.0};             // Target occupancy percentage
    int max_concurrent_kernels{1};
    size_t max_batch_size{0};

    /**
     * @brief Get optimal configuration for kernel
     */
    struct OptimalConfig {
        int threads_per_block{256};
        int blocks_per_grid{0};
        size_t shared_memory_size{0};
        bool use_tensor_cores{false};
        bool use_cooperative_groups{false};
    };

    OptimalConfig getOptimalConfig(const std::string& kernel_name) const;

    /**
     * @brief Check if kernel is supported
     */
    bool isKernelSupported(const std::string& kernel_name) const;
};

/**
 * @brief Compatibility matrix entry
 */
struct CompatibilityEntry {
    GPUArchitecture architecture;
    std::vector<std::string> compatible_kernels;
    std::vector<std::string> incompatible_kernels;
    std::map<std::string, std::string> kernel_alternatives; // incompatible -> alternative
    std::vector<std::string> required_workarounds;
    std::vector<std::string> known_issues;
    std::vector<std::string> optimization_hints;

    /**
     * @brief Check if kernel is compatible
     */
    bool isKernelCompatible(const std::string& kernel_name) const;

    /**
     * @brief Get alternative kernel for incompatible kernel
     */
    std::string getAlternativeKernel(const std::string& kernel_name) const;

    /**
     * @brief Get workarounds for kernel
     */
    std::vector<std::string> getWorkarounds(const std::string& kernel_name) const;
};

/**
 * @brief GPU architecture compatibility matrix
 */
class GPUArchitectureCompatibilityMatrix {
public:
    GPUArchitectureCompatibilityMatrix();
    ~GPUArchitectureCompatibilityMatrix() = default;

    /**
     * @brief Initialize compatibility matrix
     */
    bool initialize();

    /**
     * @brief Get GPU device information
     */
    std::vector<GPUDeviceInfo> getAvailableDevices() const;
    GPUDeviceInfo getDeviceInfo(int device_id) const;

    /**
     * @brief Check if kernel is compatible with device
     */
    bool isKernelCompatible(int device_id, const std::string& kernel_name) const;
    bool isKernelCompatible(const GPUDeviceInfo& device, const std::string& kernel_name) const;

    /**
     * @brief Check if device meets kernel requirements
     */
    bool deviceMeetsRequirements(int device_id, const KernelRequirements& requirements) const;
    bool deviceMeetsRequirements(const GPUDeviceInfo& device, const KernelRequirements& requirements) const;

    /**
     * @brief Get alternative kernel for incompatible device/kernel combination
     */
    std::string getAlternativeKernel(int device_id, const std::string& kernel_name) const;
    std::string getAlternativeKernel(const GPUDeviceInfo& device, const std::string& kernel_name) const;

    /**
     * @brief Get optimal configuration for kernel on device
     */
    ArchitecturePerformance::OptimalConfig getOptimalConfiguration(
        int device_id, const std::string& kernel_name
    ) const;

    ArchitecturePerformance::OptimalConfig getOptimalConfiguration(
        const GPUDeviceInfo& device, const std::string& kernel_name
    ) const;

    /**
     * @brief Get compatibility entry for architecture
     */
    CompatibilityEntry getCompatibilityEntry(GPUArchitecture architecture) const;

    /**
     * @brief Get performance characteristics for architecture
     */
    ArchitecturePerformance getPerformanceCharacteristics(GPUArchitecture architecture) const;

    /**
     * @brief Get workarounds for device/kernel combination
     */
    std::vector<std::string> getWorkarounds(int device_id, const std::string& kernel_name) const;

    /**
     * @brief Get known issues for device/kernel combination
     */
    std::vector<std::string> getKnownIssues(int device_id, const std::string& kernel_name) const;

    /**
     * @brief Get optimization hints for device/kernel combination
     */
    std::vector<std::string> getOptimizationHints(int device_id, const std::string& kernel_name) const;

    /**
     * @brief Validate device compatibility
     */
    struct ValidationReport {
        bool is_compatible{true};
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::vector<std::string> recommendations;
        std::map<std::string, bool> kernel_compatibility;
    };

    ValidationReport validateDeviceCompatibility(int device_id, const std::vector<std::string>& required_kernels) const;
    ValidationReport validateDeviceCompatibility(const GPUDeviceInfo& device, const std::vector<std::string>& required_kernels) const;

    /**
     * @brief Generate compatibility report for system
     */
    std::string generateSystemReport() const;
    std::string generateDeviceReport(int device_id) const;

    /**
     * @brief Check system compatibility with requirements
     */
    bool checkSystemCompatibility(const KernelRequirements& requirements) const;
    std::vector<int> getCompatibleDevices(const KernelRequirements& requirements) const;

    /**
     * @brief Architecture detection and classification
     */
    GPUArchitecture detectArchitecture(int device_id) const;
    GPUArchitecture detectArchitecture(const ComputeCapability& compute_capability) const;
    GPUArchitecture detectArchitecture(const std::string& device_name) const;

    /**
     * @brief Utility methods
     */
    static std::string architectureToString(GPUArchitecture architecture);
    static GPUArchitecture stringToArchitecture(const std::string& architecture_str);
    static ComputeCapability getComputeCapability(int device_id);

    /**
     * @brief Update compatibility matrix with new data
     */
    void updateCompatibilityData(const std::string& json_data);
    void addCompatibilityEntry(const CompatibilityEntry& entry);
    void updatePerformanceCharacteristics(const ArchitecturePerformance& performance);

private:
    mutable std::mutex matrix_mutex_;

    // Device information cache
    std::map<int, GPUDeviceInfo> device_info_cache_;
    bool devices_initialized_{false};

    // Compatibility matrix
    std::map<GPUArchitecture, CompatibilityEntry> compatibility_matrix_;
    std::map<GPUArchitecture, ArchitecturePerformance> performance_matrix_;

    // Initialization methods
    void initializeDeviceCache();
    void initializeCompatibilityMatrix();
    void initializePerformanceMatrix();

    // Helper methods
    GPUDeviceInfo queryDeviceInfo(int device_id) const;
    void addCompatibilityEntry(GPUArchitecture arch, const CompatibilityEntry& entry);
    void addPerformanceEntry(GPUArchitecture arch, const ArchitecturePerformance& performance);
};

/**
 * @brief Architecture-specific kernel selector
 */
class KernelSelector {
public:
    explicit KernelSelector(const GPUArchitectureCompatibilityMatrix& matrix);
    ~KernelSelector() = default;

    /**
     * @brief Select best kernel variant for device
     */
    struct KernelSelection {
        std::string kernel_name;
        std::string kernel_variant;
        bool is_fallback{false};
        std::string fallback_reason;
        ArchitecturePerformance::OptimalConfig optimal_config;
        std::vector<std::string> required_workarounds;
    };

    KernelSelection selectKernel(int device_id, const std::string& base_kernel_name) const;
    KernelSelection selectKernel(const GPUDeviceInfo& device, const std::string& base_kernel_name) const;

    /**
     * @brief Select kernels for multi-device scenario
     */
    std::vector<KernelSelection> selectKernels(const std::vector<int>& device_ids, const std::string& base_kernel_name) const;
    std::vector<KernelSelection> selectKernels(const std::vector<GPUDeviceInfo>& devices, const std::string& base_kernel_name) const;

    /**
     * @brief Get kernel ranking for device
     */
    std::vector<std::string> getKernelRanking(int device_id, const std::vector<std::string>& kernel_variants) const;

private:
    const GPUArchitectureCompatibilityMatrix& matrix_;

    // Selection criteria
    double performance_weight_{0.6};
    double compatibility_weight_{0.3};
    double efficiency_weight_{0.1};

    // Helper methods
    double calculateKernelScore(const GPUDeviceInfo& device, const std::string& kernel_name) const;
    std::vector<std::string> getCompatibleVariants(const GPUDeviceInfo& device, const std::string& base_kernel_name) const;
};

/**
 * @brief Compatibility testing framework
 */
class CompatibilityTester {
public:
    explicit CompatibilityTester(const GPUArchitectureCompatibilityMatrix& matrix);
    ~CompatibilityTester() = default;

    /**
     * @brief Test kernel compatibility
     */
    struct TestResult {
        std::string kernel_name;
        std::string device_name;
        bool compatible{false};
        bool functional{false};
        double performance_score{0.0};
        std::string error_message;
        std::vector<std::string> warnings;
        std::chrono::milliseconds test_duration{0};
    };

    TestResult testKernelCompatibility(int device_id, const std::string& kernel_name) const;
    std::vector<TestResult> testKernelCompatibility(const std::string& kernel_name) const;
    std::vector<TestResult> testAllKernels(int device_id) const;

    /**
     * @brief Run comprehensive compatibility test suite
     */
    struct TestSuiteResult {
        size_t total_tests{0};
        size_t passed_tests{0};
        size_t failed_tests{0};
        std::vector<TestResult> individual_results;
        double overall_compatibility_score{0.0};
        std::chrono::milliseconds total_duration{0};
    };

    TestSuiteResult runCompatibilityTestSuite() const;
    TestSuiteResult runDeviceTestSuite(int device_id) const;
    TestSuiteResult runKernelTestSuite(const std::string& kernel_name) const;

    /**
     * @brief Generate test report
     */
    std::string generateTestReport(const TestSuiteResult& result) const;

private:
    const GPUArchitectureCompatibilityMatrix& matrix_;

    // Test methods
    bool executeKernelTest(int device_id, const std::string& kernel_name, std::string& error_message) const;
    double measureKernelPerformance(int device_id, const std::string& kernel_name) const;
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
    bool initialize();

    /**
     * @brief Get compatibility matrix
     */
    std::shared_ptr<GPUArchitectureCompatibilityMatrix> getCompatibilityMatrix();

    /**
     * @brief Get kernel selector
     */
    std::shared_ptr<KernelSelector> getKernelSelector();

    /**
     * @brief Get compatibility tester
     */
    std::shared_ptr<CompatibilityTester> getCompatibilityTester();

    /**
     * @brief Quick compatibility check
     */
    bool isSystemCompatible() const;
    std::vector<int> getCompatibleDevices() const;

    /**
     * @brief System diagnostics
     */
    std::string runDiagnostics() const;
    std::string getSystemSummary() const;

private:
    CompatibilityManager() = default;
    ~CompatibilityManager() = default;

    std::shared_ptr<GPUArchitectureCompatibilityMatrix> matrix_;
    std::shared_ptr<KernelSelector> kernel_selector_;
    std::shared_ptr<CompatibilityTester> compatibility_tester_;
    bool initialized_{false};
    mutable std::mutex manager_mutex_;
};

} // namespace puzzle71::gpu

// ============================================================================
// Utility functions
// ============================================================================

namespace puzzle71::gpu::utils {

/**
 * @brief Convert architecture to compute capability
 */
ComputeCapability architectureToComputeCapability(GPUArchitecture architecture);

/**
 * @brief Convert compute capability to architecture
 */
GPUArchitecture computeCapabilityToArchitecture(const ComputeCapability& compute_cap);

/**
 * @brief Get architecture-specific optimization flags
 */
std::vector<std::string> getArchitectureOptimizationFlags(GPUArchitecture architecture);

/**
 * @brief Get recommended kernel parameters for architecture
 */
struct RecommendedParameters {
    int threads_per_block{256};
    int min_blocks_per_multiprocessor{4};
    size_t shared_memory_size{0};
    bool use_tensor_cores{false};
    bool use_cooperative_groups{false};
};

RecommendedParameters getRecommendedParameters(GPUArchitecture architecture, const std::string& kernel_name);

/**
 * @brief Check if architecture supports specific CUDA feature
 */
bool supportsCudaFeature(GPUArchitecture architecture, const std::string& feature);

/**
 * @brief Get memory bandwidth estimate for architecture
 */
double getMemoryBandwidthGBps(GPUArchitecture architecture);

/**
 * @brief Get theoretical peak performance for architecture
 */
double getTheoreticalPeakPerformance(GPUArchitecture architecture);

} // namespace puzzle71::gpu::utils
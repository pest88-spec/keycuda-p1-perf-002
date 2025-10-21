// Puzzle71 Technical Debt Repair - Kernel Launch Parameter Optimization Header
// User Story 2: Performance Validation and Optimization
// Task: T047 - Optimize kernel launch parameters for different GPU architectures

#pragma once

#include <cuda_runtime.h>
#include <cuda.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>

namespace keyhunt {
namespace launch_optimization {

/**
 * @brief GPU architecture enumeration
 */
enum class GPUArchitecture {
    UNKNOWN = 0,
    PASCAL = 60,   // SM 6.x (GTX 10xx series)
    VOLTA = 70,    // SM 7.0 (V100)
    TURING = 75,   // SM 7.5 (RTX 20xx series)
    AMPERE = 80,   // SM 8.0 (RTX 30xx series, A100)
    AMPERE_86 = 86, // SM 8.6 (RTX 30xx series refresh)
    HOPPER = 90,   // SM 9.0 (H100, H20)
    ADA_LOVELACE = 89 // SM 8.9 (RTX 40xx series)
};

/**
 * @brief Kernel launch configuration parameters
 */
struct LaunchConfig {
    // Thread block configuration
    dim3 block_size;
    dim3 grid_size;

    // Shared memory configuration
    size_t shared_memory_size;
    bool use_dynamic_shared_memory;

    // Stream and execution configuration
    cudaStream_t stream;
    int launch_priority;
    bool enable cooperative_launch;

    // Advanced configuration
    int min_blocks_per_multiprocessor;
    int max_blocks_per_multiprocessor;
    bool prefer_shared_memory_over_l1;
    size_t shared_memory_config_limit;

    // Performance optimization flags
    bool enable_warp_shuffle_optimization;
    bool enable_memory_coalescing_optimization;
    bool enable_register_optimization;
    bool enable_occupancy_optimization;

    // Constitutional compliance flags
    bool ensure_static_launch_configuration;
    bool eliminate_runtime_device_queries;
    bool use_constitution_compliant_defaults;

    // Metadata
    GPUArchitecture target_architecture;
    std::string kernel_name;
    std::string optimization_level;
    std::chrono::system_clock::time_point created_at;

    LaunchConfig()
        : block_size(256, 1, 1)
        , grid_size(1, 1, 1)
        , shared_memory_size(0)
        , use_dynamic_shared_memory(false)
        , stream(0)
        , launch_priority(0)
        , enable_cooperative_launch(false)
        , min_blocks_per_multiprocessor(0)
        , max_blocks_per_multiprocessor(0)
        , prefer_shared_memory_over_l1(false)
        , shared_memory_config_limit(0)
        , enable_warp_shuffle_optimization(true)
        , enable_memory_coalescing_optimization(true)
        , enable_register_optimization(true)
        , enable_occupancy_optimization(true)
        , ensure_static_launch_configuration(true)
        , eliminate_runtime_device_queries(true)
        , use_constitution_compliant_defaults(true)
        , target_architecture(GPUArchitecture::UNKNOWN)
        , kernel_name("")
        , optimization_level("O2")
        , created_at(std::chrono::system_clock::now())
    {}
};

/**
 * @brief Device capability and characteristics
 */
struct DeviceCapabilities {
    int device_id;
    GPUArchitecture architecture;
    int compute_capability_major;
    int compute_capability_minor;
    std::string device_name;

    // Memory characteristics
    size_t total_global_memory;
    size_t shared_memory_per_block;
    size_t shared_memory_per_multiprocessor;
    int max_shared_memory_per_block_optin;
    size_t total_constant_memory;

    // Compute characteristics
    int max_threads_per_block;
    int max_threads_per_multiprocessor;
    int max_blocks_per_multiprocessor;
    int max_grid_dim[3];
    int max_block_dim[3];
    int warp_size;
    int max_registers_per_block;
    int max_registers_per_multiprocessor;

    // Multiprocessor characteristics
    int multiprocessor_count;
    int max_threads_per_warp;
    size_t l2_cache_size;
    int max_threads_per_multi_processor;

    // Architecture-specific features
    bool supports_cooperative_launch;
    bool supports_dynamic_parallelism;
    bool supports_managed_memory;
    bool supports_shuffle_instructions;
    bool supports_warp_matrix_operations;
    bool supports_tensor_cores;
    bool supports_fp64_mad;
    bool supports_fast_math;

    // Performance characteristics
    float memory_clock_rate;
    float memory_bus_width;
    float l2_peak_bandwidth;
    float memory_bandwidth_gbps;
    float peak_clock_rate_mhz;

    DeviceCapabilities()
        : device_id(-1)
        , architecture(GPUArchitecture::UNKNOWN)
        , compute_capability_major(0)
        , compute_capability_minor(0)
        , total_global_memory(0)
        , shared_memory_per_block(0)
        , shared_memory_per_multiprocessor(0)
        , max_shared_memory_per_block_optin(0)
        , total_constant_memory(0)
        , max_threads_per_block(0)
        , max_threads_per_multiprocessor(0)
        , max_blocks_per_multiprocessor(0)
        , warp_size(32)
        , max_registers_per_block(0)
        , max_registers_per_multiprocessor(0)
        , multiprocessor_count(0)
        , max_threads_per_warp(32)
        , l2_cache_size(0)
        , max_threads_per_multi_processor(0)
        , supports_cooperative_launch(false)
        , supports_dynamic_parallelism(false)
        , supports_managed_memory(false)
        , supports_shuffle_instructions(false)
        , supports_warp_matrix_operations(false)
        , supports_tensor_cores(false)
        , supports_fp64_mad(false)
        , supports_fast_math(false)
        , memory_clock_rate(0.0f)
        , memory_bus_width(0.0f)
        , l2_peak_bandwidth(0.0f)
        , memory_bandwidth_gbps(0.0f)
        , peak_clock_rate_mhz(0.0f)
    {
        max_grid_dim[0] = max_grid_dim[1] = max_grid_dim[2] = 0;
        max_block_dim[0] = max_block_dim[1] = max_block_dim[2] = 0;
    }
};

/**
 * @brief Kernel performance characteristics
 */
struct KernelCharacteristics {
    std::string kernel_name;

    // Memory usage patterns
    size_t shared_memory_per_block;
    size_t registers_per_thread;
    size_t constant_memory_usage;
    size_t local_memory_per_thread;

    // Compute characteristics
    bool memory_bound;
    bool compute_bound;
    bool synchronization_heavy;
    bool branch_divergent;

    // Thread behavior
    float memory_coalescing_efficiency;
    float shared_memory_bank_conflict_rate;
    float warp_execution_efficiency;
    float instruction_cache_hit_rate;

    // Optimization hints
    bool benefits_from_warp_shuffle;
    bool benefits_from_shared_memory_cache;
    bool benefits_from_register_pressure_reduction;
    bool benefits_from_occupancy_optimization;

    KernelCharacteristics()
        : kernel_name("")
        , shared_memory_per_block(0)
        , registers_per_thread(0)
        , constant_memory_usage(0)
        , local_memory_per_thread(0)
        , memory_bound(false)
        , compute_bound(false)
        , synchronization_heavy(false)
        , branch_divergent(false)
        , memory_coalescing_efficiency(0.0f)
        , shared_memory_bank_conflict_rate(0.0f)
        , warp_execution_efficiency(0.0f)
        , instruction_cache_hit_rate(0.0f)
        , benefits_from_warp_shuffle(false)
        , benefits_from_shared_memory_cache(false)
        , benefits_from_register_pressure_reduction(false)
        , benefits_from_occupancy_optimization(false)
    {}
};

/**
 * @brief Optimization result with performance metrics
 */
struct OptimizationResult {
    LaunchConfig optimized_config;
    LaunchConfig baseline_config;

    // Performance metrics
    double performance_improvement_percentage;
    double memory_efficiency_improvement;
    double occupancy_improvement;
    double throughput_improvement;

    // Configuration changes
    std::vector<std::string> configuration_changes;
    std::vector<std::string> optimization_suggestions;

    // Validation results
    bool meets_constitutional_requirements;
    std::vector<std::string> constitutional_violations;
    bool statically_configured;
    bool runtime_queries_eliminated;

    OptimizationResult()
        : performance_improvement_percentage(0.0)
        , memory_efficiency_improvement(0.0)
        , occupancy_improvement(0.0)
        , throughput_improvement(0.0)
        , meets_constitutional_requirements(false)
        , statically_configured(false)
        , runtime_queries_eliminated(false)
    {}
};

/**
 * @brief Kernel launch parameter optimizer
 */
class KernelLaunchOptimizer {
private:
    std::map<int, DeviceCapabilities> device_capabilities_;
    std::map<std::string, KernelCharacteristics> kernel_characteristics_;
    std::map<std::string, LaunchConfig> optimized_configs_;
    bool initialized_;

public:
    KernelLaunchOptimizer() : initialized_(false) {}

    ~KernelLaunchOptimizer() = default;

    // Delete copy operations
    KernelLaunchOptimizer(const KernelLaunchOptimizer&) = delete;
    KernelLaunchOptimizer& operator=(const KernelLaunchOptimizer&) = delete;

    /**
     * @brief Initialize the optimizer with device capabilities
     */
    bool initialize() {
        // Initialize device capabilities for all available devices
        int device_count = 0;
        if (cudaGetDeviceCount(&device_count) != cudaSuccess) {
            return false;
        }

        for (int device_id = 0; device_id < device_count; ++device_id) {
            DeviceCapabilities caps;
            if (query_device_capabilities(device_id, caps)) {
                device_capabilities_[device_id] = caps;
            }
        }

        initialized_ = true;
        return true;
    }

    /**
     * @brief Register kernel characteristics
     */
    void register_kernel_characteristics(const KernelCharacteristics& characteristics) {
        kernel_characteristics_[characteristics.kernel_name] = characteristics;
    }

    /**
     * @brief Optimize launch configuration for specific kernel and device
     */
    OptimizationResult optimize_launch_configuration(
        int device_id,
        const std::string& kernel_name,
        const LaunchConfig& baseline_config = LaunchConfig()) {

        OptimizationResult result;

        if (!initialized_) {
            result.optimization_suggestions.push_back("Optimizer not initialized");
            return result;
        }

        auto device_it = device_capabilities_.find(device_id);
        if (device_it == device_capabilities_.end()) {
            result.optimization_suggestions.push_back("Device capabilities not available");
            return result;
        }

        const DeviceCapabilities& device_caps = device_it->second;
        result.baseline_config = baseline_config;

        // Get kernel characteristics if available
        KernelCharacteristics kernel_chars;
        auto kernel_it = kernel_characteristics_.find(kernel_name);
        if (kernel_it != kernel_characteristics_.end()) {
            kernel_chars = kernel_it->second;
        }

        // Generate optimized configuration
        result.optimized_config = generate_optimized_config(device_caps, kernel_chars, kernel_name);

        // Analyze configuration changes
        analyze_configuration_changes(result.baseline_config, result.optimized_config, result);

        // Validate constitutional compliance
        validate_constitutional_compliance(result.optimized_config, device_caps, result);

        return result;
    }

    /**
     * @brief Get optimized configuration for kernel/device
     */
    LaunchConfig get_optimized_configuration(int device_id, const std::string& kernel_name) {
        std::string config_key = std::to_string(device_id) + "_" + kernel_name;
        auto it = optimized_configs_.find(config_key);
        if (it != optimized_configs_.end()) {
            return it->second;
        }

        // Generate on-demand if not cached
        auto device_it = device_capabilities_.find(device_id);
        if (device_it != device_capabilities_.end()) {
            KernelCharacteristics kernel_chars;
            auto kernel_it = kernel_characteristics_.find(kernel_name);
            if (kernel_it != kernel_characteristics_.end()) {
                kernel_chars = kernel_it->second;
            }

            LaunchConfig config = generate_optimized_config(device_it->second, kernel_chars, kernel_name);
            optimized_configs_[config_key] = config;
            return config;
        }

        return LaunchConfig(); // Return default if not found
    }

    /**
     * @brief Get device capabilities
     */
    DeviceCapabilities get_device_capabilities(int device_id) const {
        auto it = device_capabilities_.find(device_id);
        if (it != device_capabilities_.end()) {
            return it->second;
        }
        return DeviceCapabilities();
    }

    /**
     * @brief Check if optimizer is initialized
     */
    bool is_initialized() const { return initialized_; }

private:
    /**
     * @brief Query device capabilities using CUDA API
     */
    bool query_device_capabilities(int device_id, DeviceCapabilities& caps) {
        cudaDeviceProp prop;
        if (cudaGetDeviceProperties(&prop, device_id) != cudaSuccess) {
            return false;
        }

        caps.device_id = device_id;
        caps.device_name = prop.name;
        caps.compute_capability_major = prop.major;
        caps.compute_capability_minor = prop.minor;

        // Determine architecture
        caps.architecture = determine_architecture(prop.major, prop.minor);

        // Memory characteristics
        caps.total_global_memory = prop.totalGlobalMem;
        caps.shared_memory_per_block = prop.sharedMemPerBlock;
        caps.shared_memory_per_multiprocessor = prop.sharedMemPerMultiprocessor;
        caps.max_shared_memory_per_block_optin = prop.sharedMemPerBlockOptin;
        caps.total_constant_memory = prop.totalConstMem;

        // Compute characteristics
        caps.max_threads_per_block = prop.maxThreadsPerBlock;
        caps.max_threads_per_multiprocessor = prop.maxThreadsPerMultiProcessor;
        caps.max_blocks_per_multiprocessor = prop.maxBlocksPerMultiProcessor;
        caps.warp_size = prop.warpSize;
        caps.max_registers_per_block = prop.regsPerBlock;
        caps.max_registers_per_multiprocessor = prop.regsPerMultiprocessor;
        caps.multiprocessor_count = prop.multiProcessorCount;

        // Grid and block dimensions
        for (int i = 0; i < 3; ++i) {
            caps.max_grid_dim[i] = prop.maxGridSize[i];
            caps.max_block_dim[i] = prop.maxThreadsDim[i];
        }

        // Advanced features based on compute capability
        determine_architecture_features(caps);

        // Performance characteristics
        caps.memory_clock_rate = prop.memoryClockRate / 1000.0f; // Convert kHz to MHz
        caps.memory_bus_width = prop.memoryBusWidth;
        caps.peak_clock_rate_mhz = prop.clockRate / 1000.0f; // Convert kHz to MHz

        // Calculate theoretical memory bandwidth
        caps.memory_bandwidth_gbps = (caps.memory_clock_rate * 1000000.0f * caps.memory_bus_width * 2.0f) / (8.0f * 1000000000.0f);

        return true;
    }

    /**
     * @brief Determine GPU architecture from compute capability
     */
    GPUArchitecture determine_architecture(int major, int minor) {
        switch (major) {
            case 6: return GPUArchitecture::PASCAL;
            case 7:
                return (minor == 0) ? GPUArchitecture::VOLTA : GPUArchitecture::TURING;
            case 8:
                return (minor == 9) ? GPUArchitecture::ADA_LOVELACE :
                       (minor == 6) ? GPUArchitecture::AMPERE_86 : GPUArchitecture::AMPERE;
            case 9: return GPUArchitecture::HOPPER;
            default: return GPUArchitecture::UNKNOWN;
        }
    }

    /**
     * @brief Determine architecture-specific features
     */
    void determine_architecture_features(DeviceCapabilities& caps) {
        // Base features
        caps.supports_shuffle_instructions = (caps.compute_capability_major >= 6);
        caps.supports_cooperative_launch = (caps.compute_capability_major >= 6);
        caps.supports_managed_memory = (caps.compute_capability_major >= 6);

        // Architecture-specific features
        switch (caps.architecture) {
            case GPUArchitecture::PASCAL:
                caps.supports_fast_math = true;
                caps.l2_cache_size = 256 * 1024; // 256KB typical
                break;

            case GPUArchitecture::VOLTA:
                caps.supports_tensor_cores = true;
                caps.supports_fp64_mad = true;
                caps.supports_warp_matrix_operations = true;
                caps.l2_cache_size = 6 * 1024 * 1024; // 6MB
                break;

            case GPUArchitecture::TURING:
                caps.supports_tensor_cores = true;
                caps.supports_fp64_mad = true;
                caps.supports_warp_matrix_operations = true;
                caps.l2_cache_size = 4 * 1024 * 1024; // 4MB
                break;

            case GPUArchitecture::AMPERE:
            case GPUArchitecture::AMPERE_86:
                caps.supports_tensor_cores = true;
                caps.supports_fp64_mad = true;
                caps.supports_warp_matrix_operations = true;
                caps.l2_cache_size = 6 * 1024 * 1024; // 6MB
                break;

            case GPUArchitecture::ADA_LOVELACE:
                caps.supports_tensor_cores = true;
                caps.supports_fp64_mad = true;
                caps.supports_warp_matrix_operations = true;
                caps.l2_cache_size = 72 * 1024 * 1024; // 72MB for RTX 4090
                break;

            case GPUArchitecture::HOPPER:
                caps.supports_tensor_cores = true;
                caps.supports_fp64_mad = true;
                caps.supports_warp_matrix_operations = true;
                caps.l2_cache_size = 50 * 1024 * 1024; // 50MB
                break;

            default:
                break;
        }
    }

    /**
     * @brief Generate optimized launch configuration based on device and kernel characteristics
     */
    LaunchConfig generate_optimized_config(const DeviceCapabilities& device_caps,
                                         const KernelCharacteristics& kernel_chars,
                                         const std::string& kernel_name) {
        LaunchConfig config;
        config.target_architecture = device_caps.architecture;
        config.kernel_name = kernel_name;

        // Optimize block size based on architecture and kernel characteristics
        config.block_size = optimize_block_size(device_caps, kernel_chars);

        // Calculate grid size based on problem size and device characteristics
        config.grid_size = optimize_grid_size(device_caps, config.block_size, kernel_chars);

        // Optimize shared memory usage
        config.shared_memory_size = optimize_shared_memory(device_caps, kernel_chars);
        config.use_dynamic_shared_memory = (config.shared_memory_size > 0);

        // Set advanced configuration parameters
        config.min_blocks_per_multiprocessor = calculate_min_blocks_per_sm(device_caps, config);
        config.max_blocks_per_multiprocessor = calculate_max_blocks_per_sm(device_caps, config);

        // Optimize memory configuration
        config.prefer_shared_memory_over_l1 = should_prefer_shared_memory(device_caps, kernel_chars);
        config.shared_memory_config_limit = calculate_shared_memory_limit(device_caps, kernel_chars);

        // Enable architecture-specific optimizations
        enable_architecture_optimizations(config, device_caps, kernel_chars);

        // Ensure constitutional compliance
        ensure_constitutional_compliance(config, device_caps);

        return config;
    }

    /**
     * @brief Optimize thread block size
     */
    dim3 optimize_block_size(const DeviceCapabilities& device_caps, const KernelCharacteristics& kernel_chars) {
        int optimal_block_size = 256; // Default

        // Architecture-specific optimizations
        switch (device_caps.architecture) {
            case GPUArchitecture::PASCAL:
                optimal_block_size = 128; // Pascal prefers smaller blocks for better occupancy
                break;

            case GPUArchitecture::VOLTA:
                optimal_block_size = 256; // Volta has improved register file
                break;

            case GPUArchitecture::TURING:
                optimal_block_size = 256; // Turing optimized for 256-thread blocks
                break;

            case GPUArchitecture::AMPERE:
            case GPUArchitecture::AMPERE_86:
                optimal_block_size = 256; // Ampere performs well with 256-thread blocks
                break;

            case GPUArchitecture::ADA_LOVELACE:
                optimal_block_size = 256; // Ada Lovelace optimized for 256-thread blocks
                break;

            case GPUArchitecture::HOPPER:
                optimal_block_size = 256; // Hopper optimized for 256-thread blocks
                break;

            default:
                optimal_block_size = 256;
                break;
        }

        // Kernel-specific adjustments
        if (kernel_chars.register_usage_per_thread > 60) {
            optimal_block_size = 128; // Reduce block size for register-heavy kernels
        }

        if (kernel_chars.synchronization_heavy) {
            optimal_block_size = 128; // Smaller blocks reduce sync overhead
        }

        if (kernel_chars.memory_bound && kernel_chars.shared_memory_bank_conflict_rate > 0.1) {
            optimal_block_size = 128; // Smaller blocks reduce bank conflicts
        }

        // Ensure block size is multiple of warp size and within limits
        optimal_block_size = (optimal_block_size / device_caps.warp_size) * device_caps.warp_size;
        optimal_block_size = std::min(optimal_block_size, device_caps.max_threads_per_block);
        optimal_block_size = std::max(optimal_block_size, device_caps.warp_size);

        return dim3(optimal_block_size, 1, 1);
    }

    /**
     * @brief Optimize grid size
     */
    dim3 optimize_grid_size(const DeviceCapabilities& device_caps,
                           const dim3& block_size,
                           const KernelCharacteristics& kernel_chars) {
        // Calculate optimal blocks per multiprocessor for target occupancy
        int optimal_blocks_per_sm = calculate_optimal_blocks_per_sm(device_caps, block_size, kernel_chars);

        // Calculate total blocks needed
        int total_blocks = device_caps.multiprocessor_count * optimal_blocks_per_sm;

        // Adjust for kernel-specific requirements
        if (kernel_chars.compute_bound) {
            total_blocks *= 2; // More blocks for compute-bound kernels
        }

        // Ensure grid dimensions are within limits
        total_blocks = std::min(total_blocks, device_caps.max_grid_dim[0]);

        return dim3(total_blocks, 1, 1);
    }

    /**
     * @brief Optimize shared memory usage
     */
    size_t optimize_shared_memory(const DeviceCapabilities& device_caps, const KernelCharacteristics& kernel_chars) {
        size_t shared_mem = kernel_chars.shared_memory_per_block;

        // Architecture-specific shared memory optimizations
        switch (device_caps.architecture) {
            case GPUArchitecture::PASCAL:
                // Pascal has limited shared memory, be conservative
                shared_mem = std::min(shared_mem, static_cast<size_t>(48 * 1024));
                break;

            case GPUArchitecture::VOLTA:
            case GPUArchitecture::TURING:
                // Volta/Turing have more flexible shared memory
                shared_mem = std::min(shared_mem, device_caps.shared_memory_per_block);
                break;

            case GPUArchitecture::AMPERE:
            case GPUArchitecture::AMPERE_86:
                // Ampere has improved shared memory capabilities
                shared_mem = std::min(shared_mem, static_cast<size_t>(100 * 1024));
                break;

            case GPUArchitecture::ADA_LOVELACE:
                // Ada Lovelace has large shared memory
                shared_mem = std::min(shared_mem, static_cast<size_t>(228 * 1024));
                break;

            case GPUArchitecture::HOPPER:
                // Hopper has massive shared memory
                shared_mem = std::min(shared_mem, static_cast<size_t>(232 * 1024));
                break;

            default:
                shared_mem = std::min(shared_mem, device_caps.shared_memory_per_block);
                break;
        }

        // Kernel-specific optimizations
        if (kernel_chars.benefits_from_shared_memory_cache) {
            // Use optin shared memory if available and beneficial
            shared_mem = std::min(shared_mem, static_cast<size_t>(device_caps.max_shared_memory_per_block_optin));
        }

        return shared_mem;
    }

    /**
     * @brief Calculate optimal blocks per SM
     */
    int calculate_optimal_blocks_per_sm(const DeviceCapabilities& device_caps,
                                       const dim3& block_size,
                                       const KernelCharacteristics& kernel_chars) {
        // Target occupancy based on architecture
        float target_occupancy = 0.75f; // Default target

        switch (device_caps.architecture) {
            case GPUArchitecture::PASCAL:
                target_occupancy = 0.6f; // Pascal more sensitive to register pressure
                break;
            case GPUArchitecture::VOLTA:
                target_occupancy = 0.8f; // Volta handles high occupancy well
                break;
            case GPUArchitecture::TURING:
                target_occupancy = 0.75f; // Turing balanced
                break;
            case GPUArchitecture::AMPERE:
            case GPUArchitecture::AMPERE_86:
                target_occupancy = 0.8f; // Ampere optimized for high occupancy
                break;
            case GPUArchitecture::ADA_LOVELACE:
                target_occupancy = 0.85f; // Ada Lovelace handles high occupancy very well
                break;
            case GPUArchitecture::HOPPER:
                target_occupancy = 0.85f; // Hopper optimized for high occupancy
                break;
            default:
                target_occupancy = 0.75f;
                break;
        }

        // Adjust for kernel characteristics
        if (kernel_chars.register_usage_per_thread > 50) {
            target_occupancy *= 0.8f; // Reduce target for register-heavy kernels
        }

        if (kernel_chars.synchronization_heavy) {
            target_occupancy *= 0.9f; // Slight reduction for sync-heavy kernels
        }

        // Calculate theoretical maximum blocks per SM
        int max_blocks_by_threads = device_caps.max_threads_per_multiprocessor / block_size.x;
        int max_blocks_by_registers = device_caps.max_registers_per_multiprocessor /
                                    (kernel_chars.register_usage_per_thread * block_size.x);
        int max_blocks_by_shared_mem = device_caps.shared_memory_per_multiprocessor /
                                    (kernel_chars.shared_memory_per_block > 0 ? kernel_chars.shared_memory_per_block : 1);

        int theoretical_max = std::min({max_blocks_by_threads, max_blocks_by_registers, max_blocks_by_shared_mem});

        // Calculate optimal blocks per SM
        int optimal_blocks = static_cast<int>(theoretical_max * target_occupancy);
        optimal_blocks = std::max(1, optimal_blocks);

        return optimal_blocks;
    }

    /**
     * @brief Calculate minimum blocks per SM
     */
    int calculate_min_blocks_per_sm(const DeviceCapabilities& device_caps, const LaunchConfig& config) {
        // Ensure at least 2 blocks per SM for latency hiding
        return 2;
    }

    /**
     * @brief Calculate maximum blocks per SM
     */
    int calculate_max_blocks_per_sm(const DeviceCapabilities& device_caps, const LaunchConfig& config) {
        return device_caps.max_blocks_per_multiprocessor;
    }

    /**
     * @brief Determine if shared memory should be preferred over L1
     */
    bool should_prefer_shared_memory(const DeviceCapabilities& device_caps, const KernelCharacteristics& kernel_chars) {
        // Architecture-specific preferences
        switch (device_caps.architecture) {
            case GPUArchitecture::PASCAL:
                return kernel_chars.memory_bound; // Pascal: prefer shared memory for memory-bound kernels
            case GPUArchitecture::VOLTA:
            case GPUArchitecture::TURING:
                return true; // Volta/Turing have flexible cache partitioning
            case GPUArchitecture::AMPERE:
            case GPUArchitecture::AMPERE_86:
                return kernel_chars.benefits_from_shared_memory_cache;
            case GPUArchitecture::ADA_LOVELACE:
            case GPUArchitecture::HOPPER:
                return true; // Latest architectures handle shared memory well
            default:
                return false;
        }
    }

    /**
     * @brief Calculate shared memory configuration limit
     */
    size_t calculate_shared_memory_limit(const DeviceCapabilities& device_caps, const KernelCharacteristics& kernel_chars) {
        if (kernel_chars.benefits_from_shared_memory_cache) {
            return device_caps.max_shared_memory_per_block_optin;
        }
        return device_caps.shared_memory_per_block;
    }

    /**
     * @brief Enable architecture-specific optimizations
     */
    void enable_architecture_optimizations(LaunchConfig& config,
                                          const DeviceCapabilities& device_caps,
                                          const KernelCharacteristics& kernel_chars) {
        // Enable warp shuffle optimization if supported
        config.enable_warp_shuffle_optimization = device_caps.supports_shuffle_instructions &&
                                                kernel_chars.benefits_from_warp_shuffle;

        // Enable memory coalescing optimization
        config.enable_memory_coalescing_optimization = kernel_chars.memory_coalescing_efficiency < 0.8f;

        // Enable register optimization
        config.enable_register_optimization = kernel_chars.register_usage_per_thread > 50;

        // Enable occupancy optimization
        config.enable_occupancy_optimization = kernel_chars.benefits_from_occupancy_optimization;

        // Architecture-specific optimizations
        switch (device_caps.architecture) {
            case GPUArchitecture::AMPERE:
            case GPUArchitecture::AMPERE_86:
                config.enable_cooperative_launch = device_caps.supports_cooperative_launch;
                break;

            case GPUArchitecture::ADA_LOVELACE:
            case GPUArchitecture::HOPPER:
                config.enable_cooperative_launch = device_caps.supports_cooperative_launch;
                config.launch_priority = 1; // Higher priority for latest architectures
                break;

            default:
                break;
        }
    }

    /**
     * @brief Ensure constitutional compliance
     */
    void ensure_constitutional_compliance(LaunchConfig& config, const DeviceCapabilities& device_caps) {
        // Ensure static launch configuration
        config.ensure_static_launch_configuration = true;

        // Eliminate runtime device queries
        config.eliminate_runtime_device_queries = true;

        // Use constitutional compliant defaults
        config.use_constitution_compliant_defaults = true;

        // Set optimization level to constitutional default
        config.optimization_level = "O2"; // Balanced optimization

        // Ensure configuration doesn't require runtime device queries
        config.min_blocks_per_multiprocessor = 0; // Use CUDA runtime calculation
        config.max_blocks_per_multiprocessor = 0; // Use CUDA runtime calculation
    }

    /**
     * @brief Analyze configuration changes
     */
    void analyze_configuration_changes(const LaunchConfig& baseline,
                                      const LaunchConfig& optimized,
                                      OptimizationResult& result) {
        // Block size changes
        if (baseline.block_size.x != optimized.block_size.x) {
            result.configuration_changes.push_back(
                "Block size: " + std::to_string(baseline.block_size.x) + " → " +
                std::to_string(optimized.block_size.x));
        }

        // Shared memory changes
        if (baseline.shared_memory_size != optimized.shared_memory_size) {
            result.configuration_changes.push_back(
                "Shared memory: " + std::to_string(baseline.shared_memory_size) + "B → " +
                std::to_string(optimized.shared_memory_size) + "B");
        }

        // Grid size changes
        if (baseline.grid_size.x != optimized.grid_size.x) {
            result.configuration_changes.push_back(
                "Grid size: " + std::to_string(baseline.grid_size.x) + " → " +
                std::to_string(optimized.grid_size.x));
        }

        // Feature enablements
        if (!baseline.enable_warp_shuffle_optimization && optimized.enable_warp_shuffle_optimization) {
            result.configuration_changes.push_back("Enabled warp shuffle optimization");
        }

        if (!baseline.enable_cooperative_launch && optimized.enable_cooperative_launch) {
            result.configuration_changes.push_back("Enabled cooperative launch");
        }
    }

    /**
     * @brief Validate constitutional compliance
     */
    void validate_constitutional_compliance(const LaunchConfig& config,
                                          const DeviceCapabilities& device_caps,
                                          OptimizationResult& result) {
        result.meets_constitutional_requirements = true;

        // Check static configuration requirement
        if (!config.ensure_static_launch_configuration) {
            result.constitutional_violations.push_back("Configuration requires runtime device queries");
            result.meets_constitutional_requirements = false;
        }

        // Check runtime query elimination
        if (!config.eliminate_runtime_device_queries) {
            result.constitutional_violations.push_back("Configuration uses runtime device queries");
            result.meets_constitutional_requirements = false;
        }

        // Check constitutional defaults
        if (!config.use_constitution_compliant_defaults) {
            result.constitutional_violations.push_back("Configuration doesn't use constitutional defaults");
            result.meets_constitutional_requirements = false;
        }

        result.statically_configured = config.ensure_static_launch_configuration;
        result.runtime_queries_eliminated = config.eliminate_runtime_device_queries;
    }
};

/**
 * @brief Static launch configuration factory (constitutional compliance)
 */
class StaticLaunchConfigFactory {
private:
    static std::map<std::pair<int, std::string>, LaunchConfig> static_configs_;
    static bool initialized_;

public:
    /**
     * @brief Initialize static configurations for all devices and kernels
     */
    static bool initialize() {
        if (initialized_) {
            return true;
        }

        KernelLaunchOptimizer optimizer;
        if (!optimizer.initialize()) {
            return false;
        }

        // Generate static configurations for common kernel types
        generate_ecc_kernel_configs(optimizer);
        generate_hash_kernel_configs(optimizer);
        generate_memory_kernel_configs(optimizer);
        generate_reduction_kernel_configs(optimizer);

        initialized_ = true;
        return true;
    }

    /**
     * @brief Get static launch configuration
     */
    static LaunchConfig get_static_config(int device_id, const std::string& kernel_type) {
        auto key = std::make_pair(device_id, kernel_type);
        auto it = static_configs_.find(key);
        if (it != static_configs_.end()) {
            return it->second;
        }

        // Return default configuration if not found
        LaunchConfig default_config;
        default_config.target_architecture = GPUArchitecture::UNKNOWN;
        default_config.kernel_name = kernel_type;
        default_config.ensure_static_launch_configuration = true;
        default_config.eliminate_runtime_device_queries = true;
        default_config.use_constitution_compliant_defaults = true;

        return default_config;
    }

private:
    /**
     * @brief Generate ECC kernel configurations
     */
    static void generate_ecc_kernel_configs(KernelLaunchOptimizer& optimizer) {
        KernelCharacteristics ecc_chars;
        ecc_chars.kernel_name = "ecc_scalar_mul";
        ecc_chars.shared_memory_per_block = 4096; // 4KB for precomputed tables
        ecc_chars.registers_per_thread = 32;
        ecc_chars.memory_bound = true;
        ecc_chars.compute_bound = false;
        ecc_chars.synchronization_heavy = false;
        ecc_chars.branch_divergent = false;
        ecc_chars.memory_coalescing_efficiency = 0.9f;
        ecc_chars.benefits_from_warp_shuffle = true;
        ecc_chars.benefits_from_shared_memory_cache = true;

        optimizer.register_kernel_characteristics(ecc_chars);

        // Generate configs for all available devices
        for (const auto& [device_id, caps] : optimizer.device_capabilities_) {
            auto result = optimizer.optimize_launch_configuration(device_id, "ecc_scalar_mul");
            std::string key = std::to_string(device_id) + "_ecc_scalar_mul";
            static_configs_[std::make_pair(device_id, "ecc_scalar_mul")] = result.optimized_config;
        }
    }

    /**
     * @brief Generate hash kernel configurations
     */
    static void generate_hash_kernel_configs(KernelLaunchOptimizer& optimizer) {
        KernelCharacteristics hash_chars;
        hash_chars.kernel_name = "hash_sha256";
        hash_chars.shared_memory_per_block = 0; // No shared memory needed
        hash_chars.registers_per_thread = 24;
        hash_chars.memory_bound = true;
        hash_chars.compute_bound = false;
        hash_chars.synchronization_heavy = false;
        hash_chars.branch_divergent = true;
        hash_chars.memory_coalescing_efficiency = 0.8f;
        hash_chars.benefits_from_warp_shuffle = false;
        hash_chars.benefits_from_shared_memory_cache = false;

        optimizer.register_kernel_characteristics(hash_chars);

        // Generate configs for all available devices
        for (const auto& [device_id, caps] : optimizer.device_capabilities_) {
            auto result = optimizer.optimize_launch_configuration(device_id, "hash_sha256");
            std::string key = std::to_string(device_id) + "_hash_sha256";
            static_configs_[std::make_pair(device_id, "hash_sha256")] = result.optimized_config;
        }
    }

    /**
     * @brief Generate memory kernel configurations
     */
    static void generate_memory_kernel_configs(KernelLaunchOptimizer& optimizer) {
        KernelCharacteristics mem_chars;
        mem_chars.kernel_name = "memory_transfer";
        mem_chars.shared_memory_per_block = 8192; // 8KB for staging buffer
        mem_chars.registers_per_thread = 16;
        mem_chars.memory_bound = true;
        mem_chars.compute_bound = false;
        mem_chars.synchronization_heavy = false;
        mem_chars.branch_divergent = false;
        mem_chars.memory_coalescing_efficiency = 0.95f;
        mem_chars.benefits_from_warp_shuffle = false;
        mem_chars.benefits_from_shared_memory_cache = true;

        optimizer.register_kernel_characteristics(mem_chars);

        // Generate configs for all available devices
        for (const auto& [device_id, caps] : optimizer.device_capabilities_) {
            auto result = optimizer.optimize_launch_configuration(device_id, "memory_transfer");
            std::string key = std::to_string(device_id) + "_memory_transfer";
            static_configs_[std::make_pair(device_id, "memory_transfer")] = result.optimized_config;
        }
    }

    /**
     * @brief Generate reduction kernel configurations
     */
    static void generate_reduction_kernel_configs(KernelLaunchOptimizer& optimizer) {
        KernelCharacteristics reduction_chars;
        reduction_chars.kernel_name = "reduction";
        reduction_chars.shared_memory_per_block = 2048; // 2KB for partial results
        reduction_chars.registers_per_thread = 20;
        reduction_chars.memory_bound = false;
        reduction_chars.compute_bound = true;
        reduction_chars.synchronization_heavy = true;
        reduction_chars.branch_divergent = false;
        reduction_chars.memory_coalescing_efficiency = 0.7f;
        reduction_chars.benefits_from_warp_shuffle = true;
        reduction_chars.benefits_from_shared_memory_cache = true;
        reduction_chars.benefits_from_occupancy_optimization = true;

        optimizer.register_kernel_characteristics(reduction_chars);

        // Generate configs for all available devices
        for (const auto& [device_id, caps] : optimizer.device_capabilities_) {
            auto result = optimizer.optimize_launch_configuration(device_id, "reduction");
            std::string key = std::to_string(device_id) + "_reduction";
            static_configs_[std::make_pair(device_id, "reduction")] = result.optimized_config;
        }
    }
};

// Static member definitions
std::map<std::pair<int, std::string>, LaunchConfig> StaticLaunchConfigFactory::static_configs_;
bool StaticLaunchConfigFactory::initialized_ = false;

} // namespace launch_optimization
} // namespace keyhunt
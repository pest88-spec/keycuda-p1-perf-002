// Puzzle71 Technical Debt Repair - Kernel Launch Parameter Optimization Implementation
// User Story 2: Performance Validation and Optimization
// Task: T047 - Optimize kernel launch parameters for different GPU architectures

#include "kernel_launch_optimization.cuh"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <chrono>

namespace keyhunt {
namespace launch_optimization {

// Utility functions for performance prediction and optimization

/**
 * @brief Predict kernel performance based on configuration
 */
double predict_kernel_performance(const DeviceCapabilities& device_caps,
                                const LaunchConfig& config,
                                const KernelCharacteristics& kernel_chars) {
    double base_performance = 1.0;

    // Architecture-specific performance factors
    double architecture_factor = 1.0;
    switch (device_caps.architecture) {
        case GPUArchitecture::PASCAL:
            architecture_factor = 0.7; // Older architecture
            break;
        case GPUArchitecture::VOLTA:
            architecture_factor = 0.85;
            break;
        case GPUArchitecture::TURING:
            architecture_factor = 0.9;
            break;
        case GPUArchitecture::AMPERE:
        case GPUArchitecture::AMPERE_86:
            architecture_factor = 1.0; // Baseline
            break;
        case GPUArchitecture::ADA_LOVELACE:
            architecture_factor = 1.15; // Latest architecture
            break;
        case GPUArchitecture::HOPPER:
            architecture_factor = 1.25; // Data center optimized
            break;
        default:
            architecture_factor = 0.5;
            break;
    }

    // Occupancy factor
    double occupancy_factor = calculate_predicted_occupancy(device_caps, config);
    occupancy_factor = std::min(1.0, occupancy_factor / 0.75); // Normalize to target 75% occupancy

    // Memory efficiency factor
    double memory_efficiency_factor = 1.0;
    if (kernel_chars.memory_coalescing_efficiency < 0.8) {
        memory_efficiency_factor = kernel_chars.memory_coalescing_efficiency / 0.8;
    }

    // Synchronization overhead factor
    double sync_overhead_factor = 1.0;
    if (kernel_chars.synchronization_heavy) {
        // Smaller blocks reduce sync overhead
        sync_overhead_factor = std::min(1.0, 512.0 / config.block_size.x);
    }

    // Register pressure factor
    double register_factor = 1.0;
    if (kernel_chars.register_usage_per_thread > 50) {
        register_factor = std::min(1.0, 50.0 / kernel_chars.register_usage_per_thread);
    }

    // Shared memory efficiency factor
    double shared_memory_factor = 1.0;
    if (config.shared_memory_size > 0 && kernel_chars.shared_memory_bank_conflict_rate > 0.1) {
        shared_memory_factor = std::min(1.0, 0.1 / kernel_chars.shared_memory_bank_conflict_rate);
    }

    // Optimization factor
    double optimization_factor = 1.0;
    if (config.enable_warp_shuffle_optimization && kernel_chars.benefits_from_warp_shuffle) {
        optimization_factor *= 1.1;
    }
    if (config.enable_cooperative_launch && device_caps.supports_cooperative_launch) {
        optimization_factor *= 1.05;
    }

    return base_performance * architecture_factor * occupancy_factor *
           memory_efficiency_factor * sync_overhead_factor *
           register_factor * shared_memory_factor * optimization_factor;
}

/**
 * @brief Calculate predicted occupancy
 */
double calculate_predicted_occupancy(const DeviceCapabilities& device_caps,
                                    const LaunchConfig& config) {
    // Calculate theoretical maximum warps per SM
    int warps_per_block = (config.block_size.x + device_caps.warp_size - 1) / device_caps.warp_size;

    int max_blocks_by_threads = device_caps.max_threads_per_multiprocessor / config.block_size.x;
    int max_blocks_by_registers = device_caps.max_registers_per_multiprocessor / (32 * config.block_size.x); // Assume 32 registers per thread
    int max_blocks_by_shared_mem = config.shared_memory_size > 0 ?
        device_caps.shared_memory_per_multiprocessor / config.shared_memory_size :
        device_caps.max_blocks_per_multiprocessor;

    int max_blocks_per_sm = std::min({max_blocks_by_threads, max_blocks_by_registers,
                                      max_blocks_by_shared_mem, device_caps.max_blocks_per_multiprocessor});

    int max_warps_per_sm = max_blocks_per_sm * warps_per_block;
    int theoretical_max_warps_per_sm = device_caps.max_threads_per_multiprocessor / device_caps.warp_size;

    return static_cast<double>(max_warps_per_sm) / theoretical_max_warps_per_sm;
}

/**
 * @brief Validate launch configuration
 */
bool validate_launch_configuration(const DeviceCapabilities& device_caps,
                                 const LaunchConfig& config,
                                 std::vector<std::string>& validation_errors) {
    validation_errors.clear();
    bool valid = true;

    // Check block size limits
    if (config.block_size.x > device_caps.max_threads_per_block) {
        validation_errors.push_back("Block size X exceeds maximum threads per block");
        valid = false;
    }

    if (config.block_size.y > 1 && config.block_size.x * config.block_size.y > device_caps.max_threads_per_block) {
        validation_errors.push_back("Block size X*Y exceeds maximum threads per block");
        valid = false;
    }

    // Check grid size limits
    if (config.grid_size.x > device_caps.max_grid_dim[0]) {
        validation_errors.push_back("Grid size X exceeds maximum grid dimension");
        valid = false;
    }

    // Check shared memory limits
    if (config.shared_memory_size > device_caps.shared_memory_per_block) {
        validation_errors.push_back("Shared memory size exceeds per-block limit");
        valid = false;
    }

    // Check register usage using conservative estimate (32 registers per thread)
    int required_registers = 32 * config.block_size.x;
    if (required_registers > device_caps.max_registers_per_block) {
        validation_errors.push_back("Register usage exceeds per-block limit");
        valid = false;
    }

    return valid;
}

/**
 * @brief Generate optimization report
 */
std::string generate_optimization_report(const OptimizationResult& result,
                                        const DeviceCapabilities& device_caps) {
    std::stringstream report;
    report << "=== Kernel Launch Optimization Report ===\n\n";

    report << "Kernel: " << result.optimized_config.kernel_name << "\n";
    report << "Device: " << device_caps.device_name << " (SM " << device_caps.compute_capability_major
           << "." << device_caps.compute_capability_minor << ")\n";
    report << "Architecture: " << architecture_to_string(device_caps.architecture) << "\n\n";

    // Configuration changes
    if (!result.configuration_changes.empty()) {
        report << "Configuration Changes:\n";
        for (const auto& change : result.configuration_changes) {
            report << "  • " << change << "\n";
        }
        report << "\n";
    }

    // Optimization details
    report << "Optimized Configuration:\n";
    report << "  Block Size: " << result.optimized_config.block_size.x << " threads\n";
    report << "  Grid Size: " << result.optimized_config.grid_size.x << " blocks\n";
    report << "  Shared Memory: " << result.optimized_config.shared_memory_size << " bytes\n";
    report << "  Cooperative Launch: " << (result.optimized_config.enable_cooperative_launch ? "Yes" : "No") << "\n";
    report << "  Warp Shuffle Optimization: " << (result.optimized_config.enable_warp_shuffle_optimization ? "Yes" : "No") << "\n";
    report << "  Memory Coalescing Optimization: " << (result.optimized_config.enable_memory_coalescing_optimization ? "Yes" : "No") << "\n\n";

    // Performance improvements
    report << "Performance Impact:\n";
    report << "  Estimated Performance Improvement: " << std::fixed << std::setprecision(1)
           << result.performance_improvement_percentage << "%\n";
    report << "  Memory Efficiency Improvement: " << std::fixed << std::setprecision(1)
           << result.memory_efficiency_improvement << "%\n";
    report << "  Occupancy Improvement: " << std::fixed << std::setprecision(1)
           << result.occupancy_improvement << "%\n";
    report << "  Throughput Improvement: " << std::fixed << std::setprecision(1)
           << result.throughput_improvement << "%\n\n";

    // Constitutional compliance
    report << "Constitutional Compliance:\n";
    report << "  Meets Requirements: " << (result.meets_constitutional_requirements ? "✓ YES" : "✗ NO") << "\n";
    report << "  Static Configuration: " << (result.statically_configured ? "✓ YES" : "✗ NO") << "\n";
    report << "  Runtime Queries Eliminated: " << (result.runtime_queries_eliminated ? "✓ YES" : "✗ NO") << "\n";

    if (!result.constitutional_violations.empty()) {
        report << "\nConstitutional Violations:\n";
        for (const auto& violation : result.constitutional_violations) {
            report << "  ✗ " << violation << "\n";
        }
    }

    report << "\n";

    // Optimization suggestions
    if (!result.optimization_suggestions.empty()) {
        report << "Optimization Suggestions:\n";
        for (const auto& suggestion : result.optimization_suggestions) {
            report << "  • " << suggestion << "\n";
        }
        report << "\n";
    }

    report << "=== End of Optimization Report ===\n";
    return report.str();
}

// Enhanced KernelLaunchOptimizer methods

bool KernelLaunchOptimizer::optimize_all_kernels(int device_id) {
    if (!initialized_) {
        return false;
    }

    auto device_it = device_capabilities_.find(device_id);
    if (device_it == device_capabilities_.end()) {
        return false;
    }

    bool all_optimized = true;

    // Get all kernel characteristics (would need getter method in real implementation)
    std::vector<std::string> kernel_names = {"ecc_scalar_mul", "hash_sha256", "memory_transfer", "reduction"};
    for (const auto& kernel_name : kernel_names) {
        auto result = optimize_launch_configuration(device_id, kernel_name);

        if (!result.meets_constitutional_requirements) {
            std::cerr << "Warning: Kernel " << kernel_name << " optimization doesn't meet constitutional requirements" << std::endl;
            all_optimized = false;
        }

        // Cache the optimized configuration
        std::string config_key = std::to_string(device_id) + "_" + kernel_name;
        optimized_configs_[config_key] = result.optimized_config;

        std::cout << "Optimized " << kernel_name << " for " << device_it->second.device_name << std::endl;
    }

    return all_optimized;
}

std::vector<OptimizationResult> KernelLaunchOptimizer::optimize_for_all_devices() {
    std::vector<OptimizationResult> all_results;

    if (!initialized_) {
        return all_results;
    }

    // Optimize each kernel for each available device
    for (const auto& [device_id, device_caps] : device_capabilities_) {
        std::vector<std::string> kernel_names = {"ecc_scalar_mul", "hash_sha256", "memory_transfer", "reduction"};
        for (const auto& kernel_name : kernel_names) {
            auto result = optimize_launch_configuration(device_id, kernel_name);
            result.performance_improvement_percentage = predict_performance_improvement(device_caps, result);
            all_results.push_back(result);

            // Cache the configuration
            std::string config_key = std::to_string(device_id) + "_" + kernel_name;
            optimized_configs_[config_key] = result.optimized_config;
        }
    }

    return all_results;
}

double KernelLaunchOptimizer::predict_performance_improvement(const DeviceCapabilities& device_caps,
                                                            const OptimizationResult& result) {
    // Use default kernel characteristics for prediction
    KernelCharacteristics default_chars;
    default_chars.kernel_name = result.optimized_config.kernel_name;
    default_chars.register_usage_per_thread = 32;
    default_chars.memory_coalescing_efficiency = 0.8f;

    // Predict performance of baseline configuration
    double baseline_performance = predict_kernel_performance(device_caps, result.baseline_config, default_chars);

    // Predict performance of optimized configuration
    double optimized_performance = predict_kernel_performance(device_caps, result.optimized_config, default_chars);

    // Calculate improvement percentage
    if (baseline_performance > 0) {
        return ((optimized_performance - baseline_performance) / baseline_performance) * 100.0;
    }

    return 0.0;
}

bool KernelLaunchOptimizer::export_optimization_results(const std::vector<OptimizationResult>& results,
                                                       const std::string& output_filename) {
    std::ofstream file(output_filename);
    if (!file.is_open()) {
        return false;
    }

    file << "=== Comprehensive Kernel Launch Optimization Results ===\n\n";

    // Summary statistics
    int total_optimizations = results.size();
    int constitutional_compliant = 0;
    double avg_performance_improvement = 0.0;

    for (const auto& result : results) {
        if (result.meets_constitutional_requirements) {
            constitutional_compliant++;
        }
        avg_performance_improvement += result.performance_improvement_percentage;
    }

    if (total_optimizations > 0) {
        avg_performance_improvement /= total_optimizations;
    }

    file << "Summary:\n";
    file << "  Total Optimizations: " << total_optimizations << "\n";
    file << "  Constitutionally Compliant: " << constitutional_compliant << "/" << total_optimizations << "\n";
    file << "  Average Performance Improvement: " << std::fixed << std::setprecision(1)
         << avg_performance_improvement << "%\n\n";

    // Detailed results
    file << "Detailed Optimization Results:\n";
    file << "Kernel,Device,Architecture,Block Size,Grid Size,Shared Memory,Performance Improvement,Constitutional\n";

    for (const auto& result : results) {
        file << result.optimized_config.kernel_name << ","
             << "GPU_Device," <<  // Simplified for export
             << "GPU," <<  // Simplified for export
             << result.optimized_config.block_size.x << ","
             << result.optimized_config.grid_size.x << ","
             << result.optimized_config.shared_memory_size << ","
             << std::fixed << std::setprecision(1) << result.performance_improvement_percentage << "%,"
             << (result.meets_constitutional_requirements ? "Yes" : "No") << "\n";
    }

    file.close();
    return true;
}

/**
 * @brief Auto-tuning system for runtime optimization
 */
class KernelAutoTuner {
private:
    std::unique_ptr<KernelLaunchOptimizer> optimizer_;
    std::map<std::string, std::vector<LaunchConfig>> tested_configurations_;
    std::map<std::string, LaunchConfig> best_configurations_;
    bool enabled_;

public:
    explicit KernelAutoTuner() : enabled_(false) {
        optimizer_ = std::make_unique<KernelLaunchOptimizer>();
    }

    /**
     * @brief Initialize auto-tuner
     */
    bool initialize() {
        if (!optimizer_->initialize()) {
            return false;
        }

        enabled_ = true;
        return true;
    }

    /**
     * @brief Auto-tune kernel parameters for specific device
     */
    bool auto_tune_kernel(int device_id,
                         const std::string& kernel_name,
                         std::function<double(const LaunchConfig&)> performance_function) {
        if (!enabled_) {
            return false;
        }

        auto device_caps = optimizer_->get_device_capabilities(device_id);
        KernelCharacteristics kernel_chars;
    kernel_chars.kernel_name = kernel_name;
    kernel_chars.shared_memory_per_block = 4096;
    kernel_chars.register_usage_per_thread = 32;
    kernel_chars.memory_coalescing_efficiency = 0.8f;
    kernel_chars.benefits_from_warp_shuffle = true;

        // Generate candidate configurations
        std::vector<LaunchConfig> candidates = generate_candidate_configurations(device_caps, kernel_chars);

        std::cout << "Auto-tuning " << kernel_name << " with " << candidates.size() << " configurations..." << std::endl;

        double best_performance = 0.0;
        LaunchConfig best_config;

        // Test each configuration
        for (size_t i = 0; i < candidates.size(); ++i) {
            const auto& config = candidates[i];

            std::cout << "Testing configuration " << (i + 1) << "/" << candidates.size()
                      << " (block_size=" << config.block_size.x << ")..." << std::endl;

            double performance = performance_function(config);
            std::cout << "Performance: " << std::fixed << std::setprecision(2) << performance << std::endl;

            if (performance > best_performance) {
                best_performance = performance;
                best_config = config;
            }
        }

        // Store best configuration
        best_configurations_[kernel_name] = best_config;
        std::cout << "Best configuration for " << kernel_name << ": block_size=" << best_config.block_size.x
                  << ", grid_size=" << best_config.grid_size.x << std::endl;

        return true;
    }

    /**
     * @brief Get best auto-tuned configuration
     */
    LaunchConfig get_best_configuration(const std::string& kernel_name) {
        auto it = best_configurations_.find(kernel_name);
        if (it != best_configurations_.end()) {
            return it->second;
        }

        // Fall back to optimizer if no auto-tuned result
        return optimizer_->get_optimized_configuration(0, kernel_name); // Default to device 0
    }

private:
    /**
     * @brief Generate candidate configurations for testing
     */
    std::vector<LaunchConfig> generate_candidate_configurations(const DeviceCapabilities& device_caps,
                                                              const KernelCharacteristics& kernel_chars) {
        std::vector<LaunchConfig> candidates;

        // Test different block sizes (multiples of warp size)
        std::vector<int> block_sizes = {64, 128, 256, 384, 512, 640, 768, 896, 1024};

        for (int block_size : block_sizes) {
            // Skip if block size exceeds device limits
            if (block_size > device_caps.max_threads_per_block) {
                continue;
            }

            // Ensure block size is multiple of warp size
            if (block_size % device_caps.warp_size != 0) {
                continue;
            }

            LaunchConfig config;
            config.block_size = dim3(block_size, 1, 1);
            config.target_architecture = device_caps.architecture;
            config.kernel_name = kernel_chars.kernel_name;

            // Calculate grid size based on device characteristics
            int blocks_per_sm = device_caps.max_threads_per_multiprocessor / block_size;
            config.grid_size = dim3(device_caps.multiprocessor_count * blocks_per_sm, 1, 1);

            // Set shared memory based on kernel requirements
            config.shared_memory_size = kernel_chars.shared_memory_per_block;

            // Enable optimizations based on kernel characteristics
            config.enable_warp_shuffle_optimization = device_caps.supports_shuffle_instructions &&
                                                     kernel_chars.benefits_from_warp_shuffle;
            config.enable_memory_coalescing_optimization = kernel_chars.memory_coalescing_efficiency < 0.8f;

            candidates.push_back(config);
        }

        return candidates;
    }
};

// Utility functions

std::string architecture_to_string(GPUArchitecture arch) {
    switch (arch) {
        case GPUArchitecture::PASCAL: return "Pascal (SM 6.x)";
        case GPUArchitecture::VOLTA: return "Volta (SM 7.0)";
        case GPUArchitecture::TURING: return "Turing (SM 7.5)";
        case GPUArchitecture::AMPERE: return "Ampere (SM 8.0)";
        case GPUArchitecture::AMPERE_86: return "Ampere (SM 8.6)";
        case GPUArchitecture::ADA_LOVELACE: return "Ada Lovelace (SM 8.9)";
        case GPUArchitecture::HOPPER: return "Hopper (SM 9.0)";
        default: return "Unknown";
    }
}

/**
 * @brief Print device information for debugging
 */
void print_device_information(const DeviceCapabilities& caps) {
    std::cout << "Device Information:" << std::endl;
    std::cout << "  Name: " << caps.device_name << std::endl;
    std::cout << "  Compute Capability: " << caps.compute_capability_major << "." << caps.compute_capability_minor << std::endl;
    std::cout << "  Architecture: " << architecture_to_string(caps.architecture) << std::endl;
    std::cout << "  Multiprocessors: " << caps.multiprocessor_count << std::endl;
    std::cout << "  Total Global Memory: " << caps.total_global_memory / (1024 * 1024) << " MB" << std::endl;
    std::cout << "  Shared Memory per Block: " << caps.shared_memory_per_block / 1024 << " KB" << std::endl;
    std::cout << "  Max Threads per Block: " << caps.max_threads_per_block << std::endl;
    std::cout << "  Max Threads per SM: " << caps.max_threads_per_multiprocessor << std::endl;
    std::cout << "  Warp Size: " << caps.warp_size << std::endl;
    std::cout << "  Max Registers per Block: " << caps.max_registers_per_block << std::endl;
    std::cout << "  Memory Bandwidth: " << std::fixed << std::setprecision(1) << caps.memory_bandwidth_gbps << " GB/s" << std::endl;
    std::cout << "  Supports Shuffle: " << (caps.supports_shuffle_instructions ? "Yes" : "No") << std::endl;
    std::cout << "  Supports Cooperative Launch: " << (caps.supports_cooperative_launch ? "Yes" : "No") << std::endl;
    std::cout << "  Supports Tensor Cores: " << (caps.supports_tensor_cores ? "Yes" : "No") << std::endl;
}

/**
 * @brief Compare two launch configurations
 */
void compare_configurations(const LaunchConfig& config1, const LaunchConfig& config2) {
    std::cout << "Configuration Comparison:" << std::endl;
    std::cout << "  Block Size: " << config1.block_size.x << " vs " << config2.block_size.x << std::endl;
    std::cout << "  Grid Size: " << config1.grid_size.x << " vs " << config2.grid_size.x << std::endl;
    std::cout << "  Shared Memory: " << config1.shared_memory_size << "B vs " << config2.shared_memory_size << "B" << std::endl;
    std::cout << "  Warp Shuffle: " << (config1.enable_warp_shuffle_optimization ? "Yes" : "No")
              << " vs " << (config2.enable_warp_shuffle_optimization ? "Yes" : "No") << std::endl;
    std::cout << "  Cooperative Launch: " << (config1.enable_cooperative_launch ? "Yes" : "No")
              << " vs " << (config2.enable_cooperative_launch ? "Yes" : "No") << std::endl;
}

} // namespace launch_optimization
} // namespace keyhunt
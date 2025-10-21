// Test file demonstrating the enhanced static launch configuration system
// This file shows how to use the new static configuration features

#include "static_launch_config.h"
#include <iostream>

using namespace keyhunt::config;

void demonstrate_basic_usage() {
    std::cout << "=== Basic Static Launch Configuration Demo ===" << std::endl;

    // Get configuration for current architecture
    const auto& config = get_current_static_config();
    std::cout << "Current architecture config: " << StaticLaunchConfigManager::get_config_summary(config) << std::endl;

    // Validate configuration
    bool is_valid = validate_current_static_config();
    std::cout << "Configuration is valid: " << (is_valid ? "YES" : "NO") << std::endl;

    // Calculate total keys per batch
    uint64_t total_keys = calculateTotalKeys({
        .grid_size_x = config.grid_dim.x,
        .grid_size_y = config.grid_dim.y,
        .grid_size_z = config.grid_dim.z,
        .block_size_x = config.block_dim.x,
        .block_size_y = config.block_dim.y,
        .block_size_z = config.block_dim.z,
        .points_per_thread = 256,
        .shared_memory_size = static_cast<uint32_t>(config.shared_memory_size),
        .enforce_static_config = true,
        .disable_runtime_override = true
    });
    std::cout << "Total keys per batch: " << total_keys << std::endl;
    std::cout << std::endl;
}

void demonstrate_advanced_usage() {
    std::cout << "=== Advanced Static Launch Configuration Demo ===" << std::endl;

    // Get advanced configuration
    const auto& advanced_config = get_current_advanced_config();
    std::cout << "Advanced configuration loaded successfully" << std::endl;

    // Get cache configuration
    const auto& cache_config = get_current_cache_config();
    std::cout << "Cache config - L1 size: " << cache_config.l1_cache_size_bytes / 1024 << " KB" << std::endl;
    std::cout << "Cache config - Target hit rate: " << cache_config.target_cache_hit_rate_percent << "%" << std::endl;

    // Get memory bandwidth configuration
    const auto& memory_config = get_current_memory_config();
    std::cout << "Memory config - Vector size: " << memory_config.vector_size_bytes << " bytes" << std::endl;
    std::cout << "Memory config - Target throughput: " << memory_config.target_memory_throughput_gbps << " GB/s" << std::endl;

    // Validate advanced configuration
    bool is_valid = validate_current_advanced_config();
    std::cout << "Advanced configuration is valid: " << (is_valid ? "YES" : "NO") << std::endl;
    std::cout << std::endl;
}

void demonstrate_performance_estimation() {
    std::cout << "=== Performance Estimation Demo ===" << std::endl;

    // Get performance estimates
    auto performance_metrics = get_current_performance_estimate();

    std::cout << "Performance Metrics:" << std::endl;
    for (const auto& [key, value] : performance_metrics) {
        std::cout << "  " << key << ": " << value << std::endl;
    }
    std::cout << std::endl;
}

void demonstrate_optimization_recommendations() {
    std::cout << "=== Optimization Recommendations Demo ===" << std::endl;

    // Get optimization recommendations
    auto recommendations = get_current_optimization_recommendations();

    if (recommendations.empty()) {
        std::cout << "No optimization recommendations - configuration is already optimal!" << std::endl;
    } else {
        std::cout << "Optimization Recommendations:" << std::endl;
        for (size_t i = 0; i < recommendations.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << recommendations[i] << std::endl;
        }
    }
    std::cout << std::endl;
}

void demonstrate_architecture_specific_configs() {
    std::cout << "=== Architecture-Specific Configuration Demo ===" << std::endl;

    // Show configurations for different architectures
    const char* arch_names[] = {"Turing", "Ampere", "Ada Lovelace", "Hopper"};
    GPUArchitecture archs[] = {GPUArchitecture::TURING, GPUArchitecture::AMPERE,
                              GPUArchitecture::ADA_LOVELACE, GPUArchitecture::HOPPER};

    for (int i = 0; i < 4; ++i) {
        const auto& config = StaticLaunchConfigManager::get_launch_config(archs[i]);
        std::cout << arch_names[i] << " config: " << StaticLaunchConfigManager::get_config_summary(config) << std::endl;
    }
    std::cout << std::endl;
}

void demonstrate_yaml_loading() {
    std::cout << "=== YAML Configuration Loading Demo ===" << std::endl;

    try {
        // Try to load from YAML file (this is a placeholder implementation)
        auto yaml_config = load_current_advanced_config_from_yaml("config/enhanced_static_config.yaml");
        std::cout << "YAML configuration loaded successfully" << std::endl;
        std::cout << "Note: Full YAML loading implementation coming in next phase" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "YAML loading not yet fully implemented: " << e.what() << std::endl;
    }
    std::cout << std::endl;
}

void demonstrate_error_handling() {
    std::cout << "=== Error Handling and Fallback Demo ===" << std::endl;

    // Get fallback configuration
    const auto& fallback_config = StaticLaunchConfigManager::get_fallback_config(
        GPUArchitecture::UNKNOWN, CONFIG_FILE_NOT_FOUND
    );

    std::cout << "Fallback configuration loaded for unknown architecture" << std::endl;
    std::cout << "Fallback occupancy target: " << fallback_config.target_occupancy_percent << "%" << std::endl;
    std::cout << "This demonstrates the system's robust error handling" << std::endl;
    std::cout << std::endl;
}

void demonstrate_constitutional_compliance() {
    std::cout << "=== Constitutional Compliance Demo ===" << std::endl;

    // Show constitutional compliance constants
    std::cout << "Constitutional v5.5 Compliance Constants:" << std::endl;
    std::cout << "  Minimum occupancy: " << MINIMUM_OCCUPANCY_PERCENT << "%" << std::endl;
    std::cout << "  Minimum memory efficiency: " << MINIMUM_MEMORY_EFFICIENCY_PERCENT << "%" << std::endl;
    std::cout << "  Minimum GPU utilization: " << MINIMUM_GPU_UTILIZATION_PERCENT << "%" << std::endl;
    std::cout << "  Precision tolerance: " << PRECISION_TOLERANCE << std::endl;
    std::cout << "  Maximum power: " << MAXIMUM_POWER_WATTS << "W" << std::endl;
    std::cout << "  Maximum temperature: " << MAXIMUM_TEMPERATURE_CELSIUS << "°C" << std::endl;

    // Verify current configuration compliance
    const auto& config = get_current_advanced_config();
    bool is_compliant = StaticLaunchConfigManager::validate_advanced_config(config);
    std::cout << "Current configuration is constitutionally compliant: " << (is_compliant ? "YES" : "NO") << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "Enhanced Static Launch Configuration System Demonstration" << std::endl;
    std::cout << "=========================================================" << std::endl;
    std::cout << std::endl;

    demonstrate_basic_usage();
    demonstrate_advanced_usage();
    demonstrate_performance_estimation();
    demonstrate_optimization_recommendations();
    demonstrate_architecture_specific_configs();
    demonstrate_yaml_loading();
    demonstrate_error_handling();
    demonstrate_constitutional_compliance();

    std::cout << "=== System Information ===" << std::endl;
    std::cout << "Static configuration available: " << (StaticLaunchConfigManager::is_static_config_available() ? "YES" : "NO") << std::endl;
    std::cout << "Current architecture detection: ";

    switch (StaticLaunchConfigManager::detect_architecture()) {
        case GPUArchitecture::TURING:
            std::cout << "Turing (sm_75)";
            break;
        case GPUArchitecture::AMPERE:
            std::cout << "Ampere (sm_86)";
            break;
        case GPUArchitecture::ADA_LOVELACE:
            std::cout << "Ada Lovelace (sm_89)";
            break;
        case GPUArchitecture::HOPPER:
            std::cout << "Hopper (sm_90)";
            break;
        default:
            std::cout << "Unknown";
            break;
    }
    std::cout << std::endl;

    std::cout << std::endl;
    std::cout << "Enhanced Static Launch Configuration System - DEMO COMPLETE" << std::endl;

    return 0;
}
// Puzzle71 Technical Debt Repair - Static Configuration Integration Usage Example (T030)
// Demonstrates complete integration of static configuration system with kernel launches
// Shows constitutional compliance, validation, fallback mechanisms, and performance monitoring

#include <iostream>
#include <iomanip>
#include <chrono>
#include <array>

#include "compute/gpu/static_config_integration.h"
#include "compute/gpu/kernel_config_validator.h"
#include "compute/adapters/static_config_adapter.h"

using namespace keyhunt;
using namespace keyhunt::integration;
using namespace keyhunt::validation;
using namespace keyhunt::adapters;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n";
}

void print_configuration_info(const IntegratedLaunchConfig& config) {
    std::cout << "Configuration Details:\n";
    std::cout << "  Source: " << config.config_source << "\n";
    std::cout << "  Architecture: " << integration_utils::architecture_to_string(config.architecture) << "\n";
    std::cout << "  Grid: (" << config.grid_dim.x << ", " << config.grid_dim.y << ", " << config.grid_dim.z << ")\n";
    std::cout << "  Block: (" << config.block_dim.x << ", " << config.block_dim.y << ", " << config.block_dim.z << ")\n";
    std::cout << "  Shared Memory: " << config.shared_memory_size << " bytes\n";
    std::cout << "  Points per Thread: " << config.points_per_thread << "\n";
    std::cout << "  Batch Size: " << config.batch_size << "\n";
    std::cout << "  Constitutional Compliance: " << (config.constitutional_compliance ? "YES" : "NO") << "\n";
    std::cout << "  Performance Targets:\n";
    std::cout << "    Memory Efficiency: " << config.target_memory_efficiency_percent << "%\n";
    std::cout << "    GPU Utilization: " << config.target_gpu_utilization_percent << "%\n";
    std::cout << "    Occupancy: " << config.target_occupancy_percent << "%\n";
}

void print_validation_result(const KernelValidationResult& result) {
    std::cout << "Validation Result: " << result.getSummary() << "\n";
    std::cout << "Compliance Score: " << std::fixed << std::setprecision(2)
              << (result.compliance_score * 100) << "%\n";
    std::cout << "Validation Time: " << result.validation_time.count() << "ms\n";

    if (!result.errors.empty()) {
        std::cout << "Errors (" << result.errors.size() << "):\n";
        for (const auto& error : result.errors) {
            std::cout << "  ❌ " << error << "\n";
        }
    }

    if (!result.warnings.empty()) {
        std::cout << "Warnings (" << result.warnings.size() << "):\n";
        for (const auto& warning : result.warnings) {
            std::cout << "  ⚠️  " << warning << "\n";
        }
    }

    if (!result.info.empty()) {
        std::cout << "Information (" << result.info.size() << "):\n";
        for (const auto& info : result.info) {
            std::cout << "  ℹ️  " << info << "\n";
        }
    }
}

void print_performance_metrics(const KernelLaunchMetrics& metrics) {
    std::cout << "Performance Metrics:\n";
    std::cout << "  Keys Processed: " << metrics.keys_processed << "\n";
    std::cout << "  Keys/Second: " << std::fixed << std::setprecision(2) << metrics.keys_per_second << "\n";
    std::cout << "  Execution Time: " << metrics.execution_time.count() << " μs\n";
    std::cout << "  Total Time: " << metrics.total_time.count() << " μs\n";
    std::cout << "  Memory Efficiency: " << metrics.memory_efficiency_percent << "%\n";
    std::cout << "  GPU Utilization: " << metrics.gpu_utilization_percent << "%\n";
    std::cout << "  Occupancy: " << metrics.occupancy_percent << "%\n";
    std::cout << "  Constitutional Compliance: " << (metrics.constitutional_compliance ? "YES" : "NO") << "\n";
}

void example_basic_configuration() {
    print_separator("Basic Static Configuration Example");

    // Create integration manager for device 0
    StaticConfigIntegrationManager integration_manager(0);

    // Get configuration for ECC kernel
    uint64_t batch_size = 1000000;  // 1 million keys
    auto config = integration_manager.get_launch_config("ecc_scalar_mul_kernel", batch_size);

    print_configuration_info(config);

    // Check constitutional compliance
    bool is_compliant = integration_manager.validate_constitutional_compliance();
    std::cout << "Constitutional Compliance Check: " << (is_compliant ? "PASS" : "FAIL") << "\n";

    // Get compatibility report
    std::string report = integration_manager.get_compatibility_report();
    std::cout << "\nCompatibility Report:\n" << report << "\n";
}

void example_configuration_validation() {
    print_separator("Configuration Validation Example");

    // Create validator with constitutional constraints
    auto constraints = KernelValidationConstraints::get_constitutional_constraints();
    KernelConfigValidator validator(0, constraints);

    // Create test configuration
    IntegratedLaunchConfig config;
    config.static_config = config::StaticLaunchConfigManager::get_launch_config(
        config::StaticLaunchConfigManager::detect_architecture()
    );
    config.ecc_config = config::StaticLaunchConfigManager::get_ecc_config(
        config::StaticLaunchConfigManager::detect_architecture()
    );
    config.grid_dim = dim3(256, 1, 1);
    config.block_dim = dim3(128, 1, 1);
    config.shared_memory_size = 8192;
    config.points_per_thread = 256;
    config.batch_size = config.get_total_threads() * config.points_per_thread;
    config.architecture = config::StaticLaunchConfigManager::detect_architecture();
    config.config_source = "example";
    config.constitutional_compliance = true;
    config.target_memory_efficiency_percent = 95.0;
    config.target_gpu_utilization_percent = 90.0;
    config.target_occupancy_percent = 75.0;

    // Validate configuration
    auto result = validator.validate_config(config);
    print_validation_result(result);

    // Test validation guard
    std::cout << "\nTesting Validation Guard:\n";
    KernelLaunchValidationGuard guard(validator, config);
    if (guard.is_valid()) {
        std::cout << "✅ Configuration validated successfully\n";
    } else {
        std::cout << "❌ Configuration validation failed\n";
        std::cout << guard.get_report() << "\n";
    }
}

void example_fallback_configurations() {
    print_separator("Fallback Configuration Example");

    // Test safe fallback
    auto safe_fallback = FallbackConfigurationProvider::get_safe_fallback(
        config::GPUArchitecture::TURING, 404
    );
    std::cout << "Safe Fallback Configuration:\n";
    std::cout << "  Source: " << safe_fallback.config_source << "\n";
    std::cout << "  Constitutional: " << (safe_fallback.constitutional_compliance ? "YES" : "NO") << "\n";
    std::cout << "  Grid: (" << safe_fallback.grid_dim.x << ", " << safe_fallback.grid_dim.y << ")\n";
    std::cout << "  Block: (" << safe_fallback.block_dim.x << ", " << safe_fallback.block_dim.y << ")\n";

    // Test performance fallback
    auto perf_fallback = FallbackConfigurationProvider::get_performance_fallback(
        config::GPUArchitecture::AMPERE
    );
    std::cout << "\nPerformance Fallback Configuration:\n";
    std::cout << "  Source: " << perf_fallback.config_source << "\n";
    std::cout << "  Constitutional: " << (perf_fallback.constitutional_compliance ? "YES" : "NO") << "\n";
    std::cout << "  Grid: (" << perf_fallback.grid_dim.x << ", " << perf_fallback.grid_dim.y << ")\n";
    std::cout << "  Block: (" << perf_fallback.block_dim.x << ", " << perf_fallback.block_dim.y << ")\n";

    // Test minimal configuration
    auto minimal_config = FallbackConfigurationProvider::get_minimal_config();
    std::cout << "\nMinimal Configuration:\n";
    std::cout << "  Source: " << minimal_config.config_source << "\n";
    std::cout << "  Constitutional: " << (minimal_config.constitutional_compliance ? "YES" : "NO") << "\n";
    std::cout << "  Grid: (" << minimal_config.grid_dim.x << ", " << minimal_config.grid_dim.y << ")\n";
    std::cout << "  Block: (" << minimal_config.block_dim.x << ", " << minimal_config.block_dim.y << ")\n";
}

void example_deterministic_mode() {
    print_separator("Deterministic Mode Example");

    StaticConfigIntegrationManager integration_manager(0);

    // Enable deterministic mode with seed
    uint32_t deterministic_seed = 12345;
    integration_manager.set_deterministic_mode(true, deterministic_seed);

    std::cout << "Deterministic Mode: " << (integration_manager.is_deterministic_mode() ? "ENABLED" : "DISABLED") << "\n";
    std::cout << "Deterministic Seed: " << deterministic_seed << "\n";

    // Get multiple configurations - should be deterministic
    auto config1 = integration_manager.get_launch_config("ecc_kernel", 100000);
    auto config2 = integration_manager.get_launch_config("ecc_kernel", 100000);

    // Configurations should be identical in deterministic mode
    bool configs_match = (config1.grid_dim.x == config2.grid_dim.x) &&
                        (config1.block_dim.x == config2.block_dim.x) &&
                        (config1.shared_memory_size == config2.shared_memory_size);

    std::cout << "Deterministic Configuration Consistency: " << (configs_match ? "PASS" : "FAIL") << "\n";

    // Disable deterministic mode
    integration_manager.set_deterministic_mode(false);
    std::cout << "Deterministic Mode Disabled\n";
}

void example_yaml_configuration() {
    print_separator("YAML Configuration Example");

    // Create test YAML configuration file
    std::ofstream yaml_file("example_config.yaml");
    yaml_file << R"(# Example Configuration
config_version: "5.5"
config_schema_version: "1.0"

gpu_devices:
  - device_id: 0
    device_name: "Example GPU"
    compute_capability:
      major: 7
      minor: 5
    total_memory_bytes: 8589934592
    max_threads_per_block: 1024
    warp_size: 32
    architecture_family: "Turing"

kernel_configs:
  - kernel_name: "example_kernel"
    architecture_family: "Turing"
    block_size:
      x: 256
      y: 1
      z: 1
    min_grid_size: 512
    max_grid_size: 32768
    shared_memory_size_bytes: 12288
    registers_per_thread: 28
    expected_occupancy: 0.80

performance_config:
  enable_shared_memory_optimization: true
  enable_warp_level_optimization: true
  enable_memory_coalescing: true
  target_gpu_utilization_percent: 92.0
  target_memory_efficiency_percent: 96.0

validation_config:
  enable_deterministic_validation: true
  enable_constitutional_compliance: true
  validation_precision_tolerance: 1.0e-10
)";
    yaml_file.close();

    // Create integration manager with YAML file
    StaticConfigIntegrationManager integration_manager(0, "example_config.yaml");

    // Get configuration from YAML
    auto config = integration_manager.get_launch_config("example_kernel", 500000, true);

    std::cout << "YAML Configuration Loaded:\n";
    print_configuration_info(config);

    // Validate the configuration
    bool is_valid = integration_manager.validate_launch_config(config);
    std::cout << "YAML Configuration Validation: " << (is_valid ? "PASS" : "FAIL") << "\n";

    // Cleanup
    std::remove("example_config.yaml");
}

void example_kernel_launcher() {
    print_separator("Kernel Launcher Example");

    // Create kernel launcher
    StaticConfigKernelLauncher launcher(0);

    // Set target hash (example)
    std::array<uint32_t, 5> target_hash = {
        0x12345678, 0x87654321, 0xABCDEF00, 0x00FEDCBA, 0x13579BDF
    };

    std::cout << "Launching ECC Kernel...\n";

    // Launch ECC kernel
    cudaError_t result = launcher.launch_ecc_kernel(
        100000,  // batch size
        1,       // compression type (0=uncompressed, 1=compressed, 2=both)
        target_hash
    );

    if (result == cudaSuccess) {
        std::cout << "✅ ECC kernel launched successfully\n";
        auto metrics = launcher.get_metrics();
        print_performance_metrics(metrics);
    } else {
        std::cout << "❌ ECC kernel launch failed: " << cudaGetErrorString(result) << "\n";
    }

    // Launch fixed kernel
    std::cout << "\nLaunching Fixed Kernel...\n";
    result = launcher.launch_fixed_kernel(128, 2);

    if (result == cudaSuccess) {
        std::cout << "✅ Fixed kernel launched successfully\n";
        auto metrics = launcher.get_metrics();
        print_performance_metrics(metrics);
    } else {
        std::cout << "❌ Fixed kernel launch failed: " << cudaGetErrorString(result) << "\n";
    }
}

void example_adapter_integration() {
    print_separator("Adapter Integration Example");

    // Create static configuration adapter
    auto adapter = std::make_unique<StaticConfigurationAdapter>(0);

    if (adapter->initialize()) {
        std::cout << "✅ Static Configuration Adapter initialized\n";
        std::cout << "Adapter Info: " << adapter->get_adapter_info() << "\n";

        // Get configuration for kernel
        auto config = adapter->get_launch_config("test_kernel", 200000);
        std::cout << "Configuration from adapter:\n";
        print_configuration_info(config);

        // Validate configuration through adapter
        bool is_valid = adapter->validate_config(config);
        std::cout << "Adapter Configuration Validation: " << (is_valid ? "PASS" : "FAIL") << "\n";

        // Get adapter statistics
        auto stats = adapter->get_statistics();
        std::cout << "Adapter Statistics:\n";
        for (const auto& [key, value] : stats) {
            std::cout << "  " << key << ": " << value << "\n";
        }
    } else {
        std::cout << "❌ Static Configuration Adapter initialization failed\n";
    }

    // Test adapter factory
    std::cout << "\nTesting Adapter Factory:\n";
    auto best_adapter = ConfigurationAdapterFactory::create_best_adapter(0);
    if (best_adapter) {
        std::cout << "✅ Best adapter created: " << best_adapter->get_adapter_info() << "\n";
    } else {
        std::cout << "❌ Failed to create best adapter\n";
    }
}

void example_performance_estimation() {
    print_separator("Performance Estimation Example");

    // Create test configuration
    IntegratedLaunchConfig config;
    config.static_config = config::StaticLaunchConfigManager::get_launch_config(
        config::StaticLaunchConfigManager::detect_architecture()
    );
    config.grid_dim = dim3(512, 1, 1);
    config.block_dim = dim3(256, 1, 1);
    config.shared_memory_size = 16384;
    config.points_per_thread = 512;
    config.batch_size = config.get_total_threads() * config.points_per_thread;
    config.architecture = config::StaticLaunchConfigManager::detect_architecture();

    // Estimate performance
    auto metrics = integration_utils::estimate_performance(config);

    std::cout << "Performance Estimation:\n";
    std::cout << "  Estimated Throughput: " << std::fixed << std::setprecision(2)
              << metrics["estimated_keys_per_second"] << " keys/sec\n";
    std::cout << "  Theoretical Throughput: " << std::setprecision(2)
              << metrics["theoretical_throughput"] << " keys/sec\n";
    std::cout << "  Efficiency Factor: " << std::setprecision(3)
              << metrics["efficiency_factor"] << "\n";
    std::cout << "  Architecture Multiplier: " << std::setprecision(1)
              << metrics["architecture_multiplier"] << "x\n";
    std::cout << "  Total Threads: " << std::setprecision(0)
              << metrics["total_threads"] << "\n";

    // Test different architectures
    std::vector<config::GPUArchitecture> architectures = {
        config::GPUArchitecture::TURING,
        config::GPUArchitecture::AMPERE,
        config::GPUArchitecture::ADA_LOVELACE,
        config::GPUArchitecture::HOPPER
    };

    std::cout << "\nArchitecture Performance Comparison:\n";
    std::cout << std::setw(15) << "Architecture" << std::setw(20) << "Throughput (keys/s)" << "\n";
    std::cout << std::string(35, '-') << "\n";

    for (auto arch : architectures) {
        config.architecture = arch;
        auto arch_metrics = integration_utils::estimate_performance(config);
        std::cout << std::setw(15) << integration_utils::architecture_to_string(arch)
                  << std::setw(20) << std::fixed << std::setprecision(0)
                  << arch_metrics["estimated_keys_per_second"] << "\n";
    }
}

void example_cache_performance() {
    print_separator("Cache Performance Example");

    ValidationCacheManager cache_manager;
    KernelConfigValidator validator(0);

    // Create test configurations
    std::vector<IntegratedLaunchConfig> configs;
    std::vector<std::string> fingerprints;

    for (int i = 0; i < 10; ++i) {
        IntegratedLaunchConfig config;
        config.static_config = config::StaticLaunchConfigManager::get_launch_config(
            config::StaticLaunchConfigManager::detect_architecture()
        );
        config.grid_dim.x = 256 + i * 64;
        config.block_dim.x = 128 + i * 32;
        config.shared_memory_size = 8192 + i * 1024;
        config.points_per_thread = 256;
        config.batch_size = config.get_total_threads() * config.points_per_thread;
        config.architecture = config::StaticLaunchConfigManager::detect_architecture();
        config.config_source = "cache_test";

        configs.push_back(config);

        // Validate and cache
        auto result = validator.validate_config(config);
        std::string fingerprint = integration_utils::generate_config_fingerprint(config);
        fingerprints.push_back(fingerprint);
        cache_manager.cache_result(fingerprint, result);
    }

    // Benchmark cache hits
    const int num_accesses = 1000;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_accesses; ++i) {
        std::string fingerprint = fingerprints[i % fingerprints.size()];
        auto cached_result = cache_manager.get_cached_result(fingerprint);
        ASSERT_TRUE(cached_result.has_value());
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    double avg_cache_hit_time_us = static_cast<double>(duration.count()) / num_accesses;
    double cache_hit_rate_hz = 1000000.0 / avg_cache_hit_time_us;

    std::cout << "Cache Performance Results:\n";
    std::cout << "  Cached Configurations: " << fingerprints.size() << "\n";
    std::cout << "  Cache Accesses: " << num_accesses << "\n";
    std::cout << "  Average Hit Time: " << std::fixed << std::setprecision(3)
              << avg_cache_hit_time_us << " μs\n";
    std::cout << "  Cache Hit Rate: " << std::setprecision(0) << cache_hit_rate_hz << " hits/sec\n";

    // Get cache statistics
    auto cache_stats = cache_manager.get_cache_statistics();
    std::cout << "  Cache Statistics:\n";
    for (const auto& [key, value] : cache_stats) {
        std::cout << "    " << key << ": " << value << "\n";
    }
}

int main() {
    std::cout << "Puzzle71 Static Configuration Integration Examples\n";
    std::cout << "=================================================\n";

    try {
        // Run all examples
        example_basic_configuration();
        example_configuration_validation();
        example_fallback_configurations();
        example_deterministic_mode();
        example_yaml_configuration();
        example_kernel_launcher();
        example_adapter_integration();
        example_performance_estimation();
        example_cache_performance();

        print_separator("All Examples Completed Successfully");
        std::cout << "✅ All examples completed without errors\n";
        std::cout << "\nKey Features Demonstrated:\n";
        std::cout << "  ✅ Static configuration loading and validation\n";
        std::cout << "  ✅ Constitutional compliance checking\n";
        std::cout << "  ✅ Fallback configuration mechanisms\n";
        std::cout << "  ✅ Deterministic mode operation\n";
        std::cout << "  ✅ YAML configuration support\n";
        std::cout << "  ✅ Kernel launching with configuration\n";
        std::cout << "  ✅ Adapter pattern integration\n";
        std::cout << "  ✅ Performance estimation\n";
        std::cout << "  ✅ Caching and optimization\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
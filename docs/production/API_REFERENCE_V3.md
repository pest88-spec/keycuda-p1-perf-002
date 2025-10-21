# Puzzle71 CUDA Technical Debt Repair System - API Reference v3.0

**Version**: 3.0.0
**Updated**: 2025-10-21
**Branch**: `002-techdebt-repair`

## Overview

This document provides a comprehensive reference for the Puzzle71 CUDA Technical Debt Repair System APIs, including command-line interface, configuration options, and programmatic interfaces.

## Command Line Interface

### Core Commands

#### `Puzzle71Solver` - Main Application

**Syntax**: `./Puzzle71Solver [OPTIONS]`

**Description**: Main executable for Bitcoin private key scanning with technical debt repairs implemented.

#### Global Options

| Option | Short | Type | Description | Default |
|--------|-------|------|-------------|---------|
| `--help` | `-h` | flag | Display help message and exit | - |
| `--version` | `-V` | flag | Display version information | - |
| `--config` | `-c` | path | Configuration file path | `data/config.yaml` |
| `--verbose` | `-v` | flag | Enable verbose output | - |
| `--quiet` | `-q` | flag | Suppress non-error output | - |
| `--log-level` | `-l` | string | Logging level (DEBUG, INFO, WARN, ERROR) | `INFO` |
| `--log-file` | - | path | Log file path | `logs/puzzle71.log` |

#### Core Scanning Options

| Option | Type | Description | Default |
|--------|------|-------------|---------|
| `--range-file` | path | Private key ranges file | `data/private_ranges.txt` |
| `--targets-file` | path | Target addresses file | `data/target_addresses.txt` |
| `--output` | path | Output file path | `results/output.json` |
| `--gpu-device` | int | GPU device ID | `0` |
| `--multi-gpu` | flag | Enable multi-GPU mode | - |
| `--gpu-devices` | string | Comma-separated GPU device IDs | `0` |
| `--threads` | int | Number of CPU threads | `4` |
| `--batch-size` | int | Batch size for GPU processing | `1000000` |

#### Performance Options

| Option | Type | Description | Default |
|--------|------|-------------|---------|
| `--thread-block-size` | int | CUDA thread block size | `256` |
| `--memory-limit` | float | Memory limit in GB | `22.0` |
| `--max-streams` | int | Maximum CUDA streams | `4` |
| `--enable-tuning` | flag | Enable auto-tuning | - |
| `--no-shared-memory` | flag | Disable shared memory optimization | - |

#### Security and Validation Options

| Option | Type | Description | Default |
|--------|------|-------------|---------|
| `--validate-ecc` | flag | Validate ECC operations against CPU reference | - |
| `--precision` | float | Validation precision tolerance | `1e-10` |
| `--deterministic` | flag | Enable deterministic mode | - |
| `--check-integrity` | flag | Check file integrity | - |

### Specialized Commands

#### Benchmarking
```bash
./Puzzle71Solver --benchmark [OPTIONS]
```

| Option | Type | Description |
|--------|------|-------------|
| `--duration` | int | Benchmark duration in seconds |
| `--format` | string | Output format (json, csv, console) |
| `--output` | path | Benchmark results file |
| `--warmup` | int | Warmup duration in seconds |

#### Health Check
```bash
./Puzzle71Solver --health-check [OPTIONS]
```

| Option | Type | Description |
|--------|------|-------------|
| `--thresholds` | float | Performance threshold ratio (0.0-1.0) |
| `--gpu-check` | flag | Include GPU health check |
| `--memory-check` | flag | Include memory health check |

#### Configuration Validation
```bash
./Puzzle71Solver --validate-config [OPTIONS]
```

| Option | Type | Description |
|--------|------|-------------|
| `--config-file` | path | Configuration file to validate |
| `--strict` | flag | Enable strict validation |
| `--fix` | flag | Attempt to fix configuration issues |

## Configuration API

### Configuration File Format (YAML)

#### System Configuration
```yaml
system:
  gpu_device_id: 0                    # GPU device to use
  cuda_streams: 4                      # Number of CUDA streams
  thread_block_size: 256               # CUDA thread block size
  max_batch_size: 1000000              # Maximum batch size
  memory_pool_size: 2147483648         # Memory pool size in bytes
  enable_multi_gpu: false              # Multi-GPU mode
  gpu_devices: [0]                     # GPU device list for multi-GPU
```

#### Performance Configuration
```yaml
performance:
  memory_efficiency_target: 95.0       # Target memory efficiency (%)
  gpu_utilization_target: 85.0         # Target GPU utilization (%)
  max_memory_usage_gb: 22.0            # Maximum memory usage (GB)
  auto_tune_performance: true          # Enable auto-tuning
  shared_memory_size: 49152            # Shared memory size (bytes)
  warp_size_optimization: true         # Enable warp optimization
```

#### Security Configuration
```yaml
security:
  validate_ecc_operations: true        # Validate ECC operations
  use_cpu_reference: true              # Use CPU reference validation
  precision_tolerance: 1e-10           # Validation precision tolerance
  enable_deterministic_mode: true      # Deterministic operation mode
  check_file_integrity: true           # Check file integrity
  encrypt_output: false                # Encrypt output files
```

#### Monitoring Configuration
```yaml
monitoring:
  enable_telemetry: true               # Enable telemetry collection
  metrics_interval_sec: 30             # Metrics collection interval
  performance_baselines: "data/performance_baselines.json"
  health_check_interval_sec: 60        # Health check interval
  enable_profiling: false              # Enable profiling
  log_performance_metrics: true        # Log performance metrics
```

### Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `CUDA_VISIBLE_DEVICES` | Visible GPU devices | `0` |
| `CUDA_DEVICE_ORDER` | GPU device order | `PCI_BUS_ID` |
| `CUDA_LAUNCH_BLOCKING` | Blocking CUDA launches | `0` |
| `CUDA_ERROR_LEVEL` | CUDA error level | `0` |
| `PUZZLE71_CONFIG_FILE` | Configuration file override | - |
| `PUZZLE71_LOG_LEVEL` | Log level override | - |
| `PUZZLE71_OUTPUT_DIR` | Output directory override | - |

## Programmatic API

### C++ API

#### Core Classes

##### `Puzzle71Solver` - Main Solver Class
```cpp
#include "Puzzle71Solver.h"

class Puzzle71Solver {
public:
    // Constructor
    Puzzle71Solver(const std::string& config_file = "");

    // Core operations
    bool initialize();
    bool scanRange(const PrivateKeyRange& range, const std::vector<std::string>& targets);
    bool scanMultiGPU(const std::vector<PrivateKeyRange>& ranges, const std::vector<std::string>& targets);

    // Configuration
    bool loadConfiguration(const std::string& config_file);
    bool validateConfiguration() const;

    // Monitoring
    PerformanceMetrics getPerformanceMetrics() const;
    SystemHealth getSystemHealth() const;
    bool isHealthy() const;

    // Results
    std::vector<MatchResult> getResults() const;
    bool saveResults(const std::string& filename) const;

    // Control
    bool start();
    bool pause();
    bool resume();
    bool stop();
    bool reset();

    // Destructor
    ~Puzzle71Solver();
};
```

##### `ConfigurationManager` - Configuration Management
```cpp
#include "ConfigurationManager.h"

class ConfigurationManager {
public:
    ConfigurationManager(const std::string& config_file = "");

    // Loading and validation
    bool loadConfiguration(const std::string& filename);
    bool validateConfiguration() const;
    bool saveConfiguration(const std::string& filename) const;

    // Accessors
    SystemConfig getSystemConfig() const;
    PerformanceConfig getPerformanceConfig() const;
    SecurityConfig getSecurityConfig() const;
    MonitoringConfig getMonitoringConfig() const;

    // Modifiers
    bool setSystemConfig(const SystemConfig& config);
    bool setPerformanceConfig(const PerformanceConfig& config);
    bool setSecurityConfig(const SecurityConfig& config);
    bool setMonitoringConfig(const MonitoringConfig& config);

    // Utilities
    std::string getConfigurationString() const;
    bool resetToDefaults();
};
```

##### `PerformanceMonitor` - Performance Monitoring
```cpp
#include "PerformanceMonitor.h"

class PerformanceMonitor {
public:
    PerformanceMonitor(const MonitoringConfig& config);

    // Monitoring
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;

    // Metrics
    PerformanceMetrics getCurrentMetrics() const;
    std::vector<PerformanceMetrics> getHistoricalMetrics() const;
    PerformanceMetrics getBaselineMetrics() const;

    // Analysis
    bool isPerformanceDegraded(double threshold = 0.9) const;
    std::vector<std::string> getPerformanceIssues() const;
    bool generatePerformanceReport(const std::string& filename) const;

    // Alerts
    void setAlertCallback(std::function<void(const Alert&)> callback);
    std::vector<Alert> getActiveAlerts() const;
};
```

#### Data Structures

##### `PrivateKeyRange`
```cpp
struct PrivateKeyRange {
    uint256_t start;           // Start of range (inclusive)
    uint256_t end;             // End of range (inclusive)
    double weight;             // Search weight (1.0 = normal)
    std::string label;         // Optional label
    std::chrono::system_clock::time_point created_at;
};
```

##### `MatchResult`
```cpp
struct MatchResult {
    uint256_t private_key;     // Matching private key
    std::string address;       // Bitcoin address
    uint256_t public_key;      // Public key
    std::string signature;     // Digital signature
    std::chrono::system_clock::time_point found_at;
    int gpu_device_id;         // GPU device that found the match
    uint64_t iterations;       // Number of iterations to find match
    double computation_time_ms; // Computation time in milliseconds
};
```

##### `PerformanceMetrics`
```cpp
struct PerformanceMetrics {
    // Throughput metrics
    double throughput_keys_per_sec;
    double throughput_addresses_per_sec;

    // Memory metrics
    double memory_efficiency_percent;
    double memory_usage_gb;
    double shared_memory_usage_percent;

    // GPU metrics
    double gpu_utilization_percent;
    double gpu_temperature_celsius;
    double gpu_power_usage_watts;

    // Computation metrics
    double kernel_execution_time_ms;
    double host_to_device_transfer_time_ms;
    double device_to_host_transfer_time_ms;

    // System metrics
    double cpu_utilization_percent;
    double system_memory_usage_percent;

    // Timestamp
    std::chrono::system_clock::time_point timestamp;
};
```

#### Example Usage

##### Basic Usage
```cpp
#include "Puzzle71Solver.h"

int main(int argc, char* argv[]) {
    try {
        // Create solver with default configuration
        Puzzle71Solver solver("data/config.yaml");

        // Initialize
        if (!solver.initialize()) {
            std::cerr << "Failed to initialize solver" << std::endl;
            return 1;
        }

        // Define search range
        PrivateKeyRange range;
        range.start = uint256_t("0000000000000000000000000000000000000000000000000000000000000000");
        range.end = uint256_t("00000000000000000000000000000000000000000000000000000000000ffffffff");
        range.weight = 1.0;

        // Define target addresses
        std::vector<std::string> targets = {
            "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
            "1dice8EMZmqKvrGE4Qc9bUFf9PX3xaYDp"
        };

        // Start scanning
        std::cout << "Starting scan..." << std::endl;
        if (!solver.scanRange(range, targets)) {
            std::cerr << "Scan failed" << std::endl;
            return 1;
        }

        // Get results
        auto results = solver.getResults();
        std::cout << "Found " << results.size() << " matches:" << std::endl;

        for (const auto& result : results) {
            std::cout << "Private Key: " << result.private_key.ToString() << std::endl;
            std::cout << "Address: " << result.address << std::endl;
            std::cout << "Found in: " << result.computation_time_ms << "ms" << std::endl;
            std::cout << "---" << std::endl;
        }

        // Save results
        if (!results.empty()) {
            solver.saveResults("results/matches.json");
        }

        // Performance metrics
        auto metrics = solver.getPerformanceMetrics();
        std::cout << "Performance: " << metrics.throughput_keys_per_sec << " keys/sec" << std::endl;
        std::cout << "Memory efficiency: " << metrics.memory_efficiency_percent << "%" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

##### Multi-GPU Usage
```cpp
#include "Puzzle71Solver.h"

int main() {
    // Create solver for multi-GPU operation
    Puzzle71Solver solver("data/multi_gpu_config.yaml");

    if (!solver.initialize()) {
        return 1;
    }

    // Define multiple ranges for different GPUs
    std::vector<PrivateKeyRange> ranges;

    // Range for GPU 0
    PrivateKeyRange range0;
    range0.start = uint256_t("0000000000000000000000000000000000000000000000000000000000000000");
    range0.end = uint256_t("0000000000000000000000000000000000000000000000000000000080000000");
    range0.weight = 1.0;
    ranges.push_back(range0);

    // Range for GPU 1
    PrivateKeyRange range1;
    range1.start = uint256_t("0000000000000000000000000000000000000000000000000000000080000001");
    range1.end = uint256_t("00000000000000000000000000000000000000000000000000000000ffffffff");
    range1.weight = 1.0;
    ranges.push_back(range1);

    // Target addresses
    std::vector<std::string> targets = {
        "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa"
    };

    // Scan with multi-GPU
    if (solver.scanMultiGPU(ranges, targets)) {
        auto results = solver.getResults();
        std::cout << "Multi-GPU scan found " << results.size() << " matches" << std::endl;
    }

    return 0;
}
```

### Python API (via ctypes)

#### Basic Python Wrapper
```python
import ctypes
import numpy as np
from typing import List, Dict, Optional

class Puzzle71Solver:
    def __init__(self, config_file: str = "data/config.yaml"):
        self.lib = ctypes.CDLL("./libPuzzle71Solver.so")
        self._setup_function_signatures()
        self.solver = self.lib.Puzzle71Solver_create(config_file.encode())

    def _setup_function_signatures(self):
        # Define function signatures
        self.lib.Puzzle71Solver_create.argtypes = [ctypes.c_char_p]
        self.lib.Puzzle71Solver_create.restype = ctypes.c_void_p

        self.lib.Puzzle71Solver_initialize.argtypes = [ctypes.c_void_p]
        self.lib.Puzzle71Solver_initialize.restype = ctypes.c_bool

        self.lib.Puzzle71Solver_scan_range.argtypes = [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_char_p
        ]
        self.lib.Puzzle71Solver_scan_range.restype = ctypes.c_bool

        self.lib.Puzzle71Solver_get_results.argtypes = [ctypes.c_void_p]
        self.lib.Puzzle71Solver_get_results.restype = ctypes.c_void_p

        self.lib.Puzzle71Solver_get_metrics.argtypes = [ctypes.c_void_p]
        self.lib.Puzzle71Solver_get_metrics.restype = ctypes.c_void_p

    def initialize(self) -> bool:
        return self.lib.Puzzle71Solver_initialize(self.solver)

    def scan_range(self, start_hex: str, end_hex: str, targets: List[str]) -> bool:
        targets_str = ",".join(targets)
        return self.lib.Puzzle71Solver_scan_range(
            self.solver,
            start_hex.encode(),
            end_hex.encode(),
            targets_str.encode()
        )

    def get_results(self) -> List[Dict]:
        """Get scan results"""
        results_ptr = self.lib.Puzzle71Solver_get_results(self.solver)
        # Implementation for parsing results would go here
        return []

    def get_performance_metrics(self) -> Dict:
        """Get performance metrics"""
        metrics_ptr = self.lib.Puzzle71Solver_get_metrics(self.solver)
        # Implementation for parsing metrics would go here
        return {}

    def __del__(self):
        if hasattr(self, 'solver') and self.solver:
            self.lib.Puzzle71Solver_destroy(self.solver)
```

#### Python Usage Example
```python
from puzzle71_solver import Puzzle71Solver

def main():
    # Create solver instance
    solver = Puzzle71Solver("data/config.yaml")

    # Initialize
    if not solver.initialize():
        print("Failed to initialize solver")
        return

    # Define scan range
    start_hex = "0000000000000000000000000000000000000000000000000000000000000000"
    end_hex = "00000000000000000000000000000000000000000000000000000000000ffffffff"

    # Target addresses
    targets = [
        "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
        "1dice8EMZmqKvrGE4Qc9bUFf9PX3xaYDp"
    ]

    # Start scanning
    print("Starting scan...")
    if solver.scan_range(start_hex, end_hex, targets):
        # Get results
        results = solver.get_results()
        print(f"Found {len(results)} matches")

        # Get performance metrics
        metrics = solver.get_performance_metrics()
        print(f"Throughput: {metrics.get('throughput', 0):.2f} keys/sec")
    else:
        print("Scan failed")

if __name__ == "__main__":
    main()
```

## Error Handling

### Error Codes

| Code | Description | Recovery |
|------|-------------|-----------|
| `0` | Success | - |
| `1` | General Error | Check logs |
| `100` | Configuration Error | Validate configuration file |
| `101` | GPU Initialization Error | Check GPU drivers |
| `102` | Memory Allocation Error | Reduce batch size |
| `103` | CUDA Runtime Error | Check CUDA installation |
| `104` | File I/O Error | Check file permissions |
| `105` | Validation Error | Check input data |

### Exception Handling

#### C++ Exceptions
```cpp
try {
    Puzzle71Solver solver("config.yaml");
    solver.initialize();
    solver.scanRange(range, targets);
} catch (const ConfigurationException& e) {
    std::cerr << "Configuration error: " << e.what() << std::endl;
} catch (const GPUException& e) {
    std::cerr << "GPU error: " << e.what() << std::endl;
} catch (const ValidationException& e) {
    std::cerr << "Validation error: " << e.what() << std::endl;
} catch (const std::exception& e) {
    std::cerr << "General error: " << e.what() << std::endl;
}
```

## Performance Optimization

### Recommended Settings for Different Use Cases

#### Maximum Performance
```yaml
performance:
  memory_efficiency_target: 98.0
  gpu_utilization_target: 95.0
  max_memory_usage_gb: 24.0
  auto_tune_performance: true
  shared_memory_size: 65536
  warp_size_optimization: true
  enable_profiling: false
```

#### Energy Efficiency
```yaml
performance:
  memory_efficiency_target: 90.0
  gpu_utilization_target: 75.0
  max_memory_usage_gb: 16.0
  auto_tune_performance: false
  shared_memory_size: 32768
  warp_size_optimization: true
  enable_profiling: false
```

#### Reliability Focus
```yaml
performance:
  memory_efficiency_target: 95.0
  gpu_utilization_target: 80.0
  max_memory_usage_gb: 20.0
  auto_tune_performance: false
  shared_memory_size: 49152
  warp_size_optimization: true
  enable_profiling: true

security:
  validate_ecc_operations: true
  use_cpu_reference: true
  precision_tolerance: 1e-12
  enable_deterministic_mode: true
  check_file_integrity: true
```

---

**API Reference v3.0**
**Last Updated**: 2025-10-21
**Next Review**: 2025-11-21
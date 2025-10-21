# Puzzle71Solver Modern Scripts Framework

## Overview

The Puzzle71Solver Modern Scripts Framework provides a comprehensive, standardized approach to building, testing, profiling, and monitoring the CUDA-based Bitcoin private key scanner. This framework replaces ad-hoc scripts with a unified, professional toolset that supports technical debt elimination and performance optimization goals.

## Architecture

### Directory Structure

```
scripts/modern/
├── scripts                 # Main entry point script
├── utils.sh               # Core utility library
├── README.md              # This documentation
├── workflow/              # Development workflow scripts
│   ├── setup.sh          # Environment initialization
│   ├── build.sh          # Build system with optimizations
│   ├── test.sh           # Comprehensive testing
│   ├── benchmark.sh      # Performance benchmarking
│   ├── profile.sh        # CUDA profiling
│   └── clean.sh          # Cleanup utilities
├── automation/            # Automation and CI/CD scripts
│   ├── validate.sh       # Code quality and validation
│   ├── monitor.sh        # Performance monitoring
│   └── deploy.sh         # Deployment automation
├── testing/               # Testing framework scripts
│   ├── unit.sh           # Unit test runner
│   ├── integration.sh    # Integration test runner
│   └── regression.sh     # Regression testing
└── ci/                   # CI/CD pipeline scripts
    ├── github-actions.sh # GitHub Actions integration
    ├── performance-gate.sh # Performance validation
    └── security-scan.sh  # Security scanning
```

### Design Principles

1. **Unified Interface**: Single entry point (`scripts`) for all operations
2. **Consistent Patterns**: Standardized argument parsing and error handling
3. **Comprehensive Logging**: Structured logging with multiple verbosity levels
4. **Environment Detection**: Automatic OS and GPU capability detection
5. **Safety First**: Dry-run modes and confirmation prompts for destructive operations
6. **Performance Focus**: Built-in profiling and regression detection
7. **CI/CD Ready**: JSON/JUnit output formats for automated pipelines

## Quick Start

### Installation

1. **Clone the repository** (if not already done):
   ```bash
   git clone <repository-url>
   cd PuzzleKeyhunt
   ```

2. **Initialize development environment**:
   ```bash
   ./scripts/modern/scripts setup
   ```

3. **Build the project**:
   ```bash
   ./scripts/modern/scripts build
   ```

4. **Run tests**:
   ```bash
   ./scripts/modern/scripts test
   ```

### Basic Usage

The main `scripts` entry point provides access to all functionality:

```bash
# Show help
./scripts/modern/scripts --help

# Development workflow
./scripts/modern/scripts setup           # Initialize environment
./scripts/modern/scripts build           # Build project
./scripts/modern/scripts test            # Run tests
./scripts/modern/scripts benchmark       # Run benchmarks
./scripts/modern/scripts profile         # Profile CUDA kernels
./scripts/modern/scripts clean           # Clean build artifacts

# Automation and monitoring
./scripts/modern/scripts validate        # Validate code quality
./scripts/modern/scripts monitor         # Performance monitoring
./scripts/modern/scripts deploy          # Create deployment packages

# Utilities
./scripts/modern/scripts env             # Show environment information
./scripts/modern/scripts logs            # View application logs
./scripts/modern/scripts config          # Manage configuration
./scripts/modern/scripts docs            # Generate documentation
```

## Detailed Usage

### Development Workflow

#### Environment Setup

```bash
# Standard setup
./scripts/modern/scripts setup

# Advanced setup with options
./scripts/modern/scripts setup \
    --build-type Release \
    --cuda-archs "75;86;89;90" \
    --offline \
    --verbose
```

**Setup Features:**
- Automatic dependency installation
- CUDA toolkit detection and configuration
- Build directory structure creation
- Development tools initialization
- Git hooks installation

#### Building

```bash
# Standard build
./scripts/modern/scripts build

# Optimized build
./scripts/modern/scripts build --optimize --build-type Release

# Parallel build with 8 jobs
./scripts/modern/scripts build --jobs 8

# Clean build
./scripts/modern/scripts build --clean
```

**Build Features:**
- Aggressive optimization flags (-O3, -maxrregcount, etc.)
- CUDA architecture-specific optimizations
- Parallel compilation support
- Incremental builds with dependency tracking
- Build-time performance validation

#### Testing

```bash
# Run all tests
./scripts/modern/scripts test

# Run specific test categories
./scripts/modern/scripts test unit
./scripts/modern/scripts test integration
./scripts/modern/scripts test performance

# Parallel testing with 8 jobs
./scripts/modern/scripts test --parallel 8

# Performance regression testing
./scripts/modern/scripts test --baseline baseline.json
```

**Testing Features:**
- Unit, integration, and performance testing
- Parallel test execution
- Baseline comparison and regression detection
- Multiple output formats (JSON, XML, Markdown)
- GPU-specific test validation

#### Benchmarking

```bash
# Standard 10-minute benchmark
./scripts/modern/scripts benchmark

# Custom benchmark duration
./scripts/modern/scripts benchmark --duration 300

# GPU-specific benchmark
./scripts/modern/scripts benchmark --gpu 0

# Performance baseline comparison
./scripts/modern/scripts benchmark --baseline rtx3090_baseline.json
```

**Benchmarking Features:**
- Sustained 10-minute benchmarking (as per requirements)
- Real-time telemetry collection
- GPU utilization and memory bandwidth monitoring
- Automatic baseline generation and comparison
- Performance regression detection

#### Profiling

```bash
# Basic profiling
./scripts/modern/scripts profile

# Nsight Compute profiling
./scripts/modern/scripts profile --nsight --visualize

# Profile specific kernel
./scripts/modern/scripts profile --kernel eccScalarMul

# Memory access profiling
./scripts/modern/scripts profile --type memory
```

**Profiling Features:**
- Nsight Compute integration
- Kernel-level performance analysis
- Memory access pattern analysis
- Power consumption profiling
- Automated report generation

### Automation and Monitoring

#### Validation

```bash
# Standard validation
./scripts/modern/scripts validate

# Comprehensive validation with fixes
./scripts/modern/scripts validate --level comprehensive --fix

# CI/CD mode validation
./scripts/modern/scripts validate --ci-mode --format json
```

**Validation Features:**
- Code quality analysis (clang-format, cppcheck, clang-tidy)
- Performance regression validation
- Security vulnerability scanning
- Documentation validation
- Platform compatibility checking

#### Monitoring

```bash
# Start 1-hour monitoring session
./scripts/modern/scripts monitor start --duration 3600

# Continuous monitoring with web interface
./scripts/modern/scripts monitor start --continuous --web-interface

# Monitor specific metrics
./scripts/modern/scripts monitor start --metrics "gpu-utilization,temperature,power"
```

**Monitoring Features:**
- Real-time GPU performance monitoring
- Configurable metrics collection
- Web-based monitoring dashboard
- Performance alerting
- Historical data analysis

## Configuration

### Environment Variables

Key environment variables that control script behavior:

```bash
# Build configuration
export BUILD_TYPE=Release
export BUILD_JOBS=8
export CUDA_ARCHS="75;86;89;90"

# Performance settings
export BENCHMARK_DURATION=600
export BENCHMARK_GPU=0
export BENCHMARK_OUTPUT_DIR="./benchmarks"

# Validation settings
export VALIDATION_LEVEL=comprehensive
export VALIDATION_OUTPUT_DIR="./validation"
```

### Configuration Files

- `scripts.conf`: Main configuration file (auto-generated)
- `build/config.txt`: CUDA runtime configuration
- `data/config.txt`: Application configuration

## Integration with Build Systems

### CMake Integration

The scripts integrate seamlessly with the existing CMake build system:

```cmake
# Custom targets in CMakeLists.txt
add_custom_target(run-benchmarks
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/modern/scripts benchmark
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(validate-code
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/modern/scripts validate
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
```

### Makefile Integration

```makefile
# Makefile targets
.PHONY: benchmark validate monitor

benchmark:
	./scripts/modern/scripts benchmark

validate:
	./scripts/modern/scripts validate

monitor:
	./scripts/modern/scripts monitor start --continuous
```

## CI/CD Integration

### GitHub Actions

```yaml
name: Performance Validation
on: [push, pull_request]

jobs:
  benchmark:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Setup CUDA
        uses: Jimver/cuda-toolkit@v0.2.4
      - name: Build and Benchmark
        run: |
          ./scripts/modern/scripts setup
          ./scripts/modern/scripts build
          ./scripts/modern/scripts benchmark --ci-mode
```

### Performance Gates

```bash
# CI performance validation
./scripts/modern/scripts validate \
    --level comprehensive \
    --baseline ci_baseline.json \
    --regression-check \
    --ci-mode
```

## Troubleshooting

### Common Issues

1. **CUDA not found**:
   ```bash
   # Check CUDA installation
   nvcc --version

   # Verify CUDA paths
   echo $CUDA_HOME
   which nvcc
   ```

2. **Build failures**:
   ```bash
   # Clean build
   ./scripts/modern/scripts clean --force

   # Check dependencies
   ./scripts/modern/scripts validate build
   ```

3. **Performance issues**:
   ```bash
   # Check GPU status
   nvidia-smi

   # Run profiling
   ./scripts/modern/scripts profile --nsight

   # Monitor performance
   ./scripts/modern/scripts monitor start --duration 300
   ```

### Debug Mode

Enable debug output for troubleshooting:

```bash
# Enable debug logging
export DEBUG=true
export VERBOSE=true

# Run with verbose output
./scripts/modern/scripts build --verbose
./scripts/modern/scripts test --verbose
```

### Log Files

Important log locations:

- `build/logs/`: Build and test logs
- `build/validation/`: Validation reports
- `build/benchmarks/`: Benchmark results
- `build/monitoring/`: Monitoring data

## Performance Optimization

### Build Optimization

```bash
# Maximum optimization
./scripts/modern/scripts build \
    --optimize-speed \
    --build-type Release \
    --gpu-archs "native"
```

### Runtime Optimization

```bash
# Memory optimization
./scripts/modern/scripts benchmark --optimize-memory

# Speed optimization
./scripts/modern/scripts benchmark --optimize-speed

# GPU utilization optimization
./scripts/modern/scripts profile --type occupancy
```

## Security

### Security Validation

```bash
# Comprehensive security check
./scripts/modern/scripts validate --level comprehensive --security-scan

# Dependency vulnerability check
./scripts/modern/scripts validate --dependency-check
```

### Safe Script Execution

- All scripts use `set -euo pipefail` for safety
- Destructive operations require confirmation
- Dry-run modes available for testing
- Comprehensive input validation

## Contributing

### Script Development Guidelines

1. **Follow existing patterns** for argument parsing and error handling
2. **Use utility functions** from `utils.sh` for common operations
3. **Add comprehensive help** documentation for new scripts
4. **Include logging** at appropriate levels (info, warning, error)
5. **Test scripts** with `--dry-run` and `--verbose` modes
6. **Document configuration options** and environment variables

### Adding New Scripts

1. Create script in appropriate directory (`workflow/`, `automation/`, etc.)
2. Include standard script header with description
3. Use utility functions for common operations
4. Add help documentation and argument parsing
5. Include error handling and validation
6. Update main `scripts` entry point if needed
7. Add tests and documentation

## License and Credits

This script framework is part of the Puzzle71Solver project for Bitcoin private key scanning using CUDA acceleration.

**Technical Debt Elimination Focus:**
- Code deduplication and modularization
- Performance optimization and monitoring
- Automated validation and regression detection
- Comprehensive documentation and testing

For more information about the Puzzle71Solver project, see the main project documentation.
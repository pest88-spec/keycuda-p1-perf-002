# Quickstart Guide: Puzzle71 Technical Debt Repair System

**Version**: 1.0.0
**Created**: 2025-10-20
**Purpose**: Quick start guide for implementing and validating technical debt repairs

## Prerequisites

### Hardware Requirements
- NVIDIA GPU with Compute Capability 3.5 or higher
- Minimum 8GB GPU memory for development
- Recommended 16GB+ GPU memory for production testing

### Software Requirements
- CUDA Toolkit 11.0 or higher
- CMake 3.18 or higher
- C++17 compatible compiler (GCC 9+, Clang 10+)
- GoogleTest framework
- Docker (for containerized testing)
- NVIDIA Nsight Compute (for performance profiling)

### System Requirements
- Linux operating system (Ubuntu 18.04+ recommended)
- 16GB+ system RAM
- 100GB+ free disk space

## Installation

### 1. Clone and Setup Repository
```bash
git clone <repository-url>
cd PuzzleKeyhunt
git checkout 002-techdebt-repair
```

### 2. Install Dependencies
```bash
# Install CUDA Toolkit (if not already installed)
# Follow NVIDIA's official installation guide for your OS

# Install CMake
sudo apt-get install cmake

# Install GoogleTest
sudo apt-get install libgtest-dev libgmock-dev

# Install Docker
sudo apt-get install docker.io
sudo systemctl start docker
sudo systemctl enable docker
```

### 3. Build the Project
```bash
mkdir build && cd build
cmake ../src/KeyhuntCore -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
make -j$(nproc)
```

### 4. Run Initial Tests
```bash
# Verify build success
./tests/unit/techdebt_unit_tests

# Run basic functionality tests
./tests/integration/techdebt_integration_tests
```

## Configuration

### 1. Create Configuration File
```bash
mkdir -p config
cp config/puzzle71.yaml.example config/puzzle71.yaml
```

### 2. Edit Configuration
Edit `config/puzzle71.yaml` with your specific settings:

```yaml
deterministic_config:
  version: "5.5"
  kernel_launch:
    grid_dim: 1024
    block_dim: 256
    points_per_thread: 8
    shared_mem_bytes: 49152
  rng:
    algorithm: "xorshift64"
    base_seed: 0x123456789ABCDEF0
    derivation: "replay_seed + blockIdx.x * blockDim.x + threadIdx.x"
  memory_layout:
    scalar_format: "little_endian_u32x8"
    point_format: "jacobian_projective"

performance:
  checkpoint_interval_sec: 1800
  telemetry_interval_sec: 1
  benchmark:
    warmup_iterations: 3
    measurement_iterations: 5
    discard_warmup: true

operator:
  default_id: "developer"
  default_purpose: "testing"
  require_explicit: true
```

### 3. Validate Configuration
```bash
./build/tools/config_validator config/puzzle71.yaml
```

## Development Workflow

### 1. Set Up Development Environment
```bash
# Create development branch
git checkout -b dev/techdebt-repair-<your-name>

# Set up pre-commit hooks
./scripts/setup/pre-commit-hooks.sh
```

### 2. Test-Driven Development Process

#### Step 1: Write Failing Tests
```bash
# Create test file for new functionality
vim tests/unit/test_new_ecc_operations.cpp

# Write tests that will initially fail
./build/tests/unit/test_new_ecc_operations --gtest_filter="NewECCOperations.*"
```

#### Step 2: Save Test Evidence
```bash
# Save failing test output as evidence
mkdir -p docs/validation/evidence
./build/tests/unit/test_new_ecc_operations 2>&1 | tee docs/validation/evidence/TXXX_test_failures.log
```

#### Step 3: Implement Solution
```bash
# Implement the actual functionality
vim src/KeyhuntCore/common/ecc_operations_fixed.cuh
vim src/KeyhuntCore/common/ecc_operations_fixed.cu
```

#### Step 4: Verify Tests Pass
```bash
# Build and run tests
make -j$(nproc)
./build/tests/unit/test_new_ecc_operations
```

### 3. Performance Validation

#### Run Benchmarks
```bash
# Execute performance benchmarks
./scripts/run-benchmarks.sh 0 5

# View results
cat benchmarks/run_*.json | jq '.measurement.statistics'
```

#### Profile with Nsight
```bash
# Profile kernel performance
./scripts/tools/nsight_profiling.sh 0 eccScalarMulKernel

# Analyze results
ncu --set full ./build/Puzzle71Solver --benchmark
```

### 4. Constitutional Compliance Check

#### Run Compliance Validation
```bash
# Check constitutional compliance
./scripts/ci/constitution_compliance.sh

# Verify v5.5 constraints
./scripts/ci/v5_5_constraints_check.sh
```

#### Validate Deterministic Replay
```bash
# Test deterministic replay
./scripts/test/deterministic_replay_test.sh
```

## Testing Strategies

### Unit Testing
```bash
# Run all unit tests
./build/tests/unit/techdebt_unit_tests

# Run specific test categories
./build/tests/unit/techdebt_unit_tests --gtest_filter="ECCOperations.*"
./build/tests/unit/techdebt_unit_tests --gtest_filter="MemoryAccess.*"
./build/tests/unit/techdebt_unit_tests --gtest_filter="Configuration.*"
```

### Integration Testing
```bash
# Run end-to-end integration tests
./build/tests/integration/techdebt_integration_tests

# Test specific workflows
./build/tests/integration/end_to_end_kernel_test
./build/tests/integration/deterministic_replay_test
```

### Performance Testing
```bash
# Run performance regression tests
./scripts/ci/performance_gate.sh

# Generate performance reports
./scripts/tools/generate_performance_report.sh
```

### Compliance Testing
```bash
# Validate constitutional compliance
./build/tests/validation/constitutional_compliance_test

# Check v5.5 constraint compliance
./build/tests/validation/v5_5_constraints_test
```

## Docker Testing

### Build Docker Image
```bash
# Build testing container
docker build -t puzzle71-techdebt-test .

# Run tests in container
docker run --gpus all -v $(pwd):/workspace puzzle71-techdebt-test
```

### CI/CD Integration
```bash
# Test Jenkins pipeline locally
docker run -p 8080:8080 jenkins/jenkins:lts

# Execute Jenkins job
curl -X POST http://localhost:8080/job/techdebt-repair/build
```

## Troubleshooting

### Common Build Issues

#### CUDA Not Found
```bash
# Check CUDA installation
nvcc --version
echo $CUDA_HOME

# Fix CUDA path issues
export CUDA_HOME=/usr/local/cuda
export PATH=$CUDA_HOME/bin:$PATH
export LD_LIBRARY_PATH=$CUDA_HOME/lib64:$LD_LIBRARY_PATH
```

#### CMake Configuration Errors
```bash
# Clean build directory
rm -rf build
mkdir build && cd build

# Configure with verbose output
cmake ../src/KeyhuntCore -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=ON
```

### Runtime Issues

#### GPU Memory Errors
```bash
# Check GPU memory usage
nvidia-smi

# Monitor during execution
watch -n 1 nvidia-smi
```

#### Performance Degradation
```bash
# Check GPU utilization
nvidia-smi dmon -s u

# Profile bottlenecks
./scripts/tools/profile_bottlenecks.sh
```

### Testing Failures

#### Test Timeout Issues
```bash
# Increase test timeout
export GTEST_TIMEOUT=30000

# Run tests with verbose output
./build/tests/unit/techdebt_unit_tests --gtest_output=xml
```

#### Flaky Tests
```bash
# Run tests multiple times
for i in {1..5}; do
  ./build/tests/unit/techdebt_unit_tests || echo "Test run $i failed"
done
```

## Performance Optimization

### Memory Optimization
```bash
# Check memory access patterns
./scripts/tools/analyze_memory_access.sh

# Optimize shared memory usage
./scripts/tools/optimize_shared_memory.sh
```

### Kernel Optimization
```bash
# Analyze kernel performance
./scripts/tools/analyze_kernel_performance.sh

# Optimize launch parameters
./scripts/tools/optimize_launch_params.sh
```

## Monitoring and Logging

### Enable Performance Monitoring
```bash
# Start performance monitoring
./scripts/monitoring/start_performance_monitor.sh

# View telemetry data
tail -f telemetry/telemetry_*.jsonl
```

### Configure Logging
```bash
# Set log level
export LOG_LEVEL=DEBUG

# Enable detailed logging
export PUZZLE71_DEBUG=1
```

## Getting Help

### Documentation
- [Complete API Reference](contracts/README.md)
- [Performance Tuning Guide](../docs/performance_tuning.md)
- [Troubleshooting Guide](../docs/troubleshooting.md)

### Support
- Check [GitHub Issues](https://github.com/project/issues) for known problems
- Join the [developer community](https://discord.gg/project) for discussions
- Review [technical debt audit](../audits/puzzle71_techdebt_audit_v5.5.md) for context

## Next Steps

1. **Complete Phase 1**: Implement all core technical debt repairs
2. **Run Performance Validation**: Ensure all performance targets are met
3. **Execute Integration Testing**: Verify end-to-end functionality
4. **Compliance Verification**: Confirm constitutional v5.5 compliance
5. **Documentation Updates**: Update all relevant documentation
6. **Production Deployment**: Deploy validated fixes to production environment

## Success Criteria

You have successfully completed the technical debt repair when:
- ✅ All P0 blocking issues are resolved
- ✅ All P1 high priority issues are resolved
- ✅ Memory efficiency >90% (target 95%+)
- ✅ GPU utilization ≥70% (target 80%+)
- ✅ Synchronization overhead ≤50% of baseline
- ✅ All constitutional v5.5 constraints are met
- ✅ Comprehensive test coverage is achieved
- ✅ Performance regression tests pass
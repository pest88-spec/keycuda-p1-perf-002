# TDD ECC Operations Implementation Guide

## Overview

This document describes the Test-Driven Development (TDD) approach for implementing ECC operations in `src/KeyhuntCore/common/ecc_operations_fixed.cuh`. The tests in `tests/unit/test_ecc_operations.cpp` are designed to **FAIL initially** and will only pass after proper implementation.

## TDD Phases

### Phase 1: RED (Current State)
- All tests are disabled using `DISABLED_` prefix
- Tests will fail when enabled because implementation is missing
- This establishes the requirements and failure conditions

### Phase 2: GREEN (Implementation)
- Implement minimal code to make tests pass
- Focus on functionality over optimization
- Ensure all tests succeed

### Phase 3: REFACTOR (Optimization)
- Optimize implementation while maintaining test coverage
- Improve performance, memory efficiency, and code quality
- All tests must continue to pass

## Test Structure

### Test Categories

1. **Initialization Tests** (`DISABLED_Initialization`)
   - Test proper initialization with valid configuration
   - Test configuration validation and storage

2. **Core ECC Operations**
   - `DISABLED_BatchScalarMultiplication`: Main ECC operation P = k * G
   - `DISABLED_BatchPointAddition`: Point addition R = P + Q
   - `DISABLED_BatchPointDoubling`: Point doubling R = 2 * P

3. **Memory Optimization**
   - `DISABLED_MemoryLayoutOptimization`: Structure-of-Arrays layout
   - Memory efficiency (>90%) and coalescing verification

4. **Performance Testing**
   - `DISABLED_PerformanceBenchmarks`: Throughput (>1M ops/sec)
   - Large batch processing and timing validation

5. **Error Handling**
   - `DISABLED_ErrorHandlingAndValidation`: Edge cases and validation
   - Zero keys, invalid keys, error reporting

6. **Advanced Features**
   - `DISABLED_PointValidation`: Point validation on curve
   - `DISABLED_MemoryManagement`: Large allocations and cleanup
   - `DISABLED_DeterministicReplay`: Consistent results
   - `DISABLED_MultiGPUSupport`: Multi-GPU operations
   - `DISABLED_ConstitutionalCompliance`: No crypto reimplementation

## Implementation Requirements

### Constitutional Compliance

**No Cryptographic Reimplementation**: All ECC operations must use bitcoin-core/secp256k1 as CPU reference validation. The GPU implementation must achieve <1e-10 precision relative to the CPU reference.

### Memory Layout Requirements

```cpp
// Structure-of-Arrays layout (required for performance)
struct alignas(128) ECCPointSoA {
    uint32_t* x_words;      // Separate X coordinate array
    uint32_t* y_words;      // Separate Y coordinate array
    bool* is_valid;         // Validity flags
    size_t size;
};
```

### Performance Requirements

- **Throughput**: >1,000,000 operations/second
- **Memory Efficiency**: >90%
- **GPU Utilization**: >70%
- **Precision**: <1e-10 relative error vs CPU reference
- **Alignment**: 128-byte memory alignment
- **Batch Size**: Support for up to 100,000 operations

### ECC Operations Interface

```cpp
class ECCOperationsFixed {
public:
    // Core operations
    bool scalar_multiply_batch(const uint32_t* private_keys,
                               ECCPointSoA* public_keys,
                               size_t batch_size,
                               ECCOperationResult& result);

    bool point_addition_batch(const ECCPointSoA* points_p,
                             const ECCPointSoA* points_q,
                             ECCPointSoA* points_r,
                             size_t batch_size,
                             ECCOperationResult& result);

    bool point_doubling_batch(const ECCPointSoA* points_p,
                             ECCPointSoA* points_r,
                             size_t batch_size,
                             ECCOperationResult& result);

    // Validation
    bool validate_against_cpu_reference(const uint32_t* private_keys,
                                        const ECCPointSoA* gpu_public_keys,
                                        size_t batch_size,
                                        double& max_relative_error);

    // Memory management
    bool allocate_soa_points(ECCPointSoA* points, size_t size);
    bool free_soa_points(ECCPointSoA* points);
    bool optimize_memory_layout();

    // Performance
    bool benchmark_operations(double& throughput, double& efficiency);
    double calculate_memory_efficiency();
    double calculate_gpu_utilization();
};
```

## Running Tests

### Enable Tests (Start TDD RED Phase)

```bash
# Enable all disabled tests
sed -i 's/DISABLED_//' tests/unit/test_ecc_operations.cpp

# Run the test script
./scripts/run_failing_ecc_tests.sh
```

### Expected Output (RED Phase)

```
================================================
Running ECC Operations TDD Tests (RED Phase)
================================================
Expected: All tests should FAIL before implementation
================================================

[ RUN      ] ECCOperationsTest.Initialization
[  FAILED  ] ECCOperationsTest.Initialization (0 ms)
[ RUN      ] ECCOperationsTest.BatchScalarMultiplication
[  FAILED  ] ECCOperationsTest.BatchScalarMultiplication (0 ms)
...
[  FAILED  ] ECCOperationsTest.ConstitutionalCompliance (0 ms)

✓ EXPECTED: Tests failed (RED phase)
```

## Implementation Roadmap

### Step 1: Basic Class Structure
1. Implement `ECCOperationsFixed` constructor/destructor
2. Add basic member variables and initialization
3. Implement configuration validation

### Step 2: Memory Management
1. Implement `allocate_soa_points()` and `free_soa_points()`
2. Add 128-byte aligned memory allocation
3. Implement memory layout optimization

### Step 3: CUDA Kernels
1. Implement `ecc_scalar_mul_kernel()` for batch scalar multiplication
2. Implement `ecc_point_add_kernel()` for point addition
3. Implement `ecc_point_double_kernel()` for point doubling
4. Add error handling and synchronization

### Step 4: CPU Reference Integration
1. Integrate secp256k1 for CPU reference calculations
2. Implement `validate_against_cpu_reference()`
3. Ensure <1e-10 precision requirements

### Step 5: Performance Optimization
1. Optimize memory coalescing
2. Implement shared memory optimization
3. Optimize kernel launch parameters
4. Achieve >1M ops/sec throughput

### Step 6: Advanced Features
1. Add point validation on curve
2. Implement deterministic replay
3. Add multi-GPU support
4. Implement comprehensive error handling

## Validation Requirements

### CPU/GPU Consistency

Every GPU ECC operation must be validated against the CPU reference:

```cpp
// Example validation pattern
double max_relative_error = 0.0;
bool validation_success = ecc_ops_->validate_against_cpu_reference(
    gpu_private_keys, &gpu_points, batch_size, max_relative_error);

ASSERT_EQ(validation_success, true);
ASSERT_LT(max_relative_error, 1e-10);  // Constitutional requirement
```

### Performance Validation

```cpp
// Example performance requirements
EXPECT_GT(result.throughput_ops_per_sec, 1000000.0);  // >1M ops/sec
EXPECT_GT(result.memory_efficiency_percent, 90.0f);   // >90% efficiency
EXPECT_GT(result.gpu_utilization_percent, 70.0f);     // >70% utilization
EXPECT_LT(result.precision_achieved, 1e-10);          // <1e-10 precision
```

## Debugging Tips

### Common Issues

1. **Compilation Errors**: Check CUDA toolkit installation and compute capabilities
2. **Memory Errors**: Verify proper CUDA memory allocation and deallocation
3. **Precision Issues**: Validate against secp256k1 CPU reference implementation
4. **Performance Issues**: Profile with Nsight Compute for optimization opportunities

### Debugging Commands

```bash
# Check CUDA installation
nvcc --version
nvidia-smi

# Build with debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON

# Run with CUDA memory checking
cuda-memcheck ./build/techdebt_repair_tests --gtest_filter="*ECCOperationsTest*"

# Profile kernels
nvprof ./build/techdebt_repair_tests --gtest_filter="*ECCOperationsTest*BatchScalarMultiplication"
```

## Success Criteria

The implementation is complete when:

1. ✅ All tests pass without DISABLED_ prefix
2. ✅ CPU/GPU precision <1e-10 relative error
3. ✅ Throughput >1M operations/second
4. ✅ Memory efficiency >90%
5. ✅ GPU utilization >70%
6. ✅ No cryptographic reimplementation (uses secp256k1 reference)
7. ✅ Structure-of-Arrays memory layout implemented
8. ✅ Proper error handling and edge case validation
9. ✅ Deterministic replay works consistently
10. ✅ Multi-GPU support functional (when multiple GPUs available)

## Files to Implement

1. **Primary**: `src/KeyhuntCore/common/ecc_operations_fixed.cuh`
2. **Implementation**: `src/KeyhuntCore/common/ecc_operations_fixed.cu`
3. **CUDA Kernels**: Embedded in the .cu file
4. **Tests**: `tests/unit/test_ecc_operations.cpp` (already created)

## References

- [GoogleTest Documentation](https://google.github.io/googletest/)
- [CUDA Programming Guide](https://docs.nvidia.com/cuda/cuda-c-programming-guide/)
- [secp256k1 Bitcoin Core](https://github.com/bitcoin-core/secp256k1)
- [Project CLAUDE.md](../CLAUDE.md) for architectural guidance
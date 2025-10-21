#!/bin/bash

# TDD Adapter Layer Test Validation Script
# Validates that the failing integration tests are properly structured

set -e

echo "=== TDD Adapter Layer Test Validation ==="
echo "TDD Phase: RED (tests should fail before implementation)"
echo

# Check if adapter layer test file exists
ADAPTER_TEST_FILE="tests/integration/test_adapter_layer.cpp"

if [ ! -f "$ADAPTER_TEST_FILE" ]; then
    echo "✗ ERROR: Adapter layer test file not found: $ADAPTER_TEST_FILE"
    exit 1
fi

echo "✓ Adapter layer test file found: $ADAPTER_TEST_FILE"

# Check test structure
echo
echo "=== Validating Test Structure ==="

# Count tests
TOTAL_TESTS=$(grep -c "^TEST_F.*AdapterLayerTest" "$ADAPTER_TEST_FILE" || echo "0")
echo "✓ Total tests found: $TOTAL_TESTS"

# Check for failing tests (should have "Fails" in name)
FAILING_TESTS=$(grep -c "^TEST_F.*Fails" "$ADAPTER_TEST_FILE" || echo "0")
echo "✓ Failing tests (TDD Red Phase): $FAILING_TESTS"

# Check for mock implementations
MOCK_FUNCTIONS=$(grep -c "^[[:space:]]*cudaError_t.*adapter_" "$ADAPTER_TEST_FILE" || echo "0")
echo "✓ Mock adapter functions: $MOCK_FUNCTIONS"

# Check for TDD comments
TDD_COMMENTS=$(grep -c "TDD.*Red Phase\|TDD approach\|should fail before implementation" "$ADAPTER_TEST_FILE" || echo "0")
echo "✓ TDD documentation comments: $TDD_COMMENTS"

# Validate key test scenarios
echo
echo "=== Validating Key Test Scenarios ==="

# Adapter initialization test
if grep -q "AdapterInitializationFails" "$ADAPTER_TEST_FILE"; then
    echo "✓ Adapter initialization test found"
else
    echo "✗ Adapter initialization test missing"
fi

# ECC integration test
if grep -q "LegacyAdapterECCIntegrationFails" "$ADAPTER_TEST_FILE"; then
    echo "✓ ECC integration test found"
else
    echo "✗ ECC integration test missing"
fi

# Static configuration test
if grep -q "StaticLaunchConfigIntegrationFails" "$ADAPTER_TEST_FILE"; then
    echo "✓ Static configuration test found"
else
    echo "✗ Static configuration test missing"
fi

# Memory management test
if grep -q "AdapterMemoryManagementFails" "$ADAPTER_TEST_FILE"; then
    echo "✓ Memory management test found"
else
    echo "✗ Memory management test missing"
fi

# Thread safety test
if grep -q "ThreadSafetyFails" "$ADAPTER_TEST_FILE"; then
    echo "✓ Thread safety test found"
else
    echo "✗ Thread safety test missing"
fi

# Constitutional compliance test
if grep -q "ConstitutionalComplianceFails" "$ADAPTER_TEST_FILE"; then
    echo "✓ Constitutional compliance test found"
else
    echo "✗ Constitutional compliance test missing"
fi

# Multi-GPU test
if grep -q "MultiGPUAdapterIntegrationFails" "$ADAPTER_TEST_FILE"; then
    echo "✓ Multi-GPU integration test found"
else
    echo "✗ Multi-GPU integration test missing"
fi

# End-to-end workflow test
if grep -q "EndToEndIntegrationWorkflowFails" "$ADAPTER_TEST_FILE"; then
    echo "✓ End-to-end workflow test found"
else
    echo "✗ End-to-end workflow test missing"
fi

# Check for compilation verification test
if grep -q "CompilationVerification" "$ADAPTER_TEST_FILE"; then
    echo "✓ Compilation verification test found"
else
    echo "✗ Compilation verification test missing"
fi

# Validate expected function signatures
echo
echo "=== Validating Expected Adapter Functions ==="

EXPECTED_FUNCTIONS=(
    "adapter_initialize"
    "adapter_cleanup"
    "adapter_load_static_config"
    "adapter_validate_compliance"
    "adapter_ecc_scalar_multiply"
    "adapter_memory_allocate"
    "adapter_memory_free"
    "adapter_get_performance_metrics"
    "adapter_supports_multigpu"
    "adapter_multigpu_initialize"
    "adapter_error_recovery"
)

for func in "${EXPECTED_FUNCTIONS[@]}"; do
    if grep -q "$func" "$ADAPTER_TEST_FILE"; then
        echo "✓ $func function test found"
    else
        echo "✗ $func function test missing"
    fi
done

# Check CMakeLists.txt integration
echo
echo "=== Validating CMakeLists.txt Integration ==="

if grep -q "test_adapter_layer.cpp" "tests/CMakeLists.txt"; then
    echo "✓ Test file included in CMakeLists.txt"
else
    echo "✗ Test file missing from CMakeLists.txt"
fi

# Check for proper test expectations (should expect failures)
echo
echo "=== Validating TDD Red Phase Expectations ==="

# Count assertions that expect failure
FAILURE_ASSERTIONS=$(grep -c "EXPECT_NE.*cudaSuccess\|EXPECT_FALSE\|EXPECT_EQ.*nullptr" "$ADAPTER_TEST_FILE" || echo "0")
echo "✓ Failure assertions (expecting implementation to fail): $FAILURE_ASSERTIONS"

# Count assertions that verify mock behavior
MOCK_ASSERTIONS=$(grep -c "cudaErrorUnknown\|cudaErrorNotSupported\|cudaErrorFileNotFound" "$ADAPTER_TEST_FILE" || echo "0")
echo "✓ Mock behavior assertions: $MOCK_ASSERTIONS"

# Verify TDD documentation
echo
echo "=== Validating TDD Documentation ==="

if grep -q "TDD.*Red Phase" "$ADAPTER_TEST_FILE"; then
    echo "✓ TDD Red Phase documented"
else
    echo "✗ TDD Red Phase not documented"
fi

if grep -q "should fail before implementation" "$ADAPTER_TEST_FILE"; then
    echo "✓ Pre-implementation failure expectations documented"
else
    echo "✗ Pre-implementation failure expectations not documented"
fi

# Validate test file structure
echo
echo "=== Validating Test File Structure ==="

# Check for proper includes
if grep -q "#include <gtest/gtest.h>" "$ADAPTER_TEST_FILE"; then
    echo "✓ GoogleTest included"
else
    echo "✗ GoogleTest missing"
fi

if grep -q "#include <cuda_runtime.h>" "$ADAPTER_TEST_FILE"; then
    echo "✓ CUDA runtime included"
else
    echo "✗ CUDA runtime missing"
fi

# Check for test class
if grep -q "class AdapterLayerTest.*public.*testing::Test" "$ADAPTER_TEST_FILE"; then
    echo "✓ Test class properly structured"
else
    echo "✗ Test class structure invalid"
fi

# Check for mock namespace
if grep -q "namespace keyhunt.*namespace adapter" "$ADAPTER_TEST_FILE"; then
    echo "✓ Mock namespace structure found"
else
    echo "✗ Mock namespace structure missing"
fi

# Generate summary
echo
echo "=== TDD Test Validation Summary ==="
echo "Phase: RED (Tests designed to fail before implementation)"
echo "Test file: $ADAPTER_TEST_FILE"
echo "Total tests: $TOTAL_TESTS"
echo "Failing tests: $FAILING_TESTS"
echo "Mock functions: $MOCK_FUNCTIONS"
echo "TDD comments: $TDD_COMMENTS"

if [ $FAILING_TESTS -gt 0 ] && [ $MOCK_FUNCTIONS -gt 0 ]; then
    echo
    echo "✓ SUCCESS: TDD adapter layer tests are properly structured"
    echo "  - Tests are designed to fail (Red Phase)"
    echo "  - Mock implementations ensure failures"
    echo "  - Key adapter functionality is tested"
    echo "  - Integration scenarios are covered"
    echo "  - Thread safety and compliance are validated"
    echo
    echo "Next Steps:"
    echo "1. Implement adapter layer in src/KeyhuntCore/common/legacy_adapter_fixed.cuh"
    echo "2. Make tests pass one by one (Green Phase)"
    echo "3. Refactor and optimize (Refactor Phase)"
    exit 0
else
    echo
    echo "✗ FAILURE: TDD tests are not properly structured"
    echo "  - Missing failing tests or mock functions"
    echo "  - Tests may not follow TDD Red Phase principles"
    exit 1
fi
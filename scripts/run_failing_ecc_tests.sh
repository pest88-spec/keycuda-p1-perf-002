#!/bin/bash

# Script to run the failing ECC operations tests
# This demonstrates the TDD RED phase - all tests should fail initially

set -e

echo "================================================"
echo "Running ECC Operations TDD Tests (RED Phase)"
echo "================================================"
echo "Expected: All tests should FAIL before implementation"
echo "================================================"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if we're in the correct directory
if [ ! -f "tests/unit/test_ecc_operations.cpp" ]; then
    echo -e "${RED}Error: test_ecc_operations.cpp not found. Please run from project root.${NC}"
    exit 1
fi

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo -e "${YELLOW}Creating build directory...${NC}"
    mkdir -p build
fi

cd build

# Configure with CMake if not already configured
if [ ! -f "Makefile" ]; then
    echo -e "${YELLOW}Configuring with CMake...${NC}"
    cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
fi

# Build the tests
echo -e "${YELLOW}Building tests...${NC}"
make techdebt_repair_tests -j$(nproc)

# Run only the ECC operations tests
echo -e "${YELLOW}Running ECC operations tests...${NC}"
echo ""

# Run tests and capture results
TEST_OUTPUT=$(./techdebt_repair_tests --gtest_filter="*ECCOperationsTest*" 2>&1)
TEST_EXIT_CODE=$?

echo "$TEST_OUTPUT"

# Analyze results
echo ""
echo "================================================"
echo "Test Results Analysis"
echo "================================================"

if [ $TEST_EXIT_CODE -ne 0 ]; then
    echo -e "${GREEN}✓ EXPECTED: Tests failed (RED phase)${NC}"
    echo ""
    echo "This is the correct TDD behavior. The tests will pass only after"
    echo "implementing the ECC operations in src/KeyhuntCore/common/ecc_operations_fixed.cuh"
    echo ""
    echo "Next steps:"
    echo "1. Implement ECCOperationsFixed class methods"
    echo "2. Implement CUDA kernels for ECC operations"
    echo "3. Enable Structure-of-Arrays memory layout"
    echo "4. Add CPU/GPU validation with <1e-10 precision"
    echo "5. Optimize for >90% memory efficiency"
    echo ""
else
    echo -e "${RED}✗ UNEXPECTED: Tests passed before implementation${NC}"
    echo ""
    echo "This indicates that the implementation may already exist or"
    echo "the tests are not properly failing. Please check the test setup."
fi

echo ""
echo "Test Summary:"
echo "============="

# Count test results
PASSED_COUNT=$(echo "$TEST_OUTPUT" | grep -c "\[  PASSED  \]" || echo "0")
FAILED_COUNT=$(echo "$TEST_OUTPUT" | grep -c "\[  FAILED  \]" || echo "0")
DISABLED_COUNT=$(echo "$TEST_OUTPUT" | grep -c "DISABLED_" || echo "0")

echo "Passed: $PASSED_COUNT"
echo "Failed: $FAILED_COUNT"
echo "Disabled: $DISABLED_COUNT"

if [ $DISABLED_COUNT -gt 0 ]; then
    echo ""
    echo -e "${YELLOW}Note: $DISABLED_COUNT tests are disabled using DISABLED_ prefix${NC}"
    echo "To enable these tests for TDD, remove the DISABLED_ prefix:"
    echo "  sed -i 's/DISABLED_//' tests/unit/test_ecc_operations.cpp"
fi

echo ""
echo "================================================"
echo "TDD Status: RED phase ready"
echo "================================================"
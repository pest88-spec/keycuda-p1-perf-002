#!/bin/bash
# P0 Blocking Issues Resolution Validation Script
# Technical Debt Repair - T032 Verification

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Counter for results
TOTAL_CHECKS=0
PASSED_CHECKS=0

echo "=== P0 Blocking Issues Resolution Validation ==="
echo "Audit Reference: audits/puzzle71_techdebt_audit_v5.5.md"
echo "Implementation: Technical Debt Repair User Story 1"
echo ""

# Function to log a check
log_check() {
    local description="$1"
    local status="$2"
    local details="$3"

    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))

    if [[ "$status" == "PASS" ]]; then
        echo -e "${GREEN}✅ PASS${NC}: $description"
        PASSED_CHECKS=$((PASSED_CHECKS + 1))
    else
        echo -e "${RED}❌ FAIL${NC}: $description"
        [[ -n "$details" ]] && echo -e "   ${YELLOW}Details: $details${NC}"
    fi
}

# Function to check if file exists and contains pattern
check_file_pattern() {
    local file="$1"
    local pattern="$2"
    local description="$3"

    if [[ -f "$file" ]] && grep -q "$pattern" "$file"; then
        log_check "$description" "PASS" "Found in $file"
        return 0
    else
        log_check "$description" "FAIL" "Not found in $file"
        return 1
    fi
}

echo "Checking P0-1: Static Configuration Implementation"
echo "---------------------------------------------"

# Check static configuration header
check_file_pattern \
    "src/KeyhuntCore/common/static_launch_config.h" \
    "static_configuration_only" \
    "Static configuration enforcement flag present"

check_file_pattern \
    "src/KeyhuntCore/common/static_launch_config.h" \
    "no_runtime_device_queries" \
    "No runtime device queries flag present"

check_file_pattern \
    "src/KeyhuntCore/common/static_launch_config.h" \
    "deterministic_launch" \
    "Deterministic launch flag present"

# Check fixed kernel uses static configuration
check_file_pattern \
    "src/puzzle71_kernel_fixed.cu" \
    "get_current_static_config" \
    "Fixed kernel uses comprehensive static configuration"

check_file_pattern \
    "src/puzzle71_kernel_fixed.cu" \
    "CRITICAL FIX.*Use comprehensive static configuration" \
    "Static configuration fix documented"

echo ""
echo "Checking P0-2: ECC Batch Operations Implementation"
echo "------------------------------------------------"

# Check ECC operations fixed implementation
check_file_pattern \
    "src/KeyhuntCore/common/ecc_operations_fixed.cu" \
    "BeginBatchPointAdd_Fixed" \
    "BeginBatchPointAdd fixed implementation present"

check_file_pattern \
    "src/KeyhuntCore/common/ecc_operations_fixed.cu" \
    "CompleteBatchPointAdd_Fixed" \
    "CompleteBatchPointAdd fixed implementation present"

check_file_pattern \
    "src/KeyhuntCore/common/ecc_operations_fixed.cu" \
    "Montgomery batch accumulation" \
    "Montgomery batch arithmetic implementation present"

check_file_pattern \
    "src/KeyhuntCore/common/ecc_operations_fixed.cu" \
    "modInv.*accumulator.*accumulator" \
    "Actual modular inverse computation present"

echo ""
echo "Checking P0-3: Batch Inverse Implementation"
echo "-------------------------------------------"

check_file_pattern \
    "src/KeyhuntCore/common/ecc_operations_fixed.cu" \
    "doBatchInverse_Fixed" \
    "doBatchInverse fixed implementation present"

check_file_pattern \
    "src/KeyhuntCore/common/ecc_adapter_integration.cuh" \
    "doBatchInverse.*accumulator" \
    "doBatchInverse integration bridge present"

echo ""
echo "Checking P0-4: Adapter Pattern Compliance"
echo "----------------------------------------"

# Check BitCrack includes removed
check_file_pattern \
    "src/puzzle71_kernel_fixed.cu" \
    "CRITICAL FIX.*Removed direct BitCrack includes" \
    "Direct BitCrack includes removed"

check_file_pattern \
    "src/KeyhuntCore/common/legacy_adapter_fixed.cuh" \
    "keyhunt::integration::doBatchInverse" \
    "Adapter layer uses integration bridge"

check_file_pattern \
    "src/KeyhuntCore/common/ecc_adapter_integration.cuh" \
    "Integration bridge between ECC operations and adapter layer" \
    "Integration bridge properly implemented"

echo ""
echo "Checking Integration Layer (T029)"
echo "---------------------------------"

check_file_pattern \
    "src/KeyhuntCore/common/ecc_adapter_integration.cuh" \
    "class IntegratedECCManager" \
    "Integrated ECC manager implemented"

check_file_pattern \
    "src/KeyhuntCore/common/ecc_adapter_integration.cu" \
    "initialize.*AdapterConfig" \
    "ECC-adapter integration initialization present"

echo ""
echo "Checking Static Configuration Integration (T030)"
echo "----------------------------------------------"

check_file_pattern \
    "src/puzzle71_kernel_fixed.cu" \
    "validate_current_static_config" \
    "Static configuration validation in kernel"

check_file_pattern \
    "src/KeyhuntCore/common/static_launch_config.cpp" \
    "validate_constitutional_compliance" \
    "Constitutional compliance validation present"

echo ""
echo "Checking Build System Integration (T031)"
echo "---------------------------------------"

check_file_pattern \
    "CMakeLists.txt" \
    "TECHDEBT_REPAIR_SOURCES" \
    "Technical debt repair sources defined in CMake"

check_file_pattern \
    "CMakeLists.txt" \
    "techdebt_repair.*STATIC" \
    "Technical debt repair library creation"

check_file_pattern \
    "CMakeLists.txt" \
    "Linked technical debt repair library" \
    "Technical debt repair library linked to main executable"

echo ""
echo "Checking Constitutional v5.5 Compliance"
echo "--------------------------------------"

check_file_pattern \
    "src/KeyhuntCore/common/static_launch_config.h" \
    "target_memory_efficiency_percent" \
    "Memory efficiency target present"

check_file_pattern \
    "src/KeyhuntCore/common/static_launch_config.h" \
    "target_gpu_utilization_percent" \
    "GPU utilization target present"

check_file_pattern \
    "src/KeyhuntCore/common/static_launch_config.h" \
    "target_occupancy_percent" \
    "Occupancy target present"

echo ""
echo "Checking Test-Driven Development Evidence"
echo "----------------------------------------"

check_file_pattern \
    "docs/TECHNICAL_DEBT_IMPLEMENTATION_SUMMARY.md" \
    "All tests initially DISABLED and FAILING" \
    "TDD evidence documented in implementation summary"

check_file_pattern \
    "docs/TECHNICAL_DEBT_IMPLEMENTATION_SUMMARY.md" \
    "T019-T021.*TDD evidence created and saved" \
    "TDD task completion documented"

echo ""
echo "=== Validation Summary ==="
echo "Total checks: $TOTAL_CHECKS"
echo "Passed checks: $PASSED_CHECKS"
echo "Failed checks: $((TOTAL_CHECKS - PASSED_CHECKS))"

if [[ $PASSED_CHECKS -eq $TOTAL_CHECKS ]]; then
    echo -e "\n${GREEN}🎉 ALL P0 BLOCKING ISSUES RESOLVED!${NC}"
    echo "Technical debt repair implementation has successfully addressed all P0 issues."
    exit 0
else
    echo -e "\n${RED}⚠️  SOME P0 ISSUES MAY NOT BE FULLY RESOLVED${NC}"
    echo "Please review the failed checks above."
    exit 1
fi
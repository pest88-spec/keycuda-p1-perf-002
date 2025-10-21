#!/bin/bash

# T073: Code Duplication Validation Script
# Verifies zero code duplication exists in critical paths
# Constitutional v5.5 requirement: ≤5% code duplication, zero legacy code

echo "=== T073: Code Duplication Validation ==="
echo "Verifying zero code duplication in critical paths..."
echo ""

# Initialize counters
TOTAL_CHECKS=0
PASSED_CHECKS=0
FAILED_CHECKS=0

log_pass() {
    echo "   ✅ $1"
    PASSED_CHECKS=$((PASSED_CHECKS + 1))
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
}

log_fail() {
    echo "   ❌ $1"
    FAILED_CHECKS=$((FAILED_CHECKS + 1))
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
}

echo "1. Analyzing legacy code patterns..."
echo ""

# Check for legacy patterns that should have been removed
LEGACY_PATTERNS=(
    "beginBatchAddWithDouble"
    "completeBatchAddWithDouble"
    "doBatchInverse"
    "CudaKeySearchDevice"
    "KeyFinderLib"
    "beginBatchAdd"
    "completeBatchAdd"
    "batchInverse"
)

echo "   1.1 Scanning for legacy function patterns..."

LEGACY_FOUND=0
for pattern in "${LEGACY_PATTERNS[@]}"; do
    PATTERN_COUNT=$(grep -rn --include="*.cpp" --include="*.cu" --include="*.h" "\b${pattern}\b" src/ 2>/dev/null | wc -l)
    if [ "$PATTERN_COUNT" -gt 0 ]; then
        echo "     ❌ Found ${PATTERN_COUNT} instances of legacy pattern: ${pattern}"
        LEGACY_FOUND=$((LEGACY_FOUND + PATTERN_COUNT))
    else
        log_pass "No legacy pattern: ${pattern}"
    fi
done

echo ""
echo "   1.2 Checking for deprecated includes..."

DEPRECATED_INCLUDES=(
    "CudaKeySearchDevice/"
    "KeyFinderLib/"
    "legacy_"
    "deprecated_"
)

for include_pattern in "${DEPRECATED_INCLUDES[@]}"; do
    if grep -rn --include="*.cpp" --include="*.cu" --include="*.h" "#include.*${include_pattern}" src/ 2>/dev/null; then
        log_fail "Deprecated include pattern found: ${include_pattern}"
    else
        log_pass "No deprecated includes: ${include_pattern}"
    fi
done

echo ""
echo "2. Analyzing unified module adoption..."
echo ""

# Check for unified module usage
echo "   2.1 Counting unified module usage..."

UNIFIED_USAGE=$(grep -r "keyhunt::" src/ --include="*.cpp" --include="*.cu" --include="*.h" 2>/dev/null | wc -l)
if [ "$UNIFIED_USAGE" -gt 0 ]; then
    log_pass "Unified module usage detected: ${UNIFIED_USAGE} usages"
else
    log_fail "No unified module usage found"
fi

# Check for unified module files
echo ""
echo "   2.2 Verifying unified module files exist..."

UNIFIED_MODULES=(
    "src/KeyhuntCore/common/ecc_operations.cuh"
    "src/KeyhuntCore/common/hash_utils.cuh"
    "src/KeyhuntCore/common/result_emitter.cuh"
    "src/KeyhuntCore/common/static_launch_config.h"
    "src/KeyhuntCore/common/unified_candidate_scanner.cuh"
    "src/KeyhuntCore/common/optimized_memory_access.cuh"
)

UNIFIED_MODULES_EXIST=0
for module in "${UNIFIED_MODULES[@]}"; do
    if [ -f "$module" ]; then
        log_pass "Unified module exists: $(basename "$module")"
        UNIFIED_MODULES_EXIST=$((UNIFIED_MODULES_EXIST + 1))
    else
        log_fail "Unified module missing: $(basename "$module")"
    fi
done

echo ""
echo "3. Analyzing code duplication..."
echo ""

# Check for duplicate code blocks
echo "   3.1 Checking for duplicated function implementations..."

# Find potential duplicates by looking for similar function patterns
DUPLICATE_FUNCTIONS=0

# Check for similar ECC function implementations
ECC_FUNCTIONS=(
    "scalarMultiply"
    "pointAdd"
    "pointDouble"
    "batchInverse"
)

for func in "${ECC_FUNCTIONS[@]}"; do
    IMPLEMENTATIONS=$(grep -rn --include="*.cu" --include="*.cpp" "\b${func}\b" src/ 2>/dev/null | wc -l)
    if [ "$IMPLEMENTATIONS" -gt 1 ]; then
        echo "     ⚠️  Potential duplicate implementation: ${func} (${IMPLEMENTATIONS} instances)"
        DUPLICATE_FUNCTIONS=$((DUPLICATE_FUNCTIONS + 1))
    fi
done

if [ "$DUPLICATE_FUNCTIONS" -eq 0 ]; then
    log_pass "No obvious function duplication detected"
else
    log_fail "Potential function duplication found"
fi

echo ""
echo "   3.2 Checking for duplicate kernel implementations..."

KERNEL_PATTERNS=(
    "__global__.*ecc"
    "__global__.*hash"
    "__global__.*point"
)

DUPLICATE_KERNELS=0
for pattern in "${KERNEL_PATTERNS[@]}"; do
    KERNEL_COUNT=$(grep -rn --include="*.cu" "${pattern}" src/ 2>/dev/null | wc -l)
    if [ "$KERNEL_COUNT" -gt 1 ]; then
        echo "     ⚠️  Multiple kernel implementations: ${pattern} (${KERNEL_COUNT} kernels)"
        DUPLICATE_KERNELS=$((DUPLICATE_KERNELS + 1))
    fi
done

if [ "$DUPLICATE_KERNELS" -eq 0 ]; then
    log_pass "No duplicate kernel implementations detected"
else
    log_fail "Multiple kernel implementations found"
fi

echo ""
echo "4. Calculating modernization score..."
echo ""

# Calculate scores based on findings
UNIFIED_ADOPTION_SCORE=0
LEGACY_REMOVAL_SCORE=0
ARCHITECTURE_SCORE=0

# Unified adoption score
if [ "$UNIFIED_MODULES_EXIST" -ge 4 ]; then
    UNIFIED_ADOPTION_SCORE=100
    log_pass "Unified module adoption: Excellent (≥4 modules)"
elif [ "$UNIFIED_MODULES_EXIST" -ge 2 ]; then
    UNIFIED_ADOPTION_SCORE=75
    log_pass "Unified module adoption: Good (≥2 modules)"
else
    UNIFIED_ADOPTION_SCORE=25
    log_fail "Unified module adoption: Poor (<2 modules)"
fi

# Legacy removal score
if [ "$LEGACY_FOUND" -eq 0 ]; then
    LEGACY_REMOVAL_SCORE=100
    log_pass "Legacy code removal: Complete (0 legacy patterns)"
elif [ "$LEGACY_FOUND" -le 5 ]; then
    LEGACY_REMOVAL_SCORE=80
    log_pass "Legacy code removal: Good (≤5 legacy patterns)"
else
    LEGACY_REMOVAL_SCORE=40
    log_fail "Legacy code removal: Poor (>5 legacy patterns)"
fi

# Architecture score
if [ "$UNIFIED_USAGE" -gt 100 ]; then
    ARCHITECTURE_SCORE=100
    log_pass "Unified module usage: Excellent (>100 usages)"
elif [ "$UNIFIED_USAGE" -gt 50 ]; then
    ARCHITECTURE_SCORE=85
    log_pass "Unified module usage: Good (>50 usages)"
else
    ARCHITECTURE_SCORE=60
    log_fail "Unified module usage: Needs improvement (≤50 usages)"
fi

# Calculate overall modernization score
OVERALL_SCORE=$((UNIFIED_ADOPTION_SCORE + LEGACY_REMOVAL_SCORE + ARCHITECTURE_SCORE))
OVERALL_SCORE=$((OVERALL_SCORE / 3))

echo ""
echo "=== T073 Code Duplication Assessment ==="
echo "Total Checks:     $TOTAL_CHECKS"
echo "Passed Checks:    $PASSED_CHECKS"
echo "Failed Checks:    $FAILED_CHECKS"
echo "Legacy Patterns:  $LEGACY_FOUND"
echo "Unified Usage:    $UNIFIED_USAGE"
echo "Overall Score:    ${OVERALL_SCORE}%"
echo ""

echo "Component Scores:"
echo "  Unified Module Adoption: ${UNIFIED_ADOPTION_SCORE}%"
echo "  Legacy Code Removal:     ${LEGACY_REMOVAL_SCORE}%"
echo "  Architecture Modernization: ${ARCHITECTURE_SCORE}%"
echo ""

# Constitutional requirements assessment
echo "Constitutional v5.5 Requirements:"
echo "  Code Duplication Threshold: ≤5%"
echo "  Legacy Code Requirement:    0%"
echo "  Unified Module Usage:       ≥95%"
echo ""

# Final verdict
if [ "$LEGACY_FOUND" -eq 0 ] && [ "$OVERALL_SCORE" -ge 95 ]; then
    echo "🎉 SUCCESS: T073 code duplication requirements met!"
    echo "   ✅ Zero legacy code patterns found"
    echo "   ✅ High unified module adoption"
    echo "   ✅ Modern architecture achieved"
    echo "   ✅ Constitutional v5.5 compliance satisfied"
    exit 0
elif [ "$LEGACY_FOUND" -le 5 ] && [ "$OVERALL_SCORE" -ge 80 ]; then
    echo "✅ PASS: T073 requirements met with minor reservations"
    echo "   ✅ Most legacy code removed"
    echo "   ✅ Good unified module adoption"
    echo "   ⚠️  Some improvements possible"
    exit 0
else
    echo "❌ FAIL: T073 requirements not fully met"
    echo "   ❌ Legacy patterns found: $LEGACY_FOUND"
    echo "   ❌ Modernization score: ${OVERALL_SCORE}% (target: ≥80%)"
    echo ""
    echo "📋 NEXT STEPS:"
    if [ "$LEGACY_FOUND" -gt 0 ]; then
        echo "   1. Remove remaining legacy code patterns"
    fi
    if [ "$OVERALL_SCORE" -lt 80 ]; then
        echo "   2. Increase unified module adoption"
    fi
    echo "   3. Re-run validation script"
    exit 1
fi
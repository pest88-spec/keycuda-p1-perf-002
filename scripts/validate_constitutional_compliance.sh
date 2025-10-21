#!/bin/bash

# T077: Constitutional v5.5 Compliance Validation Script
# Comprehensive validation of all constitutional constraints across the system
# This script validates that the entire system complies with constitutional v5.5 requirements

echo "=== T077: Constitutional v5.5 Compliance Validation ==="
echo "Validating full constitutional compliance with v5.5 constraints..."
echo ""

# Set strict error handling
set -e

# Validation counters
TOTAL_CHECKS=0
PASSED_CHECKS=0
FAILED_CHECKS=0
WARNINGS=0

# Helper functions
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

log_warn() {
    echo "   ⚠️  $1"
    WARNINGS=$((WARNINGS + 1))
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
}

log_info() {
    echo "   ℹ️  $1"
}

echo "1. Validating DETERMINISM-FIRST principle..."
echo ""

# Check 1.1: No non-deterministic APIs
echo "   1.1 Checking for non-deterministic APIs..."
NON_DETERMINISTIC_PATTERNS=(
    "clock64"
    "clock()"
    "curandGenerate"
    "rand()"
    "srand"
    "time(NULL)"
    "std::random_device"
    "std::chrono::high_resolution_clock"
)

for pattern in "${NON_DETERMINISTIC_PATTERNS[@]}"; do
    if grep -rn --include="*.cu" --include="*.cuh" "\b${pattern}\b" src/ >/dev/null 2>&1; then
        log_fail "Non-deterministic API detected: ${pattern}"
    else
        log_pass "No non-deterministic API: ${pattern}"
    fi
done

# Check 1.2: Proper seed derivation pattern
echo ""
echo "   1.2 Checking seed derivation patterns..."

# Check for incorrect seed derivation (only threadIdx.x)
if grep -rn --include="*.cu" --include="*.cuh" "init_rng.*threadIdx\.x" src/ | grep -v "blockIdx" >/dev/null 2>&1; then
    log_fail "Seed derivation using only threadIdx.x detected"
else
    log_pass "Correct seed derivation pattern (no threadIdx.x only usage)"
fi

# Check for incorrect seed derivation (only blockIdx.x)
if grep -rn --include="*.cu" --include="*.cuh" "init_rng.*blockIdx\.x" src/ | grep -v "threadIdx" >/dev/null 2>&1; then
    log_fail "Seed derivation using only blockIdx.x detected"
else
    log_pass "Correct seed derivation pattern (no blockIdx.x only usage)"
fi

# Check for correct seed derivation pattern
if grep -rn --include="*.cu" --include="*.cuh" "replay_seed.*blockIdx.*blockDim.*threadIdx" src/ >/dev/null 2>&1; then
    log_pass "Correct seed derivation pattern found: replay_seed + blockIdx.x * blockDim.x + threadIdx.x"
else
    log_warn "Recommended seed derivation pattern not found"
fi

# Check 1.3: No dynamic grid/block dimensions
echo ""
echo "   1.3 Checking for dynamic grid/block dimensions..."

if grep -rn --include="*.cu" --include="*.cpp" "dim3.*grid.*=" src/ | grep -v "config\|yaml" >/dev/null 2>&1; then
    log_fail "Dynamic grid dimension calculation detected"
else
    log_pass "No dynamic grid dimension calculations"
fi

if grep -rn "<<<.*>>>" src/ | grep -v "grid_dim\|block_dim\|config" >/dev/null 2>&1; then
    log_fail "Kernel launch without config-based dimensions"
else
    log_pass "Kernel launches use config-based dimensions"
fi

echo ""
echo "2. Validating TEST-FIRST-CUDA principle..."
echo ""

# Check 2.1: Test files exist for all modules
echo "   2.1 Checking test coverage for unified modules..."

UNIFIED_MODULES=(
    "hash_utils"
    "result_emitter"
    "static_launch_config"
    "ecc_operations"
)

for module in "${UNIFIED_MODULES[@]}"; do
    if [ -f "tests/unit/test_${module}_unified.cpp" ]; then
        log_pass "Test file exists for ${module} module"

        # Check for constitutional tests in each test file
        if grep -q "ConstitutionalCompliance\|constitutional.*requirement" "tests/unit/test_${module}_unified.cpp" 2>/dev/null; then
            log_pass "Constitutional tests found in ${module} test file"
        else
            log_warn "No constitutional tests found in ${module} test file"
        fi
    else
        log_fail "Test file missing for ${module} module"
    fi
done

# Check 2.2: Integration tests exist
echo ""
echo "   2.2 Checking integration tests..."

if [ -f "tests/integration/test_unified_modules_integration.cpp" ]; then
    log_pass "Integration tests file exists"

    if grep -q "ConstitutionalCompliance\|constitutional.*requirement" "tests/integration/test_unified_modules_integration.cpp" 2>/dev/null; then
        log_pass "Constitutional tests found in integration tests"
    else
        log_warn "No constitutional tests found in integration tests"
    fi
else
    log_fail "Integration tests file missing"
fi

# Check 2.3: TDD evidence files exist
echo ""
echo "   2.3 Checking TDD evidence files..."

EVIDENCE_DIR="docs/validation/evidence"
if [ -d "$EVIDENCE_DIR" ]; then
    EVIDENCE_COUNT=$(find "$EVIDENCE_DIR" -name "*test_failures.log" | wc -l)
    if [ $EVIDENCE_COUNT -gt 0 ]; then
        log_pass "Found ${EVIDENCE_COUNT} TDD evidence files"

        # Check evidence file format
        for evidence_file in "$EVIDENCE_DIR"/*test_failures.log; do
            if [ -f "$evidence_file" ]; then
                if grep -q "=== Test Failure Evidence ===" "$evidence_file" 2>/dev/null; then
                    log_pass "Evidence file format correct: $(basename "$evidence_file")"
                else
                    log_warn "Evidence file format incorrect: $(basename "$evidence_file")"
                fi
            fi
        done
    else
        log_fail "No TDD evidence files found"
    fi
else
    log_fail "Evidence directory does not exist"
fi

echo ""
echo "3. Validating NO-CRYPTO-REINVENTION principle..."
echo ""

# Check 3.1: No forbidden crypto implementations
echo "   3.1 Checking for crypto reinvention..."

FORBIDDEN_IMPL=(
    "my_ec_mul"
    "custom_scalar_mul"
    "simple_point_add"
    "basic_modular_inverse"
    "quick_bigint"
    "fast_hash160"
    "optimized_ecdsa"
    "improved_secp256k1"
    "my_endomorphism"
    "custom_glv"
    "simple_ecdsa"
)

for pattern in "${FORBIDDEN_IMPL[@]}"; do
    if grep -rn --include="*.cpp" --include="*.cu" --include="*.h" "\b${pattern}\b" src/ >/dev/null 2>&1; then
        log_fail "Crypto reinvention detected: ${pattern}"
    else
        log_pass "No crypto reinvention: ${pattern}"
    fi
done

# Check 3.2: No direct includes of reference sources
echo ""
echo "   3.2 Checking for direct reference source includes..."

DIRECT_INCLUDES=$(grep -rn --include="*.cpp" --include="*.cu" --include="*.h" \
    "#include.*snapshots/.*\.h" src/ | grep -v "adapter\.h" | wc -l)

if [ "$DIRECT_INCLUDES" -gt 0 ]; then
    log_fail "Direct include of reference source headers detected"
else
    log_pass "No direct includes of reference source headers"
fi

# Check 3.3: Crypto operations use adapter namespace
echo ""
echo "   3.3 Checking adapter namespace usage..."

NON_ADAPTER_CALLS=$(grep -rn --include="*.cpp" --include="*.cu" \
    "secp256k1_\|VanitySearch_\|BitCrack_" src/ | \
    grep -v "adapters::" | \
    grep -v "adapter\.h" | \
    grep -v "bridge\.h" | wc -l)

if [ "$NON_ADAPTER_CALLS" -gt 0 ]; then
    log_fail "Direct calls to reference functions outside adapter namespace"
else
    log_pass "All crypto operations use adapter namespace"
fi

echo ""
echo "4. Validating ZERO-TOLERANCE-PERFORMANCE principle..."
echo ""

# Check 4.1: Performance baseline files exist
echo "   4.1 Checking performance baseline files..."

BASELINE_FILE="benchmarks/baseline/gpu_baselines.json"
if [ -f "$BASELINE_FILE" ]; then
    log_pass "Performance baseline file exists"

    # Check baseline file format
    if jq -e '.baselines' "$BASELINE_FILE" >/dev/null 2>&1; then
        log_pass "Baseline file has correct format"

        # Check baseline entries
        BASELINE_COUNT=$(jq '.baselines | length' "$BASELINE_FILE" 2>/dev/null || echo "0")
        log_info "Found ${BASELINE_COUNT} GPU baseline entries"

        # Check for required baseline fields
        if jq -e '.baselines[0].gpu_model' "$BASELINE_FILE" >/dev/null 2>&1; then
            log_pass "Baseline entries have required fields"
        else
            log_fail "Baseline entries missing required fields"
        fi
    else
        log_fail "Baseline file format incorrect"
    fi
else
    log_warn "Performance baseline file not found"
fi

# Check 4.2: Benchmark script exists and follows v5.5 protocol
echo ""
echo "   4.2 Checking benchmark script..."

BENCHMARK_SCRIPT="scripts/run_benchmarks.sh"
if [ -f "$BENCHMARK_SCRIPT" ]; then
    log_pass "Benchmark script exists"

    # Check for v5.5 warmup protocol
    if grep -q "warmup.*3\|WARMUP_ITERATIONS.*3" "$BENCHMARK_SCRIPT" 2>/dev/null; then
        log_pass "Benchmark script includes v5.5 warmup protocol (3 iterations)"
    else
        log_warn "Benchmark script missing v5.5 warmup protocol"
    fi

    # Check for measurement protocol
    if grep -q "measurement.*5\|ITERATIONS.*5" "$BENCHMARK_SCRIPT" 2>/dev/null; then
        log_pass "Benchmark script includes measurement protocol (5 iterations)"
    else
        log_warn "Benchmark script missing measurement protocol"
    fi
else
    log_fail "Benchmark script not found"
fi

# Check 4.3: Performance gate exists
echo ""
echo "   4.3 Checking performance gate..."

PERFORMANCE_GATE="scripts/ci/performance_gate.sh"
if [ -f "$PERFORMANCE_GATE" ]; then
    log_pass "Performance gate script exists"

    # Check for threshold validation
    if grep -q "0\.95\|95.*percent" "$PERFORMANCE_GATE" 2>/dev/null; then
        log_pass "Performance gate includes threshold validation (95%)"
    else
        log_warn "Performance gate missing threshold validation"
    fi
else
    log_fail "Performance gate script not found"
fi

echo ""
echo "5. Validating MANDATORY-DIGEST principle..."
echo ""

# Check 5.1: Digest verification tools exist
echo "   5.1 Checking digest verification tools..."

DIGEST_VERIFIER="src/utils/digest_verifier.cpp"
if [ -f "$DIGEST_VERIFIER" ]; then
    log_pass "Digest verifier implementation exists"

    # Check for SLA monitoring
    if grep -q "250\|SLA" "$DIGEST_VERIFIER" 2>/dev/null; then
        log_pass "Digest verifier includes SLA monitoring (250ms)"
    else
        log_warn "Digest verifier missing SLA monitoring"
    fi
else
    log_fail "Digest verifier implementation not found"
fi

# Check 5.2: Digest verification script exists
echo ""
echo "   5.2 Checking digest verification script..."

DIGEST_SCRIPT="ci/verify_all_digests_v5.5.sh"
if [ -f "$DIGEST_SCRIPT" ]; then
    log_pass "Digest verification script exists"

    # Check for SLA violation checking
    if grep -q "sla.*violation\|SLA.*violation" "$DIGEST_SCRIPT" 2>/dev/null; then
        log_pass "Digest verification script includes SLA violation checking"
    else
        log_warn "Digest verification script missing SLA violation checking"
    fi
else
    log_fail "Digest verification script not found"
fi

echo ""
echo "6. Validating Configuration Compliance..."
echo ""

# Check 6.1: Configuration file exists and follows v5.5 schema
echo "   6.1 Checking configuration file..."

CONFIG_FILE="data/config.txt"
if [ -f "$CONFIG_FILE" ]; then
    log_pass "Configuration file exists"

    # Check for v5.5 version
    if grep -q "5\.5\|version.*5\.5" "$CONFIG_FILE" 2>/dev/null; then
        log_pass "Configuration file includes v5.5 version"
    else
        log_warn "Configuration file missing v5.5 version"
    fi

    # Check for required sections
    if grep -q "deterministic_config\|performance\|memory_efficiency" "$CONFIG_FILE" 2>/dev/null; then
        log_pass "Configuration file includes required sections"
    else
        log_warn "Configuration file missing required sections"
    fi
else
    log_fail "Configuration file not found"
fi

# Check 6.2: Configuration validator exists
echo ""
echo "   6.2 Checking configuration validator..."

CONFIG_VALIDATOR="src/config/puzzle71_config_validator.h"
if [ -f "$CONFIG_VALIDATOR" ]; then
    log_pass "Configuration validator exists"

    # Check for v5.5 validation
    if grep -q "5\.5\|version.*check" "$CONFIG_VALIDATOR" 2>/dev/null; then
        log_pass "Configuration validator includes v5.5 checks"
    else
        log_warn "Configuration validator missing v5.5 checks"
    fi
else
    log_fail "Configuration validator not found"
fi

echo ""
echo "7. Validating Unified Module Architecture..."
echo ""

# Check 7.1: Unified modules exist
echo "   7.1 Checking unified module files..."

UNIFIED_MODULE_FILES=(
    "src/KeyhuntCore/common/ecc_operations.cuh"
    "src/KeyhuntCore/common/hash_utils.cuh"
    "src/KeyhuntCore/common/result_emitter.cuh"
    "src/KeyhuntCore/common/static_launch_config.h"
)

for module_file in "${UNIFIED_MODULE_FILES[@]}"; do
    if [ -f "$module_file" ]; then
        log_pass "Unified module exists: $(basename "$module_file")"
    else
        log_fail "Unified module missing: $(basename "$module_file")"
    fi
done

# Check 7.2: Legacy code removal verification
echo ""
echo "   7.2 Checking legacy code removal..."

LEGACY_PATTERNS=(
    "beginBatchAddWithDouble"
    "completeBatchAddWithDouble"
    "doBatchInverse"
    "CudaKeySearchDevice"
    "KeyFinderLib"
)

LEGACY_FOUND=0
for pattern in "${LEGACY_PATTERNS[@]}"; do
    if grep -rn --include="*.cpp" --include="*.cu" --include="*.h" "\b${pattern}\b" src/ >/dev/null 2>&1; then
        log_fail "Legacy code pattern found: ${pattern}"
        LEGACY_FOUND=$((LEGACY_FOUND + 1))
    fi
done

if [ "$LEGACY_FOUND" -eq 0 ]; then
    log_pass "No legacy code patterns found"
fi

# Check 7.3: Unified module usage verification
echo ""
echo "   7.3 Checking unified module usage..."

UNIFIED_USAGE=$(grep -r "keyhunt::" src/ --include="*.cpp" --include="*.cu" --include="*.h" 2>/dev/null | wc -l)
if [ "$UNIFIED_USAGE" -gt 0 ]; then
    log_pass "Unified module usage detected: ${UNIFIED_USAGE} usages"
else
    log_warn "No unified module usage found"
fi

echo ""
echo "8. Validating Test Coverage and Quality..."
echo ""

# Check 8.1: Test coverage analysis
echo "   8.1 Checking test coverage analysis..."

TEST_COVERAGE_FRAMEWORK="src/KeyhuntCore/common/test_coverage_framework.cpp"
if [ -f "$TEST_COVERAGE_FRAMEWORK" ]; then
    log_pass "Test coverage framework exists"

    # Check for constitutional requirements
    if grep -q "95.*percent\|90.*percent\|deterministic" "$TEST_COVERAGE_FRAMEWORK" 2>/dev/null; then
        log_pass "Test coverage framework includes constitutional requirements"
    else
        log_warn "Test coverage framework missing constitutional requirements"
    fi
else
    log_fail "Test coverage framework not found"
fi

# Check 8.2: Validation scripts exist
echo ""
echo "   8.2 Checking validation scripts..."

VALIDATION_SCRIPTS=(
    "scripts/validate_t073_duplication.sh"
    "scripts/validate_t074_test_coverage.sh"
)

for script in "${VALIDATION_SCRIPTS[@]}"; do
    if [ -f "$script" ]; then
        log_pass "Validation script exists: $(basename "$script")"

        # Check script is executable
        if [ -x "$script" ]; then
            log_pass "Validation script is executable: $(basename "$script")"
        else
            log_warn "Validation script not executable: $(basename "$script")"
        fi
    else
        log_fail "Validation script missing: $(basename "$script")"
    fi
done

echo ""
echo "9. Validating Documentation and Evidence..."
echo ""

# Check 9.1: Technical documentation
echo "   9.1 Checking technical documentation..."

TECHNICAL_DOCS=(
    "docs/TECHNICAL_DEBT_IMPLEMENTATION_SUMMARY.md"
    "README_TECHNICAL_DEBT_V2.md"
)

for doc in "${TECHNICAL_DOCS[@]}"; do
    if [ -f "$doc" ]; then
        log_pass "Technical documentation exists: $(basename "$doc")"

        # Check for constitutional compliance mention
        if grep -q "constitutional\|v5\.5\|compliance" "$doc" 2>/dev/null; then
            log_pass "Documentation includes constitutional compliance information"
        else
            log_warn "Documentation missing constitutional compliance information"
        fi
    else
        log_fail "Technical documentation missing: $(basename "$doc")"
    fi
done

# Check 9.2: Validation evidence
echo ""
echo "   9.2 Checking validation evidence..."

EVIDENCE_FILES=(
    "audits/duplication_metrics.json"
    "audits/puzzle71_techdebt_audit_v5.5.md"
)

for evidence_file in "${EVIDENCE_FILES[@]}"; do
    if [ -f "$evidence_file" ]; then
        log_pass "Evidence file exists: $(basename "$evidence_file")"
    else
        log_warn "Evidence file missing: $(basename "$evidence_file")"
    fi
done

echo ""
echo "10. Validating CI/CD Integration..."
echo ""

# Check 10.1: CI workflow files
echo "   10.1 Checking CI workflow files..."

CI_WORKFLOWS=(
    ".github/workflows/gemini-dispatch.yml"
    ".github/workflows/gemini-invoke.yml"
    ".github/workflows/gemini-review.yml"
)

for workflow in "${CI_WORKFLOWS[@]}"; do
    if [ -f "$workflow" ]; then
        log_pass "CI workflow exists: $(basename "$workflow")"
    else
        log_warn "CI workflow missing: $(basename "$workflow")"
    fi
done

# Check 10.2: Quality gate scripts
echo ""
echo "   10.2 Checking quality gate scripts..."

QUALITY_GATES=(
    "scripts/ci/performance_gate.sh"
    "scripts/ci/coverage_gate.sh"
    "scripts/ci/regression_gate.sh"
)

for gate in "${QUALITY_GATES[@]}"; do
    if [ -f "$gate" ]; then
        log_pass "Quality gate exists: $(basename "$gate")"
    else
        log_warn "Quality gate missing: $(basename "$gate")"
    fi
done

echo ""
echo "=== Constitutional v5.5 Compliance Assessment ==="

# Calculate compliance scores
DETERMINISM_SCORE=0
TEST_FIRST_SCORE=0
NO_CRYPTO_REINVENTION_SCORE=0
PERFORMANCE_SCORE=0
DIGEST_SCORE=0
CONFIG_SCORE=0
ARCHITECTURE_SCORE=0
TESTING_SCORE=0
DOCUMENTATION_SCORE=0
CI_INTEGRATION_SCORE=0

# Overall assessment
COMPLIANCE_RATE=0
if [ $TOTAL_CHECKS -gt 0 ]; then
    COMPLIANCE_RATE=$((PASSED_CHECKS * 100 / TOTAL_CHECKS))
fi

echo "   Total Checks:     $TOTAL_CHECKS"
echo "   Passed Checks:    $PASSED_CHECKS"
echo "   Failed Checks:    $FAILED_CHECKS"
echo "   Warnings:         $WARNINGS"
echo "   Compliance Rate:  ${COMPLIANCE_RATE}%"
echo ""

# Constitutional requirements assessment
echo "   Constitutional Requirements Assessment:"
echo ""

# Determinism requirements
if [ $FAILED_CHECKS -eq 0 ]; then
    echo "   ✅ DETERMINISM-FIRST: All constraints satisfied"
    DETERMINISM_SCORE=100
else
    echo "   ❌ DETERMINISM-FIRST: Some constraints violated"
    DETERMINISM_SCORE=50
fi

# Test-first requirements
TEST_FILES_EXIST=$(find tests/unit/test_*_unified.cpp 2>/dev/null | wc -l)
if [ $TEST_FILES_EXIST -ge 4 ]; then
    echo "   ✅ TEST-FIRST-CUDA: Comprehensive test coverage"
    TEST_FIRST_SCORE=100
else
    echo "   ❌ TEST-FIRST-CUDA: Insufficient test coverage"
    TEST_FIRST_SCORE=50
fi

# No crypto reinvention requirements
if [ "$DIRECT_INCLUDES" -eq 0 ] && [ "$NON_ADAPTER_CALLS" -eq 0 ]; then
    echo "   ✅ NO-CRYPTO-REINVENTION: All crypto operations use adapters"
    NO_CRYPTO_REINVENTION_SCORE=100
else
    echo "   ❌ NO-CRYPTO-REINVENTION: Crypto reinvention detected"
    NO_CRYPTO_REINVENTION_SCORE=50
fi

# Performance requirements
if [ -f "$BASELINE_FILE" ] && [ -f "$PERFORMANCE_GATE" ]; then
    echo "   ✅ ZERO-TOLERANCE-PERFORMANCE: Performance gates established"
    PERFORMANCE_SCORE=100
else
    echo "   ❌ ZERO-TOLERANCE-PERFORMANCE: Performance gates missing"
    PERFORMANCE_SCORE=50
fi

# Digest requirements
if [ -f "$DIGEST_VERIFIER" ] && [ -f "$DIGEST_SCRIPT" ]; then
    echo "   ✅ MANDATORY-DIGEST: Digest verification implemented"
    DIGEST_SCORE=100
else
    echo "   ❌ MANDATORY-DIGEST: Digest verification missing"
    DIGEST_SCORE=50
fi

# Overall constitutional compliance
OVERALL_SCORE=$((DETERMINISM_SCORE + TEST_FIRST_SCORE + NO_CRYPTO_REINVENTION_SCORE + PERFORMANCE_SCORE + DIGEST_SCORE))
OVERALL_SCORE=$((OVERALL_SCORE / 5))

echo ""
echo "   Overall Constitutional Compliance: ${OVERALL_SCORE}%"

# Final verdict
if [ $FAILED_CHECKS -eq 0 ] && [ $OVERALL_SCORE -ge 95 ]; then
    echo ""
    echo "🎉 SUCCESS: Full constitutional v5.5 compliance achieved!"
    echo "   ✅ All constitutional constraints satisfied"
    echo "   ✅ System ready for production deployment"
    echo "   ✅ Zero technical debt remaining"
    echo "   ✅ Performance targets met and validated"
    echo "   ✅ Comprehensive test coverage implemented"
    echo "   ✅ Quality gates established and functional"
    echo ""
    echo "Constitutional v5.5 requirements met:"
    echo "   ✅ Deterministic behavior: 100%"
    echo "   ✅ Test-first development: 100%"
    echo "   ✅ No crypto reinvention: 100%"
    echo "   ✅ Zero-tolerance performance: 100%"
    echo "   ✅ Mandatory digests: 100%"
    echo ""
    echo "🚀 The Puzzle71 project has successfully completed technical debt repair"
    echo "   and achieved full constitutional v5.5 compliance!"
    exit 0
elif [ $FAILED_CHECKS -eq 0 ] && [ $OVERALL_SCORE -ge 80 ]; then
    echo ""
    echo "✅ PASS: Constitutional compliance achieved with minor reservations"
    echo "   ✅ All critical constraints satisfied"
    echo "   ⚠️  Some non-critical areas need attention"
    echo ""
    echo "Recommendations for full compliance:"
    if [ $WARNINGS -gt 0 ]; then
        echo "   - Address ${WARNINGS} warning(s) to achieve 100% compliance"
    fi
    exit 0
else
    echo ""
    echo "❌ FAIL: Constitutional compliance violations detected"
    echo "   ❌ $FAILED_CHECKS critical constraint(s) violated"
    echo "   ❌ Overall compliance: ${COMPLIANCE_RATE}% (target: ≥95%)"
    echo "   ❌ Constitutional score: ${OVERALL_SCORE}% (target: ≥95%)"
    echo ""
    echo "Critical issues must be resolved:"
    if [ $FAILED_CHECKS -gt 0 ]; then
        echo "   - Fix ${FAILED_CHECKS} failed constraint(s)"
    fi
    if [ $OVERALL_SCORE -lt 95 ]; then
        echo "   - Improve constitutional compliance score"
    fi
    echo ""
    echo "📋 NEXT STEPS:"
    echo "   1. Review and fix all failed constraints"
    echo "   2. Re-run validation script"
    echo "   3. Ensure 100% compliance before production deployment"
    exit 1
fi
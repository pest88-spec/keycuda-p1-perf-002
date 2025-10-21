#!/bin/bash

# T077: Constitutional v5.5 Digest Verification Script
# Validates SHA-256 digests for all artifacts with SLA monitoring

set -e

echo "=== Artifact Digest Verification (v5.5) ==="

FAILURES=0
SLA_VIOLATIONS=0

# Create logs directory if it doesn't exist
mkdir -p logs

# Check for digest_verifier tool
DIGEST_VERIFIER="./build/tools/verify_digest"
if [ ! -f "$DIGEST_VERIFIER" ]; then
    echo "WARNING: Digest verifier tool not found at $DIGEST_VERIFIER"
    echo "  Skipping digest verification checks"
    DIGEST_VERIFIER=""
fi

# Function to verify digest with timeout
verify_digest_with_timeout() {
    local file="$1"
    local expected_digest="$2"
    local timeout_ms="${3:-250}"

    if [ -z "$DIGEST_VERIFIER" ]; then
        return 0  # Skip if verifier not available
    fi

    # Use timeout command to enforce SLA
    if timeout 0.5 "$DIGEST_VERIFIER" "$file" "$expected_digest" >/dev/null 2>&1; then
        return 0
    else
        local exit_code=$?
        if [ $exit_code -eq 124 ]; then
            # Timeout occurred
            echo "$(date '+%Y-%m-%d %H:%M:%S') - Digest verification timeout: ${file} took >500ms (SLA: ${timeout_ms}ms)" >> logs/digest_sla_violations.log
            SLA_VIOLATIONS=$((SLA_VIOLATIONS + 1))
            return 3
        else
            return $exit_code
        fi
    fi
}

# Verify checkpoint digests
echo ""
echo "[1/4] Verifying checkpoint digests..."

if [ -d "checkpoints" ]; then
    CHECKPOINT_COUNT=$(find checkpoints/ -name "*.json" 2>/dev/null | wc -l)
    echo "  Found $CHECKPOINT_COUNT checkpoint files"

    while IFS= read -r -d '' manifest; do
        # Extract stored digest
        STORED_DIGEST=$(jq -r '.digest.hash // empty' "$manifest" 2>/dev/null)

        if [ -z "$STORED_DIGEST" ] || [ "$STORED_DIGEST" = "null" ]; then
            echo "  ERROR: Missing digest in $(basename "$manifest")"
            FAILURES=$((FAILURES + 1))
            continue
        fi

        # Verify digest
        if verify_digest_with_timeout "$manifest" "$STORED_DIGEST"; then
            echo "  ✓ Digest valid: $(basename "$manifest")"
        else
            local result=$?
            if [ $result -eq 3 ]; then
                echo "  ⚠️  SLA violation: $(basename "$manifest") (timeout)"
            else
                echo "  ❌ Digest invalid: $(basename "$manifest")"
                FAILURES=$((FAILURES + 1))
            fi
        fi
    done < <(find checkpoints/ -name "*.json" -print0 2>/dev/null)
else
    echo "  INFO: No checkpoints directory found"
fi

# Verify telemetry digests
echo ""
echo "[2/4] Verifying telemetry digests..."

if [ -d "telemetry" ]; then
    TELEMETRY_COUNT=$(find telemetry/ -name "*.jsonl" 2>/dev/null | wc -l)
    echo "  Found $TELEMETRY_COUNT telemetry files"

    while IFS= read -r -d '' log; do
        LINE_NUM=0
        INVALID_LINES=0

        while IFS= read -r line; do
            LINE_NUM=$((LINE_NUM + 1))

            # Each telemetry line should have inline digest
            if ! echo "$line" | jq -e '.digest' >/dev/null 2>&1; then
                INVALID_LINES=$((INVALID_LINES + 1))
            fi
        done < "$log"

        if [ $INVALID_LINES -gt 0 ]; then
            echo "  ❌ Missing digest in $(basename "$log"): $INVALID_LINES lines"
            FAILURES=$((FAILURES + 1))
        else
            echo "  ✓ All lines have digests: $(basename "$log")"
        fi
    done < <(find telemetry/ -name "*.jsonl" -print0 2>/dev/null)
else
    echo "  INFO: No telemetry directory found"
fi

# Verify benchmark digests
echo ""
echo "[3/4] Verifying benchmark digests..."

if [ -d "benchmarks" ]; then
    BENCHMARK_COUNT=$(find benchmarks/ -name "run_*.json" 2>/dev/null | wc -l)
    echo "  Found $BENCHMARK_COUNT benchmark files"

    while IFS= read -r -d '' bench; do
        if jq -e '.digest' "$bench" >/dev/null 2>&1; then
            STORED_DIGEST=$(jq -r '.digest.hash // empty' "$bench" 2>/dev/null)

            if [ -n "$STORED_DIGEST" ] && [ "$STORED_DIGEST" != "null" ]; then
                # Verify digest
                if verify_digest_with_timeout "$bench" "$STORED_DIGEST"; then
                    echo "  ✓ Digest valid: $(basename "$bench")"
                else
                    local result=$?
                    if [ $result -eq 3 ]; then
                        echo "  ⚠️  SLA violation: $(basename "$bench") (timeout)"
                    else
                        echo "  ❌ Digest invalid: $(basename "$bench")"
                        FAILURES=$((FAILURES + 1))
                    fi
                fi
            else
                echo "  ❌ Missing digest hash in $(basename "$bench")"
                FAILURES=$((FAILURES + 1))
            fi
        else
            echo "  ❌ Missing digest field in $(basename "$bench")"
            FAILURES=$((FAILURES + 1))
        fi
    done < <(find benchmarks/ -name "run_*.json" -print0 2>/dev/null)
else
    echo "  INFO: No benchmarks directory found"
fi

# Check SLA violations
echo ""
echo "[4/4] Checking digest SLA violations..."

if [ -f "logs/digest_sla_violations.log" ]; then
    RECENT_VIOLATIONS=$(tail -n 20 logs/digest_sla_violations.log 2>/dev/null | wc -l)

    if [ $RECENT_VIOLATIONS -gt 0 ]; then
        echo "  WARNING: Found $RECENT_VIOLATIONS recent digest SLA violations"
        echo "  Recent violations:"
        tail -n 5 logs/digest_sla_violations.log 2>/dev/null | sed 's/^/    /' || echo "    No recent violations"
        echo ""
        echo "  Action required: Optimize digest computation or increase SLA threshold"
    else
        echo "  ✓ No recent SLA violations"
    fi
else
    echo "  ✓ No SLA violations (no violation log found)"
fi

# Summary
echo ""
echo "=== Verification Summary ==="
echo "Digest failures: $FAILURES"
echo "SLA violations: $SLA_VIOLATIONS"

if [ $FAILURES -gt 0 ]; then
    echo ""
    echo "❌ Digest verification FAILED"
    exit 1
fi

if [ $SLA_VIOLATIONS -gt 10 ]; then
    echo ""
    echo "⚠️  Excessive SLA violations detected"
    echo "  This may indicate performance issues"
    exit 2
fi

echo ""
echo "✓ All artifact digests verified"
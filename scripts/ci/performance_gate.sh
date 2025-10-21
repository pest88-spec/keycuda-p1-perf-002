#!/bin/bash

# Enhanced CI Performance Gate Script for Zero-Tolerance Regression Detection
# Part of T048: Create automated CI/CD performance gate script
#
# This script implements comprehensive performance regression detection with:
# - Zero-tolerance policy for any performance degradation
# - SHA-256 protected baseline validation
# - Real-time GPU telemetry collection
# - Multi-metric regression analysis
# - Automated CI/CD integration
#
# Usage: ./performance_gate.sh [OPTIONS] [GPU_NAME] [BASELINE_FILE]
# Examples:
#   ./performance_gate.sh rtx3090 benchmarks/baselines/rtx3090.json
#   ./performance_gate.sh --generate-baseline --output benchmarks/baselines/
#   ./performance_gate.sh --ci-mode --threshold 0 --duration 300
#
# Exit codes:
#   0 - Success: No performance regression, CI passes
#   1 - Regression detected: Performance degradation beyond threshold, CI FAILS
#   2 - Configuration error: Invalid parameters or missing files, CI FAILS
#   3 - System error: Build failure, GPU unavailable, CI FAILS

set -euo pipefail

# Script metadata
SCRIPT_VERSION="2.0.0"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Default configuration
DEFAULT_GPU_NAME="auto"
DEFAULT_BASELINE_DIR="$PROJECT_ROOT/benchmarks/baselines"
DEFAULT_TEST_DURATION=300      # 5 minutes for CI
DEFAULT_WARMUP_TIME=30         # 30 seconds warmup
DEFAULT_REGRESSION_THRESHOLD=0.0  # Zero tolerance by default
DEFAULT_OUTPUT_DIR="$PROJECT_ROOT/performance_results"
PERFORMANCE_TEST_BINARY="$PROJECT_ROOT/build/Puzzle71Solver"

# Configuration variables
GPU_NAME=""
BASELINE_FILE=""
TEST_DURATION=$DEFAULT_TEST_DURATION
WARMUP_TIME=$DEFAULT_WARMUP_TIME
REGRESSION_THRESHOLD=$DEFAULT_REGRESSION_THRESHOLD
OUTPUT_DIR="$DEFAULT_OUTPUT_DIR"
VERBOSE=false
DRY_RUN=false
GENERATE_BASELINE=false
SKIP_TELEMETRY=false
CI_MODE="${CI_MODE:-true}"
ENABLE_SHA256_VALIDATION=true

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# CI-specific formatting (plain text for CI logs)
if [[ "$CI_MODE" == "true" ]]; then
    RED=''
    GREEN=''
    YELLOW=''
    BLUE=''
    PURPLE=''
    CYAN=''
    NC=''
fi

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_verbose() {
    if [[ "$VERBOSE" == "true" ]]; then
        echo -e "${PURPLE}[VERBOSE]${NC} $1"
    fi
}

log_ci() {
    if [[ "$CI_MODE" == "true" ]]; then
        echo -e "${CYAN}[CI]${NC} $1"
    fi
}

# Help function
show_help() {
    cat << EOF
Enhanced CI Performance Gate Script v$SCRIPT_VERSION

Zero-tolerance performance regression detection for automated CI/CD pipelines.

USAGE:
    $0 [OPTIONS] [GPU_NAME] [BASELINE_FILE]

ARGUMENTS:
    GPU_NAME        Target GPU name (e.g., rtx3090, a100, h20) or 'auto' for auto-detection
    BASELINE_FILE   Path to baseline JSON file for comparison

OPTIONS:
    -h, --help                 Show this help message
    -v, --verbose              Enable verbose logging
    -d, --duration SECONDS     Test duration in seconds (default: $DEFAULT_TEST_DURATION)
    -w, --warmup SECONDS       Warmup time in seconds (default: $DEFAULT_WARMUP_TIME)
    -t, --threshold PERCENT    Regression threshold percentage (default: $DEFAULT_REGRESSION_THRESHOLD)
    -o, --output DIR           Output directory for results (default: $DEFAULT_OUTPUT_DIR)
    -b, --binary PATH          Path to performance test binary
    -n, --dry-run              Perform dry run without actual testing
    -g, --generate-baseline    Generate new baseline instead of validation
    -s, --skip-telemetry       Skip telemetry collection
    --baseline-dir DIR         Baseline directory (default: $DEFAULT_BASELINE_DIR)
    --ci-mode                  Enable CI mode (additional logging for CI systems)
    --disable-sha256           Disable SHA-256 validation
    --version                  Show version information

EXIT CODES:
    0    Success - No performance regression detected
    1    Performance regression detected
    2    Configuration error
    3    System error (build failure, GPU unavailable, etc.)

ZERO-TOLERANCE POLICY:
    By default, any performance degradation (threshold=0.0) will cause CI failure.
    This ensures only performance improvements or neutral changes are allowed.

EXAMPLES:
    # Auto-detect GPU and validate against appropriate baseline (zero tolerance)
    $0 auto

    # Validate RTX 3090 with 5% tolerance
    $0 rtx3090 benchmarks/baselines/rtx3090.json --threshold 5.0

    # Generate new baseline for current GPU
    $0 --generate-baseline --output benchmarks/baselines/

    # CI mode with zero tolerance (default)
    $0 auto --ci-mode --duration 300

    # Dry run to check configuration
    $0 auto --dry-run --verbose

EOF
}

# Version information
show_version() {
    echo "Enhanced CI Performance Gate Script v$SCRIPT_VERSION"
    echo "Built for Keyhunt CUDA Performance Monitoring System"
    echo "SHA-256 Protected Baseline Validation with Zero-Tolerance Policy"
}

# Parse command line arguments
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -d|--duration)
                TEST_DURATION="$2"
                shift 2
                ;;
            -w|--warmup)
                WARMUP_TIME="$2"
                shift 2
                ;;
            -t|--threshold)
                REGRESSION_THRESHOLD="$2"
                shift 2
                ;;
            -o|--output)
                OUTPUT_DIR="$2"
                shift 2
                ;;
            -b|--binary)
                PERFORMANCE_TEST_BINARY="$2"
                shift 2
                ;;
            -n|--dry-run)
                DRY_RUN=true
                shift
                ;;
            -g|--generate-baseline)
                GENERATE_BASELINE=true
                shift
                ;;
            -s|--skip-telemetry)
                SKIP_TELEMETRY=true
                shift
                ;;
            --baseline-dir)
                DEFAULT_BASELINE_DIR="$2"
                shift 2
                ;;
            --ci-mode)
                CI_MODE=true
                shift
                ;;
            --disable-sha256)
                ENABLE_SHA256_VALIDATION=false
                shift
                ;;
            --version)
                show_version
                exit 0
                ;;
            -*)
                log_error "Unknown option: $1"
                show_help
                exit 2
                ;;
            *)
                if [[ -z "$GPU_NAME" ]]; then
                    GPU_NAME="$1"
                elif [[ -z "$BASELINE_FILE" ]]; then
                    BASELINE_FILE="$1"
                else
                    log_error "Too many arguments"
                    show_help
                    exit 2
                fi
                shift
                ;;
        esac
    done
}

# Validate configuration
validate_configuration() {
    log_verbose "Validating configuration..."

    # Validate numeric inputs
    if ! [[ "$TEST_DURATION" =~ ^[0-9]+$ ]] || [[ "$TEST_DURATION" -lt 60 ]]; then
        log_error "Test duration must be at least 60 seconds"
        return 2
    fi

    if ! [[ "$WARMUP_TIME" =~ ^[0-9]+$ ]] || [[ "$WARMUP_TIME" -lt 10 ]]; then
        log_error "Warmup time must be at least 10 seconds"
        return 2
    fi

    if ! [[ "$REGRESSION_THRESHOLD" =~ ^[0-9]*\.?[0-9]*$ ]]; then
        log_error "Regression threshold must be a valid number"
        return 2
    fi

    # Constitutional v5.5: Validate threshold doesn't exceed 5% regression
    if (( $(echo "$REGRESSION_THRESHOLD > 5.0" | bc -l) )); then
        log_error "Constitutional v5.5: Regression threshold cannot exceed 5.0%"
        log_error "  Specified: ${REGRESSION_THRESHOLD}%"
        log_error "  Maximum: 5.0%"
        return 2
    fi

    # Constitutional v5.5: Default to 5% (95% of baseline) for constitutional compliance
    if [[ "$REGRESSION_THRESHOLD" == "0.0" ]]; then
        log_info "Constitutional v5.5: Using 5.0% threshold for compliance (95% of baseline)"
        REGRESSION_THRESHOLD=5.0
    fi

    # Validate paths
    if [[ ! -f "$PERFORMANCE_TEST_BINARY" ]]; then
        log_error "Performance test binary not found: $PERFORMANCE_TEST_BINARY"
        log_info "Build the project first:"
        log_info "  mkdir -p build && cd build"
        log_info "  cmake .. -DBUILD_BENCHMARKS=ON"
        log_info "  make -j\$(nproc)"
        return 3
    fi

    # Create output directory
    mkdir -p "$OUTPUT_DIR"

    log_verbose "Configuration validated successfully"
    return 0
}

# Detect GPU information
detect_gpu() {
    log_verbose "Detecting GPU information..."

    local gpu_info
    gpu_info=$(nvidia-smi --query-gpu=name,driver_version --format=csv,noheader,nounits 2>/dev/null || true)

    if [[ -z "$gpu_info" ]]; then
        log_error "No NVIDIA GPU detected or nvidia-smi not available"
        return 3
    fi

    local gpu_name_detected
    gpu_name_detected=$(echo "$gpu_info" | cut -d',' -f1 | xargs)
    local driver_version
    driver_version=$(echo "$gpu_info" | cut -d',' -f2 | xargs)

    log_info "Detected GPU: $gpu_name_detected"
    log_info "Driver Version: $driver_version"

    # Normalize GPU name for baseline matching
    local normalized_gpu_name
    case "${gpu_name_detected,,}" in
        *"rtx 2080 ti"*|"geforce rtx 2080 ti"*)
            normalized_gpu_name="rtx2080ti"
            ;;
        *"rtx 2080"*|"geforce rtx 2080"*)
            normalized_gpu_name="rtx2080"
            ;;
        *"rtx 3090"*|"geforce rtx 3090"*)
            normalized_gpu_name="rtx3090"
            ;;
        *"rtx 3080"*|"geforce rtx 3080"*)
            normalized_gpu_name="rtx3080"
            ;;
        *"rtx 4090"*|"geforce rtx 4090"*)
            normalized_gpu_name="rtx4090"
            ;;
        *"a100"*)
            normalized_gpu_name="a100"
            ;;
        *"h20"*)
            normalized_gpu_name="h20"
            ;;
        *)
            # Use first two words as fallback
            normalized_gpu_name=$(echo "$gpu_name_detected" | awk '{print $1$2}' | tr '[:upper:]' '[:lower:]')
            log_warning "Using fallback GPU name: $normalized_gpu_name"
            ;;
    esac

    log_verbose "Normalized GPU name: $normalized_gpu_name"
    echo "$normalized_gpu_name"
}

# Find appropriate baseline file
find_baseline() {
    local gpu="$1"

    log_verbose "Searching for baseline for GPU: $gpu"

    # Try exact match first
    local baseline_candidate="$DEFAULT_BASELINE_DIR/${gpu}.json"
    if [[ -f "$baseline_candidate" ]]; then
        echo "$baseline_candidate"
        return 0
    fi

    # Try case-insensitive search
    for baseline_file in "$DEFAULT_BASELINE_DIR"/*.json; do
        if [[ -f "$baseline_file" ]]; then
            local baseline_name
            baseline_name=$(basename "$baseline_file" .json)
            if [[ "${baseline_name,,}" == "${gpu,,}" ]]; then
                echo "$baseline_file"
                return 0
            fi
        fi
    done

    # Try partial match
    for baseline_file in "$DEFAULT_BASELINE_DIR"/*.json; do
        if [[ -f "$baseline_file" ]]; then
            local baseline_name
            baseline_name=$(basename "$baseline_file" .json)
            if [[ "${baseline_name,,}" == *"${gpu,,}"* ]] || [[ "${gpu,,}" == *"${baseline_name,,}"* ]]; then
                log_warning "Using partial match: $baseline_name for $gpu"
                echo "$baseline_file"
                return 0
            fi
        fi
    done

    log_error "No baseline found for GPU: $gpu"
    log_info "Available baselines:"
    ls -1 "$DEFAULT_BASELINE_DIR"/*.json 2>/dev/null | sed 's/.*\///' | sed 's/\.json$//' || echo "  No baselines found"
    return 2
}

# Validate baseline file integrity
validate_baseline() {
    local baseline_file="$1"

    log_verbose "Validating baseline file: $baseline_file"

    if [[ ! -f "$baseline_file" ]]; then
        log_error "Baseline file not found: $baseline_file"
        return 2
    fi

    # Check SHA-256 integrity if enabled
    if [[ "$ENABLE_SHA256_VALIDATION" == "true" ]]; then
        log_verbose "Validating SHA-256 integrity..."

        if ! python3 -c "
import json
import hashlib

try:
    with open('$baseline_file', 'r') as f:
        data = json.load(f)

    # Check for SHA-256 digest
    if 'metadata' in data and 'sha256_digest' in data['metadata']:
        stored_digest = data['metadata']['sha256_digest']

        # Recalculate digest
        content = json.dumps(data, separators=(',', ':'), sort_keys=True)
        calculated_digest = hashlib.sha256(content.encode()).hexdigest()

        if stored_digest != calculated_digest:
            print('ERROR: SHA-256 digest mismatch')
            print(f'Stored: {stored_digest}')
            print(f'Calculated: {calculated_digest}')
            exit(1)
        else:
            print('SHA-256 validation passed')
    else:
        print('WARNING: No SHA-256 digest found in baseline')

except Exception as e:
    print(f'ERROR: Baseline validation failed: {e}')
    exit(2)
" 2>/dev/null; then
            log_error "Baseline SHA-256 validation failed"
            return 2
        fi
    fi

    # Check if it's valid JSON
    if ! python3 -c "import json; json.load(open('$baseline_file'))" 2>/dev/null; then
        log_error "Invalid JSON in baseline file: $baseline_file"
        return 2
    fi

    # Check for required fields
    local required_fields=("device_name" "metrics" "metadata")
    for field in "${required_fields[@]}"; do
        if ! python3 -c "import json; data=json.load(open('$baseline_file')); assert '$field' in data" 2>/dev/null; then
            log_error "Missing required field in baseline: $field"
            return 2
        fi
    done

    log_verbose "Baseline file validation passed"
    return 0
}

# Run performance test with telemetry
run_performance_test() {
    log_info "Starting performance test with telemetry..."
    log_info "Test duration: ${TEST_DURATION}s, Warmup: ${WARMUP_TIME}s"

    local test_start_time
    test_start_time=$(date +%s)

    local output_file="$OUTPUT_DIR/performance_test_$(date +%Y%m%d_%H%M%S).log"
    local telemetry_file="$OUTPUT_DIR/telemetry_$(date +%Y%m%d_%H%M%S).json"

    if [[ "$DRY_RUN" == "true" ]]; then
        log_info "[DRY RUN] Would run: $PERFORMANCE_TEST_BINARY --duration $TEST_DURATION --warmup $WARMUP_TIME"
        return 0
    fi

    # Build test command
    local test_command="$PERFORMANCE_TEST_BINARY --duration $TEST_DURATION --warmup $WARMUP_TIME"

    if [[ "$SKIP_TELEMETRY" != "true" ]]; then
        test_command="$test_command --telemetry --telemetry-output $telemetry_file"
        log_verbose "Telemetry enabled: $telemetry_file"
    fi

    log_verbose "Running command: $test_command"

    # Run performance test with timeout protection
    local timeout_duration=$((TEST_DURATION + WARMUP_TIME + 60))  # Add 1 minute buffer

    if timeout "$timeout_duration" $test_command > "$output_file" 2>&1; then
        local test_end_time
        test_end_time=$(date +%s)
        local actual_duration=$((test_end_time - test_start_time))

        log_success "Performance test completed in ${actual_duration}s"
        log_verbose "Output saved to: $output_file"
        return 0
    else
        local exit_code=$?
        local test_end_time
        test_end_time=$(date +%s)
        local actual_duration=$((test_end_time - test_start_time))

        log_error "Performance test failed after ${actual_duration}s (exit code: $exit_code)"
        log_info "Check output file: $output_file"

        # Show error details
        if [[ -f "$output_file" ]]; then
            log_error "Last 20 lines of test output:"
            tail -20 "$output_file" >&2
        fi

        return 3
    fi
}

# Parse performance test results
parse_results() {
    local log_file="$1"

    log_verbose "Parsing performance test results..."

    if [[ ! -f "$log_file" ]]; then
        log_error "Performance test log file not found: $log_file"
        return 3
    fi

    # Extract performance metrics
    local throughput gpu_utilization memory_bandwidth power_usage temperature

    throughput=$(grep -i "throughput\|keys/s\|mkeys/s" "$log_file" | tail -1 | grep -o '[0-9.]*' | head -1 || echo "0")
    gpu_utilization=$(grep -i "gpu utilization\|gpu util" "$log_file" | tail -1 | grep -o '[0-9.]*' | head -1 || echo "0")
    memory_bandwidth=$(grep -i "memory bandwidth\|mem bw" "$log_file" | tail -1 | grep -o '[0-9.]*' | head -1 || echo "0")
    power_usage=$(grep -i "power\|watts" "$log_file" | tail -1 | grep -o '[0-9.]*' | head -1 || echo "0")
    temperature=$(grep -i "temperature\|celsius" "$log_file" | tail -1 | grep -o '[0-9.]*' | head -1 || echo "0")

    # Create current metrics JSON
    local current_metrics_file="$OUTPUT_DIR/current_metrics.json"
    cat > "$current_metrics_file" << EOF
{
    "device_name": "$(detect_gpu)",
    "timestamp": $(date +%s),
    "metrics": {
        "throughput_mkeys_per_sec": $throughput,
        "gpu_utilization_percent": $gpu_utilization,
        "memory_bandwidth_gbps": $memory_bandwidth,
        "power_consumption_watts": $power_usage,
        "temperature_celsius": $temperature
    },
    "test_config": {
        "duration_seconds": $TEST_DURATION,
        "warmup_seconds": $WARMUP_TIME,
        "binary": "$PERFORMANCE_TEST_BINARY",
        "regression_threshold_percent": $REGRESSION_THRESHOLD
    }
}
EOF

    log_info "Current performance metrics:"
    log_info "  Throughput: ${throughput} Mkeys/s"
    log_info "  GPU Utilization: ${gpu_utilization}%"
    log_info "  Memory Bandwidth: ${memory_bandwidth} GB/s"
    log_info "  Power Usage: ${power_usage} W"
    log_info "  Temperature: ${temperature}°C"

    echo "$current_metrics_file"
}

# Compare against baseline with zero-tolerance policy
compare_with_baseline() {
    local current_metrics_file="$1"
    local baseline_file="$2"

    log_verbose "Comparing with baseline: $baseline_file"

    # Use Python for detailed comparison
    local comparison_result_file="$OUTPUT_DIR/comparison_result.json"

    if ! python3 << EOF
import json
import sys

try:
    # Load current metrics
    with open('$current_metrics_file', 'r') as f:
        current = json.load(f)

    # Load baseline
    with open('$baseline_file', 'r') as f:
        baseline = json.load(f)

    # Extract performance metrics
    current_throughput = current['metrics']['throughput_mkeys_per_sec']
    baseline_throughput = baseline['metrics']['throughput_mkeys_per_sec']

    if baseline_throughput == 0:
        print("ERROR: Baseline throughput is zero")
        sys.exit(1)

    # Calculate performance difference
    throughput_diff = ((current_throughput - baseline_throughput) / baseline_throughput) * 100

    # Zero-tolerance policy: any negative delta is a regression
    is_regression = throughput_diff < 0
    within_tolerance = not is_regression

    result = {
        "baseline_file": "$baseline_file",
        "current_metrics": current,
        "baseline_metrics": baseline,
        "throughput_diff_percent": throughput_diff,
        "is_regression": is_regression,
        "within_tolerance": within_tolerance,
        "regression_threshold_percent": $REGRESSION_THRESHOLD,
        "comparison_timestamp": $(date +%s),
        "zero_tolerance_policy": True
    }

    # Save result
    with open('$comparison_result_file', 'w') as f:
        json.dump(result, f, indent=2)

    print(f"Throughput difference: {throughput_diff:.2f}%")

    if is_regression:
        print(f"REGRESSION DETECTED: {throughput_diff:.2f}% < 0%")
        print("ZERO-TOLERANCE POLICY: Any performance degradation = CI FAILURE")
        sys.exit(1)
    else:
        print(f"Performance improvement or neutral: {throughput_diff:.2f}%")
        print("ZERO-TOLERANCE POLICY: No degradation = CI PASS")
        sys.exit(0)

except Exception as e:
    print(f"ERROR: Comparison failed: {e}")
    sys.exit(2)
EOF
    then
        return 0
    else
        return 1
    fi
}

# Generate performance baseline
generate_baseline() {
    log_info "Generating performance baseline with SHA-256 protection..."

    local gpu_name
    gpu_name=$(detect_gpu)

    local baseline_file="$OUTPUT_DIR/${gpu_name}_baseline_$(date +%Y%m%d_%H%M%S).json"

    if [[ "$DRY_RUN" == "true" ]]; then
        log_info "[DRY RUN] Would generate baseline: $baseline_file"
        return 0
    fi

    # Run performance test
    if ! run_performance_test; then
        log_error "Performance test failed during baseline generation"
        return 3
    fi

    # Find the most recent test log
    local latest_log
    latest_log=$(ls -t "$OUTPUT_DIR"/performance_test_*.log 2>/dev/null | head -1)

    if [[ -z "$latest_log" ]]; then
        log_error "No performance test log found"
        return 3
    fi

    # Parse results
    local current_metrics_file
    current_metrics_file=$(parse_results "$latest_log")

    # Create baseline with comprehensive metadata
    local content_without_digest
    content_without_digest=$(python3 -c "
import json
import sys
from datetime import datetime

# Load current metrics
with open('$current_metrics_file', 'r') as f:
    current = json.load(f)

# Create baseline
baseline = {
    'metadata': {
        'file_version': '2.0',
        'creation_tool': 'performance_gate.sh',
        'creation_tool_version': '$SCRIPT_VERSION',
        'creation_time': int(datetime.now().timestamp()),
        'creator_name': 'CI System',
        'description': 'Auto-generated performance baseline for $gpu_name',
        'tags': ['auto-generated', 'ci', '$gpu_name', 'zero-tolerance'],
        'sha256_digest': '',
        'is_protected': True
    },
    'device_name': '$gpu_name',
    'baseline_id': '${gpu_name}_$(date +%Y%m%d_%H%M%S)',
    'metrics': current['metrics'],
    'test_config': current['test_config'],
    'creation_environment': {
        'ci_mode': $CI_MODE,
        'hostname': '$(hostname)',
        'platform': '$(uname -s)',
        'git_commit': '$(git rev-parse HEAD 2>/dev/null || echo unknown)',
        'git_branch': '$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo unknown)',
        'regression_policy': 'zero_tolerance'
    }
}

print(json.dumps(baseline, separators=(',', ':'), sort_keys=True))
")

    # Calculate SHA-256 digest
    local sha256_digest
    sha256_digest=$(echo "$content_without_digest" | sha256sum | cut -d' ' -f1)

    # Create final baseline with digest
    cat > "$baseline_file" << EOF
$content_without_digestEOF

    # Update baseline with SHA-256 digest
    python3 -c "
import json

with open('$baseline_file', 'r') as f:
    data = json.load(f)

data['metadata']['sha256_digest'] = '$sha256_digest'

with open('$baseline_file', 'w') as f:
    json.dump(data, f, indent=2)
"

    log_success "Baseline generated: $baseline_file"
    log_info "SHA-256 Digest: $sha256_digest"

    # Move to baseline directory if it exists and is writable
    if [[ -d "$DEFAULT_BASELINE_DIR" ]] && [[ -w "$DEFAULT_BASELINE_DIR" ]]; then
        local final_baseline_file="$DEFAULT_BASELINE_DIR/${gpu_name}.json"
        mv "$baseline_file" "$final_baseline_file"
        baseline_file="$final_baseline_file"
        log_info "Baseline moved to: $baseline_file"
    fi

    # Validate the generated baseline
    if validate_baseline "$baseline_file"; then
        log_success "Baseline validation passed"
    else
        log_error "Baseline validation failed"
        return 2
    fi

    return 0
}

# Generate CI performance report
generate_ci_report() {
    log_verbose "Generating CI performance report..."

    local report_file="$OUTPUT_DIR/ci_performance_report_$(date +%Y%m%d_%H%M%S).json"

    python3 -c "
import json
from datetime import datetime
import os

# Collect CI environment information
ci_env = {
    'ci_mode': $CI_MODE,
    'hostname': os.uname().nodename,
    'platform': os.uname().sysname,
    'git_commit': os.popen('git rev-parse HEAD 2>/dev/null || echo unknown').read().strip(),
    'git_branch': os.popen('git rev-parse --abbrev-ref HEAD 2>/dev/null || echo unknown').read().strip(),
    'script_version': '$SCRIPT_VERSION'
}

# Create CI report
report = {
    'ci_metadata': {
        'timestamp': datetime.utcnow().isoformat() + 'Z',
        'performance_gate_version': '$SCRIPT_VERSION',
        'zero_tolerance_policy': True,
        'regression_threshold': $REGRESSION_THRESHOLD,
        'environment': ci_env
    },
    'test_configuration': {
        'gpu_name': '$GPU_NAME',
        'baseline_file': '$BASELINE_FILE',
        'test_duration': $TEST_DURATION,
        'warmup_time': $WARMUP_TIME,
        'sha256_validation': $ENABLE_SHA256_VALIDATION
    }
}

# Add comparison results if available
comparison_file = '$OUTPUT_DIR/comparison_result.json'
if os.path.exists(comparison_file):
    with open(comparison_file, 'r') as f:
        comparison = json.load(f)
    report['performance_comparison'] = comparison
    report['ci_result'] = {
        'status': 'PASS' if not comparison.get('is_regression', True) else 'FAIL',
        'reason': 'No regression detected' if not comparison.get('is_regression', True) else 'Performance regression detected'
    }
else:
    report['ci_result'] = {
        'status': 'UNKNOWN',
        'reason': 'No comparison results available'
    }

# Save CI report
with open('$report_file', 'w') as f:
    json.dump(report, f, indent=2)

print(f'CI performance report saved: $report_file')
"

    log_verbose "CI report generated: $report_file"
}

# Main execution
main() {
    log_info "Enhanced CI Performance Gate Script v$SCRIPT_VERSION"
    log_info "Zero-Tolerance Performance Regression Detection"
    log_info "SHA-256 Protected Baseline Validation"

    if [[ "$CI_MODE" == "true" ]]; then
        log_ci "Running in CI mode"
        log_ci "Environment: ${CI_SERVER_NAME:-Unknown}"
        log_ci "Pipeline: ${CI_PIPELINE_ID:-Unknown}"
        log_ci "Branch: ${CI_BRANCH:-Unknown}"
        log_ci "Commit: ${CI_COMMIT_SHA:-Unknown}"
    fi

    # Parse arguments
    parse_arguments "$@"

    # Validate configuration
    if ! validate_configuration; then
        exit 2
    fi

    # Auto-detect GPU if needed
    if [[ "$GPU_NAME" == "auto" ]] || [[ -z "$GPU_NAME" ]]; then
        GPU_NAME=$(detect_gpu)
        if [[ -z "$GPU_NAME" ]]; then
            exit 3
        fi
    fi

    log_info "Target GPU: $GPU_NAME"
    log_info "Regression Threshold: ${REGRESSION_THRESHOLD}%"
    log_info "Test Duration: ${TEST_DURATION}s"

    # Handle baseline generation mode
    if [[ "$GENERATE_BASELINE" == "true" ]]; then
        generate_baseline
        exit $?
    fi

    # Find baseline file
    if [[ -z "$BASELINE_FILE" ]]; then
        BASELINE_FILE=$(find_baseline "$GPU_NAME")
        if [[ -z "$BASELINE_FILE" ]]; then
            log_error "No baseline found for GPU: $GPU_NAME"
            log_info "Generate a baseline first with: $0 --generate-baseline"
            exit 2
        fi
    fi

    log_info "Using baseline: $BASELINE_FILE"

    # Validate baseline
    if ! validate_baseline "$BASELINE_FILE"; then
        exit 2
    fi

    # Run performance test
    if ! run_performance_test; then
        exit 3
    fi

    # Parse results
    local latest_log
    latest_log=$(ls -t "$OUTPUT_DIR"/performance_test_*.log 2>/dev/null | head -1)

    if [[ -z "$latest_log" ]]; then
        log_error "No performance test results found"
        exit 3
    fi

    local current_metrics_file
    current_metrics_file=$(parse_results "$latest_log")

    # Compare with baseline
    local regression_detected=false
    if ! compare_with_baseline "$current_metrics_file" "$BASELINE_FILE"; then
        regression_detected=true
    fi

    # Generate CI report
    generate_ci_report

    # Final CI decision
    echo
    if [[ "$CI_MODE" == "true" ]]; then
        log_ci "CI Performance Gate Decision:"
    else
        log_info "Performance Gate Decision:"
    fi

    if $regression_detected; then
        log_error "🚨 PERFORMANCE GATE FAILED"
        log_error "Reason: Performance regression detected (zero-tolerance policy)"
        log_error "Action: Required performance optimization before merge"
        log_error "Impact: Build FAILED - cannot proceed with merge"
        exit 1
    else
        log_success "✅ PERFORMANCE GATE PASSED"
        log_success "Reason: No performance regression detected"
        log_success "Action: Approved for merge"
        log_success "Impact: Build PASSED - can proceed with CI pipeline"
        exit 0
    fi
}

# Script entry point
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
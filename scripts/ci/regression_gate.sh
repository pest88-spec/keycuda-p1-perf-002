#!/bin/bash

# Puzzle71Solver - Performance Regression Gate Script
# Zero-tolerance performance regression detection for CI/CD pipelines
# Part of T011: Create performance regression testing framework

set -euo pipefail

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
BENCHMARKS_DIR="$PROJECT_ROOT/benchmarks"
REGRESSION_RESULTS_DIR="$BENCHMARKS_DIR/regression_results"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} RegressionGate: $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} RegressionGate: $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} RegressionGate: $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} RegressionGate: $1"
}

# Function to display usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS] <GPU_NAME>

Performance regression gate for CI/CD pipelines with zero-tolerance policy.

Arguments:
    GPU_NAME        Target GPU device name for regression testing

Options:
    --help, -h      Display this help message
    --verbose       Enable verbose logging
    --strict        Enable strict mode (fail on warnings)
    --baseline-dir  Specify baseline directory (default: benchmarks/baselines)
    --results-dir   Specify results directory (default: benchmarks/regression_results)
    --timeout       Set test timeout in minutes (default: 30)
    --update        Update baseline if improvement detected
    --dry-run       Run tests without blocking CI

Examples:
    $0 rtx3090
    $0 --verbose --strict rtx2080ti
    $0 --baseline-dir custom/baselines --timeout 60 a100

Exit Codes:
    0   Success - No regressions detected
    1   Regression detected - CI blocked
    2   Configuration or execution error
    3   Timeout or infrastructure failure

EOF
}

# Parse command line arguments
GPU_NAME=""
VERBOSE=false
STRICT=false
BASELINE_DIR="$BENCHMARKS_DIR/baselines"
RESULTS_DIR="$REGRESSION_RESULTS_DIR"
TIMEOUT_MINUTES=30
UPDATE_BASELINE=false
DRY_RUN=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --help|-h)
            usage
            exit 0
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --strict)
            STRICT=true
            shift
            ;;
        --baseline-dir)
            BASELINE_DIR="$2"
            shift 2
            ;;
        --results-dir)
            RESULTS_DIR="$2"
            shift 2
            ;;
        --timeout)
            TIMEOUT_MINUTES="$2"
            shift 2
            ;;
        --update)
            UPDATE_BASELINE=true
            shift
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        -*)
            log_error "Unknown option: $1"
            usage
            exit 2
            ;;
        *)
            if [[ -z "$GPU_NAME" ]]; then
                GPU_NAME="$1"
            else
                log_error "Multiple GPU names specified: $GPU_NAME and $1"
                usage
                exit 2
            fi
            shift
            ;;
    esac
done

# Validate arguments
if [[ -z "$GPU_NAME" ]]; then
    log_error "GPU_NAME is required"
    usage
    exit 2
fi

# Set verbose mode
if [[ "$VERBOSE" == true ]]; then
    set -x
    log_info "Verbose mode enabled"
fi

log_info "Starting performance regression gate for GPU: $GPU_NAME"
log_info "Zero-tolerance policy: Any performance regression will block CI"

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."

    # Check if build directory exists
    if [[ ! -d "$BUILD_DIR" ]]; then
        log_error "Build directory not found: $BUILD_DIR"
        log_error "Please build the project first with: cmake && make"
        exit 2
    fi

    # Check if benchmark executable exists
    local benchmark_exe="$BUILD_DIR/keyhunt_benchmarks"
    if [[ ! -f "$benchmark_exe" ]]; then
        log_error "Benchmark executable not found: $benchmark_exe"
        log_error "Please build benchmarks first with: make keyhunt_benchmarks"
        exit 2
    fi

    # Check if baseline exists
    local baseline_file="$BASELINE_DIR/${GPU_NAME}.json"
    if [[ ! -f "$baseline_file" ]]; then
        log_error "Baseline file not found: $baseline_file"
        log_error "Please establish a baseline first with: ./scripts/run_benchmarks.sh $GPU_NAME"
        exit 2
    fi

    # Check CUDA availability
    if ! command -v nvidia-smi &> /dev/null; then
        log_error "nvidia-smi not found. CUDA environment required."
        exit 2
    fi

    # Check if target GPU is available
    if ! nvidia-smi --query-gpu=name --format=csv,noheader | grep -iq "$GPU_NAME"; then
        log_warning "GPU '$GPU_NAME' not found in nvidia-smi output"
        log_warning "Available GPUs:"
        nvidia-smi --query-gpu=name --format=csv,noheader | sed 's/^/  - /'
        if [[ "$STRICT" == true ]]; then
            exit 2
        else
            log_warning "Continuing anyway (may fail if GPU is not available)"
        fi
    fi

    # Create results directory
    mkdir -p "$RESULTS_DIR"

    log_success "Prerequisites check passed"
}

# Execute regression tests
run_regression_tests() {
    log_info "Executing regression tests..."

    local benchmark_exe="$BUILD_DIR/keyhunt_benchmarks"
    local test_start_time=$(date +%s)
    local timeout_seconds=$((TIMEOUT_MINUTES * 60))

    # Prepare test command
    local test_cmd=(
        "$benchmark_exe"
        "--regression-test"
        "--gpu-name" "$GPU_NAME"
        "--baseline-dir" "$BASELINE_DIR"
        "--results-dir" "$RESULTS_DIR"
        "--timeout" "$TIMEOUT_MINUTES"
        "--zero-tolerance"
    )

    if [[ "$VERBOSE" == true ]]; then
        test_cmd+=("--verbose")
    fi

    if [[ "$STRICT" == true ]]; then
        test_cmd+=("--strict")
    fi

    log_info "Running: ${test_cmd[*]}"

    # Execute tests with timeout
    local test_result=0
    if [[ "$DRY_RUN" == true ]]; then
        log_info "DRY RUN: Would execute regression tests"
        return 0
    fi

    if ! timeout "$timeout_seconds" "${test_cmd[@]}" 2>&1; then
        test_result=$?
        if [[ $test_result -eq 124 ]]; then
            log_error "Regression tests timed out after ${TIMEOUT_MINUTES} minutes"
            return 3
        else
            log_error "Regression tests failed with exit code: $test_result"
            return 1
        fi
    fi

    local test_end_time=$(date +%s)
    local test_duration=$((test_end_time - test_start_time))
    log_success "Regression tests completed in ${test_duration} seconds"

    return 0
}

# Analyze results and generate report
analyze_results() {
    log_info "Analyzing regression test results..."

    # Find the latest results file for this GPU
    local latest_results=$(find "$RESULTS_DIR" -name "*_${GPU_NAME}.json" -type f -printf '%T@ %p\n' | sort -n | tail -1 | cut -d' ' -f2-)

    if [[ -z "$latest_results" ]]; then
        log_error "No results file found for GPU: $GPU_NAME"
        return 2
    fi

    log_info "Analyzing results file: $latest_results"

    # Parse results using jq if available, otherwise use basic text parsing
    if command -v jq &> /dev/null; then
        analyze_results_with_jq "$latest_results"
    else
        analyze_results_text "$latest_results"
    fi
}

# Analyze results using jq (JSON parser)
analyze_results_with_jq() {
    local results_file="$1"

    local overall_status=$(jq -r '.overall_status // "unknown"' "$results_file")
    local total_tests=$(jq -r '.test_summary.total_tests // 0' "$results_file")
    local passed_tests=$(jq -r '.test_summary.passed_tests // 0' "$results_file")
    local failed_tests=$(jq -r '.test_summary.failed_tests // 0' "$results_file)
    local warning_tests=$(jq -r '.test_summary.warning_tests // 0' "$results_file")
    local error_tests=$(jq -r '.test_summary.error_tests // 0' "$results_file")

    log_info "Test Summary:"
    log_info "  Overall Status: $overall_status"
    log_info "  Total Tests: $total_tests"
    log_info "  Passed: $passed_tests"
    log_info "  Failed: $failed_tests"
    log_info "  Warnings: $warning_tests"
    log_info "  Errors: $error_tests"

    # Check for regressions
    if [[ "$overall_status" == "1" || "$overall_status" == "2" ]]; then  # FAILED or ERROR
        log_error "PERFORMANCE REGRESSION DETECTED!"
        log_error "Zero-tolerance policy: CI is BLOCKED"

        # Show failed tests
        log_error "Failed tests:"
        jq -r '.test_results[] | select(.status == 1 or .status == 2) | "  - \(.test_name): \(.description)"' "$results_file" | while read -r line; do
            log_error "$line"
        done

        return 1
    elif [[ "$overall_status" == "3" && "$STRICT" == true ]]; then  # WARNING with strict mode
        log_error "PERFORMANCE WARNING DETECTED (STRICT MODE)!"
        log_error "Strict mode: CI is BLOCKED"

        # Show warning tests
        log_error "Warning tests:"
        jq -r '.test_results[] | select(.status == 3) | "  - \(.test_name): \(.description)"' "$results_file" | while read -r line; do
            log_error "$line"
        done

        return 1
    else
        log_success "No performance regressions detected"
        return 0
    fi
}

# Fallback text analysis when jq is not available
analyze_results_text() {
    local results_file="$1"

    log_info "Text-based analysis (jq not available)"

    # Look for key indicators in the JSON file
    if grep -q '"overall_status": 1' "$results_file" || grep -q '"overall_status": 2' "$results_file"; then
        log_error "PERFORMANCE REGRESSION DETECTED!"
        log_error "Zero-tolerance policy: CI is BLOCKED"
        return 1
    elif grep -q '"overall_status": 3' "$results_file" && [[ "$STRICT" == true ]]; then
        log_error "PERFORMANCE WARNING DETECTED (STRICT MODE)!"
        log_error "Strict mode: CI is BLOCKED"
        return 1
    else
        log_success "No performance regressions detected"
        return 0
    fi
}

# Update baseline if improvement detected
update_baseline_if_needed() {
    if [[ "$UPDATE_BASELINE" != true ]]; then
        return 0
    fi

    log_info "Checking for performance improvements to update baseline..."

    local latest_results=$(find "$RESULTS_DIR" -name "*_${GPU_NAME}.json" -type f -printf '%T@ %p\n' | sort -n | tail -1 | cut -d' ' -f2-)

    if [[ -z "$latest_results" ]]; then
        log_warning "No results file found for baseline update analysis"
        return 0
    fi

    # Check if improvement detected (this would require the benchmark tool to support baseline updates)
    log_info "Baseline update logic would be implemented here"
    log_info "For now, manual baseline updates are required"
}

# Generate summary report
generate_summary_report() {
    log_info "Generating summary report..."

    local summary_file="$RESULTS_DIR/regression_gate_summary_${GPU_NAME}_$(date +%Y%m%d_%H%M%S).txt"

    cat > "$summary_file" << EOF
Puzzle71Solver Performance Regression Gate Summary
================================================

GPU Name: $GPU_NAME
Timestamp: $(date)
Configuration:
  Strict Mode: $STRICT
  Zero Tolerance: enabled
  Baseline Directory: $BASELINE_DIR
  Results Directory: $RESULTS_DIR
  Timeout: ${TIMEOUT_MINUTES} minutes

Results:
  Exit Code: $?
  Status: $([ $? -eq 0 ] && echo "PASSED - No regressions" || echo "FAILED - Regression detected")

Next Steps:
  - Review detailed results in JSON format
  - If regression detected: investigate performance changes
  - If improvement detected: consider baseline update
  - Monitor trends over time

EOF

    log_info "Summary report saved to: $summary_file"
}

# Main execution
main() {
    log_info "Puzzle71Solver Performance Regression Gate v1.0"
    log_info "Zero-tolerance performance regression detection enabled"

    # Execute pipeline
    check_prerequisites
    run_regression_tests
    local test_result=$?

    if [[ $test_result -eq 0 ]]; then
        analyze_results
        local analysis_result=$?

        if [[ $analysis_result -eq 0 ]]; then
            update_baseline_if_needed
            generate_summary_report
            log_success "Performance regression gate PASSED"
            log_success "No regressions detected - CI may proceed"
            exit 0
        else
            generate_summary_report
            log_error "Performance regression gate FAILED"
            log_error "Regressions detected - CI BLOCKED by zero-tolerance policy"
            exit 1
        fi
    else
        generate_summary_report
        log_error "Performance regression gate FAILED"
        log_error "Test execution failed - CI BLOCKED"
        exit $test_result
    fi
}

# Execute main function
main "$@"
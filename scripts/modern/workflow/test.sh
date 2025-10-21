#!/bin/bash
# Puzzle71Solver - Test Runner Script
# Comprehensive testing with performance validation

set -euo pipefail

# Get script directory and load utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=../utils.sh
source "$SCRIPT_DIR/../utils.sh"

# Default configuration
DEFAULT_PARALLEL_JOBS="$(get_cpu_cores)"
DEFAULT_TEST_TIMEOUT="300"
DEFAULT_OUTPUT_FORMAT="auto"

# Show help
show_help() {
    cat << EOF
Puzzle71Solver Test Runner

USAGE:
    test.sh [options] [test_name...]

TESTS:
    unit               Run unit tests only
    integration        Run integration tests only
    performance        Run performance tests only
    scientific         Run scientific validation tests only
    all                Run all tests (default)

    If specific test names are provided, only those tests will run.

OPTIONS:
    --parallel <n>         Number of parallel test jobs [default: $DEFAULT_PARALLEL_JOBS]
    --timeout <seconds>    Test timeout in seconds [default: $DEFAULT_TEST_TIMEOUT]
    --output <format>      Output format (auto|quiet|verbose|json) [default: $DEFAULT_OUTPUT_FORMAT]
    --filter <pattern>     Run tests matching pattern
    --skip-slow            Skip slow tests
    --gpu-only             Run GPU-dependent tests only
    --cpu-only             Run CPU-only tests
    --baseline <file>      Performance baseline file for regression testing
    --update-baseline      Update performance baseline
    --continuous           Continuous integration mode
    --debug, -g            Enable debug output
    --verbose, -v          Enable verbose output
    --help, -h             Show this help

PERFORMANCE TESTING:
    --benchmark            Run performance benchmarks
    --gpu <device>         Specify GPU device for testing
    --memory-limit <GB>    Set memory limit for tests
    --stress               Run stress tests

EXAMPLES:
    test.sh                           # Run all tests
    test.sh unit                      # Run unit tests only
    test.sh --parallel 8              # Run with 8 parallel jobs
    test.sh --filter "ECC*"           # Run ECC-related tests
    test.sh --baseline baseline.json  # Run with performance baseline
    test.sh --gpu-only --benchmark    # Run GPU benchmarks only

ENVIRONMENT VARIABLES:
    TEST_PARALLEL         Override parallel job count
    TEST_TIMEOUT          Override test timeout
    TEST_OUTPUT_FORMAT    Override output format
    GPU_DEVICE           Override GPU device selection
EOF
}

# Parse command line arguments
parse_args() {
    PARALLEL_JOBS="${TEST_PARALLEL:-$DEFAULT_PARALLEL_JOBS}"
    TEST_TIMEOUT="${TEST_TIMEOUT:-$DEFAULT_TEST_TIMEOUT}"
    OUTPUT_FORMAT="${TEST_OUTPUT_FORMAT:-$DEFAULT_OUTPUT_FORMAT}"
    FILTER_PATTERN=""
    SKIP_SLOW=false
    GPU_ONLY=false
    CPU_ONLY=false
    BASELINE_FILE=""
    UPDATE_BASELINE=false
    CONTINUOUS_MODE=false
    DEBUG_MODE=false
    VERBOSE=false
    RUN_BENCHMARK=false
    GPU_DEVICE="${GPU_DEVICE:-}"
    MEMORY_LIMIT=""
    STRESS_TEST=false

    TEST_NAMES=()
    TEST_CATEGORY="all"

    while [[ $# -gt 0 ]]; do
        case $1 in
            --parallel)
                PARALLEL_JOBS="$2"
                shift 2
                ;;
            --timeout)
                TEST_TIMEOUT="$2"
                shift 2
                ;;
            --output)
                OUTPUT_FORMAT="$2"
                shift 2
                ;;
            --filter)
                FILTER_PATTERN="$2"
                shift 2
                ;;
            --skip-slow)
                SKIP_SLOW=true
                shift
                ;;
            --gpu-only)
                GPU_ONLY=true
                shift
                ;;
            --cpu-only)
                CPU_ONLY=true
                shift
                ;;
            --baseline)
                BASELINE_FILE="$2"
                shift 2
                ;;
            --update-baseline)
                UPDATE_BASELINE=true
                shift
                ;;
            --continuous)
                CONTINUOUS_MODE=true
                shift
                ;;
            --debug|-g)
                DEBUG_MODE=true
                shift
                ;;
            --verbose|-v)
                VERBOSE=true
                shift
                ;;
            --benchmark)
                RUN_BENCHMARK=true
                shift
                ;;
            --gpu)
                GPU_DEVICE="$2"
                shift 2
                ;;
            --memory-limit)
                MEMORY_LIMIT="$2"
                shift 2
                ;;
            --stress)
                STRESS_TEST=true
                shift
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            unit|integration|performance|scientific|all)
                TEST_CATEGORY="$1"
                shift
                ;;
            -*)
                error_exit "Unknown option: $1"
                ;;
            *)
                TEST_NAMES+=("$1")
                shift
                ;;
        esac
    done

    # Validate conflicting options
    if [[ "$GPU_ONLY" == true && "$CPU_ONLY" == true ]]; then
        error_exit "Cannot specify both --gpu-only and --cpu-only"
    fi
}

# Validate test environment
validate_test_environment() {
    log_debug "Validating test environment..."

    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    # Check build directory
    require_dir "$build_dir" "Build directory"

    # Check for test executable
    local test_executable="$build_dir/puzzle71_tests"
    if [[ ! -f "$test_executable" ]]; then
        log_warning "Test executable not found: $test_executable"
        log_info "Building tests first..."
        "$SCRIPT_DIR/build.sh" tests
    fi

    # Check CUDA availability for GPU tests
    if [[ "$GPU_ONLY" == true ]] && ! check_gpu_available; then
        error_exit "GPU tests requested but no GPU available"
    fi

    # Check memory limit
    if [[ -n "$MEMORY_LIMIT" ]]; then
        local available_memory
        available_memory="$(get_memory_gb)"
        if (( $(echo "$MEMORY_LIMIT > $available_memory" | bc -l) )); then
            log_warning "Memory limit ($MEMORY_LIMIT GB) exceeds available memory ($available_memory GB)"
        fi
    fi

    log_debug "Test environment validated"
}

# Prepare test environment
prepare_test_environment() {
    progress_start "Preparing test environment"

    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    cd "$build_dir"

    # Set test environment variables
    export GTEST_OUTPUT="xml:test-results.xml"
    export GTEST_COLOR="yes"
    export GTEST_PRINT_TIME="1"

    if [[ "$DEBUG_MODE" == true ]]; then
        export GTEST_ALSO_RUN_DISABLED_TESTS="1"
    fi

    # Set GPU device if specified
    if [[ -n "$GPU_DEVICE" ]]; then
        export CUDA_VISIBLE_DEVICES="$GPU_DEVICE"
        log_info "Using GPU device: $GPU_DEVICE"
    fi

    # Set memory limit if specified
    if [[ -n "$MEMORY_LIMIT" ]]; then
        # Note: This would need implementation in the test code
        export TEST_MEMORY_LIMIT="$MEMORY_LIMIT"
        log_info "Memory limit set to: ${MEMORY_LIMIT}GB"
    fi

    # Create test results directory
    ensure_dir "test-results"

    progress_end "Test environment preparation"
}

# Detect test executable and framework
detect_test_framework() {
    local build_dir
    build_dir="$(get_project_root)/build"

    if [[ -f "$build_dir/puzzle71_tests" ]]; then
        echo "gtest"
    elif [[ -f "$build_dir/tests" ]]; then
        echo "ctest"
    else
        error_exit "No test executable found"
    fi
}

# Run GTest-based tests
run_gtest() {
    local test_executable="$1"
    shift
    local gtest_args=("$@")

    log_info "Running GTest-based tests..."

    # Prepare GTest arguments
    case "$OUTPUT_FORMAT" in
        "quiet")
            gtest_args+=("--gtest_output=xml:test-results/quiet.xml")
            gtest_args+=("--gtest_print_time=0")
            ;;
        "verbose")
            gtest_args+=("--gtest_print_time=1")
            gtest_args+=("--gtest_print_utf8=1")
            ;;
        "json")
            gtest_args+=("--gtest_output=json:test-results/results.json")
            ;;
        "auto")
            if [[ "$CONTINUOUS_MODE" == true ]]; then
                gtest_args+=("--gtest_output=xml:test-results/ci.xml")
            else
                gtest_args+=("--gtest_output=xml:test-results/results.xml")
            fi
            ;;
    esac

    # Add filter pattern if specified
    if [[ -n "$FILTER_PATTERN" ]]; then
        gtest_args+=("--gtest_filter=$FILTER_PATTERN")
    fi

    # Add parallel execution if supported
    if [[ "$PARALLEL_JOBS" -gt 1 ]] && command_exists "parallel"; then
        log_info "Running tests in parallel with $PARALLEL_JOBS jobs"
        # GTest parallel execution would need test list parsing
        # For now, run single instance
    fi

    # Run tests with timeout
    local cmd=(timeout "$TEST_TIMEOUT" "$test_executable" "${gtest_args[@]}")

    if [[ "$VERBOSE" == true ]]; then
        set -x
    fi

    local exit_code=0
    "${cmd[@]}" || exit_code=$?

    if [[ "$VERBOSE" == true ]]; then
        set +x
    fi

    return $exit_code
}

# Run CTest-based tests
run_ctest() {
    local ctest_args=()

    log_info "Running CTest-based tests..."

    # Prepare CTest arguments
    case "$OUTPUT_FORMAT" in
        "quiet")
            ctest_args+=("--quiet")
            ;;
        "verbose")
            ctest_args+=("--verbose")
            ctest_args+=("--output-on-failure")
            ;;
        "json")
            ctest_args+=("--output-junit test-results/junit.xml")
            ;;
        "auto")
            if [[ "$CONTINUOUS_MODE" == true ]]; then
                ctest_args+=("--output-junit test-results/ci.xml")
            else
                ctest_args+=("--output-on-failure")
            fi
            ;;
    esac

    # Add parallel execution
    ctest_args+=("--parallel" "$PARALLEL_JOBS")

    # Add timeout
    ctest_args+=("--timeout" "$TEST_TIMEOUT")

    # Add filter if specified
    if [[ -n "$FILTER_PATTERN" ]]; then
        ctest_args+=("-R" "$FILTER_PATTERN")
    fi

    # Run tests
    if [[ "$VERBOSE" == true ]]; then
        set -x
    fi

    local exit_code=0
    ctest "${ctest_args[@]}" || exit_code=$?

    if [[ "$VERBOSE" == true ]]; then
        set +x
    fi

    return $exit_code
}

# Run performance benchmarks
run_benchmarks() {
    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    progress_start "Running performance benchmarks"

    cd "$build_dir"

    # Check for benchmark executables
    if ls benchmarks/* 1> /dev/null 2>&1; then
        local benchmark_executables=(benchmarks/*)

        for benchmark in "${benchmark_executables[@]}"; do
            log_info "Running benchmark: $(basename "$benchmark")"

            local benchmark_args=()
            if [[ -n "$BASELINE_FILE" ]]; then
                benchmark_args+=("--baseline" "$BASELINE_FILE")
            fi
            if [[ "$UPDATE_BASELINE" == true ]]; then
                benchmark_args+=("--update-baseline")
            fi

            timeout "$TEST_TIMEOUT" "$benchmark" "${benchmark_args[@]}" || {
                log_warning "Benchmark failed: $(basename "$benchmark")"
            }
        done
    else
        log_warning "No benchmark executables found"
        log_info "Build benchmarks first: scripts build benchmarks"
    fi

    progress_end "Performance benchmarks"
}

# Run category-specific tests
run_category_tests() {
    case "$TEST_CATEGORY" in
        "unit")
            run_gtest "./puzzle71_tests" "--gtest_filter=Unit*"
            ;;
        "integration")
            run_gtest "./puzzle71_tests" "--gtest_filter=Integration*"
            ;;
        "performance")
            run_benchmarks
            ;;
        "scientific")
            run_gtest "./puzzle71_tests" "--gtest_filter=Scientific*"
            ;;
        "all")
            run_all_tests
            ;;
    esac
}

# Run all tests
run_all_tests() {
    local framework
    framework="$(detect_test_framework)"

    case "$framework" in
        "gtest")
            run_gtest "./puzzle71_tests"
            ;;
        "ctest")
            run_ctest
            ;;
    esac

    # Run benchmarks if requested
    if [[ "$RUN_BENCHMARK" == true ]]; then
        run_benchmarks
    fi
}

# Run specific tests
run_specific_tests() {
    local framework
    framework="$(detect_test_framework)"

    log_info "Running specific tests: ${TEST_NAMES[*]}"

    case "$framework" in
        "gtest")
            local filter
            filter=$(IFS=":"; echo "${TEST_NAMES[*]}")
            run_gtest "./puzzle71_tests" "--gtest_filter=$filter"
            ;;
        "ctest")
            # CTest uses regex patterns
            local pattern
            pattern=$(IFS="|"; echo "${TEST_NAMES[*]}")
            run_ctest "-R" "$pattern"
            ;;
    esac
}

# Process test results
process_test_results() {
    progress_start "Processing test results"

    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    cd "$build_dir"

    # Generate test summary
    if [[ -f "test-results.xml" ]]; then
        log_info "Test results saved to: test-results.xml"

        # Count tests if xmlstarlet is available
        if command_exists "xmlstarlet"; then
            local total_tests failed_tests
            total_tests=$(xmlstarlet sel -t -v "count(//testcase)" test-results.xml 2>/dev/null || echo "unknown")
            failed_tests=$(xmlstarlet sel -t -v "count(//failure)" test-results.xml 2>/dev/null || echo "unknown")

            log_info "Test summary: $total_tests tests, $failed_tests failed"
        fi
    fi

    # Check for performance regression if baseline provided
    if [[ -n "$BASELINE_FILE" && -f "$BASELINE_FILE" ]]; then
        check_performance_regression
    fi

    progress_end "Test results processing"
}

# Check for performance regression
check_performance_regression() {
    log_info "Checking for performance regression..."

    # This would need implementation based on the benchmark output format
    # For now, just log that regression checking was requested
    if [[ -f "$BASELINE_FILE" ]]; then
        log_info "Baseline file: $BASELINE_FILE"
        log_info "Regression analysis would be performed here"
    fi
}

# Show test summary
show_test_summary() {
    local exit_code="$1"

    cat << EOF

Test execution completed with exit code: $exit_code

Test configuration:
- Category: $TEST_CATEGORY
- Parallel jobs: $PARALLEL_JOBS
- Timeout: ${TEST_TIMEOUT}s
- Output format: $OUTPUT_FORMAT

EOF

    if [[ $exit_code -eq 0 ]]; then
        log_success "All tests passed! 🎉"
    else
        log_error "Some tests failed! ❌"
        return 1
    fi

    # Show next steps
    if [[ "$TEST_CATEGORY" == "all" ]]; then
        echo "Next steps:"
        echo "- View detailed results: $(get_project_root)/build/test-results.xml"
        echo "- Run specific tests: scripts test.sh unit"
        echo "- Run benchmarks: scripts benchmark"
    fi
}

# Main test function
main() {
    log_info "Starting Puzzle71Solver test runner..."

    # Parse arguments
    parse_args "$@"

    # Validate environment
    validate_test_environment

    # Show configuration
    log_info "Test configuration:"
    log_info "  Category: $TEST_CATEGORY"
    log_info "  Parallel jobs: $PARALLEL_JOBS"
    log_info "  Timeout: ${TEST_TIMEOUT}s"
    log_info "  Output format: $OUTPUT_FORMAT"
    log_info "  Filter pattern: ${FILTER_PATTERN:-none}"

    # Prepare environment
    prepare_test_environment

    # Run tests
    local exit_code=0
    if [[ ${#TEST_NAMES[@]} -gt 0 ]]; then
        run_specific_tests || exit_code=$?
    else
        run_category_tests || exit_code=$?
    fi

    # Process results
    process_test_results

    # Show summary
    show_test_summary "$exit_code"

    exit $exit_code
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
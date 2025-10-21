#!/bin/bash
# Architecture Modernization Integration Test Runner
# Comprehensive test suite for Phase 5: User Story 3 - Architecture Modernization

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
TEST_RESULTS_DIR="${PROJECT_ROOT}/test_results"
ARCHITECTURE_RESULTS_DIR="${TEST_RESULTS_DIR}/architecture"

# Test configuration
TIMEOUT_MINUTES=30
ENABLE_PROFILING=${ENABLE_PROFILING:-false}
ENABLE_MEMORY_CHECK=${ENABLE_MEMORY_CHECK:-false}
CI_MODE=${CI_MODE:-false}
GPU_DEVICE=${GPU_DEVICE:-0}

# Test categories
CATEGORIES=(
    "unified_candidate"
    "naming_conventions"
    "launch_configuration"
    "replay_verification"
    "memory_optimization"
    "warp_operations"
    "adaptive_batch_sizing"
    "end_to_end"
    "performance_regression"
)

# Logging function
log() {
    local level=$1
    shift
    local message="$*"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')

    case $level in
        "INFO")
            echo -e "${GREEN}[INFO]${NC} [${timestamp}] ${message}"
            ;;
        "WARN")
            echo -e "${YELLOW}[WARN]${NC} [${timestamp}] ${message}"
            ;;
        "ERROR")
            echo -e "${RED}[ERROR]${NC} [${timestamp}] ${message}"
            ;;
        "DEBUG")
            if [[ "${DEBUG:-false}" == "true" ]]; then
                echo -e "${BLUE}[DEBUG]${NC} [${timestamp}] ${message}"
            fi
            ;;
    esac
}

# Check prerequisites
check_prerequisites() {
    log "INFO" "Checking prerequisites for architecture modernization tests..."

    # Check CUDA installation
    if ! command -v nvcc &> /dev/null; then
        log "ERROR" "CUDA compiler (nvcc) not found in PATH"
        exit 1
    fi

    # Check GPU availability
    if ! nvidia-smi &> /dev/null; then
        log "ERROR" "NVIDIA GPU not available or nvidia-smi not found"
        exit 1
    fi

    # Check specified GPU device
    if ! nvidia-smi --query-gpu=index --format=csv,noheader | grep -q "^${GPU_DEVICE}$"; then
        log "ERROR" "GPU device ${GPU_DEVICE} not found"
        exit 1
    fi

    # Check build directory
    if [[ ! -d "${BUILD_DIR}" ]]; then
        log "ERROR" "Build directory ${BUILD_DIR} does not exist. Run 'make build' first."
        exit 1
    fi

    # Check test executable
    local test_exe="${BUILD_DIR}/tests/integration/test_architecture_modernization"
    if [[ ! -f "${test_exe}" ]]; then
        log "ERROR" "Test executable ${test_exe} not found. Build the tests first."
        exit 1
    fi

    # Create results directory
    mkdir -p "${ARCHITECTURE_RESULTS_DIR}"

    log "INFO" "Prerequisites check completed"
}

# Run a specific test category
run_test_category() {
    local category=$1
    local results_file="${ARCHITECTURE_RESULTS_DIR}/${category}_results.xml"
    local log_file="${ARCHITECTURE_RESULTS_DIR}/${category}.log"

    log "INFO" "Running architecture test category: ${category}"

    local start_time=$(date +%s)

    # Set GPU device
    export CUDA_VISIBLE_DEVICES=${GPU_DEVICE}

    # Run the test with appropriate filter
    local filter_pattern=""
    case ${category} in
        "unified_candidate")
            filter_pattern="*UnifiedCandidate*"
            ;;
        "naming_conventions")
            filter_pattern="*NamingConventions*"
            ;;
        "launch_configuration")
            filter_pattern="*LaunchConfiguration*"
            ;;
        "replay_verification")
            filter_pattern="*ReplayVerification*"
            ;;
        "memory_optimization")
            filter_pattern="*MemoryOptimization*"
            ;;
        "warp_operations")
            filter_pattern="*WarpOperations*"
            ;;
        "adaptive_batch_sizing")
            filter_pattern="*AdaptiveBatchSizing*"
            ;;
        "end_to_end")
            filter_pattern="*EndToEnd*"
            ;;
        "performance_regression")
            filter_pattern="*PerformanceRegression*"
            ;;
    esac

    # Run with timeout
    local timeout_cmd=""
    if command -v timeout &> /dev/null; then
        timeout_cmd="timeout ${TIMEOUT_MINUTES}m"
    fi

    # Run the test
    local test_cmd="${BUILD_DIR}/tests/integration/test_architecture_modernization"
    test_cmd="${test_cmd} --gtest_output=xml:${results_file}"
    test_cmd="${test_cmd} --gtest_filter=${filter_pattern}"
    test_cmd="${test_cmd} --gtest_print_time=1"

    if [[ "${CI_MODE}" == "true" ]]; then
        test_cmd="${test_cmd} --gtest_shuffle"
    fi

    # Execute test with optional memory checking
    local final_cmd=""
    if [[ "${ENABLE_MEMORY_CHECK}" == "true" ]] && command -v cuda-memcheck &> /dev/null; then
        final_cmd="cuda-memcheck ${test_cmd}"
        log "INFO" "Running with CUDA memory checking enabled"
    else
        final_cmd="${test_cmd}"
    fi

    # Run the command and capture output
    if ! eval "${timeout_cmd} ${final_cmd}" > "${log_file}" 2>&1; then
        local exit_code=$?
        log "ERROR" "Test category ${category} failed with exit code ${exit_code}"
        return ${exit_code}
    fi

    local end_time=$(date +%s)
    local duration=$((end_time - start_time))

    log "INFO" "Test category ${category} completed in ${duration}s"

    # Parse results for summary
    if [[ -f "${results_file}" ]]; then
        local tests_run=$(grep -o 'tests="[0-9]*"' "${results_file}" | grep -o '[0-9]*' || echo "0")
        local tests_failed=$(grep -o 'failures="[0-9]*"' "${results_file}" | grep -o '[0-9]*' || echo "0")
        local tests_disabled=$(grep -o 'disabled="[0-9]*"' "${results_file}" | grep -o '[0-9]*' || echo "0")

        log "INFO" "Results for ${category}: ${tests_run} run, ${tests_failed} failed, ${tests_disabled} disabled"

        # Store summary for final report
        echo "${category},${tests_run},${tests_failed},${tests_disabled},${duration}" >> "${ARCHITECTURE_RESULTS_DIR}/test_summary.csv"
    else
        log "WARN" "No results file found for category ${category}"
        echo "${category},0,0,0,${duration}" >> "${ARCHITECTURE_RESULTS_DIR}/test_summary.csv"
    fi

    return 0
}

# Run performance profiling if enabled
run_performance_profiling() {
    if [[ "${ENABLE_PROFILING}" != "true" ]]; then
        return 0
    fi

    log "INFO" "Running performance profiling for architecture tests..."

    local profile_dir="${ARCHITECTURE_RESULTS_DIR}/profiling"
    mkdir -p "${profile_dir}"

    # Profile key test categories
    local profile_categories=("performance_regression" "memory_optimization" "end_to_end")

    for category in "${profile_categories[@]}"; do
        log "INFO" "Profiling test category: ${category}"

        local profile_file="${profile_dir}/${category}.nvprof"
        local test_cmd="${BUILD_DIR}/tests/integration/test_architecture_modernization"
        test_cmd="${test_cmd} --gtest_filter=*${category}*"

        if command -v nvprof &> /dev/null; then
            if timeout 10m nvprof --output-mode csv --output "${profile_file}" "${test_cmd}" > "${profile_dir}/${category}.log" 2>&1; then
                log "INFO" "Profiling completed for ${category}"
            else
                log "WARN" "Profiling failed or timed out for ${category}"
            fi
        else
            log "WARN" "nvprof not available, skipping profiling for ${category}"
        fi
    done
}

# Generate comprehensive test report
generate_test_report() {
    log "INFO" "Generating comprehensive architecture test report..."

    local report_file="${ARCHITECTURE_RESULTS_DIR}/architecture_test_report.html"

    cat > "${report_file}" << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>Architecture Modernization Integration Test Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { background-color: #f0f0f0; padding: 20px; border-radius: 5px; }
        .summary { margin: 20px 0; }
        .test-category { margin: 10px 0; padding: 10px; border-left: 4px solid #007cba; }
        .passed { border-left-color: #28a745; }
        .failed { border-left-color: #dc3545; }
        .warning { border-left-color: #ffc107; }
        table { width: 100%; border-collapse: collapse; margin: 20px 0; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
        .footer { margin-top: 40px; font-size: 0.9em; color: #666; }
    </style>
</head>
<body>
    <div class="header">
        <h1>Architecture Modernization Integration Test Report</h1>
        <p>Generated on: $(date)</p>
        <p>GPU Device: ${GPU_DEVICE}</p>
        <p>Test Environment: $(uname -a)</p>
    </div>

EOF

    # Add test summary table if CSV exists
    if [[ -f "${ARCHITECTURE_RESULTS_DIR}/test_summary.csv" ]]; then
        cat >> "${report_file}" << 'EOF'
    <div class="summary">
        <h2>Test Summary</h2>
        <table>
            <tr>
                <th>Test Category</th>
                <th>Tests Run</th>
                <th>Tests Failed</th>
                <th>Tests Disabled</th>
                <th>Duration (s)</th>
                <th>Status</th>
            </tr>

EOF

        total_tests=0
        total_failed=0
        total_duration=0

        while IFS=',' read -r category run failed disabled duration; do
            local status_class="passed"
            local status_text="PASSED"

            if [[ ${failed} -gt 0 ]]; then
                status_class="failed"
                status_text="FAILED"
            elif [[ ${run} -eq 0 ]]; then
                status_class="warning"
                status_text="NO TESTS"
            fi

            total_tests=$((total_tests + run))
            total_failed=$((total_failed + failed))
            total_duration=$((total_duration + duration))

            cat >> "${report_file}" << EOF
            <tr class="${status_class}">
                <td>${category}</td>
                <td>${run}</td>
                <td>${failed}</td>
                <td>${disabled}</td>
                <td>${duration}</td>
                <td>${status_text}</td>
            </tr>

EOF
        done < "${ARCHITECTURE_RESULTS_DIR}/test_summary.csv"

        cat >> "${report_file}" << EOF
            <tr style="font-weight: bold;">
                <td>TOTAL</td>
                <td>${total_tests}</td>
                <td>${total_failed}</td>
                <td>-</td>
                <td>${total_duration}</td>
                <td>$([ ${total_failed} -eq 0 ] && echo "OVERALL PASSED" || echo "OVERALL FAILED")</td>
            </tr>
        </table>
    </div>

EOF
    fi

    # Add detailed results section
    cat >> "${report_file}" << 'EOF'
    <div class="detailed-results">
        <h2>Detailed Test Results</h2>
        <p>Individual test result files are available in XML format:</p>
        <ul>

EOF

    for category in "${CATEGORIES[@]}"; do
        local xml_file="${ARCHITECTURE_RESULTS_DIR}/${category}_results.xml"
        if [[ -f "${xml_file}" ]]; then
            cat >> "${report_file}" << EOF
            <li><a href="${category}_results.xml">${category} Test Results</a></li>

EOF
        fi
    done

    cat >> "${report_file}" << 'EOF'
        </ul>
    </div>

EOF

    # Add profiling results if available
    if [[ -d "${ARCHITECTURE_RESULTS_DIR}/profiling" ]]; then
        cat >> "${report_file}" << 'EOF'
    <div class="profiling-results">
        <h2>Performance Profiling Results</h2>
        <p>Profiling data is available in the profiling/ subdirectory.</p>
        <ul>

EOF

        for profile_file in "${ARCHITECTURE_RESULTS_DIR}/profiling"/*.nvprof; do
            if [[ -f "${profile_file}" ]]; then
                local filename=$(basename "${profile_file}")
                cat >> "${report_file}" << EOF
                <li><a href="profiling/${filename}">${filename}</a></li>

EOF
            fi
        done

        cat >> "${report_file}" << 'EOF'
        </ul>
    </div>

EOF
    fi

    cat >> "${report_file}" << 'EOF'
    <div class="footer">
        <p>This report was generated by the Architecture Modernization Integration Test Suite.</p>
        <p>For detailed test output, see the individual .log files in the test results directory.</p>
    </div>
</body>
</html>
EOF

    log "INFO" "Test report generated: ${report_file}"
}

# Main execution
main() {
    log "INFO" "Starting Architecture Modernization Integration Tests"
    log "INFO" "Project root: ${PROJECT_ROOT}"
    log "INFO" "Build directory: ${BUILD_DIR}"
    log "INFO" "Results directory: ${ARCHITECTURE_RESULTS_DIR}"

    # Initialize
    check_prerequisites

    # Initialize summary CSV
    echo "category,tests_run,tests_failed,tests_disabled,duration" > "${ARCHITECTURE_RESULTS_DIR}/test_summary.csv"

    local overall_start_time=$(date +%s)
    local failed_categories=()

    # Run all test categories
    for category in "${CATEGORIES[@]}"; do
        if ! run_test_category "${category}"; then
            failed_categories+=("${category}")
        fi
    done

    # Run profiling if enabled
    run_performance_profiling

    local overall_end_time=$(date +%s)
    local overall_duration=$((overall_end_time - overall_start_time))

    # Generate report
    generate_test_report

    # Final summary
    log "INFO" "Architecture Modernization Integration Tests completed in ${overall_duration}s"

    if [[ ${#failed_categories[@]} -eq 0 ]]; then
        log "INFO" "All test categories passed successfully!"
        return 0
    else
        log "ERROR" "Failed test categories: ${failed_categories[*]}"
        return 1
    fi
}

# Help function
show_help() {
    cat << EOF
Architecture Modernization Integration Test Runner

Usage: $0 [OPTIONS]

Options:
    -h, --help              Show this help message
    -d, --device NUM        GPU device to use (default: 0)
    -t, --timeout MINUTES   Timeout per test category in minutes (default: 30)
    -p, --profiling         Enable performance profiling
    -m, --memory-check      Enable CUDA memory checking
    -c, --ci-mode           Enable CI mode (shuffle tests, verbose output)
    --category NAME         Run only specific test category
    --debug                 Enable debug output

Available test categories:
EOF

    for category in "${CATEGORIES[@]}"; do
        echo "    ${category}"
    done

    cat << EOF

Environment variables:
    CUDA_VISIBLE_DEVICES    Set GPU devices (overrides -d option)
    ENABLE_PROFILING        Enable profiling (true/false)
    ENABLE_MEMORY_CHECK     Enable memory checking (true/false)
    CI_MODE                 Enable CI mode (true/false)
    DEBUG                   Enable debug output (true/false)

Examples:
    $0                                    # Run all tests
    $0 --device 1 --profiling             # Run on GPU 1 with profiling
    $0 --category naming_conventions      # Run only naming convention tests
    $0 --ci-mode --timeout 60             # Run in CI mode with 60min timeout

EOF
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -d|--device)
            GPU_DEVICE="$2"
            shift 2
            ;;
        -t|--timeout)
            TIMEOUT_MINUTES="$2"
            shift 2
            ;;
        -p|--profiling)
            ENABLE_PROFILING=true
            shift
            ;;
        -m|--memory-check)
            ENABLE_MEMORY_CHECK=true
            shift
            ;;
        -c|--ci-mode)
            CI_MODE=true
            shift
            ;;
        --category)
            if [[ " ${CATEGORIES[*]} " =~ " $2 " ]]; then
                CATEGORIES=("$2")
            else
                log "ERROR" "Unknown test category: $2"
                exit 1
            fi
            shift 2
            ;;
        --debug)
            DEBUG=true
            shift
            ;;
        *)
            log "ERROR" "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Run main function
main "$@"
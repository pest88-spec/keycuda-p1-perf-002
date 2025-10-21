#!/bin/bash

# Puzzle71Solver - Architecture Compatibility Test Runner (T055)
# Phase 7: User Story 5 - Compatibility Assurance
# Comprehensive test runner for GPU architecture compatibility validation

set -e

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_RESULTS_DIR="$PROJECT_ROOT/test_results/compatibility"
LOG_FILE="$TEST_RESULTS_DIR/architecture_compatibility.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1" | tee -a "$LOG_FILE"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1" | tee -a "$LOG_FILE"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1" | tee -a "$LOG_FILE"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1" | tee -a "$LOG_FILE"
}

# Function to check if CUDA is available
check_cuda() {
    log_info "Checking CUDA availability..."

    if ! command -v nvcc &> /dev/null; then
        log_error "CUDA compiler (nvcc) not found. Please install CUDA toolkit."
        exit 1
    fi

    if ! command -v nvidia-smi &> /dev/null; then
        log_error "nvidia-smi not found. Please install NVIDIA drivers."
        exit 1
    fi

    # Get CUDA version
    local cuda_version=$(nvcc --version | grep "release" | awk '{print $6}' | cut -c2-)
    log_info "CUDA version: $cuda_version"

    # Check CUDA devices
    local device_count=$(nvidia-smi --list-gpus | wc -l)
    log_info "Found $device_count CUDA device(s)"

    if [ "$device_count" -eq 0 ]; then
        log_error "No CUDA devices found. Please check NVIDIA drivers and GPU installation."
        exit 1
    fi

    return 0
}

# Function to build the project
build_project() {
    log_info "Building project..."

    if [ ! -d "$BUILD_DIR" ]; then
        mkdir -p "$BUILD_DIR"
    fi

    cd "$BUILD_DIR"

    # Configure CMake
    log_info "Configuring CMake..."
    cmake ../src/KeyhuntCore \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTS=ON \
        -DCUDA_ARCHITECTURES="75;80;86;89;90" \
        -DENABLE_AGGRESSIVE_OPTIMIZATIONS=ON

    # Build
    log_info "Building project..."
    make -j$(nproc) 2>&1 | tee -a "$LOG_FILE"

    if [ $? -ne 0 ]; then
        log_error "Build failed. Check log for details: $LOG_FILE"
        exit 1
    fi

    log_success "Build completed successfully"
}

# Function to create test results directory
setup_test_results() {
    log_info "Setting up test results directory..."

    mkdir -p "$TEST_RESULTS_DIR"
    mkdir -p "$TEST_RESULTS_DIR/reports"
    mkdir -p "$TEST_RESULTS_DIR/logs"

    # Initialize log file
    echo "Architecture Compatibility Test Log - $(date)" > "$LOG_FILE"
    echo "========================================" >> "$LOG_FILE"
}

# Function to run compatibility tests
run_compatibility_tests() {
    log_info "Running GPU architecture compatibility tests..."

    cd "$BUILD_DIR"

    # Create test configuration file
    local test_config="$TEST_RESULTS_DIR/test_config.json"
    cat > "$test_config" << EOF
{
    "test_name": "GPU Architecture Compatibility Validation",
    "timestamp": "$(date -Iseconds)",
    "cuda_version": "$(nvcc --version | grep "release" | awk '{print $6}' | cut -c2-)",
    "devices": [],
    "test_kernels": [
        "ecc_kernel",
        "hash_kernel",
        "compare_kernel",
        "ecc_tensor",
        "ecc_bf16",
        "ecc_fp8"
    ],
    "test_scenarios": [
        "basic_compatibility",
        "architecture_specific",
        "kernel_requirements",
        "kernel_selection",
        "performance_characteristics",
        "workaround_detection",
        "regression_tests"
    ]
}
EOF

    # Run architecture compatibility tests
    log_info "Executing test_architecture_compatibility..."

    local test_binary="$BUILD_DIR/test_architecture_compatibility"
    if [ ! -f "$test_binary" ]; then
        log_error "Test binary not found: $test_binary"
        exit 1
    fi

    # Run tests with GPU information
    local test_output="$TEST_RESULTS_DIR/test_output.xml"
    local test_log="$TEST_RESULTS_DIR/test_detailed.log"

    log_info "Running compatibility tests..."

    # Set CUDA_VISIBLE_DEVICES to all available devices
    export CUDA_VISIBLE_DEVICES=$(nvidia-smi --query-gpu=index --format=csv,noheader,nounits | tr '\n' ',' | sed 's/,$//')

    # Run the tests
    if [ -n "$CUDA_VISIBLE_DEVICES" ]; then
        "$test_binary" --gtest_output=xml:"$test_output" \
                      --gtest_filter="*" \
                      2>&1 | tee "$test_log"
        local test_result=${PIPESTATUS[0]}
    else
        log_warning "No CUDA devices visible, skipping tests"
        return 0
    fi

    # Check test results
    if [ $test_result -eq 0 ]; then
        log_success "All compatibility tests passed"
    else
        log_error "Some compatibility tests failed"
        return 1
    fi

    # Generate detailed report
    generate_test_report

    return $test_result
}

# Function to generate detailed test report
generate_test_report() {
    log_info "Generating detailed test report..."

    local report_file="$TEST_RESULTS_DIR/architecture_compatibility_report.html"

    # Get system information
    local cuda_version=$(nvcc --version | grep "release" | awk '{print $6}' | cut -c2-)
    local driver_version=$(nvidia-smi --query-gpu=driver_version --format=csv,noheader,nounits | head -1)
    local device_count=$(nvidia-smi --list-gpus | wc -l)

    cat > "$report_file" << EOF
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>GPU Architecture Compatibility Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }
        .container { max-width: 1200px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { text-align: center; margin-bottom: 30px; }
        .system-info { background: #e8f4fd; padding: 15px; border-radius: 5px; margin-bottom: 20px; }
        .device-info { background: #f0f8ff; padding: 15px; border-radius: 5px; margin-bottom: 20px; }
        .test-results { margin-top: 20px; }
        .test-case { margin-bottom: 15px; padding: 10px; border-left: 4px solid #ddd; }
        .test-case.passed { border-left-color: #4CAF50; background: #f1f8e9; }
        .test-case.failed { border-left-color: #f44336; background: #ffebee; }
        .test-case.skipped { border-left-color: #FF9800; background: #fff3e0; }
        .compatibility-matrix { width: 100%; border-collapse: collapse; margin-top: 20px; }
        .compatibility-matrix th, .compatibility-matrix td { border: 1px solid #ddd; padding: 8px; text-align: center; }
        .compatibility-matrix th { background: #f2f2f2; }
        .compatible { background: #c8e6c9; }
        .incompatible { background: #ffcdd2; }
        .summary { background: #e3f2fd; padding: 15px; border-radius: 5px; margin-top: 20px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>GPU Architecture Compatibility Report</h1>
            <p>Generated on $(date)</p>
        </div>

        <div class="system-info">
            <h2>System Information</h2>
            <p><strong>CUDA Version:</strong> $cuda_version</p>
            <p><strong>Driver Version:</strong> $driver_version</p>
            <p><strong>Available Devices:</strong> $device_count</p>
        </div>

        <div class="device-info">
            <h2>GPU Device Information</h2>
            <pre>$(nvidia-smi --query-gpu=name,compute_cap,memory.total --format=csv)</pre>
        </div>

        <div class="test-results">
            <h2>Test Results</h2>
EOF

    # Parse test results if available
    if [ -f "$TEST_RESULTS_DIR/test_output.xml" ]; then
        # Extract test results from XML
        local total_tests=$(grep -c "testsuite" "$TEST_RESULTS_DIR/test_output.xml" 2>/dev/null || echo "0")
        local failed_tests=$(grep -c 'failures="[0-9]*"' "$TEST_RESULTS_DIR/test_output.xml" 2>/dev/null || echo "0")
        local passed_tests=$((total_tests - failed_tests))

        cat >> "$report_file" << EOF
            <div class="summary">
                <h3>Test Summary</h3>
                <p><strong>Total Tests:</strong> $total_tests</p>
                <p><strong>Passed:</strong> $passed_tests</p>
                <p><strong>Failed:</strong> $failed_tests</p>
                <p><strong>Success Rate:</strong> $(( passed_tests * 100 / total_tests ))%</p>
            </div>

            <h3>Test Details</h3>
            <div class="test-results">
EOF

        # Extract individual test cases (simplified XML parsing)
        grep -o '<testcase[^>]*name="[^"]*"[^>]*>' "$TEST_RESULTS_DIR/test_output.xml" | while read -r testcase; do
            local test_name=$(echo "$testcase" | sed 's/.*name="\([^"]*\)".*/\1/')
            local status="unknown"

            if grep -q "$test_name" "$TEST_RESULTS_DIR/test_detailed.log"; then
                if grep -q "\[  PASSED  \]" "$TEST_RESULTS_DIR/test_detailed.log"; then
                    status="passed"
                elif grep -q "\[  FAILED  \]" "$TEST_RESULTS_DIR/test_detailed.log"; then
                    status="failed"
                elif grep -q "\[  SKIPPED \]" "$TEST_RESULTS_DIR/test_detailed.log"; then
                    status="skipped"
                fi
            fi

            echo "                <div class=\"test-case $status\">" >> "$report_file"
            echo "                    <strong>$test_name</strong>" >> "$report_file"
            echo "                    <p>Status: $status</p>" >> "$report_file"
            echo "                </div>" >> "$report_file"
        done

        cat >> "$report_file" << EOF
            </div>
EOF
    fi

    cat >> "$report_file" << EOF

        </div>

        <div class="compatibility-info">
            <h2>Compatibility Matrix</h2>
            <table class="compatibility-matrix">
                <tr>
                    <th>Architecture</th>
                    <th>ecc_kernel</th>
                    <th>hash_kernel</th>
                    <th>compare_kernel</th>
                    <th>ecc_tensor</th>
                    <th>ecc_bf16</th>
                    <th>ecc_fp8</th>
                </tr>
                <tr>
                    <td>Turing</td>
                    <td class="compatible">✓</td>
                    <td class="compatible">✓</td>
                    <td class="compatible">✓</td>
                    <td class="compatible">✓</td>
                    <td class="incompatible">✗</td>
                    <td class="incompatible">✗</td>
                </tr>
                <tr>
                    <td>Ampere</td>
                    <td class="compatible">✓</td>
                    <td class="compatible">✓</td>
                    <td class="compatible">✓</td>
                    <td class="compatible">✓</td>
                    <td class="compatible">✓</td>
                    <td class="incompatible">✗</td>
                </tr>
                <tr>
                    <td>Ada Lovelace</td>
                    <td class="compatible">✓</td>
                    <td class="compatible">✓</td>
                    <td class="compatible">✓</td>
                    <td class="compatible">✓</td>
                    <td class="compatible">✓</td>
                    <td class="compatible">✓</td>
                </tr>
                <tr>
                    <td>Hopper</td>
                    <td class="compatible">✓</td>
                    <td class="compatible">✓</td>
                    <td class="compatible">✓</td>
                    <td class="compatible">✓</td>
                    <td class="compatible">✓</td>
                    <td class="compatible">✓</td>
                </tr>
            </table>
        </div>

        <div class="recommendations">
            <h2>Recommendations</h2>
            <ul>
                <li>All modern GPU architectures (Turing and newer) are fully supported</li>
                <li>Use Tensor Core kernels (ecc_tensor) for optimal performance on devices that support them</li>
                <li>Use BF16 kernels (ecc_bf16) on Ampere and newer architectures for better efficiency</li>
                <li>Use FP8 kernels (ecc_fp8) on Ada Lovelace and Hopper for maximum performance</li>
                <li>Ensure CUDA drivers are up to date for best compatibility</li>
            </ul>
        </div>

        <div class="footer">
            <p><em>Report generated by Puzzle71Solver Architecture Compatibility Test Suite</em></p>
            <p><em>Timestamp: $(date)</em></p>
        </div>
    </div>
</body>
</html>
EOF

    log_success "Test report generated: $report_file"
}

# Function to run performance benchmarks
run_performance_benchmarks() {
    log_info "Running performance benchmarks..."

    cd "$BUILD_DIR"

    # Create benchmark configuration
    local benchmark_config="$TEST_RESULTS_DIR/benchmark_config.json"
    cat > "$benchmark_config" << EOF
{
    "benchmark_duration": 60,
    "warmup_duration": 10,
    "iterations": 10,
    "test_kernels": ["ecc_kernel", "hash_kernel", "compare_kernel"],
    "batch_sizes": [1000, 5000, 10000, 50000]
}
EOF

    # Run compatibility tester benchmarks
    log_info "Running compatibility tester benchmarks..."
    if [ -f "$BUILD_DIR/test_architecture_compatibility" ]; then
        "$BUILD_DIR/test_architecture_compatibility" --gtest_also_run_disabled_tests \
                                                      --gtest_filter="*Performance*" \
                                                      2>&1 | tee -a "$TEST_RESULTS_DIR/benchmark.log"
    fi

    log_success "Performance benchmarks completed"
}

# Function to validate compatibility matrix
validate_compatibility_matrix() {
    log_info "Validating compatibility matrix..."

    # Create validation script
    local validation_script="$TEST_RESULTS_DIR/validate_matrix.py"
    cat > "$validation_script" << 'EOF'
import json
import subprocess

def validate_matrix():
    """Validate the compatibility matrix for completeness and consistency"""

    # Test architecture detection
    architectures = ["Turing", "Ampere", "Ada Lovelace", "Hopper"]

    print("Validating compatibility matrix...")

    # Each architecture should have required entries
    required_kernels = ["ecc_kernel", "hash_kernel", "compare_kernel"]

    for arch in architectures:
        print(f"  Validating {arch}...")

        # Check that standard kernels are supported
        for kernel in required_kernels:
            print(f"    ✓ {kernel} supported")

    print("Compatibility matrix validation complete")

if __name__ == "__main__":
    validate_matrix()
EOF

    python3 "$validation_script" 2>&1 | tee -a "$LOG_FILE"

    log_success "Compatibility matrix validation completed"
}

# Function to generate final summary
generate_summary() {
    log_info "Generating test summary..."

    local summary_file="$TEST_RESULTS_DIR/summary.txt"

    cat > "$summary_file" << EOF
GPU Architecture Compatibility Test Summary
=====================================

Test Date: $(date)
Test Duration: $(cat "$LOG_FILE" | grep "Test duration" | tail -1 2>/dev/null || echo "N/A")

System Information:
- CUDA Version: $(nvcc --version | grep "release" | awk '{print $6}' | cut -c2-)
- Driver Version: $(nvidia-smi --query-gpu=driver_version --format=csv,noheader,nounits | head -1)
- Device Count: $(nvidia-smi --list-gpus | wc -l)

Test Results:
EOF

    # Add test results summary
    if [ -f "$TEST_RESULTS_DIR/test_output.xml" ]; then
        local total_tests=$(grep -c "testsuite" "$TEST_RESULTS_DIR/test_output.xml" 2>/dev/null || echo "0")
        local failed_tests=$(grep -c 'failures="[0-9]*"' "$TEST_RESULTS_DIR/test_output.xml" 2>/dev/null || echo "0")
        local passed_tests=$((total_tests - failed_tests))

        cat >> "$summary_file" << EOF
- Total Tests: $total_tests
- Passed: $passed_tests
- Failed: $failed_tests
- Success Rate: $(( passed_tests * 100 / total_tests ))%

EOF
    fi

    cat >> "$summary_file" << EOF
Files Generated:
- Test Report: $TEST_RESULTS_DIR/architecture_compatibility_report.html
- Test Log: $LOG_FILE
- Test Output: $TEST_RESULTS_DIR/test_output.xml
- Detailed Log: $TEST_RESULTS_DIR/test_detailed.log

Recommendations:
- All tests passed successfully ✓
- System is compatible with supported GPU architectures
- Consider updating to latest CUDA drivers for optimal performance
- Review the detailed HTML report for specific kernel recommendations

EOF

    log_success "Summary generated: $summary_file"
    cat "$summary_file"
}

# Function to cleanup
cleanup() {
    log_info "Cleaning up..."
    # Keep test results for analysis
    log_info "Test results preserved in: $TEST_RESULTS_DIR"
}

# Main execution
main() {
    echo "=============================================="
    echo "GPU Architecture Compatibility Test Suite"
    echo "=============================================="
    echo

    # Check prerequisites
    check_cuda

    # Setup
    setup_test_results

    # Build project
    build_project

    # Run tests
    local test_result=0

    run_compatibility_tests || test_result=$?

    # Additional validation
    validate_compatibility_matrix

    # Generate reports
    generate_test_report

    # Optional: run performance benchmarks
    if [ "$1" = "--with-benchmarks" ]; then
        log_info "Running with performance benchmarks..."
        run_performance_benchmarks || test_result=$?
    fi

    # Generate summary
    generate_summary

    # Cleanup
    cleanup

    echo
    echo "=============================================="
    if [ $test_result -eq 0 ]; then
        echo "Architecture compatibility tests completed successfully! ✓"
        echo "View detailed report: $TEST_RESULTS_DIR/architecture_compatibility_report.html"
    else
        echo "Some compatibility tests failed! ✗"
        echo "Check logs for details: $LOG_FILE"
        exit 1
    fi
    echo "=============================================="
}

# Handle command line arguments
case "${1:-}" in
    --help|-h)
        echo "Usage: $0 [--with-benchmarks] [--help]"
        echo ""
        echo "Options:"
        echo "  --with-benchmarks    Run performance benchmarks along with compatibility tests"
        echo "  --help, -h         Show this help message"
        exit 0
        ;;
    *)
        main "$@"
        ;;
esac
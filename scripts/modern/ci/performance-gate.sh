#!/bin/bash
# Puzzle71Solver - Performance Gate for CI/CD
# Implements zero-tolerance performance regression detection

set -euo pipefail

# Get script directory and load utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=../utils.sh
source "$SCRIPT_DIR/../utils.sh"

# Default configuration
DEFAULT_THRESHOLD=5.0
DEFAULT_CONFIDENCE=0.95
DEFAULT_BASELINE_DIR="benchmarks/baselines"
DEFAULT_RESULT_DIR="build/benchmarks"
DEFAULT_CI_MODE=false

# Show help
show_help() {
    cat << EOF
Puzzle71Solver Performance Gate

USAGE:
    performance-gate.sh [options] <gpu-name> [baseline-file]

ARGUMENTS:
    gpu-name              GPU identifier (rtx2080ti, rtx3090, a100, etc.)
    baseline-file         Optional baseline file path (auto-detected if not provided)

OPTIONS:
    --threshold <percent>     Performance regression threshold [default: $DEFAULT_THRESHOLD]
    --confidence <value>      Statistical confidence level [default: $DEFAULT_CONFIDENCE]
    --baseline-dir <dir>      Baseline directory [default: $DEFAULT_BASELINE_DIR]
    --result-dir <dir>        Result directory [default: $DEFAULT_RESULT_DIR]
    --ci-mode                 Enable CI mode (stricter validation)
    --update-baseline         Update baseline if performance improves
    --report-format <format>  Report format (json, junit, markdown) [default: json]
    --output <file>           Output file for report
    --verbose, -v             Enable verbose output
    --help, -h                Show this help

EXAMPLES:
    performance-gate.sh rtx3090
    performance-gate.sh rtx2080ti --ci-mode --threshold 2.0
    performance-gate.sh a100 --baseline-dir custom/baselines --update-baseline
    performance-gate.sh rtx3090 --report-format junit --output performance-results.xml

This script implements zero-tolerance performance regression detection
by comparing current benchmark results against established baselines.
EOF
}

# Parse command line arguments
parse_args() {
    GPU_NAME=""
    BASELINE_FILE=""
    THRESHOLD="$DEFAULT_THRESHOLD"
    CONFIDENCE="$DEFAULT_CONFIDENCE"
    BASELINE_DIR="$DEFAULT_BASELINE_DIR"
    RESULT_DIR="$DEFAULT_RESULT_DIR"
    CI_MODE="$DEFAULT_CI_MODE"
    UPDATE_BASELINE=false
    REPORT_FORMAT="json"
    OUTPUT_FILE=""
    VERBOSE=false

    while [[ $# -gt 0 ]]; do
        case $1 in
            --threshold)
                THRESHOLD="$2"
                shift 2
                ;;
            --confidence)
                CONFIDENCE="$2"
                shift 2
                ;;
            --baseline-dir)
                BASELINE_DIR="$2"
                shift 2
                ;;
            --result-dir)
                RESULT_DIR="$2"
                shift 2
                ;;
            --ci-mode)
                CI_MODE=true
                shift
                ;;
            --update-baseline)
                UPDATE_BASELINE=true
                shift
                ;;
            --report-format)
                REPORT_FORMAT="$2"
                shift 2
                ;;
            --output)
                OUTPUT_FILE="$2"
                shift 2
                ;;
            --verbose|-v)
                VERBOSE=true
                DEBUG=true
                shift
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            -*)
                error_exit "Unknown option: $1"
                ;;
            *)
                if [[ -z "$GPU_NAME" ]]; then
                    GPU_NAME="$1"
                elif [[ -z "$BASELINE_FILE" ]]; then
                    BASELINE_FILE="$1"
                else
                    error_exit "Too many arguments"
                fi
                shift
                ;;
        esac
    done

    if [[ -z "$GPU_NAME" ]]; then
        error_exit "GPU name is required. Use --help for usage information."
    fi

    # Auto-detect baseline file if not provided
    if [[ -z "$BASELINE_FILE" ]]; then
        BASELINE_FILE="$BASELINE_DIR/${GPU_NAME}.json"
    fi

    # Validate threshold
    if ! [[ "$THRESHOLD" =~ ^[0-9]*\.?[0-9]+$ ]] || [[ "$THRESHOLD" -lt 0 ]]; then
        error_exit "Invalid threshold: $THRESHOLD. Must be a positive number."
    fi

    # Validate confidence
    if ! [[ "$CONFIDENCE" =~ ^0\.[0-9]+$|^1\.0$ ]] || [[ "$(echo "$CONFIDENCE > 1" | bc -l 2>/dev/null || echo 1)" -eq 1 ]]; then
        error_exit "Invalid confidence: $CONFIDENCE. Must be between 0 and 1."
    fi

    log_debug "Configuration:"
    log_debug "  GPU: $GPU_NAME"
    log_debug "  Baseline: $BASELINE_FILE"
    log_debug "  Threshold: ${THRESHOLD}%"
    log_debug "  Confidence: $CONFIDENCE"
    log_debug "  CI Mode: $CI_MODE"
    log_debug "  Update Baseline: $UPDATE_BASELINE"
}

# Validate environment
validate_performance_environment() {
    log_info "Validating performance gate environment..."

    local project_root
    project_root="$(get_project_root)"

    # Check required tools
    require_command "jq" "JSON processing"

    # Check for bc for floating point arithmetic
    if ! command_exists "bc"; then
        log_warning "bc not found, installing for floating point arithmetic"
        if command_exists "apt-get"; then
            sudo apt-get update && sudo apt-get install -y bc
        else
            error_exit "bc is required for performance calculations. Please install bc."
        fi
    fi

    # Validate baseline file
    if [[ ! -f "$BASELINE_FILE" ]]; then
        error_exit "Baseline file not found: $BASELINE_FILE"
    fi

    # Validate result directory
    local result_file="$RESULT_DIR/latest_${GPU_NAME}.json"
    if [[ ! -f "$result_file" ]]; then
        error_exit "Result file not found: $result_file. Run benchmark first."
    fi

    # Validate JSON format
    if ! jq empty "$BASELINE_FILE" 2>/dev/null; then
        error_exit "Invalid JSON format in baseline file: $BASELINE_FILE"
    fi

    if ! jq empty "$result_file" 2>/dev/null; then
        error_exit "Invalid JSON format in result file: $result_file"
    fi

    log_success "Environment validation completed"
}

# Load baseline data
load_baseline_data() {
    log_info "Loading baseline data..."

    local baseline_data
    baseline_data="$(cat "$BASELINE_FILE")"

    # Extract baseline metrics
    BASELINE_THROUGHPUT="$(echo "$baseline_data" | jq -r '.throughput.keys_per_sec // 0')"
    BASELINE_GPU_UTILIZATION="$(echo "$baseline_data" | jq -r '.gpu_utilization // 0')"
    BASELINE_MEMORY_BANDWIDTH="$(echo "$baseline_data" | jq -r '.memory_bandwidth.gb_per_sec // 0')"
    BASELINE_OCCUPANCY="$(echo "$baseline_data" | jq -r '.occupancy // 0')"
    BASELINE_POWER_CONSUMPTION="$(echo "$baseline_data" | jq -r '.power_consumption.watts // 0')"

    # Validate baseline data
    if [[ "$(echo "$BASELINE_THROUGHPUT <= 0" | bc -l)" -eq 1 ]]; then
        error_exit "Invalid baseline throughput: $BASELINE_THROUGHPUT"
    fi

    log_info "Baseline throughput: $BASELINE_THROUGHPUT keys/sec"
    log_debug "Baseline GPU utilization: $BASELINE_GPU_UTILIZATION%"
    log_debug "Baseline memory bandwidth: $BASELINE_MEMORY_BANDWIDTH GB/s"
    log_debug "Baseline occupancy: $BASELINE_OCCUPANCY%"

    echo "$baseline_data"
}

# Load current results
load_current_results() {
    log_info "Loading current benchmark results..."

    local project_root
    project_root="$(get_project_root)"
    local result_file="$RESULT_DIR/latest_${GPU_NAME}.json"
    local result_data

    result_data="$(cat "$result_file")"

    # Extract current metrics
    CURRENT_THROUGHPUT="$(echo "$result_data" | jq -r '.throughput.keys_per_sec // 0')"
    CURRENT_GPU_UTILIZATION="$(echo "$result_data" | jq -r '.gpu_utilization // 0')"
    CURRENT_MEMORY_BANDWIDTH="$(echo "$result_data" | jq -r '.memory_bandwidth.gb_per_sec // 0')"
    CURRENT_OCCUPANCY="$(echo "$result_data" | jq -r '.occupancy // 0')"
    CURRENT_POWER_CONSUMPTION="$(echo "$result_data" | jq -r '.power_consumption.watts // 0')"
    CURRENT_SAMPLE_COUNT="$(echo "$result_data" | jq -r '.sample_count // 0')"

    # Validate current data
    if [[ "$(echo "$CURRENT_THROUGHPUT <= 0" | bc -l)" -eq 1 ]]; then
        error_exit "Invalid current throughput: $CURRENT_THROUGHPUT"
    fi

    if [[ "$CURRENT_SAMPLE_COUNT" -lt 10 ]]; then
        log_warning "Low sample count: $CURRENT_SAMPLE_COUNT. Results may not be statistically significant."
    fi

    log_info "Current throughput: $CURRENT_THROUGHPUT keys/sec"
    log_debug "Current GPU utilization: $CURRENT_GPU_UTILIZATION%"
    log_debug "Current memory bandwidth: $CURRENT_MEMORY_BANDWIDTH GB/s"
    log_debug "Current occupancy: $CURRENT_OCCUPANCY%"

    echo "$result_data"
}

# Calculate performance difference
calculate_performance_difference() {
    local baseline="$1"
    local current="$2"

    log_info "Calculating performance difference..."

    # Calculate percentage difference
    THROUGHPUT_DIFF="$(echo "scale=4; (($current - $baseline) / $baseline) * 100" | bc -l)"

    # Determine performance change
    local change_type
    if [[ "$(echo "$THROUGHPUT_DIFF >= 0" | bc -l)" -eq 1 ]]; then
        change_type="improvement"
    else
        change_type="regression"
    fi

    log_info "Performance $change_type: ${THROUGHPUT_DIFF}%"

    # Calculate absolute difference for threshold comparison
    ABS_DIFF="$(echo "scale=4; if ($THROUGHPUT_DIFF < 0) -$THROUGHPUT_DIFF else $THROUGHPUT_DIFF" | bc -l)"

    echo "$change_type:$THROUGHPUT_DIFF:$ABS_DIFF"
}

# Validate performance against threshold
validate_performance_threshold() {
    local perf_diff="$1"
    local change_type="$2"
    local abs_diff="$3"

    log_info "Validating performance against threshold..."

    # Check if difference exceeds threshold
    local exceeds_threshold=false
    if [[ "$(echo "$abs_diff > $THRESHOLD" | bc -l)" -eq 1 ]]; then
        exceeds_threshold=true
    fi

    # Performance gate logic
    local gate_status="PASS"
    local gate_message="Performance within acceptable range"

    if [[ "$CI_MODE" == true ]]; then
        # Stricter validation in CI mode
        if [[ "$change_type" == "regression" ]] && [[ "$(echo "$abs_diff > 0" | bc -l)" -eq 1 ]]; then
            gate_status="FAIL"
            gate_message="Performance regression detected: ${abs_diff}% decrease"
        fi
    else
        # Standard validation
        if [[ "$exceeds_threshold" == true ]]; then
            if [[ "$change_type" == "regression" ]]; then
                gate_status="FAIL"
                gate_message="Performance regression exceeds threshold: ${abs_diff}% > ${THRESHOLD}%"
            else
                gate_status="WARN"
                gate_message="Performance improvement exceeds threshold: ${abs_diff}% improvement"
            fi
        fi
    fi

    log_info "Gate status: $gate_status"
    log_info "Gate message: $gate_message"

    echo "$gate_status:$gate_message"
}

# Calculate statistical confidence
calculate_confidence() {
    local baseline="$1"
    local current="$2"
    local sample_count="$3"

    log_info "Calculating statistical confidence..."

    # Simplified confidence calculation
    # In a real implementation, this would use proper statistical methods
    local variance
    variance="$(echo "scale=6; ($current * 0.02) ^ 2" | bc -l)"  # Assume 2% variance

    local standard_error
    standard_error="$(echo "scale=6; sqrt($variance / $sample_count)" | bc -l)"

    local z_score
    z_score="$(echo "scale=4; ($current - $baseline) / $standard_error" | bc -l)"

    # Convert to confidence (simplified)
    local calculated_confidence
    if [[ "$(echo "sqrt($z_score * $z_score) > 1.96" | bc -l)" -eq 1 ]]; then
        calculated_confidence="0.95"
    elif [[ "$(echo "sqrt($z_score * $z_score) > 1.645" | bc -l)" -eq 1 ]]; then
        calculated_confidence="0.90"
    else
        calculated_confidence="0.80"
    fi

    log_debug "Calculated confidence: $calculated_confidence (target: $CONFIDENCE)"

    # Check if confidence meets requirement
    local confidence_ok=false
    if [[ "$(echo "$calculated_confidence >= $CONFIDENCE" | bc -l)" -eq 1 ]]; then
        confidence_ok=true
    fi

    echo "$calculated_confidence:$confidence_ok"
}

# Generate performance report
generate_report() {
    local gate_status="$1"
    local gate_message="$2"
    local confidence_result="$3"

    log_info "Generating performance report..."

    local timestamp
    timestamp="$(get_timestamp)"

    local confidence_value
    local confidence_ok
    IFS=':' read -r confidence_value confidence_ok <<< "$confidence_result"

    # Create report data
    local report_data
    report_data="$(cat << EOF
{
  "timestamp": "$timestamp",
  "gpu_name": "$GPU_NAME",
  "baseline_file": "$BASELINE_FILE",
  "baseline_throughput": $BASELINE_THROUGHPUT,
  "current_throughput": $CURRENT_THROUGHPUT,
  "performance_difference": $THROUGHPUT_DIFF,
  "threshold_percent": $THRESHOLD,
  "confidence_level": $confidence_value,
  "confidence_required": $CONFIDENCE,
  "gate_status": "$gate_status",
  "gate_message": "$gate_message",
  "ci_mode": $CI_MODE,
  "metrics": {
    "baseline": {
      "throughput_keys_per_sec": $BASELINE_THROUGHPUT,
      "gpu_utilization_percent": $BASELINE_GPU_UTILIZATION,
      "memory_bandwidth_gb_per_sec": $BASELINE_MEMORY_BANDWIDTH,
      "occupancy_percent": $BASELINE_OCCUPANCY,
      "power_consumption_watts": $BASELINE_POWER_CONSUMPTION
    },
    "current": {
      "throughput_keys_per_sec": $CURRENT_THROUGHPUT,
      "gpu_utilization_percent": $CURRENT_GPU_UTILIZATION,
      "memory_bandwidth_gb_per_sec": $CURRENT_MEMORY_BANDWIDTH,
      "occupancy_percent": $CURRENT_OCCUPANCY,
      "power_consumption_watts": $CURRENT_POWER_CONSUMPTION,
      "sample_count": $CURRENT_SAMPLE_COUNT
    }
  },
  "system_info": {
    "cuda_version": "$(nvcc --version | grep release | awk '{print $6}' | cut -c2- 2>/dev/null || echo "unknown")",
    "driver_version": "$(nvidia-smi --query-gpu=driver_version --format=csv,noheader,nounits | head -n1 2>/dev/null || echo "unknown")",
    "gpu_name": "$(nvidia-smi --query-gpu=name --format=csv,noheader,nounits | head -n1 2>/dev/null || echo "unknown")"
  }
}
EOF
)"

    # Output report based on format
    case "$REPORT_FORMAT" in
        json)
            if [[ -n "$OUTPUT_FILE" ]]; then
                echo "$report_data" > "$OUTPUT_FILE"
                log_info "Report saved to: $OUTPUT_FILE"
            else
                echo "$report_data"
            fi
            ;;
        junit)
            generate_junit_report "$gate_status" "$gate_message" "$report_data"
            ;;
        markdown)
            generate_markdown_report "$gate_status" "$gate_message" "$report_data"
            ;;
        *)
            error_exit "Unsupported report format: $REPORT_FORMAT"
            ;;
    esac

    # Save copy to validation directory
    local project_root
    project_root="$(get_project_root)"
    local validation_dir="$project_root/build/validation"
    ensure_dir "$validation_dir"

    echo "$report_data" > "$validation_dir/performance_gate_${GPU_NAME}_${timestamp}.json"
    log_debug "Report saved to validation directory"
}

# Generate JUnit XML report
generate_junit_report() {
    local gate_status="$1"
    local gate_message="$2"
    local report_data="$3"

    local timestamp
    timestamp="$(date -Iseconds)"

    local test_case
    if [[ "$gate_status" == "PASS" ]]; then
        test_case=""
    else
        test_case="<failure message=\"$gate_message\">Performance regression detected</failure>"
    fi

    cat << EOF
<?xml version="1.0" encoding="UTF-8"?>
<testsuites name="Performance Gate" tests="1" failures="$([[ "$gate_status" == "FAIL" ]] && echo 1 || echo 0)" time="0">
  <testsuite name="Performance Gate" tests="1" failures="$([[ "$gate_status" == "FAIL" ]] && echo 1 || echo 0)" timestamp="$timestamp">
    <testcase name="Performance Validation" classname="Performance.${GPU_NAME}" time="0">
      $test_case
    </testcase>
  </testsuite>
</testsuites>
EOF

    if [[ -n "$OUTPUT_FILE" ]]; then
        cat << EOF > "$OUTPUT_FILE"
<?xml version="1.0" encoding="UTF-8"?>
<testsuites name="Performance Gate" tests="1" failures="$([[ "$gate_status" == "FAIL" ]] && echo 1 || echo 0)" time="0">
  <testsuite name="Performance Gate" tests="1" failures="$([[ "$gate_status" == "FAIL" ]] && echo 1 || echo 0)" timestamp="$timestamp">
    <testcase name="Performance Validation" classname="Performance.${GPU_NAME}" time="0">
      $test_case
    </testcase>
  </testsuite>
</testsuites>
EOF
        log_info "JUnit report saved to: $OUTPUT_FILE"
    fi
}

# Generate Markdown report
generate_markdown_report() {
    local gate_status="$1"
    local gate_message="$2"
    local report_data="$3"

    local status_icon
    case "$gate_status" in
        PASS) status_icon="✅" ;;
        FAIL) status_icon="❌" ;;
        WARN) status_icon="⚠️" ;;
        *) status_icon="❓" ;;
    esac

    cat << EOF
# Performance Gate Report

## Summary

- **Status**: $status_icon $gate_status
- **GPU**: $GPU_NAME
- **Message**: $gate_message
- **Timestamp**: $(date)

## Performance Comparison

| Metric | Baseline | Current | Difference |
|--------|----------|---------|------------|
| Throughput (keys/sec) | $BASELINE_THROUGHPUT | $CURRENT_THROUGHPUT | ${THROUGHPUT_DIFF}% |
| GPU Utilization (%) | $BASELINE_GPU_UTILIZATION | $CURRENT_GPU_UTILIZATION | $(echo "scale=2; ($CURRENT_GPU_UTILIZATION - $BASELINE_GPU_UTILIZATION)" | bc -l)% |
| Memory Bandwidth (GB/s) | $BASELINE_MEMORY_BANDWIDTH | $CURRENT_MEMORY_BANDWIDTH | $(echo "scale=2; ($CURRENT_MEMORY_BANDWIDTH - $BASELINE_MEMORY_BANDWIDTH)" | bc -l) |
| Occupancy (%) | $BASELINE_OCCUPANCY | $CURRENT_OCCUPANCY | $(echo "scale=2; ($CURRENT_OCCUPANCY - $BASELINE_OCCUPANCY)" | bc -l) |

## Validation Results

- **Threshold**: ${THRESHOLD}%
- **Confidence**: $(echo "$confidence_result" | cut -d':' -f1) (required: $CONFIDENCE)
- **CI Mode**: $CI_MODE

## System Information

- **CUDA Version**: $(nvcc --version | grep release | awk '{print $6}' | cut -c2- 2>/dev/null || echo "unknown")
- **Driver Version**: $(nvidia-smi --query-gpu=driver_version --format=csv,noheader,nounits | head -n1 2>/dev/null || echo "unknown")
- **GPU Name**: $(nvidia-smi --query-gpu=name --format=csv,noheader,nounits | head -n1 2>/dev/null || echo "unknown")

EOF

    if [[ -n "$OUTPUT_FILE" ]]; then
        cat << EOF > "$OUTPUT_FILE"
# Performance Gate Report

## Summary

- **Status**: $status_icon $gate_status
- **GPU**: $GPU_NAME
- **Message**: $gate_message
- **Timestamp**: $(date)

## Performance Comparison

| Metric | Baseline | Current | Difference |
|--------|----------|---------|------------|
| Throughput (keys/sec) | $BASELINE_THROUGHPUT | $CURRENT_THROUGHPUT | ${THROUGHPUT_DIFF}% |
| GPU Utilization (%) | $BASELINE_GPU_UTILIZATION | $CURRENT_GPU_UTILIZATION | $(echo "scale=2; ($CURRENT_GPU_UTILIZATION - $BASELINE_GPU_UTILIZATION)" | bc -l)% |
| Memory Bandwidth (GB/s) | $BASELINE_MEMORY_BANDWIDTH | $CURRENT_MEMORY_BANDWIDTH | $(echo "scale=2; ($CURRENT_MEMORY_BANDWIDTH - $BASELINE_MEMORY_BANDWIDTH)" | bc -l) |
| Occupancy (%) | $BASELINE_OCCUPANCY | $CURRENT_OCCUPANCY | $(echo "scale=2; ($CURRENT_OCCUPANCY - $BASELINE_OCCUPANCY)" | bc -l) |

## Validation Results

- **Threshold**: ${THRESHOLD}%
- **Confidence**: $(echo "$confidence_result" | cut -d':' -f1) (required: $CONFIDENCE)
- **CI Mode**: $CI_MODE

## System Information

- **CUDA Version**: $(nvcc --version | grep release | awk '{print $6}' | cut -c2- 2>/dev/null || echo "unknown")
- **Driver Version**: $(nvidia-smi --query-gpu=driver_version --format=csv,noheader,nounits | head -n1 2>/dev/null || echo "unknown")
- **GPU Name**: $(nvidia-smi --query-gpu=name --format=csv,noheader,nounits | head -n1 2>/dev/null || echo "unknown")

EOF
        log_info "Markdown report saved to: $OUTPUT_FILE"
    fi
}

# Update baseline if improvement detected
update_baseline_if_needed() {
    if [[ "$UPDATE_BASELINE" != true ]]; then
        return 0
    fi

    local change_type="$1"

    if [[ "$change_type" == "improvement" ]]; then
        log_info "Updating baseline with improved performance..."

        local project_root
        project_root="$(get_project_root)"
        local result_file="$RESULT_DIR/latest_${GPU_NAME}.json"
        local backup_file="${BASELINE_FILE}.backup.$(date +%Y%m%d_%H%M%S)"

        # Backup current baseline
        if [[ -f "$BASELINE_FILE" ]]; then
            cp "$BASELINE_FILE" "$backup_file"
            log_info "Baseline backed up to: $backup_file"
        fi

        # Copy new results as baseline
        cp "$result_file" "$BASELINE_FILE"
        log_success "Baseline updated with improved performance"
    fi
}

# Handle gate result
handle_gate_result() {
    local gate_status="$1"
    local gate_message="$2"

    log_info "Performance gate result: $gate_status"

    case "$gate_status" in
        PASS)
            log_success "Performance gate PASSED: $gate_message"
            return 0
            ;;
        WARN)
            log_warning "Performance gate WARNING: $gate_message"
            return 0
            ;;
        FAIL)
            log_error "Performance gate FAILED: $gate_message"

            if [[ "$CI_MODE" == true ]]; then
                # In CI mode, create failure indicator file
                local project_root
                project_root="$(get_project_root)"
                local failure_file="$project_root/build/validation/performance_regression.json"
                ensure_dir "$(dirname "$failure_file")"

                cat > "$failure_file" << EOF
{
  "status": "FAILED",
  "message": "$gate_message",
  "gpu": "$GPU_NAME",
  "timestamp": "$(date -Iseconds)",
  "baseline_throughput": $BASELINE_THROUGHPUT,
  "current_throughput": $CURRENT_THROUGHPUT,
  "performance_difference_percent": $THROUGHPUT_DIFF,
  "threshold_percent": $THRESHOLD
}
EOF
                log_error "Performance regression file created: $failure_file"
            fi

            return 1
            ;;
        *)
            log_error "Unknown gate status: $gate_status"
            return 1
            ;;
    esac
}

# Main function
main() {
    # Parse arguments first
    parse_args "$@"

    log_info "Starting performance gate validation for GPU: $GPU_NAME"

    # Validate environment
    validate_performance_environment

    # Load baseline and current data
    load_baseline_data > /dev/null
    load_current_results > /dev/null

    # Calculate performance difference
    local perf_result
    perf_result="$(calculate_performance_difference "$BASELINE_THROUGHPUT" "$CURRENT_THROUGHPUT")"

    local change_type
    local throughput_diff
    local abs_diff
    IFS=':' read -r change_type throughput_diff abs_diff <<< "$perf_result"
    THROUGHPUT_DIFF="$throughput_diff"

    # Validate against threshold
    local gate_result
    gate_result="$(validate_performance_threshold "$perf_result" "$change_type" "$abs_diff")"

    local gate_status
    local gate_message
    IFS=':' read -r gate_status gate_message <<< "$gate_result"

    # Calculate statistical confidence
    local confidence_result
    confidence_result="$(calculate_confidence "$BASELINE_THROUGHPUT" "$CURRENT_THROUGHPUT" "$CURRENT_SAMPLE_COUNT")"

    # Generate report
    generate_report "$gate_status" "$gate_message" "$confidence_result"

    # Update baseline if needed
    update_baseline_if_needed "$change_type"

    # Handle result
    handle_gate_result "$gate_status" "$gate_message"
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
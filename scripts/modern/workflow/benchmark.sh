#!/bin/bash
# Puzzle71Solver - Performance Benchmark Script
# GPU performance monitoring and regression detection

set -euo pipefail

# Get script directory and load utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=../utils.sh
source "$SCRIPT_DIR/../utils.sh"

# Default configuration
DEFAULT_DURATION="600"  # 10 minutes
DEFAULT_WARMUP_TIME="60"  # 1 minute
DEFAULT_SAMPLE_INTERVAL="30"
DEFAULT_OUTPUT_FORMAT="json"
DEFAULT_TARGET="Puzzle71Solver"

# Show help
show_help() {
    cat << EOF
Puzzle71Solver Performance Benchmark Script

USAGE:
    benchmark.sh [options]

TARGETS:
    Puzzle71Solver      Benchmark main executable (default)
    all                 Benchmark all available executables

OPTIONS:
    --duration <seconds>     Benchmark duration [default: $DEFAULT_DURATION]
    --warmup <seconds>       Warmup time [default: $DEFAULT_WARMUP_TIME]
    --interval <seconds>     Sample interval [default: $DEFAULT_SAMPLE_INTERVAL]
    --gpu <device>           GPU device to use [default: auto-detect]
    --baseline <file>        Baseline file for comparison
    --update-baseline <file> Update baseline file with results
    --output <format>        Output format (json|csv|markdown) [default: $DEFAULT_OUTPUT_FORMAT]
    --output-dir <dir>       Output directory [default: build/benchmarks]
    --profile                Enable CUDA profiling
    --stress                 Run stress benchmark
    --memory                 Memory bandwidth benchmark
    --compute                Compute benchmark only
    --continuous             Continuous monitoring mode
    --regression-check       Check for performance regression
    --threshold <percent>    Regression threshold percentage [default: 5.0]
    --verbose, -v            Enable verbose output
    --help, -h               Show this help

GPU PROFILING:
    --profile-kernel <name>  Profile specific kernel
    --profile-metrics <list> Comma-separated list of metrics
    --nsight                 Use Nsight Compute for profiling

EXAMPLES:
    benchmark.sh                          # Standard 10-minute benchmark
    benchmark.sh --duration 300            # 5-minute benchmark
    benchmark.sh --gpu 0                   # Benchmark GPU 0
    benchmark.sh --baseline baseline.json  # Compare with baseline
    benchmark.sh --profile --nsight        # Full profiling session
    benchmark.sh --stress --duration 120   # 2-minute stress test

ENVIRONMENT VARIABLES:
    BENCHMARK_DURATION    Override benchmark duration
    BENCHMARK_GPU        Override GPU device
    BENCHMARK_OUTPUT_DIR Override output directory
EOF
}

# Parse command line arguments
parse_args() {
    DURATION="${BENCHMARK_DURATION:-$DEFAULT_DURATION}"
    WARMUP_TIME="${BENCHMARK_WARMUP:-$DEFAULT_WARMUP_TIME}"
    SAMPLE_INTERVAL="${BENCHMARK_INTERVAL:-$DEFAULT_SAMPLE_INTERVAL}"
    OUTPUT_FORMAT="${BENCHMARK_OUTPUT:-$DEFAULT_OUTPUT_FORMAT}"
    OUTPUT_DIR="${BENCHMARK_OUTPUT_DIR:-}"
    TARGET="${BENCHMARK_TARGET:-$DEFAULT_TARGET}"
    GPU_DEVICE="${BENCHMARK_GPU:-}"
    BASELINE_FILE=""
    UPDATE_BASELINE=""
    ENABLE_PROFILING=false
    STRESS_TEST=false
    MEMORY_BENCHMARK=false
    COMPUTE_BENCHMARK=false
    CONTINUOUS_MODE=false
    REGRESSION_CHECK=false
    REGRESSION_THRESHOLD="5.0"
    PROFILE_KERNEL=""
    PROFILE_METRICS=""
    NSIGHT_PROFILING=false
    VERBOSE=false

    while [[ $# -gt 0 ]]; do
        case $1 in
            --duration)
                DURATION="$2"
                shift 2
                ;;
            --warmup)
                WARMUP_TIME="$2"
                shift 2
                ;;
            --interval)
                SAMPLE_INTERVAL="$2"
                shift 2
                ;;
            --gpu)
                GPU_DEVICE="$2"
                shift 2
                ;;
            --baseline)
                BASELINE_FILE="$2"
                shift 2
                ;;
            --update-baseline)
                UPDATE_BASELINE="$2"
                shift 2
                ;;
            --output)
                OUTPUT_FORMAT="$2"
                shift 2
                ;;
            --output-dir)
                OUTPUT_DIR="$2"
                shift 2
                ;;
            --profile)
                ENABLE_PROFILING=true
                shift
                ;;
            --stress)
                STRESS_TEST=true
                shift
                ;;
            --memory)
                MEMORY_BENCHMARK=true
                shift
                ;;
            --compute)
                COMPUTE_BENCHMARK=true
                shift
                ;;
            --continuous)
                CONTINUOUS_MODE=true
                shift
                ;;
            --regression-check)
                REGRESSION_CHECK=true
                shift
                ;;
            --threshold)
                REGRESSION_THRESHOLD="$2"
                shift 2
                ;;
            --profile-kernel)
                PROFILE_KERNEL="$2"
                shift 2
                ;;
            --profile-metrics)
                PROFILE_METRICS="$2"
                shift 2
                ;;
            --nsight)
                NSIGHT_PROFILING=true
                ENABLE_PROFILING=true
                shift
                ;;
            --verbose|-v)
                VERBOSE=true
                shift
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            Puzzle71Solver|all)
                TARGET="$1"
                shift
                ;;
            *)
                error_exit "Unknown option: $1"
                ;;
        esac
    done

    # Set default output directory
    if [[ -z "$OUTPUT_DIR" ]]; then
        OUTPUT_DIR="$(get_project_root)/build/benchmarks"
    fi

    # Validate duration
    if ! [[ "$DURATION" =~ ^[0-9]+$ ]] || [[ "$DURATION" -lt 10 ]]; then
        error_exit "Duration must be at least 10 seconds"
    fi
}

# Validate benchmark environment
validate_benchmark_environment() {
    log_debug "Validating benchmark environment..."

    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    # Check build directory
    require_dir "$build_dir" "Build directory"

    # Check for target executable
    local executable="$build_dir/$TARGET"
    if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
        executable="${executable}.exe"
    fi

    require_file "$executable" "Benchmark executable"

    # Check GPU availability
    if ! check_gpu_available; then
        error_exit "GPU not available for benchmarking"
    fi

    # Check CUDA tools for profiling
    if [[ "$ENABLE_PROFILING" == true ]]; then
        if ! command_exists "nvprof" && ! command_exists "nsight"; then
            log_warning "No CUDA profiling tools found. Install Nsight Compute or CUDA Toolkit."
            ENABLE_PROFILING=false
        fi
    fi

    log_debug "Benchmark environment validated"
}

# Detect GPU information
detect_gpu_info() {
    local gpu_info
    gpu_info="$(get_gpu_info)"

    if [[ "$gpu_info" == "No GPU detected" ]]; then
        error_exit "No GPU detected for benchmarking"
    fi

    log_info "GPU detected: $gpu_info"

    # Extract GPU name for baseline files
    GPU_NAME=$(echo "$gpu_info" | cut -d',' -f1 | tr ' ' '_' | tr '[:upper:]' '[:lower:]')
    GPU_MEMORY=$(echo "$gpu_info" | cut -d',' -f2 | tr -d ' ')

    log_debug "GPU name: $GPU_NAME"
    log_debug "GPU memory: ${GPU_MEMORY}MB"
}

# Prepare benchmark environment
prepare_benchmark_environment() {
    progress_start "Preparing benchmark environment"

    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    cd "$build_dir"

    # Create output directory
    ensure_dir "$OUTPUT_DIR"

    # Set GPU device if specified
    if [[ -n "$GPU_DEVICE" ]]; then
        export CUDA_VISIBLE_DEVICES="$GPU_DEVICE"
        log_info "Using GPU device: $GPU_DEVICE"
    fi

    # Set benchmarking environment variables
    export BENCHMARK_MODE="1"
    export BENCHMARK_DURATION="$DURATION"
    export BENCHMARK_WARMUP="$WARMUP_TIME"
    export BENCHMARK_INTERVAL="$SAMPLE_INTERVAL"

    # Create results file
    local timestamp
    timestamp="$(get_timestamp)"
    RESULTS_FILE="$OUTPUT_DIR/benchmark_${GPU_NAME}_${timestamp}.json"
    add_temp_file "$RESULTS_FILE"

    progress_end "Benchmark environment preparation"
}

# Generate benchmark command
generate_benchmark_command() {
    local executable="$1"
    local cmd=("$executable")

    # Add benchmark-specific arguments
    cmd+=("--benchmark")
    cmd+=("--duration" "$DURATION")
    cmd+=("--warmup" "$WARMUP_TIME")
    cmd+=("--interval" "$SAMPLE_INTERVAL")
    cmd+=("--output" "$RESULTS_FILE")

    # Add stress test arguments
    if [[ "$STRESS_TEST" == true ]]; then
        cmd+=("--stress")
    fi

    # Add memory benchmark arguments
    if [[ "$MEMORY_BENCHMARK" == true ]]; then
        cmd+=("--memory-bandwidth")
    fi

    # Add compute benchmark arguments
    if [[ "$COMPUTE_BENCHMARK" == true ]]; then
        cmd+=("--compute-only")
    fi

    # Add profiling arguments
    if [[ "$ENABLE_PROFILING" == true ]]; then
        if [[ "$NSIGHT_PROFILING" == true ]]; then
            cmd+=("--nsight-profile")
        else
            cmd+=("--cuda-profile")
        fi

        if [[ -n "$PROFILE_KERNEL" ]]; then
            cmd+=("--profile-kernel" "$PROFILE_KERNEL")
        fi

        if [[ -n "$PROFILE_METRICS" ]]; then
            cmd+=("--profile-metrics" "$PROFILE_METRICS")
        fi
    fi

    # Add continuous mode arguments
    if [[ "$CONTINUOUS_MODE" == true ]]; then
        cmd+=("--continuous")
    fi

    echo "${cmd[@]}"
}

# Run benchmark with profiling
run_benchmark_with_profiling() {
    local executable="$1"
    shift
    local benchmark_cmd=("$@")

    log_info "Running benchmark with CUDA profiling..."

    local profiling_output="${RESULTS_FILE%.json}_profile"
    local profile_cmd

    if command_exists "nsight" && [[ "$NSIGHT_PROFILING" == true ]]; then
        # Use Nsight Compute
        profile_cmd=(
            nsight
            --profile-all-processes
            --stats=true
            --export="$profiling_output"
            "${benchmark_cmd[@]}"
        )
    elif command_exists "nvprof"; then
        # Use nvprof
        profile_cmd=(
            nvprof
            --output-profile "$profiling_output.prof"
            --print-gpu-trace
            "${benchmark_cmd[@]}"
        )
    else
        log_warning "No profiling tools available, running without profiling"
        run_simple_benchmark "$executable" "${benchmark_cmd[@]}"
        return
    fi

    if [[ "$VERBOSE" == true ]]; then
        set -x
    fi

    local exit_code=0
    timeout $((DURATION + WARMUP_TIME + 120)) "${profile_cmd[@]}" || exit_code=$?

    if [[ "$VERBOSE" == true ]]; then
        set +x
    fi

    return $exit_code
}

# Run simple benchmark
run_simple_benchmark() {
    local executable="$1"
    shift
    local benchmark_cmd=("$@")

    log_info "Running benchmark..."

    if [[ "$VERBOSE" == true ]]; then
        set -x
    fi

    local exit_code=0
    timeout $((DURATION + WARMUP_TIME + 60)) "${benchmark_cmd[@]}" || exit_code=$?

    if [[ "$VERBOSE" == true ]]; then
        set +x
    fi

    return $exit_code
}

# Parse benchmark results
parse_benchmark_results() {
    local results_file="$1"

    if [[ ! -f "$results_file" ]]; then
        log_warning "Results file not found: $results_file"
        return 1
    fi

    log_info "Parsing benchmark results..."

    # Extract key metrics (this depends on the actual output format)
    if command_exists "jq"; then
        local avg_throughput
        avg_throughput=$(jq -r '.average_throughput // 0' "$results_file" 2>/dev/null || echo "0")
        local peak_throughput
        peak_throughput=$(jq -r '.peak_throughput // 0' "$results_file" 2>/dev/null || echo "0")
        local gpu_utilization
        gpu_utilization=$(jq -r '.gpu_utilization // 0' "$results_file" 2>/dev/null || echo "0")
        local memory_bandwidth
        memory_bandwidth=$(jq -r '.memory_bandwidth // 0' "$results_file" 2>/dev/null || echo "0")

        log_info "Benchmark results:"
        log_info "  Average throughput: ${avg_throughput} keys/sec"
        log_info "  Peak throughput: ${peak_throughput} keys/sec"
        log_info "  GPU utilization: ${gpu_utilization}%"
        log_info "  Memory bandwidth: ${memory_bandwidth} GB/s"

        # Store results for later comparison
        AVG_THROUGHPUT="$avg_throughput"
        PEAK_THROUGHPUT="$peak_throughput"
        GPU_UTILIZATION="$gpu_utilization"
        MEMORY_BANDWIDTH="$memory_bandwidth"
    else
        log_warning "jq not available, cannot parse JSON results"
    fi
}

# Compare with baseline
compare_with_baseline() {
    if [[ -z "$BASELINE_FILE" || ! -f "$BASELINE_FILE" ]]; then
        log_info "No baseline file provided for comparison"
        return 0
    fi

    log_info "Comparing with baseline: $BASELINE_FILE"

    if command_exists "jq"; then
        local baseline_throughput
        baseline_throughput=$(jq -r '.average_throughput // 0' "$BASELINE_FILE" 2>/dev/null || echo "0")

        if [[ "$baseline_throughput" == "0" ]]; then
            log_warning "Could not extract baseline throughput"
            return 1
        fi

        local throughput_change
        throughput_change=$(echo "scale=2; ($AVG_THROUGHPUT - $baseline_throughput) / $baseline_throughput * 100" | bc -l 2>/dev/null || echo "0")

        log_info "Baseline throughput: ${baseline_throughput} keys/sec"
        log_info "Current throughput: ${AVG_THROUGHPUT} keys/sec"
        log_info "Performance change: ${throughput_change}%"

        # Check for regression
        if [[ "$REGRESSION_CHECK" == true ]]; then
            local regression_threshold_negative
            regression_threshold_negative=$(echo "scale=2; -$REGRESSION_THRESHOLD" | bc -l)

            if (( $(echo "$throughput_change < $regression_threshold_negative" | bc -l) )); then
                log_error "Performance regression detected! Change: ${throughput_change}% (threshold: -$REGRESSION_THRESHOLD%)"
                return 1
            else
                log_success "No performance regression detected"
            fi
        fi
    else
        log_warning "jq not available, cannot compare with baseline"
    fi
}

# Update baseline file
update_baseline_file() {
    if [[ -z "$UPDATE_BASELINE" ]]; then
        return 0
    fi

    log_info "Updating baseline file: $UPDATE_BASELINE"

    # Ensure baseline directory exists
    local baseline_dir
    baseline_dir="$(dirname "$UPDATE_BASELINE")"
    ensure_dir "$baseline_dir"

    # Copy results to baseline
    if [[ -f "$RESULTS_FILE" ]]; then
        cp "$RESULTS_FILE" "$UPDATE_BASELINE"
        log_success "Baseline updated: $UPDATE_BASELINE"
    else
        log_warning "No results file to update baseline with"
    fi
}

# Generate benchmark report
generate_benchmark_report() {
    local output_file="${RESULTS_FILE%.json}_report.${OUTPUT_FORMAT}"

    log_info "Generating benchmark report: $output_file"

    case "$OUTPUT_FORMAT" in
        "json")
            generate_json_report "$output_file"
            ;;
        "csv")
            generate_csv_report "$output_file"
            ;;
        "markdown")
            generate_markdown_report "$output_file"
            ;;
        *)
            log_warning "Unknown output format: $OUTPUT_FORMAT"
            ;;
    esac
}

# Generate JSON report
generate_json_report() {
    local output_file="$1"

    if [[ -f "$RESULTS_FILE" ]]; then
        # Add metadata to results
        if command_exists "jq"; then
            jq --arg timestamp "$(date -Iseconds)" \
               --arg gpu_name "$GPU_NAME" \
               --arg gpu_memory "$GPU_MEMORY" \
               --arg duration "$DURATION" \
               '. + {
                   timestamp: $timestamp,
                   gpu_name: $gpu_name,
                   gpu_memory_mb: ($gpu_memory | tonumber),
                   duration_seconds: ($duration | tonumber)
               }' "$RESULTS_FILE" > "$output_file"

            log_success "JSON report generated: $output_file"
        else
            cp "$RESULTS_FILE" "$output_file"
            log_info "Results copied to: $output_file"
        fi
    fi
}

# Generate CSV report
generate_csv_report() {
    local output_file="$1"

    cat > "$output_file" << EOF
timestamp,gpu_name,duration,avg_throughput,peak_throughput,gpu_utilization,memory_bandwidth
$(date -Iseconds),$GPU_NAME,$DURATION,$AVG_THROUGHPUT,$PEAK_THROUGHPUT,$GPU_UTILIZATION,$MEMORY_BANDWIDTH
EOF

    log_success "CSV report generated: $output_file"
}

# Generate Markdown report
generate_markdown_report() {
    local output_file="$1"

    cat > "$output_file" << EOF
# Puzzle71Solver Performance Benchmark Report

## Configuration
- **GPU**: $GPU_NAME (${GPU_MEMORY}MB)
- **Duration**: ${DURATION}s
- **Warmup**: ${WARMUP_TIME}s
- **Sample Interval**: ${SAMPLE_INTERVAL}s
- **Timestamp**: $(date -Iseconds)

## Results

| Metric | Value |
|--------|-------|
| Average Throughput | ${AVG_THROUGHPUT} keys/sec |
| Peak Throughput | ${PEAK_THROUGHPUT} keys/sec |
| GPU Utilization | ${GPU_UTILIZATION}% |
| Memory Bandwidth | ${MEMORY_BANDWIDTH} GB/s |

## Performance Analysis

EOF

    if [[ -n "$BASELINE_FILE" ]]; then
        cat >> "$output_file" << EOF
### Baseline Comparison
- **Baseline**: $BASELINE_FILE
- **Performance Change**: Calculated above

EOF
    fi

    cat >> "$output_file" << EOF
### Recommendations
- Target throughput: >1000M keys/s (Turing), >4000M keys/s (Hopper)
- Target GPU utilization: >90%
- Target memory bandwidth: >70% of peak

## Files
- **Raw Results**: $RESULTS_FILE
- **Report**: $output_file
EOF

    log_success "Markdown report generated: $output_file"
}

# Show benchmark summary
show_benchmark_summary() {
    local exit_code="$1"

    cat << EOF

🎯 Benchmark execution completed with exit code: $exit_code

Benchmark summary:
- GPU: $GPU_NAME (${GPU_MEMORY}MB)
- Duration: ${DURATION}s
- Results: $RESULTS_FILE

Performance results:
- Average throughput: ${AVG_THROUGHPUT:-N/A} keys/sec
- Peak throughput: ${PEAK_THROUGHPUT:-N/A} keys/sec
- GPU utilization: ${GPU_UTILIZATION:-N/A}%
- Memory bandwidth: ${MEMORY_BANDWIDTH:-N/A} GB/s

EOF

    if [[ $exit_code -eq 0 ]]; then
        log_success "Benchmark completed successfully! 🚀"
    else
        log_error "Benchmark failed! ❌"
        return 1
    fi

    # Show performance targets
    local target_throughput="1000"  # Default for Turing
    if [[ "$GPU_NAME" =~ rtx3090 ]]; then
        target_throughput="2000"
    elif [[ "$GPU_NAME" =~ (h20|a100) ]]; then
        target_throughput="4000"
    fi

    echo "Performance targets:"
    echo "- Target throughput: ${target_throughput}M+ keys/sec"
    echo "- Target GPU utilization: 90%+"
    echo "- Target memory bandwidth: 70%+ of peak"
    echo ""
}

# Main benchmark function
main() {
    log_info "Starting Puzzle71Solver performance benchmark..."

    # Parse arguments
    parse_args "$@"

    # Validate environment
    validate_benchmark_environment

    # Detect GPU information
    detect_gpu_info

    # Show configuration
    log_info "Benchmark configuration:"
    log_info "  Target: $TARGET"
    log_info "  Duration: ${DURATION}s"
    log_info "  Warmup: ${WARMUP_TIME}s"
    log_info "  GPU: $GPU_NAME"
    log_info "  Output format: $OUTPUT_FORMAT"
    log_info "  Output directory: $OUTPUT_DIR"

    # Prepare environment
    prepare_benchmark_environment

    # Generate benchmark command
    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"
    local executable="$build_dir/$TARGET"

    local benchmark_cmd_str
    benchmark_cmd_str="$(generate_benchmark_command "$executable")"
    read -ra benchmark_cmd <<< "$benchmark_cmd_str"

    # Run benchmark
    local exit_code=0
    if [[ "$ENABLE_PROFILING" == true ]]; then
        run_benchmark_with_profiling "$executable" "${benchmark_cmd[@]}" || exit_code=$?
    else
        run_simple_benchmark "$executable" "${benchmark_cmd[@]}" || exit_code=$?
    fi

    # Process results
    parse_benchmark_results "$RESULTS_FILE"

    # Compare with baseline
    compare_with_baseline

    # Update baseline if requested
    update_baseline_file

    # Generate report
    generate_benchmark_report

    # Show summary
    show_benchmark_summary "$exit_code"

    exit $exit_code
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
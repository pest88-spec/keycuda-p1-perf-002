#!/bin/bash
# Puzzle71Solver - CUDA Profiling Script
# Advanced GPU kernel profiling and performance analysis

set -euo pipefail

# Get script directory and load utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=../utils.sh
source "$SCRIPT_DIR/../utils.sh"

# Default configuration
DEFAULT_PROFILE_TYPE="compute"
DEFAULT_DURATION="60"
DEFAULT_OUTPUT_DIR="build/profiles"

# Show help
show_help() {
    cat << EOF
Puzzle71Solver CUDA Profiling Script

USAGE:
    profile.sh [options] [executable]

PROFILE TYPES:
    compute            Compute performance profiling (default)
    memory             Memory access profiling
    bandwidth          Memory bandwidth analysis
    occupancy          GPU occupancy analysis
    instruction        Instruction-level profiling
    power              Power consumption profiling
    trace              Full execution trace
    all                Run all profile types

OPTIONS:
    --type <type>         Profile type [default: $DEFAULT_PROFILE_TYPE]
    --duration <seconds>  Profiling duration [default: $DEFAULT_DURATION]
    --gpu <device>        GPU device to profile [default: auto-detect]
    --kernel <name>       Profile specific kernel only
    --metrics <list>      Custom metrics (comma-separated)
    --output-dir <dir>    Output directory [default: $DEFAULT_OUTPUT_DIR]
    --nsight              Use Nsight Compute instead of nvprof
    --visualize           Generate visualization reports
    --compare <file>      Compare with previous profile
    --baseline <file>     Baseline profile for regression detection
    --continuous          Continuous profiling mode
    --sampling <rate>     Sampling rate (Hz) [default: 1000]
    --verbose, -v         Enable verbose output
    --help, -h            Show this help

ADVANCED OPTIONS:
    --range-analysis      Range analysis for memory access patterns
    --memory-trace        Detailed memory access tracing
    --kernel-trace        Kernel launch and execution tracing
    --cuda-memcheck       Run with CUDA memory checking
    --device-query        Query device capabilities first

EXAMPLES:
    profile.sh                           # Basic compute profiling
    profile.sh --type memory             # Memory access profiling
    profile.sh --nsight --visualize      # Nsight profiling with visualization
    profile.sh --kernel eccScalarMul     # Profile specific kernel
    profile.sh --metrics sm__warps_active.avg.pct_of_peak_sustained_active

NSIGHT INTEGRATION:
    --nsight-sections <sections>   Nsight sections to run
    --nsight-kernel <kernel>       Nsight kernel filtering
    --nsight-metrics <metrics>     Nsight custom metrics

REQUIREMENTS:
    - CUDA Toolkit 11.0+
    - Nsight Compute (for --nsight option)
    - Sufficient GPU memory for profiling overhead

ENVIRONMENT VARIABLES:
    PROFILE_GPU           Override GPU device
    PROFILE_OUTPUT_DIR    Override output directory
EOF
}

# Parse command line arguments
parse_args() {
    PROFILE_TYPE="${PROFILE_TYPE:-$DEFAULT_PROFILE_TYPE}"
    DURATION="${PROFILE_DURATION:-$DEFAULT_DURATION}"
    OUTPUT_DIR="${PROFILE_OUTPUT_DIR:-$DEFAULT_OUTPUT_DIR}"
    GPU_DEVICE="${PROFILE_GPU:-}"
    KERNEL_NAME=""
    CUSTOM_METRICS=""
    USE_NSIGHT=false
    GENERATE_VISUALIZATION=false
    COMPARE_FILE=""
    BASELINE_FILE=""
    CONTINUOUS_MODE=false
    SAMPLING_RATE="1000"
    VERBOSE=false
    RANGE_ANALYSIS=false
    MEMORY_TRACE=false
    KERNEL_TRACE=false
    CUDA_MEMCHECK=false
    DEVICE_QUERY=false
    NSIGHT_SECTIONS=""
    NSIGHT_KERNEL_FILTER=""
    NSIGHT_METRICS=""

    EXECUTABLE="${1:-Puzzle71Solver}"

    while [[ $# -gt 0 ]]; do
        case $1 in
            --type)
                PROFILE_TYPE="$2"
                shift 2
                ;;
            --duration)
                DURATION="$2"
                shift 2
                ;;
            --gpu)
                GPU_DEVICE="$2"
                shift 2
                ;;
            --kernel)
                KERNEL_NAME="$2"
                shift 2
                ;;
            --metrics)
                CUSTOM_METRICS="$2"
                shift 2
                ;;
            --output-dir)
                OUTPUT_DIR="$2"
                shift 2
                ;;
            --nsight)
                USE_NSIGHT=true
                shift
                ;;
            --visualize)
                GENERATE_VISUALIZATION=true
                shift
                ;;
            --compare)
                COMPARE_FILE="$2"
                shift 2
                ;;
            --baseline)
                BASELINE_FILE="$2"
                shift 2
                ;;
            --continuous)
                CONTINUOUS_MODE=true
                shift
                ;;
            --sampling)
                SAMPLING_RATE="$2"
                shift 2
                ;;
            --range-analysis)
                RANGE_ANALYSIS=true
                shift
                ;;
            --memory-trace)
                MEMORY_TRACE=true
                shift
                ;;
            --kernel-trace)
                KERNEL_TRACE=true
                shift
                ;;
            --cuda-memcheck)
                CUDA_MEMCHECK=true
                shift
                ;;
            --device-query)
                DEVICE_QUERY=true
                shift
                ;;
            --nsight-sections)
                NSIGHT_SECTIONS="$2"
                shift 2
                ;;
            --nsight-kernel)
                NSIGHT_KERNEL_FILTER="$2"
                shift 2
                ;;
            --nsight-metrics)
                NSIGHT_METRICS="$2"
                shift 2
                ;;
            --verbose|-v)
                VERBOSE=true
                shift
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            Puzzle71Solver|*.exe)
                EXECUTABLE="$1"
                shift
                ;;
            *)
                error_exit "Unknown option: $1"
                ;;
        esac
    done
}

# Validate profiling environment
validate_profiling_environment() {
    log_debug "Validating profiling environment..."

    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    # Check build directory
    require_dir "$build_dir" "Build directory"

    # Check for executable
    local executable="$build_dir/$EXECUTABLE"
    if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
        executable="${executable}.exe"
    fi

    require_file "$executable" "Executable to profile"

    # Check GPU availability
    if ! check_gpu_available; then
        error_exit "GPU not available for profiling"
    fi

    # Check profiling tools
    if [[ "$USE_NSIGHT" == true ]]; then
        if ! command_exists "nsight"; then
            error_exit "Nsight Compute not found. Install Nsight Compute for CUDA profiling."
        fi
    else
        if ! command_exists "nvprof"; then
            log_warning "nvprof not found. Some profiling features may be unavailable."
        fi
    fi

    # Check for CUDA memory checking
    if [[ "$CUDA_MEMCHECK" == true ]] && ! command_exists "cuda-memcheck"; then
        error_exit "cuda-memcheck not found"
    fi

    log_debug "Profiling environment validated"
}

# Query device capabilities
query_device_capabilities() {
    progress_start "Querying device capabilities"

    local device_id="${GPU_DEVICE:-0}"
    local query_file="$OUTPUT_DIR/device_query_$device_id.txt"

    if ! command_exists "deviceQuery"; then
        log_warning "deviceQuery not found, using nvidia-smi for basic info"
        nvidia-smi --query-gpu=name,memory.total,compute_cap,multiprocessors,clocks.max.sm --format=csv > "$query_file"
    else
        deviceQuery --device="$device_id" > "$query_file" 2>&1
    fi

    log_info "Device capabilities saved to: $query_file"

    # Extract key information
    if [[ -f "$query_file" ]]; then
        local compute_cap
        compute_cap=$(grep -i "compute capability" "$query_file" | head -n1 | awk '{print $3}' || echo "unknown")
        log_info "Compute capability: $compute_cap"

        local sm_count
        sm_count=$(grep -i "multiprocessors" "$query_file" | head -n1 | awk '{print $2}' || echo "unknown")
        log_info "SM count: $sm_count"
    fi

    progress_end "Device capabilities query"
}

# Prepare profiling environment
prepare_profiling_environment() {
    progress_start "Preparing profiling environment"

    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    cd "$build_dir"

    # Create output directory
    ensure_dir "$OUTPUT_DIR"

    # Set GPU device
    if [[ -n "$GPU_DEVICE" ]]; then
        export CUDA_VISIBLE_DEVICES="$GPU_DEVICE"
        log_info "Using GPU device: $GPU_DEVICE"
    fi

    # Set profiling environment variables
    export CUDA_PROFILE_ENABLE="1"
    export CUDA_PROFILE_CSV="1"

    # Create profile filename with timestamp
    local timestamp
    timestamp="$(get_timestamp)"
    PROFILE_FILE="$OUTPUT_DIR/profile_${PROFILE_TYPE}_${timestamp}.csv"

    progress_end "Profiling environment preparation"
}

# Get profiling metrics for profile type
get_profile_metrics() {
    local profile_type="$1"

    case "$profile_type" in
        "compute")
            echo "sm__warps_active.avg.pct_of_peak_sustained_active,sm__inst_executed.avg.pct_of_peak_sustained_active,sm__inst_integer.avg.pct_of_peak_sustained_active,sm__inst_fp.avg.pct_of_peak_sustained_active"
            ;;
        "memory")
            echo "dram__bytes_read.sum,dram__bytes_written.sum,l2__bytes_read.sum,l2__bytes_written.sum"
            ;;
        "bandwidth")
            echo "dram__bytes_read.sum,dram__bytes_written.sum,sm__sass_thread_inst_executed_op_memory_pred_on.sum"
            ;;
        "occupancy")
            echo "sm__warps_active.avg.pct_of_peak_sustained_active,sm__warps_issued.avg.pct_of_peak_sustained_active,sm__warps_launched.avg.pct_of_peak_sustained_active"
            ;;
        "instruction")
            echo "sm__inst_executed.avg.per_cycle_active,sm__inst_issued.avg.per_cycle_active,sm__inst_fp.avg.per_cycle_active,sm__inst_integer.avg.per_cycle_active"
            ;;
        "power")
            echo "gpu__power_average.instant,gpu__power_average.violations,sm__warps_active.avg.pct_of_peak_sustained_active"
            ;;
        *)
            echo ""
            ;;
    esac
}

# Generate Nsight profiling command
generate_nsight_command() {
    local executable="$1"
    shift
    local nsight_cmd=("nsight" "--profile-all-processes" "--stats=true")

    # Add output configuration
    nsight_cmd+=("--export=$OUTPUT_DIR/nsight_profile")

    # Add kernel filter if specified
    if [[ -n "$KERNEL_NAME" ]]; then
        nsight_cmd+=("--kernel-name-base" "regex" "--kernel-filter" "$KERNEL_NAME")
    fi

    # Add custom sections
    if [[ -n "$NSIGHT_SECTIONS" ]]; then
        nsight_cmd+=("--sections" "$NSIGHT_SECTIONS")
    fi

    # Add custom metrics
    if [[ -n "$NSIGHT_METRICS" ]]; then
        nsight_cmd+=("--metrics" "$NSIGHT_METRICS")
    elif [[ -n "$CUSTOM_METRICS" ]]; then
        nsight_cmd+=("--metrics" "$CUSTOM_METRICS")
    elif [[ "$PROFILE_TYPE" != "all" ]]; then
        local metrics
        metrics=$(get_profile_metrics "$PROFILE_TYPE")
        if [[ -n "$metrics" ]]; then
            nsight_cmd+=("--metrics" "$metrics")
        fi
    fi

    # Add application arguments
    nsight_cmd+=("--" "$executable")

    # Add application-specific arguments
    nsight_cmd+=("--profile" "--duration" "$DURATION")

    if [[ -n "$KERNEL_NAME" ]]; then
        nsight_cmd+=("--kernel-filter" "$KERNEL_NAME")
    fi

    echo "${nsight_cmd[@]}"
}

# Generate nvprof profiling command
generate_nvprof_command() {
    local executable="$1"
    shift
    local nvprof_cmd=("nvprof")

    # Add output configuration
    nvprof_cmd+=("--output-profile" "$OUTPUT_DIR/nvprof_profile.prof")
    nvprof_cmd+=("--csv" "--log-file" "$OUTPUT_DIR/nvprof.log")

    # Add profiling metrics
    if [[ -n "$CUSTOM_METRICS" ]]; then
        IFS=',' read -ra metrics <<< "$CUSTOM_METRICS"
        for metric in "${metrics[@]}"; do
            nvprof_cmd+=("--metrics" "$metric")
        done
    elif [[ "$PROFILE_TYPE" != "all" ]]; then
        local metrics
        metrics=$(get_profile_metrics "$PROFILE_TYPE")
        if [[ -n "$metrics" ]]; then
            IFS=',' read -ra metric_array <<< "$metrics"
            for metric in "${metric_array[@]}"; do
                nvprof_cmd+=("--metrics" "$metric")
            done
        fi
    fi

    # Add memory tracing if requested
    if [[ "$MEMORY_TRACE" == true ]]; then
        nvprof_cmd+=("--memory-trace" "all")
    fi

    # Add kernel tracing if requested
    if [[ "$KERNEL_TRACE" == true ]]; then
        nvprof_cmd+=("--print-gpu-trace")
    fi

    # Add application
    nvprof_cmd+=("$executable")

    # Add application arguments
    nvprof_cmd+=("--profile" "--duration" "$DURATION")

    if [[ -n "$KERNEL_NAME" ]]; then
        nvprof_cmd+=("--kernel-filter" "$KERNEL_NAME")
    fi

    echo "${nvprof_cmd[@]}"
}

# Run profiling with Nsight
run_nsight_profiling() {
    local executable="$1"

    log_info "Running Nsight Compute profiling..."

    local nsight_cmd_str
    nsight_cmd_str="$(generate_nsight_command "$executable")"
    read -ra nsight_cmd <<< "$nsight_cmd_str"

    if [[ "$VERBOSE" == true ]]; then
        set -x
    fi

    local exit_code=0
    timeout $((DURATION + 120)) "${nsight_cmd[@]}" || exit_code=$?

    if [[ "$VERBOSE" == true ]]; then
        set +x
    fi

    return $exit_code
}

# Run profiling with nvprof
run_nvprof_profiling() {
    local executable="$1"

    log_info "Running nvprof profiling..."

    local nvprof_cmd_str
    nvprof_cmd_str="$(generate_nvprof_command "$executable")"
    read -ra nvprof_cmd <<< "$nvprof_cmd_str"

    if [[ "$VERBOSE" == true ]]; then
        set -x
    fi

    local exit_code=0
    timeout $((DURATION + 120)) "${nvprof_cmd[@]}" || exit_code=$?

    if [[ "$VERBOSE" == true ]]; then
        set +x
    fi

    return $exit_code
}

# Run profiling with cuda-memcheck
run_cuda_memcheck() {
    local executable="$1"

    log_info "Running profiling with CUDA memory checking..."

    local memcheck_cmd=("cuda-memcheck" "--tool=memcheck" "--leak-check=full")

    # Add executable
    memcheck_cmd+=("$executable")
    memcheck_cmd+=("--profile" "--duration" "$DURATION")

    if [[ -n "$KERNEL_NAME" ]]; then
        memcheck_cmd+=("--kernel-filter" "$KERNEL_NAME")
    fi

    if [[ "$VERBOSE" == true ]]; then
        set -x
    fi

    local exit_code=0
    timeout $((DURATION + 300)) "${memcheck_cmd[@]}" 2>&1 | tee "$OUTPUT_DIR/cuda_memcheck.log" || exit_code=$?

    if [[ "$VERBOSE" == true ]]; then
        set +x
    fi

    return $exit_code
}

# Process profiling results
process_profiling_results() {
    progress_start "Processing profiling results"

    local results_found=false

    # Check for Nsight results
    if ls "$OUTPUT_DIR"/nsight_profile*.csv 1> /dev/null 2>&1; then
        log_info "Processing Nsight results..."
        process_nsight_results
        results_found=true
    fi

    # Check for nvprof results
    if [[ -f "$OUTPUT_DIR/nvprof_profile.prof" ]]; then
        log_info "Processing nvprof results..."
        process_nvprof_results
        results_found=true
    fi

    # Check for memory check results
    if [[ -f "$OUTPUT_DIR/cuda_memcheck.log" ]]; then
        log_info "Processing memory check results..."
        process_memcheck_results
        results_found=true
    fi

    if [[ "$results_found" == false ]]; then
        log_warning "No profiling results found"
    fi

    progress_end "Profiling results processing"
}

# Process Nsight results
process_nsight_results() {
    local report_file="$OUTPUT_DIR/nsight_report.md"

    cat > "$report_file" << EOF
# Nsight Compute Profiling Report

## Configuration
- **Profile Type**: $PROFILE_TYPE
- **Duration**: ${DURATION}s
- **GPU Device**: ${GPU_DEVICE:-auto}
- **Timestamp**: $(date -Iseconds)

## Results

EOF

    # Process CSV files if available
    for csv_file in "$OUTPUT_DIR"/nsight_profile*.csv; do
        if [[ -f "$csv_file" ]]; then
            echo "### $(basename "$csv_file")" >> "$report_file"
            echo "" >> "$report_file"

            # Convert CSV to markdown table (simplified)
            if command_exists "python3"; then
                python3 -c "
import csv, sys
with open('$csv_file', 'r') as f:
    reader = csv.reader(f)
    headers = next(reader)
    print('| ' + ' | '.join(headers) + ' |')
    print('|' + '---|' * len(headers))
    for i, row in enumerate(reader):
        if i < 10:  # Limit to first 10 rows
            print('| ' + ' | '.join(row) + ' |')
" >> "$report_file"
            fi
        fi
    done

    log_success "Nsight report generated: $report_file"
}

# Process nvprof results
process_nvprof_results() {
    local report_file="$OUTPUT_DIR/nvprof_report.md"

    cat > "$report_file" << EOF
# nvprof Profiling Report

## Configuration
- **Profile Type**: $PROFILE_TYPE
- **Duration**: ${DURATION}s
- **GPU Device**: ${GPU_DEVICE:-auto}
- **Timestamp**: $(date -Iseconds)

## Results

EOF

    # Extract key metrics from nvprof output
    if [[ -f "$OUTPUT_DIR/nvprof.log" ]]; then
        echo "### Profile Summary" >> "$report_file"
        echo "" >> "$report_file"
        grep -E "(==[0-9]+==|Result)" "$OUTPUT_DIR/nvprof.log" | head -20 >> "$report_file"
    fi

    log_success "nvprof report generated: $report_file"
}

# Process memory check results
process_memcheck_results() {
    local report_file="$OUTPUT_DIR/memcheck_report.md"

    cat > "$report_file" << EOF
# CUDA Memory Check Report

## Configuration
- **Duration**: ${DURATION}s
- **GPU Device**: ${GPU_DEVICE:-auto}
- **Timestamp**: $(date -Iseconds)

## Results

EOF

    # Extract memory errors
    if [[ -f "$OUTPUT_DIR/cuda_memcheck.log" ]]; then
        local error_count
        error_count=$(grep -c "ERROR" "$OUTPUT_DIR/cuda_memcheck.log" 2>/dev/null || echo "0")
        local leak_count
        leak_count=$(grep -c "LEAK" "$OUTPUT_DIR/cuda_memcheck.log" 2>/dev/null || echo "0")

        echo "- **Memory Errors**: $error_count" >> "$report_file"
        echo "- **Memory Leaks**: $leak_count" >> "$report_file"
        echo "" >> "$report_file"

        if [[ "$error_count" -gt 0 || "$leak_count" -gt 0 ]]; then
            echo "### Issues Found" >> "$report_file"
            echo "" >> "$report_file"
            grep -E "(ERROR|LEAK)" "$OUTPUT_DIR/cuda_memcheck.log" | head -20 >> "$report_file"
        else
            echo "✅ No memory issues detected" >> "$report_file"
        fi
    fi

    log_success "Memory check report generated: $report_file"
}

# Generate visualization
generate_visualization() {
    if [[ "$GENERATE_VISUALIZATION" == false ]]; then
        return 0
    fi

    progress_start "Generating visualization"

    # Check for visualization tools
    if command_exists "python3"; then
        log_info "Generating performance visualization..."

        # Create Python visualization script
        cat > "$OUTPUT_DIR/visualize.py" << 'EOF'
#!/usr/bin/env python3
import matplotlib.pyplot as plt
import pandas as pd
import sys
import os

def plot_metrics(csv_files, output_dir):
    """Plot performance metrics from CSV files"""

    for csv_file in csv_files:
        if not os.path.exists(csv_file):
            continue

        try:
            df = pd.read_csv(csv_file)

            # Create plots for numeric columns
            numeric_cols = df.select_dtypes(include=['number']).columns

            if len(numeric_cols) > 0:
                fig, axes = plt.subplots(len(numeric_cols), 1, figsize=(10, 4*len(numeric_cols)))
                if len(numeric_cols) == 1:
                    axes = [axes]

                for i, col in enumerate(numeric_cols):
                    axes[i].plot(df.index, df[col])
                    axes[i].set_title(col)
                    axes[i].set_xlabel('Sample')
                    axes[i].grid(True)

                plt.tight_layout()
                output_file = os.path.join(output_dir, f"{os.path.basename(csv_file)}.png")
                plt.savefig(output_file, dpi=300, bbox_inches='tight')
                plt.close()

                print(f"Generated plot: {output_file}")

        except Exception as e:
            print(f"Error processing {csv_file}: {e}")

if __name__ == "__main__":
    csv_files = [f for f in sys.argv[1:] if f.endswith('.csv')]
    output_dir = os.path.dirname(csv_files[0]) if csv_files else "."

    if csv_files:
        plot_metrics(csv_files, output_dir)
    else:
        print("No CSV files found for visualization")
EOF

        # Run visualization
        python3 "$OUTPUT_DIR/visualize.py" "$OUTPUT_DIR"/*.csv 2>/dev/null || {
            log_warning "Visualization generation failed (matplotlib may not be available)"
        }
    else
        log_warning "Python3 not available, skipping visualization"
    fi

    progress_end "Visualization generation"
}

# Compare with baseline
compare_with_baseline() {
    if [[ -z "$BASELINE_FILE" || ! -f "$BASELINE_FILE" ]]; then
        return 0
    fi

    log_info "Comparing with baseline: $BASELINE_FILE"

    # This would need detailed implementation based on the profiling format
    log_info "Baseline comparison would be performed here"
}

# Show profiling summary
show_profiling_summary() {
    local exit_code="$1"

    cat << EOF

🔬 Profiling execution completed with exit code: $exit_code

Profiling summary:
- Type: $PROFILE_TYPE
- Duration: ${DURATION}s
- GPU: ${GPU_DEVICE:-auto}
- Output directory: $OUTPUT_DIR

Generated files:
EOF

    # List generated files
    if ls "$OUTPUT_DIR"/*.{md,csv,prof,png,log} 1> /dev/null 2>&1; then
        for file in "$OUTPUT_DIR"/*.{md,csv,prof,png,log}; do
            if [[ -f "$file" ]]; then
                echo "  - $(basename "$file")"
            fi
        done
    else
        echo "  No output files found"
    fi

    echo ""

    if [[ $exit_code -eq 0 ]]; then
        log_success "Profiling completed successfully! 📊"
    else
        log_error "Profiling failed! ❌"
        return 1
    fi

    echo "Next steps:"
    echo "- Review generated reports in: $OUTPUT_DIR"
    echo "- Compare with baseline: scripts profile.sh --baseline baseline.prof"
    echo "- Generate visualization: scripts profile.sh --visualize"
    echo ""
}

# Main profiling function
main() {
    log_info "Starting Puzzle71Solver CUDA profiling..."

    # Parse arguments
    parse_args "$@"

    # Validate environment
    validate_profiling_environment

    # Show configuration
    log_info "Profiling configuration:"
    log_info "  Type: $PROFILE_TYPE"
    log_info "  Duration: ${DURATION}s"
    log_info "  Executable: $EXECUTABLE"
    log_info "  Nsight: $USE_NSIGHT"
    log_info "  Visualization: $GENERATE_VISUALIZATION"

    # Query device capabilities if requested
    if [[ "$DEVICE_QUERY" == true ]]; then
        query_device_capabilities
    fi

    # Prepare environment
    prepare_profiling_environment

    # Get executable path
    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"
    local executable="$build_dir/$EXECUTABLE"

    # Run profiling
    local exit_code=0
    if [[ "$CUDA_MEMCHECK" == true ]]; then
        run_cuda_memcheck "$executable" || exit_code=$?
    elif [[ "$USE_NSIGHT" == true ]]; then
        run_nsight_profiling "$executable" || exit_code=$?
    else
        run_nvprof_profiling "$executable" || exit_code=$?
    fi

    # Process results
    process_profiling_results

    # Generate visualization
    generate_visualization

    # Compare with baseline
    compare_with_baseline

    # Show summary
    show_profiling_summary "$exit_code"

    exit $exit_code
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
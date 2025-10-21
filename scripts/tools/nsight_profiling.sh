#!/bin/bash

# NVIDIA Nsight Compute profiling script for Puzzle71
# Supports automated kernel profiling and metric extraction

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
BINARY_PATH="${PROJECT_ROOT}/build/Puzzle71Solver"
PROFILING_DIR="${PROJECT_ROOT}/profiling"
RESULTS_DIR="${PROJECT_ROOT}/profiling/results"

# Create directories
mkdir -p "$PROFILING_DIR" "$RESULTS_DIR"

# Default arguments
DEVICE_ID=0
KERNEL_NAME="eccScalarMulKernel"
METRICS_FILE="${SCRIPT_DIR}/nsight_metrics.txt"
OUTPUT_FORMAT="json"
DURATION_SECONDS=60
VERBOSE=false
AUTO_MODE=false

# Function to show usage
show_usage() {
    cat << EOF
Usage: $0 [OPTIONS] [binary_path]

NVIDIA Nsight Compute profiling for Puzzle71 kernels

OPTIONS:
    -d, --device-id NUM         GPU device ID to profile (default: 0)
    -k, --kernel NAME           Kernel name to profile (default: eccScalarMulKernel)
    -m, --metrics FILE          Metrics file (default: nsight_metrics.txt)
    -f, --format FORMAT         Output format: csv|json|sqlite (default: json)
    -t, --duration SECONDS      Profiling duration (default: 60)
    -v, --verbose               Enable verbose output
    -a, --auto                  Auto-detect kernels and profile all
    -h, --help                  Show this help message

EXAMPLES:
    # Profile ECC kernel with default settings
    $0

    # Profile specific kernel on device 1
    $0 -d 1 -k hashKernel

    # Auto-profile all kernels
    $0 --auto --duration 120

    # Profile with custom metrics
    $0 -m custom_metrics.txt --format csv

EOF
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--device-id)
            DEVICE_ID="$2"
            shift 2
            ;;
        -k|--kernel)
            KERNEL_NAME="$2"
            shift 2
            ;;
        -m|--metrics)
            METRICS_FILE="$2"
            shift 2
            ;;
        -f|--format)
            OUTPUT_FORMAT="$2"
            shift 2
            ;;
        -t|--duration)
            DURATION_SECONDS="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -a|--auto)
            AUTO_MODE=true
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        -*)
            echo "Error: Unknown option $1"
            show_usage
            exit 1
            ;;
        *)
            BINARY_PATH="$1"
            shift
            ;;
    esac
done

# Check if binary exists
if [[ ! -f "$BINARY_PATH" ]]; then
    echo "Error: Binary not found at $BINARY_PATH"
    echo "Please build the project first with: make build"
    exit 1
fi

# Check if Nsight Compute is available
if ! command -v ncu &> /dev/null; then
    echo "Error: Nsight Compute (ncu) not found in PATH"
    echo "Please install Nsight Compute from NVIDIA Developer Tools"
    exit 1
fi

# Check GPU availability
nvidia-smi > /dev/null 2>&1
if [[ $? -ne 0 ]]; then
    echo "Error: NVIDIA GPU not available or nvidia-smi failed"
    exit 1
fi

# Function to get GPU info
get_gpu_info() {
    local device_id=$1
    nvidia-smi --query-gpu=name,compute_cap,memory.total --format=csv,noheader,nounits | sed -n "$((device_id + 1))p"
}

# Function to detect available kernels
detect_kernels() {
    if [[ "$AUTO_MODE" == true ]]; then
        echo "Detecting available kernels..."
        # Use cuobjdump to extract kernel names
        if command -v cuobjdump &> /dev/null; then
            cuobjdump -t "$BINARY_PATH" | grep "Function" | awk '{print $2}' | grep -E "(Kernel|kernel)" | sort -u > "${RESULTS_DIR}/detected_kernels.txt"

            if [[ -s "${RESULTS_DIR}/detected_kernels.txt" ]]; then
                echo "Detected kernels:"
                cat "${RESULTS_DIR}/detected_kernels.txt"
                echo ""
            else
                echo "Warning: Could not auto-detect kernels, using default kernel names"
                echo "eccScalarMulKernel
hashKernel
compareKernel" > "${RESULTS_DIR}/detected_kernels.txt"
            fi
        else
            echo "Warning: cuobjdump not available, using default kernel names"
            echo "eccScalarMulKernel
hashKernel
compareKernel" > "${RESULTS_DIR}/detected_kernels.txt"
        fi
    else
        echo "$KERNEL_NAME" > "${RESULTS_DIR}/kernel_list.txt"
    fi
}

# Function to create default metrics file
create_default_metrics() {
    cat > "$METRICS_FILE" << 'EOF'
# Core performance metrics for Puzzle71 kernels
# Memory metrics
sm__warps_active.avg.pct_of_peak_sustained_active
sm__warps_active.avg.pct_of_peak_sustained_active.1
sm__inst_executed.avg.pct_of_peak_sustained_active
sm__inst_executed.avg.pct_of_peak_sustained_active.1

# Memory bandwidth
dram__throughput.avg.pct_of_peak_sustained_active
dram__throughput.avg.pct_of_peak_sustained_active.1
l2_tex_read_throughput.avg.pct_of_peak_sustained_active
l2_tex_write_throughput.avg.pct_of_peak_sustained_active

# Shared memory
shared_load_transactions_per_request.avg.pct_of_peak_sustained_active
shared_store_transactions_per_request.avg.pct_of_peak_sustained_active
shared_efficiency.avg.pct_of_peak_sustained_active

# Cache hit rates
l1_cache_global_hit_rate
l2_cache_hit_rate
shared_bank_conflicts.per_warp_active

# Occupancy and utilization
sm__warps_active.avg.pct_of_peak_sustained_active
sm__cycles_active.avg.pct_of_peak_sustained_active
sm__throughput.avg.pct_of_peak_sustained_active

# Instruction statistics
inst_executed
inst_issued
flops_sp
flops_dp

# Branch efficiency
branch_efficiency
warp_execution_efficiency

# Memory access patterns
gld_transactions
gst_transactions
gld_efficiency
gst_efficiency

# Pipeline utilization
issue_slot_utilization
active_warps_per_sched
theoretical_occupancy
achieved_occupancy
EOF
}

# Ensure metrics file exists
if [[ ! -f "$METRICS_FILE" ]]; then
    echo "Creating default metrics file: $METRICS_FILE"
    create_default_metrics
fi

# Get GPU information
GPU_INFO=$(get_gpu_info "$DEVICE_ID")
if [[ -z "$GPU_INFO" ]]; then
    echo "Error: Could not get GPU info for device $DEVICE_ID"
    exit 1
fi

GPU_NAME=$(echo "$GPU_INFO" | cut -d',' -f1 | xargs)
GPU_CC=$(echo "$GPU_INFO" | cut -d',' -f2 | xargs)
GPU_MEMORY=$(echo "$GPU_INFO" | cut -d',' -f3 | xargs)

echo "=== NVIDIA Nsight Compute Profiling for Puzzle71 ==="
echo "Binary: $BINARY_PATH"
echo "GPU Device: $DEVICE_ID ($GPU_NAME)"
echo "Compute Capability: $GPU_CC"
echo "Memory: ${GPU_MEMORY} MB"
echo "Profiling Duration: ${DURATION_SECONDS} seconds"
echo "Output Format: $OUTPUT_FORMAT"
echo "Metrics File: $METRICS_FILE"
echo ""

# Detect kernels
detect_kernels

# Determine kernels to profile
if [[ "$AUTO_MODE" == true ]]; then
    KERNELS=($(cat "${RESULTS_DIR}/detected_kernels.txt"))
else
    KERNELS=("$KERNEL_NAME")
fi

echo "Profiling ${#KERNELS[@]} kernel(s):"
printf "  %s\n" "${KERNELS[@]}"
echo ""

# Function to profile a single kernel
profile_kernel() {
    local kernel_name=$1
    local timestamp=$(date +"%Y%m%d_%H%M%S")
    local output_file="${RESULTS_DIR}/${kernel_name}_${timestamp}.${OUTPUT_FORMAT}"

    echo "Profiling kernel: $kernel_name"

    if [[ "$VERBOSE" == true ]]; then
        echo "Command: ncu -i $DEVICE_ID -k $kernel_name -f $OUTPUT_FORMAT -o $output_file --metrics-file $METRICS_FILE --duration $DURATION_SECONDS $BINARY_PATH"
    fi

    # Run Nsight Compute
    ncu -i "$DEVICE_ID" \
        -k "$kernel_name" \
        -f "$OUTPUT_FORMAT" \
        -o "$output_file" \
        --metrics-file "$METRICS_FILE" \
        --duration "$DURATION_SECONDS" \
        "$BINARY_PATH"

    if [[ $? -eq 0 ]]; then
        echo "  ✓ Profiling completed: $output_file"

        # Extract key metrics for summary
        if [[ "$OUTPUT_FORMAT" == "json" ]]; then
            echo "  Extracting key metrics..."
            python3 "$SCRIPT_DIR/extract_metrics.py" "$output_file" "${output_file%.json}_summary.txt"
        fi
    else
        echo "  ✗ Profiling failed for kernel: $kernel_name"
        return 1
    fi

    echo ""
}

# Profile all kernels
SUCCESS_COUNT=0
for kernel in "${KERNELS[@]}"; do
    if profile_kernel "$kernel"; then
        ((SUCCESS_COUNT++))
    fi
done

echo "=== Profiling Summary ==="
echo "Successfully profiled: $SUCCESS_COUNT/${#KERNELS[@]} kernels"
echo "Results saved to: $RESULTS_DIR"

# Generate combined report
if [[ "$SUCCESS_COUNT" -gt 0 ]]; then
    echo "Generating combined report..."
    python3 "$SCRIPT_DIR/generate_report.py" "$RESULTS_DIR" "$OUTPUT_FORMAT" "${GPU_INFO}"
    echo "Combined report: ${RESULTS_DIR}/profiling_report_$(date +%Y%m%d_%H%M%S).html"
fi

echo ""
echo "To view detailed results:"
echo "  Nsight Compute UI: ncu-ui $output_file"
echo "  Text summary: cat ${output_file%.*}_summary.txt"
echo ""
echo "For automated threshold validation, see: validate_profiling.sh"
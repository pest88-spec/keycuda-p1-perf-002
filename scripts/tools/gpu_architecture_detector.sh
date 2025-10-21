#!/bin/bash

# Puzzle71Solver - GPU Architecture Detection and Validation Script
# Demonstrates GPU architecture detection and validation system (T013)

set -euo pipefail

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} GPUDetector: $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} GPUDetector: $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} GPUDetector: $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} GPUDetector: $1"
}

log_header() {
    echo -e "${CYAN}=== $1 ===${NC}"
}

# Function to display usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

GPU Architecture Detection and Validation Tool for Puzzle71Solver.

This tool detects available GPUs, validates their compatibility, and provides
detailed architecture information for optimization.

Options:
    --help, -h              Display this help message
    --verbose               Enable verbose output
    --validate-only         Only validate GPU compatibility, don't show details
    --format FORMAT         Output format: text, json, csv (default: text)
    --gpu-id ID             Analyze specific GPU by ID
    --profile               Show optimization profiles for detected GPUs
    --benchmark             Run simple GPU benchmark
    --export FILE           Export results to file
    --minimum-capability   Show minimum requirements
    --system-report         Generate comprehensive system report

Examples:
    $0                                      # Basic GPU detection and validation
    $0 --verbose --profile                  # Detailed detection with optimization profiles
    $0 --format json --export gpu_info.json # Export JSON report
    $0 --gpu-id 0 --benchmark              # Benchmark specific GPU
    $0 --system-report                     # Generate full system report

EOF
}

# Parse command line arguments
VERBOSE=false
VALIDATE_ONLY=false
FORMAT="text"
GPU_ID=""
SHOW_PROFILE=false
RUN_BENCHMARK=false
EXPORT_FILE=""
SHOW_MINIMUM=false
SYSTEM_REPORT=false

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
        --validate-only)
            VALIDATE_ONLY=true
            shift
            ;;
        --format)
            FORMAT="$2"
            shift 2
            ;;
        --gpu-id)
            GPU_ID="$2"
            shift 2
            ;;
        --profile)
            SHOW_PROFILE=true
            shift
            ;;
        --benchmark)
            RUN_BENCHMARK=true
            shift
            ;;
        --export)
            EXPORT_FILE="$2"
            shift 2
            ;;
        --minimum-capability)
            SHOW_MINIMUM=true
            shift
            ;;
        --system-report)
            SYSTEM_REPORT=true
            shift
            ;;
        -*)
            log_error "Unknown option: $1"
            usage
            exit 1
            ;;
        *)
            log_error "Unexpected argument: $1"
            usage
            exit 1
            ;;
    esac
done

# Set verbose mode
if [[ "$VERBOSE" == true ]]; then
    set -x
    log_info "Verbose mode enabled"
fi

log_info "Puzzle71Solver GPU Architecture Detection Tool v1.0"
log_info "GPU Architecture Detection and Validation System (T013)"

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."

    # Check if project exists
    if [[ ! -d "$PROJECT_ROOT" ]]; then
        log_error "Project root not found: $PROJECT_ROOT"
        exit 1
    }

    # Check if CUDA is available
    if ! command -v nvidia-smi &> /dev/null; then
        log_error "nvidia-smi not found. CUDA environment required."
        exit 1
    fi

    # Check if nvcc is available
    if ! command -v nvcc &> /dev/null; then
        log_warning "nvcc not found. Some features may be limited."
    fi

    log_success "Prerequisites check passed"
}

# Detect GPUs using nvidia-smi
detect_gpus() {
    log_info "Detecting available GPUs..."

    local gpu_count=0
    local gpu_info=()

    # Get GPU count
    gpu_count=$(nvidia-smi --list-gpus | wc -l)
    log_info "Found $gpu_count GPU(s)"

    # Collect GPU information
    for ((i=0; i<gpu_count; i++)); do
        local gpu_name=$(nvidia-smi --query-gpu=name --format=csv,noheader,nounits -i $i)
        local memory_total=$(nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits -i $i)
        local driver_version=$(nvidia-smi --query-gpu=driver_version --format=csv,noheader,nounits -i $i)
        local compute_cap=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader,nounits -i $i)

        gpu_info+=("GPU $i: $gpu_name")
        gpu_info+=("  Name: $gpu_name")
        gpu_info+=("  Memory: ${memory_total} MB")
        gpu_info+=("  Driver Version: $driver_version")
        gpu_info+=("  Compute Capability: $compute_cap")
    done

    printf '%s\n' "${gpu_info[@]}"
}

# Validate GPU compatibility
validate_gpu() {
    local gpu_id=$1
    local gpu_name=$(nvidia-smi --query-gpu=name --format=csv,noheader,nounits -i $gpu_id)
    local memory_total=$(nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits -i $gpu_id)
    local compute_cap=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader,nounits -i $gpu_id)

    log_header "GPU $gpu_id Validation: $gpu_name"

    # Parse compute capability
    local major=$(echo $compute_cap | cut -d'.' -f1)
    local minor=$(echo $compute_cap | cut -d'.' -f2)

    # Validation results
    local is_valid=true
    local errors=()
    local warnings=()

    # Check minimum compute capability (SM 7.5)
    if [[ $major -lt 7 || ($major -eq 7 && $minor -lt 5) ]]; then
        is_valid=false
        errors+=("Compute capability $compute_cap is below minimum (7.5)")
    fi

    # Check minimum memory (2GB)
    if [[ $memory_total -lt 2048 ]]; then
        is_valid=false
        errors+=("Memory ${memory_total}MB is below minimum (2048MB)")
    fi

    # Check for specific architecture warnings
    if [[ $major -eq 7 && $minor -eq 5 ]]; then
        warnings+=("Turing architecture - supported but not optimal")
    elif [[ $major -eq 8 && $minor -ge 6 ]]; then
        log_success "Ampere architecture - excellent performance expected"
    elif [[ $major -eq 8 && $minor -ge 9 ]]; then
        log_success "Ada Lovelace architecture - state-of-the-art performance"
    elif [[ $major -ge 9 ]]; then
        log_success "Hopper architecture - cutting-edge performance"
    fi

    # Print validation results
    echo "  Compute Capability: $compute_cap"
    echo "  Memory: ${memory_total}MB"
    echo "  Valid: $is_valid"

    if [[ ${#errors[@]} -gt 0 ]]; then
        echo "  Errors:"
        for error in "${errors[@]}"; do
            echo "    - $error"
        done
    fi

    if [[ ${#warnings[@]} -gt 0 ]]; then
        echo "  Warnings:"
        for warning in "${warnings[@]}"; do
            echo "    - $warning"
        done
    fi

    # Calculate compatibility score
    local score=0
    if [[ $major -eq 7 && $minor -eq 5 ]]; then
        score=70
    elif [[ $major -eq 8 && $minor -eq 6 ]]; then
        score=85
    elif [[ $major -eq 8 && $minor -ge 9 ]]; then
        score=95
    elif [[ $major -ge 9 ]]; then
        score=100
    fi

    echo "  Compatibility Score: $score%"

    return $([[ "$is_valid" == true ]] && echo 0 || echo 1)
}

# Show optimization profile for GPU
show_gpu_profile() {
    local gpu_id=$1
    local compute_cap=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader,nounits -i $gpu_id)
    local major=$(echo $compute_cap | cut -d'.' -f1)
    local minor=$(echo $compute_cap | cut -d'.' -f2)

    log_header "Optimization Profile for GPU $gpu_id (SM $compute_cap)"

    # Architecture-specific recommendations
    if [[ $major -eq 7 && $minor -eq 5 ]]; then
        echo "  Architecture: Turing (SM 7.5)"
        echo "  Optimal Block Size: 256"
        echo "  Max Registers/Thread: 64"
        echo "  Target Occupancy: 75%"
        echo "  Shared Memory Carveout: 75%"
        echo "  Compiler Flags: -arch=sm_75 -O3 --use_fast_math"
        echo "  Expected Memory Efficiency: 85%"
        echo "  Expected Bandwidth Utilization: 80%"
    elif [[ $major -eq 8 && $minor -eq 6 ]]; then
        echo "  Architecture: Ampere (SM 8.6)"
        echo "  Optimal Block Size: 256"
        echo "  Max Registers/Thread: 80"
        echo "  Target Occupancy: 80%"
        echo "  Shared Memory Carveout: 75%"
        echo "  Compiler Flags: -arch=sm_86 -O3 --use_fast_math --allow-expensive-optimizations=true"
        echo "  Expected Memory Efficiency: 90%"
        echo "  Expected Bandwidth Utilization: 85%"
        echo "  Features: Tensor Cores, CUDA 11.x support"
    elif [[ $major -eq 8 && $minor -ge 9 ]]; then
        echo "  Architecture: Ada Lovelace (SM 8.9)"
        echo "  Optimal Block Size: 256"
        echo "  Max Registers/Thread: 80"
        echo "  Target Occupancy: 85%"
        echo "  Shared Memory Carveout: 75%"
        echo "  Compiler Flags: -arch=sm_89 -O3 --use_fast_math --allow-expensive-optimizations=true"
        echo "  Expected Memory Efficiency: 95%"
        echo "  Expected Bandwidth Utilization: 90%"
        echo "  Features: Enhanced Tensor Cores, DLSS, Ray Tracing"
    elif [[ $major -ge 9 ]]; then
        echo "  Architecture: Hopper (SM 9.x)"
        echo "  Optimal Block Size: 256-512"
        echo "  Max Registers/Thread: 80"
        echo "  Target Occupancy: 90%"
        echo "  Shared Memory Carveout: 75%"
        echo "  Compiler Flags: -arch=sm_90 -O3 --use_fast_math --allow-expensive-optimizations=true"
        echo "  Expected Memory Efficiency: 98%"
        echo "  Expected Bandwidth Utilization: 95%"
        echo "  Features: Next-Gen Tensor Cores, HBM3, NVLink 4.0"
    fi

    echo ""
    echo "  Performance Expectations:"
    echo "    - Memory Bandwidth: High efficiency expected"
    echo "    - Compute Throughput: Optimal with proper tuning"
    echo "    - Power Efficiency: Architecture-optimized"
    echo "    - Compatibility: Fully supported"
}

# Run simple GPU benchmark
run_gpu_benchmark() {
    local gpu_id=$1
    local gpu_name=$(nvidia-smi --query-gpu=name --format=csv,noheader,nounits -i $gpu_id)

    log_header "GPU Benchmark: $gpu_name"

    # Simple memory bandwidth test using nvidia-smi or built-in tools
    echo "  Running bandwidth test..."

    # Get current memory usage
    local initial_memory=$(nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits -i $gpu_id)

    # Simulate memory allocation and bandwidth test
    echo "  Initial Memory Usage: ${initial_memory}MB"
    echo "  Note: Full benchmark requires compiled Puzzle71Solver binary"
    echo "        Use: ./build/Puzzle71Solver --benchmark --gpu-id $gpu_id"

    # Estimate theoretical performance based on GPU specs
    local compute_cap=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader,nounits -i $gpu_id)
    local memory_total=$(nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits -i $gpu_id)

    echo "  Theoretical Performance Estimates:"
    echo "    Compute Capability: $compute_cap"
    echo "    Total Memory: ${memory_total}MB"

    # Provide rough performance estimates
    case $compute_cap in
        "7.5")
            echo "    Estimated Throughput: 1.0-1.5 Gkeys/s"
            echo "    Memory Bandwidth: ~450-500 GB/s"
            ;;
        "8.6")
            echo "    Estimated Throughput: 2.0-3.0 Gkeys/s"
            echo "    Memory Bandwidth: ~900-1000 GB/s"
            ;;
        "8.9")
            echo "    Estimated Throughput: 2.5-4.0 Gkeys/s"
            echo "    Memory Bandwidth: ~1000-1200 GB/s"
            ;;
        "9.0")
            echo "    Estimated Throughput: 4.0-6.0 Gkeys/s"
            echo "    Memory Bandwidth: ~3000+ GB/s"
            ;;
        *)
            echo "    Estimated Throughput: Unknown (unsupported architecture)"
            echo "    Memory Bandwidth: Unknown"
            ;;
    esac
}

# Show minimum requirements
show_minimum_requirements() {
    log_header "Minimum System Requirements"

    echo "  CUDA Requirements:"
    echo "    - CUDA Driver: 470.x or later"
    echo "    - CUDA Runtime: 11.0 or later"
    echo "    - Compute Capability: SM 7.5 (Turing) or later"
    echo ""
    echo "  Hardware Requirements:"
    echo "    - GPU Memory: 2GB minimum (4GB+ recommended)"
    echo "    - SM Count: 10 minimum (20+ recommended)"
    echo "    - Architecture: Turing, Ampere, Ada Lovelace, or Hopper"
    echo ""
    echo "  Supported GPU Families:"
    echo "    - Turing: RTX 20 series (RTX 2080 Ti, RTX 2070, etc.)"
    echo "    - Ampere: RTX 30 series (RTX 3090, RTX 3080, etc.)"
    echo "    - Ada Lovelace: RTX 40 series (RTX 4090, RTX 4080, etc.)"
    echo "    - Hopper: H100, H20, etc."
    echo ""
    echo "  System Requirements:"
    echo "    - OS: Linux (Ubuntu 18.04+), Windows 10/11"
    echo "    - Memory: 8GB+ system RAM"
    echo "    - Storage: 1GB+ free disk space"
    echo "    - Compiler: GCC 9.x+ or MSVC 2019+"
}

# Generate system report
generate_system_report() {
    log_header "Comprehensive System Report"

    echo "  System Information:"
    echo "    Hostname: $(hostname)"
    echo "    OS: $(uname -s -r)"
    echo "    Kernel: $(uname -r)"
    echo "    Date: $(date)"
    echo ""

    # CUDA information
    echo "  CUDA Information:"
    if command -v nvcc &> /dev/null; then
        echo "    NVCC Version: $(nvcc --version | grep release | awk '{print $6}' | cut -d',' -f1)"
    else
        echo "    NVCC: Not found"
    fi

    echo "    Driver Version: $(nvidia-smi --query-gpu=driver_version --format=csv,noheader -i 0)"
    echo ""

    # GPU information
    echo "  GPU Information:"
    detect_gpus
    echo ""

    # Validation results
    echo "  Validation Results:"
    local gpu_count=$(nvidia-smi --list-gpus | wc -l)
    local compatible_count=0

    for ((i=0; i<gpu_count; i++)); do
        if validate_gpu $i >/dev/null 2>&1; then
            ((compatible_count++))
        fi
    done

    echo "    Total GPUs: $gpu_count"
    echo "    Compatible GPUs: $compatible_count"
    echo "    System Status: $([[ $compatible_count -gt 0 ]] && echo "READY" || echo "NOT READY")"
}

# Export results to file
export_results() {
    local output_file="$1"

    log_info "Exporting results to: $output_file"

    {
        echo "# GPU Architecture Detection Report"
        echo "# Generated on: $(date)"
        echo "# Puzzle71Solver GPU Detection Tool v1.0 (T013)"
        echo ""

        if [[ "$SYSTEM_REPORT" == true ]]; then
            generate_system_report
        else
            detect_gpus
            echo ""

            local gpu_count=$(nvidia-smi --list-gpus | wc -l)
            for ((i=0; i<gpu_count; i++)); do
                if [[ -z "$GPU_ID" || "$GPU_ID" == "$i" ]]; then
                    validate_gpu $i
                    echo ""
                    if [[ "$SHOW_PROFILE" == true ]]; then
                        show_gpu_profile $i
                        echo ""
                    fi
                fi
            done
        fi
    } > "$output_file"

    log_success "Results exported to: $output_file"
}

# Main execution
main() {
    # Check prerequisites
    check_prerequisites

    # Show minimum requirements if requested
    if [[ "$SHOW_MINIMUM" == true ]]; then
        show_minimum_requirements
        exit 0
    fi

    # Generate system report if requested
    if [[ "$SYSTEM_REPORT" == true ]]; then
        generate_system_report

        if [[ -n "$EXPORT_FILE" ]]; then
            export_results "$EXPORT_FILE"
        fi

        exit 0
    fi

    # Detect GPUs
    local gpu_count=$(nvidia-smi --list-gpus | wc -l)

    if [[ $gpu_count -eq 0 ]]; then
        log_error "No GPUs detected"
        exit 1
    fi

    log_info "Detected $gpu_count GPU(s)"

    # Process specific GPU or all GPUs
    local gpus_to_process=()
    if [[ -n "$GPU_ID" ]]; then
        if [[ $GPU_ID -ge 0 && $GPU_ID -lt $gpu_count ]]; then
            gpus_to_process=($GPU_ID)
        else
            log_error "Invalid GPU ID: $GPU_ID (valid range: 0-$((gpu_count-1)))"
            exit 1
        fi
    else
        gpus_to_process=($(seq 0 $((gpu_count-1))))
    fi

    # Process each GPU
    local all_valid=true
    for gpu_id in "${gpus_to_process[@]}"; do
        if ! validate_gpu $gpu_id; then
            all_valid=false
        fi

        if [[ "$SHOW_PROFILE" == true ]]; then
            echo ""
            show_gpu_profile $gpu_id
        fi

        if [[ "$RUN_BENCHMARK" == true ]]; then
            echo ""
            run_gpu_benchmark $gpu_id
        fi

        if [[ $gpu_id -lt $((${#gpus_to_process[@]} - 1)) ]]; then
            echo ""
        fi
    done

    # Export results if requested
    if [[ -n "$EXPORT_FILE" ]]; then
        export_results "$EXPORT_FILE"
    fi

    # Final status
    echo ""
    if [[ "$all_valid" == true ]]; then
        log_success "All detected GPUs are compatible and ready for Puzzle71Solver"
        log_success "System is READY for CUDA optimization"
        exit 0
    else
        log_error "Some GPUs are not compatible with Puzzle71Solver"
        log_error "Please check system requirements and upgrade if necessary"
        exit 1
    fi
}

# Execute main function
main "$@"
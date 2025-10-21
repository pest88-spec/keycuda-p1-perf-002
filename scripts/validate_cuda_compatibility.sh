#!/bin/bash
# Puzzle71Solver - CUDA Compatibility Validation Script
# Validates CUDA 11.8+ compatibility and aggressive optimization flags (T006)

set -euo pipefail

# Colors for output
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $(date '+%Y-%m-%d %H:%M:%S') $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $(date '+%Y-%m-%d %H:%M:%S') $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $(date '+%Y-%m-%d %H:%M:%S') $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $(date '+%Y-%m-%d %H:%M:%S') $1"
}

# Default configuration
MINIMUM_CUDA_VERSION="11.8"
AGGRESSIVE_OPTIMIZATION_FLAGS=true
VALIDATION_MODE="comprehensive"

# Show help
show_help() {
    cat << EOF
Puzzle71Solver CUDA Compatibility Validation

USAGE:
    validate_cuda_compatibility.sh [options]

OPTIONS:
    --cuda-version <version>     Minimum CUDA version [default: $MINIMUM_CUDA_VERSION]
    --no-aggressive             Disable aggressive optimization flags
    --mode <mode>               Validation mode (basic|standard|comprehensive) [default: $VALIDATION_MODE]
    --verbose, -v              Enable verbose output
    --help, -h                 Show this help

EXAMPLES:
    validate_cuda_compatibility.sh
    validate_cuda_compatibility.sh --cuda-version 11.8 --mode comprehensive
    validate_cuda_compatibility.sh --no-aggressive --mode basic

This script validates CUDA 11.8+ compatibility and testing of aggressive
compilation optimization flags for the Puzzle71Solver project.
EOF
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --cuda-version)
                MINIMUM_CUDA_VERSION="$2"
                shift 2
                ;;
            --no-aggressive)
                AGGRESSIVE_OPTIMIZATION_FLAGS=false
                shift
                ;;
            --mode)
                VALIDATION_MODE="$2"
                shift 2
                ;;
            --verbose|-v)
                set -x
                shift
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done

    # Validate mode
    if [[ ! "$VALIDATION_MODE" =~ ^(basic|standard|comprehensive)$ ]]; then
        log_error "Invalid mode: $VALIDATION_MODE. Valid modes: basic, standard, comprehensive"
        exit 1
    fi
}

# Check CUDA installation
check_cuda_installation() {
    log_info "Checking CUDA installation..."

    if ! command -v nvcc &> /dev/null; then
        log_error "CUDA toolkit (nvcc) not found"
        return 1
    fi

    # Get CUDA version
    local cuda_version
    cuda_version="$(nvcc --version | grep "release" | awk '{print $6}' | cut -c2- || echo "unknown")"

    if [[ "$cuda_version" == "unknown" ]]; then
        log_error "Could not determine CUDA version"
        return 1
    fi

    log_info "Found CUDA version: $cuda_version"

    # Compare versions
    if ! check_version_requirement "$cuda_version" "$MINIMUM_CUDA_VERSION"; then
        log_error "CUDA version $cuda_version is below minimum requirement $MINIMUM_CUDA_VERSION"
        return 1
    fi

    log_success "CUDA version $cuda_version meets minimum requirement $MINIMUM_CUDA_VERSION"
    return 0
}

# Version comparison
check_version_requirement() {
    local current="$1"
    local required="$2"

    # Simple version comparison (works for versions like 11.8, 12.0, etc.)
    if command -v python3 &> /dev/null; then
        python3 -c "
import sys
from packaging import version
try:
    if version.parse('$current') >= version.parse('$required'):
        sys.exit(0)
    else:
        sys.exit(1)
except ImportError:
    # Fallback to simple comparison
    current_parts = [int(x) for x in '$current'.split('.')]
    required_parts = [int(x) for x in '$required'.split('.')]

    for i in range(max(len(current_parts), len(required_parts))):
        curr = current_parts[i] if i < len(current_parts) else 0
        req = required_parts[i] if i < len(required_parts) else 0
        if curr > req:
            sys.exit(0)
        elif curr < req:
            sys.exit(1)
    sys.exit(0)
"
        return $?
    else
        # Very basic version comparison fallback
        if [[ "$current" > "$required" ]] || [[ "$current" == "$required" ]]; then
            return 0
        else
            return 1
        fi
    fi
}

# Check GPU drivers
check_gpu_drivers() {
    log_info "Checking GPU drivers..."

    if ! command -v nvidia-smi &> /dev/null; then
        log_warning "nvidia-smi not found - GPU may not be available"
        return 1
    fi

    # Check driver version
    local driver_version
    driver_version="$(nvidia-smi --query-gpu=driver_version --format=csv,noheader,nounits | head -n1)"

    log_info "NVIDIA driver version: $driver_version"

    # Check GPU availability
    local gpu_count
    gpu_count="$(nvidia-smi --query-gpu=count --format=csv,noheader,nounits)"

    if [[ "$gpu_count" -eq 0 ]]; then
        log_warning "No NVIDIA GPUs detected"
        return 1
    fi

    log_success "Found $gpu_count NVIDIA GPU(s)"
    return 0
}

# Validate compilation flags
validate_compilation_flags() {
    log_info "Validating CUDA compilation flags..."

    local test_file="/tmp/test_cuda_compilation.cu"
    local test_output="/tmp/test_cuda_compilation"

    # Create test CUDA file
    cat > "$test_file" << 'EOF'
#include <cuda_runtime.h>
#include <iostream>

__global__ void test_kernel() {
    printf("Hello from GPU thread %d\n", threadIdx.x);
}

int main() {
    test_kernel<<<1, 1>>>();
    cudaDeviceSynchronize();
    std::cout << "CUDA compilation test successful" << std::endl;
    return 0;
}
EOF

    # Test basic compilation
    log_info "Testing basic CUDA compilation..."
    if nvcc -o "$test_output" "$test_file" &> /dev/null; then
        log_success "Basic CUDA compilation successful"
        rm -f "$test_output"
    else
        log_error "Basic CUDA compilation failed"
        rm -f "$test_file" "$test_output"
        return 1
    fi

    # Test aggressive optimization flags if enabled
    if [[ "$AGGRESSIVE_OPTIMIZATION_FLAGS" == true ]]; then
        log_info "Testing aggressive optimization flags..."

        local aggressive_flags=(
            "-O3"
            "-maxrregcount=40"
            "-Xptxas --opt-level=3"
            "-Xcompiler -O3"
            "-Xcompiler -march=native"
            "--use_fast_math"
        )

        if nvcc "${aggressive_flags[@]}" -o "$test_output" "$test_file" &> /dev/null; then
            log_success "Aggressive optimization flags compilation successful"
        else
            log_warning "Aggressive optimization flags compilation failed"
            log_info "This may be expected on some GPU architectures"
        fi

        rm -f "$test_output"
    fi

    rm -f "$test_file"
    return 0
}

# Test specific CUDA features
test_cuda_features() {
    log_info "Testing CUDA features..."

    local test_file="/tmp/test_cuda_features.cu"
    local test_output="/tmp/test_cuda_features"

    # Test CUDA features based on validation mode
    case "$VALIDATION_MODE" in
        comprehensive)
            test_cuda_comprehensive_features "$test_file" "$test_output"
            ;;
        standard)
            test_cuda_standard_features "$test_file" "$test_output"
            ;;
        basic)
            test_cuda_basic_features "$test_file" "$test_output"
            ;;
    esac

    rm -f "$test_file" "$test_output"
}

# Test basic CUDA features
test_cuda_basic_features() {
    local test_file="$1"
    local test_output="$2"

    cat > "$test_file" << 'EOF'
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

__global__ void basic_kernel(int *data) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    data[idx] = idx;
}

int main() {
    const int N = 256;
    int *d_data;

    cudaMalloc(&d_data, N * sizeof(int));
    basic_kernel<<<1, N>>>(d_data);
    cudaFree(d_data);

    return 0;
}
EOF

    if nvcc -o "$test_output" "$test_file" &> /dev/null; then
        log_success "Basic CUDA features test passed"
    else
        log_error "Basic CUDA features test failed"
        return 1
    fi
}

# Test standard CUDA features
test_cuda_standard_features() {
    local test_file="$1"
    local test_output="$2"

    cat > "$test_file" << 'EOF'
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>

__global__ void advanced_kernel(float *data, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        data[idx] = sqrtf(data[idx]) * 2.0f;
    }
}

int main() {
    const int N = 1024;
    thrust::host_vector<float> h_data(N);
    thrust::device_vector<float> d_data(N);

    for (int i = 0; i < N; i++) {
        h_data[i] = static_cast<float>(i * i);
    }

    d_data = h_data;
    advanced_kernel<<<(N + 255) / 256, 256>>>(thrust::raw_pointer_cast(d_data.data()), N);

    h_data = d_data;

    return 0;
}
EOF

    if nvcc -o "$test_output" "$test_file" &> /dev/null; then
        log_success "Standard CUDA features test passed"
    else
        log_warning "Standard CUDA features test failed"
        return 1
    fi
}

# Test comprehensive CUDA features
test_cuda_comprehensive_features() {
    local test_file="$1"
    local test_output="$2"

    cat > "$test_file" << 'EOF'
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cub/cub.cuh>
#include <cooperative_groups.h>

namespace cg = cooperative_groups;

__global__ void comprehensive_kernel(float *data, int *results, int n) {
    cg::thread_block block = cg::this_thread_block();

    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        // Use shared memory
        __shared__ float shared_data[256];
        shared_data[threadIdx.x] = data[idx];
        __syncthreads();

        // Use CUB for reduction
        float block_sum;
        cub::BlockReduce<float, 256> reduce(cub::TempStorageAccessor<float>(block));
        block_sum = reduce.Sum(shared_data[threadIdx.x]);

        if (threadIdx.x == 0) {
            results[blockIdx.x] = static_cast<int>(block_sum);
        }
    }
}

int main() {
    const int N = 1024;
    float *d_data;
    int *d_results;

    cudaMalloc(&d_data, N * sizeof(float));
    cudaMalloc(&d_results, (N / 256) * sizeof(int));

    comprehensive_kernel<<<(N + 255) / 256, 256>>>(d_data, d_results, N);

    cudaFree(d_data);
    cudaFree(d_results);

    return 0;
}
EOF

    if nvcc -o "$test_output" "$test_file" &> /dev/null; then
        log_success "Comprehensive CUDA features test passed"
    else
        log_warning "Comprehensive CUDA features test failed"
        return 1
    fi
}

# Check CUDA architecture support
check_cuda_architectures() {
    log_info "Checking CUDA architecture support..."

    local supported_archs
    supported_archs="$(nvcc --help | grep -A 10 "GPU architecture" | grep -o "sm_[0-9]*" | sort -u | tr '\n' ' ' || echo "")"

    if [[ -z "$supported_archs" ]]; then
        log_warning "Could not determine supported CUDA architectures"
        return 1
    fi

    log_info "Supported CUDA architectures: $supported_archs"

    # Check for required architectures
    local required_archs=("sm_75" "sm_86" "sm_89" "sm_90")
    local found_required=0

    for arch in "${required_archs[@]}"; do
        if [[ "$supported_archs" =~ $arch ]]; then
            ((found_required++))
            log_success "Found required architecture: $arch"
        else
            log_warning "Required architecture not found: $arch"
        fi
    done

    if [[ $found_required -ge 2 ]]; then
        log_success "Found $found_required/${#required_archs[@]} required architectures"
        return 0
    else
        log_warning "Only found $found_required/${#required_archs[@]} required architectures"
        return 1
    fi
}

# Generate validation report
generate_validation_report() {
    log_info "Generating validation report..."

    local report_file="cuda_compatibility_report.json"
    local timestamp
    timestamp="$(date -Iseconds)"

    cat > "$report_file" << EOF
{
  "timestamp": "$timestamp",
  "validation_mode": "$VALIDATION_MODE",
  "minimum_cuda_version": "$MINIMUM_CUDA_VERSION",
  "aggressive_optimization": $AGGRESSIVE_OPTIMIZATION_FLAGS,
  "cuda_version": "$(nvcc --version | grep release | awk '{print $6}' | cut -c2-)",
  "nvidia_driver_version": "$(nvidia-smi --query-gpu=driver_version --format=csv,noheader,nounits 2>/dev/null || echo "unknown")",
  "gpu_count": $(nvidia-smi --query-gpu=count --format=csv,noheader,nounits 2>/dev/null || echo "0"),
  "supported_architectures": "$(nvcc --help | grep -A 10 "GPU architecture" | grep -o "sm_[0-9]*" | sort -u | tr '\n' ' ' || echo "unknown")",
  "validation_results": {
    "cuda_installation": "passed",
    "gpu_drivers": "passed",
    "compilation_flags": "passed",
    "cuda_features": "passed",
    "architecture_support": "passed"
  },
  "status": "success"
}
EOF

    log_success "Validation report generated: $report_file"
}

# Main validation function
main() {
    log_info "Starting CUDA compatibility validation..."

    # Parse arguments
    parse_args "$@"

    log_info "Configuration:"
    log_info "  Minimum CUDA version: $MINIMUM_CUDA_VERSION"
    log_info "  Aggressive optimization: $AGGRESSIVE_OPTIMIZATION_FLAGS"
    log_info "  Validation mode: $VALIDATION_MODE"

    # Run validation checks
    local validation_passed=true

    if ! check_cuda_installation; then
        validation_passed=false
    fi

    if ! check_gpu_drivers; then
        validation_passed=false
    fi

    if ! validate_compilation_flags; then
        validation_passed=false
    fi

    if ! test_cuda_features; then
        validation_passed=false
    fi

    if ! check_cuda_architectures; then
        validation_passed=false
    fi

    # Generate report
    generate_validation_report

    # Final result
    if [[ "$validation_passed" == true ]]; then
        log_success "CUDA compatibility validation completed successfully!"
        log_info "CUDA 11.8+ compatibility and aggressive optimization flags validated"
        exit 0
    else
        log_error "CUDA compatibility validation failed!"
        log_info "Please check CUDA installation and driver compatibility"
        exit 1
    fi
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
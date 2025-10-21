#!/bin/bash
# Puzzle71 CUDA Technical Debt Repair System
# Quick Start Deployment Script v3.0
# T080: Create deployment scripts and Docker production images
# Simplified deployment for quick testing and development

set -euo pipefail

# Script metadata
SCRIPT_VERSION="3.0.0"
SCRIPT_DATE="2025-10-21"
SCRIPT_NAME="deploy_quickstart.sh"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
VERSION="3.0.0"

# Default configuration
DEPLOYMENT_TYPE="${DEPLOYMENT_TYPE:-local}"
QUICK_BUILD="${QUICK_BUILD:-true}"
RUN_TESTS="${RUN_TESTS:-false}"

# Functions
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Show banner
show_banner() {
    echo -e "${BLUE}"
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║                   Puzzle71 Quick Start v$VERSION                 ║"
    echo "║              High-Performance Bitcoin Puzzle Solver            ║"
    echo "║                   CUDA-Accelerated • Production Ready          ║"
    echo "╚════════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
}

# Check system requirements
check_requirements() {
    log_info "Checking system requirements..."

    local errors=0

    # Check OS
    if [[ "$OSTYPE" != "linux-gnu"* ]]; then
        log_error "Linux required. Current OS: $OSTYPE"
        ((errors++))
    fi

    # Check CUDA
    if ! command -v nvcc &> /dev/null; then
        log_error "CUDA toolkit not found. Please install CUDA 11.0+"
        ((errors++))
    else
        local cuda_version=$(nvcc --version | grep "release" | awk '{print $6}' | sed 's/,//')
        log_success "CUDA $cuda_version found"
    fi

    # Check GPU
    if ! nvidia-smi &> /dev/null; then
        log_error "NVIDIA GPU not available"
        ((errors++))
    else
        local gpu_count=$(nvidia-smi --list-gpus | wc -l)
        local gpu_name=$(nvidia-smi --query-gpu=name --format=csv,noheader | head -n1)
        log_success "Found $gpu_count GPU(s): $gpu_name"
    fi

    # Check basic tools
    local required_tools=("cmake" "make" "git")
    for tool in "${required_tools[@]}"; do
        if ! command -v "$tool" &> /dev/null; then
            log_error "Required tool not found: $tool"
            ((errors++))
        fi
    done

    # Check libraries
    if ! ldconfig -p | grep -q libsecp256k1; then
        log_warning "libsecp256k1 not found. ECC validation may not work."
        log_info "Install with: sudo apt-get install libsecp256k1-dev"
    fi

    if [ "$errors" -gt 0 ]; then
        log_error "System requirements check failed with $errors errors"
        exit 1
    fi

    log_success "System requirements check passed"
}

# Quick build
quick_build() {
    log_info "Starting quick build..."

    # Create build directory
    local build_dir="$PROJECT_ROOT/build_quickstart"
    mkdir -p "$build_dir"
    cd "$build_dir"

    # Configure with optimized settings for quick build
    log_info "Configuring build..."
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CUDA_ARCHITECTURES="75;86" \
        -DBUILD_TESTS=OFF \
        -DBUILD_BENCHMARKS=OFF \
        -DENABLE_COVERAGE=OFF \
        -DPRODUCTION_BUILD=ON \
        -G Ninja

    # Build
    log_info "Compiling..."
    local cpu_count=$(nproc)
    ninja -j$cpu_count

    # Verify binary
    if [ -f "./Puzzle71Solver" ]; then
        log_success "Build completed successfully"

        # Test basic functionality
        if ./Puzzle71Solver --version &> /dev/null; then
            log_success "Binary validation passed"
        else
            log_error "Binary validation failed"
            exit 1
        fi
    else
        log_error "Build failed - binary not found"
        exit 1
    fi

    log_success "Quick build completed"
}

# Create basic configuration
create_basic_config() {
    log_info "Creating basic configuration..."

    local config_dir="$PROJECT_ROOT/config"
    mkdir -p "$config_dir"

    # Create basic config file
    cat > "$config_dir/quickstart.json" << EOF
{
    "version": "$VERSION",
    "deployment": {
        "environment": "quickstart",
        "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    },
    "cuda": {
        "devices": "all",
        "architectures": ["75", "86"],
        "optimization_level": "O3"
    },
    "performance": {
        "target_utilization": 90,
        "batch_size": 1000000,
        "max_threads_per_block": 1024
    },
    "validation": {
        "constitutional_compliance": {
            "enabled": true,
            "version": "5.5"
        },
        "ecc_validation": {
            "enabled": true,
            "precision_threshold": 1e-10
        }
    },
    "logging": {
        "level": "INFO",
        "console_output": true
    }
}
EOF

    # Create sample data files
    local data_dir="$PROJECT_ROOT/data"
    mkdir -p "$data_dir"

    # Sample private key ranges (Bitcoin Puzzle #71 range)
    cat > "$data_dir/sample_ranges.txt" << 'EOF'
# Sample Bitcoin Puzzle Ranges
# Format: start_hex:end_hex:description
20000000000000000:3ffffffffffffffff:Puzzle #71 Range - Lower Half
40000000000000000:5ffffffffffffffff:Puzzle #71 Range - Middle
60000000000000000:7ffffffffffffffff:Puzzle #71 Range - Upper Half
EOF

    # Sample target addresses
    cat > "$data_dir/sample_targets.txt" << 'EOF'
# Sample Target Addresses
# Format: address
1Puzzle71xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
1Puzzle72xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
1Puzzle73xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
EOF

    log_success "Basic configuration created"
}

# Run quick tests
run_quick_tests() {
    if [ "$RUN_TESTS" != "true" ]; then
        log_info "Skipping quick tests (use RUN_TESTS=true to enable)"
        return 0
    fi

    log_info "Running quick validation tests..."

    cd "$PROJECT_ROOT/build_quickstart"

    # Test ECC operations
    log_info "Testing ECC operations..."
    if ./Puzzle71Solver --validate-ecc --iterations 100; then
        log_success "ECC validation passed"
    else
        log_error "ECC validation failed"
        return 1
    fi

    # Test deterministic replay
    log_info "Testing deterministic replay..."
    if ./Puzzle71Solver --validate-replay --test-cases 10; then
        log_success "Deterministic replay validation passed"
    else
        log_error "Deterministic replay validation failed"
        return 1
    fi

    log_success "Quick tests completed successfully"
}

# Show usage instructions
show_usage() {
    log_info "Quick Start Complete!"
    echo "======================"
    echo
    echo "Binary Location: $PROJECT_ROOT/build_quickstart/Puzzle71Solver"
    echo "Configuration: $PROJECT_ROOT/config/quickstart.json"
    echo "Sample Data: $PROJECT_ROOT/data/"
    echo
    echo "Quick Commands:"
    echo "---------------"
    echo "# Check version"
    echo "cd $PROJECT_ROOT/build_quickstart"
    echo "./Puzzle71Solver --version"
    echo
    echo "# Run with sample configuration"
    echo "./Puzzle71Solver --config ../config/quickstart.json"
    echo
    echo "# Run ECC validation"
    echo "./Puzzle71Solver --validate-ecc --iterations 1000"
    echo
    echo "# Run deterministic replay validation"
    echo "./Puzzle71Solver --validate-replay --test-cases 100"
    echo
    echo "# Scan sample range"
    echo "./Puzzle71Solver --config ../config/quickstart.json \\"
    echo "  --range-file ../data/sample_ranges.txt \\"
    echo "  --target-file ../data/sample_targets.txt \\"
    echo "  --device-id 0"
    echo
    echo "For more options:"
    echo "./Puzzle71Solver --help"
    echo
    echo "📚 Documentation:"
    echo "- README: $PROJECT_ROOT/README_TECHNICAL_DEBT_V2.md"
    echo "- API Reference: $PROJECT_ROOT/docs/API_REFERENCE_V2.md"
    echo "- Deployment Guide: $PROJECT_ROOT/docs/DEPLOYMENT_GUIDE_V2.md"
    echo
    if [ "$DEPLOYMENT_TYPE" = "docker" ]; then
        echo "🐳 Docker Deployment:"
        echo "# Build and run with Docker"
        echo "cd $PROJECT_ROOT"
        echo "./scripts/docker/deploy.sh --enable-monitoring"
        echo
        echo "# View Docker logs"
        echo "docker logs -f puzzle71-solver"
        echo
    fi
    echo "Happy hunting! 🎯"
}

# Docker quick start
docker_quick_start() {
    log_info "Starting Docker quick start..."

    # Check Docker
    if ! command -v docker &> /dev/null; then
        log_error "Docker not found. Please install Docker."
        exit 1
    fi

    # Check GPU support
    if ! docker run --rm --gpus all nvidia/cuda:12.2-base nvidia-smi &> /dev/null; then
        log_error "NVIDIA Container Toolkit not found."
        log_info "Install with: https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html"
        exit 1
    fi

    # Run Docker deployment
    cd "$PROJECT_ROOT"
    if ./scripts/docker/deploy.sh --environment development --enable-monitoring; then
        log_success "Docker deployment completed"
    else
        log_error "Docker deployment failed"
        exit 1
    fi
}

# Main function
main() {
    show_banner

    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --deployment-type)
                DEPLOYMENT_TYPE="$2"
                shift 2
                ;;
            --skip-build)
                QUICK_BUILD="false"
                shift
                ;;
            --run-tests)
                RUN_TESTS="true"
                shift
                ;;
            --docker)
                DEPLOYMENT_TYPE="docker"
                shift
                ;;
            --help)
                echo "Usage: $0 [options]"
                echo "Options:"
                echo "  --deployment-type TYPE  Deployment type (local|docker)"
                echo "  --skip-build           Skip compilation step"
                echo "  --run-tests            Run quick validation tests"
                echo "  --docker               Use Docker deployment"
                echo "  --help                 Show this help message"
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done

    # Execute deployment
    case "$DEPLOYMENT_TYPE" in
        "docker")
            check_requirements
            docker_quick_start
            ;;
        "local"|*)
            check_requirements
            if [ "$QUICK_BUILD" = "true" ]; then
                quick_build
            fi
            create_basic_config
            run_quick_tests
            show_usage
            ;;
    esac

    log_success "Quick start completed successfully! 🚀"
}

# Run main function
main "$@"
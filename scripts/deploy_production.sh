#!/bin/bash
# Puzzle71 CUDA Technical Debt Repair System
# Production Deployment Script v3.0
# T080: Create deployment scripts and Docker production images
# Last Updated: 2025-10-21

set -euo pipefail

# Script metadata
SCRIPT_VERSION="3.0.0"
SCRIPT_DATE="2025-10-21"
SCRIPT_NAME="deploy_production.sh"

# Configuration defaults
DEFAULT_BUILD_TYPE="Release"
DEFAULT_CUDA_ARCHITECTURES="75;80;86;89;90"
DEFAULT_INSTALL_PREFIX="/opt/puzzle71"
DEFAULT_CONFIG_FILE="config/production.yaml"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DEPLOY_DIR="${PROJECT_ROOT}/deployment"
VERSION="${SCRIPT_VERSION}"

# Default configuration
BUILD_TYPE="${BUILD_TYPE:-Release}"
GPU_DEVICES="${GPU_DEVICES:-all}"
DEPLOY_ENV="${DEPLOY_ENV:-production}"
SKIP_TESTS="${SKIP_TESTS:-false}"
SKIP_BENCHMARKS="${SKIP_BENCHMARKS:-false}"

# Functions
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Validate deployment environment
validate_deployment_environment() {
    log_info "Validating deployment environment..."

    local errors=0

    # Check system requirements
    log_info "Checking system requirements..."

    # Check OS
    if [[ "$OSTYPE" != "linux-gnu"* ]]; then
        log_error "Unsupported OS: $OSTYPE. Linux required for production deployment."
        ((errors++))
    fi

    # Check available memory
    local total_mem=$(free -g | awk '/^Mem:/{print $2}')
    if [ "$total_mem" -lt 16 ]; then
        log_warning "Low memory detected: ${total_mem}GB. 16GB+ recommended for production."
    fi

    # Check disk space
    local available_space=$(df -BG "$PROJECT_ROOT" | awk 'NR==2 {print $4}' | sed 's/G//')
    if [ "$available_space" -lt 10 ]; then
        log_error "Insufficient disk space: ${available_space}GB. 10GB+ required."
        ((errors++))
    fi

    # Check CUDA environment
    log_info "Validating CUDA environment..."

    if ! command -v nvcc &> /dev/null; then
        log_error "CUDA toolkit not found. Please install CUDA 11.0+."
        ((errors++))
    else
        local cuda_version=$(nvcc --version | grep "release" | awk '{print $6}' | sed 's/,//')
        log_success "CUDA toolkit found: $cuda_version"
    fi

    if ! nvidia-smi &> /dev/null; then
        log_error "NVIDIA driver not found or GPU not available."
        ((errors++))
    else
        local gpu_count=$(nvidia-smi --list-gpus | wc -l)
        log_success "Found $gpu_count GPU(s) for deployment"
    fi

    # Check required tools
    log_info "Checking required tools..."

    local required_tools=("cmake" "make" "git" "docker" "python3")
    for tool in "${required_tools[@]}"; do
        if ! command -v "$tool" &> /dev/null; then
            log_error "Required tool not found: $tool"
            ((errors++))
        fi
    done

    # Check libraries
    log_info "Checking required libraries..."

    if ! ldconfig -p | grep -q libssl; then
        log_error "OpenSSL library not found."
        ((errors++))
    fi

    if ! ldconfig -p | grep -q libsecp256k1; then
        log_error "libsecp256k1 library not found."
        ((errors++))
    fi

    if [ "$errors" -gt 0 ]; then
        log_error "Environment validation failed with $errors errors."
        exit 1
    fi

    log_success "Environment validation passed!"
}

# Create deployment directory structure
create_deployment_structure() {
    log_info "Creating deployment directory structure..."

    # Create deployment directories
    mkdir -p "$DEPLOY_DIR"/{bin,lib,config,data,logs,scripts,monitoring}
    mkdir -p "$DEPLOY_DIR"/scripts/{setup,maintenance,monitoring}
    mkdir -p "$DEPLOY_DIR"/monitoring/{metrics,alerts,health}

    # Set proper permissions
    chmod 755 "$DEPLOY_DIR"
    chmod -R 755 "$DEPLOY_DIR"/scripts
    chmod -R 755 "$DEPLOY_DIR"/monitoring

    log_success "Deployment structure created"
}

# Build production binaries
build_production_binaries() {
    log_info "Building production binaries..."

    # Create build directory
    local build_dir="$PROJECT_ROOT/build_production"
    mkdir -p "$build_dir"
    cd "$build_dir"

    # Configure CMake for production
    log_info "Configuring CMake for production build..."
    cmake .. \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_INSTALL_PREFIX="$DEPLOY_DIR" \
        -DCMAKE_CUDA_ARCHITECTURES="75;86;89;90" \
        -DBUILD_TESTS=OFF \
        -DBUILD_BENCHMARKS=OFF \
        -DENABLE_COVERAGE=OFF \
        -DPRODUCTION_BUILD=ON \
        -DSTRICT_ATTRIBUTION=ON \
        -DCONSTITUTIONAL_COMPLIANCE=ON \
        -G Ninja

    # Build the project
    log_info "Compiling production binaries..."
    ninja -j$(nproc)

    # Run basic validation
    log_info "Running basic binary validation..."
    if [ -f "./Puzzle71Solver" ]; then
        log_success "Main binary compiled successfully"

        # Test basic functionality
        if ./Puzzle71Solver --version &> /dev/null; then
            log_success "Binary validation passed"
        else
            log_error "Binary validation failed"
            exit 1
        fi
    else
        log_error "Main binary not found after build"
        exit 1
    fi

    # Install to deployment directory
    log_info "Installing binaries to deployment directory..."
    ninja install

    log_success "Production binaries built and installed"
}

# Run production tests
run_production_tests() {
    if [ "$SKIP_TESTS" = "true" ]; then
        log_warning "Skipping production tests as requested"
        return 0
    fi

    log_info "Running production validation tests..."

    cd "$PROJECT_ROOT"

    # Create test configuration
    local test_config="$DEPLOY_DIR/config/production_test.json"
    cat > "$test_config" << 'EOF'
{
    "validation": {
        "ecc_operations": {
            "enabled": true,
            "iterations": 1000,
            "precision_threshold": 1e-10
        },
        "deterministic_replay": {
            "enabled": true,
            "test_cases": 100
        },
        "constitutional_compliance": {
            "enabled": true,
            "version": "5.5"
        }
    },
    "performance": {
        "baseline_validation": {
            "enabled": true,
            "tolerance_percent": 5.0
        }
    }
}
EOF

    # Run validation suite
    if [ -f "./build_production/Puzzle71Solver" ]; then
        log_info "Running ECC validation tests..."
        ./build_production/Puzzle71Solver --validate-ecc --config "$test_config"

        log_info "Running deterministic replay tests..."
        ./build_production/Puzzle71Solver --validate-replay --config "$test_config"

        log_info "Running constitutional compliance tests..."
        ./build_production/Puzzle71Solver --validate-constitutional --config "$test_config"

        log_success "Production validation tests completed"
    else
        log_error "Production binary not found for testing"
        exit 1
    fi
}

# Create deployment configuration files
create_deployment_config() {
    log_info "Creating deployment configuration files..."

    # Main production configuration
    cat > "$DEPLOY_DIR/config/production.json" << EOF
{
    "version": "$VERSION",
    "deployment": {
        "environment": "$DEPLOY_ENV",
        "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
        "build_type": "$BUILD_TYPE"
    },
    "cuda": {
        "devices": "$GPU_DEVICES",
        "architectures": ["75", "86", "89", "90"],
        "optimization_level": "O3",
        "separable_compilation": true
    },
    "performance": {
        "target_utilization": 90,
        "memory_efficiency_target": 95,
        "batch_size": 1000000,
        "max_threads_per_block": 1024
    },
    "validation": {
        "constitutional_compliance": {
            "enabled": true,
            "version": "5.5",
            "strict_mode": true
        },
        "deterministic_replay": {
            "enabled": true,
            "hash_protection": "sha256"
        }
    },
    "monitoring": {
        "telemetry": {
            "enabled": true,
            "interval_seconds": 30,
            "metrics_retention_days": 30
        },
        "health_checks": {
            "enabled": true,
            "interval_seconds": 60
        }
    },
    "logging": {
        "level": "INFO",
        "file": "$DEPLOY_DIR/logs/puzzle71.log",
        "max_size_mb": 100,
        "backup_count": 5
    }
}
EOF

    # Environment-specific configuration
    if [ "$DEPLOY_ENV" = "development" ]; then
        cat > "$DEPLOY_DIR/config/development.json" << 'EOF'
{
    "debug": {
        "enabled": true,
        "verbose_logging": true,
        "memory_checks": true
    },
    "performance": {
        "profiling": true,
        "optimization_level": "O1"
    }
}
EOF
    elif [ "$DEPLOY_ENV" = "staging" ]; then
        cat > "$DEPLOY_DIR/config/staging.json" << 'EOF'
{
    "debug": {
        "enabled": false,
        "verbose_logging": true
    },
    "performance": {
        "profiling": false,
        "optimization_level": "O2"
    }
}
EOF
    fi

    log_success "Deployment configuration created"
}

# Create deployment scripts
create_deployment_scripts() {
    log_info "Creating deployment scripts..."

    # Start script
    cat > "$DEPLOY_DIR/scripts/start.sh" << 'EOF'
#!/bin/bash

# Puzzle71 Production Start Script
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPLOY_DIR="$(dirname "$SCRIPT_DIR")"

# Load configuration
CONFIG_FILE="$DEPLOY_DIR/config/production.json"
if [ ! -f "$CONFIG_FILE" ]; then
    echo "ERROR: Configuration file not found: $CONFIG_FILE"
    exit 1
fi

# Check if binary exists
BINARY="$DEPLOY_DIR/bin/Puzzle71Solver"
if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary not found: $BINARY"
    exit 1
fi

# Start the application
echo "Starting Puzzle71 Solver..."
cd "$DEPLOY_DIR"
"$BINARY" --config "$CONFIG_FILE" --daemon

echo "Puzzle71 Solver started successfully"
EOF

    # Stop script
    cat > "$DEPLOY_DIR/scripts/stop.sh" << 'EOF'
#!/bin/bash

# Puzzle71 Production Stop Script
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPLOY_DIR="$(dirname "$SCRIPT_DIR")"

# Stop the application
echo "Stopping Puzzle71 Solver..."
pkill -f "Puzzle71Solver" || true

# Wait for graceful shutdown
sleep 5

# Verify stopped
if pgrep -f "Puzzle71Solver" > /dev/null; then
    echo "WARNING: Some processes may still be running"
    pkill -9 -f "Puzzle71Solver" || true
fi

echo "Puzzle71 Solver stopped"
EOF

    # Health check script
    cat > "$DEPLOY_DIR/scripts/health_check.sh" << 'EOF'
#!/bin/bash

# Puzzle71 Health Check Script
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPLOY_DIR="$(dirname "$SCRIPT_DIR")"

# Check if process is running
if ! pgrep -f "Puzzle71Solver" > /dev/null; then
    echo "CRITICAL: Puzzle71Solver is not running"
    exit 2
fi

# Check GPU availability
if ! nvidia-smi &> /dev/null; then
    echo "CRITICAL: NVIDIA GPU not available"
    exit 2
fi

# Check log files
LOG_FILE="$DEPLOY_DIR/logs/puzzle71.log"
if [ ! -f "$LOG_FILE" ]; then
    echo "WARNING: Log file not found: $LOG_FILE"
else
    # Check for recent errors
    if tail -100 "$LOG_FILE" | grep -i "error\|critical\|fatal" > /dev/null; then
        echo "WARNING: Recent errors found in logs"
    fi
fi

echo "OK: Puzzle71Solver is healthy"
EOF

    # Maintenance script
    cat > "$DEPLOY_DIR/scripts/maintenance/cleanup.sh" << 'EOF'
#!/bin/bash

# Puzzle71 Maintenance Cleanup Script
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPLOY_DIR="$(dirname "$(dirname "$SCRIPT_DIR")")"

# Clean old logs
find "$DEPLOY_DIR/logs" -name "*.log" -mtime +7 -delete

# Clean old telemetry data
find "$DEPLOY_DIR/monitoring/metrics" -name "*.json" -mtime +30 -delete

# Clean temporary files
find /tmp -name "puzzle71_*" -mtime +1 -delete

echo "Cleanup completed"
EOF

    # Make scripts executable
    chmod +x "$DEPLOY_DIR/scripts"/*.sh
    chmod +x "$DEPLOY_DIR/scripts/maintenance"/*.sh

    log_success "Deployment scripts created"
}

# Create monitoring setup
create_monitoring_setup() {
    log_info "Creating monitoring setup..."

    # Monitoring configuration
    cat > "$DEPLOY_DIR/monitoring/metrics_collector.py" << 'EOF'
#!/usr/bin/env python3

import json
import time
import subprocess
import os
from datetime import datetime, timedelta

def collect_gpu_metrics():
    """Collect GPU performance metrics"""
    try:
        result = subprocess.run(['nvidia-smi', '--query-gpu=utilization.gpu,memory.used,memory.total,temperature.gpu,power.draw', '--format=csv,noheader,nounits'],
                              capture_output=True, text=True, check=True)
        lines = result.stdout.strip().split('\n')
        metrics = []
        for i, line in enumerate(lines):
            utilization, memory_used, memory_total, temperature, power = line.split(', ')
            metrics.append({
                'gpu_id': i,
                'utilization_percent': float(utilization),
                'memory_used_mb': float(memory_used),
                'memory_total_mb': float(memory_total),
                'temperature_c': float(temperature),
                'power_watts': float(power)
            })
        return metrics
    except Exception as e:
        print(f"Error collecting GPU metrics: {e}")
        return []

def collect_system_metrics():
    """Collect system performance metrics"""
    try:
        # CPU usage
        cpu_usage = float(subprocess.run(['top', '-bn1'], capture_output=True, text=True, check=True)
                          .stdout.split('%Cpu(s):')[1].split()[0])

        # Memory usage
        mem_info = subprocess.run(['free', '-m'], capture_output=True, text=True, check=True).stdout
        mem_lines = mem_info.split('\n')
        mem_total = int(mem_lines[1].split()[1])
        mem_used = int(mem_lines[1].split()[2])
        mem_usage_percent = (mem_used / mem_total) * 100

        return {
            'cpu_usage_percent': cpu_usage,
            'memory_used_mb': mem_used,
            'memory_total_mb': mem_total,
            'memory_usage_percent': mem_usage_percent
        }
    except Exception as e:
        print(f"Error collecting system metrics: {e}")
        return {}

def main():
    deploy_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    metrics_dir = os.path.join(deploy_dir, 'monitoring', 'metrics')

    while True:
        timestamp = datetime.utcnow().isoformat() + 'Z'

        metrics = {
            'timestamp': timestamp,
            'gpu_metrics': collect_gpu_metrics(),
            'system_metrics': collect_system_metrics()
        }

        # Write metrics to file
        metrics_file = os.path.join(metrics_dir, f'metrics_{datetime.now().strftime("%Y%m%d")}.jsonl')
        with open(metrics_file, 'a') as f:
            f.write(json.dumps(metrics) + '\n')

        # Clean old metrics files
        cutoff_date = datetime.now() - timedelta(days=30)
        for filename in os.listdir(metrics_dir):
            if filename.startswith('metrics_') and filename.endswith('.jsonl'):
                file_date = datetime.strptime(filename[8:16], '%Y%m%d')
                if file_date < cutoff_date:
                    os.remove(os.path.join(metrics_dir, filename))

        time.sleep(30)  # Collect metrics every 30 seconds

if __name__ == '__main__':
    main()
EOF

    chmod +x "$DEPLOY_DIR/monitoring/metrics_collector.py"

    # Health check configuration
    cat > "$DEPLOY_DIR/monitoring/health_config.json" << 'EOF'
{
    "checks": {
        "process_running": {
            "enabled": true,
            "critical": true
        },
        "gpu_available": {
            "enabled": true,
            "critical": true
        },
        "memory_usage": {
            "enabled": true,
            "threshold_percent": 90,
            "critical": false
        },
        "disk_space": {
            "enabled": true,
            "threshold_gb": 5,
            "critical": true
        },
        "log_errors": {
            "enabled": true,
            "critical": false
        }
    },
    "notifications": {
        "email": {
            "enabled": false,
            "recipients": []
        },
        "webhook": {
            "enabled": false,
            "url": ""
        }
    }
}
EOF

    log_success "Monitoring setup created"
}

# Create deployment manifest
create_deployment_manifest() {
    log_info "Creating deployment manifest..."

    local manifest_file="$DEPLOY_DIR/deployment-manifest.json"

    cat > "$manifest_file" << EOF
{
    "deployment": {
        "version": "$VERSION",
        "environment": "$DEPLOY_ENV",
        "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
        "build_type": "$BUILD_TYPE"
    },
    "system": {
        "hostname": "$(hostname)",
        "platform": "$(uname -s)",
        "architecture": "$(uname -m)",
        "cuda_version": "$(nvcc --version | grep "release" | awk '{print $6}' | sed 's/,//' 2>/dev/null || echo 'unknown')",
        "gpu_count": "$(nvidia-smi --list-gpus | wc -l 2>/dev/null || echo '0')"
    },
    "binaries": {
        "main": {
            "path": "bin/Puzzle71Solver",
            "version": "$VERSION",
            "build_timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
        }
    },
    "configuration": {
        "production": "config/production.json",
        "environment_specific": "config/$DEPLOY_ENV.json"
    },
    "directories": {
        "bin": "bin/",
        "config": "config/",
        "data": "data/",
        "logs": "logs/",
        "monitoring": "monitoring/",
        "scripts": "scripts/"
    },
    "validation": {
        "constitutional_compliance": {
            "enabled": true,
            "version": "5.5"
        },
        "performance_baselines": {
            "enabled": true,
            "validation_required": true
        }
    },
    "checksums": {
        "manifest": "$(sha256sum "$manifest_file" | cut -d' ' -f1)"
    }
}
EOF

    log_success "Deployment manifest created: $manifest_file"
}

# Validate deployment
validate_deployment() {
    log_info "Validating deployment..."

    local errors=0

    # Check required files
    local required_files=(
        "bin/Puzzle71Solver"
        "config/production.json"
        "scripts/start.sh"
        "scripts/stop.sh"
        "scripts/health_check.sh"
        "deployment-manifest.json"
    )

    for file in "${required_files[@]}"; do
        if [ ! -f "$DEPLOY_DIR/$file" ]; then
            log_error "Required file missing: $file"
            ((errors++))
        fi
    done

    # Check executable permissions
    local executable_files=(
        "bin/Puzzle71Solver"
        "scripts/start.sh"
        "scripts/stop.sh"
        "scripts/health_check.sh"
        "monitoring/metrics_collector.py"
    )

    for file in "${executable_files[@]}"; do
        if [ -f "$DEPLOY_DIR/$file" ] && [ ! -x "$DEPLOY_DIR/$file" ]; then
            log_error "File not executable: $file"
            ((errors++))
        fi
    done

    # Test configuration loading
    if [ -f "$DEPLOY_DIR/config/production.json" ]; then
        if python3 -c "import json; json.load(open('$DEPLOY_DIR/config/production.json'))" 2>/dev/null; then
            log_success "Configuration files are valid JSON"
        else
            log_error "Invalid JSON in configuration files"
            ((errors++))
        fi
    fi

    # Test basic binary functionality
    if [ -f "$DEPLOY_DIR/bin/Puzzle71Solver" ]; then
        if "$DEPLOY_DIR/bin/Puzzle71Solver" --version &> /dev/null; then
            log_success "Binary functionality validated"
        else
            log_error "Binary functionality test failed"
            ((errors++))
        fi
    fi

    if [ "$errors" -gt 0 ]; then
        log_error "Deployment validation failed with $errors errors"
        exit 1
    fi

    log_success "Deployment validation passed!"
}

# Create deployment archive
create_deployment_archive() {
    log_info "Creating deployment archive..."

    local archive_name="puzzle71-${VERSION}-${DEPLOY_ENV}-$(date +%Y%m%d_%H%M%S).tar.gz"
    local archive_path="$PROJECT_ROOT/$archive_name"

    # Create archive
    cd "$PROJECT_ROOT"
    tar -czf "$archive_path" -C "$DEPLOY_DIR" .

    # Create checksum
    local checksum_file="${archive_path}.sha256"
    sha256sum "$archive_path" > "$checksum_file"

    log_success "Deployment archive created: $archive_path"
    log_info "Archive checksum: $(cat "$checksum_file")"

    # Display archive info
    local archive_size=$(du -h "$archive_path" | cut -f1)
    log_info "Archive size: $archive_size"
}

# Main deployment function
main() {
    log_info "Puzzle71 Production Deployment v$VERSION"
    log_info "====================================="

    # Record start time
    local start_time=$(date +%s)

    # Execute deployment steps
    validate_deployment_environment
    create_deployment_structure
    build_production_binaries
    run_production_tests
    create_deployment_config
    create_deployment_scripts
    create_monitoring_setup
    create_deployment_manifest
    validate_deployment
    create_deployment_archive

    # Calculate deployment time
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))

    log_success "Deployment completed successfully in ${duration}s!"
    log_info "Deployment directory: $DEPLOY_DIR"
    log_info "Configuration: $DEPLOY_DIR/config/production.json"
    log_info "Start command: $DEPLOY_DIR/scripts/start.sh"
    log_info "Health check: $DEPLOY_DIR/scripts/health_check.sh"
    log_info ""
    log_info "Next steps:"
    log_info "1. Review configuration in $DEPLOY_DIR/config/"
    log_info "2. Start the service: $DEPLOY_DIR/scripts/start.sh"
    log_info "3. Monitor health: $DEPLOY_DIR/scripts/health_check.sh"
    log_info "4. Check logs: tail -f $DEPLOY_DIR/logs/puzzle71.log"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --gpu-devices)
            GPU_DEVICES="$2"
            shift 2
            ;;
        --environment)
            DEPLOY_ENV="$2"
            shift 2
            ;;
        --skip-tests)
            SKIP_TESTS="true"
            shift
            ;;
        --skip-benchmarks)
            SKIP_BENCHMARKS="true"
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --build-type TYPE     Build type (Debug|Release|RelWithDebInfo)"
            echo "  --gpu-devices DEVICES GPU devices to use (all|0|1,2,etc)"
            echo "  --environment ENV     Deployment environment (development|staging|production)"
            echo "  --skip-tests          Skip production tests"
            echo "  --skip-benchmarks     Skip performance benchmarks"
            echo "  --help                Show this help message"
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Run deployment
main
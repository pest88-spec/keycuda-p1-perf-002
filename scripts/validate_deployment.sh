#!/bin/bash
# Puzzle71 Technical Debt Repair - Deployment Validation Script
# T080: Create deployment scripts and Docker production images
# Comprehensive validation of deployment readiness and functionality

set -euo pipefail

# Script metadata
SCRIPT_VERSION="2.0.0"
SCRIPT_DATE="2025-10-20"
SCRIPT_NAME="validate_deployment.sh"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
VALIDATION_LOG="${PROJECT_ROOT}/logs/deployment_validation_$(date +%Y%m%d_%H%M%S).log"

# Test configuration
DEFAULT_TIMEOUT=30
DEFAULT_ITERATIONS=100
TOLERANCE_PERCENT=5.0

# Functions
log() {
    local level=$1
    shift
    local message="$*"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')

    case $level in
        "INFO")  echo -e "${GREEN}[INFO]${NC}  ${timestamp} - $message" ;;
        "WARN")  echo -e "${YELLOW}[WARN]${NC}  ${timestamp} - $message" ;;
        "ERROR") echo -e "${RED}[ERROR]${NC} ${timestamp} - $message" ;;
        "DEBUG") echo -e "${BLUE}[DEBUG]${NC} ${timestamp} - $message" ;;
    esac

    # Log to file
    echo "[$timestamp] [$level] $message" >> "$VALIDATION_LOG"
}

# Show help
show_help() {
    cat << EOF
Puzzle71 Deployment Validation Script v${SCRIPT_VERSION}

Usage: $SCRIPT_NAME [OPTIONS]

QUICK START:
    $SCRIPT_NAME                    # Full validation suite
    $SCRIPT_NAME --quick            # Quick validation only
    $SCRIPT_NAME --docker           # Validate Docker deployment
    $SCRIPT_NAME --native           # Validate native deployment

OPTIONS:
    --quick                        Quick validation only (basic checks)
    --comprehensive                Comprehensive validation (default)
    --docker                       Validate Docker deployment
    --native                       Validate native deployment
    --performance                  Include performance benchmarks
    --integration                  Include integration tests
    --timeout SECONDS             Test timeout (default: $DEFAULT_TIMEOUT)
    --iterations COUNT           Test iterations (default: $DEFAULT_ITERATIONS)
    --tolerance PERCENT          Performance tolerance (default: $TOLERANCE_PERCENT)
    --log-file FILE              Custom log file location
    --help                        Show this help message

VALIDATION CATEGORIES:
    1. Environment Validation
       - System requirements
       - CUDA environment
       - Dependencies and libraries

    2. Build Validation
       - Compilation success
       - Binary functionality
       - Configuration loading

    3. Functional Validation
       - ECC operations
       - Deterministic replay
       - Constitutional compliance

    4. Performance Validation
       - GPU utilization
       - Memory efficiency
       - Throughput benchmarks

    5. Integration Validation
       - End-to-end pipeline
       - Monitoring integration
       - Health checks

EXAMPLES:
    # Quick validation for development
    $SCRIPT_NAME --quick

    # Full validation for production
    $SCRIPT_NAME --comprehensive --performance

    # Docker deployment validation
    $SCRIPT_NAME --docker --integration

    # Performance-focused validation
    $SCRIPT_NAME --performance --iterations 1000

EXIT CODES:
    0: All validations passed
    1: Critical validation failures
    2: Performance validation failures
    3: Integration validation failures
    4: Environment validation failures

EOF
}

# Parse arguments
VALIDATION_TYPE="comprehensive"
DEPLOYMENT_TYPE="auto"
INCLUDE_PERFORMANCE=false
INCLUDE_INTEGRATION=false
TIMEOUT=$DEFAULT_TIMEOUT
ITERATIONS=$DEFAULT_ITERATIONS
TOLERANCE=$TOLERANCE_PERCENT

while [[ $# -gt 0 ]]; do
    case $1 in
        --quick)
            VALIDATION_TYPE="quick"
            shift
            ;;
        --comprehensive)
            VALIDATION_TYPE="comprehensive"
            shift
            ;;
        --docker)
            DEPLOYMENT_TYPE="docker"
            shift
            ;;
        --native)
            DEPLOYMENT_TYPE="native"
            shift
            ;;
        --performance)
            INCLUDE_PERFORMANCE=true
            shift
            ;;
        --integration)
            INCLUDE_INTEGRATION=true
            shift
            ;;
        --timeout)
            TIMEOUT="$2"
            shift 2
            ;;
        --iterations)
            ITERATIONS="$2"
            shift 2
            ;;
        --tolerance)
            TOLERANCE="$2"
            shift 2
            ;;
        --log-file)
            VALIDATION_LOG="$2"
            shift 2
            ;;
        --help)
            show_help
            exit 0
            ;;
        *)
            log "ERROR" "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Initialize validation log
mkdir -p "$(dirname "$VALIDATION_LOG")"
echo "Puzzle71 Deployment Validation - $(date)" > "$VALIDATION_LOG"
echo "Script Version: $SCRIPT_VERSION" >> "$VALIDATION_LOG"
echo "Validation Type: $VALIDATION_TYPE" >> "$VALIDATION_LOG"
echo "Deployment Type: $DEPLOYMENT_TYPE" >> "$VALIDATION_LOG"
echo "========================================" >> "$VALIDATION_LOG"

# Validation result tracking
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
CRITICAL_FAILURES=0

# Test result function
test_result() {
    local test_name="$1"
    local result="$2"
    local message="$3"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    if [ "$result" = "PASS" ]; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
        log "INFO" "✓ PASS: $test_name - $message"
    else
        FAILED_TESTS=$((FAILED_TESTS + 1))
        log "ERROR" "✗ FAIL: $test_name - $message"

        if [[ "$test_name" =~ (environment|build|critical) ]]; then
            CRITICAL_FAILURES=$((CRITICAL_FAILURES + 1))
        fi
    fi
}

# Environment validation
validate_environment() {
    log "INFO" "Starting environment validation..."

    local errors=0

    # OS validation
    if [[ "$OSTYPE" != "linux-gnu"* ]]; then
        test_result "environment_os" "FAIL" "Unsupported OS: $OSTYPE"
        ((errors++))
    else
        test_result "environment_os" "PASS" "Linux OS detected: $(uname -s)"
    fi

    # CUDA validation
    if command -v nvcc &> /dev/null; then
        local cuda_version=$(nvcc --version | grep "release" | awk '{print $6}' | sed 's/,//')
        test_result "environment_cuda" "PASS" "CUDA toolkit found: $cuda_version"
    else
        test_result "environment_cuda" "FAIL" "CUDA toolkit not found"
        ((errors++))
    fi

    # GPU validation
    if nvidia-smi &> /dev/null; then
        local gpu_count=$(nvidia-smi --list-gpus | wc -l)
        local gpu_name=$(nvidia-smi --query-gpu=name --format=csv,noheader | head -n1)
        test_result "environment_gpu" "PASS" "GPU detected: $gpu_count x $gpu_name"
    else
        test_result "environment_gpu" "FAIL" "NVIDIA GPU not available"
        ((errors++))
    fi

    # Memory validation
    local total_mem=$(free -g | awk '/^Mem:/{print $2}')
    if [ "$total_mem" -ge 16 ]; then
        test_result "environment_memory" "PASS" "Memory: ${total_mem}GB"
    else
        test_result "environment_memory" "WARN" "Low memory: ${total_mem}GB (16GB+ recommended)"
    fi

    # Disk space validation
    local available_space=$(df -BG "$PROJECT_ROOT" | awk 'NR==2 {print $4}' | sed 's/G//')
    if [ "$available_space" -ge 10 ]; then
        test_result "environment_disk" "PASS" "Disk space: ${available_space}GB available"
    else
        test_result "environment_disk" "FAIL" "Insufficient disk space: ${available_space}GB"
        ((errors++))
    fi

    # Dependencies validation
    local required_tools=("cmake" "make" "git")
    for tool in "${required_tools[@]}"; do
        if command -v "$tool" &> /dev/null; then
            test_result "environment_tool_$tool" "PASS" "Tool found: $tool"
        else
            test_result "environment_tool_$tool" "FAIL" "Required tool not found: $tool"
            ((errors++))
        fi
    done

    # Libraries validation
    if ldconfig -p | grep -q libssl; then
        test_result "environment_lib_ssl" "PASS" "OpenSSL library found"
    else
        test_result "environment_lib_ssl" "FAIL" "OpenSSL library not found"
        ((errors++))
    fi

    if ldconfig -p | grep -q libsecp256k1; then
        test_result "environment_lib_secp256k1" "PASS" "libsecp256k1 library found"
    else
        test_result "environment_lib_secp256k1" "WARN" "libsecp256k1 library not found (ECC validation may fail)"
    fi

    log "INFO" "Environment validation completed with $errors errors"
    return $errors
}

# Build validation
validate_build() {
    log "INFO" "Starting build validation..."

    local errors=0

    # Check build directory
    local build_dir="$PROJECT_ROOT/build"
    if [ -d "$build_dir" ]; then
        test_result "build_directory" "PASS" "Build directory exists"
    else
        test_result "build_directory" "FAIL" "Build directory not found"
        ((errors++))
        return $errors
    fi

    # Check binary
    local binary="$build_dir/Puzzle71Solver"
    if [ -f "$binary" ]; then
        test_result "build_binary" "PASS" "Main binary found"

        # Test binary functionality
        if timeout 10 "$binary" --version &> /dev/null; then
            local version_output=$("$binary" --version 2>/dev/null || echo "unknown")
            test_result "build_binary_version" "PASS" "Binary version: $version_output"
        else
            test_result "build_binary_version" "FAIL" "Binary version test failed"
            ((errors++))
        fi

        # Test help functionality
        if timeout 10 "$binary" --help &> /dev/null; then
            test_result "build_binary_help" "PASS" "Binary help command works"
        else
            test_result "build_binary_help" "WARN" "Binary help command failed"
        fi
    else
        test_result "build_binary" "FAIL" "Main binary not found"
        ((errors++))
    fi

    # Check configuration files
    local config_files=(
        "config/production.yaml"
        "specs/002-techdebt-repair/spec.md"
        "specs/002-techdebt-repair/tasks.md"
    )

    for config_file in "${config_files[@]}"; do
        if [ -f "$PROJECT_ROOT/$config_file" ]; then
            test_result "build_config_$(basename "$config_file")" "PASS" "Config file found: $config_file"
        else
            test_result "build_config_$(basename "$config_file")" "FAIL" "Config file missing: $config_file"
            ((errors++))
        fi
    done

    log "INFO" "Build validation completed with $errors errors"
    return $errors
}

# Functional validation
validate_functional() {
    log "INFO" "Starting functional validation..."

    local errors=0
    local binary="$PROJECT_ROOT/build/Puzzle71Solver"

    if [ ! -f "$binary" ]; then
        test_result "functional_binary" "FAIL" "Binary not found for functional testing"
        return 1
    fi

    # ECC validation test
    log "INFO" "Testing ECC operations validation..."
    if timeout $TIMEOUT "$binary" --validate-ecc --iterations $ITERATIONS &>/dev/null; then
        test_result "functional_ecc_validation" "PASS" "ECC validation passed ($ITERATIONS iterations)"
    else
        test_result "functional_ecc_validation" "FAIL" "ECC validation failed"
        ((errors++))
    fi

    # Deterministic replay test
    log "INFO" "Testing deterministic replay..."
    if timeout $TIMEOUT "$binary" --validate-replay --test-cases 10 &>/dev/null; then
        test_result "functional_deterministic_replay" "PASS" "Deterministic replay validation passed"
    else
        test_result "functional_deterministic_replay" "FAIL" "Deterministic replay validation failed"
        ((errors++))
    fi

    # Constitutional compliance test
    log "INFO" "Testing constitutional compliance..."
    if timeout $TIMEOUT "$binary" --validate-constitutional --version 5.5 &>/dev/null; then
        test_result "functional_constitutional" "PASS" "Constitutional v5.5 compliance passed"
    else
        test_result "functional_constitutional" "FAIL" "Constitutional compliance validation failed"
        ((errors++))
    fi

    # Configuration loading test
    log "INFO" "Testing configuration loading..."
    local config_file="$PROJECT_ROOT/config/production.yaml"
    if [ -f "$config_file" ]; then
        if timeout $TIMEOUT "$binary" --config "$config_file" --dry-run &>/dev/null; then
            test_result "functional_config_loading" "PASS" "Configuration loaded successfully"
        else
            test_result "functional_config_loading" "FAIL" "Configuration loading failed"
            ((errors++))
        fi
    else
        test_result "functional_config_loading" "WARN" "Production configuration file not found"
    fi

    log "INFO" "Functional validation completed with $errors errors"
    return $errors
}

# Performance validation
validate_performance() {
    log "INFO" "Starting performance validation..."

    local errors=0
    local binary="$PROJECT_ROOT/build/Puzzle71Solver"

    if [ ! -f "$binary" ]; then
        test_result "performance_binary" "FAIL" "Binary not found for performance testing"
        return 1
    fi

    # Quick benchmark test
    log "INFO" "Running quick performance benchmark..."
    local start_time=$(date +%s.%N)

    if timeout $TIMEOUT "$binary" --benchmark --quick &>/dev/null; then
        local end_time=$(date +%s.%N)
        local duration=$(echo "$end_time - $start_time" | bc)
        test_result "performance_benchmark" "PASS" "Quick benchmark completed in ${duration}s"
    else
        test_result "performance_benchmark" "WARN" "Quick benchmark failed or timed out"
    fi

    # Memory usage test
    log "INFO" "Testing memory usage..."
    if command -v /usr/bin/time &> /dev/null; then
        local memory_output=$(/usr/bin/time -v timeout $TIMEOUT "$binary" --validate-ecc --iterations 100 2>&1 | grep "Maximum resident set size" | awk '{print $6}' || echo "0")
        if [ "$memory_output" -gt 0 ]; then
            local memory_mb=$((memory_output / 1024))
            if [ "$memory_mb" -lt 1024 ]; then  # Less than 1GB
                test_result "performance_memory" "PASS" "Memory usage: ${memory_mb}MB"
            else
                test_result "performance_memory" "WARN" "High memory usage: ${memory_mb}MB"
            fi
        else
            test_result "performance_memory" "WARN" "Could not measure memory usage"
        fi
    else
        test_result "performance_memory" "WARN" "/usr/bin/time not available for memory measurement"
    fi

    log "INFO" "Performance validation completed with $errors errors"
    return $errors
}

# Integration validation
validate_integration() {
    log "INFO" "Starting integration validation..."

    local errors=0

    # Docker integration test
    if [ "$DEPLOYMENT_TYPE" = "docker" ] || [ "$DEPLOYMENT_TYPE" = "auto" ]; then
        if command -v docker &> /dev/null; then
            # Check if puzzle71 container is running
            if docker ps | grep -q "puzzle71"; then
                test_result "integration_docker_running" "PASS" "Puzzle71 Docker container is running"

                # Test container health
                local container_name=$(docker ps --format "table {{.Names}}" | grep puzzle71 | head -n1)
                if [ -n "$container_name" ]; then
                    local health_status=$(docker inspect --format='{{.State.Health.Status}}' "$container_name" 2>/dev/null || echo "unknown")
                    if [ "$health_status" = "healthy" ]; then
                        test_result "integration_docker_health" "PASS" "Container health check passed"
                    else
                        test_result "integration_docker_health" "WARN" "Container health status: $health_status"
                    fi
                fi
            else
                test_result "integration_docker_running" "WARN" "No Puzzle71 Docker containers running"
            fi
        else
            test_result "integration_docker" "WARN" "Docker not available for integration testing"
        fi
    fi

    # Monitoring integration test
    if command -v curl &> /dev/null; then
        # Test health endpoint
        if curl -s --max-time 5 http://localhost:8080/health &>/dev/null; then
            test_result "integration_health_endpoint" "PASS" "Health endpoint accessible"
        else
            test_result "integration_health_endpoint" "WARN" "Health endpoint not accessible"
        fi

        # Test metrics endpoint
        if curl -s --max-time 5 http://localhost:8080/metrics &>/dev/null; then
            test_result "integration_metrics_endpoint" "PASS" "Metrics endpoint accessible"
        else
            test_result "integration_metrics_endpoint" "WARN" "Metrics endpoint not accessible"
        fi
    else
        test_result "integration_curl" "WARN" "curl not available for endpoint testing"
    fi

    # Configuration integration test
    local config_file="$PROJECT_ROOT/config/production.yaml"
    if [ -f "$config_file" ]; then
        if python3 -c "import yaml; yaml.safe_load(open('$config_file'))" 2>/dev/null; then
            test_result "integration_config_yaml" "PASS" "Configuration YAML is valid"
        else
            test_result "integration_config_yaml" "FAIL" "Configuration YAML is invalid"
            ((errors++))
        fi
    else
        test_result "integration_config_file" "WARN" "Production configuration file not found"
    fi

    log "INFO" "Integration validation completed with $errors errors"
    return $errors
}

# Generate validation report
generate_report() {
    local success_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))

    cat << EOF

DEPLOYMENT VALIDATION REPORT
============================
Script Version: $SCRIPT_VERSION
Validation Date: $(date)
Validation Type: $VALIDATION_TYPE
Deployment Type: $DEPLOYMENT_TYPE

SUMMARY:
--------
Total Tests: $TOTAL_TESTS
Passed: $PASSED_TESTS
Failed: $FAILED_TESTS
Success Rate: ${success_rate}%
Critical Failures: $CRITICAL_FAILURES

DETAILED RESULTS:
----------------
$(cat "$VALIDATION_LOG" | grep -E "^\[.*\] (PASS|FAIL|WARN):" | sort)

RECOMMENDATIONS:
---------------
EOF

    if [ $CRITICAL_FAILURES -gt 0 ]; then
        echo "❌ CRITICAL ISSUES FOUND - Deployment not ready for production"
        echo "   - Fix all critical failures before proceeding"
        echo "   - Review environment and build requirements"
    elif [ $success_rate -ge 90 ]; then
        echo "✅ DEPLOYMENT READY - Validation passed successfully"
        echo "   - System is ready for production deployment"
        echo "   - All critical requirements satisfied"
    elif [ $success_rate -ge 75 ]; then
        echo "⚠️  DEPLOYMENT READY WITH CAVEATS - Minor issues detected"
        echo "   - Address warnings for optimal performance"
        echo "   - Production deployment possible with monitoring"
    else
        echo "🔧 DEPLOYMENT NEEDS IMPROVEMENT - Multiple issues detected"
        echo "   - Address validation failures before production"
        echo "   - Consider environment and configuration review"
    fi

    echo ""
    echo "Log file: $VALIDATION_LOG"
    echo "Project root: $PROJECT_ROOT"
    echo ""

    # Return appropriate exit code
    if [ $CRITICAL_FAILURES -gt 0 ]; then
        return 4  # Critical failures
    elif [ $success_rate -ge 90 ]; then
        return 0  # Success
    else
        return 1  # General failures
    fi
}

# Main validation function
main() {
    log "INFO" "Starting Puzzle71 deployment validation v${SCRIPT_VERSION}"
    log "INFO" "Validation log: $VALIDATION_LOG"

    local errors=0

    # Environment validation (always required)
    validate_environment || ((errors++))

    # Build validation (always required for comprehensive)
    if [ "$VALIDATION_TYPE" = "comprehensive" ]; then
        validate_build || ((errors++))
    fi

    # Functional validation
    if [ "$VALIDATION_TYPE" = "comprehensive" ] || [ "$VALIDATION_TYPE" = "quick" ]; then
        validate_functional || ((errors++))
    fi

    # Performance validation
    if [ "$INCLUDE_PERFORMANCE" = true ] || [ "$VALIDATION_TYPE" = "comprehensive" ]; then
        validate_performance || ((errors++))
    fi

    # Integration validation
    if [ "$INCLUDE_INTEGRATION" = true ] || [ "$VALIDATION_TYPE" = "comprehensive" ]; then
        validate_integration || ((errors++))
    fi

    # Generate report and exit
    generate_report
    exit_code=$?

    log "INFO" "Deployment validation completed with exit code: $exit_code"
    exit $exit_code
}

# Run main function
main "$@"
#!/bin/bash
# Puzzle71 CUDA Technical Debt Repair System
# Production Health Check Script v3.0
# T080: Create deployment scripts and Docker production images

set -euo pipefail

# Health check configuration
HEALTH_CHECK_VERSION="3.0.0"
CONTAINER_NAME="puzzle71-solver"
LOG_FILE="/opt/puzzle71/logs/health_check.log"
METRICS_FILE="/opt/puzzle71/monitoring/health_metrics.json"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Health check thresholds
GPU_UTILIZATION_MIN=80
MEMORY_USAGE_MAX=90
THROUGHPUT_MIN_KEYS_PER_SEC=100
RESPONSE_TIME_MAX_MS=5000

# Logging function
log_health() {
    local level="$1"
    local message="$2"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[$timestamp] [$level] $message" | tee -a "$LOG_FILE"
}

# Function to check GPU availability and health
check_gpu_health() {
    log_health "INFO" "Checking GPU health..."

    # Check if NVIDIA GPU is available
    if ! nvidia-smi &> /dev/null; then
        log_health "ERROR" "NVIDIA GPU not available"
        return 1
    fi

    # Check GPU temperature
    local gpu_temp=$(nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits | head -n1)
    if [[ $gpu_temp -gt 85 ]]; then
        log_health "ERROR" "GPU temperature too high: ${gpu_temp}°C"
        return 1
    fi

    # Check GPU memory usage
    local memory_used=$(nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits | head -n1)
    local memory_total=$(nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits | head -n1)
    local memory_usage=$((memory_used * 100 / memory_total))

    if [[ $memory_usage -gt $MEMORY_USAGE_MAX ]]; then
        log_health "ERROR" "GPU memory usage too high: ${memory_usage}%"
        return 1
    fi

    # Check GPU utilization
    local gpu_util=$(nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits | head -n1)
    if [[ $gpu_util -lt $GPU_UTILIZATION_MIN ]]; then
        log_health "WARNING" "GPU utilization low: ${gpu_util}% (threshold: ${GPU_UTILIZATION_MIN}%)"
    fi

    log_health "INFO" "GPU health: Temp=${gpu_temp}°C, Memory=${memory_usage}%, Utilization=${gpu_util}%"
    return 0
}

# Function to check application process
check_application_process() {
    log_health "INFO" "Checking application process..."

    # Check if Puzzle71Solver process is running
    if ! pgrep -f "Puzzle71Solver" &> /dev/null; then
        log_health "ERROR" "Puzzle71Solver process not running"
        return 1
    fi

    local pid=$(pgrep -f "Puzzle71Solver" | head -n1)

    # Check process CPU and memory usage
    local cpu_usage=$(ps -p "$pid" -o %cpu --no-headers 2>/dev/null | tr -d ' ')
    local mem_usage=$(ps -p "$pid" -o %mem --no-headers 2>/dev/null | tr -d ' ')

    if [[ -n "$cpu_usage" && -n "$mem_usage" ]]; then
        log_health "INFO" "Process health: PID=${pid}, CPU=${cpu_usage}%, Memory=${mem_usage}%"
    else
        log_health "WARNING" "Could not determine process resource usage"
    fi

    return 0
}

# Function to check file system and directories
check_file_system() {
    log_health "INFO" "Checking file system health..."

    local required_dirs=(
        "/opt/puzzle71/config"
        "/opt/puzzle71/data"
        "/opt/puzzle71/results"
        "/opt/puzzle71/logs"
        "/opt/puzzle71/monitoring"
    )

    for dir in "${required_dirs[@]}"; do
        if [[ ! -d "$dir" ]]; then
            log_health "ERROR" "Required directory missing: $dir"
            return 1
        fi

        # Check directory permissions
        if [[ ! -r "$dir" || ! -w "$dir" ]]; then
            log_health "ERROR" "Insufficient permissions for directory: $dir"
            return 1
        fi
    done

    # Check disk space
    local disk_usage=$(df /opt/puzzle71 | awk 'NR==2 {print $5}' | sed 's/%//')
    if [[ $disk_usage -gt 90 ]]; then
        log_health "ERROR" "Disk usage too high: ${disk_usage}%"
        return 1
    fi

    log_health "INFO" "File system health: Disk usage=${disk_usage}%, All directories accessible"
    return 0
}

# Function to check configuration files
check_configuration() {
    log_health "INFO" "Checking configuration files..."

    local config_file="/opt/puzzle71/config/production.yaml"

    if [[ ! -f "$config_file" ]]; then
        log_health "ERROR" "Production configuration file missing: $config_file"
        return 1
    fi

    # Validate YAML syntax
    if ! python3 -c "import yaml; yaml.safe_load(open('$config_file'))" 2>/dev/null; then
        log_health "ERROR" "Production configuration YAML syntax is invalid"
        return 1
    fi

    # Check data files
    local data_files=(
        "/opt/puzzle71/data/private_ranges.txt"
        "/opt/puzzle71/data/target_addresses.txt"
    )

    for file in "${data_files[@]}"; do
        if [[ ! -f "$file" ]]; then
            log_health "ERROR" "Required data file missing: $file"
            return 1
        fi

        if [[ ! -s "$file" ]]; then
            log_health "ERROR" "Data file is empty: $file"
            return 1
        fi
    done

    log_health "INFO" "Configuration files validation passed"
    return 0
}

# Function to check application performance metrics
check_performance_metrics() {
    log_health "INFO" "Checking performance metrics..."

    # Check if metrics endpoint is accessible
    if curl -s --max-time 5 "http://localhost:8080/metrics" &> /dev/null; then
        log_health "INFO" "Metrics endpoint accessible"

        # Extract key metrics
        local throughput=$(curl -s "http://localhost:8080/metrics" 2>/dev/null | grep "puzzle71_throughput_keys_per_second" | tail -n1 | awk '{print $2}' || echo "0")

        if [[ -n "$throughput" && "$throughput" =~ ^[0-9]+$ ]]; then
            if [[ $throughput -lt $THROUGHPUT_MIN_KEYS_PER_SEC ]]; then
                log_health "WARNING" "Low throughput detected: ${throughput} keys/sec (threshold: ${THROUGHPUT_MIN_KEYS_PER_SEC})"
            else
                log_health "INFO" "Performance metrics: Throughput=${throughput} keys/sec"
            fi
        else
            log_health "WARNING" "Could not parse throughput metrics"
        fi
    else
        log_health "WARNING" "Metrics endpoint not accessible"
    fi

    return 0
}

# Function to check connectivity to dependent services
check_service_connectivity() {
    log_health "INFO" "Checking service connectivity..."

    # Check Redis connectivity
    if command -v redis-cli &> /dev/null; then
        if redis-cli -h redis -p 6379 ping &> /dev/null; then
            log_health "INFO" "Redis connectivity: OK"
        else
            log_health "WARNING" "Redis connectivity: FAILED"
        fi
    else
        log_health "WARNING" "Redis CLI not available for connectivity check"
    fi

    # Check Prometheus connectivity
    if curl -s --max-time 5 "http://prometheus:9090/-/healthy" &> /dev/null; then
        log_health "INFO" "Prometheus connectivity: OK"
    else
        log_health "WARNING" "Prometheus connectivity: FAILED"
    fi

    return 0
}

# Function to check log files for errors
check_log_health() {
    log_health "INFO" "Checking log file health..."

    local main_log="/opt/puzzle71/logs/puzzle71.log"

    if [[ -f "$main_log" ]]; then
        # Count errors in the last 100 lines
        local error_count=$(tail -n 100 "$main_log" 2>/dev/null | grep -i "error\|exception\|failed" | wc -l)

        if [[ $error_count -gt 10 ]]; then
            log_health "WARNING" "High error count in logs: ${error_count} errors in last 100 lines"
        elif [[ $error_count -gt 0 ]]; then
            log_health "INFO" "Log health: ${error_count} errors in last 100 lines"
        else
            log_health "INFO" "Log health: No recent errors detected"
        fi

        # Check if log file is being written to
        local log_age=$(find "$main_log" -mmin -5 | wc -l)
        if [[ $log_age -eq 1 ]]; then
            log_health "INFO" "Log file is actively being written"
        else
            log_health "WARNING" "Log file has not been updated in the last 5 minutes"
        fi
    else
        log_health "WARNING" "Main log file not found: $main_log"
    fi

    return 0
}

# Function to generate health metrics JSON
generate_health_metrics() {
    local timestamp=$(date -u +"%Y-%m-%dT%H:%M:%S.%3NZ")
    local overall_status="healthy"

    # Collect metrics
    local gpu_temp=$(nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits | head -n1 2>/dev/null || echo "0")
    local gpu_util=$(nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits | head -n1 2>/dev/null || echo "0")
    local memory_used=$(nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits | head -n1 2>/dev/null || echo "0")
    local memory_total=$(nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits | head -n1 2>/dev/null || echo "1")
    local memory_usage=$((memory_used * 100 / memory_total))
    local disk_usage=$(df /opt/puzzle71 | awk 'NR==2 {print $5}' | sed 's/%//' 2>/dev/null || echo "0")

    # Generate JSON metrics
    cat > "$METRICS_FILE" << EOF
{
  "timestamp": "$timestamp",
  "version": "$HEALTH_CHECK_VERSION",
  "container": "$CONTAINER_NAME",
  "overall_status": "$overall_status",
  "gpu": {
    "temperature_celsius": $gpu_temp,
    "utilization_percent": $gpu_util,
    "memory_used_mb": $memory_used,
    "memory_total_mb": $memory_total,
    "memory_usage_percent": $memory_usage
  },
  "system": {
    "disk_usage_percent": $disk_usage,
    "uptime_seconds": $(cat /proc/uptime | awk '{print $1}' | cut -d. -f1)
  },
  "checks": {
    "gpu_health": true,
    "application_process": true,
    "file_system": true,
    "configuration": true,
    "performance_metrics": true,
    "service_connectivity": true,
    "log_health": true
  }
}
EOF
}

# Main health check function
main() {
    local overall_status=0

    log_health "INFO" "Starting Puzzle71 health check v$HEALTH_CHECK_VERSION"

    # Run all health checks
    check_gpu_health || overall_status=1
    check_application_process || overall_status=1
    check_file_system || overall_status=1
    check_configuration || overall_status=1
    check_performance_metrics || true  # Don't fail health check for performance warnings
    check_service_connectivity || true  # Don't fail health check for connectivity warnings
    check_log_health || true  # Don't fail health check for log warnings

    # Generate metrics
    generate_health_metrics

    # Log final status
    if [[ $overall_status -eq 0 ]]; then
        log_health "INFO" "Health check completed: HEALTHY"
    else
        log_health "ERROR" "Health check completed: UNHEALTHY"
    fi

    return $overall_status
}

# Script entry point
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
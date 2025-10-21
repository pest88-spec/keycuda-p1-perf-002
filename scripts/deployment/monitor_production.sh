#!/bin/bash

# Puzzle71 Production Monitoring Script v2.0
# Real-time monitoring and alerting for production deployment

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
MONITOR_VERSION="2.0.0"
ALERT_THRESHOLD_FILE="/opt/puzzle71/monitoring/alert_thresholds.json"
METRICS_EXPORT_FILE="/opt/puzzle71/monitoring/current_metrics.json"
LOG_FILE="/opt/puzzle71/logs/monitor.log"

# Alert thresholds
GPU_TEMP_MAX=85
GPU_UTIL_MIN=70
MEMORY_USAGE_MAX=85
DISK_USAGE_MAX=90
THROUGHPUT_MIN_KEYS_PER_SEC=500
RESPONSE_TIME_MAX_MS=10000

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_monitor() {
    local level="$1"
    local message="$2"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[$timestamp] [$level] $message" | tee -a "$LOG_FILE"
}

# Function to get GPU metrics
get_gpu_metrics() {
    local gpu_metrics
    gpu_metrics=$(nvidia-smi --query-gpu=temperature.gpu,utilization.gpu,memory.used,memory.total,power.draw --format=csv,noheader,nounits 2>/dev/null | head -n1)

    if [[ -n "$gpu_metrics" ]]; then
        echo "$gpu_metrics"
    else
        echo "0,0,0,0,0"
    fi
}

# Function to get system metrics
get_system_metrics() {
    local cpu_usage=$(top -bn1 | grep "Cpu(s)" | awk '{print $2}' | sed 's/%us,//')
    local memory_usage=$(free | awk 'NR==2{printf "%.1f", $3*100/$2}')
    local disk_usage=$(df /opt/puzzle71 | awk 'NR==2 {print $5}' | sed 's/%//')
    local load_avg=$(uptime | awk -F'load average:' '{print $2}' | awk '{print $1}' | sed 's/,//')

    echo "${cpu_usage:-0},${memory_usage:-0},${disk_usage:-0},${load_avg:-0}"
}

# Function to get application metrics
get_application_metrics() {
    local throughput=0
    local response_time=0
    local error_count=0

    # Get metrics from application endpoint
    if curl -s --max-time 5 "http://localhost:8080/metrics" &> /dev/null; then
        throughput=$(curl -s "http://localhost:8080/metrics" 2>/dev/null | grep "puzzle71_throughput_keys_per_second" | tail -n1 | awk '{print $2}' || echo "0")
        response_time=$(curl -s "http://localhost:8080/metrics" 2>/dev/null | grep "puzzle71_response_time_ms" | tail -n1 | awk '{print $2}' || echo "0")
        error_count=$(curl -s "http://localhost:8080/metrics" 2>/dev/null | grep "puzzle71_errors_total" | tail -n1 | awk '{print $2}' || echo "0")
    fi

    echo "${throughput},${response_time},${error_count}"
}

# Function to check GPU health
check_gpu_health() {
    local gpu_metrics="$1"
    IFS=',' read -r gpu_temp gpu_util memory_used memory_total power_draw <<< "$gpu_metrics"

    local alerts=()

    # Check temperature
    if [[ $gpu_temp -gt $GPU_TEMP_MAX ]]; then
        alerts+=("GPU temperature too high: ${gpu_temp}°C (threshold: ${GPU_TEMP_MAX}°C)")
    fi

    # Check utilization
    if [[ $gpu_util -lt $GPU_UTIL_MIN ]]; then
        alerts+=("GPU utilization low: ${gpu_util}% (threshold: ${GPU_UTIL_MIN}%)")
    fi

    # Check memory usage
    local memory_usage=$((memory_used * 100 / memory_total))
    if [[ $memory_usage -gt $MEMORY_USAGE_MAX ]]; then
        alerts+=("GPU memory usage high: ${memory_usage}% (threshold: ${MEMORY_USAGE_MAX}%)")
    fi

    # Print alerts if any
    if [[ ${#alerts[@]} -gt 0 ]]; then
        for alert in "${alerts[@]}"; do
            log_monitor "WARNING" "$alert"
        done
        return 1
    else
        log_monitor "INFO" "GPU health: Temp=${gpu_temp}°C, Util=${gpu_util}%, Memory=${memory_usage}%, Power=${power_draw}W"
        return 0
    fi
}

# Function to check system health
check_system_health() {
    local system_metrics="$1"
    IFS=',' read -r cpu_usage memory_usage disk_usage load_avg <<< "$system_metrics"

    local alerts=()

    # Check memory usage
    if [[ $(echo "$memory_usage > 80" | bc -l 2>/dev/null || echo "0") -eq 1 ]]; then
        alerts+=("System memory usage high: ${memory_usage}%")
    fi

    # Check disk usage
    if [[ $disk_usage -gt $DISK_USAGE_MAX ]]; then
        alerts+=("Disk usage high: ${disk_usage}% (threshold: ${DISK_USAGE_MAX}%)")
    fi

    # Check load average
    local cpu_count=$(nproc)
    local load_threshold=$(echo "$cpu_count * 2.0" | bc -l 2>/dev/null || echo "4")
    if [[ $(echo "$load_avg > $load_threshold" | bc -l 2>/dev/null || echo "0") -eq 1 ]]; then
        alerts+=("System load high: $load_avg (threshold: $load_threshold)")
    fi

    # Print alerts if any
    if [[ ${#alerts[@]} -gt 0 ]]; then
        for alert in "${alerts[@]}"; do
            log_monitor "WARNING" "$alert"
        done
        return 1
    else
        log_monitor "INFO" "System health: CPU=${cpu_usage}%, Memory=${memory_usage}%, Disk=${disk_usage}%, Load=${load_avg}"
        return 0
    fi
}

# Function to check application performance
check_application_performance() {
    local app_metrics="$1"
    IFS=',' read -r throughput response_time error_count <<< "$app_metrics"

    local alerts=()

    # Check throughput
    if [[ $throughput -lt $THROUGHPUT_MIN_KEYS_PER_SEC ]]; then
        alerts+=("Low throughput: ${throughput} keys/sec (threshold: ${THROUGHPUT_MIN_KEYS_PER_SEC})")
    fi

    # Check response time
    if [[ $response_time -gt $RESPONSE_TIME_MAX_MS ]]; then
        alerts+=("High response time: ${response_time}ms (threshold: ${RESPONSE_TIME_MAX_MS}ms)")
    fi

    # Check error count
    if [[ $error_count -gt 0 ]]; then
        alerts+=("Application errors detected: $error_count")
    fi

    # Print alerts if any
    if [[ ${#alerts[@]} -gt 0 ]]; then
        for alert in "${alerts[@]}"; do
            log_monitor "WARNING" "$alert"
        done
        return 1
    else
        log_monitor "INFO" "Application performance: Throughput=${throughput} keys/sec, Response=${response_time}ms, Errors=$error_count"
        return 0
    fi
}

# Function to export metrics to JSON
export_metrics_json() {
    local gpu_metrics="$1"
    local system_metrics="$2"
    local app_metrics="$3"
    local timestamp=$(date -u +"%Y-%m-%dT%H:%M:%S.%3NZ")

    IFS=',' read -r gpu_temp gpu_util memory_used memory_total power_draw <<< "$gpu_metrics"
    IFS=',' read -r cpu_usage memory_usage disk_usage load_avg <<< "$system_metrics"
    IFS=',' read -r throughput response_time error_count <<< "$app_metrics"

    local gpu_memory_usage=$((memory_used * 100 / memory_total))

    cat > "$METRICS_EXPORT_FILE" << EOF
{
  "timestamp": "$timestamp",
  "monitoring_version": "$MONITOR_VERSION",
  "gpu": {
    "temperature_celsius": $gpu_temp,
    "utilization_percent": $gpu_util,
    "memory_used_mb": $memory_used,
    "memory_total_mb": $memory_total,
    "memory_usage_percent": $gpu_memory_usage,
    "power_draw_watts": $power_draw
  },
  "system": {
    "cpu_usage_percent": $cpu_usage,
    "memory_usage_percent": $memory_usage,
    "disk_usage_percent": $disk_usage,
    "load_average": $load_avg,
    "uptime_seconds": $(cat /proc/uptime | awk '{print $1}' | cut -d. -f1)
  },
  "application": {
    "throughput_keys_per_second": $throughput,
    "response_time_ms": $response_time,
    "error_count": $error_count,
    "process_uptime_seconds": $(pgrep -f "Puzzle71Solver" | xargs -I {} ps -o etimes= -p {} 2>/dev/null | head -n1 || echo "0")
  },
  "alerts": {
    "gpu_temp_threshold": $GPU_TEMP_MAX,
    "gpu_util_threshold": $GPU_UTIL_MIN,
    "memory_threshold": $MEMORY_USAGE_MAX,
    "disk_threshold": $DISK_USAGE_MAX,
    "throughput_threshold": $THROUGHPUT_MIN_KEYS_PER_SEC,
    "response_time_threshold": $RESPONSE_TIME_MAX_MS
  }
}
EOF
}

# Function to send alerts (placeholder for future integration)
send_alert() {
    local alert_type="$1"
    local message="$2"
    local severity="$3"

    # Log alert
    log_monitor "ALERT" "[$severity] $alert_type: $message"

    # Future: Add integration with alerting systems (Slack, Email, PagerDuty, etc.)
    # For now, just log the alert
}

# Function to display dashboard
display_dashboard() {
    local gpu_metrics="$1"
    local system_metrics="$2"
    local app_metrics="$3"

    clear
    echo "=========================================="
    echo "  Puzzle71 Production Monitoring Dashboard"
    echo "  Version: $MONITOR_VERSION"
    echo "  $(date '+%Y-%m-%d %H:%M:%S')"
    echo "=========================================="
    echo ""

    # GPU Metrics
    IFS=',' read -r gpu_temp gpu_util memory_used memory_total power_draw <<< "$gpu_metrics"
    local gpu_memory_usage=$((memory_used * 100 / memory_total))

    echo "🎮 GPU Metrics:"
    echo "   Temperature: ${gpu_temp}°C $([[ $gpu_temp -gt $GPU_TEMP_MAX ]] && echo "🔴" || echo "🟢")"
    echo "   Utilization: ${gpu_util}% $([[ $gpu_util -lt $GPU_UTIL_MIN ]] && echo "🔴" || echo "🟢")"
    echo "   Memory: ${memory_used}MB/${memory_total}MB (${gpu_memory_usage}%) $([[ $gpu_memory_usage -gt $MEMORY_USAGE_MAX ]] && echo "🔴" || echo "🟢")"
    echo "   Power: ${power_draw}W"
    echo ""

    # System Metrics
    IFS=',' read -r cpu_usage memory_usage disk_usage load_avg <<< "$system_metrics"
    echo "💻 System Metrics:"
    echo "   CPU Usage: ${cpu_usage}%"
    echo "   Memory Usage: ${memory_usage}%"
    echo "   Disk Usage: ${disk_usage}% $([[ $disk_usage -gt $DISK_USAGE_MAX ]] && echo "🔴" || echo "🟢")"
    echo "   Load Average: $load_avg"
    echo ""

    # Application Metrics
    IFS=',' read -r throughput response_time error_count <<< "$app_metrics"
    echo "🚀 Application Metrics:"
    echo "   Throughput: ${throughput} keys/sec $([[ $throughput -lt $THROUGHPUT_MIN_KEYS_PER_SEC ]] && echo "🔴" || echo "🟢")"
    echo "   Response Time: ${response_time}ms $([[ $response_time -gt $RESPONSE_TIME_MAX_MS ]] && echo "🔴" || echo "🟢")"
    echo "   Error Count: $error_count $([[ $error_count -gt 0 ]] && echo "🔴" || echo "🟢")"
    echo ""

    # Recent Alerts
    echo "📊 Recent Alerts (last 10):"
    tail -n 10 "$LOG_FILE" | grep -E "(WARNING|ERROR|ALERT)" | while IFS= read -r line; do
        echo "   $line"
    done
    echo ""

    echo "Press Ctrl+C to exit monitoring"
}

# Function to monitor continuously
monitor_continuous() {
    local interval=${1:-30}
    local dashboard=${2:-true}

    log_monitor "INFO" "Starting continuous monitoring with ${interval}s interval"

    while true; do
        # Collect metrics
        local gpu_metrics=$(get_gpu_metrics)
        local system_metrics=$(get_system_metrics)
        local app_metrics=$(get_application_metrics)

        # Run health checks
        local gpu_healthy=true
        local system_healthy=true
        local app_healthy=true

        check_gpu_health "$gpu_metrics" || gpu_healthy=false
        check_system_health "$system_metrics" || system_healthy=false
        check_application_performance "$app_metrics" || app_healthy=false

        # Export metrics
        export_metrics_json "$gpu_metrics" "$system_metrics" "$app_metrics"

        # Send critical alerts if needed
        if [[ "$gpu_healthy" == "false" ]] || [[ "$system_healthy" == "false" ]] || [[ "$app_healthy" == "false" ]]; then
            send_alert "health_check_failure" "One or more health checks failed" "WARNING"
        fi

        # Display dashboard if enabled
        if [[ "$dashboard" == "true" ]]; then
            display_dashboard "$gpu_metrics" "$system_metrics" "$app_metrics"
        fi

        sleep "$interval"
    done
}

# Function to run single health check
run_health_check() {
    log_monitor "INFO" "Running single health check"

    local gpu_metrics=$(get_gpu_metrics)
    local system_metrics=$(get_system_metrics)
    local app_metrics=$(get_application_metrics)

    local overall_healthy=true

    check_gpu_health "$gpu_metrics" || overall_healthy=false
    check_system_health "$system_metrics" || overall_healthy=false
    check_application_performance "$app_metrics" || overall_healthy=false

    export_metrics_json "$gpu_metrics" "$system_metrics" "$app_metrics"

    if [[ "$overall_healthy" == "true" ]]; then
        log_monitor "INFO" "Overall system health: HEALTHY"
        return 0
    else
        log_monitor "WARNING" "Overall system health: UNHEALTHY"
        return 1
    fi
}

# Main function
main() {
    local mode="${1:-continuous}"
    local interval="${2:-30}"
    local dashboard="${3:-true}"

    log_monitor "INFO" "Starting Puzzle71 production monitoring v$MONITOR_VERSION"

    # Ensure monitoring directory exists
    mkdir -p "$(dirname "$METRICS_EXPORT_FILE")"
    mkdir -p "$(dirname "$LOG_FILE")"

    case "$mode" in
        "continuous")
            monitor_continuous "$interval" "$dashboard"
            ;;
        "check")
            run_health_check
            ;;
        *)
            echo "Usage: $0 [continuous|check] [interval_seconds] [show_dashboard]"
            echo "  continuous: Run continuous monitoring (default)"
            echo "  check: Run single health check"
            echo "  interval_seconds: Monitoring interval in seconds (default: 30)"
            echo "  show_dashboard: Show dashboard (true/false, default: true)"
            exit 1
            ;;
    esac
}

# Handle script interruption
trap 'log_monitor "INFO" "Monitoring stopped by user"; exit 0' INT TERM

# Script entry point
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
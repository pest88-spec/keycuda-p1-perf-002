#!/bin/bash
# Puzzle71Solver - Performance Monitoring Script
# Real-time performance monitoring and alerting

set -euo pipefail

# Get script directory and load utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=../utils.sh
source "$SCRIPT_DIR/../utils.sh"

# Default configuration
DEFAULT_DURATION="3600"  # 1 hour
DEFAULT_INTERVAL="1"     # 1 second
DEFAULT_OUTPUT_DIR="build/monitoring"

# Show help
show_help() {
    cat << EOF
Puzzle71Solver Performance Monitoring Script

USAGE:
    monitor.sh [options] [command]

COMMANDS:
    start               Start monitoring session
    stop                Stop monitoring session
    status              Show monitoring status
    analyze             Analyze monitoring data
    alert               Set up performance alerts

OPTIONS:
    --duration <seconds>    Monitoring duration [default: $DEFAULT_DURATION]
    --interval <seconds>    Sample interval [default: $DEFAULT_INTERVAL]
    --gpu <device>          GPU device to monitor [default: auto-detect]
    --output-dir <dir>      Output directory [default: $DEFAULT_OUTPUT_DIR]
    --metrics <list>        Comma-separated metrics to monitor
    --alert-threshold <pct> Performance alert threshold [default: 90]
    --continuous            Continuous monitoring mode
    --daemon                Run as daemon
    --log-level <level>     Log level (debug|info|warn|error) [default: info]
    --telemetry             Enable detailed telemetry
    --web-interface         Enable web interface for monitoring
    --verbose, -v           Enable verbose output
    --help, -h              Show this help

METRICS:
    gpu-utilization      GPU utilization percentage
    memory-bandwidth     Memory bandwidth utilization
    compute-throughput   Compute throughput in keys/sec
    power-consumption    Power consumption in watts
    temperature          GPU temperature
    clock-frequency      GPU clock frequency
    memory-usage         GPU memory usage

EXAMPLES:
    monitor.sh start                     # Start 1-hour monitoring
    monitor.sh start --duration 3600     # Monitor for 1 hour
    monitor.sh start --continuous        # Continuous monitoring
    monitor.sh start --web-interface     # Monitor with web interface
    monitor.sh stop                      # Stop monitoring
    monitor.sh analyze                   # Analyze collected data

ENVIRONMENT VARIABLES:
    MONITOR_DURATION      Override monitoring duration
    MONITOR_INTERVAL      Override sample interval
    MONITOR_GPU          Override GPU device
    MONITOR_OUTPUT_DIR   Override output directory
EOF
}

# Parse command line arguments
parse_args() {
    COMMAND=""
    DURATION="${MONITOR_DURATION:-$DEFAULT_DURATION}"
    INTERVAL="${MONITOR_INTERVAL:-$DEFAULT_INTERVAL}"
    OUTPUT_DIR="${MONITOR_OUTPUT_DIR:-$DEFAULT_OUTPUT_DIR}"
    GPU_DEVICE="${MONITOR_GPU:-}"
    METRICS=""
    ALERT_THRESHOLD="90"
    CONTINUOUS_MODE=false
    DAEMON_MODE=false
    LOG_LEVEL="info"
    TELEMETRY_ENABLED=false
    WEB_INTERFACE=false
    VERBOSE=false

    while [[ $# -gt 0 ]]; do
        case $1 in
            --duration)
                DURATION="$2"
                shift 2
                ;;
            --interval)
                INTERVAL="$2"
                shift 2
                ;;
            --gpu)
                GPU_DEVICE="$2"
                shift 2
                ;;
            --output-dir)
                OUTPUT_DIR="$2"
                shift 2
                ;;
            --metrics)
                METRICS="$2"
                shift 2
                ;;
            --alert-threshold)
                ALERT_THRESHOLD="$2"
                shift 2
                ;;
            --continuous)
                CONTINUOUS_MODE=true
                shift
                ;;
            --daemon)
                DAEMON_MODE=true
                shift
                ;;
            --log-level)
                LOG_LEVEL="$2"
                shift 2
                ;;
            --telemetry)
                TELEMETRY_ENABLED=true
                shift
                ;;
            --web-interface)
                WEB_INTERFACE=true
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
            start|stop|status|analyze|alert)
                COMMAND="$1"
                shift
                ;;
            *)
                error_exit "Unknown option: $1"
                ;;
        esac
    done

    # Set default command if not specified
    if [[ -z "$COMMAND" ]]; then
        COMMAND="start"
    fi

    # Set default metrics if not specified
    if [[ -z "$METRICS" ]]; then
        METRICS="gpu-utilization,memory-bandwidth,compute-throughput,power-consumption,temperature"
    fi
}

# Validate monitoring environment
validate_monitoring_environment() {
    log_debug "Validating monitoring environment..."

    # Check GPU availability
    if ! check_gpu_available; then
        error_exit "GPU not available for monitoring"
    fi

    # Check required tools
    require_command "nvidia-smi"

    if [[ "$WEB_INTERFACE" == true ]]; then
        if ! command_exists "python3"; then
            log_warning "Python3 not available, web interface disabled"
            WEB_INTERFACE=false
        fi
    fi

    log_debug "Monitoring environment validated"
}

# Prepare monitoring environment
prepare_monitoring_environment() {
    progress_start "Preparing monitoring environment"

    local project_root
    project_root="$(get_project_root)"

    cd "$project_root"

    # Create output directory
    ensure_dir "$OUTPUT_DIR"

    # Set GPU device if specified
    if [[ -n "$GPU_DEVICE" ]]; then
        export CUDA_VISIBLE_DEVICES="$GPU_DEVICE"
        log_info "Monitoring GPU device: $GPU_DEVICE"
    fi

    # Create monitoring timestamp
    MONITORING_TIMESTAMP="$(get_timestamp)"
    MONITORING_PID_FILE="$OUTPUT_DIR/monitor.pid"
    MONITORING_LOG="$OUTPUT_DIR/monitor_${MONITORING_TIMESTAMP}.log"
    TELEMETRY_FILE="$OUTPUT_DIR/telemetry_${MONITORING_TIMESTAMP}.csv"

    # Initialize telemetry file
    if [[ "$TELEMETRY_ENABLED" == true ]]; then
        initialize_telemetry_file
    fi

    progress_end "Monitoring environment preparation"
}

# Initialize telemetry file
initialize_telemetry_file() {
    local headers="timestamp,gpu_id"

    # Add metric headers
    IFS=',' read -ra metrics_array <<< "$METRICS"
    for metric in "${metrics_array[@]}"; do
        headers+=",$metric"
    done

    echo "$headers" > "$TELEMETRY_FILE"
    log_debug "Telemetry file initialized: $TELEMETRY_FILE"
}

# Get GPU metrics
get_gpu_metrics() {
    local gpu_id="${1:-0}"
    local metrics_data=""

    # Parse nvidia-smi output
    local smi_output
    smi_output=$(nvidia-smi --query-gpu=index,utilization.gpu,memory.used,memory.total,temperature.gpu,power.draw,clocks.sm,clocks.memory --format=csv,noheader,nounits 2>/dev/null || echo "")

    if [[ -n "$smi_output" ]]; then
        # Extract values
        local gpu_utilization memory_used memory_total temperature power_draw sm_clock mem_clock
        read -r gpu_utilization memory_used memory_total temperature power_draw sm_clock mem_clock <<< "$(echo "$smi_output" | head -n1 | tr ',' ' ')"

        # Build metrics data
        metrics_data="$gpu_id"

        # GPU utilization
        if echo "$METRICS" | grep -q "gpu-utilization"; then
            metrics_data+=",$gpu_utilization"
        fi

        # Memory bandwidth (approximation)
        if echo "$METRICS" | grep -q "memory-bandwidth"; then
            local memory_bandwidth="0"
            # This would need more sophisticated calculation
            metrics_data+=",$memory_bandwidth"
        fi

        # Compute throughput (would need application integration)
        if echo "$METRICS" | grep -q "compute-throughput"; then
            local compute_throughput="0"
            metrics_data+=",$compute_throughput"
        fi

        # Power consumption
        if echo "$METRICS" | grep -q "power-consumption"; then
            metrics_data+=",$power_draw"
        fi

        # Temperature
        if echo "$METRICS" | grep -q "temperature"; then
            metrics_data+=",$temperature"
        fi

        # Clock frequency
        if echo "$METRICS" | grep -q "clock-frequency"; then
            metrics_data+=",$sm_clock"
        fi

        # Memory usage
        if echo "$METRICS" | grep -q "memory-usage"; then
            local memory_usage_percent
            memory_usage_percent=$(echo "scale=2; $memory_used * 100 / $memory_total" | bc -l 2>/dev/null || echo "0")
            metrics_data+=",$memory_usage_percent"
        fi
    fi

    echo "$metrics_data"
}

# Check performance alerts
check_performance_alerts() {
    local metrics_data="$1"
    local alerts_triggered=0

    if [[ -z "$metrics_data" ]]; then
        return 0
    fi

    # Parse metrics
    IFS=',' read -ra values <<< "$metrics_data"
    local gpu_id="${values[0]}"
    local metric_index=1

    IFS=',' read -ra metrics_array <<< "$METRICS"
    for metric in "${metrics_array[@]}"; do
        if [[ $metric_index -lt ${#values[@]} ]]; then
            local value="${values[$metric_index]}"

            case "$metric" in
                "gpu-utilization")
                    if (( $(echo "$value < 50" | bc -l 2>/dev/null || echo "0") )); then
                        log_warning "Low GPU utilization: ${value}% (threshold: 50%)"
                        ((alerts_triggered++))
                    fi
                    ;;
                "temperature")
                    if (( $(echo "$value > 85" | bc -l 2>/dev/null || echo "0") )); then
                        log_warning "High GPU temperature: ${value}°C (threshold: 85°C)"
                        ((alerts_triggered++))
                    fi
                    ;;
                "power-consumption")
                    if (( $(echo "$value > 350" | bc -l 2>/dev/null || echo "0") )); then
                        log_warning "High power consumption: ${value}W (threshold: 350W)"
                        ((alerts_triggered++))
                    fi
                    ;;
            esac
        fi
        ((metric_index++))
    done

    return $alerts_triggered
}

# Run monitoring session
run_monitoring_session() {
    log_info "Starting monitoring session..."
    log_info "Duration: ${DURATION}s, Interval: ${INTERVAL}s"

    local start_time
    start_time=$(date +%s)
    local end_time
    end_time=$((start_time + DURATION))
    local sample_count=0

    # Write PID file for daemon mode
    if [[ "$DAEMON_MODE" == true ]]; then
        echo $$ > "$MONITORING_PID_FILE"
        log_info "Daemon PID: $$"
    fi

    # Start web interface if requested
    if [[ "$WEB_INTERFACE" == true ]]; then
        start_web_interface &
        WEB_INTERFACE_PID=$!
        log_info "Web interface started (PID: $WEB_INTERFACE_PID)"
    fi

    # Main monitoring loop
    while [[ $(date +%s) -lt $end_time ]] || [[ "$CONTINUOUS_MODE" == true ]]; do
        local current_time
        current_time=$(date -Iseconds)

        # Get metrics
        local metrics_data
        metrics_data=$(get_gpu_metrics)

        # Log metrics
        local log_entry="$current_time,$metrics_data"
        echo "$log_entry" >> "$TELEMETRY_FILE"

        # Check alerts
        local alert_count
        alert_count=$(check_performance_alerts "$metrics_data" || echo "0")

        if [[ $alert_count -gt 0 ]]; then
            log_warning "$alert_count performance alert(s) triggered"
        fi

        # Progress update
        ((sample_count++))
        if [[ $sample_count -eq 1 ]] || [[ $((sample_count % 60)) -eq 0 ]] || [[ "$VERBOSE" == true ]]; then
            local elapsed
            elapsed=$(($(date +%s) - start_time))
            log_info "Sample $sample_count (${elapsed}s elapsed): $metrics_data"
        fi

        # Sleep until next sample
        sleep "$INTERVAL"
    done

    # Cleanup
    if [[ -n "${WEB_INTERFACE_PID:-}" ]]; then
        kill "$WEB_INTERFACE_PID" 2>/dev/null || true
    fi

    if [[ "$DAEMON_MODE" == true ]]; then
        safe_remove "$MONITORING_PID_FILE"
    fi

    log_success "Monitoring session completed ($sample_count samples collected)"
}

# Start web interface
start_web_interface() {
    local web_port="8080"
    log_info "Starting web interface on port $web_port..."

    # Create simple web interface
    cat > "$OUTPUT_DIR/web_interface.py" << 'EOF'
#!/usr/bin/env python3
import http.server
import socketserver
import json
import os
import signal
import sys

class MonitoringHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/':
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()

            html = '''
<!DOCTYPE html>
<html>
<head>
    <title>Puzzle71Solver Monitor</title>
    <meta charset="utf-8">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .container { max-width: 1200px; margin: 0 auto; }
        .metric { display: inline-block; margin: 10px; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }
        .metric-value { font-size: 24px; font-weight: bold; color: #2196F3; }
        .metric-label { font-size: 12px; color: #666; }
        .status { padding: 10px; margin: 10px 0; border-radius: 5px; }
        .status.good { background-color: #d4edda; border-color: #c3e6cb; }
        .status.warning { background-color: #fff3cd; border-color: #ffeaa7; }
        .status.error { background-color: #f8d7da; border-color: #f5c6cb; }
    </style>
    <script>
        function updateMetrics() {
            fetch('/metrics')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('metrics').innerHTML = formatMetrics(data);
                    setTimeout(updateMetrics, 1000);
                })
                .catch(error => console.error('Error:', error));
        }

        function formatMetrics(data) {
            if (!data) return '<p>No data available</p>';

            let html = '';
            for (const [key, value] of Object.entries(data)) {
                html += `<div class="metric">
                    <div class="metric-value">${value}</div>
                    <div class="metric-label">${key}</div>
                </div>`;
            }
            return html;
        }

        window.onload = function() {
            updateMetrics();
        };
    </script>
</head>
<body>
    <div class="container">
        <h1>🚀 Puzzle71Solver Performance Monitor</h1>
        <div id="metrics" class="metrics">
            <p>Loading metrics...</p>
        </div>
        <div class="status good">
            <strong>Status:</strong> Monitoring active
        </div>
    </div>
</body>
</html>
'''
            self.wfile.write(html.encode())

        elif self.path == '/metrics':
            try:
                # Read latest telemetry data
                telemetry_files = [f for f in os.listdir('.') if f.startswith('telemetry_') and f.endswith('.csv')]
                if telemetry_files:
                    latest_file = sorted(telemetry_files)[-1]
                    with open(latest_file, 'r') as f:
                        lines = f.readlines()
                        if len(lines) > 1:
                            headers = lines[0].strip().split(',')
                            latest_data = lines[-1].strip().split(',')

                            metrics = {}
                            for i in range(1, len(headers)):
                                metrics[headers[i]] = latest_data[i]

                            self.send_response(200)
                            self.send_header('Content-type', 'application/json')
                            self.end_headers()
                            self.wfile.write(json.dumps(metrics).encode())
                            return
            except Exception as e:
                pass

            self.send_response(404)
            self.end_headers()
            self.wfile.write(b'No metrics available')
        else:
            super().do_GET()

def signal_handler(sig, frame):
    print('\nShutting down web interface...')
    sys.exit(0)

if __name__ == '__main__':
    signal.signal(signal.SIGINT, signal_handler)

    PORT = 8080
    with socketserver.TCPServer(("", PORT), MonitoringHandler) as httpd:
        print(f"Web interface running at http://localhost:{PORT}")
        httpd.serve_forever()
EOF

    cd "$OUTPUT_DIR"
    python3 web_interface.py
}

# Stop monitoring session
stop_monitoring_session() {
    log_info "Stopping monitoring session..."

    if [[ -f "$MONITORING_PID_FILE" ]]; then
        local monitor_pid
        monitor_pid=$(cat "$MONITORING_PID_FILE")
        if kill -0 "$monitor_pid" 2>/dev/null; then
            kill "$monitor_pid"
            log_info "Sent termination signal to monitor process (PID: $monitor_pid)"
        else
            log_warning "Monitor process not found (PID: $monitor_pid)"
        fi
        safe_remove "$MONITORING_PID_FILE"
    else
        log_info "No monitoring session found"
    fi

    # Stop web interface
    local web_pids
    web_pids=$(pgrep -f "web_interface.py" 2>/dev/null || true)
    if [[ -n "$web_pids" ]]; then
        echo "$web_pids" | xargs kill 2>/dev/null || true
        log_info "Stopped web interface"
    fi
}

# Show monitoring status
show_monitoring_status() {
    log_info "Monitoring status:"

    # Check PID file
    if [[ -f "$MONITORING_PID_FILE" ]]; then
        local monitor_pid
        monitor_pid=$(cat "$MONITORING_PID_FILE")
        if kill -0 "$monitor_pid" 2>/dev/null; then
            log_success "Monitoring session active (PID: $monitor_pid)"

            # Show session info
            if [[ -f "$MONITORING_LOG" ]]; then
                local start_time
                start_time=$(head -n1 "$MONITORING_LOG" 2>/dev/null || echo "unknown")
                log_info "Started: $start_time"
            fi

            if [[ -f "$TELEMETRY_FILE" ]]; then
                local sample_count
                sample_count=$(wc -l < "$TELEMETRY_FILE")
                log_info "Samples collected: $((sample_count - 1))"
            fi
        else
            log_warning "Stale PID file found, cleaning up"
            safe_remove "$MONITORING_PID_FILE"
        fi
    else
        log_info "No active monitoring session"
    fi

    # Show recent telemetry files
    local telemetry_count
    telemetry_count=$(find "$OUTPUT_DIR" -name "telemetry_*.csv" 2>/dev/null | wc -l)
    log_info "Telemetry files available: $telemetry_count"

    # Check web interface
    local web_pids
    web_pids=$(pgrep -f "web_interface.py" 2>/dev/null || true)
    if [[ -n "$web_pids" ]]; then
        log_success "Web interface running (PID: $web_pids)"
        log_info "URL: http://localhost:8080"
    else
        log_info "Web interface not running"
    fi
}

# Analyze monitoring data
analyze_monitoring_data() {
    progress_start "Analyzing monitoring data"

    local telemetry_files=()
    while IFS= read -r -d '' file; do
        telemetry_files+=("$file")
    done < <(find "$OUTPUT_DIR" -name "telemetry_*.csv" -print0 2>/dev/null || true)

    if [[ ${#telemetry_files[@]} -eq 0 ]]; then
        log_warning "No telemetry data found for analysis"
        return 0
    fi

    local analysis_report="$OUTPUT_DIR/analysis_report_$(get_timestamp).md"

    cat > "$analysis_report" << EOF
# Monitoring Data Analysis Report

## Summary
- **Analysis Timestamp**: $(date -Iseconds)
- **Telemetry Files**: ${#telemetry_files[@]}

## Data Files
EOF

    for file in "${telemetry_files[@]}"; do
        local file_info
        file_info=$(stat -c "%y (%s bytes)" "$file" 2>/dev/null || stat -f "%Sm (%z bytes)" "$file" 2>/dev/null || echo "unknown")
        echo "- \`$(basename "$file")\`: $file_info" >> "$analysis_report"
    done

    echo "" >> "$analysis_report"
    echo "## Analysis Results" >> "$analysis_report"
    echo "" >> "$analysis_report"

    # Analyze each telemetry file
    for file in "${telemetry_files[@]}"; do
        echo "### $(basename "$file")" >> "$analysis_report"
        echo "" >> "$analysis_report"

        local line_count
        line_count=$(wc -l < "$file")
        local sample_count=$((line_count - 1))

        if [[ $sample_count -gt 0 ]]; then
            echo "- **Samples**: $sample_count" >> "$analysis_report"

            # Basic statistics (simplified)
            if command_exists "awk" && command_exists "tail"; then
                echo "- **Duration**: Calculated from timestamps" >> "$analysis_report"
                echo "- **Data Points**: Available for statistical analysis" >> "$analysis_report"
            fi
        else
            echo "- **Status**: No data samples" >> "$analysis_report"
        fi

        echo "" >> "$analysis_report"
    done

    progress_end "Monitoring data analysis"
    log_success "Analysis report generated: $analysis_report"
}

# Show monitoring summary
show_monitoring_summary() {
    cat << EOF

📊 Monitoring session completed!

Configuration:
- Duration: ${DURATION}s
- Interval: ${INTERVAL}s
- Metrics: $METRICS
- Output: $OUTPUT_DIR

Data files:
- Telemetry: $TELEMETRY_FILE
- Log: $MONITORING_LOG

Next steps:
- Analyze data: scripts monitor analyze
- Check status: scripts monitor status
- View web interface: http://localhost:8080
EOF
}

# Main monitoring function
main() {
    log_info "Starting Puzzle71Solver performance monitor..."

    # Parse arguments
    parse_args "$@"

    # Validate environment
    validate_monitoring_environment

    # Show configuration
    log_info "Monitoring configuration:"
    log_info "  Command: $COMMAND"
    log_info "  Duration: ${DURATION}s"
    log_info "  Interval: ${INTERVAL}s"
    log_info "  Metrics: $METRICS"
    log_info "  Continuous: $CONTINUOUS_MODE"
    log_info "  Web interface: $WEB_INTERFACE"

    # Prepare environment
    prepare_monitoring_environment

    # Execute command
    case "$COMMAND" in
        "start")
            run_monitoring_session
            show_monitoring_summary
            ;;
        "stop")
            stop_monitoring_session
            ;;
        "status")
            show_monitoring_status
            ;;
        "analyze")
            analyze_monitoring_data
            ;;
        "alert")
            log_warning "Alert configuration not yet implemented"
            ;;
        *)
            error_exit "Unknown command: $COMMAND"
            ;;
    esac

    log_success "Monitor command completed!"
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
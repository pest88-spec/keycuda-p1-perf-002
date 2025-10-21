#!/bin/bash

# Puzzle71 Docker Entrypoint Script
# Handles initialization, configuration, and service startup

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PUZZLE71_HOME="${PUZZLE71_HOME:-/opt/puzzle71}"
PUZZLE71_DATA="${PUZZLE71_DATA:-/data/puzzle71}"
PUZZLE71_LOGS="${PUZZLE71_LOGS:-/logs/puzzle71}"
CONFIG_FILE="${PUZZLE71_HOME}/config/production.json"

# Functions
log_info() { echo -e "${BLUE}[INFO]${NC} $(date '+%Y-%m-%d %H:%M:%S') $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $(date '+%Y-%m-%d %H:%M:%S') $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $(date '+%Y-%m-%d %H:%M:%S') $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $(date '+%Y-%m-%d %H:%M:%S') $1"; }

# Check if running as root
check_privileges() {
    if [ "$(id -u)" = "0" ]; then
        log_warning "Running as root. Consider using the puzzle71 user for security."
    fi
}

# Validate environment
validate_environment() {
    log_info "Validating Docker environment..."

    # Check required directories
    local required_dirs=("$PUZZLE71_HOME" "$PUZZLE71_DATA" "$PUZZLE71_LOGS")
    for dir in "${required_dirs[@]}"; do
        if [ ! -d "$dir" ]; then
            log_error "Required directory missing: $dir"
            exit 1
        fi
    done

    # Check GPU availability
    if ! nvidia-smi &> /dev/null; then
        log_warning "NVIDIA GPU not detected. Running in CPU-only mode."
        export CUDA_VISIBLE_DEVICES=""
    else
        local gpu_count=$(nvidia-smi --list-gpus | wc -l)
        log_success "Found $gpu_count GPU(s) available"
    fi

    # Check CUDA installation
    if command -v nvcc &> /dev/null; then
        local cuda_version=$(nvcc --version | grep "release" | awk '{print $6}' | sed 's/,//')
        log_success "CUDA version: $cuda_version"
    else
        log_error "CUDA toolkit not found"
        exit 1
    fi

    # Check application binary
    if [ ! -f "${PUZZLE71_HOME}/bin/Puzzle71Solver" ]; then
        log_error "Application binary not found: ${PUZZLE71_HOME}/bin/Puzzle71Solver"
        exit 1
    fi

    log_success "Environment validation passed"
}

# Initialize configuration
initialize_configuration() {
    log_info "Initializing configuration..."

    # Create production configuration from template if it doesn't exist
    if [ ! -f "$CONFIG_FILE" ] && [ -f "${CONFIG_FILE}.template" ]; then
        log_info "Creating production configuration from template..."

        # Replace template variables
        sed -e "s/{{DEPLOY_TIMESTAMP}}/$(date -u +%Y-%m-%dT%H:%M:%SZ)/g" \
            "${CONFIG_FILE}.template" > "$CONFIG_FILE"

        log_success "Configuration created: $CONFIG_FILE"
    fi

    # Validate configuration
    if [ -f "$CONFIG_FILE" ]; then
        if python3 -c "import json; json.load(open('$CONFIG_FILE'))" 2>/dev/null; then
            log_success "Configuration validation passed"
        else
            log_error "Invalid JSON in configuration file"
            exit 1
        fi
    else
        log_error "Configuration file not found: $CONFIG_FILE"
        exit 1
    fi

    # Create necessary subdirectories
    mkdir -p "${PUZZLE71_DATA}"/{checkpoints,results,temporary}
    mkdir -p "${PUZZLE71_LOGS}"
    mkdir -p "${PUZZLE71_HOME}/monitoring"/{metrics,alerts,health}

    # Set proper permissions
    chown -R puzzle71:puzzle71 "$PUZZLE71_HOME" 2>/dev/null || true
    chown -R puzzle71:puzzle71 "$PUZZLE71_DATA" 2>/dev/null || true
    chown -R puzzle71:puzzle71 "$PUZZLE71_LOGS" 2>/dev/null || true
}

# Setup monitoring
setup_monitoring() {
    log_info "Setting up monitoring..."

    # Create health check script
    cat > "${PUZZLE71_HOME}/monitoring/health_check.py" << 'EOF'
#!/usr/bin/env python3

import json
import os
import sys
import subprocess
import time
from datetime import datetime

def check_process():
    """Check if Puzzle71Solver process is running"""
    try:
        result = subprocess.run(['pgrep', '-f', 'Puzzle71Solver'],
                              capture_output=True, text=True)
        return len(result.stdout.strip()) > 0
    except:
        return False

def check_gpu():
    """Check GPU availability and health"""
    try:
        result = subprocess.run(['nvidia-smi', '--query-gpu=name,temperature.gpu,utilization.gpu,memory.used,memory.total',
                               '--format=csv,noheader,nounits'],
                              capture_output=True, text=True, check=True)
        lines = result.stdout.strip().split('\n')
        gpu_info = []
        for i, line in enumerate(lines):
            if line.strip():
                parts = line.split(', ')
                gpu_info.append({
                    'id': i,
                    'name': parts[0],
                    'temperature': int(parts[1]),
                    'utilization': int(parts[2]),
                    'memory_used': int(parts[3]),
                    'memory_total': int(parts[4])
                })
        return gpu_info
    except Exception as e:
        print(f"GPU check failed: {e}")
        return []

def check_disk_space():
    """Check available disk space"""
    try:
        data_dir = os.environ.get('PUZZLE71_DATA', '/data/puzzle71')
        result = subprocess.run(['df', '-BG', data_dir],
                              capture_output=True, text=True, check=True)
        lines = result.stdout.strip().split('\n')
        if len(lines) >= 2:
            available = int(lines[1].split()[3].replace('G', ''))
            return available
    except:
        pass
    return 0

def main():
    health_status = {
        'timestamp': datetime.utcnow().isoformat() + 'Z',
        'status': 'healthy',
        'checks': {}
    }

    # Check process
    process_running = check_process()
    health_status['checks']['process'] = {
        'status': 'pass' if process_running else 'fail',
        'message': 'Puzzle71Solver is running' if process_running else 'Puzzle71Solver is not running'
    }

    # Check GPU
    gpu_info = check_gpu()
    if gpu_info:
        gpu_status = 'pass'
        gpu_message = f"{len(gpu_info)} GPU(s) available"
        for gpu in gpu_info:
            if gpu['temperature'] > 85:
                gpu_status = 'warn'
                gpu_message += f" (GPU {gpu['id']} high temp: {gpu['temperature']}°C)"
            if gpu['utilization'] < 10:
                gpu_status = 'warn'
                gpu_message += f" (GPU {gpu['id']} low utilization: {gpu['utilization']}%)"
    else:
        gpu_status = 'fail'
        gpu_message = 'No GPUs available'

    health_status['checks']['gpu'] = {
        'status': gpu_status,
        'message': gpu_message,
        'details': gpu_info
    }

    # Check disk space
    disk_gb = check_disk_space()
    disk_status = 'pass' if disk_gb > 5 else 'warn' if disk_gb > 1 else 'fail'
    disk_message = f"{disk_gb}GB available"

    health_status['checks']['disk'] = {
        'status': disk_status,
        'message': disk_message
    }

    # Overall status
    if any(check['status'] == 'fail' for check in health_status['checks'].values()):
        health_status['status'] = 'unhealthy'
    elif any(check['status'] == 'warn' for check in health_status['checks'].values()):
        health_status['status'] = 'degraded'

    # Output health status
    print(json.dumps(health_status, indent=2))

    # Exit with appropriate code
    if health_status['status'] == 'healthy':
        sys.exit(0)
    elif health_status['status'] == 'degraded':
        sys.exit(1)
    else:
        sys.exit(2)

if __name__ == '__main__':
    main()
EOF

    chmod +x "${PUZZLE71_HOME}/monitoring/health_check.py"

    # Create metrics collector
    cat > "${PUZZLE71_HOME}/monitoring/metrics_collector.py" << 'EOF'
#!/usr/bin/env python3

import json
import os
import sys
import time
import subprocess
import psutil
from datetime import datetime, timedelta
try:
    import pynvml
    PYNVML_AVAILABLE = True
    pynvml.nvmlInit()
except ImportError:
    PYNVML_AVAILABLE = False

def collect_gpu_metrics():
    """Collect GPU performance metrics"""
    metrics = []

    if PYNVML_AVAILABLE:
        try:
            device_count = pynvml.nvmlDeviceGetCount()
            for i in range(device_count):
                handle = pynvml.nvmlDeviceGetHandleByIndex(i)

                # Get GPU info
                name = pynvml.nvmlDeviceGetName(handle).decode('utf-8')
                util = pynvml.nvmlDeviceGetUtilizationRates(handle)
                memory_info = pynvml.nvmlDeviceGetMemoryInfo(handle)
                temperature = pynvml.nvmlDeviceGetTemperature(handle, pynvml.NVML_TEMPERATURE_GPU)
                power = pynvml.nvmlDeviceGetPowerUsage(handle) / 1000.0  # Convert mW to W

                metrics.append({
                    'gpu_id': i,
                    'name': name,
                    'utilization_gpu': util.gpu,
                    'utilization_memory': util.memory,
                    'memory_used_mb': memory_info.used // (1024*1024),
                    'memory_total_mb': memory_info.total // (1024*1024),
                    'memory_percent': (memory_info.used / memory_info.total) * 100,
                    'temperature_c': temperature,
                    'power_watts': power
                })
        except Exception as e:
            print(f"Error collecting GPU metrics: {e}", file=sys.stderr)
    else:
        # Fallback to nvidia-smi
        try:
            result = subprocess.run(['nvidia-smi', '--query-gpu=utilization.gpu,memory.used,memory.total,temperature.gpu,power.draw',
                                   '--format=csv,noheader,nounits'],
                                  capture_output=True, text=True, check=True)
            lines = result.stdout.strip().split('\n')
            for i, line in enumerate(lines):
                if line.strip():
                    parts = line.split(', ')
                    metrics.append({
                        'gpu_id': i,
                        'utilization_gpu': int(parts[0]),
                        'memory_used_mb': int(parts[1]),
                        'memory_total_mb': int(parts[2]),
                        'memory_percent': (int(parts[1]) / int(parts[2])) * 100,
                        'temperature_c': float(parts[3]),
                        'power_watts': float(parts[4])
                    })
        except Exception as e:
            print(f"Error collecting GPU metrics with nvidia-smi: {e}", file=sys.stderr)

    return metrics

def collect_system_metrics():
    """Collect system performance metrics"""
    try:
        # CPU metrics
        cpu_percent = psutil.cpu_percent(interval=1)
        cpu_count = psutil.cpu_count()
        load_avg = os.getloadavg()

        # Memory metrics
        memory = psutil.virtual_memory()

        # Disk metrics
        disk = psutil.disk_usage('/')

        return {
            'cpu': {
                'percent': cpu_percent,
                'count': cpu_count,
                'load_1min': load_avg[0],
                'load_5min': load_avg[1],
                'load_15min': load_avg[2]
            },
            'memory': {
                'total_gb': memory.total // (1024**3),
                'used_gb': memory.used // (1024**3),
                'free_gb': memory.available // (1024**3),
                'percent': memory.percent
            },
            'disk': {
                'total_gb': disk.total // (1024**3),
                'used_gb': disk.used // (1024**3),
                'free_gb': disk.free // (1024**3),
                'percent': (disk.used / disk.total) * 100
            }
        }
    except Exception as e:
        print(f"Error collecting system metrics: {e}", file=sys.stderr)
        return {}

def collect_process_metrics():
    """Collect Puzzle71Solver process metrics"""
    try:
        for proc in psutil.process_iter(['pid', 'name', 'cmdline', 'cpu_percent', 'memory_info']):
            if 'Puzzle71Solver' in proc.info['name'] or any('Puzzle71Solver' in arg for arg in proc.info['cmdline'] or []):
                return {
                    'pid': proc.info['pid'],
                    'cpu_percent': proc.info['cpu_percent'],
                    'memory_rss_mb': proc.info['memory_info'].rss // (1024*1024),
                    'memory_vms_mb': proc.info['memory_info'].vms // (1024*1024)
                }
    except Exception as e:
        print(f"Error collecting process metrics: {e}", file=sys.stderr)
    return {}

def main():
    # Configuration
    metrics_dir = os.environ.get('PUZZLE71_HOME', '/opt/puzzle71') + '/monitoring/metrics'
    os.makedirs(metrics_dir, exist_ok=True)

    interval = int(os.environ.get('METRICS_INTERVAL', '30'))

    while True:
        try:
            timestamp = datetime.utcnow().isoformat() + 'Z'

            metrics = {
                'timestamp': timestamp,
                'gpu_metrics': collect_gpu_metrics(),
                'system_metrics': collect_system_metrics(),
                'process_metrics': collect_process_metrics()
            }

            # Write metrics to file
            metrics_file = os.path.join(metrics_dir, f'metrics_{datetime.now().strftime("%Y%m%d")}.jsonl')
            with open(metrics_file, 'a') as f:
                f.write(json.dumps(metrics) + '\n')

            # Clean old metrics files
            cutoff_date = datetime.now() - timedelta(days=30)
            for filename in os.listdir(metrics_dir):
                if filename.startswith('metrics_') and filename.endswith('.jsonl'):
                    try:
                        file_date = datetime.strptime(filename[8:16], '%Y%m%d')
                        if file_date < cutoff_date:
                            os.remove(os.path.join(metrics_dir, filename))
                    except ValueError:
                        continue

            time.sleep(interval)

        except KeyboardInterrupt:
            break
        except Exception as e:
            print(f"Error in metrics collection: {e}", file=sys.stderr)
            time.sleep(interval)

if __name__ == '__main__':
    main()
EOF

    chmod +x "${PUZZLE71_HOME}/monitoring/metrics_collector.py"

    log_success "Monitoring setup completed"
}

# Start services
start_services() {
    log_info "Starting Puzzle71 services..."

    # Start metrics collector in background
    if [ "${ENABLE_METRICS:-true}" = "true" ]; then
        log_info "Starting metrics collector..."
        python3 "${PUZZLE71_HOME}/monitoring/metrics_collector.py" &
        METRICS_PID=$!
        echo $METRICS_PID > "${PUZZLE71_HOME}/monitoring/metrics_collector.pid"
    fi

    # Create PID directory
    mkdir -p "${PUZZLE71_HOME}/run"

    log_success "Services started"
}

# Stop services
stop_services() {
    log_info "Stopping Puzzle71 services..."

    # Stop metrics collector
    if [ -f "${PUZZLE71_HOME}/monitoring/metrics_collector.pid" ]; then
        local pid=$(cat "${PUZZLE71_HOME}/monitoring/metrics_collector.pid")
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
            sleep 2
            kill -9 "$pid" 2>/dev/null || true
        fi
        rm -f "${PUZZLE71_HOME}/monitoring/metrics_collector.pid"
    fi

    # Stop main application
    if [ -f "${PUZZLE71_HOME}/run/puzzle71.pid" ]; then
        local pid=$(cat "${PUZZLE71_HOME}/run/puzzle71.pid")
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
            sleep 5
            kill -9 "$pid" 2>/dev/null || true
        fi
        rm -f "${PUZZLE71_HOME}/run/puzzle71.pid"
    fi

    log_success "Services stopped"
}

# Signal handlers
trap stop_services SIGTERM SIGINT

# Main function
main() {
    log_info "Puzzle71 Docker Container v${PUZZLE71_VERSION:-2.0.0}"
    log_info "=========================================="

    # Initialize
    check_privileges
    validate_environment
    initialize_configuration
    setup_monitoring

    # Start services
    start_services

    # Run the application
    log_info "Starting Puzzle71Solver..."

    # Default arguments
    args=("--config" "$CONFIG_FILE")

    # Add custom arguments from command line
    if [ $# -gt 0 ]; then
        args+=("$@")
    fi

    # Start the main application
    cd "$PUZZLE71_HOME"
    exec "${PUZZLE71_HOME}/bin/Puzzle71Solver" "${args[@]}"
}

# Handle different commands
case "${1:-}" in
    --health-check)
        python3 "${PUZZLE71_HOME}/monitoring/health_check.py"
        ;;
    --start-metrics)
        setup_monitoring
        python3 "${PUZZLE71_HOME}/monitoring/metrics_collector.py" &
        ;;
    --shell)
        exec /bin/bash
        ;;
    "")
        # Default behavior - start the application
        main "$@"
        ;;
    *)
        # Custom command
        exec "$@"
        ;;
esac
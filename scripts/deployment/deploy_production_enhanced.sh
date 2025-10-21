#!/bin/bash

# Puzzle71 Enhanced Production Deployment Script v2.0
# Comprehensive production deployment with advanced monitoring, security, and optimization

set -euo pipefail

# Script metadata
SCRIPT_VERSION="2.0.0"
SCRIPT_NAME="$(basename "$0")"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
DEPLOYMENT_CONFIG_FILE="$PROJECT_ROOT/config/deployment.yaml"

# Configuration defaults
DEPLOYMENT_VERSION="${DEPLOYMENT_VERSION:-2.0.0}"
DOCKER_REGISTRY="${DOCKER_REGISTRY:-puzzle71}"
IMAGE_NAME="${IMAGE_NAME:-puzzle71-solver}"
ENVIRONMENT="${ENVIRONMENT:-production}"
BACKUP_ENABLED="${BACKUP_ENABLED:-true}"
MONITORING_ENABLED="${MONITORING_ENABLED:-true}"
SECURITY_SCAN_ENABLED="${SECURITY_SCAN_ENABLED:-true}"
PERFORMANCE_TEST_ENABLED="${PERFORMANCE_TEST_ENABLED:-true}"

# Colors for enhanced output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color

# Unicode symbols for better visual feedback
SYMBOL_OK="✅"
SYMBOL_ERROR="❌"
SYMBOL_WARNING="⚠️"
SYMBOL_INFO="ℹ️"
SYMBOL_PROGRESS="⏳"
SYMBOL_ROCKET="🚀"
SYMBOL_SHIELD="🛡️"
SYMBOL_CHART="📊"

# Enhanced logging functions
log_with_symbol() {
    local symbol="$1"
    local color="$2"
    local level="$3"
    shift 3
    local message="$*"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo -e "${color}${symbol} [${level}]${NC} ${timestamp} - $message" | tee -a "$PROJECT_ROOT/logs/deployment.log"
}

log_info() { log_with_symbol "$SYMBOL_INFO" "$BLUE" "INFO" "$@"; }
log_success() { log_with_symbol "$SYMBOL_OK" "$GREEN" "SUCCESS" "$@"; }
log_warning() { log_with_symbol "$SYMBOL_WARNING" "$YELLOW" "WARNING" "$@"; }
log_error() { log_with_symbol "$SYMBOL_ERROR" "$RED" "ERROR" "$@"; }
log_progress() { log_with_symbol "$SYMBOL_PROGRESS" "$PURPLE" "PROGRESS" "$@"; }
log_security() { log_with_symbol "$SYMBOL_SHIELD" "$CYAN" "SECURITY" "$@"; }
log_performance() { log_with_symbol "$SYMBOL_CHART" "$CYAN" "PERF" "$@"; }

# Enhanced error handling with context
error_exit() {
    local exit_code=${1:-1}
    local error_message="$2"
    local line_number="${BASH_LINENO[0]}"
    local function_name="${FUNCNAME[1]}"

    log_error "Deployment failed at line $line_number in function $function_name"
    log_error "Error: $error_message"
    log_error "Exit code: $exit_code"

    # Generate error report
    generate_error_report "$exit_code" "$error_message" "$line_number" "$function_name"

    # Cleanup on failure
    cleanup_on_failure

    exit "$exit_code"
}

# Trap for cleanup on script termination
trap 'error_exit 130 "Script interrupted by user"' INT TERM
trap 'error_exit 143 "Script terminated"' TERM

# Function to create necessary directories
setup_directories() {
    log_progress "Setting up deployment directories..."

    local directories=(
        "$PROJECT_ROOT/logs"
        "$PROJECT_ROOT/backups"
        "$PROJECT_ROOT/monitoring/grafana/provisioning/datasources"
        "$PROJECT_ROOT/monitoring/grafana/provisioning/dashboards"
        "$PROJECT_ROOT/monitoring/grafana/dashboards"
        "$PROJECT_ROOT/config/secrets"
        "$PROJECT_ROOT/deployment/cache"
        "$PROJECT_ROOT/deployment/reports"
    )

    for dir in "${directories[@]}"; do
        if [[ ! -d "$dir" ]]; then
            mkdir -p "$dir"
            chown -R puzzle71:puzzle71 "$dir" 2>/dev/null || true
            log_info "Created directory: $dir"
        fi
    done

    log_success "Directory structure ready"
}

# Function to load deployment configuration
load_deployment_config() {
    log_progress "Loading deployment configuration..."

    # Create default configuration if it doesn't exist
    if [[ ! -f "$DEPLOYMENT_CONFIG_FILE" ]]; then
        cat > "$DEPLOYMENT_CONFIG_FILE" << 'EOF'
# Puzzle71 Deployment Configuration v2.0
deployment:
  version: "2.0.0"
  environment: "production"
  backup_enabled: true
  monitoring_enabled: true
  security_scan_enabled: true
  performance_test_enabled: true

docker:
  registry: "puzzle71"
  image_name: "puzzle71-solver"
  build_timeout: 1800  # 30 minutes

gpu:
  target_utilization: 95
  memory_efficiency_target: 90
  max_temperature: 85

security:
  enable_trivy_scan: true
  enable_snyk_scan: false
  vulnerability_threshold: "high"

monitoring:
  prometheus_retention: "30d"
  grafana_admin_password: "puzzle71_admin_password_change_me"

performance:
  benchmark_duration: 3600  # 1 hour
  target_throughput: 1000000  # keys/sec
  regression_threshold: 5  # percentage

EOF
    fi

    # Load configuration using Python YAML parser for safety
    if python3 -c "
import yaml
import os
import sys

try:
    with open('$DEPLOYMENT_CONFIG_FILE', 'r') as f:
        config = yaml.safe_load(f)

    # Export configuration as environment variables
    deployment = config.get('deployment', {})
    docker = config.get('docker', {})
    gpu = config.get('gpu', {})
    security = config.get('security', {})
    monitoring = config.get('monitoring', {})
    performance = config.get('performance', {})

    print(f'export DEPLOYMENT_VERSION=\"{deployment.get(\"version\", \"2.0.0\")}\"')
    print(f'export ENVIRONMENT=\"{deployment.get(\"environment\", \"production\")}\"')
    print(f'export BACKUP_ENABLED=\"{deployment.get(\"backup_enabled\", \"true\")}\"')
    print(f'export MONITORING_ENABLED=\"{deployment.get(\"monitoring_enabled\", \"true\")}\"')
    print(f'export SECURITY_SCAN_ENABLED=\"{deployment.get(\"security_scan_enabled\", \"true\")}\"')
    print(f'export PERFORMANCE_TEST_ENABLED=\"{deployment.get(\"performance_test_enabled\", \"true\")}\"')

    print(f'export DOCKER_REGISTRY=\"{docker.get(\"registry\", \"puzzle71\")}\"')
    print(f'export IMAGE_NAME=\"{docker.get(\"image_name\", \"puzzle71-solver\")}\"')
    print(f'export BUILD_TIMEOUT=\"{docker.get(\"build_timeout\", 1800)}\"')

    print(f'export GPU_TARGET_UTILIZATION=\"{gpu.get(\"target_utilization\", 95)}\"')
    print(f'export GPU_MAX_TEMPERATURE=\"{gpu.get(\"max_temperature\", 85)}\"')

    print(f'export VULNERABILITY_THRESHOLD=\"{security.get(\"vulnerability_threshold\", \"high\")}\"')

    print(f'export PROMETHEUS_RETENTION=\"{monitoring.get(\"prometheus_retention\", \"30d\")}\"')
    print(f'export GRAFANA_ADMIN_PASSWORD=\"{monitoring.get(\"grafana_admin_password\", \"puzzle71_admin_password_change_me\")}\"')

    print(f'export BENCHMARK_DURATION=\"{performance.get(\"benchmark_duration\", 3600)}\"')
    print(f'export TARGET_THROUGHPUT=\"{performance.get(\"target_throughput\", 1000000)}\"')

except Exception as e:
    print(f'echo \"Error loading configuration: {e}\"', file=sys.stderr)
    sys.exit(1)
" > /tmp/deployment_env.sh; then
        source /tmp/deployment_env.sh
        rm /tmp/deployment_env.sh
        log_success "Configuration loaded successfully"
    else
        log_warning "Failed to load YAML configuration, using defaults"
    fi
}

# Function to perform comprehensive prerequisite checks
check_prerequisites() {
    log_progress "Checking deployment prerequisites..."

    local prerequisites_ok=true

    # Check Docker
    if ! command -v docker &> /dev/null; then
        log_error "Docker is not installed or not in PATH"
        prerequisites_ok=false
    else
        local docker_version=$(docker --version | awk '{print $3}' | sed 's/,//')
        log_info "Docker version: $docker_version"
    fi

    # Check Docker Compose
    if ! command -v docker-compose &> /dev/null; then
        log_error "Docker Compose is not installed or not in PATH"
        prerequisites_ok=false
    else
        local compose_version=$(docker-compose --version | awk '{print $3}' | sed 's/,//')
        log_info "Docker Compose version: $compose_version"
    fi

    # Check Docker daemon
    if ! docker info &> /dev/null; then
        log_error "Docker daemon is not running or accessible"
        prerequisites_ok=false
    fi

    # Check NVIDIA Docker support
    if ! docker run --rm --gpus all nvidia/cuda:12.1-base nvidia-smi &> /dev/null; then
        log_error "NVIDIA Docker support is not available"
        prerequisites_ok=false
    fi

    # Check GPU availability and health
    if ! nvidia-smi &> /dev/null; then
        log_error "NVIDIA GPU is not available"
        prerequisites_ok=false
    else
        # Check GPU details
        local gpu_count=$(nvidia-smi --list-gpus | wc -l)
        local gpu_name=$(nvidia-smi --query-gpu=name --format=csv,noheader | head -n1)
        local gpu_memory=$(nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits | head -n1)
        log_info "GPU: $gpu_name ($gpu_count available, ${gpu_memory}MB memory)"

        # Check GPU temperature
        local gpu_temp=$(nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits | head -n1)
        if [[ $gpu_temp -gt ${GPU_MAX_TEMPERATURE:-85} ]]; then
            log_warning "GPU temperature high: ${gpu_temp}°C (threshold: ${GPU_MAX_TEMPERATURE}°C)"
        fi
    fi

    # Check system resources
    local total_memory=$(free -m | awk 'NR==2{print $2}')
    local available_memory=$(free -m | awk 'NR==2{print $7}')
    local memory_usage=$((100 - (available_memory * 100 / total_memory)))

    if [[ $memory_usage -gt 80 ]]; then
        log_warning "System memory usage high: ${memory_usage}%"
    fi

    log_info "System memory: ${available_memory}MB available (${memory_usage}% used)"

    # Check disk space
    local disk_usage=$(df "$PROJECT_ROOT" | awk 'NR==2 {print $5}' | sed 's/%//')
    if [[ $disk_usage -gt 85 ]]; then
        log_warning "Disk space low: ${disk_usage}% used"
    fi

    # Check required directories and files
    local required_dirs=("config" "data" "monitoring")
    local required_files=(
        "CMakeLists.txt"
        "src/KeyhuntCore"
        "docker-compose.production.yml"
    )

    for dir in "${required_dirs[@]}"; do
        if [[ ! -d "$PROJECT_ROOT/$dir" ]]; then
            log_error "Required directory missing: $dir"
            prerequisites_ok=false
        fi
    done

    for file in "${required_files[@]}"; do
        if [[ ! -e "$PROJECT_ROOT/$file" ]]; then
            log_error "Required file missing: $file"
            prerequisites_ok=false
        fi
    done

    if [[ "$prerequisites_ok" == true ]]; then
        log_success "All prerequisites satisfied"
    else
        error_exit 1 "Prerequisites check failed"
    fi
}

# Function to perform security vulnerability scanning
security_scan() {
    if [[ "$SECURITY_SCAN_ENABLED" != "true" ]]; then
        log_info "Security scanning disabled"
        return 0
    fi

    log_security "Performing security vulnerability scanning..."

    # Check if Trivy is available
    if command -v trivy &> /dev/null; then
        log_security "Running Trivy vulnerability scan..."

        local scan_report="$PROJECT_ROOT/deployment/reports/trivy_scan_$(date +%Y%m%d_%H%M%S).json"

        if trivy image --format json --output "$scan_report" \
            --severity "$VULNERABILITY_THRESHOLD" \
            "${DOCKER_REGISTRY}/${IMAGE_NAME}:${DEPLOYMENT_VERSION}" 2>/dev/null; then
            log_success "Trivy scan completed: $scan_report"

            # Analyze scan results
            local vulnerability_count=$(jq '.Results[]?.Vulnerabilities[]? | .VulnerabilityID' "$scan_report" | wc -l || echo "0")
            if [[ $vulnerability_count -gt 0 ]]; then
                log_warning "Found $vulnerability_count vulnerabilities"
            else
                log_success "No vulnerabilities found above threshold: $VULNERABILITY_THRESHOLD"
            fi
        else
            log_warning "Trivy scan failed"
        fi
    else
        log_info "Trivy not available, skipping vulnerability scanning"
    fi

    # Check for secrets in Docker image
    log_security "Scanning for potential secrets..."

    # Simple secret pattern scanning (enhanced as needed)
    local temp_container=$(docker create "${DOCKER_REGISTRY}/${IMAGE_NAME}:${DEPLOYMENT_VERSION}")
    docker cp "$temp_container:/opt/puzzle71" /tmp/puzzle71_scan/

    # Scan for common secret patterns
    local secret_patterns=(
        "password.*=.*['\"][^'\"]{8,}['\"]"
        "api_key.*=.*['\"][^'\"]{20,}['\"]"
        "secret.*=.*['\"][^'\"]{16,}['\"]"
        "token.*=.*['\"][^'\"]{20,}['\"]"
    )

    local secrets_found=false
    for pattern in "${secret_patterns[@]}"; do
        if grep -r -i -E "$pattern" /tmp/puzzle71_scan/ 2>/dev/null; then
            log_warning "Potential secret pattern detected: $pattern"
            secrets_found=true
        fi
    done

    # Cleanup
    rm -rf /tmp/puzzle71_scan/
    docker rm "$temp_container" 2>/dev/null || true

    if [[ "$secrets_found" == false ]]; then
        log_success "No obvious secrets detected in image"
    fi

    log_security "Security scanning completed"
}

# Function to perform performance benchmarking
performance_benchmark() {
    if [[ "$PERFORMANCE_TEST_ENABLED" != "true" ]]; then
        log_info "Performance testing disabled"
        return 0
    fi

    log_performance "Running performance benchmarks..."

    # Create performance test container
    local benchmark_container="puzzle71-benchmark-$(date +%s)"

    # Start benchmark container
    if docker run --name "$benchmark_container" \
        --gpus all \
        --rm -d \
        -v "$PROJECT_ROOT/config:/opt/puzzle71/config:ro" \
        -v "$PROJECT_ROOT/data:/opt/puzzle71/data:ro" \
        -v "$PROJECT_ROOT/benchmark_results:/opt/puzzle71/results" \
        "${DOCKER_REGISTRY}/${IMAGE_NAME}:${DEPLOYMENT_VERSION}" \
        ./benchmark/run_performance_benchmark.sh; then

        log_performance "Benchmark container started: $benchmark_container"

        # Wait for benchmark to complete
        local benchmark_timeout=$((BENCHMARK_DURATION + 300)) # 5 minute buffer
        local elapsed=0

        while [[ $elapsed -lt $benchmark_timeout ]]; do
            local container_status=$(docker inspect -f '{{.State.Status}}' "$benchmark_container" 2>/dev/null || echo "unknown")

            if [[ "$container_status" == "exited" ]]; then
                break
            fi

            sleep 30
            elapsed=$((elapsed + 30))
            log_progress "Benchmark running... (${elapsed}s elapsed)"
        done

        # Stop container if still running
        if docker ps -q -f name="$benchmark_container" | grep -q .; then
            docker stop "$benchmark_container"
        fi

        # Collect benchmark results
        if [[ -f "$PROJECT_ROOT/benchmark_results/performance_report.json" ]]; then
            local actual_throughput=$(jq -r '.throughput_keys_per_second // 0' "$PROJECT_ROOT/benchmark_results/performance_report.json")
            local performance_ratio=$(echo "scale=2; $actual_throughput * 100 / $TARGET_THROUGHPUT" | bc -l)

            log_performance "Benchmark completed:"
            log_performance "  Target throughput: $TARGET_THROUGHPUT keys/sec"
            log_performance "  Actual throughput: $actual_throughput keys/sec"
            log_performance "  Performance ratio: ${performance_ratio}%"

            # Check performance regression threshold
            local regression_threshold=${REGRESSION_THRESHOLD:-5}
            if (( $(echo "$performance_ratio < (100 - $regression_threshold)" | bc -l) )); then
                log_warning "Performance below threshold: ${performance_ratio}% (minimum: $((100 - regression_threshold))%)"
            else
                log_success "Performance meets requirements: ${performance_ratio}%"
            fi
        else
            log_warning "Benchmark results not found"
        fi

    else
        log_error "Failed to start benchmark container"
    fi
}

# Function to backup existing deployment
backup_deployment() {
    if [[ "$BACKUP_ENABLED" != "true" ]]; then
        log_info "Backup disabled"
        return 0
    fi

    log_progress "Creating deployment backup..."

    local backup_timestamp=$(date +%Y%m%d_%H%M%S)
    local backup_dir="$PROJECT_ROOT/backups/deployment_$backup_timestamp"

    mkdir -p "$backup_dir"

    # Backup current Docker images
    log_info "Backing up Docker images..."
    docker images | grep "$IMAGE_NAME" | while read -r repo tag image_id _; do
        if [[ -n "$image_id" ]]; then
            docker save "$repo:$tag" | gzip > "$backup_dir/${repo}_${tag}.tar.gz"
            log_info "Backed up image: $repo:$tag"
        fi
    done

    # Backup configuration files
    log_info "Backing up configuration..."
    cp -r "$PROJECT_ROOT/config" "$backup_dir/" 2>/dev/null || true
    cp -r "$PROJECT_ROOT/data" "$backup_dir/" 2>/dev/null || true
    cp "$PROJECT_ROOT/docker-compose.production.yml" "$backup_dir/" 2>/dev/null || true

    # Backup current running containers info
    if docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" ps &> /dev/null; then
        docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" ps > "$backup_dir/container_status.txt"
        log_info "Backed up container status"
    fi

    # Create backup metadata
    cat > "$backup_dir/backup_metadata.txt" << EOF
Backup created: $(date)
Version: $DEPLOYMENT_VERSION
Environment: $ENVIRONMENT
Docker images: $(docker images | grep "$IMAGE_NAME" | wc -l)
Running containers: $(docker ps | grep "$IMAGE_NAME" | wc -l)
EOF

    log_success "Backup created: $backup_dir"

    # Cleanup old backups (keep last 5)
    find "$PROJECT_ROOT/backups" -name "deployment_*" -type d | sort -r | tail -n +6 | xargs rm -rf 2>/dev/null || true
}

# Function to build optimized production Docker image
build_production_image() {
    log_progress "Building enhanced production Docker image..."

    cd "$PROJECT_ROOT"

    # Create build cache directory
    mkdir -p "$PROJECT_ROOT/.docker-cache"

    # Build with advanced options
    local build_args=(
        --target production
        --build-arg DEPLOYMENT_VERSION="$DEPLOYMENT_VERSION"
        --build-arg BUILD_DATE="$(date -u +'%Y-%m-%dT%H:%M:%SZ')"
        --build-arg VCS_REF="$(git rev-parse HEAD 2>/dev/null || echo 'unknown')"
        --build-arg GIT_COMMIT="$(git log -1 --format='%H' 2>/dev/null || echo 'unknown')"
        --build-arg GIT_BRANCH="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo 'unknown')"
        --cache-from "$DOCKER_REGISTRY/${IMAGE_NAME}:latest"
        --cache-to "$DOCKER_REGISTRY/${IMAGE_NAME}:buildcache"
        --label "org.opencontainers.image.created=$(date -u +'%Y-%m-%dT%H:%M:%SZ')"
        --label "org.opencontainers.image.version=$DEPLOYMENT_VERSION"
        --label "org.opencontainers.image.revision=$(git rev-parse HEAD 2>/dev/null || echo 'unknown')"
        --label "org.opencontainers.image.source=https://github.com/puzzle71/PuzzleKeyhunt"
        --label "com.puzzle71.build.environment=$ENVIRONMENT"
        --label "com.puzzle71.build.timestamp=$(date +%s)"
        --progress=plain
        --tag "${DOCKER_REGISTRY}/${IMAGE_NAME}:${DEPLOYMENT_VERSION}"
        --tag "${DOCKER_REGISTRY}/${IMAGE_NAME}:latest"
        -f Dockerfile.production.enhanced
        .
    )

    log_info "Build command: docker build ${build_args[*]}"

    if timeout "${BUILD_TIMEOUT:-1800}" docker build "${build_args[@]}"; then
        log_success "Production Docker image built successfully"

        # Display image information
        local image_size=$(docker images --format "table {{.Repository}}:{{.Tag}}\t{{.Size}}" | grep "$IMAGE_NAME" | grep "$DEPLOYMENT_VERSION" | awk '{print $2}')
        log_info "Image size: $image_size"

        # Run security scan on new image
        security_scan

    else
        error_exit 1 "Failed to build production Docker image within timeout"
    fi
}

# Function to deploy services with health checks
deploy_services() {
    log_progress "Deploying Puzzle71 production services..."

    cd "$PROJECT_ROOT"

    # Backup existing deployment
    backup_deployment

    # Stop existing services gracefully
    log_info "Stopping existing services..."
    if docker-compose -f docker-compose.production.yml ps -q | grep -q .; then
        docker-compose -f docker-compose.production.yml down --remove-orphans --timeout 300 || \
            log_warning "Some services may not have stopped gracefully"
    fi

    # Clean up unused resources
    log_info "Cleaning up unused Docker resources..."
    docker system prune -f 2>/dev/null || true
    docker volume prune -f 2>/dev/null || true

    # Pull latest images for services
    log_info "Pulling latest service images..."
    docker-compose -f docker-compose.production.yml pull || \
        log_warning "Failed to pull some service images"

    # Start services
    log_info "Starting production services..."
    if docker-compose -f docker-compose.production.yml up -d; then
        log_success "Services deployed successfully"
    else
        error_exit 1 "Failed to start services"
    fi

    # Wait for services to be healthy with enhanced monitoring
    wait_for_services_enhanced
}

# Function to wait for services with enhanced monitoring
wait_for_services_enhanced() {
    log_progress "Waiting for services to become healthy..."

    local max_wait_time=600  # 10 minutes
    local wait_interval=15
    local elapsed_time=0
    local health_check_log="$PROJECT_ROOT/logs/health_checks.log"

    echo "$(date): Starting health checks" > "$health_check_log"

    while [[ $elapsed_time -lt $max_wait_time ]]; do
        local healthy_count=0
        local total_services=5
        local service_status=""

        # Check Puzzle71 Solver
        if docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" exec -T puzzle71-solver /opt/puzzle71/health_check.sh &>> "$health_check_log"; then
            ((healthy_count++))
            service_status="${service_status}puzzle71-solver:OK "
        else
            service_status="${service_status}puzzle71-solver:FAIL "
        fi

        # Check Redis
        if docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" exec -T redis redis-cli ping &>> "$health_check_log"; then
            ((healthy_count++))
            service_status="${service_status}redis:OK "
        else
            service_status="${service_status}redis:FAIL "
        fi

        # Check Prometheus
        if curl -s --max-time 5 http://localhost:9090/metrics &>> "$health_check_log"; then
            ((healthy_count++))
            service_status="${service_status}prometheus:OK "
        else
            service_status="${service_status}prometheus:FAIL "
        fi

        # Check Grafana
        if curl -s --max-time 5 http://localhost:3000/api/health &>> "$health_check_log"; then
            ((healthy_count++))
            service_status="${service_status}grafana:OK "
        else
            service_status="${service_status}grafana:FAIL "
        fi

        # Check Node Exporter
        if curl -s --max-time 5 http://localhost:9100/metrics &>> "$health_check_log"; then
            ((healthy_count++))
            service_status="${service_status}node-exporter:OK "
        else
            service_status="${service_status}node-exporter:FAIL "
        fi

        echo "$(date): Service status - $service_status" >> "$health_check_log"

        if [[ $healthy_count -eq $total_services ]]; then
            log_success "All $total_services services are healthy"
            return 0
        fi

        log_progress "Services healthy: $healthy_count/$total_services ($service_status) - waiting... (${elapsed_time}s/${max_wait_time}s)"
        sleep $wait_interval
        elapsed_time=$((elapsed_time + wait_interval))
    done

    # Generate service status report
    generate_service_status_report

    error_exit 1 "Timeout waiting for services to become healthy"
}

# Function to generate service status report
generate_service_status_report() {
    local report_file="$PROJECT_ROOT/deployment/reports/service_status_$(date +%Y%m%d_%H%M%S).json"
    mkdir -p "$(dirname "$report_file")"

    cat > "$report_file" << EOF
{
  "timestamp": "$(date -u +'%Y-%m-%dT%H:%M:%SZ')",
  "deployment_version": "$DEPLOYMENT_VERSION",
  "environment": "$ENVIRONMENT",
  "services": {
EOF

    # Get detailed status for each service
    local services=("puzzle71-solver" "redis" "prometheus" "grafana" "node-exporter")
    local first=true

    for service in "${services[@]}"; do
        if [[ "$first" == false ]]; then
            echo "," >> "$report_file"
        fi
        first=false

        local status="unknown"
        local uptime="0"
        local cpu_usage="0"
        local memory_usage="0"

        if docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" ps -q "$service" | grep -q .; then
            local container_id=$(docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" ps -q "$service")
            status=$(docker inspect -f '{{.State.Status}}' "$container_id" 2>/dev/null || echo "unknown")
            uptime=$(docker inspect -f '{{.State.StartedAt}}' "$container_id" 2>/dev/null | xargs -I {} date -d {} +%s 2>/dev/null || echo "0")
            cpu_usage=$(docker stats --no-stream --format "{{.CPUPerc}}" "$container_id" 2>/dev/null | sed 's/%//' || echo "0")
            memory_usage=$(docker stats --no-stream --format "{{.MemPerc}}" "$container_id" 2>/dev/null | sed 's/%//' || echo "0")
        fi

        cat >> "$report_file" << EOF
    "$service": {
      "status": "$status",
      "uptime_seconds": $uptime,
      "cpu_usage_percent": $cpu_usage,
      "memory_usage_percent": $memory_usage
    }
EOF
    done

    cat >> "$report_file" << EOF
  }
}
EOF

    log_info "Service status report generated: $report_file"
}

# Function to perform post-deployment verification
verify_deployment() {
    log_progress "Performing comprehensive post-deployment verification..."

    # Check GPU utilization
    local gpu_util=$(nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits | head -n1)
    local gpu_memory=$(nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits | head -n1)

    log_info "GPU utilization: ${gpu_util}%"
    log_info "GPU memory usage: ${gpu_memory}MB"

    if [[ $gpu_util -lt 10 ]]; then
        log_warning "GPU utilization is low: ${gpu_util}%"
    fi

    # Check service endpoints with detailed status
    local endpoints=(
        "http://localhost:8080/metrics:Puzzle71 Solver Metrics"
        "http://localhost:9090/api/v1/status/config:Prometheus Status"
        "http://localhost:3000/api/health:Grafana Health"
        "http://localhost:6379:Redis Health"
        "http://localhost:9100/metrics:Node Exporter Metrics"
    )

    local endpoint_results=()
    for endpoint_info in "${endpoints[@]}"; do
        local endpoint="${endpoint_info%:*}"
        local name="${endpoint_info#*:}"
        local status_code=$(curl -s -o /dev/null -w "%{http_code}" --max-time 10 "$endpoint" || echo "000")

        if [[ "$status_code" =~ ^[23] ]]; then
            log_success "$name is accessible (HTTP $status_code)"
            endpoint_results+=("OK")
        else
            log_warning "$name is not accessible (HTTP $status_code)"
            endpoint_results+=("FAIL")
        fi
    done

    # Check log files
    local log_files=(
        "$PROJECT_ROOT/logs/puzzle71.log"
        "$PROJECT_ROOT/logs/deployment.log"
    )

    for log_file in "${log_files[@]}"; do
        if [[ -f "$log_file" ]]; then
            local log_size=$(stat -c%s "$log_file" 2>/dev/null || echo "0")
            if [[ $log_size -gt 0 ]]; then
                local error_count=$(tail -n 100 "$log_file" 2>/dev/null | grep -i "error\|exception\|failed" | wc -l || echo "0")
                log_info "Log file: $(basename "$log_file") (${log_size} bytes, ${error_count} recent errors)"
            else
                log_warning "Log file is empty: $(basename "$log_file")"
            fi
        else
            log_warning "Log file not found: $(basename "$log_file")"
        fi
    done

    # Run performance benchmark if enabled
    performance_benchmark

    log_success "Post-deployment verification completed"
}

# Function to display deployment status
display_deployment_status() {
    log_info "🚀 Deployment Status Summary"
    echo "=============================="
    echo "Version: $DEPLOYMENT_VERSION"
    echo "Environment: $ENVIRONMENT"
    echo "Timestamp: $(date '+%Y-%m-%d %H:%M:%S')"
    echo ""

    # Show container status
    echo "Service Status:"
    docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" ps
    echo ""

    # Show resource usage
    echo "Resource Usage:"
    if command -v docker-stats &> /dev/null; then
        docker stats --no-stream --format "table {{.Container}}\t{{.CPUPerc}}\t{{.MemUsage}}\t{{.MemPerc}}"
    else
        docker stats --no-stream
    fi
    echo ""

    # Show access URLs
    echo "🔗 Access URLs:"
    echo "  - Puzzle71 Metrics: http://localhost:8080/metrics"
    echo "  - Prometheus: http://localhost:9090"
    echo "  - Grafana: http://localhost:3000 (admin/$GRAFANA_ADMIN_PASSWORD)"
    echo "  - Node Exporter: http://localhost:9100/metrics"
    echo ""

    # Show useful commands
    echo "🛠️  Useful Commands:"
    echo "  - View logs: docker-compose -f $PROJECT_ROOT/docker-compose.production.yml logs -f puzzle71-solver"
    echo "  - Stop services: docker-compose -f $PROJECT_ROOT/docker-compose.production.yml down"
    echo "  - Restart services: docker-compose -f $PROJECT_ROOT/docker-compose.production.yml restart"
    echo "  - Health check: docker-compose -f $PROJECT_ROOT/docker-compose.production.yml exec puzzle71-solver /opt/puzzle71/health_check.sh"
    echo ""

    # Show performance metrics
    if curl -s http://localhost:8080/metrics &> /dev/null; then
        echo "📊 Current Performance:"
        local current_throughput=$(curl -s http://localhost:8080/metrics 2>/dev/null | grep "puzzle71_throughput_keys_per_second" | tail -n1 | awk '{print $2}' || echo "N/A")
        local current_gpu_util=$(nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits | head -n1)
        echo "  - Current throughput: ${current_throughput} keys/sec"
        echo "  - Current GPU utilization: ${current_gpu_util}%"
    fi
}

# Function to generate deployment report
generate_deployment_report() {
    local report_file="$PROJECT_ROOT/deployment/reports/deployment_report_$(date +%Y%m%d_%H%M%S).json"
    mkdir -p "$(dirname "$report_file")"

    cat > "$report_file" << EOF
{
  "deployment": {
    "version": "$DEPLOYMENT_VERSION",
    "environment": "$ENVIRONMENT",
    "timestamp": "$(date -u +'%Y-%m-%dT%H:%M:%SZ')",
    "script_version": "$SCRIPT_VERSION",
    "success": true
  },
  "system": {
    "hostname": "$(hostname)",
    "os": "$(uname -a)",
    "docker_version": "$(docker --version)",
    "gpu_info": "$(nvidia-smi --query-gpu=name,memory.total --format=csv,noheader,nounits | head -n1 | tr '\n' ' ')"
  },
  "configuration": {
    "backup_enabled": $BACKUP_ENABLED,
    "monitoring_enabled": $MONITORING_ENABLED,
    "security_scan_enabled": $SECURITY_SCAN_ENABLED,
    "performance_test_enabled": $PERFORMANCE_TEST_ENABLED
  },
  "services": $(docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" ps --format json | jq '.')
}
EOF

    log_info "Deployment report generated: $report_file"
}

# Function to generate error report
generate_error_report() {
    local exit_code="$1"
    local error_message="$2"
    local line_number="$3"
    local function_name="$4"

    local report_file="$PROJECT_ROOT/deployment/reports/error_report_$(date +%Y%m%d_%H%M%S).json"
    mkdir -p "$(dirname "$report_file")"

    cat > "$report_file" << EOF
{
  "error": {
    "exit_code": $exit_code,
    "message": "$error_message",
    "line_number": $line_number,
    "function_name": "$function_name",
    "timestamp": "$(date -u +'%Y-%m-%dT%H:%M:%SZ')"
  },
  "context": {
    "deployment_version": "$DEPLOYMENT_VERSION",
    "environment": "$ENVIRONMENT",
    "script_version": "$SCRIPT_VERSION",
    "working_directory": "$(pwd)",
    "git_commit": "$(git rev-parse HEAD 2>/dev/null || echo 'unknown')",
    "git_branch": "$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo 'unknown')"
  },
  "system": {
    "hostname": "$(hostname)",
    "os": "$(uname -a)",
    "docker_version": "$(docker --version)",
    "gpu_info": "$(nvidia-smi --query-gpu=name,memory.total --format=csv,noheader,nounits | head -n1 | tr '\n' ' ')"
  }
}
EOF

    log_info "Error report generated: $report_file"
}

# Function to cleanup on failure
cleanup_on_failure() {
    log_warning "Performing cleanup on deployment failure..."

    # Stop any partially started services
    if docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" ps -q | grep -q .; then
        log_info "Stopping partially started services..."
        docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml down --remove-orphans || true
    fi

    # Cleanup failed build artifacts
    if docker images -q "${DOCKER_REGISTRY}/${IMAGE_NAME}:${DEPLOYMENT_VERSION}" | grep -q .; then
        log_info "Removing failed Docker image..."
        docker rmi "${DOCKER_REGISTRY}/${IMAGE_NAME}:${DEPLOYMENT_VERSION}" || true
    fi

    log_info "Cleanup completed"
}

# Main deployment function
main() {
    log_info "🚀 Starting Puzzle71 Enhanced Production Deployment v$SCRIPT_VERSION"
    log_info "Configuration: Version=$DEPLOYMENT_VERSION, Environment=$ENVIRONMENT"

    # Initialize environment
    setup_directories
    load_deployment_config

    # Execute deployment pipeline
    check_prerequisites
    build_production_image
    deploy_services
    verify_deployment

    # Generate reports and status
    generate_deployment_report
    display_deployment_status

    log_success "🎉 Puzzle71 production deployment completed successfully!"
    log_info "Deployment details saved to deployment reports directory"
}

# Script entry point
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
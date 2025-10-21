#!/bin/bash

# Puzzle71 Production Deployment Script v2.0
# Automated production deployment with health checks and monitoring

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
DEPLOYMENT_VERSION="2.0.0"
DOCKER_REGISTRY="puzzle71"
IMAGE_NAME="puzzle71-solver"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $(date '+%Y-%m-%d %H:%M:%S') - $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $(date '+%Y-%m-%d %H:%M:%S') - $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $(date '+%Y-%m-%d %H:%M:%S') - $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $(date '+%Y-%m-%d %H:%M:%S') - $1"
}

# Error handling
error_exit() {
    log_error "$1"
    log_error "Deployment failed!"
    exit 1
}

# Function to check prerequisites
check_prerequisites() {
    log_info "Checking deployment prerequisites..."

    # Check Docker
    if ! command -v docker &> /dev/null; then
        error_exit "Docker is not installed or not in PATH"
    fi

    # Check Docker Compose
    if ! command -v docker-compose &> /dev/null; then
        error_exit "Docker Compose is not installed or not in PATH"
    fi

    # Check Docker daemon
    if ! docker info &> /dev/null; then
        error_exit "Docker daemon is not running"
    fi

    # Check NVIDIA Docker support
    if ! docker run --rm --gpus all nvidia/cuda:12.1-base nvidia-smi &> /dev/null; then
        error_exit "NVIDIA Docker support is not available"
    fi

    # Check GPU availability
    if ! nvidia-smi &> /dev/null; then
        error_exit "NVIDIA GPU is not available"
    fi

    # Check required directories
    local required_dirs=("config" "data" "monitoring")
    for dir in "${required_dirs[@]}"; do
        if [[ ! -d "$PROJECT_ROOT/$dir" ]]; then
            error_exit "Required directory '$dir' does not exist"
        fi
    done

    # Check required files
    local required_files=(
        "config/production.yaml"
        "data/private_ranges.txt"
        "data/target_addresses.txt"
        "monitoring/prometheus.yml"
        "docker-compose.production.yml"
    )
    for file in "${required_files[@]}"; do
        if [[ ! -f "$PROJECT_ROOT/$file" ]]; then
            error_exit "Required file '$file' does not exist"
        fi
    done

    log_success "All prerequisites satisfied"
}

# Function to validate configuration
validate_configuration() {
    log_info "Validating production configuration..."

    # Validate YAML syntax
    if ! python3 -c "import yaml; yaml.safe_load(open('$PROJECT_ROOT/config/production.yaml'))" 2>/dev/null; then
        error_exit "Production configuration YAML syntax is invalid"
    fi

    # Validate Docker Compose syntax
    if ! docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" config &> /dev/null; then
        error_exit "Docker Compose configuration is invalid"
    fi

    # Validate data files
    if [[ ! -s "$PROJECT_ROOT/data/private_ranges.txt" ]]; then
        error_exit "Private ranges file is empty"
    fi

    if [[ ! -s "$PROJECT_ROOT/data/target_addresses.txt" ]]; then
        error_exit "Target addresses file is empty"
    fi

    log_success "Configuration validation passed"
}

# Function to build production Docker image
build_production_image() {
    log_info "Building production Docker image..."

    cd "$PROJECT_ROOT"

    # Build multi-stage Docker image
    docker build \
        --target production \
        -t "${DOCKER_REGISTRY}/${IMAGE_NAME}:${DEPLOYMENT_VERSION}" \
        -t "${DOCKER_REGISTRY}/${IMAGE_NAME}:latest" \
        -f Dockerfile.production \
        . || error_exit "Failed to build production Docker image"

    log_success "Production Docker image built successfully"
}

# Function to setup monitoring configuration
setup_monitoring() {
    log_info "Setting up monitoring configuration..."

    # Create monitoring directories
    mkdir -p "$PROJECT_ROOT/monitoring/grafana/provisioning/datasources"
    mkdir -p "$PROJECT_ROOT/monitoring/grafana/provisioning/dashboards"
    mkdir -p "$PROJECT_ROOT/monitoring/grafana/dashboards"
    mkdir -p "$PROJECT_ROOT/logs"

    # Create Grafana datasource configuration
    cat > "$PROJECT_ROOT/monitoring/grafana/provisioning/datasources/prometheus.yml" << 'EOF'
apiVersion: 1

datasources:
  - name: Prometheus
    type: prometheus
    access: proxy
    url: http://prometheus:9090
    isDefault: true
    editable: true
EOF

    # Create Grafana dashboard configuration
    cat > "$PROJECT_ROOT/monitoring/grafana/provisioning/dashboards/dashboards.yml" << 'EOF'
apiVersion: 1

providers:
  - name: 'default'
    orgId: 1
    folder: ''
    type: file
    disableDeletion: false
    updateIntervalSeconds: 10
    options:
      path: /var/lib/grafana/dashboards
EOF

    # Create basic Prometheus configuration
    cat > "$PROJECT_ROOT/monitoring/prometheus.yml" << 'EOF'
global:
  scrape_interval: 15s
  evaluation_interval: 15s

rule_files:
  # - "first_rules.yml"
  # - "second_rules.yml"

scrape_configs:
  - job_name: 'prometheus'
    static_configs:
      - targets: ['localhost:9090']

  - job_name: 'puzzle71-solver'
    static_configs:
      - targets: ['puzzle71-solver:8080']
    metrics_path: /metrics
    scrape_interval: 5s

  - job_name: 'node-exporter'
    static_configs:
      - targets: ['node-exporter:9100']
EOF

    # Create Redis configuration
    cat > "$PROJECT_ROOT/config/redis.conf" << 'EOF'
bind 0.0.0.0
port 6379
save 900 1
save 300 10
save 60 10000
maxmemory 256mb
maxmemory-policy allkeys-lru
appendonly yes
appendfsync everysec
EOF

    log_success "Monitoring configuration setup completed"
}

# Function to deploy services
deploy_services() {
    log_info "Deploying Puzzle71 production services..."

    cd "$PROJECT_ROOT"

    # Stop existing services
    docker-compose -f docker-compose.production.yml down --remove-orphans || true

    # Pull latest images
    docker-compose -f docker-compose.production.yml pull || log_warning "Failed to pull some images"

    # Start services
    docker-compose -f docker-compose.production.yml up -d || error_exit "Failed to start services"

    log_success "Services deployed successfully"
}

# Function to wait for services to be healthy
wait_for_services() {
    log_info "Waiting for services to become healthy..."

    local max_wait_time=300  # 5 minutes
    local wait_interval=10
    local elapsed_time=0

    while [[ $elapsed_time -lt $max_wait_time ]]; do
        local healthy_count=0
        local total_services=5  # puzzle71-solver, redis, prometheus, grafana, node-exporter

        # Check Puzzle71 Solver
        if docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" exec -T puzzle71-solver /opt/puzzle71/health_check.sh &> /dev/null; then
            ((healthy_count++))
        fi

        # Check Redis
        if docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" exec -T redis redis-cli ping &> /dev/null; then
            ((healthy_count++))
        fi

        # Check Prometheus
        if curl -s http://localhost:9090/metrics &> /dev/null; then
            ((healthy_count++))
        fi

        # Check Grafana
        if curl -s http://localhost:3000/api/health &> /dev/null; then
            ((healthy_count++))
        fi

        # Check Node Exporter
        if curl -s http://localhost:9100/metrics &> /dev/null; then
            ((healthy_count++))
        fi

        if [[ $healthy_count -eq $total_services ]]; then
            log_success "All $total_services services are healthy"
            return 0
        fi

        log_info "Services healthy: $healthy_count/$total_services (waiting...)"
        sleep $wait_interval
        elapsed_time=$((elapsed_time + wait_interval))
    done

    error_exit "Timeout waiting for services to become healthy"
}

# Function to perform post-deployment verification
verify_deployment() {
    log_info "Performing post-deployment verification..."

    # Check GPU utilization
    local gpu_util=$(nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits | head -n1)
    if [[ $gpu_util -lt 10 ]]; then
        log_warning "GPU utilization is low: ${gpu_util}%"
    fi

    # Check service endpoints
    local endpoints=(
        "http://localhost:8080/metrics:Puzzle71 Solver Metrics"
        "http://localhost:9090/targets:Prometheus Targets"
        "http://localhost:3000:Grafana Dashboard"
    )

    for endpoint_info in "${endpoints[@]}"; do
        local endpoint="${endpoint_info%:*}"
        local name="${endpoint_info#*:}"

        if curl -s "$endpoint" &> /dev/null; then
            log_success "$name is accessible"
        else
            log_warning "$name is not accessible at $endpoint"
        fi
    done

    # Check log files
    if [[ -f "$PROJECT_ROOT/logs/puzzle71.log" ]]; then
        local log_size=$(stat -c%s "$PROJECT_ROOT/logs/puzzle71.log" 2>/dev/null || echo "0")
        if [[ $log_size -gt 0 ]]; then
            log_success "Application logging is working"
        else
            log_warning "Application log file is empty"
        fi
    else
        log_warning "Application log file not found"
    fi

    log_success "Post-deployment verification completed"
}

# Function to display deployment status
display_status() {
    log_info "Deployment Status Summary"
    echo "=============================="
    echo "Version: $DEPLOYMENT_VERSION"
    echo "Timestamp: $(date '+%Y-%m-%d %H:%M:%S')"
    echo ""
    echo "Service Status:"
    docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" ps
    echo ""
    echo "Access URLs:"
    echo "  - Puzzle71 Metrics: http://localhost:8080/metrics"
    echo "  - Prometheus: http://localhost:9090"
    echo "  - Grafana: http://localhost:3000 (admin/puzzle71_admin_password)"
    echo "  - Node Exporter: http://localhost:9100/metrics"
    echo ""
    echo "Useful Commands:"
    echo "  - View logs: docker-compose -f $PROJECT_ROOT/docker-compose.production.yml logs -f puzzle71-solver"
    echo "  - Stop services: docker-compose -f $PROJECT_ROOT/docker-compose.production.yml down"
    echo "  - Restart services: docker-compose -f $PROJECT_ROOT/docker-compose.production.yml restart"
}

# Main deployment function
main() {
    log_info "Starting Puzzle71 Production Deployment v$DEPLOYMENT_VERSION"

    # Change to project directory
    cd "$PROJECT_ROOT" || error_exit "Failed to change to project directory"

    # Execute deployment steps
    check_prerequisites
    validate_configuration
    setup_monitoring
    build_production_image
    deploy_services
    wait_for_services
    verify_deployment
    display_status

    log_success "Puzzle71 production deployment completed successfully!"
}

# Handle script interruption
trap 'error_exit "Deployment interrupted by user"' INT TERM

# Script entry point
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
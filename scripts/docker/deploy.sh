#!/bin/bash

# Puzzle71 Docker Deployment Script
# Automated Docker deployment for production environments

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
DEPLOY_DIR="${PROJECT_ROOT}/deployment"
VERSION="2.0.0"

# Default configuration
ENVIRONMENT="${ENVIRONMENT:-production}"
ENABLE_MONITORING="${ENABLE_MONITORING:-false}"
GPU_DEVICES="${GPU_DEVICES:-all}"
COMPOSE_FILE="${DEPLOY_DIR}/docker/docker-compose.yml"

# Functions
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check Docker installation
check_docker() {
    log_info "Checking Docker installation..."

    if ! command -v docker &> /dev/null; then
        log_error "Docker not found. Please install Docker."
        exit 1
    fi

    if ! command -v docker-compose &> /dev/null && ! docker compose version &> /dev/null; then
        log_error "Docker Compose not found. Please install Docker Compose."
        exit 1
    fi

    # Check if Docker daemon is running
    if ! docker info &> /dev/null; then
        log_error "Docker daemon is not running. Please start Docker."
        exit 1
    fi

    # Check NVIDIA Container Toolkit
    if ! docker run --rm --gpus all nvidia/cuda:12.2-base nvidia-smi &> /dev/null; then
        log_error "NVIDIA Container Toolkit not found or GPU support not available."
        log_info "Please install NVIDIA Container Toolkit for GPU support."
        exit 1
    fi

    log_success "Docker installation validated"
}

# Prepare deployment environment
prepare_environment() {
    log_info "Preparing deployment environment..."

    # Create necessary directories
    mkdir -p "${DEPLOY_DIR}/data"
    mkdir -p "${DEPLOY_DIR}/logs"
    mkdir -p "${DEPLOY_DIR}/config"
    mkdir -p "${DEPLOY_DIR}/monitoring/prometheus"
    mkdir -p "${DEPLOY_DIR}/monitoring/grafana/provisioning/datasources"
    mkdir -p "${DEPLOY_DIR}/monitoring/grafana/provisioning/dashboards"
    mkdir -p "${DEPLOY_DIR}/monitoring/grafana/dashboards"

    # Create Prometheus configuration
    cat > "${DEPLOY_DIR}/monitoring/prometheus.yml" << 'EOF'
global:
  scrape_interval: 15s
  evaluation_interval: 15s

rule_files:
  # - "first_rules.yml"
  # - "second_rules.yml"

scrape_configs:
  - job_name: 'puzzle71-solver'
    static_configs:
      - targets: ['puzzle71-solver:8080']
    metrics_path: '/metrics'
    scrape_interval: 30s

  - job_name: 'node-exporter'
    static_configs:
      - targets: ['node-exporter:9100']
    scrape_interval: 30s
EOF

    # Create Grafana datasource configuration
    cat > "${DEPLOY_DIR}/monitoring/grafana/provisioning/datasources/prometheus.yml" << 'EOF'
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
    cat > "${DEPLOY_DIR}/monitoring/grafana/provisioning/dashboards/dashboard.yml" << 'EOF'
apiVersion: 1

providers:
  - name: 'default'
    orgId: 1
    folder: ''
    type: file
    disableDeletion: false
    updateIntervalSeconds: 10
    allowUiUpdates: true
    options:
      path: /var/lib/grafana/dashboards
EOF

    # Create basic Grafana dashboard
    cat > "${DEPLOY_DIR}/monitoring/grafana/dashboards/puzzle71-dashboard.json" << 'EOF'
{
  "dashboard": {
    "id": null,
    "title": "Puzzle71 Solver Dashboard",
    "tags": ["puzzle71"],
    "timezone": "browser",
    "panels": [
      {
        "id": 1,
        "title": "GPU Utilization",
        "type": "stat",
        "targets": [
          {
            "expr": "avg(puzzle71_gpu_utilization)",
            "legendFormat": "Average GPU Utilization"
          }
        ],
        "fieldConfig": {
          "defaults": {
            "unit": "percent",
            "min": 0,
            "max": 100
          }
        },
        "gridPos": {"h": 8, "w": 12, "x": 0, "y": 0}
      },
      {
        "id": 2,
        "title": "Memory Usage",
        "type": "stat",
        "targets": [
          {
            "expr": "avg(puzzle71_memory_usage_percent)",
            "legendFormat": "Memory Usage"
          }
        ],
        "fieldConfig": {
          "defaults": {
            "unit": "percent",
            "min": 0,
            "max": 100
          }
        },
        "gridPos": {"h": 8, "w": 12, "x": 12, "y": 0}
      }
    ],
    "time": {"from": "now-1h", "to": "now"},
    "refresh": "5s"
  }
}
EOF

    # Create environment-specific configuration
    cat > "${DEPLOY_DIR}/config/production.json" << EOF
{
    "version": "$VERSION",
    "deployment": {
        "environment": "$ENVIRONMENT",
        "containerized": true,
        "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
        "build_type": "Release"
    },
    "cuda": {
        "devices": "$GPU_DEVICES",
        "architectures": ["75", "86", "89", "90"],
        "optimization_level": "O3"
    },
    "performance": {
        "target_utilization": 95,
        "batch_size": 2000000
    },
    "monitoring": {
        "telemetry": {
            "enabled": true,
            "interval_seconds": 30,
            "prometheus_enabled": true,
            "prometheus_port": 8080
        }
    },
    "logging": {
        "level": "INFO",
        "console_output": true
    }
}
EOF

    log_success "Deployment environment prepared"
}

# Build Docker image
build_image() {
    log_info "Building Puzzle71 Docker image..."

    cd "$PROJECT_ROOT"

    # Build the Docker image
    docker build \
        -f deployment/docker/Dockerfile.production \
        -t puzzle71-solver:$VERSION \
        -t puzzle71-solver:latest \
        .

    log_success "Docker image built successfully"
}

# Deploy services
deploy_services() {
    log_info "Deploying Puzzle71 services..."

    cd "$DEPLOY_DIR"

    # Set compose command based on available version
    if docker compose version &> /dev/null; then
        COMPOSE_CMD="docker compose"
    else
        COMPOSE_CMD="docker-compose"
    fi

    # Create .env file
    cat > .env << EOF
PUZZLE71_VERSION=$VERSION
ENVIRONMENT=$ENVIRONMENT
GPU_DEVICES=$GPU_DEVICES
ENABLE_METRICS=true
EOF

    # Start main service
    log_info "Starting Puzzle71 solver service..."
    $COMPOSE_CMD -f "$COMPOSE_FILE" up -d puzzle71-solver

    # Start monitoring if enabled
    if [ "$ENABLE_MONITORING" = "true" ]; then
        log_info "Starting monitoring services..."
        $COMPOSE_CMD -f "$COMPOSE_FILE" --profile monitoring up -d
    fi

    log_success "Services deployed successfully"
}

# Wait for services to be healthy
wait_for_services() {
    log_info "Waiting for services to become healthy..."

    cd "$DEPLOY_DIR"

    # Set compose command
    if docker compose version &> /dev/null; then
        COMPOSE_CMD="docker compose"
    else
        COMPOSE_CMD="docker-compose"
    fi

    # Wait for main service
    local max_wait=300
    local wait_time=0

    while [ $wait_time -lt $max_wait ]; do
        if $COMPOSE_CMD -f "$COMPOSE_FILE" ps puzzle71-solver | grep -q "healthy"; then
            log_success "Puzzle71 solver service is healthy"
            break
        fi

        if [ $wait_time -eq 0 ]; then
            log_info "Waiting for Puzzle71 solver service to become healthy..."
        fi

        sleep 10
        wait_time=$((wait_time + 10))

        if [ $wait_time -ge $max_wait ]; then
            log_error "Timeout waiting for Puzzle71 solver service to become healthy"
            return 1
        fi
    done

    # Check monitoring services if enabled
    if [ "$ENABLE_MONITORING" = "true" ]; then
        log_info "Checking monitoring services..."

        # Check Prometheus
        if $COMPOSE_CMD -f "$COMPOSE_FILE" ps prometheus | grep -q "healthy\|Up"; then
            log_success "Prometheus is running"
        else
            log_warning "Prometheus may not be fully ready yet"
        fi

        # Check Grafana
        if $COMPOSE_CMD -f "$COMPOSE_FILE" ps grafana | grep -q "healthy\|Up"; then
            log_success "Grafana is running"
        else
            log_warning "Grafana may not be fully ready yet"
        fi
    fi
}

# Show deployment status
show_status() {
    log_info "Deployment Status:"
    echo "=================="

    cd "$DEPLOY_DIR"

    # Set compose command
    if docker compose version &> /dev/null; then
        COMPOSE_CMD="docker compose"
    else
        COMPOSE_CMD="docker-compose"
    fi

    # Show service status
    echo
    $COMPOSE_CMD -f "$COMPOSE_FILE" ps

    # Show access URLs
    echo
    echo "Access URLs:"
    echo "-----------"
    echo "Puzzle71 Solver: docker exec -it puzzle71-solver /opt/puzzle71/bin/Puzzle71Solver --help"

    if [ "$ENABLE_MONITORING" = "true" ]; then
        echo "Prometheus: http://localhost:9090"
        echo "Grafana: http://localhost:3000 (admin/puzzle71)"
    fi

    # Show logs command
    echo
    echo "Useful Commands:"
    echo "----------------"
    echo "View logs: $COMPOSE_CMD -f $COMPOSE_FILE logs -f puzzle71-solver"
    echo "Stop services: $COMPOSE_CMD -f $COMPOSE_FILE down"
    echo "Restart services: $COMPOSE_CMD -f $COMPOSE_FILE restart puzzle71-solver"

    if [ "$ENABLE_MONITORING" = "true" ]; then
        echo "View monitoring logs: $COMPOSE_CMD -f $COMPOSE_FILE logs -f prometheus grafana"
    fi
}

# Cleanup function
cleanup() {
    log_info "Cleaning up deployment..."

    cd "$DEPLOY_DIR"

    # Set compose command
    if docker compose version &> /dev/null; then
        COMPOSE_CMD="docker compose"
    else
        COMPOSE_CMD="docker-compose"
    fi

    # Stop and remove containers
    $COMPOSE_CMD -f "$COMPOSE_FILE" down -v --remove-orphans 2>/dev/null || true

    # Remove images
    docker rmi puzzle71-solver:$VERSION 2>/dev/null || true
    docker rmi puzzle71-solver:latest 2>/dev/null || true

    # Clean up unused resources
    docker system prune -f

    log_success "Cleanup completed"
}

# Main function
main() {
    log_info "Puzzle71 Docker Deployment v$VERSION"
    log_info "===================================="

    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --environment)
                ENVIRONMENT="$2"
                shift 2
                ;;
            --gpu-devices)
                GPU_DEVICES="$2"
                shift 2
                ;;
            --enable-monitoring)
                ENABLE_MONITORING="true"
                shift
                ;;
            --cleanup)
                cleanup
                exit 0
                ;;
            --status)
                show_status
                exit 0
                ;;
            --help)
                echo "Usage: $0 [options]"
                echo "Options:"
                echo "  --environment ENV      Deployment environment (development|staging|production)"
                echo "  --gpu-devices DEVICES  GPU devices to use (all|0|1,2,etc)"
                echo "  --enable-monitoring    Enable monitoring stack (Prometheus + Grafana)"
                echo "  --cleanup              Remove all containers and images"
                echo "  --status               Show deployment status"
                echo "  --help                 Show this help message"
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done

    # Execute deployment steps
    check_docker
    prepare_environment
    build_image
    deploy_services
    wait_for_services
    show_status

    log_success "Docker deployment completed successfully!"
    log_info "Environment: $ENVIRONMENT"
    log_info "GPU Devices: $GPU_DEVICES"
    log_info "Monitoring: $ENABLE_MONITORING"
}

# Run main function
main "$@"
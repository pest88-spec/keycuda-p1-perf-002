#!/bin/bash

# Puzzle71 Production Update Script v2.0
# Safe production update with rollback capabilities

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
UPDATE_VERSION="2.0.0"
BACKUP_DIR="/opt/puzzle71/backups"
ROLLBACK_DIR="/opt/puzzle71/rollback"

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

# Function to create rollback point
create_rollback_point() {
    local rollback_name="rollback_$(date '+%Y%m%d_%H%M%S')"
    local current_rollback_dir="$ROLLBACK_DIR/$rollback_name"

    log_info "Creating rollback point: $rollback_name"

    mkdir -p "$current_rollback_dir"

    # Backup current running configuration
    docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" config > "$current_rollback_dir/docker-compose.yml"
    cp -r "$PROJECT_ROOT/config" "$current_rollback_dir/"
    cp -r "$PROJECT_ROOT/data" "$current_rollback_dir/"

    # Save current Docker images
    docker images puzzle71-solver -q | head -1 | xargs -I {} docker save {} -o "$current_rollback_dir/puzzle71-solver.tar" 2>/dev/null || true

    # Create rollback manifest
    cat > "$current_rollback_dir/rollback_manifest.json" << EOF
{
  "rollback_name": "$rollback_name",
  "timestamp": "$(date -u +"%Y-%m-%dT%H:%M:%S.%3NZ")",
  "current_version": "2.0.0",
  "backup_location": "$current_rollback_dir",
  "docker_compose_sha256": "$(sha256sum "$current_rollback_dir/docker-compose.yml" | awk '{print $1}')",
  "services_running": $(docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" ps -q | wc -l)
}
EOF

    echo "$current_rollback_dir"
}

# Function to perform health check before update
pre_update_health_check() {
    log_info "Performing pre-update health check..."

    # Check if all services are healthy
    local unhealthy_services=()

    # Check Puzzle71 Solver
    if ! docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" exec -T puzzle71-solver /opt/puzzle71/health_check.sh &> /dev/null; then
        unhealthy_services+=("puzzle71-solver")
    fi

    # Check Redis
    if ! docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" exec -T redis redis-cli ping &> /dev/null; then
        unhealthy_services+=("redis")
    fi

    # Check Prometheus
    if ! curl -s http://localhost:9090/-/healthy &> /dev/null; then
        unhealthy_services+=("prometheus")
    fi

    if [[ ${#unhealthy_services[@]} -gt 0 ]]; then
        log_error "Unhealthy services detected: ${unhealthy_services[*]}"
        log_error "Cannot proceed with update. Please resolve health issues first."
        return 1
    fi

    log_success "All services are healthy - proceeding with update"
    return 0
}

# Function to backup current deployment
backup_current_deployment() {
    log_info "Creating backup of current deployment..."

    local backup_timestamp=$(date '+%Y%m%d_%H%M%S')
    local backup_name="pre_update_backup_${backup_timestamp}"
    local backup_dir="$BACKUP_DIR/$backup_name"

    mkdir -p "$backup_dir"

    # Export Docker volumes
    docker run --rm -v puzzle71_redis-data:/data -v "$backup_dir":/backup alpine tar czf "/backup/redis_data.tar.gz" -C /data . 2>/dev/null || true
    docker run --rm -v puzzle71_prometheus-data:/prometheus -v "$backup_dir":/backup alpine tar czf "/backup/prometheus_data.tar.gz" -C /prometheus . 2>/dev/null || true
    docker run --rm -v puzzle71_grafana-data:/var/lib/grafana -v "$backup_dir":/backup alpine tar czf "/backup/grafana_data.tar.gz" -C /var/lib/grafana . 2>/dev/null || true

    # Backup results and logs
    cp -r "$PROJECT_ROOT/results" "$backup_dir/" 2>/dev/null || true
    cp -r "$PROJECT_ROOT/logs" "$backup_dir/" 2>/dev/null || true

    log_success "Backup created: $backup_dir"
    echo "$backup_dir"
}

# Function to build new Docker image
build_new_image() {
    log_info "Building new Docker image..."

    cd "$PROJECT_ROOT"

    # Build with specific version tag
    docker build \
        --target production \
        -t "puzzle71-solver:${UPDATE_VERSION}" \
        -t "puzzle71-solver:latest-update" \
        -f Dockerfile.production \
        . || log_error "Failed to build new Docker image"

    log_success "New Docker image built successfully"
}

# Function to perform rolling update
perform_rolling_update() {
    log_info "Performing rolling update..."

    cd "$PROJECT_ROOT"

    # Update Docker Compose file to use new image
    sed -i "s|puzzle71-solver:.*|puzzle71-solver:${UPDATE_VERSION}|g" docker-compose.production.yml

    # Pull updated images for other services
    docker-compose -f docker-compose.production.yml pull redis prometheus grafana node-exporter || log_warning "Failed to pull some images"

    # Perform rolling restart
    log_info "Restarting services with new configuration..."

    # Restart dependent services first
    docker-compose -f docker-compose.production.yml up -d redis prometheus grafana node-exporter

    # Wait for dependent services to be healthy
    sleep 30

    # Restart main application
    docker-compose -f docker-compose.production.yml up -d puzzle71-solver

    log_success "Rolling update completed"
}

# Function to wait for services to be healthy after update
wait_for_services_health() {
    log_info "Waiting for services to become healthy after update..."

    local max_wait_time=300  # 5 minutes
    local wait_interval=10
    local elapsed_time=0

    while [[ $elapsed_time -lt $max_wait_time ]]; do
        local healthy_count=0
        local total_services=5

        # Check Puzzle71 Solver
        if docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" exec -T puzzle71-solver /opt/puzzle71/health_check.sh &> /dev/null; then
            ((healthy_count++))
        fi

        # Check Redis
        if docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" exec -T redis redis-cli ping &> /dev/null; then
            ((healthy_count++))
        fi

        # Check Prometheus
        if curl -s http://localhost:9090/-/healthy &> /dev/null; then
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
            log_success "All services are healthy after update"
            return 0
        fi

        log_info "Services healthy: $healthy_count/$total_services (waiting...)"
        sleep $wait_interval
        elapsed_time=$((elapsed_time + wait_interval))
    done

    log_error "Timeout waiting for services to become healthy after update"
    return 1
}

# Function to validate update
validate_update() {
    log_info "Validating update..."

    # Check if new version is running
    local current_version=$(docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" exec -T puzzle71-solver \
        grep -o 'version.*[0-9]\+\.[0-9]\+\.[0-9]\+' /opt/puzzle71/config/production.yaml 2>/dev/null || echo "unknown")

    if [[ "$current_version" == "$UPDATE_VERSION" ]]; then
        log_success "Version validation passed: $current_version"
    else
        log_warning "Version mismatch: expected $UPDATE_VERSION, found $current_version"
    fi

    # Check application functionality
    if curl -s http://localhost:8080/metrics &> /dev/null; then
        log_success "Application endpoint validation passed"
    else
        log_error "Application endpoint validation failed"
        return 1
    fi

    # Check performance metrics
    local throughput=$(curl -s "http://localhost:8080/metrics" 2>/dev/null | grep "puzzle71_throughput_keys_per_second" | tail -n1 | awk '{print $2}' || echo "0")
    if [[ $throughput -gt 100 ]]; then
        log_success "Performance validation passed: ${throughput} keys/sec"
    else
        log_warning "Performance validation warning: ${throughput} keys/sec"
    fi

    log_success "Update validation completed"
    return 0
}

# Function to rollback if update fails
rollback_update() {
    local rollback_dir="$1"

    log_error "Initiating rollback due to update failure..."

    if [[ ! -d "$rollback_dir" ]]; then
        log_error "Rollback directory not found: $rollback_dir"
        return 1
    fi

    log_info "Rolling back to: $rollback_dir"

    # Stop current services
    docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml down

    # Restore previous Docker Compose configuration
    cp "$rollback_dir/docker-compose.yml" "$PROJECT_ROOT/docker-compose.production.yml"

    # Restore previous Docker image
    if [[ -f "$rollback_dir/puzzle71-solver.tar" ]]; then
        docker load -i "$rollback_dir/puzzle71-solver.tar" || log_warning "Failed to restore Docker image"
    fi

    # Restore configuration and data
    cp -r "$rollback_dir/config/"* "$PROJECT_ROOT/config/"
    cp -r "$rollback_dir/data/"* "$PROJECT_ROOT/data/"

    # Restart services
    docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" up -d

    # Wait for services to be healthy
    if wait_for_services_health; then
        log_success "Rollback completed successfully"
        return 0
    else
        log_error "Rollback failed - manual intervention required"
        return 1
    fi
}

# Function to cleanup old rollbacks
cleanup_old_rollbacks() {
    log_info "Cleaning up old rollback points..."

    local keep_count=5
    local deleted_count=0

    while IFS= read -r -d '' rollback_dir; do
        log_info "Deleting old rollback: $(basename "$rollback_dir")"
        rm -rf "$rollback_dir"
        ((deleted_count++))
    done < <(find "$ROLLBACK_DIR" -name "rollback_*" -type d | sort -r | tail -n +$((keep_count + 1)))

    if [[ $deleted_count -gt 0 ]]; then
        log_success "Deleted $deleted_count old rollback points"
    else
        log_info "No old rollbacks to delete"
    fi
}

# Main update function
main() {
    local mode="${1:-update}"
    local skip_backup="${2:-false}"

    log_info "Starting Puzzle71 production update v$UPDATE_VERSION"

    # Ensure required directories exist
    mkdir -p "$BACKUP_DIR" "$ROLLBACK_DIR"

    case "$mode" in
        "update")
            # Pre-update checks
            pre_update_health_check || exit 1

            # Create rollback point
            local rollback_dir=$(create_rollback_point)

            # Backup current deployment
            local backup_dir=""
            if [[ "$skip_backup" != "true" ]]; then
                backup_dir=$(backup_current_deployment)
            fi

            # Perform update
            build_new_image
            perform_rolling_update

            # Validate update
            if wait_for_services_health && validate_update; then
                log_success "Production update completed successfully!"
                log_info "Rollback point: $rollback_dir"
                [[ -n "$backup_dir" ]] && log_info "Backup: $backup_dir"
                cleanup_old_rollbacks
            else
                log_error "Update validation failed - initiating rollback"
                rollback_update "$rollback_dir"
                exit 1
            fi
            ;;

        "rollback")
            local rollback_name="${2:-}"
            if [[ -z "$rollback_name" ]]; then
                # Find most recent rollback
                rollback_name=$(find "$ROLLBACK_DIR" -name "rollback_*" -type d | sort -r | head -n1)
                if [[ -z "$rollback_name" ]]; then
                    log_error "No rollback points found"
                    exit 1
                fi
            fi

            if [[ -d "$ROLLBACK_DIR/$rollback_name" ]]; then
                rollback_update "$ROLLBACK_DIR/$rollback_name"
            else
                log_error "Rollback point not found: $rollback_name"
                exit 1
            fi
            ;;

        "list-rollbacks")
            log_info "Available rollback points:"
            find "$ROLLBACK_DIR" -name "rollback_*" -type d | sort -r | while IFS= read -r rollback_dir; do
                local name=$(basename "$rollback_dir")
                local timestamp=$(jq -r '.timestamp' "$rollback_dir/rollback_manifest.json" 2>/dev/null || echo "unknown")
                echo "  - $name ($timestamp)"
            done
            ;;

        *)
            echo "Usage: $0 [update|rollback|list-rollbacks] [options]"
            echo "  update [skip_backup]: Perform production update (default: false)"
            echo "  rollback [name]: Rollback to specific rollback point"
            echo "  list-rollbacks: List available rollback points"
            exit 1
            ;;
    esac
}

# Handle script interruption
trap 'log_error "Update interrupted by user"; exit 1' INT TERM

# Script entry point
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
#!/bin/bash

# Puzzle71 Production Backup Script v2.0
# Automated backup system for production deployment

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BACKUP_VERSION="2.0.0"
BACKUP_DIR="/opt/puzzle71/backups"
RETENTION_DAYS=30

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

# Function to create backup directory
create_backup_directory() {
    local timestamp=$(date '+%Y%m%d_%H%M%S')
    local backup_name="puzzle71_backup_${timestamp}"
    local current_backup_dir="$BACKUP_DIR/$backup_name"

    mkdir -p "$current_backup_dir"
    echo "$current_backup_dir"
}

# Function to backup configuration files
backup_configuration() {
    local backup_dir="$1"

    log_info "Backing up configuration files..."

    mkdir -p "$backup_dir/config"
    cp -r "$PROJECT_ROOT/config/"* "$backup_dir/config/" 2>/dev/null || true

    # Create checksum
    find "$backup_dir/config" -type f -exec sha256sum {} \; > "$backup_dir/config.sha256"

    log_success "Configuration backup completed"
}

# Function to backup data files
backup_data() {
    local backup_dir="$1"

    log_info "Backing up data files..."

    mkdir -p "$backup_dir/data"
    cp -r "$PROJECT_ROOT/data/"* "$backup_dir/data/" 2>/dev/null || true

    # Create checksum
    find "$backup_dir/data" -type f -exec sha256sum {} \; > "$backup_dir/data.sha256"

    log_success "Data backup completed"
}

# Function to backup results and logs
backup_results_and_logs() {
    local backup_dir="$1"

    log_info "Backing up results and logs..."

    mkdir -p "$backup_dir/results" "$backup_dir/logs"

    # Backup results
    cp -r "$PROJECT_ROOT/results/"* "$backup_dir/results/" 2>/dev/null || true

    # Backup logs
    cp -r "$PROJECT_ROOT/logs/"* "$backup_dir/logs/" 2>/dev/null || true

    # Create checksums
    find "$backup_dir/results" -type f -exec sha256sum {} \; > "$backup_dir/results.sha256" 2>/dev/null || true
    find "$backup_dir/logs" -type f -exec sha256sum {} \; > "$backup_dir/logs.sha256" 2>/dev/null || true

    log_success "Results and logs backup completed"
}

# Function to backup Docker volumes
backup_docker_volumes() {
    local backup_dir="$1"

    log_info "Backing up Docker volumes..."

    mkdir -p "$backup_dir/docker-volumes"

    # Backup Docker Compose configuration
    cp "$PROJECT_ROOT/docker-compose.production.yml" "$backup_dir/docker-volumes/"

    # Export Docker images
    docker images puzzle71-solver -q | head -1 | xargs -I {} docker save {} -o "$backup_dir/docker-volumes/puzzle71-solver.tar" 2>/dev/null || true

    # Backup Docker volumes
    docker run --rm -v puzzle71_redis-data:/data -v "$backup_dir/docker-volumes":/backup alpine tar czf /backup/redis-data.tar.gz -C /data . 2>/dev/null || true
    docker run --rm -v puzzle71_prometheus-data:/prometheus -v "$backup_dir/docker-volumes":/backup alpine tar czf /backup/prometheus-data.tar.gz -C /prometheus . 2>/dev/null || true
    docker run --rm -v puzzle71_grafana-data:/var/lib/grafana -v "$backup_dir/docker-volumes":/backup alpine tar czf /backup/grafana-data.tar.gz -C /var/lib/grafana . 2>/dev/null || true

    log_success "Docker volumes backup completed"
}

# Function to create backup manifest
create_backup_manifest() {
    local backup_dir="$1"
    local backup_name="$2"

    log_info "Creating backup manifest..."

    cat > "$backup_dir/manifest.json" << EOF
{
  "backup_version": "$BACKUP_VERSION",
  "backup_name": "$backup_name",
  "timestamp": "$(date -u +"%Y-%m-%dT%H:%M:%S.%3NZ")",
  "project_root": "$PROJECT_ROOT",
  "backup_directory": "$backup_dir",
  "puzzle71_version": "2.0.0",
  "backup_components": {
    "configuration": true,
    "data": true,
    "results": true,
    "logs": true,
    "docker_volumes": true
  },
  "checksums": {
    "config": "$(test -f '$backup_dir/config.sha256' && sha256sum '$backup_dir/config.sha256' | awk '{print $1}' || echo 'N/A')",
    "data": "$(test -f '$backup_dir/data.sha256' && sha256sum '$backup_dir/data.sha256' | awk '{print $1}' || echo 'N/A')",
    "results": "$(test -f '$backup_dir/results.sha256' && sha256sum '$backup_dir/results.sha256' | awk '{print $1}' || echo 'N/A')",
    "logs": "$(test -f '$backup_dir/logs.sha256' && sha256sum '$backup_dir/logs.sha256' | awk '{print $1}' || echo 'N/A')"
  },
  "system_info": {
    "hostname": "$(hostname)",
    "kernel": "$(uname -r)",
    "docker_version": "$(docker --version)",
    "gpu_info": "$(nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null | head -n1 || echo 'N/A')"
  }
}
EOF

    log_success "Backup manifest created"
}

# Function to compress backup
compress_backup() {
    local backup_dir="$1"
    local backup_name="$2"

    log_info "Compressing backup..."

    cd "$BACKUP_DIR"
    tar czf "${backup_name}.tar.gz" -C "$BACKUP_DIR" "$backup_name"

    # Verify compressed archive
    if tar -tzf "${backup_name}.tar.gz" | head -n1 &> /dev/null; then
        log_success "Backup compression completed: ${backup_name}.tar.gz"

        # Calculate compressed size
        local compressed_size=$(du -h "${backup_name}.tar.gz" | cut -f1)
        log_info "Compressed backup size: $compressed_size"

        # Remove uncompressed directory
        rm -rf "$backup_dir"

        echo "${BACKUP_DIR}/${backup_name}.tar.gz"
    else
        log_error "Backup compression failed"
        echo "$backup_dir"
    fi
}

# Function to cleanup old backups
cleanup_old_backups() {
    log_info "Cleaning up old backups (retention: $RETENTION_DAYS days)..."

    local deleted_count=0
    while IFS= read -r -d '' backup_file; do
        log_info "Deleting old backup: $(basename "$backup_file")"
        rm -f "$backup_file"
        ((deleted_count++))
    done < <(find "$BACKUP_DIR" -name "puzzle71_backup_*.tar.gz" -type f -mtime +$RETENTION_DAYS -print0 2>/dev/null)

    if [[ $deleted_count -gt 0 ]]; then
        log_success "Deleted $deleted_count old backup(s)"
    else
        log_info "No old backups to delete"
    fi
}

# Function to verify backup integrity
verify_backup() {
    local backup_file="$1"

    log_info "Verifying backup integrity..."

    # Test archive integrity
    if tar -tzf "$backup_file" &> /dev/null; then
        log_success "Backup integrity verified"
        return 0
    else
        log_error "Backup integrity check failed"
        return 1
    fi
}

# Main backup function
main() {
    log_info "Starting Puzzle71 production backup v$BACKUP_VERSION"

    # Ensure backup directory exists
    mkdir -p "$BACKUP_DIR"

    # Create backup directory
    local backup_name=$(basename "$BACKUP_DIR")/$(date '+%Y%m%d_%H%M%S')/puzzle71_backup_$(date '+%Y%m%d_%H%M%S')
    local backup_dir=$(create_backup_directory)
    local backup_name_final=$(basename "$backup_dir")

    # Perform backup operations
    backup_configuration "$backup_dir"
    backup_data "$backup_dir"
    backup_results_and_logs "$backup_dir"
    backup_docker_volumes "$backup_dir"
    create_backup_manifest "$backup_dir" "$backup_name_final"

    # Compress backup
    local final_backup_file=$(compress_backup "$backup_dir" "$backup_name_final")

    # Verify backup
    if verify_backup "$final_backup_file"; then
        log_success "Production backup completed successfully: $final_backup_file"

        # Cleanup old backups
        cleanup_old_backups

        # Display backup summary
        local backup_size=$(du -h "$final_backup_file" | cut -f1)
        log_info "Backup summary:"
        log_info "  - File: $final_backup_file"
        log_info "  - Size: $backup_size"
        log_info "  - Retention: $RETENTION_DAYS days"
    else
        log_error "Backup verification failed"
        exit 1
    fi
}

# Handle script interruption
trap 'log_error "Backup interrupted by user"; exit 1' INT TERM

# Script entry point
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
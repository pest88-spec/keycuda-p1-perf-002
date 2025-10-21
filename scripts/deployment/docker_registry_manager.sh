#!/bin/bash

# Puzzle71 Docker Registry Manager v2.0
# Comprehensive Docker image registry management for production deployments

set -euo pipefail

# Script metadata
SCRIPT_VERSION="2.0.0"
SCRIPT_NAME="$(basename "$0")"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Configuration
REGISTRY_CONFIG_FILE="$PROJECT_ROOT/config/registry.yaml"
DEFAULT_REGISTRY="registry.puzzle71.io"
DEFAULT_NAMESPACE="puzzle71"
DEFAULT_TAG="2.0.0"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color

# Unicode symbols
SYMBOL_OK="✅"
SYMBOL_ERROR="❌"
SYMBOL_WARNING="⚠️"
SYMBOL_INFO="ℹ️"
SYMBOL_PROGRESS="⏳"
SYMBOL_UPLOAD="📤"
SYMBOL_DOWNLOAD="📥"
SYMBOL_DELETE="🗑️"
SYMBOL_SEARCH="🔍"

# Logging functions
log() {
    local level="$1"
    shift
    local message="$*"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo -e "${level} [$timestamp] $message"
}

log_info() { log "$SYMBOL_INFO $BLUE[INFO]" "$@"; }
log_success() { log "$SYMBOL_OK $GREEN[SUCCESS]" "$@"; }
log_warning() { log "$SYMBOL_WARNING $YELLOW[WARNING]" "$@"; }
log_error() { log "$SYMBOL_ERROR $RED[ERROR]" "$@"; }
log_progress() { log "$SYMBOL_PROGRESS $PURPLE[PROGRESS]" "$@"; }

# Function to show usage
show_usage() {
    cat << EOF
$SCRIPT_NAME v$SCRIPT_VERSION - Puzzle71 Docker Registry Manager

Usage: $SCRIPT_NAME <command> [options]

Commands:
  init                          Initialize local registry
  build                         Build all Docker images
  push                          Push images to registry
  pull                          Pull images from registry
  list                          List available images
  scan                          Scan images for vulnerabilities
  cleanup                       Clean up unused images
  tag                           Tag images with version
  delete                        Delete images from registry
  info                          Show registry information
  backup                        Backup registry images
  restore                       Restore registry images

Options:
  -r, --registry REGISTRY       Registry URL (default: $DEFAULT_REGISTRY)
  -n, --namespace NAMESPACE     Registry namespace (default: $DEFAULT_NAMESPACE)
  -t, --tag TAG                Image tag (default: $DEFAULT_TAG)
  -f, --force                   Force operation without confirmation
  -v, --verbose                 Verbose output
  -h, --help                    Show this help message

Examples:
  $SCRIPT_NAME init                    # Initialize local registry
  $SCRIPT_NAME build                    # Build all images
  $SCRIPT_NAME push                     # Push to default registry
  $SCRIPT_NAME pull -r docker.io/library # Pull from different registry
  $SCRIPT_NAME scan -t latest          # Scan latest images
  $SCRIPT_NAME cleanup -f              # Force cleanup
  $SCRIPT_NAME info                    # Show registry info

EOF
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -r|--registry)
                REGISTRY_URL="$2"
                shift 2
                ;;
            -n|--namespace)
                REGISTRY_NAMESPACE="$2"
                shift 2
                ;;
            -t|--tag)
                IMAGE_TAG="$2"
                shift 2
                ;;
            -f|--force)
                FORCE_MODE=true
                shift
                ;;
            -v|--verbose)
                VERBOSE_MODE=true
                shift
                ;;
            -h|--help)
                show_usage
                exit 0
                ;;
            init|build|push|pull|list|scan|cleanup|tag|delete|info|backup|restore)
                COMMAND="$1"
                shift
                ;;
            *)
                log_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done

    # Set defaults
    REGISTRY_URL="${REGISTRY_URL:-$DEFAULT_REGISTRY}"
    REGISTRY_NAMESPACE="${REGISTRY_NAMESPACE:-$DEFAULT_NAMESPACE}"
    IMAGE_TAG="${IMAGE_TAG:-$DEFAULT_TAG}"
    FORCE_MODE="${FORCE_MODE:-false}"
    VERBOSE_MODE="${VERBOSE_MODE:-false}"
    COMMAND="${COMMAND:-}"

    if [[ -z "$COMMAND" ]]; then
        log_error "No command specified"
        show_usage
        exit 1
    fi
}

# Function to load registry configuration
load_registry_config() {
    if [[ -f "$REGISTRY_CONFIG_FILE" ]]; then
        log_info "Loading registry configuration from $REGISTRY_CONFIG_FILE"
        if command -v python3 &> /dev/null && python3 -c "import yaml" 2>/dev/null; then
            eval $(python3 -c "
import yaml
with open('$REGISTRY_CONFIG_FILE', 'r') as f:
    config = yaml.safe_load(f)

registry = config.get('registry', {})
print(f'export REGISTRY_URL=\"{registry.get(\"url\", \"$REGISTRY_URL\")}\"')
print(f'export REGISTRY_NAMESPACE=\"{registry.get(\"namespace\", \"$REGISTRY_NAMESPACE\")}\"')
print(f'export REGISTRY_USERNAME=\"{registry.get(\"username\", \"\")}\"')
print(f'export REGISTRY_PASSWORD=\"{registry.get(\"password\", \"\")}\"')
print(f'export REGISTRY_EMAIL=\"{registry.get(\"email\", \"\")}\"')

security = config.get('security', {})
print(f'export SCAN_ENABLED=\"{security.get(\"vulnerability_scan\", \"true\")}\"')
print(f'export SCAN_SEVERITY=\"{security.get(\"severity_threshold\", \"high\")}\"')

build = config.get('build', {})
print(f'export BUILD_TIMEOUT=\"{build.get(\"timeout\", \"1800\")}\"')
print(f'export BUILD_PARALLEL=\"{build.get(\"parallel\", \"true\")}\"')
")
        else
            log_warning "Python YAML library not available, using defaults"
        fi
    else
        log_info "Creating default registry configuration"
        mkdir -p "$(dirname "$REGISTRY_CONFIG_FILE")"
        cat > "$REGISTRY_CONFIG_FILE" << EOF
# Puzzle71 Docker Registry Configuration v2.0
registry:
  url: "$REGISTRY_URL"
  namespace: "$REGISTRY_NAMESPACE"
  username: ""
  password: ""
  email: ""

security:
  vulnerability_scan: true
  severity_threshold: "high"

build:
  timeout: 1800  # 30 minutes
  parallel: true

images:
  main:
    name: "puzzle71-solver"
    dockerfile: "Dockerfile.production.enhanced"
    build_args:
      - DEPLOYMENT_VERSION=2.0.0
      - BUILD_DATE=\$(date -u +'%Y-%m-%dT%H:%M:%SZ')

  benchmark:
    name: "puzzle71-benchmark"
    dockerfile: "Dockerfile.production.enhanced"
    target: "benchmark"

  development:
    name: "puzzle71-dev"
    dockerfile: "Dockerfile.production.enhanced"
    target: "development"

storage:
  local_registry:
    enabled: false
    port: 5000
    data_dir: "./registry-data"

  backup:
    enabled: true
    compression: "gzip"
    retention_days: 30
EOF
    fi
}

# Function to initialize local registry
init_local_registry() {
    log_progress "Initializing local Docker registry..."

    # Check if local registry is enabled
    if ! python3 -c "import yaml; print(yaml.safe_load(open('$REGISTRY_CONFIG_FILE')).get('storage', {}).get('local_registry', {}).get('enabled', False))" 2>/dev/null; then
        log_info "Local registry disabled in configuration"
        return 0
    fi

    # Get local registry configuration
    local registry_port=$(python3 -c "import yaml; print(yaml.safe_load(open('$REGISTRY_CONFIG_FILE')).get('storage', {}).get('local_registry', {}).get('port', 5000))" 2>/dev/null || echo "5000")
    local registry_data=$(python3 -c "import yaml; print(yaml.safe_load(open('$REGISTRY_CONFIG_FILE')).get('storage', {}).get('local_registry', {}).get('data_dir', './registry-data'))" 2>/dev/null || echo "./registry-data")

    # Create registry data directory
    mkdir -p "$registry_data"
    chmod 755 "$registry_data"

    # Start local registry container
    if docker ps -q -f name="puzzle71-registry" | grep -q .; then
        log_info "Local registry already running"
    else
        log_info "Starting local registry on port $registry_port"
        docker run -d \
            --name puzzle71-registry \
            --restart unless-stopped \
            -p "$registry_port:5000" \
            -v "$registry_data:/var/lib/registry" \
            -e REGISTRY_STORAGE_DELETE_ENABLED=true \
            registry:2.8

        # Wait for registry to start
        sleep 5

        if curl -s "http://localhost:$registry_port/v2/_catalog" &> /dev/null; then
            log_success "Local registry started successfully on port $registry_port"
        else
            log_error "Failed to start local registry"
            return 1
        fi
    fi

    # Update registry URL for local registry
    REGISTRY_URL="localhost:$registry_port"
    log_info "Using local registry: $REGISTRY_URL"
}

# Function to build Docker images
build_images() {
    log_progress "Building Puzzle71 Docker images..."

    cd "$PROJECT_ROOT"

    # Get build configuration
    local build_timeout=$(python3 -c "import yaml; print(yaml.safe_load(open('$REGISTRY_CONFIG_FILE')).get('build', {}).get('timeout', 1800))" 2>/dev/null || echo "1800")
    local build_parallel=$(python3 -c "import yaml; print(yaml.safe_load(open('$REGISTRY_CONFIG_FILE')).get('build', {}).get('parallel', 'true'))" 2>/dev/null || echo "true")

    # Get image configurations
    local images=$(python3 -c "
import yaml
with open('$REGISTRY_CONFIG_FILE', 'r') as f:
    config = yaml.safe_load(f)

for name, img_config in config.get('images', {}).items():
    dockerfile = img_config.get('dockerfile', 'Dockerfile.production.enhanced')
    target = img_config.get('target', '')
    print(f'{name}:{dockerfile}:{target}')
" 2>/dev/null || echo "main:Dockerfile.production.enhanced:")

    # Build each image
    while IFS=':' read -r name dockerfile target; do
        [[ -z "$name" ]] && continue

        local image_name="$REGISTRY_NAMESPACE/${name}"
        local full_image_name="$REGISTRY_URL/$image_name:$IMAGE_TAG"
        local latest_image_name="$REGISTRY_URL/$image_name:latest"

        log_info "Building image: $name"

        # Prepare build command
        local build_args=(
            --build-arg DEPLOYMENT_VERSION="$IMAGE_TAG"
            --build-arg BUILD_DATE="$(date -u +'%Y-%m-%dT%H:%M:%SZ')"
            --build-arg VCS_REF="$(git rev-parse HEAD 2>/dev/null || echo 'unknown')"
            --build-arg GIT_COMMIT="$(git log -1 --format='%H' 2>/dev/null || echo 'unknown')"
            --build-arg GIT_BRANCH="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo 'unknown')"
        )

        # Add target if specified
        if [[ -n "$target" ]]; then
            build_args+=(--target "$target")
        fi

        # Build with tags
        build_args+=(
            --tag "$full_image_name"
            --tag "$latest_image_name"
        )

        # Add build args from configuration
        local img_build_args=$(python3 -c "
import yaml
with open('$REGISTRY_CONFIG_FILE', 'r') as f:
    config = yaml.safe_load(f)
args = config.get('images', {}).get('$name', {}).get('build_args', [])
for arg in args:
    print(f'--build-arg {arg}')
" 2>/dev/null || echo "")

        if [[ -n "$img_build_args" ]]; then
            while read -r arg; do
                [[ -n "$arg" ]] && build_args+=($arg)
            done <<< "$img_build_args"
        fi

        # Execute build
        log_info "Build command: docker build ${build_args[*]} -f $dockerfile ."

        if [[ "$VERBOSE_MODE" == true ]]; then
            set -x
        fi

        if timeout "$build_timeout" docker build "${build_args[@]}" -f "$dockerfile" .; then
            log_success "Built image: $full_image_name"

            # Get image size
            local image_size=$(docker images --format "{{.Size}}" "$full_image_name" 2>/dev/null || echo "unknown")
            log_info "Image size: $image_size"
        else
            log_error "Failed to build image: $full_image_name"
            return 1
        fi

        if [[ "$VERBOSE_MODE" == true ]]; then
            set +x
        fi

    done <<< "$images"

    log_success "All Docker images built successfully"
}

# Function to push images to registry
push_images() {
    log_progress "Pushing images to registry: $REGISTRY_URL"

    # Check if registry authentication is needed
    if [[ -n "${REGISTRY_USERNAME:-}" && -n "${REGISTRY_PASSWORD:-}" ]]; then
        log_info "Authenticating with registry..."
        echo "$REGISTRY_PASSWORD" | docker login "$REGISTRY_URL" -u "$REGISTRY_PASSWORD" --password-stdin
    fi

    # Get images to push
    local images=$(python3 -c "
import yaml
with open('$REGISTRY_CONFIG_FILE', 'r') as f:
    config = yaml.safe_load(f)

for name in config.get('images', {}).keys():
    print(f'{REGISTRY_NAMESPACE}/{name}')
" 2>/dev/null || echo "$REGISTRY_NAMESPACE/puzzle71-solver")

    while IFS= read -r image_name; do
        [[ -z "$image_name" ]] && continue

        local full_image_name="$REGISTRY_URL/$image_name:$IMAGE_TAG"
        local latest_image_name="$REGISTRY_URL/$image_name:latest"

        log_info "Pushing image: $image_name"

        # Check if image exists locally
        if ! docker images --format "{{.Repository}}:{{.Tag}}" | grep -q "^${full_image_name//\//\\/}$"; then
            log_warning "Image not found locally: $full_image_name"
            continue
        fi

        # Push versioned tag
        log_progress "Pushing $full_image_name..."
        if docker push "$full_image_name"; then
            log_success "Pushed: $full_image_name"
        else
            log_error "Failed to push: $full_image_name"
            return 1
        fi

        # Push latest tag
        log_progress "Pushing $latest_image_name..."
        if docker push "$latest_image_name"; then
            log_success "Pushed: $latest_image_name"
        else
            log_error "Failed to push: $latest_image_name"
            return 1
        fi

    done <<< "$images"

    log_success "All images pushed to registry successfully"
}

# Function to pull images from registry
pull_images() {
    log_progress "Pulling images from registry: $REGISTRY_URL"

    # Check if registry authentication is needed
    if [[ -n "${REGISTRY_USERNAME:-}" && -n "${REGISTRY_PASSWORD:-}" ]]; then
        log_info "Authenticating with registry..."
        echo "$REGISTRY_PASSWORD" | docker login "$REGISTRY_URL" -u "$REGISTRY_PASSWORD" --password-stdin
    fi

    # Get images to pull
    local images=$(python3 -c "
import yaml
with open('$REGISTRY_CONFIG_FILE', 'r') as f:
    config = yaml.safe_load(f)

for name in config.get('images', {}).keys():
    print(f'{REGISTRY_NAMESPACE}/{name}')
" 2>/dev/null || echo "$REGISTRY_NAMESPACE/puzzle71-solver")

    while IFS= read -r image_name; do
        [[ -z "$image_name" ]] && continue

        local full_image_name="$REGISTRY_URL/$image_name:$IMAGE_TAG"

        log_info "Pulling image: $image_name"

        if docker pull "$full_image_name"; then
            log_success "Pulled: $full_image_name"

            # Tag as latest for local use
            docker tag "$full_image_name" "$image_name:latest"
        else
            log_error "Failed to pull: $full_image_name"
            return 1
        fi

    done <<< "$images"

    log_success "All images pulled from registry successfully"
}

# Function to list available images
list_images() {
    log_info "Listing images in registry: $REGISTRY_URL"

    # List local images
    log_info "Local images:"
    docker images --format "table {{.Repository}}\t{{.Tag}}\t{{.Size}}\t{{.CreatedAt}}" | grep "$REGISTRY_NAMESPACE" || log_info "No local images found"

    # List remote images (if registry is accessible)
    if curl -s "http://${REGISTRY_URL#https://}/v2/_catalog" &> /dev/null; then
        log_info "Remote images in registry:"
        curl -s "http://${REGISTRY_URL#https://}/v2/_catalog" | jq -r '.repositories[]' 2>/dev/null | grep "$REGISTRY_NAMESPACE" | while read -r repo; do
            if [[ -n "$repo" ]]; then
                # Get tags for this repository
                local tags=$(curl -s "http://${REGISTRY_URL#https://}/v2/$repo/tags/list" | jq -r '.tags[]' 2>/dev/null || echo "")
                if [[ -n "$tags" ]]; then
                    while read -r tag; do
                        [[ -n "$tag" ]] && echo "  $REGISTRY_URL/$repo:$tag"
                    done <<< "$tags"
                fi
            fi
        done
    else
        log_warning "Cannot access remote registry catalog"
    fi
}

# Function to scan images for vulnerabilities
scan_images() {
    log_progress "Scanning images for vulnerabilities..."

    # Check if scanning is enabled
    local scan_enabled=$(python3 -c "import yaml; print(yaml.safe_load(open('$REGISTRY_CONFIG_FILE')).get('security', {}).get('vulnerability_scan', 'true'))" 2>/dev/null || echo "true")

    if [[ "$scan_enabled" != "true" ]]; then
        log_info "Vulnerability scanning disabled in configuration"
        return 0
    fi

    # Get severity threshold
    local scan_severity=$(python3 -c "import yaml; print(yaml.safe_load(open('$REGISTRY_CONFIG_FILE')).get('security', {}).get('severity_threshold', 'high'))" 2>/dev/null || echo "high")

    # Get images to scan
    local images=$(docker images --format "{{.Repository}}:{{.Tag}}" | grep "$REGISTRY_NAMESPACE" | grep "$IMAGE_TAG" || echo "")

    if [[ -z "$images" ]]; then
        log_warning "No images found to scan"
        return 0
    fi

    # Check if Trivy is available
    if ! command -v trivy &> /dev/null; then
        log_warning "Trivy not found. Install with: apt-get install trivy"
        return 0
    fi

    # Create scan results directory
    local scan_dir="$PROJECT_ROOT/security/scans"
    mkdir -p "$scan_dir"

    # Scan each image
    while IFS= read -r image; do
        [[ -z "$image" ]] && continue

        log_info "Scanning image: $image"

        local scan_file="$scan_dir/$(echo "$image" | tr '/' '_')_scan_$(date +%Y%m%d_%H%M%S).json"

        if trivy image --format json --output "$scan_file" --severity "$scan_severity" "$image"; then
            # Analyze results
            local critical_vulns=$(jq -r '.Results[]?.Vulnerabilities[]? | select(.Severity == "CRITICAL") | .VulnerabilityID' "$scan_file" 2>/dev/null | wc -l || echo "0")
            local high_vulns=$(jq -r '.Results[]?.Vulnerabilities[]? | select(.Severity == "HIGH") | .VulnerabilityID' "$scan_file" 2>/dev/null | wc -l || echo "0")
            local total_vulns=$(jq -r '.Results[]?.Vulnerabilities[]? | .VulnerabilityID' "$scan_file" 2>/dev/null | wc -l || echo "0")

            log_info "Scan results for $image:"
            log_info "  Critical vulnerabilities: $critical_vulns"
            log_info "  High vulnerabilities: $high_vulns"
            log_info "  Total vulnerabilities: $total_vulns"
            log_info "  Scan report: $scan_file"

            if [[ $critical_vulns -gt 0 ]]; then
                log_error "Critical vulnerabilities found in $image"
                return 1
            elif [[ $high_vulns -gt 0 ]]; then
                log_warning "High vulnerabilities found in $image"
            else
                log_success "No vulnerabilities above threshold found in $image"
            fi
        else
            log_error "Failed to scan image: $image"
            return 1
        fi
    done <<< "$images"

    log_success "Vulnerability scanning completed"
}

# Function to cleanup unused images
cleanup_images() {
    log_progress "Cleaning up unused Docker images..."

    local cleanup_count=0
    local space_freed=0

    # Remove dangling images
    local dangling_images=$(docker images -f "dangling=true" -q)
    if [[ -n "$dangling_images" ]]; then
        log_info "Removing dangling images..."
        local space_before=$(docker system df --format "{{.Size}}" | tail -n1 | numfmt --from=iec 2>/dev/null || echo "0")

        if [[ "$FORCE_MODE" == true ]]; then
            docker rmi $dangling_images
        else
            docker rmi $dangling_images || true
        fi

        local space_after=$(docker system df --format "{{.Size}}" | tail -n1 | numfmt --from=iec 2>/dev/null || echo "0")
        space_freed=$((space_before - space_after))
        cleanup_count=$(echo "$dangling_images" | wc -l)

        log_success "Removed $cleanup_count dangling images, freed ${space_freed} bytes"
    fi

    # Remove old versions of Puzzle71 images (keep last 2 versions)
    local old_images=$(docker images --format "{{.Repository}}:{{.Tag}}" | grep "$REGISTRY_NAMESPACE" | grep -v ":latest$" | sort -r | tail -n +3)

    if [[ -n "$old_images" ]]; then
        log_info "Removing old versions of Puzzle71 images..."

        while IFS= read -r image; do
            [[ -z "$image" ]] && continue

            if [[ "$FORCE_MODE" == true ]] || docker rmi "$image" &>/dev/null; then
                log_info "Removed: $image"
                ((cleanup_count++))
            else
                log_warning "Could not remove: $image (may be in use)"
            fi
        done <<< "$old_images"
    fi

    # Cleanup Docker system
    log_info "Cleaning up Docker system..."
    docker system prune -f 2>/dev/null || true

    log_success "Cleanup completed. Removed $cleanup_count images"
}

# Function to tag images
tag_images() {
    local new_tag="${1:-}"

    if [[ -z "$new_tag" ]]; then
        log_error "No new tag specified"
        log_info "Usage: $SCRIPT_NAME tag <new_tag>"
        return 1
    fi

    log_progress "Tagging images with new tag: $new_tag"

    # Get images to tag
    local images=$(docker images --format "{{.Repository}}:{{.Tag}}" | grep "$REGISTRY_NAMESPACE" | grep "$IMAGE_TAG" || echo "")

    while IFS= read -r image; do
        [[ -z "$image" ]] && continue

        local repository=$(echo "$image" | cut -d: -f1)
        local new_image_name="$repository:$new_tag"

        log_info "Tagging: $image -> $new_image_name"

        if docker tag "$image" "$new_image_name"; then
            log_success "Tagged: $new_image_name"
        else
            log_error "Failed to tag: $new_image_name"
            return 1
        fi
    done <<< "$images"

    log_success "All images tagged successfully"
}

# Function to delete images from registry
delete_images() {
    local tag_to_delete="${1:-$IMAGE_TAG}"

    log_progress "Deleting images from registry: $REGISTRY_URL (tag: $tag_to_delete)"

    # Confirmation prompt
    if [[ "$FORCE_MODE" != true ]]; then
        read -p "Are you sure you want to delete images with tag '$tag_to_delete' from $REGISTRY_URL? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            log_info "Deletion cancelled"
            return 0
        fi
    fi

    # Get images to delete
    local images=$(python3 -c "
import yaml
with open('$REGISTRY_CONFIG_FILE', 'r') as f:
    config = yaml.safe_load(f)

for name in config.get('images', {}).keys():
    print(f'{REGISTRY_NAMESPACE}/{name}')
" 2>/dev/null || echo "$REGISTRY_NAMESPACE/puzzle71-solver")

    while IFS= read -r image_name; do
        [[ -z "$image_name" ]] && continue

        local full_image_name="$REGISTRY_URL/$image_name:$tag_to_delete"

        log_info "Deleting: $full_image_name"

        # Delete from registry
        if docker manifest rm "$full_image_name" 2>/dev/null || true; then
            log_success "Deleted from registry: $full_image_name"
        else
            log_warning "Could not delete from registry (may not exist): $full_image_name"
        fi

        # Delete local copy
        if docker rmi "$full_image_name" 2>/dev/null || true; then
            log_success "Deleted local copy: $full_image_name"
        fi

    done <<< "$images"

    log_success "Image deletion completed"
}

# Function to show registry information
show_info() {
    log_info "Registry Information:"
    echo "================================"
    echo "Registry URL: $REGISTRY_URL"
    echo "Namespace: $REGISTRY_NAMESPACE"
    echo "Default Tag: $IMAGE_TAG"
    echo ""

    # Show local images
    log_info "Local Puzzle71 Images:"
    docker images --format "table {{.Repository}}\t{{.Tag}}\t{{.Size}}\t{{.CreatedAt}}" | grep "$REGISTRY_NAMESPACE" || log_info "No local images found"
    echo ""

    # Show disk usage
    log_info "Docker Disk Usage:"
    docker system df
    echo ""

    # Show system info
    log_info "System Information:"
    echo "Docker Version: $(docker --version)"
    echo "Docker Compose Version: $(docker compose version)"
    echo "NVIDIA Docker Support: $(docker run --rm --gpus all nvidia/cuda:12.1-base nvidia-smi &>/dev/null && echo "Available" || echo "Not Available")"
    echo "Available Disk Space: $(df -h . | tail -n1 | awk '{print $4}')"
}

# Function to backup registry images
backup_images() {
    log_progress "Backing up registry images..."

    # Check if backup is enabled
    local backup_enabled=$(python3 -c "import yaml; print(yaml.safe_load(open('$REGISTRY_CONFIG_FILE')).get('storage', {}).get('backup', {}).get('enabled', 'true'))" 2>/dev/null || echo "true")

    if [[ "$backup_enabled" != "true" ]]; then
        log_info "Backup disabled in configuration"
        return 0
    fi

    # Create backup directory
    local backup_dir="$PROJECT_ROOT/backups/registry/$(date +%Y%m%d_%H%M%S)"
    mkdir -p "$backup_dir"

    # Get backup configuration
    local compression=$(python3 -c "import yaml; print(yaml.safe_load(open('$REGISTRY_CONFIG_FILE')).get('storage', {}).get('backup', {}).get('compression', 'gzip'))" 2>/dev/null || echo "gzip")

    # Backup images
    local images=$(docker images --format "{{.Repository}}:{{.Tag}}" | grep "$REGISTRY_NAMESPACE" || echo "")
    local backup_count=0

    while IFS= read -r image; do
        [[ -z "$image" ]] && continue

        local filename=$(echo "$image" | tr '/' '_').tar
        if [[ "$compression" == "gzip" ]]; then
            filename="$filename.gz"
        fi

        log_info "Backing up: $image -> $filename"

        if docker save "$image" | gzip > "$backup_dir/$filename"; then
            log_success "Backed up: $image"
            ((backup_count++))
        else
            log_error "Failed to backup: $image"
            return 1
        fi
    done <<< "$images"

    # Create backup metadata
    cat > "$backup_dir/metadata.txt" << EOF
Backup created: $(date)
Images backed up: $backup_count
Registry URL: $REGISTRY_URL
Namespace: $REGISTRY_NAMESPACE
Compression: $compression
Total size: $(du -sh "$backup_dir" | cut -f1)
EOF

    log_success "Backup completed: $backup_dir ($backup_count images)"
}

# Function to restore registry images
restore_images() {
    local backup_dir="$1"

    if [[ -z "$backup_dir" ]]; then
        log_error "No backup directory specified"
        log_info "Usage: $SCRIPT_NAME restore <backup_directory>"
        return 1
    fi

    if [[ ! -d "$backup_dir" ]]; then
        log_error "Backup directory does not exist: $backup_dir"
        return 1
    fi

    log_progress "Restoring images from backup: $backup_dir"

    # Check backup metadata
    if [[ -f "$backup_dir/metadata.txt" ]]; then
        log_info "Backup metadata:"
        cat "$backup_dir/metadata.txt"
        echo ""
    fi

    # Restore images
    local restore_count=0
    local backup_files=$(find "$backup_dir" -name "*.tar.gz" -o -name "*.tar" 2>/dev/null)

    while IFS= read -r backup_file; do
        [[ -z "$backup_file" ]] && continue

        log_info "Restoring from: $(basename "$backup_file")"

        if [[ "$backup_file" == *.gz ]]; then
            if gunzip -c "$backup_file" | docker load; then
                log_success "Restored: $(basename "$backup_file")"
                ((restore_count++))
            else
                log_error "Failed to restore: $(basename "$backup_file")"
                return 1
            fi
        else
            if docker load -i "$backup_file"; then
                log_success "Restored: $(basename "$backup_file")"
                ((restore_count++))
            else
                log_error "Failed to restore: $(basename "$backup_file")"
                return 1
            fi
        fi
    done <<< "$backup_files"

    log_success "Restore completed: $restore_count images restored"
}

# Main execution function
main() {
    # Parse command line arguments
    parse_args "$@"

    # Load configuration
    load_registry_config

    # Execute command
    case "$COMMAND" in
        init)
            init_local_registry
            ;;
        build)
            build_images
            ;;
        push)
            push_images
            ;;
        pull)
            pull_images
            ;;
        list)
            list_images
            ;;
        scan)
            scan_images
            ;;
        cleanup)
            cleanup_images
            ;;
        tag)
            tag_images "${1:-}"
            ;;
        delete)
            delete_images "${1:-}"
            ;;
        info)
            show_info
            ;;
        backup)
            backup_images
            ;;
        restore)
            restore_images "${1:-}"
            ;;
        *)
            log_error "Unknown command: $COMMAND"
            show_usage
            exit 1
            ;;
    esac
}

# Script entry point
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
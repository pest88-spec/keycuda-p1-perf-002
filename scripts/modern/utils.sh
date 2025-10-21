#!/bin/bash
# Puzzle71Solver - Core Utility Library
# Provides common functions for all scripts

set -euo pipefail

# Colors for output
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly PURPLE='\033[0;35m'
readonly CYAN='\033[0;36m'
readonly NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $(date '+%Y-%m-%d %H:%M:%S') $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $(date '+%Y-%m-%d %H:%M:%S') $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $(date '+%Y-%m-%d %H:%M:%S') $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $(date '+%Y-%m-%d %H:%M:%S') $1" >&2
}

log_debug() {
    if [[ "${DEBUG:-false}" == "true" ]]; then
        echo -e "${PURPLE}[DEBUG]${NC} $(date '+%Y-%m-%d %H:%M:%S') $1"
    fi
}

# Progress reporting
progress_start() {
    local task="$1"
    echo -e "${CYAN}[PROGRESS]${NC} Starting: $task"
}

progress_end() {
    local task="$1"
    echo -e "${CYAN}[PROGRESS]${NC} Completed: $task"
}

# Error handling
error_exit() {
    local message="$1"
    local exit_code="${2:-1}"
    log_error "$message"
    exit "$exit_code"
}

# Check if command exists
command_exists() {
    command -v "$1" &> /dev/null
}

# Require command to exist
require_command() {
    local cmd="$1"
    if ! command_exists "$cmd"; then
        error_exit "Required command not found: $cmd. Please install $cmd to continue."
    fi
}

# Check if file exists and is readable
require_file() {
    local file="$1"
    local description="${2:-File}"
    if [[ ! -f "$file" ]]; then
        error_exit "$description not found: $file"
    fi
    if [[ ! -r "$file" ]]; then
        error_exit "$description not readable: $file"
    fi
}

# Check if directory exists and is writable
require_dir() {
    local dir="$1"
    local description="${2:-Directory}"
    if [[ ! -d "$dir" ]]; then
        error_exit "$description not found: $dir"
    fi
    if [[ ! -w "$dir" ]]; then
        error_exit "$description not writable: $dir"
    fi
}

# Create directory if it doesn't exist
ensure_dir() {
    local dir="$1"
    local mode="${2:-755}"
    if [[ ! -d "$dir" ]]; then
        mkdir -p "$dir"
        chmod "$mode" "$dir"
        log_debug "Created directory: $dir"
    fi
}

# Remove file or directory safely
safe_remove() {
    local path="$1"
    if [[ -e "$path" ]]; then
        rm -rf "$path"
        log_debug "Removed: $path"
    fi
}

# Get script directory
get_script_dir() {
    cd "$(dirname "${BASH_SOURCE[0]}")" && pwd
}

# Get project root directory
get_project_root() {
    local script_dir
    script_dir="$(get_script_dir)"
    while [[ "$script_dir" != "/" ]]; do
        if [[ -f "$script_dir/CMakeLists.txt" ]]; then
            echo "$script_dir"
            return 0
        fi
        script_dir="$(dirname "$script_dir")"
    done
    error_exit "Project root not found (no CMakeLists.txt found)"
}

# Validate environment
validate_environment() {
    log_info "Validating environment..."

    # Check basic requirements
    require_command "bash"
    require_command "cmake"
    require_command "make"

    # Check for CUDA
    if command_exists "nvcc"; then
        log_info "CUDA found: $(nvcc --version | head -n1)"
    else
        log_warning "CUDA not found - GPU features will be unavailable"
    fi

    # Check for git
    if command_exists "git"; then
        log_info "Git found: $(git --version)"
    else
        log_warning "Git not found - version control features unavailable"
    fi

    log_success "Environment validation completed"
}

# Detect OS
detect_os() {
    case "$(uname -s)" in
        Linux*)     echo "linux" ;;
        Darwin*)    echo "macos" ;;
        CYGWIN*|MINGW*|MSYS*) echo "windows" ;;
        *)          echo "unknown" ;;
    esac
}

# Detect architecture
detect_arch() {
    case "$(uname -m)" in
        x86_64)    echo "x86_64" ;;
        aarch64)   echo "arm64" ;;
        arm64)     echo "arm64" ;;
        *)         echo "unknown" ;;
    esac
}

# Get number of CPU cores
get_cpu_cores() {
    if command_exists "nproc"; then
        nproc
    elif command_exists "sysctl"; then
        sysctl -n hw.ncpu
    else
        echo "4"  # Conservative default
    fi
}

# Get available memory in GB
get_memory_gb() {
    local os
    os="$(detect_os)"
    case "$os" in
        linux)
            awk '/MemAvailable/ {printf "%.0f", $2/1024/1024}' /proc/meminfo 2>/dev/null || echo "4"
            ;;
        macos)
            sysctl -n hw.memsize | awk '{printf "%.0f", $1/1024/1024/1024}'
            ;;
        *)
            echo "4"  # Conservative default
            ;;
    esac
}

# Check if GPU is available
check_gpu_available() {
    if command_exists "nvidia-smi"; then
        nvidia-smi &> /dev/null
    else
        return 1
    fi
}

# Get GPU information
get_gpu_info() {
    if check_gpu_available; then
        nvidia-smi --query-gpu=name,memory.total --format=csv,noheader,nounits | head -n1
    else
        echo "No GPU detected"
    fi
}

# Validate version requirement
check_version() {
    local current="$1"
    local required="$2"
    local tool="$3"

    if command_exists "sort" && sort --help 2>/dev/null | grep -q "version-sort"; then
        if [[ "$(printf '%s\n' "$required" "$current" | sort --version-sort | head -n1)" == "$required" ]]; then
            return 0
        fi
    fi

    log_warning "Cannot verify $tool version (required: $required, found: $current)"
}

# Check CMake version
check_cmake_version() {
    if command_exists "cmake"; then
        local current_version
        current_version="$(cmake --version | head -n1 | awk '{print $3}')"
        check_version "$current_version" "3.22.0" "cmake"
    else
        error_exit "CMake not found"
    fi
}

# Check CUDA version
check_cuda_version() {
    if command_exists "nvcc"; then
        local current_version
        current_version="$(nvcc --version | grep "release" | awk '{print $6}' | cut -c2-)"
        check_version "$current_version" "11.8.0" "CUDA"
        log_info "CUDA version: $current_version"
    else
        log_warning "CUDA not available"
    fi
}

# Parse configuration file
parse_config() {
    local config_file="$1"
    local key="$2"
    local default_value="${3:-}"

    if [[ -f "$config_file" ]]; then
        grep "^${key}=" "$config_file" 2>/dev/null | cut -d'=' -f2- | tr -d '"' || echo "$default_value"
    else
        echo "$default_value"
    fi
}

# Set configuration value
set_config() {
    local config_file="$1"
    local key="$2"
    local value="$3"

    # Create config file if it doesn't exist
    touch "$config_file"

    if grep -q "^${key}=" "$config_file"; then
        # Update existing key
        if [[ "$OSTYPE" == "darwin"* ]]; then
            sed -i '' "s/^${key}=.*/${key}=${value}/" "$config_file"
        else
            sed -i "s/^${key}=.*/${key}=${value}/" "$config_file"
        fi
    else
        # Add new key
        echo "${key}=${value}" >> "$config_file"
    fi
    log_debug "Set config: ${key}=${value}"
}

# Generate timestamp
get_timestamp() {
    date '+%Y-%m-%d_%H-%M-%S'
}

# Generate unique ID
get_unique_id() {
    date '+%Y%m%d_%H%M%S_%N' 2>/dev/null || date '+%Y%m%d_%H%M%S'
}

# Cleanup function
cleanup_on_exit() {
    local temp_dirs=("${TEMP_DIRS[@]:-}")
    local temp_files=("${TEMP_FILES[@]:-}")

    for dir in "${temp_dirs[@]}"; do
        if [[ -d "$dir" ]]; then
            safe_remove "$dir"
        fi
    done

    for file in "${temp_files[@]}"; do
        if [[ -f "$file" ]]; then
            safe_remove "$file"
        fi
    done
}

# Register cleanup
register_cleanup() {
    trap cleanup_on_exit EXIT
}

# Add temporary directory for cleanup
add_temp_dir() {
    TEMP_DIRS+=("$1")
}

# Add temporary file for cleanup
add_temp_file() {
    TEMP_FILES+=("$1")
}

# Initialize global arrays
TEMP_DIRS=()
TEMP_FILES=()

# Load configuration if available
load_config() {
    local config_file="${1:-$(get_project_root)/scripts.conf}"
    if [[ -f "$config_file" ]]; then
        # shellcheck source=/dev/null
        source "$config_file"
        log_debug "Loaded configuration from: $config_file"
    fi
}

# Save configuration
save_config() {
    local config_file="${1:-$(get_project_root)/scripts.conf}"
    local config_dir
    config_dir="$(dirname "$config_file")"
    ensure_dir "$config_dir"

    # Export key variables to config
    cat > "$config_file" << EOF
# Puzzle71Solver Scripts Configuration
# Generated on $(date)

# Environment
DEBUG="${DEBUG:-false}"
VERBOSE="${VERBOSE:-false}"

# Build settings
BUILD_TYPE="${BUILD_TYPE:-RelWithDebInfo}"
BUILD_PARALLEL="${BUILD_PARALLEL:-true}"
BUILD_JOBS="${BUILD_JOBS:-$(get_cpu_cores)}"

# Performance settings
CUDA_ARCHS="${CUDA_ARCHS:-75;86;89;90}"
OPTIMIZATION_LEVEL="${OPTIMIZATION_LEVEL:-3}"

# Testing
TEST_PARALLEL="${TEST_PARALLEL:-true}"
TEST_JOBS="${TEST_JOBS:-$(get_cpu_cores)}"

# Monitoring
ENABLE_TELEMETRY="${ENABLE_TELEMETRY:-true}"
TELEMETRY_INTERVAL="${TELEMETRY_INTERVAL:-1}"

# Deployment
DEPLOYMENT_SELF_CONTAINED="${DEPLOYMENT_SELF_CONTAINED:-true}"
DEPLOYMENT_INCLUDE_DOCS="${DEPLOYMENT_INCLUDE_DOCS:-true}"
EOF

    log_debug "Saved configuration to: $config_file"
}

# Auto-initialize when sourced
if [[ "${BASH_SOURCE[0]}" != "${0}" ]]; then
    # Script is being sourced
    load_config
    register_cleanup
fi
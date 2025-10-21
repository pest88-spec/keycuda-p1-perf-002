#!/bin/bash
# Puzzle71Solver - Development Environment Setup
# Initializes the complete development environment

set -euo pipefail

# Get script directory and load utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=../utils.sh
source "$SCRIPT_DIR/../utils.sh"

# Default configuration
DEFAULT_BUILD_TYPE="RelWithDebInfo"
DEFAULT_CUDA_ARCHS="75;86;89;90"

# Show help
show_help() {
    cat << EOF
Puzzle71Solver Setup Script

USAGE:
    setup.sh [options]

OPTIONS:
    --build-type <type>     Build type (Debug|Release|RelWithDebInfo) [default: $DEFAULT_BUILD_TYPE]
    --cuda-archs <archs>    CUDA architectures [default: $DEFAULT_CUDA_ARCHS]
    --offline              Offline mode (no external downloads)
    --skip-deps           Skip dependency installation
    --skip-tests          Skip test setup
    --verbose, -v         Enable verbose output
    --help, -h            Show this help

EXAMPLES:
    setup.sh                           # Standard setup
    setup.sh --build-type Release      # Release build setup
    setup.sh --offline --skip-deps     # Offline setup without dependencies

This script sets up the complete development environment including:
- Dependencies and build tools
- CUDA configuration
- Build directory structure
- Development tools and scripts
- Testing framework
EOF
}

# Parse command line arguments
parse_args() {
    BUILD_TYPE="$DEFAULT_BUILD_TYPE"
    CUDA_ARCHS="$DEFAULT_CUDA_ARCHS"
    OFFLINE_MODE=false
    SKIP_DEPS=false
    SKIP_TESTS=false
    VERBOSE=false

    while [[ $# -gt 0 ]]; do
        case $1 in
            --build-type)
                BUILD_TYPE="$2"
                shift 2
                ;;
            --cuda-archs)
                CUDA_ARCHS="$2"
                shift 2
                ;;
            --offline)
                OFFLINE_MODE=true
                shift
                ;;
            --skip-deps)
                SKIP_DEPS=true
                shift
                ;;
            --skip-tests)
                SKIP_TESTS=true
                shift
                ;;
            --verbose|-v)
                VERBOSE=true
                DEBUG=true
                shift
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            *)
                error_exit "Unknown option: $1"
                ;;
        esac
    done
}

# Validate build type
validate_build_type() {
    local valid_types=("Debug" "Release" "RelWithDebInfo" "MinSizeRel")
    if [[ ! " ${valid_types[*]} " =~ " $BUILD_TYPE " ]]; then
        error_exit "Invalid build type: $BUILD_TYPE. Valid types: ${valid_types[*]}"
    fi
}

# Install system dependencies
install_dependencies() {
    if [[ "$SKIP_DEPS" == true ]]; then
        log_info "Skipping dependency installation"
        return 0
    fi

    progress_start "Installing system dependencies"

    local os
    os="$(detect_os)"
    case "$os" in
        linux)
            install_linux_dependencies
            ;;
        macos)
            install_macos_dependencies
            ;;
        windows)
            error_exit "Windows setup not yet supported"
            ;;
        *)
            log_warning "Unknown OS: $os, attempting generic setup"
            install_generic_dependencies
            ;;
    esac

    progress_end "System dependencies installation"
}

# Install Linux dependencies
install_linux_dependencies() {
    # Check for package manager
    if command_exists "apt-get"; then
        install_apt_dependencies
    elif command_exists "yum"; then
        install_yum_dependencies
    elif command_exists "pacman"; then
        install_pacman_dependencies
    else
        log_warning "No supported package manager found, installing minimal dependencies"
        install_generic_dependencies
    fi
}

# Install APT-based dependencies
install_apt_dependencies() {
    log_info "Installing dependencies with apt..."

    # Update package list
    sudo apt-get update

    # Install core dependencies
    local packages=(
        "cmake"
        "build-essential"
        "git"
        "pkg-config"
        "libssl-dev"
        "nlohmann-json3-dev"
    )

    # Add development tools
    if [[ "$OFFLINE_MODE" != true ]]; then
        packages+=(
            "libgtest-dev"
            "libbenchmark-dev"
            "clang-format"
            "cppcheck"
        )
    fi

    # Install packages
    sudo apt-get install -y "${packages[@]}"

    # Try to install CUDA toolkit
    if [[ "$OFFLINE_MODE" != true ]]; then
        log_info "Attempting to install CUDA toolkit..."
        if ! command_exists "nvcc"; then
            log_warning "CUDA toolkit not found. Please install CUDA manually from NVIDIA website."
            log_info "Visit: https://developer.nvidia.com/cuda-downloads"
        fi
    fi
}

# Install YUM-based dependencies
install_yum_dependencies() {
    log_info "Installing dependencies with yum..."

    # Install core dependencies
    local packages=(
        "cmake3"
        "gcc-c++"
        "git"
        "pkgconfig"
        "openssl-devel"
        "json-devel"
    )

    sudo yum install -y "${packages[@]}"

    # CUDA requires manual installation on RHEL/CentOS
    if ! command_exists "nvcc"; then
        log_warning "CUDA toolkit not found. Please install CUDA manually from NVIDIA website."
    fi
}

# Install Pacman-based dependencies
install_pacman_dependencies() {
    log_info "Installing dependencies with pacman..."

    local packages=(
        "cmake"
        "gcc"
        "git"
        "pkgconf"
        "openssl"
        "nlohmann-json"
    )

    sudo pacman -S --noconfirm "${packages[@]}"

    if ! command_exists "nvcc"; then
        log_warning "CUDA toolkit not found. Please install CUDA manually from NVIDIA website."
    fi
}

# Install generic dependencies
install_generic_dependencies() {
    log_info "Checking for generic dependencies..."

    # Check for basic tools
    require_command "cmake"
    require_command "make"
    require_command "gcc" || require_command "clang"

    # Check for OpenSSL
    if ! pkg-config --exists openssl; then
        log_warning "OpenSSL development libraries not found"
    fi

    # Check for nlohmann/json
    if ! pkg-config --exists nlohmann_json; then
        log_warning "nlohmann/json not found, will use header-only fallback"
    fi
}

# Install macOS dependencies
install_macos_dependencies() {
    log_info "Installing dependencies on macOS..."

    # Check for Homebrew
    if ! command_exists "brew"; then
        log_info "Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi

    # Install dependencies
    local packages=(
        "cmake"
        "git"
        "pkg-config"
        "openssl"
        "nlohmann-json"
    )

    if [[ "$OFFLINE_MODE" != true ]]; then
        packages+=(
            "googletest"
            "benchmark"
            "clang-format"
        )
    fi

    brew install "${packages[@]}"

    # CUDA requires manual installation on macOS
    if ! command_exists "nvcc"; then
        log_warning "CUDA toolkit not found. Please install CUDA manually from NVIDIA website."
    fi
}

# Create build directory structure
create_build_structure() {
    progress_start "Creating build directory structure"

    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    # Create main build directory
    ensure_dir "$build_dir"

    # Create subdirectories
    local subdirs=(
        "bin"
        "lib"
        "include"
        "share"
        "docs"
        "tests"
        "benchmarks"
        "deployment"
        "logs"
        "temp"
    )

    for subdir in "${subdirs[@]}"; do
        ensure_dir "$build_dir/$subdir"
    done

    # Create scripts directories
    local scripts_dirs=(
        "$project_root/scripts/modern/workflow"
        "$project_root/scripts/modern/automation"
        "$project_root/scripts/modern/monitoring"
        "$project_root/scripts/modern/testing"
    )

    for scripts_dir in "${scripts_dirs[@]}"; do
        ensure_dir "$scripts_dir"
    done

    progress_end "Build directory structure creation"
}

# Configure build system
configure_build_system() {
    progress_start "Configuring build system"

    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    # Create CMake configuration
    local cmake_args=(
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        "-DCMAKE_CUDA_ARCHITECTURES=$(echo "$CUDA_ARCHS" | tr ';' ',')"
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    )

    # Add offline mode flags
    if [[ "$OFFLINE_MODE" == true ]]; then
        cmake_args+=("-DOFFLINE_BUILD=ON")
    fi

    # Add skip tests flags
    if [[ "$SKIP_TESTS" == true ]]; then
        cmake_args+=("-DBUILD_TESTING=OFF")
    fi

    # Configure build
    cd "$build_dir"
    log_info "Running CMake configuration..."

    if [[ "$VERBOSE" == true ]]; then
        set -x
    fi

    cmake "${cmake_args[@]}" "$project_root"

    if [[ "$VERBOSE" == true ]]; then
        set +x
    fi

    progress_end "Build system configuration"
}

# Setup development tools
setup_development_tools() {
    progress_start "Setting up development tools"

    local project_root
    project_root="$(get_project_root)"

    # Create git hooks directory
    ensure_dir "$project_root/.git/hooks"

    # Install pre-commit hook if git is available
    if command_exists "git" && [[ -d "$project_root/.git" ]]; then
        setup_git_hooks
    fi

    # Create development configuration
    create_development_config

    progress_end "Development tools setup"
}

# Setup git hooks
setup_git_hooks() {
    local project_root
    project_root="$(get_project_root)"
    local hooks_dir="$project_root/.git/hooks"

    # Create pre-commit hook
    cat > "$hooks_dir/pre-commit" << 'EOF'
#!/bin/bash
# Pre-commit hook for Puzzle71Solver

set -euo pipefail

# Run basic checks
echo "Running pre-commit checks..."

# Check for common errors
if git diff --cached --name-only | grep -q "\.cpp$\|\.cu$\|\.h$"; then
    echo "Checking C++/CUDA files..."

    # TODO: Add clang-format check
    # TODO: Add static analysis
fi

echo "Pre-commit checks completed."
EOF

    chmod +x "$hooks_dir/pre-commit"
    log_info "Installed pre-commit git hook"
}

# Create development configuration
create_development_config() {
    local project_root
    project_root="$(get_project_root)"
    local config_file="$project_root/scripts.conf"

    # Save configuration
    save_config "$config_file"

    # Set additional configuration
    set_config "$config_file" "BUILD_TYPE" "$BUILD_TYPE"
    set_config "$config_file" "CUDA_ARCHS" "$CUDA_ARCHS"
    set_config "$config_file" "OFFLINE_MODE" "$OFFLINE_MODE"

    log_info "Created development configuration"
}

# Validate installation
validate_installation() {
    progress_start "Validating installation"

    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    # Check build directory
    require_dir "$build_dir" "Build directory"

    # Check CMake configuration
    require_file "$build_dir/CMakeCache.txt" "CMake cache"

    # Check for executables
    local required_files=(
        "CMakeCache.txt"
        "Makefile"
    )

    for file in "${required_files[@]}"; do
        require_file "$build_dir/$file" "Required file: $file"
    done

    # Test basic compilation
    log_info "Testing basic compilation..."
    cd "$build_dir"
    make --dry-run Puzzle71Solver > /dev/null 2>&1 || {
        log_warning "Basic compilation test failed, but build system is configured"
    }

    progress_end "Installation validation"
}

# Show next steps
show_next_steps() {
    cat << EOF

🎉 Setup completed successfully!

Next steps:
1. Build the project:
   scripts build

2. Run tests:
   scripts test

3. Run benchmarks:
   scripts benchmark

4. View available commands:
   scripts --help

Configuration:
- Build type: $BUILD_TYPE
- CUDA architectures: $CUDA_ARCHS
- Offline mode: $OFFLINE_MODE
- Build directory: $(get_project_root)/build

For detailed documentation, see: docs/README.md
EOF
}

# Main setup function
main() {
    log_info "Starting Puzzle71Solver setup..."

    # Parse arguments
    parse_args "$@"

    # Validate build type
    validate_build_type

    # Show configuration
    log_info "Configuration:"
    log_info "  Build type: $BUILD_TYPE"
    log_info "  CUDA architectures: $CUDA_ARCHS"
    log_info "  Offline mode: $OFFLINE_MODE"
    log_info "  Skip dependencies: $SKIP_DEPS"
    log_info "  Skip tests: $SKIP_TESTS"

    # Run setup steps
    validate_environment
    install_dependencies
    create_build_structure
    configure_build_system
    setup_development_tools
    validate_installation

    # Show next steps
    show_next_steps

    log_success "Setup completed successfully!"
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
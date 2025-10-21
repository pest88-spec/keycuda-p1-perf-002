#!/bin/bash
# Puzzle71Solver - Build Script
# Optimized build system with technical debt elimination features

set -euo pipefail

# Get script directory and load utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=../utils.sh
source "$SCRIPT_DIR/../utils.sh"

# Default configuration
DEFAULT_BUILD_TYPE="RelWithDebInfo"
DEFAULT_BUILD_JOBS="$(get_cpu_cores)"
DEFAULT_CLEAN_BUILD=false
DEFAULT_TARGET="all"

# Show help
show_help() {
    cat << EOF
Puzzle71Solver Build Script

USAGE:
    build.sh [options] [target]

TARGETS:
    all                 Build all targets (default)
    Puzzle71Solver      Build main executable
    tests               Build test suite
    benchmarks          Build performance benchmarks
    clean               Clean build artifacts
    install             Install built artifacts

OPTIONS:
    --build-type <type>     Build type (Debug|Release|RelWithDebInfo|MinSizeRel)
    --jobs <n>              Number of parallel build jobs [default: $DEFAULT_BUILD_JOBS]
    --clean                 Clean before building
    --verbose, -v           Enable verbose build output
    --optimize             Enable aggressive optimizations
    --debug                Include debug symbols
    --profile              Enable profiling support
    --help, -h              Show this help

PERFORMANCE OPTIMIZATION:
    --optimize-memory       Optimize for memory efficiency
    --optimize-speed        Optimize for maximum speed
    --optimize-size         Optimize for binary size
    --gpu-archs <archs>     Override CUDA architectures

EXAMPLES:
    build.sh                           # Standard build
    build.sh --clean --optimize        # Clean optimized build
    build.sh --build-type Release      # Release build
    build.sh --jobs 8 tests            # Build tests with 8 jobs
    build.sh --optimize-speed Puzzle71Solver  # Optimized main executable

ENVIRONMENT VARIABLES:
    BUILD_TYPE          Override default build type
    BUILD_JOBS          Override default job count
    CUDA_ARCHS          Override CUDA architectures
    CMAKE_ARGS          Additional CMake arguments
EOF
}

# Parse command line arguments
parse_args() {
    BUILD_TYPE="${BUILD_TYPE:-$DEFAULT_BUILD_TYPE}"
    BUILD_JOBS="${BUILD_JOBS:-$DEFAULT_BUILD_JOBS}"
    CLEAN_BUILD="${CLEAN_BUILD:-$DEFAULT_CLEAN_BUILD}"
    TARGET="${DEFAULT_TARGET}"
    VERBOSE=false
    AGGRESSIVE_OPTIMIZE=false
    MEMORY_OPTIMIZE=false
    SPEED_OPTIMIZE=false
    SIZE_OPTIMIZE=false
    DEBUG_BUILD=false
    PROFILE_BUILD=false
    OVERRIDE_CUDA_ARCHS=""

    while [[ $# -gt 0 ]]; do
        case $1 in
            --build-type)
                BUILD_TYPE="$2"
                shift 2
                ;;
            --jobs)
                BUILD_JOBS="$2"
                shift 2
                ;;
            --clean)
                CLEAN_BUILD=true
                shift
                ;;
            --verbose|-v)
                VERBOSE=true
                shift
                ;;
            --optimize)
                AGGRESSIVE_OPTIMIZE=true
                shift
                ;;
            --optimize-memory)
                MEMORY_OPTIMIZE=true
                shift
                ;;
            --optimize-speed)
                SPEED_OPTIMIZE=true
                shift
                ;;
            --optimize-size)
                SIZE_OPTIMIZE=true
                shift
                ;;
            --debug)
                DEBUG_BUILD=true
                BUILD_TYPE="Debug"
                shift
                ;;
            --profile)
                PROFILE_BUILD=true
                shift
                ;;
            --gpu-archs)
                OVERRIDE_CUDA_ARCHS="$2"
                shift 2
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            all|Puzzle71Solver|tests|benchmarks|clean|install)
                TARGET="$1"
                shift
                ;;
            *)
                error_exit "Unknown option or target: $1"
                ;;
        esac
    done

    # Apply optimization presets
    if [[ "$AGGRESSIVE_OPTIMIZE" == true ]]; then
        SPEED_OPTIMIZE=true
        MEMORY_OPTIMIZE=true
        if [[ "$BUILD_TYPE" == "RelWithDebInfo" ]]; then
            BUILD_TYPE="Release"
        fi
    fi
}

# Validate build configuration
validate_build_config() {
    log_debug "Validating build configuration..."

    # Validate build type
    local valid_types=("Debug" "Release" "RelWithDebInfo" "MinSizeRel")
    if [[ ! " ${valid_types[*]} " =~ " $BUILD_TYPE " ]]; then
        error_exit "Invalid build type: $BUILD_TYPE. Valid types: ${valid_types[*]}"
    fi

    # Validate job count
    if ! [[ "$BUILD_JOBS" =~ ^[0-9]+$ ]] || [[ "$BUILD_JOBS" -lt 1 ]]; then
        error_exit "Invalid job count: $BUILD_JOBS. Must be a positive integer."
    fi

    # Check CUDA availability for GPU targets
    if [[ "$TARGET" == "Puzzle71Solver" || "$TARGET" == "all" ]] && ! command_exists "nvcc"; then
        log_warning "CUDA not found - GPU acceleration will be unavailable"
    fi

    log_debug "Build configuration validated"
}

# Get CUDA architectures
get_cuda_archs() {
    if [[ -n "$OVERRIDE_CUDA_ARCHS" ]]; then
        echo "$OVERRIDE_CUDA_ARCHS"
        return
    fi

    local archs="${CUDA_ARCHS:-75-real;86-real;89-real;90-real}"

    # If CUDA is available, detect supported architectures
    if command_exists "nvcc"; then
        local supported_archs
        supported_archs=$(nvcc --help | grep -oE 'sm_[0-9]+' | sort -u | tr '\n' ';' | sed 's/;$//')
        if [[ -n "$supported_archs" ]]; then
            archs="$supported_archs"
            log_debug "Detected CUDA architectures: $archs"
        fi
    fi

    echo "$archs"
}

# Configure CMake arguments
configure_cmake_args() {
    local cmake_args=(
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    )

    # Add CUDA architectures
    local cuda_archs
    cuda_archs="$(get_cuda_archs)"
    cmake_args+=("-DCMAKE_CUDA_ARCHITECTURES=$cuda_archs")

    # Add optimization flags
    if [[ "$SPEED_OPTIMIZE" == true ]]; then
        cmake_args+=("-DENABLE_AGGRESSIVE_OPTIMIZATIONS=ON")
    fi

    # Add memory optimization
    if [[ "$MEMORY_OPTIMIZE" == true ]]; then
        cmake_args+=("-DMEMORY_OPTIMIZATION=ON")
    fi

    # Add profiling support
    if [[ "$PROFILE_BUILD" == true ]]; then
        cmake_args+=("-DENABLE_PROFILING=ON")
        cmake_args+=("-DCMAKE_BUILD_TYPE=RelWithDebInfo")
    fi

    # Add debugging support
    if [[ "$DEBUG_BUILD" == true ]]; then
        cmake_args+=("-DCMAKE_VERBOSE_MAKEFILE=ON")
        cmake_args+=("-DCMAKE_CXX_FLAGS_DEBUG=-g -O0 -DDEBUG")
        cmake_args+=("-DCMAKE_CUDA_FLAGS_DEBUG=-g -G -O0 -DDEBUG")
    fi

    # Add testing support
    if [[ "$TARGET" == "tests" || "$TARGET" == "all" ]]; then
        cmake_args+=("-DBUILD_TESTING=ON")
    fi

    # Add benchmarking support
    if [[ "$TARGET" == "benchmarks" || "$TARGET" == "all" ]]; then
        cmake_args+=("-DBUILD_BENCHMARKS=ON")
    fi

    # Add custom CMake arguments from environment
    if [[ -n "${CMAKE_ARGS:-}" ]]; then
        # Split CMAKE_ARGS by spaces and add to cmake_args
        read -ra extra_args <<< "$CMAKE_ARGS"
        cmake_args+=("${extra_args[@]}")
    fi

    log_debug "CMake arguments: ${cmake_args[*]}"
    echo "${cmake_args[@]}"
}

# Prepare build directory
prepare_build_dir() {
    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    progress_start "Preparing build directory"

    # Clean build directory if requested
    if [[ "$CLEAN_BUILD" == true ]]; then
        log_info "Cleaning build directory..."
        safe_remove "$build_dir"
    fi

    # Create build directory
    ensure_dir "$build_dir"

    progress_end "Build directory preparation"
}

# Configure build system
configure_build() {
    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    progress_start "Configuring build system"

    cd "$build_dir"

    # Get CMake arguments
    local cmake_args
    read -ra cmake_args <<< "$(configure_cmake_args)"

    log_info "Configuring with build type: $BUILD_TYPE"
    log_info "Using $BUILD_JOBS parallel jobs"

    if [[ "$VERBOSE" == true ]]; then
        set -x
    fi

    # Run CMake configuration
    cmake "${cmake_args[@]}" "$project_root"

    if [[ "$VERBOSE" == true ]]; then
        set +x
    fi

    progress_end "Build system configuration"
}

# Build target
build_target() {
    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    progress_start "Building target: $TARGET"

    cd "$build_dir"

    # Prepare make arguments
    local make_args=(
        "-j$BUILD_JOBS"
    )

    # Add verbosity
    if [[ "$VERBOSE" == true ]]; then
        make_args+=("VERBOSE=1")
    fi

    # Handle special targets
    case "$TARGET" in
        "clean")
            log_info "Cleaning build artifacts..."
            make clean
            progress_end "Build cleanup"
            return 0
            ;;
        "install")
            make_args+=("install")
            ;;
    esac

    # Build the target
    if [[ "$VERBOSE" == true ]]; then
        set -x
    fi

    # Build with make or ninja if available
    if command_exists "ninja" && [[ -f "build.ninja" ]]; then
        ninja "${make_args[@]}" "$TARGET"
    else
        make "${make_args[@]}" "$TARGET"
    fi

    if [[ "$VERBOSE" == true ]]; then
        set +x
    fi

    progress_end "Target build: $TARGET"
}

# Validate build output
validate_build() {
    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    progress_start "Validating build output"

    cd "$build_dir"

    # Check for main executable
    if [[ "$TARGET" == "Puzzle71Solver" || "$TARGET" == "all" ]]; then
        local executable="Puzzle71Solver"
        if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
            executable="${executable}.exe"
        fi

        if [[ -f "$executable" ]]; then
            log_success "Main executable built: $(realpath "$executable")"

            # Show executable information
            if command_exists "file"; then
                log_info "File type: $(file "$executable")"
            fi

            # Show binary size
            if command_exists "du"; then
                local size
                size=$(du -h "$executable" | cut -f1)
                log_info "Binary size: $size"
            fi
        else
            error_exit "Main executable not found: $executable"
        fi
    fi

    # Check for test executables
    if [[ "$TARGET" == "tests" || "$TARGET" == "all" ]]; then
        if [[ -f "puzzle71_tests" ]]; then
            log_success "Test executable built: $(realpath puzzle71_tests)"
        fi
    fi

    # Check for benchmark executables
    if [[ "$TARGET" == "benchmarks" || "$TARGET" == "all" ]]; then
        if ls benchmarks/* 1> /dev/null 2>&1; then
            log_success "Benchmark executables built"
        fi
    fi

    progress_end "Build output validation"
}

# Show build summary
show_build_summary() {
    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    cat << EOF

🎉 Build completed successfully!

Build summary:
- Target: $TARGET
- Build type: $BUILD_TYPE
- Parallel jobs: $BUILD_JOBS
- Optimizations: $AGGRESSIVE_OPTIMIZE
- Memory optimization: $MEMORY_OPTIMIZE
- Speed optimization: $SPEED_OPTIMIZE

Build artifacts:
- Directory: $build_dir

Next steps:
EOF

    case "$TARGET" in
        "Puzzle71Solver"|"all")
            echo "1. Run the solver:"
            echo "   cd $build_dir && ./Puzzle71Solver --help"
            ;;
        "tests")
            echo "1. Run tests:"
            echo "   cd $build_dir && make test"
            echo "   or: cd $build_dir && ./puzzle71_tests"
            ;;
        "benchmarks")
            echo "1. Run benchmarks:"
            echo "   cd $build_dir && make run-benchmarks"
            ;;
    esac

    echo ""
    echo "Other commands:"
    echo "- scripts test                    # Run test suite"
    echo "- scripts benchmark               # Run performance benchmarks"
    echo "- scripts profile                 # Profile CUDA kernels"
    echo ""
}

# Main build function
main() {
    log_info "Starting Puzzle71Solver build..."

    # Parse arguments
    parse_args "$@"

    # Validate configuration
    validate_build_config

    # Show configuration
    log_info "Build configuration:"
    log_info "  Target: $TARGET"
    log_info "  Build type: $BUILD_TYPE"
    log_info "  Parallel jobs: $BUILD_JOBS"
    log_info "  Clean build: $CLEAN_BUILD"
    log_info "  Aggressive optimize: $AGGRESSIVE_OPTIMIZE"

    # Run build steps
    prepare_build_dir
    configure_build
    build_target
    validate_build

    # Show summary
    show_build_summary

    log_success "Build completed successfully!"
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
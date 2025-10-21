#!/bin/bash
# Puzzle71Solver - Environment Information
# Shows detailed environment and system information

set -euo pipefail

# Get script directory and load utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=../utils.sh
source "$SCRIPT_DIR/../utils.sh"

# Show help
show_help() {
    cat << EOF
Puzzle71Solver Environment Information

USAGE:
    env.sh [options]

OPTIONS:
    --verbose, -v         Enable verbose output
    --json                Output in JSON format
    --help, -h            Show this help

EXAMPLES:
    env.sh
    env.sh --verbose
    env.sh --json

This script displays comprehensive environment information including
OS, hardware, CUDA setup, and project configuration.
EOF
}

# Parse command line arguments
parse_args() {
    VERBOSE=false
    JSON_OUTPUT=false

    while [[ $# -gt 0 ]]; do
        case $1 in
            --verbose|-v)
                VERBOSE=true
                DEBUG=true
                shift
                ;;
            --json)
                JSON_OUTPUT=true
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

# Show system information
show_system_info() {
    if [[ "$JSON_OUTPUT" == true ]]; then
        local os
        os="$(detect_os)"
        local arch
        arch="$(detect_arch)"
        local cpu_cores
        cpu_cores="$(get_cpu_cores)"
        local memory_gb
        memory_gb="$(get_memory_gb)"

        cat << EOF
{
  "system": {
    "os": "$os",
    "architecture": "$arch",
    "cpu_cores": $cpu_cores,
    "memory_gb": $memory_gb,
    "shell": "$BASH_VERSION"
  }
}
EOF
    else
        log_info "System Information:"
        log_info "  OS: $(detect_os)"
        log_info "  Architecture: $(detect_arch)"
        log_info "  CPU Cores: $(get_cpu_cores)"
        log_info "  Memory: $(get_memory_gb) GB"
        log_info "  Shell: $BASH_VERSION"
    fi
}

# Show CUDA information
show_cuda_info() {
    if command_exists "nvcc"; then
        local cuda_version
        cuda_version="$(nvcc --version | grep release | awk '{print $6}' | cut -c2- 2>/dev/null || echo "unknown")"

        if [[ "$JSON_OUTPUT" == true ]]; then
            cat << EOF
{
  "cuda": {
    "installed": true,
    "version": "$cuda_version",
    "compiler_path": "$(which nvcc)"
  }
}
EOF
        else
            log_info "CUDA Information:"
            log_info "  Installed: Yes"
            log_info "  Version: $cuda_version"
            log_info "  Compiler: $(which nvcc)"
        fi
    else
        if [[ "$JSON_OUTPUT" == true ]]; then
            cat << EOF
{
  "cuda": {
    "installed": false,
    "version": null,
    "compiler_path": null
  }
}
EOF
        else
            log_info "CUDA Information:"
            log_info "  Installed: No"
            log_warning "CUDA toolkit not found - GPU features will be unavailable"
        fi
    fi
}

# Show GPU information
show_gpu_info() {
    if check_gpu_available; then
        local gpu_name
        gpu_name="$(get_gpu_info)"
        local gpu_count
        gpu_count="$(nvidia-smi --query-gpu=count --format=csv,noheader,nounits 2>/dev/null || echo "0")"

        if [[ "$JSON_OUTPUT" == true ]]; then
            cat << EOF
{
  "gpu": {
    "available": true,
    "count": $gpu_count,
    "name": "$gpu_name",
    "driver_version": "$(nvidia-smi --query-gpu=driver_version --format=csv,noheader,nounits | head -n1 2>/dev/null || echo "unknown")"
  }
}
EOF
        else
            log_info "GPU Information:"
            log_info "  Available: Yes"
            log_info "  Count: $gpu_count"
            log_info "  Name: $gpu_name"
            log_info "  Driver Version: $(nvidia-smi --query-gpu=driver_version --format=csv,noheader,nounits | head -n1 2>/dev/null || echo "unknown")"
        fi
    else
        if [[ "$JSON_OUTPUT" == true ]]; then
            cat << EOF
{
  "gpu": {
    "available": false,
    "count": 0,
    "name": null,
    "driver_version": null
  }
}
EOF
        else
            log_info "GPU Information:"
            log_info "  Available: No"
            log_warning "No GPU detected - GPU acceleration will be unavailable"
        fi
    fi
}

# Show project information
show_project_info() {
    local project_root
    project_root="$(get_project_root)"

    if [[ "$JSON_OUTPUT" == true ]]; then
        cat << EOF
{
  "project": {
    "root": "$project_root",
    "name": "Puzzle71Solver",
    "type": "CUDA Bitcoin Private Key Scanner",
    "scripts_directory": "$project_root/scripts/modern",
    "build_directory": "$project_root/build"
  }
}
EOF
    else
        log_info "Project Information:"
        log_info "  Name: Puzzle71Solver"
        log_info "  Type: CUDA Bitcoin Private Key Scanner"
        log_info "  Root: $project_root"
        log_info "  Scripts: $project_root/scripts/modern"
        log_info "  Build: $project_root/build"
    fi
}

# Show development tools
show_dev_tools() {
    if [[ "$JSON_OUTPUT" == true ]]; then
        local cmake_version="not_found"
        if command_exists "cmake"; then
            cmake_version="$(cmake --version | head -n1 | awk '{print $3}')"
        fi

        local make_version="not_found"
        if command_exists "make"; then
            make_version="$(make --version 2>/dev/null | head -n1 | awk '{print $3}' || echo "unknown")"
        fi

        local git_version="not_found"
        if command_exists "git"; then
            git_version="$(git --version | awk '{print $3}')"
        fi

        cat << EOF
{
  "development_tools": {
    "cmake": "$cmake_version",
    "make": "$make_version",
    "git": "$git_version",
    "clang_format": $(command_exists "clang-format" && echo "\"$(clang-format --version | head -n1 | awk '{print $3}')\"" || echo "null"),
    "cppcheck": $(command_exists "cppcheck" && echo "\"$(cppcheck --version | head -n1 | awk '{print $2}')\"" || echo "null")
  }
}
EOF
    else
        log_info "Development Tools:"

        if command_exists "cmake"; then
            log_info "  CMake: $(cmake --version | head -n1 | awk '{print $3}')"
        else
            log_warning "  CMake: Not found"
        fi

        if command_exists "make"; then
            log_info "  Make: $(make --version 2>/dev/null | head -n1 | awk '{print $3}' || echo "unknown")"
        else
            log_warning "  Make: Not found"
        fi

        if command_exists "git"; then
            log_info "  Git: $(git --version | awk '{print $3}')"
        else
            log_warning "  Git: Not found"
        fi

        if command_exists "clang-format"; then
            log_info "  Clang-Format: $(clang-format --version | head -n1 | awk '{print $3}')"
        else
            log_info "  Clang-Format: Not found"
        fi

        if command_exists "cppcheck"; then
            log_info "  Cppcheck: $(cppcheck --version | head -n1 | awk '{print $2}')"
        else
            log_info "  Cppcheck: Not found"
        fi
    fi
}

# Show performance expectations
show_performance_expectations() {
    if [[ "$JSON_OUTPUT" == true ]]; then
        local gpu_available
        gpu_available="$(check_gpu_available && echo true || echo false)"

        cat << EOF
{
  "performance_expectations": {
    "gpu_available": $gpu_available,
    "expected_throughput": {
      "turing": "1.0-2.0 Gkeys/s",
      "ampere": "2.0-3.0 Gkeys/s",
      "hopper": "3.5-4.0+ Gkeys/s"
    },
    "optimization_features": [
      "Shared memory optimization",
      "Warp-level primitives",
      "Structure-of-arrays layout",
      "Register pressure reduction",
      "Memory coalescing"
    ]
  }
}
EOF
    else
        log_info "Performance Expectations:"

        if check_gpu_available; then
            log_info "  GPU Available: Yes"
            log_info "  Expected Throughput:"
            log_info "    Turing (RTX 20xx): 1.0-2.0 Gkeys/s"
            log_info "    Ampere (RTX 30xx): 2.0-3.0 Gkeys/s"
            log_info "    Hopper (RTX 40xx/H100): 3.5-4.0+ Gkeys/s"
        else
            log_info "  GPU Available: No"
            log_info "  Expected Throughput: CPU-only mode (significantly slower)"
        fi

        log_info "  Optimization Features:"
        log_info "    - Shared memory optimization"
        log_info "    - Warp-level primitives"
        log_info "    - Structure-of-arrays layout"
        log_info "    - Register pressure reduction"
        log_info "    - Memory coalescing"
    fi
}

# Main function
main() {
    # Parse arguments
    parse_args "$@"

    if [[ "$JSON_OUTPUT" == true ]]; then
        # Output complete JSON environment report
        cat << EOF
{
  "timestamp": "$(date -Iseconds)",
  "environment_report": {
EOF

        # Add each section
        echo -n "    "
        show_system_info
        echo ","
        echo -n "    "
        show_cuda_info
        echo ","
        echo -n "    "
        show_gpu_info
        echo ","
        echo -n "    "
        show_project_info
        echo ","
        echo -n "    "
        show_dev_tools
        echo ","
        echo -n "    "
        show_performance_expectations

        cat << EOF
  }
}
EOF
    else
        # Output human-readable format
        log_info "Puzzle71Solver Environment Report"
        log_info "================================="
        echo ""

        show_system_info
        echo ""

        show_cuda_info
        echo ""

        show_gpu_info
        echo ""

        show_project_info
        echo ""

        show_dev_tools
        echo ""

        show_performance_expectations
        echo ""

        if [[ "$VERBOSE" == true ]]; then
            log_info "Verbose Information:"
            log_info "  Script Directory: $SCRIPT_DIR"
            log_info "  Working Directory: $(pwd)"
            log_info "  User: $(whoami)"
            log_info "  Home: $HOME"
            log_info "  Path: $PATH"
        fi
    fi
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
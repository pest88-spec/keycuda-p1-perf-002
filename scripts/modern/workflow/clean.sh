#!/bin/bash
# Puzzle71Solver - Build Cleanup Script
# Comprehensive cleanup of build artifacts and temporary files

set -euo pipefail

# Get script directory and load utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=../utils.sh
source "$SCRIPT_DIR/../utils.sh"

# Show help
show_help() {
    cat << EOF
Puzzle71Solver Cleanup Script

USAGE:
    clean.sh [options]

CLEANUP TYPES:
    build              Clean build artifacts (default)
    all                Clean everything including dependencies
    deps               Clean only dependencies
    profiles           Clean profiling results
    benchmarks         Clean benchmark results
    logs               Clean log files
    temp               Clean temporary files
    git                Clean git ignored files

OPTIONS:
    --dry-run          Show what would be deleted without actually deleting
    --verbose, -v      Enable verbose output
    --force            Skip confirmation prompts
    --keep-cache       Keep CMake cache files
    --keep-deps        Keep external dependencies
    --keep-results     Keep benchmark/profiling results
    --older-than <days> Clean files older than specified days
    --help, -h         Show this help

EXAMPLES:
    clean.sh                           # Clean build artifacts
    clean.sh --dry-run                 # Preview what would be cleaned
    clean.sh --force all               # Clean everything without confirmation
    clean.sh --keep-results            # Clean but preserve results
    clean.sh --older-than 7 profiles   # Clean profiling results older than 7 days

ENVIRONMENT VARIABLES:
    CLEAN_FORCE          Skip confirmation prompts
    CLEAN_KEEP_CACHE     Keep CMake cache
    CLEAN_KEEP_DEPS      Keep dependencies
    CLEAN_KEEP_RESULTS   Keep results
EOF
}

# Parse command line arguments
parse_args() {
    CLEAN_TYPE="build"
    DRY_RUN=false
    VERBOSE=false
    FORCE="${CLEAN_FORCE:-false}"
    KEEP_CACHE="${CLEAN_KEEP_CACHE:-false}"
    KEEP_DEPS="${CLEAN_KEEP_DEPS:-false}"
    KEEP_RESULTS="${CLEAN_KEEP_RESULTS:-false}"
    OLDER_THAN=""

    while [[ $# -gt 0 ]]; do
        case $1 in
            --dry-run)
                DRY_RUN=true
                shift
                ;;
            --verbose|-v)
                VERBOSE=true
                shift
                ;;
            --force)
                FORCE=true
                shift
                ;;
            --keep-cache)
                KEEP_CACHE=true
                shift
                ;;
            --keep-deps)
                KEEP_DEPS=true
                shift
                ;;
            --keep-results)
                KEEP_RESULTS=true
                shift
                ;;
            --older-than)
                OLDER_THAN="$2"
                shift 2
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            build|all|deps|profiles|benchmarks|logs|temp|git)
                CLEAN_TYPE="$1"
                shift
                ;;
            *)
                error_exit "Unknown option: $1"
                ;;
        esac
    done
}

# Confirm cleanup operation
confirm_cleanup() {
    if [[ "$FORCE" == true || "$DRY_RUN" == true ]]; then
        return 0
    fi

    echo
    log_warning "This will clean: $CLEAN_TYPE"
    echo "Are you sure you want to continue? [y/N]"
    read -r response

    case "$response" in
        [yY]|[yY][eE][sS])
            return 0
            ;;
        *)
            log_info "Cleanup cancelled"
            exit 0
            ;;
    esac
}

# Get file size in human readable format
get_file_size() {
    local file="$1"
    if [[ -f "$file" ]]; then
        if command_exists "du"; then
            du -h "$file" | cut -f1
        else
            echo "unknown"
        fi
    else
        echo "0"
    fi
}

# Check if file is older than specified days
is_older_than() {
    local file="$1"
    local days="$2"

    if [[ ! -f "$file" ]]; then
        return 1
    fi

    local file_age
    file_age=$(find "$file" -mtime "+$days" -print 2>/dev/null)
    [[ -n "$file_age" ]]
}

# Count files and directories
count_files() {
    local path="$1"
    if [[ -d "$path" ]]; then
        find "$path" -type f | wc -l
    else
        echo "0"
    fi
}

# Calculate total size
calculate_size() {
    local path="$1"
    if [[ -d "$path" ]]; then
        if command_exists "du"; then
            du -sh "$path" 2>/dev/null | cut -f1 || echo "0"
        else
            echo "unknown"
        fi
    elif [[ -f "$path" ]]; then
        get_file_size "$path"
    else
        echo "0"
    fi
}

# Remove file or directory with logging
remove_with_logging() {
    local path="$1"
    local description="${2:-item}"

    if [[ ! -e "$path" ]]; then
        return 0
    fi

    local size
    size=$(calculate_size "$path")

    if [[ "$DRY_RUN" == true ]]; then
        log_info "[DRY RUN] Would remove: $path ($size)"
        return 0
    fi

    if [[ "$VERBOSE" == true ]]; then
        log_info "Removing: $path ($size)"
    fi

    if safe_remove "$path"; then
        log_debug "Removed: $description"
    else
        log_warning "Failed to remove: $path"
    fi
}

# Clean build artifacts
clean_build() {
    progress_start "Cleaning build artifacts"

    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    if [[ -d "$build_dir" ]]; then
        local file_count
        file_count=$(count_files "$build_dir")
        local size
        size=$(calculate_size "$build_dir")

        log_info "Build directory: $build_dir"
        log_info "Files to clean: $file_count"
        log_info "Size: $size"

        # Remove build subdirectories
        local subdirs=(
            "bin"
            "lib"
            "include"
            "tests"
            "benchmarks"
            "CMakeFiles"
            "profiles"
            "logs"
            "temp"
        )

        for subdir in "${subdirs[@]}"; do
            remove_with_logging "$build_dir/$subdir" "build subdirectory: $subdir"
        done

        # Remove build files
        local build_files=(
            "CMakeCache.txt"
            "cmake_install.cmake"
            "Makefile"
            "build.ninja"
            "*.prof"
            "*.csv"
            "*.log"
        )

        for pattern in "${build_files[@]}"; do
            for file in "$build_dir"/$pattern; do
                if [[ -f "$file" ]]; then
                    remove_with_logging "$file" "build file: $(basename "$file")"
                fi
            done
        done

        # Remove executables
        local executables=(
            "Puzzle71Solver"
            "Puzzle71Solver.exe"
            "puzzle71_tests"
            "puzzle71_tests.exe"
        )

        for exe in "${executables[@]}"; do
            remove_with_logging "$build_dir/$exe" "executable: $exe"
        done

        # Keep CMake cache if requested
        if [[ "$KEEP_CACHE" == false ]]; then
            remove_with_logging "$build_dir/CMakeCache.txt" "CMake cache"
        fi

        # Check if build directory is empty and can be removed
        if [[ "$DRY_RUN" != true ]]; then
            local remaining_files
            remaining_files=$(find "$build_dir" -type f | wc -l)
            if [[ "$remaining_files" -eq 0 ]]; then
                log_info "Build directory is empty, removing it"
                rmdir "$build_dir" 2>/dev/null || true
            else
                log_info "Build directory contains $remaining_files remaining files"
            fi
        fi
    else
        log_info "Build directory not found: $build_dir"
    fi

    progress_end "Build artifacts cleanup"
}

# Clean dependencies
clean_deps() {
    progress_start "Cleaning dependencies"

    local project_root
    project_root="$(get_project_root)"

    # Remove third-party build directories
    local third_party_dirs=(
        "$project_root/third_party/build"
        "$project_root/build/third_party"
        "$project_root/external/build"
    )

    for dir in "${third_party_dirs[@]}"; do
        remove_with_logging "$dir" "third-party build directory"
    done

    # Remove dependency cache
    local cache_dirs=(
        "$project_root/.deps"
        "$project_root/.cache"
        "$project_root/build/_deps"
    )

    for dir in "${cache_dirs[@]}"; do
        remove_with_logging "$dir" "dependency cache"
    done

    # Keep external dependencies if requested
    if [[ "$KEEP_DEPS" == false ]]; then
        # Remove extracted dependencies
        local extracted_dirs=(
            "$project_root/src/extracted"
            "$project_root/external"
        )

        for dir in "${extracted_dirs[@]}"; do
            # Be more conservative with extracted dependencies
            if [[ "$DRY_RUN" == true ]]; then
                log_info "[DRY RUN] Would remove extracted dependencies in: $dir"
            else
                log_warning "Skipping extracted dependencies (use --force to remove)"
            fi
        done
    fi

    progress_end "Dependencies cleanup"
}

# Clean profiling results
clean_profiles() {
    progress_start "Cleaning profiling results"

    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    # Profile directories
    local profile_dirs=(
        "$build_dir/profiles"
        "$build_dir/nsight-results"
        "$project_root/profiles"
    )

    for dir in "${profile_dirs[@]}"; do
        if [[ -d "$dir" ]]; then
            local file_count
            file_count=$(count_files "$dir")
            local size
            size=$(calculate_size "$dir")

            log_info "Profile directory: $dir"
            log_info "Files: $file_count, Size: $size"

            if [[ -n "$OLDER_THAN" ]]; then
                # Remove only old files
                find "$dir" -type f -mtime "+$OLDER_THAN" -print0 | while IFS= read -r -d '' file; do
                    remove_with_logging "$file" "old profile file: $(basename "$file")"
                done
            else
                remove_with_logging "$dir" "profile directory"
            fi
        fi
    done

    # Profile files
    local profile_files=(
        "*.prof"
        "*.csv"
        "*.nsight-rep"
        "nvprof.log"
        "cuda_memcheck.log"
    )

    for pattern in "${profile_files[@]}"; do
        for file in "$build_dir"/$pattern "$project_root"/$pattern; do
            if [[ -f "$file" ]]; then
                if [[ -n "$OLDER_THAN" ]] && is_older_than "$file" "$OLDER_THAN"; then
                    remove_with_logging "$file" "old profile file: $(basename "$file")"
                elif [[ -z "$OLDER_THAN" ]]; then
                    remove_with_logging "$file" "profile file: $(basename "$file")"
                fi
            fi
        done
    done

    progress_end "Profiling results cleanup"
}

# Clean benchmark results
clean_benchmarks() {
    progress_start "Cleaning benchmark results"

    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    # Benchmark directories
    local benchmark_dirs=(
        "$build_dir/benchmarks"
        "$project_root/benchmarks"
    )

    for dir in "${benchmark_dirs[@]}"; do
        if [[ -d "$dir" ]]; then
            local file_count
            file_count=$(count_files "$dir")
            local size
            size=$(calculate_size "$dir")

            log_info "Benchmark directory: $dir"
            log_info "Files: $file_count, Size: $size"

            if [[ "$KEEP_RESULTS" == false ]]; then
                if [[ -n "$OLDER_THAN" ]]; then
                    find "$dir" -type f -mtime "+$OLDER_THAN" -print0 | while IFS= read -r -d '' file; do
                        remove_with_logging "$file" "old benchmark file: $(basename "$file")"
                    done
                else
                    remove_with_logging "$dir" "benchmark directory"
                fi
            else
                log_info "Keeping benchmark results (as requested)"
            fi
        fi
    done

    # Baseline files
    local baseline_files=(
        "$project_root/baseline*.json"
        "$build_dir/baseline*.json"
    )

    for file in "${baseline_files[@]}"; do
        if [[ -f "$file" ]]; then
            if [[ "$KEEP_RESULTS" == false ]]; then
                remove_with_logging "$file" "baseline file: $(basename "$file")"
            else
                log_info "Keeping baseline file: $(basename "$file")"
            fi
        fi
    done

    progress_end "Benchmark results cleanup"
}

# Clean log files
clean_logs() {
    progress_start "Cleaning log files"

    local project_root
    project_root="$(get_project_root")
    local build_dir="$project_root/build"

    # Log directories
    local log_dirs=(
        "$build_dir/logs"
        "$project_root/logs"
        "$project_root/.logs"
    )

    for dir in "${log_dirs[@]}"; do
        if [[ -d "$dir" ]]; then
            if [[ -n "$OLDER_THAN" ]]; then
                find "$dir" -type f -name "*.log" -mtime "+$OLDER_THAN" -print0 | while IFS= read -r -d '' file; do
                    remove_with_logging "$file" "old log file: $(basename "$file")"
                done
            else
                remove_with_logging "$dir" "log directory"
            fi
        fi
    done

    # Log files
    local log_files=(
        "*.log"
        "*.out"
        "*.err"
        "test-results.xml"
        "coverage.xml"
    )

    for pattern in "${log_files[@]}"; do
        for file in "$build_dir"/$pattern "$project_root"/$pattern; do
            if [[ -f "$file" ]]; then
                if [[ -n "$OLDER_THAN" ]] && is_older_than "$file" "$OLDER_THAN"; then
                    remove_with_logging "$file" "old log file: $(basename "$file")"
                elif [[ -z "$OLDER_THAN" ]]; then
                    remove_with_logging "$file" "log file: $(basename "$file")"
                fi
            fi
        done
    done

    progress_end "Log files cleanup"
}

# Clean temporary files
clean_temp() {
    progress_start "Cleaning temporary files"

    local project_root
    project_root="$(get_project_root")
    local build_dir="$project_root/build"

    # Temp directories
    local temp_dirs=(
        "$build_dir/temp"
        "$build_dir/tmp"
        "$project_root/temp"
        "$project_root/tmp"
        "$project_root/.tmp"
    )

    for dir in "${temp_dirs[@]}"; do
        remove_with_logging "$dir" "temporary directory"
    done

    # Temporary files
    local temp_files=(
        "*.tmp"
        "*.temp"
        "*.bak"
        "*~"
        ".DS_Store"
        "Thumbs.db"
        "*.swp"
        "*.swo"
        ".#*"
        "#*#"
    )

    for pattern in "${temp_files[@]}"; do
        find "$project_root" -name "$pattern" -type f -print0 | while IFS= read -r -d '' file; do
            remove_with_logging "$file" "temporary file: $(basename "$file")"
        done
    done

    # Editor backup files
    local editor_dirs=(
        "$project_root/.vscode"
        "$project_root/.idea"
        "$project_root/.cache"
    )

    for dir in "${editor_dirs[@]}"; do
        # Be conservative with editor directories
        if [[ -d "$dir" ]]; then
            log_info "Found editor directory: $dir (use --force to remove)"
        fi
    done

    progress_end "Temporary files cleanup"
}

# Clean git ignored files
clean_git() {
    progress_start "Cleaning git ignored files"

    local project_root
    project_root="$(get_project_root)"

    cd "$project_root"

    if [[ ! -d ".git" ]]; then
        log_warning "Not a git repository, skipping git clean"
        return 0
    fi

    # Show what would be cleaned
    if [[ "$DRY_RUN" == true ]]; then
        log_info "[DRY RUN] Git clean would remove:"
        git clean -ndX
        return 0
    fi

    # Perform git clean
    if [[ "$VERBOSE" == true ]]; then
        log_info "Running git clean..."
        git clean -fdX
    else
        git clean -fdX > /dev/null 2>&1
    fi

    progress_end "Git ignored files cleanup"
}

# Show cleanup summary
show_cleanup_summary() {
    cat << EOF

🧹 Cleanup completed successfully!

Cleanup type: $CLEAN_TYPE
${OLDER_THAN:+Files older than: $OLDER_THAN days}
${DRY_RUN:+Mode: Dry run (no files were actually deleted)}

Space freed and files removed based on selected cleanup type.

Next steps:
- Rebuild project: scripts build
- Run benchmarks: scripts benchmark
- Check status: scripts env
EOF
}

# Main cleanup function
main() {
    log_info "Starting Puzzle71Solver cleanup..."

    # Parse arguments
    parse_args "$@"

    # Show configuration
    log_info "Cleanup configuration:"
    log_info "  Type: $CLEAN_TYPE"
    log_info "  Dry run: $DRY_RUN"
    log_info "  Force: $FORCE"
    log_info "  Keep cache: $KEEP_CACHE"
    log_info "  Keep deps: $KEEP_DEPS"
    log_info "  Keep results: $KEEP_RESULTS"

    # Confirm cleanup
    confirm_cleanup

    # Execute cleanup based on type
    case "$CLEAN_TYPE" in
        "build")
            clean_build
            ;;
        "all")
            clean_build
            clean_deps
            clean_profiles
            clean_benchmarks
            clean_logs
            clean_temp
            ;;
        "deps")
            clean_deps
            ;;
        "profiles")
            clean_profiles
            ;;
        "benchmarks")
            clean_benchmarks
            ;;
        "logs")
            clean_logs
            ;;
        "temp")
            clean_temp
            ;;
        "git")
            clean_git
            ;;
        *)
            error_exit "Unknown cleanup type: $CLEAN_TYPE"
            ;;
    esac

    # Show summary
    show_cleanup_summary

    log_success "Cleanup completed successfully!"
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
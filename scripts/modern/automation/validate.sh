#!/bin/bash
# Puzzle71Solver - Comprehensive Validation Script
# Code quality, performance, and security validation

set -euo pipefail

# Get script directory and load utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=../utils.sh
source "$SCRIPT_DIR/../utils.sh"

# Default configuration
DEFAULT_VALIDATION_LEVEL="standard"
DEFAULT_OUTPUT_FORMAT="markdown"
DEFAULT_REPORT_DIR="build/validation"

# Show help
show_help() {
    cat << EOF
Puzzle71Solver Validation Script

USAGE:
    validate.sh [options] [validation_types...]

VALIDATION TYPES:
    code                Code quality and style validation
    build               Build system validation
    performance         Performance regression validation
    security            Security vulnerability validation
    docs                Documentation validation
    compatibility       Platform compatibility validation
    all                 Run all validations (default)

OPTIONS:
    --level <level>         Validation level (basic|standard|comprehensive) [default: $DEFAULT_VALIDATION_LEVEL]
    --output <format>       Output format (markdown|json|junit) [default: $DEFAULT_OUTPUT_FORMAT]
    --report-dir <dir>      Report output directory [default: $DEFAULT_REPORT_DIR]
    --baseline <file>       Performance baseline for comparison
    --threshold <percent>   Performance regression threshold [default: 5.0]
    --fix                  Attempt to fix minor issues automatically
    --continue-on-error     Continue validation despite errors
    --parallel              Run validations in parallel where possible
    --timeout <seconds>     Validation timeout [default: 1800]
    --ci-mode              CI/CD mode with specific output formats
    --verbose, -v          Enable verbose output
    --help, -h             Show this help

CODE QUALITY VALIDATION:
    --clang-format         Check code formatting
    --cppcheck             Static analysis
    --clang-tidy           Clang-tidy checks
    --include-what-you-use Check include dependencies

PERFORMANCE VALIDATION:
    --benchmark            Run performance benchmarks
    --memory-profile       Memory profiling
    --gpu-profile          GPU profiling
    --regression-check     Check for performance regression

SECURITY VALIDATION:
    --security-scan        Security vulnerability scan
    --dependency-check     Check for vulnerable dependencies
    --code-analysis        Security code analysis

EXAMPLES:
    validate.sh                           # Standard validation of all types
    validate.sh --level comprehensive      # Comprehensive validation
    validate.sh code performance          # Specific validation types
    validate.sh --ci-mode --format json   # CI/CD mode with JSON output
    validate.sh --fix --clang-format      # Fix formatting issues

ENVIRONMENT VARIABLES:
    VALIDATION_LEVEL       Override validation level
    VALIDATION_OUTPUT_DIR  Override report directory
    VALIDATION_BASELINE    Override performance baseline
EOF
}

# Parse command line arguments
parse_args() {
    VALIDATION_LEVEL="${VALIDATION_LEVEL:-$DEFAULT_VALIDATION_LEVEL}"
    OUTPUT_FORMAT="${VALIDATION_OUTPUT:-$DEFAULT_OUTPUT_FORMAT}"
    REPORT_DIR="${VALIDATION_OUTPUT_DIR:-$DEFAULT_REPORT_DIR}"
    BASELINE_FILE="${VALIDATION_BASELINE:-}"
    REGRESSION_THRESHOLD="5.0"
    AUTO_FIX=false
    CONTINUE_ON_ERROR=false
    PARALLEL_VALIDATION=false
    TIMEOUT="1800"
    CI_MODE=false
    VERBOSE=false

    # Code quality options
    CLANG_FORMAT_CHECK=false
    CPPCHECK_ANALYSIS=false
    CLANG_TIDY_CHECK=false
    INCLUDE_WHAT_YOU_USE=false

    # Performance options
    RUN_BENCHMARK=false
    MEMORY_PROFILE=false
    GPU_PROFILE=false
    REGRESSION_CHECK=false

    # Security options
    SECURITY_SCAN=false
    DEPENDENCY_CHECK=false
    CODE_ANALYSIS=false

    VALIDATION_TYPES=()

    while [[ $# -gt 0 ]]; do
        case $1 in
            --level)
                VALIDATION_LEVEL="$2"
                shift 2
                ;;
            --output)
                OUTPUT_FORMAT="$2"
                shift 2
                ;;
            --report-dir)
                REPORT_DIR="$2"
                shift 2
                ;;
            --baseline)
                BASELINE_FILE="$2"
                shift 2
                ;;
            --threshold)
                REGRESSION_THRESHOLD="$2"
                shift 2
                ;;
            --fix)
                AUTO_FIX=true
                shift
                ;;
            --continue-on-error)
                CONTINUE_ON_ERROR=true
                shift
                ;;
            --parallel)
                PARALLEL_VALIDATION=true
                shift
                ;;
            --timeout)
                TIMEOUT="$2"
                shift 2
                ;;
            --ci-mode)
                CI_MODE=true
                OUTPUT_FORMAT="junit"
                CONTINUE_ON_ERROR=true
                shift
                ;;
            --verbose|-v)
                VERBOSE=true
                shift
                ;;
            --clang-format)
                CLANG_FORMAT_CHECK=true
                shift
                ;;
            --cppcheck)
                CPPCHECK_ANALYSIS=true
                shift
                ;;
            --clang-tidy)
                CLANG_TIDY_CHECK=true
                shift
                ;;
            --include-what-you-use)
                INCLUDE_WHAT_YOU_USE=true
                shift
                ;;
            --benchmark)
                RUN_BENCHMARK=true
                shift
                ;;
            --memory-profile)
                MEMORY_PROFILE=true
                shift
                ;;
            --gpu-profile)
                GPU_PROFILE=true
                shift
                ;;
            --regression-check)
                REGRESSION_CHECK=true
                shift
                ;;
            --security-scan)
                SECURITY_SCAN=true
                shift
                ;;
            --dependency-check)
                DEPENDENCY_CHECK=true
                shift
                ;;
            --code-analysis)
                CODE_ANALYSIS=true
                shift
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            code|build|performance|security|docs|compatibility|all)
                VALIDATION_TYPES+=("$1")
                shift
                ;;
            *)
                error_exit "Unknown option: $1"
                ;;
        esac
    done

    # Set default validation type if none specified
    if [[ ${#VALIDATION_TYPES[@]} -eq 0 ]]; then
        VALIDATION_TYPES=("all")
    fi

    # Set validation options based on level
    case "$VALIDATION_LEVEL" in
        "basic")
            # Basic checks only
            ;;
        "standard")
            # Standard checks
            CLANG_FORMAT_CHECK=true
            CPPCHECK_ANALYSIS=true
            RUN_BENCHMARK=true
            REGRESSION_CHECK=true
            ;;
        "comprehensive")
            # All checks
            CLANG_FORMAT_CHECK=true
            CPPCHECK_ANALYSIS=true
            CLANG_TIDY_CHECK=true
            INCLUDE_WHAT_YOU_USE=true
            RUN_BENCHMARK=true
            MEMORY_PROFILE=true
            GPU_PROFILE=true
            REGRESSION_CHECK=true
            SECURITY_SCAN=true
            DEPENDENCY_CHECK=true
            CODE_ANALYSIS=true
            ;;
    esac
}

# Validate validation environment
validate_validation_environment() {
    log_debug "Validating validation environment..."

    local project_root
    project_root="$(get_project_root)"
    local build_dir="$project_root/build"

    # Check project structure
    require_dir "$project_root" "Project root"

    # Create report directory
    ensure_dir "$REPORT_DIR"

    # Check for required tools based on validation types
    for validation_type in "${VALIDATION_TYPES[@]}"; do
        case "$validation_type" in
            "code")
                validate_code_tools
                ;;
            "performance")
                validate_performance_tools
                ;;
            "security")
                validate_security_tools
                ;;
            "build")
                validate_build_tools
                ;;
        esac
    done

    log_debug "Validation environment validated"
}

# Validate code analysis tools
validate_code_tools() {
    if [[ "$CLANG_FORMAT_CHECK" == true ]]; then
        if ! command_exists "clang-format"; then
            log_warning "clang-format not found, skipping formatting checks"
            CLANG_FORMAT_CHECK=false
        fi
    fi

    if [[ "$CPPCHECK_ANALYSIS" == true ]]; then
        if ! command_exists "cppcheck"; then
            log_warning "cppcheck not found, skipping static analysis"
            CPPCHECK_ANALYSIS=false
        fi
    fi

    if [[ "$CLANG_TIDY_CHECK" == true ]]; then
        if ! command_exists "clang-tidy"; then
            log_warning "clang-tidy not found, skipping tidy checks"
            CLANG_TIDY_CHECK=false
        fi
    fi

    if [[ "$INCLUDE_WHAT_YOU_USE" == true ]]; then
        if ! command_exists "include-what-you-use"; then
            log_warning "include-what-you-use not found, skipping include analysis"
            INCLUDE_WHAT_YOU_USE=false
        fi
    fi
}

# Validate performance tools
validate_performance_tools() {
    if [[ "$RUN_BENCHMARK" == true ]] || [[ "$REGRESSION_CHECK" == true ]]; then
        local project_root
        project_root="$(get_project_root)"
        local build_dir="$project_root/build"
        local executable="$build_dir/Puzzle71Solver"

        if [[ ! -f "$executable" ]]; then
            log_warning "Executable not found: $executable"
            log_info "Building executable first..."
            "$SCRIPT_DIR/../workflow/build.sh" Puzzle71Solver
        fi
    fi

    if [[ "$GPU_PROFILE" == true ]] && ! check_gpu_available; then
        log_warning "GPU not available, skipping GPU profiling"
        GPU_PROFILE=false
    fi
}

# Validate security tools
validate_security_tools() {
    if [[ "$SECURITY_SCAN" == true ]]; then
        if ! command_exists "semgrep"; then
            log_warning "semgrep not found, skipping security scan"
            SECURITY_SCAN=false
        fi
    fi

    if [[ "$DEPENDENCY_CHECK" == true ]]; then
        if ! command_exists "safety"; then
            log_warning "safety not found, skipping dependency check"
            DEPENDENCY_CHECK=false
        fi
    fi
}

# Validate build tools
validate_build_tools() {
    require_command "cmake"
    require_command "make"

    if ! check_gpu_available; then
        log_warning "CUDA not available, some build validations may fail"
    fi
}

# Prepare validation environment
prepare_validation_environment() {
    progress_start "Preparing validation environment"

    local project_root
    project_root="$(get_project_root)"

    cd "$project_root"

    # Set validation environment variables
    export VALIDATION_MODE="1"
    export VALIDATION_LEVEL="$VALIDATION_LEVEL"
    export VALIDATION_REPORT_DIR="$REPORT_DIR"

    # Create validation timestamp
    VALIDATION_TIMESTAMP="$(get_timestamp)"
    REPORT_FILE="$REPORT_DIR/validation_report_${VALIDATION_TIMESTAMP}.${OUTPUT_FORMAT}"

    # Initialize validation results
    VALIDATION_RESULTS=()
    VALIDATION_ERRORS=0
    VALIDATION_WARNINGS=0

    progress_end "Validation environment preparation"
}

# Run code quality validation
validate_code_quality() {
    progress_start "Running code quality validation"

    local code_report="$REPORT_DIR/code_quality_${VALIDATION_TIMESTAMP}.md"

    cat > "$code_report" << EOF
# Code Quality Validation Report

## Configuration
- **Validation Level**: $VALIDATION_LEVEL
- **Timestamp**: $(date -Iseconds)
- **Auto Fix**: $AUTO_FIX

## Results

EOF

    # Clang format check
    if [[ "$CLANG_FORMAT_CHECK" == true ]]; then
        log_info "Running clang-format checks..."
        run_clang_format_check "$code_report"
    fi

    # Cppcheck analysis
    if [[ "$CPPCHECK_ANALYSIS" == true ]]; then
        log_info "Running cppcheck analysis..."
        run_cppcheck_analysis "$code_report"
    fi

    # Clang tidy check
    if [[ "$CLANG_TIDY_CHECK" == true ]]; then
        log_info "Running clang-tidy checks..."
        run_clang_tidy_check "$code_report"
    fi

    # Include what you use
    if [[ "$INCLUDE_WHAT_YOU_USE" == true ]]; then
        log_info "Running include-what-you-use analysis..."
        run_include_what_you_use_check "$code_report"
    fi

    progress_end "Code quality validation"
}

# Run clang-format check
run_clang_format_check() {
    local report_file="$1"

    echo "### Clang Format Check" >> "$report_file"
    echo "" >> "$report_file"

    local project_root
    project_root="$(get_project_root)"

    # Find source files
    local source_files=()
    while IFS= read -r -d '' file; do
        source_files+=("$file")
    done < <(find "$project_root/src" -name "*.cpp" -o -name "*.cu" -o -name "*.h" -print0 2>/dev/null || true)

    if [[ ${#source_files[@]} -eq 0 ]]; then
        echo "No source files found for clang-format check" >> "$report_file"
        echo "" >> "$report_file"
        return 0
    fi

    local format_issues=0
    local formatted_files=()

    for file in "${source_files[@]}"; do
        if ! clang-format --dry-run --Werror "$file" > /dev/null 2>&1; then
            ((format_issues++))
            formatted_files+=("$file")

            if [[ "$AUTO_FIX" == true ]]; then
                log_info "Fixing formatting: $(basename "$file")"
                clang-format -i "$file"
            fi
        fi
    done

    echo "- **Files checked**: ${#source_files[@]}" >> "$report_file"
    echo "- **Formatting issues**: $format_issues" >> "$report_file"

    if [[ $format_issues -gt 0 ]]; then
        echo "- **Status**: ❌ Issues found" >> "$report_file"
        echo "" >> "$report_file"
        echo "#### Files with formatting issues:" >> "$report_file"
        for file in "${formatted_files[@]}"; do
            echo "- \`$(basename "$file")\`" >> "$report_file"
        done

        if [[ "$AUTO_FIX" == true ]]; then
            echo "" >> "$report_file"
            echo "**Auto-fix applied** - Run \`git diff\` to see changes" >> "$report_file"
        fi

        ((VALIDATION_WARNINGS += format_issues))
    else
        echo "- **Status**: ✅ All files properly formatted" >> "$report_file"
    fi

    echo "" >> "$report_file"
}

# Run performance validation
validate_performance() {
    progress_start "Running performance validation"

    local perf_report="$REPORT_DIR/performance_${VALIDATION_TIMESTAMP}.md"

    cat > "$perf_report" << EOF
# Performance Validation Report

## Configuration
- **Timestamp**: $(date -Iseconds)
- **Baseline**: ${BASELINE_FILE:-none}
- **Regression Threshold**: ${REGRESSION_THRESHOLD}%

## Results

EOF

    # Run benchmarks
    if [[ "$RUN_BENCHMARK" == true ]]; then
        log_info "Running performance benchmarks..."
        run_performance_benchmarks "$perf_report"
    fi

    # Regression check
    if [[ "$REGRESSION_CHECK" == true ]]; then
        log_info "Checking for performance regression..."
        check_performance_regression "$perf_report"
    fi

    progress_end "Performance validation"
}

# Run performance benchmarks
run_performance_benchmarks() {
    local report_file="$1"

    echo "### Performance Benchmarks" >> "$report_file"
    echo "" >> "$report_file"

    local benchmark_script="$SCRIPT_DIR/../workflow/benchmark.sh"

    if [[ ! -f "$benchmark_script" ]]; then
        echo "- **Status**: ❌ Benchmark script not found" >> "$report_file"
        echo "" >> "$report_file"
        return 1
    fi

    local benchmark_args=(
        "--duration" "60"  # Short benchmark for validation
        "--output" "json"
        "--output-dir" "$REPORT_DIR"
    )

    if [[ -n "$BASELINE_FILE" ]]; then
        benchmark_args+=("--baseline" "$BASELINE_FILE")
    fi

    local benchmark_exit_code=0
    timeout "$TIMEOUT" "$benchmark_script" "${benchmark_args[@]}" || benchmark_exit_code=$?

    if [[ $benchmark_exit_code -eq 0 ]]; then
        echo "- **Status**: ✅ Benchmarks completed successfully" >> "$report_file"

        # Parse results if available
        local results_file="$REPORT_DIR/benchmark_*.json"
        if ls $results_file 1> /dev/null 2>&1; then
            echo "- **Results**: See JSON files in $REPORT_DIR" >> "$report_file"
        fi
    else
        echo "- **Status**: ❌ Benchmarks failed (exit code: $benchmark_exit_code)" >> "$report_file"
        ((VALIDATION_ERRORS++))
    fi

    echo "" >> "$report_file"
}

# Check performance regression
check_performance_regression() {
    local report_file="$1"

    echo "### Performance Regression Check" >> "$report_file"
    echo "" >> "$report_file"

    if [[ -z "$BASELINE_FILE" || ! -f "$BASELINE_FILE" ]]; then
        echo "- **Status**: ⚠️ No baseline file provided" >> "$report_file"
        echo "" >> "$report_file"
        return 0
    fi

    echo "- **Baseline**: $BASELINE_FILE" >> "$report_file"
    echo "- **Threshold**: ${REGRESSION_THRESHOLD}%" >> "$report_file"

    # This would need detailed implementation based on baseline format
    echo "- **Status**: ⚠️ Regression analysis not yet implemented" >> "$report_file"
    echo "" >> "$report_file"
}

# Generate final validation report
generate_final_report() {
    progress_start "Generating final validation report"

    local final_report="$REPORT_FILE"

    cat > "$final_report" << EOF
# Puzzle71Solver Validation Report

## Summary
- **Validation Level**: $VALIDATION_LEVEL
- **Timestamp**: $(date -Iseconds)
- **Total Errors**: $VALIDATION_ERRORS
- **Total Warnings**: $VALIDATION_WARNINGS

## Overall Status
EOF

    if [[ $VALIDATION_ERRORS -eq 0 ]]; then
        if [[ $VALIDATION_WARNINGS -eq 0 ]]; then
            echo "- **Status**: ✅ All validations passed" >> "$final_report"
        else
            echo "- **Status**: ⚠️ Passed with warnings" >> "$final_report"
        fi
    else
        echo "- **Status**: ❌ Validations failed" >> "$final_report"
    fi

    echo "" >> "$final_report"
    echo "## Detailed Results" >> "$final_report"
    echo "" >> "$final_report"

    # Link to detailed reports
    for validation_type in "${VALIDATION_TYPES[@]}"; do
        case "$validation_type" in
            "code"|"all")
                if [[ -f "$REPORT_DIR/code_quality_${VALIDATION_TIMESTAMP}.md" ]]; then
                    echo "### [Code Quality Details](code_quality_${VALIDATION_TIMESTAMP}.md)" >> "$final_report"
                fi
                ;;
            "performance"|"all")
                if [[ -f "$REPORT_DIR/performance_${VALIDATION_TIMESTAMP}.md" ]]; then
                    echo "### [Performance Details](performance_${VALIDATION_TIMESTAMP}.md)" >> "$final_report"
                fi
                ;;
            "security"|"all")
                if [[ -f "$REPORT_DIR/security_${VALIDATION_TIMESTAMP}.md" ]]; then
                    echo "### [Security Details](security_${VALIDATION_TIMESTAMP}.md)" >> "$final_report"
                fi
                ;;
            "build"|"all")
                if [[ -f "$REPORT_DIR/build_${VALIDATION_TIMESTAMP}.md" ]]; then
                    echo "### [Build Details](build_${VALIDATION_TIMESTAMP}.md)" >> "$final_report"
                fi
                ;;
            "docs"|"all")
                if [[ -f "$REPORT_DIR/docs_${VALIDATION_TIMESTAMP}.md" ]]; then
                    echo "### [Documentation Details](docs_${VALIDATION_TIMESTAMP}.md)" >> "$final_report"
                fi
                ;;
            "compatibility"|"all")
                if [[ -f "$REPORT_DIR/compatibility_${VALIDATION_TIMESTAMP}.md" ]]; then
                    echo "### [Compatibility Details](compatibility_${VALIDATION_TIMESTAMP}.md)" >> "$final_report"
                fi
                ;;
        esac
    done

    progress_end "Final validation report generation"
}

# Show validation summary
show_validation_summary() {
    cat << EOF

🔍 Validation completed!

Summary:
- Errors: $VALIDATION_ERRORS
- Warnings: $VALIDATION_WARNINGS
- Report: $REPORT_FILE

EOF

    if [[ $VALIDATION_ERRORS -eq 0 ]]; then
        if [[ $VALIDATION_WARNINGS -eq 0 ]]; then
            log_success "All validations passed! 🎉"
        else
            log_warning "Validations passed with $VALIDATION_WARNINGS warnings ⚠️"
        fi
    else
        log_error "$VALIDATION_ERRORS validation errors found! ❌"
        return 1
    fi

    echo "Next steps:"
    echo "- Review detailed report: $REPORT_FILE"
    echo "- Fix any issues found"
    echo "- Re-run validation: scripts validate"
    echo ""
}

# Main validation function
main() {
    log_info "Starting Puzzle71Solver validation..."

    # Parse arguments
    parse_args "$@"

    # Validate environment
    validate_validation_environment

    # Show configuration
    log_info "Validation configuration:"
    log_info "  Level: $VALIDATION_LEVEL"
    log_info "  Types: ${VALIDATION_TYPES[*]}"
    log_info "  Output: $OUTPUT_FORMAT"
    log_info "  Report dir: $REPORT_DIR"

    # Prepare environment
    prepare_validation_environment

    # Run validations based on types
    local validation_exit_code=0
    for validation_type in "${VALIDATION_TYPES[@]}"; do
        case "$validation_type" in
            "code")
                validate_code_quality || validation_exit_code=$?
                ;;
            "performance")
                validate_performance || validation_exit_code=$?
                ;;
            "security")
                validate_security || validation_exit_code=$?
                ;;
            "build")
                validate_build || validation_exit_code=$?
                ;;
            "docs")
                validate_docs || validation_exit_code=$?
                ;;
            "compatibility")
                validate_compatibility || validation_exit_code=$?
                ;;
            "all")
                validate_code_quality || validation_exit_code=$?
                validate_performance || validation_exit_code=$?
                validate_security || validation_exit_code=$?
                validate_build || validation_exit_code=$?
                validate_docs || validation_exit_code=$?
                validate_compatibility || validation_exit_code=$?
                ;;
        esac

        # Continue on error if requested
        if [[ $validation_exit_code -ne 0 && "$CONTINUE_ON_ERROR" != true ]]; then
            log_error "Validation failed, aborting..."
            break
        fi
    done

    # Generate final report
    generate_final_report

    # Show summary
    show_validation_summary

    exit $validation_exit_code
}

# Placeholder functions for validation types not yet implemented
validate_security() {
    progress_start "Running security validation"
    log_info "Security validation placeholder - not yet implemented"
    progress_end "Security validation"
}

validate_build() {
    progress_start "Running build validation"
    log_info "Build validation placeholder - not yet implemented"
    progress_end "Build validation"
}

validate_docs() {
    progress_start "Running documentation validation"
    log_info "Documentation validation placeholder - not yet implemented"
    progress_end "Documentation validation"
}

validate_compatibility() {
    progress_start "Running compatibility validation"
    log_info "Compatibility validation placeholder - not yet implemented"
    progress_end "Compatibility validation"
}

run_cppcheck_analysis() {
    local report_file="$1"
    echo "### Cppcheck Static Analysis" >> "$report_file"
    echo "- **Status**: ⚠️ Cppcheck analysis not yet implemented" >> "$report_file"
    echo "" >> "$report_file"
}

run_clang_tidy_check() {
    local report_file="$1"
    echo "### Clang Tidy Check" >> "$report_file"
    echo "- **Status**: ⚠️ Clang-tidy analysis not yet implemented" >> "$report_file"
    echo "" >> "$report_file"
}

run_include_what_you_use_check() {
    local report_file="$1"
    echo "### Include What You Use Check" >> "$report_file"
    echo "- **Status**: ⚠️ Include analysis not yet implemented" >> "$report_file"
    echo "" >> "$report_file"
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
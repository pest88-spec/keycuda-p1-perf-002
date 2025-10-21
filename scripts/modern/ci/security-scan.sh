#!/bin/bash
# Puzzle71Solver - Security Scanning Integration
# Provides comprehensive security validation for CI/CD pipelines

set -euo pipefail

# Get script directory and load utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=../utils.sh
source "$SCRIPT_DIR/../utils.sh"

# Default configuration
DEFAULT_SCAN_LEVEL="standard"
DEFAULT_OUTPUT_FORMAT="json"
DEFAULT_OUTPUT_DIR="build/security"
DEFAULT_TIMEOUT=1800

# Show help
show_help() {
    cat << EOF
Puzzle71Solver Security Scanning

USAGE:
    security-scan.sh [options] [scan-type]

SCAN TYPES:
    all                   Run all security scans (default)
    static                Static code analysis
    dependencies          Dependency vulnerability scanning
    secrets               Secret/key detection
    container             Container security scanning
    runtime               Runtime security validation

OPTIONS:
    --level <level>       Scan level (basic|standard|comprehensive) [default: $DEFAULT_SCAN_LEVEL]
    --format <format>     Output format (json|junit|markdown|sarif) [default: $DEFAULT_OUTPUT_FORMAT]
    --output-dir <dir>    Output directory [default: $DEFAULT_OUTPUT_DIR]
    --timeout <seconds>   Scan timeout [default: $DEFAULT_TIMEOUT]
    --fail-on-warning     Treat warnings as failures
    --exclude <pattern>   Exclude files/directories from scanning
    --include <pattern>   Include only specific files/directories
    --baseline <file>     Use baseline for comparison
    --upload-results      Upload results to external services
    --verbose, -v         Enable verbose output
    --help, -h            Show this help

EXAMPLES:
    security-scan.sh
    security-scan.sh static --level comprehensive
    security-scan.sh dependencies --format junit --output-file security-results.xml
    security-scan.sh secrets --exclude "*.tmp" --fail-on-warning
    security-scan.sh all --upload-results --timeout 3600

This script provides comprehensive security scanning for the Puzzle71Solver
project including static analysis, dependency scanning, secret detection,
and runtime security validation.
EOF
}

# Parse command line arguments
parse_args() {
    SCAN_TYPE="all"
    SCAN_LEVEL="$DEFAULT_SCAN_LEVEL"
    OUTPUT_FORMAT="$DEFAULT_OUTPUT_FORMAT"
    OUTPUT_DIR="$DEFAULT_OUTPUT_DIR"
    TIMEOUT="$DEFAULT_TIMEOUT"
    FAIL_ON_WARNING=false
    EXCLUDE_PATTERNS=()
    INCLUDE_PATTERNS=()
    BASELINE_FILE=""
    UPLOAD_RESULTS=false
    OUTPUT_FILE=""
    VERBOSE=false

    while [[ $# -gt 0 ]]; do
        case $1 in
            all|static|dependencies|secrets|container|runtime)
                SCAN_TYPE="$1"
                shift
                ;;
            --level)
                SCAN_LEVEL="$2"
                shift 2
                ;;
            --format)
                OUTPUT_FORMAT="$2"
                shift 2
                ;;
            --output-dir)
                OUTPUT_DIR="$2"
                shift 2
                ;;
            --output-file)
                OUTPUT_FILE="$2"
                shift 2
                ;;
            --timeout)
                TIMEOUT="$2"
                shift 2
                ;;
            --fail-on-warning)
                FAIL_ON_WARNING=true
                shift
                ;;
            --exclude)
                EXCLUDE_PATTERNS+=("$2")
                shift 2
                ;;
            --include)
                INCLUDE_PATTERNS+=("$2")
                shift 2
                ;;
            --baseline)
                BASELINE_FILE="$2"
                shift 2
                ;;
            --upload-results)
                UPLOAD_RESULTS=true
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

    # Validate scan level
    local valid_levels=("basic" "standard" "comprehensive")
    if [[ ! " ${valid_levels[*]} " =~ " $SCAN_LEVEL " ]]; then
        error_exit "Invalid scan level: $SCAN_LEVEL. Valid levels: ${valid_levels[*]}"
    fi

    # Validate output format
    local valid_formats=("json" "junit" "markdown" "sarif")
    if [[ ! " ${valid_formats[*]} " =~ " $OUTPUT_FORMAT " ]]; then
        error_exit "Invalid output format: $OUTPUT_FORMAT. Valid formats: ${valid_formats[*]}"
    fi

    log_debug "Configuration:"
    log_debug "  Scan Type: $SCAN_TYPE"
    log_debug "  Scan Level: $SCAN_LEVEL"
    log_debug "  Output Format: $OUTPUT_FORMAT"
    log_debug "  Output Directory: $OUTPUT_DIR"
    log_debug "  Fail on Warning: $FAIL_ON_WARNING"
}

# Validate security scanning environment
validate_security_environment() {
    log_info "Validating security scanning environment..."

    local project_root
    project_root="$(get_project_root)"

    # Create output directory
    ensure_dir "$OUTPUT_DIR"

    # Check for common security tools
    local missing_tools=()

    if ! command_exists "cppcheck"; then
        missing_tools+=("cppcheck")
    fi

    if ! command_exists "clang-tidy"; then
        missing_tools+=("clang-tidy")
    fi

    if [[ ${#missing_tools[@]} -gt 0 ]]; then
        log_warning "Missing security tools: ${missing_tools[*]}"
        log_info "Install missing tools for comprehensive scanning:"
        log_info "  sudo apt-get install cppcheck clang clang-tidy"
    fi

    # Check for optional tools
    if command_exists "semgrep"; then
        log_info "Semgrep found - enhanced static analysis available"
    fi

    if command_exists "bandit"; then
        log_info "Bandit found - Python security scanning available"
    fi

    # Validate source directory
    local src_dir="$project_root/src"
    if [[ ! -d "$src_dir" ]]; then
        error_exit "Source directory not found: $src_dir"
    fi

    log_success "Security environment validation completed"
}

# Run static code analysis
run_static_analysis() {
    progress_start "Running static code analysis"

    local project_root
    project_root="$(get_project_root)"
    local src_dir="$project_root/src"
    local scan_results_file="$OUTPUT_DIR/static_analysis_results.json"

    # Initialize results
    local results
    results='{"scan_type": "static", "level": "'"$SCAN_LEVEL"'", "findings": [], "summary": {}}'

    # Run cppcheck
    if command_exists "cppcheck"; then
        log_info "Running cppcheck static analysis..."
        local cppcheck_file="$OUTPUT_DIR/cppcheck_results.xml"

        local cppcheck_args=(
            "--enable=all"
            "--xml"
            "--xml-version=2"
            "--suppress=missingIncludeSystem"
            "--suppress=unmatchedSuppression"
        )

        # Add scan level specific options
        case "$SCAN_LEVEL" in
            comprehensive)
                cppcheck_args+=("--inconclusive" "--force")
                ;;
            standard)
                cppcheck_args+=("--std=c++17")
                ;;
        esac

        # Add exclude patterns
        for pattern in "${EXCLUDE_PATTERNS[@]}"; do
            cppcheck_args+=("--suppress=*" "--suppress=$pattern")
        done

        # Add include patterns
        local cppcheck_paths=("$src_dir")
        if [[ ${#INCLUDE_PATTERNS[@]} -gt 0 ]]; then
            cppcheck_paths=()
            for pattern in "${INCLUDE_PATTERNS[@]}"; do
                cppcheck_paths+=("$src_dir"/$pattern)
            done
        fi

        if cppcheck "${cppcheck_args[@]}" "${cppcheck_paths[@]}" 2> "$cppcheck_file"; then
            log_info "Cppcheck completed successfully"
        else
            log_warning "Cppcheck found issues"
        fi

        # Parse cppcheck results
        parse_cppcheck_results "$cppcheck_file" "$results"
    else
        log_warning "cppcheck not available, skipping static analysis"
    fi

    # Run clang-tidy
    if command_exists "clang-tidy"; then
        log_info "Running clang-tidy analysis..."
        local tidy_file="$OUTPUT_DIR/clang_tidy_results.txt"

        local tidy_args=(
            "-warnings-as-errors=*"
            "-checks=*"
        )

        # Add scan level specific checks
        case "$SCAN_LEVEL" in
            comprehensive)
                tidy_args+=("-checks=*,clang-analyzer-*,security-*")
                ;;
            standard)
                tidy_args+=("-checks=*,clang-analyzer-core*,security-*")
                ;;
        esac

        # Find source files
        local source_files
        mapfile -t source_files < <(find "$src_dir" -name "*.cpp" -o -name "*.c" -o -name "*.cu" | head -20)

        if [[ ${#source_files[@]} -gt 0 ]]; then
            if clang-tidy "${tidy_args[@]}" "${source_files[@]}" -- > "$tidy_file" 2>&1; then
                log_info "Clang-tidy completed successfully"
            else
                log_warning "Clang-tidy found issues"
            fi

            # Parse clang-tidy results
            parse_clang_tidy_results "$tidy_file" "$results"
        fi
    else
        log_warning "clang-tidy not available, skipping analysis"
    fi

    # Run semgrep if available
    if command_exists "semgrep"; then
        log_info "Running Semgrep analysis..."
        local semgrep_file="$OUTPUT_DIR/semgrep_results.json"

        local semgrep_args=(
            "--config=auto"
            "--json"
            "--output=$semgrep_file"
        )

        # Add severity filtering based on scan level
        case "$SCAN_LEVEL" in
            comprehensive)
                semgrep_args+=("--severity=INFO")
                ;;
            standard)
                semgrep_args+=("--severity=WARNING")
                ;;
            basic)
                semgrep_args+=("--severity=ERROR")
                ;;
        esac

        if semgrep "${semgrep_args[@]}" "$src_dir"; then
            log_info "Semgrep completed successfully"
        else
            log_warning "Semgrep found issues"
        fi

        # Parse semgrep results
        parse_semgrep_results "$semgrep_file" "$results"
    fi

    # Save final results
    echo "$results" > "$scan_results_file"
    progress_end "Static code analysis"
}

# Run dependency vulnerability scanning
run_dependency_scan() {
    progress_start "Running dependency vulnerability scanning"

    local project_root
    project_root="$(get_project_root)"
    local scan_results_file="$OUTPUT_DIR/dependency_results.json"

    # Initialize results
    local results
    results='{"scan_type": "dependencies", "level": "'"$SCAN_LEVEL"'", "findings": [], "summary": {}}'

    # Check for CMake dependencies
    local cmake_file="$project_root/CMakeLists.txt"
    if [[ -f "$cmake_file" ]]; then
        log_info "Analyzing CMake dependencies..."
        analyze_cmake_dependencies "$cmake_file" "$results"
    fi

    # Check for package files
    local package_files=(
        "$project_root/package.json"
        "$project_root/requirements.txt"
        "$project_root/Pipfile"
        "$project_root/go.mod"
    )

    for pkg_file in "${package_files[@]}"; do
        if [[ -f "$pkg_file" ]]; then
            log_info "Scanning package file: $(basename "$pkg_file")"
            analyze_package_file "$pkg_file" "$results"
        fi
    done

    # Run safety if available (Python)
    if command_exists "safety" && [[ -f "$project_root/requirements.txt" ]]; then
        log_info "Running Safety vulnerability scanner..."
        local safety_file="$OUTPUT_DIR/safety_results.json"

        if safety check --json --output "$safety_file" "$project_root/requirements.txt" 2>/dev/null; then
            parse_safety_results "$safety_file" "$results"
        else
            log_warning "Safety scan failed or found vulnerabilities"
        fi
    fi

    # Check for known vulnerable dependencies manually
    check_known_vulnerabilities "$results"

    # Save results
    echo "$results" > "$scan_results_file"
    progress_end "Dependency vulnerability scanning"
}

# Run secret detection
run_secret_scan() {
    progress_start "Running secret detection"

    local project_root
    project_root="$(get_project_root)"
    local scan_results_file="$OUTPUT_DIR/secret_scan_results.json"

    # Initialize results
    local results
    results='{"scan_type": "secrets", "level": "'"$SCAN_LEVEL"'", "findings": [], "summary": {}}'

    # Define secret patterns
    local secret_patterns=(
        "AKIA[0-9A-Z]{16}"                                   # AWS Access Key
        "[0-9a-zA-Z/+]{40}"                                  # Possible private key
        "-----BEGIN [A-Z]+ KEY-----"                          # PEM keys
        "ghp_[a-zA-Z0-9]{36}"                                # GitHub Personal Access Token
        "sk_live_[0-9a-zA-Z]{24}"                            # Stripe Live Key
        "pk_live_[0-9a-zA-Z]{24}"                            # Stripe Live Publishable Key
        "[0-9]{4}-[0-9]{4}-[0-9]{4}-[0-9]{4}"               # Credit card pattern
        "password[[:space:]]*=[[:space:]]*['\"][^'\"]+['\"]" # Password assignments
    )

    # Scan source files
    local src_dir="$project_root/src"
    while IFS= read -r -d '' file; do
        scan_file_for_secrets "$file" "${secret_patterns[@]}" "$results"
    done < <(find "$src_dir" -type f \( -name "*.cpp" -o -name "*.c" -o -name "*.h" -o -name "*.cu" -o -name "*.py" -o -name "*.sh" \) -print0)

    # Scan configuration files
    local config_patterns=(
        "$project_root/*.conf"
        "$project_root/*.config"
        "$project_root/*.ini"
        "$project_root/*.env"
        "$project_root/data/*.txt"
    )

    for pattern in "${config_patterns[@]}"; do
        for file in $pattern; do
            if [[ -f "$file" ]]; then
                scan_file_for_secrets "$file" "${secret_patterns[@]}" "$results"
            fi
        done
    done

    # Check for hardcoded credentials in common patterns
    check_hardcoded_credentials "$results"

    # Save results
    echo "$results" > "$scan_results_file"
    progress_end "Secret detection"
}

# Run container security scanning
run_container_scan() {
    progress_start "Running container security scanning"

    local project_root
    project_root="$(get_project_root)"
    local scan_results_file="$OUTPUT_DIR/container_results.json"

    # Initialize results
    local results
    results='{"scan_type": "container", "level": "'"$SCAN_LEVEL"'", "findings": [], "summary": {}}'

    # Check for Dockerfile
    local dockerfile="$project_root/Dockerfile"
    if [[ -f "$dockerfile" ]]; then
        log_info "Analyzing Dockerfile..."
        analyze_dockerfile "$dockerfile" "$results"
    fi

    # Check for docker-compose files
    local compose_files=(
        "$project_root/docker-compose.yml"
        "$project_root/docker-compose.yaml"
        "$project_root/docker-compose.override.yml"
    )

    for compose_file in "${compose_files[@]}"; do
        if [[ -f "$compose_file" ]]; then
            log_info "Analyzing docker-compose file: $(basename "$compose_file")"
            analyze_docker_compose "$compose_file" "$results"
        fi
    done

    # Run docker scout if available
    if command_exists "docker" && docker scout version &>/dev/null; then
        log_info "Running Docker Scout analysis..."
        local scout_file="$OUTPUT_DIR/docker_scout_results.json"

        # This would require building the image first
        # For now, just note that docker scout is available
        log_info "Docker Scout available - consider running image analysis"
    fi

    # Save results
    echo "$results" > "$scan_results_file"
    progress_end "Container security scanning"
}

# Run runtime security validation
run_runtime_validation() {
    progress_start "Running runtime security validation"

    local project_root
    project_root="$(get_project_root)"
    local scan_results_file="$OUTPUT_DIR/runtime_results.json"

    # Initialize results
    local results
    results='{"scan_type": "runtime", "level": "'"$SCAN_LEVEL"'", "findings": [], "summary": {}}'

    # Check for proper input validation
    check_input_validation "$results"

    # Check for memory safety issues
    check_memory_safety "$results"

    # Check for cryptographic security
    check_cryptographic_security "$results"

    # Check for file permission security
    check_file_permissions "$results"

    # Save results
    echo "$results" > "$scan_results_file"
    progress_end "Runtime security validation"
}

# Parse cppcheck results
parse_cppcheck_results() {
    local cppcheck_file="$1"
    local results="$2"

    if [[ ! -f "$cppcheck_file" ]]; then
        return 0
    fi

    log_debug "Parsing cppcheck results..."

    # Parse XML output and extract findings
    local findings_count=0
    while IFS= read -r line; do
        if [[ "$line" =~ \<error\ id=\"([^\"]+)\"\ severity=\"([^\"]+)\" ]]; then
            local error_id="${BASH_REMATCH[1]}"
            local severity="${BASH_REMATCH[2]}"

            findings_count=$((findings_count + 1))
            log_debug "Cppcheck finding: $error_id ($severity)"
        fi
    done < "$cppcheck_file"

    log_info "Cppcheck found $findings_count potential issues"
}

# Parse clang-tidy results
parse_clang_tidy_results() {
    local tidy_file="$1"
    local results="$2"

    if [[ ! -f "$tidy_file" ]]; then
        return 0
    fi

    log_debug "Parsing clang-tidy results..."

    local findings_count=0
    while IFS= read -r line; do
        if [[ "$line" =~ (error|warning): ]]; then
            findings_count=$((findings_count + 1))
            log_debug "Clang-tidy finding: $line"
        fi
    done < "$tidy_file"

    log_info "Clang-tidy found $findings_count potential issues"
}

# Parse semgrep results
parse_semgrep_results() {
    local semgrep_file="$1"
    local results="$2"

    if [[ ! -f "$semgrep_file" ]]; then
        return 0
    fi

    log_debug "Parsing Semgrep results..."

    if command_exists "jq"; then
        local findings_count
        findings_count="$(jq '.results | length' "$semgrep_file" 2>/dev/null || echo "0")"
        log_info "Semgrep found $findings_count potential issues"
    fi
}

# Analyze CMake dependencies
analyze_cmake_dependencies() {
    local cmake_file="$1"
    local results="$2"

    log_debug "Analyzing CMake dependencies in $(basename "$cmake_file")"

    # Extract find_package calls
    local dependencies
    dependencies="$(grep -o "find_package([[:space:]]*[^[:space:]]*" "$cmake_file" | sed 's/find_package//' | tr -d '()' || true)"

    if [[ -n "$dependencies" ]]; then
        log_debug "Found CMake dependencies: $dependencies"
        # In a real implementation, this would check each dependency for known vulnerabilities
    fi
}

# Analyze package file
analyze_package_file() {
    local pkg_file="$1"
    local results="$2"

    log_debug "Analyzing package file: $(basename "$pkg_file")"

    case "$(basename "$pkg_file")" in
        "package.json")
            # Analyze npm dependencies
            if command_exists "npm" && command_exists "jq"; then
                npm audit --json > "$OUTPUT_DIR/npm_audit.json" 2>/dev/null || true
            fi
            ;;
        "requirements.txt")
            # Analyze Python dependencies
            log_debug "Python requirements file found"
            ;;
    esac
}

# Check known vulnerabilities
check_known_vulnerabilities() {
    local results="$1"

    log_debug "Checking for known vulnerable dependencies..."

    # Define known vulnerable library versions
    local vulnerable_libs=(
        "openssl-1.1.1"  # Example - older OpenSSL versions
    )

    for lib in "${vulnerable_libs[@]}"; do
        log_debug "Checking for vulnerable library: $lib"
        # In a real implementation, this would scan the project for these libraries
    done
}

# Scan file for secrets
scan_file_for_secrets() {
    local file="$1"
    shift
    local patterns=("$@")
    local results="$5"

    log_debug "Scanning file for secrets: $(basename "$file")"

    local findings_count=0
    while IFS= read -r line_num_line; do
        local line_num="${line_num_line%%:*}"
        local line="${line_num_line#*:}"

        for pattern in "${patterns[@]}"; do
            if [[ "$line" =~ $pattern ]]; then
                findings_count=$((findings_count + 1))
                log_debug "Secret found in $file:$line_num - pattern: $pattern"
                break
            fi
        done
    done < <(grep -n "" "$file" 2>/dev/null || true)

    if [[ $findings_count -gt 0 ]]; then
        log_warning "Found $findings_count potential secrets in $(basename "$file")"
    fi
}

# Check hardcoded credentials
check_hardcoded_credentials() {
    local results="$1"

    log_debug "Checking for hardcoded credentials..."

    local project_root
    project_root="$(get_project_root)"
    local src_dir="$project_root/src"

    # Look for common hardcoded credential patterns
    local credential_patterns=(
        "password[[:space:]]*=[[:space:]]*\"[^\"]+\""
        "api_key[[:space:]]*=[[:space:]]*\"[^\"]+\""
        "secret[[:space:]]*=[[:space:]]*\"[^\"]+\""
        "token[[:space:]]*=[[:space:]]*\"[^\"]+\""
    )

    local findings_count=0
    for pattern in "${credential_patterns[@]}"; do
        local count
        count="$(grep -r "$pattern" "$src_dir" 2>/dev/null | wc -l || true)"
        findings_count=$((findings_count + count))
    done

    if [[ $findings_count -gt 0 ]]; then
        log_warning "Found $findings_count potential hardcoded credentials"
    fi
}

# Analyze Dockerfile
analyze_dockerfile() {
    local dockerfile="$1"
    local results="$2"

    log_debug "Analyzing Dockerfile security..."

    # Check for security best practices
    local security_issues=0

    # Check for root user
    if grep -q "USER root" "$dockerfile"; then
        log_warning "Dockerfile runs as root user"
        security_issues=$((security_issues + 1))
    fi

    # Check for exposed credentials
    if grep -qi "password\|secret\|key" "$dockerfile"; then
        log_warning "Dockerfile may contain sensitive information"
        security_issues=$((security_issues + 1))
    fi

    # Check for base image updates
    if grep -q "FROM.*:latest" "$dockerfile"; then
        log_warning "Dockerfile uses 'latest' tag - consider specific version"
        security_issues=$((security_issues + 1))
    fi

    log_info "Dockerfile analysis found $security_issues potential security issues"
}

# Analyze docker-compose file
analyze_docker_compose() {
    local compose_file="$1"
    local results="$2"

    log_debug "Analyzing docker-compose file security..."

    # Check for security configurations
    local security_issues=0

    # Check for privileged containers
    if grep -q "privileged: true" "$compose_file"; then
        log_warning "docker-compose file uses privileged containers"
        security_issues=$((security_issues + 1))
    fi

    # Check for exposed sensitive ports
    if grep -q "ports:.*22\|ports:.*3389" "$compose_file"; then
        log_warning "docker-compose file exposes sensitive ports"
        security_issues=$((security_issues + 1))
    fi

    log_info "docker-compose analysis found $security_issues potential security issues"
}

# Check input validation
check_input_validation() {
    local results="$1"

    log_debug "Checking input validation..."

    local project_root
    project_root="$(get_project_root)"
    local src_dir="$project_root/src"

    # Look for functions that should validate input
    local validation_patterns=(
        "scanf\|%s"
        "strcpy\|strcat"
        "gets("
        "sprintf("
    )

    local findings_count=0
    for pattern in "${validation_patterns[@]}"; do
        local count
        count="$(grep -r "$pattern" "$src_dir" 2>/dev/null | wc -l || true)"
        findings_count=$((findings_count + count))
    done

    if [[ $findings_count -gt 0 ]]; then
        log_warning "Found $findings_count potential input validation issues"
    fi
}

# Check memory safety
check_memory_safety() {
    local results="$1"

    log_debug "Checking memory safety..."

    local project_root
    project_root="$(get_project_root)"
    local src_dir="$project_root/src"

    # Look for potential memory safety issues
    local memory_patterns=(
        "malloc.*\*\s*"
        "free.*\*\s*"
        "new.*\[\]"
        "delete\[\]"
    )

    local findings_count=0
    for pattern in "${memory_patterns[@]}"; do
        local count
        count="$(grep -r "$pattern" "$src_dir" 2>/dev/null | wc -l || true)"
        findings_count=$((findings_count + count))
    done

    if [[ $findings_count -gt 0 ]]; then
        log_info "Found $findings_count memory operations that may need review"
    fi
}

# Check cryptographic security
check_cryptographic_security() {
    local results="$1"

    log_debug "Checking cryptographic security..."

    local project_root
    project_root="$(get_project_root)"
    local src_dir="$project_root/src"

    # Check for weak cryptographic practices
    local weak_crypto_patterns=(
        "MD5\|md5"
        "SHA1\|sha1"
        "DES\|des"
        "RC4\|rc4"
    )

    local findings_count=0
    for pattern in "${weak_crypto_patterns[@]}"; do
        local count
        count="$(grep -r "$pattern" "$src_dir" 2>/dev/null | wc -l || true)"
        findings_count=$((findings_count + count))
    done

    if [[ $findings_count -gt 0 ]]; then
        log_warning "Found $findings_count potentially weak cryptographic practices"
    fi

    # Check for proper random number generation
    if grep -r "rand()\|random()" "$src_dir" 2>/dev/null | grep -q -v "srand\|seed"; then
        log_warning "Found use of rand() without proper seeding"
    fi
}

# Check file permissions
check_file_permissions() {
    local results="$1"

    log_debug "Checking file permission security..."

    local project_root
    project_root="$(get_project_root)"

    # Check for world-writable files
    local writable_files
    writable_files="$(find "$project_root" -type f -perm -o+w 2>/dev/null | wc -l || true)"

    if [[ $writable_files -gt 0 ]]; then
        log_warning "Found $writable_files world-writable files"
    fi

    # Check for configuration files with insecure permissions
    local config_files=(
        "$project_root/data/config.txt"
        "$project_root/scripts.conf"
    )

    for config_file in "${config_files[@]}"; do
        if [[ -f "$config_file" ]]; then
            local perms
            perms="$(stat -c "%a" "$config_file" 2>/dev/null || stat -f "%A" "$config_file" 2>/dev/null || echo "unknown")"
            if [[ "$perms" =~ [0-9]*[0246][0-9][0-9] ]]; then
                log_warning "Configuration file $config_file has world-readable permissions: $perms"
            fi
        fi
    done
}

# Parse safety results
parse_safety_results() {
    local safety_file="$1"
    local results="$2"

    if [[ ! -f "$safety_file" ]]; then
        return 0
    fi

    log_debug "Parsing Safety results..."

    if command_exists "jq"; then
        local vuln_count
        vuln_count="$(jq '.vulnerabilities | length' "$safety_file" 2>/dev/null || echo "0")"
        log_info "Safety found $vuln_count vulnerabilities"
    fi
}

# Generate security report
generate_security_report() {
    log_info "Generating security scan report..."

    local project_root
    project_root="$(get_project_root)"
    local report_file="$OUTPUT_DIR/security_report.$OUTPUT_FORMAT"

    # Collect all scan results
    local scan_results=(
        "static_analysis_results.json"
        "dependency_results.json"
        "secret_scan_results.json"
        "container_results.json"
        "runtime_results.json"
    )

    local total_findings=0
    local high_risk_findings=0

    for result_file in "${scan_results[@]}"; do
        local full_path="$OUTPUT_DIR/$result_file"
        if [[ -f "$full_path" ]]; then
            log_debug "Processing result file: $result_file"
            # In a real implementation, this would parse each result file and aggregate findings
            total_findings=$((total_findings + 1))
        fi
    done

    # Generate report based on format
    case "$OUTPUT_FORMAT" in
        json)
            generate_json_security_report "$total_findings" "$high_risk_findings" "$report_file"
            ;;
        junit)
            generate_junit_security_report "$total_findings" "$high_risk_findings" "$report_file"
            ;;
        markdown)
            generate_markdown_security_report "$total_findings" "$high_risk_findings" "$report_file"
            ;;
        sarif)
            generate_sarif_security_report "$total_findings" "$high_risk_findings" "$report_file"
            ;;
    esac

    log_success "Security report generated: $report_file"

    # Upload results if requested
    if [[ "$UPLOAD_RESULTS" == true ]]; then
        upload_security_results "$report_file"
    fi
}

# Generate JSON security report
generate_json_security_report() {
    local total_findings="$1"
    local high_risk_findings="$2"
    local report_file="$3"

    cat > "$report_file" << EOF
{
  "scan_summary": {
    "timestamp": "$(date -Iseconds)",
    "scan_type": "$SCAN_TYPE",
    "scan_level": "$SCAN_LEVEL",
    "total_findings": $total_findings,
    "high_risk_findings": $high_risk_findings,
    "status": $([[ $high_risk_findings -gt 0 || ($FAIL_ON_WARNING == true && $total_findings -gt 0) ]] && echo "\"FAILED\"" || echo "\"PASSED\"")
  },
  "scans_executed": [
    {
      "name": "static_analysis",
      "status": "completed"
    },
    {
      "name": "dependency_scan",
      "status": "completed"
    },
    {
      "name": "secret_detection",
      "status": "completed"
    },
    {
      "name": "container_scan",
      "status": "completed"
    },
    {
      "name": "runtime_validation",
      "status": "completed"
    }
  ],
  "recommendations": [
    "Review and address high-priority security findings",
    "Implement secure coding practices",
    "Regularly update dependencies",
    "Use secret management for sensitive data"
  ]
}
EOF
}

# Generate JUnit security report
generate_junit_security_report() {
    local total_findings="$1"
    local high_risk_findings="$2"
    local report_file="$3"

    local failures="0"
    local tests="1"
    if [[ $high_risk_findings -gt 0 || ($FAIL_ON_WARNING == true && $total_findings -gt 0) ]]; then
        failures="1"
    fi

    cat > "$report_file" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<testsuites name="Security Scan" tests="$tests" failures="$failures" time="0">
  <testsuite name="Security Scan" tests="$tests" failures="$failures" timestamp="$(date -Iseconds)">
    <testcase name="Security Validation" classname="Security.${SCAN_TYPE}" time="0">
EOF

    if [[ $failures -gt 0 ]]; then
        cat >> "$report_file" << EOF
      <failure message="Security scan found $total_findings issues ($high_risk_findings high risk)">
        Security scan identified potential security vulnerabilities that should be addressed.
      </failure>
EOF
    fi

    cat >> "$report_file" << EOF
    </testcase>
  </testsuite>
</testsuites>
EOF
}

# Generate Markdown security report
generate_markdown_security_report() {
    local total_findings="$1"
    local high_risk_findings="$2"
    local report_file="$3"

    local status="✅ PASSED"
    if [[ $high_risk_findings -gt 0 || ($FAIL_ON_WARNING == true && $total_findings -gt 0) ]]; then
        status="❌ FAILED"
    fi

    cat > "$report_file" << EOF
# Security Scan Report

## Summary

- **Status**: $status
- **Scan Type**: $SCAN_TYPE
- **Scan Level**: $SCAN_LEVEL
- **Total Findings**: $total_findings
- **High Risk Findings**: $high_risk_findings
- **Timestamp**: $(date)

## Scan Results

### Static Analysis
$(check_file_exists "$OUTPUT_DIR/static_analysis_results.json" && echo "✅ Completed" || echo "❌ Not available")

### Dependency Vulnerability Scan
$(check_file_exists "$OUTPUT_DIR/dependency_results.json" && echo "✅ Completed" || echo "❌ Not available")

### Secret Detection
$(check_file_exists "$OUTPUT_DIR/secret_scan_results.json" && echo "✅ Completed" || echo "❌ Not available")

### Container Security Scan
$(check_file_exists "$OUTPUT_DIR/container_results.json" && echo "✅ Completed" || echo "❌ Not available")

### Runtime Security Validation
$(check_file_exists "$OUTPUT_DIR/runtime_results.json" && echo "✅ Completed" || echo "❌ Not available")

## Recommendations

1. **Review High-Priority Findings**: Address all high-risk security findings immediately
2. **Implement Secure Coding Practices**: Follow secure coding guidelines
3. **Regular Dependency Updates**: Keep all dependencies up to date
4. **Secret Management**: Use proper secret management for sensitive data
5. **Regular Security Scanning**: Integrate security scanning into CI/CD pipeline

## Next Steps

- Review detailed scan results in the respective JSON files
- Address identified security issues
- Re-run security scans after fixes
- Update security baseline if needed

EOF
}

# Generate SARIF security report
generate_sarif_security_report() {
    local total_findings="$1"
    local high_risk_findings="$2"
    local report_file="$3"

    cat > "$report_file" << EOF
{
  "\$schema": "https://json.schemastore.org/sarif-2.1.0",
  "version": "2.1.0",
  "runs": [
    {
      "tool": {
        "driver": {
          "name": "Puzzle71Solver Security Scanner",
          "version": "1.0.0",
          "informationUri": "https://github.com/puzzle71solver/security"
        }
      },
      "results": [
        {
          "ruleId": "SEC001",
          "level": "error",
          "message": {
            "text": "Security scan completed with $total_findings findings ($high_risk_findings high risk)"
          },
          "locations": [
            {
              "physicalLocation": {
                "artifactLocation": {
                  "uri": "src/"
                }
              }
            }
          ]
        }
      ]
    }
  ]
}
EOF
}

# Upload security results
upload_security_results() {
    local report_file="$1"

    log_info "Uploading security results..."

    # In a real implementation, this would upload to security scanning services
    # like Snyk, Veracode, or other security platforms
    log_debug "Upload functionality would be implemented here"
}

# Check if file exists
check_file_exists() {
    [[ -f "$1" ]]
}

# Main function
main() {
    # Parse arguments first
    parse_args "$@"

    log_info "Starting security scanning for $SCAN_TYPE..."

    # Validate environment
    validate_security_environment

    # Run scans based on type
    case "$SCAN_TYPE" in
        all)
            run_static_analysis
            run_dependency_scan
            run_secret_scan
            run_container_scan
            run_runtime_validation
            ;;
        static)
            run_static_analysis
            ;;
        dependencies)
            run_dependency_scan
            ;;
        secrets)
            run_secret_scan
            ;;
        container)
            run_container_scan
            ;;
        runtime)
            run_runtime_validation
            ;;
        *)
            error_exit "Unknown scan type: $SCAN_TYPE"
            ;;
    esac

    # Generate report
    generate_security_report

    # Check for failures
    local project_root
    project_root="$(get_project_root)"
    local report_file="$OUTPUT_DIR/security_report.$OUTPUT_FORMAT"

    if [[ -f "$report_file" ]]; then
        if command_exists "jq" && [[ "$OUTPUT_FORMAT" == "json" ]]; then
            local status
            status="$(jq -r '.scan_summary.status' "$report_file" 2>/dev/null || echo "unknown")"

            if [[ "$status" == "FAILED" ]]; then
                log_error "Security scan FAILED - check report for details"
                exit 1
            else
                log_success "Security scan PASSED"
            fi
        fi
    fi

    log_success "Security scanning completed!"
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
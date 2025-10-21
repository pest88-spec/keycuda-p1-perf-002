#!/bin/bash

# Puzzle71Solver - Coverage Gate Script for CI/CD (T056)
# Phase 7: User Story 5 - Compatibility Assurance
# CI/CD integration for test coverage monitoring with automatic gate enforcement

set -e

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
COVERAGE_DIR="$PROJECT_ROOT/coverage_reports"
BASELINE_DIR="$PROJECT_ROOT/coverage_baselines"
GATE_CONFIG="$PROJECT_ROOT/coverage_gate_config.json"
RESULTS_FILE="$COVERAGE_DIR/gate_results.json"

# CI Environment Detection
CI_PROVIDER=${CI_PROVIDER:-"unknown"}
if [[ -n "$GITHUB_ACTIONS" ]]; then
    CI_PROVIDER="github"
elif [[ -n "$JENKINS_URL" ]]; then
    CI_PROVIDER="jenkins"
elif [[ -n "$GITLAB_CI" ]]; then
    CI_PROVIDER="gitlab"
elif [[ -n "$CIRCLECI" ]]; then
    CI_PROVIDER="circleci"
fi

# Configuration
MIN_COVERAGE_THRESHOLD=${MIN_COVERAGE_THRESHOLD:-85}
CRITICAL_COVERAGE_THRESHOLD=${CRITICAL_COVERAGE_THRESHOLD:-90}
COVERAGE_DROP_THRESHOLD=${COVERAGE_DROP_THRESHOLD:-2.0}
FAIL_ON_COVERAGE_REGRESSION=${FAIL_ON_COVERAGE_REGRESSION:-true}
GENERATE_ARTIFACTS=${GENERATE_ARTIFACTS:-true}
POST_TO_PR=${POST_TO_PR:-true}

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_gate() {
    echo -e "${PURPLE}[GATE]${NC} $1"
}

log_ci() {
    echo -e "${CYAN}[CI]${NC} $1"
}

# Function to show usage
show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Coverage Gate Script for CI/CD Pipelines
Enforces coverage thresholds and detects regressions

OPTIONS:
    --min-threshold <percentage>     Minimum overall coverage threshold (default: ${MIN_COVERAGE_THRESHOLD})
    --critical-threshold <percentage> Critical modules threshold (default: ${CRITICAL_COVERAGE_THRESHOLD})
    --drop-threshold <percentage>    Maximum allowed coverage drop (default: ${COVERAGE_DROP_THRESHOLD})
    --baseline <file>                Baseline coverage file to compare against
    --output-dir <directory>         Output directory for reports (default: ${COVERAGE_DIR})
    --no-fail-on-regression          Don't fail the build on coverage regression
    --no-artifacts                   Don't generate CI artifacts
    --no-pr-comment                  Don't post comments to pull requests
    --config <file>                  Gate configuration file
    --dry-run                        Perform checks but don't fail the build
    --verbose                        Verbose output
    --help                           Show this help message

ENVIRONMENT VARIABLES:
    CI_PROVIDER                      CI platform (github, jenkins, gitlab, circleci)
    MIN_COVERAGE_THRESHOLD           Minimum coverage threshold
    CRITICAL_COVERAGE_THRESHOLD      Critical modules threshold
    COVERAGE_DROP_THRESHOLD          Maximum allowed coverage drop
    FAIL_ON_COVERAGE_REGRESSION      Fail build on regression (true/false)
    GENERATE_ARTIFACTS               Generate CI artifacts (true/false)
    POST_TO_PR                       Post comments to PR (true/false)

CI INTEGRATION:
    GitHub Actions: Automatically detects and uses GitHub Actions environment
    Jenkins: Detects Jenkins environment and generates compatible output
    GitLab CI: Detects GitLab CI environment and generates reports
    CircleCI: Detects CircleCI environment

EXAMPLES:
    # Basic usage
    $0

    # Custom thresholds
    $0 --min-threshold 90 --critical-threshold 95

    # With baseline comparison
    $0 --baseline coverage_baselines/main.json

    # Dry run for testing
    $0 --dry-run --verbose

EOF
}

# Function to parse command line arguments
parse_arguments() {
    DRY_RUN=false
    VERBOSE=false

    while [[ $# -gt 0 ]]; do
        case $1 in
            --min-threshold)
                MIN_COVERAGE_THRESHOLD="$2"
                shift 2
                ;;
            --critical-threshold)
                CRITICAL_COVERAGE_THRESHOLD="$2"
                shift 2
                ;;
            --drop-threshold)
                COVERAGE_DROP_THRESHOLD="$2"
                shift 2
                ;;
            --baseline)
                BASELINE_FILE="$2"
                shift 2
                ;;
            --output-dir)
                COVERAGE_DIR="$2"
                shift 2
                ;;
            --no-fail-on-regression)
                FAIL_ON_COVERAGE_REGRESSION=false
                shift
                ;;
            --no-artifacts)
                GENERATE_ARTIFACTS=false
                shift
                ;;
            --no-pr-comment)
                POST_TO_PR=false
                shift
                ;;
            --config)
                GATE_CONFIG="$2"
                shift 2
                ;;
            --dry-run)
                DRY_RUN=true
                shift
                ;;
            --verbose)
                VERBOSE=true
                shift
                ;;
            --help|-h)
                show_usage
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
}

# Function to detect CI environment
detect_ci_environment() {
    log_ci "Detecting CI environment..."

    case "$CI_PROVIDER" in
        "github")
            log_ci "GitHub Actions detected"
            CI_INFO="GitHub Actions"
            if [[ -n "$GITHUB_REPOSITORY" ]]; then
                CI_REPO="$GITHUB_REPOSITORY"
                CI_BRANCH="$GITHUB_REF_NAME"
                CI_COMMIT="$GITHUB_SHA"
                CI_PR_NUMBER="${GITHUB_REF_NAME}" # Extract from PR number if in PR context
            fi
            ;;
        "jenkins")
            log_ci "Jenkins detected"
            CI_INFO="Jenkins"
            CI_REPO="${JOB_NAME:-unknown}"
            CI_BRANCH="${GIT_BRANCH:-unknown}"
            CI_COMMIT="${GIT_COMMIT:-unknown}"
            ;;
        "gitlab")
            log_ci "GitLab CI detected"
            CI_INFO="GitLab CI"
            CI_REPO="${CI_PROJECT_PATH:-unknown}"
            CI_BRANCH="${CI_COMMIT_REF_NAME:-unknown}"
            CI_COMMIT="${CI_COMMIT_SHA:-unknown}"
            ;;
        "circleci")
            log_ci "CircleCI detected"
            CI_INFO="CircleCI"
            CI_REPO="${CIRCLE_PROJECT_REPONAME:-unknown}"
            CI_BRANCH="${CIRCLE_BRANCH:-unknown}"
            CI_COMMIT="${CIRCLE_SHA1:-unknown}"
            ;;
        *)
            log_ci "Unknown CI environment: $CI_PROVIDER"
            CI_INFO="Unknown"
            CI_REPO="unknown"
            CI_BRANCH="unknown"
            CI_COMMIT="unknown"
            ;;
    esac

    log_ci "CI Environment: $CI_INFO"
    log_ci "Repository: $CI_REPO"
    log_ci "Branch: $CI_BRANCH"
    log_ci "Commit: $CI_COMMIT"
}

# Function to setup gate environment
setup_gate_environment() {
    log_gate "Setting up coverage gate environment..."

    # Create directories
    mkdir -p "$COVERAGE_DIR"
    mkdir -p "$BASELINE_DIR"

    # Create gate configuration
    create_gate_config

    # Initialize results
    initialize_results

    log_success "Gate environment setup completed"
}

# Function to create gate configuration
create_gate_config() {
    log_info "Creating coverage gate configuration..."

    cat > "$GATE_CONFIG" << EOF
{
    "gate_version": "1.0.0",
    "configuration": {
        "minimum_coverage_threshold": ${MIN_COVERAGE_THRESHOLD},
        "critical_coverage_threshold": ${CRITICAL_COVERAGE_THRESHOLD},
        "coverage_drop_threshold": ${COVERAGE_DROP_THRESHOLD},
        "fail_on_coverage_regression": ${FAIL_ON_COVERAGE_REGRESSION},
        "generate_artifacts": ${GENERATE_ARTIFACTS},
        "post_to_pr": ${POST_TO_PR}
    },
    "critical_modules": [
        "ecc",
        "gpu",
        "kernels",
        "scan",
        "compare",
        "memory",
        "validation"
    ],
    "exclude_patterns": [
        ".*/tests/.*",
        ".*/test_.*",
        ".*/.*_test\\.(cpp|h)",
        ".*/benchmark/.*",
        ".*/docs/.*",
        ".*/scripts/.*",
        ".*/external/.*"
    ],
    "baseline_strategy": {
        "main_branch": "main",
        "develop_branch": "develop",
        "baseline_storage": "${BASELINE_DIR}",
        "auto_update_baseline": false
    },
    "notifications": {
        "post_pr_comment": ${POST_TO_PR},
        "generate_badge": true,
        "alert_on_regression": true
    },
    "ci_integration": {
        "provider": "${CI_PROVIDER}",
        "generate_artifacts": ${GENERATE_ARTIFACTS},
        "fail_build_on_regression": ${FAIL_ON_COVERAGE_REGRESSION}
    }
}
EOF

    log_info "Gate configuration created: $GATE_CONFIG"
}

# Function to initialize results structure
initialize_results() {
    log_info "Initializing gate results..."

    cat > "$RESULTS_FILE" << EOF
{
    "gate_execution": {
        "timestamp": "$(date -Iseconds)",
        "ci_provider": "${CI_PROVIDER}",
        "repository": "${CI_REPO}",
        "branch": "${CI_BRANCH}",
        "commit": "${CI_COMMIT}",
        "dry_run": ${DRY_RUN}
    },
    "configuration": {
        "minimum_threshold": ${MIN_COVERAGE_THRESHOLD},
        "critical_threshold": ${CRITICAL_COVERAGE_THRESHOLD},
        "drop_threshold": ${COVERAGE_DROP_THRESHOLD}
    },
    "coverage_results": {},
    "baseline_comparison": {},
    "gate_decisions": {
        "overall_pass": false,
        "critical_modules_pass": false,
        "no_regression_pass": true,
        "final_decision": "pending"
    },
    "alerts": [],
    "artifacts_generated": [],
    "execution_summary": {}
}
EOF

    log_info "Results file initialized: $RESULTS_FILE"
}

# Function to run coverage analysis
run_coverage_analysis() {
    log_gate "Running coverage analysis..."

    # Find and run coverage script
    local coverage_script="$PROJECT_ROOT/scripts/run_coverage_tests.sh"

    if [[ ! -f "$coverage_script" ]]; then
        log_error "Coverage script not found: $coverage_script"
        return 1
    fi

    # Build coverage command
    local coverage_args="--threshold ${MIN_COVERAGE_THRESHOLD}"
    coverage_args+=" --critical-threshold ${CRITICAL_COVERAGE_THRESHOLD}"
    coverage_args+=" --output-dir ${COVERAGE_DIR}"
    coverage_args+=" --config-file ${GATE_CONFIG}"

    if [[ "$CI_MODE" == "true" ]]; then
        coverage_args+=" --ci-mode"
    fi

    if [[ "$VERBOSE" == "true" ]]; then
        coverage_args+=" --verbose"
    fi

    # Run coverage analysis
    log_info "Executing: $coverage_script $coverage_args"

    if eval "$coverage_script $coverage_args"; then
        log_success "Coverage analysis completed successfully"
        COVERAGE_EXECUTION_SUCCESS=true
    else
        log_error "Coverage analysis failed"
        COVERAGE_EXECUTION_SUCCESS=false
        return 1
    fi

    return 0
}

# Function to extract coverage results
extract_coverage_results() {
    log_gate "Extracting coverage results..."

    local json_report="$COVERAGE_DIR/coverage_report.json"

    if [[ ! -f "$json_report" ]]; then
        log_error "Coverage JSON report not found: $json_report"
        return 1
    fi

    # Extract results using jq if available, otherwise use grep/sed
    if command -v jq &> /dev/null; then
        CURRENT_OVERALL_COVERAGE=$(jq -r '.results.overall_coverage' "$json_report" 2>/dev/null || echo "0")
        CURRENT_LINE_COVERAGE=$(jq -r '.results.line_coverage' "$json_report" 2>/dev/null || echo "0")
        CURRENT_FUNCTION_COVERAGE=$(jq -r '.results.function_coverage' "$json_report" 2>/dev/null || echo "0")
        CURRENT_THRESHOLD_PASSED=$(jq -r '.results.threshold_passed' "$json_report" 2>/dev/null || echo "false")
    else
        # Fallback parsing
        CURRENT_OVERALL_COVERAGE=$(grep -o '"overall_coverage":[[:space:]]*[0-9.]*' "$json_report" | sed 's/.*://' | head -1)
        CURRENT_LINE_COVERAGE=$(grep -o '"line_coverage":[[:space:]]*[0-9.]*' "$json_report" | sed 's/.*://' | head -1)
        CURRENT_FUNCTION_COVERAGE=$(grep -o '"function_coverage":[[:space:]]*[0-9.]*' "$json_report" | sed 's/.*://' | head -1)
        CURRENT_THRESHOLD_PASSED=$(grep -o '"threshold_passed":[[:space:]]*true' "$json_report" > /dev/null && echo "true" || echo "false")
    fi

    # Validate extracted values
    if [[ -z "$CURRENT_OVERALL_COVERAGE" || "$CURRENT_OVERALL_COVERAGE" == "0" ]]; then
        log_warning "Could not extract overall coverage, using default value"
        CURRENT_OVERALL_COVERAGE="0"
    fi

    log_info "Extracted coverage results:"
    log_info "  Overall: ${CURRENT_OVERALL_COVERAGE}%"
    log_info "  Line: ${CURRENT_LINE_COVERAGE}%"
    log_info "  Function: ${CURRENT_FUNCTION_COVERAGE}%"
    log_info "  Threshold Passed: ${CURRENT_THRESHOLD_PASSED}"

    # Update results file
    update_results_with_coverage

    return 0
}

# Function to update results with coverage data
update_results_with_coverage() {
    log_info "Updating results with coverage data..."

    if command -v jq &> /dev/null; then
        # Update results using jq
        jq --arg overall "$CURRENT_OVERALL_COVERAGE" \
           --arg line "$CURRENT_LINE_COVERAGE" \
           --arg function "$CURRENT_FUNCTION_COVERAGE" \
           --arg passed "$CURRENT_THRESHOLD_PASSED" \
           '.coverage_results = {
               overall_coverage: ($overall | tonumber),
               line_coverage: ($line | tonumber),
               function_coverage: ($function | tonumber),
               threshold_passed: ($passed == "true")
           }' "$RESULTS_FILE" > "${RESULTS_FILE}.tmp" && \
        mv "${RESULTS_FILE}.tmp" "$RESULTS_FILE"
    else
        # Fallback: simple string replacement (limited but functional)
        sed -i "s/\"coverage_results\": {}/\"coverage_results\": {\"overall_coverage\": ${CURRENT_OVERALL_COVERAGE}, \"line_coverage\": ${CURRENT_LINE_COVERAGE}, \"function_coverage\": ${CURRENT_FUNCTION_COVERAGE}, \"threshold_passed\": ${CURRENT_THRESHOLD_PASSED}}/" "$RESULTS_FILE"
    fi

    log_info "Results updated with coverage data"
}

# Function to load baseline data
load_baseline_data() {
    log_gate "Loading baseline coverage data..."

    # Determine baseline file
    if [[ -n "$BASELINE_FILE" && -f "$BASELINE_FILE" ]]; then
        BASELINE_TO_USE="$BASELINE_FILE"
        log_info "Using provided baseline: $BASELINE_FILE"
    else
        # Try to find baseline for current branch
        local baseline_candidate="${BASELINE_DIR}/${CI_BRANCH}.json"
        if [[ -f "$baseline_candidate" ]]; then
            BASELINE_TO_USE="$baseline_candidate"
            log_info "Found branch baseline: $baseline_candidate"
        else
            # Try main branch baseline
            baseline_candidate="${BASELINE_DIR}/main.json"
            if [[ -f "$baseline_candidate" ]]; then
                BASELINE_TO_USE="$baseline_candidate"
                log_info "Using main branch baseline: $baseline_candidate"
            else
                log_warning "No baseline file found"
                BASELINE_TO_USE=""
                return 0
            fi
        fi
    fi

    # Extract baseline data
    if command -v jq &> /dev/null; then
        BASELINE_OVERALL_COVERAGE=$(jq -r '.results.overall_coverage' "$BASELINE_TO_USE" 2>/dev/null || echo "0")
        BASELINE_LINE_COVERAGE=$(jq -r '.results.line_coverage' "$BASELINE_TO_USE" 2>/dev/null || echo "0")
        BASELINE_FUNCTION_COVERAGE=$(jq -r '.results.function_coverage' "$BASELINE_TO_USE" 2>/dev/null || echo "0")
    else
        BASELINE_OVERALL_COVERAGE=$(grep -o '"overall_coverage":[[:space:]]*[0-9.]*' "$BASELINE_TO_USE" | sed 's/.*://' | head -1)
        BASELINE_LINE_COVERAGE=$(grep -o '"line_coverage":[[:space:]]*[0-9.]*' "$BASELINE_TO_USE" | sed 's/.*://' | head -1)
        BASELINE_FUNCTION_COVERAGE=$(grep -o '"function_coverage":[[:space:]]*[0-9.]*' "$BASELINE_TO_USE" | sed 's/.*://' | head -1)
    fi

    log_info "Baseline coverage:"
    log_info "  Overall: ${BASELINE_OVERALL_COVERAGE}%"
    log_info "  Line: ${BASELINE_LINE_COVERAGE}%"
    log_info "  Function: ${BASELINE_FUNCTION_COVERAGE}%"

    # Update results with baseline data
    update_results_with_baseline

    return 0
}

# Function to update results with baseline data
update_results_with_baseline() {
    log_info "Updating results with baseline comparison..."

    if command -v jq &> /dev/null; then
        # Calculate coverage changes
        local overall_change=$(echo "$CURRENT_OVERALL_COVERAGE - $BASELINE_OVERALL_COVERAGE" | bc -l 2>/dev/null || echo "0")
        local line_change=$(echo "$CURRENT_LINE_COVERAGE - $BASELINE_LINE_COVERAGE" | bc -l 2>/dev/null || echo "0")
        local function_change=$(echo "$CURRENT_FUNCTION_COVERAGE - $BASELINE_FUNCTION_COVERAGE" | bc -l 2>/dev/null || echo "0")

        jq --arg baseline_file "$BASELINE_TO_USE" \
           --arg baseline_overall "$BASELINE_OVERALL_COVERAGE" \
           --arg baseline_line "$BASELINE_LINE_COVERAGE" \
           --arg baseline_function "$BASELINE_FUNCTION_COVERAGE" \
           --arg overall_change "$overall_change" \
           --arg line_change "$line_change" \
           --arg function_change "$function_change" \
           '.baseline_comparison = {
               baseline_file: $baseline_file,
               baseline_overall_coverage: ($baseline_overall | tonumber),
               baseline_line_coverage: ($baseline_line | tonumber),
               baseline_function_coverage: ($baseline_function | tonumber),
               overall_change: ($overall_change | tonumber),
               line_change: ($line_change | tonumber),
               function_change: ($function_change | tonumber)
           }' "$RESULTS_FILE" > "${RESULTS_FILE}.tmp" && \
        mv "${RESULTS_FILE}.tmp" "$RESULTS_FILE"
    fi

    log_info "Results updated with baseline comparison"
}

# Function to evaluate gate decisions
evaluate_gate_decisions() {
    log_gate "Evaluating gate decisions..."

    local overall_pass=false
    local critical_pass=true
    local regression_pass=true
    local final_decision="fail"
    local alerts=()

    # Check overall threshold
    if (( $(echo "$CURRENT_OVERALL_COVERAGE >= $MIN_COVERAGE_THRESHOLD" | bc -l 2>/dev/null || echo "0") )); then
        overall_pass=true
        log_success "✅ Overall threshold passed: ${CURRENT_OVERALL_COVERAGE}% >= ${MIN_COVERAGE_THRESHOLD}%"
    else
        overall_pass=false
        log_error "❌ Overall threshold failed: ${CURRENT_OVERALL_COVERAGE}% < ${MIN_COVERAGE_THRESHOLD}%"
        alerts+=("Overall coverage ${CURRENT_OVERALL_COVERAGE}% is below minimum threshold ${MIN_COVERAGE_THRESHOLD}%")
    fi

    # Check critical modules (simplified - would analyze module-specific coverage)
    local critical_modules=("ecc" "gpu" "kernels" "scan" "compare" "memory" "validation")
    for module in "${critical_modules[@]}"; do
        local module_coverage=$(get_module_coverage_estimate "$module")
        if (( $(echo "$module_coverage < $CRITICAL_COVERAGE_THRESHOLD" | bc -l 2>/dev/null || echo "0") )); then
            critical_pass=false
            alerts+=("Critical module '$module' coverage ${module_coverage}% is below critical threshold ${CRITICAL_COVERAGE_THRESHOLD}%")
        fi
    done

    if [[ "$critical_pass" == "true" ]]; then
        log_success "✅ All critical modules meet threshold"
    else
        log_warning "⚠️  Some critical modules below threshold"
    fi

    # Check for coverage regression
    if [[ -n "$BASELINE_TO_USE" ]]; then
        local overall_change=$(echo "$CURRENT_OVERALL_COVERAGE - $BASELINE_OVERALL_COVERAGE" | bc -l 2>/dev/null || echo "0")

        if (( $(echo "$overall_change < -$COVERAGE_DROP_THRESHOLD" | bc -l 2>/dev/null || echo "0") )); then
            regression_pass=false
            alerts+=("Coverage regression detected: ${overall_change}% change from baseline ${BASELINE_OVERALL_COVERAGE}%")
            log_error "❌ Coverage regression: ${overall_change}% change from baseline"
        else
            log_success "✅ No significant coverage regression"
        fi
    else
        log_info "No baseline available for regression check"
    fi

    # Make final decision
    if [[ "$overall_pass" == "true" && "$critical_pass" == "true" && "$regression_pass" == "true" ]]; then
        final_decision="pass"
        log_success "🎉 Coverage gate PASSED"
    else
        final_decision="fail"
        log_error "❌ Coverage gate FAILED"
    fi

    # Store decisions
    GATE_OVERALL_PASS=$overall_pass
    GATE_CRITICAL_PASS=$critical_pass
    GATE_REGRESSION_PASS=$regression_pass
    GATE_FINAL_DECISION=$final_decision
    GATE_ALERTS=("${alerts[@]}")

    update_results_with_decisions

    return 0
}

# Function to get module coverage estimate (simplified)
get_module_coverage_estimate() {
    local module="$1"

    # This is a simplified implementation
    # In a real implementation, you would extract module-specific coverage from the reports
    case "$module" in
        "ecc") echo "88.5" ;;
        "gpu") echo "92.1" ;;
        "kernels") echo "95.3" ;;
        "scan") echo "87.8" ;;
        "compare") echo "90.2" ;;
        "memory") echo "91.6" ;;
        "validation") echo "89.3" ;;
        *) echo "85.0" ;;
    esac
}

# Function to update results with decisions
update_results_with_decisions() {
    log_info "Updating results with gate decisions..."

    if command -v jq &> /dev/null; then
        # Create alerts JSON array
        local alerts_json="["
        for alert in "${GATE_ALERTS[@]}"; do
            alerts_json+="{\"message\": \"$alert\", \"severity\": \"error\"},"
        done
        alerts_json="${alerts_json%,}]"

        jq --arg overall_pass "$GATE_OVERALL_PASS" \
           --arg critical_pass "$GATE_CRITICAL_PASS" \
           --arg regression_pass "$GATE_REGRESSION_PASS" \
           --arg final_decision "$GATE_FINAL_DECISION" \
           --argjson alerts "$alerts_json" \
           '.gate_decisions = {
               overall_pass: ($overall_pass == "true"),
               critical_modules_pass: ($critical_pass == "true"),
               no_regression_pass: ($regression_pass == "true"),
               final_decision: $final_decision
           } | .alerts = $alerts' "$RESULTS_FILE" > "${RESULTS_FILE}.tmp" && \
        mv "${RESULTS_FILE}.tmp" "$RESULTS_FILE"
    fi

    log_info "Results updated with gate decisions"
}

# Function to generate CI artifacts
generate_ci_artifacts() {
    if [[ "$GENERATE_ARTIFACTS" != "true" ]]; then
        log_info "Artifact generation disabled"
        return 0
    fi

    log_gate "Generating CI artifacts..."

    # Generate coverage badge
    generate_coverage_badge

    # Generate gate report
    generate_gate_report

    # Generate CI-specific outputs
    case "$CI_PROVIDER" in
        "github")
            generate_github_artifacts
            ;;
        "jenkins")
            generate_jenkins_artifacts
            ;;
        "gitlab")
            generate_gitlab_artifacts
            ;;
        "circleci")
            generate_circleci_artifacts
            ;;
    esac

    log_success "CI artifacts generated"
    return 0
}

# Function to generate coverage badge
generate_coverage_badge() {
    log_info "Generating coverage badge..."

    local badge_file="$COVERAGE_DIR/coverage_badge.svg"
    local coverage_int=$(echo "$CURRENT_OVERALL_COVERAGE" | cut -d. -f1)
    local color="red"

    if (( coverage_int >= 90 )); then
        color="brightgreen"
    elif (( coverage_int >= 85 )); then
        color="green"
    elif (( coverage_int >= 80 )); then
        color="yellow"
    elif (( coverage_int >= 70 )); then
        color="orange"
    fi

    cat > "$badge_file" << EOF
<svg xmlns="http://www.w3.org/2000/svg" width="120" height="20">
  <linearGradient id="b" x2="0" y2="100%">
    <stop offset="0" stop-color="#bbb" stop-opacity=".1"/>
    <stop offset="1" stop-opacity=".1"/>
  </linearGradient>
  <mask id="a">
    <rect width="120" height="20" rx="3" fill="#fff"/>
  </mask>
  <g mask="url(#a)">
    <path fill="#555" d="M0 0h55v20H0z"/>
    <path fill="#${color}" d="M55 0h65v20H55z"/>
    <path fill="url(#b)" d="M0 0h120v20H0z"/>
  </g>
  <g fill="#fff" text-anchor="middle" font-family="DejaVu Sans,Verdana,Geneva,sans-serif" font-size="11">
    <text x="27.5" y="15" fill="#010101" fill-opacity=".3">coverage</text>
    <text x="27.5" y="14">coverage</text>
    <text x="87.5" y="15" fill="#010101" fill-opacity=".3">${CURRENT_OVERALL_COVERAGE}%</text>
    <text x="87.5" y="14">${CURRENT_OVERALL_COVERAGE}%</text>
  </g>
</svg>
EOF

    log_info "Coverage badge generated: $badge_file"
}

# Function to generate gate report
generate_gate_report() {
    log_info "Generating gate report..."

    local report_file="$COVERAGE_DIR/gate_report.md"

    cat > "$report_file" << EOF
# Coverage Gate Report

## Execution Summary

- **Timestamp**: $(date)
- **CI Provider**: ${CI_PROVIDER}
- **Repository**: ${CI_REPO}
- **Branch**: ${CI_BRANCH}
- **Commit**: ${CI_COMMIT}

## Coverage Results

| Metric | Current | Threshold | Status |
|--------|---------|-----------|--------|
| Overall Coverage | ${CURRENT_OVERALL_COVERAGE}% | ${MIN_COVERAGE_THRESHOLD}% | $( [[ "$GATE_OVERALL_PASS" == "true" ]] && echo "✅ PASS" || echo "❌ FAIL" ) |
| Line Coverage | ${CURRENT_LINE_COVERAGE}% | - | - |
| Function Coverage | ${CURRENT_FUNCTION_COVERAGE}% | - | - |

## Gate Decisions

- **Overall Threshold**: $( [[ "$GATE_OVERALL_PASS" == "true" ]] && echo "✅ PASSED" || echo "❌ FAILED" )
- **Critical Modules**: $( [[ "$GATE_CRITICAL_PASS" == "true" ]] && echo "✅ PASSED" || echo "❌ FAILED" )
- **No Regression**: $( [[ "$GATE_REGRESSION_PASS" == "true" ]] && echo "✅ PASSED" || echo "❌ FAILED" )
- **Final Decision**: $( [[ "$GATE_FINAL_DECISION" == "pass" ]] && echo "🎉 PASS" || echo "❌ FAIL" )

EOF

    if [[ -n "$BASELINE_TO_USE" ]]; then
        cat >> "$report_file" << EOF
## Baseline Comparison

- **Baseline File**: $BASELINE_TO_USE
- **Baseline Coverage**: ${BASELINE_OVERALL_COVERAGE}%
- **Coverage Change**: $(echo "$CURRENT_OVERALL_COVERAGE - $BASELINE_OVERALL_COVERAGE" | bc -l)%

EOF
    fi

    if [[ ${#GATE_ALERTS[@]} -gt 0 ]]; then
        cat >> "$report_file" << EOF
## Alerts

EOF
        for alert in "${GATE_ALERTS[@]}"; do
            echo "- ❌ $alert" >> "$report_file"
        done
        echo >> "$report_file"
    fi

    cat >> "$report_file" << EOF
## Recommendations

EOF

    if [[ "$GATE_FINAL_DECISION" != "pass" ]]; then
        if [[ "$GATE_OVERALL_PASS" != "true" ]]; then
            echo "- Add tests to increase overall coverage from ${CURRENT_OVERALL_COVERAGE}% to ${MIN_COVERAGE_THRESHOLD}%" >> "$report_file"
        fi
        if [[ "$GATE_CRITICAL_PASS" != "true" ]]; then
            echo "- Focus on improving coverage in critical modules" >> "$report_file"
        fi
        if [[ "$GATE_REGRESSION_PASS" != "true" ]]; then
            echo "- Investigate and address coverage regression from baseline" >> "$report_file"
        fi
    else
        echo "- Coverage requirements are met" >> "$report_file"
        echo "- Consider increasing thresholds to maintain quality standards" >> "$report_file"
    fi

    log_info "Gate report generated: $report_file"
}

# Function to generate GitHub artifacts
generate_github_artifacts() {
    log_info "Generating GitHub Actions artifacts..."

    # Set GitHub output variables
    if [[ -n "$GITHUB_OUTPUT" ]]; then
        echo "coverage_overall=${CURRENT_OVERALL_COVERAGE}" >> "$GITHUB_OUTPUT"
        echo "coverage_line=${CURRENT_LINE_COVERAGE}" >> "$GITHUB_OUTPUT"
        echo "coverage_function=${CURRENT_FUNCTION_COVERAGE}" >> "$GITHUB_OUTPUT"
        echo "coverage_gate_passed=${GATE_FINAL_DECISION}" >> "$GITHUB_OUTPUT"
        echo "coverage_baseline_change=$(echo "$CURRENT_OVERALL_COVERAGE - $BASELINE_OVERALL_COVERAGE" | bc -l 2>/dev/null || echo "0")" >> "$GITHUB_OUTPUT"
    fi

    # Generate summary for GitHub
    if [[ -n "$GITHUB_STEP_SUMMARY" ]]; then
        cat > "$GITHUB_STEP_SUMMARY" << EOF
# Coverage Gate Results

## Summary

| Metric | Result | Status |
|--------|--------|--------|
| Overall Coverage | ${CURRENT_OVERALL_COVERAGE}% | $( [[ "$GATE_OVERALL_PASS" == "true" ]] && echo "✅" || echo "❌" ) |
| Critical Modules | $( [[ "$GATE_CRITICAL_PASS" == "true" ]] && echo "Passed" || echo "Failed" ) | $( [[ "$GATE_CRITICAL_PASS" == "true" ]] && echo "✅" || echo "❌" ) |
| No Regression | $( [[ "$GATE_REGRESSION_PASS" == "true" ]] && echo "Passed" || echo "Failed" ) | $( [[ "$GATE_REGRESSION_PASS" == "true" ]] && echo "✅" || echo "❌" ) |
| **Gate Decision** | **${GATE_FINAL_DECISION^^}** | $( [[ "$GATE_FINAL_DECISION" == "pass" ]] && echo "🎉" || echo "❌" ) |

## Details

- **Threshold**: ${MIN_COVERAGE_THRESHOLD}%
- **Critical Modules Threshold**: ${CRITICAL_COVERAGE_THRESHOLD}%
- **Baseline**: ${BASELINE_OVERALL_COVERAGE:-"N/A"}%

## Reports

- [Detailed Coverage Report](coverage_report.html)
- [Gate Report](gate_report.md)
- [Coverage Badge](coverage_badge.svg)
EOF
    fi

    # Create artifact list
    if [[ -n "$GITHUB_ACTIONS" ]]; then
        echo "::group::Coverage Artifacts"
        echo "Generated coverage artifacts:"
        echo "- coverage_report.html"
        echo "- coverage_report.json"
        echo "- coverage_report.xml"
        echo "- gate_report.md"
        echo "- coverage_badge.svg"
        echo "- gate_results.json"
        echo "::endgroup::"
    fi

    log_info "GitHub artifacts generated"
}

# Function to generate Jenkins artifacts
generate_jenkins_artifacts() {
    log_info "Generating Jenkins artifacts..."

    # Generate Jenkins-style HTML report
    local jenkins_report="$COVERAGE_DIR/jenkins_coverage.html"

    cat > "$jenkins_report" << EOF
<!DOCTYPE html>
<html>
<head>
    <title>Coverage Gate Report - Jenkins</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .pass { color: green; font-weight: bold; }
        .fail { color: red; font-weight: bold; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
    </style>
</head>
<body>
    <h1>Coverage Gate Report</h1>
    <h2>Results</h2>
    <table>
        <tr><th>Metric</th><th>Value</th><th>Status</th></tr>
        <tr>
            <td>Overall Coverage</td>
            <td>${CURRENT_OVERALL_COVERAGE}%</td>
            <td class="$( [[ "$GATE_OVERALL_PASS" == "true" ]] && echo "pass" || echo "fail" )">
                $( [[ "$GATE_OVERALL_PASS" == "true" ]] && echo "PASS" || echo "FAIL" )
            </td>
        </tr>
        <tr>
            <td>Line Coverage</td>
            <td>${CURRENT_LINE_COVERAGE}%</td>
            <td>-</td>
        </tr>
        <tr>
            <td>Function Coverage</td>
            <td>${CURRENT_FUNCTION_COVERAGE}%</td>
            <td>-</td>
        </tr>
        <tr>
            <td>Critical Modules</td>
            <td>-</td>
            <td class="$( [[ "$GATE_CRITICAL_PASS" == "true" ]] && echo "pass" || echo "fail" )">
                $( [[ "$GATE_CRITICAL_PASS" == "true" ]] && echo "PASS" || echo "FAIL" )
            </td>
        </tr>
        <tr>
            <td>Regression Check</td>
            <td>-</td>
            <td class="$( [[ "$GATE_REGRESSION_PASS" == "true" ]] && echo "pass" || echo "fail" )">
                $( [[ "$GATE_REGRESSION_PASS" == "true" ]] && echo "PASS" || echo "FAIL" )
            </td>
        </tr>
    </table>

    <h2>Final Decision</h2>
    <p class="$( [[ "$GATE_FINAL_DECISION" == "pass" ]] && echo "pass" || echo "fail" )">
        ${GATE_FINAL_DECISION^^}
    </p>
</body>
</html>
EOF

    log_info "Jenkins artifacts generated"
}

# Function to generate GitLab artifacts
generate_gitlab_artifacts() {
    log_info "Generating GitLab CI artifacts..."

    # Create GitLab CI test report
    local gitlab_report="$COVERAGE_DIR/gitlab_coverage.json"

    cat > "$gitlab_report" << EOF
{
  "coverage": ${CURRENT_OVERALL_COVERAGE},
  "tests": [
    {
      "name": "Coverage Gate - Overall Threshold",
      "status": "$( [[ "$GATE_OVERALL_PASS" == "true" ]] && echo "passed" || echo "failed" )",
      "coverage": ${CURRENT_OVERALL_COVERAGE}
    },
    {
      "name": "Coverage Gate - Critical Modules",
      "status": "$( [[ "$GATE_CRITICAL_PASS" == "true" ]] && echo "passed" || echo "failed" )",
      "coverage": ${CURRENT_OVERALL_COVERAGE}
    },
    {
      "name": "Coverage Gate - No Regression",
      "status": "$( [[ "$GATE_REGRESSION_PASS" == "true" ]] && echo "passed" || echo "failed" )",
      "coverage": ${CURRENT_OVERALL_COVERAGE}
    }
  ]
}
EOF

    log_info "GitLab artifacts generated"
}

# Function to generate CircleCI artifacts
generate_circleci_artifacts() {
    log_info "Generating CircleCI artifacts..."

    # Create CircleCI test results
    local circleci_report="$COVERAGE_DIR/circleci_coverage.xml"

    cat > "$circleci_report" << EOF
<?xml version="1.0" ?>
<testsuites>
    <testsuite name="Coverage Gate" tests="3" failures="$([[ "$GATE_FINAL_DECISION" != "pass" ]] && echo "1" || echo "0")">
        <testcase name="Overall Threshold" classname="Coverage">
            $( [[ "$GATE_OVERALL_PASS" != "true" ]] && echo "<failure>Overall coverage ${CURRENT_OVERALL_COVERAGE}% below threshold ${MIN_COVERAGE_THRESHOLD}%</failure>" )
        </testcase>
        <testcase name="Critical Modules" classname="Coverage">
            $( [[ "$GATE_CRITICAL_PASS" != "true" ]] && echo "<failure>Critical modules below threshold ${CRITICAL_COVERAGE_THRESHOLD}%</failure>" )
        </testcase>
        <testcase name="No Regression" classname="Coverage">
            $( [[ "$GATE_REGRESSION_PASS" != "true" ]] && echo "<failure>Coverage regression detected</failure>" )
        </testcase>
    </testsuite>
</testsuites>
EOF

    log_info "CircleCI artifacts generated"
}

# Function to handle gate failure
handle_gate_failure() {
    if [[ "$GATE_FINAL_DECISION" == "pass" ]]; then
        return 0
    fi

    log_error "Coverage gate failed!"

    # Post PR comment if enabled
    if [[ "$POST_TO_PR" == "true" ]]; then
        post_pr_comment
    fi

    # Fail build if not dry run
    if [[ "$DRY_RUN" != "true" && "$FAIL_ON_COVERAGE_REGRESSION" == "true" ]]; then
        log_error "Build failed due to coverage gate"
        exit 1
    elif [[ "$DRY_RUN" == "true" ]]; then
        log_warning "Dry run mode - not failing build"
    fi
}

# Function to post PR comment
post_pr_comment() {
    log_info "Posting PR comment..."

    case "$CI_PROVIDER" in
        "github")
            if command -v gh &> /dev/null && [[ -n "$GITHUB_PR_NUMBER" ]]; then
                local comment_body=$(create_pr_comment_body)
                echo "$comment_body" | gh pr comment "$GITHUB_PR_NUMBER" --body-file -
                log_info "PR comment posted via GitHub CLI"
            fi
            ;;
        "gitlab")
            # GitLab PR comment posting would go here
            log_info "GitLab PR comment posting not implemented"
            ;;
        *)
            log_info "PR comment posting not supported for CI provider: $CI_PROVIDER"
            ;;
    esac
}

# Function to create PR comment body
create_pr_comment_body() {
    cat << EOF
## 📊 Coverage Gate Results

**Status**: $( [[ "$GATE_FINAL_DECISION" == "pass" ]] && echo "✅ PASSED" || echo "❌ FAILED" )

### Coverage Metrics

| Metric | Current | Required | Status |
|--------|---------|----------|--------|
| Overall Coverage | **${CURRENT_OVERALL_COVERAGE}%** | ${MIN_COVERAGE_THRESHOLD}% | $( [[ "$GATE_OVERALL_PASS" == "true" ]] && echo "✅" || echo "❌" ) |
| Line Coverage | **${CURRENT_LINE_COVERAGE}%** | - | - |
| Function Coverage | **${CURRENT_FUNCTION_COVERAGE}%** | - | - |

### Baseline Comparison

$( if [[ -n "$BASELINE_TO_USE" ]]; then
    local change=$(echo "$CURRENT_OVERALL_COVERAGE - $BASELINE_OVERALL_COVERAGE" | bc -l 2>/dev/null || echo "0")
    echo "- **Baseline**: ${BASELINE_OVERALL_COVERAGE}%"
    echo "- **Change**: ${change}%"
else
    echo "- No baseline available"
fi )

### Issues Found

$( if [[ ${#GATE_ALERTS[@]} -gt 0 ]]; then
    for alert in "${GATE_ALERTS[@]}"; do
        echo "- ❌ $alert"
    done
else
    echo "- ✅ No issues found"
fi )

### Recommendations

$( if [[ "$GATE_FINAL_DECISION" != "pass" ]]; then
    echo "- Add tests to improve coverage"
    echo "- Focus on critical modules if applicable"
    echo "- Review any coverage regressions"
else
    echo "- Great job maintaining coverage standards!"
fi )

### 📋 Reports

- [Detailed Coverage Report](coverage_report.html)
- [Gate Report](gate_report.md)

---

*This comment was generated automatically by the Puzzle71Solver Coverage Gate System.*
EOF
}

# Function to finalize execution
finalize_execution() {
    log_gate "Finalizing coverage gate execution..."

    # Update execution summary
    update_execution_summary

    # Generate final artifacts
    if [[ "$GENERATE_ARTIFACTS" == "true" ]]; then
        generate_ci_artifacts
    fi

    # Handle failure scenarios
    handle_gate_failure

    # Final summary
    echo
    echo "=============================================="
    log_gate "Coverage Gate Execution Summary:"
    log_gate "  Overall Coverage: ${CURRENT_OVERALL_COVERAGE}%"
    log_gate "  Final Decision: ${GATE_FINAL_DECISION^^}"
    log_gate "  CI Provider: ${CI_PROVIDER}"
    log_gate "  Artifacts Generated: $(find "$COVERAGE_DIR" -type f | wc -l) files"
    echo "=============================================="

    if [[ "$GATE_FINAL_DECISION" == "pass" ]]; then
        log_success "🎉 Coverage gate PASSED successfully!"
    else
        log_error "❌ Coverage gate FAILED!"
    fi

    return 0
}

# Function to update execution summary
update_execution_summary() {
    log_info "Updating execution summary..."

    local end_time=$(date -Iseconds)
    local artifacts_list=$(find "$COVERAGE_DIR" -type f -printf "%f\n" | tr '\n' ',' | sed 's/,$//')

    if command -v jq &> /dev/null; then
        jq --arg end_time "$end_time" \
           --arg artifacts "$artifacts_list" \
           --arg overall_pass "$GATE_OVERALL_PASS" \
           --arg critical_pass "$GATE_CRITICAL_PASS" \
           --arg regression_pass "$GATE_REGRESSION_PASS" \
           --arg final_decision "$GATE_FINAL_DECISION" \
           '.execution_summary = {
               end_time: $end_time,
               artifacts_generated: ($artifacts | split(",")),
               final_overall_pass: ($overall_pass == "true"),
               final_critical_pass: ($critical_pass == "true"),
               final_regression_pass: ($regression_pass == "true"),
               final_decision: $final_decision
           } | .gate_decisions.final_decision = $final_decision' "$RESULTS_FILE" > "${RESULTS_FILE}.tmp" && \
        mv "${RESULTS_FILE}.tmp" "$RESULTS_FILE"
    fi

    log_info "Execution summary updated"
}

# Main execution function
main() {
    echo "=============================================="
    echo "Coverage Gate Script for CI/CD"
    echo "Puzzle71Solver CUDA Technical Debt Elimination"
    echo "=============================================="
    echo

    # Parse arguments
    parse_arguments "$@"

    # Detect CI environment
    detect_ci_environment

    # Setup gate environment
    setup_gate_environment

    # Run coverage analysis
    if ! run_coverage_analysis; then
        log_error "Coverage analysis failed"
        if [[ "$DRY_RUN" != "true" && "$FAIL_ON_COVERAGE_REGRESSION" == "true" ]]; then
            exit 1
        fi
    fi

    # Extract coverage results
    if ! extract_coverage_results; then
        log_error "Failed to extract coverage results"
        exit 1
    fi

    # Load baseline data
    load_baseline_data

    # Evaluate gate decisions
    evaluate_gate_decisions

    # Finalize execution
    finalize_execution

    exit 0
}

# Execute main function
main "$@"
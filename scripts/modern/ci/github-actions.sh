#!/bin/bash
# Puzzle71Solver - GitHub Actions Integration
# Provides CI/CD pipeline integration for GitHub Actions

set -euo pipefail

# Get script directory and load utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=../utils.sh
source "$SCRIPT_DIR/../utils.sh"

# Default configuration
DEFAULT_WORKFLOW="performance-validation"
DEFAULT_TIMEOUT=3600

# Show help
show_help() {
    cat << EOF
Puzzle71Solver GitHub Actions Integration

USAGE:
    github-actions.sh [options] <command>

COMMANDS:
    setup                 Setup GitHub Actions workflow files
    validate              Validate CI configuration
    test                  Run CI pipeline locally
    deploy                Deploy build artifacts
    cleanup               Cleanup CI resources

OPTIONS:
    --workflow <name>     Workflow type [default: $DEFAULT_WORKFLOW]
    --timeout <seconds>   Timeout for operations [default: $DEFAULT_TIMEOUT]
    --token <token>       GitHub token for API access
    --repo <repo>         Repository name (owner/repo)
    --dry-run             Show what would be done without executing
    --verbose, -v         Enable verbose output
    --help, -h            Show this help

EXAMPLES:
    github-actions.sh setup
    github-actions.sh validate --workflow performance-validation
    github-actions.sh test --dry-run
    github-actions.sh deploy --token \$GITHUB_TOKEN

This script provides GitHub Actions integration for CI/CD pipelines
including performance validation, testing, and automated deployments.
EOF
}

# Parse command line arguments
parse_args() {
    COMMAND=""
    WORKFLOW="$DEFAULT_WORKFLOW"
    TIMEOUT="$DEFAULT_TIMEOUT"
    GITHUB_TOKEN=""
    REPOSITORY=""
    DRY_RUN=false
    VERBOSE=false

    while [[ $# -gt 0 ]]; do
        case $1 in
            setup|validate|test|deploy|cleanup)
                COMMAND="$1"
                shift
                ;;
            --workflow)
                WORKFLOW="$2"
                shift 2
                ;;
            --timeout)
                TIMEOUT="$2"
                shift 2
                ;;
            --token)
                GITHUB_TOKEN="$2"
                shift 2
                ;;
            --repo)
                REPOSITORY="$2"
                shift 2
                ;;
            --dry-run)
                DRY_RUN=true
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

    if [[ -z "$COMMAND" ]]; then
        error_exit "Command is required. Use --help for usage information."
    fi
}

# Validate GitHub CLI
validate_github_cli() {
    if ! command_exists "gh"; then
        log_warning "GitHub CLI not found. Install for full functionality:"
        log_info "  https://cli.github.com/manual/installation"
        return 1
    fi

    if [[ -z "$GITHUB_TOKEN" ]]; then
        if [[ -n "${GITHUB_TOKEN:-}" ]]; then
            GITHUB_TOKEN="$GITHUB_TOKEN"
        elif gh auth status &>/dev/null; then
            log_info "Using GitHub CLI authentication"
        else
            log_warning "GitHub token not provided. Use --token or run 'gh auth login'"
            return 1
        fi
    fi

    return 0
}

# Get repository information
get_repository_info() {
    local project_root
    project_root="$(get_project_root)"

    if [[ -z "$REPOSITORY" ]]; then
        if [[ -d "$project_root/.git" ]]; then
            REPOSITORY="$(cd "$project_root" && git config --get remote.origin.url | sed 's/.*:\/\/github.com\///;s/\.git$//' || echo "")"
            if [[ -z "$REPOSITORY" ]]; then
                REPOSITORY="owner/repo"
                log_warning "Could not detect repository, using placeholder: $REPOSITORY"
            fi
        else
            REPOSITORY="owner/repo"
            log_warning "Not a git repository, using placeholder: $REPOSITORY"
        fi
    fi

    log_debug "Repository: $REPOSITORY"
}

# Setup GitHub Actions workflows
setup_workflows() {
    progress_start "Setting up GitHub Actions workflows"

    local project_root
    project_root="$(get_project_root)"
    local workflows_dir="$project_root/.github/workflows"

    # Create workflows directory
    ensure_dir "$workflows_dir"

    # Create performance validation workflow
    create_performance_workflow "$workflows_dir"

    # Create testing workflow
    create_testing_workflow "$workflows_dir"

    # Create deployment workflow
    create_deployment_workflow "$workflows_dir"

    # Create security workflow
    create_security_workflow "$workflows_dir"

    progress_end "GitHub Actions workflows setup"
}

# Create performance validation workflow
create_performance_workflow() {
    local workflows_dir="$1"

    cat > "$workflows_dir/performance-validation.yml" << 'EOF'
name: Performance Validation

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]
  schedule:
    - cron: '0 2 * * *'  # Daily at 2 AM UTC

jobs:
  performance-test:
    runs-on: self-hosted
    timeout-minutes: 60

    strategy:
      matrix:
        gpu: [rtx2080ti, rtx3090, a100]

    steps:
    - name: Checkout code
      uses: actions/checkout@v3
      with:
        submodules: recursive

    - name: Setup CUDA
      uses: Jimver/cuda-toolkit@v0.2.4
      with:
        cuda: '11.8'

    - name: Setup dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y cmake build-essential libsecp256k1-dev

    - name: Setup environment
      run: |
        ./scripts/modern/scripts setup --build-type Release --verbose

    - name: Build project
      run: |
        ./scripts/modern/scripts build --optimize --parallel 8

    - name: Run performance benchmark
      run: |
        ./scripts/modern/scripts benchmark --duration 600 --gpu 0 --output-format json

    - name: Validate performance
      run: |
        ./scripts/modern/scripts validate --level performance --baseline benchmarks/baselines/${{ matrix.gpu }}.json

    - name: Upload results
      uses: actions/upload-artifact@v3
      if: always()
      with:
        name: performance-results-${{ matrix.gpu }}
        path: |
          build/benchmarks/
          build/validation/
          build/logs/

    - name: Performance regression check
      run: |
        if [[ -f "build/validation/performance_regression.json" ]]; then
          echo "Performance regression detected!"
          cat build/validation/performance_regression.json
          exit 1
        fi
EOF

    log_info "Created performance validation workflow"
}

# Create testing workflow
create_testing_workflow() {
    local workflows_dir="$1"

    cat > "$workflows_dir/testing.yml" << 'EOF'
name: Testing

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  unit-tests:
    runs-on: ubuntu-latest
    container: nvidia/cuda:11.8-devel-ubuntu20.04

    steps:
    - name: Install dependencies
      run: |
        apt-get update
        apt-get install -y cmake build-essential git libsecp256k1-dev python3

    - name: Checkout code
      uses: actions/checkout@v3
      with:
        submodules: recursive

    - name: Setup environment
      run: |
        ./scripts/modern/scripts setup --build-type RelWithDebInfo --skip-deps

    - name: Build project
      run: |
        ./scripts/modern/scripts build --parallel 4

    - name: Run tests
      run: |
        ./scripts/modern/scripts test --parallel 4 --format junit --output-file test-results.xml

    - name: Upload test results
      uses: actions/upload-artifact@v3
      if: always()
      with:
        name: test-results
        path: test-results.xml

    - name: Publish test results
      uses: dorny/test-reporter@v1
      if: success() || failure()
      with:
        name: Test Results
        path: test-results.xml
        reporter: java-junit

  integration-tests:
    runs-on: self-hosted
    needs: unit-tests
    if: success()

    steps:
    - name: Checkout code
      uses: actions/checkout@v3
      with:
        submodules: recursive

    - name: Setup CUDA
      uses: Jimver/cuda-toolkit@v0.2.4

    - name: Build and test
      run: |
        ./scripts/modern/scripts setup
        ./scripts/modern/scripts build
        ./scripts/modern/scripts test integration --verbose
EOF

    log_info "Created testing workflow"
}

# Create deployment workflow
create_deployment_workflow() {
    local workflows_dir="$1"

    cat > "$workflows_dir/deployment.yml" << 'EOF'
name: Deployment

on:
  push:
    tags: [ 'v*' ]
  workflow_dispatch:
    inputs:
      environment:
        description: 'Deployment environment'
        required: true
        default: 'staging'
        type: choice
        options:
        - staging
        - production

jobs:
  build-and-deploy:
    runs-on: ubuntu-latest
    timeout-minutes: 120

    environment:
      name: ${{ github.event.inputs.environment || 'staging' }}

    steps:
    - name: Checkout code
      uses: actions/checkout@v3
      with:
        submodules: recursive

    - name: Setup CUDA
      uses: Jimver/cuda-toolkit@v0.2.4

    - name: Build release
      run: |
        ./scripts/modern/scripts setup --build-type Release
        ./scripts/modern/scripts build --optimize --parallel 8

    - name: Run validation
      run: |
        ./scripts/modern/scripts validate --level comprehensive

    - name: Create deployment package
      run: |
        ./scripts/modern/scripts deploy --create-package --output-dir deployment/

    - name: Upload deployment artifacts
      uses: actions/upload-artifact@v3
      with:
        name: deployment-package
        path: deployment/

    - name: Deploy to staging
      if: github.event.inputs.environment == 'staging' || github.ref == 'refs/heads/main'
      run: |
        echo "Deploying to staging..."
        # Add staging deployment commands here

    - name: Deploy to production
      if: github.event.inputs.environment == 'production' || startsWith(github.ref, 'refs/tags/v')
      run: |
        echo "Deploying to production..."
        # Add production deployment commands here
EOF

    log_info "Created deployment workflow"
}

# Create security workflow
create_security_workflow() {
    local workflows_dir="$1"

    cat > "$workflows_dir/security.yml" << 'EOF'
name: Security

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]
  schedule:
    - cron: '0 6 * * 1'  # Weekly on Monday at 6 AM UTC

jobs:
  security-scan:
    runs-on: ubuntu-latest

    steps:
    - name: Checkout code
      uses: actions/checkout@v3

    - name: Run security scan
      run: |
        ./scripts/modern/scripts validate --level comprehensive --security-scan

    - name: Scan for secrets
      uses: trufflesecurity/trufflehog@main
      with:
        path: ./
        base: main
        head: HEAD

    - name: Dependency vulnerability scan
      run: |
        if command -v safety &>/dev/null; then
          safety check
        fi

    - name: Upload security results
      uses: actions/upload-artifact@v3
      if: always()
      with:
        name: security-results
        path: build/security/
EOF

    log_info "Created security workflow"
}

# Validate CI configuration
validate_configuration() {
    progress_start "Validating CI configuration"

    local project_root
    project_root="$(get_project_root)"
    local workflows_dir="$project_root/.github/workflows"

    # Check workflows directory
    require_dir "$workflows_dir" "GitHub workflows directory"

    # Validate workflow files
    local workflows=(
        "performance-validation.yml"
        "testing.yml"
        "deployment.yml"
        "security.yml"
    )

    for workflow in "${workflows[@]}"; do
        local workflow_file="$workflows_dir/$workflow"
        require_file "$workflow_file" "Workflow file: $workflow"

        # Validate YAML syntax
        if command_exists "yamllint"; then
            yamllint "$workflow_file" || log_warning "YAML linting issues found in $workflow"
        fi

        log_debug "Validated workflow: $workflow"
    done

    # Check scripts integration
    local main_script="$project_root/scripts/modern/scripts"
    require_file "$main_script" "Main scripts entry point"

    # Test script functionality
    log_info "Testing script integration..."
    if "$main_script" --help &>/dev/null; then
        log_success "Script integration test passed"
    else
        log_error "Script integration test failed"
        return 1
    fi

    progress_end "CI configuration validation"
}

# Run CI pipeline locally
test_pipeline() {
    progress_start "Running CI pipeline locally"

    local project_root
    project_root="$(get_project_root)"

    # Simulate CI environment
    export CI=true
    export GITHUB_ACTIONS=true
    export GITHUB_WORKFLOW="$WORKFLOW"
    export GITHUB_REPOSITORY="$REPOSITORY"

    log_info "Simulating $WORKFLOW workflow..."

    case "$WORKFLOW" in
        performance-validation)
            run_performance_tests
            ;;
        testing)
            run_unit_tests
            run_integration_tests
            ;;
        security)
            run_security_tests
            ;;
        *)
            log_warning "Unknown workflow: $WORKFLOW"
            ;;
    esac

    progress_end "CI pipeline test"
}

# Run performance tests
run_performance_tests() {
    log_info "Running performance validation..."

    local project_root
    project_root="$(get_project_root)"

    cd "$project_root"

    if [[ "$DRY_RUN" == true ]]; then
        log_info "[DRY RUN] Would run: ./scripts/modern/scripts setup"
        log_info "[DRY RUN] Would run: ./scripts/modern/scripts build --optimize"
        log_info "[DRY RUN] Would run: ./scripts/modern/scripts benchmark --duration 60"
        log_info "[DRY RUN] Would run: ./scripts/modern/scripts validate --level performance"
    else
        ./scripts/modern/scripts setup --build-type Release
        ./scripts/modern/scripts build --optimize
        ./scripts/modern/scripts benchmark --duration 60 --output-format json
        ./scripts/modern/scripts validate --level performance
    fi
}

# Run unit tests
run_unit_tests() {
    log_info "Running unit tests..."

    local project_root
    project_root="$(get_project_root)"

    cd "$project_root"

    if [[ "$DRY_RUN" == true ]]; then
        log_info "[DRY RUN] Would run: ./scripts/modern/scripts test unit --format junit"
    else
        ./scripts/modern/scripts test unit --format junit --output-file test-results.xml
    fi
}

# Run integration tests
run_integration_tests() {
    log_info "Running integration tests..."

    local project_root
    project_root="$(get_project_root)"

    cd "$project_root"

    if [[ "$DRY_RUN" == true ]]; then
        log_info "[DRY RUN] Would run: ./scripts/modern/scripts test integration"
    else
        ./scripts/modern/scripts test integration
    fi
}

# Run security tests
run_security_tests() {
    log_info "Running security tests..."

    local project_root
    project_root="$(get_project_root)"

    cd "$project_root"

    if [[ "$DRY_RUN" == true ]]; then
        log_info "[DRY RUN] Would run: ./scripts/modern/scripts validate --security-scan"
    else
        ./scripts/modern/scripts validate --level comprehensive --security-scan
    fi
}

# Deploy build artifacts
deploy_artifacts() {
    progress_start "Deploying build artifacts"

    local project_root
    project_root="$(get_project_root)"

    cd "$project_root"

    if [[ "$DRY_RUN" == true ]]; then
        log_info "[DRY RUN] Would run: ./scripts/modern/scripts deploy --create-package"
        log_info "[DRY RUN] Would upload artifacts to repository: $REPOSITORY"
    else
        # Create deployment package
        ./scripts/modern/scripts deploy --create-package --output-dir deployment/

        # Upload to GitHub releases if this is a tag
        if [[ "$GITHUB_REF" =~ ^refs/tags/v ]]; then
            upload_github_release
        fi
    fi

    progress_end "Build artifacts deployment"
}

# Upload GitHub release
upload_github_release() {
    if ! validate_github_cli; then
        log_warning "GitHub CLI not available, skipping release upload"
        return 0
    fi

    local tag="${GITHUB_REF#refs/tags/}"

    log_info "Creating GitHub release: $tag"

    # Create release
    if ! gh release create "$tag" --title "Release $tag" --generate-notes deployment/; then
        log_warning "Failed to create GitHub release"
        return 1
    fi

    log_success "GitHub release created: $tag"
}

# Cleanup CI resources
cleanup_resources() {
    progress_start "Cleaning up CI resources"

    local project_root
    project_root="$(get_project_root)"

    # Cleanup old artifacts
    if [[ -d "$project_root/build" ]]; then
        find "$project_root/build" -name "*.log" -type f -mtime +7 -delete 2>/dev/null || true
        find "$project_root/build" -name "telemetry_*" -type f -mtime +7 -delete 2>/dev/null || true
    fi

    # Cleanup temporary files
    if [[ -d "$project_root/tmp" ]]; then
        find "$project_root/tmp" -type f -mtime +1 -delete 2>/dev/null || true
    fi

    progress_end "CI resources cleanup"
}

# Show status
show_status() {
    log_info "GitHub Actions Integration Status:"
    log_info "  Repository: $REPOSITORY"
    log_info "  Workflow: $WORKFLOW"
    log_info "  GitHub CLI: $(command_exists "gh" && echo "Available" || echo "Not available")"
    log_info "  Authentication: ${GITHUB_TOKEN:+Set}${GITHUB_TOKEN:-Not set}"
    log_info "  Dry Run: $DRY_RUN"
}

# Main function
main() {
    log_info "Starting GitHub Actions integration..."

    # Parse arguments
    parse_args "$@"

    # Validate environment
    validate_environment

    # Get repository information
    get_repository_info

    # Show status if verbose
    if [[ "$VERBOSE" == true ]]; then
        show_status
    fi

    # Execute command
    case "$COMMAND" in
        setup)
            setup_workflows
            ;;
        validate)
            validate_configuration
            ;;
        test)
            test_pipeline
            ;;
        deploy)
            deploy_artifacts
            ;;
        cleanup)
            cleanup_resources
            ;;
        *)
            error_exit "Unknown command: $COMMAND"
            ;;
    esac

    log_success "GitHub Actions integration completed!"
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
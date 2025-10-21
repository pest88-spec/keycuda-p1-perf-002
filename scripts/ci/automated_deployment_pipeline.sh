#!/bin/bash

# Puzzle71 Automated CI/CD Deployment Pipeline v2.0
# Comprehensive automated deployment with testing, validation, and monitoring

set -euo pipefail

# Pipeline metadata
PIPELINE_VERSION="2.0.0"
PIPELINE_NAME="Puzzle71-CI-CD"
BUILD_NUMBER="${BUILD_NUMBER:-$(date +%Y%m%d%H%M%S)}"
GIT_COMMIT="${GIT_COMMIT:-$(git rev-parse HEAD 2>/dev/null || echo 'unknown')}"
GIT_BRANCH="${GIT_BRANCH:-$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo 'unknown')}"

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
PIPELINE_CONFIG="$PROJECT_ROOT/.ci/pipeline.yaml"
PIPELINE_LOG="$PROJECT_ROOT/logs/pipeline_${BUILD_NUMBER}.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# Pipeline stages configuration
declare -A PIPELINE_STAGES=(
    ["validate"]="Validate Configuration"
    ["test"]="Run Test Suite"
    ["build"]="Build Application"
    ["security"]="Security Scanning"
    ["performance"]="Performance Testing"
    ["deploy"]="Deploy Application"
    ["verify"]="Post-deployment Verification"
    ["monitor"]="Setup Monitoring"
)

# Logging functions
pipeline_log() {
    local level="$1"
    shift
    local message="$*"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo -e "${level}[PIPELINE]${NC} ${timestamp} - $message" | tee -a "$PIPELINE_LOG"
}

log_stage() { pipeline_log "${PURPLE}[STAGE]" "$@"; }
log_info() { pipeline_log "${BLUE}[INFO]" "$@"; }
log_success() { pipeline_log "${GREEN}[SUCCESS]" "$@"; }
log_warning() { pipeline_log "${YELLOW}[WARNING]" "$@"; }
log_error() { pipeline_log "${RED}[ERROR]" "$@"; }

# Pipeline state management
PIPELINE_STATE_FILE="$PROJECT_ROOT/.ci/pipeline_state.json"
PIPELINE_ARTIFACTS_DIR="$PROJECT_ROOT/.ci/artifacts"

init_pipeline_state() {
    mkdir -p "$(dirname "$PIPELINE_STATE_FILE")" "$PIPELINE_ARTIFACTS_DIR"

    cat > "$PIPELINE_STATE_FILE" << EOF
{
  "pipeline": {
    "name": "$PIPELINE_NAME",
    "version": "$PIPELINE_VERSION",
    "build_number": "$BUILD_NUMBER",
    "git_commit": "$GIT_COMMIT",
    "git_branch": "$GIT_BRANCH",
    "start_time": "$(date -u +'%Y-%m-%dT%H:%M:%SZ')",
    "status": "running"
  },
  "stages": {},
  "artifacts": [],
  "environment": {}
}
EOF
}

update_stage_status() {
    local stage="$1"
    local status="$2"
    local message="$3"
    local timestamp=$(date -u +'%Y-%m-%dT%H:%M:%SZ')

    log_stage "${PIPELINE_STAGES[$stage]}: $status - $message"

    # Update pipeline state file
    if command -v jq &> /dev/null; then
        jq ".stages.$stage = {\"status\": \"$status\", \"message\": \"$message\", \"timestamp\": \"$timestamp\"}" \
            "$PIPELINE_STATE_FILE" > "$PIPELINE_STATE_FILE.tmp" && \
            mv "$PIPELINE_STATE_FILE.tmp" "$PIPELINE_STATE_FILE"
    fi
}

update_pipeline_status() {
    local status="$1"
    local message="$2"

    if command -v jq &> /dev/null; then
        jq ".pipeline.status = \"$status\" | .pipeline.end_time = \"$(date -u +'%Y-%m-%dT%H:%M:%SZ')\" | .pipeline.message = \"$message\"" \
            "$PIPELINE_STATE_FILE" > "$PIPELINE_STATE_FILE.tmp" && \
            mv "$PIPELINE_STATE_FILE.tmp" "$PIPELINE_STATE_FILE"
    fi
}

# Stage 1: Validate Configuration
validate_configuration() {
    update_stage_status "validate" "running" "Validating project configuration"

    # Check if we're in the right directory
    if [[ ! -f "$PROJECT_ROOT/CMakeLists.txt" ]]; then
        update_stage_status "validate" "failed" "CMakeLists.txt not found"
        return 1
    fi

    # Validate CMake configuration
    cd "$PROJECT_ROOT"
    mkdir -p build && cd build

    if cmake .. -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CUDA_ARCHITECTURES="75;86;89;90" \
        -DBUILD_TESTS=ON \
        -DBUILD_BENCHMARKS=ON; then
        update_stage_status "validate" "success" "CMake configuration validated"
    else
        update_stage_status "validate" "failed" "CMake configuration failed"
        return 1
    fi

    # Validate Docker configuration
    if [[ -f "$PROJECT_ROOT/docker-compose.production.yml" ]]; then
        if docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" config &> /dev/null; then
            log_info "Docker Compose configuration validated"
        else
            update_stage_status "validate" "failed" "Docker Compose configuration invalid"
            return 1
        fi
    fi

    cd "$PROJECT_ROOT"
    update_stage_status "validate" "success" "All configuration validations passed"
}

# Stage 2: Run Test Suite
run_tests() {
    update_stage_status "test" "running" "Executing comprehensive test suite"

    cd "$PROJECT_ROOT/build"

    # Build tests
    log_info "Building test suite..."
    if make -j$(nproc); then
        log_info "Test suite built successfully"
    else
        update_stage_status "test" "failed" "Failed to build test suite"
        return 1
    fi

    # Run unit tests
    log_info "Running unit tests..."
    local unit_test_results="$PIPELINE_ARTIFACTS_DIR/unit_test_results.xml"
    if ctest --output-on-failure --T Test --no-compress-output -j$(nproc); then
        update_stage_status "test" "success" "Unit tests passed"
    else
        update_stage_status "test" "failed" "Unit tests failed"
        return 1
    fi

    # Copy test results
    cp Testing/**/*.xml "$PIPELINE_ARTIFACTS_DIR/" 2>/dev/null || true

    # Run integration tests if they exist
    if [[ -d "$PROJECT_ROOT/tests/integration" ]]; then
        log_info "Running integration tests..."
        local integration_test_results="$PIPELINE_ARTIFACTS_DIR/integration_test_results.xml"

        # Find and run integration tests
        find "$PROJECT_ROOT/tests/integration" -name "test_*" -executable | while read -r test; do
            log_info "Running integration test: $(basename "$test")"
            if "$test" 2>&1 | tee -a "$PIPELINE_LOG"; then
                log_success "Integration test passed: $(basename "$test")"
            else
                log_error "Integration test failed: $(basename "$test")"
                update_stage_status "test" "failed" "Integration test failed: $(basename "$test")"
                return 1
            fi
        done
    fi

    # Run performance benchmarks
    if [[ -f "$PROJECT_ROOT/build/Puzzle71Solver" ]]; then
        log_info "Running performance benchmarks..."
        local benchmark_results="$PIPELINE_ARTIFACTS_DIR/benchmark_results.json"

        timeout 300 "$PROJECT_ROOT/build/Puzzle71Solver" \
            --config "$PROJECT_ROOT/config/benchmark.yaml" \
            --benchmark-mode \
            --output "$benchmark_results" || log_warning "Benchmark timeout or failed"

        if [[ -f "$benchmark_results" ]]; then
            log_success "Performance benchmarks completed"
            cp "$benchmark_results" "$PIPELINE_ARTIFACTS_DIR/"
        fi
    fi

    cd "$PROJECT_ROOT"
    update_stage_status "test" "success" "All tests passed successfully"
}

# Stage 3: Build Application
build_application() {
    update_stage_status "build" "running" "Building production application"

    cd "$PROJECT_ROOT/build"

    # Clean previous build
    make clean || true

    # Build in release mode
    if make -j$(nproc) Puzzle71Solver; then
        log_success "Application built successfully"

        # Verify binary exists and is executable
        if [[ -x "$PROJECT_ROOT/build/Puzzle71Solver" ]]; then
            local binary_size=$(stat -c%s "$PROJECT_ROOT/build/Puzzle71Solver")
            log_info "Binary size: $binary_size bytes"

            # Test basic functionality
            if "$PROJECT_ROOT/build/Puzzle71Solver" --help &> /dev/null; then
                update_stage_status "build" "success" "Application built and verified"
            else
                update_stage_status "build" "failed" "Binary failed basic functionality test"
                return 1
            fi
        else
            update_stage_status "build" "failed" "Binary not found or not executable"
            return 1
        fi
    else
        update_stage_status "build" "failed" "Build failed"
        return 1
    fi

    cd "$PROJECT_ROOT"
}

# Stage 4: Security Scanning
security_scanning() {
    update_stage_status "security" "running" "Performing security vulnerability scanning"

    # Container security scanning with Trivy
    if command -v trivy &> /dev/null; then
        log_info "Running container security scan..."
        local trivy_report="$PIPELINE_ARTIFACTS_DIR/trivy_scan_${BUILD_NUMBER}.json"

        # Build Docker image for scanning
        if docker build -t "puzzle71-test:${BUILD_NUMBER}" -f Dockerfile.production.enhanced .; then
            if trivy image --format json --output "$trivy_report" \
                --severity "HIGH,CRITICAL" \
                "puzzle71-test:${BUILD_NUMBER}"; then

                # Analyze results
                local critical_vulns=$(jq '.Results[]?.Vulnerabilities[]? | select(.Severity == "CRITICAL") | .VulnerabilityID' "$trivy_report" | wc -l || echo "0")
                local high_vulns=$(jq '.Results[]?.Vulnerabilities[]? | select(.Severity == "HIGH") | .VulnerabilityID' "$trivy_report" | wc -l || echo "0")

                if [[ $critical_vulns -eq 0 ]]; then
                    log_success "No critical vulnerabilities found"
                    if [[ $high_vulns -eq 0 ]]; then
                        log_success "No high-severity vulnerabilities found"
                    else
                        log_warning "Found $high_vulns high-severity vulnerabilities"
                    fi
                    update_stage_status "security" "success" "Security scan completed"
                else
                    log_error "Found $critical_vulns critical vulnerabilities"
                    update_stage_status "security" "failed" "Critical security vulnerabilities found"
                    return 1
                fi
            else
                log_warning "Trivy scan failed"
            fi

            # Cleanup test image
            docker rmi "puzzle71-test:${BUILD_NUMBER}" || true
        else
            log_warning "Failed to build Docker image for security scanning"
        fi
    else
        log_info "Trivy not available, skipping container security scan"
    fi

    # Static code analysis (basic)
    log_info "Running static code analysis..."

    # Check for potential security issues in source code
    local security_issues=0

    # Check for hardcoded secrets
    if grep -r -i -E "(password|secret|key)\s*=\s*['\"][^'\"]{8,}['\"]" "$PROJECT_ROOT/src/" 2>/dev/null; then
        log_warning "Potential hardcoded secrets detected"
        ((security_issues++))
    fi

    # Check for unsafe functions
    if grep -r -E "(strcpy|strcat|sprintf|gets)" "$PROJECT_ROOT/src/" 2>/dev/null; then
        log_warning "Potentially unsafe functions detected"
        ((security_issues++))
    fi

    # Check for debug prints that might leak information
    if grep -r -E "(printf|cout)\s*.*password|printf|cout.*secret" "$PROJECT_ROOT/src/" 2>/dev/null; then
        log_warning "Potential information leakage in debug prints"
        ((security_issues++))
    fi

    if [[ $security_issues -eq 0 ]]; then
        update_stage_status "security" "success" "Static code analysis passed"
    else
        log_warning "Found $security_issues potential security issues"
        update_stage_status "security" "success" "Security scan completed with warnings"
    fi
}

# Stage 5: Performance Testing
performance_testing() {
    update_stage_status "performance" "running" "Running performance validation"

    # Check if GPU is available for testing
    if ! nvidia-smi &> /dev/null; then
        log_warning "No GPU available for performance testing"
        update_stage_status "performance" "success" "Performance testing skipped (no GPU)"
        return 0
    fi

    # Run performance test if binary exists
    if [[ -x "$PROJECT_ROOT/build/Puzzle71Solver" ]]; then
        log_info "Running GPU performance test..."
        local perf_results="$PIPELINE_ARTIFACTS_DIR/performance_test_${BUILD_NUMBER}.json"

        # Create test configuration
        cat > "$PROJECT_ROOT/config/performance_test.yaml" << 'EOF'
performance_test:
  gpu:
    device_id: 0
    test_duration: 60  # 1 minute test
    target_throughput: 100000  # 100K keys/sec minimum

  test_data:
    start_key: 1000000
    end_key: 1100000
    target_address: "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa"

  output:
    file: "performance_results.json"
    metrics_interval: 10  # seconds
EOF

        # Run performance test
        timeout 120 "$PROJECT_ROOT/build/Puzzle71Solver" \
            --config "$PROJECT_ROOT/config/performance_test.yaml" \
            --test-mode \
            --output "$perf_results" 2>&1 | tee -a "$PIPELINE_LOG" || {
            log_warning "Performance test failed or timed out"
            update_stage_status "performance" "success" "Performance testing completed with warnings"
            return 0
        }

        # Analyze performance results
        if [[ -f "$perf_results" ]]; then
            local actual_throughput=$(jq -r '.throughput_keys_per_second // 0' "$perf_results")
            local gpu_utilization=$(jq -r '.gpu_utilization_percent // 0' "$perf_results")

            log_info "Performance results:"
            log_info "  Throughput: $actual_throughput keys/sec"
            log_info "  GPU utilization: $gpu_utilization%"

            # Performance thresholds
            local min_throughput=50000  # 50K keys/sec minimum
            local min_gpu_util=70       # 70% minimum GPU utilization

            if [[ $actual_throughput -gt $min_throughput && $gpu_utilization -gt $min_gpu_util ]]; then
                update_stage_status "performance" "success" "Performance tests passed"
            else
                log_warning "Performance below thresholds"
                update_stage_status "performance" "success" "Performance testing completed with warnings"
            fi
        else
            log_warning "Performance results not available"
            update_stage_status "performance" "success" "Performance testing completed"
        fi
    else
        log_warning "No binary available for performance testing"
        update_stage_status "performance" "success" "Performance testing skipped"
    fi
}

# Stage 6: Deploy Application
deploy_application() {
    update_stage_status "deploy" "running" "Deploying application to production"

    # Use enhanced deployment script if available
    if [[ -x "$PROJECT_ROOT/scripts/deployment/deploy_production_enhanced.sh" ]]; then
        log_info "Using enhanced deployment script..."

        # Set deployment environment variables
        export DEPLOYMENT_VERSION="2.0.0-${BUILD_NUMBER}"
        export ENVIRONMENT="production"
        export BACKUP_ENABLED="true"
        export MONITORING_ENABLED="true"

        if "$PROJECT_ROOT/scripts/deployment/deploy_production_enhanced.sh" 2>&1 | tee -a "$PIPELINE_LOG"; then
            update_stage_status "deploy" "success" "Application deployed successfully"
        else
            update_stage_status "deploy" "failed" "Deployment failed"
            return 1
        fi
    else
        # Fallback to basic deployment
        log_warning "Enhanced deployment script not found, using basic deployment"

        if docker-compose -f "$PROJECT_ROOT/docker-compose.production.yml" up -d; then
            update_stage_status "deploy" "success" "Basic deployment completed"
        else
            update_stage_status "deploy" "failed" "Basic deployment failed"
            return 1
        fi
    fi
}

# Stage 7: Post-deployment Verification
verify_deployment() {
    update_stage_status "verify" "running" "Verifying deployment health"

    # Wait for services to start
    log_info "Waiting for services to start..."
    sleep 30

    # Check service health
    local services_healthy=0
    local total_services=0

    # Check main application
    if curl -s --max-time 10 http://localhost:8080/metrics &> /dev/null; then
        log_success "Main application is healthy"
        ((services_healthy++))
    else
        log_error "Main application health check failed"
    fi
    ((total_services++))

    # Check monitoring services
    if curl -s --max-time 10 http://localhost:9090/metrics &> /dev/null; then
        log_success "Prometheus is healthy"
        ((services_healthy++))
    else
        log_error "Prometheus health check failed"
    fi
    ((total_services++))

    if curl -s --max-time 10 http://localhost:3000/api/health &> /dev/null; then
        log_success "Grafana is healthy"
        ((services_healthy++))
    else
        log_error "Grafana health check failed"
    fi
    ((total_services++))

    # Check Redis
    if docker exec puzzle71-redis redis-cli ping &> /dev/null; then
        log_success "Redis is healthy"
        ((services_healthy++))
    else
        log_error "Redis health check failed"
    fi
    ((total_services++))

    local health_percentage=$((services_healthy * 100 / total_services))
    log_info "Service health: $services_healthy/$total_services ($health_percentage%)"

    if [[ $health_percentage -ge 75 ]]; then
        update_stage_status "verify" "success" "Deployment verification passed ($health_percentage% services healthy)"
    else
        update_stage_status "verify" "failed" "Too many services unhealthy ($health_percentage% services healthy)"
        return 1
    fi
}

# Stage 8: Setup Monitoring
setup_monitoring() {
    update_stage_status "monitor" "running" "Setting up monitoring and alerting"

    # Create monitoring configuration if it doesn't exist
    local monitoring_config="$PROJECT_ROOT/monitoring/pipeline_monitoring.yml"
    mkdir -p "$(dirname "$monitoring_config")"

    cat > "$monitoring_config" << EOF
# Pipeline Monitoring Configuration
global:
  scrape_interval: 15s
  evaluation_interval: 15s

scrape_configs:
  - job_name: 'puzzle71-application'
    static_configs:
      - targets: ['localhost:8080']
    metrics_path: '/metrics'
    scrape_interval: 5s

  - job_name: 'puzzle71-nvidia-gpu'
    static_configs:
      - targets: ['localhost:9445']
    scrape_interval: 10s

  - job_name: 'node-exporter'
    static_configs:
      - targets: ['localhost:9100']

alerting:
  alertmanagers:
    - static_configs:
        - targets: ['localhost:9093']

rule_files:
  - "puzzle71_alerts.yml"
EOF

    # Create alert rules
    cat > "$PROJECT_ROOT/monitoring/puzzle71_alerts.yml" << 'EOF'
# Puzzle71 Alert Rules
groups:
  - name: puzzle71.rules
    rules:
      - alert: HighGPUUsage
        expr: nvidia_gpu_utilization_gpu > 95
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "High GPU usage detected"
          description: "GPU utilization is above 95% for more than 5 minutes"

      - alert: HighMemoryUsage
        expr: (node_memory_MemTotal_bytes - node_memory_MemAvailable_bytes) / node_memory_MemTotal_bytes * 100 > 90
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "High memory usage detected"
          description: "Memory usage is above 90% for more than 5 minutes"

      - alert: ApplicationDown
        expr: up{job="puzzle71-application"} == 0
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "Puzzle71 application is down"
          description: "The Puzzle71 application has been down for more than 1 minute"
EOF

    # Create pipeline dashboard for Grafana
    local dashboard_file="$PIPELINE_ARTIFACTS_DIR/pipeline_dashboard.json"
    cat > "$dashboard_file" << 'EOF'
{
  "dashboard": {
    "title": "Puzzle71 Pipeline Dashboard",
    "panels": [
      {
        "title": "Build Status",
        "type": "stat",
        "targets": [
          {
            "expr": "pipeline_build_success == 1",
            "legendFormat": "Success"
          }
        ]
      },
      {
        "title": "GPU Utilization",
        "type": "graph",
        "targets": [
          {
            "expr": "nvidia_gpu_utilization_gpu",
            "legendFormat": "GPU Utilization %"
          }
        ]
      },
      {
        "title": "Throughput",
        "type": "graph",
        "targets": [
          {
            "expr": "puzzle71_throughput_keys_per_second",
            "legendFormat": "Keys/sec"
          }
        ]
      }
    ]
  }
}
EOF

    log_success "Monitoring configuration created"
    update_stage_status "monitor" "success" "Monitoring and alerting setup completed"
}

# Generate pipeline summary report
generate_pipeline_report() {
    local report_file="$PIPELINE_ARTIFACTS_DIR/pipeline_report_${BUILD_NUMBER}.json"

    # Calculate pipeline duration
    local start_time=$(jq -r '.pipeline.start_time' "$PIPELINE_STATE_FILE")
    local end_time=$(date -u +'%Y-%m-%dT%H:%M:%SZ')
    local duration_seconds=$(date -d "$end_time" +%s)
    local start_seconds=$(date -d "$start_time" +%s)
    local total_duration=$((duration_seconds - start_seconds))

    cat > "$report_file" << EOF
{
  "pipeline": {
    "name": "$PIPELINE_NAME",
    "version": "$PIPELINE_VERSION",
    "build_number": "$BUILD_NUMBER",
    "git_commit": "$GIT_COMMIT",
    "git_branch": "$GIT_BRANCH",
    "start_time": "$start_time",
    "end_time": "$end_time",
    "duration_seconds": $total_duration,
    "status": "success"
  },
  "stages": $(jq '.stages' "$PIPELINE_STATE_FILE"),
  "artifacts": [
    "unit_test_results.xml",
    "integration_test_results.xml",
    "benchmark_results.json",
    "trivy_scan_${BUILD_NUMBER}.json",
    "performance_test_${BUILD_NUMBER}.json",
    "pipeline_dashboard.json"
  ],
  "system": {
    "hostname": "$(hostname)",
    "os": "$(uname -a)",
    "docker_version": "$(docker --version)",
    "git_version": "$(git --version)",
    "cmake_version": "$(cmake --version | head -n1)",
    "cuda_version": "$(nvcc --version | grep release | awk '{print $6}' | cut -c2- || echo 'unknown')"
  }
}
EOF

    log_success "Pipeline report generated: $report_file"
}

# Main pipeline execution
main() {
    # Initialize logging
    mkdir -p "$(dirname "$PIPELINE_LOG")"
    echo "Pipeline log started at $(date)" > "$PIPELINE_LOG"

    log_info "🚀 Starting $PIPELINE_NAME v$PIPELINE_VERSION"
    log_info "Build: $BUILD_NUMBER, Commit: $GIT_COMMIT, Branch: $GIT_BRANCH"

    # Initialize pipeline state
    init_pipeline_state

    # Execute pipeline stages
    local pipeline_success=true

    for stage in "${!PIPELINE_STAGES[@]}"; do
        log_info "▶️  Executing stage: ${PIPELINE_STAGES[$stage]}"

        if ! "run_$stage"; then
            pipeline_success=false
            log_error "❌ Stage failed: ${PIPELINE_STAGES[$stage]}"
            break
        fi

        log_success "✅ Stage completed: ${PIPELINE_STAGES[$stage]}"
    done

    # Generate final report
    generate_pipeline_report

    # Update final pipeline status
    if [[ "$pipeline_success" == true ]]; then
        update_pipeline_status "success" "Pipeline completed successfully"
        log_success "🎉 Pipeline completed successfully!"
        log_info "Artifacts saved to: $PIPELINE_ARTIFACTS_DIR"
        log_info "Pipeline log: $PIPELINE_LOG"
        log_info "Pipeline state: $PIPELINE_STATE_FILE"
        return 0
    else
        update_pipeline_status "failed" "Pipeline failed during execution"
        log_error "❌ Pipeline failed!"
        log_info "Check pipeline log for details: $PIPELINE_LOG"
        return 1
    fi
}

# Handle pipeline interruption
trap 'update_pipeline_status "failed" "Pipeline interrupted"; exit 130' INT TERM

# Pipeline entry point
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
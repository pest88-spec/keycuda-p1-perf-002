# P1-PERF-002: CI/CD Configuration

**Date**: 2025-10-16  
**Status**: ✅ READY FOR INTEGRATION

---

## Overview

This document provides CI/CD pipeline configuration snippets for P1-PERF-002 deployment automation.

---

## GitHub Actions Workflow

### File: `.github/workflows/p1-perf-002-deploy.yml`

```yaml
name: P1-PERF-002 Production Deployment

on:
  push:
    tags:
      - 'v0.3.2*'
  workflow_dispatch:
    inputs:
      deployment_mode:
        description: 'Deployment mode (canary or full)'
        required: true
        default: 'canary'

jobs:
  deploy:
    runs-on: [self-hosted, gpu]
    timeout-minutes: 30
    
    steps:
      - name: Checkout code
        uses: actions/checkout@v3
        
      - name: Setup environment
        run: |
          echo "DEPLOYMENT_VERSION=${{ github.ref_name }}" >> $GITHUB_ENV
          echo "DEPLOYMENT_MODE=${{ github.event.inputs.deployment_mode || 'canary' }}" >> $GITHUB_ENV
          
      - name: Build
        run: |
          mkdir -p build && cd build
          cmake .. -DCMAKE_BUILD_TYPE=Release
          make -j$(nproc)
          
      - name: Verify binary
        run: |
          ./build/Puzzle71Solver --help | grep -E "(streams|auto-batch)"
          
      - name: Run deployment
        run: |
          bash scripts/deploy_prod.sh ${{ env.DEPLOYMENT_MODE }} 300
          
      - name: Upload deployment report
        if: always()
        uses: actions/upload-artifact@v3
        with:
          name: deployment-report-${{ github.run_id }}
          path: logs/deployment_report_*.md
          
      - name: Notify Slack
        if: always()
        uses: slackapi/slack-github-action@v1
        with:
          webhook-url: ${{ secrets.SLACK_WEBHOOK }}
          payload: |
            {
              "text": "P1-PERF-002 Deployment: ${{ job.status }}",
              "blocks": [
                {
                  "type": "section",
                  "text": {
                    "type": "mrkdwn",
                    "text": "*P1-PERF-002 Deployment*\nVersion: ${{ env.DEPLOYMENT_VERSION }}\nMode: ${{ env.DEPLOYMENT_MODE }}\nStatus: ${{ job.status }}"
                  }
                }
              ]
            }
```

---

## GitLab CI Configuration

### File: `.gitlab-ci.yml` (P1-PERF-002 section)

```yaml
stages:
  - build
  - test
  - deploy
  - monitor

variables:
  TARGET_CUDA: "11.0"
  DEPLOYMENT_MODE: "canary"

build:p1-perf-002:
  stage: build
  image: nvidia/cuda:11.0-devel-ubuntu20.04
  script:
    - mkdir -p build && cd build
    - cmake .. -DCMAKE_BUILD_TYPE=Release
    - make -j$(nproc)
  artifacts:
    paths:
      - build/Puzzle71Solver
    expire_in: 1 day

test:p1-perf-002:
  stage: test
  image: nvidia/cuda:11.0-devel-ubuntu20.04
  script:
    - ./build/Puzzle71Solver --help | grep -E "(streams|auto-batch)"
    - bash scripts/deploy_perf.sh quick 0
  artifacts:
    paths:
      - reports/deployment_*/
    expire_in: 7 days

deploy:p1-perf-002:
  stage: deploy
  image: nvidia/cuda:11.0-devel-ubuntu20.04
  script:
    - bash scripts/deploy_prod.sh $DEPLOYMENT_MODE 300
  artifacts:
    paths:
      - logs/deployment_*.log
      - logs/deployment_report_*.md
    expire_in: 30 days
  only:
    - tags
  when: manual

monitor:p1-perf-002:
  stage: monitor
  image: nvidia/cuda:11.0-devel-ubuntu20.04
  script:
    - bash scripts/run_profiling.sh 0 eccScalarMulKernel
  artifacts:
    paths:
      - profiling/
    expire_in: 30 days
  only:
    - tags
```

---

## Jenkins Pipeline

### File: `Jenkinsfile.p1-perf-002`

```groovy
pipeline {
    agent {
        label 'gpu-node'
    }
    
    parameters {
        choice(
            name: 'DEPLOYMENT_MODE',
            choices: ['canary', 'full'],
            description: 'Deployment mode'
        )
        string(
            name: 'MONITOR_DURATION',
            defaultValue: '300',
            description: 'Monitoring duration in seconds'
        )
    }
    
    environment {
        TARGET_CUDA = '11.0'
        DEPLOYMENT_VERSION = "${BUILD_TAG}"
    }
    
    stages {
        stage('Build') {
            steps {
                sh '''
                    mkdir -p build && cd build
                    cmake .. -DCMAKE_BUILD_TYPE=Release
                    make -j$(nproc)
                '''
            }
        }
        
        stage('Verify') {
            steps {
                sh '''
                    ./build/Puzzle71Solver --help | grep -E "(streams|auto-batch)"
                '''
            }
        }
        
        stage('Deploy') {
            steps {
                sh '''
                    bash scripts/deploy_prod.sh ${DEPLOYMENT_MODE} ${MONITOR_DURATION}
                '''
            }
        }
        
        stage('Monitor') {
            steps {
                sh '''
                    bash scripts/run_profiling.sh 0 eccScalarMulKernel
                '''
            }
        }
    }
    
    post {
        always {
            archiveArtifacts artifacts: 'logs/**/*.log,logs/**/*.md', allowEmptyArchive: true
            publishHTML([
                reportDir: 'logs',
                reportFiles: 'deployment_report_*.md',
                reportName: 'Deployment Report'
            ])
        }
        success {
            slackSend(
                color: 'good',
                message: "P1-PERF-002 Deployment Successful: ${BUILD_TAG}"
            )
        }
        failure {
            slackSend(
                color: 'danger',
                message: "P1-PERF-002 Deployment Failed: ${BUILD_TAG}"
            )
        }
    }
}
```

---

## Environment Variables

### Required Environment Variables

```bash
# CUDA Configuration
export TARGET_CUDA=11.0
export CUDA_HOME=/usr/local/cuda

# Deployment Configuration
export DEPLOYMENT_MODE=canary  # or 'full'
export MONITOR_DURATION=300    # seconds
export GPU_DEVICE=0

# Monitoring Configuration
export PROMETHEUS_URL=http://localhost:9090
export GRAFANA_URL=http://localhost:3000
export SLACK_WEBHOOK=https://hooks.slack.com/...

# Performance Thresholds
export THROUGHPUT_BASELINE=1000000000  # 1 Gkeys/s
export THROUGHPUT_DROP_THRESHOLD=5    # 5%
export ERROR_RATE_THRESHOLD=1         # 1%
export GPU_UTIL_THRESHOLD=50          # 50%
```

---

## Performance Gate Configuration

### File: `scripts/ci/performance_gate.sh`

```bash
#!/bin/bash

# P1-PERF-002 Performance Gate
# Validates performance against baseline

BASELINE_THROUGHPUT=${THROUGHPUT_BASELINE:-1000000000}
THROUGHPUT_DROP_THRESHOLD=${THROUGHPUT_DROP_THRESHOLD:-5}
ERROR_RATE_THRESHOLD=${ERROR_RATE_THRESHOLD:-1}

# Get current metrics
CURRENT_THROUGHPUT=$(nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader | head -1)
CURRENT_ERROR_RATE=$(grep -o "error_rate_percent: [0-9]*" logs/deployment_*.log | cut -d' ' -f2)

# Calculate throughput drop
THROUGHPUT_DROP=$((100 * (BASELINE_THROUGHPUT - CURRENT_THROUGHPUT) / BASELINE_THROUGHPUT))

# Check thresholds
if [ "$THROUGHPUT_DROP" -gt "$THROUGHPUT_DROP_THRESHOLD" ]; then
    echo "FAIL: Throughput drop ($THROUGHPUT_DROP%) exceeds threshold ($THROUGHPUT_DROP_THRESHOLD%)"
    exit 1
fi

if [ "$CURRENT_ERROR_RATE" -gt "$ERROR_RATE_THRESHOLD" ]; then
    echo "FAIL: Error rate ($CURRENT_ERROR_RATE%) exceeds threshold ($ERROR_RATE_THRESHOLD%)"
    exit 1
fi

echo "PASS: Performance gate validation successful"
exit 0
```

---

## Monitoring Integration

### Prometheus Configuration

```yaml
# prometheus.yml
global:
  scrape_interval: 15s
  evaluation_interval: 15s

scrape_configs:
  - job_name: 'puzzle71-p1-perf-002'
    static_configs:
      - targets: ['localhost:9090']
    metrics_path: '/metrics'
    scrape_interval: 30s
```

### Alert Rules

```yaml
# alert_rules.yml
groups:
  - name: p1_perf_002
    interval: 30s
    rules:
      - alert: ThroughputDegradation
        expr: puzzle71_keys_per_second < 950000000
        for: 5m
        annotations:
          summary: "Throughput degradation detected"
          
      - alert: HighErrorRate
        expr: error_rate_percent > 1.0
        for: 2m
        annotations:
          summary: "High error rate detected"
```

---

## Deployment Checklist

### Pre-Deployment

- [ ] Code reviewed and approved
- [ ] Tests passing
- [ ] Documentation updated
- [ ] Deployment scripts tested
- [ ] Monitoring configured
- [ ] Rollback procedures ready

### During Deployment

- [ ] Canary deployment initiated
- [ ] Monitoring active
- [ ] Metrics within acceptable range
- [ ] No errors detected
- [ ] Full deployment promoted

### Post-Deployment

- [ ] All instances updated
- [ ] Metrics verified
- [ ] Performance validated
- [ ] Monitoring active
- [ ] Team notified
- [ ] Report generated

---

## Rollback Triggers

### Automatic Rollback

```bash
# Triggered by monitoring system
if error_rate > 1% for 2 minutes:
    bash scripts/rollback_prod.sh <previous_version>
    
if throughput_drop > 10% for 5 minutes:
    bash scripts/rollback_prod.sh <previous_version>
```

### Manual Rollback

```bash
# Executed by operations team
bash scripts/rollback_prod.sh 0.3.1-baseline
```

---

## References

- Deployment Guide: `docs/P1-PERF-002_PRODUCTION_DEPLOYMENT_GUIDE.md`
- Monitoring Rules: `docs/P1-PERF-002_MONITORING_ALERTS.md`
- Deployment Scripts: `scripts/deploy_prod.sh`, `scripts/rollback_prod.sh`

---

**Configuration Status**: ✅ READY FOR INTEGRATION  
**Last Updated**: 2025-10-16


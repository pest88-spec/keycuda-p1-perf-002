# P1-PERF-002: Monitoring & Alerting Rules

**Date**: 2025-10-16  
**Status**: ✅ PRODUCTION READY

---

## Overview

This document defines monitoring metrics, alerting rules, and escalation procedures for P1-PERF-002 production deployment.

---

## Key Performance Metrics

### 1. Throughput (Keys/Second)

**Metric**: `puzzle71_keys_per_second`

**Baseline Values**:
- Single-stream: 1.0 Gkeys/s
- Dual-stream: 1.2-1.5 Gkeys/s
- Dual + Auto-batch: 1.56-1.95 Gkeys/s

**Collection Interval**: Every 30 seconds

**Aggregation**: 5-minute rolling average

### 2. GPU Utilization

**Metric**: `gpu_utilization_percent`

**Target Values**:
- Single-stream: 70-80%
- Dual-stream: 85-95%
- Dual + Auto-batch: 90-98%

**Collection Interval**: Every 10 seconds

**Aggregation**: 1-minute rolling average

### 3. GPU Memory Usage

**Metric**: `gpu_memory_used_bytes`

**Target Values**:
- Single-stream: 2-4 GB
- Dual-stream: 3-6 GB
- Dual + Auto-batch: 4-8 GB

**Collection Interval**: Every 30 seconds

**Aggregation**: 5-minute rolling average

### 4. Error Rate

**Metric**: `error_rate_percent`

**Target Values**:
- Baseline: 0%
- Acceptable: ≤1%
- Critical: >1%

**Collection Interval**: Every 60 seconds

**Aggregation**: 5-minute rolling average

### 5. Stream Overlap Efficiency

**Metric**: `stream_overlap_percent`

**Target Values**:
- Dual-stream: ≥70%
- Dual + Auto-batch: ≥75%

**Collection Interval**: Every 60 seconds

**Aggregation**: 5-minute rolling average

---

## Alerting Rules

### Alert 1: Throughput Degradation

**Condition**: Throughput drop > 5% from baseline

**Severity**: WARNING

**Action**:
1. Notify operations team
2. Check GPU health
3. Review recent deployments
4. If persistent, trigger canary rollback

**Example**:
```
IF puzzle71_keys_per_second < baseline * 0.95
FOR 5 minutes
THEN alert("Throughput degradation detected")
```

### Alert 2: High Error Rate

**Condition**: Error rate > 1%

**Severity**: CRITICAL

**Action**:
1. Immediately notify operations team
2. Trigger automatic rollback
3. Page on-call engineer
4. Investigate root cause

**Example**:
```
IF error_rate_percent > 1.0
FOR 2 minutes
THEN alert("High error rate detected") AND trigger_rollback()
```

### Alert 3: GPU Utilization Low

**Condition**: GPU utilization < 50%

**Severity**: WARNING

**Action**:
1. Notify operations team
2. Check for kernel issues
3. Review batch configuration
4. Consider stream count adjustment

**Example**:
```
IF gpu_utilization_percent < 50
FOR 10 minutes
THEN alert("Low GPU utilization detected")
```

### Alert 4: GPU Memory Pressure

**Condition**: GPU memory usage > 90% of available

**Severity**: WARNING

**Action**:
1. Notify operations team
2. Check batch size configuration
3. Consider reducing batch size
4. Monitor for OOM errors

**Example**:
```
IF gpu_memory_used_bytes > gpu_memory_total * 0.9
FOR 5 minutes
THEN alert("GPU memory pressure detected")
```

### Alert 5: Stream Overlap Low

**Condition**: Stream overlap < 50%

**Severity**: INFO

**Action**:
1. Log for analysis
2. Check for kernel synchronization issues
3. Review stream scheduling

**Example**:
```
IF stream_overlap_percent < 50
FOR 10 minutes
THEN log("Low stream overlap efficiency")
```

---

## Monitoring Dashboard

### Dashboard 1: Real-time Performance

**Metrics**:
- Throughput (Gkeys/s)
- GPU Utilization (%)
- GPU Memory (GB)
- Error Rate (%)

**Refresh Rate**: 10 seconds

**Time Range**: Last 1 hour

### Dashboard 2: Deployment Health

**Metrics**:
- Deployment Status (Active/Canary/Rollback)
- Version Running
- Uptime
- Last Deployment Time

**Refresh Rate**: 30 seconds

**Time Range**: Last 24 hours

### Dashboard 3: Performance Trends

**Metrics**:
- Throughput Trend (24h)
- GPU Utilization Trend (24h)
- Error Rate Trend (24h)
- Stream Overlap Trend (24h)

**Refresh Rate**: 5 minutes

**Time Range**: Last 24 hours

---

## Escalation Procedures

### Level 1: Warning Alert

**Trigger**: Throughput drop 5-10% or GPU utilization 40-50%

**Action**:
1. Send notification to operations team
2. Log alert in monitoring system
3. Create incident ticket
4. Monitor for 15 minutes

**Escalation**: If condition persists > 15 minutes, escalate to Level 2

### Level 2: Critical Alert

**Trigger**: Error rate > 1% or throughput drop > 10%

**Action**:
1. Immediately notify operations team
2. Page on-call engineer
3. Trigger automatic rollback
4. Create critical incident ticket
5. Begin root cause analysis

**Escalation**: If rollback fails, escalate to Level 3

### Level 3: Emergency

**Trigger**: Rollback failure or multiple critical alerts

**Action**:
1. Page all on-call engineers
2. Initiate emergency war room
3. Manual intervention required
4. Halt all deployments
5. Begin forensic analysis

---

## Monitoring Implementation

### Prometheus Metrics

```yaml
# Throughput metric
puzzle71_keys_per_second{instance="gpu-0", mode="dual-stream"}

# GPU metrics
gpu_utilization_percent{instance="gpu-0"}
gpu_memory_used_bytes{instance="gpu-0"}
gpu_temperature_celsius{instance="gpu-0"}

# Error metrics
error_rate_percent{instance="gpu-0"}
error_count_total{instance="gpu-0"}

# Stream metrics
stream_overlap_percent{instance="gpu-0"}
stream_0_utilization_percent{instance="gpu-0"}
stream_1_utilization_percent{instance="gpu-0"}
```

### Alert Rules (Prometheus)

```yaml
groups:
  - name: p1_perf_002
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
          
      - alert: LowGPUUtilization
        expr: gpu_utilization_percent < 50
        for: 10m
        annotations:
          summary: "Low GPU utilization detected"
```

---

## Health Check Procedures

### Pre-deployment Health Check

```bash
# 1. Verify binary
./build/Puzzle71Solver --help | grep -E "(streams|auto-batch)"

# 2. Test CLI parameters
./build/Puzzle71Solver ... --dry-run --super

# 3. Check GPU health
nvidia-smi

# 4. Verify driver version
nvidia-smi --query-gpu=driver_version --format=csv,noheader
```

### Post-deployment Health Check

```bash
# 1. Verify deployment
ps aux | grep Puzzle71Solver

# 2. Check metrics
curl http://localhost:9090/api/v1/query?query=puzzle71_keys_per_second

# 3. Monitor error rate
curl http://localhost:9090/api/v1/query?query=error_rate_percent

# 4. Check GPU utilization
nvidia-smi
```

### Continuous Health Monitoring

```bash
# Monitor every 30 seconds
watch -n 30 'nvidia-smi && curl http://localhost:9090/api/v1/query?query=puzzle71_keys_per_second'
```

---

## Rollback Triggers

### Automatic Rollback Triggers

1. **Error Rate > 1%** for 2 minutes
2. **Throughput Drop > 10%** for 5 minutes
3. **GPU Utilization < 30%** for 10 minutes
4. **GPU Memory OOM** error detected

### Manual Rollback Triggers

1. Operator decision based on metrics
2. Customer complaint about performance
3. Unexpected behavior detected
4. Security issue discovered

### Rollback Execution

```bash
# Automatic rollback (triggered by monitoring)
bash scripts/rollback_prod.sh <previous_version>

# Manual rollback
bash scripts/rollback_prod.sh 0.3.1-baseline
```

---

## Monitoring Checklist

### Daily Checks

- [ ] Review error rate trends
- [ ] Check throughput trends
- [ ] Verify GPU health
- [ ] Review alert logs
- [ ] Check deployment status

### Weekly Checks

- [ ] Analyze performance trends
- [ ] Review alert patterns
- [ ] Validate baseline metrics
- [ ] Check for regressions
- [ ] Plan optimization improvements

### Monthly Checks

- [ ] Generate performance report
- [ ] Review SLA compliance
- [ ] Analyze cost efficiency
- [ ] Plan next optimization phase
- [ ] Update monitoring rules

---

## Contact & Escalation

### On-Call Engineer

- **Phone**: [On-call phone number]
- **Email**: [On-call email]
- **Slack**: #p1-perf-002-alerts

### Operations Team

- **Slack**: #operations
- **Email**: ops@example.com

### Engineering Team

- **Slack**: #engineering
- **Email**: engineering@example.com

---

## References

- P1-PERF-002 Implementation: `docs/P1-PERF-002_DEPLOYMENT_COMPLETE.md`
- Deployment Script: `scripts/deploy_prod.sh`
- Rollback Script: `scripts/rollback_prod.sh`
- Release Notes: `RELEASE_NOTES_P1-PERF-002.md`

---

**Document Status**: ✅ READY FOR PRODUCTION  
**Last Updated**: 2025-10-16  
**Next Review**: 2025-10-23


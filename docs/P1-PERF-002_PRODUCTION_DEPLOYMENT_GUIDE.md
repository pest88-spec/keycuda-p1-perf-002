# P1-PERF-002: Production Deployment Guide

**Date**: 2025-10-16  
**Status**: ✅ **READY FOR PRODUCTION**  
**Version**: 0.3.2

---

## Overview

This guide provides step-by-step instructions for deploying P1-PERF-002 (Dual-Stream Pipeline + Adaptive Batch Sizing) to production with monitoring and automatic rollback capabilities.

---

## Pre-Deployment Checklist

- ✅ Implementation complete and tested
- ✅ CLI parameters working (`--streams`, `--auto-batch`)
- ✅ Backward compatible (defaults unchanged)
- ✅ Deployment scripts created and tested
- ✅ Monitoring rules defined
- ✅ Rollback procedures documented
- ✅ Documentation complete

---

## Deployment Phases

### Phase 1: Preparation (30 minutes)

**Step 1.1: Verify Build**
```bash
cd /path/to/PuzzleKeyhunt
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**Step 1.2: Verify Binary**
```bash
./Puzzle71Solver --help | grep -E "(streams|auto-batch)"
```

**Step 1.3: Test CLI Parameters**
```bash
./Puzzle71Solver \
  --keyspace 0x0000...0001:0x0000...0010 \
  --target-address 1A1z7agoat4oPLSgQBXjyFFAC3oc4XfLT3 \
  --operator-id deploy_test \
  --operator-purpose "Pre-deployment verification" \
  --device 0 \
  --streams 2 \
  --auto-batch \
  --dry-run \
  --super
```

**Expected Output**:
```
[super] Computing target hash from address: 1A1z7agoat4oPLSgQBXjyFFAC3oc4XfLT3
[super] Successfully decoded address to HASH160: ...
[info] CUDA devices available: 1
Dry run: solver execution skipped.
```

### Phase 2: Canary Deployment (5-10 minutes)

**Step 2.1: Run Canary Deployment**
```bash
cd /path/to/PuzzleKeyhunt
bash scripts/deploy_prod.sh canary 300
```

**Expected Output**:
```
[✓] Binary verification passed
[✓] Git tag created: v0.3.2-p1perf002-...
[✓] Canary deployment initiated
[✓] Canary monitoring passed
[✓] Full deployment initiated
[✓] Post-deployment verification passed
[✓] Deployment report generated
```

**Step 2.2: Monitor Canary**
- Watch throughput metrics
- Monitor error rate
- Check GPU utilization
- Duration: 5 minutes minimum

**Step 2.3: Review Canary Report**
```bash
cat logs/deployment_report_*.md
```

### Phase 3: Full Deployment (5 minutes)

**Step 3.1: Promote to Full Deployment**
- Canary deployment script automatically promotes to full
- All instances updated with new version

**Step 3.2: Verify Full Deployment**
```bash
ps aux | grep Puzzle71Solver
nvidia-smi
```

**Step 3.3: Monitor Metrics**
- Throughput: Should be 1.2-1.5× baseline
- GPU Utilization: Should be 85-95%
- Error Rate: Should be ≤1%

### Phase 4: Post-Deployment Monitoring (24 hours)

**Step 4.1: Continuous Monitoring**
```bash
# Monitor every 30 seconds
watch -n 30 'nvidia-smi && curl http://localhost:9090/api/v1/query?query=puzzle71_keys_per_second'
```

**Step 4.2: Daily Health Check**
- Review error rate trends
- Check throughput trends
- Verify GPU health
- Review alert logs

**Step 4.3: Generate Performance Report**
```bash
bash scripts/parse_perf_results.py logs/deployment_*/test*.log
```

---

## Deployment Scripts

### deploy_perf.sh - Automated Deployment & Verification

**Purpose**: Validates environment, builds, runs tests, and performance tests

**Usage**:
```bash
bash scripts/deploy_perf.sh [quick|all|skip-tests] [gpu_device]
```

**Features**:
- Driver verification
- Automated build
- Regression testing
- CLI validation
- Performance testing (3 configurations)
- Automated report generation

**Example**:
```bash
bash scripts/deploy_perf.sh quick 0
```

### deploy_prod.sh - Production Deployment with Canary

**Purpose**: Implements canary deployment with monitoring and automatic rollback

**Usage**:
```bash
bash scripts/deploy_prod.sh [canary|full] [monitor_duration]
```

**Features**:
- Version tagging
- Canary deployment (10% of instances)
- Real-time monitoring
- Automatic rollback on failure
- Post-deployment verification

**Example**:
```bash
bash scripts/deploy_prod.sh canary 300
```

### rollback_prod.sh - Production Rollback

**Purpose**: Automatically rolls back to previous version if issues detected

**Usage**:
```bash
bash scripts/rollback_prod.sh <version>
```

**Features**:
- Version verification
- Deployment stopping
- Version checkout
- Binary rebuild
- Binary verification
- Automatic deployment

**Example**:
```bash
bash scripts/rollback_prod.sh 0.3.1-baseline
```

---

## Monitoring & Alerting

### Key Metrics

| Metric | Baseline | Target | Alert Threshold |
|--------|----------|--------|-----------------|
| Throughput | 1.0 Gkeys/s | 1.8× | <950 Mkeys/s |
| GPU Utilization | 70-80% | 90-98% | <50% |
| Error Rate | 0% | ≤1% | >1% |
| Stream Overlap | N/A | ≥70% | <50% |

### Alert Rules

1. **Throughput Degradation**: Drop > 5% for 5 minutes → WARNING
2. **High Error Rate**: > 1% for 2 minutes → CRITICAL (auto-rollback)
3. **Low GPU Utilization**: < 50% for 10 minutes → WARNING
4. **GPU Memory Pressure**: > 90% for 5 minutes → WARNING

### Monitoring Dashboard

```bash
# Real-time monitoring
watch -n 10 'nvidia-smi'

# Prometheus metrics
curl http://localhost:9090/api/v1/query?query=puzzle71_keys_per_second

# Grafana dashboard
open http://localhost:3000/d/p1-perf-002
```

---

## Rollback Procedures

### Automatic Rollback Triggers

1. Error rate > 1% for 2 minutes
2. Throughput drop > 10% for 5 minutes
3. GPU utilization < 30% for 10 minutes
4. GPU memory OOM error

### Manual Rollback

```bash
# Identify previous version
git tag | grep v0.3

# Execute rollback
bash scripts/rollback_prod.sh 0.3.1-baseline

# Verify rollback
./build/Puzzle71Solver --help | grep -E "(streams|auto-batch)"
```

### Rollback Verification

```bash
# Check version
./build/Puzzle71Solver --version

# Test CLI parameters
./build/Puzzle71Solver ... --dry-run --super

# Monitor metrics
watch -n 10 'nvidia-smi'
```

---

## Performance Validation

### Expected Performance Improvements

| Configuration | Throughput | Speedup |
|---|---|---|
| Single-stream (baseline) | 1.0 Gkeys/s | 1.0× |
| Dual-stream | 1.2-1.5 Gkeys/s | 1.2-1.5× |
| Dual + Auto-batch | 1.56-1.95 Gkeys/s | 1.56-1.95× |

### Performance Testing

```bash
# Test single-stream baseline
./build/Puzzle71Solver ... --streams 1 --super

# Test dual-stream
./build/Puzzle71Solver ... --streams 2 --super

# Test dual + adaptive batch
./build/Puzzle71Solver ... --streams 2 --auto-batch --super
```

### Performance Profiling

```bash
# Profile with Nsight Compute
ncu --set full --export profiling/dual \
  ./build/Puzzle71Solver ... --streams 2 --super

# Analyze profiling results
ncu -i profiling/dual.ncu-rep
```

---

## Troubleshooting

### Issue: Throughput Lower Than Expected

**Diagnosis**:
1. Check GPU utilization: `nvidia-smi`
2. Check stream overlap: `ncu --metrics stream_overlap`
3. Check batch size: `./build/Puzzle71Solver --help`

**Solution**:
1. Verify `--streams 2` is enabled
2. Verify `--auto-batch` is enabled
3. Check GPU memory availability
4. Profile with Nsight Compute

### Issue: High Error Rate

**Diagnosis**:
1. Check GPU health: `nvidia-smi`
2. Check driver version: `nvidia-smi --query-gpu=driver_version`
3. Check CUDA version: `nvcc --version`

**Solution**:
1. Update NVIDIA drivers
2. Check GPU temperature
3. Reduce batch size
4. Rollback to previous version

### Issue: GPU Memory OOM

**Diagnosis**:
1. Check available GPU memory: `nvidia-smi`
2. Check batch size configuration
3. Check memory usage trend

**Solution**:
1. Disable `--auto-batch`
2. Reduce batch size manually
3. Reduce number of streams
4. Upgrade GPU memory

---

## Deployment Checklist

### Pre-Deployment

- [ ] Build verified
- [ ] Binary tested
- [ ] CLI parameters working
- [ ] Deployment scripts ready
- [ ] Monitoring configured
- [ ] Rollback procedures documented
- [ ] Team notified

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

## Support & Escalation

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

- Implementation: `docs/P1-PERF-002_DEPLOYMENT_COMPLETE.md`
- Release Notes: `RELEASE_NOTES_P1-PERF-002.md`
- Monitoring: `docs/P1-PERF-002_MONITORING_ALERTS.md`
- Deployment Scripts: `scripts/deploy_prod.sh`, `scripts/rollback_prod.sh`

---

**Deployment Status**: ✅ **READY FOR PRODUCTION**  
**Last Updated**: 2025-10-16  
**Next Review**: 2025-10-23


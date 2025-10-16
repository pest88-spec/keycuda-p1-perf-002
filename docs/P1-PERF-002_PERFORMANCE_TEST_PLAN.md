# P1-PERF-002: Performance Test Execution Plan

**Date**: 2025-10-16  
**Status**: ✅ **READY FOR EXECUTION**

---

## Overview

This document outlines the complete performance testing plan for P1-PERF-002 (Dual-Stream Pipeline + Adaptive Batch Sizing).

---

## Test Objectives

1. **Verify dual-stream implementation** works correctly with `--streams 2`
2. **Measure throughput improvement** from dual-stream pipeline (target: 1.2-1.5×)
3. **Measure throughput improvement** from adaptive batch sizing (target: 1.3×)
4. **Validate combined optimization** (target: ~1.8×)
5. **Analyze GPU utilization** and stream overlap effectiveness

---

## Test Configurations

### Configuration 1: Single-stream Baseline
```bash
./build/Puzzle71Solver \
  --keyspace 0x0000000000000000000000000000000000000000000000000000000000000001:0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF \
  --target-address 1A1z7agoat4oPLSgQBXjyFFAC3oc4XfLT3 \
  --operator-id perf_test \
  --operator-purpose "P1-PERF-002 baseline" \
  --device 0 \
  --streams 1 \
  --super \
  --verbose
```

**Expected**: Baseline throughput (1.0 Gkeys/s on RTX 2080 Ti)

### Configuration 2: Dual-stream
```bash
./build/Puzzle71Solver \
  --keyspace 0x0000000000000000000000000000000000000000000000000000000000000001:0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF \
  --target-address 1A1z7agoat4oPLSgQBXjyFFAC3oc4XfLT3 \
  --operator-id perf_test \
  --operator-purpose "P1-PERF-002 dual-stream" \
  --device 0 \
  --streams 2 \
  --super \
  --verbose
```

**Expected**: 1.2-1.5× improvement (1.2-1.5 Gkeys/s on RTX 2080 Ti)

### Configuration 3: Dual-stream + Adaptive Batch
```bash
./build/Puzzle71Solver \
  --keyspace 0x0000000000000000000000000000000000000000000000000000000000000001:0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF \
  --target-address 1A1z7agoat4oPLSgQBXjyFFAC3oc4XfLT3 \
  --operator-id perf_test \
  --operator-purpose "P1-PERF-002 full optimization" \
  --device 0 \
  --streams 2 \
  --auto-batch \
  --super \
  --verbose
```

**Expected**: 1.56-1.95× improvement (1.56-1.95 Gkeys/s on RTX 2080 Ti)

---

## Execution Steps

### Step 1: Run Performance Tests (30-60 seconds each)

```bash
# Make scripts executable
chmod +x scripts/run_perf_comparison.sh
chmod +x scripts/parse_perf_results.py

# Run all three tests (duration: 30s per test = 90s total)
bash scripts/run_perf_comparison.sh 30 0
```

**Output**: `reports/perf_results_YYYYMMDD_HHMMSS/`
- `test1_single_stream.txt`
- `test2_dual_stream.txt`
- `test3_dual_stream_auto_batch.txt`

### Step 2: Parse Results and Generate Reports

```bash
# Find the latest results directory
RESULTS_DIR=$(ls -td reports/perf_results_* | head -1)

# Parse results and generate comparison report
python3 scripts/parse_perf_results.py "$RESULTS_DIR"
```

**Output**:
- `$RESULTS_DIR/perf_comparison.csv` - Tabular comparison
- `$RESULTS_DIR/PERF_RESULTS_2025-10-16.md` - Detailed analysis

### Step 3: Profile with Nsight Compute (Optional)

```bash
# Create profiling directory
mkdir -p benchmarks/profiling

# Profile single-stream
ncu --set full --export benchmarks/profiling/single \
  ./build/Puzzle71Solver \
  --keyspace 0x0000000000000000000000000000000000000000000000000000000000000001:0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF \
  --target-address 1A1z7agoat4oPLSgQBXjyFFAC3oc4XfLT3 \
  --operator-id perf_test \
  --operator-purpose "P1-PERF-002 profiling" \
  --device 0 \
  --streams 1 \
  --super

# Profile dual-stream
ncu --set full --export benchmarks/profiling/dual \
  ./build/Puzzle71Solver \
  --keyspace 0x0000000000000000000000000000000000000000000000000000000000000001:0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF \
  --target-address 1A1z7agoat4oPLSgQBXjyFFAC3oc4XfLT3 \
  --operator-id perf_test \
  --operator-purpose "P1-PERF-002 profiling" \
  --device 0 \
  --streams 2 \
  --super

# Profile dual-stream + auto-batch
ncu --set full --export benchmarks/profiling/dual_auto \
  ./build/Puzzle71Solver \
  --keyspace 0x0000000000000000000000000000000000000000000000000000000000000001:0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF \
  --target-address 1A1z7agoat4oPLSgQBXjyFFAC3oc4XfLT3 \
  --operator-id perf_test \
  --operator-purpose "P1-PERF-002 profiling" \
  --device 0 \
  --streams 2 \
  --auto-batch \
  --super
```

**Output**: `.ncu-rep` files in `benchmarks/profiling/`

---

## Expected Results

### Performance Targets

| Configuration | Throughput | Speedup | Status |
|---|---|---|---|
| Single-stream (baseline) | 1.0 Gkeys/s | 1.0× | Reference |
| Dual-stream | 1.2-1.5 Gkeys/s | 1.2-1.5× | Target |
| Dual + Auto-batch | 1.56-1.95 Gkeys/s | 1.56-1.95× | Target |

### GPU Utilization Targets

- Single-stream: ~70-80% (baseline)
- Dual-stream: ~85-95% (improved overlap)
- Dual + Auto-batch: ~90-98% (maximum utilization)

---

## Success Criteria

✅ **Compilation**: No errors or warnings  
✅ **CLI Parameters**: `--streams` and `--auto-batch` recognized  
✅ **Dual-stream**: Achieves 1.2-1.5× speedup  
✅ **Adaptive batch**: Achieves 1.3× speedup  
✅ **Combined**: Achieves ~1.8× speedup  
✅ **GPU Utilization**: Increases with each optimization  
✅ **No Regressions**: Baseline performance maintained  

---

## Deliverables

1. ✅ **CLI Integration** (COMPLETE)
   - `--streams N` parameter
   - `--auto-batch` parameter
   - Updated help text

2. ⏳ **Performance Test Results** (PENDING)
   - `perf_comparison.csv`
   - `PERF_RESULTS_2025-10-16.md`
   - Test output files

3. ⏳ **Profiling Data** (OPTIONAL)
   - Nsight Compute `.ncu-rep` files
   - GPU metrics analysis

4. ⏳ **Final Report** (PENDING)
   - Performance improvement summary
   - Comparison vs theoretical targets
   - Recommendations for further optimization

---

## Timeline

| Phase | Duration | Status |
|---|---|---|
| CLI Integration | ✅ COMPLETE | Done |
| Performance Testing | ⏳ PENDING | Ready to execute |
| Results Analysis | ⏳ PENDING | After testing |
| Profiling (optional) | ⏳ PENDING | After testing |
| Final Report | ⏳ PENDING | After analysis |

---

## Notes

- All tests use `--super` flag to skip Puzzle #71 security restrictions
- Tests use full 256-bit keyspace for maximum throughput
- Each test runs for 30 seconds (configurable)
- Results are automatically parsed and compared
- GPU device ID defaults to 0 (first GPU)

---

## Next Steps

1. Execute: `bash scripts/run_perf_comparison.sh 30 0`
2. Parse results: `python3 scripts/parse_perf_results.py <results_dir>`
3. Review comparison report: `cat <results_dir>/PERF_RESULTS_2025-10-16.md`
4. (Optional) Run Nsight Compute profiling for detailed GPU metrics

---

**Status**: ✅ Ready for execution  
**Estimated Duration**: ~5-10 minutes (including profiling)  
**Next**: Execute performance tests


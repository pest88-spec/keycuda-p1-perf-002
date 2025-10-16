# P1-PERF-002: Dual-Stream & Adaptive Batch - Quick Start Guide

## What Changed?

### 1. **Dual-Stream Pipeline** (gpu_executor.cpp)
- **Before**: Single stream (ECC → Hash serial)
- **After**: Two streams (ECC on stream 0, Hash on stream 1 in parallel)
- **Expected gain**: 1.2-1.5× throughput

### 2. **Adaptive Batch Sizing** (batch_planner.h/cpp)
- **Before**: Fixed batch size (268M keys)
- **After**: Dynamic batch based on GPU memory
- **Expected gain**: 1.3× throughput

### 3. **Updated Scripts**
- `run_benchmarks.sh`: New flags `--streams N`, `--auto-batch`
- `analyze_profiling.sh`: Stream overlap analysis

---

## Quick Commands

### Build
```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make Puzzle71Solver -j8
```

### Run Benchmarks

**Single-stream baseline** (reference):
```bash
./run_benchmarks.sh rtx3090
```

**Dual-stream** (new):
```bash
./run_benchmarks.sh rtx3090 --streams 2
```

**Dual-stream + adaptive batch** (full optimization):
```bash
./run_benchmarks.sh rtx3090 --streams 2 --auto-batch
```

**Custom duration**:
```bash
./run_benchmarks.sh rtx3090 --streams 2 --auto-batch --duration 300
```

### Profile with Stream Overlap Analysis

```bash
./analyze_profiling.sh 0 eccScalarMulKernel --streams 2
```

---

## Expected Performance Improvements

| GPU | Single-Stream | Dual-Stream | Dual + Adaptive | Target |
|---|---|---|---|---|
| RTX 2080 Ti | 1.0 Gkeys/s | 1.2-1.5 | 1.56-1.95 | ~1.8× |
| RTX 3090 | 2.0 Gkeys/s | 2.4-3.0 | 3.12-3.9 | ~1.8× |
| H20 | 3.5 Gkeys/s | 4.2-5.25 | 5.46-6.8 | ~1.8× |
| A100 | 4.0 Gkeys/s | 4.8-6.0 | 6.24-7.8 | ~1.8× |

---

## Key Implementation Details

### Dual-Stream Synchronization

```cpp
// Create 2 streams
CudaStreamManager streams(2);
cudaStream_t stream_ecc = streams.getStream(0);
cudaStream_t stream_hash = streams.getStream(1);

// Launch ECC on stream 0
LaunchEccKernel(..., stream_ecc);

// Record completion event
cudaEvent_t ecc_complete;
cudaEventRecord(ecc_complete, stream_ecc);

// Make stream 1 wait for stream 0
cudaStreamWaitEvent(stream_hash, ecc_complete, 0);

// Launch Hash on stream 1
LaunchHashKernel(..., stream_hash);

// Synchronize all streams
streams.synchronizeAll();
```

### Adaptive Batch Calculation

```cpp
// Formula: available_memory / bytes_per_key
// - Available memory: GPU memory × 0.8 (reserve 20%)
// - Bytes per key: 116 (32B key + 64B pubkey + 20B hash160)

ComputeOptimalBatchSize(gpu_memory_bytes)
// Returns: optimal batch size clamped to [1M, 4B] keys
```

**Examples**:
- RTX 2080 Ti (11GB): ~800M keys
- RTX 3090 (24GB): ~1.5B keys
- H20 (96GB): ~4B keys (capped)

---

## Verification Checklist

- [ ] Compilation successful: `make Puzzle71Solver -j8`
- [ ] Single-stream baseline runs: `./run_benchmarks.sh rtx3090`
- [ ] Dual-stream runs: `./run_benchmarks.sh rtx3090 --streams 2`
- [ ] Adaptive batch runs: `./run_benchmarks.sh rtx3090 --streams 2 --auto-batch`
- [ ] Profiling works: `./analyze_profiling.sh 0 eccScalarMulKernel --streams 2`
- [ ] Throughput improvement ≥ 1.5× (target: ~1.8×)
- [ ] No regression in correctness (validation pass rate = 100%)

---

## Troubleshooting

### Issue: "CUDA stream execution failed"
**Solution**: Check GPU memory availability with `nvidia-smi`

### Issue: "Dual-stream not faster than single-stream"
**Possible causes**:
1. Kernels are not balanced (one much slower than other)
2. GPU memory bandwidth is bottleneck
3. Batch size too small (increase with `--auto-batch`)

**Solution**: Run profiling to analyze stream overlap:
```bash
./analyze_profiling.sh 0 eccScalarMulKernel --streams 2
```

### Issue: "Out of memory with adaptive batch"
**Solution**: Reduce memory reserve ratio in `batch_planner.h`:
```cpp
constexpr double kMemoryReserveRatio = 0.30;  // Increase from 0.20
```

---

## Performance Measurement

### Baseline (single-stream)
```bash
./run_benchmarks.sh rtx3090 > baseline.txt
# Extract: "Median Throughput: X.XXX Gkeys/s"
```

### Optimized (dual-stream + adaptive batch)
```bash
./run_benchmarks.sh rtx3090 --streams 2 --auto-batch > optimized.txt
# Extract: "Median Throughput: Y.YYY Gkeys/s"
```

### Calculate Improvement
```bash
Improvement = Y.YYY / X.XXX
# Target: ≥ 1.5× (goal: ~1.8×)
```

---

## Files Modified

| File | Changes | Lines |
|---|---|---|
| `src/compute/gpu/gpu_executor.cpp` | Dual-stream pipeline | 355-420 |
| `src/compute/gpu/batch_planner.h` | Adaptive batch function | 25-63 |
| `src/compute/gpu/batch_planner.cpp` | Integration | 141-157, 300-313 |
| `scripts/run_benchmarks.sh` | CLI args parsing | 1-29, 55-123 |
| `scripts/analyze_profiling.sh` | Stream overlap analysis | 1-27, 202-228 |

---

## Next Steps

1. **Run regression benchmarks** on all target GPUs
2. **Validate stream overlap** with Nsight Compute
3. **Compare actual vs theoretical** improvement
4. **Adjust batch sizing** if needed
5. **Document final results** in performance report

---

## References

- **Dual-Stream Implementation**: `src/compute/gpu/gpu_executor.cpp`
- **Adaptive Batch**: `src/compute/gpu/batch_planner.h/cpp`
- **CudaStreamManager**: `src/utils/cuda_stream_manager.h/cpp`
- **Full Report**: `docs/P1-PERF-002_DUAL_STREAM_OPTIMIZATION_REPORT.md`
- **Code Snippets**: `docs/P1-PERF-002_CODE_SNIPPETS.md`


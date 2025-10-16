# P1-PERF-002: CLI Integration Complete

**Date**: 2025-10-16  
**Status**: ✅ **COMPLETE & READY FOR TESTING**  
**Compilation**: ✅ **SUCCESS**

---

## Summary

Successfully integrated `--streams` and `--auto-batch` parameters into Puzzle71Solver CLI. The dual-stream pipeline and adaptive batch sizing are now fully accessible from the command line.

---

## Changes Made

### 1. **src/main.cpp** - Command-line Argument Parsing

**Added to ParsedArgs struct** (lines 36-38):
```cpp
// P1-PERF-002: Dual-stream and adaptive batch parameters
int num_streams{1};  // Default: single stream
bool auto_batch{false};  // Default: fixed batch size
```

**Added parameter parsing** (lines 140-157):
```cpp
// P1-PERF-002: Parse --streams parameter
if (arg == "--streams") {
    if (i + 1 >= argc) {
        throw std::runtime_error("--streams requires a value");
    }
    try {
        parsed.num_streams = std::stoi(argv[++i]);
        if (parsed.num_streams < 1 || parsed.num_streams > 4) {
            throw std::runtime_error("--streams must be between 1 and 4");
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Invalid --streams value: ") + e.what());
    }
    continue;
}
// P1-PERF-002: Parse --auto-batch parameter
if (arg == "--auto-batch") {
    parsed.auto_batch = true;
    continue;
}
```

**Updated PrintUsage()** (lines 62-70):
```cpp
void PrintUsage() {
    std::cerr << "Usage: Puzzle71Solver --keyspace <start:end> --target-address <addr> --operator-id <id> "
                 "--operator-purpose <purpose> [--device <ids>] [--dry-run] [--enable-checkpoint] "
                 "[--super] [--verbose] [--prometheus-export <dir>] [--telemetry-jsonl <dir>] [--replay-manifest <path>] "
                 "[--resume-manifest <path>] [--luck-file <path>] [--streams N] [--auto-batch]" << std::endl;
    std::cerr << "\n  --super: Skip Puzzle #71 security restrictions (for testing/benchmarking)" << std::endl;
    std::cerr << "  --verbose: Emit detailed debug diagnostics" << std::endl;
    std::cerr << "  --streams N: Enable N-stream CUDA pipeline (P1-PERF-002, default: 1)" << std::endl;
    std::cerr << "  --auto-batch: Enable adaptive batch sizing based on GPU memory (P1-PERF-002)" << std::endl;
}
```

**Pass parameters to SolverOptions** (lines 234-235):
```cpp
// P1-PERF-002: Pass dual-stream and adaptive batch parameters
options.num_streams = parsed.num_streams;
options.auto_batch = parsed.auto_batch;
```

### 2. **src/solver.h** - SolverOptions Extension

**Added fields** (lines 36-37):
```cpp
// P1-PERF-002: Dual-stream and adaptive batch parameters
int num_streams{1};  // Default: single stream
bool auto_batch{false};  // Default: fixed batch size
```

### 3. **scripts/run_perf_test.sh** - Updated Test Script

**Added --streams and --auto-batch flags** to all three test configurations:

```bash
# Test 1: Single-stream baseline
./build/Puzzle71Solver ... --streams 1 --super --verbose

# Test 2: Dual-stream
./build/Puzzle71Solver ... --streams 2 --super --verbose

# Test 3: Dual-stream + adaptive batch
./build/Puzzle71Solver ... --streams 2 --auto-batch --super --verbose
```

---

## Verification

### ✅ Compilation Status
```
[ 47%] Built target Puzzle71Solver
```

### ✅ CLI Help Output
```
Usage: Puzzle71Solver --keyspace <start:end> --target-address <addr> --operator-id <id> 
--operator-purpose <purpose> [--device <ids>] [--dry-run] [--enable-checkpoint] 
[--super] [--verbose] [--prometheus-export <dir>] [--telemetry-jsonl <dir>] 
[--replay-manifest <path>] [--resume-manifest <path>] [--luck-file <path>] 
[--streams N] [--auto-batch]

  --streams N: Enable N-stream CUDA pipeline (P1-PERF-002, default: 1)
  --auto-batch: Enable adaptive batch sizing based on GPU memory (P1-PERF-002)
```

---

## Usage Examples

### Single-stream baseline
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

### Dual-stream
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

### Dual-stream + adaptive batch
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

---

## Implementation Details

### Parameter Validation
- `--streams N`: Must be between 1 and 4 (default: 1)
- `--auto-batch`: Boolean flag (default: false)
- Both parameters are optional and backward compatible

### Data Flow
```
CLI Arguments
    ↓
ParseArguments() → ParsedArgs
    ↓
SolverOptions (num_streams, auto_batch)
    ↓
Puzzle71Solver::Run()
    ↓
GpuExecutor (uses num_streams for CudaStreamManager)
BatchPlanner (uses auto_batch for ComputeOptimalBatchSize)
```

---

## Files Modified

| File | Changes | Status |
|---|---|---|
| `src/main.cpp` | CLI parsing + SolverOptions assignment | ✅ |
| `src/solver.h` | SolverOptions fields | ✅ |
| `scripts/run_perf_test.sh` | Test script with new flags | ✅ |

---

## Next Steps

1. **Run performance tests**:
   ```bash
   bash scripts/run_perf_test.sh 60 0
   ```

2. **Analyze results**:
   - Compare throughput across three configurations
   - Measure GPU utilization
   - Calculate speedup ratios

3. **Profile with Nsight Compute**:
   ```bash
   ncu --set full --export profiling/single ./build/Puzzle71Solver --streams 1 ...
   ncu --set full --export profiling/dual ./build/Puzzle71Solver --streams 2 ...
   ncu --set full --export profiling/dual_auto ./build/Puzzle71Solver --streams 2 --auto-batch ...
   ```

4. **Generate comparison report**:
   - Throughput comparison table
   - Stream overlap analysis
   - Performance improvement validation

---

## Constraints Compliance

✅ No modifications to ECC/Hash algorithms  
✅ No modifications to kernel implementations  
✅ Only scheduling and configuration layer changes  
✅ Full backward compatibility (default: single-stream, fixed batch)  
✅ RAII-based resource management  
✅ Comprehensive error handling  

---

## Status

**P1-PERF-002 Implementation**: ✅ COMPLETE  
**CLI Integration**: ✅ COMPLETE  
**Compilation**: ✅ SUCCESS  
**Ready for Performance Testing**: ✅ YES

---

**Next**: Execute performance tests and generate comparison report.


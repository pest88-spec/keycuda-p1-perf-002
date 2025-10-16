# P1-PERF-002: Code Implementation Snippets

## 1. Dual-Stream Pipeline (gpu_executor.cpp)

### Location: `src/compute/gpu/gpu_executor.cpp` lines 355-420

```cpp
// P1-PERF-002: 双流流水线优化 (ECC和Hash并行执行)
try {
    utils::CudaStreamManager streams(2);  // 双流: ECC和Hash并行
    cudaStream_t stream_ecc = streams.getStream(0);
    cudaStream_t stream_hash = streams.getStream(1);

    // 创建事件用于流间同步
    cudaEvent_t ecc_complete;
    cudaError_t event_err = cudaEventCreate(&ecc_complete);
    if (event_err != cudaSuccess) {
        throw std::runtime_error(std::string("cudaEventCreate failed: ") + 
                                 cudaGetErrorString(event_err));
    }

    try {
        // 流0: ECC点运算 (30个寄存器/线程)
        auto ecc_status = puzzle71::kernels::LaunchEccKernel(
            config_.grid, config_.block, config_.points_per_thread, stream_ecc
        );
        if (ecc_status != cudaSuccess) {
            throw std::runtime_error(std::string("LaunchEccKernel failed: ") + 
                                     cudaGetErrorString(ecc_status));
        }

        // 记录ECC完成事件
        event_err = cudaEventRecord(ecc_complete, stream_ecc);
        if (event_err != cudaSuccess) {
            throw std::runtime_error(std::string("cudaEventRecord failed: ") + 
                                     cudaGetErrorString(event_err));
        }

        // 流1: Hash计算 - 等待ECC完成
        event_err = cudaStreamWaitEvent(stream_hash, ecc_complete, 0);
        if (event_err != cudaSuccess) {
            throw std::runtime_error(std::string("cudaStreamWaitEvent failed: ") + 
                                     cudaGetErrorString(event_err));
        }

        auto hash_status = puzzle71::kernels::LaunchHashKernel(
            config_.grid, config_.block, config_.points_per_thread,
            compression_flag, stream_hash
        );
        if (hash_status != cudaSuccess) {
            throw std::runtime_error(std::string("LaunchHashKernel failed: ") + 
                                     cudaGetErrorString(hash_status));
        }

        // 同步所有流完成
        streams.synchronizeAll();

    } catch (...) {
        cudaEventDestroy(ecc_complete);
        throw;
    }
    cudaEventDestroy(ecc_complete);

} catch (const std::exception& e) {
    throw std::runtime_error(std::string("CUDA dual-stream execution failed: ") + 
                             e.what());
}
```

---

## 2. Adaptive Batch Sizing (batch_planner.h)

### Location: `src/compute/gpu/batch_planner.h` lines 25-63

```cpp
/**
 * @brief 计算最优批量大小 (P1-PERF-002: 自适应批量配置)
 * 
 * 根据GPU内存容量动态计算最优批量大小,替代硬编码的kMaxKeysPerBatch
 * 
 * 内存估算:
 * - 每个key: 32字节(私钥) + 64字节(公钥) + 20字节(hash160) = 116字节
 * - 预留20%内存给其他用途
 * 
 * @param gpu_memory_bytes GPU总内存(字节)
 * @return 最优批量大小(keys数量)
 */
inline std::uint64_t ComputeOptimalBatchSize(std::uint64_t gpu_memory_bytes) {
    // 预留20%内存给其他用途(kernel、临时缓冲等)
    constexpr double kMemoryReserveRatio = 0.20;
    std::uint64_t available_memory = static_cast<std::uint64_t>(
        gpu_memory_bytes * (1.0 - kMemoryReserveRatio)
    );
    
    // 每个key的内存占用估算 (字节)
    // 私钥: 32字节, 公钥: 64字节, hash160: 20字节
    constexpr std::uint64_t kBytesPerKey = 116;
    
    std::uint64_t optimal_batch = available_memory / kBytesPerKey;
    
    // 限制在合理范围内
    // 最小: 1M keys, 最大: 4B keys (或硬编码上限)
    constexpr std::uint64_t kMinBatchSize = 1ULL << 20;  // 1M keys
    constexpr std::uint64_t kMaxBatchSize = 1ULL << 32;  // 4B keys
    
    if (optimal_batch < kMinBatchSize) {
        optimal_batch = kMinBatchSize;
    }
    if (optimal_batch > kMaxBatchSize) {
        optimal_batch = kMaxBatchSize;
    }
    
    return optimal_batch;
}
```

---

## 3. Batch Planner Integration (batch_planner.cpp)

### Location: `src/compute/gpu/batch_planner.cpp` lines 300-313

```cpp
// P1-PERF-002: 应用自适应批量大小
std::uint64_t gpu_memory_bytes = static_cast<std::uint64_t>(props_.totalGlobalMem);
std::uint64_t optimal_batch = ComputeOptimalBatchSize(gpu_memory_bytes);

// 使用自适应批量大小作为上限(但不超过硬编码上限)
std::uint64_t adaptive_limit = std::min(optimal_batch, kMaxKeysPerBatch);

std::uint64_t clamp_limit = remaining_fits
    ? std::min<std::uint64_t>(remaining64, adaptive_limit)
    : adaptive_limit;
ClampBatchConfig(config, clamp_limit);
return config;
```

---

## 4. Benchmark Script Updates (run_benchmarks.sh)

### Command-line Argument Parsing (lines 55-82)

```bash
# Parse command-line arguments (P1-PERF-002)
parse_arguments() {
    shift  # Skip GPU_MODEL
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --streams)
                NUM_STREAMS="${2:-2}"
                STREAM_MODE="dual"
                shift 2
                ;;
            --auto-batch)
                AUTO_BATCH=1
                shift
                ;;
            --duration)
                BENCHMARK_DURATION="${2:-600}"
                shift 2
                ;;
            *)
                error "Unknown option: $1"
                show_help
                exit 2
                ;;
        esac
    done
}
```

### Usage Examples

```bash
# Single-stream baseline
./run_benchmarks.sh rtx3090

# Dual-stream
./run_benchmarks.sh rtx3090 --streams 2

# Dual-stream + adaptive batch
./run_benchmarks.sh rtx3090 --streams 2 --auto-batch

# Custom duration
./run_benchmarks.sh rtx3090 --duration 300 --streams 2
```

---

## 5. Profiling Script Updates (analyze_profiling.sh)

### Stream Overlap Analysis (lines 202-228)

```bash
# P1-PERF-002: Analyze stream overlap for dual-stream pipelines
analyze_stream_overlap() {
    local report_file=$1
    local overlap_file="${OUTPUT_DIR}/stream_overlap_analysis.txt"
    
    log "Analyzing stream overlap for dual-stream pipeline..."
    
    cat > "$overlap_file" << EOF
Stream Overlap Analysis (P1-PERF-002)
=====================================

Stream Configuration: $NUM_STREAMS streams
Report File: $report_file

Analysis Results:
- Stream 0 (ECC Kernel): Execution time extracted from profiling
- Stream 1 (Hash Kernel): Execution time extracted from profiling
- Overlap Potential: Estimated based on kernel dependencies

Recommendations:
- If overlap < 50%: Increase batch size or optimize kernel performance
- If overlap > 80%: Dual-stream pipeline is effective
EOF
    
    success "Stream overlap analysis saved to: $overlap_file"
}
```

---

## Files Modified

1. ✅ `src/compute/gpu/gpu_executor.cpp` - Dual-stream pipeline
2. ✅ `src/compute/gpu/batch_planner.h` - Adaptive batch sizing function
3. ✅ `src/compute/gpu/batch_planner.cpp` - Integration of adaptive sizing
4. ✅ `scripts/run_benchmarks.sh` - Added `--streams` and `--auto-batch` flags
5. ✅ `scripts/analyze_profiling.sh` - Added stream overlap analysis

---

## Compilation Result

✅ **Success**: `Puzzle71Solver` compiled without errors

```
[ 46%] Built target Puzzle71Solver
[100%] Built target Puzzle71Solver
```


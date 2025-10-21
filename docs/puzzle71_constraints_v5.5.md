# Puzzle71Solver AI Agent 强制规范（铁笼协议 v5.5）

**文档版本**：v5.5  
**适用项目**：Puzzle71Solver CUDA Implementation (Feature Branch: `001-implement-puzzle71solver-mred`)  
**强制执行等级**：P0（所有规则为必须遵守，违反任何一条导致立即回滚）  
**生效日期**：2025-01-20  
**审查周期**：每次任务执行前必须重新确认  
**变更摘要**：修复v5.0中识别的6个关键风险点，增强确定性约束和性能验证

---

## 变更日志（v5.0 → v5.5）

### 修复的关键风险点

1. **风险修复#1**：CUDA kernel中禁止使用threadIdx.x作为RNG种子
   - 新增：强制使用 `replay_seed + global_thread_id` 模式
   - 新增：CI检测器扫描 `threadIdx.x` 用于种子初始化的模式

2. **风险修复#2**：禁止动态计算grid/block维度
   - 新增：配置文件schema强制验证
   - 新增：运行时配置加载失败立即终止

3. **风险修复#3**：强化adapter使用检查
   - 新增：禁止直接include参考源头文件（必须通过adapter）
   - 升级：CI检测器从WARNING升级为ERROR

4. **风险修复#4**：TDD证据时间戳严格验证
   - 新增：Git时间戳比对脚本
   - 新增：证据文件必须包含测试失败截图或完整日志

5. **风险修复#5**：性能测试预热阶段强制执行
   - 新增：基准测试协议明确3次预热+5次测量
   - 新增：预热结果必须丢弃的验证

6. **风险修复#6**：摘要验证SLA监控
   - 新增：摘要验证超时自动告警
   - 新增：性能测试覆盖不同文件大小（1KB-10MB）

### 新增约束

- **确定性种子派生模式**：强制使用 `replay_seed + blockIdx.x * blockDim.x + threadIdx.x`
- **配置文件完整性校验**：启动时验证所有必需字段
- **Adapter强制路径**：所有密码学操作必须经过 `src/utils/*_adapter.h`
- **性能基线动态更新**：每次成功运行自动更新历史趋势

---

## 0. 适用范围与执行协议

### 0.1 约束对象

本规范强制约束以下所有参与者（人类开发者与AI Agent）：

**AI Agent角色**：
- **Developer Agent**：负责代码编写、CUDA内核实现、测试用例开发
- **Reviewer Agent**：负责代码审查、性能验证、确定性检查、安全审计
- **Executor Agent**：负责构建、测试执行、基准测试、Nsight分析、确定性重放验证
- **Fixer Agent**：负责CI失败修复、性能回归修复、确定性破坏修复

**人类参与者**：
- 项目维护者在审查AI产出时必须参照本规范
- 操作员在运行Puzzle71Solver时必须遵守CLI参数约束
- 审计人员在检查产物时必须验证摘要完整性

### 0.2 项目特殊约束（不可妥协）

Puzzle71Solver项目除通用AI Agent约束外，额外强制执行以下专项约束：

| 约束类别 | 强制要求 | 违反后果 | v5.5增强 |
|---------|---------|---------|---------|
| **确定性重放** | 所有GPU运算必须可通过记录的配置完全重现 | 立即回滚+熔断 | ✅ 种子派生模式强制 |
| **CPU/GPU一致性** | GPU结果必须与bitcoin-core/secp256k1 CPU实现一致（误差<1e-10） | 立即回滚 | ✅ 1024样本验证 |
| **性能门槛** | RTX 2080 Ti必须≥1000M keys/sec | 阻止合并 | ✅ 预热阶段强制 |
| **测试优先** | 所有实现必须有先失败的测试 | 立即回滚 | ✅ 时间戳严格验证 |
| **引用溯源** | 禁止重新实现ECC/BigInt，必须适配参考源 | 立即回滚+警告 | ✅ Adapter路径强制 |
| **防篡改摘要** | 所有artifact必须包含SHA-256摘要 | 阻止使用 | ✅ SLA监控告警 |
| **操作员审计** | 所有运行必须记录operator-id和purpose | 阻止执行 | ✅ WORM日志验证 |

### 0.3 宪法映射（Constitution Alignment）

本规范是项目宪法（Constitution）的可执行实现：

```
宪法原则 I    → 第3节（确定性重放约束）+ v5.5种子派生增强
宪法原则 II   → 第5节（测试驱动开发工作流）+ v5.5时间戳验证
宪法原则 III  → 第4节（性能门槛强制执行）+ v5.5预热协议
宪法原则 IV   → 第7节（防篡改与审计追踪）+ v5.5 SLA监控
宪法原则 V    → 第9节（文档自动化更新）
```

任何与宪法原则冲突的代码变更必须在设计阶段被拒绝。

---

## 1. 核心原则（铁律层 - L1）

### 1.1 DETERMINISM-FIRST 原则（确定性优先）

**规则**：所有GPU计算、随机数生成、数据结构遍历必须保证确定性重放。

**v5.5 强制种子派生模式**：
```cpp
// ✅ 正确：使用全局线程ID派生种子
__global__ void batch_kernel(
    const uint256_t* scalars,
    ec_point_t* results,
    uint64_t replay_seed,    // 必须从配置加载
    int grid_dim,            // 必须从配置加载
    int block_dim            // 必须从配置加载
) {
    // 计算全局线程ID
    int global_tid = blockIdx.x * blockDim.x + threadIdx.x;
    
    // ✅ 正确：从replay_seed派生唯一种子
    uint64_t thread_seed = replay_seed + global_tid;
    xorshift64_state rng = init_rng(thread_seed);
    
    // 处理数据...
}

// ❌ 错误1：使用硬件时钟
__global__ void bad_kernel_v1(const uint256_t* scalars, ec_point_t* results) {
    uint64_t rng = clock64();  // 不可重放！
}

// ❌ 错误2：仅使用threadIdx.x作为种子
__global__ void bad_kernel_v2(const uint256_t* scalars, ec_point_t* results, uint64_t seed) {
    uint64_t rng_seed = threadIdx.x;  // 不同block会重复！
}

// ❌ 错误3：使用blockIdx.x但未加threadIdx.x
__global__ void bad_kernel_v3(const uint256_t* scalars, ec_point_t* results, uint64_t seed) {
    uint64_t rng_seed = seed + blockIdx.x;  // 同一block内线程种子相同！
}
```

**v5.5 增强CI检测器**：
```bash
#!/bin/bash
# ci/check_determinism.sh (v5.5)

VIOLATIONS=0

# 检查CUDA代码中的非确定性API
NON_DET_APIS=(
    "clock64" "clock" "curandGenerate" "rand()" "srand" "time(NULL)"
    "std::random_device" "std::chrono::high_resolution_clock"
)

for api in "${NON_DET_APIS[@]}"; do
    if grep -rn --include="*.cu" --include="*.cuh" "\b${api}\b" src/; then
        echo "ERROR: Non-deterministic API detected: ${api}"
        echo "  Use replay_seed from config instead"
        VIOLATIONS=$((VIOLATIONS + 1))
    fi
done

# v5.5 新增：检查错误的种子派生模式
echo "Checking seed derivation patterns..."

# 检查仅使用threadIdx.x的模式
if grep -rn --include="*.cu" --include="*.cuh" "init_rng.*threadIdx\.x" src/ | grep -v "blockIdx"; then
    echo "ERROR: Seed derivation using only threadIdx.x detected"
    echo "  Must use: replay_seed + blockIdx.x * blockDim.x + threadIdx.x"
    VIOLATIONS=$((VIOLATIONS + 1))
fi

# 检查仅使用blockIdx.x的模式
if grep -rn --include="*.cu" --include="*.cuh" "init_rng.*blockIdx\.x" src/ | grep -v "threadIdx"; then
    echo "ERROR: Seed derivation using only blockIdx.x detected"
    echo "  Must include threadIdx.x for per-thread uniqueness"
    VIOLATIONS=$((VIOLATIONS + 1))
fi

# 检查是否有未记录的launch配置
if grep -rn "<<<.*>>>" src/ | grep -v "grid_dim\|block_dim\|config"; then
    echo "ERROR: Kernel launch without config-based dimensions"
    VIOLATIONS=$((VIOLATIONS + 1))
fi

# v5.5 新增：检查动态计算的grid/block维度
if grep -rn --include="*.cu" --include="*.cpp" "dim3.*grid.*=" src/ | grep -v "config\|yaml"; then
    echo "ERROR: Dynamic grid dimension calculation detected"
    echo "  Grid/block dimensions must be loaded from config/puzzle71.yaml"
    VIOLATIONS=$((VIOLATIONS + 1))
fi

exit $VIOLATIONS
```


---

### 1.2 TEST-FIRST-CUDA 原则（测试优先CUDA）

**规则**：所有CUDA内核、host函数、CLI功能必须先编写失败的测试，确认测试失败后才能编写实现。

**强制工作流**：
```
[收到实现任务 T029: Implement fused batch kernel]
    ↓
[步骤1] 编写失败的测试
    - tests/unit/test_kernel_interfaces.cu
    - tests/validation/test_hash160_gpu_cpu_parity.cpp
    - tests/perf/test_range_scan_benchmark.cu
    ↓
[步骤2] 运行测试，确认全部失败（红灯）
    - 必须截图或保存完整日志作为证据
    - 提交到 docs/validation/evidence/T029_test_failures.log
    - v5.5: 证据文件必须包含时间戳和测试输出
    ↓
[步骤3] 编写最小实现
    - src/puzzle71_kernel.cu
    ↓
[步骤4] 运行测试，确认全部通过（绿灯）
    - 如仍有失败，回到步骤3
    ↓
[步骤5] 提交代码
    - Commit message必须引用测试失败证据文件
    - v5.5: CI自动验证证据时间戳早于实现
```

**v5.5 增强TDD证据格式**：
```
# docs/validation/evidence/T029_test_failures.log

=== Test Failure Evidence ===
Task ID: T029
Timestamp: 2025-01-20T10:30:00Z
Operator: developer@example.com
Purpose: TDD evidence for fused batch kernel

--- Test Execution ---
$ cd build && ctest -R test_kernel_interfaces -V

Test #1: test_kernel_interfaces
  Start 1: test_kernel_interfaces
1: Test command: /path/to/test_kernel_interfaces
1: Test timeout computed to be: 600
1: [==========] Running 3 tests from 1 test suite.
1: [----------] Global test environment set-up.
1: [----------] 3 tests from BatchKernelTest
1: [ RUN      ] BatchKernelTest.LaunchWithValidParams
1: /path/to/test_kernel_interfaces.cu:45: Failure
1: Expected: kernel_launch_success
1:   Actual: false (Kernel not implemented yet)
1: [  FAILED  ] BatchKernelTest.LaunchWithValidParams (2 ms)
1: [ RUN      ] BatchKernelTest.ProducesCorrectResults
1: /path/to/test_kernel_interfaces.cu:67: Failure
1: Expected: results_match
1:   Actual: false (Kernel not implemented yet)
1: [  FAILED  ] BatchKernelTest.ProducesCorrectResults (1 ms)
1: [ RUN      ] BatchKernelTest.MeetsPerformanceBaseline
1: /path/to/test_kernel_interfaces.cu:89: Failure
1: Expected: throughput >= 1000000000
1:   Actual: 0 (Kernel not implemented yet)
1: [  FAILED  ] BatchKernelTest.MeetsPerformanceBaseline (0 ms)
1: [----------] 3 tests from BatchKernelTest (3 ms total)
1: [==========] 3 tests from 1 test suite ran. (3 ms total)
1: [  PASSED  ] 0 tests.
1: [  FAILED  ] 3 tests, listed below:
1: [  FAILED  ] BatchKernelTest.LaunchWithValidParams
1: [  FAILED  ] BatchKernelTest.ProducesCorrectResults
1: [  FAILED  ] BatchKernelTest.MeetsPerformanceBaseline

--- Evidence Digest ---
SHA-256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
Generated: 2025-01-20T10:30:15Z
```

**v5.5 增强CI门禁**：
```bash
#!/bin/bash
# ci/tdd_gate_v5.5.sh

set -e

echo "=== TDD Compliance Gate (v5.5) ==="

VIOLATIONS=0

# 提取本次PR修改的源文件
IMPL_FILES=$(git diff --name-only origin/main | grep "^src/")

if [ -z "$IMPL_FILES" ]; then
    echo "No implementation files changed, skipping TDD check"
    exit 0
fi

for impl_file in $IMPL_FILES; do
    echo "Checking TDD compliance for: $impl_file"
    
    # 提取任务ID
    TASK_ID=$(git log -1 --pretty=%B "$impl_file" | grep -oP 'T\d+' | head -n1)
    
    if [ -z "$TASK_ID" ]; then
        echo "ERROR: $impl_file missing task ID in commit message"
        echo "  Commit message must start with 'T0XX: <description>'"
        VIOLATIONS=$((VIOLATIONS + 1))
        continue
    fi
    
    echo "  Task ID: $TASK_ID"
    
    # 检查测试失败证据文件
    EVIDENCE_FILE="docs/validation/evidence/${TASK_ID}_test_failures.log"
    
    if [ ! -f "$EVIDENCE_FILE" ]; then
        echo "ERROR: Missing test failure evidence for $TASK_ID"
        echo "  Expected: $EVIDENCE_FILE"
        echo "  TDD requires test-first approach"
        VIOLATIONS=$((VIOLATIONS + 1))
        continue
    fi
    
    echo "  Evidence file: $EVIDENCE_FILE"
    
    # v5.5: 验证证据文件格式
    if ! grep -q "=== Test Failure Evidence ===" "$EVIDENCE_FILE"; then
        echo "ERROR: Evidence file missing required header"
        echo "  Evidence must include '=== Test Failure Evidence ==='"
        VIOLATIONS=$((VIOLATIONS + 1))
    fi
    
    if ! grep -q "Timestamp:" "$EVIDENCE_FILE"; then
        echo "ERROR: Evidence file missing timestamp"
        VIOLATIONS=$((VIOLATIONS + 1))
    fi
    
    if ! grep -q "\[  FAILED  \]" "$EVIDENCE_FILE"; then
        echo "ERROR: Evidence file does not show test failures"
        echo "  TDD requires tests to fail before implementation"
        VIOLATIONS=$((VIOLATIONS + 1))
    fi
    
    # v5.5: 验证Git时间戳
    if git ls-files --error-unmatch "$EVIDENCE_FILE" > /dev/null 2>&1; then
        EVIDENCE_TIME=$(git log -1 --format=%ct "$EVIDENCE_FILE" 2>/dev/null || echo "0")
        IMPL_TIME=$(git log -1 --format=%ct "$impl_file" 2>/dev/null || echo "0")
        
        if [ "$EVIDENCE_TIME" -gt "$IMPL_TIME" ]; then
            echo "ERROR: Test evidence created AFTER implementation"
            echo "  Evidence time: $(date -d @$EVIDENCE_TIME)"
            echo "  Implementation time: $(date -d @$IMPL_TIME)"
            echo "  This violates TDD (test-first) principle"
            VIOLATIONS=$((VIOLATIONS + 1))
        else
            echo "  ✓ Evidence timestamp verified (before implementation)"
        fi
    fi
done

if [ $VIOLATIONS -gt 0 ]; then
    echo ""
    echo "❌ TDD compliance check FAILED with $VIOLATIONS violations"
    exit 1
fi

echo ""
echo "✓ TDD compliance verified"
```

---

### 1.3 NO-CRYPTO-REINVENTION 原则（禁止重新实现密码学）

**规则**：禁止重新实现任何椭圆曲线运算、大数运算、哈希算法，必须适配参考源。

**强制参考源（SoT）**：

| 功能领域 | 参考源 | 同步命令 | 适配方式 | v5.5强制路径 |
|---------|--------|---------|---------|-------------|
| Endomorphism | secp256k1-zkp | `sync_reference_sources.sh` | 通过adapter调用 | `src/utils/endomorphism_adapter.h` |
| Batch Stepping | VanitySearch | `sync_reference_sources.sh` | 移植kernel模式 | `src/utils/batch_adapter.h` |
| CPU Validation | bitcoin-core/secp256k1 | `sync_reference_sources.sh` | 直接链接库 | `src/utils/secp256k1_bridge.h` |
| HASH160 | BitCrack | `sync_reference_sources.sh` | 复用device代码 | `src/utils/hash160_adapter.h` |

**v5.5 强制Adapter路径**：
```cpp
// ✅ 正确：通过adapter调用参考源
// src/utils/endomorphism_adapter.h

#include "snapshots/secp256k1-zkp/src/scalar_impl.h"

namespace puzzle71 {
namespace adapters {

/**
 * @brief Adapter for secp256k1-zkp endomorphism split
 * @origin https://github.com/ElementsProject/secp256k1-zkp
 * @origin_commit [记录在docs/reference-locks.md]
 * @spdx_license_identifier MIT
 * @v5.5_enforcement This is the ONLY allowed path for endomorphism operations
 */
inline void split_scalar_lambda(
    const secp256k1_scalar* scalar,
    secp256k1_scalar* k1,
    secp256k1_scalar* k2
) {
    // 调用参考源函数
    secp256k1_scalar_split_lambda(k1, k2, scalar);
}

} // namespace adapters
} // namespace puzzle71

// ✅ 正确：在业务代码中使用adapter
// src/puzzle71_kernel.cu
#include "utils/endomorphism_adapter.h"

__host__ void prepare_scalars(const uint256_t* keys, secp256k1_scalar* k1, secp256k1_scalar* k2) {
    puzzle71::adapters::split_scalar_lambda(&keys[0], k1, k2);
}

// ❌ 错误1：重新实现endomorphism
namespace puzzle71 {
void my_endomorphism_split(uint256_t k, uint256_t* k1, uint256_t* k2) {
    // 自己实现GLV分解 - 严重违规！
}
}

// ❌ 错误2：直接include参考源（v5.5禁止）
#include "snapshots/secp256k1-zkp/src/scalar_impl.h"  // 必须通过adapter！

void some_function() {
    secp256k1_scalar_split_lambda(...);  // 直接调用 - 违规！
}

// ❌ 错误3：绕过adapter的wrapper
namespace puzzle71 {
void wrapper_split(uint256_t k, uint256_t* k1, uint256_t* k2) {
    // 即使调用了参考源，但不在adapter命名空间 - 违规！
    secp256k1_scalar_split_lambda(...);
}
}
```

**v5.5 增强CI检测器**：
```bash
#!/bin/bash
# ci/check_crypto_reinvention_v5.5.sh

VIOLATIONS=0

echo "=== Crypto Reinvention Check (v5.5) ==="

# 禁止的自定义实现标识
FORBIDDEN_IMPL=(
    "my_ec_mul" "custom_scalar_mul" "simple_point_add"
    "basic_modular_inverse" "quick_bigint" "fast_hash160"
    "optimized_ecdsa" "improved_secp256k1"
    "my_endomorphism" "custom_glv" "simple_ecdsa"
)

for pattern in "${FORBIDDEN_IMPL[@]}"; do
    if grep -rn --include="*.cpp" --include="*.cu" --include="*.h" "\b${pattern}\b" src/; then
        echo "ERROR: Detected crypto reinvention: ${pattern}"
        echo "  Use reference sources from snapshots/"
        VIOLATIONS=$((VIOLATIONS + 1))
    fi
done

# v5.5: 升级为ERROR - 禁止直接include参考源头文件
echo ""
echo "Checking direct reference source includes..."

DIRECT_INCLUDES=$(grep -rn --include="*.cpp" --include="*.cu" --include="*.h" \
    "#include.*snapshots/.*\.h" src/ | grep -v "adapter\.h")

if [ -n "$DIRECT_INCLUDES" ]; then
    echo "ERROR: Direct include of reference source headers detected"
    echo "$DIRECT_INCLUDES"
    echo ""
    echo "  Reference sources must ONLY be included in adapter files:"
    echo "    - src/utils/endomorphism_adapter.h"
    echo "    - src/utils/batch_adapter.h"
    echo "    - src/utils/secp256k1_bridge.h"
    echo "    - src/utils/hash160_adapter.h"
    echo ""
    echo "  Business code must use adapters, not direct includes"
    VIOLATIONS=$((VIOLATIONS + 1))
fi

# v5.5: 检查adapter命名空间使用
echo ""
echo "Checking adapter namespace usage..."

NON_ADAPTER_CALLS=$(grep -rn --include="*.cpp" --include="*.cu" \
    "secp256k1_\|VanitySearch_\|BitCrack_" src/ | \
    grep -v "adapters::" | \
    grep -v "adapter\.h" | \
    grep -v "bridge\.h")

if [ -n "$NON_ADAPTER_CALLS" ]; then
    echo "ERROR: Direct calls to reference functions outside adapter namespace"
    echo "$NON_ADAPTER_CALLS"
    echo ""
    echo "  All crypto operations must use puzzle71::adapters:: namespace"
    VIOLATIONS=$((VIOLATIONS + 1))
fi

if [ $VIOLATIONS -gt 0 ]; then
    echo ""
    echo "❌ Crypto reinvention check FAILED with $VIOLATIONS violations"
    exit 1
fi

echo ""
echo "✓ No crypto reinvention detected"
```


---

### 1.4 ZERO-TOLERANCE-PERFORMANCE 原则（零容忍性能退化）

**规则**：所有性能关键路径的修改必须通过基准测试，不得低于已记录的基线。

**性能基线（Baseline Table）**：
```json
// benchmarks/baseline/gpu_baselines.json
{
  "version": "1.0",
  "updated_at": "2025-01-20T00:00:00Z",
  "baselines": [
    {
      "gpu_model": "NVIDIA GeForce RTX 2080 Ti",
      "min_keys_per_sec": 1000000000,
      "max_variance_pct": 5.0,
      "reference_occupancy": 85.0,
      "measured_at": "2025-01-15T10:00:00Z",
      "commit_sha": "abc123def456"
    },
    {
      "gpu_model": "NVIDIA GeForce RTX 3090",
      "min_keys_per_sec": 2000000000,
      "max_variance_pct": 5.0,
      "reference_occupancy": 90.0,
      "measured_at": "2025-01-15T10:00:00Z",
      "commit_sha": "abc123def456"
    },
    {
      "gpu_model": "NVIDIA A100-SXM4-40GB",
      "min_keys_per_sec": 4000000000,
      "max_variance_pct": 5.0,
      "reference_occupancy": 95.0,
      "measured_at": "2025-01-15T10:00:00Z",
      "commit_sha": "abc123def456"
    }
  ]
}
```

**v5.5 强制预热协议**：
```bash
#!/bin/bash
# scripts/run-benchmarks.sh (v5.5)

DEVICE_ID=${1:-0}
ITERATIONS=${2:-5}
WARMUP_ITERATIONS=3  # v5.5: 强制3次预热

OUTPUT_FILE="benchmarks/run_$(date +%Y%m%d_%H%M%S).json"

echo "=== Puzzle71Solver Performance Benchmark (v5.5) ==="
echo "Device: $DEVICE_ID"
echo "Warmup: $WARMUP_ITERATIONS iterations (results discarded)"
echo "Measurement: $ITERATIONS iterations"
echo ""

# 获取GPU型号
GPU_MODEL=$(nvidia-smi --id=$DEVICE_ID --query-gpu=name --format=csv,noheader)
echo "GPU Model: $GPU_MODEL"

# v5.5: 预热阶段（结果必须丢弃）
echo ""
echo "=== Warmup Phase ==="
WARMUP_RESULTS=()

for i in $(seq 1 $WARMUP_ITERATIONS); do
    echo "  Warmup $i/$WARMUP_ITERATIONS"
    
    WARMUP_RESULT=$(./Puzzle71Solver \
        --keyspace 0x400000000000000000:0x40000000000FFFFF \
        --target-address 1PWo3JeB9jrGwfHDNpdGK54CRas7fsVzXU \
        --operator-id benchmark \
        --operator-purpose warmup \
        --device $DEVICE_ID \
        --benchmark-mode \
        2>&1 | grep "Throughput:" | awk '{print $2}')
    
    WARMUP_RESULTS+=($WARMUP_RESULT)
    echo "    Throughput: $WARMUP_RESULT keys/sec (discarded)"
done

echo ""
echo "Warmup complete. Results discarded as per v5.5 protocol."

# v5.5: 测量阶段
echo ""
echo "=== Measurement Phase ==="
THROUGHPUTS=()

for i in $(seq 1 $ITERATIONS); do
    echo "  Iteration $i/$ITERATIONS"
    
    RESULT=$(./Puzzle71Solver \
        --keyspace 0x400000000000000000:0x40000000000FFFFFF \
        --target-address 1PWo3JeB9jrGwfHDNpdGK54CRas7fsVzXU \
        --operator-id benchmark \
        --operator-purpose measurement \
        --device $DEVICE_ID \
        --benchmark-mode \
        2>&1 | grep "Throughput:" | awk '{print $2}')
    
    THROUGHPUTS+=($RESULT)
    echo "    Throughput: $RESULT keys/sec"
done

# 统计分析
MEDIAN=$(printf '%s\n' "${THROUGHPUTS[@]}" | sort -n | awk '{a[NR]=$1} END{print (NR%2==1)?a[(NR+1)/2]:(a[NR/2]+a[NR/2+1])/2}')
MIN=$(printf '%s\n' "${THROUGHPUTS[@]}" | sort -n | head -n1)
MAX=$(printf '%s\n' "${THROUGHPUTS[@]}" | sort -n | tail -n1)

# 计算标准差
STDDEV=$(python3 << EOF
import statistics
data = [${THROUGHPUTS[*]}]
print(statistics.stdev(data) if len(data) > 1 else 0)
EOF
)

# v5.5: 生成报告（包含预热信息）
cat > "$OUTPUT_FILE" << EOF
{
  "version": "5.5",
  "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "gpu_model": "$GPU_MODEL",
  "device_id": $DEVICE_ID,
  "warmup": {
    "iterations": $WARMUP_ITERATIONS,
    "results_discarded": [$(IFS=,; echo "${WARMUP_RESULTS[*]}")],
    "note": "Warmup results not used in statistics per v5.5 protocol"
  },
  "measurement": {
    "iterations": $ITERATIONS,
    "throughputs": [$(IFS=,; echo "${THROUGHPUTS[*]}")],
    "statistics": {
      "median_keys_per_sec": $MEDIAN,
      "min_keys_per_sec": $MIN,
      "max_keys_per_sec": $MAX,
      "stddev": $STDDEV
    }
  },
  "digest": {
    "algorithm": "SHA-256",
    "hash": "$(echo "$OUTPUT_FILE" | sha256sum | awk '{print $1}')",
    "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  }
}
EOF

echo ""
echo "=== Benchmark Results ==="
echo "Median:  $MEDIAN keys/sec"
echo "Min:     $MIN keys/sec"
echo "Max:     $MAX keys/sec"
echo "StdDev:  $STDDEV"
echo ""
echo "Report saved to: $OUTPUT_FILE"

# 基线比对
BASELINE_FILE="benchmarks/baseline/gpu_baselines.json"
if [ -f "$BASELINE_FILE" ]; then
    BASELINE=$(jq -r ".baselines[] | select(.gpu_model == \"$GPU_MODEL\") | .min_keys_per_sec" "$BASELINE_FILE")
    
    if [ -n "$BASELINE" ] && [ "$BASELINE" != "null" ]; then
        RATIO=$(echo "scale=4; $MEDIAN / $BASELINE" | bc)
        echo "Baseline: $BASELINE keys/sec"
        echo "Ratio:    ${RATIO}x"
        
        if (( $(echo "$RATIO < 0.95" | bc -l) )); then
            echo ""
            echo "❌ PERFORMANCE REGRESSION"
            exit 1
        else
            echo ""
            echo "✓ Performance meets baseline"
        fi
    fi
fi
```

**v5.5 增强性能门禁**：
```bash
#!/bin/bash
# ci/performance_gate_v5.5.sh

set -e

echo "=== Performance Gate (v5.5) ==="

GPU_MODEL=$(nvidia-smi --query-gpu=name --format=csv,noheader | head -n1)
BASELINE_FILE="benchmarks/baseline/gpu_baselines.json"

# 运行基准测试（v5.5协议：3次预热+5次测量）
echo "Running benchmark with v5.5 protocol..."
./scripts/run-benchmarks.sh 0 5

# 查找最新的benchmark结果
LATEST_RESULT=$(ls -t benchmarks/run_*.json | head -n1)

if [ ! -f "$LATEST_RESULT" ]; then
    echo "ERROR: Benchmark result file not found"
    exit 1
fi

echo "Latest result: $LATEST_RESULT"

# v5.5: 验证预热阶段执行
WARMUP_COUNT=$(jq '.warmup.iterations' "$LATEST_RESULT")
if [ "$WARMUP_COUNT" != "3" ]; then
    echo "ERROR: Warmup phase not executed correctly"
    echo "  Expected 3 warmup iterations, got: $WARMUP_COUNT"
    exit 1
fi

echo "✓ Warmup phase verified (3 iterations, results discarded)"

# 提取当前吞吐量
CURRENT_THROUGHPUT=$(jq '.measurement.statistics.median_keys_per_sec' "$LATEST_RESULT")

echo "Current throughput: $CURRENT_THROUGHPUT keys/sec"

# 提取基线吞吐量
BASELINE_THROUGHPUT=$(jq -r \
    ".baselines[] | select(.gpu_model == \"${GPU_MODEL}\") | .min_keys_per_sec" \
    "$BASELINE_FILE")

if [ -z "$BASELINE_THROUGHPUT" ] || [ "$BASELINE_THROUGHPUT" == "null" ]; then
    echo "WARNING: No baseline for ${GPU_MODEL}"
    echo "  Recording current performance as new baseline"
    
    # v5.5: 自动添加新GPU型号基线
    jq ".baselines += [{
        \"gpu_model\": \"$GPU_MODEL\",
        \"min_keys_per_sec\": $CURRENT_THROUGHPUT,
        \"max_variance_pct\": 5.0,
        \"reference_occupancy\": 85.0,
        \"measured_at\": \"$(date -u +%Y-%m-%dT%H:%M:%SZ)\",
        \"commit_sha\": \"$(git rev-parse HEAD)\"
    }]" "$BASELINE_FILE" > "${BASELINE_FILE}.tmp"
    
    mv "${BASELINE_FILE}.tmp" "$BASELINE_FILE"
    echo "✓ New baseline recorded"
    exit 0
fi

# 计算比率
RATIO=$(echo "scale=4; $CURRENT_THROUGHPUT / $BASELINE_THROUGHPUT" | bc)
THRESHOLD=0.95

echo "Baseline: $BASELINE_THROUGHPUT keys/sec"
echo "Ratio: ${RATIO}x (threshold: ${THRESHOLD}x)"

if (( $(echo "$RATIO < $THRESHOLD" | bc -l) )); then
    echo ""
    echo "❌ PERFORMANCE REGRESSION DETECTED"
    echo "  GPU Model:     ${GPU_MODEL}"
    echo "  Baseline:      ${BASELINE_THROUGHPUT} keys/sec"
    echo "  Current:       ${CURRENT_THROUGHPUT} keys/sec"
    echo "  Ratio:         ${RATIO}x (threshold: ${THRESHOLD}x)"
    echo "  Degradation:   $(echo "scale=2; (1 - $RATIO) * 100" | bc)%"
    echo ""
    echo "This PR is BLOCKED until performance is restored."
    echo ""
    echo "Troubleshooting steps:"
    echo "  1. Check CUDA register usage: ./tools/nsight/check_register_budget.sh"
    echo "  2. Profile with Nsight Compute: ncu --set full ./Puzzle71Solver ..."
    echo "  3. Verify occupancy: Check SM occupancy in telemetry"
    echo "  4. Review recent changes: git diff origin/main src/"
    exit 1
fi

echo ""
echo "✓ Performance check passed: ${RATIO}x of baseline"

# v5.5: 更新性能趋势
TREND_FILE="benchmarks/history/performance_trend.jsonl"
mkdir -p benchmarks/history

echo "{\"timestamp\": \"$(date -u +%Y-%m-%dT%H:%M:%SZ)\", \"gpu_model\": \"$GPU_MODEL\", \"throughput\": $CURRENT_THROUGHPUT, \"ratio\": $RATIO, \"commit\": \"$(git rev-parse HEAD)\"}" >> "$TREND_FILE"

echo "✓ Performance trend updated"
```


---

### 1.5 MANDATORY-DIGEST 原则（强制防篡改摘要）

**规则**：所有checkpoint、telemetry、benchmark、report文件必须包含SHA-256摘要。

**摘要格式标准**：
```json
{
  "payload": "... actual data ...",
  "digest": {
    "algorithm": "SHA-256",
    "hash": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
    "timestamp": "2025-01-20T12:34:56Z",
    "generator": "Puzzle71Solver v1.0.0"
  }
}
```

**v5.5 摘要验证SLA监控**：
```cpp
// src/utils/digest_verifier.h (v5.5)

namespace puzzle71 {

/**
 * @brief 计算artifact的SHA-256摘要
 * @param data 原始数据
 * @param size 数据大小
 * @param out_digest 输出摘要（32字节）
 * @return 0成功，非0失败
 */
int compute_sha256_digest(const void* data, size_t size, uint8_t out_digest[32]);

/**
 * @brief 验证artifact的SHA-256摘要（v5.5: 带SLA监控）
 * @param artifact_path 文件路径
 * @param expected_digest 期望摘要（hex字符串）
 * @return 0成功，-1文件不存在，-2摘要不匹配，-3超时
 * @sla 必须在250ms内完成验证
 * @v5.5_enhancement 超时自动记录到告警日志
 */
int verify_artifact_digest(const char* artifact_path, const char* expected_digest);

/**
 * @brief 为artifact添加摘要字段
 * @param artifact_json JSON对象
 * @return 0成功，非0失败
 */
int append_digest_to_artifact(json& artifact_json);

} // namespace puzzle71
```

**v5.5 摘要验证实现（带超时监控）**：
```cpp
// src/utils/digest_verifier.cpp (v5.5)

#include "digest_verifier.h"
#include <chrono>
#include <fstream>
#include <openssl/sha.h>

namespace puzzle71 {

int verify_artifact_digest(const char* artifact_path, const char* expected_digest) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // 读取文件
    std::ifstream ifs(artifact_path, std::ios::binary);
    if (!ifs.is_open()) {
        return -1;  // 文件不存在
    }
    
    // 读取内容
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(ifs)),
                               std::istreambuf_iterator<char>());
    ifs.close();
    
    // 计算摘要
    uint8_t computed_digest[32];
    int ret = compute_sha256_digest(data.data(), data.size(), computed_digest);
    if (ret != 0) {
        return ret;
    }
    
    // 转换为hex字符串
    char computed_hex[65];
    for (int i = 0; i < 32; i++) {
        sprintf(&computed_hex[i*2], "%02x", computed_digest[i]);
    }
    
    // 比对摘要
    if (strcmp(computed_hex, expected_digest) != 0) {
        return -2;  // 摘要不匹配
    }
    
    // v5.5: 检查SLA
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    if (duration.count() > 250) {
        // v5.5: 超时记录到告警日志
        std::ofstream alert_log("logs/digest_sla_violations.log", std::ios::app);
        alert_log << "[" << std::time(nullptr) << "] "
                  << "Digest verification timeout: " << artifact_path
                  << " took " << duration.count() << "ms (SLA: 250ms)"
                  << std::endl;
        alert_log.close();
        
        fprintf(stderr, "WARNING: Digest verification took %ldms (SLA: 250ms)\n", 
                duration.count());
        
        return -3;  // 超时
    }
    
    return 0;  // 成功
}

int compute_sha256_digest(const void* data, size_t size, uint8_t out_digest[32]) {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, size);
    SHA256_Final(out_digest, &ctx);
    return 0;
}

} // namespace puzzle71
```

**v5.5 增强CI摘要验证**：
```bash
#!/bin/bash
# ci/verify_all_digests_v5.5.sh

set -e

echo "=== Artifact Digest Verification (v5.5) ==="

FAILURES=0
SLA_VIOLATIONS=0

# 验证checkpoint摘要
echo ""
echo "[1/4] Verifying checkpoint digests..."

if [ -d "checkpoints" ]; then
    CHECKPOINT_COUNT=$(find checkpoints/ -name "*.json" | wc -l)
    echo "  Found $CHECKPOINT_COUNT checkpoint files"
    
    find checkpoints/ -name "*.json" | while read manifest; do
        # 提取存储的摘要
        STORED_DIGEST=$(jq -r '.digest.hash' "$manifest" 2>/dev/null)
        
        if [ -z "$STORED_DIGEST" ] || [ "$STORED_DIGEST" == "null" ]; then
            echo "  ERROR: Missing digest in $manifest"
            FAILURES=$((FAILURES + 1))
            continue
        fi
        
        # 验证摘要（使用C++工具）
        if ! ./build/tools/verify_digest "$manifest" "$STORED_DIGEST"; then
            echo "  ERROR: Digest verification failed for $manifest"
            FAILURES=$((FAILURES + 1))
        fi
    done
fi

# 验证telemetry摘要
echo ""
echo "[2/4] Verifying telemetry digests..."

if [ -d "telemetry" ]; then
    TELEMETRY_COUNT=$(find telemetry/ -name "*.jsonl" | wc -l)
    echo "  Found $TELEMETRY_COUNT telemetry files"
    
    find telemetry/ -name "*.jsonl" | while read log; do
        LINE_NUM=0
        while read line; do
            LINE_NUM=$((LINE_NUM + 1))
            
            # 每行telemetry都应该有内联摘要
            if ! echo "$line" | jq -e '.digest' > /dev/null 2>&1; then
                echo "  ERROR: Missing digest in $log:$LINE_NUM"
                FAILURES=$((FAILURES + 1))
                break
            fi
        done < "$log"
    done
fi

# 验证benchmark摘要
echo ""
echo "[3/4] Verifying benchmark digests..."

if [ -d "benchmarks" ]; then
    BENCHMARK_COUNT=$(find benchmarks/ -name "run_*.json" | wc -l)
    echo "  Found $BENCHMARK_COUNT benchmark files"
    
    find benchmarks/ -name "run_*.json" | while read bench; do
        if ! jq -e '.digest' "$bench" > /dev/null 2>&1; then
            echo "  ERROR: Missing digest in $bench"
            FAILURES=$((FAILURES + 1))
        fi
    done
fi

# v5.5: 检查SLA违规
echo ""
echo "[4/4] Checking digest SLA violations..."

if [ -f "logs/digest_sla_violations.log" ]; then
    SLA_VIOLATIONS=$(wc -l < logs/digest_sla_violations.log)
    
    if [ $SLA_VIOLATIONS -gt 0 ]; then
        echo "  WARNING: Found $SLA_VIOLATIONS digest SLA violations"
        echo "  Recent violations:"
        tail -n 5 logs/digest_sla_violations.log | sed 's/^/    /'
        echo ""
        echo "  Action required: Optimize digest computation or increase SLA threshold"
    else
        echo "  ✓ No SLA violations"
    fi
else
    echo "  ✓ No SLA violations"
fi

# 汇总结果
echo ""
echo "=== Verification Summary ==="
echo "Digest failures: $FAILURES"
echo "SLA violations: $SLA_VIOLATIONS"

if [ $FAILURES -gt 0 ]; then
    echo ""
    echo "❌ Digest verification FAILED"
    exit 1
fi

if [ $SLA_VIOLATIONS -gt 10 ]; then
    echo ""
    echo "⚠️  Excessive SLA violations detected"
    echo "  This may indicate performance issues"
    exit 2
fi

echo ""
echo "✓ All artifact digests verified"
```

---

## 2. 配置文件强制验证（v5.5新增）

### 2.1 配置文件Schema

**v5.5 完整配置Schema**：
```yaml
# config/puzzle71.yaml (v5.5)

# 确定性重放配置（所有字段必需）
deterministic_config:
  version: "5.5"
  
  # Kernel启动配置（必须固定，不得动态计算）
  kernel_launch:
    grid_dim: 1024        # 必须从配置加载
    block_dim: 256        # 必须从配置加载
    points_per_thread: 8  # 必须固定
    shared_mem_bytes: 49152
  
  # 随机数生成器（必须使用固定种子）
  rng:
    algorithm: "xorshift64"
    base_seed: 0x123456789ABCDEF0  # 从这个基础种子派生
    # v5.5: 种子派生公式
    derivation: "replay_seed + blockIdx.x * blockDim.x + threadIdx.x"
  
  # 数据布局（必须固定）
  memory_layout:
    scalar_format: "little_endian_u32x8"
    point_format: "jacobian_projective"

# 操作员默认值
operator:
  default_id: "unknown"
  default_purpose: "testing"
  require_explicit: true  # v5.5: 强制要求显式指定

# 性能配置
performance:
  checkpoint_interval_sec: 1800  # 30分钟
  checkpoint_interval_keys: 3000000000000  # 3e12
  telemetry_interval_sec: 1
  # v5.5: 基准测试配置
  benchmark:
    warmup_iterations: 3
    measurement_iterations: 5
    discard_warmup: true

# v5.5: 摘要配置
digest:
  algorithm: "SHA-256"
  sla_ms: 250
  alert_on_violation: true
```

### 2.2 配置加载强制校验

**v5.5 配置加载器**：
```cpp
// src/config/puzzle71_config.cpp (v5.5)

namespace puzzle71 {

struct DeterministicConfig {
    std::string version;
    int grid_dim;
    int block_dim;
    int points_per_thread;
    uint64_t base_seed;
    std::string seed_derivation;
    std::string scalar_format;
    std::string point_format;
    bool require_explicit_operator;
};

/**
 * @brief 加载并验证确定性配置（v5.5增强）
 * @param config_path 配置文件路径
 * @param out_config 输出配置
 * @return 0成功，非0失败
 * @note 配置必须完整，任何字段缺失都会导致加载失败
 * @v5.5_enhancement 增加版本检查和字段完整性验证
 */
int load_deterministic_config(
    const char* config_path,
    DeterministicConfig* out_config
) {
    // 读取YAML
    YAML::Node config;
    try {
        config = YAML::LoadFile(config_path);
    } catch (const YAML::Exception& e) {
        fprintf(stderr, "ERROR: Failed to load config: %s\n", e.what());
        return -1;
    }
    
    // v5.5: 检查版本
    if (!config["deterministic_config"]) {
        fprintf(stderr, "ERROR: Missing deterministic_config section\n");
        return -1;
    }
    
    auto det = config["deterministic_config"];
    
    std::string version = det["version"].as<std::string>("");
    if (version != "5.5") {
        fprintf(stderr, "ERROR: Config version mismatch\n");
        fprintf(stderr, "  Expected: 5.5\n");
        fprintf(stderr, "  Found: %s\n", version.c_str());
        return -1;
    }
    out_config->version = version;
    
    // 检查kernel_launch（所有字段必需）
    if (!det["kernel_launch"]) {
        fprintf(stderr, "ERROR: Missing kernel_launch config\n");
        return -1;
    }
    
    auto launch = det["kernel_launch"];
    
    if (!launch["grid_dim"] || !launch["block_dim"] || !launch["points_per_thread"]) {
        fprintf(stderr, "ERROR: Incomplete kernel_launch config\n");
        fprintf(stderr, "  Required fields: grid_dim, block_dim, points_per_thread\n");
        return -1;
    }
    
    out_config->grid_dim = launch["grid_dim"].as<int>();
    out_config->block_dim = launch["block_dim"].as<int>();
    out_config->points_per_thread = launch["points_per_thread"].as<int>();
    
    // 检查RNG配置
    if (!det["rng"]) {
        fprintf(stderr, "ERROR: Missing rng config\n");
        return -1;
    }
    
    auto rng = det["rng"];
    
    if (!rng["base_seed"] || !rng["derivation"]) {
        fprintf(stderr, "ERROR: Incomplete rng config\n");
        fprintf(stderr, "  Required fields: base_seed, derivation\n");
        return -1;
    }
    
    out_config->base_seed = rng["base_seed"].as<uint64_t>();
    out_config->seed_derivation = rng["derivation"].as<std::string>();
    
    // v5.5: 验证种子派生公式
    std::string expected_derivation = "replay_seed + blockIdx.x * blockDim.x + threadIdx.x";
    if (out_config->seed_derivation != expected_derivation) {
        fprintf(stderr, "ERROR: Invalid seed derivation formula\n");
        fprintf(stderr, "  Expected: %s\n", expected_derivation.c_str());
        fprintf(stderr, "  Found: %s\n", out_config->seed_derivation.c_str());
        return -1;
    }
    
    // 验证配置合理性
    if (out_config->grid_dim <= 0 || out_config->block_dim <= 0) {
        fprintf(stderr, "ERROR: Invalid kernel dimensions\n");
        return -1;
    }
    
    if (out_config->block_dim > 1024) {
        fprintf(stderr, "ERROR: block_dim exceeds hardware limit (1024)\n");
        return -1;
    }
    
    // v5.5: 检查操作员配置
    if (config["operator"] && config["operator"]["require_explicit"]) {
        out_config->require_explicit_operator = 
            config["operator"]["require_explicit"].as<bool>();
    } else {
        out_config->require_explicit_operator = true;  // 默认强制
    }
    
    fprintf(stdout, "✓ Configuration loaded successfully (v5.5)\n");
    fprintf(stdout, "  Grid: %d, Block: %d\n", 
            out_config->grid_dim, out_config->block_dim);
    fprintf(stdout, "  Base seed: 0x%lx\n", out_config->base_seed);
    fprintf(stdout, "  Seed derivation: %s\n", 
            out_config->seed_derivation.c_str());
    
    return 0;
}

} // namespace puzzle71
```


---

## 3. v5.5 快速参考卡

### 开发者速查（v5.5）

```bash
# === 工作前准备 ===
./tools/sync_reference_sources.sh apply
./tools/sync_reference_sources.sh verify

# === TDD工作流（v5.5增强） ===
# 1. 编写失败的测试
vim tests/unit/test_xxx.cpp
mkdir -p build && cd build && cmake .. && make && cd ..

# 2. 运行测试并保存证据（v5.5格式）
./build/tests/test_xxx 2>&1 | tee docs/validation/evidence/T0XX_test_failures.log

# 3. 验证证据格式
grep "=== Test Failure Evidence ===" docs/validation/evidence/T0XX_test_failures.log
grep "\[  FAILED  \]" docs/validation/evidence/T0XX_test_failures.log

# 4. 编写实现
vim src/xxx.cpp

# 5. 验证测试通过
./build/tests/test_xxx

# === 本地验证（提交前必做 - v5.5） ===
./ci/check_determinism.sh                    # 检查非确定性API和种子派生
./ci/check_crypto_reinvention_v5.5.sh        # 检查密码学重新实现和adapter使用
./ci/tdd_gate_v5.5.sh                        # 验证TDD证据时间戳
./ci/scan_placeholders.sh                    # 扫描占位符
./scripts/replay/verify-replay.sh checkpoints/latest.json  # 确定性重放
./scripts/run-benchmarks.sh 0 5              # 性能基准（3次预热+5次测量）
./tools/nsight/check_register_budget.sh      # CUDA寄存器预算
./ci/verify_all_digests_v5.5.sh              # 摘要验证+SLA检查

# === 提交（v5.5格式） ===
git add <files>
git commit -m "T0XX: <description>

Test evidence: docs/validation/evidence/T0XX_test_failures.log
Performance: XXX keys/sec (YY% above baseline)
Determinism: Verified with replay test
Seed derivation: replay_seed + blockIdx.x * blockDim.x + threadIdx.x
Adapter usage: All crypto ops via puzzle71::adapters::
Warmup protocol: 3 iterations discarded, 5 measured
"

git push origin <branch>
```

### v5.5 关键检查点

**确定性检查**：
- ✅ 禁用 clock64(), time(NULL), std::random_device
- ✅ 种子派生：`replay_seed + blockIdx.x * blockDim.x + threadIdx.x`
- ✅ Grid/block维度从 config/puzzle71.yaml 加载
- ✅ 配置版本必须为 "5.5"

**TDD检查**：
- ✅ 测试失败证据包含 "=== Test Failure Evidence ==="
- ✅ 证据包含时间戳和 [FAILED] 标记
- ✅ 证据Git时间戳早于实现代码

**密码学检查**：
- ✅ 所有ECC/BigInt通过 `puzzle71::adapters::` 调用
- ✅ 禁止直接 include snapshots/ 头文件（仅adapter可以）
- ✅ 禁止 my_*, custom_*, simple_* 等自定义实现

**性能检查**：
- ✅ 基准测试执行3次预热（结果丢弃）
- ✅ 基准测试执行5次测量
- ✅ 吞吐量 ≥ baseline的95%
- ✅ 性能趋势自动记录

**摘要检查**：
- ✅ 所有artifact包含SHA-256摘要
- ✅ 摘要验证SLA ≤ 250ms
- ✅ 超时自动记录到告警日志

---

## 4. v5.5 风险修复验证清单

### 修复验证#1：种子派生模式

**问题**：CUDA kernel使用threadIdx.x作为RNG种子导致不同block重复
**修复**：强制使用全局线程ID派生种子
**验证**：
```bash
# 检查代码中的种子派生模式
grep -rn "init_rng" src/ --include="*.cu"

# 应该看到：
# replay_seed + blockIdx.x * blockDim.x + threadIdx.x

# 不应该看到：
# threadIdx.x
# blockIdx.x（单独使用）
```

### 修复验证#2：动态grid/block维度

**问题**：动态计算grid/block维度导致不可重放
**修复**：所有维度从配置文件加载
**验证**：
```bash
# 检查动态维度计算
grep -rn "dim3.*grid" src/ --include="*.cu" --include="*.cpp"

# 应该看到：
# dim3 grid(config.grid_dim);
# dim3 block(config.block_dim);

# 不应该看到：
# dim3 grid(calculate_grid_size(...));
# int grid_dim = (n + block_dim - 1) / block_dim;
```

### 修复验证#3：Adapter强制路径

**问题**：直接include参考源头文件绕过adapter
**修复**：升级CI检测器为ERROR级别
**验证**：
```bash
# 运行增强检测器
./ci/check_crypto_reinvention_v5.5.sh

# 应该通过，不应该有：
# ERROR: Direct include of reference source headers
# ERROR: Direct calls to reference functions outside adapter namespace
```

### 修复验证#4：TDD时间戳验证

**问题**：测试失败证据可能在实现后创建
**修复**：Git时间戳比对
**验证**：
```bash
# 运行TDD门禁
./ci/tdd_gate_v5.5.sh

# 应该看到：
# ✓ Evidence timestamp verified (before implementation)
```

### 修复验证#5：预热阶段强制

**问题**：性能测试未预热导致结果不稳定
**修复**：强制3次预热+5次测量
**验证**：
```bash
# 运行基准测试
./scripts/run-benchmarks.sh 0 5

# 检查输出JSON
jq '.warmup.iterations' benchmarks/run_*.json
# 应该输出：3

jq '.warmup.note' benchmarks/run_*.json
# 应该包含："Warmup results not used in statistics"
```

### 修复验证#6：摘要SLA监控

**问题**：摘要验证超时未被监控
**修复**：超时自动记录告警日志
**验证**：
```bash
# 运行摘要验证
./ci/verify_all_digests_v5.5.sh

# 检查SLA违规日志
if [ -f logs/digest_sla_violations.log ]; then
    echo "Found SLA violations:"
    cat logs/digest_sla_violations.log
fi
```

---

## 5. v5.5 AI Agent执行承诺

作为AI Agent，我承诺严格执行以下v5.5增强约束：

### 确定性约束
- ✅ 所有RNG使用 `replay_seed + blockIdx.x * blockDim.x + threadIdx.x`
- ✅ 所有kernel launch使用config中的grid_dim/block_dim
- ✅ 配置文件版本必须为"5.5"
- ✅ 禁用所有非确定性API（clock64/time/random_device）

### TDD约束
- ✅ 先写失败测试，保存完整证据（包含时间戳和失败输出）
- ✅ 证据文件格式符合v5.5标准
- ✅ 确保证据Git时间戳早于实现代码
- ✅ Commit message引用证据文件

### 密码学约束
- ✅ 所有ECC/BigInt操作通过 `puzzle71::adapters::` 命名空间
- ✅ 禁止直接include snapshots/头文件（仅adapter可以）
- ✅ 禁止任何形式的密码学重新实现
- ✅ 每个crypto函数标注@sot_ref引用

### 性能约束
- ✅ 基准测试执行3次预热（结果丢弃）
- ✅ 基准测试执行5次测量
- ✅ 验证吞吐量≥baseline的95%
- ✅ 自动更新性能趋势

### 摘要约束
- ✅ 所有artifact包含SHA-256摘要
- ✅ 摘要验证SLA≤250ms
- ✅ 超时自动记录告警
- ✅ CI验证所有摘要完整性

### 提交前验证
- ✅ 运行所有v5.5检测器
- ✅ 确保所有测试通过
- ✅ 验证确定性重放
- ✅ 验证性能基线
- ✅ 检查摘要SLA

---

## 6. 文档版本与变更记录

| 版本 | 日期 | 主要变更 | 作者 |
|------|------|---------|------|
| v5.5 | 2025-01-20 | **风险修复增强版** | AI Agent Review Team |
|      |            | - 修复#1: 强制种子派生模式（全局线程ID） | |
|      |            | - 修复#2: 禁止动态grid/block维度计算 | |
|      |            | - 修复#3: Adapter路径强制（升级为ERROR） | |
|      |            | - 修复#4: TDD时间戳严格验证 | |
|      |            | - 修复#5: 预热阶段强制执行（3+5协议） | |
|      |            | - 修复#6: 摘要SLA监控告警 | |
|      |            | - 新增：配置文件版本检查 | |
|      |            | - 新增：种子派生公式验证 | |
|      |            | - 新增：性能趋势自动记录 | |
|      |            | - 新增：SLA违规日志 | |
|      |            | - 增强：CI检测器覆盖更多模式 | |
|      |            | - 增强：TDD证据格式标准化 | |
| v5.0 | 2025-09-30 | **Puzzle71Solver铁笼协议初始版** | Constraint Team |
|      |            | - 针对CUDA密码学项目定制 | |
|      |            | - 四大铁律（DETERMINISM/TEST-FIRST/NO-REINVENTION/ZERO-TOLERANCE） | |

---

**文档结束。所有规则强制执行，违反任何一条导致立即回滚。**

**v5.5 CRITICAL**: 本版本修复了v5.0中识别的6个关键风险点。所有AI Agent和开发者必须立即升级到v5.5协议。配置文件版本必须更新为"5.5"，否则程序将拒绝启动。

**记住v5.5核心改进**：
1. 种子派生必须使用全局线程ID
2. Grid/block维度必须从配置加载
3. Adapter路径强制执行（ERROR级别）
4. TDD时间戳严格验证
5. 预热阶段强制执行（3+5协议）
6. 摘要SLA监控告警

**v5.5执行口号**：确定性、测试优先、零重新实现、零性能退化、零摘要缺失！

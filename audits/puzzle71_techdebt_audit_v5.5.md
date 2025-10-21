# Puzzle71Solver 技术债务审计报告（基于 v5.5 铁笼协议）

## 1. 审计范围与方法

- 规范依据：`docs/puzzle71_constraints_v5.5.md:20` 明确禁止动态 grid/block，强制适配器路径与性能门槛；`docs/puzzle71_constraints_v5.5.md:42` 规定配置完整性校验与确定性要求。
- 代码范围：`src/puzzle71_kernel.cu`、`src/kernels/hash_kernel.cu`、`src/KeyhuntCore/common/*.cuh` 以及 `src/config/puzzle71_config.cpp`、`src/extracted/bitcrack/*`。
- 支撑资料：既有重复度分析档 `audits/duplication_metrics.json:163` 与性能瓶颈档 `docs/analysis/PERFORMANCE_BOTTLENECK_ANALYSIS_2025-10-18.md:1`。
- 工具与方法：使用 `rg`/`sed` 定点审阅 + 手动比对统一模块与 BitCrack 代码，重点关注重复度、性能与算法能效。

## 2. 关键发现摘要（按严重度排序）

- **P0/阻塞**：动态 launch 配置仍然依赖运行时设备属性计算，直接违反 v5.5 禁止动态 grid/block 的硬性约束（`docs/puzzle71_constraints_v5.5.md:20` 对照 `src/puzzle71_kernel.cu:200`）。
- **P0/阻塞**：统一 ECC 模块仍保留占位实现，`BeginBatchPointAdd`/`CompleteBatchPointAdd` 未执行任何椭圆曲线算术，导致批量逆元链路实质失效（`src/KeyhuntCore/common/ecc_operations.cuh:246`，`src/KeyhuntCore/common/ecc_operations.cuh:290`）。
- **P0/阻塞**：`keyhunt::legacy::doBatchInverse` 空实现，一旦调用将破坏确定性 replay（`src/KeyhuntCore/common/legacy_adapter.cuh:143`）。
- **P0/阻塞**：核心 kernel 直接 include BitCrack 头文件，未经过 adapter 路径，违背适配器强制路径要求（`docs/puzzle71_constraints_v5.5.md:24`，`src/puzzle71_kernel.cu:25`）。
- **P1/高**：候选扫描逻辑在三处并行实现，修复需重复劳动且增加缺陷传播风险（`src/puzzle71_kernel.cu:92`、`src/kernels/hash_kernel.cu:94`、`src/KeyhuntCore/kernels/hash_separated.cu:57`）。
- **P1/高**：`ReadBigInt`/`WriteBigInt` 每次调用都伴随两次 `__syncthreads`，在大批量场景下带来严重同步开销（`src/KeyhuntCore/common/ecc_operations.cuh:45`、`src/KeyhuntCore/common/ecc_operations.cuh:95`）。
- **P1/高**：性能档显示 GPU 利用率仅 35-38%，远低于 v5.5 要求的 ≥90%，性能缺口仍达 3.6×（`docs/analysis/PERFORMANCE_BOTTLENECK_ANALYSIS_2025-10-18.md:5`）。
- **P2/中**：配置解析缺少必填校验，无法保证 replay 所需参数完整，易触发运行时不确定行为（`docs/puzzle71_constraints_v5.5.md:42` 对照 `src/config/puzzle71_config.cpp:57`）。

## 3. 代码重复度分析

- 🎯 **候选扫描循环重复**  
  - 位置：`src/puzzle71_kernel.cu:92`、`src/kernels/hash_kernel.cu:94`、`src/KeyhuntCore/kernels/hash_separated.cu:57`。  
  - 表现：哈希计算、比对与候选写入逻辑逐字复制，唯一差异仅是注释与局部变量命名。  
  - 风险：修复哈希溢出/寄存器调优需三处同步，极易出现版本漂移。建议抽象为单一 `__device__` 函数并在三处调用，结合单元测试确保一致性。
- 🎯 **BitCrack 函数平行存在**  
  - 位置：`src/KeyhuntCore/common/ecc_operations.cuh:45` 与 `src/extracted/bitcrack/cudaMath/secp256k1.cuh:120`。  
  - 表现：统一模块直接复制 BitCrack 的 `readInt_Optimized` 等实现，没有真正复用，导致维护两套相同代码。  
  - 风险：当 BitCrack 上游修补 bug 时需手动同步，建议将 BitCrack 源作为单一实现并通过适配器导出。
- 🎯 **已有重复度档未落地改进**  
  - 参考 `audits/duplication_metrics.json:163`，虽然报告显示“duplication_percentage: 0.0”，但目前代码仍保留上述重复段，表示重构未完成或回归。

## 4. 性能瓶颈检测

- ⚡ **动态共享内存浪费并降低并发**：`LaunchFusedKernel` 为每个 block 分配 `block.x * 8 * sizeof(unsigned int)` 动态共享内存，但 `DoPuzzle71Iteration` 内部并未复用共享缓存，导致 SM 能驻留的 block 数量下降（`src/puzzle71_kernel.cu:309`）。建议改为静态常量或仅在批量阶段申请。
- ⚡ **同步开销巨大**：`ReadBigInt` 与 `WriteBigInt` 每次执行都会执行两次 `__syncthreads`，points_per_thread=512 时单 kernel 至少触发 3000+ 次同步（`src/KeyhuntCore/common/ecc_operations.cuh:55`）。需改为按 warp 粒度或使用向量化加载减少 barrier。
- ⚡ **低 GPU Occupancy**：性能分析档指出每 SM 活跃 warp 仅 12-15，GPU 利用率 35-38%，主要受限于 points_per_thread 与 grid 配置（`docs/analysis/PERFORMANCE_BOTTLENECK_ANALYSIS_2025-10-18.md:32`）。现行动态计算方式反而难以满足 v5.5 静态约束。
- ⚡ **批量逆元链路失效**：统一 ECC 模块未实现 Montgomery 逆元，`BeginBatchPointAdd` 只将数据写回链，而 `CompleteBatchPointAdd` 直接返回原值（`src/KeyhuntCore/common/ecc_operations.cuh:262`、`src/KeyhuntCore/common/ecc_operations.cuh:310`）。这会让 `doBatchInverse` 形同虚设，导致 ECC kernel 实际上退化为逐点复制，严重拖慢吞吐。
- ⚡ **注册审计标志未配合优化**：`g_register_audit` 仅打印寄存器数，却无自动调节策略（`src/puzzle71_kernel.cu:299`），无法辅助定位寄存器压力源。

## 5. 算法能效评估

- 现状：RTX 2080 Ti 实测 279 Mkeys/s（`docs/analysis/PERFORMANCE_BOTTLENECK_ANALYSIS_2025-10-18.md:5`）≈ 1.1 Mkeys/s·W（以 250W 粗估）。
- 目标：协议要求 ≥1000 Mkeys/s，折算能效 ≥4 Mkeys/s·W（`docs/puzzle71_constraints_v5.5.md:70` 性能门槛条款）。
- 缺口：吞吐需提升 3.6×，能效需提升 ≥3.6×，主要受低占用率、重复加载与缺失的批量逆元影响。

## 6. 优化方向与建议

1. **统一候选扫描模块**  
   - 将哈希计算与候选发射提炼为单一 `__device__` 模板，供 fused/separated kernel 共享，减少重复并便于调优。
2. **严格静态 launch 配置**  
   - 在配置加载阶段校验 grid/block/points_per_thread，并禁止运行时覆盖，满足 `docs/puzzle71_constraints_v5.5.md:20` 要求；必要时提供离线配置生成脚本。
3. **完成 ECC 批量实现**  
   - 补齐 `BeginBatchPointAdd`、`CompleteBatchPointAdd` 与 `doBatchInverse` 的实际曲线算术逻辑，确保与 BitCrack 行为等价后再考虑优化。
4. **适配器化 BitCrack 依赖**  
   - 通过 `src/utils/*_adapter.h` 暴露 BitCrack 功能，在 kernel 中移除直接 include，满足适配器强制路径（`docs/puzzle71_constraints_v5.5.md:24`）。
5. **降低同步开销与共享内存占用**  
   - 改用 `int4`/`uint4` 向量化读写，并将同步范围降至 warp 级；仅在批量阶段使用共享内存并复用缓存。
6. **性能监控闭环**  
   - 结合 `g_register_audit` 加入自动告警与配置建议，辅以 Nsight Compute 基线，验证 occupancy≥70%、共享内存利用率≥70%。

## 7. 风险评估与后续行动

- **短期（1-2 天）**  
  1. 修正配置加载逻辑，加入必填字段校验与静态 launch 限制。  
  2. 屏蔽 `legacy::doBatchInverse` 空实现，避免误用。  
  3. 起草统一候选扫描函数，保持行为一致并新增单测。
- **中期（本迭代内）**  
  1. 完成 ECC 批量算术迁移，验证 GPU/CPU 一致性。  
  2. 调整共享内存策略，基于 Nsight 数据验证 occupancy 与吞吐提升。  
  3. 引入性能回归测试，确保 ≥3 次预热 + 5 次测量符合规范（`docs/puzzle71_constraints_v5.5.md:32`）。
- **风险提示**  
  - 在 ECC 模块完善前，任何性能结论可能偏离真实水平。  
  - 若继续依赖动态配置，CI 按 v5.5 规则应直接阻塞合并。  
  - 抽象合并后需补充单元/集成测试，否则存在隐形功能回归风险。

---

> 审计结论：当前版本未满足铁笼协议 v5.5 的 P0 强制要求，必须优先修复配置确定性、批量 ECC 实现与重复代码问题，随后才能展开性能调优。完成以上工作并通过自动化验证后，方可推进 1000 Mkeys/s 目标的性能调优阶段。

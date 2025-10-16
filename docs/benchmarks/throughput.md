# Puzzle71Solver Throughput Benchmark Notes

## Overview
- 使用 `scripts/run_performance_benchmark.sh` 运行 GPU 吞吐基准；脚本默认读取 `./build/Puzzle71Solver` 并接受 `--gpu`, `--keyspace`, `--duration` 等参数。
- 每次运行会生成：
  - `benchmarks/<run_id>_*.json`：每块 GPU 的结构化总结（schema_version=1）。
  - `telemetry/runs/<run_id>.jsonl`：逐批次遥测（NDJSON），包含初始化资料与 batch 指标。
  - `benchmarks/latest.json`：聚合所有 GPU 的结果快照。

## 初始化遥测（Initialization Profile）
- 单次 warm-up 构建由 `InitProfileBuilder` 负责，遥测条目结构如下：

```json
{
  "type": "initialization_profile",
  "gpu_arch": "RTX3090",
  "warmup_strategy": "SinglePass",
  "host_precompute_ns": 7142030,
  "upload_duration_ns": 158903421,
  "table_checksum": "d53e70c8c371438503f093587858e2d1c246c7a3be4db69d47da706228187203",
  "init_batch_id": 1,
  "init_completed_at": "2025-10-10T06:32:44Z",
  "small_keyspace_bypass": false
}
```

- `small_keyspace_bypass = true` 表示本批次（≤4,096 points）仅更新 private keys，复用现有增量表。
- checksum 可用于验证初始化数据是否一致；失败时需 rerun determinism gate。

## Batch 遥测字段补充
- `keys_per_sec`：单批吞吐；`processed_keys_hex` 保留十六进制区间长度。
- `memory_used_mb` / `gpu_idle_ms`：由运行时遥测与 NVML 快照共同提供，Prometheus 规则位于 `telemetry/prometheus/puzzle71_metrics.yml`。
- JSONL 文件中若出现 dropped candidates，应同步检查 `logs/ci/security_scan.json` 与 Parity 验证。

## 基准脚本用法示例
```bash
# RTX 3090，手动指定输出目录与持续时间
scripts/run_performance_benchmark.sh \
  --gpu RTX3090:0 \
  --keyspace 0x1:0x100000 \
  --duration 120 \
  --output-dir benchmarks/custom \
  --telemetry-dir telemetry/runs
```

## 验证要点
- 初始化阶段必须 ≤250ms 单次 warm-up；检查 `host_precompute_ns + upload_duration_ns` 是否符合目标。
- 小 keyspace（例如 smoke test）应出现 `small_keyspace_bypass=true`，表明未重新构建 256 步暖机。
- 调整脚本时需更新本文档与 `specs/002-bitcrack-256-gpu/tasks.md` 中的 Phase 3 说明。

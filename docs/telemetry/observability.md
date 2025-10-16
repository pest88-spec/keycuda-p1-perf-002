# Observability Guide: Puzzle71Solver Throughput Remediation

## 1. 遥测数据流概览
- **运行时 JSONL**：`telemetry/runs/<run_id>.jsonl`，由 `SlidingWindowAggregator` 与 `PerfLogger` 按批次写入，关键字段：
  - `keys_processed`、`elapsed_ms` → 计算实时吞吐。
  - `gpu_idle_ms`、`memory_used_mb` → GPU 负载诊断。
  - `initialization_profile` → 单次 warm-up 校验（包含 `small_keyspace_bypass` 标记）。
- **基准摘要**：`benchmarks/<run_id>_*.json` 由 `run_performance_benchmark.sh` 生成，聚合 NVML、dmon 指标及 solver 命令行。
- **守门报告**：`logs/ci/performance_gate.json`，`ci/performance_gate.sh` 对比最新基线后生成，用于 CI Failure 排查。
- **警报报告**：`logs/alerts/performance_alert.json`，`scripts/alerts/performance_alert.py` 在告警发送后写入（含 Slack/Email 结果与 baseline provenance）。

## 2. 关键指标
| 指标 | 描述 | 来源 |
|------|------|------|
| `puzzle71_throughput_keys_sec` | 当前批次或滑动窗口平均吞吐 | `telemetry/metrics_registry.cpp` |
| `puzzle71_gpu_idle_ms` | GPU 空闲时间（ms） | 同上 |
| `puzzle71_parity_overlap_pct` | Parity 与 Kernel 并行程度 | 同上 |
| `telemetry.telemetry_duration_sec` | 本次运行累计秒数 | `run_performance_benchmark.sh` | 
| `telemetry.avg_throughput_keys_sec` | Telemetry 计算的平均吞吐 | `run_performance_benchmark.sh` |
| `nvml_snapshot.temperature.gpu` | GPU 温度 | `scripts/tools/nvml_snapshot.py` |

> Prometheus 导出：默认 `telemetry/metrics_registry.cpp` 将指标暴露为文本，可通过 `scripts/tools/prometheus_exporter.py`（可选）挂载 HTTP。

## 3. 基线与阈值
- 基线写入位置：`telemetry/baselines/<GPU>/<timestamp>_<baseline_id>.json`。
  - 若 `thermal_state = Throttled` 或警报元数据标记热降级，`PerfStore` 会将文件写入 `telemetry/baselines/quarantine/<GPU>/`，主流程不会消费。
- 阈值配置：`ops/thresholds.yaml`（可由环境变量 `PUZZLE71_THRESHOLD_FILE` 覆盖）。
  ```yaml
  window_size: 3
  throughput_targets:
    RTX3090: 2000000000
  max_parity_error_rate: 0.0001
  ```
  - `SessionController` 与 `SlidingWindowAggregator` 启动时自动读取。
  - 更新后需重新运行 `ci/performance_gate.sh` 与 `puzzle71_tests` 验证。

## 4. 告警流水线
1. 守门或调优流程将异常信息写入 `telemetry/alerts/<date>.jsonl`。
2. 执行：
   ```bash
   scripts/alerts/performance_alert.py \
     --alert-file telemetry/alerts/<alert>.json \
     --baseline-dir telemetry/baselines \
     --dry-run --slack-webhook https://hooks.slack.com/services/... \
     --email-to ops@example.com --smtp-server smtp.example.com
   ```
3. dry-run 下消息输出到控制台并生成报告；移除 `--dry-run` 即可发送真实通知。
4. Slack/Email 推送后需在 runbook 中记录处理结果并更新 `telemetry/baselines`。

## 5. 仪表盘建议
- **GPU 性能面板**：吞吐、GPU Idle、Parity Overlap、NVML 温度/功率。
- **守门历史**：读取 `logs/ci/performance_gate.json` 的 `checks[]`，展示每次 CI 比对的吞吐比例与Parity错误率。
- **告警追踪**：从 `logs/alerts/performance_alert.json` 中提取 `breach_type`、`baseline_id`、`thermal_state` 绘制时序。

## 6. 故障与自愈
| 症状 | 排查步骤 |
|------|----------|
| CI 性能守门失败 | 检查 `logs/ci/performance_gate.json` → 确认 `slowdown_injected` 或真实退化；若是真实退化，导出最新基准并归档调查。 |
| 告警频繁触发 | 检查 `ops/thresholds.yaml` 是否过于严格；使用 `ci/performance_gate.sh --inject-slowdown` 再现阈值敏感度。 |
| JSON 解析错误 | 确认 `third_party/python_packages` 中安装 `pyyaml==6.0.1`（运行 `scripts/setup-yaml.sh`）。 |
| 基线丢失 | 查看 `telemetry/baselines/quarantine/<GPU>/` 是否存在大量隔离样本，若是硬件降频，需联系运维更换设备。 |

## 7. MCP 工具记录
- **`Sequential Thinking`**：规划基线刷新及守门脚本的执行顺序，确保在阈值调整后编排新的验证计划。
- **`Serena`**：定位 `PerfStore` / `SessionController` / `performance_alert.py` 相关代码，当需要更新 JSON 模式或插入新指标时使用。
- **`Context7`**：查询 CUDA/NVML API、Prometheus Exporter 参考文档。
- **`DuckDuckGo`**：获取 GPU 热管理最佳实践，务必记录来源用于审计。

保持以上数据链路畅通，可在性能回归、温度异常或调优失败时迅速定位问题并完成闭环处理。

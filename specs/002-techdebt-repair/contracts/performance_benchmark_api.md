# Performance Benchmark API Contract

**Version**: 1.0.0
**Created**: 2025-10-20
**Purpose**: API contract for performance benchmarking and validation system

## Overview

This API defines the contract for running performance benchmarks, collecting metrics, and validating against established baselines for the Puzzle71 technical debt repair system.

## Core Operations

### 1. Execute Performance Benchmark

**Endpoint**: `POST /api/v1/benchmarks/execute`

**Request Body**:
```json
{
  "benchmark_type": "memory_efficiency" | "gpu_utilization" | "throughput" | "latency",
  "gpu_device_id": 0,
  "test_duration_seconds": 300,
  "warmup_iterations": 3,
  "measurement_iterations": 5,
  "configuration": {
    "grid_dim": 1024,
    "block_dim": 256,
    "batch_size": 1000000
  },
  "baseline_id": "baseline_rtx3080_v1.0"
}
```

**Response**:
```json
{
  "benchmark_id": "bench_20251020_143022_a1b2c3",
  "status": "running" | "completed" | "failed",
  "started_at": "2025-10-20T14:30:22Z",
  "estimated_completion": "2025-10-20T14:35:22Z",
  "progress_percentage": 45.2
}
```

### 2. Get Benchmark Results

**Endpoint**: `GET /api/v1/benchmarks/{benchmark_id}/results`

**Response**:
```json
{
  "benchmark_id": "bench_20251020_143022_a1b2c3",
  "status": "completed",
  "completed_at": "2025-10-20T14:35:18Z",
  "duration_seconds": 296,
  "metrics": {
    "memory_efficiency": {
      "measured_value": 94.2,
      "baseline_value": 90.0,
      "variance_percentage": 4.7,
      "status": "PASS"
    },
    "gpu_utilization": {
      "measured_value": 78.5,
      "baseline_value": 70.0,
      "variance_percentage": 12.1,
      "status": "PASS"
    },
    "throughput": {
      "measured_value": 2100000000,
      "baseline_value": 2000000000,
      "variance_percentage": 5.0,
      "status": "PASS"
    },
    "synchronization_overhead": {
      "measured_value": 45.2,
      "baseline_value": 100.0,
      "variance_percentage": -54.8,
      "status": "PASS"
    }
  },
  "overall_status": "PASS",
  "performance_regression_detected": false,
  "detailed_measurements": [
    {
      "iteration": 1,
      "memory_efficiency": 93.8,
      "gpu_utilization": 77.2,
      "throughput": 2080000000,
      "synchronization_overhead": 47.1
    }
  ],
  "nsight_metrics": {
    "global_load_efficiency": 91.2,
    "shared_memory_efficiency": 96.8,
    "warp_execution_efficiency": 89.4,
    "occupancy": 76.3
  }
}
```

### 3. Compare Against Baseline

**Endpoint**: `POST /api/v1/benchmarks/compare`

**Request Body**:
```json
{
  "current_benchmark_id": "bench_20251020_143022_a1b2c3",
  "baseline_benchmark_id": "baseline_rtx3080_v1.0",
  "comparison_threshold_percentage": 5.0
}
```

**Response**:
```json
{
  "comparison_id": "comp_20251020_143522_d4e5f6",
  "summary": {
    "overall_status": "PASS",
    "metrics_compared": 4,
    "metrics_passed": 4,
    "metrics_failed": 0,
    "regression_detected": false
  },
  "detailed_comparison": [
    {
      "metric_name": "memory_efficiency",
      "current_value": 94.2,
      "baseline_value": 90.0,
      "difference_percentage": 4.7,
      "threshold_percentage": 5.0,
      "status": "PASS"
    }
  ]
}
```

## Data Schemas

### BenchmarkConfiguration
```json
{
  "type": "object",
  "required": ["benchmark_type", "gpu_device_id"],
  "properties": {
    "benchmark_type": {
      "type": "string",
      "enum": ["memory_efficiency", "gpu_utilization", "throughput", "latency"]
    },
    "gpu_device_id": {
      "type": "integer",
      "minimum": 0
    },
    "test_duration_seconds": {
      "type": "integer",
      "minimum": 60,
      "default": 300
    },
    "warmup_iterations": {
      "type": "integer",
      "minimum": 0,
      "default": 3
    },
    "measurement_iterations": {
      "type": "integer",
      "minimum": 1,
      "default": 5
    },
    "configuration": {
      "type": "object",
      "properties": {
        "grid_dim": {"type": "integer", "minimum": 1},
        "block_dim": {"type": "integer", "minimum": 1, "maximum": 1024},
        "batch_size": {"type": "integer", "minimum": 1}
      }
    },
    "baseline_id": {
      "type": "string",
      "pattern": "^[a-zA-Z0-9_]+$"
    }
  }
}
```

### PerformanceMetrics
```json
{
  "type": "object",
  "properties": {
    "memory_efficiency": {
      "type": "object",
      "properties": {
        "measured_value": {"type": "number", "minimum": 0, "maximum": 100},
        "baseline_value": {"type": "number", "minimum": 0, "maximum": 100},
        "variance_percentage": {"type": "number"},
        "status": {"type": "string", "enum": ["PASS", "FAIL", "WARNING"]}
      }
    },
    "gpu_utilization": {
      "type": "object",
      "properties": {
        "measured_value": {"type": "number", "minimum": 0, "maximum": 100},
        "baseline_value": {"type": "number", "minimum": 0, "maximum": 100},
        "variance_percentage": {"type": "number"},
        "status": {"type": "string", "enum": ["PASS", "FAIL", "WARNING"]}
      }
    },
    "throughput": {
      "type": "object",
      "properties": {
        "measured_value": {"type": "number", "minimum": 0},
        "baseline_value": {"type": "number", "minimum": 0},
        "variance_percentage": {"type": "number"},
        "status": {"type": "string", "enum": ["PASS", "FAIL", "WARNING"]}
      }
    },
    "synchronization_overhead": {
      "type": "object",
      "properties": {
        "measured_value": {"type": "number", "minimum": 0},
        "baseline_value": {"type": "number", "minimum": 0},
        "variance_percentage": {"type": "number"},
        "status": {"type": "string", "enum": ["PASS", "FAIL", "WARNING"]}
      }
    }
  }
}
```

## Error Handling

### Error Response Format
```json
{
  "error": {
    "code": "BENCHMARK_EXECUTION_FAILED",
    "message": "Failed to execute benchmark: Insufficient GPU memory",
    "details": {
      "gpu_id": 0,
      "required_memory_gb": 8.5,
      "available_memory_gb": 6.2
    },
    "timestamp": "2025-10-20T14:30:45Z",
    "request_id": "req_20251020_143022_a1b2c3"
  }
}
```

### Error Codes
- `BENCHMARK_EXECUTION_FAILED`: Benchmark could not be executed
- `BASELINE_NOT_FOUND`: Specified baseline does not exist
- `INVALID_CONFIGURATION`: Benchmark configuration is invalid
- `GPU_NOT_AVAILABLE`: Specified GPU device is not available
- `INSUFFICIENT_RESOURCES`: Not enough GPU memory or compute resources
- `TIMEOUT_EXCEEDED`: Benchmark execution exceeded time limit

## Performance Thresholds

### Success Criteria
- Memory efficiency ≥ 90.0% (target 95.0%)
- GPU utilization ≥ 70.0% (target 80.0%)
- Synchronization overhead ≤ 50.0% of baseline
- Overall throughput ≥ 95.0% of baseline

### Warning Conditions
- Memory efficiency between 85.0% and 90.0%
- GPU utilization between 60.0% and 70.0%
- Synchronization overhead between 50.0% and 75.0% of baseline
- Overall throughput between 90.0% and 95.0% of baseline

### Failure Conditions
- Memory efficiency < 85.0%
- GPU utilization < 60.0%
- Synchronization overhead > 75.0% of baseline
- Overall throughput < 90.0% of baseline
- Any benchmark execution error

## Rate Limiting

- Maximum 10 concurrent benchmark executions per GPU device
- Maximum 100 benchmark requests per minute per client
- Benchmark results cached for 24 hours for identical configurations

## Security Considerations

- All benchmark execution requires authentication
- GPU device access restricted to authorized users
- Performance data encrypted in transit and at rest
- Audit logging for all benchmark operations
- Resource quotas prevent denial of service
# Performance Baseline API Contract

**Version**: 1.0.0
**Date**: 2025-10-17
**Purpose**: API specification for performance baseline management

## Overview

This API defines the contract for managing SHA-256 protected performance baselines used in zero-tolerance regression detection for the Puzzle71Solver CUDA refactoring project.

## Data Structures

### PerformanceBaseline

```json
{
  "gpu_name": "string",
  "baseline_version": "string",
  "target_throughput": "uint64",
  "min_throughput": "uint64",
  "memory_efficiency": "double",
  "max_registers": "uint32",
  "min_occupancy": "double",
  "sha256_digest": "string",
  "created_at": "timestamp"
}
```

### BaselineComparisonResult

```json
{
  "gpu_name": "string",
  "baseline_version": "string",
  "current_throughput": "uint64",
  "target_throughput": "uint64",
  "min_throughput": "uint64",
  "performance_ratio": "double",
  "is_regression": "boolean",
  "memory_efficiency": "double",
  "register_usage": "uint32",
  "occupancy_rate": "double",
  "comparison_timestamp": "timestamp"
}
```

## API Endpoints

### Load Baseline

**Endpoint**: `GET /api/baselines/{gpu_name}`

**Response**:
```json
{
  "status": "success|error",
  "data": {
    "baseline": PerformanceBaseline,
    "is_valid": "boolean",
    "digest_verified": "boolean"
  },
  "error": "string|null"
}
```

**Error Codes**:
- `404`: Baseline not found for specified GPU
- `422`: Baseline file corrupted (SHA-256 mismatch)
- `500`: Internal server error

### Update Baseline

**Endpoint**: `PUT /api/baselines/{gpu_name}`

**Request Body**:
```json
{
  "baseline": PerformanceBaseline,
  "approval_required": "boolean",
  "approver": "string|null"
}
```

**Response**:
```json
{
  "status": "success|pending_approval|error",
  "data": {
    "baseline_id": "string",
    "version": "string",
    "sha256_digest": "string",
    "requires_approval": "boolean"
  },
  "error": "string|null"
}
```

**Business Rules**:
- Performance improvements require manual approval
- SHA-256 digest automatically computed and stored
- Old baseline versions archived for audit trail

### Compare Performance

**Endpoint**: `POST /api/baselines/{gpu_name}/compare`

**Request Body**:
```json
{
  "current_metrics": {
    "throughput": "uint64",
    "memory_efficiency": "double",
    "register_usage": "uint32",
    "occupancy_rate": "double"
  },
  "test_duration_seconds": "uint32",
  "sample_count": "uint32"
}
```

**Response**:
```json
{
  "status": "success|error",
  "data": {
    "comparison": BaselineComparisonResult,
    "regression_detected": "boolean",
    "performance_gate_passed": "boolean"
  },
  "error": "string|null"
}
```

**Regression Detection Rules**:
- `regression_detected = true` if `current_throughput < min_throughput`
- `performance_gate_passed = false` if any metric below minimum thresholds
- Automatic CI/CD gate failure on regression detection

### List Baselines

**Endpoint**: `GET /api/baselines`

**Query Parameters**:
- `gpu_name` (optional): Filter by specific GPU
- `version` (optional): Filter by baseline version
- `limit` (optional): Maximum number of results (default: 50)

**Response**:
```json
{
  "status": "success|error",
  "data": {
    "baselines": [PerformanceBaseline],
    "total_count": "uint32",
    "page_info": {
      "current_page": "uint32",
      "total_pages": "uint32",
      "has_next": "boolean"
    }
  },
  "error": "string|null"
}
```

## Validation Rules

### Input Validation

1. **GPU Name**:
   - Must match supported CUDA architectures
   - Format: "RTX 2080 Ti", "RTX 3090", "H20", "A100"
   - Case-insensitive matching

2. **Performance Metrics**:
   - `target_throughput`: Must be > 0 and realistic for GPU model
   - `min_throughput`: Must be ≥ 95% of target_throughput
   - `memory_efficiency`: Must be between 0.0 and 1.0
   - `max_registers`: Must be ≤ 128 (CUDA limit)
   - `min_occupancy`: Must be between 0.0 and 1.0

3. **SHA-256 Digest**:
   - Must be valid 64-character hexadecimal string
   - Computed over entire baseline JSON object
   - Verification required before baseline acceptance

### Output Validation

1. **Response Format**:
   - All API responses follow consistent structure
   - Error messages provide actionable information
   - Timestamps in ISO 8601 format

2. **Performance Comparison**:
   - Ratios calculated with double precision
   - Regression detection with zero-tolerance policy
   - Detailed metrics for debugging optimization issues

## Error Handling

### Standard Error Response

```json
{
  "status": "error",
  "error": {
    "code": "string",
    "message": "string",
    "details": "object|null"
  }
}
```

### Error Codes

| Code | HTTP Status | Description |
|------|-------------|-------------|
| `BASELINE_NOT_FOUND` | 404 | No baseline exists for specified GPU |
| `INVALID_GPU_NAME` | 400 | GPU name not in supported list |
| `DIGEST_MISMATCH` | 422 | SHA-256 digest verification failed |
| `PERFORMANCE_REGRESSION` | 409 | Current performance below minimum threshold |
| `APPROVAL_REQUIRED` | 403 | Baseline update requires explicit approval |
| `INVALID_METRICS` | 400 | Performance metrics outside valid ranges |
| `INTERNAL_ERROR` | 500 | Unexpected server error |

## Security Considerations

### Access Control

1. **Read Operations**:
   - Public access for baseline retrieval
   - No authentication required for GET operations

2. **Write Operations**:
   - Baseline updates require authentication
   - Approval workflow for performance improvements
   - Audit trail for all modifications

### Integrity Protection

1. **SHA-256 Verification**:
   - All baseline files cryptographically protected
   - Automatic digest verification on load
   - Tamper detection on every access

2. **Audit Trail**:
   - Complete history of baseline changes
   - User attribution for all modifications
   - Immutable log with cryptographic protection

## Performance Requirements

### Response Time Targets

- **Baseline Load**: < 100ms (file system cached)
- **Performance Comparison**: < 50ms (in-memory calculation)
- **Baseline Update**: < 500ms (including digest computation)

### Throughput Targets

- **Concurrent Comparisons**: 100+ simultaneous comparisons
- **Baseline Storage**: 1000+ baseline versions supported
- **Query Performance**: < 10ms for indexed queries

## Integration Points

### CI/CD Integration

```bash
# Performance gate check
curl -X POST "http://localhost:8080/api/baselines/RTX%203090/compare" \
  -H "Content-Type: application/json" \
  -d @current_performance.json

# Response handling
if [[ $(jq .data.regression_detected) == "true" ]]; then
  echo "Performance regression detected - failing build"
  exit 1
fi
```

### Monitoring Integration

```bash
# Real-time performance monitoring
while true; do
  curl -X GET "http://localhost:8080/api/baselines/RTX%203090" | \
    jq '.data.baseline.target_throughput'
  sleep 60
done
```

## Testing Requirements

### Unit Tests

1. **Digest Verification**:
   - Test SHA-256 computation and verification
   - Test tamper detection scenarios
   - Test baseline integrity validation

2. **Performance Comparison**:
   - Test regression detection logic
   - Test boundary conditions (exactly at minimum)
   - Test metric calculation accuracy

### Integration Tests

1. **API Endpoints**:
   - Test all endpoints with valid/invalid inputs
   - Test error handling and response codes
   - Test concurrent access scenarios

2. **File System Operations**:
   - Test baseline file creation and loading
   - Test concurrent file access
   - Test file permission handling

### Performance Tests

1. **Load Testing**:
   - 1000+ concurrent baseline comparisons
   - Sustained API throughput measurement
   - Memory usage monitoring under load

2. **Stress Testing**:
   - Large number of baseline versions
   - High-frequency update operations
   - Resource exhaustion scenarios
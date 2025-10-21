# Data Model: Puzzle71Solver CUDA Technical Debt Elimination

**Date**: 2025-10-17
**Purpose**: Data entities and relationships for CUDA refactoring project
**Input**: Feature specification and research findings

## Core Data Entities

### Performance Baseline Entity

**Purpose**: SHA-256 protected performance baselines for regression detection

```cpp
struct PerformanceBaseline {
    std::string gpu_name;           // GPU model identifier
    std::string baseline_version;   // Version of the baseline
    uint64_t target_throughput;     // Target keys per second
    uint64_t min_throughput;        // Minimum acceptable throughput (95% of target)
    double memory_efficiency;       // Memory access efficiency target (>90%)
    uint32_t max_registers;         // Maximum registers per thread (≤40)
    double min_occupancy;           // Minimum GPU occupancy (≥80%)
    std::string sha256_digest;      // SHA-256 of baseline data for integrity
    std::chrono::system_clock::time_point created_at;
};
```

**Storage**: `benchmarks/baselines/{gpu_name}.json`

### Code Duplication Metrics

**Purpose**: Track code duplication elimination progress

```cpp
struct DuplicationMetrics {
    std::string function_name;      // Function being tracked
    uint32_t duplicate_count;       // Number of duplicate instances
    uint32_t total_lines;           // Total lines across all duplicates
    std::vector<std::string> locations; // File paths of duplicate instances
    bool is_consolidated;           // True if consolidated into unified module
    std::string unified_location;   // Location of unified implementation
};
```

**Storage**: `audits/duplication_metrics.json`

### Kernel Performance Profile

**Purpose**: Real-time kernel performance monitoring

```cpp
struct KernelProfile {
    std::string kernel_name;        // Kernel identifier
    uint32_t threads_per_block;     // CUDA threads per block configuration
    uint32_t blocks_per_grid;       // CUDA blocks per grid configuration
    uint32_t registers_per_thread;  // Register usage per thread
    double shared_memory_usage;     // Shared memory usage in bytes
    double execution_time_ms;       // Average execution time in milliseconds
    double occupancy_rate;          // GPU occupancy percentage
    double memory_bandwidth_util;   // Memory bandwidth utilization percentage
    std::chrono::system_clock::time_point measured_at;
};
```

**Storage**: `telemetry/kernel_profiles_{timestamp}.jsonl`

### Checkpoint Data

**Purpose**: Enhanced checkpoint system for refactored architecture

```cpp
struct CheckpointData {
    uint64_t private_key_start;     // Starting private key in range
    uint64_t private_key_end;       // Ending private key in range
    uint64_t current_position;      // Current scanning position
    std::vector<std::string> found_keys; // List of found private keys
    uint32_t kernel_phase;          // Current kernel phase (ECC/Hash/Compare)
    std::string gpu_name;           // GPU used for this checkpoint
    std::string sha256_digest;      // Checkpoint integrity verification
    std::chrono::system_clock::time_point created_at;
};
```

**Storage**: `data/checkpoint.dat` (existing format with enhancements)

## Data Relationships

### Performance Baseline Hierarchy

```
PerformanceBaseline
├── gpu_name (GPU model)
│   ├── target_throughput (per GPU)
│   ├── min_throughput (95% of target)
│   └── kernel_profiles[]
└── baseline_version (tracking multiple versions)
```

### Code Consolidation Tracking

```
DuplicationMetrics
├── function_name (EmitCandidate, FinalizeDigest, etc.)
│   ├── duplicate_count (before/after consolidation)
│   ├── locations[] (source file paths)
│   └── unified_location (target common module path)
└── is_consolidated (boolean flag)
```

### Performance Monitoring Flow

```
KernelExecution → KernelProfile → PerformanceBaseline
    ↓                ↓                    ↓
Real-time       Telemetry          Regression Detection
Collection       Storage            (if below min_throughput)
```

## Data Flow Architecture

### Input Data Flow

1. **Configuration Files**:
   - `data/config.txt` → CUDA parameters
   - `data/private_ranges.txt` → Private key scanning ranges
   - `data/target_addresses.txt` → Target Bitcoin addresses

2. **Checkpoint Loading**:
   - `data/checkpoint.dat` → Resume scanning position
   - SHA-256 verification → Checkpoint integrity

### Processing Data Flow

1. **ECC Kernel Phase**:
   - Private key range → ECC operations → Public keys
   - Performance metrics → `KernelProfile` records

2. **Hash Kernel Phase**:
   - Public keys → SHA256 → RIPEMD160 → Hash160
   - Memory efficiency tracking → Telemetry storage

3. **Compare Kernel Phase**:
   - Hash160 → Target comparison → Candidate results
   - Found keys → `CheckpointData` updates

### Output Data Flow

1. **Results**:
   - Found private keys → `CheckpointData.found_keys`
   - Performance summary → Telemetry aggregation

2. **Monitoring**:
   - `KernelProfile` → Real-time dashboard
   - `PerformanceBaseline` comparison → Regression alerts

## Data Integrity Measures

### SHA-256 Protection

**Critical Files Protected**:
- All performance baseline files
- Checkpoint data files
- Configuration parameters affecting performance

**Verification Process**:
```cpp
bool verifyDataIntegrity(const std::string& file_path, const std::string& expected_digest) {
    std::string actual_digest = computeSHA256(file_path);
    return actual_digest == expected_digest;
}
```

### Atomic Operations

**Performance-Critical Updates**:
- Checkpoint position updates
- Performance baseline modifications
- Telemetry data writes

**Implementation**: File-based atomic writes with temporary files and rename operations.

## Performance Considerations

### Memory Layout Optimization

**Structure of Arrays (SoA) for GPU Processing**:
```cpp
// Instead of Array of Structures (AoS)
struct KeyBatch {
    uint64_t* private_keys;    // Separate arrays for memory coalescing
    uint8_t* public_keys;      // 65 bytes each
    uint160_t* hash160s;       // 20 bytes each
};
```

**Shared Memory Caching**:
- ECC precomputed tables in shared memory
- Bloom filter for address comparison
- Performance counters in registers

### I/O Optimization

**Telemetry Storage**:
- Append-only log format (`jsonl`)
- Buffered writes every N samples
- Compression for long-term storage

**Checkpoint Frequency**:
- Configurable checkpoint intervals
- Minimal checkpoint size for fast I/O
- Background checkpoint writing

## Data Access Patterns

### Read-Heavy Operations

1. **Configuration Loading**:
   - Load once at startup
   - Cache in device constant memory

2. **Target Address Loading**:
   - Load into GPU shared memory
   - Bloom filter for O(1) lookup

### Write-Heavy Operations

1. **Telemetry Collection**:
   - High-frequency kernel profiling data
   - Buffered batch writes to reduce I/O overhead

2. **Checkpoint Updates**:
   - Periodic position updates
   - Atomic file operations for consistency

### Mixed Access Patterns

1. **Performance Baselines**:
   - Read during regression checks
   - Write only during explicit baseline updates
   - SHA-256 verification on every access

## Data Validation Rules

### Input Validation

1. **Private Key Ranges**:
   - Hexadecimal format validation
   - Range bounds checking (within secp256k1 order)
   - No overlapping ranges

2. **Target Addresses**:
   - Bitcoin address format validation
   - Hash160 format checking
   - Duplicate address removal

### Output Validation

1. **Found Keys**:
   - Private key format validation
   - Address recomputation verification
   - Duplicate result filtering

2. **Performance Data**:
   - Reasonable range checking (no negative throughputs)
   - Timestamp ordering validation
   - Consistency verification across metrics

## Data Migration Strategy

### Checkpoint Compatibility

**Forward Compatibility**:
- Existing checkpoint files remain readable
- New fields added with default values
- Version bump for major format changes

**Migration Process**:
```cpp
struct CheckpointV1 {
    // Legacy format fields
};

struct CheckpointV2 : public CheckpointV1 {
    // New fields for refactored architecture
    uint32_t kernel_phase;
    std::string gpu_name;
};
```

### Baseline Migration

**Existing Baselines**:
- Automatic detection of legacy baseline formats
- Conversion to new SHA-256 protected format
- Version tracking for audit trail

## Security Considerations

### Data Protection

1. **Sensitive Data**:
   - Private keys in memory (zeroization after use)
   - Checkpoint files with appropriate permissions
   - No plaintext key logging

2. **Integrity Protection**:
   - SHA-256 digests for all critical files
   - Detect tampering attempts
   - Audit trail for baseline modifications

### Access Control

1. **File Permissions**:
   - Configuration files: read-only during execution
   - Checkpoint files: read/write for owner only
   - Baseline files: read-only, digest-protected

2. **Runtime Protection**:
   - Memory bounds checking
   - Input sanitization
   - Error handling without information leakage
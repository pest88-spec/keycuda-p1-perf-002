# Implementation Plan: Puzzle71Solver CUDA Technical Debt Elimination

**Branch**: `001-specify-scripts-bash` | **Date**: 2025-10-17 | **Spec**: [specs/001-specify-scripts-bash/spec.md](spec.md)
**Input**: Feature specification from `/specs/001-specify-scripts-bash/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command. See `.specify/templates/commands/plan.md` for the execution workflow.

## Summary

This implementation plan addresses critical technical debt in the Puzzle71Solver GPU-accelerated Bitcoin key scanner. The project will eliminate 100% code duplication for EmitCandidate, FinalizeDigest, and ECC computation functions while achieving 2.5-3.0× performance improvements through CUDA optimization techniques. Key objectives include memory access efficiency improvement from 15.6% to >90%, register pressure reduction from 51-99 to ≤40 registers/thread, and GPU occupancy increase from 25% to ≥80%. The architecture will be modernized through kernel separation (ECC/Hash/Compare), naming convention standardization, and implementation of zero-tolerance performance regression gates.

## Technical Context

**Language/Version**: CUDA 11.8, C++17
**Primary Dependencies**: CUDA Toolkit 11.8+, CMake 3.18+, libsecp256k1-dev, nlohmann_json, GoogleTest
**Storage**: Local file system (checkpoint files, configuration files)
**Testing**: Google Test framework with CUDA device fixtures, custom validation tests
**Target Platform**: Linux (CUDA-capable GPUs from Turing to Hopper architectures)
**Project Type**: Single project with modular kernel architecture
**Performance Goals**: RTX 2080 Ti ≥1000M keys/sec, RTX 3090 ≥2000M keys/sec, Hopper ≥3500M keys/sec
**Constraints**: Zero tolerance for performance regression, ≥85% test coverage maintenance, backward API compatibility
**Scale/Scope**: 25,931 lines of CUDA/C++ code across 131 source files

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Code Deduplication Requirements ✅ ADDRESSED
- [x] **Implementation Plan**: All EmitCandidate functions consolidated into `src/KeyhuntCore/common/result_emitter.cuh` (research.md §Code Duplication Analysis)
- [x] **Implementation Plan**: All FinalizeDigest functions unified in `src/KeyhuntCore/common/hash_utils.cuh` with shared access paths
- [x] **Implementation Plan**: All ECC computation duplicates eliminated through `src/KeyhuntCore/common/ecc_operations.cuh` unified module

### CUDA Performance Standards ✅ ADDRESSED
- [x] **Implementation Plan**: Memory access efficiency target: 15.6% → >90% through forced optimization flags and kernel separation (research.md §Memory Access Optimization)
- [x] **Implementation Plan**: Register pressure target: 51-99 → ≤40 registers/thread through kernel separation (EccKernel ≤32, HashKernel ≤40, CompareKernel ≤24)
- [x] **Implementation Plan**: GPU occupancy target: 25% → ≥80% through resource optimization and adaptive batch sizing
- [x] **Implementation Plan**: Performance baselines established: RTX 2080 Ti ≥1000M keys/sec, RTX 3090 ≥2000M keys/sec, Hopper ≥3500M keys/sec

### Architecture Modernization Requirements ✅ ADDRESSED
- [x] **Implementation Plan**: Fused kernel separated into EccKernel, HashKernel, CompareKernel modules (plan.md §Source Code Structure)
- [x] **Implementation Plan**: Naming conventions standardized (camelCase functions, PascalCase constants) across unified modules
- [x] **Implementation Plan**: Adapter layer simplified through direct module integration and eliminated redundant abstractions

### Performance Regression Prevention ✅ ADDRESSED
- [x] **Implementation Plan**: Zero-tolerance performance regression gates implemented via SHA-256 protected baselines (data-model.md §Performance Baseline Entity)
- [x] **Implementation Plan**: Code quality gates established through comprehensive validation framework
- [x] **Implementation Plan**: P0-P3 priority-based delivery enforced through phased implementation approach
- [x] **Implementation Plan**: SHA-256 digests protecting performance baselines and checkpoint files

### Multi-Architecture Compatibility ✅ ADDRESSED
- [x] **Implementation Plan**: Backward API compatibility maintained through legacy adapter layer (contracts/unified_modules_api.md)
- [x] **Implementation Plan**: Test coverage maintained at ≥85% through comprehensive test framework
- [x] **Implementation Plan**: Source code provenance traceable to analysis report chapters through detailed implementation references

**CONCLUSION**: All Constitution requirements have been addressed in the design phase. The implementation plan fully aligns with the established principles and constraints. ✅ CONSTITUTION COMPLIANT

## Project Structure

### Documentation (this feature)

```
specs/001-specify-scripts-bash/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)

```
src/
├── KeyhuntCore/
│   ├── common/          # NEW: unified modules for code deduplication
│   │   ├── result_emitter.cuh    # unified EmitCandidate implementation
│   │   ├── hash_utils.cuh        # unified FinalizeDigest implementation
│   │   └── ecc_operations.cuh    # unified ECC computation library
│   ├── kernels/
│   │   ├── ecc_separated.cu      # NEW: Separated ECC kernel
│   │   ├── hash_separated.cu     # NEW: Separated hash kernel
│   │   ├── compare_separated.cu  # NEW: Separated compare kernel
│   │   └── puzzle71_kernel.cu    # EXISTING: To be refactored
│   ├── benchmarks/       # NEW: Performance monitoring infrastructure
│   │   ├── baseline_manager.cpp   # SHA-256 protected baseline storage
│   │   ├── benchmark_runner.cpp   # 10-minute sustained benchmarking
│   │   └── telemetry_collector.cpp # Real-time performance telemetry
│   └── [existing modules...]
├── extracted/bitcrack/   # EXISTING: Reference implementations (read-only)
└── extracted/secp256k1-zkp/ # EXISTING: Reference implementations (read-only)

tests/
├── unit/                # EXISTING: Unit test framework
├── performance/         # NEW: Performance regression tests
└── integration/         # EXISTING: Integration tests
```

**Structure Decision**: Single project with modular kernel architecture. NEW components added for code deduplication (`src/KeyhuntCore/common/`) and performance monitoring (`src/KeyhuntCore/benchmarks/`). Existing extracted libraries preserved as read-only reference implementations.

## Complexity Tracking

*Fill ONLY if Constitution Check has violations that must be justified*

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| [e.g., 4th project] | [current need] | [why 3 projects insufficient] |
| [e.g., Repository pattern] | [specific problem] | [why direct DB access insufficient] |


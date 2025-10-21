# KeyhuntCore - Unified Modules Architecture

This directory contains the unified modules for the Puzzle71Solver CUDA Technical Debt Elimination project.

## Directory Structure

```
KeyhuntCore/
├── common/           # Common utilities and shared modules
├── kernels/          # Separated CUDA kernels for performance optimization
├── benchmarks/       # Performance benchmarking and baseline management
├── gpu/             # GPU management and architecture detection
├── memory/          # Memory optimization and management
├── validation/      # CPU/GPU consistency validation
├── utils/           # Utility functions and helpers
├── monitoring/      # Performance monitoring and telemetry
├── security/        # Security features and SHA-256 protection
├── build/           # Build system optimization and validation
├── docs/            # Documentation generation
├── compatibility/   # Backward compatibility layer
├── config/          # Configuration management
├── adapters/        # Legacy adapter layer
└── main/            # Main execution flow
```

## Design Principles

1. **Modular Architecture**: Each module has a single responsibility
2. **Zero-Tolerance Regression**: All performance changes must maintain baseline
3. **SHA-256 Protection**: All baseline files are cryptographically protected
4. **Backward Compatibility**: Legacy APIs are preserved through adapters
5. **Performance First**: All optimizations must be validated

## Module Dependencies

- **common/**: Core shared utilities, no dependencies
- **kernels/**: CUDA kernels, depends on common/, memory/
- **benchmarks/**: Performance testing, depends on common/, utils/
- **validation/**: Consistency checking, depends on common/, utils/
- **gpu/**: GPU management, depends on common/
- **memory/**: Memory optimization, depends on common/
- **monitoring/**: Performance monitoring, depends on common/, utils/
- **security/**: SHA-256 protection, depends on common/, utils/

## Integration Notes

All modules are designed to work together as a cohesive system while maintaining
independence for testing and validation purposes. The unified architecture eliminates
code duplication while providing clear separation of concerns.

## Performance Targets

- Memory efficiency: >90%
- GPU occupancy: ≥80%
- Performance improvement: 2.5-3× over baseline
- Zero tolerance for performance regression
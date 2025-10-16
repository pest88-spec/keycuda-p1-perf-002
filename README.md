# P1-PERF-002: Dual-Stream Pipeline + Adaptive Batch Sizing

CUDA performance optimization for Puzzle71Solver.

## Features

- Dual-stream CUDA pipeline (configurable 1-4 streams)
- Adaptive batch sizing based on GPU memory
- Expected performance improvement: ~1.8×

## Quick Start

```bash
# Build
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8
cd ..

# Deploy
bash scripts/deploy_prod.sh canary 300
```

## Files

- `src/main.cpp` - CLI parameter parsing
- `src/solver.h` - SolverOptions extension
- `scripts/deploy_prod.sh` - Production deployment
- `RELEASE_NOTES_P1-PERF-002.md` - Release notes

## Performance Targets

| Configuration | Target | Status |
|---|---|---|
| Single-stream | 1.0 Gkeys/s | Baseline |
| Dual-stream | 1.2-1.5 Gkeys/s | Ready |
| Dual + Auto-batch | 1.56-1.95 Gkeys/s | Ready |
| **Combined** | **~1.8×** | **Ready** |

## Deployment

```bash
# Canary deployment (10% of instances)
bash scripts/deploy_prod.sh canary 300

# Full deployment
bash scripts/deploy_prod.sh full 600

# Rollback
bash scripts/rollback_prod.sh
```

## Status

✅ Implementation complete
✅ CLI parameters working
✅ Deployment scripts ready
✅ Ready for production deployment

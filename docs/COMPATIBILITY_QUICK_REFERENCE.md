# Compatibility Quick Reference Card - Puzzle71Solver

## Overview

Quick reference for Puzzle71Solver v2.0+ compatibility features. Keep this handy for day-to-day operations.

---

## 🔄 API Compatibility

### Legacy API (Still Works)
```cpp
// These calls continue to work automatically
solver.set_threads(256);
solver.set_device(0);
solver.set_memory(4096);
solver.set_batch(1000);
solver.set_range("1000:2000");
solver.set_output("results.txt");
solver.set_verbose(true);
```

### Modern API (Recommended)
```cpp
// Recommended for new code
solver.set_gpu_threads(256);
solver.set_gpu_device_id(0);
solver.set_gpu_memory_size(4096);
solver.set_batch_size(1000);
solver.set_key_range("1000:2000");
solver.set_output_file("results.txt");
solver.set_verbosity_level(3);
```

### API Translation (Automatic)
```bash
# Enable automatic API translation
export PUZZLE71_AUTO_API_TRANSLATION=1
./puzzle71solver

# Check API compatibility
./puzzle71_solver --check-api-compatibility
```

---

## ⚙️ Configuration Compatibility

### Supported Formats
| Format | Extension | Status | Migration |
|--------|----------|---------|----------|
| Legacy V1 | `.cfg` | ✅ Supported | Auto |
| Legacy V2 | `.conf` | ✅ Supported | Auto |
| JSON V1 | `.json` | ✅ Supported | Auto |
| JSON V2 | `.json` | ✅ Recommended | - |
| YAML V1 | `.yaml` | ✅ Supported | Auto |
| TOML V1 | `.toml` | ✅ Supported | Auto |

### Migration Commands
```bash
# Basic migration
./scripts/migrate_config.sh legacy.conf modern.json

# With validation
./scripts/migrate_config.sh --validate-only config.conf

# Batch migration
./scripts/migrate_config.sh --batch --source-dir ./configs/

# Dry run
./scripts/migrate_config.sh --dry-run --verbose

# List supported formats
./scripts/migrate_config.sh --list-formats
```

### Validation
```bash
# Validate configuration format
./scripts/migrate_config.sh --validate-only config.conf

# Check compatibility
./scripts/migrate_config.sh --check-compatibility config.conf

# Test loaded configuration
./puzzle71solver --test-config config.json
```

---

## 🖥️ GPU Architecture Compatibility

### Supported Architectures
| Architecture | Compute Cap | Status | Kernels |
|-------------|--------------|---------|---------|
| Turing | SM 7.5 | ✅ Full | ecc_kernel |
| Ampere | SM 8.0/8.6 | ✅ Full | ecc_tensor, ecc_bf16 |
| Ada Lovelace | SM 8.9 | ✅ Full | ecc_tensor, ecc_bf16, ecc_fp8 |
| Hopper | SM 9.0 | ✅ Full | ecc_tensor, ecc_bf16, ecc_fp8 |

### Detection & Selection
```bash
# Detect GPU architectures
./puzzle71_solver --detect-gpu

# Check compatibility
./puzzle71_solver --gpu-compatibility

# Auto kernel selection
./puzzle71_solver --auto-kernel-selection

# Test kernels
./puzzle71_solver --test-kernels
```

### Configuration Example
```json
{
  "sections": {
    "gpu": {
      "parameters": {
        "auto_kernel_selection": true,
        "preferred_kernels": ["ecc_tensor", "ecc_bf16"]
      }
    }
  }
}
```

---

## 📊 Test Coverage Compatibility

### Thresholds
- **Overall Coverage**: ≥85%
- **Critical Modules**: ≥90%
- **API Coverage**: ≥95%
- **Configuration Coverage**: ≥90%

### Coverage Commands
```bash
# Run coverage tests
./scripts/run_coverage_tests.sh

# Custom thresholds
./scripts/run_coverage_tests.sh --threshold 90

# Generate HTML report
./scripts/run_coverage_tests.sh --html-report

# CI/CD integration
./scripts/ci/coverage_gate.sh --threshold 85
```

### Report Generation
```bash
# Comprehensive report
./scripts/run_coverage_tests.sh --comprehensive

# Trend analysis
./scripts/run_coverage_tests.sh --trend-analysis

# Badge generation
./scripts/run_coverage_tests.sh --badge
```

---

## 🔧 Validation & Testing

### Compatibility Tests
```bash
# Run all compatibility tests
./scripts/run_compatibility_validation.sh

# Specific categories
./scripts/run_compatibility_validation.sh --api-tests
./scripts/run_compatibility_validation.sh --config-tests
./scripts/run_compatibility_validation.sh --architecture-tests

# Generate reports
./scripts/run_compatibility_validation.sh --html-report
```

### Individual Tests
```bash
# API compatibility
./puzzle71_solver --test-api-compatibility

# Configuration compatibility
./puzzle71_solver --test-config-compatibility

# GPU compatibility
./puzzle71_solver --test-gpu-compatibility

# Performance validation
./puzzle71_solver --performance-validation
```

---

## 🚀 Performance Optimization

### Automatic Optimization
```bash
# Auto-optimize settings
./puzzle71_solver --auto-optimize

# Performance recommendations
./puzzle71_solver --optimize-recommendations

# Benchmark comparison
./puzzle71_solver --benchmark --compare baseline.json
```

### Architecture-Specific Optimization
```cpp
// Enable automatic optimization
solver.enable_automatic_optimization();

// Set optimization level
solver.set_performance_optimization_level(2);

// Enable memory pooling
solver.enable_memory_pool(true);
```

---

## 🛠️ Troubleshooting

### Common Issues

#### API Issues
```bash
# Check API compatibility
./puzzle71_solver --check-api-compatibility

# Enable debug logging
./puzzle71_solver --debug --api-translation
```

#### Configuration Issues
```bash
# Validate configuration
./scripts/migrate_config.sh --validate-only config.conf

# Check format compatibility
./scripts/migrate_config.sh --check-format config.conf
```

#### GPU Issues
```bash
# Check GPU compatibility
./puzzle71_solver --gpu-compatibility

# Test kernel availability
./puzzle71_solver --test-kernels
```

### Debug Mode
```bash
# Enable comprehensive debugging
./puzzle71_solver --debug --verbose

# Check system requirements
./puzzle71_solver --check-requirements

# Generate diagnostic report
./puzzle71_solver --diagnostic
```

---

## 📋 Validation Checklist

### Pre-Migration
- [ ] Backup current configuration files
- [ ] Test current functionality
- [ ] Check system requirements
- [ ] Verify GPU compatibility

### Migration
- [ ] Migrate configuration files
- [ ] Validate migrated configurations
- [ ] Update API calls (optional)
- [ ] Test new features

### Post-Migration
- [ ] Run compatibility tests
- [ ] Validate performance
- [ ] Check coverage thresholds
- [ ] Update documentation
- [ ] Train team on new features

---

## 📞 Support Resources

### Documentation
- **API Docs**: `docs/API.md`
- **Config Guide**: `docs/CONFIGURATION.md`
- **Migration Guide**: `docs/MIGRATION_GUIDE.md`
- **Compatibility Guarantees**: `docs/COMPATIBILITY_GUARANTEES.md`

### Support
- **Issues**: [GitHub Issues](https://github.com/puzzle71/puzzle71-solver/issues)
- **Discussions**: [GitHub Discussions](https://github.com/puzzle71/puzzle71-solver/discussions)
- **Email**: support@puzzle71.com

### Tools
- **Migration**: `scripts/migrate_config.sh`
- **Validation**: `scripts/run_compatibility_validation.sh`
- **Coverage**: `scripts/run_coverage_tests.sh`
- **CI/CD**: `scripts/ci/coverage_gate.sh`

---

## ⚡ Quick Commands

### One-Command Migration
```bash
# Auto-migrate and validate
./scripts/migrate_config.sh --auto-migrate --validate config.conf

# Run all compatibility tests
./scripts/run_compatibility_validation.sh --quick

# Generate comprehensive report
./scripts/run_compatibility_validation.sh --html-report --comprehensive
```

### Validation Commands
```bash
# Quick validation
./puzzle71_solver --test-all

# Performance check
./puzzle71_solver --performance-check

# Compatibility check
./puzzle71solver --compatibility-check
```

### Report Generation
```bash
# Generate all reports
./scripts/generate_all_reports.sh

# Quick report summary
./scripts/report_summary.sh
```

---

## 🔄 Migration Commands

### Configuration Migration
```bash
# Legacy to Modern
./scripts/migrate_config.sh legacy.conf modern.json

# With backup
./scripts/migrate_config.sh legacy.conf modern.json --backup

# Batch migration
./scripts/migrate_config.sh --batch --source-dir ./configs/
```

### Validation Migration
```bash
# Validate migrated config
./puzzle71_solver --validate-config modern.json

# Run compatibility tests
./scripts/run_compatibility_validation.sh --config-tests

# Generate migration report
./scripts/migrate_config.sh --report migration_report.json
```

---

*This quick reference card is designed for daily use and troubleshooting. For detailed information, refer to the full documentation.*
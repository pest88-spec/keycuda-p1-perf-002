#!/bin/bash
#
# Phase 1 Validation Quick Start Script
#
# OpenSpec: unify-kernel-execution-paths
# This script helps you run Phase 1 validation step-by-step
#

set -euo pipefail

echo "========================================="
echo "  Phase 1 Validation Quick Start"
echo "========================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check function
check_requirement() {
    local name=$1
    local command=$2

    echo -n "Checking $name... "
    if eval "$command" &>/dev/null; then
        echo -e "${GREEN}✓${NC}"
        return 0
    else
        echo -e "${RED}✗${NC}"
        return 1
    fi
}

# Stage 1: Environment Verification
echo "Stage 1: Environment Verification"
echo "-----------------------------------"

REQUIREMENTS_MET=true

check_requirement "CUDA device" "nvidia-smi" || REQUIREMENTS_MET=false
check_requirement "CUDA compiler" "nvcc --version" || REQUIREMENTS_MET=false
check_requirement "CMake" "cmake --version" || REQUIREMENTS_MET=false
check_requirement "Git" "git --version" || REQUIREMENTS_MET=false

echo ""

if [ "$REQUIREMENTS_MET" = false ]; then
    echo -e "${RED}Error: Missing requirements. Please install missing components.${NC}"
    exit 1
fi

# Display GPU info
echo "GPU Information:"
echo "-----------------------------------"
nvidia-smi --query-gpu=name,compute_cap,memory.total --format=csv,noheader
echo ""

# Stage 2: Build Project
echo "Stage 2: Build Project"
echo "-----------------------------------"

if [ -d "build" ]; then
    echo -e "${YELLOW}Warning: build directory exists. Clean rebuild? (y/n)${NC}"
    read -r response
    if [ "$response" = "y" ]; then
        rm -rf build
    fi
fi

mkdir -p build
cd build

echo "Running CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON \
    -DBUILD_BENCHMARKS=ON \
    -DCMAKE_CUDA_ARCHITECTURES="75;80;86;89;90" || {
    echo -e "${RED}CMake configuration failed!${NC}"
    exit 1
}

echo "Building project..."
make -j$(nproc) || {
    echo -e "${RED}Build failed!${NC}"
    exit 1
}

cd ..
echo -e "${GREEN}✓ Build successful${NC}"
echo ""

# Stage 3: Run Baseline Measurements
echo "Stage 3: Run Baseline Measurements"
echo "-----------------------------------"

mkdir -p benchmark_results

echo "This will run 4 benchmark tests. Each may take several minutes."
echo "Press Enter to continue or Ctrl+C to cancel..."
read -r

# Test 1: Fused Architecture
echo ""
echo "[1/4] Testing Fused Architecture..."
echo "-----------------------------------"
./build/tests/performance/kernel_architecture_comparison_tests \
    --gtest_filter="KernelArchitectureTest.FusedArchitecturePerformance" || {
    echo -e "${RED}Fused architecture test failed${NC}"
    exit 1
}
echo -e "${GREEN}✓ Fused architecture test passed${NC}"

# Test 2: Separated ECC
echo ""
echo "[2/4] Testing Separated ECC Architecture..."
echo "-----------------------------------"
./build/tests/performance/kernel_architecture_comparison_tests \
    --gtest_filter="KernelArchitectureTest.SeparatedEccArchitecturePerformance" || {
    echo -e "${RED}Separated ECC test failed${NC}"
    exit 1
}
echo -e "${GREEN}✓ Separated ECC test passed${NC}"

# Test 3: Separated Hash
echo ""
echo "[3/4] Testing Separated Hash Architecture..."
echo "-----------------------------------"
./build/tests/performance/kernel_architecture_comparison_tests \
    --gtest_filter="KernelArchitectureTest.SeparatedHashArchitecturePerformance" || {
    echo -e "${RED}Separated Hash test failed${NC}"
    exit 1
}
echo -e "${GREEN}✓ Separated Hash test passed${NC}"

# Test 4: Separated Full Pipeline
echo ""
echo "[4/4] Testing Separated Full Pipeline..."
echo "-----------------------------------"
./build/tests/performance/kernel_architecture_comparison_tests \
    --gtest_filter="KernelArchitectureTest.SeparatedFullPipelinePerformance" || {
    echo -e "${RED}Separated Full Pipeline test failed${NC}"
    exit 1
}
echo -e "${GREEN}✓ Separated Full Pipeline test passed${NC}"

# Run full benchmark suite
echo ""
echo "Running full benchmark suite..."
echo "-----------------------------------"
./scripts/benchmark/run_kernel_architecture_benchmarks.sh \
    --output-dir benchmark_results \
    --iterations 10 || {
    echo -e "${RED}Benchmark suite failed${NC}"
    exit 1
}

echo ""
echo -e "${GREEN}✓ All benchmarks completed successfully${NC}"
echo ""

# Stage 4: Display Results
echo "Stage 4: Results Summary"
echo "-----------------------------------"

if [ -f "benchmark_results/summary_"*".txt" ]; then
    cat benchmark_results/summary_*.txt
else
    echo -e "${YELLOW}Warning: Summary file not found${NC}"
fi

echo ""

# Stage 5: Generate Baselines
echo "Stage 5: Generate Baseline Files"
echo "-----------------------------------"

mkdir -p benchmarks/baselines

echo "Copying measurement results to baselines..."

if [ -f "benchmark_results/fused_architecture.json" ]; then
    cp benchmark_results/fused_architecture.json \
       benchmarks/baselines/fused_architecture_baseline.json
    echo -e "${GREEN}✓${NC} Fused baseline created"
else
    echo -e "${YELLOW}⚠${NC} Fused architecture results not found"
fi

if [ -f "benchmark_results/separated_ecc_architecture.json" ]; then
    cp benchmark_results/separated_ecc_architecture.json \
       benchmarks/baselines/separated_ecc_baseline.json
    echo -e "${GREEN}✓${NC} Separated ECC baseline created"
else
    echo -e "${YELLOW}⚠${NC} Separated ECC results not found"
fi

if [ -f "benchmark_results/separated_hash_architecture.json" ]; then
    cp benchmark_results/separated_hash_architecture.json \
       benchmarks/baselines/separated_hash_baseline.json
    echo -e "${GREEN}✓${NC} Separated Hash baseline created"
else
    echo -e "${YELLOW}⚠${NC} Separated Hash results not found"
fi

if [ -f "benchmark_results/separated_full_pipeline.json" ]; then
    cp benchmark_results/separated_full_pipeline.json \
       benchmarks/baselines/separated_full_pipeline_baseline.json
    echo -e "${GREEN}✓${NC} Separated Full Pipeline baseline created"
else
    echo -e "${YELLOW}⚠${NC} Separated Full Pipeline results not found"
fi

# Generate checksums
echo ""
echo "Generating SHA-256 checksums..."
cd benchmarks/baselines
sha256sum *_baseline.json > CHECKSUMS.sha256 2>/dev/null || {
    echo -e "${YELLOW}Warning: Could not generate checksums (no baseline files?)${NC}"
}

if [ -f "CHECKSUMS.sha256" ]; then
    echo -e "${GREEN}✓${NC} Checksums generated"
    echo ""
    echo "Checksums:"
    cat CHECKSUMS.sha256
else
    echo -e "${YELLOW}⚠${NC} Checksums not generated"
fi

cd ../..
echo ""

# Stage 6: Validation Report
echo "========================================="
echo "  Phase 1 Validation Complete!"
echo "========================================="
echo ""

echo "Next Steps:"
echo "1. Review results in benchmark_results/"
echo "2. Compare against targets in benchmarks/performance_targets.json"
echo "3. Fill out validation checklist: openspec/changes/unify-kernel-execution-paths/PHASE1_VALIDATION_CHECKLIST.md"
echo "4. Get approvals from technical lead and architecture review"
echo "5. Once approved, proceed to Phase 2"
echo ""

echo "Files created:"
echo "- benchmark_results/*.json (measurement results)"
echo "- benchmark_results/summary_*.txt (summary report)"
echo "- benchmarks/baselines/*_baseline.json (baseline files)"
echo "- benchmarks/baselines/CHECKSUMS.sha256 (SHA-256 protection)"
echo ""

echo -e "${GREEN}Phase 1 validation preparation complete!${NC}"
echo ""

# Create validation status file
cat > openspec/changes/unify-kernel-execution-paths/VALIDATION_STATUS.txt << EOF
Phase 1 Validation Status
=========================

Date: $(date +%Y-%m-%d)
GPU: $(nvidia-smi --query-gpu=name --format=csv,noheader | head -1)

Status: MEASUREMENTS COMPLETE - AWAITING REVIEW

Completed:
- ✓ Environment verification
- ✓ Project build
- ✓ Baseline measurements
- ✓ Baseline files generated
- ✓ SHA-256 checksums created

Pending:
- [ ] Results review
- [ ] Target validation
- [ ] Interface design approval
- [ ] Technical lead approval
- [ ] Architecture review approval

Next Action: Review PHASE1_VALIDATION_CHECKLIST.md
EOF

echo "Validation status saved to: openspec/changes/unify-kernel-execution-paths/VALIDATION_STATUS.txt"

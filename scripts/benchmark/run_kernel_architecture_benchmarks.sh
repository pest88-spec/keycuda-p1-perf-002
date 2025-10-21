#!/bin/bash
#
# Automated Kernel Architecture Benchmark Runner
#
# OpenSpec: unify-kernel-execution-paths
# Task: 1.1.2 - Implement automated architecture tests
#
# This script automates the execution of all three kernel architecture
# performance tests and generates comparison reports.
#
# Usage:
#   ./run_kernel_architecture_benchmarks.sh [--output-dir DIR] [--iterations N]
#

set -euo pipefail

# Default configuration
OUTPUT_DIR="benchmark_results"
ITERATIONS=10
TEST_BINARY="./build/tests/performance/kernel_architecture_comparison_tests"
TIMESTAMP=$(date +%Y-%m-%d_%H-%M-%S)

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --iterations)
            ITERATIONS="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --output-dir DIR    Output directory for benchmark results (default: benchmark_results)"
            echo "  --iterations N      Number of benchmark iterations (default: 10)"
            echo "  --help              Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Create output directory
mkdir -p "${OUTPUT_DIR}"

echo "========================================"
echo "  Kernel Architecture Benchmark Suite"
echo "========================================"
echo ""
echo "Configuration:"
echo "  Output Directory: ${OUTPUT_DIR}"
echo "  Iterations: ${ITERATIONS}"
echo "  Timestamp: ${TIMESTAMP}"
echo ""

# Check if test binary exists
if [[ ! -f "${TEST_BINARY}" ]]; then
    echo "Error: Test binary not found: ${TEST_BINARY}"
    echo "Please build the project first:"
    echo "  mkdir -p build && cd build"
    echo "  cmake .. -DBUILD_TESTS=ON"
    echo "  make -j\$(nproc)"
    exit 1
fi

# Check CUDA device availability
echo "Checking CUDA devices..."
if ! nvidia-smi &>/dev/null; then
    echo "Error: nvidia-smi not found or no CUDA devices available"
    exit 1
fi

# Get GPU information
GPU_NAME=$(nvidia-smi --query-gpu=name --format=csv,noheader | head -1)
GPU_ARCH=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader | head -1)

echo "GPU Detected:"
echo "  Name: ${GPU_NAME}"
echo "  Compute Capability: ${GPU_ARCH}"
echo ""

# Run benchmark tests
echo "========================================"
echo "Running Benchmark Tests..."
echo "========================================"
echo ""

# Test 1: Fused Architecture
echo "[1/4] Testing Fused Architecture (puzzle71_kernel.cu)..."
${TEST_BINARY} --gtest_filter="KernelArchitectureTest.FusedArchitecturePerformance" \
    --gtest_output="json:${OUTPUT_DIR}/fused_gtest_${TIMESTAMP}.json"

# Test 2: Separated ECC Architecture
echo "[2/4] Testing Separated ECC Architecture (ecc_kernel.cu)..."
${TEST_BINARY} --gtest_filter="KernelArchitectureTest.SeparatedEccArchitecturePerformance" \
    --gtest_output="json:${OUTPUT_DIR}/separated_ecc_gtest_${TIMESTAMP}.json"

# Test 3: Separated Hash Architecture
echo "[3/4] Testing Separated Hash Architecture (hash_kernel.cu)..."
${TEST_BINARY} --gtest_filter="KernelArchitectureTest.SeparatedHashArchitecturePerformance" \
    --gtest_output="json:${OUTPUT_DIR}/separated_hash_gtest_${TIMESTAMP}.json"

# Test 4: Full Separated Pipeline
echo "[4/4] Testing Full Separated Pipeline (ECC + Hash)..."
${TEST_BINARY} --gtest_filter="KernelArchitectureTest.SeparatedFullPipelinePerformance" \
    --gtest_output="json:${OUTPUT_DIR}/separated_full_gtest_${TIMESTAMP}.json"

# Run comparison test
echo ""
echo "Generating comparison report..."
${TEST_BINARY} --gtest_filter="KernelArchitectureTest.CompareAllArchitectures" \
    --gtest_output="json:${OUTPUT_DIR}/comparison_gtest_${TIMESTAMP}.json"

echo ""
echo "========================================"
echo "Benchmark Results Summary"
echo "========================================"
echo ""

# Generate summary report
SUMMARY_FILE="${OUTPUT_DIR}/summary_${TIMESTAMP}.txt"
{
    echo "Kernel Architecture Benchmark Summary"
    echo "====================================="
    echo ""
    echo "Timestamp: ${TIMESTAMP}"
    echo "GPU: ${GPU_NAME}"
    echo "Compute Capability: ${GPU_ARCH}"
    echo "Iterations: ${ITERATIONS}"
    echo ""
    echo "Test Results:"
    echo ""

    # Check if JSON files exist and extract key metrics
    if [[ -f "${OUTPUT_DIR}/fused_architecture.json" ]]; then
        echo "1. Fused Architecture:"
        jq -r '"   Throughput: \(.keys_per_second) keys/sec"' "${OUTPUT_DIR}/fused_architecture.json" 2>/dev/null || echo "   (Results pending)"
        jq -r '"   Registers: \(.registers_per_thread) per thread"' "${OUTPUT_DIR}/fused_architecture.json" 2>/dev/null || echo ""
        jq -r '"   Occupancy: \(.gpu_occupancy_percent)%"' "${OUTPUT_DIR}/fused_architecture.json" 2>/dev/null || echo ""
        echo ""
    fi

    if [[ -f "${OUTPUT_DIR}/separated_ecc_architecture.json" ]]; then
        echo "2. Separated ECC Architecture:"
        jq -r '"   Throughput: \(.keys_per_second) keys/sec"' "${OUTPUT_DIR}/separated_ecc_architecture.json" 2>/dev/null || echo "   (Results pending)"
        jq -r '"   Registers: \(.registers_per_thread) per thread"' "${OUTPUT_DIR}/separated_ecc_architecture.json" 2>/dev/null || echo ""
        jq -r '"   Occupancy: \(.gpu_occupancy_percent)%"' "${OUTPUT_DIR}/separated_ecc_architecture.json" 2>/dev/null || echo ""
        echo ""
    fi

    if [[ -f "${OUTPUT_DIR}/separated_hash_architecture.json" ]]; then
        echo "3. Separated Hash Architecture:"
        jq -r '"   Throughput: \(.keys_per_second) keys/sec"' "${OUTPUT_DIR}/separated_hash_architecture.json" 2>/dev/null || echo "   (Results pending)"
        jq -r '"   Registers: \(.registers_per_thread) per thread"' "${OUTPUT_DIR}/separated_hash_architecture.json" 2>/dev/null || echo ""
        jq -r '"   Occupancy: \(.gpu_occupancy_percent)%"' "${OUTPUT_DIR}/separated_hash_architecture.json" 2>/dev/null || echo ""
        echo ""
    fi

    if [[ -f "${OUTPUT_DIR}/separated_full_pipeline.json" ]]; then
        echo "4. Separated Full Pipeline:"
        jq -r '"   Throughput: \(.keys_per_second) keys/sec"' "${OUTPUT_DIR}/separated_full_pipeline.json" 2>/dev/null || echo "   (Results pending)"
        jq -r '"   Registers: \(.registers_per_thread) per thread (max)"' "${OUTPUT_DIR}/separated_full_pipeline.json" 2>/dev/null || echo ""
        jq -r '"   Occupancy: \(.gpu_occupancy_percent)% (min)"' "${OUTPUT_DIR}/separated_full_pipeline.json" 2>/dev/null || echo ""
        echo ""
    fi

    echo "========================================"
    echo ""
    echo "All benchmark results saved to: ${OUTPUT_DIR}/"
    echo "Summary report: ${SUMMARY_FILE}"

} | tee "${SUMMARY_FILE}"

echo ""
echo "Benchmark suite completed successfully!"
echo ""
echo "Next steps:"
echo "  1. Review results in ${OUTPUT_DIR}/"
echo "  2. Compare architectures using comparison_report"
echo "  3. Set performance targets based on baseline measurements"
echo ""

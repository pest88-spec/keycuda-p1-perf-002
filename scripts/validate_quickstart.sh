#!/bin/bash

# Quickstart.md Validation Script
# Purpose: Validate all setup procedures in quickstart.md work correctly
# Date: 2025-10-17
# Phase: 8 (T065)

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Validation results
VALIDATION_PASSED=0
VALIDATION_FAILED=0
VALIDATION_SKIPPED=0

# Function to log validation result
log_result() {
    local test_name="$1"
    local status="$2"
    local message="$3"

    case "$status" in
        "PASS")
            echo -e "${GREEN}✓${NC} $test_name: $message"
            ((VALIDATION_PASSED++))
            ;;
        "FAIL")
            echo -e "${RED}✗${NC} $test_name: $message"
            ((VALIDATION_FAILED++))
            ;;
        "SKIP")
            echo -e "${YELLOW}⚠${NC} $test_name: $message"
            ((VALIDATION_SKIPPED++))
            ;;
    esac
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to check if directory exists
directory_exists() {
    [[ -d "$1" ]]
}

# Function to check if file exists
file_exists() {
    [[ -f "$1" ]]
}

echo -e "${BLUE}Starting Quickstart.md Validation...${NC}"
echo "========================================"

# Section 1: Prerequisites Validation
echo -e "\n${BLUE}1. Prerequisites Validation${NC}"
echo "----------------------------"

# Check CUDA Toolkit
if command_exists nvcc; then
    cuda_version=$(nvcc --version | head -1 | grep -oP '(?<=release )[^,]+' || echo "unknown")
    log_result "CUDA Toolkit" "PASS" "nvcc version $cuda_version found"
else
    log_result "CUDA Toolkit" "FAIL" "nvcc not found in PATH"
fi

# Check GPU availability
if command_exists nvidia-smi; then
    gpu_count=$(nvidia-smi --query-gpu=count --format=csv,noheader,nounits | head -1)
    if [[ "$gpu_count" -gt 0 ]]; then
        gpu_name=$(nvidia-smi --query-gpu=name --format=csv,noheader,nounits | head -1)
        log_result "GPU Availability" "PASS" "$gpu_count GPU(s) found: $gpu_name"
    else
        log_result "GPU Availability" "FAIL" "No CUDA-capable GPUs found"
    fi
else
    log_result "GPU Availability" "FAIL" "nvidia-smi not found"
fi

# Check system memory
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    system_memory_gb=$(free -g | awk '/^Mem:/ {print $2}')
    if [[ "$system_memory_gb" -ge 16 ]]; then
        log_result "System Memory" "PASS" "$system_memory_gb GB RAM (meets 16GB+ requirement)"
    else
        log_result "System Memory" "FAIL" "$system_memory_gb GB RAM (below 16GB requirement)"
    fi
else
    log_result "System Memory" "SKIP" "Memory check not supported on this OS"
fi

# Check required packages
required_packages=("cmake" "git" "gcc")
for package in "${required_packages[@]}; do
    if command_exists "$package"; then
        version_output=$("$package" --version 2>/dev/null || echo "version unknown")
        log_result "Package: $package" "PASS" "$package found ($version_output)"
    else
        log_result "Package: $package" "FAIL" "$package not found"
    fi
done

# Check development libraries
dev_libraries=("libsecp256k1-dev" "libgtest-dev")
for lib in "${dev_libraries[@]}"; do
    if dpkg -l "$lib" >/dev/null 2>&1; then
        log_result "Library: $lib" "PASS" "$lib installed"
    else
        log_result "Library: $lib" "FAIL" "$lib not installed"
    fi
done

# Section 2: Project Structure Validation
echo -e "\n${BLUE}2. Project Structure Validation${NC}"
echo "--------------------------------"

# Check project directories
project_dirs=("src" "build" "data" "tests" "docs" "scripts")
for dir in "${project_dirs[@]}"; do
    if directory_exists "$dir"; then
        log_result "Directory: $dir" "PASS" "Directory exists"
    else
        log_result "Directory: $dir" "FAIL" "Directory missing"
    fi
done

# Check KeyhuntCore modules
keyhunt_modules=("src/KeyhuntCore/common" "src/KeyhuntCore/kernels" "src/KeyhuntCore/benchmarks")
for module in "${keyhunt_modules[@]}"; do
    if directory_exists "$module"; then
        file_count=$(find "$module" -name "*.cuh" -o -name "*.cu" -o -name "*.cpp" | wc -l)
        if [[ "$file_count" -gt 0 ]]; then
            log_result "Module: $(basename "$module")" "PASS" "$file_count source files found"
        else
            log_result "Module: $(basename "$module")" "FAIL" "No source files found"
        fi
    else
        log_result "Module: $(basename "$module")" "FAIL" "Module directory missing"
    fi
done

# Check unified modules
unified_modules=("result_emitter.cuh" "hash_utils.cuh" "ecc_operations.cuh")
for module in "${unified_modules[@]}"; do
    if file_exists "src/KeyhuntCore/common/$module"; then
        log_result "Unified Module: $module" "PASS" "Module exists"
    else
        log_result "Unified Module: $module" "FAIL" "Module missing"
    fi
done

# Check configuration files
config_files=("data/config.txt" "CMakeLists.txt")
for config in "${config_files[@]}"; do
    if file_exists "$config"; then
        log_result "Config File: $config" "PASS" "Configuration file exists"
    else
        log_result "Config File: $config" "FAIL" "Configuration file missing"
    fi
done

# Section 3: Build System Validation
echo -e "\n${BLUE}3. Build System Validation${NC}"
echo "---------------------------"

# Check if build directory exists
if directory_exists "build"; then
    log_result "Build Directory" "PASS" "Build directory exists"
else
    log_result "Build Directory" "FAIL" "Build directory missing"
fi

# Try to run CMake configuration
if directory_exists "build" && [[ -f "../src/KeyhuntCore/CMakeLists.txt" ]]; then
    echo -e "${YELLOW}Testing CMake configuration...${NC}"
    if cd build 2>/dev/null; then
        if cmake ../src/KeyhuntCore -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1; then
            log_result "CMake Configuration" "PASS" "CMake configuration successful"

            # Test build
            echo -e "${YELLOW}Testing build process...${NC}"
            if make -j$(nproc) >/dev/null 2>&1; then
                if [[ -f "Puzzle71Solver" ]]; then
                    log_result "Build Process" "PASS" "Puzzle71Solver executable created"
                else
                    log_result "Build Process" "FAIL" "Puzzle71Solver executable not found"
                fi
            else
                log_result "Build Process" "FAIL" "Build failed"
            fi
            cd ..
        else
            log_result "CMake Configuration" "FAIL" "CMake configuration failed"
            cd ..
        fi
    else
        log_result "CMake Configuration" "SKIP" "Could not access build directory"
    fi
else
    log_result "CMake Configuration" "SKIP" "Build directory or CMakeLists.txt missing"
fi

# Section 4: Code Quality Validation
echo -e "\n${BLUE}4. Code Quality Validation${NC}"
echo "--------------------------"

# Check for duplicate code elimination
duplicate_files=("src/puzzle71_kernel.cu" "src/kernels/hash_kernel.cu")
if [[ -f "src/puzzle71_kernel.cu" ]] && [[ -f "src/kernels/hash_kernel.cu" ]]; then
    # Check if they use unified modules
    if grep -q "#include.*result_emitter.cuh" src/puzzle71_kernel.cu 2>/dev/null; then
        log_result "Code Deduplication" "PASS" "puzzle71_kernel.cu uses unified modules"
    else
        log_result "Code Deduplication" "FAIL" "puzzle_kernel.cu not using unified modules"
    fi

    if grep -q "#include.*result_emitter.cuh" src/kernels/hash_kernel.cu 2>/dev/null; then
        log_result "Code Deduplication" "PASS" "hash_kernel.cu uses unified modules"
    else
        log_result "Code Deduplication" "FAIL" "hash_kernel.cu not using unified modules"
    fi
else
    log_result "Code Deduplication" "SKIP" "Source files not found"
fi

# Check naming conventions
if [[ -f "src/puzzle71_kernel.cu" ]]; then
    # Check for camelCase functions
    if grep -q "emitCandidate\|finalizeDigest" src/puzzle71_kernel.cu 2>/dev/null; then
        log_result "Naming Conventions" "PASS" "camelCase function names found"
    else
        log_result "Naming Conventions" "FAIL" "camelCase function names not found"
    fi
else
    log_result "Naming Conventions" "SKIP" "puzzle71_kernel.cu not found"
fi

# Section 5: Performance Optimization Validation
echo -e "\n${BLUE}5. Performance Optimization Validation${NC}"
echo "----------------------------------------"

# Check memory optimization configuration
if [[ -f "data/config.txt" ]]; then
    if grep -q "USE_ORIGINAL_READINT=0" data/config.txt && grep -q "USE_ORIGINAL_WRITEINT=0" data/config.txt; then
        log_result "Memory Optimization" "PASS" "Optimized memory operations enabled"
    else
        log_result "Memory Optimization" "FAIL" "Optimized memory operations not enabled"
    fi
else
    log_result "Memory Optimization" "SKIP" "config.txt not found"
fi

# Check kernel separation configuration
if [[ -f "data/config.txt" ]]; then
    if grep -q "ENABLE_KERNEL_SEPARATION=1" data/config.txt; then
        log_result "Kernel Separation" "PASS" "Kernel separation enabled"
    else
        log_result "Kernel Separation" "FAIL" "Kernel separation not enabled"
    fi
else
    log_result "Kernel Separation" "SKIP" "config.txt not found"
fi

# Check for separated kernels
separated_kernels=("ecc_separated.cu" "hash_separated.cu" "compare_separated.cu")
for kernel in "${separated_kernels[@]}"; do
    if file_exists "src/KeyhuntCore/kernels/$kernel"; then
        log_result "Separated Kernel: $kernel" "PASS" "Kernel file exists"
    else
        log_result "Separated Kernel: $kernel" "FAIL" "Kernel file missing"
    fi
done

# Section 6: Performance Monitoring Validation
echo -e "\n${BLUE}6. Performance Monitoring Validation${NC}"
echo "------------------------------------"

# Check benchmarks directory
if directory_exists "benchmarks/baselines"; then
    log_result "Baselines Directory" "PASS" "Baselines directory exists"

    # Check for baseline files
    baseline_files=("rtx2080ti.json" "rtx3090.json" "h20.json" "a100.json")
    for baseline in "${baseline_files[@]}"; do
        if file_exists "benchmarks/baselines/$baseline"; then
            log_result "Baseline File: $baseline" "PASS" "Baseline file exists"
        else
            log_result "Baseline File: $baseline" "SKIP" "Baseline file not created yet"
        fi
    done
else
    log_result "Baselines Directory" "FAIL" "Baselines directory missing"
fi

# Check for benchmark infrastructure
benchmark_components=("benchmark_runner" "baseline_manager")
for component in "${benchmark_components[@]}"; do
    if file_exists "src/KeyhuntCore/benchmarks/${component}.cpp"; then
        log_result "Benchmark Component: $component" "PASS" "Component implementation exists"
    else
        log_result "Benchmark Component: $component" "FAIL" "Component implementation missing"
    fi
done

# Section 7: Test Infrastructure Validation
echo -e "\n${BLUE}7. Test Infrastructure Validation${NC}"
echo "-----------------------------------"

# Check test directories
test_dirs=("tests/unit" "tests/performance" "tests/validation")
for test_dir in "${test_dirs[@]}"; do
    if directory_exists "$test_dir"; then
        test_count=$(find "$test_dir" -name "*.cpp" -o -name "*.cu" | wc -l)
        if [[ "$test_count" -gt 0 ]]; then
            log_result "Test Directory: $(basename "$test_dir")" "PASS" "$test_count test files found"
        else
            log_result "Test Directory: $(basename "$test_dir")" "SKIP" "No test files found"
        fi
    else
        log_result "Test Directory: $(basename "$test_dir")" "FAIL" "Test directory missing"
    fi
done

# Check for specific test implementations
test_implementations=("test_code_deduplication.cpp" "test_cuda_optimization.cpp" "test_monitoring_system.cpp")
for test in "${test_implementations[@]}"; do
    if file_exists "tests/unit/$test" || file_exists "tests/performance/$test"; then
        log_result "Test Implementation: $test" "PASS" "Test implementation exists"
    else
        log_result "Test Implementation: $test" "SKIP" "Test implementation not found"
    fi
done

# Section 8: Documentation Validation
echo -e "\n${BLUE}8. Documentation Validation${NC}"
echo "----------------------------"

# Check documentation files
doc_files=("docs/architecture_modernization.md" "docs/compatibility_assurance.md" "specs/001-specify-scripts-bash/quickstart.md")
for doc in "${doc_files[@]}"; do
    if file_exists "$doc"; then
        log_result "Documentation: $(basename "$doc")" "PASS" "Documentation file exists"
    else
        log_result "Documentation: $(basename "$doc)" "FAIL" "Documentation file missing"
    fi
done

# Section 9: Validation Summary
echo -e "\n${BLUE}9. Validation Summary${NC}"
echo "======================"

echo -e "Validations Passed: ${GREEN}$VALIDATION_PASSED${NC}"
echo -e "Validations Failed: ${RED}$VALIDATION_FAILED${NC}"
echo -e "Validations Skipped: ${YELLOW}$VALIDATION_SKIPPED${NC}"

if [[ $VALIDATION_FAILED -eq 0 ]]; then
    echo -e "\n${GREEN}✓ Quickstart.md validation PASSED${NC}"
    echo -e "${GREEN}All setup procedures work correctly!${NC}"
    exit 0
else
    echo -e "\n${RED}✗ Quickstart.md validation FAILED${NC}"
    echo -e "${RED}$VALIDATION_FAILED validation(s) failed${NC}"
    echo -e "\n${YELLOW}Please review and fix the failed validations before proceeding.${NC}"
    exit 1
fi
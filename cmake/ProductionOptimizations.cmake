# Puzzle71 CUDA Technical Debt Repair System
# Production Build Optimizations (T079)
# Enhanced CMake configuration for production deployment

cmake_minimum_required(VERSION 3.22)

# Production build configuration
set(PRODUCTION_BUILD TRUE CACHE BOOL "Production build with optimizations")

# Performance optimization flags
option(ENABLE_AGGRESSIVE_OPTIMIZATIONS "Enable aggressive compiler optimizations" ON)
option(ENABLE_LINK_TIME_OPTIMIZATION "Enable link-time optimization (LTO)" ON)
option(ENABLE_PROFILE_GUIDED_OPTIMIZATION "Enable profile-guided optimization" OFF)
option(ENABLE_BUILD_TIME_PROFILING "Enable build-time profiling" OFF)

# Build type optimizations
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type for production")

# Release configuration optimizations
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CUDA_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-O3")

# Aggressive optimization flags
if(ENABLE_AGGRESSIVE_OPTIMIZATIONS)
    list(APPEND CMAKE_CXX_FLAGS_RELEASE
        -march=native
        -mtune=native
        -flto
        -ffast-math
        -funroll-loops
        -fno-semantic-interposition
    )

    list(APPEND CMAKE_CUDA_FLAGS_RELEASE
        -Xcompiler=-march=native
        -Xcompiler=-mtune=native
        -Xcompiler=-O3
        -Xcompiler=-ffast-math
        -Xcompiler=-funroll-loops
    )
endif()

# Link-time optimization
if(ENABLE_LINK_TIME_OPTIMIZATION)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CUDA_FLAGS_RELEASE} -flto")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")

    # Required for LTO
    set(CMAKE_AR "gcc-ar")
    set(CMAKE_RANLIB "gcc-ranlib")
    set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_APPEND "<CMAKE_AR> rs <TARGET> <OBJECTS>")
endif()

# CUDA-specific optimizations
set(CMAKE_CUDA_ARCHITECTURES "75-real;80-real;86-real;89-real;90-real" CACHE STRING "CUDA architectures")
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON CACHE BOOL "Enable CUDA separable compilation")

# Memory optimization
set_property(CACHE PROPERTY CUDA_RESOLVE_DEVICE_SYMBOLS ON)
set_property(CACHE PROPERTY CUDA_DELAY_LOADING_INITIALIZATION_SYMBOLS ON)
set_property(CACHE PROPERTY CUDA_ATTACH_SEPARABLE_COMPILATION_OBJECTS ON)

# Build parallelism
include(ProcessorCount)
ProcessorCount(N)
if(NOT DEFINED CMAKE_BUILD_PARALLEL_LEVEL OR CMAKE_BUILD_PARALLEL_LEVEL EQUALS 0)
    set(CMAKE_BUILD_PARALLEL_LEVEL ${N} CACHE STRING "Parallel build level")
endif()

# Unity build for faster compilation
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.14")
    # Unity build for C++ files
    set(CMAKE_UNITY_BUILD TRUE CACHE BOOL "Enable unity build for C++")
    set(CMAKE_UNITY_BUILD_BATCH_SIZE 32 CACHE STRING "Unity build batch size")

    # Unity build for CUDA files (CMake 3.18+)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
        set(CMAKE_CUDA_UNITY_BUILD TRUE CACHE BOOL "Enable unity build for CUDA")
        set(CMAKE_CUDA_UNITY_BUILD_BATCH_SIZE 8 CACHE STRING "CUDA unity build batch size")
    endif()
endif()

# Precompiled headers
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    # Enable precompiled headers
    target_precompile_headers(Puzzle71Solver PRIVATE)

    # PCH for common headers
    target_precompile_headers(Puzzle71Solver PRIVATE
        <CUDAToolkit_INCLUDE_DIRS>
        <src>
        <src/KeyhuntCore/common>
        <src/KeyhuntCore/kernels>
    )
endif()

# Output directory configuration
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Build artifact management
set(CMAKE_DEBUG_POSTFIX "" CACHE STRING "Debug build suffix")
set(CMAKE_RELEASE_POSTFIX "" CACHE STRING "Release build suffix")

# Build timestamps and versioning
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S" UTC)
string(TIMESTAMP BUILD_DATE "%Y%m%d" UTC)
set(BUILD_TIMESTAMP "${BUILD_TIMESTAMP}" CACHE STRING "Build timestamp")

# Version management
set(PUZZLE71_VERSION "${PROJECT_VERSION}" CACHE STRING "Puzzle71 version")
set(PUZZLE71_BUILD_NUMBER "${BUILD_DATE}.${CMAKE_BUILD_TYPE}" CACHE STRING "Build number")

# Deployment configuration
set(DEPLOYMENT_NAME "puzzle71-${PUZZLE71_VERSION}" CACHE STRING "Deployment package name")
set(DEPLOYMENT_ARCHIVE_NAME "${DEPLOYMENT_NAME}-${CMAKE_SYSTEM_PROCESSOR}" CACHE STRING "Archive name")

# Performance metrics collection
option(COLLECT_BUILD_METRICS "Collect build performance metrics" ON)
if(COLLECT_BUILD_METRICS)
    set(BUILD_METRICS_FILE "${CMAKE_BINARY_DIR}/build_metrics.json" CACHE STRING "Build metrics output file")
endif()

# Build optimization targets
add_custom_target(optimize-build
    COMMAND ${CMAKE_COMMAND} -E echo "Optimizing build for production..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target Puzzle71Solver --parallel ${CMAKE_BUILD_PARALLEL_LEVEL}
    COMMAND ${CMAKE_COMMAND} -E echo "Production build optimization complete"
    COMMENT "Optimize build for production deployment"
)

# Clean build with optimized configuration
add_custom_target(clean-optimized
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaning optimized build..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMENT "Clean build artifacts for fresh rebuild"
)

# Fast build target (development)
add_custom_target(build-fast
    COMMAND ${CMAKE_COMMAND} -E echo "Building with fast configuration..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target Puzzle71Solver --parallel ${CMAKE_BUILD_PARALLEL_LEVEL}
    COMMENT "Fast build target for development"
)

# Production build target
add_custom_target(build-production
    COMMAND ${CMAKE_COMMAND} -E echo "Building for production deployment..."
    COMMAND ${CMAKE_COMMAND} -E echo "Build timestamp: ${BUILD_TIMESTAMP}"
    COMMAND ${CMAKE_COMMAND} -E echo "CUDA architectures: ${CMAKE_CUDA_ARCHITECTURES}"
    COMMAND ${CMAKE_COMMAND} -E echo "Parallel jobs: ${CMAKE_BUILD_PARALLEL_LEVEL}"
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target Puzzle71Solver --parallel ${CMAKE_BUILD_PARALLEL_LEVEL}
    COMMAND ${CMAKE_COMMAND} -E echo "Production build complete"
    COMMENT "Build optimized for production deployment"
)

# Parallel compilation verification
add_custom_target(verify-parallel-build
    COMMAND ${CMAKE_COMMAND} -E echo "Verifying parallel build configuration..."
    COMMAND ${CMAKE_COMMAND} -E "Available CPU cores: ${N}"
    COMMAND ${CMAKE_COMMAND} -E "Parallel build level: ${CMAKE_BUILD_PARALLEL_LEVEL}"
    COMMAND ${CMAKE_COMMAND} -E "CMake parallel build support: ${CMAKE_VERSION}"
    COMMAND ${CMAKE_COMMAND} -E "Unity build: ${CMAKE_UNITY_BUILD}"
    COMMAND ${CMAKE_COMMAND} -E "CUDA unity build: ${CMAKE_CUDA_UNITY_BUILD}"
    COMMENT "Verify parallel build configuration"
)

# Cache warming (for subsequent builds)
add_custom_target(warm-build-cache
    COMMAND ${CMAKE_COMMAND} -E echo "Warming build cache..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target Puzzle71Solver --parallel 1
    COMMAND ${CMAKE_COMMAND} -E "Build cache warmed"
    COMMENT "Warm build cache for faster subsequent builds"
)

# Build performance analysis
add_custom_target(analyze-build-performance
    COMMAND ${CMAKE_COMMAND} -E echo "Analyzing build performance..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target Puzzle71Solver --parallel ${CMAKE_BUILD_PARALLEL_LEVEL}
        --timing
    COMMAND ${CMAKE_COMMAND} -E "Build performance analysis complete"
    COMMENT "Analyze build performance and identify bottlenecks"
)

# Memory usage optimization
add_custom_target(optimize-memory-usage
    COMMAND ${CMAKE_COMMAND} -E "Optimizing build memory usage..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target Puzzle71Solver --parallel $((CMAKE_BUILD_PARALLEL_LEVEL / 2))
    COMMAND ${CMAKE_COMMAND} -E "Memory-optimized build complete"
    COMMENT "Build with reduced parallelism for memory-constrained systems"
)

# Resource-constrained build (for CI/limited environments)
add_custom_target(build-constrained
    COMMAND ${CMAKE_COMMAND} -E "Building with resource constraints..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target Puzzle71Solver --parallel 1
    COMMAND ${CMAKE_COMMAND} -E "Resource-constrained build complete"
    COMMENT "Build with minimal resource usage"
)

# Complete build optimization pipeline
add_custom_target(build-optimized-production
    COMMAND ${CMAKE_COMMAND} -E "Running optimized production build pipeline..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target verify-parallel-build
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target build-production
    COMMAND ${CMAKE_COMMAND} -E "Optimized production build complete"
    COMMENT "Complete optimized production build pipeline"
)

# Build quality validation
add_custom_target(validate-build-quality
    COMMAND ${CMAKE_COMMAND} -E "Validating build quality..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target Puzzle71Solver
    COMMAND ${CMAKE_COMMAND} -E "Build quality validation complete"
    COMMENT "Validate build meets quality standards"
)

# Performance benchmarking for build system
add_custom_target(benchmark-build-system
    COMMAND ${CMAKE_COMMAND} -E "Benchmarking build system performance..."
    COMMAND ${CMAKE_COMMAND} -E echo "Starting benchmark at: $(date)"
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean-optimized
    COMMAND ${CMAKE_COMMAND} -E "Clean completed at: $(date)"
    COMMAND ${CMAKE_COMMAND} -E "Starting production build at: $(date)"
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target build-production
    COMMAND ${CMAKE_COMMAND} -E "Production build completed at: $(date)"
    COMMAND ${CMAKE_COMMAND} -E "Build system benchmark complete"
    COMMENT "Benchmark build system performance"
)

# Cross-platform build validation
add_custom_target(validate-cross-platform-build
    COMMAND ${CMAKE_COMMAND} -E "Validating cross-platform build compatibility..."
    COMMAND ${CMAKE_COMMAND} -E "Build system: ${CMAKE_SYSTEM_NAME}"
    COMMAND ${CMAKE_COMMAND} -E "Processor: ${CMAKE_SYSTEM_PROCESSOR}"
    COMMAND ${CMAKE_COMMAND} -E "Compiler: ${CMAKE_CXX_COMPILER_ID}"
    COMMAND ${CMAKE_COMMAND} -E "CUDA version: ${CMAKE_CUDA_COMPILER_VERSION}"
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target Puzzle71Solver
    COMMAND ${CMAKE_COMMAND} -E "Cross-platform build validation complete"
    COMMENT "Validate build works across platforms"
)

# Configuration summary generation
add_custom_target(show-build-config)
    COMMAND ${CMAKE_COMMAND} -E "Puzzle71 Production Build Configuration Summary"
    COMMAND ${CMAKE_COMMAND} -E "=================================="
    COMMAND ${CMAKE_COMMAND} -E "Version: ${PUZZLE71_VERSION}"
    COMMAND ${CMAKE_COMMAND} -E "Build Type: ${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} -E "CUDA Architectures: ${CMAKE_CUDA_ARCHITECTURES}"
    COMMAND ${CMAKE_COMMAND} -E "Parallel Jobs: ${CMAKE_BUILD_PARALLEL_LEVEL}"
    COMMAND ${CMAKE_COMMAND} -E "Aggressive Optimizations: ${ENABLE_AGGRESSIVE_OPTIMIZATIONS}"
    COMMAND ${CMAKE_COMMAND} -E "LTO: ${ENABLE_LINK_TIME_OPTIMIZATION}"
    COMMAND ${CMAKE_COMMAND} -E "Unity Build: ${CMAKE_UNITY_BUILD}"
    COMMAND ${CMAKE_COMMAND} -E "CUDA Unity: ${CMAKE_CUDA_UNITY_BUILD}"
    COMMAND ${CMAKE_COMMAND} -E "Separable Compilation: ${CMAKE_CUDA_SEPARABLE_COMPILATION}"
    COMMAND ${CMAKE_COMMAND} -E "=================================="
    COMMENT "Display current build configuration"
)

# Set build configuration variables for other targets
set(BUILD_OPTIMIZED TRUE CACHE BOOL "Build is optimized for production")

# Configure optimization variables for use in other CMake files
set(OPTIMIZATION_FLAGS
    "$<$<CONFIG:Release>:${CMAKE_CXX_FLAGS_RELEASE}>"
    "$<$<CONFIG:Debug>:${CMAKE_CXX_FLAGS_DEBUG}>"
)

message(STATUS "Production build optimizations configured:")
message(STATUS "  - Aggressive optimizations: ${ENABLE_AGGRESSIVE_OPTIMIZATIONS}")
message(STATUS "  - Link-time optimization: ${ENABLE_LINK_TIME_OPTIMIZATION}")
message(STATUS "  - Unity build: ${CMAKE_UNITY_BUILD}")
message(STATUS "  - CUDA unity build: ${CMAKE_CUDA_UNITY_BUILD}")
message(STATUS "  - Parallel build level: ${CMAKE_BUILD_PARALLEL_LEVEL}")
message(STATUS "  - CUDA architectures: ${CMAKE_CUDA_ARCHITECTURES}")
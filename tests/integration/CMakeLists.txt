# Architecture Modernization Integration Tests CMake Configuration
# Comprehensive integration tests for Phase 5: User Story 3 - Architecture Modernization

cmake_minimum_required(VERSION 3.18)
project(ArchitectureModernizationTests)

# C++20 and CUDA requirements
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# Find required packages
find_package(CUDA REQUIRED)
find_package(GTest REQUIRED)

# Enable CUDA separable compilation
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)

# Architecture-specific flags
set(CMAKE_CUDA_ARCHITECTURES "75;80;86;89;90" CACHE STRING "CUDA architectures")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/KeyhuntCore
    ${CMAKE_CURRENT_SOURCE_DIR}/../../external
    ${CUDA_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
)

# Source files for architecture modernization tests
set(ARCHITECTURE_TEST_SOURCES
    test_architecture_modernization.cpp
)

# Object library for test compilation (allows device code separation)
cuda_add_library(architecture_test_objs OBJECT
    ${ARCHITECTURE_TEST_SOURCES}
    OPTIONS
        -Xcompiler -Wall
        -Xcompiler -Wextra
        -Xcompiler -Werror
        -Xcompiler -Wno-unused-parameter
        -lineinfo
        -Xptxas=-warn-spills
        -Xptxas=-warn-local-memory
        -Xptxas=-warn-on-local-memory-usage
        --expt-relaxed-constexpr
        --expt-extended-lambda
)

# Main test executable
add_executable(test_architecture_modernization
    $<TARGET_OBJECTS:architecture_test_objs>
)

# Link libraries
target_link_libraries(test_architecture_modernization
    ${CUDA_LIBRARIES}
    ${GTEST_LIBRARIES}
    pthread
    # Core project libraries
    KeyhuntCore_ecc
    KeyhuntCore_memory
    KeyhuntCore_performance
    KeyhuntCore_standards
    KeyhuntCore_utils
    # GPU components
    unified_candidate
    launch_config
    replay_verification
    batch_planner
    # CUDA runtime
    ${CUDA_CUDART_LIBRARY}
)

# CUDA compilation flags
target_compile_options(test_architecture_modernization PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:
        -O3
        --use_fast_math
        -Xptxas=-O3
        -Xptxas=-allow-expensive-optimizations=true
        -Xptxas=-warn-on-local-memory-usage
        -Xcompiler -march=native
        -Xcompiler -mtune=native
    >
    $<$<COMPILE_LANGUAGE:CXX>:
        -O3
        -march=native
        -mtune=native
        -ffast-math
        -Wall
        -Wextra
        -Werror
        -Wno-unused-parameter
    >
)

# Enable separate compilation for device code
set_target_properties(test_architecture_modernization PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)

# Add test to CTest
add_test(NAME ArchitectureModernizationIntegration
         COMMAND test_architecture_modernization)

# Set test properties
set_tests_properties(ArchitectureModernizationIntegration PROPERTIES
    TIMEOUT 300  # 5 minute timeout
    PASS_REGULAR_EXPRESSION ".*\\[  PASSED  \\].*"
)

# Custom target for running architecture tests
add_custom_target(run_architecture_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_architecture_modernization
        --gtest_output=xml:architecture_test_results.xml
        --gtest_filter="*"
    DEPENDS test_architecture_modernization
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running architecture modernization integration tests"
)

# Custom target for running performance-specific tests
add_custom_target(run_architecture_performance_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_architecture_modernization
        --gtest_output=xml:architecture_performance_results.xml
        --gtest_filter="*Performance*"
    DEPENDS test_architecture_modernization
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running architecture performance validation tests"
)

# Custom target for memory validation tests
add_custom_target(run_memory_validation_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_architecture_modernization
        --gtest_output=xml:memory_validation_results.xml
        --gtest_filter="*Memory*"
    DEPENDS test_architecture_modernization
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running memory optimization validation tests"
)

# Custom target for naming convention tests
add_custom_target(run_naming_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_architecture_modernization
        --gtest_output=xml:naming_test_results.xml
        --gtest_filter="*Naming*"
    DEPENDS test_architecture_modernization
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running naming convention validation tests"
)

# Installation rules
install(TARGETS test_architecture_modernization
    RUNTIME DESTINATION bin/tests
)

# Test data generation (if needed)
add_custom_command(TARGET test_architecture_modernization POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/test_data
    COMMENT "Creating test data directory"
)

# Integration with CI system
if(CI_MODE)
    # Enable verbose output for CI
    set_target_properties(test_architecture_modernization PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    # Add custom test configuration for CI
    add_custom_target(ci_architecture_tests
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_architecture_modernization
            --gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/ci_architecture_results.xml
            --gtest_print_time=1
            --gtest_shuffle
        DEPENDS test_architecture_modernization
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running architecture tests in CI mode"
    )
endif()

# Profiling integration (optional)
if(ENABLE_PROFILING)
    # Add profiling flags
    target_compile_options(test_architecture_modernization PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:
            -lineinfo
            -Xptxas=-v
        >
    )

    # Custom target for profiling
    add_custom_target(profile_architecture_tests
        COMMAND nvprof --profile-api-trace all
               ${CMAKE_CURRENT_BINARY_DIR}/test_architecture_modernization
               --gtest_filter="*Performance*"
        DEPENDS test_architecture_modernization
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Profiling architecture test performance"
    )
endif()

# Memory checking integration (optional)
if(ENABLE_MEMORY_CHECK)
    # Custom target for memory checking
    add_custom_target(memcheck_architecture_tests
        COMMAND cuda-memcheck
               ${CMAKE_CURRENT_BINARY_DIR}/test_architecture_modernization
        DEPENDS test_architecture_modernization
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running architecture tests with memory checking"
    )
endif()

# Documentation generation
add_custom_target(generate_architecture_test_docs
    COMMAND ${CMAKE_COMMAND} -E echo "Generating architecture test documentation..."
    COMMAND doxygen ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile 2>/dev/null || true
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating architecture test documentation"
)

# Performance baseline generation
add_custom_target(generate_architecture_baseline
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_architecture_modernization
        --gtest_output=xml:architecture_baseline.xml
        --gtest_filter="*Performance*"
    DEPENDS test_architecture_modernization
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating architecture performance baseline"
)

# Regression testing against baseline
add_custom_target(test_architecture_regression
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/compare_performance.py
        --baseline architecture_baseline.xml
        --current architecture_test_results.xml
        --tolerance 5.0
    DEPENDS test_architecture_modernization
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Testing for architecture performance regressions"
)

# Coverage reporting (if enabled)
if(ENABLE_COVERAGE)
    # Enable coverage flags
    target_compile_options(test_architecture_modernization PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:--coverage>
        $<$<COMPILE_LANGUAGE:CXX>:-fprofile-arcs>
        $<$<COMPILE_LANGUAGE:CXX>:-ftest-coverage>
    )

    target_link_libraries(test_architecture_modernization PRIVATE --coverage)

    # Generate coverage report
    add_custom_target(architecture_coverage
        COMMAND gcov ${ARCHITECTURE_TEST_SOURCES}
        COMMAND lcov --capture --directory . --output-file architecture_coverage.info
        COMMAND genhtml architecture_coverage.info --output-directory coverage_html
        DEPENDS test_architecture_modernization
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating architecture test coverage report"
    )
endif()

# Help target
add_custom_target(help_architecture_tests
    COMMAND ${CMAKE_COMMAND} -E echo "Architecture Modernization Integration Tests:"
    COMMAND ${CMAKE_COMMAND} -E echo "  make run_architecture_tests           - Run all architecture tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  make run_architecture_performance_tests - Run performance validation tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  make run_memory_validation_tests       - Run memory optimization tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  make run_naming_tests                - Run naming convention tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  make generate_architecture_baseline   - Generate performance baseline"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test_architecture_regression     - Test for regressions"
    COMMENT "Display architecture test help"
)
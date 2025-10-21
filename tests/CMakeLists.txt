cmake_minimum_required(VERSION 3.22)

project(TechDebtRepairTests VERSION 1.0.0 LANGUAGES CXX CUDA)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# Find required packages
find_package(CUDAToolkit REQUIRED)
find_package(GTest REQUIRED)

# Include directories - Updated T031
include_directories(
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/KeyhuntCore/common
  ${CMAKE_SOURCE_DIR}/src/KeyhuntCore/validation
  ${CMAKE_SOURCE_DIR}/src/KeyhuntCore/monitoring
  ${CMAKE_SOURCE_DIR}/src/KeyhuntCore/benchmarks
  ${CMAKE_SOURCE_DIR}/src/KeyhuntCore/gpu
  ${CMAKE_SOURCE_DIR}/src/KeyhuntCore/memory
  ${CMAKE_SOURCE_DIR}/src/KeyhuntCore/utils
  ${CMAKE_SOURCE_DIR}/src/compute/gpu
  ${CMAKE_SOURCE_DIR}/src/config
  ${CMAKE_SOURCE_DIR}/src/utils
  ${CMAKE_SOURCE_DIR}/tests
  ${CMAKE_SOURCE_DIR}/tests/unit
  ${CMAKE_SOURCE_DIR}/tests/validation
  ${CMAKE_SOURCE_DIR}/tests/performance
  ${CMAKE_SOURCE_DIR}/tests/integration
  ${CMAKE_SOURCE_DIR}/tests/architecture
  ${CMAKE_SOURCE_DIR}/tests/migration
  ${CMAKE_SOURCE_DIR}/tests/compatibility
  ${CMAKE_SOURCE_DIR}/tests/coverage
  ${CMAKE_SOURCE_DIR}/tests/benchmark
  ${CMAKE_SOURCE_DIR}/tests/testing
  ${CUDAToolkit_INCLUDE_DIRS}
)

# Test sources - Updated T031
set(TEST_SOURCES
  # Unit tests for User Story 1 (Enhanced)
  tests/unit/test_ecc_operations.cpp
  tests/unit/test_ecc_operations_unified.cpp
  tests/unit/test_config_validation.cpp
  tests/unit/test_legacy_adapter.cpp
  tests/unit/test_static_launch_config_unified.cpp
  tests/unit/test_hash_utils_unified.cpp
  tests/unit/test_result_emitter_unified.cpp
  tests/unit/test_code_deduplication.cpp

  # Unit tests for User Story 2
  tests/unit/test_unified_scanner.cpp
  tests/unit/test_memory_optimization.cpp

  # Performance tests for User Story 2 (Enhanced)
  tests/performance/test_gpu_utilization.cpp
  tests/performance/test_memory_efficiency.cpp
  tests/performance/test_synchronization_overhead.cpp
  tests/performance/performance_validation_tests.cpp
  tests/performance/performance_validation_tests.cuh
  tests/performance/kernel_config_validation_benchmark.cpp
  tests/benchmark/test_performance_validation.cpp

  # Unit tests for User Story 4 (Enhanced)
  tests/validation/test_ecc_validation.cpp
  tests/validation/test_deterministic_replay.cpp
  tests/validation/test_constitutional_compliance.cpp
  tests/validation/test_batch_step_increment.cpp
  tests/validation/test_cpu_gpu_parity.cpp
  tests/validation/test_endomorphism_split.cpp
  tests/validation/test_hash160_gpu_cpu_parity.cu
  tests/validation/test_kernel_config_validation.cpp
  tests/validation/test_known_private_key_chain.cpp
  tests/validation/test_sha256_baseline_validator.cpp
  tests/validation/test_comprehensive_validation_system.cpp

  # Integration tests (Enhanced)
  tests/integration/test_end_to_end_kernel.cpp
  tests/integration/test_performance_regression.cpp
  tests/integration/test_adapter_layer.cpp
  tests/integration/test_ecc_adapter_integration.cpp
  tests/integration/test_architecture_modernization.cpp
  tests/integration/test_unified_modules_integration.cpp

  # Architecture tests
  tests/architecture/test_architectural_compliance.cpp

  # Migration tests
  tests/migration/test_legacy_removal.cpp

  # Compatibility tests
  tests/compatibility/test_compatibility_validation.cpp
  tests/test_compatibility_validation.cpp

  # Coverage tests
  tests/coverage/test_coverage_analysis.cpp
  tests/testing/test_coverage_monitor.cpp
)

# Create test executable
add_executable(techdebt_repair_tests ${TEST_SOURCES})

# Link libraries
target_link_libraries(techdebt_repair_tests PRIVATE
  GTest::gtest_main
  GTest::gtest
  GTest::gmock
  CUDA::cudart
  techdebt_repair
)

# Add OpenSSL and other dependencies if available
if(OPENSSL_FOUND)
  target_link_libraries(techdebt_repair_tests PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

if(TARGET nlohmann_json::nlohmann_json)
  target_link_libraries(techdebt_repair_tests PRIVATE nlohmann_json::nlohmann_json)
endif()

if(TARGET secp256k1)
  target_link_libraries(techdebt_repair_tests PRIVATE secp256k1)
endif()

# Set target properties - Updated T031
set_target_properties(techdebt_repair_tests PROPERTIES
  CUDA_ARCHITECTURES "75;80;86;89;90"
  CUDA_SEPARABLE_COMPILATION ON
)

# Enable testing
enable_testing()

# Add test cases
add_test(NAME techdebt_repair_unit_tests COMMAND techdebt_repair_tests --gtest_filter="Unit.*")
add_test(NAME techdebt_repair_integration_tests COMMAND techdebt_repair_tests --gtest_filter="Integration.*")
add_test(NAME techdebt_repair_validation_tests COMMAND techdebt_repair_tests --gtest_filter="Validation.*")
add_test(NAME techdebt_repair_architecture_tests COMMAND techdebt_repair_tests --gtest_filter="Architecture.*")
add_test(NAME techdebt_repair_migration_tests COMMAND techdebt_repair_tests --gtest_filter="Migration.*")
add_test(NAME techdebt_repair_performance_tests COMMAND techdebt_repair_tests --gtest_filter="GPUUtilizationTest.*")
add_test(NAME techdebt_repair_all_tests COMMAND techdebt_repair_tests)

# Set test properties
set_tests_properties(techdebt_repair_unit_tests PROPERTIES TIMEOUT 300)
set_tests_properties(techdebt_repair_integration_tests PROPERTIES TIMEOUT 600)
set_tests_properties(techdebt_repair_validation_tests PROPERTIES TIMEOUT 900)
set_tests_properties(techdebt_repair_architecture_tests PROPERTIES TIMEOUT 300)
set_tests_properties(techdebt_repair_migration_tests PROPERTIES TIMEOUT 300)
set_tests_properties(techdebt_repair_performance_tests PROPERTIES TIMEOUT 600)
set_tests_properties(techdebt_repair_all_tests PROPERTIES TIMEOUT 1800)
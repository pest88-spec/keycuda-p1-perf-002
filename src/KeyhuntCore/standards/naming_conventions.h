// Puzzle71Solver - Naming Conventions Standard
// Architecture Modernization - Standardized naming conventions (T037)
//
// This header defines the standardized naming conventions for the entire
// Puzzle71Solver project to ensure consistency and maintainability.
//
// Naming Convention Standards:
// 1. Functions: camelCase (first letter lowercase, subsequent words capitalized)
//    - Examples: emitCandidate(), finalizeDigest(), getDeviceCount()
//    - Legacy: EmitCandidate() → emitCandidate()
//    - Legacy: FinalizeDigest() → finalizeDigest()
//
// 2. Variables: snake_case (all lowercase, words separated by underscores)
//    - Examples: device_count, batch_size, target_hash
//
// 3. Constants/Enumerations: PascalCase (all words capitalized)
//    - Examples: MaxBatchSize, DefaultThreadsPerBlock
//    - Legacy: MAX_BATCH_SIZE → MaxBatchSize
//
// 4. Classes/Structs: PascalCase (all words capitalized)
//    - Examples: GpuExecutor, SeparatedKernelExecutor, BatchConfig
//    - (No change needed - already follows convention)
//
// 5. Member Variables: snake_case with trailing underscore
//    - Examples: device_id_, batch_size_, target_hash_
//    - (No change needed - already follows convention)
//
// 6. Namespaces: lowercase (single word or snake_case for multiple)
//    - Examples: keyhunt, puzzle71, common::utils
//    - (No change needed - already follows convention)
//
// 7. Template Parameters: PascalCase with descriptive T_ prefix
//    - Examples: T_DeviceType, T_AllocatorType
//
// 8. CUDA Kernels: camelCase with _kernel suffix
//    - Examples: eccScalarMul_kernel(), hashSha256_kernel()
//    - Legacy: ecc_scalar_mul_kernel → eccScalarMul_kernel()
//
// 9. Device Functions: camelCase
//    - Examples: emitCandidate(), finalizeDigest(), computeEccPoint()
//
// 10. File Names: snake_case
//     - Examples: naming_conventions.h, gpu_executor.cpp
//     - (No change needed - already follows convention)

#pragma once

#include <string>

namespace keyhunt {
namespace standards {

/**
 * @brief Naming convention validation utilities
 */
class NamingConventions {
public:
    /**
     * @brief Check if a function name follows camelCase convention
     * @param name Function name to check
     * @return true if follows camelCase, false otherwise
     */
    static bool isCamelCase(const std::string& name);

    /**
     * @brief Check if a variable name follows snake_case convention
     * @param name Variable name to check
     * @return true if follows snake_case, false otherwise
     */
    static bool isSnakeCase(const std::string& name);

    /**
     * @brief Check if a constant name follows PascalCase convention
     * @param name Constant name to check
     * @return true if follows PascalCase, false otherwise
     */
    static bool isPascalCase(const std::string& name);

    /**
     * @brief Convert PascalCase to camelCase
     * @param name PascalCase name to convert
     * @return camelCase version of the name
     */
    static std::string pascalToCamel(const std::string& name);

    /**
     * @brief Convert snake_case to camelCase
     * @param name snake_case name to convert
     * @return camelCase version of the name
     */
    static std::string snakeToCamel(const std::string& name);

    /**
     * @brief Suggest standardized name for legacy naming
     * @param legacy_name Legacy function/variable name
     * @param type Type of identifier (function, variable, constant)
     * @return Suggested standardized name
     */
    static std::string suggestStandardName(const std::string& legacy_name, const std::string& type);
};

// Predefined standard names for common patterns
namespace StandardNames {
    // Function names (camelCase)
    constexpr const char* EMIT_CANDIDATE = "emitCandidate";
    constexpr const char* FINALIZE_DIGEST = "finalizeDigest";
    constexpr const char* COMPUTE_ECC_POINT = "computeEccPoint";
    constexpr const char* HASH_SHA256 = "hashSha256";
    constexpr const char* HASH_RIPEMD160 = "hashRipemd160";
    constexpr const char* COMPARE_DIGEST = "compareDigest";
    constexpr const char* PREPARE_BATCH = "prepareBatch";
    constexpr const char* EXECUTE_KERNEL = "executeKernel";
    constexpr const char* GET_DEVICE_COUNT = "getDeviceCount";
    constexpr const char* SET_DEVICE = "setDevice";

    // Constant names (PascalCase)
    constexpr const char* MAX_BATCH_SIZE = "MaxBatchSize";
    constexpr const char* DEFAULT_THREADS_PER_BLOCK = "DefaultThreadsPerBlock";
    constexpr const char* MAX_WARP_SIZE = "MaxWarpSize";
    constexpr const char* GPU_MEMORY_ALIGNMENT = "GpuMemoryAlignment";

    // Variable names (snake_case)
    constexpr const char* DEVICE_COUNT = "device_count";
    constexpr const char* BATCH_SIZE = "batch_size";
    constexpr const char* TARGET_HASH = "target_hash";
    constexpr const char* PRIVATE_KEY = "private_key";
    constexpr const char* PUBLIC_KEY = "public_key";
}

} // namespace standards
} // namespace keyhunt
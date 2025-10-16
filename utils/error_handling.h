/**
 * Unified Error Handling Utilities
 * 
 * Implements P2-001: Code Duplication Optimization
 * - CUDA error checking macros
 * - Exception handling utilities
 * - Error logging and reporting
 * - DRY principle compliance
 * 
 * @origin       https://github.com/Puzzle71Solver/Puzzle71Solver
 * @origin_path  src/utils/error_handling.h
 * @origin_commit <current_commit>
 * @origin_license MIT
 * @extracted_date   2025-10-13
 * @extracted_by     Puzzle71Solver Team
 * @modifications    Created for P2-001 code duplication optimization
 * @spdx_license_identifier MIT
 */

#pragma once

#include <cuda_runtime.h>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream>

namespace puzzle71 {
namespace utils {

/**
 * CUDA Error Checking Macros
 * 
 * Provides unified error checking for CUDA API calls.
 * Throws std::runtime_error with detailed error information.
 */

/**
 * Check CUDA error and throw exception if error detected
 * 
 * Usage:
 *   CUDA_CHECK(cudaMalloc(&ptr, size));
 *   CUDA_CHECK(cudaMemcpy(dst, src, size, cudaMemcpyHostToDevice));
 */
#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            std::ostringstream oss; \
            oss << "CUDA error at " << __FILE__ << ":" << __LINE__ \
                << " in " << __FUNCTION__ << "()\n" \
                << "  Call: " << #call << "\n" \
                << "  Error: " << cudaGetErrorString(err) \
                << " (code " << static_cast<int>(err) << ")"; \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

/**
 * Check CUDA kernel launch errors
 * 
 * Usage:
 *   myKernel<<<grid, block>>>(args...);
 *   CUDA_CHECK_KERNEL();
 */
#define CUDA_CHECK_KERNEL() \
    do { \
        cudaError_t err = cudaGetLastError(); \
        if (err != cudaSuccess) { \
            std::ostringstream oss; \
            oss << "CUDA kernel launch error at " << __FILE__ << ":" << __LINE__ \
                << " in " << __FUNCTION__ << "()\n" \
                << "  Error: " << cudaGetErrorString(err) \
                << " (code " << static_cast<int>(err) << ")"; \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

/**
 * Synchronize device and check for kernel execution errors
 * 
 * Usage:
 *   myKernel<<<grid, block>>>(args...);
 *   CUDA_SYNC_CHECK();
 */
#define CUDA_SYNC_CHECK() \
    do { \
        cudaError_t err = cudaGetLastError(); \
        if (err != cudaSuccess) { \
            std::ostringstream oss; \
            oss << "CUDA kernel launch error at " << __FILE__ << ":" << __LINE__ \
                << " in " << __FUNCTION__ << "()\n" \
                << "  Error: " << cudaGetErrorString(err) \
                << " (code " << static_cast<int>(err) << ")"; \
            throw std::runtime_error(oss.str()); \
        } \
        err = cudaDeviceSynchronize(); \
        if (err != cudaSuccess) { \
            std::ostringstream oss; \
            oss << "CUDA kernel execution error at " << __FILE__ << ":" << __LINE__ \
                << " in " << __FUNCTION__ << "()\n" \
                << "  Error: " << cudaGetErrorString(err) \
                << " (code " << static_cast<int>(err) << ")"; \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

/**
 * Exception Handling Utilities
 */

/**
 * Try-catch wrapper with error logging
 * 
 * Usage:
 *   TRY_CATCH_LOG("Operation name", {
 *       // Code that may throw
 *   });
 */
#define TRY_CATCH_LOG(operation_name, code_block) \
    try { \
        code_block \
    } catch (const std::exception& e) { \
        std::cerr << "ERROR in " << operation_name << ": " \
                  << e.what() << std::endl; \
        throw; \
    } catch (...) { \
        std::cerr << "UNKNOWN ERROR in " << operation_name << std::endl; \
        throw; \
    }

/**
 * Try-catch wrapper with error logging and return value
 * 
 * Usage:
 *   bool success = TRY_CATCH_RETURN("Operation", false, {
 *       // Code that may throw
 *       return true;
 *   });
 */
#define TRY_CATCH_RETURN(operation_name, default_return, code_block) \
    [&]() { \
        try { \
            code_block \
        } catch (const std::exception& e) { \
            std::cerr << "ERROR in " << operation_name << ": " \
                      << e.what() << std::endl; \
            return default_return; \
        } catch (...) { \
            std::cerr << "UNKNOWN ERROR in " << operation_name << std::endl; \
            return default_return; \
        } \
    }()

/**
 * Error Logging Functions
 */

/**
 * Log error message to stderr
 * 
 * @param context Context or operation name
 * @param message Error message
 */
inline void logError(const std::string& context, const std::string& message) {
    std::cerr << "[ERROR] " << context << ": " << message << std::endl;
}

/**
 * Log warning message to stderr
 * 
 * @param context Context or operation name
 * @param message Warning message
 */
inline void logWarning(const std::string& context, const std::string& message) {
    std::cerr << "[WARNING] " << context << ": " << message << std::endl;
}

/**
 * Log info message to stdout
 * 
 * @param context Context or operation name
 * @param message Info message
 */
inline void logInfo(const std::string& context, const std::string& message) {
    std::cout << "[INFO] " << context << ": " << message << std::endl;
}

/**
 * Validation Utilities
 */

/**
 * Validate pointer is not null
 * 
 * @param ptr Pointer to validate
 * @param name Parameter name for error message
 * @throws std::invalid_argument if pointer is null
 */
inline void validateNotNull(const void* ptr, const char* name) {
    if (!ptr) {
        throw std::invalid_argument(
            std::string("Null pointer: ") + name);
    }
}

/**
 * Validate value is positive
 * 
 * @param value Value to validate
 * @param name Parameter name for error message
 * @throws std::invalid_argument if value <= 0
 */
inline void validatePositive(int value, const char* name) {
    if (value <= 0) {
        std::ostringstream oss;
        oss << "Invalid value for " << name << ": " << value
            << " (must be positive)";
        throw std::invalid_argument(oss.str());
    }
}

/**
 * Validate value is in range [min, max]
 * 
 * @param value Value to validate
 * @param min Minimum allowed value
 * @param max Maximum allowed value
 * @param name Parameter name for error message
 * @throws std::out_of_range if value not in range
 */
inline void validateRange(int value, int min, int max, const char* name) {
    if (value < min || value > max) {
        std::ostringstream oss;
        oss << "Value out of range for " << name << ": " << value
            << " (must be in [" << min << ", " << max << "])";
        throw std::out_of_range(oss.str());
    }
}

} // namespace utils
} // namespace puzzle71


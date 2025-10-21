/**
 * @file deterministic_replay.h
 * @brief Deterministic replay testing infrastructure for Puzzle71 technical debt validation
 *
 * This header defines the infrastructure for deterministic replay testing,
 * ensuring GPU operations produce identical results across multiple runs
 * with SHA-256 protection for constitutional compliance.
 *
 * Requirements Addressed:
 * - Constitutional Principle 1: Deterministic Operations
 * - FR-015: Deterministic replay validation system
 * - T015: Setup deterministic replay testing infrastructure
 *
 * @author Puzzle71 Technical Debt Repair System
 * @version 5.5
 * @date 2025-10-20
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <sha256.h>

namespace puzzle71 {
namespace validation {

/**
 * @brief Structure for replay test input parameters
 *
 * Encapsulates all input data needed to reproduce a specific
 * GPU computation exactly.
 */
struct ReplayTestInput {
    std::string test_id;                    ///< Unique test identifier
    std::string kernel_name;                ///< GPU kernel function name
    std::vector<uint8_t> input_data;        ///< Serialized input parameters
    std::vector<uint32_t> grid_dims;        ///< CUDA grid dimensions (x, y, z)
    std::vector<uint32_t> block_dims;       ///< CUDA block dimensions (x, y, z)
    uint64_t random_seed;                   ///< Random seed for any stochastic operations
    std::chrono::system_clock::time_point timestamp; ///< Test creation timestamp

    /**
     * @brief Serialize input data for storage
     * @return Serialized byte array
     */
    std::vector<uint8_t> serialize() const;

    /**
     * @brief Deserialize input data from storage
     * @param data Serialized byte array
     * @return True if deserialization successful
     */
    bool deserialize(const std::vector<uint8_t>& data);
};

/**
 * @brief Structure for replay test output results
 *
 * Contains the complete output from a GPU computation
 * for validation and comparison purposes.
 */
struct ReplayTestOutput {
    std::string test_id;                    ///< Corresponding test identifier
    std::vector<uint8_t> output_data;       ///< Serialized output results
    std::vector<uint8_t> gpu_state;         ///< GPU state snapshot (if applicable)
    std::string device_info;                ///< GPU device identification
    std::chrono::system_clock::time_point timestamp; ///< Result generation timestamp
    double execution_time_ms;               ///< Kernel execution time in milliseconds

    /**
     * @brief Compute SHA-256 digest of output data
     * @return SHA-256 hash as hex string
     */
    std::string compute_digest() const;

    /**
     * @brief Serialize output data for storage
     * @return Serialized byte array
     */
    std::vector<uint8_t> serialize() const;

    /**
     * @brief Deserialize output data from storage
     * @param data Serialized byte array
     * @return True if deserialization successful
     */
    bool deserialize(const std::vector<uint8_t>& data);
};

/**
 * @brief Replay test session metadata
 *
 * Tracks the complete lifecycle of a replay test session
 * for constitutional compliance validation.
 */
struct ReplaySession {
    std::string session_id;                 ///< Unique session identifier
    std::string description;                ///< Session description/purpose
    std::chrono::system_clock::time_point start_time; ///< Session start time
    std::chrono::system_clock::time_point end_time;   ///< Session end time
    std::vector<std::string> test_ids;      ///< All test IDs in this session
    std::string git_commit_hash;            ///< Git commit hash for reproducibility
    std::string build_configuration;        ///< Build configuration details

    /**
     * @brief Compute session duration
     * @return Duration in seconds
     */
    double duration_seconds() const;

    /**
     * @brief Serialize session metadata
     * @return Serialized byte array
     */
    std::vector<uint8_t> serialize() const;

    /**
     * @brief Deserialize session metadata
     * @param data Serialized byte array
     * @return True if deserialization successful
     */
    bool deserialize(const std::vector<uint8_t>& data);
};

/**
 * @brief Main deterministic replay testing engine
 *
 * Provides comprehensive deterministic replay testing capabilities
 * for GPU operations with SHA-256 protected result validation.
 */
class DeterministicReplayEngine {
public:
    /**
     * @brief Constructor
     * @param base_dir Base directory for storing replay data
     * @param enable_validation Enable CPU-GPU validation
     */
    explicit DeterministicReplayEngine(const std::string& base_dir,
                                     bool enable_validation = true);

    /**
     * @brief Destructor - cleanup resources
     */
    ~DeterministicReplayEngine();

    /**
     * @brief Start a new replay testing session
     * @param description Session description
     * @return Session ID for tracking
     */
    std::string start_session(const std::string& description);

    /**
     * @brief Execute a deterministic replay test
     *
     * @param kernel_func GPU kernel function to test
     * @param input Test input parameters
     * @param output Output result storage
     * @return True if test executed successfully
     */
    bool execute_replay_test(std::function<void(const ReplayTestInput&, ReplayTestOutput&)> kernel_func,
                           const ReplayTestInput& input,
                           ReplayTestOutput& output);

    /**
     * @brief Validate output against stored baseline
     *
     * @param test_id Test identifier
     * @param output Current output to validate
     * @param expected_baseline Expected baseline output (optional, loads from storage if empty)
     * @return True if output matches baseline within tolerance
     */
    bool validate_output(const std::string& test_id,
                        const ReplayTestOutput& output,
                        const ReplayTestOutput* expected_baseline = nullptr);

    /**
     * @brief Replay a stored test and compare results
     *
     * @param session_id Session ID containing the test
     * @param test_id Test ID to replay
     * @param tolerance Maximum allowed relative difference
     * @return True if replay produces identical results
     */
    bool replay_and_compare(const std::string& session_id,
                           const std::string& test_id,
                           double tolerance = 1e-10);

    /**
     * @brief Generate replay test report
     *
     * @param session_id Session ID to generate report for
     * @return JSON formatted report string
     */
    std::string generate_report(const std::string& session_id) const;

    /**
     * @brief Validate session integrity with SHA-256
     *
     * @param session_id Session ID to validate
     * @return True if session data is cryptographically valid
     */
    bool validate_session_integrity(const std::string& session_id) const;

    /**
     * @brief Get all stored session IDs
     * @return Vector of session IDs
     */
    std::vector<std::string> get_session_ids() const;

    /**
     * @brief Clean up old session data
     *
     * @param max_age_days Maximum age in days to keep
     */
    void cleanup_old_sessions(int max_age_days = 30);

private:
    struct Impl;  ///< Forward declaration for implementation
    std::unique_ptr<Impl> pimpl_;  ///< Pimpl idiom for encapsulation

    /**
     * @brief Get file path for stored test data
     * @param session_id Session ID
     * @param test_id Test ID
     * @param is_input True for input data, false for output
     * @return File path
     */
    std::string get_test_file_path(const std::string& session_id,
                                  const std::string& test_id,
                                  bool is_input) const;

    /**
     * @brief Get session metadata file path
     * @param session_id Session ID
     * @return File path
     */
    std::string get_session_file_path(const std::string& session_id) const;

    /**
     * @brief Ensure directory exists for storage
     * @param directory Directory path
     */
    void ensure_directory_exists(const std::string& directory) const;

    /**
     * @brief Read binary data from file
     * @param file_path File path
     * @return Read data
     */
    std::vector<uint8_t> read_binary_file(const std::string& file_path) const;

    /**
     * @brief Write binary data to file
     * @param file_path File path
     * @param data Data to write
     * @return True if write successful
     */
    bool write_binary_file(const std::string& file_path,
                          const std::vector<uint8_t>& data) const;
};

/**
 * @brief RAII helper for automatic session management
 *
 * Automatically starts and ends replay sessions within a scope
 * for convenient resource management.
 */
class ReplaySessionGuard {
public:
    /**
     * @brief Constructor - starts session
     * @param engine Replay engine reference
     * @param description Session description
     */
    ReplaySessionGuard(DeterministicReplayEngine& engine,
                      const std::string& description);

    /**
     * @brief Destructor - ends session and generates report
     */
    ~ReplaySessionGuard();

    /**
     * @brief Get session ID
     * @return Session ID
     */
    const std::string& session_id() const { return session_id_; }

private:
    DeterministicReplayEngine& engine_;  ///< Replay engine reference
    std::string session_id_;            ///< Session ID
    bool session_active_;               ///< Session active flag
};

} // namespace validation
} // namespace puzzle71
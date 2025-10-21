// Puzzle71Solver - Deterministic Replay Verification System (T042)
// Architecture Modernization - Deterministic GPU operation verification
//
// This system provides comprehensive deterministic replay verification for all GPU
// operations, ensuring exact reproducibility across different runs, devices, and
// environments. It validates that GPU operations produce identical results when
// given the same inputs and configuration.
//
// Key Features:
// - Complete GPU state capture and replay
// - Deterministic random number generation
// - Bit-level result verification
// - Cross-device compatibility validation
// - Performance consistency monitoring
// - Comprehensive logging and reporting

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

#include "core/uint256.h"
#include "launch_config.h"
#include "unified_candidate.h"

namespace puzzle71::gpu {

/**
 * @brief Deterministic replay capture state
 *
 * This structure captures all the information needed to exactly reproduce
 * a GPU kernel execution, including inputs, configuration, and random state.
 */
struct ReplayCapture {
    // Identification
    std::string capture_id;
    std::chrono::system_clock::time_point capture_time;
    int device_id;
    std::string device_name;
    std::string kernel_name;

    // Kernel configuration
    KernelLaunchConfig launch_config;
    std::uint64_t deterministic_seed;

    // Input data
    core::UInt256 batch_start;
    std::vector<std::uint32_t> target_hash160;
    bool compressed_format;

    // Device state
    std::vector<unsigned char> device_memory_snapshot;
    std::vector<std::uint32_t> random_state;
    std::vector<std::uint64_t> random_state64;

    // Execution results
    std::vector<UnifiedCandidate> output_candidates;
    std::uint64_t total_keys_processed;
    std::chrono::microseconds execution_time;

    // Verification metadata
    std::string checksum;
    std::array<std::uint8_t, 32> fingerprint;
    bool capture_successful{false};
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    /**
     * @brief Calculate checksum for this capture
     */
    std::string calculateChecksum() const;

    /**
     * @brief Generate fingerprint for this capture
     */
    std::array<std::uint8_t, 32> generateFingerprint() const;

    /**
     * @brief Serialize capture to JSON
     */
    std::string toJson() const;

    /**
     * @brief Deserialize capture from JSON
     */
    static ReplayCapture fromJson(const std::string& json);

    /**
     * @brief Validate capture integrity
     */
    bool validate() const;
};

/**
 * @brief Deterministic replay verification result
 */
struct ReplayVerificationResult {
    bool verification_passed{false};
    bool bit_identical{false};
    bool performance_consistent{false};

    // Result comparison
    size_t candidates_match_count{0};
    size_t candidates_mismatch_count{0};
    size_t total_candidates{0};

    // Performance comparison
    double original_throughput{0.0};
    double replay_throughput{0.0};
    double throughput_difference_percent{0.0};
    std::chrono::microseconds time_difference;

    // Verification details
    std::vector<std::string> mismatch_details;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    // Verification statistics
    std::chrono::system_clock::time_point verification_time;
    std::chrono::microseconds verification_duration;

    /**
     * @brief Get overall verification summary
     */
    std::string getSummary() const;

    /**
     * @brief Check if verification is acceptable
     */
    bool isAcceptable() const {
        return verification_passed &&
               bit_identical &&
               std::abs(throughput_difference_percent) < 10.0;  // 10% tolerance
    }
};

/**
 * @brief Deterministic replay verifier
 *
 * This class provides comprehensive deterministic replay verification for GPU
 * operations, capturing execution state and replaying it to ensure identical
 * results across different environments and time periods.
 */
class DeterministicReplayVerifier {
public:
    explicit DeterministicReplayVerifier(int device_id);
    ~DeterministicReplayVerifier();

    /**
     * @brief Capture a deterministic GPU execution
     *
     * @param kernel_name Name of the kernel being executed
     * @param config Kernel launch configuration
     * @param batch_start Starting scalar for the batch
     * @param target_hash160 Target hash160 to match
     * @param compressed Whether to use compressed format
     * @param deterministic_seed Seed for deterministic execution
     * @return Capture of the execution
     */
    ReplayCapture captureExecution(
        const std::string& kernel_name,
        const KernelLaunchConfig& config,
        const core::UInt256& batch_start,
        const std::vector<std::uint32_t>& target_hash160,
        bool compressed_format,
        std::uint64_t deterministic_seed
    );

    /**
     * @brief Replay a captured execution and verify results
     *
     * @param capture Previously captured execution to replay
     * @param enable_performance_comparison Enable throughput comparison
     * @return Verification result
     */
    ReplayVerificationResult replayAndVerify(
        const ReplayCapture& capture,
        bool enable_performance_comparison = true
    );

    /**
     * @brief Verify two captures produce identical results
     *
     * @param capture1 First execution capture
     * @param capture2 Second execution capture
     * @return Verification result comparing the two captures
     */
    ReplayVerificationResult verifyCaptures(
        const ReplayCapture& capture1,
        const ReplayCapture& capture2
    );

    /**
     * @brief Validate cross-device reproducibility
     *
     * @param capture Original capture
     * @param target_device_id Device to replay on
     * @return Verification result
     */
    ReplayVerificationResult validateCrossDeviceReproducibility(
        const ReplayCapture& capture,
        int target_device_id
    );

    /**
     * @brief Batch verification of multiple captures
     *
     * @param captures Vector of captures to verify
     * @return Vector of verification results
     */
    std::vector<ReplayVerificationResult> batchVerify(
        const std::vector<ReplayCapture>& captures
    );

    /**
     * @brief Export capture to file
     *
     * @param capture Capture to export
     * @param filename Output filename
     * @return true if export successful
     */
    bool exportCapture(const ReplayCapture& capture, const std::string& filename);

    /**
     * @brief Import capture from file
     *
     * @param filename Input filename
     * @return Imported capture or empty optional
     */
    std::optional<ReplayCapture> importCapture(const std::string& filename);

    /**
     * @brief Generate deterministic replay report
     *
     * @param captures Captures to include in report
     * @param output_filename Output report filename
     * @return true if report generated successfully
     */
    bool generateReplayReport(
        const std::vector<ReplayCapture>& captures,
        const std::string& output_filename
    );

    /**
     * @brief Set deterministic random seed
     *
     * @param seed Deterministic seed value
     */
    void setDeterministicSeed(std::uint64_t seed);

    /**
     * @brief Get current deterministic seed
     */
    std::uint64_t getDeterministicSeed() const { return deterministic_seed_; }

    /**
     * @brief Enable/disable performance consistency monitoring
     */
    void setPerformanceMonitoringEnabled(bool enabled) {
        performance_monitoring_enabled_ = enabled;
    }

    /**
     * @brief Get verification statistics
     */
    struct VerificationStats {
        size_t total_verifications{0};
        size_t successful_verifications{0};
        size_t failed_verifications{0};
        double average_throughput_difference{0.0};
        std::chrono::microseconds average_verification_time{0};
    };

    VerificationStats getVerificationStats() const;

    /**
     * @brief Clear verification statistics
     */
    void clearVerificationStats();

private:
    int device_id_;
    std::uint64_t deterministic_seed_;
    bool performance_monitoring_enabled_{true};

    // Verification statistics
    mutable VerificationStats verification_stats_;

    // Device state management
    struct DeviceState {
        std::vector<unsigned char> memory_snapshot;
        std::vector<std::uint32_t> random_state;
        std::vector<std::uint64_t> random_state64;
        bool captured{false};
    };

    /**
     * @brief Capture device state before execution
     */
    DeviceState captureDeviceState();

    /**
     * @brief Restore device state for replay
     */
    bool restoreDeviceState(const DeviceState& state);

    /**
     * @brief Execute kernel with deterministic parameters
     */
    std::vector<UnifiedCandidate> executeKernelDeterministic(
        const std::string& kernel_name,
        const KernelLaunchConfig& config,
        const core::UInt256& batch_start,
        const std::vector<std::uint32_t>& target_hash160,
        bool compressed_format
    );

    /**
     * @brief Compare two result vectors for bit-level equality
     */
    bool compareResults(
        const std::vector<UnifiedCandidate>& results1,
        const std::vector<UnifiedCandidate>& results2,
        ReplayVerificationResult& verification_result
    ) const;

    /**
     * @brief Validate candidate equality
     */
    bool areCandidatesEqual(const UnifiedCandidate& c1, const UnifiedCandidate& c2) const;

    /**
     * @brief Generate detailed mismatch report
     */
    std::string generateMismatchReport(
        const UnifiedCandidate& expected,
        const UnifiedCandidate& actual
    ) const;

    /**
     * @brief Log verification details
     */
    void logVerification(const ReplayVerificationResult& result) const;

    /**
     * @brief Setup deterministic random number generation
     */
    void setupDeterministicRNG();

    /**
     * @brief Create deterministic execution environment
     */
    void createDeterministicEnvironment();

    /**
     * @brief Cleanup deterministic execution environment
     */
    void cleanupDeterministicEnvironment();

    /**
     * @brief Validate device capabilities for deterministic execution
     */
    bool validateDeviceCapabilities() const;

    /**
     * @brief Calculate throughput for execution
     */
    double calculateThroughput(
        const std::vector<UnifiedCandidate>& results,
        std::chrono::microseconds execution_time
    ) const;
};

/**
 * @brief Factory for creating deterministic replay verifiers
 */
class ReplayVerifierFactory {
public:
    /**
     * @brief Create verifier for specified device
     */
    static std::unique_ptr<DeterministicReplayVerifier> create(int device_id);

    /**
     * @brief Create verifier with custom configuration
     */
    static std::unique_ptr<DeterministicReplayVerifier> create(
        int device_id,
        std::uint64_t deterministic_seed,
        bool performance_monitoring = true
    );
};

/**
 * @brief Utility functions for deterministic replay
 */
namespace replay_utils {

/**
 * @brief Generate deterministic execution ID
 */
std::string generateExecutionId(const std::string& kernel_name, int device_id);

/**
 * @brief Validate device compatibility for replay
 */
bool validateDeviceCompatibility(int source_device, int target_device);

/**
 * @brief Calculate memory requirements for replay
 */
size_t calculateReplayMemoryRequirements(const ReplayCapture& capture);

/**
 * @brief Estimate replay time overhead
 */
std::chrono::microseconds estimateReplayOverhead(const ReplayCapture& capture);

/**
 * @brief Format verification results for logging
 */
std::string formatVerificationResult(const ReplayVerificationResult& result);

/**
 * @brief Parse verification report from JSON
 */
std::vector<ReplayVerificationResult> parseVerificationReport(const std::string& json);

} // namespace replay_utils

} // namespace puzzle71::gpu
/**
 * @file memory_validator.h
 * @brief Memory optimization validation framework
 *
 * This header defines the memory validation framework for analyzing and
 * validating GPU memory access patterns, coalescing efficiency, and
 * bandwidth utilization during technical debt repair.
 *
 * Requirements Addressed:
 * - T018: Setup memory optimization validation framework
 * - Memory access pattern analysis for CUDA kernels
 * - Constitutional compliance for memory efficiency (≥90% target)
 *
 * @author Puzzle71 Technical Debt Repair System
 * @version 5.5
 * @date 2025-10-20
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

namespace puzzle71 {
namespace monitoring {

/**
 * @brief Memory access pattern types
 */
enum class AccessPattern {
    SEQUENTIAL = 0,         ///< Sequential memory access
    STRIDED = 1,           ///< Strided memory access
    RANDOM = 2,            ///< Random memory access
    COALESCED = 3,         ///< Coalesced memory access
    UNCOALESCED = 4        ///< Uncoalesced memory access
};

/**
 * @brief Memory hierarchy levels
 */
enum class MemoryLevel {
    GLOBAL = 0,            ///< Global memory
    SHARED = 1,            ///< Shared memory
    CONSTANT = 2,          ///< Constant memory
    TEXTURE = 3,           ///< Texture memory
    L1_CACHE = 4,          ///< L1 cache
    L2_CACHE = 5           ///< L2 cache
};

/**
 * @brief Memory operation types
 */
enum class MemoryOperation {
    LOAD = 0,              ///< Memory load operation
    STORE = 1,             ///< Memory store operation
    ATOMIC = 2,            ///< Atomic memory operation
    TRANSFER = 3           ///< Memory transfer (host-device)
};

/**
 * @brief Memory efficiency metrics
 */
struct MemoryEfficiencyMetrics {
    double coalescing_efficiency;          ///< Coalesced access efficiency (0-100%)
    double bandwidth_utilization;          ///< Memory bandwidth utilization (GB/s)
    double cache_hit_rate;                 ///< Cache hit rate (0-100%)
    double bank_conflict_rate;             ///< Shared memory bank conflict rate (0-100%)
    double memory_access_efficiency;       ///< Overall memory access efficiency (0-100%)
    std::map<MemoryLevel, double> level_efficiency; ///< Efficiency by memory level
    std::map<AccessPattern, double> pattern_efficiency; ///< Efficiency by access pattern
    std::map<MemoryOperation, double> operation_efficiency; ///< Efficiency by operation

    /**
     * @brief Calculate overall efficiency score
     * @return Overall efficiency score (0-100)
     */
    double get_overall_score() const;

    /**
     * @brief Check if efficiency meets constitutional requirements
     * @return True if all constitutional requirements are met
     */
    bool meets_constitutional_requirements() const;

    /**
     * @brief Serialize metrics to JSON
     * @return JSON representation
     */
    nlohmann::json to_json() const;

    /**
     * @brief Load metrics from JSON
     * @param j JSON object
     * @return True if loading successful
     */
    bool from_json(const nlohmann::json& j);
};

/**
 * @brief Memory access trace entry
 */
struct MemoryAccessTrace {
    std::chrono::nanoseconds timestamp;
    MemoryOperation operation;
    MemoryLevel level;
    size_t address;
    size_t size_bytes;
    int thread_id;
    int block_id;
    AccessPattern pattern;
    double access_latency_ns;
    bool is_coalesced;

    /**
     * @brief Serialize trace to JSON
     * @return JSON representation
     */
    nlohmann::json to_json() const;

    /**
     * @brief Load trace from JSON
     * @param j JSON object
     * @return True if loading successful
     */
    bool from_json(const nlohmann::json& j);
};

/**
 * @brief Memory validation configuration
 */
struct MemoryValidationConfig {
    int gpu_device_id;
    std::vector<std::string> kernel_names;
    std::chrono::seconds trace_duration;
    size_t max_trace_entries;
    bool enable_detailed_profiling;
    bool enable_bank_conflict_analysis;
    bool enable_coalescing_analysis;
    std::map<std::string, double> efficiency_thresholds;
    std::vector<AccessPattern> patterns_to_analyze;

    /**
     * @brief Validate configuration
     * @return True if configuration is valid
     */
    bool is_valid() const;

    /**
     * @brief Serialize configuration to JSON
     * @return JSON representation
     */
    nlohmann::json to_json() const;

    /**
     * @brief Load configuration from JSON
     * @param j JSON object
     * @return True if loading successful
     */
    bool from_json(const nlohmann::json& j);
};

/**
 * @brief Memory validation session
 */
struct MemoryValidationSession {
    std::string session_id;
    MemoryValidationConfig config;
    std::chrono::system_clock::time_point started_at;
    std::chrono::system_clock::time_point completed_at;
    std::string status;
    std::vector<MemoryAccessTrace> traces;
    MemoryEfficiencyMetrics metrics;
    std::vector<std::string> optimization_recommendations;
    std::map<std::string, double> detailed_profiling_data;
    std::string error_message;

    /**
     * @brief Calculate session duration
     * @return Duration in seconds
     */
    double get_duration_seconds() const;

    /**
     * @brief Get validation result
     * @return True if validation passed
     */
    bool is_validation_passed() const;

    /**
     * @brief Serialize session to JSON
     * @return JSON representation
     */
    nlohmann::json to_json() const;

    /**
     * @brief Load session from JSON
     * @param j JSON object
     * @return True if loading successful
     */
    bool from_json(const nlohmann::json& j);
};

/**
 * @brief Memory optimization validator main class
 *
 * Provides comprehensive memory validation including access pattern analysis,
 * coalescing efficiency measurement, and bandwidth utilization validation.
 */
class MemoryValidator {
public:
    /**
     * @brief Constructor
     * @param storage_dir Directory for storing validation data
     */
    explicit MemoryValidator(const std::string& storage_dir);

    /**
     * @brief Destructor
     */
    ~MemoryValidator();

    /**
     * @brief Start memory validation session
     * @param config Validation configuration
     * @return Session ID for tracking
     */
    std::string start_validation(const MemoryValidationConfig& config);

    /**
     * @brief Record memory access trace
     * @param session_id Session identifier
     * @param trace Memory access trace
     * @return True if recording successful
     */
    bool record_trace(const std::string& session_id, const MemoryAccessTrace& trace);

    /**
     * @brief Complete validation and analyze traces
     * @param session_id Session identifier
     * @return True if completion successful
     */
    bool complete_validation(const std::string& session_id);

    /**
     * @brief Get validation session
     * @param session_id Session identifier
     * @return Pointer to session or nullptr if not found
     */
    const MemoryValidationSession* get_session(const std::string& session_id) const;

    /**
     * @brief Analyze memory access patterns
     * @param traces List of memory access traces
     * @return Pattern analysis results
     */
    std::map<AccessPattern, double> analyze_access_patterns(
        const std::vector<MemoryAccessTrace>& traces);

    /**
     * @brief Calculate coalescing efficiency
     * @param traces List of memory access traces
     * @return Coalescing efficiency metrics
     */
    std::map<std::string, double> calculate_coalescing_efficiency(
        const std::vector<MemoryAccessTrace>& traces);

    /**
     * @brief Detect shared memory bank conflicts
     * @param traces List of shared memory access traces
     * @return Bank conflict analysis results
     */
    std::map<std::string, double> detect_bank_conflicts(
        const std::vector<MemoryAccessTrace>& traces);

    /**
     * @brief Measure memory bandwidth utilization
     * @param gpu_device_id GPU device ID
     * @param duration_ms Measurement duration in milliseconds
     * @return Bandwidth utilization metrics
     */
    std::map<std::string, double> measure_bandwidth_utilization(
        int gpu_device_id, int duration_ms);

    /**
     * @brief Validate against constitutional requirements
     * @param session_id Session ID to validate
     * @return Validation result with specific requirements
     */
    std::map<std::string, bool> validate_constitutional_requirements(
        const std::string& session_id) const;

    /**
     * @brief Generate optimization recommendations
     * @param session_id Session ID
     * @return List of optimization recommendations
     */
    std::vector<std::string> generate_optimization_recommendations(
        const std::string& session_id) const;

    /**
     * @brief Export validation results
     * @param output_file Output file path
     * @param session_ids List of session IDs to export (empty = all)
     * @param format Export format (json, csv, markdown)
     * @return True if export successful
     */
    bool export_results(const std::string& output_file,
                       const std::vector<std::string>& session_ids = {},
                       const std::string& format = "json") const;

    /**
     * @brief Compare validation sessions
     * @param current_session_id Current session ID
     * @param baseline_session_id Baseline session ID
     * @return Comparison results
     */
    nlohmann::json compare_sessions(const std::string& current_session_id,
                                   const std::string& baseline_session_id) const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

    /**
     * @brief Get file path for validation data
     * @param filename Base filename
     * @return Full file path
     */
    std::string get_data_path(const std::string& filename) const;

    /**
     * @brief Ensure storage directory exists
     */
    void ensure_storage_directory() const;

    /**
     * @brief Generate unique session ID
     * @return Unique session identifier
     */
    std::string generate_session_id() const;

    /**
     * @brief Convert access pattern to string
     * @param pattern Access pattern
     * @return String representation
     */
    std::string access_pattern_to_string(AccessPattern pattern) const;

    /**
     * @brief Convert string to access pattern
     * @param str Pattern string
     * @return Access pattern enum value
     */
    AccessPattern string_to_access_pattern(const std::string& str) const;

    /**
     * @brief Analyze trace for coalescing
     * @param traces List of traces to analyze
     * @return Coalescing analysis results
     */
    std::map<std::string, double> analyze_trace_coalescing(
        const std::vector<MemoryAccessTrace>& traces);

    /**
     * @brief Calculate memory access efficiency
     * @param traces List of memory access traces
     * @return Access efficiency metrics
     */
    std::map<std::string, double> calculate_access_efficiency(
        const std::vector<MemoryAccessTrace>& traces);

    /**
     * @brief Detect memory access patterns
     * @param traces List of traces to analyze
     * @return Detected patterns and their frequencies
     */
    std::map<AccessPattern, int> detect_patterns(
        const std::vector<MemoryAccessTrace>& traces);
};

/**
 * @brief RAII helper for validation sessions
 *
 * Automatically manages validation session lifecycle within a scope.
 */
class MemoryValidationGuard {
public:
    /**
     * @brief Constructor - starts validation session
     * @param validator Validator instance
     * @param config Validation configuration
     */
    MemoryValidationGuard(MemoryValidator& validator,
                         const MemoryValidationConfig& config);

    /**
     * @brief Destructor - completes session and analyzes traces
     */
    ~MemoryValidationGuard();

    /**
     * @brief Get session ID
     * @return Session ID
     */
    const std::string& session_id() const { return session_id_; }

    /**
     * @brief Record a memory trace
     * @param trace Memory access trace
     * @return True if recording successful
     */
    bool record_trace(const MemoryAccessTrace& trace);

    /**
     * @brief Get validation results
     * @return Validation results
     */
    const MemoryValidationSession* get_results() const;

private:
    MemoryValidator& validator_;
    std::string session_id_;
    bool session_active_;
};

/**
 * @brief Memory validation utilities
 */
namespace memory_validation_utils {
    /**
     * @brief Get constitutional memory requirements
     * @return Map of requirement names to threshold values
     */
    std::map<std::string, double> get_constitutional_requirements();

    /**
     * @brief Check if memory access is coalesced
     * @param traces List of memory access traces from the same warp
     * @return True if access is coalesced
     */
    bool is_coalesced_access(const std::vector<MemoryAccessTrace>& traces);

    /**
     * @brief Calculate memory stride
     * @param traces List of consecutive memory accesses
     * @return Memory stride in bytes
     */
    size_t calculate_memory_stride(const std::vector<MemoryAccessTrace>& traces);

    /**
     * @brief Detect shared memory bank conflicts
     * @param traces List of shared memory accesses
     * @return Number of bank conflicts detected
     */
    int detect_shared_bank_conflicts(const std::vector<MemoryAccessTrace>& traces);

    /**
     * @brief Generate memory layout recommendation
     * @param traces List of memory access traces
     * @return Layout optimization recommendation
     */
    std::string generate_layout_recommendation(const std::vector<MemoryAccessTrace>& traces);

    /**
     * @brief Validate Structure-of-Arrays vs Array-of-Structures
     * @param traces Memory access traces
     * @param struct_definition Structure layout definition
     * @return Recommendation for SoA vs AoS
     */
    std::string validate_soa_vs_aos(const std::vector<MemoryAccessTrace>& traces,
                                   const std::map<std::string, size_t>& struct_definition);
}

} // namespace monitoring
} // namespace puzzle71
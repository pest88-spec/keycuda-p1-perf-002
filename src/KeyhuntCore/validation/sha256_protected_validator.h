// Puzzle71 Technical Debt Repair - SHA-256 Protected Baseline and Result Validation System
// Task: T059 [P] [US3] Implement SHA-256 protected baseline and result validation
// Phase: Phase 4 - User Story 3 Integration Testing and Validation System

#pragma once

#include <cuda_runtime.h>
#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <chrono>
#include <map>
#include <functional>
#include <fstream>

namespace puzzle71 {
namespace validation {

// SHA-256 hash structure
struct SHA256Hash {
    std::vector<uint8_t> bytes;              // 32-byte SHA-256 hash
    std::string hex_string;                 // Hexadecimal representation
    std::chrono::system_clock::time_point timestamp; // Hash generation time

    SHA256Hash() {
        bytes.resize(32);
    }

    bool operator==(const SHA256Hash& other) const {
        return bytes == other.bytes && hex_string == other.hex_string;
    }

    bool operator!=(const SHA256Hash& other) const {
        return !(*this == other);
    }
};

// Protected baseline data structure
struct ProtectedBaseline {
    std::string baseline_id;                 // Unique baseline identifier
    std::string baseline_type;               // Type of baseline (performance, ecc, etc.)
    std::string description;                  // Baseline description
    SHA256Hash content_hash;                 // SHA-256 hash of baseline content
    std::vector<uint8_t> encrypted_content;   // Encrypted baseline data
    std::chrono::system_clock::time_point creation_time;    // Baseline creation time
    std::chrono::system_clock::time_point last_verified;       // Last verification time
    std::string creator_signature;           // Creator signature (for audit trail)
    std::map<std::string, std::string> metadata;               // Additional metadata

    bool isValid() const {
        return !baseline_id.empty() && !baseline_type.empty() &&
               content_hash.bytes.size() == 32 && !encrypted_content.empty();
    }
};

// Validation result structure
struct ValidationResult {
    std::string validation_id;               // Unique validation identifier
    bool is_valid;                          // Whether validation passed
    bool hash_matches;                       // Whether hash matches computed hash
    bool signature_valid;                   // Whether signature is valid
    std::string validation_type;              // Type of validation (baseline, result, etc.)
    std::string validation_details;          // Detailed validation information
    std::chrono::system_clock::time_point validation_time;      // Validation timestamp
    SHA256Hash computed_hash;               // Computed SHA-256 hash
    std::vector<std::string> validation_steps;   // Steps performed during validation
    std::vector<std::string> warnings;         // Validation warnings
    std::vector<std::string> errors;           // Validation errors

    ValidationResult() : is_valid(false), hash_matches(false), signature_valid(false) {}
};

// SHA-256 validation configuration
struct SHA256ValidationConfig {
    bool enable_signature_verification = true;   // Enable signature verification
    bool enable_timestamp_validation = true;     // Enable timestamp validation
    bool enable_metadata_validation = true;      // Enable metadata validation
    bool enable_content_encryption = true;       // Enable content encryption
    size_t max_baseline_age_days = 365;           // Maximum baseline age (1 year)
    std::string trusted_signatures_file = "trusted_signatures.json"; // Trusted signatures
    std::string encryption_key_file = "encryption_key.bin";      // Encryption key file
    bool enable_audit_logging = true;          // Enable comprehensive audit logging
    std::string audit_log_file = "audit.log"; // Audit log file path

    SHA256ValidationConfig() = default;
};

// SHA-256 protected validator
class SHA256ProtectedValidator {
public:
    SHA256ProtectedValidator();
    ~SHA256ProtectedValidator();

    // Initialize the validator
    bool initialize(const SHA256ValidationConfig& config = SHA256ValidationConfig{});

    // Create protected baseline
    bool createProtectedBaseline(const std::string& baseline_id,
                                  const std::string& baseline_type,
                                  const std::string& description,
                                  const std::vector<uint8_t>& content,
                                  const std::string& creator_signature = "");

    // Validate protected baseline integrity
    bool validateBaseline(const std::string& baseline_id, ValidationResult& result);

    // Update existing protected baseline
    bool updateProtectedBaseline(const std::string& baseline_id,
                                  const std::vector<uint8_t>& new_content,
                                  const std::string& update_signature);

    // Delete protected baseline
    bool deleteProtectedBaseline(const std::string& baseline_id);

    // Validate result against baseline
    bool validateResultAgainstBaseline(const std::string& baseline_id,
                                         const std::vector<uint8_t>& result_data,
                                         ValidationResult& validation_result);

    // Batch validation of multiple results
    bool validateResultsBatch(const std::string& baseline_id,
                               const std::vector<std::vector<uint8_t>>& results_data,
                               std::vector<ValidationResult>& validation_results);

    // Load protected baselines from storage
    bool loadBaselines(const std::string& storage_directory = "baselines/");

    // Save protected baselines to storage
    bool saveBaselines(const std::string& storage_directory = "baselines/");

    // Generate baseline manifest with SHA-256 integrity
    bool generateBaselineManifest(std::string& manifest_content);

    // Validate baseline manifest integrity
    bool validateBaselineManifest(const std::string& manifest_content, ValidationResult& result);

    // Get list of available baselines
    std::vector<std::string> getAvailableBaselines() const;

    // Get baseline metadata
    bool getBaselineMetadata(const std::string& baseline_id, ProtectedBaseline& baseline);

    // Check if baseline exists
    bool baselineExists(const std::string& baseline_id) const;

    // Get validation statistics
    bool getValidationStatistics(size_t& total_validations,
                                   size_t& successful_validations,
                                   size_t& failed_validations,
                                   std::chrono::system_clock::time_point& last_validation_time);

    // Audit trail operations
    bool getAuditTrail(const std::string& baseline_id, std::vector<std::string>& audit_entries);
    bool addAuditEntry(const std::string& baseline_id, const std::string& operation, const std::string& details);

    // Check if validator is ready
    bool isReady() const { return initialized_ && storage_directory_initialized_; }

    // Get last error
    std::string getLastError() const { return last_error_; }

    // Configuration management
    void updateConfiguration(const SHA256ValidationConfig& config);
    const SHA256ValidationConfig& getConfiguration() const { return config_; }

private:
    // Internal cryptographic operations
    bool computeSHA256Hash(const std::vector<uint8_t>& data, SHA256Hash& hash);
    bool encryptContent(const std::vector<uint8_t>& content, std::vector<uint8_t>& encrypted_content);
    bool decryptContent(const std::vector<uint8_t>& encrypted_content, std::vector<uint8_t>& content);
    bool verifySignature(const std::string& data, const std::string& signature, bool& is_valid);

    // File I/O operations
    std::string getBaselineFilePath(const std::string& baseline_id) const;
    std::string getManifestFilePath() const;
    bool readBinaryFile(const std::string& file_path, std::vector<uint8_t>& data);
    bool writeBinaryFile(const std::string& file_path, const std::vector<uint8_t>& data);
    bool readTextFile(const std::string& file_path, std::string& content);
    bool writeTextFile(const std::string& file_path, const std::string& content);
    bool deleteFile(const std::string& file_path);

    // Serialization operations
    bool serializeBaseline(const ProtectedBaseline& baseline, std::vector<uint8_t>& data);
    bool deserializeBaseline(const std::vector<uint8_t>& data, ProtectedBaseline& baseline);
    bool serializeValidationResult(const ValidationResult& result, std::vector<uint8_t>& data);
    bool deserializeValidationResult(const std::vector<uint8_t>& data, ValidationResult& result);

    // Validation helper methods
    bool validateBaselineIntegrity(const ProtectedBaseline& baseline, ValidationResult& result);
    bool validateTimestamp(const ProtectedBaseline& baseline, ValidationResult& result);
    bool validateMetadata(const ProtectedBaseline& baseline, ValidationResult& result);
    bool validateAge(const ProtectedBaseline& baseline, ValidationResult& result);
    bool validateContent(const std::vector<uint8_t>& content, const SHA256Hash& expected_hash, ValidationResult& result);

    // Audit logging
    bool logAuditEntry(const std::string& baseline_id, const std::string& operation, const std::string& details);
    bool loadAuditTrail(const std::string& baseline_id, std::vector<std::string>& audit_entries);
    bool saveAuditTrail(const std::string& baseline_id, const std::vector<std::string>& audit_entries);

    // Directory management
    bool initializeStorageDirectory(const std::string& directory);
    bool ensureDirectoryExists(const std::string& directory);
    bool createAuditDirectory(const std::string& baseline_id);

    // Utility methods
    std::string hashToHexString(const SHA256Hash& hash);
    SHA256Hash hexStringToHash(const std::string& hex_string);
    std::string getCurrentTimestamp();
    std::string generateUniqueId();

    void setError(const std::string& error);
    void clearError();

private:
    bool initialized_;
    bool storage_directory_initialized_;
    std::string last_error_;
    SHA256ValidationConfig config_;
    std::string storage_directory_;

    // Baseline storage
    std::map<std::string, ProtectedBaseline> baselines_;
    std::map<std::string, std::vector<std::string>> audit_trails_;

    // Validation statistics
    size_t total_validations_;
    size_t successful_validations_;
    size_t failed_validations_;
    std::chrono::system_clock::time_point last_validation_time_;

    // Cryptographic keys
    std::vector<uint8_t> encryption_key_;
    std::vector<std::string> trusted_signatures_;
};

// Utility functions for SHA-256 protection
namespace sha256_protection_utils {

    // Cryptographic utilities
    bool computeSHA256(const std::vector<uint8_t>& data, std::vector<uint8_t>& hash);
    std::string sha256ToHexString(const std::vector<uint8_t>& hash);
    std::vector<uint8_t> hexStringToSHA256(const std::string& hex_string);

    // Encryption utilities (simplified for demonstration)
    bool encryptData(const std::vector<uint8_t>& data, const std::vector<uint8_t>& key, std::vector<uint8_t>& encrypted);
    bool decryptData(const std::vector<uint8_t>& encrypted, const std::vector<uint8_t>& key, std::vector<uint8_t>& decrypted);

    // Digital signature utilities (simplified for demonstration)
    bool signData(const std::string& data, const std::string& private_key, std::string& signature);
    bool verifySignature(const std::string& data, const std::string& signature, const std::string& public_key, bool& is_valid);

    // File integrity utilities
    bool verifyFileIntegrity(const std::string& file_path, const SHA256Hash& expected_hash);
    bool createFileIntegrityRecord(const std::string& file_path, SHA256Hash& computed_hash);

    // JSON utilities for manifest generation
    std::string generateManifestJSON(const std::map<std::string, ProtectedBaseline>& baselines);
    bool parseManifestJSON(const std::string& json_content, std::map<std::string, ProtectedBaseline>& baselines);

    // Validation report utilities
    std::string generateValidationReport(const std::vector<ValidationResult>& results);
    std::string generateAuditReport(const std::map<std::string, std::vector<std::string>>& audit_trails);

    // Time utilities
    std::string formatTimestamp(std::chrono::system_clock::time_point time_point, const std::string& format = "%Y-%m-%d %H:%M:%S");
    bool isTimestampWithinRange(const std::chrono::system_clock::time_point& timestamp,
                                   const std::chrono::system_clock::time_point& start,
                                   const std::chrono::system_clock::time_point& end);

    // String utilities
    std::string generateRandomString(size_t length);
    std::string escapeJsonString(const std::string& input);
    std::string unescapeJsonString(const std::string& input);

    // Configuration utilities
    SHA256ValidationConfig loadConfigurationFromFile(const std::string& config_file);
    bool saveConfigurationToFile(const SHA256ValidationConfig& config, const std::string& config_file);
    bool validateConfiguration(const SHA256ValidationConfig& config);

    // Performance monitoring utilities
    class PerformanceTimer {
    public:
        PerformanceTimer();
        void start();
        void stop();
        double getElapsedMilliseconds() const;
        double getElapsedSeconds() const;
    private:
        std::chrono::high_resolution_clock::time_point start_time_;
        std::chrono::high_resolution_clock::time_point end_time_;
        bool running_;
    };

    // Memory utilities
    bool secureZeroMemory(void* ptr, size_t size);
    bool secureEraseFile(const std::string& file_path);
}

// Constants for SHA-256 protection (Constitutional v5.5 compliance)
namespace sha256_protection_constants {
    constexpr size_t SHA256_HASH_SIZE = 32;               // 32 bytes
    constexpr size_t ENCRYPTION_KEY_SIZE = 32;             // 32 bytes
    constexpr size_t MAX_BASELINE_SIZE_MB = 100;           // 100MB max baseline size
    constexpr size_t MAX_METADATA_SIZE = 1024;             // 1KB max metadata size
    constexpr size_t MAX_SIGNATURE_SIZE = 512;             // 512 bytes max signature size
    constexpr size_t MAX_AUDIT_ENTRIES = 1000;               // 1000 audit entries max
    constexpr int MAX_BASELINE_AGE_DAYS = 365;               // 1 year max age
    constexpr double HASH_COMPARISON_TOLERANCE = 0.0;        // Exact hash match required
}

} // namespace validation
} // namespace puzzle71
// Puzzle71 Technical Debt Repair - SHA-256 Protected Baseline and Result Validation
// Task: T058 [P] [US3] Create comprehensive validation report generation
// Phase: Phase 4B - User Story 3 Integration Testing and Validation System
//
// This system provides SHA-256 cryptographic protection for baseline files,
// validation results, and ensures integrity of all validation artifacts.

#pragma once

#include <cuda_runtime.h>
#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <chrono>
#include <map>
#include <sstream>
#include <iomanip>

namespace puzzle71 {
namespace validation {

// SHA-256 hash wrapper for cryptographic integrity protection
class SHA256Hash {
public:
    static constexpr size_t HASH_SIZE = 32; // SHA-256 produces 32-byte hash

    // Compute SHA-256 hash of data
    static std::vector<unsigned char> computeHash(const std::vector<unsigned char>& data);
    static std::vector<unsigned char> computeHash(const std::string& data);
    static std::vector<unsigned char> computeHash(const void* data, size_t size);

    // Convert hash to hexadecimal string
    static std::string hashToString(const std::vector<unsigned char>& hash);
    static std::vector<unsigned char> stringToHash(const std::string& hex_string);

    // Verify hash integrity
    static bool verifyHash(const std::vector<unsigned char>& data, const std::vector<unsigned char>& expected_hash);
    static bool verifyHash(const std::string& data, const std::vector<unsigned char>& expected_hash);

    // Generate random salt for hash salting
    static std::vector<unsigned char> generateSalt(size_t size = 16);

    // Compute salted hash
    static std::vector<unsigned char> computeSaltedHash(const std::vector<unsigned char>& data,
                                                       const std::vector<unsigned char>& salt);

private:
    // Internal SHA-256 computation
    static std::vector<unsigned char> computeSHA256(const void* data, size_t size);
};

// Baseline entry with cryptographic protection
struct BaselineEntry {
    std::string test_name;                           // Test identifier
    std::vector<unsigned char> data_hash;            // SHA-256 hash of test data
    std::vector<unsigned char> result_hash;          // SHA-256 hash of test results
    std::vector<unsigned char> metadata_hash;        // SHA-256 hash of metadata
    std::map<std::string, double> performance_metrics; // Performance measurements
    std::chrono::system_clock::time_point timestamp;  // Creation timestamp
    std::vector<unsigned char> signature_hash;       // Combined signature hash
    std::string version;                             // Baseline format version
    std::vector<unsigned char> salt;                 // Cryptographic salt

    BaselineEntry() : timestamp(std::chrono::system_clock::now()), version("1.0") {}
};

// Validation result with cryptographic protection
struct ValidationResult {
    std::string test_name;                           // Test identifier
    bool passed;                                     // Test pass/fail status
    double execution_time_ms;                        // Execution time in milliseconds
    std::vector<unsigned char> result_hash;          // SHA-256 hash of complete result
    std::map<std::string, double> metrics;           // Test metrics
    std::string error_message;                       // Error details (if any)
    std::chrono::system_clock::time_point timestamp;  // Test execution timestamp
    std::string baseline_version;                    // Baseline version used for comparison
    std::vector<unsigned char> integrity_hash;       // Overall integrity hash
    std::vector<unsigned char> salt;                 // Cryptographic salt

    ValidationResult() : passed(false), execution_time_ms(0.0),
                       timestamp(std::chrono::system_clock::now()) {}
};

// SHA-256 Protected Baseline Validator
class SHA256BaselineValidator {
public:
    SHA256BaselineValidator();
    ~SHA256BaselineValidator();

    // Initialize the validator
    bool initialize(const std::string& baseline_directory = "baselines/",
                   const std::string& validation_directory = "validation/");

    // Baseline management
    bool createBaseline(const std::string& test_name,
                       const std::vector<unsigned char>& test_data,
                       const std::map<std::string, double>& performance_metrics,
                       BaselineEntry& baseline);

    bool saveBaseline(const BaselineEntry& baseline);
    bool loadBaseline(const std::string& test_name, BaselineEntry& baseline);
    bool deleteBaseline(const std::string& test_name);

    // Validation with integrity protection
    bool validateAgainstBaseline(const std::string& test_name,
                                const std::vector<unsigned char>& current_data,
                                const std::map<std::string, double>& current_metrics,
                                ValidationResult& result);

    bool validateResult(const ValidationResult& result);
    bool validateBaseline(const BaselineEntry& baseline);

    // Batch validation operations
    bool validateBatchAgainstBaselines(
        const std::map<std::string, std::vector<unsigned char>>& test_data_map,
        const std::map<std::string, std::map<std::string, double>>& metrics_map,
        std::vector<ValidationResult>& results);

    // Baseline integrity verification
    bool verifyBaselineIntegrity(const std::string& test_name);
    bool verifyAllBaselinesIntegrity(std::vector<std::string>& corrupted_baselines);

    // Baseline comparison and regression detection
    bool compareWithBaseline(const std::string& test_name,
                            const std::map<std::string, double>& current_metrics,
                            std::map<std::string, double>& regressions,
                            double tolerance_percentage = 5.0);

    // Performance regression detection
    bool detectPerformanceRegression(const std::string& test_name,
                                    double current_value,
                                    double baseline_value,
                                    double tolerance_percentage,
                                    bool& is_regression,
                                    double& regression_percentage);

    // Baseline migration and versioning
    bool migrateBaseline(const std::string& test_name, const std::string& new_version);
    bool getBaselineVersion(const std::string& test_name, std::string& version);

    // Report generation
    bool generateIntegrityReport(std::string& report);
    bool generateBaselineReport(const std::string& test_name, std::string& report);
    bool generateValidationReport(const ValidationResult& result, std::string& report);

    // Configuration
    void setPerformanceTolerance(double tolerance) { performance_tolerance_ = tolerance; }
    double getPerformanceTolerance() const { return performance_tolerance_; }

    void setEnableDetailedLogging(bool enable) { detailed_logging_ = enable; }
    bool isDetailedLoggingEnabled() const { return detailed_logging_; }

    // Statistics and monitoring
    size_t getBaselineCount() const;
    std::vector<std::string> getBaselineNames() const;
    bool hasBaseline(const std::string& test_name) const;

    // Error handling
    std::string getLastError() const { return last_error_; }
    bool hasErrors() const { return !last_error_.empty(); }

private:
    // Internal helper methods
    bool computeBaselineSignature(BaselineEntry& baseline);
    bool computeResultSignature(ValidationResult& result);
    bool verifyBaselineSignature(const BaselineEntry& baseline);
    bool verifyResultSignature(const ValidationResult& result);

    // File I/O with integrity protection
    bool saveBaselineToFile(const BaselineEntry& baseline, const std::string& filepath);
    bool loadBaselineFromFile(const std::string& filepath, BaselineEntry& baseline);

    // Data serialization helpers
    std::string serializeBaselineEntry(const BaselineEntry& baseline);
    bool deserializeBaselineEntry(const std::string& serialized_data, BaselineEntry& baseline);
    std::string serializeValidationResult(const ValidationResult& result);
    bool deserializeValidationResult(const std::string& serialized_data, ValidationResult& result);

    // JSON-like serialization helpers
    std::string serializeMap(const std::map<std::string, double>& metrics);
    bool deserializeMap(const std::string& serialized, std::map<std::string, double>& metrics);

    // Cryptographic helpers
    std::vector<unsigned char> generateTimestampHash();
    std::vector<unsigned char> computeDataIntegrityHash(const std::string& test_name,
                                                        const std::vector<unsigned char>& data_hash,
                                                        const std::vector<unsigned char>& result_hash);

    // Path management
    std::string getBaselineFilePath(const std::string& test_name);
    std::string getValidationFilePath(const std::string& test_name);
    bool ensureDirectoryExists(const std::string& dir_path);

    // Error handling
    void setError(const std::string& error);
    void clearError();

private:
    std::string baseline_directory_;
    std::string validation_directory_;
    double performance_tolerance_;
    bool detailed_logging_;
    std::string last_error_;
    bool initialized_;

    // Cache for loaded baselines
    std::map<std::string, BaselineEntry> baseline_cache_;

    // Version information
    static constexpr char BASELINE_FORMAT_VERSION[] = "1.0";
    static constexpr size_t MAX_BASELINE_SIZE = 1024 * 1024; // 1MB max baseline size
};

// Baseline integrity monitor for continuous validation
class BaselineIntegrityMonitor {
public:
    BaselineIntegrityMonitor();
    ~BaselineIntegrityMonitor();

    // Initialize monitor
    bool initialize(std::shared_ptr<SHA256BaselineValidator> validator,
                   int monitoring_interval_seconds = 300); // 5 minutes default

    // Start/stop monitoring
    bool startMonitoring();
    bool stopMonitoring();

    // Manual integrity check
    bool performIntegrityCheck(std::vector<std::string>& corrupted_baselines);

    // Monitoring status
    bool isMonitoringActive() const { return monitoring_active_; }
    std::chrono::system_clock::time_point getLastCheckTime() const { return last_check_time_; }
    size_t getTotalChecksPerformed() const { return total_checks_performed_; }

    // Configuration
    void setMonitoringInterval(int seconds) { monitoring_interval_seconds_ = seconds; }
    int getMonitoringInterval() const { return monitoring_interval_seconds_; }

    // Error handling
    std::string getLastError() const { return last_error_; }

private:
    void monitoringLoop();
    void performScheduledCheck();

    std::shared_ptr<SHA256BaselineValidator> validator_;
    int monitoring_interval_seconds_;
    bool monitoring_active_;
    std::thread monitoring_thread_;
    std::chrono::system_clock::time_point last_check_time_;
    size_t total_checks_performed_;
    std::string last_error_;
    bool should_stop_monitoring_;
};

// Utility functions for baseline management
namespace baseline_utils {

    // Baseline comparison utilities
    bool comparePerformanceMetrics(const std::map<std::string, double>& current,
                                  const std::map<std::string, double>& baseline,
                                  std::map<std::string, double>& regressions,
                                  double tolerance_percentage);

    // Baseline statistics
    struct BaselineStatistics {
        size_t total_baselines;
        size_t valid_baselines;
        size_t corrupted_baselines;
        std::chrono::system_clock::time_point oldest_baseline;
        std::chrono::system_clock::time_point newest_baseline;
        double average_baseline_size_kb;
    };

    bool calculateBaselineStatistics(std::shared_ptr<SHA256BaselineValidator> validator,
                                     BaselineStatistics& stats);

    // Baseline backup and recovery
    bool backupBaselines(const std::string& source_dir, const std::string& backup_dir);
    bool restoreBaselines(const std::string& backup_dir, const std::string& target_dir);

    // Baseline migration tools
    bool migrateBaselineFormat(const std::string& old_format_file,
                              const std::string& new_format_file);
    bool validateBaselineMigration(const std::string& old_file, const std::string& new_file);

    // Baseline cleanup utilities
    bool cleanupOldBaselines(const std::string& baseline_dir,
                             std::chrono::system_clock::duration max_age);
    bool cleanupCorruptedBaselines(std::shared_ptr<SHA256BaselineValidator> validator);

    // Format conversion utilities
    std::string formatTimestamp(const std::chrono::system_clock::time_point& timestamp);
    std::string formatMetrics(const std::map<std::string, double>& metrics);
    std::string formatHash(const std::vector<unsigned char>& hash);

    // Validation report generation
    bool generateComprehensiveIntegrityReport(std::shared_ptr<SHA256BaselineValidator> validator,
                                             std::string& report);

    // Baseline verification tools
    bool verifyBaselineChain(const std::vector<std::string>& test_names,
                            std::shared_ptr<SHA256BaselineValidator> validator);

    // Performance analysis
    struct PerformanceAnalysis {
        std::string metric_name;
        double current_value;
        double baseline_value;
        double change_percentage;
        bool is_regression;
        bool is_improvement;
        double significance_threshold;
    };

    bool analyzePerformanceChanges(const std::string& test_name,
                                   const std::map<std::string, double>& current_metrics,
                                   std::shared_ptr<SHA256BaselineValidator> validator,
                                   std::vector<PerformanceAnalysis>& analysis);
}

// Constants for baseline validation
namespace baseline_constants {
    constexpr double DEFAULT_PERFORMANCE_TOLERANCE = 5.0;     // 5%
    constexpr int DEFAULT_MONITORING_INTERVAL = 300;          // 5 minutes
    constexpr size_t MAX_BASELINE_SIZE_BYTES = 1024 * 1024;   // 1MB
    constexpr int MAX_BASELINE_AGE_DAYS = 365;                // 1 year
    constexpr size_t BASELINE_CACHE_SIZE = 1000;              // Max cached baselines
    constexpr double SIGNIFICANCE_THRESHOLD = 1.0;            // 1% for significant changes
    constexpr int INTEGRITY_CHECK_RETRIES = 3;                // Max retries for integrity checks
}

} // namespace validation
} // namespace puzzle71
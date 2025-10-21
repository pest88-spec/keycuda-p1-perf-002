// Puzzle71 Technical Debt Repair - SHA-256 Protected Baseline and Result Validation Implementation
// Task: T058 [P] [US3] Create comprehensive validation report generation
// Phase: Phase 4B - User Story 3 Integration Testing and Validation System

#include "sha256_baseline_validator.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <thread>
#include <atomic>
#include <openssl/sha.h>
#include <openssl/rand.h>

namespace puzzle71 {
namespace validation {

// SHA256Hash implementation
std::vector<unsigned char> SHA256Hash::computeHash(const std::vector<unsigned char>& data) {
    return computeSHA256(data.data(), data.size());
}

std::vector<unsigned char> SHA256Hash::computeHash(const std::string& data) {
    return computeSHA256(data.data(), data.size());
}

std::vector<unsigned char> SHA256Hash::computeHash(const void* data, size_t size) {
    return computeSHA256(data, size);
}

std::vector<unsigned char> SHA256Hash::computeSHA256(const void* data, size_t size) {
    std::vector<unsigned char> hash(HASH_SIZE);
    SHA256_CTX ctx;

    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, size);
    SHA256_Final(hash.data(), &ctx);

    return hash;
}

std::string SHA256Hash::hashToString(const std::vector<unsigned char>& hash) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char byte : hash) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

std::vector<unsigned char> SHA256Hash::stringToHash(const std::string& hex_string) {
    std::vector<unsigned char> hash;
    hash.reserve(HASH_SIZE);

    for (size_t i = 0; i < hex_string.length() && hash.size() < HASH_SIZE; i += 2) {
        std::string byte_string = hex_string.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(std::stoi(byte_string, nullptr, 16));
        hash.push_back(byte);
    }

    return hash;
}

bool SHA256Hash::verifyHash(const std::vector<unsigned char>& data, const std::vector<unsigned char>& expected_hash) {
    if (expected_hash.size() != HASH_SIZE) return false;

    std::vector<unsigned char> computed_hash = computeHash(data);
    return std::equal(computed_hash.begin(), computed_hash.end(), expected_hash.begin());
}

bool SHA256Hash::verifyHash(const std::string& data, const std::vector<unsigned char>& expected_hash) {
    if (expected_hash.size() != HASH_SIZE) return false;

    std::vector<unsigned char> computed_hash = computeHash(data);
    return std::equal(computed_hash.begin(), computed_hash.end(), expected_hash.begin());
}

std::vector<unsigned char> SHA256Hash::generateSalt(size_t size) {
    std::vector<unsigned char> salt(size);
    if (RAND_bytes(salt.data(), size) != 1) {
        // Fallback to pseudo-random generation
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(0, 255);
        for (size_t i = 0; i < size; ++i) {
            salt[i] = static_cast<unsigned char>(dist(gen));
        }
    }
    return salt;
}

std::vector<unsigned char> SHA256Hash::computeSaltedHash(const std::vector<unsigned char>& data,
                                                       const std::vector<unsigned char>& salt) {
    std::vector<unsigned char> combined;
    combined.reserve(data.size() + salt.size());
    combined.insert(combined.end(), data.begin(), data.end());
    combined.insert(combined.end(), salt.begin(), salt.end());

    return computeHash(combined);
}

// SHA256BaselineValidator implementation
SHA256BaselineValidator::SHA256BaselineValidator()
    : performance_tolerance_(baseline_constants::DEFAULT_PERFORMANCE_TOLERANCE),
      detailed_logging_(false), initialized_(false) {
}

SHA256BaselineValidator::~SHA256BaselineValidator() {
    // Cleanup if needed
}

bool SHA256BaselineValidator::initialize(const std::string& baseline_directory,
                                          const std::string& validation_directory) {
    clearError();

    baseline_directory_ = baseline_directory;
    validation_directory_ = validation_directory;

    // Ensure directories exist
    if (!ensureDirectoryExists(baseline_directory_)) {
        setError("Failed to create baseline directory: " + baseline_directory_);
        return false;
    }

    if (!ensureDirectoryExists(validation_directory_)) {
        setError("Failed to create validation directory: " + validation_directory_);
        return false;
    }

    initialized_ = true;

    if (detailed_logging_) {
        std::cout << "SHA256 Baseline Validator initialized" << std::endl;
        std::cout << "  Baseline directory: " << baseline_directory_ << std::endl;
        std::cout << "  Validation directory: " << validation_directory_ << std::endl;
    }

    return true;
}

bool SHA256BaselineValidator::createBaseline(const std::string& test_name,
                                            const std::vector<unsigned char>& test_data,
                                            const std::map<std::string, double>& performance_metrics,
                                            BaselineEntry& baseline) {
    clearError();

    if (!initialized_) {
        setError("Validator not initialized");
        return false;
    }

    baseline = BaselineEntry();
    baseline.test_name = test_name;
    baseline.performance_metrics = performance_metrics;
    baseline.salt = SHA256Hash::generateSalt();
    baseline.version = BASELINE_FORMAT_VERSION;

    // Compute cryptographic hashes
    baseline.data_hash = SHA256Hash::computeSaltedHash(test_data, baseline.salt);

    // Compute metadata hash
    std::ostringstream metadata_stream;
    metadata_stream << test_name << "|" << baseline.version << "|"
                     << baseline.performance_metrics.size() << "|"
                     << std::chrono::duration_cast<std::chrono::seconds>(
                         baseline.timestamp.time_since_epoch()).count();

    baseline.metadata_hash = SHA256Hash::computeSaltedHash(metadata_stream.str(), baseline.salt);

    // Compute result hash (placeholder for now, will be updated with actual results)
    baseline.result_hash = SHA256Hash::computeSaltedHash("baseline_placeholder", baseline.salt);

    // Compute signature hash
    if (!computeBaselineSignature(baseline)) {
        setError("Failed to compute baseline signature");
        return false;
    }

    if (detailed_logging_) {
        std::cout << "Created baseline for test: " << test_name << std::endl;
        std::cout << "  Data hash: " << SHA256Hash::hashToString(baseline.data_hash).substr(0, 16) << "..." << std::endl;
        std::cout << "  Metrics count: " << performance_metrics.size() << std::endl;
    }

    return true;
}

bool SHA256BaselineValidator::saveBaseline(const BaselineEntry& baseline) {
    clearError();

    if (!initialized_) {
        setError("Validator not initialized");
        return false;
    }

    // Verify baseline integrity before saving
    if (!verifyBaselineSignature(baseline)) {
        setError("Baseline signature verification failed");
        return false;
    }

    std::string filepath = getBaselineFilePath(baseline.test_name);

    if (!saveBaselineToFile(baseline, filepath)) {
        setError("Failed to save baseline to file: " + filepath);
        return false;
    }

    // Update cache
    baseline_cache_[baseline.test_name] = baseline;

    if (detailed_logging_) {
        std::cout << "Saved baseline: " << baseline.test_name << " to " << filepath << std::endl;
    }

    return true;
}

bool SHA256BaselineValidator::loadBaseline(const std::string& test_name, BaselineEntry& baseline) {
    clearError();

    if (!initialized_) {
        setError("Validator not initialized");
        return false;
    }

    // Check cache first
    auto it = baseline_cache_.find(test_name);
    if (it != baseline_cache_.end()) {
        baseline = it->second;
        return verifyBaselineSignature(baseline);
    }

    std::string filepath = getBaselineFilePath(test_name);

    if (!std::filesystem::exists(filepath)) {
        setError("Baseline file does not exist: " + filepath);
        return false;
    }

    if (!loadBaselineFromFile(filepath, baseline)) {
        setError("Failed to load baseline from file: " + filepath);
        return false;
    }

    // Verify baseline integrity
    if (!verifyBaselineSignature(baseline)) {
        setError("Loaded baseline signature verification failed");
        return false;
    }

    // Update cache
    baseline_cache_[test_name] = baseline;

    if (detailed_logging_) {
        std::cout << "Loaded baseline: " << test_name << " from " << filepath << std::endl;
    }

    return true;
}

bool SHA256BaselineValidator::validateAgainstBaseline(const std::string& test_name,
                                                     const std::vector<unsigned char>& current_data,
                                                     const std::map<std::string, double>& current_metrics,
                                                     ValidationResult& result) {
    clearError();

    result = ValidationResult();
    result.test_name = test_name;
    result.timestamp = std::chrono::system_clock::now();
    result.salt = SHA256Hash::generateSalt();

    // Load baseline
    BaselineEntry baseline;
    if (!loadBaseline(test_name, baseline)) {
        result.passed = false;
        result.error_message = "Failed to load baseline for test: " + test_name;
        return false;
    }

    result.baseline_version = baseline.version;

    auto start_time = std::chrono::high_resolution_clock::now();

    // Validate data integrity
    std::vector<unsigned char> current_data_hash = SHA256Hash::computeSaltedHash(current_data, baseline.salt);
    if (current_data_hash != baseline.data_hash) {
        result.passed = false;
        result.error_message = "Data integrity validation failed - hash mismatch";
        result.execution_time_ms = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - start_time).count() / 1000.0;
        return false;
    }

    // Validate performance metrics
    std::map<std::string, double> regressions;
    bool performance_valid = baseline_utils::comparePerformanceMetrics(
        current_metrics, baseline.performance_metrics, regressions, performance_tolerance_);

    if (!performance_valid) {
        result.passed = false;
        result.error_message = "Performance regression detected";
        result.metrics = regressions;
    } else {
        result.passed = true;
        result.metrics = current_metrics;
    }

    result.execution_time_ms = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - start_time).count() / 1000.0;

    // Compute result integrity hash
    if (!computeResultSignature(result)) {
        setError("Failed to compute result signature");
        return false;
    }

    // Save validation result
    std::string validation_filepath = getValidationFilePath(test_name);
    std::string serialized_result = serializeValidationResult(result);

    std::ofstream validation_file(validation_filepath);
    if (!validation_file.is_open()) {
        setError("Failed to save validation result to: " + validation_filepath);
        return false;
    }

    validation_file << serialized_result;
    validation_file.close();

    if (detailed_logging_) {
        std::cout << "Validation completed for: " << test_name << std::endl;
        std::cout << "  Status: " << (result.passed ? "PASSED" : "FAILED") << std::endl;
        std::cout << "  Execution time: " << std::fixed << std::setprecision(2)
                  << result.execution_time_ms << " ms" << std::endl;
    }

    return result.passed;
}

bool SHA256BaselineValidator::validateBatchAgainstBaselines(
    const std::map<std::string, std::vector<unsigned char>>& test_data_map,
    const std::map<std::string, std::map<std::string, double>>& metrics_map,
    std::vector<ValidationResult>& results) {
    clearError();
    results.clear();

    if (!initialized_) {
        setError("Validator not initialized");
        return false;
    }

    bool all_passed = true;

    for (const auto& [test_name, test_data] : test_data_map) {
        ValidationResult result;

        auto metrics_it = metrics_map.find(test_name);
        std::map<std::string, double> metrics = (metrics_it != metrics_map.end()) ?
                                                  metrics_it->second :
                                                  std::map<std::string, double>();

        bool success = validateAgainstBaseline(test_name, test_data, metrics, result);

        results.push_back(result);

        if (!success) {
            all_passed = false;
        }
    }

    if (detailed_logging_) {
        std::cout << "Batch validation completed: " << results.size() << " tests" << std::endl;
        std::cout << "  Passed: " << getPassedTestCount(results) << std::endl;
        std::cout << "  Failed: " << getFailedTestCount(results) << std::endl;
    }

    return all_passed;
}

bool SHA256BaselineValidator::verifyBaselineIntegrity(const std::string& test_name) {
    clearError();

    BaselineEntry baseline;
    if (!loadBaseline(test_name, baseline)) {
        return false;
    }

    return verifyBaselineSignature(baseline);
}

bool SHA256BaselineValidator::verifyAllBaselinesIntegrity(std::vector<std::string>& corrupted_baselines) {
    clearError();
    corrupted_baselines.clear();

    if (!initialized_) {
        setError("Validator not initialized");
        return false;
    }

    for (const auto& entry : std::filesystem::directory_iterator(baseline_directory_)) {
        if (entry.path().extension() == ".baseline") {
            std::string test_name = entry.path().stem().string();

            if (!verifyBaselineIntegrity(test_name)) {
                corrupted_baselines.push_back(test_name);
            }
        }
    }

    if (detailed_logging_) {
        std::cout << "Baseline integrity verification completed" << std::endl;
        std::cout << "  Total baselines: " << getBaselineCount() << std::endl;
        std::cout << "  Corrupted baselines: " << corrupted_baselines.size() << std::endl;
    }

    return corrupted_baselines.empty();
}

bool SHA256BaselineValidator::compareWithBaseline(const std::string& test_name,
                                                const std::map<std::string, double>& current_metrics,
                                                std::map<std::string, double>& regressions,
                                                double tolerance_percentage) {
    clearError();
    regressions.clear();

    BaselineEntry baseline;
    if (!loadBaseline(test_name, baseline)) {
        setError("Failed to load baseline for comparison: " + test_name);
        return false;
    }

    return baseline_utils::comparePerformanceMetrics(
        current_metrics, baseline.performance_metrics, regressions, tolerance_percentage);
}

bool SHA256BaselineValidator::detectPerformanceRegression(const std::string& test_name,
                                                          double current_value,
                                                          double baseline_value,
                                                          double tolerance_percentage,
                                                          bool& is_regression,
                                                          double& regression_percentage) {
    clearError();

    BaselineEntry baseline;
    if (!loadBaseline(test_name, baseline)) {
        setError("Failed to load baseline for regression detection: " + test_name);
        return false;
    }

    if (baseline_value == 0.0) {
        setError("Baseline value is zero, cannot compute regression");
        return false;
    }

    regression_percentage = ((current_value - baseline_value) / baseline_value) * 100.0;
    is_regression = regression_percentage < -tolerance_percentage;

    return true;
}

bool SHA256BaselineValidator::generateIntegrityReport(std::string& report) {
    clearError();

    if (!initialized_) {
        setError("Validator not initialized");
        return false;
    }

    std::ostringstream oss;
    oss << baseline_utils::generateComprehensiveIntegrityReport(
        std::shared_ptr<SHA256BaselineValidator>(this, [](SHA256BaselineValidator*) {}), report);

    report = oss.str();
    return !report.empty();
}

bool SHA256BaselineValidator::generateBaselineReport(const std::string& test_name, std::string& report) {
    clearError();

    BaselineEntry baseline;
    if (!loadBaseline(test_name, baseline)) {
        setError("Failed to load baseline for report generation: " + test_name);
        return false;
    }

    std::ostringstream oss;
    oss << "Baseline Report for: " << test_name << std::endl;
    oss << "========================" << std::endl;
    oss << "Version: " << baseline.version << std::endl;
    oss << "Created: " << baseline_utils::formatTimestamp(baseline.timestamp) << std::endl;
    oss << "Performance Metrics:" << std::endl;
    oss << baseline_utils::formatMetrics(baseline.performance_metrics) << std::endl;
    oss << "Data Hash: " << baseline_utils::formatHash(baseline.data_hash) << std::endl;
    oss << "Metadata Hash: " << baseline_utils::formatHash(baseline.metadata_hash) << std::endl;
    oss << "Signature Hash: " << baseline_utils::formatHash(baseline.signature_hash) << std::endl;

    report = oss.str();
    return true;
}

bool SHA256BaselineValidator::generateValidationReport(const ValidationResult& result, std::string& report) {
    clearError();

    std::ostringstream oss;
    oss << "Validation Report for: " << result.test_name << std::endl;
    oss << "===========================" << std::endl;
    oss << "Status: " << (result.passed ? "PASSED" : "FAILED") << std::endl;
    oss << "Execution Time: " << std::fixed << std::setprecision(2)
        << result.execution_time_ms << " ms" << std::endl;
    oss << "Timestamp: " << baseline_utils::formatTimestamp(result.timestamp) << std::endl;
    oss << "Baseline Version: " << result.baseline_version << std::endl;

    if (!result.passed && !result.error_message.empty()) {
        oss << "Error: " << result.error_message << std::endl;
    }

    if (!result.metrics.empty()) {
        oss << "Metrics:" << std::endl;
        oss << baseline_utils::formatMetrics(result.metrics) << std::endl;
    }

    oss << "Result Hash: " << baseline_utils::formatHash(result.result_hash) << std::endl;
    oss << "Integrity Hash: " << baseline_utils::formatHash(result.integrity_hash) << std::endl;

    report = oss.str();
    return true;
}

size_t SHA256BaselineValidator::getBaselineCount() const {
    if (!initialized_) return 0;

    size_t count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(baseline_directory_)) {
        if (entry.path().extension() == ".baseline") {
            count++;
        }
    }
    return count;
}

std::vector<std::string> SHA256BaselineValidator::getBaselineNames() const {
    std::vector<std::string> names;

    if (!initialized_) return names;

    for (const auto& entry : std::filesystem::directory_iterator(baseline_directory_)) {
        if (entry.path().extension() == ".baseline") {
            names.push_back(entry.path().stem().string());
        }
    }

    std::sort(names.begin(), names.end());
    return names;
}

bool SHA256BaselineValidator::hasBaseline(const std::string& test_name) const {
    if (!initialized_) return false;

    std::string filepath = getBaselineFilePath(test_name);
    return std::filesystem::exists(filepath);
}

// Private helper methods
bool SHA256BaselineValidator::computeBaselineSignature(BaselineEntry& baseline) {
    // Combine all critical elements for signature
    std::ostringstream signature_data;
    signature_data << baseline.test_name << "|"
                     << baseline.version << "|"
                     << SHA256Hash::hashToString(baseline.data_hash) << "|"
                     << SHA256Hash::hashToString(baseline.result_hash) << "|"
                     << SHA256Hash::hashToString(baseline.metadata_hash) << "|"
                     << baseline.performance_metrics.size() << "|"
                     << std::chrono::duration_cast<std::chrono::seconds>(
                         baseline.timestamp.time_since_epoch()).count();

    baseline.signature_hash = SHA256Hash::computeSaltedHash(signature_data.str(), baseline.salt);
    return true;
}

bool SHA256BaselineValidator::computeResultSignature(ValidationResult& result) {
    // Combine all critical elements for integrity hash
    std::ostringstream integrity_data;
    integrity_data << result.test_name << "|"
                    << (result.passed ? "PASSED" : "FAILED") << "|"
                    << std::fixed << std::setprecision(6) << result.execution_time_ms << "|"
                    << result.metrics.size() << "|"
                    << result.baseline_version << "|"
                    << result.error_message << "|"
                    << std::chrono::duration_cast<std::chrono::seconds>(
                        result.timestamp.time_since_epoch()).count();

    result.integrity_hash = SHA256Hash::computeSaltedHash(integrity_data.str(), result.salt);
    result.result_hash = SHA256Hash::computeSaltedHash(integrity_data.str(), result.salt);
    return true;
}

bool SHA256BaselineValidator::verifyBaselineSignature(const BaselineEntry& baseline) {
    // Recompute signature and compare
    BaselineEntry test_baseline = baseline;
    return computeBaselineSignature(test_baseline) &&
           (test_baseline.signature_hash == baseline.signature_hash);
}

bool SHA256BaselineValidator::verifyResultSignature(const ValidationResult& result) {
    // Recompute integrity hash and compare
    ValidationResult test_result = result;
    return computeResultSignature(test_result) &&
           (test_result.integrity_hash == result.integrity_hash);
}

bool SHA256BaselineValidator::saveBaselineToFile(const BaselineEntry& baseline, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    std::string serialized = serializeBaselineEntry(baseline);
    file << serialized;

    return file.good();
}

bool SHA256BaselineValidator::loadBaselineFromFile(const std::string& filepath, BaselineEntry& baseline) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string serialized = buffer.str();

    return deserializeBaselineEntry(serialized, baseline);
}

std::string SHA256BaselineValidator::serializeBaselineEntry(const BaselineEntry& baseline) {
    std::ostringstream oss;
    oss << "# SHA256 Protected Baseline Entry" << std::endl;
    oss << "version=" << baseline.version << std::endl;
    oss << "test_name=" << baseline.test_name << std::endl;
    oss << "timestamp=" << std::chrono::duration_cast<std::chrono::seconds>(
        baseline.timestamp.time_since_epoch()).count() << std::endl;

    // Write salt
    oss << "salt=" << SHA256Hash::hashToString(baseline.salt) << std::endl;

    // Write hashes
    oss << "data_hash=" << SHA256Hash::hashToString(baseline.data_hash) << std::endl;
    oss << "result_hash=" << SHA256Hash::hashToString(baseline.result_hash) << std::endl;
    oss << "metadata_hash=" << SHA256Hash::hashToString(baseline.metadata_hash) << std::endl;
    oss << "signature_hash=" << SHA256Hash::hashToString(baseline.signature_hash) << std::endl;

    // Write performance metrics
    oss << "performance_metrics:" << std::endl;
    oss << serializeMap(baseline.performance_metrics) << std::endl;

    return oss.str();
}

bool SHA256BaselineValidator::deserializeBaselineEntry(const std::string& serialized_data, BaselineEntry& baseline) {
    std::istringstream iss(serialized_data);
    std::string line;

    baseline = BaselineEntry();

    while (std::getline(iss, line)) {
        if (line.empty() || line[0] == '#') continue;

        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;

        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);

        if (key == "version") {
            baseline.version = value;
        } else if (key == "test_name") {
            baseline.test_name = value;
        } else if (key == "timestamp") {
            baseline.timestamp = std::chrono::system_clock::from_time_t(std::stoll(value));
        } else if (key == "salt") {
            baseline.salt = SHA256Hash::stringToHash(value);
        } else if (key == "data_hash") {
            baseline.data_hash = SHA256Hash::stringToHash(value);
        } else if (key == "result_hash") {
            baseline.result_hash = SHA256Hash::stringToHash(value);
        } else if (key == "metadata_hash") {
            baseline.metadata_hash = SHA256Hash::stringToHash(value);
        } else if (key == "signature_hash") {
            baseline.signature_hash = SHA256Hash::stringToHash(value);
        } else if (key == "performance_metrics") {
            // Read the following lines as metrics
            std::string metrics_line;
            std::map<std::string, double> metrics;
            while (std::getline(iss, metrics_line) && !metrics_line.empty() && metrics_line[0] == ' ') {
                size_t metric_eq_pos = metrics_line.find('=');
                if (metric_eq_pos != std::string::npos) {
                    std::string metric_key = metrics_line.substr(1, metric_eq_pos - 1);
                    std::string metric_value = metrics_line.substr(metric_eq_pos + 1);
                    metrics[metric_key] = std::stod(metric_value);
                }
            }
            baseline.performance_metrics = metrics;
        }
    }

    return true;
}

std::string SHA256BaselineValidator::serializeMap(const std::map<std::string, double>& metrics) {
    std::ostringstream oss;
    for (const auto& [key, value] : metrics) {
        oss << "  " << key << "=" << std::scientific << std::setprecision(15) << value << std::endl;
    }
    return oss.str();
}

bool SHA256BaselineValidator::deserializeMap(const std::string& serialized, std::map<std::string, double>& metrics) {
    std::istringstream iss(serialized);
    std::string line;

    while (std::getline(iss, line)) {
        if (line.empty() || line[0] != ' ') continue;

        size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = line.substr(1, eq_pos - 1);
            std::string value = line.substr(eq_pos + 1);
            metrics[key] = std::stod(value);
        }
    }

    return true;
}

std::string SHA256BaselineValidator::getBaselineFilePath(const std::string& test_name) {
    return baseline_directory_ + "/" + test_name + ".baseline";
}

std::string SHA256BaselineValidator::getValidationFilePath(const std::string& test_name) {
    return validation_directory_ + "/" + test_name + ".validation";
}

bool SHA256BaselineValidator::ensureDirectoryExists(const std::string& dir_path) {
    return std::filesystem::create_directories(dir_path);
}

void SHA256BaselineValidator::setError(const std::string& error) {
    last_error_ = error;
    if (detailed_logging_) {
        std::cerr << "SHA256BaselineValidator Error: " << error << std::endl;
    }
}

void SHA256BaselineValidator::clearError() {
    last_error_.clear();
}

size_t SHA256BaselineValidator::getPassedTestCount(const std::vector<ValidationResult>& results) const {
    return std::count_if(results.begin(), results.end(),
                        [](const ValidationResult& result) { return result.passed; });
}

size_t SHA256BaselineValidator::getFailedTestCount(const std::vector<ValidationResult>& results) const {
    return std::count_if(results.begin(), results.end(),
                        [](const ValidationResult& result) { return !result.passed; });
}

// BaselineIntegrityMonitor implementation
BaselineIntegrityMonitor::BaselineIntegrityMonitor()
    : monitoring_interval_seconds_(baseline_constants::DEFAULT_MONITORING_INTERVAL),
      monitoring_active_(false), total_checks_performed_(0), should_stop_monitoring_(false) {
}

BaselineIntegrityMonitor::~BaselineIntegrityMonitor() {
    stopMonitoring();
}

bool BaselineIntegrityMonitor::initialize(std::shared_ptr<SHA256BaselineValidator> validator,
                                             int monitoring_interval_seconds) {
    validator_ = validator;
    monitoring_interval_seconds_ = monitoring_interval_seconds;

    if (!validator_ || !validator_->isInitialized()) {
        last_error_ = "Invalid validator provided";
        return false;
    }

    return true;
}

bool BaselineIntegrityMonitor::startMonitoring() {
    if (monitoring_active_) {
        return true; // Already monitoring
    }

    if (!validator_) {
        last_error_ = "Validator not initialized";
        return false;
    }

    should_stop_monitoring_ = false;
    monitoring_active_ = true;

    monitoring_thread_ = std::thread(&BaselineIntegrityMonitor::monitoringLoop, this);

    std::cout << "Baseline integrity monitoring started (interval: "
              << monitoring_interval_seconds_ << " seconds)" << std::endl;

    return true;
}

bool BaselineIntegrityMonitor::stopMonitoring() {
    if (!monitoring_active_) {
        return true; // Not monitoring
    }

    should_stop_monitoring_ = true;
    monitoring_active_ = false;

    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }

    std::cout << "Baseline integrity monitoring stopped" << std::endl;
    return true;
}

bool BaselineIntegrityMonitor::performIntegrityCheck(std::vector<std::string>& corrupted_baselines) {
    if (!validator_) {
        last_error_ = "Validator not initialized";
        return false;
    }

    bool success = validator_->verifyAllBaselinesIntegrity(corrupted_baselines);
    total_checks_performed_++;
    last_check_time_ = std::chrono::system_clock::now();

    return success;
}

void BaselineIntegrityMonitor::monitoringLoop() {
    while (!should_stop_monitoring_) {
        performScheduledCheck();

        // Sleep for monitoring interval
        std::this_thread::sleep_for(std::chrono::seconds(monitoring_interval_seconds_));
    }
}

void BaselineIntegrityMonitor::performScheduledCheck() {
    std::vector<std::string> corrupted_baselines;
    bool success = performIntegrityCheck(corrupted_baselines);

    if (!success) {
        std::cout << "Integrity check failed: " << last_error_ << std::endl;
    } else if (!corrupted_baselines.empty()) {
        std::cout << "Detected " << corrupted_baselines.size() << " corrupted baselines:" << std::endl;
        for (const auto& baseline : corrupted_baselines) {
            std::cout << "  - " << baseline << std::endl;
        }
    }
}

// Utility functions implementation
namespace baseline_utils {

bool comparePerformanceMetrics(const std::map<std::string, double>& current,
                              const std::map<std::string, double>& baseline,
                              std::map<std::string, double>& regressions,
                              double tolerance_percentage) {
    regressions.clear();
    bool has_regression = false;

    for (const auto& [metric_name, baseline_value] : baseline) {
        auto current_it = current.find(metric_name);
        if (current_it != current.end()) {
            double current_value = current_it->second;
            double change_percentage = ((current_value - baseline_value) / baseline_value) * 100.0;

            // Check for regression (negative change beyond tolerance)
            if (change_percentage < -tolerance_percentage) {
                regressions[metric_name] = change_percentage;
                has_regression = true;
            }
        }
    }

    return !has_regression;
}

bool calculateBaselineStatistics(std::shared_ptr<SHA256BaselineValidator> validator,
                                 BaselineStatistics& stats) {
    if (!validator || !validator->isInitialized()) {
        return false;
    }

    stats = BaselineStatistics();
    stats.total_baselines = validator->getBaselineCount();
    stats.valid_baselines = stats.total_baselines;

    // Check integrity to find corrupted baselines
    std::vector<std::string> corrupted_baselines;
    validator->verifyAllBaselinesIntegrity(corrupted_baselines);
    stats.corrupted_baselines = corrupted_baselines.size();
    stats.valid_baselines = stats.total_baselines - stats.corrupted_baselines;

    // Calculate size statistics
    std::vector<std::string> baseline_names = validator->getBaselineNames();
    double total_size = 0.0;

    for (const auto& name : baseline_names) {
        BaselineEntry baseline;
        if (validator->loadBaseline(name, baseline)) {
            std::string serialized = validator->serializeBaselineEntry(baseline);
            total_size += serialized.size();
        }
    }

    if (stats.total_baselines > 0) {
        stats.average_baseline_size_kb = total_size / stats.total_baselines / 1024.0;
    }

    return true;
}

std::string formatTimestamp(const std::chrono::system_clock::time_point& timestamp) {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string formatMetrics(const std::map<std::string, double>& metrics) {
    std::ostringstream oss;
    for (const auto& [name, value] : metrics) {
        oss << "  " << name << ": " << std::fixed << std::setprecision(6) << value << std::endl;
    }
    return oss.str();
}

std::string formatHash(const std::vector<unsigned char>& hash) {
    return SHA256Hash::hashToString(hash);
}

bool generateComprehensiveIntegrityReport(std::shared_ptr<SHA256BaselineValidator> validator,
                                             std::string& report) {
    if (!validator || !validator->isInitialized()) {
        return false;
    }

    std::ostringstream oss;
    oss << "Comprehensive Baseline Integrity Report" << std::endl;
    oss << "======================================" << std::endl;
    oss << "Generated: " << formatTimestamp(std::chrono::system_clock::now()) << std::endl;
    oss << std::endl;

    // Calculate statistics
    BaselineStatistics stats;
    if (!calculateBaselineStatistics(validator, stats)) {
        return false;
    }

    oss << "Baseline Statistics:" << std::endl;
    oss << "  Total baselines: " << stats.total_baselines << std::endl;
    oss << "  Valid baselines: " << stats.valid_baselines << std::endl;
    oss << "  Corrupted baselines: " << stats.corrupted_baselines << std::endl;
    oss << "  Average size: " << std::fixed << std::setprecision(2)
        << stats.average_baseline_size_kb << " KB" << std::endl;
    oss << std::endl;

    // Check all baselines integrity
    std::vector<std::string> corrupted_baselines;
    bool integrity_ok = validator->verifyAllBaselinesIntegrity(corrupted_baselines);

    oss << "Integrity Status: " << (integrity_ok ? "PASSED" : "FAILED") << std::endl;

    if (!corrupted_baselines.empty()) {
        oss << "Corrupted Baselines:" << std::endl;
        for (const auto& baseline : corrupted_baselines) {
            oss << "  - " << baseline << std::endl;
        }
    }

    report = oss.str();
    return true;
}

} // namespace baseline_utils

} // namespace validation
} // namespace puzzle71
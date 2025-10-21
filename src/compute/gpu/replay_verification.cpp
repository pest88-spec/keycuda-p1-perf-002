// Puzzle71Solver - Deterministic Replay Verification Implementation (T042)
// Architecture Modernization - Deterministic GPU operation verification

#include "replay_verification.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <cstring>
#include <openssl/sha.h>

using json = nlohmann::json;

namespace puzzle71::gpu {

// ReplayCapture Implementation
std::string ReplayCapture::calculateChecksum() const {
    std::ostringstream oss;
    oss << capture_id << device_id << kernel_name
        << launch_config.grid.x << launch_config.grid.y << launch_config.grid.z
        << launch_config.block.x << launch_config.block.y << launch_config.block.z
        << points_per_thread << batch_start.ToHex()
        << deterministic_seed << compressed_format;

    std::string data = oss.str();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);

    std::ostringstream hex_stream;
    hex_stream << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        hex_stream << std::setw(2) << static_cast<int>(hash[i]);
    }
    return hex_stream.str();
}

std::array<std::uint8_t, 32> ReplayCapture::generateFingerprint() const {
    std::array<std::uint8_t, 32> fingerprint{};

    // Create combined data for fingerprinting
    std::vector<std::uint8_t> data;

    // Add basic metadata
    for (char c : capture_id) data.push_back(static_cast<std::uint8_t>(c));
    data.push_back(static_cast<std::uint8_t>(device_id));

    // Add target hash
    for (auto h : target_hash160) {
        data.push_back(static_cast<std::uint8_t>(h >> 24));
        data.push_back(static_cast<std::uint8_t>(h >> 16));
        data.push_back(static_cast<std::uint8_t>(h >> 8));
        data.push_back(static_cast<std::uint8_t>(h));
    }

    // Add deterministic seed
    for (int i = 0; i < 8; ++i) {
        data.push_back(static_cast<std::uint8_t>(deterministic_seed >> (i * 8)));
    }

    // Add compressed format flag
    data.push_back(compressed_format ? 1 : 0);

    // Calculate SHA-256 fingerprint
    SHA256(data.data(), data.size(), fingerprint.data());

    return fingerprint;
}

std::string ReplayCapture::toJson() const {
    json j;

    j["capture_id"] = capture_id;
    j["capture_time"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        capture_time.time_since_epoch()).count();
    j["device_id"] = device_id;
    j["device_name"] = device_name;
    j["kernel_name"] = kernel_name;

    // Launch configuration
    j["launch_config"] = json::parse(launch_config.toJson());
    j["deterministic_seed"] = deterministic_seed;

    // Input data
    j["batch_start"] = batch_start.ToHex();
    j["target_hash160"] = target_hash160;
    j["compressed_format"] = compressed_format;

    // Device state
    j["device_memory_snapshot_base64"] = "";  // Would encode binary data if needed
    j["random_state"] = random_state;
    j["random_state64"] = random_state64;

    // Execution results
    j["total_keys_processed"] = total_keys_processed;
    j["execution_time_us"] = execution_time.count();

    // Output candidates (serialize essential fields)
    json candidates_json = json::array();
    for (const auto& candidate : output_candidates) {
        json candidate_json;
        candidate_json["private_key"] = candidate.private_key.ToHex();
        candidate_json["x_256"] = candidate.x_256.ToHex();
        candidate_json["y_256"] = candidate.y_256.ToHex();
        candidate_json["compressed"] = candidate.isCompressed();
        candidate_json["block"] = candidate.block;
        candidate_json["thread"] = candidate.thread;
        candidate_json["idx"] = candidate.idx;
        candidates_json.push_back(candidate_json);
    }
    j["output_candidates"] = candidates_json;

    // Verification metadata
    j["checksum"] = checksum;
    j["fingerprint_hex"] = std::string(fingerprint.begin(), fingerprint.end());
    j["capture_successful"] = capture_successful;
    j["warnings"] = warnings;
    j["errors"] = errors;

    return j.dump(2);
}

ReplayCapture ReplayCapture::fromJson(const std::string& json_str) {
    ReplayCapture capture;

    try {
        json j = json::parse(json_str);

        capture.capture_id = j.value("capture_id", "");
        capture.capture_time = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(j.value("capture_time", 0)));
        capture.device_id = j.value("device_id", -1);
        capture.device_name = j.value("device_name", "");
        capture.kernel_name = j.value("kernel_name", "");

        // Launch configuration
        if (j.contains("launch_config")) {
            capture.launch_config = KernelLaunchConfig::fromJson(j["launch_config"].dump());
        }

        capture.deterministic_seed = j.value("deterministic_seed", 0);

        // Input data
        capture.batch_start = core::UInt256::FromHex(j.value("batch_start", ""));
        capture.target_hash160 = j.value("target_hash160", std::vector<std::uint32_t>{});
        capture.compressed_format = j.value("compressed_format", false);

        // Device state
        capture.random_state = j.value("random_state", std::vector<std::uint32_t>{});
        capture.random_state64 = j.value("random_state64", std::vector<std::uint64_t>{});

        // Execution results
        capture.total_keys_processed = j.value("total_keys_processed", 0);
        capture.execution_time = std::chrono::microseconds(j.value("execution_time_us", 0));

        // Output candidates
        capture.output_candidates.clear();
        if (j.contains("output_candidates")) {
            for (const auto& candidate_json : j["output_candidates"]) {
                UnifiedCandidate candidate;
                candidate.private_key = core::UInt256::FromHex(candidate_json.value("private_key", ""));
                candidate.x_256 = core::UInt256::FromHex(candidate_json.value("x_256", ""));
                candidate.y_256 = core::UInt256::FromHex(candidate_json.value("y_256", ""));
                candidate.compressed = candidate_json.value("compressed", false);
                candidate.block = candidate_json.value("block", 0);
                candidate.thread = candidate_json.value("thread", 0);
                candidate.idx = candidate_json.value("idx", 0);
                capture.output_candidates.push_back(candidate);
            }
        }

        // Verification metadata
        capture.checksum = j.value("checksum", "");

        std::string fingerprint_hex = j.value("fingerprint_hex", "");
        if (fingerprint_hex.length() == 64) {
            for (size_t i = 0; i < 32; ++i) {
                std::string byte_str = fingerprint_hex.substr(i * 2, 2);
                capture.fingerprint[i] = static_cast<std::uint8_t>(std::stoi(byte_str, nullptr, 16));
            }
        }

        capture.capture_successful = j.value("capture_successful", false);
        capture.warnings = j.value("warnings", std::vector<std::string>{});
        capture.errors = j.value("errors", std::vector<std::string>{});

    } catch (const std::exception& e) {
        // Return empty capture on parse error
        capture = ReplayCapture{};
    }

    return capture;
}

bool ReplayCapture::validate() const {
    if (capture_id.empty()) return false;
    if (device_id < 0) return false;
    if (kernel_name.empty()) return false;
    if (!launch_config.validate()) return false;
    if (target_hash160.size() != 5) return false;
    if (checksum.empty()) return false;

    // Validate fingerprint
    std::array<std::uint8_t, 32> calculated_fingerprint = generateFingerprint();
    if (fingerprint != calculated_fingerprint) return false;

    return true;
}

// ReplayVerificationResult Implementation
std::string ReplayVerificationResult::getSummary() const {
    std::ostringstream oss;

    oss << "Replay Verification Summary:\n";
    oss << "  Verification Status: " << (verification_passed ? "PASSED" : "FAILED") << "\n";
    oss << "  Bit Identical: " << (bit_identical ? "YES" : "NO") << "\n";
    oss << "  Performance Consistent: " << (performance_consistent ? "YES" : "NO") << "\n";
    oss << "  Candidates Match: " << candidates_match_count << "/" << total_candidates << "\n";
    oss << "  Throughput Difference: " << std::fixed << std::setprecision(2) << throughput_difference_percent << "%\n";
    oss << "  Time Difference: " << time_difference.count() << "μs\n";

    if (!errors.empty()) {
        oss << "  Errors (" << errors.size() << "):\n";
        for (const auto& error : errors) {
            oss << "    - " << error << "\n";
        }
    }

    if (!warnings.empty()) {
        oss << "  Warnings (" << warnings.size() << "):\n";
        for (const auto& warning : warnings) {
            oss << "    - " << warning << "\n";
        }
    }

    return oss.str();
}

// DeterministicReplayVerifier Implementation
DeterministicReplayVerifier::DeterministicReplayVerifier(int device_id)
    : device_id_(device_id), deterministic_seed_(123456789ULL) {

    if (!validateDeviceCapabilities()) {
        throw std::runtime_error("Device " + std::to_string(device_id) +
                                 " does not support deterministic replay verification");
    }
}

DeterministicReplayVerifier::~DeterministicReplayVerifier() = default;

ReplayCapture DeterministicReplayVerifier::captureExecution(
    const std::string& kernel_name,
    const KernelLaunchConfig& config,
    const core::UInt256& batch_start,
    const::vector<std::uint32_t>& target_hash160,
    bool compressed_format,
    std::uint64_t deterministic_seed
) {
    ReplayCapture capture;

    // Set deterministic seed
    deterministic_seed_ = deterministic_seed;

    // Generate capture ID
    capture.capture_id = replay_utils::generateExecutionId(kernel_name, device_id_);
    capture.capture_time = std::chrono::system_clock::now();
    capture.device_id = device_id_;
    capture.device_name = "GPU_" + std::to_string(device_id_);  // Would get actual name
    capture.kernel_name = kernel_name;

    // Store configuration and inputs
    capture.launch_config = config;
    capture.deterministic_seed = deterministic_seed;
    capture.batch_start = batch_start;
    capture.target_hash160 = target_hash160;
    capture.compressed_format = compressed_format;

    try {
        // Setup deterministic environment
        createDeterministicEnvironment();

        // Capture device state
        auto device_state = captureDeviceState();

        // Constitutional v5.5: Use deterministic GPU timing
        cudaEvent_t start_event, end_event;
        cudaEventCreate(&start_event);
        cudaEventCreate(&end_event);
        cudaEventRecord(start_event);

        // Execute kernel deterministically
        capture.output_candidates = executeKernelDeterministic(
            kernel_name, config, batch_start, target_hash160, compressed_format
        );

        // Constitutional v5.5: Record end event and calculate deterministic timing
        cudaEventRecord(end_event);
        cudaEventSynchronize(end_event);

        float milliseconds = 0;
        cudaEventElapsedTime(&milliseconds, start_event, end_event);

        // Clean up events
        cudaEventDestroy(start_event);
        cudaEventDestroy(end_event);

        // Convert to microseconds for compatibility
        capture.execution_time = std::chrono::microseconds(static_cast<std::uint64_t>(milliseconds * 1000));

        // Calculate total keys processed
        capture.total_keys_processed = capture.output_candidates.size() * config.points_per_thread;

        // Generate verification metadata
        capture.checksum = capture.calculateChecksum();
        capture.fingerprint = capture.generateFingerprint();
        capture.capture_successful = true;

        // Cleanup deterministic environment
        cleanupDeterministicEnvironment();

    } catch (const std::exception& e) {
        capture.capture_successful = false;
        capture.errors.push_back(std::string("Capture failed: ") + e.what());
    }

    return capture;
}

ReplayVerificationResult DeterministicReplayVerifier::replayAndVerify(
    const ReplayCapture& capture,
    bool enable_performance_comparison
) {
    ReplayVerificationResult result;
    result.verification_time = std::chrono::system_clock::now();

    // Validate capture integrity
    if (!capture.validate()) {
        result.verification_passed = false;
        result.errors.push_back("Invalid capture data");
        return result;
    }

    try {
        // Setup deterministic environment with captured seed
        deterministic_seed_ = capture.deterministic_seed;
        createDeterministicEnvironment();

        // Restore device state if available
        if (!capture.random_state.empty()) {
            // Restore random state
        }

        // Constitutional v5.5: Use deterministic GPU timing
        cudaEvent_t replay_start_event, replay_end_event;
        cudaEventCreate(&replay_start_event);
        cudaEventCreate(&replay_end_event);
        cudaEventRecord(replay_start_event);

        // Execute kernel with captured parameters
        auto replay_results = executeKernelDeterministic(
            capture.kernel_name,
            capture.launch_config,
            capture.batch_start,
            capture.target_hash160,
            capture.compressed_format
        );

        // Record end event and calculate deterministic timing
        cudaEventRecord(replay_end_event);
        cudaEventSynchronize(replay_end_event);

        float replay_milliseconds = 0;
        cudaEventElapsedTime(&replay_milliseconds, replay_start_event, replay_end_event);

        // Clean up events
        cudaEventDestroy(replay_start_event);
        cudaEventDestroy(replay_end_event);

        // Constitutional v5.5: Use deterministic GPU timing result
        auto replay_time = std::chrono::microseconds(static_cast<std::uint64_t>(replay_milliseconds * 1000));
        result.verification_duration = replay_time;

        // Compare results
        result.total_candidates = std::max(capture.output_candidates.size(), replay_results.size());
        result.bit_identical = compareResults(capture.output_candidates, replay_results, result);

        // Performance comparison if enabled
        if (enable_performance_comparison) {
            double original_throughput = calculateThroughput(capture.output_candidates, capture.execution_time);
            double replay_throughput = calculateThroughput(replay_results, replay_time);

            result.original_throughput = original_throughput;
            result.replay_throughput = replay_throughput;

            if (original_throughput > 0) {
                result.throughput_difference_percent =
                    ((replay_throughput - original_throughput) / original_throughput) * 100.0;
                result.performance_consistent = std::abs(result.throughput_difference_percent) < 5.0;  // 5% tolerance
            }
        } else {
            result.performance_consistent = true;  // Skip performance check
        }

        // Overall verification status
        result.verification_passed = result.bit_identical && result.errors.empty();

        // Cleanup
        cleanupDeterministicEnvironment();

    } catch (const std::exception& e) {
        result.verification_passed = false;
        result.errors.push_back(std::string("Replay verification failed: ") + e.what());
    }

    // Update statistics
    verification_stats_.total_verifications++;
    if (result.verification_passed) {
        verification_stats_.successful_verifications++;
    } else {
        verification_stats_.failed_verifications++;
    }

    // Calculate average throughput difference
    if (performance_monitoring_enabled_ && result.original_throughput > 0) {
        double total_diff = verification_stats_.average_throughput_difference * (verification_stats_.total_verifications - 1) +
                          result.throughput_difference_percent;
        verification_stats_.average_throughput_difference = total_diff / verification_stats_.total_verifications;
    }

    // Calculate average verification time
    std::chrono::microseconds total_time = verification_stats_.average_verification_time *
                                           (verification_stats_.total_verifications - 1) + result.verification_duration;
    verification_stats_.average_verification_time = total_time / verification_stats_.total_verifications;

    // Log verification
    logVerification(result);

    return result;
}

ReplayVerificationResult DeterministicReplayVerifier::verifyCaptures(
    const ReplayCapture& capture1,
    const ReplayCapture& capture2
) {
    ReplayVerificationResult result;
    result.verification_time = std::chrono::system_clock::now();

    // Validate both captures
    if (!capture1.validate() || !capture2.validate()) {
        result.verification_passed = false;
        result.errors.push_back("Invalid capture data");
        return result;
    }

    // Check compatibility
    if (capture1.device_id != capture2.device_id) {
        result.errors.push_back("Captures from different devices");
    }

    if (capture1.kernel_name != capture2.kernel_name) {
        result.errors.push_back("Captures from different kernels");
    }

    if (capture1.deterministic_seed != capture2.deterministic_seed) {
        result.errors.push_back("Captures have different deterministic seeds");
    }

    // Compare captures
    result.total_candidates = std::max(capture1.output_candidates.size(), capture2.output_candidates.size());
    result.bit_identical = compareResults(capture1.output_candidates, capture2.output_candidates, result);

    // Performance comparison
    double throughput1 = calculateThroughput(capture1.output_candidates, capture1.execution_time);
    double throughput2 = calculateThroughput(capture2.output_candidates, capture2.execution_time);

    result.original_throughput = throughput1;
    result.replay_throughput = throughput2;

    if (throughput1 > 0) {
        result.throughput_difference_percent = ((throughput2 - throughput1) / throughput1) * 100.0;
        result.performance_consistent = std::abs(result.throughput_difference_percent) < 5.0;
    }

    result.time_difference = capture2.execution_time - capture1.execution_time;

    // Overall verification
    result.verification_passed = result.bit_identical && result.errors.empty();

    // Update statistics
    verification_stats_.total_verifications++;
    if (result.verification_passed) {
        verification_stats_.successful_verifications++;
    } else {
        verification_stats_.failed_verifications++;
    }

    // Log verification
    logVerification(result);

    return result;
}

std::vector<ReplayVerificationResult> DeterministicReplayVerifier::batchVerify(
    const std::vector<ReplayCapture>& captures
) {
    std::vector<ReplayVerificationResult> results;
    results.reserve(captures.size());

    for (const auto& capture : captures) {
        results.push_back(replayAndVerify(capture, true));
    }

    return results;
}

bool DeterministicReplayVerifier::exportCapture(const ReplayCapture& capture, const std::string& filename) {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        file << capture.toJson();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::optional<ReplayCapture> DeterministicReplayVerifier::importCapture(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return std::nullopt;
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        ReplayCapture capture = ReplayCapture::fromJson(content);

        if (capture.validate()) {
            return capture;
        } else {
            return std::nullopt;
        }
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

bool DeterministicReplayVerifier::generateReplayReport(
    const std::vector<ReplayCapture>& captures,
    const std::string& output_filename
) {
    try {
        std::ofstream file(output_filename);
        if (!file.is_open()) {
            return false;
        }

        json report;
        report["report_type"] = "deterministic_replay_verification";
        report["generation_time"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        report["device_id"] = device_id_;
        report["total_captures"] = captures.size();

        // Add capture summary statistics
        size_t successful_captures = 0;
        size_t total_keys_processed = 0;
        std::chrono::microseconds total_execution_time{0};

        for (const auto& capture : captures) {
            if (capture.capture_successful) {
                successful_captures++;
                total_keys_processed += capture.total_keys_processed;
                total_execution_time += capture.execution_time;
            }
        }

        report["successful_captures"] = successful_captures;
        report["total_keys_processed"] = total_keys_processed;
        report["total_execution_time_us"] = total_execution_time.count();
        report["average_throughput"] = total_execution_time.count() > 0 ?
            static_cast<double>(total_keys_processed) / (total_execution_time.count() / 1e6) : 0.0;

        // Add verification statistics
        auto stats = getVerificationStats();
        report["verification_stats"]["total_verifications"] = stats.total_verifications;
        report["verification_stats"]["successful_verifications"] = stats.successful_verifications;
        report["verification_stats"]["failed_verifications"] = stats.failed_verifications;
        report["verification_stats"]["average_throughput_difference_percent"] = stats.average_throughput_difference;
        report["verification_stats"]["average_verification_time_us"] = stats.average_verification_time.count();

        file << report.dump(2);
        return true;

    } catch (const std::exception&) {
        return false;
    }
}

void DeterministicReplayVerifier::setDeterministicSeed(std::uint64_t seed) {
    deterministic_seed_ = seed;
}

DeterministicReplayVerifier::VerificationStats DeterministicReplayVerifier::getVerificationStats() const {
    return verification_stats_;
}

void DeterministicReplayVerifier::clearVerificationStats() {
    verification_stats_ = VerificationStats{};
}

// Private Implementation
DeterministicReplayVerifier::DeviceState DeterministicReplayVerifier::captureDeviceState() {
    DeviceState state;

    // Capture CUDA random state
    // This would capture cuRAND or other random generator state
    // For now, we'll use a simplified approach

    state.captured = true;
    return state;
}

bool DeterministicReplayVerifier::restoreDeviceState(const DeviceState& state) {
    // Restore device state from captured snapshot
    // Implementation would depend on specific GPU and CUDA version

    return state.captured;
}

std::vector<UnifiedCandidate> DeterministicReplayVerifier::executeKernelDeterministic(
    const std::string& kernel_name,
    const KernelLaunchConfig& config,
    const core::UInt256& batch_start,
    const std::vector<std::uint32_t>& target_hash160,
    bool compressed_format
) {
    // This would execute the actual GPU kernel with deterministic parameters
    // For now, we'll return a placeholder result

    std::vector<UnifiedCandidate> results;

    // Execute kernel with deterministic configuration
    // Implementation would depend on the specific kernel type

    return results;
}

bool DeterministicReplayVerifier::compareResults(
    const std::vector<UnifiedCandidate>& results1,
    const std::vector<UnifiedCandidate>& results2,
    ReplayVerificationResult& verification_result
) const {
    verification_result.candidates_match_count = 0;
    verification_result.candidates_mismatch_count = 0;

    size_t min_size = std::min(results1.size(), results2.size());

    for (size_t i = 0; i < min_size; ++i) {
        if (areCandidatesEqual(results1[i], results2[i])) {
            verification_result.candidates_match_count++;
        } else {
            verification_result.candidates_mismatch_count++;

            // Generate mismatch detail
            std::string mismatch_detail = generateMismatchReport(results1[i], results2[i]);
            verification_result.mismatch_details.push_back(mismatch_detail);
        }
    }

    // Check for size differences
    if (results1.size() != results2.size()) {
        verification_result.warnings.push_back("Different result counts: " +
            std::to_string(results1.size()) + " vs " + std::to_string(results2.size()));
    }

    return verification_result.candidates_mismatch_count == 0 &&
           results1.size() == results2.size();
}

bool DeterministicReplayVerifier::areCandidatesEqual(const UnifiedCandidate& c1, const UnifiedCandidate& c2) const {
    return c1.block == c2.block &&
           c1.thread == c2.thread &&
           c1.idx == c2.idx &&
           c1.compressed == c2.compressed &&
           c1.private_key == c2.private_key &&
           c1.x_256 == c2.x_256 &&
           c1.y_256 == c2.y_256;
}

std::string DeterministicReplayVerifier::generateMismatchReport(
    const UnifiedCandidate& expected,
    const UnifiedCandidate& actual
) const {
    std::ostringstream oss;

    oss << "Candidate mismatch (idx=" << actual.idx << "):\n";

    if (expected.block != actual.block) {
        oss << "  Block: expected " << expected.block << ", actual " << actual.block << "\n";
    }

    if (expected.thread != actual.thread) {
        oss << "  Thread: expected " << expected.thread << ", actual " << actual.thread << "\n";
    }

    if (expected.compressed != actual.compressed) {
        oss << "  Compressed: expected " << expected.compressed << ", actual " << actual.compressed << "\n";
    }

    if (expected.private_key != actual.private_key) {
        oss << "  Private Key: expected " << expected.private_key.ToHex()
            << ", actual " << actual.private_key.ToHex() << "\n";
    }

    if (expected.x_256 != actual.x_256) {
        oss << "  X Coordinate: expected " << expected.x_256.ToHex()
            << ", actual " << actual.x_256.ToHex() << "\n";
    }

    if (expected.y_256 != actual.y_256) {
        oss << "  Y Coordinate: expected " << expected.y_256.ToHex()
            << ", actual " << actual.y_256.ToHex() << "\n";
    }

    return oss.str();
}

void DeterministicReplayVerifier::logVerification(const ReplayVerificationResult& result) const {
    // Log verification results to console or file
    // Implementation depends on logging configuration
}

void DeterministicReplayVerifier::setupDeterministicRNG() {
    // Setup deterministic random number generation
    // This would initialize cuRAND or other RNG with deterministic seed
}

void DeterministicReplayVerifier::createDeterministicEnvironment() {
    // Create deterministic execution environment
    setupDeterministicRNG();

    // Disable any sources of nondeterminism
    // Set fixed clock, disable GPU boost, etc.
}

void DeterministicReplayVerifier::cleanupDeterministicEnvironment() {
    // Cleanup deterministic environment
    // Restore normal random number generation
}

bool DeterministicReplayVerifier::validateDeviceCapabilities() const {
    // Check if device supports deterministic replay
    // This would query CUDA capabilities

    cudaDeviceProp props;
    cudaError_t err = cudaGetDeviceProperties(&props, device_id_);
    if (err != cudaSuccess) {
        return false;
    }

    // Check for compute capability 6.0+ (Pascal or later)
    if (props.major < 6) {
        return false;
    }

    // Check for required features
    // This would check for specific CUDA features needed

    return true;
}

double DeterministicReplayVerifier::calculateThroughput(
    const std::vector<UnifiedCandidate>& results,
    std::chrono::microseconds execution_time
) const {
    if (execution_time.count() == 0) return 0.0;

    return static_cast<double>(results.size()) / (execution_time.count() / 1e6);
}

// ReplayVerifierFactory Implementation
std::unique_ptr<DeterministicReplayVerifier> ReplayVerifierFactory::create(int device_id) {
    return std::make_unique<DeterministicReplayVerifier>(device_id);
}

std::unique_ptr<DeterministicReplayVerifier> ReplayVerifierFactory::create(
    int device_id,
    std::uint64_t deterministic_seed,
    bool performance_monitoring
) {
    auto verifier = std::make_unique<DeterministicReplayVerifier>(device_id);
    verifier->setDeterministicSeed(deterministic_seed);
    verifier->setPerformanceMonitoringEnabled(performance_monitoring);
    return verifier;
}

// ReplayUtils Implementation
namespace replay_utils {

std::string generateExecutionId(const std::string& kernel_name, int device_id) {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    std::ostringstream oss;
    oss << kernel_name << "_dev" << device_id << "_" << timestamp;
    return oss.str();
}

bool validateDeviceCompatibility(int source_device, int target_device) {
    // Check device compatibility for replay
    // This would compare device properties and capabilities

    cudaDeviceProp source_props, target_props;
    if (cudaGetDeviceProperties(&source_props, source_device) != cudaSuccess ||
        cudaGetDeviceProperties(&target_props, target_device) != cudaSuccess) {
        return false;
    }

    // Check compute capability compatibility
    if (source_props.major != target_props.major) {
        return false;
    }

    // Check memory compatibility
    if (target_props.totalGlobalMem < source_props.totalGlobalMem * 0.9) {
        return false;
    }

    return true;
}

size_t calculateReplayMemoryRequirements(const ReplayCapture& capture) {
    // Estimate memory needed for replay
    size_t base_memory = sizeof(ReplayCapture) + 1024;  // Base overhead

    // Memory for input data
    size_t input_memory = capture.target_hash160.size() * sizeof(std::uint32_t);

    // Memory for output candidates
    size_t output_memory = capture.output_candidates.size() * sizeof(UnifiedCandidate);

    // Memory for device state
    size_t state_memory = capture.random_state.size() * sizeof(std::uint32_t) +
                       capture.random_state64.size() * sizeof(std::uint64_t) +
                       capture.device_memory_snapshot.size();

    return base_memory + input_memory + output_memory + state_memory;
}

std::chrono::microseconds estimateReplayOverhead(const ReplayCapture& capture) {
    // Estimate replay overhead (capture vs. execution time)
    // Typically 10-20% overhead for deterministic setup

    std::chrono::microseconds overhead = capture.execution_time / 10;
    return std::min(overhead, std::chrono::microseconds(1000000));  // Cap at 1 second
}

std::string formatVerificationResult(const ReplayVerificationResult& result) {
    return result.getSummary();
}

std::vector<ReplayVerificationResult> parseVerificationReport(const std::string& json_str) {
    std::vector<ReplayVerificationResult> results;

    try {
        json report = json::parse(json_str);

        // Parse verification results from report
        // Implementation would depend on report format

    } catch (const std::exception&) {
        // Return empty results on parse error
    }

    return results;
}

} // namespace replay_utils

} // namespace puzzle71::gpu
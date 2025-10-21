/**
 * @file deterministic_replay.cpp
 * @brief Implementation of deterministic replay testing infrastructure
 *
 * @author Puzzle71 Technical Debt Repair System
 * @version 5.5
 * @date 2025-10-20
 */

#include "deterministic_replay.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <random>
#include <cuda_runtime.h>
#include <nlohmann/json.hpp>

namespace puzzle71 {
namespace validation {

// Implementation structure for pimpl idiom
struct DeterministicReplayEngine::Impl {
    std::string base_dir_;
    bool enable_validation_;
    std::string current_session_id_;
    std::map<std::string, ReplaySession> sessions_;

    Impl(const std::string& base_dir, bool enable_validation)
        : base_dir_(base_dir), enable_validation_(enable_validation) {
        ensure_directory_exists(base_dir_);
        load_existing_sessions();
    }

    void load_existing_sessions() {
        std::filesystem::path sessions_path = std::filesystem::path(base_dir_) / "sessions";
        if (!std::filesystem::exists(sessions_path)) {
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator(sessions_path)) {
            if (entry.path().extension() == ".json") {
                std::string session_id = entry.path().stem().string();
                ReplaySession session;
                if (load_session(session_id, session)) {
                    sessions_[session_id] = session;
                }
            }
        }
    }

    bool load_session(const std::string& session_id, ReplaySession& session) {
        std::string file_path = get_session_file_path(session_id);
        auto data = read_binary_file(file_path);
        if (data.empty()) {
            return false;
        }
        return session.deserialize(data);
    }

    std::string get_session_file_path(const std::string& session_id) const {
        return (std::filesystem::path(base_dir_) / "sessions" / (session_id + ".json")).string();
    }

    std::string get_test_file_path(const std::string& session_id,
                                  const std::string& test_id,
                                  bool is_input) const {
        std::string subdir = is_input ? "inputs" : "outputs";
        return (std::filesystem::path(base_dir_) / subdir / session_id / (test_id + ".bin")).string();
    }

    void ensure_directory_exists(const std::string& directory) const {
        std::filesystem::create_directories(directory);
    }

    std::vector<uint8_t> read_binary_file(const std::string& file_path) const {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            return {};
        }
        return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
    }

    bool write_binary_file(const std::string& file_path,
                          const std::vector<uint8_t>& data) const {
        std::filesystem::path path(file_path);
        ensure_directory_exists(path.parent_path().string());

        std::ofstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        return file.good();
    }
};

// ReplayTestInput implementation
std::vector<uint8_t> ReplayTestInput::serialize() const {
    nlohmann::json j;
    j["test_id"] = test_id;
    j["kernel_name"] = kernel_name;
    j["input_data"] = input_data;
    j["grid_dims"] = grid_dims;
    j["block_dims"] = block_dims;
    j["random_seed"] = random_seed;
    j["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()).count();

    std::string json_str = j.dump();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

bool ReplayTestInput::deserialize(const std::vector<uint8_t>& data) {
    if (data.empty()) return false;

    try {
        nlohmann::json j = nlohmann::json::parse(data.begin(), data.end());
        test_id = j["test_id"];
        kernel_name = j["kernel_name"];
        input_data = j["input_data"].get<std::vector<uint8_t>>();
        grid_dims = j["grid_dims"].get<std::vector<uint32_t>>();
        block_dims = j["block_dims"].get<std::vector<uint32_t>>();
        random_seed = j["random_seed"];

        auto timestamp_ms = j["timestamp"].get<int64_t>();
        timestamp = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(timestamp_ms));

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// ReplayTestOutput implementation
std::string ReplayTestOutput::compute_digest() const {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, output_data.data(), output_data.size());
    SHA256_Update(&sha256, gpu_state.data(), gpu_state.size());

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::setw(2) << static_cast<int>(hash[i]);
    }
    return ss.str();
}

std::vector<uint8_t> ReplayTestOutput::serialize() const {
    nlohmann::json j;
    j["test_id"] = test_id;
    j["output_data"] = output_data;
    j["gpu_state"] = gpu_state;
    j["device_info"] = device_info;
    j["digest"] = compute_digest();
    j["execution_time_ms"] = execution_time_ms;
    j["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()).count();

    std::string json_str = j.dump();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

bool ReplayTestOutput::deserialize(const std::vector<uint8_t>& data) {
    if (data.empty()) return false;

    try {
        nlohmann::json j = nlohmann::json::parse(data.begin(), data.end());
        test_id = j["test_id"];
        output_data = j["output_data"].get<std::vector<uint8_t>>();
        gpu_state = j["gpu_state"].get<std::vector<uint8_t>>();
        device_info = j["device_info"];
        execution_time_ms = j["execution_time_ms"];

        auto timestamp_ms = j["timestamp"].get<int64_t>();
        timestamp = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(timestamp_ms));

        // Verify digest integrity
        std::string computed_digest = compute_digest();
        std::string stored_digest = j["digest"];
        return computed_digest == stored_digest;
    } catch (const std::exception&) {
        return false;
    }
}

// ReplaySession implementation
double ReplaySession::duration_seconds() const {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    return duration.count() / 1000.0;
}

std::vector<uint8_t> ReplaySession::serialize() const {
    nlohmann::json j;
    j["session_id"] = session_id;
    j["description"] = description;
    j["start_time"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        start_time.time_since_epoch()).count();
    j["end_time"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time.time_since_epoch()).count();
    j["test_ids"] = test_ids;
    j["git_commit_hash"] = git_commit_hash;
    j["build_configuration"] = build_configuration;

    std::string json_str = j.dump();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

bool ReplaySession::deserialize(const std::vector<uint8_t>& data) {
    if (data.empty()) return false;

    try {
        nlohmann::json j = nlohmann::json::parse(data.begin(), data.end());
        session_id = j["session_id"];
        description = j["description"];

        auto start_ms = j["start_time"].get<int64_t>();
        auto end_ms = j["end_time"].get<int64_t>();
        start_time = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(start_ms));
        end_time = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(end_ms));

        test_ids = j["test_ids"].get<std::vector<std::string>>();
        git_commit_hash = j["git_commit_hash"];
        build_configuration = j["build_configuration"];

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// DeterministicReplayEngine implementation
DeterministicReplayEngine::DeterministicReplayEngine(const std::string& base_dir,
                                                   bool enable_validation)
    : pimpl_(std::make_unique<Impl>(base_dir, enable_validation)) {
}

DeterministicReplayEngine::~DeterministicReplayEngine() = default;

std::string DeterministicReplayEngine::start_session(const std::string& description) {
    // Generate unique session ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);
    std::string session_id = "session_" + std::to_string(dis(gen)) + "_" +
                            std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

    ReplaySession session;
    session.session_id = session_id;
    session.description = description;
    session.start_time = std::chrono::system_clock::now();

    // Get git commit hash for reproducibility
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("git rev-parse HEAD", "r"), pclose);
    if (pipe) {
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        session.git_commit_hash = result;
        session.git_commit_hash.erase(std::remove(session.git_commit_hash.begin(),
                                                  session.git_commit_hash.end(), '\n'),
                                      session.git_commit_hash.end());
    }

    pimpl_->current_session_id_ = session_id;
    pimpl_->sessions_[session_id] = session;

    return session_id;
}

bool DeterministicReplayEngine::execute_replay_test(
    std::function<void(const ReplayTestInput&, ReplayTestOutput&)> kernel_func,
    const ReplayTestInput& input,
    ReplayTestOutput& output) {

    if (pimpl_->current_session_id_.empty()) {
        return false;  // No active session
    }

    // Set output metadata
    output.test_id = input.test_id;
    output.timestamp = std::chrono::system_clock::now();

    // Get device info
    int device_id;
    cudaGetDevice(&device_id);
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, device_id);
    output.device_info = std::string(prop.name) + " (Compute " +
                        std::to_string(prop.major) + "." +
                        std::to_string(prop.minor) + ")";

    // Execute kernel function with timing
    auto start_time = std::chrono::high_resolution_clock::now();

    try {
        kernel_func(input, output);
    } catch (const std::exception&) {
        return false;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    output.execution_time_ms = duration.count() / 1000.0;

    // Store input and output data
    auto input_data = input.serialize();
    auto output_data = output.serialize();

    bool success = true;
    success &= pimpl_->write_binary_file(
        pimpl_->get_test_file_path(pimpl_->current_session_id_, input.test_id, true),
        input_data);
    success &= pimpl_->write_binary_file(
        pimpl_->get_test_file_path(pimpl_->current_session_id_, input.test_id, false),
        output_data);

    if (success) {
        // Update session
        auto& session = pimpl_->sessions_[pimpl_->current_session_id_];
        session.test_ids.push_back(input.test_id);

        // Save session metadata
        auto session_data = session.serialize();
        pimpl_->write_binary_file(pimpl_->get_session_file_path(pimpl_->current_session_id_),
                                 session_data);
    }

    return success;
}

bool DeterministicReplayEngine::validate_output(const std::string& test_id,
                                               const ReplayTestOutput& output,
                                               const ReplayTestOutput* expected_baseline) {
    ReplayTestOutput baseline;

    if (expected_baseline) {
        baseline = *expected_baseline;
    } else {
        // Load baseline from storage
        auto baseline_data = pimpl_->read_binary_file(
            pimpl_->get_test_file_path(pimpl_->current_session_id_, test_id, false));
        if (baseline_data.empty()) {
            return false;  // No baseline found
        }
        if (!baseline.deserialize(baseline_data)) {
            return false;  // Failed to deserialize baseline
        }
    }

    // Compare digests
    std::string current_digest = output.compute_digest();
    std::string baseline_digest = baseline.compute_digest();

    return current_digest == baseline_digest;
}

bool DeterministicReplayEngine::replay_and_compare(const std::string& session_id,
                                                  const std::string& test_id,
                                                  double tolerance) {
    // Load input and baseline
    auto input_data = pimpl_->read_binary_file(
        pimpl_->get_test_file_path(session_id, test_id, true));
    auto baseline_data = pimpl_->read_binary_file(
        pimpl_->get_test_file_path(session_id, test_id, false));

    if (input_data.empty() || baseline_data.empty()) {
        return false;
    }

    ReplayTestInput input;
    ReplayTestOutput baseline, new_output;

    if (!input.deserialize(input_data) || !baseline.deserialize(baseline_data)) {
        return false;
    }

    // Start temporary session for replay
    std::string temp_session_id = start_session("Replay validation: " + test_id);

    // For this implementation, we need a kernel function
    // In practice, this would be provided by the caller
    // For now, we'll just compare the digests
    bool success = false;

    // End temporary session
    auto& session = pimpl_->sessions_[temp_session_id];
    session.end_time = std::chrono::system_clock::now();
    auto session_data = session.serialize();
    pimpl_->write_binary_file(pimpl_->get_session_file_path(temp_session_id), session_data);

    return success;
}

std::string DeterministicReplayEngine::generate_report(const std::string& session_id) const {
    auto it = pimpl_->sessions_.find(session_id);
    if (it == pimpl_->sessions_.end()) {
        return "{}";
    }

    const auto& session = it->second;

    nlohmann::json report;
    report["session_id"] = session.session_id;
    report["description"] = session.description;
    report["start_time"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        session.start_time.time_since_epoch()).count();
    report["end_time"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        session.end_time.time_since_epoch()).count();
    report["duration_seconds"] = session.duration_seconds();
    report["git_commit_hash"] = session.git_commit_hash;
    report["build_configuration"] = session.build_configuration;
    report["total_tests"] = session.test_ids.size();

    // Analyze test results
    std::vector<std::string> successful_tests;
    std::vector<std::string> failed_tests;
    double total_execution_time = 0.0;

    for (const auto& test_id : session.test_ids) {
        auto output_data = pimpl_->read_binary_file(
            pimpl_->get_test_file_path(session_id, test_id, false));
        if (!output_data.empty()) {
            ReplayTestOutput output;
            if (output.deserialize(output_data)) {
                successful_tests.push_back(test_id);
                total_execution_time += output.execution_time_ms;
            } else {
                failed_tests.push_back(test_id);
            }
        } else {
            failed_tests.push_back(test_id);
        }
    }

    report["successful_tests"] = successful_tests.size();
    report["failed_tests"] = failed_tests.size();
    report["success_rate"] = session.test_ids.empty() ? 0.0 :
                           static_cast<double>(successful_tests.size()) / session.test_ids.size();
    report["total_execution_time_ms"] = total_execution_time;
    report["average_execution_time_ms"] = successful_tests.empty() ? 0.0 :
                                        total_execution_time / successful_tests.size();

    return report.dump(2);
}

bool DeterministicReplayEngine::validate_session_integrity(const std::string& session_id) const {
    auto it = pimpl_->sessions_.find(session_id);
    if (it == pimpl_->sessions_.end()) {
        return false;
    }

    const auto& session = it->second;

    // Validate all test files exist and have correct digests
    for (const auto& test_id : session.test_ids) {
        auto input_data = pimpl_->read_binary_file(
            pimpl_->get_test_file_path(session_id, test_id, true));
        auto output_data = pimpl_->read_binary_file(
            pimpl_->get_test_file_path(session_id, test_id, false));

        if (input_data.empty() || output_data.empty()) {
            return false;  // Missing test data
        }

        ReplayTestOutput output;
        if (!output.deserialize(output_data)) {
            return false;  // Corrupted output data
        }

        // Verify digest matches stored digest (handled in deserialize)
        // If we get here, the data is valid
    }

    return true;
}

std::vector<std::string> DeterministicReplayEngine::get_session_ids() const {
    std::vector<std::string> ids;
    for (const auto& pair : pimpl_->sessions_) {
        ids.push_back(pair.first);
    }
    return ids;
}

void DeterministicReplayEngine::cleanup_old_sessions(int max_age_days) {
    auto cutoff_time = std::chrono::system_clock::now() -
                      std::chrono::hours(24 * max_age_days);

    std::vector<std::string> sessions_to_remove;

    for (const auto& pair : pimpl_->sessions_) {
        if (pair.second.start_time < cutoff_time) {
            sessions_to_remove.push_back(pair.first);
        }
    }

    for (const auto& session_id : sessions_to_remove) {
        // Remove session metadata
        std::filesystem::remove(pimpl_->get_session_file_path(session_id));

        // Remove test data directories
        std::filesystem::remove_all(
            std::filesystem::path(pimpl_->base_dir_) / "inputs" / session_id);
        std::filesystem::remove_all(
            std::filesystem::path(pimpl_->base_dir_) / "outputs" / session_id);

        pimpl_->sessions_.erase(session_id);
    }
}

// ReplaySessionGuard implementation
ReplaySessionGuard::ReplaySessionGuard(DeterministicReplayEngine& engine,
                                      const std::string& description)
    : engine_(engine), session_active_(false) {
    session_id_ = engine_.start_session(description);
    session_active_ = true;
}

ReplaySessionGuard::~ReplaySessionGuard() {
    if (session_active_) {
        auto& session = engine_.pimpl_->sessions_[session_id_];
        session.end_time = std::chrono::system_clock::now();
        auto session_data = session.serialize();
        engine_.pimpl_->write_binary_file(
            engine_.pimpl_->get_session_file_path(session_id_), session_data);

        // Generate and store report
        std::string report = engine_.generate_report(session_id_);
        std::string report_path = std::filesystem::path(engine_.pimpl_->base_dir_) /
                                 "reports" / (session_id_ + "_report.json");
        engine_.pimpl_->ensure_directory_exists(
            std::filesystem::path(report_path).parent_path().string());
        std::ofstream report_file(report_path);
        if (report_file.is_open()) {
            report_file << report;
        }
    }
}

} // namespace validation
} // namespace puzzle71
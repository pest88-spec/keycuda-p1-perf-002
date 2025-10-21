/**
 * @file puzzle71_static_config.cpp
 * @brief Implementation of static YAML configuration system
 *
 * @author Puzzle71 Technical Debt Repair System
 * @version 5.5
 * @date 2025-10-20
 */

#include "puzzle71_static_config.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace puzzle71 {
namespace config {

// GPUDeviceConfig implementation
YAML::Node GPUDeviceConfig::to_yaml() const {
    YAML::Node node;
    node["device_id"] = device_id;
    node["device_name"] = device_name;
    node["compute_capability"]["major"] = compute_capability_major;
    node["compute_capability"]["minor"] = compute_capability_minor;
    node["total_memory_bytes"] = total_memory_bytes;
    node["max_threads_per_block"] = max_threads_per_block;
    node["max_blocks_per_sm"] = max_blocks_per_sm;
    node["warp_size"] = warp_size;
    node["max_shared_memory_per_block"] = max_shared_memory_per_block;
    node["max_registers_per_block"] = max_registers_per_block;
    node["multiprocessor_count"] = multiprocessor_count;
    node["max_threads_per_multiprocessor"] = max_threads_per_multiprocessor;
    node["memory_bandwidth_gbps"] = memory_bandwidth_gbps;
    node["peak_compute_gflops"] = peak_compute_gflops;
    node["architecture_family"] = architecture_family;
    return node;
}

bool GPUDeviceConfig::from_yaml(const YAML::Node& node) {
    try {
        device_id = node["device_id"].as<int>();
        device_name = node["device_name"].as<std::string>();
        compute_capability_major = node["compute_capability"]["major"].as<int>();
        compute_capability_minor = node["compute_capability"]["minor"].as<int>();
        total_memory_bytes = node["total_memory_bytes"].as<uint64_t>();
        max_threads_per_block = node["max_threads_per_block"].as<uint32_t>();
        max_blocks_per_sm = node["max_blocks_per_sm"].as<uint32_t>();
        warp_size = node["warp_size"].as<uint32_t>();
        max_shared_memory_per_block = node["max_shared_memory_per_block"].as<uint32_t>();
        max_registers_per_block = node["max_registers_per_block"].as<uint32_t>();
        multiprocessor_count = node["multiprocessor_count"].as<uint32_t>();
        max_threads_per_multiprocessor = node["max_threads_per_multiprocessor"].as<uint32_t>();
        memory_bandwidth_gbps = node["memory_bandwidth_gbps"].as<double>();
        peak_compute_gflops = node["peak_compute_gflops"].as<double>();
        architecture_family = node["architecture_family"].as<std::string>();
        return true;
    } catch (const YAML::Exception&) {
        return false;
    }
}

bool GPUDeviceConfig::validate() const {
    return device_id >= 0 &&
           !device_name.empty() &&
           compute_capability_major >= 3 &&  // Minimum CUDA 3.5
           compute_capability_minor >= 0 &&
           total_memory_bytes > 0 &&
           max_threads_per_block > 0 &&
           max_blocks_per_sm > 0 &&
           warp_size > 0 &&
           max_shared_memory_per_block > 0 &&
           max_registers_per_block > 0 &&
           multiprocessor_count > 0 &&
           max_threads_per_multiprocessor > 0 &&
           memory_bandwidth_gbps > 0.0 &&
           peak_compute_gflops > 0.0 &&
           !architecture_family.empty();
}

// KernelLaunchConfig implementation
YAML::Node KernelLaunchConfig::to_yaml() const {
    YAML::Node node;
    node["kernel_name"] = kernel_name;
    node["architecture_family"] = architecture_family;
    node["block_size"]["x"] = block_size_x;
    node["block_size"]["y"] = block_size_y;
    node["block_size"]["z"] = block_size_z;
    node["min_grid_size"] = min_grid_size;
    node["max_grid_size"] = max_grid_size;
    node["shared_memory_size_bytes"] = shared_memory_size_bytes;
    node["registers_per_thread"] = registers_per_thread;
    node["expected_occupancy"] = expected_occupancy;

    YAML::Node bounds_node;
    for (const auto& bound : launch_bounds) {
        bounds_node[bound.first] = bound.second;
    }
    node["launch_bounds"] = bounds_node;

    return node;
}

bool KernelLaunchConfig::from_yaml(const YAML::Node& node) {
    try {
        kernel_name = node["kernel_name"].as<std::string>();
        architecture_family = node["architecture_family"].as<std::string>();
        block_size_x = node["block_size"]["x"].as<uint32_t>();
        block_size_y = node["block_size"]["y"].as<uint32_t>();
        block_size_z = node["block_size"]["z"].as<uint32_t>();
        min_grid_size = node["min_grid_size"].as<uint32_t>();
        max_grid_size = node["max_grid_size"].as<uint32_t>();
        shared_memory_size_bytes = node["shared_memory_size_bytes"].as<uint32_t>();
        registers_per_thread = node["registers_per_thread"].as<uint32_t>();
        expected_occupancy = node["expected_occupancy"].as<double>();

        const auto& bounds_node = node["launch_bounds"];
        for (const auto& item : bounds_node) {
            launch_bounds[item.first.as<std::string>()] = item.second.as<uint32_t>();
        }

        return true;
    } catch (const YAML::Exception&) {
        return false;
    }
}

bool KernelLaunchConfig::validate() const {
    uint32_t total_block_size = block_size_x * block_size_y * block_size_z;

    return !kernel_name.empty() &&
           !architecture_family.empty() &&
           block_size_x > 0 && block_size_y > 0 && block_size_z > 0 &&
           total_block_size <= 1024 &&  // CUDA maximum block size
           min_grid_size > 0 &&
           max_grid_size >= min_grid_size &&
           registers_per_thread > 0 &&
           registers_per_thread <= 255 &&  // CUDA maximum
           expected_occupancy > 0.0 &&
           expected_occupancy <= 1.0;
}

// PerformanceConfig implementation
YAML::Node PerformanceConfig::to_yaml() const {
    YAML::Node node;
    node["enable_shared_memory_optimization"] = enable_shared_memory_optimization;
    node["enable_warp_level_optimization"] = enable_warp_level_optimization;
    node["enable_memory_coalescing"] = enable_memory_coalescing;
    node["enable_register_optimization"] = enable_register_optimization;
    node["memory_alignment_bytes"] = memory_alignment_bytes;
    node["shared_memory_bank_size"] = shared_memory_bank_size;
    node["target_gpu_utilization_percent"] = target_gpu_utilization_percent;
    node["target_memory_efficiency_percent"] = target_memory_efficiency_percent;
    node["max_concurrent_kernels"] = max_concurrent_kernels;

    YAML::Node weights_node;
    for (const auto& weight : optimization_weights) {
        weights_node[weight.first] = weight.second;
    }
    node["optimization_weights"] = weights_node;

    return node;
}

bool PerformanceConfig::from_yaml(const YAML::Node& node) {
    try {
        enable_shared_memory_optimization = node["enable_shared_memory_optimization"].as<bool>();
        enable_warp_level_optimization = node["enable_warp_level_optimization"].as<bool>();
        enable_memory_coalescing = node["enable_memory_coalescing"].as<bool>();
        enable_register_optimization = node["enable_register_optimization"].as<bool>();
        memory_alignment_bytes = node["memory_alignment_bytes"].as<uint32_t>();
        shared_memory_bank_size = node["shared_memory_bank_size"].as<uint32_t>();
        target_gpu_utilization_percent = node["target_gpu_utilization_percent"].as<double>();
        target_memory_efficiency_percent = node["target_memory_efficiency_percent"].as<double>();
        max_concurrent_kernels = node["max_concurrent_kernels"].as<uint32_t>();

        const auto& weights_node = node["optimization_weights"];
        for (const auto& item : weights_node) {
            optimization_weights[item.first.as<std::string>()] = item.second.as<double>();
        }

        return true;
    } catch (const YAML::Exception&) {
        return false;
    }
}

bool PerformanceConfig::validate() const {
    return memory_alignment_bytes > 0 &&
           (memory_alignment_bytes & (memory_alignment_bytes - 1)) == 0 &&  // Power of 2
           shared_memory_bank_size > 0 &&
           target_gpu_utilization_percent > 0.0 &&
           target_gpu_utilization_percent <= 100.0 &&
           target_memory_efficiency_percent > 0.0 &&
           target_memory_efficiency_percent <= 100.0 &&
           max_concurrent_kernels > 0;
}

// ValidationConfig implementation
YAML::Node ValidationConfig::to_yaml() const {
    YAML::Node node;
    node["enable_deterministic_validation"] = enable_deterministic_validation;
    node["enable_cpu_gpu_validation"] = enable_cpu_gpu_validation;
    node["enable_performance_regression_detection"] = enable_performance_regression_detection;
    node["enable_constitutional_compliance"] = enable_constitutional_compliance;
    node["validation_precision_tolerance"] = validation_precision_tolerance;
    node["min_validation_iterations"] = min_validation_iterations;
    node["max_validation_runtime_seconds"] = max_validation_runtime_seconds;
    node["baseline_storage_directory"] = baseline_storage_directory;

    YAML::Node validators_node;
    for (const auto& validator : enabled_validators) {
        validators_node.push_back(validator);
    }
    node["enabled_validators"] = validators_node;

    return node;
}

bool ValidationConfig::from_yaml(const YAML::Node& node) {
    try {
        enable_deterministic_validation = node["enable_deterministic_validation"].as<bool>();
        enable_cpu_gpu_validation = node["enable_cpu_gpu_validation"].as<bool>();
        enable_performance_regression_detection = node["enable_performance_regression_detection"].as<bool>();
        enable_constitutional_compliance = node["enable_constitutional_compliance"].as<bool>();
        validation_precision_tolerance = node["validation_precision_tolerance"].as<double>();
        min_validation_iterations = node["min_validation_iterations"].as<uint32_t>();
        max_validation_runtime_seconds = node["max_validation_runtime_seconds"].as<uint32_t>();
        baseline_storage_directory = node["baseline_storage_directory"].as<std::string>();

        const auto& validators_node = node["enabled_validators"];
        for (const auto& validator : validators_node) {
            enabled_validators.push_back(validator.as<std::string>());
        }

        return true;
    } catch (const YAML::Exception&) {
        return false;
    }
}

bool ValidationConfig::validate() const {
    return validation_precision_tolerance >= 0.0 &&
           min_validation_iterations > 0 &&
           max_validation_runtime_seconds > 0 &&
           !baseline_storage_directory.empty();
}

// Puzzle71StaticConfig implementation
std::string Puzzle71StaticConfig::to_yaml_string() const {
    YAML::Node root;
    root["config_version"] = config_version;
    root["config_schema_version"] = config_schema_version;
    root["creation_timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        creation_timestamp.time_since_epoch()).count();
    root["git_commit_hash"] = git_commit_hash;
    root["config_checksum"] = config_checksum;

    YAML::Node devices_node;
    for (const auto& device : gpu_devices) {
        devices_node.push_back(device.to_yaml());
    }
    root["gpu_devices"] = devices_node;

    YAML::Node kernels_node;
    for (const auto& kernel : kernel_configs) {
        kernels_node.push_back(kernel.to_yaml());
    }
    root["kernel_configs"] = kernels_node;

    root["performance_config"] = performance_config.to_yaml();
    root["validation_config"] = validation_config.to_yaml();

    YAML::Node metadata_node;
    for (const auto& item : metadata) {
        metadata_node[item.first] = item.second;
    }
    root["metadata"] = metadata_node;

    std::stringstream ss;
    ss << root;
    return ss.str();
}

bool Puzzle71StaticConfig::from_yaml_string(const std::string& yaml_string) {
    try {
        YAML::Node root = YAML::Load(yaml_string);

        config_version = root["config_version"].as<std::string>();
        config_schema_version = root["config_schema_version"].as<std::string>();
        git_commit_hash = root["git_commit_hash"].as<std::string>();
        config_checksum = root["config_checksum"].as<std::string>();

        auto timestamp_ms = root["creation_timestamp"].as<int64_t>();
        creation_timestamp = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(timestamp_ms));

        const auto& devices_node = root["gpu_devices"];
        for (const auto& device_node : devices_node) {
            GPUDeviceConfig device;
            if (device.from_yaml(device_node)) {
                gpu_devices.push_back(device);
            }
        }

        const auto& kernels_node = root["kernel_configs"];
        for (const auto& kernel_node : kernels_node) {
            KernelLaunchConfig kernel;
            if (kernel.from_yaml(kernel_node)) {
                kernel_configs.push_back(kernel);
            }
        }

        performance_config.from_yaml(root["performance_config"]);
        validation_config.from_yaml(root["validation_config"]);

        const auto& metadata_node = root["metadata"];
        for (const auto& item : metadata_node) {
            metadata[item.first.as<std::string>()] = item.second.as<std::string>();
        }

        return true;
    } catch (const YAML::Exception&) {
        return false;
    }
}

std::string Puzzle71StaticConfig::compute_checksum() const {
    std::string config_content = to_yaml_string();

    // Exclude checksum field from computation
    size_t checksum_pos = config_content.find("config_checksum:");
    if (checksum_pos != std::string::npos) {
        size_t line_end = config_content.find('\n', checksum_pos);
        if (line_end != std::string::npos) {
            config_content.erase(checksum_pos, line_end - checksum_pos + 1);
        }
    }

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, config_content.c_str(), config_content.length());

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::setw(2) << static_cast<int>(hash[i]);
    }
    return ss.str();
}

bool Puzzle71StaticConfig::validate() const {
    if (config_version.empty() || config_schema_version.empty()) {
        return false;
    }

    // Validate all GPU device configurations
    for (const auto& device : gpu_devices) {
        if (!device.validate()) {
            return false;
        }
    }

    // Validate all kernel configurations
    for (const auto& kernel : kernel_configs) {
        if (!kernel.validate()) {
            return false;
        }
    }

    // Validate performance and validation configurations
    if (!performance_config.validate() || !validation_config.validate()) {
        return false;
    }

    // Verify checksum
    std::string computed_checksum = compute_checksum();
    if (computed_checksum != config_checksum) {
        return false;
    }

    return true;
}

const GPUDeviceConfig* Puzzle71StaticConfig::get_device_config(int device_id) const {
    auto it = std::find_if(gpu_devices.begin(), gpu_devices.end(),
                          [device_id](const GPUDeviceConfig& config) {
                              return config.device_id == device_id;
                          });
    return (it != gpu_devices.end()) ? &(*it) : nullptr;
}

const KernelLaunchConfig* Puzzle71StaticConfig::get_kernel_config(
    const std::string& kernel_name,
    const std::string& architecture_family) const {
    auto it = std::find_if(kernel_configs.begin(), kernel_configs.end(),
                          [&kernel_name, &architecture_family](const KernelLaunchConfig& config) {
                              return config.kernel_name == kernel_name &&
                                     config.architecture_family == architecture_family;
                          });
    return (it != kernel_configs.end()) ? &(*it) : nullptr;
}

// StaticConfigLoader implementation structure
struct StaticConfigLoader::Impl {
    std::string config_directory_;
    std::map<std::string, Puzzle71StaticConfig> config_cache_;

    Impl(const std::string& config_directory) : config_directory_(config_directory) {
        std::filesystem::create_directories(config_directory_);
    }
};

StaticConfigLoader::StaticConfigLoader(const std::string& config_directory)
    : pimpl_(std::make_unique<Impl>(config_directory)) {
}

StaticConfigLoader::~StaticConfigLoader() = default;

bool StaticConfigLoader::load_config(const std::string& config_file,
                                    Puzzle71StaticConfig& config) {
    std::string file_path = get_config_path(config_file);
    std::string content = read_config_file(file_path);

    if (content.empty()) {
        return false;
    }

    if (!config.from_yaml_string(content)) {
        return false;
    }

    if (!config.validate()) {
        return false;
    }

    // Cache the loaded configuration
    pimpl_->config_cache_[config_file] = config;

    return true;
}

bool StaticConfigLoader::save_config(const std::string& config_file,
                                    const Puzzle71StaticConfig& config) {
    Puzzle71StaticConfig config_copy = config;

    // Compute and set checksum
    config_copy.config_checksum = config_copy.compute_checksum();

    std::string yaml_content = config_copy.to_yaml_string();

    bool success = write_config_file(get_config_path(config_file), yaml_content);

    if (success) {
        // Update cache
        pimpl_->config_cache_[config_file] = config_copy;
    }

    return success;
}

bool StaticConfigLoader::validate_config_file(const std::string& config_file) const {
    auto it = pimpl_->config_cache_.find(config_file);
    if (it != pimpl_->config_cache_.end()) {
        return it->second.validate();
    }

    std::string content = read_config_file(get_config_path(config_file));
    if (content.empty()) {
        return false;
    }

    Puzzle71StaticConfig config;
    if (!config.from_yaml_string(content)) {
        return false;
    }

    return config.validate();
}

const Puzzle71StaticConfig* StaticConfigLoader::get_cached_config(
    const std::string& config_file) const {
    auto it = pimpl_->config_cache_.find(config_file);
    return (it != pimpl_->config_cache_.end()) ? &(it->second) : nullptr;
}

void StaticConfigLoader::clear_cache() {
    pimpl_->config_cache_.clear();
}

Puzzle71StaticConfig StaticConfigLoader::generate_default_config(
    int device_id,
    const std::string& device_name,
    int compute_major,
    int compute_minor,
    uint64_t memory_bytes) const {

    Puzzle71StaticConfig config;
    config.config_version = "5.5";
    config.config_schema_version = "1.0";
    config.creation_timestamp = std::chrono::system_clock::now();

    // Get git commit hash
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("git rev-parse HEAD", "r"), pclose);
    if (pipe) {
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        config.git_commit_hash = result;
        config.git_commit_hash.erase(std::remove(config.git_commit_hash.begin(),
                                                 config.git_commit_hash.end(), '\n'),
                                     config.git_commit_hash.end());
    }

    // GPU device configuration
    GPUDeviceConfig device;
    device.device_id = device_id;
    device.device_name = device_name;
    device.compute_capability_major = compute_major;
    device.compute_capability_minor = compute_minor;
    device.total_memory_bytes = memory_bytes;
    device.max_threads_per_block = 1024;
    device.max_blocks_per_sm = 32;
    device.warp_size = 32;
    device.max_shared_memory_per_block = 48 * 1024;
    device.max_registers_per_block = 65536;
    device.multiprocessor_count = 68;  // Default for RTX 2080 Ti
    device.max_threads_per_multiprocessor = 2048;
    device.memory_bandwidth_gbps = 616.0;  // RTX 2080 Ti
    device.peak_compute_gflops = 13414.0;  // RTX 2080 Ti

    // Determine architecture family
    if (compute_major == 7) {
        device.architecture_family = (compute_minor == 5) ? "Turing" : "Volta";
    } else if (compute_major == 8) {
        device.architecture_family = "Ampere";
    } else if (compute_major == 9) {
        device.architecture_family = "Hopper";
    } else {
        device.architecture_family = "Unknown";
    }

    config.gpu_devices.push_back(device);

    // Performance configuration
    config.performance_config.enable_shared_memory_optimization = true;
    config.performance_config.enable_warp_level_optimization = true;
    config.performance_config.enable_memory_coalescing = true;
    config.performance_config.enable_register_optimization = true;
    config.performance_config.memory_alignment_bytes = 128;
    config.performance_config.shared_memory_bank_size = 4;
    config.performance_config.target_gpu_utilization_percent = 90.0;
    config.performance_config.target_memory_efficiency_percent = 95.0;
    config.performance_config.max_concurrent_kernels = 16;

    // Validation configuration
    config.validation_config.enable_deterministic_validation = true;
    config.validation_config.enable_cpu_gpu_validation = true;
    config.validation_config.enable_performance_regression_detection = true;
    config.validation_config.enable_constitutional_compliance = true;
    config.validation_config.validation_precision_tolerance = 1e-10;
    config.validation_config.min_validation_iterations = 10000;
    config.validation_config.max_validation_runtime_seconds = 300;
    config.validation_config.baseline_storage_directory = "baselines/";
    config.validation_config.enabled_validators = {"ecc", "deterministic", "performance"};

    // Metadata
    config.metadata["generated_by"] = "Puzzle71 Technical Debt Repair System";
    config.metadata["purpose"] = "Static configuration for deterministic GPU operations";

    // Compute checksum
    config.config_checksum = config.compute_checksum();

    return config;
}

std::string StaticConfigLoader::get_config_path(const std::string& config_file) const {
    return std::filesystem::path(pimpl_->config_directory_) / config_file;
}

std::string StaticConfigLoader::read_config_file(const std::string& file_path) const {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return {};
    }
    return std::string(std::istreambuf_iterator<char>(file),
                      std::istreambuf_iterator<char>());
}

bool StaticConfigLoader::write_config_file(const std::string& file_path,
                                          const std::string& content) const {
    std::filesystem::path path(file_path);
    std::filesystem::create_directories(path.parent_path());

    std::ofstream file(file_path);
    if (!file.is_open()) {
        return false;
    }
    file << content;
    return file.good();
}

std::string StaticConfigLoader::compute_file_checksum(const std::string& content) const {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, content.c_str(), content.length());

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::setw(2) << static_cast<int>(hash[i]);
    }
    return ss.str();
}

// ConfigGuard implementation
ConfigGuard::ConfigGuard(StaticConfigLoader& loader, const std::string& config_file)
    : loader_(loader), is_valid_(false) {
    is_valid_ = loader_.load_config(config_file, config_);
}

ConfigGuard::~ConfigGuard() {
    // Automatic cleanup and validation handled by RAII
}

} // namespace config
} // namespace puzzle71
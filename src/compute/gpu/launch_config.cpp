// Puzzle71Solver - Kernel Launch Configuration Implementation (T041)
// Architecture Modernization - Standardized kernel configuration management

#include "launch_config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstring>

using json = nlohmann::json;

namespace puzzle71::gpu {

// GpuConstraints Implementation
dim3 GpuConstraints::getOptimalBlockSize(KernelType kernel_type) const {
    switch (kernel_type) {
        case KernelType::SEPARATED_ECC:
            return dim3(256, 1, 1);  // Optimized for ECC operations
        case KernelType::SEPARATED_HASH:
            return dim3(128, 1, 1);  // Optimized for hash operations
        case KernelType::SEPARATED_COMPARE:
            return dim3(512, 1, 1);  // Optimized for compare operations
        case KernelType::SEPARATED_PIPELINE:
            return dim3(256, 1, 1);  // Balanced for pipeline
        case KernelType::MEMORY_OPTIMIZED:
            return dim3(128, 1, 1);  // Conservative for memory
        case KernelType::FUSED_PUZZLE71:
        default:
            return dim3(256, 1, 1);  // Default good balance
    }
}

int GpuConstraints::getMaxThreadsPerBlock(KernelType kernel_type) const {
    switch (kernel_type) {
        case KernelType::SEPARATED_ECC:
            return std::min(max_threads_per_block, 256);
        case KernelType::SEPARATED_HASH:
            return std::min(max_threads_per_block, 128);
        case KernelType::SEPARATED_COMPARE:
            return std::min(max_threads_per_block, 512);
        default:
            return max_threads_per_block;
    }
}

// ResourceProfile Implementation
bool ResourceProfile::isFeasible(const GpuConstraints& constraints) const {
    return (registers_per_thread <= constraints.max_registers_per_thread) &&
           (shared_memory_per_block <= constraints.max_shared_memory_per_block) &&
           (threads_per_block <= constraints.max_threads_per_block) &&
           (expected_occupancy > 0.1);  // At least 10% occupancy
}

double ResourceProfile::calculateThroughput(const GpuConstraints& constraints) const {
    // Simplified throughput model: throughput ∝ occupancy × efficiency
    double base_throughput = constraints.max_threads_per_sm * expected_occupancy;

    // Architecture-specific scaling factors
    double arch_factor = 1.0;
    if (constraints.compute_capability_major >= 8) {  // Ampere and later
        arch_factor = 1.5;
    } else if (constraints.compute_capability_major >= 7) {  // Turing
        arch_factor = 1.2;
    }

    // Memory bandwidth factor
    double memory_factor = std::min(1.0, memory_bandwidth_gb_per_sec / 500.0);

    return base_throughput * arch_factor * memory_factor * compute_utilization;
}

// KernelLaunchConfig Implementation
bool KernelLaunchConfig::validate() const {
    // Basic parameter validation
    if (grid.x == 0 || grid.y == 0 || grid.z == 0) return false;
    if (block.x == 0 || block.y == 0 || block.z == 0) return false;
    if (points_per_thread <= 0) return false;
    if (batch_size == 0) return false;

    // Check total threads
    std::uint64_t total_threads = getTotalThreads();
    if (total_threads > 1ULL << 30) return false;  // Reasonable upper limit

    // Validate against device constraints
    return resource_profile.isFeasible(device_constraints);
}

std::string KernelLaunchConfig::toString() const {
    std::ostringstream oss;
    oss << "KernelLaunchConfig{\n";
    oss << "  kernel_type: " << static_cast<int>(kernel_type) << "\n";
    oss << "  grid: (" << grid.x << ", " << grid.y << ", " << grid.z << ")\n";
    oss << "  block: (" << block.x << ", " << block.y << ", " << block.z << ")\n";
    oss << "  points_per_thread: " << points_per_thread << "\n";
    oss << "  batch_size: " << batch_size << "\n";
    oss << "  total_threads: " << getTotalThreads() << "\n";
    oss << "  registers_per_thread: " << resource_profile.registers_per_thread << "\n";
    oss << "  expected_occupancy: " << (resource_profile.expected_occupancy * 100) << "%\n";
    oss << "  theoretical_throughput: " << calculateTheoreticalThroughput() << " keys/s\n";
    oss << "}";
    return oss.str();
}

std::string KernelLaunchConfig::toJson() const {
    json j;
    j["kernel_type"] = static_cast<int>(kernel_type);
    j["grid"]["x"] = grid.x;
    j["grid"]["y"] = grid.y;
    j["grid"]["z"] = grid.z;
    j["block"]["x"] = block.x;
    j["block"]["y"] = block.y;
    j["block"]["z"] = block.z;
    j["points_per_thread"] = points_per_thread;
    j["batch_size"] = batch_size;
    j["objective"] = static_cast<int>(objective);
    j["use_separated_kernels"] = use_separated_kernels;
    j["use_pinned_memory"] = use_pinned_memory;
    j["enable_profiling"] = enable_profiling;
    j["enable_deterministic_mode"] = enable_deterministic_mode;
    j["deterministic_seed"] = deterministic_seed;
    j["memory_pool_size_mb"] = memory_pool_size_mb;
    j["resource_profile"]["registers_per_thread"] = resource_profile.registers_per_thread;
    j["resource_profile"]["shared_memory_per_block"] = resource_profile.shared_memory_per_block;
    j["resource_profile"]["threads_per_block"] = resource_profile.threads_per_block;
    j["resource_profile"]["expected_occupancy"] = resource_profile.expected_occupancy;
    return j.dump(2);
}

KernelLaunchConfig KernelLaunchConfig::fromJson(const std::string& json_str) {
    KernelLaunchConfig config;
    try {
        json j = json::parse(json_str);

        config.kernel_type = static_cast<KernelType>(j.value("kernel_type", 0));
        config.grid.x = j["grid"]["x"];
        config.grid.y = j["grid"]["y"];
        config.grid.z = j["grid"]["z"];
        config.block.x = j["block"]["x"];
        config.block.y = j["block"]["y"];
        config.block.z = j["block"]["z"];
        config.points_per_thread = j.value("points_per_thread", 1);
        config.batch_size = j.value("batch_size", 0);
        config.objective = static_cast<OptimizationObjective>(j.value("objective", 0));
        config.use_separated_kernels = j.value("use_separated_kernels", false);
        config.use_pinned_memory = j.value("use_pinned_memory", true);
        config.enable_profiling = j.value("enable_profiling", false);
        config.enable_deterministic_mode = j.value("enable_deterministic_mode", false);
        config.deterministic_seed = j.value("deterministic_seed", 0);
        config.memory_pool_size_mb = j.value("memory_pool_size_mb", 2048);

        if (j.contains("resource_profile")) {
            const auto& rp = j["resource_profile"];
            config.resource_profile.registers_per_thread = rp.value("registers_per_thread", 0);
            config.resource_profile.shared_memory_per_block = rp.value("shared_memory_per_block", 0);
            config.resource_profile.threads_per_block = rp.value("threads_per_block", 0);
            config.resource_profile.expected_occupancy = rp.value("expected_occupancy", 0.0);
        }
    } catch (const std::exception& e) {
        // Return default configuration on parse error
    }
    return config;
}

// LaunchConfigManager Implementation
LaunchConfigManager::LaunchConfigManager(int device_id) : device_id_(device_id) {
    initializeDeviceConstraints();
}

LaunchConfigManager::~LaunchConfigManager() = default;

void LaunchConfigManager::initializeDeviceConstraints() {
    cudaDeviceProp props;
    cudaError_t err = cudaGetDeviceProperties(&props, device_id_);
    if (err != cudaSuccess) {
        // Use conservative defaults if device query fails
        device_constraints_ = GpuConstraints{};
        return;
    }

    device_constraints_ = GpuConstraints{
        .max_registers_per_thread = props.maxThreadsPerMultiProcessor / 4,  // Rough estimate
        .max_shared_memory_per_block = static_cast<int>(props.sharedMemPerBlock),
        .max_threads_per_block = static_cast<int>(props.maxThreadsPerBlock),
        .max_threads_per_sm = static_cast<int>(props.maxThreadsPerMultiProcessor),
        .max_blocks_per_sm = props.maxBlocksPerMultiProcessor,
        .warp_size = props.warpSize,
        .total_global_memory = props.totalGlobalMem,
        .compute_capability_major = props.major,
        .compute_capability_minor = props.minor,
        .device_name = props.name
    };
}

KernelLaunchConfig LaunchConfigManager::getOptimalConfig(
    KernelType kernel_type,
    std::uint64_t batch_size,
    OptimizationObjective objective
) {
    KernelLaunchConfig config{};
    config.kernel_type = kernel_type;
    config.batch_size = batch_size;
    config.objective = objective;
    config.device_constraints = device_constraints_;

    // Set kernel-specific parameters
    switch (kernel_type) {
        case KernelType::SEPARATED_ECC:
            config.resource_profile.registers_per_thread = 32;
            config.use_separated_kernels = true;
            break;
        case KernelType::SEPARATED_HASH:
            config.resource_profile.registers_per_thread = 40;
            config.use_separated_kernels = true;
            break;
        case KernelType::SEPARATED_COMPARE:
            config.resource_profile.registers_per_thread = 24;
            config.use_separated_kernels = true;
            break;
        case KernelType::SEPARATED_PIPELINE:
            config.resource_profile.registers_per_thread = 48;  // Combined
            config.use_separated_kernels = true;
            break;
        case KernelType::MEMORY_OPTIMIZED:
            config.resource_profile.registers_per_thread = 64;
            config.use_shared_memory_optimization = true;
            break;
        default:
            config.resource_profile.registers_per_thread = 80;
            config.use_separated_kernels = false;
            break;
    }

    // Calculate optimal block size
    config.block = device_constraints_.getOptimalBlockSize(kernel_type);
    config.resource_profile.threads_per_block = config.block.x * config.block.y * config.block.z;

    // Calculate points per thread based on batch size and objective
    switch (objective) {
        case OptimizationObjective::MAXIMIZE_THROUGHPUT:
            config.points_per_thread = std::min(1024, static_cast<int>(batch_size / 1000000));
            break;
        case OptimizationObjective::MINIMIZE_LATENCY:
            config.points_per_thread = 1;
            break;
        case OptimizationObjective::BALANCE_PERFORMANCE:
            config.points_per_thread = std::min(256, static_cast<int>(batch_size / 100000));
            break;
        case OptimizationObjective::MINIMIZE_MEMORY:
            config.points_per_thread = 64;
            break;
        case OptimizationObjective::DETERMINISTIC_REPLAY:
            config.points_per_thread = 1;
            config.enable_deterministic_mode = true;
            break;
    }

    config.points_per_thread = std::max(1, config.points_per_thread);

    // Calculate grid size based on batch size and threads
    std::uint64_t total_threads_needed = (batch_size + config.points_per_thread - 1) / config.points_per_thread;
    std::uint64_t threads_per_block = config.block.x * config.block.y * config.block.z;
    std::uint64_t blocks_needed = (total_threads_needed + threads_per_block - 1) / threads_per_block;

    // Distribute blocks across grid dimensions
    config.grid.x = std::min(static_cast<std::uint32_t>(blocks_needed), 65535u);
    config.grid.y = 1;
    config.grid.z = 1;

    // If we need more than 65535 blocks in x, use y dimension
    if (blocks_needed > 65535) {
        config.grid.x = 65535;
        config.grid.y = std::min(static_cast<std::uint32_t>((blocks_needed + 65534) / 65535), 65535u);
    }

    // Estimate resource usage
    config.resource_profile = estimateResourceUsage(config);

    // Set advanced options based on objective
    config.use_pinned_memory = (objective != OptimizationObjective::MINIMIZE_MEMORY);
    config.enable_memory_defragmentation = (objective == OptimizationObjective::MAXIMIZE_THROUGHPUT);
    config.enable_adaptive_batching = (objective == OptimizationObjective::BALANCE_PERFORMANCE);

    return findOrCreateConfig(config);
}

KernelLaunchConfig LaunchConfigManager::getSeparatedKernelConfig(
    std::uint64_t batch_size,
    OptimizationObjective objective
) {
    KernelLaunchConfig config = getOptimalConfig(KernelType::SEPARATED_PIPELINE, batch_size, objective);
    config.enable_warp_operations = true;
    config.enable_shared_memory_optimization = true;
    return config;
}

KernelLaunchConfig LaunchConfigManager::getDeterministicConfig(
    const KernelLaunchConfig& base_config,
    std::uint64_t seed
) {
    KernelLaunchConfig config = base_config;
    config.enable_deterministic_mode = true;
    config.deterministic_seed = seed;
    config.objective = OptimizationObjective::DETERMINISTIC_REPLAY;

    // Use conservative settings for determinism
    config.points_per_thread = 1;
    config.enable_adaptive_batching = false;
    config.enable_memory_defragmentation = false;

    return config;
}

bool LaunchConfigManager::validateConfig(const KernelLaunchConfig& config) const {
    return config.validate() && config.resource_profile.isFeasible(device_constraints_);
}

KernelLaunchConfig LaunchConfigManager::optimizeFromPerformance(
    const KernelLaunchConfig& config,
    double actual_throughput,
    double actual_occupancy
) {
    KernelLaunchConfig optimized = config;

    // Adjust configuration based on performance feedback
    if (actual_occupancy < 0.5 && config.resource_profile.registers_per_thread > 32) {
        // Low occupancy, try reducing register usage
        optimized.resource_profile.registers_per_thread -= 8;
    }

    if (actual_throughput < config.calculateTheoreticalThroughput() * 0.7) {
        // Performance is significantly below theoretical, try different block size
        if (optimized.block.x > 128) {
            optimized.block.x /= 2;
        } else if (optimized.block.x < 512) {
            optimized.block.x *= 2;
        }
    }

    // Re-calculate derived parameters
    optimized.resource_profile.threads_per_block = optimized.block.x * optimized.block.y * optimized.block.z;

    return optimized;
}

void LaunchConfigManager::exportConfigurationCache(const std::string& filename) const {
    json cache_json;
    cache_json["device_id"] = device_id_;
    cache_json["device_name"] = device_constraints_.device_name;
    cache_json["configurations"] = json::array();

    for (const auto& config : config_cache_) {
        cache_json["configurations"].push_back(json::parse(config.toJson()));
    }

    std::ofstream file(filename);
    file << cache_json.dump(2);
}

void LaunchConfigManager::importConfigurationCache(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    try {
        json cache_json;
        file >> cache_json;

        if (cache_json["device_id"] != device_id_) return;

        config_cache_.clear();
        for (const auto& config_json : cache_json["configurations"]) {
            config_cache_.push_back(KernelLaunchConfig::fromJson(config_json.dump()));
        }
    } catch (const std::exception&) {
        // Ignore import errors
    }
}

dim3 LaunchConfigManager::calculateOptimalGrid(const KernelLaunchConfig& base_config) const {
    std::uint64_t total_threads_needed = (base_config.batch_size + base_config.points_per_thread - 1) / base_config.points_per_thread;
    std::uint64_t threads_per_block = base_config.block.x * base_config.block.y * base_config.block.z;
    std::uint64_t blocks_needed = (total_threads_needed + threads_per_block - 1) / threads_per_block;

    dim3 grid{};
    grid.x = std::min(static_cast<std::uint32_t>(blocks_needed), 65535u);
    grid.y = 1;
    grid.z = 1;

    if (blocks_needed > 65535) {
        grid.x = 65535;
        grid.y = std::min(static_cast<std::uint32_t>((blocks_needed + 65534) / 65535), 65535u);
    }

    return grid;
}

dim3 LaunchConfigManager::calculateOptimalBlock(KernelType kernel_type, const GpuConstraints& constraints) const {
    return constraints.getOptimalBlockSize(kernel_type);
}

ResourceProfile LaunchConfigManager::estimateResourceUsage(const KernelLaunchConfig& config) const {
    ResourceProfile profile = config.resource_profile;

    // Estimate occupancy based on resource usage
    double register_pressure = static_cast<double>(profile.registers_per_thread) / device_constraints_.max_registers_per_thread;
    double shared_memory_pressure = static_cast<double>(profile.shared_memory_per_block) / device_constraints_.max_shared_memory_per_block;
    double thread_pressure = static_cast<double>(profile.threads_per_block) / device_constraints_.max_threads_per_block;

    // Calculate expected occupancy (simplified model)
    profile.expected_occupancy = 1.0 / std::max({register_pressure, shared_memory_pressure, thread_pressure, 0.1});
    profile.expected_occupancy = std::min(profile.expected_occupancy, 1.0);

    // Estimate memory bandwidth based on kernel type
    switch (config.kernel_type) {
        case KernelType::SEPARATED_ECC:
            profile.memory_bandwidth_gb_per_sec = 200;  // ECC operations
            break;
        case KernelType::SEPARATED_HASH:
            profile.memory_bandwidth_gb_per_sec = 400;  // Hash operations
            break;
        case KernelType::SEPARATED_COMPARE:
            profile.memory_bandwidth_gb_per_sec = 100;  // Compare operations
            break;
        case KernelType::SEPARATED_PIPELINE:
            profile.memory_bandwidth_gb_per_sec = 300;  // Pipeline
            break;
        default:
            profile.memory_bandwidth_gb_per_sec = 250;  // Default
            break;
    }

    // Architecture-specific adjustments
    if (device_constraints_.compute_capability_major >= 8) {
        profile.memory_bandwidth_gb_per_sec *= 1.5;  // Ampere bandwidth boost
    }

    // Compute utilization estimate
    profile.compute_utilization = 0.8;  // Default 80% utilization

    return profile;
}

KernelLaunchConfig LaunchConfigManager::findOrCreateConfig(const KernelLaunchConfig& template_config) {
    // Simple caching: look for identical configuration
    for (const auto& cached : config_cache_) {
        if (cached.kernel_type == template_config.kernel_type &&
            cached.batch_size == template_config.batch_size &&
            cached.objective == template_config.objective) {
            return cached;
        }
    }

    // Add to cache and return
    config_cache_.push_back(template_config);
    return template_config;
}

// Launch Utils Implementation
namespace launch_utils {

CudaLaunchParams toCudaParams(const KernelLaunchConfig& config, cudaStream_t stream) {
    return CudaLaunchParams{
        .grid = config.grid,
        .block = config.block,
        .shared_memory_size = static_cast<size_t>(config.resource_profile.shared_memory_per_block),
        .stream = stream
    };
}

MemoryRequirements calculateMemoryRequirements(const KernelLaunchConfig& config) {
    MemoryRequirements req{};

    // Device memory for candidates and results
    req.device_memory_bytes = config.batch_size * sizeof(DeviceCandidate);

    // Pinned memory for host-device transfers
    if (config.use_pinned_memory) {
        req.pinned_memory_bytes = req.device_memory_bytes;
    }

    // Unified memory (if enabled)
    if (config.use_unified_memory) {
        req.unified_memory_bytes = req.device_memory_bytes;
    }

    // Shared memory per block
    req.shared_memory_bytes = static_cast<size_t>(config.resource_profile.shared_memory_per_block);

    return req;
}

bool validateLaunchParams(const CudaLaunchParams& params, const GpuConstraints& constraints) {
    return (params.block.x * params.block.y * params.block.z) <= constraints.max_threads_per_block &&
           params.shared_memory_size <= constraints.max_shared_memory_per_block;
}

std::uint64_t generateConfigFingerprint(const KernelLaunchConfig& config) {
    // Simple hash of key configuration parameters
    std::uint64_t hash = 0;
    hash ^= static_cast<std::uint64_t>(config.kernel_type);
    hash ^= (static_cast<std::uint64_t>(config.grid.x) << 32) | config.grid.y;
    hash ^= (static_cast<std::uint64_t>(config.block.x) << 32) | config.block.y;
    hash ^= static_cast<std::uint64_t>(config.points_per_thread);
    hash ^= config.batch_size;
    hash ^= static_cast<std::uint64_t>(config.objective);
    return hash;
}

} // namespace launch_utils

} // namespace puzzle71::gpu
// Puzzle71Solver - GPU Architecture Compatibility Matrix (T054)
// Phase 7: User Story 5 - Compatibility Assurance
// Comprehensive GPU architecture compatibility matrix implementation

#include "arch_compatibility.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

namespace puzzle71::gpu {

// ============================================================================
// ComputeCapability Implementation
// ============================================================================

std::string ComputeCapability::toString() const {
    return std::to_string(major) + "." + std::to_string(minor);
}

int ComputeCapability::toInteger() const {
    return major * 10 + minor;
}

int ComputeCapability::compare(const ComputeCapability& other) const {
    if (major != other.major) return (major > other.major) ? 1 : -1;
    if (minor != other.minor) return (minor > other.minor) ? 1 : -1;
    return 0;
}

bool ComputeCapability::meetsMinimum(const ComputeCapability& minimum) const {
    return compare(minimum) >= 0;
}

// ============================================================================
// GPUDeviceInfo Implementation
// ============================================================================

bool GPUDeviceInfo::supportsFeature(const std::string& feature) const {
    // Map feature names to capabilities
    static const std::map<std::string, bool (GPUDeviceInfo::*) const> feature_map = {
        {"tensor_cores", &GPUDeviceInfo::supportsTensorCores},
        {"ray_tracing", &GPUDeviceInfo::supportsRayTracing},
        {"bf16", &GPUDeviceInfo::supportsBf16},
        {"tf32", &GPUDeviceInfo::supportsTf32},
        {"fp8", &GPUDeviceInfo::supportsFp8},
        {"managed_memory", &GPUDeviceInfo::supportsManagedMemory},
        {"concurrent_kernels", &GPUDeviceInfo::supportsConcurrentKernels},
        {"ecc", &GPUDeviceInfo::supportsEcc},
        {"integrated", &GPUDeviceInfo::integrated}
    };

    auto it = feature_map.find(feature);
    if (it != feature_map.end()) {
        return (this->*it->second)();
    }

    // Check for compute capability-based features
    if (feature == "cooperative_groups" && compute_capability.meetsMinimum({6, 0})) {
        return true;
    }
    if (feature == "dynamic_parallelism" && compute_capability.meetsMinimum({3, 5})) {
        return true;
    }
    if (feature == "warp_specialization" && compute_capability.meetsMinimum({7, 0})) {
        return true;
    }

    return false;
}

std::string GPUDeviceInfo::getArchitectureString() const {
    return GPUArchitectureCompatibilityMatrix::architectureToString(architecture);
}

bool GPUDeviceInfo::isCompatible(const ComputeCapability& minimum_capability) const {
    return compute_capability.meetsMinimum(minimum_capability);
}

std::string GPUDeviceInfo::toJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"device_id\":" << device_id << ","
        << "\"name\":\"" << name << "\","
        << "\"architecture\":\"" << getArchitectureString() << "\","
        << "\"compute_capability\":\"" << compute_capability.toString() << "\","
        << "\"total_memory\":" << total_memory << ","
        << "\"shared_memory_per_block\":" << shared_memory_per_block << ","
        << "\"max_threads_per_block\":" << max_threads_per_block << ","
        << "\"max_registers_per_thread\":" << max_registers_per_thread << ","
        << "\"multiprocessor_count\":" << multiprocessor_count << ","
        << "\"supports_tensor_cores\":" << (supports_tensor_cores ? "true" : "false") << ","
        << "\"supports_ray_tracing\":" << (supports_ray_tracing ? "true" : "false") << ","
        << "\"memory_bandwidth\":" << memory_bandwidth
        << "}";
    return oss.str();
}

std::string GPUDeviceInfo::getSummary() const {
    std::ostringstream oss;
    oss << name << " (" << getArchitectureString() << ", SM " << compute_capability.toString() << ")\n";
    oss << "  Memory: " << (total_memory / (1024 * 1024)) << " MB\n";
    oss << "  SMs: " << multiprocessor_count << "\n";
    oss << "  Max Threads/Block: " << max_threads_per_block << "\n";
    oss << "  Max Registers/Thread: " << max_registers_per_thread << "\n";
    oss << "  Features: ";

    std::vector<std::string> features;
    if (supports_tensor_cores) features.push_back("Tensor");
    if (supports_ray_tracing) features.push_back("RT");
    if (supports_bf16) features.push_back("BF16");
    if (supports_tf32) features.push_back("TF32");

    if (features.empty()) {
        oss << "None";
    } else {
        for (size_t i = 0; i < features.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << features[i];
        }
    }

    return oss.str();
}

// ============================================================================
// KernelRequirements Implementation
// ============================================================================

bool KernelRequirements::isDeviceCompatible(const GPUDeviceInfo& device) const {
    // Check compute capability
    if (!device.compute_capability.meetsMinimum(min_compute_capability)) {
        return false;
    }

    // Check memory requirements
    if (device.shared_memory_per_block < min_shared_memory) {
        return false;
    }

    // Check thread requirements
    if (device.max_threads_per_block < min_threads_per_block) {
        return false;
    }

    // Check register requirements
    if (device.max_registers_per_thread < min_registers_per_thread) {
        return false;
    }

    // Check feature requirements
    if (requires_tensor_cores && !device.supports_tensor_cores) {
        return false;
    }

    if (requires_ray_tracing && !device.supports_ray_tracing) {
        return false;
    }

    if (requires_fp16 && !device.supportsFeature("fp16")) {
        return false;
    }

    if (requires_bf16 && !device.supports_bf16) {
        return false;
    }

    if (requires_tf32 && !device.supports_tf32) {
        return false;
    }

    if (requires_cooperative_groups && !device.supportsFeature("cooperative_groups")) {
        return false;
    }

    if (requires_dynamic_parallelism && !device.supportsFeature("dynamic_parallelism")) {
        return false;
    }

    if (requires_managed_memory && !device.supports_managed_memory) {
        return false;
    }

    return true;
}

std::vector<std::string> KernelRequirements::getMissingRequirements(const GPUDeviceInfo& device) const {
    std::vector<std::string> missing;

    if (!device.compute_capability.meetsMinimum(min_compute_capability)) {
        missing.push_back("Compute capability " + min_compute_capability.toString());
    }

    if (device.shared_memory_per_block < min_shared_memory) {
        missing.push_back("Shared memory: " + std::to_string(min_shared_memory) + " bytes");
    }

    if (device.max_threads_per_block < min_threads_per_block) {
        missing.push_back("Threads per block: " + std::to_string(min_threads_per_block));
    }

    if (device.max_registers_per_thread < min_registers_per_thread) {
        missing.push_back("Registers per thread: " + std::to_string(min_registers_per_thread));
    }

    if (requires_tensor_cores && !device.supports_tensor_cores) {
        missing.push_back("Tensor cores");
    }

    if (requires_ray_tracing && !device.supports_ray_tracing) {
        missing.push_back("Ray tracing cores");
    }

    if (requires_bf16 && !device.supports_bf16) {
        missing.push_back("BF16 support");
    }

    if (requires_tf32 && !device.supports_tf32) {
        missing.push_back("TF32 support");
    }

    return missing;
}

std::string KernelRequirements::toJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"min_compute_capability\":\"" << min_compute_capability.toString() << "\","
        << "\"min_shared_memory\":" << min_shared_memory << ","
        << "\"min_registers_per_thread\":" << min_registers_per_thread << ","
        << "\"min_threads_per_block\":" << min_threads_per_block << ","
        << "\"requires_tensor_cores\":" << (requires_tensor_cores ? "true" : "false") << ","
        << "\"requires_ray_tracing\":" << (requires_ray_tracing ? "true" : "false") << ","
        << "\"requires_bf16\":" << (requires_bf16 ? "true" : "false") << ","
        << "\"requires_tf32\":" << (requires_tf32 ? "true" : "false")
        << "}";
    return oss.str();
}

// ============================================================================
// ArchitecturePerformance Implementation
// ============================================================================

ArchitecturePerformance::OptimalConfig ArchitecturePerformance::getOptimalConfig(const std::string& kernel_name) const {
    OptimalConfig config;

    // Find optimal configuration for this kernel
    auto block_it = optimal_block_sizes.find(kernel_name);
    if (block_it != optimal_block_sizes.end()) {
        config.threads_per_block = block_it->second;
    }

    auto grid_it = optimal_grid_sizes.find(kernel_name);
    if (grid_it != optimal_grid_sizes.end()) {
        config.blocks_per_grid = grid_it->second;
    }

    auto batch_it = optimal_batch_sizes.find(kernel_name);
    if (batch_it != optimal_batch_sizes.end()) {
        // Convert batch size to shared memory if needed
        config.shared_memory_size = batch_it->second * sizeof(uint32_t);
    }

    // Set feature flags based on architecture capabilities
    config.use_tensor_cores = supports_tensor_cores;
    config.use_cooperative_groups = supports_cooperative_launch;

    return config;
}

bool ArchitecturePerformance::isKernelSupported(const std::string& kernel_name) const {
    return std::find(supported_kernels.begin(), supported_kernels.end(), kernel_name) != supported_kernels.end();
}

// ============================================================================
// CompatibilityEntry Implementation
// ============================================================================

bool CompatibilityEntry::isKernelCompatible(const std::string& kernel_name) const {
    // Check if kernel is in compatible list
    if (std::find(compatible_kernels.begin(), compatible_kernels.end(), kernel_name) != compatible_kernels.end()) {
        return true;
    }

    // Check if kernel is in incompatible list
    if (std::find(incompatible_kernels.begin(), incompatible_kernels.end(), kernel_name) != incompatible_kernels.end()) {
        return false;
    }

    // Default to compatible if not explicitly listed
    return true;
}

std::string CompatibilityEntry::getAlternativeKernel(const std::string& kernel_name) const {
    auto it = kernel_alternatives.find(kernel_name);
    return it != kernel_alternatives.end() ? it->second : "";
}

std::vector<std::string> CompatibilityEntry::getWorkarounds(const std::string& kernel_name) const {
    std::vector<std::string> kernel_workarounds;

    // Add general workarounds
    kernel_workarounds.insert(kernel_workarounds.end(), required_workarounds.begin(), required_workarounds.end());

    // TODO: Add kernel-specific workarounds if needed

    return kernel_workarounds;
}

// ============================================================================
// GPUArchitectureCompatibilityMatrix Implementation
// ============================================================================

GPUArchitectureCompatibilityMatrix::GPUArchitectureCompatibilityMatrix() {
    initialize();
}

bool GPUArchitectureCompatibilityMatrix::initialize() {
    try {
        initializeDeviceCache();
        initializeCompatibilityMatrix();
        initializePerformanceMatrix();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize compatibility matrix: " << e.what() << std::endl;
        return false;
    }
}

std::vector<GPUDeviceInfo> GPUArchitectureCompatibilityMatrix::getAvailableDevices() const {
    std::lock_guard<std::mutex> lock(matrix_mutex_);

    if (!devices_initialized_) {
        const_cast<GPUArchitectureCompatibilityMatrix*>(this)->initializeDeviceCache();
    }

    std::vector<GPUDeviceInfo> devices;
    for (const auto& [id, info] : device_info_cache_) {
        devices.push_back(info);
    }
    return devices;
}

GPUDeviceInfo GPUArchitectureCompatibilityMatrix::getDeviceInfo(int device_id) const {
    std::lock_guard<std::mutex> lock(matrix_mutex_);

    if (!devices_initialized_) {
        const_cast<GPUArchitectureCompatibilityMatrix*>(this)->initializeDeviceCache();
    }

    auto it = device_info_cache_.find(device_id);
    return it != device_info_cache_.end() ? it->second : GPUDeviceInfo{};
}

bool GPUArchitectureCompatibilityMatrix::isKernelCompatible(int device_id, const std::string& kernel_name) const {
    auto device = getDeviceInfo(device_id);
    return isKernelCompatible(device, kernel_name);
}

bool GPUArchitectureCompatibilityMatrix::isKernelCompatible(const GPUDeviceInfo& device, const std::string& kernel_name) const {
    auto arch_it = compatibility_matrix_.find(device.architecture);
    if (arch_it == compatibility_matrix_.end()) {
        return false; // Unknown architecture
    }

    return arch_it->second.isKernelCompatible(kernel_name);
}

bool GPUArchitectureCompatibilityMatrix::deviceMeetsRequirements(int device_id, const KernelRequirements& requirements) const {
    auto device = getDeviceInfo(device_id);
    return deviceMeetsRequirements(device, requirements);
}

bool GPUArchitectureCompatibilityMatrix::deviceMeetsRequirements(const GPUDeviceInfo& device, const KernelRequirements& requirements) const {
    return requirements.isDeviceCompatible(device);
}

std::string GPUArchitectureCompatibilityMatrix::getAlternativeKernel(int device_id, const std::string& kernel_name) const {
    auto device = getDeviceInfo(device_id);
    return getAlternativeKernel(device, kernel_name);
}

std::string GPUArchitectureCompatibilityMatrix::getAlternativeKernel(const GPUDeviceInfo& device, const std::string& kernel_name) const {
    auto arch_it = compatibility_matrix_.find(device.architecture);
    if (arch_it == compatibility_matrix_.end()) {
        return "";
    }

    return arch_it->second.getAlternativeKernel(kernel_name);
}

ArchitecturePerformance::OptimalConfig GPUArchitectureCompatibilityMatrix::getOptimalConfiguration(
    int device_id, const std::string& kernel_name
) const {
    auto device = getDeviceInfo(device_id);
    return getOptimalConfiguration(device, kernel_name);
}

ArchitecturePerformance::OptimalConfig GPUArchitectureCompatibilityMatrix::getOptimalConfiguration(
    const GPUDeviceInfo& device, const std::string& kernel_name
) const {
    auto perf_it = performance_matrix_.find(device.architecture);
    if (perf_it == performance_matrix_.end()) {
        return ArchitecturePerformance::OptimalConfig{}; // Default config
    }

    return perf_it->second.getOptimalConfig(kernel_name);
}

CompatibilityEntry GPUArchitectureCompatibilityMatrix::getCompatibilityEntry(GPUArchitecture architecture) const {
    auto it = compatibility_matrix_.find(architecture);
    return it != compatibility_matrix_.end() ? it->second : CompatibilityEntry{};
}

ArchitecturePerformance GPUArchitectureCompatibilityMatrix::getPerformanceCharacteristics(GPUArchitecture architecture) const {
    auto it = performance_matrix_.find(architecture);
    return it != performance_matrix_.end() ? it->second : ArchitecturePerformance{};
}

std::vector<std::string> GPUArchitectureCompatibilityMatrix::getWorkarounds(int device_id, const std::string& kernel_name) const {
    auto device = getDeviceInfo(device_id);
    auto entry = getCompatibilityEntry(device.architecture);
    return entry.getWorkarounds(kernel_name);
}

std::vector<std::string> GPUArchitectureCompatibilityMatrix::getKnownIssues(int device_id, const std::string& kernel_name) const {
    auto device = getDeviceInfo(device_id);
    auto entry = getCompatibilityEntry(device.architecture);

    // Filter known issues for this kernel if needed
    std::vector<std::string> issues = entry.known_issues;

    // TODO: Add kernel-specific issue filtering

    return issues;
}

std::vector<std::string> GPUArchitectureCompatibilityMatrix::getOptimizationHints(int device_id, const std::string& kernel_name) const {
    auto device = getDeviceInfo(device_id);
    auto entry = getCompatibilityEntry(device.architecture);

    std::vector<std::string> hints = entry.optimization_hints;

    // Add architecture-specific optimization hints
    auto perf = getPerformanceCharacteristics(device.architecture);

    if (perf.supports_tensor_cores && kernel_name.find("tensor") != std::string::npos) {
        hints.push_back("Enable Tensor Core usage for optimal performance");
    }

    if (perf.supports_cooperative_groups && kernel_name.find("reduce") != std::string::npos) {
        hints.push_back("Consider using Cooperative Groups for reduction operations");
    }

    return hints;
}

GPUArchitectureCompatibilityMatrix::ValidationReport GPUArchitectureCompatibilityMatrix::validateDeviceCompatibility(
    int device_id, const std::vector<std::string>& required_kernels
) const {
    auto device = getDeviceInfo(device_id);
    return validateDeviceCompatibility(device, required_kernels);
}

GPUArchitectureCompatibilityMatrix::ValidationReport GPUArchitectureCompatibilityMatrix::validateDeviceCompatibility(
    const GPUDeviceInfo& device, const std::vector<std::string>& required_kernels
) const {
    ValidationReport report;

    // Check basic device compatibility
    if (device.compute_capability.toInteger() < 75) { // Minimum Turing
        report.is_compatible = false;
        report.errors.push_back("GPU architecture too old (requires Turing or newer)");
    }

    // Check kernel compatibility
    for (const auto& kernel_name : required_kernels) {
        bool compatible = isKernelCompatible(device, kernel_name);
        report.kernel_compatibility[kernel_name] = compatible;

        if (!compatible) {
            report.warnings.push_back("Kernel '" + kernel_name + "' is not compatible with " + device.name);

            // Suggest alternative
            std::string alternative = getAlternativeKernel(device, kernel_name);
            if (!alternative.empty()) {
                report.recommendations.push_back("Use alternative kernel '" + alternative + "' instead of '" + kernel_name + "'");
            }
        }
    }

    // Check memory requirements
    if (device.total_memory < 1024 * 1024 * 1024) { // Less than 1GB
        report.warnings.push_back("Low memory configuration may limit performance");
    }

    // Add recommendations based on device characteristics
    if (device.supports_tensor_cores && std::find_if(required_kernels.begin(), required_kernels.end(),
        [](const std::string& k) { return k.find("ecc") != std::string::npos; }) != required_kernels.end()) {
        report.recommendations.push_back("Consider using Tensor Core-optimized kernels for ECC operations");
    }

    return report;
}

std::string GPUArchitectureCompatibilityMatrix::generateSystemReport() const {
    std::ostringstream oss;
    oss << "GPU Architecture Compatibility Report\n";
    oss << "===================================\n\n";

    auto devices = getAvailableDevices();
    oss << "Available Devices: " << devices.size() << "\n\n";

    for (const auto& device : devices) {
        oss << "Device " << device.device_id << ": " << device.name << "\n";
        oss << "  Architecture: " << device.getArchitectureString() << "\n";
        oss << "  Compute Capability: " << device.compute_capability.toString() << "\n";
        oss << "  Memory: " << (device.total_memory / (1024 * 1024)) << " MB\n";
        oss << "  SMs: " << device.multiprocessor_count << "\n";

        // Check compatibility with common kernels
        std::vector<std::string> test_kernels = {"ecc_kernel", "hash_kernel", "compare_kernel"};
        auto report = validateDeviceCompatibility(device, test_kernels);

        if (report.is_compatible) {
            oss << "  Status: COMPATIBLE\n";
        } else {
            oss << "  Status: NOT COMPATIBLE\n";
        }

        if (!report.warnings.empty()) {
            oss << "  Warnings:\n";
            for (const auto& warning : report.warnings) {
                oss << "    - " << warning << "\n";
            }
        }

        oss << "\n";
    }

    return oss.str();
}

std::string GPUArchitectureCompatibilityMatrix::generateDeviceReport(int device_id) const {
    auto device = getDeviceInfo(device_id);
    std::ostringstream oss;

    oss << "Device Compatibility Report\n";
    oss << "==========================\n\n";
    oss << device.getSummary() << "\n";

    auto perf = getPerformanceCharacteristics(device.architecture);
    oss << "Performance Characteristics:\n";
    oss << "  Memory Bandwidth Efficiency: " << (perf.memory_bandwidth_efficiency * 100) << "%\n";
    oss << "  Compute Efficiency: " << (perf.compute_efficiency * 100) << "%\n";
    oss << "  Target Occupancy: " << (perf.occupancy_target * 100) << "%\n";

    if (!perf.supported_kernels.empty()) {
        oss << "  Supported Kernels: ";
        for (size_t i = 0; i < perf.supported_kernels.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << perf.supported_kernels[i];
        }
        oss << "\n";
    }

    return oss.str();
}

bool GPUArchitectureCompatibilityMatrix::checkSystemCompatibility(const KernelRequirements& requirements) const {
    auto devices = getAvailableDevices();
    return !devices.empty() && std::any_of(devices.begin(), devices.end(),
        [&](const GPUDeviceInfo& device) { return deviceMeetsRequirements(device, requirements); });
}

std::vector<int> GPUArchitectureCompatibilityMatrix::getCompatibleDevices(const KernelRequirements& requirements) const {
    auto devices = getAvailableDevices();
    std::vector<int> compatible_devices;

    for (const auto& device : devices) {
        if (deviceMeetsRequirements(device, requirements)) {
            compatible_devices.push_back(device.device_id);
        }
    }

    return compatible_devices;
}

GPUArchitecture GPUArchitectureCompatibilityMatrix::detectArchitecture(int device_id) const {
    auto device = getDeviceInfo(device_id);
    return device.architecture;
}

GPUArchitecture GPUArchitectureCompatibilityMatrix::detectArchitecture(const ComputeCapability& compute_capability) const {
    return utils::computeCapabilityToArchitecture(compute_capability);
}

GPUArchitecture GPUArchitectureCompatibilityMatrix::detectArchitecture(const std::string& device_name) const {
    // Simple detection based on device name patterns
    if (device_name.find("RTX 40") != std::string::npos) return GPUArchitecture::ADA_LOVELACE;
    if (device_name.find("RTX 30") != std::string::npos) return GPUArchitecture::AMPERE;
    if (device_name.find("RTX 20") != std::string::npos || device_name.find("GTX 16") != std::string::npos) return GPUArchitecture::TURING;
    if (device_name.find("GTX 10") != std::string::npos) return GPUArchitecture::PASCAL;
    if (device_name.find("Tesla V100") != std::string::npos) return GPUArchitecture::VOLTA;
    if (device_name.find("H100") != std::string::npos || device_name.find("H200") != std::string::npos) return GPUArchitecture::HOPPER;

    return GPUArchitecture::UNKNOWN;
}

std::string GPUArchitectureCompatibilityMatrix::architectureToString(GPUArchitecture architecture) {
    switch (architecture) {
        case GPUArchitecture::PASCAL: return "Pascal";
        case GPUArchitecture::VOLTA: return "Volta";
        case GPUArchitecture::TURING: return "Turing";
        case GPUArchitecture::AMPERE: return "Ampere";
        case GPUArchitecture::ADA_LOVELACE: return "Ada Lovelace";
        case GPUArchitecture::HOPPER: return "Hopper";
        case GPUArchitecture::BLACKWELL: return "Blackwell";
        default: return "Unknown";
    }
}

GPUArchitecture GPUArchitectureCompatibilityMatrix::stringToArchitecture(const std::string& architecture_str) {
    std::string lower_str = architecture_str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);

    if (lower_str == "pascal") return GPUArchitecture::PASCAL;
    if (lower_str == "volta") return GPUArchitecture::VOLTA;
    if (lower_str == "turing") return GPUArchitecture::TURING;
    if (lower_str == "ampere") return GPUArchitecture::AMPERE;
    if (lower_str == "ada lovelace" || lower_str == "ada") return GPUArchitecture::ADA_LOVELACE;
    if (lower_str == "hopper") return GPUArchitecture::HOPPER;
    if (lower_str == "blackwell") return GPUArchitecture::BLACKWELL;

    return GPUArchitecture::UNKNOWN;
}

ComputeCapability GPUArchitectureCompatibilityMatrix::getComputeCapability(int device_id) {
    cudaDeviceProp prop;
    if (cudaGetDeviceProperties(&prop, device_id) == cudaSuccess) {
        return {prop.major, prop.minor};
    }
    return {0, 0};
}

void GPUArchitectureCompatibilityMatrix::initializeDeviceCache() {
    int device_count = 0;
    if (cudaGetDeviceCount(&device_count) != cudaSuccess) {
        return;
    }

    for (int i = 0; i < device_count; ++i) {
        auto device_info = queryDeviceInfo(i);
        device_info_cache_[i] = device_info;
    }

    devices_initialized_ = true;
}

void GPUArchitectureCompatibilityMatrix::initializeCompatibilityMatrix() {
    // Initialize Turing compatibility
    CompatibilityEntry turing_entry;
    turing_entry.architecture = GPUArchitecture::TURING;
    turing_entry.compatible_kernels = {"ecc_kernel", "hash_kernel", "compare_kernel", "ecc_tensor"};
    turing_entry.incompatible_kernels = {"fp8_kernel"};
    turing_entry.required_workarounds = {"Enable legacy warp shuffle compatibility"};
    compatibility_matrix_[GPUArchitecture::TURING] = turing_entry;

    // Initialize Ampere compatibility
    CompatibilityEntry ampere_entry;
    ampere_entry.architecture = GPUArchitecture::AMPERE;
    ampere_entry.compatible_kernels = {"ecc_kernel", "hash_kernel", "compare_kernel", "ecc_tensor", "ecc_bf16"};
    ampere_entry.required_workarounds = {"Use bfloat16 for better performance"};
    compatibility_matrix_[GPUArchitecture::AMPERE] = ampere_entry;

    // Initialize Ada Lovelace compatibility
    CompatibilityEntry ada_entry;
    ada_entry.architecture = GPUArchitecture::ADA_LOVELACE;
    ada_entry.compatible_kernels = {"ecc_kernel", "hash_kernel", "compare_kernel", "ecc_tensor", "ecc_bf16"};
    ada_entry.optimization_hints = {"Enable L2 cache persistence", "Use Tensor Memory Acceleration"};
    compatibility_matrix_[GPUArchitecture::ADA_LOVELACE] = ada_entry;

    // Initialize Hopper compatibility
    CompatibilityEntry hopper_entry;
    hopper_entry.architecture = GPUArchitecture::HOPPER;
    hopper_entry.compatible_kernels = {"ecc_kernel", "hash_kernel", "compare_kernel", "ecc_tensor", "ecc_bf16", "ecc_fp8"};
    hopper_entry.optimization_hints = {"Enable Transformer Engine", "Use FP8 where possible"};
    compatibility_matrix_[GPUArchitecture::HOPPER] = hopper_entry;
}

void GPUArchitectureCompatibilityMatrix::initializePerformanceMatrix() {
    // Initialize Turing performance characteristics
    ArchitecturePerformance turing_perf;
    turing_perf.architecture = GPUArchitecture::TURING;
    turing_perf.supported_kernels = {"ecc_kernel", "hash_kernel", "compare_kernel"};
    turing_perf.supports_tensor_cores = true;
    turing_perf.supports_cooperative_launch = true;
    turing_perf.supports_memory_pool = false;
    turing_perf.memory_bandwidth_efficiency = 0.85;
    turing_perf.compute_efficiency = 0.75;
    turing_perf.occupancy_target = 0.80;
    turing_perf.max_concurrent_kernels = 1;
    turing_perf.max_batch_size = 10000;
    turing_perf.optimal_block_sizes["ecc_kernel"] = 256;
    turing_perf.optimal_block_sizes["hash_kernel"] = 128;
    turing_perf.optimal_block_sizes["compare_kernel"] = 512;
    performance_matrix_[GPUArchitecture::TURING] = turing_perf;

    // Initialize Ampere performance characteristics
    ArchitecturePerformance ampere_perf;
    ampere_perf.architecture = GPUArchitecture::AMPERE;
    ampere_perf.supported_kernels = {"ecc_kernel", "hash_kernel", "compare_kernel", "ecc_tensor", "ecc_bf16"};
    ampere_perf.supports_tensor_cores = true;
    ampere_perf.supports_cooperative_launch = true;
    ampere_perf.supports_memory_pool = true;
    ampere_perf.memory_bandwidth_efficiency = 0.90;
    ampere_perf.compute_efficiency = 0.85;
    ampere_perf.occupancy_target = 0.85;
    ampere_perf.max_concurrent_kernels = 2;
    ampere_perf.max_batch_size = 20000;
    ampere_perf.optimal_block_sizes["ecc_kernel"] = 256;
    ampere_perf.optimal_block_sizes["hash_kernel"] = 256;
    ampere_perf.optimal_block_sizes["compare_kernel"] = 512;
    ampere_perf.optimal_block_sizes["ecc_tensor"] = 128;
    ampere_perf.optimal_block_sizes["ecc_bf16"] = 256;
    performance_matrix_[GPUArchitecture::AMPERE] = ampere_perf;

    // Initialize Ada Lovelace performance characteristics
    ArchitecturePerformance ada_perf;
    ada_perf.architecture = GPUArchitecture::ADA_LOVELACE;
    ada_perf.supported_kernels = {"ecc_kernel", "hash_kernel", "compare_kernel", "ecc_tensor", "ecc_bf16"};
    ada_perf.supports_tensor_cores = true;
    ada_perf.supports_cooperative_launch = true;
    ada_perf.supports_memory_pool = true;
    ada_perf.supports_unified_memory = true;
    ada_perf.memory_bandwidth_efficiency = 0.95;
    ada_perf.compute_efficiency = 0.90;
    ada_perf.occupancy_target = 0.90;
    ada_perf.max_concurrent_kernels = 4;
    ada_perf.max_batch_size = 50000;
    ada_perf.optimal_block_sizes["ecc_kernel"] = 256;
    ada_perf.optimal_block_sizes["hash_kernel"] = 512;
    ada_perf.optimal_block_sizes["compare_kernel"] = 512;
    ada_perf.optimal_block_sizes["ecc_tensor"] = 128;
    ada_perf.optimal_block_sizes["ecc_bf16"] = 256;
    performance_matrix_[GPUArchitecture::ADA_LOVELACE] = ada_perf;

    // Initialize Hopper performance characteristics
    ArchitecturePerformance hopper_perf;
    hopper_perf.architecture = GPUArchitecture::HOPPER;
    hopper_perf.supported_kernels = {"ecc_kernel", "hash_kernel", "compare_kernel", "ecc_tensor", "ecc_bf16", "ecc_fp8"};
    hopper_perf.supports_tensor_cores = true;
    hopper_perf.supports_cooperative_launch = true;
    hopper_perf.supports_memory_pool = true;
    hopper_perf.supports_unified_memory = true;
    hopper_perf.memory_bandwidth_efficiency = 0.98;
    hopper_perf.compute_efficiency = 0.95;
    hopper_perf.occupancy_target = 0.95;
    hopper_perf.max_concurrent_kernels = 8;
    hopper_perf.max_batch_size = 100000;
    hopper_perf.optimal_block_sizes["ecc_kernel"] = 512;
    hopper_perf.optimal_block_sizes["hash_kernel"] = 512;
    hopper_perf.optimal_block_sizes["compare_kernel"] = 1024;
    hopper_perf.optimal_block_sizes["ecc_tensor"] = 256;
    hopper_perf.optimal_block_sizes["ecc_bf16"] = 512;
    hopper_perf.optimal_block_sizes["ecc_fp8"] = 1024;
    performance_matrix_[GPUArchitecture::HOPPER] = hopper_perf;
}

GPUDeviceInfo GPUArchitectureCompatibilityMatrix::queryDeviceInfo(int device_id) const {
    GPUDeviceInfo info;
    info.device_id = device_id;

    cudaDeviceProp prop;
    if (cudaGetDeviceProperties(&prop, device_id) != cudaSuccess) {
        return info;
    }

    // Basic information
    info.name = prop.name;
    info.compute_capability = {prop.major, prop.minor};
    info.architecture = detectArchitecture(info.compute_capability);
    info.total_memory = prop.totalGlobalMem;
    info.shared_memory_per_block = prop.sharedMemPerBlock;
    info.max_threads_per_block = prop.maxThreadsPerBlock;
    info.max_threads_per_multiprocessor = prop.maxThreadsPerMultiProcessor;
    info.max_blocks_per_multiprocessor = prop.maxBlocksPerMultiProcessor;
    info.warp_size = prop.warpSize;
    info.max_registers_per_block = prop.regsPerBlock;
    info.max_registers_per_thread = prop.regsPerBlock / prop.maxThreadsPerBlock;
    info.multiprocessor_count = prop.multiProcessorCount;
    info.max_threads_per_device = prop.maxThreadsPerMultiProcessor * prop.multiProcessorCount;
    info.l2_cache_size = prop.l2CacheSize;
    info.integrated = prop.integrated;
    info.supports_managed_memory = prop.managedMemory;
    info.supports_concurrent_kernels = prop.concurrentKernels;
    info.supports_ecc = prop.ECCEnabled;

    // Architecture-specific features
    if (info.compute_capability.meetsMinimum({7, 0})) {
        info.supports_tensor_cores = true; // Volta and later
    }

    if (info.compute_capability.meetsMinimum({7, 5})) {
        info.supports_tensor_cores = true; // Turing and later with improved tensor cores
    }

    if (info.compute_capability.meetsMinimum({8, 0})) {
        info.supports_bf16 = true; // Ampere and later
        info.supports_tf32 = true; // Ampere and later
    }

    if (info.compute_capability.meetsMinimum({8, 6})) {
        info.supports_ray_tracing = true; // RTX 30xx series
    }

    if (info.compute_capability.meetsMinimum({8, 9})) {
        info.supports_fp8 = true; // Ada Lovelace and later
    }

    // Estimate memory bandwidth (simplified)
    info.memory_bandwidth = static_cast<size_t>(prop.memoryClockRate * 1000.0 * prop.memoryBusWidth / 8.0);
    info.base_clock = prop.clockRate / 1000.0;

    return info;
}

void GPUArchitectureCompatibilityMatrix::addCompatibilityEntry(const CompatibilityEntry& entry) {
    std::lock_guard<std::mutex> lock(matrix_mutex_);
    compatibility_matrix_[entry.architecture] = entry;
}

void GPUArchitectureCompatibilityMatrix::addPerformanceEntry(const ArchitecturePerformance& performance) {
    std::lock_guard<std::mutex> lock(matrix_mutex_);
    performance_matrix_[performance.architecture] = performance;
}

// ============================================================================
// KernelSelector Implementation
// ============================================================================

KernelSelector::KernelSelector(const GPUArchitectureCompatibilityMatrix& matrix)
    : matrix_(matrix) {
}

KernelSelector::KernelSelection KernelSelector::selectKernel(int device_id, const std::string& base_kernel_name) const {
    auto device = matrix_.getDeviceInfo(device_id);
    return selectKernel(device, base_kernel_name);
}

KernelSelector::KernelSelection KernelSelector::selectKernel(const GPUDeviceInfo& device, const std::string& base_kernel_name) const {
    KernelSelection selection;

    // Check direct compatibility
    if (matrix_.isKernelCompatible(device, base_kernel_name)) {
        selection.kernel_name = base_kernel_name;
        selection.kernel_variant = "standard";
        selection.is_fallback = false;
        selection.optimal_config = matrix_.getOptimalConfiguration(device, base_kernel_name);
        return selection;
    }

    // Try architecture-specific variants
    std::vector<std::string> variants = {
        base_kernel_name + "_tensor",
        base_kernel_name + "_bf16",
        base_kernel_name + "_optimized",
        base_kernel_name + "_legacy"
    };

    for (const auto& variant : variants) {
        if (matrix_.isKernelCompatible(device, variant)) {
            selection.kernel_name = variant;
            selection.kernel_variant = variant.substr(base_kernel_name.length() + 1);
            selection.is_fallback = true;
            selection.fallback_reason = "Original kernel not compatible";
            selection.optimal_config = matrix_.getOptimalConfiguration(device, variant);
            return selection;
        }
    }

    // Try architecture-specific alternatives
    std::string alternative = matrix_.getAlternativeKernel(device, base_kernel_name);
    if (!alternative.empty()) {
        selection.kernel_name = alternative;
        selection.kernel_variant = "alternative";
        selection.is_fallback = true;
        selection.fallback_reason = "Using architecture-specific alternative";
        selection.optimal_config = matrix_.getOptimalConfiguration(device, alternative);
        return selection;
    }

    // No compatible kernel found
    selection.kernel_name = "";
    selection.kernel_variant = "none";
    selection.is_fallback = true;
    selection.fallback_reason = "No compatible kernel found for this architecture";

    return selection;
}

std::vector<KernelSelector::KernelSelection> KernelSelector::selectKernels(
    const std::vector<int>& device_ids, const std::string& base_kernel_name
) const {
    std::vector<KernelSelection> selections;
    selections.reserve(device_ids.size());

    for (int device_id : device_ids) {
        selections.push_back(selectKernel(device_id, base_kernel_name));
    }

    return selections;
}

std::vector<KernelSelector::KernelSelection> KernelSelector::selectKernels(
    const std::vector<GPUDeviceInfo>& devices, const std::string& base_kernel_name
) const {
    std::vector<KernelSelection> selections;
    selections.reserve(devices.size());

    for (const auto& device : devices) {
        selections.push_back(selectKernel(device, base_kernel_name));
    }

    return selections;
}

std::vector<std::string> KernelSelector::getKernelRanking(
    int device_id, const std::vector<std::string>& kernel_variants
) const {
    auto device = matrix_.getDeviceInfo(device_id);
    std::vector<std::pair<std::string, double>> scored_kernels;

    for (const auto& kernel : kernel_variants) {
        if (matrix_.isKernelCompatible(device, kernel)) {
            double score = calculateKernelScore(device, kernel);
            scored_kernels.push_back({kernel, score});
        }
    }

    // Sort by score (descending)
    std::sort(scored_kernels.begin(), scored_kernels.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<std::string> ranking;
    for (const auto& [kernel, score] : scored_kernels) {
        ranking.push_back(kernel);
    }

    return ranking;
}

double KernelSelector::calculateKernelScore(const GPUDeviceInfo& device, const std::string& kernel_name) const {
    double score = 0.0;

    // Base score for compatibility
    score += compatibility_weight_ * 100.0;

    // Performance score
    auto perf = matrix_.getPerformanceCharacteristics(device.architecture);
    auto throughput_it = perf.kernel_throughput.find(kernel_name);
    if (throughput_it != perf.kernel_throughput.end()) {
        score += performance_weight_ * (throughput_it->second / 1000.0); // Normalize to keys/sec
    }

    // Efficiency score
    auto efficiency_it = perf.kernel_efficiency.find(kernel_name);
    if (efficiency_it != perf.kernel_efficiency.end()) {
        score += efficiency_weight_ * efficiency_it->second;
    }

    // Bonus for architecture-optimized kernels
    if (kernel_name.find("_tensor") != std::string::npos && device.supports_tensor_cores) {
        score += 20.0;
    }

    if (kernel_name.find("_bf16") != std::string::npos && device.supports_bf16) {
        score += 15.0;
    }

    return score;
}

std::vector<std::string> KernelSelector::getCompatibleVariants(const GPUDeviceInfo& device, const std::string& base_kernel_name) const {
    std::vector<std::string> variants;

    // Standard variants to try
    std::vector<std::string> potential_variants = {
        base_kernel_name,
        base_kernel_name + "_standard",
        base_kernel_name + "_optimized",
        base_kernel_name + "_tensor",
        base_kernel_name + "_bf16",
        base_kernel_name + "_fp8",
        base_kernel_name + "_high_performance",
        base_kernel_name + "_low_memory"
    };

    for (const auto& variant : potential_variants) {
        if (matrix_.isKernelCompatible(device, variant)) {
            variants.push_back(variant);
        }
    }

    return variants;
}

// ============================================================================
// CompatibilityManager Implementation
// ============================================================================

CompatibilityManager& CompatibilityManager::getInstance() {
    static CompatibilityManager instance;
    return instance;
}

bool CompatibilityManager::initialize() {
    std::lock_guard<std::mutex> lock(manager_mutex_);

    if (initialized_) {
        return true;
    }

    matrix_ = std::make_shared<GPUArchitectureCompatibilityMatrix>();
    if (!matrix_->initialize()) {
        return false;
    }

    kernel_selector_ = std::make_shared<KernelSelector>(*matrix_);
    compatibility_tester_ = std::make_shared<CompatibilityTester>(*matrix_);

    initialized_ = true;
    return true;
}

std::shared_ptr<GPUArchitectureCompatibilityMatrix> CompatibilityManager::getCompatibilityMatrix() {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    return matrix_;
}

std::shared_ptr<KernelSelector> CompatibilityManager::getKernelSelector() {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    return kernel_selector_;
}

std::shared_ptr<CompatibilityTester> CompatibilityManager::getCompatibilityTester() {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    return compatibility_tester_;
}

bool CompatibilityManager::isSystemCompatible() const {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    if (!matrix_) return false;

    auto devices = matrix_->getAvailableDevices();
    return !devices.empty();
}

std::vector<int> CompatibilityManager::getCompatibleDevices() const {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    if (!matrix_) return {};

    auto devices = matrix_->getAvailableDevices();
    std::vector<int> device_ids;
    for (const auto& device : devices) {
        device_ids.push_back(device.device_id);
    }
    return device_ids;
}

std::string CompatibilityManager::runDiagnostics() const {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    if (!matrix_) return "Compatibility system not initialized";

    return matrix_->generateSystemReport();
}

std::string CompatibilityManager::getSystemSummary() const {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    if (!matrix_) return "Compatibility system not initialized";

    auto devices = matrix_->getAvailableDevices();
    std::ostringstream oss;
    oss << "GPU Compatibility Summary:\n";
    oss << "  Available Devices: " << devices.size() << "\n";

    if (!devices.empty()) {
        for (const auto& device : devices) {
            oss << "  Device " << device.device_id << ": " << device.name << " (" << device.getArchitectureString() << ")\n";
        }
    }

    return oss.str();
}

// ============================================================================
// Utility Functions Implementation
// ============================================================================

namespace utils {

ComputeCapability architectureToComputeCapability(GPUArchitecture architecture) {
    switch (architecture) {
        case GPUArchitecture::PASCAL: return {6, 0};
        case GPUArchitecture::VOLTA: return {7, 0};
        case GPUArchitecture::TURING: return {7, 5};
        case GPUArchitecture::AMPERE: return {8, 0};
        case GPUArchitecture::ADA_LOVELACE: return {8, 9};
        case GPUArchitecture::HOPPER: return {9, 0};
        case GPUArchitecture::BLACKWELL: return {10, 0};
        default: return {0, 0};
    }
}

GPUArchitecture computeCapabilityToArchitecture(const ComputeCapability& compute_cap) {
    if (compute_cap.major == 6) return GPUArchitecture::PASCAL;
    if (compute_cap.major == 7) {
        return compute_cap.minor == 0 ? GPUArchitecture::VOLTA : GPUArchitecture::TURING;
    }
    if (compute_cap.major == 8) {
        return compute_cap.minor == 9 ? GPUArchitecture::ADA_LOVELACE : GPUArchitecture::AMPERE;
    }
    if (compute_cap.major == 9) return GPUArchitecture::HOPPER;
    if (compute_cap.major == 10) return GPUArchitecture::BLACKWELL;

    return GPUArchitecture::UNKNOWN;
}

std::vector<std::string> getArchitectureOptimizationFlags(GPUArchitecture architecture) {
    std::vector<std::string> flags;

    // Common flags
    flags.push_back("-O3");
    flags.push_back("-use_fast_math");

    switch (architecture) {
        case GPUArchitecture::TURING:
            flags.push_back("-arch=sm_75");
            flags.push_back("-Xptxas --allow-expensive-optimizations=true");
            break;

        case GPUArchitecture::AMPERE:
            flags.push_back("-arch=sm_80");
            flags.push_back("-Xptxas --allow-expensive-optimizations=true");
            break;

        case GPUArchitecture::ADA_LOVELACE:
            flags.push_back("-arch=sm_89");
            flags.push_back("-Xptxas --allow-expensive-optimizations=true");
            break;

        case GPUArchitecture::HOPPER:
            flags.push_back("-arch=sm_90");
            flags.push_back("-Xptxas --allow-expensive-optimizations=true");
            break;

        default:
            flags.push_back("-arch=sm_75");
            break;
    }

    return flags;
}

RecommendedParameters getRecommendedParameters(GPUArchitecture architecture, const std::string& kernel_name) {
    RecommendedParameters params;

    switch (architecture) {
        case GPUArchitecture::TURING:
            params.threads_per_block = 256;
            params.min_blocks_per_multiprocessor = 4;
            params.use_tensor_cores = (kernel_name.find("ecc") != std::string::npos);
            break;

        case GPUArchitecture::AMPERE:
            params.threads_per_block = 256;
            params.min_blocks_per_multiprocessor = 6;
            params.use_tensor_cores = (kernel_name.find("ecc") != std::string::npos);
            params.use_cooperative_groups = true;
            break;

        case GPUArchitecture::ADA_LOVELACE:
            params.threads_per_block = 512;
            params.min_blocks_per_multiprocessor = 8;
            params.use_tensor_cores = (kernel_name.find("ecc") != std::string::npos);
            params.use_cooperative_groups = true;
            break;

        case GPUArchitecture::HOPPER:
            params.threads_per_block = 512;
            params.min_blocks_per_multiprocessor = 12;
            params.use_tensor_cores = (kernel_name.find("ecc") != std::string::npos);
            params.use_cooperative_groups = true;
            break;

        default:
            params.threads_per_block = 256;
            params.min_blocks_per_multiprocessor = 4;
            break;
    }

    return params;
}

bool supportsCudaFeature(GPUArchitecture architecture, const std::string& feature) {
    switch (architecture) {
        case GPUArchitecture::TURING:
            return feature == "tensor_cores" || feature == "cooperative_groups";

        case GPUArchitecture::AMPERE:
            return feature == "tensor_cores" || feature == "bf16" || feature == "tf32" || feature == "cooperative_groups";

        case GPUArchitecture::ADA_LOVELACE:
            return feature == "tensor_cores" || feature == "bf16" || feature == "tf32" || feature == "fp8" || feature == "cooperative_groups";

        case GPUArchitecture::HOPPER:
            return feature == "tensor_cores" || feature == "bf16" || feature == "tf32" || feature == "fp8" || feature == "cooperative_groups";

        default:
            return false;
    }
}

double getMemoryBandwidthGBps(GPUArchitecture architecture) {
    switch (architecture) {
        case GPUArchitecture::TURING: return 448.0;    // RTX 2080 Ti
        case GPUArchitecture::AMPERE: return 760.0;    // RTX 3090
        case GPUArchitecture::ADA_LOVELACE: return 1008.0; // RTX 4090
        case GPUArchitecture::HOPPER: return 3350.0;    // H100
        default: return 0.0;
    }
}

double getTheoreticalPeakPerformance(GPUArchitecture architecture) {
    switch (architecture) {
        case GPUArchitecture::TURING: return 26.9;    // RTX 2080 Ti TFLOPS
        case GPUArchitecture::AMPERE: return 35.6;    // RTX 3090 TFLOPS
        case GPUArchitecture::ADA_LOVELACE: return 82.6; // RTX 4090 TFLOPS
        case GPUArchitecture::HOPPER: return 67.3;     // H100 TFLOPS (FP64)
        default: return 0.0;
    }
}

} // namespace puzzle71::gpu::utils

} // namespace puzzle71::gpu
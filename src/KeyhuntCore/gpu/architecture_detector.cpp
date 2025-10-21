// Puzzle71Solver - GPU Architecture Detection and Validation Implementation
// Comprehensive GPU architecture detection and validation system (T013)

#include "architecture_detector.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <array>
#include <numeric>

// Include CUDA headers if available
#ifdef __CUDACC__
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#endif

using json = nlohmann::json;

namespace keyhunt {
namespace gpu {

// Constants
const int ArchitectureDetector::MIN_COMPUTE_CAPABILITY_MAJOR = 7;
const int ArchitectureDetector::MIN_COMPUTE_CAPABILITY_MINOR = 5;
const size_t ArchitectureDetector::MIN_MEMORY_MB = 2048;
const int ArchitectureDetector::MIN_SM_COUNT = 10;

// Global detector instance
std::unique_ptr<ArchitectureDetector> g_architecture_detector = nullptr;

ArchitectureDetector::ArchitectureDetector() {
    initialize_default_profiles();
}

bool ArchitectureDetector::initialize() {
    log_info("Initializing GPU Architecture Detection System");

    if (is_initialized_) {
        log_info("Already initialized");
        return true;
    }

    // Check CUDA availability
    if (!is_cuda_available()) {
        log_error("CUDA is not available on this system");
        return false;
    }

    // Detect all GPUs
    detected_gpus_ = detect_all_gpus();

    if (detected_gpus_.empty()) {
        log_error("No compatible GPUs detected");
        return false;
    }

    is_initialized_ = true;
    log_info("GPU Architecture Detection System initialized successfully");
    log_info("Detected " + std::to_string(detected_gpus_.size()) + " GPU(s)");

    return true;
}

bool ArchitectureDetector::validate_system_requirements() {
    log_info("Validating system requirements for GPU detection");

    // Check CUDA driver
    std::string driver_version = get_cuda_driver_version();
    if (driver_version.empty()) {
        log_error("CUDA driver not found");
        return false;
    }

    log_info("CUDA Driver Version: " + driver_version);

    // Check CUDA runtime
    std::string runtime_version = get_cuda_runtime_version();
    if (runtime_version.empty()) {
        log_error("CUDA runtime not found");
        return false;
    }

    log_info("CUDA Runtime Version: " + runtime_version);

    // Check for compatible GPUs
    auto compatible_gpus = get_compatible_gpus();
    if (compatible_gpus.empty()) {
        log_error("No compatible GPUs found (minimum: SM 7.5)");
        return false;
    }

    log_info("Found " + std::to_string(compatible_gpus.size()) + " compatible GPU(s)");

    // Validate system compatibility
    auto system_validation = validate_system_compatibility();
    if (!system_validation.is_valid) {
        log_error("System compatibility validation failed");
        for (const auto& error : system_validation.errors) {
            log_error("  " + error);
        }
        return false;
    }

    log_success("System requirements validation passed");
    return true;
}

std::vector<GPUDeviceInfo> ArchitectureDetector::detect_all_gpus() {
    log_info("Detecting all available GPUs");

    std::vector<GPUDeviceInfo> gpu_list;

#ifdef __CUDACC__
    int device_count = 0;
    cudaError_t cuda_err = cudaGetDeviceCount(&device_count);

    if (cuda_err != cudaSuccess) {
        log_error("Failed to get CUDA device count: " + std::string(cudaGetErrorString(cuda_err)));
        return gpu_list;
    }

    log_info("CUDA reports " + std::to_string(device_count) + " device(s)");

    for (int device_id = 0; device_id < device_count; ++device_id) {
        GPUDeviceInfo gpu_info = query_gpu_device(device_id);
        if (gpu_info.device_id != -1) {
            gpu_list.push_back(gpu_info);
            log_info("Detected GPU " + std::to_string(device_id) + ": " + gpu_info.name +
                     " (" + gpu_info.compute_capability.to_string() + ")");
        }
    }
#else
    log_warning("CUDA headers not available, using fallback detection");

    // Fallback: Try to use nvidia-smi
    std::string nvidia_smi_output = execute_command("nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv,noheader");
    if (!nvidia_smi_output.empty()) {
        // Parse nvidia-smi output (simplified)
        std::istringstream iss(nvidia_smi_output);
        std::string line;
        int device_id = 0;

        while (std::getline(iss, line)) {
            GPUDeviceInfo gpu_info;
            gpu_info.device_id = device_id++;
            gpu_info.name = line.substr(0, line.find(','));
            gpu_info.architecture = identify_architecture_from_name(gpu_info.name);
            gpu_info.supports_cuda = true;

            // Set some defaults for fallback detection
            gpu_info.compute_capability = {7, 5, "7.5"}; // Assume minimum supported
            gpu_info.total_memory_mb = 8192; // Assume 8GB
            gpu_info.warp_size = 32;
            gpu_info.max_threads_per_block = 1024;

            gpu_list.push_back(gpu_info);
        }
    }
#endif

    // Sort GPUs by performance (primary first)
    std::sort(gpu_list.begin(), gpu_list.end(),
              [](const GPUDeviceInfo& a, const GPUDeviceInfo& b) {
                  if (a.compute_capability.major != b.compute_capability.major) {
                      return a.compute_capability.major > b.compute_capability.major;
                  }
                  return a.compute_capability.minor > b.compute_capability.minor;
              });

    log_info("GPU detection completed. Found " + std::to_string(gpu_list.size()) + " GPU(s)");
    return gpu_list;
}

GPUDeviceInfo ArchitectureDetector::query_gpu_device(int device_id) {
    GPUDeviceInfo gpu_info;
    gpu_info.device_id = device_id;

#ifdef __CUDACC__
    // Query CUDA device properties
    if (!query_cuda_device_properties(device_id, gpu_info)) {
        log_error("Failed to query CUDA device properties for device " + std::to_string(device_id));
        return gpu_info;
    }

    // Query additional information from nvidia-smi if available
    query_nvidia_smi_info(device_id, gpu_info);
#else
    log_warning("CUDA not available, cannot query device properties");
#endif

    // Detect architecture from properties
    gpu_info.architecture = identify_architecture_from_compute_capability(
        gpu_info.compute_capability.major,
        gpu_info.compute_capability.minor
    );

    // Set default values for missing information
    if (gpu_info.warp_size == 0) gpu_info.warp_size = 32;
    if (gpu_info.max_threads_per_block == 0) gpu_info.max_threads_per_block = 1024;
    if (gpu_info.shared_memory_per_block_bytes == 0) gpu_info.shared_memory_per_block_bytes = 49152;

    return gpu_info;
}

#ifdef __CUDACC__
bool ArchitectureDetector::query_cuda_device_properties(int device_id, GPUDeviceInfo& info) {
    cudaDeviceProp device_prop;
    cudaError_t cuda_err = cudaGetDeviceProperties(&device_prop, device_id);

    if (cuda_err != cudaSuccess) {
        log_error("Failed to get device properties: " + std::string(cudaGetErrorString(cuda_err)));
        return false;
    }

    // Basic device information
    info.name = std::string(device_prop.name);
    info.compute_capability.major = device_prop.major;
    info.compute_capability.minor = device_prop.minor;
    info.compute_capability.compute_capability_string = std::to_string(device_prop.major) + "." + std::to_string(device_prop.minor);

    // Memory information
    info.total_memory_mb = device_prop.totalGlobalMem / (1024 * 1024);
    info.shared_memory_per_block_bytes = device_prop.sharedMemPerBlock;
    info.constant_memory_bytes = device_prop.totalConstMem;
    info.l2_cache_size_bytes = device_prop.l2CacheSize;

    // Core specifications
    info.sm_count = device_prop.multiProcessorCount;
    info.multiprocessor_count = device_prop.multiProcessorCount;
    info.max_threads_per_multiprocessor = device_prop.maxThreadsPerMultiProcessor;
    info.max_threads_per_block = device_prop.maxThreadsPerBlock;
    info.warp_size = device_prop.warpSize;

    // Block and grid dimensions
    for (int i = 0; i < 3; ++i) {
        info.max_block_dim[i] = device_prop.maxThreadsDim[i];
        info.max_grid_dim[i] = device_prop.maxGridDim[i];
    }

    // Feature support
    info.supports_cuda = true;
    info.supports_tensor_cores = (device_prop.major >= 7); // Tensor cores available on Volta and later
    info.supports_mig = (device_prop.major >= 7 && device_prop.minor >= 5); // MIG available on Turing and later

    // Clock rates (in KHz, convert to MHz)
    info.base_clock_mhz = device_prop.clockRate / 1000.0;
    info.memory_clock_mhz = device_prop.memoryClockRate / 1000.0;

    // Memory bus width
    info.memory_bus_width = device_prop.memoryBusWidth;

    // Compute theoretical performance
    double cores_per_sm = get_cores_per_sm(device_prop.major, device_prop.minor);
    info.tflops_fp32 = (cores_per_sm * info.sm_count * info.base_clock_mhz * 1000000 * 2) / 1e12; // FMA capability

    log_debug("Queried CUDA device properties for: " + info.name);
    return true;
}
#endif

bool ArchitectureDetector::query_nvidia_smi_info(int device_id, GPUDeviceInfo& info) {
    // Try to get additional information from nvidia-smi
    std::string smi_cmd = "nvidia-smi --query-gpu=driver_version,cuda_version,memory.used,memory.total,temperature.gpu,power.draw --format=csv,noheader,nounits -i " + std::to_string(device_id);
    std::string smi_output = execute_command(smi_cmd);

    if (!smi_output.empty()) {
        std::istringstream iss(smi_output);
        std::string line;
        if (std::getline(iss, line)) {
            // Parse nvidia-smi output (simplified parsing)
            std::vector<std::string> tokens;
            std::stringstream ss(line);
            std::string token;
            while (std::getline(ss, token, ',')) {
                tokens.push_back(token);
            }

            if (tokens.size() >= 2) {
                info.driver_version = tokens[0];
                info.cuda_version = tokens[1];
            }

            if (tokens.size() >= 4) {
                // tokens[2] = used memory, tokens[3] = total memory
                try {
                    size_t used_memory = std::stoul(tokens[2]);
                    size_t total_memory = std::stoul(tokens[3]);
                    info.total_memory_mb = total_memory;
                    info.free_memory_mb = total_memory - used_memory;
                } catch (const std::exception& e) {
                    log_debug("Failed to parse memory information from nvidia-smi");
                }
            }
        }
    }

    // Try to get detailed GPU information
    std::string detail_cmd = "nvidia-smi -q -i " + std::to_string(device_id);
    std::string detail_output = execute_command(detail_cmd);

    // Parse detailed output for more information
    if (!detail_output.empty()) {
        // Look for memory type, bandwidth, etc.
        // This is a simplified implementation - full parsing would be more comprehensive
        if (detail_output.find("GDDR6") != std::string::npos) {
            info.memory_type = "GDDR6";
        } else if (detail_output.find("HBM2") != std::string::npos) {
            info.memory_type = "HBM2";
        } else if (detail_output.find("GDDR5") != std::string::npos) {
            info.memory_type = "GDDR5";
        }

        // Estimate memory bandwidth from GPU type
        if (info.architecture == GPUArchitecture::AMPERE) {
            info.memory_bandwidth_gb_per_sec = info.memory_bus_width * info.memory_clock_mhz * 2 / 1000.0;
        } else if (info.architecture == GPUArchitecture::TURING) {
            info.memory_bandwidth_gb_per_sec = info.memory_bus_width * info.memory_clock_mhz * 2 / 1000.0;
        }
    }

    return true;
}

GPUArchitecture ArchitectureDetector::identify_architecture_from_name(const std::string& name) {
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

    // RTX 40 series (Ada Lovelace)
    if (lower_name.find("rtx 40") != std::string::npos) {
        return GPUArchitecture::ADA_LOVELACE;
    }

    // RTX 30 series (Ampere)
    if (lower_name.find("rtx 30") != std::string::npos ||
        lower_name.find("a100") != std::string::npos ||
        lower_name.find("a30") != std::string::npos) {
        return GPUArchitecture::AMPERE;
    }

    // RTX 20 series (Turing)
    if (lower_name.find("rtx 20") != std::string::npos ||
        lower_name.find("turing") != std::string::npos ||
        lower_name.find("titan rtx") != std::string::npos) {
        return GPUArchitecture::TURING;
    }

    // Volta
    if (lower_name.find("v100") != std::string::npos ||
        lower_name.find("volta") != std::string::npos) {
        return GPUArchitecture::VOLTA;
    }

    // Pascal
    if (lower_name.find("gtx 10") != std::string::npos ||
        lower_name.find("pascal") != std::string::npos) {
        return GPUArchitecture::PASCAL;
    }

    // Maxwell
    if (lower_name.find("gtx 9") != std::string::npos ||
        lower_name.find("maxwell") != std::string::npos) {
        return GPUArchitecture::MAXWELL;
    }

    // Hopper
    if (lower_name.find("h100") != std::string::npos ||
        lower_name.find("h20") != std::string::npos ||
        lower_name.find("hoppper") != std::string::npos) {
        return GPUArchitecture::HOPPER;
    }

    return GPUArchitecture::UNKNOWN;
}

GPUArchitecture ArchitectureDetector::identify_architecture_from_compute_capability(int major, int minor) {
    if (major == 9) {
        return GPUArchitecture::HOPPER;
    } else if (major == 8) {
        if (minor >= 9) {
            return GPUArchitecture::ADA_LOVELACE;
        } else {
            return GPUArchitecture::AMPERE;
        }
    } else if (major == 7) {
        if (minor >= 5) {
            return GPUArchitecture::TURING;
        } else {
            return GPUArchitecture::VOLTA;
        }
    } else if (major == 6) {
        return GPUArchitecture::PASCAL;
    } else if (major == 5) {
        return GPUArchitecture::MAXWELL;
    } else if (major == 3) {
        return GPUArchitecture::KEPLER;
    }

    return GPUArchitecture::UNKNOWN;
}

std::vector<GPUDeviceInfo> ArchitectureDetector::get_compatible_gpus() {
    std::vector<GPUDeviceInfo> compatible_gpus;

    for (const auto& gpu : detected_gpus_) {
        if (is_gpu_compatible(gpu)) {
            compatible_gpus.push_back(gpu);
        }
    }

    return compatible_gpus;
}

bool ArchitectureDetector::is_gpu_compatible(const GPUDeviceInfo& gpu_info) {
    // Check minimum compute capability
    if (!gpu_info.compute_capability.is_supported()) {
        log_debug("GPU " + gpu_info.name + " has unsupported compute capability: " +
                  gpu_info.compute_capability.to_string());
        return false;
    }

    // Check minimum memory
    if (gpu_info.total_memory_mb < MIN_MEMORY_MB) {
        log_debug("GPU " + gpu_info.name + " has insufficient memory: " +
                  std::to_string(gpu_info.total_memory_mb) + "MB < " + std::to_string(MIN_MEMORY_MB) + "MB");
        return false;
    }

    // Check minimum SM count
    if (gpu_info.sm_count < MIN_SM_COUNT) {
        log_debug("GPU " + gpu_info.name + " has insufficient SM count: " +
                  std::to_string(gpu_info.sm_count) + " < " + std::to_string(MIN_SM_COUNT));
        return false;
    }

    return true;
}

ValidationResult ArchitectureDetector::validate_gpu(const GPUDeviceInfo& gpu_info) {
    ValidationResult result;
    result.is_valid = true;
    result.is_compatible = true;

    // Validate compute capability
    if (!gpu_info.compute_capability.is_supported()) {
        result.is_valid = false;
        result.is_compatible = false;
        result.errors.push_back("Unsupported compute capability: " + gpu_info.compute_capability.to_string());
        result.errors.push_back("Minimum required: SM 7.5 (Turing) or later");
    }

    // Validate memory
    if (gpu_info.total_memory_mb < MIN_MEMORY_MB) {
        result.is_valid = false;
        result.errors.push_back("Insufficient GPU memory: " + std::to_string(gpu_info.total_memory_mb) + "MB");
        result.errors.push_back("Minimum required: " + std::to_string(MIN_MEMORY_MB) + "MB");
    }

    // Validate SM count
    if (gpu_info.sm_count < MIN_SM_COUNT) {
        result.is_valid = false;
        result.errors.push_back("Insufficient SM count: " + std::to_string(gpu_info.sm_count));
        result.errors.push_back("Minimum required: " + std::to_string(MIN_SM_COUNT));
    }

    // Warnings and recommendations
    if (gpu_info.total_memory_mb < 4096) {
        result.warnings.push_back("GPU memory is below 4GB, may limit performance");
    }

    if (gpu_info.compute_capability.major < 8) {
        result.warnings.push_back("GPU architecture is not latest generation");
        result.recommendations.push_back("Consider upgrading to Ampere or later for better performance");
    }

    if (!gpu_info.supports_tensor_cores) {
        result.recommendations.push_back("GPU does not support Tensor Cores, missing performance optimizations");
    }

    // Calculate compatibility score
    double score = 1.0;
    if (gpu_info.compute_capability.major == 7 && gpu_info.compute_capability.minor == 5) {
        score = 0.7; // Turing 7.5 - minimum supported
    } else if (gpu_info.compute_capability.major == 8 && gpu_info.compute_capability.minor == 6) {
        score = 0.85; // Ampere 8.6 - good
    } else if (gpu_info.compute_capability.major == 8 && gpu_info.compute_capability.minor >= 9) {
        score = 0.95; // Ada Lovelace 8.9+ - excellent
    } else if (gpu_info.compute_capability.major >= 9) {
        score = 1.0; // Hopper 9.x - best
    }

    result.compatibility_score = score;

    return result;
}

ValidationResult ArchitectureDetector::validate_system_compatibility() {
    ValidationResult result;
    result.is_valid = true;
    result.is_compatible = true;

    // Check CUDA availability
    if (!is_cuda_available()) {
        result.is_valid = false;
        result.is_compatible = false;
        result.errors.push_back("CUDA is not available on this system");
        return result;
    }

    // Check driver compatibility
    std::string driver_version = get_cuda_driver_version();
    if (driver_version.empty()) {
        result.is_valid = false;
        result.errors.push_back("CUDA driver not found or incompatible");
    }

    // Check for compatible GPUs
    auto compatible_gpus = get_compatible_gpus();
    if (compatible_gpus.empty()) {
        result.is_valid = false;
        result.is_compatible = false;
        result.errors.push_back("No compatible GPUs found");
        result.errors.push_back("Minimum requirement: SM 7.5 (Turing) with 2GB+ memory");
    }

    // Calculate overall system compatibility score
    if (!compatible_gpus.empty()) {
        double total_score = 0.0;
        for (const auto& gpu : compatible_gpus) {
            auto gpu_validation = validate_gpu(gpu);
            total_score += gpu_validation.compatibility_score;
        }
        result.compatibility_score = total_score / compatible_gpus.size();
    } else {
        result.compatibility_score = 0.0;
    }

    return result;
}

void ArchitectureDetector::initialize_default_profiles() {
    log_debug("Initializing default architecture profiles");

    // Turing profile (SM 7.5)
    ArchitectureProfile turing_profile;
    turing_profile.architecture = GPUArchitecture::TURING;
    turing_profile.min_compute_capability = {7, 5, "7.5"};
    turing_profile.max_compute_capability = {7, 5, "7.5"};
    turing_profile.optimal_block_size = 256;
    turing_profile.registers_per_thread_target = 32;
    turing_profile.max_registers_per_thread = 64;
    turing_profile.occupancy_target_percent = 75.0;
    turing_profile.shared_memory_carveout_percent = 75;
    turing_profile.preferred_block_sizes = {128, 256, 512};
    turing_profile.architecture_flags = {"-arch=sm_75"};
    turing_profile.optimization_flags = {"-Xptxas=-O3", "--use_fast_math"};
    turing_profile.expected_memory_efficiency = 0.85;
    turing_profile.expected_occupancy = 0.70;
    turing_profile.expected_bandwidth_utilization = 0.80;
    architecture_profiles_[GPUArchitecture::TURING] = turing_profile;

    // Ampere profile (SM 8.0, 8.6)
    ArchitectureProfile ampere_profile;
    ampere_profile.architecture = GPUArchitecture::AMPERE;
    ampere_profile.min_compute_capability = {8, 0, "8.0"};
    ampere_profile.max_compute_capability = {8, 6, "8.6"};
    ampere_profile.optimal_block_size = 256;
    ampere_profile.registers_per_thread_target = 32;
    ampere_profile.max_registers_per_thread = 80;
    ampere_profile.occupancy_target_percent = 80.0;
    ampere_profile.shared_memory_carveout_percent = 75;
    ampere_profile.preferred_block_sizes = {128, 256, 512};
    ampere_profile.architecture_flags = {"-arch=sm_80", "-arch=sm_86"};
    ampere_profile.optimization_flags = {"-Xptxas=-O3", "--use_fast_math", "-Xptxas=--allow-expensive-optimizations=true"};
    ampere_profile.expected_memory_efficiency = 0.90;
    ampere_profile.expected_occupancy = 0.75;
    ampere_profile.expected_bandwidth_utilization = 0.85;
    architecture_profiles_[GPUArchitecture::AMPERE] = ampere_profile;

    // Ada Lovelace profile (SM 8.9)
    ArchitectureProfile ada_profile;
    ada_profile.architecture = GPUArchitecture::ADA_LOVELACE;
    ada_profile.min_compute_capability = {8, 9, "8.9"};
    ada_profile.max_compute_capability = {8, 9, "8.9"};
    ada_profile.optimal_block_size = 256;
    ada_profile.registers_per_thread_target = 32;
    ada_profile.max_registers_per_thread = 80;
    ada_profile.occupancy_target_percent = 85.0;
    ada_profile.shared_memory_carveout_percent = 75;
    ada_profile.preferred_block_sizes = {128, 256, 512};
    ada_profile.architecture_flags = {"-arch=sm_89"};
    ada_profile.optimization_flags = {"-Xptxas=-O3", "--use_fast_math", "-Xptxas=--allow-expensive-optimizations=true"};
    ada_profile.expected_memory_efficiency = 0.95;
    ada_profile.expected_occupancy = 0.80;
    ada_profile.expected_bandwidth_utilization = 0.90;
    architecture_profiles_[GPUArchitecture::ADA_LOVELACE] = ada_profile;

    // Hopper profile (SM 9.0)
    ArchitectureProfile hopper_profile;
    hopper_profile.architecture = GPUArchitecture::HOPPER;
    hopper_profile.min_compute_capability = {9, 0, "9.0"};
    hopper_profile.max_compute_capability = {9, 0, "9.0"};
    hopper_profile.optimal_block_size = 256;
    hopper_profile.registers_per_thread_target = 40;
    hopper_profile.max_registers_per_thread = 80;
    hopper_profile.occupancy_target_percent = 90.0;
    hopper_profile.shared_memory_carveout_percent = 75;
    hopper_profile.preferred_block_sizes = {128, 256, 512, 1024};
    hopper_profile.architecture_flags = {"-arch=sm_90"};
    hopper_profile.optimization_flags = {"-Xptxas=-O3", "--use_fast_math", "-Xptxas=--allow-expensive-optimizations=true"};
    hopper_profile.expected_memory_efficiency = 0.98;
    hopper_profile.expected_occupancy = 0.85;
    hopper_profile.expected_bandwidth_utilization = 0.95;
    architecture_profiles_[GPUArchitecture::HOPPER] = hopper_profile;

    log_debug("Default architecture profiles initialized");
}

ArchitectureProfile ArchitectureDetector::get_architecture_profile(GPUArchitecture arch) {
    auto it = architecture_profiles_.find(arch);
    if (it != architecture_profiles_.end()) {
        return it->second;
    }

    // Return a default profile for unknown architectures
    ArchitectureProfile default_profile;
    default_profile.architecture = GPUArchitecture::UNKNOWN;
    default_profile.optimal_block_size = 256;
    default_profile.registers_per_thread_target = 32;
    default_profile.max_registers_per_thread = 40;
    default_profile.occupancy_target_percent = 75.0;
    return default_profile;
}

std::vector<std::string> ArchitectureDetector::generate_compiler_flags(const GPUDeviceInfo& gpu_info) {
    std::vector<std::string> flags;

    // Get architecture-specific profile
    auto profile = get_architecture_profile(gpu_info.architecture);

    // Add architecture flags
    for (const auto& arch_flag : profile.architecture_flags) {
        flags.push_back(arch_flag);
    }

    // Add optimization flags
    for (const auto& opt_flag : profile.optimization_flags) {
        flags.push_back(opt_flag);
    }

    // Add register pressure control
    flags.push_back("-maxrregcount=" + std::to_string(profile.max_registers_per_thread));

    return flags;
}

bool ArchitectureDetector::is_cuda_available() {
#ifdef __CUDACC__
    int device_count = 0;
    cudaError_t cuda_err = cudaGetDeviceCount(&device_count);
    return cuda_err == cudaSuccess && device_count > 0;
#else
    // Fallback check: try to run nvidia-smi
    std::string nvidia_smi_output = execute_command("nvidia-smi --version");
    return !nvidia_smi_output.empty();
#endif
}

std::string ArchitectureDetector::get_cuda_driver_version() {
#ifdef __CUDACC__
    int driver_version = 0;
    cudaError_t cuda_err = cudaDriverGetVersion(&driver_version);
    if (cuda_err == cudaSuccess) {
        return std::to_string(driver_version / 1000) + "." + std::to_string((driver_version % 1000) / 10);
    }
#endif

    // Fallback: try nvidia-smi
    std::string nvidia_smi_output = execute_command("nvidia-smi --query-gpu=driver_version --format=csv,noheader");
    if (!nvidia_smi_output.empty()) {
        return nvidia_smi_output.substr(0, nvidia_smi_output.find('\n'));
    }

    return "";
}

std::string ArchitectureDetector::get_cuda_runtime_version() {
#ifdef __CUDACC__
    int runtime_version = 0;
    cudaError_t cuda_err = cudaRuntimeGetVersion(&runtime_version);
    if (cuda_err == cudaSuccess) {
        return std::to_string(runtime_version / 1000) + "." + std::to_string((runtime_version % 1000) / 10);
    }
#endif
    return "";
}

std::string ArchitectureDetector::generate_system_report() {
    std::ostringstream report;

    report << "GPU Architecture Detection System Report\n";
    report << "=========================================\n\n";

    // System information
    report << "System Information:\n";
    report << "  CUDA Driver Version: " << get_cuda_driver_version() << "\n";
    report << "  CUDA Runtime Version: " << get_cuda_runtime_version() << "\n";
    report << "  CUDA Available: " << (is_cuda_available() ? "Yes" : "No") << "\n\n";

    // Detected GPUs
    report << "Detected GPUs (" << detected_gpus_.size() << " total):\n";
    for (size_t i = 0; i < detected_gpus_.size(); ++i) {
        const auto& gpu = detected_gpus_[i];
        report << "  GPU " << i << ": " << gpu.name << "\n";
        report << "    Architecture: " << architecture_to_string(gpu.architecture) << "\n";
        report << "    Compute Capability: " << gpu.compute_capability.to_string() << "\n";
        report << "    Memory: " << gpu.total_memory_mb << " MB\n";
        report << "    SM Count: " << gpu.sm_count << "\n";
        report << "    Compatible: " << (is_gpu_compatible(gpu) ? "Yes" : "No") << "\n";

        auto validation = validate_gpu(gpu);
        report << "    Compatibility Score: " << std::fixed << std::setprecision(2) << (validation.compatibility_score * 100) << "%\n";

        if (!validation.errors.empty()) {
            report << "    Errors:\n";
            for (const auto& error : validation.errors) {
                report << "      - " << error << "\n";
            }
        }

        if (!validation.warnings.empty()) {
            report << "    Warnings:\n";
            for (const auto& warning : validation.warnings) {
                report << "      - " << warning << "\n";
            }
        }
        report << "\n";
    }

    // System validation
    auto system_validation = validate_system_compatibility();
    report << "System Compatibility: " << (system_validation.is_valid ? "VALID" : "INVALID") << "\n";
    report << "Overall Compatibility Score: " << std::fixed << std::setprecision(2) << (system_validation.compatibility_score * 100) << "%\n";

    return report.str();
}

// Utility function implementations
std::string ArchitectureDetector::architecture_to_string(GPUArchitecture arch) {
    switch (arch) {
        case GPUArchitecture::KEPLER: return "Kepler";
        case GPUArchitecture::MAXWELL: return "Maxwell";
        case GPUArchitecture::PASCAL: return "Pascal";
        case GPUArchitecture::VOLTA: return "Volta";
        case GPUArchitecture::TURING: return "Turing";
        case GPUArchitecture::AMPERE: return "Ampere";
        case GPUArchitecture::ADA_LOVELACE: return "Ada Lovelace";
        case GPUArchitecture::HOPPER: return "Hopper";
        default: return "Unknown";
    }
}

int ArchitectureDetector::get_cores_per_sm(int major, int minor) {
    // Returns number of CUDA cores per SM for different architectures
    if (major == 9) { // Hopper
        return 128;
    } else if (major == 8) {
        if (minor >= 7) return 128; // Ada Lovelace
        return 128; // Ampere
    } else if (major == 7) {
        if (minor >= 5) return 64; // Turing
        return 64; // Volta
    } else if (major == 6) {
        return 128; // Pascal
    } else if (major == 5) {
        return 128; // Maxwell
    } else if (major == 3) {
        return 192; // Kepler
    }
    return 128; // Default guess
}

std::string ArchitectureDetector::execute_command(const std::string& command) {
    std::array<char, 128> buffer;
    std::string result;

#ifdef _WIN32
    std::shared_ptr<FILE> pipe(_popen(command.c_str(), "r"), _pclose);
#else
    std::shared_ptr<FILE> pipe(popen(command.c_str(), "r"), pclose);
#endif

    if (!pipe) return "";

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result;
}

void ArchitectureDetector::log_info(const std::string& message) const {
    std::cout << "[INFO] ArchitectureDetector: " << message << std::endl;
}

void ArchitectureDetector::log_warning(const std::string& message) const {
    std::cerr << "[WARNING] ArchitectureDetector: " << message << std::endl;
}

void ArchitectureDetector::log_error(const std::string& message) const {
    std::cerr << "[ERROR] ArchitectureDetector: " << message << std::endl;
}

void ArchitectureDetector::log_debug(const std::string& message) const {
    #ifdef DEBUG
    std::cout << "[DEBUG] ArchitectureDetector: " << message << std::endl;
    #endif
}

void ArchitectureDetector::log_success(const std::string& message) const {
    std::cout << "[SUCCESS] ArchitectureDetector: " << message << std::endl;
}

// Global convenience functions
bool initialize_gpu_detection() {
    if (!g_architecture_detector) {
        g_architecture_detector = std::make_unique<ArchitectureDetector>();
    }
    return g_architecture_detector->initialize();
}

std::vector<GPUDeviceInfo> get_available_gpus() {
    if (!g_architecture_detector) {
        if (!initialize_gpu_detection()) {
            return {};
        }
    }
    return g_architecture_detector->detect_all_gpus();
}

GPUDeviceInfo get_primary_gpu() {
    auto gpus = get_available_gpus();
    return gpus.empty() ? GPUDeviceInfo{} : gpus[0];
}

bool validate_system_gpu() {
    if (!g_architecture_detector) {
        if (!initialize_gpu_detection()) {
            return false;
        }
    }
    auto validation = g_architecture_detector->validate_system_compatibility();
    return validation.is_valid;
}

std::string get_system_gpu_report() {
    if (!g_architecture_detector) {
        if (!initialize_gpu_detection()) {
            return "GPU detection failed";
        }
    }
    return g_architecture_detector->generate_system_report();
}

// Namespace utilities
namespace utils {
    GPUArchitecture string_to_architecture(const std::string& arch_str) {
        std::string lower_str = arch_str;
        std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);

        if (lower_str == "kepler") return GPUArchitecture::KEPLER;
        if (lower_str == "maxwell") return GPUArchitecture::MAXWELL;
        if (lower_str == "pascal") return GPUArchitecture::PASCAL;
        if (lower_str == "volta") return GPUArchitecture::VOLTA;
        if (lower_str == "turing") return GPUArchitecture::TURING;
        if (lower_str == "ampere") return GPUArchitecture::AMPERE;
        if (lower_str == "ada_lovelace" || lower_str == "ada") return GPUArchitecture::ADA_LOVELACE;
        if (lower_str == "hoppper") return GPUArchitecture::HOPPER;

        return GPUArchitecture::UNKNOWN;
    }

    std::string architecture_to_string(GPUArchitecture arch) {
        switch (arch) {
            case GPUArchitecture::KEPLER: return "kepler";
            case GPUArchitecture::MAXWELL: return "maxwell";
            case GPUArchitecture::PASCAL: return "pascal";
            case GPUArchitecture::VOLTA: return "volta";
            case GPUArchitecture::TURING: return "turing";
            case GPUArchitecture::AMPERE: return "ampere";
            case GPUArchitecture::ADA_LOVELACE: return "ada_lovelace";
            case GPUArchitecture::HOPPER: return "hoppper";
            default: return "unknown";
        }
    }

    bool is_architecture_supported(GPUArchitecture arch) {
        return arch >= GPUArchitecture::TURING; // SM 7.5+
    }

    ComputeCapability get_minimum_compute_capability() {
        return {7, 5, "7.5"};
    }

    std::vector<GPUArchitecture> get_supported_architectures() {
        return {
            GPUArchitecture::TURING,
            GPUArchitecture::AMPERE,
            GPUArchitecture::ADA_LOVELACE,
            GPUArchitecture::HOPPER
        };
    }
}

} // namespace gpu
} // namespace keyhunt
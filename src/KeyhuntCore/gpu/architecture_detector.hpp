// Puzzle71Solver - GPU Architecture Detection and Validation Header
// Comprehensive GPU architecture detection and validation system (T013)

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>

// Add filesystem compatibility
#if defined(__cpp_lib_filesystem) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace keyhunt {
namespace gpu {

// GPU architecture enumeration
enum class GPUArchitecture {
    UNKNOWN = 0,
    KEPLER,     // SM 3.x (e.g., GTX 6xx/7xx)
    MAXWELL,    // SM 5.x (e.g., GTX 9xx)
    PASCAL,     // SM 6.x (e.g., GTX 10xx)
    VOLTA,      // SM 7.0 (e.g., V100)
    TURING,     // SM 7.5 (e.g., RTX 20xx)
    AMPERE,     // SM 8.x (e.g., RTX 30xx)
    ADA_LOVELACE, // SM 8.9 (e.g., RTX 40xx)
    HOPPER      // SM 9.x (e.g., H100)
};

// GPU capability levels
struct ComputeCapability {
    int major = 0;
    int minor = 0;
    std::string compute_capability_string; // "7.5", "8.6", etc.

    bool operator==(const ComputeCapability& other) const {
        return major == other.major && minor == other.minor;
    }

    bool operator<(const ComputeCapability& other) const {
        return major < other.major || (major == other.major && minor < other.minor);
    }

    std::string to_string() const {
        return std::to_string(major) + "." + std::to_string(minor);
    }

    bool is_supported() const {
        return (major == 7 && minor >= 5) || major >= 8; // SM 7.5+ or SM 8.x+
    }
};

// GPU device information
struct GPUDeviceInfo {
    int device_id = -1;
    std::string name;
    std::string brand;  // NVIDIA, AMD, Intel
    GPUArchitecture architecture = GPUArchitecture::UNKNOWN;
    ComputeCapability compute_capability;

    // Memory information
    size_t total_memory_mb = 0;
    size_t free_memory_mb = 0;
    size_t memory_bandwidth_gb_per_sec = 0;
    int memory_bus_width = 0;
    std::string memory_type;  // GDDR6, HBM2, etc.

    // Core specifications
    int sm_count = 0;
    int multiprocessor_count = 0;
    int max_threads_per_multiprocessor = 0;
    int max_threads_per_block = 0;
    int max_block_dim[3] = {0, 0, 0};
    int max_grid_dim[3] = {0, 0, 0};
    int warp_size = 32;

    // Cache specifications
    size_t l2_cache_size_bytes = 0;
    size_t shared_memory_per_block_bytes = 0;
    size_t total_shared_memory_per_sm_bytes = 0;
    int constant_memory_bytes = 65536;

    // Performance characteristics
    double base_clock_mhz = 0.0;
    double boost_clock_mhz = 0.0;
    double memory_clock_mhz = 0.0;
    double tflops_fp32 = 0.0;
    double tflops_fp16 = 0.0;
    double tflops_tensor = 0.0;

    // Feature support
    bool supports_cuda = false;
    bool supports_cublas = false;
    bool supports_cusparse = false;
    bool supports_cufft = false;
    bool supports_tensor_cores = false;
    bool supports_ray_tracing = false;
    bool supports_mig = false;
    bool supports_nvlink = false;

    // Driver information
    std::string driver_version;
    std::string cuda_version;
    std::string device_uuid;

    // Validation status
    bool is_validated = false;
    bool is_compatible = false;
    std::vector<std::string> validation_errors;
    std::vector<std::string> compatibility_warnings;
};

// Architecture-specific optimization profile
struct ArchitectureProfile {
    GPUArchitecture architecture;
    ComputeCapability min_compute_capability;
    ComputeCapability max_compute_capability;

    // Optimization parameters
    int optimal_block_size = 256;
    int registers_per_thread_target = 32;
    int max_registers_per_thread = 40;
    double occupancy_target_percent = 80.0;

    // Memory optimization
    int shared_memory_carveout_percent = 75;
    std::vector<int> preferred_block_sizes = {128, 256, 512};

    // Compiler flags
    std::vector<std::string> architecture_flags;
    std::vector<std::string> optimization_flags;

    // Performance characteristics
    double expected_memory_efficiency = 0.90;
    double expected_occupancy = 0.75;
    double expected_bandwidth_utilization = 0.85;
};

// Validation result
struct ValidationResult {
    bool is_valid = false;
    bool is_compatible = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::vector<std::string> recommendations;
    double compatibility_score = 0.0; // 0.0 to 1.0
};

// Main GPU architecture detector class
class ArchitectureDetector {
public:
    ArchitectureDetector();
    ~ArchitectureDetector() = default;

    // System initialization
    bool initialize();
    bool validate_system_requirements();

    // GPU detection and enumeration
    std::vector<GPUDeviceInfo> detect_all_gpus();
    GPUDeviceInfo detect_primary_gpu();
    GPUDeviceInfo get_gpu_info(int device_id);
    std::vector<GPUDeviceInfo> get_compatible_gpus();

    // Architecture detection
    GPUArchitecture detect_architecture(const std::string& gpu_name);
    GPUArchitecture detect_architecture_from_compute_capability(int major, int minor);
    ComputeCapability parse_compute_capability(const std::string& capability_str);

    // Validation and compatibility
    ValidationResult validate_gpu(const GPUDeviceInfo& gpu_info);
    ValidationResult validate_compute_capability(const ComputeCapability& cap);
    ValidationResult validate_system_compatibility();
    bool is_gpu_compatible(const GPUDeviceInfo& gpu_info);

    // Architecture profiles
    ArchitectureProfile get_architecture_profile(GPUArchitecture arch);
    ArchitectureProfile get_optimization_profile(const GPUDeviceInfo& gpu_info);
    std::vector<std::string> generate_compiler_flags(const GPUDeviceInfo& gpu_info);

    // Performance estimation
    double estimate_peak_performance(const GPUDeviceInfo& gpu_info);
    double estimate_memory_bandwidth(const GPUDeviceInfo& gpu_info);
    double estimate_theoretical_throughput(const GPUDeviceInfo& gpu_info);

    // Configuration and utilities
    bool load_architecture_profiles(const std::string& profile_file = "");
    bool save_detection_results(const std::string& output_file = "");
    bool load_detection_results(const std::string& input_file = "");

    // System information
    std::string get_cuda_driver_version();
    std::string get_cuda_runtime_version();
    int get_cuda_device_count();
    bool is_cuda_available();

    // Advanced features
    std::vector<GPUDeviceInfo> benchmark_gpus(); // Simple benchmark to validate detection
    bool compare_gpu_performance(const GPUDeviceInfo& gpu1, const GPUDeviceInfo& gpu2);
    std::string generate_system_report();

private:
    std::unordered_map<GPUArchitecture, ArchitectureProfile> architecture_profiles_;
    std::vector<GPUDeviceInfo> detected_gpus_;
    bool is_initialized_ = false;

    // Internal detection methods
    GPUDeviceInfo query_gpu_device(int device_id);
    bool query_cuda_device_properties(int device_id, GPUDeviceInfo& info);
    bool query_nvidia_smi_info(int device_id, GPUDeviceInfo& info);

    // Architecture identification
    GPUArchitecture identify_architecture_from_name(const std::string& name);
    GPUArchitecture identify_architecture_from_compute_capability(int major, int minor);

    // Validation helpers
    bool validate_minimum_requirements(const GPUDeviceInfo& gpu_info);
    bool validate_memory_requirements(const GPUDeviceInfo& gpu_info);
    bool validate_compute_capability_requirements(const GPUDeviceInfo& gpu_info);
    bool validate_driver_compatibility(const GPUDeviceInfo& gpu_info);

    // Profile management
    void initialize_default_profiles();
    void register_architecture_profile(const ArchitectureProfile& profile);

    // Utility methods
    std::string architecture_to_string(GPUArchitecture arch);
    std::string clean_gpu_name(const std::string& name);
    double parse_clock_speed(const std::string& clock_str);
    size_t parse_memory_size(const std::string& memory_str);
    std::string execute_command(const std::string& command);
    int get_cores_per_sm(int major, int minor);

    // Logging methods
    void log_info(const std::string& message) const;
    void log_warning(const std::string& message) const;
    void log_error(const std::string& message) const;
    void log_debug(const std::string& message) const;

    // Constants
    static const int MIN_COMPUTE_CAPABILITY_MAJOR = 7;
    static const int MIN_COMPUTE_CAPABILITY_MINOR = 5;
    static const size_t MIN_MEMORY_MB = 2048; // 2GB minimum
    static const int MIN_SM_COUNT = 10;
};

// Global detector instance
extern std::unique_ptr<ArchitectureDetector> g_architecture_detector;

// Convenience functions
bool initialize_gpu_detection();
std::vector<GPUDeviceInfo> get_available_gpus();
GPUDeviceInfo get_primary_gpu();
bool validate_system_gpu();
std::string get_system_gpu_report();

// GPU architecture utilities
namespace utils {
    GPUArchitecture string_to_architecture(const std::string& arch_str);
    std::string architecture_to_string(GPUArchitecture arch);
    bool is_architecture_supported(GPUArchitecture arch);
    ComputeCapability get_minimum_compute_capability();
    std::vector<GPUArchitecture> get_supported_architectures();

    // Performance prediction
    double predict_throughput_improvement(
        const GPUDeviceInfo& current_gpu,
        const GPUDeviceInfo& target_gpu
    );

    // Optimization recommendations
    std::vector<std::string> get_optimization_recommendations(const GPUDeviceInfo& gpu_info);
    std::vector<std::string> get_upgrade_recommendations(const GPUDeviceInfo& gpu_info);
}

} // namespace gpu
} // namespace keyhunt
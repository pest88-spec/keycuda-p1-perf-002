// Puzzle71Solver - GPU Utilization Analyzer
// Implements comprehensive GPU utilization measurement and analysis for T049

#include "gpu_utilization_analyzer.hpp"
#include "logging_utils.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <cmath>

using json = nlohmann::json;
using namespace std::chrono;

namespace keyhunt {
namespace performance {

GPUUtilizationAnalyzer::GPUUtilizationAnalyzer(int device_id)
    : device_id_(device_id), monitoring_active_(false),
      sampling_interval_ms_(100), measurement_duration_ms_(5000) {

    // Initialize NVML for detailed metrics
    nvml_result_ = nvmlInit();
    if (nvml_result_ == NVML_SUCCESS) {
        nvml_result_ = nvmlDeviceGetHandleByIndex(device_id_, &nvml_device_);
        if (nvml_result_ != NVML_SUCCESS) {
            log_error("Failed to get NVML device handle for device " + std::to_string(device_id_));
        }
    } else {
        log_warning("NVML initialization failed, using basic CUDA metrics only");
    }

    // Initialize CUDA device
    cudaError_t cuda_err = cudaSetDevice(device_id_);
    if (cuda_err != cudaSuccess) {
        log_error("Failed to set CUDA device " + std::to_string(device_id_) + ": " +
                 std::string(cudaGetErrorString(cuda_err)));
    }
}

GPUUtilizationAnalyzer::~GPUUtilizationAnalyzer() {
    stopContinuousMonitoring();
    if (nvml_result_ == NVML_SUCCESS) {
        nvmlShutdown();
    }
}

bool GPUUtilizationAnalyzer::initialize(int device_id) {
    device_id_ = device_id;

    // Re-initialize with new device ID
    nvml_result_ = nvmlInit();
    if (nvml_result_ == NVML_SUCCESS) {
        nvml_result_ = nvmlDeviceGetHandleByIndex(device_id_, &nvml_device_);
        if (nvml_result_ != NVML_SUCCESS) {
            log_error("Failed to get NVML device handle for device " + std::to_string(device_id_));
            return false;
        }
    }

    cudaError_t cuda_err = cudaSetDevice(device_id_);
    if (cuda_err != cudaSuccess) {
        log_error("Failed to set CUDA device " + std::to_string(device_id_) + ": " +
                 std::string(cudaGetErrorString(cuda_err)));
        return false;
    }

    log_info("GPU utilization analyzer initialized for device " + std::to_string(device_id_));
    return true;
}

bool GPUUtilizationAnalyzer::measureGPUUtilization(double& utilization_percent) {
    if (nvml_result_ != NVML_SUCCESS) {
        // Fallback to basic measurement if NVML is unavailable
        return measureBasicGPUUtilization(utilization_percent);
    }

    nvmlUtilization_t utilization;
    nvml_result_ = nvmlDeviceGetUtilizationRates(nvml_device_, &utilization);

    if (nvml_result_ != NVML_SUCCESS) {
        log_error("Failed to get GPU utilization: " + std::string(nvmlErrorString(nvml_result_)));
        return false;
    }

    utilization_percent = static_cast<double>(utilization.gpu);
    return true;
}

bool GPUUtilizationAnalyzer::measureComputeUtilization(double& compute_utilization) {
    if (nvml_result_ != NVML_SUCCESS) {
        return measureBasicComputeUtilization(compute_utilization);
    }

    nvmlUtilization_t utilization;
    nvml_result_ = nvmlDeviceGetUtilizationRates(nvml_device_, &utilization);

    if (nvml_result_ != NVML_SUCCESS) {
        log_error("Failed to get compute utilization: " + std::string(nvmlErrorString(nvml_result_)));
        return false;
    }

    compute_utilization = static_cast<double>(utilization.gpu);
    return true;
}

bool GPUUtilizationAnalyzer::measureMemoryControllerUtilization(double& mem_utilization) {
    if (nvml_result_ != NVML_SUCCESS) {
        return measureBasicMemoryUtilization(mem_utilization);
    }

    nvmlUtilization_t utilization;
    nvml_result_ = nvmlDeviceGetUtilizationRates(nvml_device_, &utilization);

    if (nvml_result_ != NVML_SUCCESS) {
        log_error("Failed to get memory controller utilization: " + std::string(nvmlErrorString(nvml_result_)));
        return false;
    }

    mem_utilization = static_cast<double>(utilization.memory);
    return true;
}

bool GPUUtilizationAnalyzer::measureSMUtilization(double& sm_utilization) {
    if (nvml_result_ != NVML_SUCCESS) {
        // Estimate SM utilization from compute utilization
        return measureComputeUtilization(sm_utilization);
    }

    // Try to get detailed metrics if available
    unsigned int sm_clock = 0;
    nvml_result_ = nvmlDeviceGetClockInfo(nvml_device_, NVML_CLOCK_SM, &sm_clock);

    if (nvml_result_ != NVML_SUCCESS) {
        return measureComputeUtilization(sm_utilization);
    }

    // Get max clock for percentage calculation
    unsigned int max_sm_clock = 0;
    nvml_result_ = nvmlDeviceGetMaxClockInfo(nvml_device_, NVML_CLOCK_SM, &max_sm_clock);

    if (nvml_result_ == NVML_SUCCESS && max_sm_clock > 0) {
        sm_utilization = (static_cast<double>(sm_clock) / max_sm_clock) * 100.0;
    } else {
        // Fallback to compute utilization
        return measureComputeUtilization(sm_utilization);
    }

    return true;
}

bool GPUUtilizationAnalyzer::measureOccupancy(double& occupancy_percent) {
    // Measure kernel launch occupancy using CUDA events
    cudaEvent_t start, stop;
    cudaError_t err = cudaEventCreate(&start);
    if (err != cudaSuccess) return false;

    err = cudaEventCreate(&stop);
    if (err != cudaSuccess) {
        cudaEventDestroy(start);
        return false;
    }

    // Launch a minimal kernel to measure occupancy characteristics
    const int block_size = 256;
    const int num_blocks = getOptimalBlockCount(block_size);

    cudaEventRecord(start);
    testOccupancyKernel<<<num_blocks, block_size>>>();
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float milliseconds = 0;
    err = cudaEventElapsedTime(&milliseconds, start, stop);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    if (err != cudaSuccess) {
        log_error("Failed to measure occupancy: " + std::string(cudaGetErrorString(err)));
        return false;
    }

    // Estimate occupancy based on kernel execution time and device capabilities
    int device;
    cudaGetDevice(&device);

    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, device);

    // Calculate theoretical max occupancy
    int max_blocks_per_sm = 0;
    int min_grid_size = 0;
    cudaOccupancyMaxPotentialBlockSize(&min_grid_size, &max_blocks_per_sm,
                                       testOccupancyKernel, 0, block_size);

    // Estimate actual occupancy
    int active_blocks = num_blocks;
    int total_sms = prop.multiProcessorCount;
    int max_active_blocks = max_blocks_per_sm * total_sms;

    occupancy_percent = (static_cast<double>(active_blocks) / max_active_blocks) * 100.0;
    occupancy_percent = std::min(occupancy_percent, 100.0);

    return true;
}

bool GPUUtilizationAnalyzer::measureWarpExecutionEfficiency(double& efficiency) {
    // This is an approximation since NVML doesn't provide direct warp efficiency metrics
    double compute_util = 0.0;
    double sm_util = 0.0;

    bool compute_success = measureComputeUtilization(compute_util);
    bool sm_success = measureSMUtilization(sm_util);

    if (!compute_success || !sm_success) {
        return false;
    }

    // Estimate warp efficiency based on compute vs SM utilization ratio
    efficiency = (compute_util / sm_util) * 95.0; // Assume 95% max efficiency
    efficiency = std::min(efficiency, 100.0);

    return true;
}

bool GPUUtilizationAnalyzer::measureInstructionThroughput(double& throughput) {
    // Measure instruction throughput using a compute-intensive kernel
    const int iterations = 1000000;
    const int block_size = 256;
    const int num_blocks = getOptimalBlockCount(block_size);

    cudaEvent_t start, stop;
    cudaError_t err = cudaEventCreate(&start);
    if (err != cudaSuccess) return false;

    err = cudaEventCreate(&stop);
    if (err != cudaSuccess) {
        cudaEventDestroy(start);
        return false;
    }

    // Launch compute-intensive kernel
    cudaEventRecord(start);
    instructionThroughputKernel<<<num_blocks, block_size>>>(iterations);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float milliseconds = 0.0f;
    err = cudaEventElapsedTime(&milliseconds, start, stop);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    if (err != cudaSuccess) {
        log_error("Failed to measure instruction throughput: " + std::string(cudaGetErrorString(err)));
        return false;
    }

    // Calculate instructions per second
    double seconds = milliseconds / 1000.0;
    double total_instructions = static_cast<double>(iterations * block_size * num_blocks);
    throughput = total_instructions / seconds;

    return true;
}

bool GPUUtilizationAnalyzer::startContinuousMonitoring() {
    if (monitoring_active_) {
        log_warning("Continuous monitoring already active");
        return false;
    }

    monitoring_active_ = true;
    utilization_time_series_.clear();

    monitoring_thread_ = std::thread(&GPUUtilizationAnalyzer::monitoringLoop, this);

    log_info("Started continuous GPU utilization monitoring");
    return true;
}

bool GPUUtilizationAnalyzer::stopContinuousMonitoring() {
    if (!monitoring_active_) {
        return false;
    }

    monitoring_active_ = false;

    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }

    log_info("Stopped continuous GPU utilization monitoring");
    return true;
}

bool GPUUtilizationAnalyzer::getUtilizationTimeSeries(std::vector<std::pair<double, double>>& time_series) {
    if (utilization_time_series_.empty()) {
        return false;
    }

    time_series = utilization_time_series_;
    return true;
}

bool GPUUtilizationAnalyzer::generateUtilizationReport(std::string& report) {
    if (utilization_time_series_.empty()) {
        report = "No utilization data available for report generation";
        return false;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    // Calculate statistics
    double total_utilization = 0.0;
    double max_utilization = 0.0;
    double min_utilization = 100.0;

    for (const auto& data_point : utilization_time_series_) {
        total_utilization += data_point.second;
        max_utilization = std::max(max_utilization, data_point.second);
        min_utilization = std::min(min_utilization, data_point.second);
    }

    double avg_utilization = total_utilization / utilization_time_series_.size();

    // Calculate variance for stability analysis
    double variance = 0.0;
    for (const auto& data_point : utilization_time_series_) {
        variance += (data_point.second - avg_utilization) * (data_point.second - avg_utilization);
    }
    variance /= utilization_time_series_.size();
    double std_deviation = sqrt(variance);
    double coefficient_of_variation = (std_deviation / avg_utilization) * 100.0;

    // Generate report
    oss << "=== GPU Utilization Analysis Report ===\n";
    oss << "Device ID: " << device_id_ << "\n";
    oss << "Measurement Duration: " << (utilization_time_series_.size() * sampling_interval_ms_ / 1000.0) << " seconds\n";
    oss << "Sampling Interval: " << sampling_interval_ms_ << " ms\n";
    oss << "Total Samples: " << utilization_time_series_.size() << "\n\n";

    oss << "Utilization Statistics:\n";
    oss << "  Average: " << avg_utilization << "%\n";
    oss << "  Maximum: " << max_utilization << "%\n";
    oss << "  Minimum: " << min_utilization << "%\n";
    oss << "  Range: " << (max_utilization - min_utilization) << "%\n";
    oss << "  Std Deviation: " << std_deviation << "%\n";
    oss << "  Coefficient of Variation: " << coefficient_of_variation << "%\n\n";

    // Current measurements
    double current_gpu_util = 0.0, current_compute_util = 0.0, current_mem_util = 0.0;
    double current_occupancy = 0.0, current_warp_eff = 0.0, current_throughput = 0.0;

    measureGPUUtilization(current_gpu_util);
    measureComputeUtilization(current_compute_util);
    measureMemoryControllerUtilization(current_mem_util);
    measureOccupancy(current_occupancy);
    measureWarpExecutionEfficiency(current_warp_eff);
    measureInstructionThroughput(current_throughput);

    oss << "Current Real-Time Metrics:\n";
    oss << "  GPU Utilization: " << current_gpu_util << "%\n";
    oss << "  Compute Utilization: " << current_compute_util << "%\n";
    oss << "  Memory Controller Utilization: " << current_mem_util << "%\n";
    oss << "  SM Utilization: " << current_occupancy << "%\n";
    oss << "  Warp Execution Efficiency: " << current_warp_eff << "%\n";
    oss << "  Instruction Throughput: " << current_throughput << " ops/sec\n\n";

    // Performance assessment
    oss << "Performance Assessment:\n";

    // Constitutional compliance checks
    const double GPU_UTIL_MIN = 70.0;
    const double OCCUPANCY_TARGET = 65.0;
    const double EFFICIENCY_TARGET = 90.0;

    bool gpu_util_ok = avg_utilization >= GPU_UTIL_MIN;
    bool occupancy_ok = current_occupancy >= OCCUPANCY_TARGET;
    bool efficiency_ok = current_warp_eff >= EFFICIENCY_TARGET;
    bool stability_ok = coefficient_of_variation < 10.0;

    oss << "  Constitutional Compliance:\n";
    oss << "    GPU Utilization (≥" << GPU_UTIL_MIN << "%): " << (gpu_util_ok ? "✅ PASS" : "❌ FAIL") << "\n";
    oss << "    Occupancy (≥" << OCCUPANCY_TARGET << "%): " << (occupancy_ok ? "✅ PASS" : "❌ FAIL") << "\n";
    oss << "    Warp Efficiency (≥" << EFFICIENCY_TARGET << "%): " << (efficiency_ok ? "✅ PASS" : "❌ FAIL") << "\n";
    oss << "    Stability (CV < 10%): " << (stability_ok ? "✅ PASS" : "❌ FAIL") << "\n\n";

    // Recommendations
    oss << "Recommendations:\n";
    if (!gpu_util_ok) {
        oss << "  - GPU utilization is below target. Consider optimizing kernel launch parameters.\n";
    }
    if (!occupancy_ok) {
        oss << "  - Occupancy is below target. Review thread block size and resource usage.\n";
    }
    if (!efficiency_ok) {
        oss << "  - Warp execution efficiency is low. Check for divergent branches or memory access patterns.\n";
    }
    if (!stability_ok) {
        oss << "  - Utilization is unstable. Investigate system load or thermal throttling.\n";
    }
    if (gpu_util_ok && occupancy_ok && efficiency_ok && stability_ok) {
        oss << "  - All performance targets met. GPU utilization is optimal.\n";
    }

    report = oss.str();
    return true;
}

// Private methods implementation

void GPUUtilizationAnalyzer::monitoringLoop() {
    auto start_time = high_resolution_clock::now();

    while (monitoring_active_) {
        auto current_time = high_resolution_clock::now();
        auto elapsed = duration_cast<milliseconds>(current_time - start_time).count();
        double elapsed_seconds = elapsed / 1000.0;

        double utilization = 0.0;
        if (measureGPUUtilization(utilization)) {
            utilization_time_series_.emplace_back(elapsed_seconds, utilization);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(sampling_interval_ms_));
    }
}

bool GPUUtilizationAnalyzer::measureBasicGPUUtilization(double& utilization_percent) {
    // Basic GPU utilization measurement using CUDA events
    cudaEvent_t start, stop;
    cudaError_t err = cudaEventCreate(&start);
    if (err != cudaSuccess) return false;

    err = cudaEventCreate(&stop);
    if (err != cudaSuccess) {
        cudaEventDestroy(start);
        return false;
    }

    // Measure GPU busy time
    const int iterations = 10000;
    const int block_size = 256;
    const int num_blocks = getOptimalBlockCount(block_size);

    cudaEventRecord(start);
    basicWorkloadKernel<<<num_blocks, block_size>>>(iterations);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float milliseconds = 0.0f;
    err = cudaEventElapsedTime(&milliseconds, start, stop);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    if (err != cudaSuccess) {
        return false;
    }

    // Estimate utilization based on execution time
    // This is a rough approximation when NVML is not available
    utilization_percent = std::min(90.0, (1000.0 / milliseconds) * 10.0);
    return true;
}

bool GPUUtilizationAnalyzer::measureBasicComputeUtilization(double& compute_utilization) {
    return measureBasicGPUUtilization(compute_utilization);
}

bool GPUUtilizationAnalyzer::measureBasicMemoryUtilization(double& mem_utilization) {
    // Basic memory utilization estimation
    size_t free_mem = 0, total_mem = 0;
    cudaError_t err = cudaMemGetInfo(&free_mem, &total_mem);

    if (err != cudaSuccess) {
        return false;
    }

    double used_mem = total_mem - free_mem;
    mem_utilization = (used_mem / total_mem) * 100.0;
    return true;
}

int GPUUtilizationAnalyzer::getOptimalBlockCount(int block_size) {
    int device;
    cudaGetDevice(&device);

    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, device);

    // Calculate optimal block count based on device properties
    int num_sms = prop.multiProcessorCount;
    int max_blocks_per_sm = 0;

    cudaOccupancyMaxActiveBlocksPerMultiprocessor(&max_blocks_per_sm,
                                                  basicWorkloadKernel,
                                                  block_size, 0);

    return num_sms * max_blocks_per_sm * 2; // 2x for good measure
}

// CUDA kernel implementations

__global__ void GPUUtilizationAnalyzer::testOccupancyKernel() {
    // Simple kernel for occupancy measurement
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    volatile int dummy = 0;

    // Do some work to keep threads active
    for (int i = 0; i < 100; ++i) {
        dummy += i * idx;
    }
}

__global__ void GPUUtilizationAnalyzer::instructionThroughputKernel(int iterations) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    volatile double result = 1.0;

    // Compute-intensive operations for instruction throughput measurement
    for (int i = 0; i < iterations; ++i) {
        result = result * 1.000001 + 0.000001;
        result = sqrt(result) * result;
        result = sin(result) + cos(result);
    }
}

__global__ void GPUUtilizationAnalyzer::basicWorkloadKernel(int iterations) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    volatile int sum = 0;

    // Basic workload for utilization measurement
    for (int i = 0; i < iterations; ++i) {
        sum += i * idx;
    }
}

} // namespace performance
} // namespace keyhunt
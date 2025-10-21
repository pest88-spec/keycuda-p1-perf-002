/**
 * @file nsight_profiler.cuh
 * @brief NVIDIA Nsight Compute profiling integration system for Puzzle71 Technical Debt Repair
 *
 * This file implements a comprehensive NVIDIA Nsight Compute profiling integration system
 * that provides automated kernel profiling, metric collection, and performance analysis.
 * The system focuses on:
 *
 * - Automated Nsight Compute profiling workflow management
 * - Real-time kernel performance metric collection
 * - Advanced GPU performance analysis and bottleneck identification
 * - Automated profiling report generation and analysis
 * - Integration with CI/CD pipeline for continuous performance monitoring
 * - Multi-GPU profiling support and orchestration
 * - Performance regression detection using profiling data
 * - Automated optimization recommendations based on profiling results
 *
 * @author Puzzle71 Technical Debt Repair System
 * @version 5.5
 * @date 2025-10-21
 * @copyright Constitutional Compliance v5.5
 */

#pragma once

#include <cuda_runtime.h>
#include <cuda_profiler_api.h>
#include <nvtx3/nvToolsExt.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <fstream>
#include <chrono>
#include <atomic>
#include "performance_telemetry.cuh"
#include "adaptive_gpu_utilization.cuh"

namespace keyhunt {
namespace profiling {
namespace nsight {

// ============================================================================
// NSIGHT PROFILER CONFIGURATION CONSTANTS
// ============================================================================

/**
 * Nsight Compute profiling configuration
 */
constexpr uint32_t DEFAULT_PROFILING_ITERATIONS = 10;
constexpr uint32_t WARMUP_ITERATIONS = 3;
constexpr uint32_t MAX_PROFILED_KERNELS = 64;
constexpr uint32_t MAX_PROFILED_METRICS = 100;
constexpr double PROFILING_OVERHEAD_TOLERANCE = 0.05; // 5% overhead tolerance
constexpr std::chrono::milliseconds DEFAULT_PROFILING_TIMEOUT(30000); // 30s

/**
 * Nsight Compute metric categories
 */
enum class MetricCategory : uint32_t {
    COMPUTE = 0,           // Compute throughput and utilization
    MEMORY = 1,            // Memory bandwidth and access patterns
    CACHE = 2,             // Cache hit rates and efficiency
    SHARED_MEMORY = 3,     // Shared memory utilization and conflicts
    INSTRUCTIONS = 4,      // Instruction throughput and efficiency
    WARPS = 5,             // Warp execution efficiency
    OCCUPANCY = 6,         // Thread block occupancy
    POWER = 7,             // Power consumption and efficiency
    LATENCY = 8,           // Memory instruction latency
    THROUGHPUT = 9         // Overall throughput metrics
};

/**
 * Profiling result structure
 */
struct alignas(64) KernelProfilingResult {
    std::string kernel_name;
    std::string device_name;
    uint32_t kernel_id;
    std::chrono::system_clock::time_point profiling_timestamp;

    // Performance metrics
    double kernel_execution_time_ms;
    double average_block_time_ms;
    double min_block_time_ms;
    double max_block_time_ms;
    double kernel_launch_overhead_ms;

    // Compute metrics
    double compute_throughput_gflops;
    double fp32_instructions_per_cycle;
    double fp64_instructions_per_cycle;
    double instruction_throughput_mips;
    double compute_utilization_percentage;

    // Memory metrics
    double global_memory_bandwidth_gbps;
    double shared_memory_bandwidth_gbps;
    double l2_cache_bandwidth_gbps;
    double memory_throughput_gbps;
    double global_memory_efficiency_percentage;

    // Cache metrics
    double l1_cache_hit_rate_percentage;
    double l2_cache_hit_rate_percentage;
    double texture_cache_hit_rate_percentage;
    double shared_memory_hit_rate_percentage;

    // Shared memory metrics
    double shared_memory_utilization_percentage;
    double shared_memory_bank_conflicts_percentage;
    double shared_memory_efficiency_percentage;

    // Warp execution metrics
    double warp_execution_efficiency_percentage;
    double warp_issue_efficiency_percentage;
    double active_warps_per_sm;
    double achieved_occupancy_percentage;

    // Occupancy metrics
    double theoretical_occupancy_percentage;
    double achieved_occupancy_percentage;
    uint32_t active_blocks_per_sm;
    uint32_t threads_per_block;

    // Power metrics
    double power_consumption_watts;
    double energy_per_kernel_joules;
    double performance_per_watt_gflops_per_watt;

    // Instruction metrics
    uint64_t total_instructions_executed;
    uint64_t branch_instructions;
    uint64_t load_instructions;
    uint64_t store_instructions;
    double instruction_reuse_rate;

    // Latency metrics
    double average_memory_latency_ns;
    double average_instruction_latency_ns;
    double max_register_pressure;
    double register_efficiency_percentage;

    // Throughput metrics
    uint64_t elements_processed_per_second;
    uint64_t bytes_processed_per_second;
    double operations_per_second;

    // Quality metrics
    double overall_performance_score;
    std::vector<std::string> optimization_recommendations;
    std::vector<std::string> identified_bottlenecks;

    KernelProfilingResult() : kernel_id(0), kernel_execution_time_ms(0.0),
                             overall_performance_score(0.0) {}
};

/**
 * Nsight Compute profiling configuration
 */
struct NsightProfileConfig {
    std::string application_name;
    std::vector<std::string> kernel_names_to_profile;
    std::vector<MetricCategory> metric_categories;
    uint32_t profiling_iterations;
    uint32_t warmup_iterations;
    bool enable_detailed_metrics;
    bool enable_kernel_trace;
    bool enable_memory_trace;
    bool enable_nvtx_markers;
    std::string output_directory;
    std::string report_format; // "json", "csv", "html"
    std::chrono::milliseconds profiling_timeout;
    double overhead_tolerance;

    NsightProfileConfig() : profiling_iterations(DEFAULT_PROFILING_ITERATIONS),
                           warmup_iterations(WARMUP_ITERATIONS),
                           enable_detailed_metrics(true),
                           enable_kernel_trace(false),
                           enable_memory_trace(false),
                           enable_nvtx_markers(true),
                           report_format("json"),
                           profiling_timeout(DEFAULT_PROFILING_TIMEOUT),
                           overhead_tolerance(PROFILING_OVERHEAD_TOLERANCE) {}
};

/**
 * Profiled kernel specification
 */
struct ProfiledKernel {
    std::string kernel_name;
    void* kernel_function;
    uint32_t grid_size;
    uint32_t block_size;
    uint32_t shared_memory_size;
    std::vector<void*> kernel_args;
    std::vector<size_t> arg_sizes;
    cudaStream_t stream;
    std::chrono::high_resolution_clock::time_point launch_time;

    ProfiledKernel() : kernel_function(nullptr), grid_size(0), block_size(0),
                      shared_memory_size(0), stream(0) {}
};

// ============================================================================
// NSIGHT COMPUTE PROFILER MANAGER
// ============================================================================

/**
 * Main Nsight Compute profiler manager
 */
class NsightProfilerManager {
private:
    NsightProfileConfig config_;
    std::vector<KernelProfilingResult> profiling_results_;
    std::vector<ProfiledKernel> registered_kernels_;
    std::map<std::string, uint32_t> kernel_name_to_id_;
    std::atomic<bool> profiling_active_;
    std::atomic<uint32_t> current_profiling_iteration_;
    std::chrono::high_resolution_clock::time_point profiling_start_time_;

    // CUDA profiler integration
    cudaProfiler_t cuda_profiler_handle_;
    bool cuda_profiler_initialized_;

    // NVTX for custom profiling ranges
    uint32_t nvtx_domain_handle_;

    // Performance baseline for comparison
    std::map<std::string, KernelProfilingResult> performance_baselines_;

    // Profiling statistics
    uint64_t total_kernels_profiled_;
    double total_profiling_overhead_;
    std::chrono::milliseconds total_profiling_time_;

public:
    /**
     * Constructor for Nsight profiler manager
     */
    explicit NsightProfilerManager(const NsightProfileConfig& config = NsightProfileConfig());

    /**
     * Destructor
     */
    ~NsightProfilerManager();

    /**
     * Initialize the profiling system
     */
    bool initialize();

    /**
     * Start profiling session
     */
    bool startProfiling();

    /**
     * Stop profiling session and collect results
     */
    void stopProfiling();

    /**
     * Register a kernel for profiling
     */
    uint32_t registerKernel(
        const std::string& kernel_name,
        void* kernel_function,
        uint32_t grid_size,
        uint32_t block_size,
        uint32_t shared_memory_size = 0,
        cudaStream_t stream = 0
    );

    /**
     * Set kernel arguments for profiling
     */
    bool setKernelArguments(uint32_t kernel_id, const std::vector<void*>& args, const std::vector<size_t>& arg_sizes);

    /**
     * Profile a single kernel
     */
    KernelProfilingResult profileKernel(uint32_t kernel_id);

    /**
     * Profile all registered kernels
     */
    std::vector<KernelProfilingResult> profileAllKernels();

    /**
     * Profile kernel with custom parameters
     */
    KernelProfilingResult profileKernelWithParams(
        const std::string& kernel_name,
        void* kernel_function,
        uint32_t grid_size,
        uint32_t block_size,
        uint32_t shared_memory_size,
        const std::vector<void*>& args,
        const std::vector<size_t>& arg_sizes,
        cudaStream_t stream = 0
    );

    /**
     * Get profiling results
     */
    const std::vector<KernelProfilingResult>& getProfilingResults() const { return profiling_results_; }

    /**
     * Get profiling result for specific kernel
     */
    KernelProfilingResult getKernelResult(const std::string& kernel_name) const;

    /**
     * Export profiling results to file
     */
    bool exportResults(const std::string& filename, const std::string& format = "json");

    /**
     * Generate performance report
     */
    std::string generatePerformanceReport();

    /**
     * Identify performance bottlenecks
     */
    std::vector<std::pair<std::string, double>> identifyBottlenecks();

    /**
     * Generate optimization recommendations
     */
    std::vector<std::string> generateOptimizationRecommendations(const std::string& kernel_name);

    /**
     * Set performance baseline for comparison
     */
    bool setPerformanceBaseline(const std::string& kernel_name, const KernelProfilingResult& baseline);

    /**
     * Check for performance regression
     */
    std::vector<std::string> detectPerformanceRegression(double tolerance_percentage = 5.0);

    /**
     * Get profiling statistics
     */
    void getProfilingStatistics(uint64_t& total_kernels, double& overhead_percentage,
                               std::chrono::milliseconds& total_time) const;

    /**
     * Configure profiling settings
     */
    void updateConfig(const NsightProfileConfig& new_config);

private:
    /**
     * Initialize CUDA profiler
     */
    bool initializeCudaProfiler();

    /**
     * Initialize NVTX domains
     */
    void initializeNVTX();

    /**
     * Collect detailed kernel metrics
     */
    KernelProfilingResult collectKernelMetrics(const ProfiledKernel& kernel);

    /**
     * Calculate performance metrics from raw data
     */
    void calculatePerformanceMetrics(KernelProfilingResult& result, const ProfiledKernel& kernel);

    /**
     * Analyze memory access patterns
     */
    void analyzeMemoryAccessPatterns(KernelProfilingResult& result);

    /**
     * Analyze cache performance
     */
    void analyzeCachePerformance(KernelProfilingResult& result);

    /**
     * Analyze warp execution efficiency
     */
    void analyzeWarpEfficiency(KernelProfilingResult& result);

    /**
     * Calculate overall performance score
     */
    double calculatePerformanceScore(const KernelProfilingResult& result);

    /**
     * Generate optimization recommendations
     */
    std::vector<std::string> generateRecommendations(const KernelProfilingResult& result);

    /**
     * Format profiling results for export
     */
    std::string formatResults(const std::string& format);

    /**
     * Cleanup profiler resources
     */
    void cleanup();
};

// ============================================================================
// AUTOMATED PROFILING WORKFLOW
// ============================================================================

/**
 * Automated profiling workflow manager
 */
class AutomatedProfilingWorkflow {
private:
    std::unique_ptr<NsightProfilerManager> profiler_;
    std::vector<std::string> kernel_names_;
    std::vector<std::map<std::string, uint32_t>> kernel_launch_configs_;
    std::map<std::string, std::vector<std::vector<void*>>> kernel_arg_sets_;
    bool workflow_active_;
    std::string workflow_name_;

public:
    /**
     * Constructor for automated profiling workflow
     */
    explicit AutomatedProfilingWorkflow(const std::string& workflow_name);

    /**
     * Add kernel to profiling workflow
     */
    bool addKernel(
        const std::string& kernel_name,
        const std::map<std::string, uint32_t>& launch_configs,
        const std::vector<std::vector<void*>>& arg_sets = {}
    );

    /**
     * Execute complete profiling workflow
     */
    bool executeWorkflow();

    /**
     * Execute workflow with custom configurations
     */
    bool executeWorkflow(const NsightProfileConfig& config);

    /**
     * Get workflow results
     */
    std::vector<KernelProfilingResult> getWorkflowResults();

    /**
     * Generate workflow report
     */
    std::string generateWorkflowReport();

    /**
     * Export workflow results
     */
    bool exportWorkflowResults(const std::string& base_filename);

private:
    /**
     * Execute kernel with all configurations
     */
    void profileKernelWithAllConfigs(const std::string& kernel_name);

    /**
     * Analyze workflow performance trends
     */
    void analyzeWorkflowTrends();
};

// ============================================================================
// PROFILING ANALYSIS TOOLS
// ============================================================================

/**
 * Advanced profiling analysis tools
 */
class ProfilingAnalyzer {
private:
    std::vector<KernelProfilingResult> profiling_results_;
    std::map<std::string, std::vector<KernelProfilingResult>> kernel_history_;

public:
    /**
     * Constructor for profiling analyzer
     */
    ProfilingAnalyzer() = default;

    /**
     * Add profiling results for analysis
     */
    void addResults(const std::vector<KernelProfilingResult>& results);

    /**
     * Add historical results for trend analysis
     */
    void addHistoricalResults(const std::string& kernel_name, const KernelProfilingResult& result);

    /**
     * Perform comprehensive performance analysis
     */
    std::map<std::string, std::vector<std::string>> performAnalysis();

    /**
     * Compare performance between runs
     */
    std::map<std::string, double> comparePerformanceRuns(const std::string& run1, const std::string& run2);

    /**
     * Identify performance optimization opportunities
     */
    std::vector<std::pair<std::string, std::vector<std::string>>> identifyOptimizationOpportunities();

    /**
     * Generate performance trends report
     */
    std::string generateTrendsReport();

private:
    /**
     * Analyze compute efficiency trends
     */
    void analyzeComputeTrends(const std::string& kernel_name, std::vector<std::string>& insights);

    /**
     * Analyze memory access patterns
     */
    void analyzeMemoryPatterns(const std::string& kernel_name, std::vector<std::string>& insights);

    /**
     * Analyze cache performance
     */
    void analyzeCachePerformance(const std::string& kernel_name, std::vector<std::string>& insights);

    /**
     * Analyze occupancy patterns
     */
    void analyzeOccupancyPatterns(const std::string& kernel_name, std::vector<std::string>& insights);
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Convert metric category to string
 */
inline const char* metricCategoryToString(MetricCategory category) {
    switch (category) {
        case MetricCategory::COMPUTE: return "Compute";
        case MetricCategory::MEMORY: return "Memory";
        case MetricCategory::CACHE: return "Cache";
        case MetricCategory::SHARED_MEMORY: return "SharedMemory";
        case MetricCategory::INSTRUCTIONS: return "Instructions";
        case MetricCategory::WARPS: return "Warps";
        case MetricCategory::OCCUPANCY: return "Occupancy";
        case MetricCategory::POWER: return "Power";
        case MetricCategory::LATENCY: return "Latency";
        case MetricCategory::THROUGHPUT: return "Throughput";
        default: return "Unknown";
    }
}

/**
 * Create default Nsight profile configuration
 */
NsightProfileConfig createDefaultProfileConfig(const std::string& app_name = "Puzzle71");

/**
 * Validate profiling results integrity
 */
bool validateProfilingResults(const KernelProfilingResult& result);

/**
 * Check if profiling results meet performance targets
 */
bool meetsPerformanceTargets(const KernelProfilingResult& result);

/**
 * Calculate performance improvement percentage
 */
double calculatePerformanceImprovement(const KernelProfilingResult& baseline, const KernelProfilingResult& current);

} // namespace nsight
} // namespace profiling
} // namespace keyhunt
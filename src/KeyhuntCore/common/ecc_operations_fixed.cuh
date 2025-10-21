// Puzzle71 Technical Debt Repair - Fixed ECC Operations Module
// Addresses P0/blocking and P1/high priority issues from v5.5 technical debt audit
// Implements T022: ECC batch operations with Structure-of-Arrays layout and high precision

#pragma once

#include <cstdint>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <vector>

namespace keyhunt {
namespace ecc {

// Device constants are defined in the .cu file

// Forward declarations for device functions (implemented in .cu file)
__device__ void addModP(const unsigned int a[8], const unsigned int b[8], unsigned int c[8]);
__device__ void subModP(const unsigned int a[8], const unsigned int b[8], unsigned int c[8]);
__device__ void mulModP(const unsigned int a[8], const unsigned int b[8], unsigned int c[8]);
__device__ void modInv(const unsigned int a[8], unsigned int result[8]);
__device__ void copyBigInt(const unsigned int src[8], unsigned int dst[8]);
__device__ void ReadBigInt(const unsigned int* ara, int idx, unsigned int x[8]);
__device__ void WriteBigInt(unsigned int* ara, int idx, const unsigned int x[8]);

// Additional ECC point operation declarations
__device__ void point_add(const unsigned int p_x[8], const unsigned int p_y[8],
                         const unsigned int q_x[8], const unsigned int q_y[8],
                         unsigned int r_x[8], unsigned int r_y[8]);

__device__ void point_double(const unsigned int p_x[8], const unsigned int p_y[8],
                            unsigned int r_x[8], unsigned int r_y[8]);

__device__ void scalar_multiply(const unsigned int k[8],
                               unsigned int r_x[8], unsigned int r_y[8]);

__device__ bool point_on_curve(const unsigned int x[8], const unsigned int y[8]);

// ECC device functions
__device__ void doBatchInverse_Fixed(unsigned int accumulator[8]);
__device__ void BeginBatchPointAdd_Fixed(
    const unsigned int incX[8],
    const unsigned int incY[8],
    unsigned int* xPtr,
    unsigned int* chain,
    int srcIdx,
    int dstIdx,
    unsigned int accumulator[8]
);
__device__ void CompleteBatchPointAdd_Fixed(
    const unsigned int incX[8],
    const unsigned int incY[8],
    unsigned int* xPtr,
    unsigned int* yPtr,
    int srcIdx,
    int dstIdx,
    unsigned int* chain,
    const unsigned int accumulator[8],
    unsigned int resultX[8],
    unsigned int resultY[8]
);

// Structure-of-Arrays layout for ECC points (optimized for coalescing)
struct alignas(128) ECCPointSoA {
    // X coordinates (separate array for memory coalescing)
    uint32_t* x_words;      // 8 words per 256-bit coordinate
    // Y coordinates (separate array for memory coalescing)
    uint32_t* y_words;      // 8 words per 256-bit coordinate
    // Validity flags
    bool* is_valid;
    // Size of arrays
    size_t size;

    __host__ __device__ ECCPointSoA() : x_words(nullptr), y_words(nullptr),
                                      is_valid(nullptr), size(0) {}
};

// Batch ECC operations configuration
struct ECCBatchConfig {
    size_t batch_size;
    bool use_montgomery;
    bool use_fixed_point;
    double precision_target;  // Must be <1e-10 for CPU/GPU consistency
    int cuda_device_id;

    // Memory layout optimization (T023 requirements)
    bool use_soa_layout;        // Structure-of-Arrays required
    size_t alignment_bytes;     // 128-byte alignment required
    bool enable_shared_memory;  // Shared memory optimization

    // Performance optimization
    int registers_per_thread;   // ≤32 registers/thread for high occupancy
    int threads_per_block;      // Typically 256 threads
    int shared_memory_size;     // Optimized per GPU architecture
};

// ECC operation results with detailed metrics
struct ECCOperationResult {
    bool success;
    cudaError_t cuda_error;
    size_t successful_operations;
    size_t failed_operations;
    double execution_time_ms;
    double throughput_ops_per_sec;
    double memory_efficiency_percent;  // Must exceed 90%
    double gpu_utilization_percent;    // Must exceed 70%
    double precision_achieved;         // Must be <1e-10
};

// Elliptic curve point operations class (T022/T023)
class ECCOperationsFixed {
public:
    // Constructor and destructor
    ECCOperationsFixed();
    ~ECCOperationsFixed();

    // Initialization with configuration validation
    bool initialize(const ECCBatchConfig& config);
    void cleanup();

    // Batch scalar multiplication: P = k * G (Core ECC operation)
    bool scalar_multiply_batch(const uint32_t* private_keys,      // Input: private keys
                               ECCPointSoA* public_keys,          // Output: public keys
                               size_t batch_size,
                               ECCOperationResult& result);

    // Batch point addition: R = P + Q
    bool point_addition_batch(const ECCPointSoA* points_p,      // Input: points P
                             const ECCPointSoA* points_q,      // Input: points Q
                             ECCPointSoA* points_r,            // Output: points R
                             size_t batch_size,
                             ECCOperationResult& result);

    // Batch point doubling: R = 2 * P
    bool point_doubling_batch(const ECCPointSoA* points_p,      // Input: points P
                             ECCPointSoA* points_r,            // Output: points R
                             size_t batch_size,
                             ECCOperationResult& result);

    // Batch point validation
    bool validate_points_batch(const ECCPointSoA* points,       // Input: points to validate
                              bool* validation_results,        // Output: validation flags
                              size_t batch_size,
                              ECCOperationResult& result);

    // Memory management for SoA layout
    bool allocate_soa_points(ECCPointSoA* points, size_t size);
    bool free_soa_points(ECCPointSoA* points);
    bool copy_to_device(const void* host_data, void* device_data, size_t size);
    bool copy_to_host(const void* device_data, void* host_data, size_t size);

    // Performance and optimization (meets T023 requirements)
    bool optimize_memory_layout();
    bool enable_shared_memory_optimization();
    bool benchmark_operations(double& throughput, double& efficiency);

    // CPU/GPU consistency validation
    bool validate_against_cpu_reference(const uint32_t* private_keys,
                                        const ECCPointSoA* gpu_public_keys,
                                        size_t batch_size,
                                        double& max_relative_error);

    // Error handling
    const char* get_last_error() const;
    cudaError_t get_last_cuda_error() const;

    // Configuration access
    const ECCBatchConfig& get_config() const { return config_; }
    bool is_initialized() const { return initialized_; }

private:
    // Internal state
    ECCBatchConfig config_;
    bool initialized_;
    char last_error_[256];
    cudaError_t last_cuda_error_;

    // CUDA resources
    void* device_workspace_;
    size_t workspace_size_;
    cudaStream_t cuda_stream_;

    // Kernel launch configuration (static, no runtime queries)
    dim3 block_dim_;
    dim3 grid_dim_;
    size_t shared_memory_size_;

    // Internal helper methods
    bool validate_config(const ECCBatchConfig& config);
    bool setup_cuda_resources();
    bool calculate_launch_parameters(size_t batch_size);
    void update_error(const char* error, cudaError_t cuda_err = cudaSuccess);

    // Memory optimization helpers (T023)
    bool setup_soa_memory_layout();
    bool optimize_shared_memory_usage();
    bool verify_memory_coalescing();
    bool check_memory_efficiency();

    // Performance measurement
    bool measure_performance_metrics(ECCOperationResult& result);
    double calculate_memory_efficiency();
    double calculate_gpu_utilization();
};

// CUDA kernel functions (T023 implementation)

// Scalar multiplication kernel with SoA layout
__global__ void ecc_scalar_mul_kernel(
    const uint32_t* __restrict__ private_keys,
    uint32_t* __restrict__ public_keys_x,
    uint32_t* __restrict__ public_keys_y,
    bool* __restrict__ is_valid,
    size_t batch_size,
    const ECCBatchConfig* config
);

// Point addition kernel with SoA layout
__global__ void ecc_point_add_kernel(
    const uint32_t* __restrict__ points_p_x,
    const uint32_t* __restrict__ points_p_y,
    const uint32_t* __restrict__ points_q_x,
    const uint32_t* __restrict__ points_q_y,
    uint32_t* __restrict__ points_r_x,
    uint32_t* __restrict__ points_r_y,
    bool* __restrict__ is_valid,
    size_t batch_size,
    const ECCBatchConfig* config
);

// Point doubling kernel with SoA layout
__global__ void ecc_point_double_kernel(
    const uint32_t* __restrict__ points_p_x,
    const uint32_t* __restrict__ points_p_y,
    uint32_t* __restrict__ points_r_x,
    uint32_t* __restrict__ points_r_y,
    bool* __restrict__ is_valid,
    size_t batch_size,
    const ECCBatchConfig* config
);

// Point validation kernel
__global__ void ecc_point_validate_kernel(
    const uint32_t* __restrict__ points_x,
    const uint32_t* __restrict__ points_y,
    bool* __restrict__ is_valid,
    size_t batch_size,
    const ECCBatchConfig* config
);

// Memory optimization kernels
__global__ void memory_coalescing_test_kernel(
    const uint32_t* __restrict__ input_data,
    uint32_t* __restrict__ output_data,
    size_t data_size,
    bool* __restrict__ success_flag
);

} // namespace ecc
} // namespace keyhunt
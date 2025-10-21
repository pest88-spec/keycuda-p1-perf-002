// Puzzle71 Technical Debt Repair - Batch Processing Optimizer
// Advanced batch processing optimization for ECC operations through adapter layer
// Implements intelligent batching, chunking, and dynamic load balancing

#pragma once

#include "ecc_adapter_integration_enhanced.cuh"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cstdint>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <future>
#include <chrono>

namespace keyhunt {
namespace batch_optimizer {

/**
 * @brief Batch processing strategy enumeration
 */
enum class BatchStrategy {
    UNIFORM,               // Process all batches with same size
    EXPONENTIAL_BACKOFF,   // Reduce batch size on errors
    ADAPTIVE_SIZE,         // Adapt batch size based on performance
    MEMORY_AWARE,          // Consider GPU memory constraints
    THROUGHPUT_OPTIMIZED,  // Optimize for maximum throughput
    LATENCY_OPTIMIZED      // Optimize for minimum latency
};

/**
 * @brief Batch processing configuration
 */
struct BatchProcessingConfig {
    // Basic batching parameters
    BatchStrategy strategy;
    size_t initial_batch_size;
    size_t min_batch_size;
    size_t max_batch_size;
    size_t chunk_size;           // Size of chunks for large batches
    int max_concurrent_batches;

    // Performance optimization parameters
    double target_throughput_ops_per_sec;
    double max_latency_ms;
    double memory_utilization_threshold;
    double gpu_utilization_threshold;

    // Adaptive parameters
    bool enable_adaptive_batching;
    bool enable_dynamic_chunking;
    bool enable_load_balancing;
    bool enable_performance_feedback;

    // Memory management
    bool enable_memory_pooling;
    size_t memory_pool_size_mb;
    bool enable_prefetching;
    size_t prefetch_ahead_count;

    // Error handling and recovery
    bool enable_error_recovery;
    int max_retry_attempts;
    double backoff_multiplier;
    bool enable_fallback_to_cpu;

    // Monitoring and telemetry
    bool enable_batch_monitoring;
    bool enable_performance_profiling;
    std::string telemetry_output_path;

    // Default constructor
    BatchProcessingConfig()
        : strategy(BatchStrategy::ADAPTIVE_SIZE)
        , initial_batch_size(2048)
        , min_batch_size(256)
        , max_batch_size(32768)
        , chunk_size(4096)
        , max_concurrent_batches(4)
        , target_throughput_ops_per_sec(2000.0)
        , max_latency_ms(100.0)
        , memory_utilization_threshold(0.85)
        , gpu_utilization_threshold(0.90)
        , enable_adaptive_batching(true)
        , enable_dynamic_chunking(true)
        , enable_load_balancing(true)
        , enable_performance_feedback(true)
        , enable_memory_pooling(true)
        , memory_pool_size_mb(512)
        , enable_prefetching(true)
        , prefetch_ahead_count(2)
        , enable_error_recovery(true)
        , max_retry_attempts(3)
        , backoff_multiplier(0.7)
        , enable_fallback_to_cpu(false)
        , enable_batch_monitoring(true)
        , enable_performance_profiling(true)
        , telemetry_output_path("batch_telemetry/")
    {}
};

/**
 * @brief Batch job description
 */
struct BatchJob {
    size_t job_id;
    uint32_t* private_keys;
    ecc::ECCPointSoA* public_keys;
    size_t batch_size;
    keyhunt::adapter::MemoryLayout input_layout;
    std::promise<bool> completion_promise;
    std::chrono::high_resolution_clock::time_point submit_time;
    int priority;
    std::string job_name;
};

/**
 * @brief Batch processing result with detailed metrics
 */
struct BatchProcessingResult {
    bool success;
    size_t batch_size;
    size_t chunks_processed;
    size_t chunks_failed;

    // Performance metrics
    double total_execution_time_ms;
    double average_chunk_time_ms;
    double throughput_ops_per_sec;
    double memory_efficiency_percent;
    double gpu_utilization_percent;

    // Batching metrics
    size_t optimal_batch_size_found;
    double average_batch_utilization;
    size_t batch_adaptations;
    size_t load_balancing_events;

    // Error handling metrics
    size_t total_retries;
    size_t fallback_attempts;
    std::vector<std::string> error_messages;

    // Memory metrics
    size_t peak_memory_usage_mb;
    double memory_pool_hit_rate;
    size_t prefetch_hits;
    size_t prefetch_misses;

    // Quality metrics
    double precision_achieved;
    size_t validation_passed;
    size_t validation_failed;

    std::chrono::high_resolution_clock::time_point completion_time;
};

/**
 * @brief Memory pool for efficient batch processing
 */
class BatchMemoryPool {
public:
    BatchMemoryPool(size_t pool_size_mb);
    ~BatchMemoryPool();

    // Allocate memory for batch operations
    bool allocate_batch_memory(uint32_t** private_keys, size_t batch_size);
    bool allocate_soa_memory(ecc::ECCPointSoA* points, size_t batch_size);

    // Deallocate memory back to pool
    void deallocate_batch_memory(uint32_t* ptr);
    void deallocate_soa_memory(ecc::ECCPointSoA* points);

    // Prefetch memory for upcoming operations
    bool prefetch_batch_memory(size_t batch_size);

    // Pool statistics
    size_t get_allocated_size_mb() const;
    double get_hit_rate() const;
    size_t get_total_allocations() const;
    size_t get_pool_hits() const;

private:
    struct MemoryBlock {
        void* ptr;
        size_t size;
        bool in_use;
        std::chrono::high_resolution_clock::time_point last_used;
    };

    size_t pool_size_bytes_;
    std::vector<MemoryBlock> memory_blocks_;
    mutable std::mutex pool_mutex_;
    std::atomic<size_t> total_allocations_{0};
    std::atomic<size_t> pool_hits_{0};

    // Internal helper methods
    MemoryBlock* find_free_block(size_t size);
    MemoryBlock* allocate_new_block(size_t size);
    void cleanup_unused_blocks();
};

/**
 * @brief Advanced batch processing optimizer
 *
 * This class provides intelligent batch processing optimization for ECC operations
 * through the adapter layer, implementing adaptive batching, dynamic chunking,
 * load balancing, and comprehensive performance monitoring.
 */
class BatchProcessingOptimizer {
public:
    // Constructor and destructor
    BatchProcessingOptimizer();
    explicit BatchProcessingOptimizer(const BatchProcessingConfig& config);
    ~BatchProcessingOptimizer();

    // Initialization and configuration
    bool initialize(keyhunt::integration::EnhancedECCAdapterIntegration* integration);
    void cleanup();
    bool reconfigure(const BatchProcessingConfig& new_config);

    // Batch job submission and processing

    /**
     * @brief Submit batch job for processing
     *
     * Submits a batch job that will be processed according to the configured
     * strategy. Returns a future that can be used to wait for completion.
     */
    std::future<bool> submit_batch_job(
        uint32_t* private_keys,
        ecc::ECCPointSoA* public_keys,
        size_t batch_size,
        keyhunt::adapter::MemoryLayout input_layout = keyhunt::adapter::MemoryLayout::AUTO_DETECT,
        int priority = 0,
        const std::string& job_name = ""
    );

    /**
     * @brief Process batch synchronously with detailed result
     */
    bool process_batch_sync(
        uint32_t* private_keys,
        ecc::ECCPointSoA* public_keys,
        size_t batch_size,
        keyhunt::adapter::MemoryLayout input_layout,
        BatchProcessingResult& result
    );

    /**
     * @brief Process large batch with automatic chunking
     */
    bool process_large_batch(
        uint32_t* private_keys,
        ecc::ECCPointSoA* public_keys,
        size_t total_batch_size,
        keyhunt::adapter::MemoryLayout input_layout,
        BatchProcessingResult& result
    );

    // Adaptive batch size optimization

    /**
     * @brief Find optimal batch size for current system
     */
    size_t find_optimal_batch_size(const ecc::ECCBatchConfig& ecc_config);

    /**
     * @brief Adapt batch size based on performance feedback
     */
    size_t adapt_batch_size(size_t current_batch_size, double current_throughput,
                           double memory_utilization, double gpu_utilization);

    /**
     * @brief Validate batch size for current constraints
     */
    bool validate_batch_size(size_t batch_size, size_t available_memory_mb);

    // Advanced batching strategies

    /**
     * @brief Process with exponential backoff strategy
     */
    bool process_with_exponential_backoff(
        uint32_t* private_keys,
        ecc::ECCPointSoA* public_keys,
        size_t batch_size,
        keyhunt::adapter::MemoryLayout input_layout,
        BatchProcessingResult& result
    );

    /**
     * @brief Process with adaptive size strategy
     */
    bool process_with_adaptive_size(
        uint32_t* private_keys,
        ecc::ECCPointSoA* public_keys,
        size_t batch_size,
        keyhunt::adapter::MemoryLayout input_layout,
        BatchProcessingResult& result
    );

    /**
     * @brief Process with memory-aware strategy
     */
    bool process_with_memory_aware(
        uint32_t* private_keys,
        ecc::ECCPointSoA* public_keys,
        size_t batch_size,
        keyhunt::adapter::MemoryLayout input_layout,
        BatchProcessingResult& result
    );

    // Chunking and load balancing

    /**
     * @brief Process large batch in optimized chunks
     */
    bool process_in_chunks(
        uint32_t* private_keys,
        ecc::ECCPointSoA* public_keys,
        size_t total_batch_size,
        size_t chunk_size,
        keyhunt::adapter::MemoryLayout input_layout,
        BatchProcessingResult& result
    );

    /**
     * @brief Balance load across multiple GPU streams
     */
    bool balance_load_across_streams(
        uint32_t* private_keys,
        ecc::ECCPointSoA* public_keys,
        size_t batch_size,
        int num_streams,
        BatchProcessingResult& result
    );

    // Memory management optimization

    /**
     * @brief Initialize memory pool for batch processing
     */
    bool initialize_memory_pool();

    /**
     * @brief Prefetch memory for upcoming operations
     */
    bool prefetch_for_next_batch(size_t expected_batch_size);

    /**
     * @brief Optimize memory layout for batch processing
     */
    bool optimize_memory_layout_for_batch(ecc::ECCPointSoA* points, size_t batch_size);

    // Performance monitoring and profiling

    /**
     * @brief Start batch processing performance monitoring
     */
    bool start_batch_monitoring(const std::string& session_name);

    /**
     * @brief Stop batch processing monitoring and get report
     */
    bool stop_batch_monitoring(BatchProcessingResult& aggregate_result);

    /**
     * @brief Get real-time batch processing metrics
     */
    struct RealTimeMetrics {
        double current_throughput_ops_per_sec;
        double average_latency_ms;
        size_t jobs_in_queue;
        size_t active_jobs;
        size_t completed_jobs;
        size_t failed_jobs;
        double memory_pool_utilization;
        double current_batch_size;
        size_t batch_adaptations_count;
    };

    RealTimeMetrics get_real_time_metrics() const;

    /**
     * @brief Export batch processing telemetry data
     */
    bool export_telemetry_data(const std::string& filename);

    // Error handling and recovery

    /**
     * @brief Handle batch processing errors with recovery
     */
    bool handle_batch_error(const std::string& error_message,
                           size_t failed_batch_size,
                           BatchProcessingResult& result);

    /**
     * @brief Attempt batch recovery with reduced size
     */
    bool attempt_batch_recovery(
        uint32_t* private_keys,
        ecc::ECCPointSoA* public_keys,
        size_t original_batch_size,
        keyhunt::adapter::MemoryLayout input_layout,
        BatchProcessingResult& result
    );

    // Configuration and state access

    /**
     * @brief Get current configuration
     */
    const BatchProcessingConfig& get_config() const { return config_; }

    /**
     * @brief Check if optimizer is initialized
     */
    bool is_initialized() const { return initialized_; }

    /**
     * @brief Get optimizer statistics
     */
    struct OptimizerStatistics {
        size_t total_batches_processed;
        size_t total_operations_processed;
        double average_throughput_ops_per_sec;
        double average_batch_size;
        size_t successful_adaptations;
        size_t total_chunks_processed;
        size_t memory_pool_hits;
        double memory_pool_hit_rate;
        size_t error_recoveries;
        double average_recovery_time_ms;
    };

    OptimizerStatistics get_optimizer_statistics() const;
    void reset_statistics();

    // Queue management

    /**
     * @brief Get current queue size
     */
    size_t get_queue_size() const;

    /**
     * @brief Clear pending jobs in queue
     */
    void clear_queue();

    /**
     * @brief Wait for all pending jobs to complete
     */
    bool wait_for_completion(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

private:
    // Core configuration and state
    BatchProcessingConfig config_;
    bool initialized_;
    keyhunt::integration::EnhancedECCAdapterIntegration* integration_;

    // Job queue and processing
    std::queue<std::unique_ptr<BatchJob>> job_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> shutdown_requested_{false};

    // Memory management
    std::unique_ptr<BatchMemoryPool> memory_pool_;

    // Performance tracking
    std::atomic<size_t> total_batches_processed_{0};
    std::atomic<size_t> total_operations_processed_{0};
    std::atomic<double> total_execution_time_{0.0};
    std::atomic<size_t> successful_adaptations_{0};
    std::atomic<size_t> total_chunks_processed_{0};
    std::atomic<size_t> error_recoveries_{0};

    // Adaptive batch size tracking
    std::atomic<size_t> current_optimal_batch_size_{2048};
    std::vector<std::pair<size_t, double>> batch_size_performance_history_;
    mutable std::mutex performance_history_mutex_;

    // Monitoring state
    std::atomic<bool> monitoring_active_{false};
    std::string current_monitoring_session_;
    std::chrono::high_resolution_clock::time_point monitoring_start_time_;
    std::vector<BatchProcessingResult> batch_results_history_;

    // CUDA resources for multi-stream processing
    std::vector<cudaStream_t> cuda_streams_;
    std::vector<cudaEvent_t> cuda_events_;

    // Internal helper methods

    /**
     * @brief Worker thread function for processing batch jobs
     */
    void worker_thread_function();

    /**
     * @brief Process single batch job
     */
    bool process_batch_job(BatchJob& job, BatchProcessingResult& result);

    /**
     * @brief Execute batch with specified strategy
     */
    bool execute_batch_strategy(
        uint32_t* private_keys,
        ecc::ECCPointSoA* public_keys,
        size_t batch_size,
        keyhunt::adapter::MemoryLayout input_layout,
        BatchProcessingResult& result
    );

    /**
     * @brief Update performance history for adaptive batching
     */
    void update_performance_history(size_t batch_size, double throughput);

    /**
     * @brief Calculate optimal batch size from performance history
     */
    size_t calculate_optimal_batch_size() const;

    /**
     * @brief Setup CUDA streams for parallel processing
     */
    bool setup_cuda_streams();

    /**
     * @brief Cleanup CUDA resources
     */
    void cleanup_cuda_resources();

    /**
     * @brief Record batch processing metrics
     */
    void record_batch_metrics(const BatchProcessingResult& result);

    /**
     * @brief Generate performance report from collected data
     */
    void generate_performance_report(BatchProcessingResult& aggregate_result);

    /**
     * @brief Validate batch processing constraints
     */
    bool validate_batch_constraints(size_t batch_size) const;

    /**
     * @brief Calculate chunk size for large batch
     */
    size_t calculate_optimal_chunk_size(size_t total_batch_size) const;

    /**
     * @brief Estimate memory usage for batch size
     */
    size_t estimate_memory_usage(size_t batch_size) const;

    /**
     * @brief Check GPU memory availability
     */
    bool check_gpu_memory_availability(size_t required_memory_mb) const;

    /**
     * @brief Apply adaptive batch size changes
     */
    void apply_adaptive_changes(size_t new_batch_size);
};

/**
 * @brief Global batch processing optimizer instance
 */
extern std::unique_ptr<BatchProcessingOptimizer> g_batch_optimizer;

/**
 * @brief Initialize global batch processing optimizer
 */
bool initialize_global_batch_optimizer(
    const BatchProcessingConfig& config = BatchProcessingConfig(),
    keyhunt::integration::EnhancedECCAdapterIntegration* integration = nullptr
);

/**
 * @brief Cleanup global batch processing optimizer
 */
void cleanup_global_batch_optimizer();

/**
 * @brief Get global batch processing optimizer instance
 */
BatchProcessingOptimizer* get_global_batch_optimizer();

/**
 * @brief Convenience functions for common batch operations
 */

// Quick batch processing with default optimization
bool quick_batch_process(uint32_t* private_keys,
                         ecc::ECCPointSoA* public_keys,
                         size_t batch_size);

// Quick large batch processing with automatic chunking
bool quick_large_batch_process(uint32_t* private_keys,
                               ecc::ECCPointSoA* public_keys,
                               size_t total_batch_size);

// Quick optimal batch size detection
size_t quick_find_optimal_batch_size();

// Quick batch processing with performance feedback
bool quick_batch_process_with_feedback(uint32_t* private_keys,
                                       ecc::ECCPointSoA* public_keys,
                                       size_t batch_size,
                                       double& throughput_achieved);

} // namespace batch_optimizer

// Namespace aliases for convenience
namespace batch = keyhunt::batch_optimizer;

} // namespace keyhunt
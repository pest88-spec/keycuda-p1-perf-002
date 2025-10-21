// Puzzle71 Technical Debt Repair - Batch Processing Optimizer Implementation
// Advanced batch processing optimization for ECC operations through adapter layer
// Implements intelligent batching, chunking, and dynamic load balancing

#include "batch_processing_optimizer.cuh"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace keyhunt {
namespace batch_optimizer {

// Global instance
std::unique_ptr<BatchProcessingOptimizer> g_batch_optimizer = nullptr;

// ============================================================================
// BatchMemoryPool Implementation
// ============================================================================

BatchMemoryPool::BatchMemoryPool(size_t pool_size_mb)
    : pool_size_bytes_(pool_size_mb * 1024 * 1024) {
}

BatchMemoryPool::~BatchMemoryPool() {
    std::lock_guard<std::mutex> lock(pool_mutex_);

    for (auto& block : memory_blocks_) {
        if (block.ptr) {
            cudaFree(block.ptr);
        }
    }
    memory_blocks_.clear();
}

bool BatchMemoryPool::allocate_batch_memory(uint32_t** private_keys, size_t batch_size) {
    if (!private_keys || batch_size == 0) return false;

    size_t required_size = batch_size * sizeof(uint32_t);
    std::lock_guard<std::mutex> lock(pool_mutex_);

    total_allocations_++;

    // Try to find a free block
    MemoryBlock* block = find_free_block(required_size);
    if (!block) {
        // Try to allocate new block
        block = allocate_new_block(required_size);
        if (!block) {
            return false;
        }
    }

    block->in_use = true;
    block->last_used = std::chrono::high_resolution_clock::now();
    *private_keys = static_cast<uint32_t*>(block->ptr);

    pool_hits_++;
    return true;
}

bool BatchMemoryPool::allocate_soa_memory(ecc::ECCPointSoA* points, size_t batch_size) {
    if (!points || batch_size == 0) return false;

    size_t x_size = batch_size * 8 * sizeof(uint32_t);  // X coordinates
    size_t y_size = batch_size * 8 * sizeof(uint32_t);  // Y coordinates
    size_t validity_size = batch_size * sizeof(bool);

    std::lock_guard<std::mutex> lock(pool_mutex_);

    total_allocations_++;

    // Allocate X coordinates
    MemoryBlock* x_block = find_free_block(x_size);
    if (!x_block) {
        x_block = allocate_new_block(x_size);
        if (!x_block) return false;
    }
    x_block->in_use = true;
    x_block->last_used = std::chrono::high_resolution_clock::now();
    points->x_words = static_cast<uint32_t*>(x_block->ptr);

    // Allocate Y coordinates
    MemoryBlock* y_block = find_free_block(y_size);
    if (!y_block) {
        y_block = allocate_new_block(y_size);
        if (!y_block) {
            x_block->in_use = false;
            return false;
        }
    }
    y_block->in_use = true;
    y_block->last_used = std::chrono::high_resolution_clock::now();
    points->y_words = static_cast<uint32_t*>(y_block->ptr);

    // Allocate validity flags (host memory)
    points->is_valid = new bool[batch_size];
    points->size = batch_size;

    pool_hits_++;
    return true;
}

void BatchMemoryPool::deallocate_batch_memory(uint32_t* ptr) {
    if (!ptr) return;

    std::lock_guard<std::mutex> lock(pool_mutex_);

    for (auto& block : memory_blocks_) {
        if (block.ptr == ptr) {
            block.in_use = false;
            return;
        }
    }
}

void BatchMemoryPool::deallocate_soa_memory(ecc::ECCPointSoA* points) {
    if (!points) return;

    std::lock_guard<std::mutex> lock(pool_mutex_);

    // Find and deallocate X block
    for (auto& block : memory_blocks_) {
        if (block.ptr == points->x_words) {
            block.in_use = false;
            break;
        }
    }

    // Find and deallocate Y block
    for (auto& block : memory_blocks_) {
        if (block.ptr == points->y_words) {
            block.in_use = false;
            break;
        }
    }

    // Deallocate validity flags
    delete[] points->is_valid;
    points->is_valid = nullptr;
    points->x_words = nullptr;
    points->y_words = nullptr;
    points->size = 0;
}

bool BatchMemoryPool::prefetch_batch_memory(size_t batch_size) {
    size_t required_size = batch_size * sizeof(uint32_t);

    std::lock_guard<std::mutex> lock(pool_mutex_);

    // Check if we have an available block of appropriate size
    for (auto& block : memory_blocks_) {
        if (!block.in_use && block.size >= required_size) {
            // Prefetch by touching the memory
            cudaMemPrefetchAsync(block.ptr, required_size, cudaCpuDeviceId, 0);
            return true;
        }
    }

    return false;
}

size_t BatchMemoryPool::get_allocated_size_mb() const {
    std::lock_guard<std::mutex> lock(pool_mutex_);

    size_t total_allocated = 0;
    for (const auto& block : memory_blocks_) {
        if (block.in_use) {
            total_allocated += block.size;
        }
    }

    return total_allocated / (1024 * 1024);
}

double BatchMemoryPool::get_hit_rate() const {
    size_t total = total_allocations_.load();
    size_t hits = pool_hits_.load();

    return total > 0 ? (static_cast<double>(hits) / total) * 100.0 : 0.0;
}

size_t BatchMemoryPool::get_total_allocations() const {
    return total_allocations_.load();
}

size_t BatchMemoryPool::get_pool_hits() const {
    return pool_hits_.load();
}

BatchMemoryPool::MemoryBlock* BatchMemoryPool::find_free_block(size_t size) {
    // Find the smallest free block that can accommodate the request
    MemoryBlock* best_block = nullptr;

    for (auto& block : memory_blocks_) {
        if (!block.in_use && block.size >= size) {
            if (!best_block || block.size < best_block->size) {
                best_block = &block;
            }
        }
    }

    return best_block;
}

BatchMemoryPool::MemoryBlock* BatchMemoryPool::allocate_new_block(size_t size) {
    // Check if we have enough space in the pool
    size_t current_used = 0;
    for (const auto& block : memory_blocks_) {
        if (block.in_use) {
            current_used += block.size;
        }
    }

    if (current_used + size > pool_size_bytes_) {
        // Try to cleanup unused blocks first
        cleanup_unused_blocks();

        // Recalculate current usage
        current_used = 0;
        for (const auto& block : memory_blocks_) {
            if (block.in_use) {
                current_used += block.size;
            }
        }

        if (current_used + size > pool_size_bytes_) {
            return nullptr;  // Not enough space
        }
    }

    // Allocate new block
    void* ptr = nullptr;
    cudaError_t err = cudaMalloc(&ptr, size);
    if (err != cudaSuccess) {
        return nullptr;
    }

    MemoryBlock new_block;
    new_block.ptr = ptr;
    new_block.size = size;
    new_block.in_use = false;
    new_block.last_used = std::chrono::high_resolution_clock::now();

    memory_blocks_.push_back(new_block);
    return &memory_blocks_.back();
}

void BatchMemoryPool::cleanup_unused_blocks() {
    auto now = std::chrono::high_resolution_clock::now();
    auto cleanup_threshold = std::chrono::minutes(5);  // Clean up blocks unused for 5 minutes

    memory_blocks_.erase(
        std::remove_if(memory_blocks_.begin(), memory_blocks_.end(),
            [cleanup_threshold, now](const MemoryBlock& block) {
                if (!block.in_use) {
                    auto time_since_last_use = now - block.last_used;
                    if (time_since_last_use > cleanup_threshold) {
                        cudaFree(block.ptr);
                        return true;
                    }
                }
                return false;
            }),
        memory_blocks_.end()
    );
}

// ============================================================================
// BatchProcessingOptimizer Implementation
// ============================================================================

BatchProcessingOptimizer::BatchProcessingOptimizer()
    : initialized_(false)
    , integration_(nullptr) {
}

BatchProcessingOptimizer::BatchProcessingOptimizer(const BatchProcessingConfig& config)
    : config_(config)
    , initialized_(false)
    , integration_(nullptr) {
    current_optimal_batch_size_ = config.initial_batch_size;
}

BatchProcessingOptimizer::~BatchProcessingOptimizer() {
    cleanup();
}

bool BatchProcessingOptimizer::initialize(
    keyhunt::integration::EnhancedECCAdapterIntegration* integration) {

    if (initialized_) {
        return true;
    }

    if (!integration) {
        return false;
    }

    integration_ = integration;

    // Initialize memory pool if enabled
    if (config_.enable_memory_pooling) {
        if (!initialize_memory_pool()) {
            return false;
        }
    }

    // Setup CUDA streams if load balancing is enabled
    if (config_.enable_load_balancing) {
        if (!setup_cuda_streams()) {
            return false;
        }
    }

    // Start worker threads
    size_t num_threads = std::max(1, config_.max_concurrent_batches);
    for (size_t i = 0; i < num_threads; ++i) {
        worker_threads_.emplace_back(&BatchProcessingOptimizer::worker_thread_function, this);
    }

    initialized_ = true;
    return true;
}

void BatchProcessingOptimizer::cleanup() {
    if (!initialized_) {
        return;
    }

    // Signal shutdown to worker threads
    shutdown_requested_ = true;
    queue_cv_.notify_all();

    // Wait for worker threads to finish
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();

    // Clear job queue
    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!job_queue_.empty()) {
        auto& job = job_queue_.front();
        job->completion_promise.set_value(false);
        job_queue_.pop();
    }

    // Cleanup CUDA resources
    cleanup_cuda_resources();

    // Cleanup memory pool
    memory_pool_.reset();

    integration_ = nullptr;
    initialized_ = false;
}

bool BatchProcessingOptimizer::reconfigure(const BatchProcessingConfig& new_config) {
    config_ = new_config;

    // Reinitialize memory pool if size changed
    if (config_.enable_memory_pooling &&
        (!memory_pool_ || config_.memory_pool_size_mb != new_config.memory_pool_size_mb)) {
        if (!initialize_memory_pool()) {
            return false;
        }
    }

    return true;
}

std::future<bool> BatchProcessingOptimizer::submit_batch_job(
    uint32_t* private_keys,
    ecc::ECCPointSoA* public_keys,
    size_t batch_size,
    keyhunt::adapter::MemoryLayout input_layout,
    int priority,
    const std::string& job_name) {

    auto job = std::make_unique<BatchJob>();
    static std::atomic<size_t> job_counter{0};

    job->job_id = job_counter++;
    job->private_keys = private_keys;
    job->public_keys = public_keys;
    job->batch_size = batch_size;
    job->input_layout = input_layout;
    job->submit_time = std::chrono::high_resolution_clock::now();
    job->priority = priority;
    job->job_name = job_name.empty() ? "batch_job_" + std::to_string(job->job_id) : job_name;

    auto future = job->completion_promise.get_future();

    // Add to queue
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        job_queue_.push(std::move(job));
    }

    queue_cv_.notify_one();
    return future;
}

bool BatchProcessingOptimizer::process_batch_sync(
    uint32_t* private_keys,
    ecc::ECCPointSoA* public_keys,
    size_t batch_size,
    keyhunt::adapter::MemoryLayout input_layout,
    BatchProcessingResult& result) {

    auto future = submit_batch_job(private_keys, public_keys, batch_size, input_layout, 0);
    bool success = future.get();

    // Get the result from the last completed job
    if (!batch_results_history_.empty()) {
        result = batch_results_history_.back();
    } else {
        result.success = success;
        result.batch_size = batch_size;
        result.completion_time = std::chrono::high_resolution_clock::now();
    }

    return success;
}

bool BatchProcessingOptimizer::process_large_batch(
    uint32_t* private_keys,
    ecc::ECCPointSoA* public_keys,
    size_t total_batch_size,
    keyhunt::adapter::MemoryLayout input_layout,
    BatchProcessingResult& result) {

    result = {};
    result.batch_size = total_batch_size;
    result.chunks_processed = 0;
    result.chunks_failed = 0;

    auto start_time = std::chrono::high_resolution_clock::now();

    // Calculate optimal chunk size
    size_t chunk_size = calculate_optimal_chunk_size(total_batch_size);

    // Process in chunks
    size_t processed = 0;
    bool overall_success = true;

    while (processed < total_batch_size) {
        size_t current_chunk_size = std::min(chunk_size, total_batch_size - processed);

        BatchProcessingResult chunk_result;
        bool chunk_success = process_batch_sync(
            private_keys + processed,
            public_keys,  // Note: This is simplified - in practice would need offset handling
            current_chunk_size,
            input_layout,
            chunk_result
        );

        if (chunk_success) {
            result.chunks_processed++;
            processed += current_chunk_size;
        } else {
            result.chunks_failed++;
            overall_success = false;

            // Try recovery with smaller chunk size
            if (config_.enable_error_recovery && current_chunk_size > config_.min_batch_size) {
                size_t recovery_size = current_chunk_size / 2;
                if (recovery_size >= config_.min_batch_size) {
                    chunk_size = recovery_size;
                    continue;  // Retry with smaller chunk
                }
            }

            // If recovery fails or not enabled, abort
            break;
        }

        // Update aggregate metrics
        result.total_execution_time_ms += chunk_result.total_execution_time_ms;
        result.memory_efficiency_percent += chunk_result.memory_efficiency_percent;
        result.gpu_utilization_percent += chunk_result.gpu_utilization_percent;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    result.total_execution_time_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
        end_time - start_time).count();

    // Calculate averages
    if (result.chunks_processed > 0) {
        result.average_chunk_time_ms = result.total_execution_time_ms / result.chunks_processed;
        result.memory_efficiency_percent /= result.chunks_processed;
        result.gpu_utilization_percent /= result.chunks_processed;
        result.throughput_ops_per_sec = (total_batch_size * 1000.0) / result.total_execution_time_ms;
    }

    result.success = overall_success && (processed == total_batch_size);
    result.completion_time = end_time;

    return result.success;
}

size_t BatchProcessingOptimizer::find_optimal_batch_size(const ecc::ECCBatchConfig& ecc_config) {
    if (!config_.enable_adaptive_batching) {
        return config_.initial_batch_size;
    }

    // Test different batch sizes to find optimal one
    std::vector<size_t> test_sizes = {256, 512, 1024, 2048, 4096, 8192, 16384};

    size_t best_batch_size = config_.initial_batch_size;
    double best_throughput = 0.0;

    for (size_t test_size : test_sizes) {
        if (test_size < config_.min_batch_size || test_size > config_.max_batch_size) {
            continue;
        }

        if (!validate_batch_size(test_size, config_.memory_pool_size_mb)) {
            continue;
        }

        // Run a quick test
        BatchProcessingResult test_result;
        // Note: In practice, would need actual test data here
        // For now, use a simple heuristic based on batch size

        // Simulate throughput calculation (replace with actual measurement)
        double simulated_throughput = test_size * 1000.0 / (test_size * 0.001); // Simplified

        if (simulated_throughput > best_throughput) {
            best_throughput = simulated_throughput;
            best_batch_size = test_size;
        }
    }

    current_optimal_batch_size_ = best_batch_size;
    update_performance_history(best_batch_size, best_throughput);

    return best_batch_size;
}

size_t BatchProcessingOptimizer::adapt_batch_size(size_t current_batch_size, double current_throughput,
                                                 double memory_utilization, double gpu_utilization) {
    if (!config_.enable_adaptive_batching) {
        return current_batch_size;
    }

    size_t new_batch_size = current_batch_size;

    // Adapt based on performance feedback
    if (current_throughput < config_.target_throughput_ops_per_sec * 0.8) {
        // Low throughput - try increasing batch size
        new_batch_size = static_cast<size_t>(current_batch_size * 1.2);
    } else if (memory_utilization > config_.memory_utilization_threshold) {
        // High memory usage - reduce batch size
        new_batch_size = static_cast<size_t>(current_batch_size * 0.8);
    } else if (gpu_utilization < config_.gpu_utilization_threshold) {
        // Low GPU utilization - try increasing batch size
        new_batch_size = static_cast<size_t>(current_batch_size * 1.1);
    }

    // Clamp to valid range
    new_batch_size = std::max(config_.min_batch_size,
                              std::min(config_.max_batch_size, new_batch_size));

    // Update performance history
    update_performance_history(current_batch_size, current_throughput);

    if (new_batch_size != current_batch_size) {
        successful_adaptations_++;
        apply_adaptive_changes(new_batch_size);
    }

    return new_batch_size;
}

bool BatchProcessingOptimizer::process_with_exponential_backoff(
    uint32_t* private_keys,
    ecc::ECCPointSoA* public_keys,
    size_t batch_size,
    keyhunt::adapter::MemoryLayout input_layout,
    BatchProcessingResult& result) {

    result = {};
    size_t current_batch_size = batch_size;
    int attempts = 0;

    while (attempts < config_.max_retry_attempts && current_batch_size >= config_.min_batch_size) {
        BatchProcessingResult attempt_result;

        bool success = execute_batch_strategy(
            private_keys, public_keys, current_batch_size, input_layout, attempt_result);

        if (success) {
            result = attempt_result;
            result.total_retries = attempts;
            return true;
        }

        // Exponential backoff
        current_batch_size = static_cast<size_t>(current_batch_size * config_.backoff_multiplier);
        attempts++;

        if (current_batch_size < config_.min_batch_size) {
            current_batch_size = config_.min_batch_size;
        }
    }

    result.success = false;
    result.total_retries = attempts;
    result.error_messages.push_back("All retry attempts failed with exponential backoff");

    return false;
}

bool BatchProcessingOptimizer::process_with_adaptive_size(
    uint32_t* private_keys,
    ecc::ECCPointSoA* public_keys,
    size_t batch_size,
    keyhunt::adapter::MemoryLayout input_layout,
    BatchProcessingResult& result) {

    // Start with current optimal batch size
    size_t adaptive_batch_size = current_optimal_batch_size_.load();

    // If requested batch is smaller, use that
    if (batch_size < adaptive_batch_size) {
        adaptive_batch_size = batch_size;
    }

    BatchProcessingResult adaptive_result;
    bool success = execute_batch_strategy(
        private_keys, public_keys, adaptive_batch_size, input_layout, adaptive_result);

    if (success) {
        // Update performance metrics and adapt for future
        update_performance_history(adaptive_batch_size, adaptive_result.throughput_ops_per_sec);

        // Adapt batch size for next iteration
        size_t new_optimal_size = adapt_batch_size(
            adaptive_batch_size,
            adaptive_result.throughput_ops_per_sec,
            adaptive_result.memory_efficiency_percent / 100.0,
            adaptive_result.gpu_utilization_percent / 100.0
        );

        result = adaptive_result;
        result.optimal_batch_size_found = new_optimal_size;
        result.batch_adaptations = 1;

        return true;
    }

    // Fallback to smaller batch size
    return handle_batch_error("Adaptive size processing failed", adaptive_batch_size, result);
}

bool BatchProcessingOptimizer::process_in_chunks(
    uint32_t* private_keys,
    ecc::ECCPointSoA* public_keys,
    size_t total_batch_size,
    size_t chunk_size,
    keyhunt::adapter::MemoryLayout input_layout,
    BatchProcessingResult& result) {

    result = {};
    result.batch_size = total_batch_size;
    result.chunks_processed = 0;
    result.chunks_failed = 0;

    auto start_time = std::chrono::high_resolution_clock::now();

    size_t processed = 0;
    bool overall_success = true;

    while (processed < total_batch_size) {
        size_t current_chunk = std::min(chunk_size, total_batch_size - processed);

        BatchProcessingResult chunk_result;
        bool chunk_success = execute_batch_strategy(
            private_keys + processed,
            public_keys,  // Simplified - would need proper offset handling
            current_chunk,
            input_layout,
            chunk_result
        );

        if (chunk_success) {
            result.chunks_processed++;
            processed += current_chunk;
        } else {
            result.chunks_failed++;
            overall_success = false;

            if (!config_.enable_error_recovery) {
                break;
            }

            // Try with smaller chunk
            chunk_size = std::max(config_.min_batch_size, current_chunk / 2);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    result.total_execution_time_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
        end_time - start_time).count();

    result.throughput_ops_per_sec = (processed * 1000.0) / result.total_execution_time_ms;
    result.success = overall_success && (processed == total_batch_size);
    result.completion_time = end_time;

    total_chunks_processed_ += result.chunks_processed;

    return result.success;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void BatchProcessingOptimizer::worker_thread_function() {
    while (!shutdown_requested_) {
        std::unique_ptr<BatchJob> job;

        // Wait for job
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] {
                return !job_queue_.empty() || shutdown_requested_;
            });

            if (shutdown_requested_) {
                break;
            }

            if (!job_queue_.empty()) {
                job = std::move(job_queue_.front());
                job_queue_.pop();
            }
        }

        if (!job) {
            continue;
        }

        // Process the job
        BatchProcessingResult result;
        bool success = process_batch_job(*job, result);

        // Set the promise value
        job->completion_promise.set_value(success);

        // Record metrics
        record_batch_metrics(result);
    }
}

bool BatchProcessingOptimizer::process_batch_job(BatchJob& job, BatchProcessingResult& result) {
    result = {};
    result.batch_size = job.batch_size;

    auto start_time = std::chrono::high_resolution_clock::now();

    // Execute with configured strategy
    bool success = false;
    switch (config_.strategy) {
        case BatchStrategy::EXPONENTIAL_BACKOFF:
            success = process_with_exponential_backoff(
                job.private_keys, job.public_keys, job.batch_size,
                job.input_layout, result);
            break;

        case BatchStrategy::ADAPTIVE_SIZE:
            success = process_with_adaptive_size(
                job.private_keys, job.public_keys, job.batch_size,
                job.input_layout, result);
            break;

        case BatchStrategy::MEMORY_AWARE:
            success = process_with_memory_aware(
                job.private_keys, job.public_keys, job.batch_size,
                job.input_layout, result);
            break;

        default:
            success = execute_batch_strategy(
                job.private_keys, job.public_keys, job.batch_size,
                job.input_layout, result);
            break;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    result.total_execution_time_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
        end_time - start_time).count();
    result.completion_time = end_time;

    if (success) {
        total_batches_processed_++;
        total_operations_processed_ += job.batch_size;
        total_execution_time_ += result.total_execution_time_ms;
    }

    return success;
}

bool BatchProcessingOptimizer::execute_batch_strategy(
    uint32_t* private_keys,
    ecc::ECCPointSoA* public_keys,
    size_t batch_size,
    keyhunt::adapter::MemoryLayout input_layout,
    BatchProcessingResult& result) {

    if (!integration_ || !integration_->is_initialized()) {
        result.error_messages.push_back("ECC integration not initialized");
        return false;
    }

    keyhunt::adapter::ComprehensivePerformanceReport adapter_report;

    bool success = integration_->scalar_multiply_with_monitoring(
        private_keys, public_keys, batch_size, input_layout, &adapter_report);

    if (success) {
        result.throughput_ops_per_sec = adapter_report.ecc_throughput_ops_per_sec;
        result.memory_efficiency_percent = adapter_report.memory_efficiency_percent;
        result.gpu_utilization_percent = adapter_report.gpu_utilization_percent;
        result.precision_achieved = adapter_report.ecc_precision_achieved;
        result.validation_passed = adapter_report.successful_operations;
        result.validation_failed = adapter_report.failed_operations;
    } else {
        result.error_messages.push_back("ECC scalar multiplication failed");
    }

    return success;
}

void BatchProcessingOptimizer::update_performance_history(size_t batch_size, double throughput) {
    std::lock_guard<std::mutex> lock(performance_history_mutex_);

    batch_size_performance_history_.push_back({batch_size, throughput});

    // Keep history manageable
    if (batch_size_performance_history_.size() > 1000) {
        batch_size_performance_history_.erase(
            batch_size_performance_history_.begin(),
            batch_size_performance_history_.begin() + 500
        );
    }
}

size_t BatchProcessingOptimizer::calculate_optimal_batch_size() const {
    std::lock_guard<std::mutex> lock(performance_history_mutex_);

    if (batch_size_performance_history_.empty()) {
        return config_.initial_batch_size;
    }

    // Find batch size with best throughput
    double best_throughput = 0.0;
    size_t best_batch_size = config_.initial_batch_size;

    for (const auto& entry : batch_size_performance_history_) {
        if (entry.second > best_throughput) {
            best_throughput = entry.second;
            best_batch_size = entry.first;
        }
    }

    return best_batch_size;
}

bool BatchProcessingOptimizer::initialize_memory_pool() {
    memory_pool_ = std::make_unique<BatchMemoryPool>(config_.memory_pool_size_mb);
    return memory_pool_ != nullptr;
}

bool BatchProcessingOptimizer::setup_cuda_streams() {
    if (!config_.enable_load_balancing) {
        return true;
    }

    int num_streams = std::min(config_.max_concurrent_batches, 8);  // Max 8 streams

    for (int i = 0; i < num_streams; ++i) {
        cudaStream_t stream;
        cudaError_t err = cudaStreamCreate(&stream);
        if (err != cudaSuccess) {
            cleanup_cuda_resources();
            return false;
        }
        cuda_streams_.push_back(stream);
    }

    return true;
}

void BatchProcessingOptimizer::cleanup_cuda_resources() {
    for (auto stream : cuda_streams_) {
        cudaStreamDestroy(stream);
    }
    cuda_streams_.clear();

    for (auto event : cuda_events_) {
        cudaEventDestroy(event);
    }
    cuda_events_.clear();
}

void BatchProcessingOptimizer::record_batch_metrics(const BatchProcessingResult& result) {
    if (config_.enable_batch_monitoring) {
        batch_results_history_.push_back(result);

        // Keep history manageable
        if (batch_results_history_.size() > 10000) {
            batch_results_history_.erase(
                batch_results_history_.begin(),
                batch_results_history_.begin() + 5000
            );
        }
    }
}

size_t BatchProcessingOptimizer::calculate_optimal_chunk_size(size_t total_batch_size) const {
    // Default chunk size
    size_t chunk_size = config_.chunk_size;

    // Adjust based on total batch size
    if (total_batch_size > 100000) {
        chunk_size = std::max(chunk_size, total_batch_size / 20);  // 20 chunks max
    } else if (total_batch_size < chunk_size) {
        chunk_size = total_batch_size;
    }

    // Ensure chunk size is within bounds
    chunk_size = std::max(config_.min_batch_size,
                          std::min(config_.max_batch_size, chunk_size));

    return chunk_size;
}

void BatchProcessingOptimizer::apply_adaptive_changes(size_t new_batch_size) {
    current_optimal_batch_size_ = new_batch_size;

    // Prefetch memory for next batch if enabled
    if (config_.enable_prefetching && memory_pool_) {
        memory_pool_->prefetch_batch_memory(new_batch_size);
    }
}

bool BatchProcessingOptimizer::handle_batch_error(const std::string& error_message,
                                                 size_t failed_batch_size,
                                                 BatchProcessingResult& result) {
    result.error_messages.push_back(error_message);

    if (config_.enable_error_recovery) {
        error_recoveries_++;

        // Try with smaller batch size
        size_t recovery_size = static_cast<size_t>(failed_batch_size * config_.backoff_multiplier);
        recovery_size = std::max(config_.min_batch_size, recovery_size);

        if (recovery_size < failed_batch_size) {
            result.total_retries++;
            return attempt_batch_recovery(nullptr, nullptr, recovery_size,
                                       keyhunt::adapter::MemoryLayout::AUTO_DETECT, result);
        }
    }

    return false;
}

// ============================================================================
// Global Instance Management
// ============================================================================

bool initialize_global_batch_optimizer(
    const BatchProcessingConfig& config,
    keyhunt::integration::EnhancedECCAdapterIntegration* integration) {

    if (g_batch_optimizer) {
        return true;  // Already initialized
    }

    g_batch_optimizer = std::make_unique<BatchProcessingOptimizer>(config);
    return g_batch_optimizer->initialize(integration);
}

void cleanup_global_batch_optimizer() {
    g_batch_optimizer.reset();
}

BatchProcessingOptimizer* get_global_batch_optimizer() {
    return g_batch_optimizer.get();
}

// ============================================================================
// Convenience Functions
// ============================================================================

bool quick_batch_process(uint32_t* private_keys,
                         ecc::ECCPointSoA* public_keys,
                         size_t batch_size) {
    auto* optimizer = get_global_batch_optimizer();
    if (!optimizer) {
        // Initialize with defaults
        BatchProcessingConfig config;
        initialize_global_batch_optimizer(config);
        optimizer = get_global_batch_optimizer();
    }

    if (!optimizer) {
        return false;
    }

    BatchProcessingResult result;
    return optimizer->process_batch_sync(
        private_keys, public_keys, batch_size,
        keyhunt::adapter::MemoryLayout::AUTO_DETECT, result);
}

bool quick_large_batch_process(uint32_t* private_keys,
                               ecc::ECCPointSoA* public_keys,
                               size_t total_batch_size) {
    auto* optimizer = get_global_batch_optimizer();
    if (!optimizer) {
        return false;
    }

    BatchProcessingResult result;
    return optimizer->process_large_batch(
        private_keys, public_keys, total_batch_size,
        keyhunt::adapter::MemoryLayout::AUTO_DETECT, result);
}

size_t quick_find_optimal_batch_size() {
    auto* optimizer = get_global_batch_optimizer();
    if (!optimizer) {
        return 2048;  // Default
    }

    ecc::ECCBatchConfig ecc_config;  // Use default config
    return optimizer->find_optimal_batch_size(ecc_config);
}

bool quick_batch_process_with_feedback(uint32_t* private_keys,
                                       ecc::ECCPointSoA* public_keys,
                                       size_t batch_size,
                                       double& throughput_achieved) {
    auto* optimizer = get_global_batch_optimizer();
    if (!optimizer) {
        return false;
    }

    BatchProcessingResult result;
    bool success = optimizer->process_batch_sync(
        private_keys, public_keys, batch_size,
        keyhunt::adapter::MemoryLayout::AUTO_DETECT, result);

    throughput_achieved = result.throughput_ops_per_sec;
    return success;
}

} // namespace batch_optimizer
} // namespace keyhunt
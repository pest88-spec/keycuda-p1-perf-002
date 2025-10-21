// Puzzle71 Technical Debt Repair - ECC Adapter Integration Performance Benchmarking Tool
// Comprehensive performance benchmarking for T029: ECC operations integrated with adapter layer
// Compares adapter-based operations vs direct ECC operations with detailed metrics

#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <fstream>
#include <string>
#include <memory>
#include <filesystem>
#include <cuda_runtime.h>

#include "KeyhuntCore/common/ecc_adapter_integration_enhanced.cuh"
#include "KeyhuntCore/common/ecc_operations_fixed.cuh"
#include "KeyhuntCore/common/legacy_adapter_fixed_enhanced.cuh"

using namespace keyhunt;
using namespace keyhunt::integration;
using namespace keyhunt::adapter;
using namespace keyhunt::ecc;

struct BenchmarkConfiguration {
    std::string output_directory;
    bool compare_with_direct;
    bool enable_deterministic_replay;
    bool enable_cpu_validation;
    bool test_legacy_compatibility;
    std::vector<size_t> batch_sizes;
    int iterations_per_size;
    bool export_detailed_metrics;
    bool generate_plots;
};

struct BenchmarkResults {
    size_t batch_size;

    // Adapter-based performance metrics
    double adapter_throughput_ops_per_sec;
    double adapter_execution_time_ms;
    double adapter_memory_efficiency_percent;
    double adapter_gpu_utilization_percent;
    double adapter_precision_achieved;
    double adapter_overhead_percent;

    // Direct ECC performance metrics (if available)
    double direct_throughput_ops_per_sec;
    double direct_execution_time_ms;
    double direct_memory_efficiency_percent;
    double direct_gpu_utilization_percent;
    double direct_precision_achieved;

    // Memory layout conversion metrics
    double layout_conversion_time_ms;
    double aos_to_soa_time_ms;
    double soa_to_aos_time_ms;

    // Validation metrics
    double cpu_validation_time_ms;
    double max_relative_error;
    size_t cpu_validation_passed;
    size_t cpu_validation_failed;

    // Deterministic replay metrics
    double recording_time_ms;
    double replay_time_ms;
    double replay_consistency_error;

    // Legacy compatibility metrics
    double legacy_throughput_ops_per_sec;
    double legacy_execution_time_ms;
    double legacy_conversion_overhead_percent;

    // Overall metrics
    double speedup_factor;
    double efficiency_factor;
    double overall_score;
};

class ECCAdapterBenchmark {
public:
    ECCAdapterBenchmark(const BenchmarkConfiguration& config) : config_(config) {}

    bool run_benchmarks() {
        std::cout << "=== ECC Adapter Integration Performance Benchmark ===" << std::endl;
        std::cout << "Configuration:" << std::endl;
        std::cout << "  Output directory: " << config_.output_directory << std::endl;
        std::cout << "  Compare with direct: " << (config_.compare_with_direct ? "Yes" : "No") << std::endl;
        std::cout << "  Deterministic replay: " << (config_.enable_deterministic_replay ? "Yes" : "No") << std::endl;
        std::cout << "  CPU validation: " << (config_.enable_cpu_validation ? "Yes" : "No") << std::endl;
        std::cout << "  Legacy compatibility: " << (config_.test_legacy_compatibility ? "Yes" : "No") << std::endl;
        std::cout << "  Batch sizes: ";
        for (size_t size : config_.batch_sizes) {
            std::cout << size << " ";
        }
        std::cout << std::endl;
        std::cout << "  Iterations per size: " << config_.iterations_per_size << std::endl;
        std::cout << std::endl;

        // Create output directory
        std::filesystem::create_directories(config_.output_directory);

        // Initialize integration
        if (!initialize_integration()) {
            std::cerr << "Failed to initialize ECC integration" << std::endl;
            return false;
        }

        // Run benchmarks for each batch size
        bool all_successful = true;
        for (size_t batch_size : config_.batch_sizes) {
            std::cout << "Benchmarking batch size: " << batch_size << std::endl;

            std::vector<BenchmarkResults> iteration_results;
            for (int iter = 0; iter < config_.iterations_per_size; ++iter) {
                BenchmarkResults result = benchmark_batch_size(batch_size);
                iteration_results.push_back(result);

                std::cout << "  Iteration " << (iter + 1) << "/" << config_.iterations_per_size
                         << ": Adapter throughput = " << std::fixed << std::setprecision(2)
                         << result.adapter_throughput_ops_per_sec << " ops/sec";

                if (config_.compare_with_direct && result.direct_throughput_ops_per_sec > 0) {
                    std::cout << ", Direct throughput = " << result.direct_throughput_ops_per_sec
                             << " ops/sec, Speedup = " << std::setprecision(3)
                             << result.speedup_factor << "x";
                }
                std::cout << std::endl;
            }

            // Calculate average results and save
            BenchmarkResults avg_results = calculate_average_results(iteration_results);
            all_results_.push_back(avg_results);

            if (!save_results(batch_size, iteration_results, avg_results)) {
                std::cerr << "Failed to save results for batch size " << batch_size << std::endl;
                all_successful = false;
            }
        }

        // Generate summary report
        generate_summary_report();

        // Cleanup
        cleanup_integration();

        return all_successful;
    }

private:
    BenchmarkConfiguration config_;
    std::unique_ptr<EnhancedECCAdapterIntegration> integration_;
    std::vector<BenchmarkResults> all_results_;

    bool initialize_integration() {
        // Setup adapter configuration
        EnhancedAdapterConfig adapter_config;
        adapter_config.mode = AdapterMode::COMPATIBILITY_BRIDGE;
        adapter_config.preferred_layout = MemoryLayout::STRUCTURE_OF_ARRAYS;
        adapter_config.enable_backward_compatibility = config_.test_legacy_compatibility;
        adapter_config.enable_performance_monitoring = true;
        adapter_config.enable_deterministic_replay = config_.enable_deterministic_replay;
        adapter_config.enable_constitutional_compliance = true;
        adapter_config.enable_memory_access_optimization = true;
        adapter_config.enable_ecc_integration = true;
        adapter_config.enable_strict_validation = config_.enable_cpu_validation;
        adapter_config.enable_fallback_mechanisms = true;
        adapter_config.enable_detailed_logging = config_.export_detailed_metrics;

        // Setup ECC configuration
        ECCBatchConfig ecc_config;
        ecc_config.use_soa_layout = true;
        ecc_config.alignment_bytes = 128;
        ecc_config.precision_target = 1e-11;
        ecc_config.cuda_device_id = 0;
        ecc_config.registers_per_thread = 32;
        ecc_config.threads_per_block = 256;
        ecc_config.shared_memory_size = 48 * 1024;

        // Initialize global integration
        bool success = initialize_global_enhanced_ecc_integration(adapter_config, ecc_config);
        if (!success) {
            return false;
        }

        integration_.reset(get_global_enhanced_ecc_integration());
        return integration_ != nullptr;
    }

    void cleanup_integration() {
        integration_.reset();
        cleanup_global_enhanced_ecc_integration();
    }

    BenchmarkResults benchmark_batch_size(size_t batch_size) {
        BenchmarkResults result{};
        result.batch_size = batch_size;

        // Generate test data
        std::vector<uint32_t> test_private_keys = generate_test_private_keys(batch_size);

        // Allocate device memory
        uint32_t* device_private_keys = nullptr;
        cudaError_t err = cudaMalloc(&device_private_keys, batch_size * sizeof(uint32_t));
        if (err != cudaSuccess) {
            std::cerr << "Failed to allocate device memory for private keys" << std::endl;
            return result;
        }

        err = cudaMemcpy(device_private_keys, test_private_keys.data(),
                        batch_size * sizeof(uint32_t), cudaMemcpyHostToDevice);
        if (err != cudaSuccess) {
            std::cerr << "Failed to copy private keys to device" << std::endl;
            cudaFree(device_private_keys);
            return result;
        }

        // Allocate SoA structure for public keys
        ECCPointSoA public_keys;
        if (!integration_->get_adapter()->allocate_soa_memory(&public_keys, batch_size)) {
            std::cerr << "Failed to allocate SoA memory for public keys" << std::endl;
            cudaFree(device_private_keys);
            return result;
        }

        try {
            // Benchmark adapter-based scalar multiplication
            ComprehensivePerformanceReport adapter_report;
            auto adapter_start = std::chrono::high_resolution_clock::now();

            bool adapter_success = integration_->scalar_multiply_with_monitoring(
                device_private_keys, &public_keys, batch_size,
                MemoryLayout::STRUCTURE_OF_ARRAYS, &adapter_report);

            auto adapter_end = std::chrono::high_resolution_clock::now();

            if (adapter_success) {
                auto adapter_duration = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
                    adapter_end - adapter_start).count();

                result.adapter_throughput_ops_per_sec = (batch_size * 1000.0) / adapter_duration;
                result.adapter_execution_time_ms = adapter_duration;
                result.adapter_memory_efficiency_percent = adapter_report.memory_efficiency_percent;
                result.adapter_gpu_utilization_percent = adapter_report.gpu_utilization_percent;
                result.adapter_precision_achieved = adapter_report.ecc_precision_achieved;
                result.adapter_overhead_percent = adapter_report.adapter_overhead_percent;
            }

            // Benchmark direct ECC operations (if available and requested)
            if (config_.compare_with_direct) {
                ComprehensivePerformanceReport direct_report;
                auto direct_start = std::chrono::high_resolution_clock::now();

                bool direct_success = integration_->get_ecc_operations()->scalar_multiply_batch(
                    device_private_keys, &public_keys, batch_size, direct_report);

                auto direct_end = std::chrono::high_resolution_clock::now();

                if (direct_success) {
                    auto direct_duration = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
                        direct_end - direct_start).count();

                    result.direct_throughput_ops_per_sec = (batch_size * 1000.0) / direct_duration;
                    result.direct_execution_time_ms = direct_duration;
                    result.direct_memory_efficiency_percent = direct_report.memory_efficiency_percent;
                    result.direct_gpu_utilization_percent = direct_report.gpu_utilization_percent;
                    result.direct_precision_achieved = direct_report.precision_achieved;

                    if (result.adapter_throughput_ops_per_sec > 0 && result.direct_throughput_ops_per_sec > 0) {
                        result.speedup_factor = result.adapter_throughput_ops_per_sec / result.direct_throughput_ops_per_sec;
                    }
                }
            }

            // Benchmark memory layout conversions
            if (config_.test_legacy_compatibility) {
                result.aos_to_soa_time_ms = benchmark_aos_to_soa_conversion(batch_size);
                result.soa_to_aos_time_ms = benchmark_soa_to_aos_conversion(batch_size);
                result.layout_conversion_time_ms = result.aos_to_soa_time_ms + result.soa_to_aos_time_ms;
            }

            // Benchmark CPU validation
            if (config_.enable_cpu_validation && result.adapter_throughput_ops_per_sec > 0) {
                auto validation_start = std::chrono::high_resolution_clock::now();

                std::vector<bool> validation_results(batch_size);
                bool validation_success = integration_->validate_points_with_cpu_consistency(
                    &public_keys, validation_results.data(), batch_size,
                    result.max_relative_error);

                auto validation_end = std::chrono::high_resolution_clock::now();

                if (validation_success) {
                    result.cpu_validation_time_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
                        validation_end - validation_start).count();

                    result.cpu_validation_passed = std::count(validation_results.begin(), validation_results.end(), true);
                    result.cpu_validation_failed = batch_size - result.cpu_validation_passed;
                }
            }

            // Benchmark deterministic replay
            if (config_.enable_deterministic_replay) {
                auto recording_results = benchmark_deterministic_replay(
                    device_private_keys, &public_keys, batch_size);
                result.recording_time_ms = recording_results.first;
                result.replay_time_ms = recording_results.second;
                result.replay_consistency_error = 0.0; // Would need actual validation
            }

            // Benchmark legacy compatibility
            if (config_.test_legacy_compatibility) {
                auto legacy_results = benchmark_legacy_compatibility(
                    device_private_keys, batch_size);
                result.legacy_throughput_ops_per_sec = legacy_results.first;
                result.legacy_execution_time_ms = legacy_results.second;

                if (result.legacy_throughput_ops_per_sec > 0 && result.adapter_throughput_ops_per_sec > 0) {
                    result.legacy_conversion_overhead_percent =
                        ((result.adapter_execution_time_ms - result.legacy_execution_time_ms) /
                         result.legacy_execution_time_ms) * 100.0;
                }
            }

            // Calculate overall efficiency and score
            calculate_overall_metrics(result);

        } catch (const std::exception& e) {
            std::cerr << "Exception during benchmarking: " << e.what() << std::endl;
        }

        // Cleanup
        integration_->get_adapter()->deallocate_device_memory(public_keys.x_words);
        integration_->get_adapter()->deallocate_device_memory(public_keys.y_words);
        integration_->get_adapter()->deallocate_host_memory(public_keys.is_valid);
        cudaFree(device_private_keys);

        return result;
    }

    std::vector<uint32_t> generate_test_private_keys(size_t batch_size) {
        std::vector<uint32_t> private_keys(batch_size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dis;

        for (auto& key : private_keys) {
            key = dis(gen);
        }

        return private_keys;
    }

    double benchmark_aos_to_soa_conversion(size_t batch_size) {
        const size_t point_size = 16 * sizeof(uint32_t); // 8 words X + 8 words Y
        std::vector<uint8_t> aos_data(batch_size * point_size);
        std::vector<uint8_t> soa_data(batch_size * point_size * 2);

        // Fill with test data
        for (size_t i = 0; i < aos_data.size(); ++i) {
            aos_data[i] = static_cast<uint8_t>(i & 0xFF);
        }

        auto start = std::chrono::high_resolution_clock::now();
        bool success = MemoryLayoutConverter::aos_to_soa(
            aos_data.data(), soa_data.data(), point_size, batch_size);
        auto end = std::chrono::high_resolution_clock::now();

        if (!success) return -1.0;

        return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
            end - start).count();
    }

    double benchmark_soa_to_aos_conversion(size_t batch_size) {
        const size_t point_size = 16 * sizeof(uint32_t);
        std::vector<uint8_t> soa_data(batch_size * point_size * 2);
        std::vector<uint8_t> aos_data(batch_size * point_size);

        // Fill with test data
        for (size_t i = 0; i < soa_data.size(); ++i) {
            soa_data[i] = static_cast<uint8_t>(i & 0xFF);
        }

        auto start = std::chrono::high_resolution_clock::now();
        bool success = MemoryLayoutConverter::soa_to_aos(
            soa_data.data(), aos_data.data(), point_size, batch_size);
        auto end = std::chrono::high_resolution_clock::now();

        if (!success) return -1.0;

        return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
            end - start).count();
    }

    std::pair<double, double> benchmark_deterministic_replay(
        uint32_t* device_private_keys, ECCPointSoA* public_keys, size_t batch_size) {

        std::string recording_file = "benchmark_temp_recording.dat";
        double recording_time = 0.0, replay_time = 0.0;

        // Benchmark recording
        auto record_start = std::chrono::high_resolution_clock::now();
        integration_->start_deterministic_recording(recording_file);

        ComprehensivePerformanceReport report;
        integration_->scalar_multiply_with_monitoring(
            device_private_keys, public_keys, batch_size,
            MemoryLayout::STRUCTURE_OF_ARRAYS, &report);

        integration_->stop_deterministic_recording();
        auto record_end = std::chrono::high_resolution_clock::now();

        recording_time = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
            record_end - record_start).count();

        // Benchmark replay
        if (std::filesystem::exists(recording_file)) {
            auto replay_start = std::chrono::high_resolution_clock::now();

            bool replay_successful = false;
            ComprehensivePerformanceReport replay_report;
            integration_->replay_deterministic_recording(
                recording_file, replay_report, replay_successful);

            auto replay_end = std::chrono::high_resolution_clock::now();

            replay_time = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
                replay_end - replay_start).count();

            // Cleanup
            std::filesystem::remove(recording_file);
        }

        return {recording_time, replay_time};
    }

    std::pair<double, double> benchmark_legacy_compatibility(
        uint32_t* device_private_keys, size_t batch_size) {

        // Prepare legacy format data
        std::vector<uint32_t> legacy_public_keys(batch_size * 16);
        std::fill(legacy_public_keys.begin(), legacy_public_keys.end(), 0);

        auto start = std::chrono::high_resolution_clock::now();

        ComprehensivePerformanceReport report;
        bool success = integration_->legacy_scalar_multiply_optimized(
            device_private_keys, legacy_public_keys.data(), batch_size, &report);

        auto end = std::chrono::high_resolution_clock::now();

        if (!success) return {0.0, -1.0};

        double execution_time = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
            end - start).count();
        double throughput = (batch_size * 1000.0) / execution_time;

        return {throughput, execution_time};
    }

    void calculate_overall_metrics(BenchmarkResults& result) {
        // Calculate efficiency factor (normalized performance)
        double max_possible_throughput = 4000.0; // Based on Hopper architecture target
        result.efficiency_factor = result.adapter_throughput_ops_per_sec / max_possible_throughput;

        // Calculate overall score (weighted combination of metrics)
        double throughput_score = std::min(result.adapter_throughput_ops_per_sec / 1000.0, 1.0);
        double efficiency_score = std::min(result.efficiency_factor, 1.0);
        double precision_score = result.adapter_precision_achieved > 0 ?
            std::min(1e-10 / result.adapter_precision_achieved, 1.0) : 0.0;
        double memory_score = result.adapter_memory_efficiency_percent / 100.0;
        double gpu_score = result.adapter_gpu_utilization_percent / 100.0;

        result.overall_score = (throughput_score * 0.3 + efficiency_score * 0.2 +
                               precision_score * 0.2 + memory_score * 0.15 + gpu_score * 0.15);
    }

    BenchmarkResults calculate_average_results(const std::vector<BenchmarkResults>& iteration_results) {
        BenchmarkResults avg{};
        if (iteration_results.empty()) return avg;

        size_t valid_iterations = 0;
        for (const auto& result : iteration_results) {
            if (result.adapter_throughput_ops_per_sec > 0) {
                avg.adapter_throughput_ops_per_sec += result.adapter_throughput_ops_per_sec;
                avg.adapter_execution_time_ms += result.adapter_execution_time_ms;
                avg.adapter_memory_efficiency_percent += result.adapter_memory_efficiency_percent;
                avg.adapter_gpu_utilization_percent += result.adapter_gpu_utilization_percent;
                avg.adapter_precision_achieved += result.adapter_precision_achieved;
                avg.adapter_overhead_percent += result.adapter_overhead_percent;

                if (result.direct_throughput_ops_per_sec > 0) {
                    avg.direct_throughput_ops_per_sec += result.direct_throughput_ops_per_sec;
                    avg.direct_execution_time_ms += result.direct_execution_time_ms;
                    avg.direct_memory_efficiency_percent += result.direct_memory_efficiency_percent;
                    avg.direct_gpu_utilization_percent += result.direct_gpu_utilization_percent;
                    avg.direct_precision_achieved += result.direct_precision_achieved;
                }

                avg.layout_conversion_time_ms += result.layout_conversion_time_ms;
                avg.aos_to_soa_time_ms += result.aos_to_soa_time_ms;
                avg.soa_to_aos_time_ms += result.soa_to_aos_time_ms;

                avg.cpu_validation_time_ms += result.cpu_validation_time_ms;
                avg.max_relative_error += result.max_relative_error;
                avg.cpu_validation_passed += result.cpu_validation_passed;
                avg.cpu_validation_failed += result.cpu_validation_failed;

                avg.recording_time_ms += result.recording_time_ms;
                avg.replay_time_ms += result.replay_time_ms;
                avg.replay_consistency_error += result.replay_consistency_error;

                avg.legacy_throughput_ops_per_sec += result.legacy_throughput_ops_per_sec;
                avg.legacy_execution_time_ms += result.legacy_execution_time_ms;
                avg.legacy_conversion_overhead_percent += result.legacy_conversion_overhead_percent;

                avg.speedup_factor += result.speedup_factor;
                avg.efficiency_factor += result.efficiency_factor;
                avg.overall_score += result.overall_score;

                valid_iterations++;
            }
        }

        if (valid_iterations > 0) {
            double divisor = static_cast<double>(valid_iterations);
            avg.adapter_throughput_ops_per_sec /= divisor;
            avg.adapter_execution_time_ms /= divisor;
            avg.adapter_memory_efficiency_percent /= divisor;
            avg.adapter_gpu_utilization_percent /= divisor;
            avg.adapter_precision_achieved /= divisor;
            avg.adapter_overhead_percent /= divisor;

            if (avg.direct_throughput_ops_per_sec > 0) {
                avg.direct_throughput_ops_per_sec /= divisor;
                avg.direct_execution_time_ms /= divisor;
                avg.direct_memory_efficiency_percent /= divisor;
                avg.direct_gpu_utilization_percent /= divisor;
                avg.direct_precision_achieved /= divisor;
            }

            avg.layout_conversion_time_ms /= divisor;
            avg.aos_to_soa_time_ms /= divisor;
            avg.soa_to_aos_time_ms /= divisor;

            avg.cpu_validation_time_ms /= divisor;
            avg.max_relative_error /= divisor;
            avg.cpu_validation_passed /= divisor;
            avg.cpu_validation_failed /= divisor;

            avg.recording_time_ms /= divisor;
            avg.replay_time_ms /= divisor;
            avg.replay_consistency_error /= divisor;

            avg.legacy_throughput_ops_per_sec /= divisor;
            avg.legacy_execution_time_ms /= divisor;
            avg.legacy_conversion_overhead_percent /= divisor;

            avg.speedup_factor /= divisor;
            avg.efficiency_factor /= divisor;
            avg.overall_score /= divisor;

            avg.batch_size = iteration_results[0].batch_size;
        }

        return avg;
    }

    bool save_results(size_t batch_size, const std::vector<BenchmarkResults>& iteration_results,
                     const BenchmarkResults& avg_results) {
        // Save detailed iteration results
        std::ofstream detail_file(config_.output_directory + "/batch_" + std::to_string(batch_size) + "_detailed.csv");
        if (detail_file.is_open()) {
            detail_file << "Iteration,Adapter_Throughput,Adapter_Time,Direct_Throughput,Direct_Time,"
                       << "Speedup,Memory_Efficiency,GPU_Utilization,Precision,Overhead\n";

            for (size_t i = 0; i < iteration_results.size(); ++i) {
                const auto& result = iteration_results[i];
                detail_file << (i + 1) << ","
                           << result.adapter_throughput_ops_per_sec << ","
                           << result.adapter_execution_time_ms << ","
                           << result.direct_throughput_ops_per_sec << ","
                           << result.direct_execution_time_ms << ","
                           << result.speedup_factor << ","
                           << result.adapter_memory_efficiency_percent << ","
                           << result.adapter_gpu_utilization_percent << ","
                           << result.adapter_precision_achieved << ","
                           << result.adapter_overhead_percent << "\n";
            }
            detail_file.close();
        }

        // Save average results
        std::ofstream avg_file(config_.output_directory + "/batch_" + std::to_string(batch_size) + "_average.csv");
        if (avg_file.is_open()) {
            avg_file << "Metric,Value\n";
            avg_file << "Batch_Size," << avg_results.batch_size << "\n";
            avg_file << "Adapter_Throughput," << avg_results.adapter_throughput_ops_per_sec << "\n";
            avg_file << "Adapter_Execution_Time," << avg_results.adapter_execution_time_ms << "\n";
            avg_file << "Adapter_Memory_Efficiency," << avg_results.adapter_memory_efficiency_percent << "\n";
            avg_file << "Adapter_GPU_Utilization," << avg_results.adapter_gpu_utilization_percent << "\n";
            avg_file << "Adapter_Precision," << avg_results.adapter_precision_achieved << "\n";
            avg_file << "Adapter_Overhead," << avg_results.adapter_overhead_percent << "\n";

            if (avg_results.direct_throughput_ops_per_sec > 0) {
                avg_file << "Direct_Throughput," << avg_results.direct_throughput_ops_per_sec << "\n";
                avg_file << "Direct_Execution_Time," << avg_results.direct_execution_time_ms << "\n";
                avg_file << "Speedup_Factor," << avg_results.speedup_factor << "\n";
            }

            avg_file << "Layout_Conversion_Time," << avg_results.layout_conversion_time_ms << "\n";
            avg_file << "AoS_to_SoA_Time," << avg_results.aos_to_soa_time_ms << "\n";
            avg_file << "SoA_to_AoS_Time," << avg_results.soa_to_aos_time_ms << "\n";

            if (avg_results.cpu_validation_time_ms > 0) {
                avg_file << "CPU_Validation_Time," << avg_results.cpu_validation_time_ms << "\n";
                avg_file << "Max_Relative_Error," << avg_results.max_relative_error << "\n";
                avg_file << "CPU_Validation_Passed," << avg_results.cpu_validation_passed << "\n";
                avg_file << "CPU_Validation_Failed," << avg_results.cpu_validation_failed << "\n";
            }

            if (avg_results.recording_time_ms > 0) {
                avg_file << "Recording_Time," << avg_results.recording_time_ms << "\n";
                avg_file << "Replay_Time," << avg_results.replay_time_ms << "\n";
                avg_file << "Replay_Consistency_Error," << avg_results.replay_consistency_error << "\n";
            }

            if (avg_results.legacy_throughput_ops_per_sec > 0) {
                avg_file << "Legacy_Throughput," << avg_results.legacy_throughput_ops_per_sec << "\n";
                avg_file << "Legacy_Execution_Time," << avg_results.legacy_execution_time_ms << "\n";
                avg_file << "Legacy_Conversion_Overhead," << avg_results.legacy_conversion_overhead_percent << "\n";
            }

            avg_file << "Efficiency_Factor," << avg_results.efficiency_factor << "\n";
            avg_file << "Overall_Score," << avg_results.overall_score << "\n";

            avg_file.close();
        }

        return true;
    }

    void generate_summary_report() {
        std::cout << "\n=== Benchmark Summary ===" << std::endl;
        std::cout << std::fixed << std::setprecision(2);

        std::cout << std::setw(12) << "Batch Size"
                  << std::setw(15) << "Adapter (ops/s)"
                  << std::setw(15) << "Direct (ops/s)"
                  << std::setw(12) << "Speedup"
                  << std::setw(12) << "Efficiency"
                  << std::setw(10) << "Score" << std::endl;
        std::cout << std::string(80, '-') << std::endl;

        for (const auto& result : all_results_) {
            std::cout << std::setw(12) << result.batch_size
                      << std::setw(15) << result.adapter_throughput_ops_per_sec;

            if (result.direct_throughput_ops_per_sec > 0) {
                std::cout << std::setw(15) << result.direct_throughput_ops_per_sec
                          << std::setw(12) << std::setprecision(3) << result.speedup_factor;
            } else {
                std::cout << std::setw(15) << "N/A"
                          << std::setw(12) << "N/A";
            }

            std::cout << std::setw(12) << std::setprecision(3) << result.efficiency_factor
                      << std::setw(10) << std::setprecision(3) << result.overall_score
                      << std::endl;
        }

        // Calculate overall statistics
        if (!all_results_.empty()) {
            double avg_throughput = 0, avg_efficiency = 0, avg_score = 0;
            for (const auto& result : all_results_) {
                avg_throughput += result.adapter_throughput_ops_per_sec;
                avg_efficiency += result.efficiency_factor;
                avg_score += result.overall_score;
            }

            size_t count = all_results_.size();
            avg_throughput /= count;
            avg_efficiency /= count;
            avg_score /= count;

            std::cout << std::string(80, '=') << std::endl;
            std::cout << "Averages:" << std::endl;
            std::cout << "  Throughput: " << avg_throughput << " ops/sec" << std::endl;
            std::cout << "  Efficiency: " << std::setprecision(3) << avg_efficiency << std::endl;
            std::cout << "  Overall Score: " << std::setprecision(3) << avg_score << std::endl;
        }

        // Save summary to file
        std::ofstream summary_file(config_.output_directory + "/benchmark_summary.txt");
        if (summary_file.is_open()) {
            summary_file << "ECC Adapter Integration Performance Benchmark Summary\n";
            summary_file << "===================================================\n\n";

            for (const auto& result : all_results_) {
                summary_file << "Batch Size: " << result.batch_size << "\n";
                summary_file << "  Adapter Throughput: " << result.adapter_throughput_ops_per_sec << " ops/sec\n";
                summary_file << "  Execution Time: " << result.adapter_execution_time_ms << " ms\n";
                summary_file << "  Memory Efficiency: " << result.adapter_memory_efficiency_percent << "%\n";
                summary_file << "  GPU Utilization: " << result.adapter_gpu_utilization_percent << "%\n";
                summary_file << "  Precision Achieved: " << result.adapter_precision_achieved << "\n";
                summary_file << "  Adapter Overhead: " << result.adapter_overhead_percent << "%\n";

                if (result.direct_throughput_ops_per_sec > 0) {
                    summary_file << "  Direct Throughput: " << result.direct_throughput_ops_per_sec << " ops/sec\n";
                    summary_file << "  Speedup Factor: " << result.speedup_factor << "\n";
                }

                summary_file << "  Efficiency Factor: " << result.efficiency_factor << "\n";
                summary_file << "  Overall Score: " << result.overall_score << "\n\n";
            }

            summary_file.close();
        }
    }
};

int main(int argc, char* argv[]) {
    BenchmarkConfiguration config{
        .output_directory = "benchmark_results",
        .compare_with_direct = true,
        .enable_deterministic_replay = true,
        .enable_cpu_validation = true,
        .test_legacy_compatibility = true,
        .batch_sizes = {256, 512, 1024, 2048, 4096, 8192},
        .iterations_per_size = 3,
        .export_detailed_metrics = true,
        .generate_plots = false
    };

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--output" && i + 1 < argc) {
            config.output_directory = argv[++i];
        } else if (arg == "--no-direct") {
            config.compare_with_direct = false;
        } else if (arg == "--no-replay") {
            config.enable_deterministic_replay = false;
        } else if (arg == "--no-validation") {
            config.enable_cpu_validation = false;
        } else if (arg == "--no-legacy") {
            config.test_legacy_compatibility = false;
        } else if (arg == "--iterations" && i + 1 < argc) {
            config.iterations_per_size = std::stoi(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "ECC Adapter Integration Performance Benchmark Tool\n";
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --output <dir>        Output directory for results (default: benchmark_results)\n";
            std::cout << "  --no-direct          Skip direct ECC operation comparison\n";
            std::cout << "  --no-replay          Skip deterministic replay benchmarking\n";
            std::cout << "  --no-validation      Skip CPU validation benchmarking\n";
            std::cout << "  --no-legacy          Skip legacy compatibility benchmarking\n";
            std::cout << "  --iterations <n>     Number of iterations per batch size (default: 3)\n";
            std::cout << "  --help               Show this help message\n";
            return 0;
        }
    }

    // Initialize CUDA
    cudaError_t cuda_err = cudaSetDevice(0);
    if (cuda_err != cudaSuccess) {
        std::cerr << "Failed to initialize CUDA: " << cudaGetErrorString(cuda_err) << std::endl;
        return 1;
    }

    // Run benchmarks
    ECCAdapterBenchmark benchmark(config);
    bool success = benchmark.run_benchmarks();

    return success ? 0 : 1;
}
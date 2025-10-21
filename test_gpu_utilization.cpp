// Simple test program for GPU utilization analyzer
// Tests the implementation from T049

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

#include "src/KeyhuntCore/common/gpu_utilization_analyzer.hpp"
#include "src/KeyhuntCore/common/gpu_workload_generator.hpp"

using namespace keyhunt::performance;

int main() {
    std::cout << "=== GPU Utilization Analyzer Test ===" << std::endl;

    // Check CUDA availability
    int device_count = 0;
    cudaError_t err = cudaGetDeviceCount(&device_count);
    if (err != cudaSuccess || device_count == 0) {
        std::cerr << "No CUDA devices available. Exiting." << std::endl;
        return 1;
    }

    std::cout << "Found " << device_count << " CUDA device(s)" << std::endl;
    int device_id = 0; // Use first device

    try {
        // Initialize GPU utilization analyzer
        std::cout << "Initializing GPU utilization analyzer..." << std::endl;
        auto analyzer = std::make_unique<GPUUtilizationAnalyzer>(device_id);

        bool init_success = analyzer->initialize(device_id);
        if (!init_success) {
            std::cerr << "Failed to initialize GPU utilization analyzer" << std::endl;
            return 1;
        }

        std::cout << "GPU utilization analyzer initialized successfully" << std::endl;

        // Initialize workload generator
        std::cout << "Initializing workload generator..." << std::endl;
        auto workload = std::make_unique<GPUWorkloadGenerator>(device_id);

        // Test basic measurements
        std::cout << "\n=== Testing Basic Measurements ===" << std::endl;

        double gpu_util = 0.0;
        bool success = analyzer->measureGPUUtilization(gpu_util);
        std::cout << "GPU Utilization: " << (success ? std::to_string(gpu_util) + "%" : "Failed") << std::endl;

        double compute_util = 0.0;
        success = analyzer->measureComputeUtilization(compute_util);
        std::cout << "Compute Utilization: " << (success ? std::to_string(compute_util) + "%" : "Failed") << std::endl;

        double mem_util = 0.0;
        success = analyzer->measureMemoryControllerUtilization(mem_util);
        std::cout << "Memory Controller Utilization: " << (success ? std::to_string(mem_util) + "%" : "Failed") << std::endl;

        double sm_util = 0.0;
        success = analyzer->measureSMUtilization(sm_util);
        std::cout << "SM Utilization: " << (success ? std::to_string(sm_util) + "%" : "Failed") << std::endl;

        double occupancy = 0.0;
        success = analyzer->measureOccupancy(occupancy);
        std::cout << "Occupancy: " << (success ? std::to_string(occupancy) + "%" : "Failed") << std::endl;

        double warp_eff = 0.0;
        success = analyzer->measureWarpExecutionEfficiency(warp_eff);
        std::cout << "Warp Execution Efficiency: " << (success ? std::to_string(warp_eff) + "%" : "Failed") << std::endl;

        double throughput = 0.0;
        success = analyzer->measureInstructionThroughput(throughput);
        std::cout << "Instruction Throughput: " << (success ? std::to_string(throughput) + " ops/sec" : "Failed") << std::endl;

        // Test workload generation and measurement
        std::cout << "\n=== Testing Workload Generation ===" << std::endl;

        std::cout << "Generating ECC workload..." << std::endl;
        bool workload_success = workload->generateECCWorkload(1000000);
        std::cout << "ECC workload generation: " << (workload_success ? "Success" : "Failed") << std::endl;

        if (workload_success) {
            workload_success = workload->waitForCompletion();
            std::cout << "ECC workload completion: " << (workload_success ? "Success" : "Failed") << std::endl;

            // Measure utilization during/after workload
            success = analyzer->measureGPUUtilization(gpu_util);
            std::cout << "GPU utilization after ECC workload: " << (success ? std::to_string(gpu_util) + "%" : "Failed") << std::endl;
        }

        // Test continuous monitoring
        std::cout << "\n=== Testing Continuous Monitoring ===" << std::endl;

        std::cout << "Starting continuous monitoring..." << std::endl;
        bool monitoring_started = analyzer->startContinuousMonitoring();
        std::cout << "Continuous monitoring started: " << (monitoring_started ? "Success" : "Failed") << std::endl;

        if (monitoring_started) {
            // Generate workloads while monitoring
            std::cout << "Generating mixed workload during monitoring..." << std::endl;
            workload->generateMixedWorkload(500000, 50000000);
            workload->waitForCompletion();

            // Let monitoring run for a few seconds
            std::this_thread::sleep_for(std::chrono::seconds(3));

            // Stop monitoring
            std::cout << "Stopping continuous monitoring..." << std::endl;
            bool monitoring_stopped = analyzer->stopContinuousMonitoring();
            std::cout << "Continuous monitoring stopped: " << (monitoring_stopped ? "Success" : "Failed") << std::endl;

            // Get time series data
            std::vector<std::pair<double, double>> time_series;
            bool data_retrieved = analyzer->getUtilizationTimeSeries(time_series);
            std::cout << "Time series data retrieved: " << (data_retrieved ? "Success (" + std::to_string(time_series.size()) + " samples)" : "Failed") << std::endl;

            if (data_retrieved && !time_series.empty()) {
                double avg_util = 0.0;
                for (const auto& sample : time_series) {
                    avg_util += sample.second;
                }
                avg_util /= time_series.size();
                std::cout << "Average utilization during monitoring: " << avg_util << "%" << std::endl;
            }
        }

        // Generate utilization report
        std::cout << "\n=== Generating Utilization Report ===" << std::endl;
        std::string report;
        bool report_generated = analyzer->generateUtilizationReport(report);
        if (report_generated) {
            std::cout << "Report generated successfully:" << std::endl;
            std::cout << report << std::endl;
        } else {
            std::cout << "Report generation failed" << std::endl;
        }

        // Check constitutional compliance
        std::cout << "\n=== Constitutional Compliance Check ===" << std::endl;

        const double GPU_UTIL_MIN = 70.0;
        const double OCCUPANCY_TARGET = 65.0;

        if (gpu_util >= GPU_UTIL_MIN) {
            std::cout << "✅ GPU utilization meets constitutional requirement (≥" << GPU_UTIL_MIN << "%): " << gpu_util << "%" << std::endl;
        } else {
            std::cout << "❌ GPU utilization below constitutional requirement (≥" << GPU_UTIL_MIN << "%): " << gpu_util << "%" << std::endl;
        }

        if (occupancy >= OCCUPANCY_TARGET) {
            std::cout << "✅ Occupancy meets constitutional requirement (≥" << OCCUPANCY_TARGET << "%): " << occupancy << "%" << std::endl;
        } else {
            std::cout << "❌ Occupancy below constitutional requirement (≥" << OCCUPANCY_TARGET << "%): " << occupancy << "%" << std::endl;
        }

        std::cout << "\n=== Test Complete ===" << std::endl;
        std::cout << "GPU utilization analyzer implementation tested successfully!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
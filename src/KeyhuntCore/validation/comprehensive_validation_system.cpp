// Puzzle71 Technical Debt Repair - Comprehensive Validation System Integration Implementation
// Task: T059 [P] [US3] Implement SHA-256 protected baseline and result validation
// Phase: Phase 4B - User Story 3 Integration Testing and Validation System

#include "comprehensive_validation_system.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <random>
#include <thread>
#include <future>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <ctime>

namespace puzzle71 {
namespace validation {

// ComprehensiveValidationSystem implementation
ComprehensiveValidationSystem::ComprehensiveValidationSystem()
    : system_status_(ValidationSystemStatus::UNINITIALIZED),
      validation_in_progress_(false), validation_cancelled_(false),
      total_validations_run_(0), successful_validations_(0), failed_validations_(0),
      selected_gpu_device_(-1), progress_monitoring_active_(false),
      current_progress_(0.0), telemetry_collector_(nullptr),
      ci_interface_(nullptr) {

    // Initialize validation framework counters
    framework_validation_counts_["ecc_validation"] = 0;
    framework_validation_counts_["deterministic_replay"] = 0;
    framework_validation_counts_["constitutional_compliance"] = 0;
    framework_validation_counts_["integration_testing"] = 0;
    framework_validation_counts_["baseline_validation"] = 0;

    framework_success_counts_["ecc_validation"] = 0;
    framework_success_counts_["deterministic_replay"] = 0;
    framework_success_counts_["constitutional_compliance"] = 0;
    framework_success_counts_["integration_testing"] = 0;
    framework_success_counts_["baseline_validation"] = 0;

    // Create framework instances
    ecc_framework_ = std::make_shared<ECCValidationFramework>();
    deterministic_framework_ = std::make_shared<DeterministicReplayFramework>();
    constitutional_framework_ = std::make_shared<ConstitutionalComplianceFramework>();
    integration_framework_ = std::make_shared<IntegrationTestingFramework>();
    baseline_validator_ = std::make_shared<SHA256BaselineValidator>();

    // Create default CI integration and telemetry
    ci_interface_ = std::make_shared<DefaultCIIntegration>();
    telemetry_collector_ = std::make_shared<DefaultTelemetryCollector>();
}

ComprehensiveValidationSystem::~ComprehensiveValidationSystem() {
    shutdown();
}

bool ComprehensiveValidationSystem::initialize(const ValidationSystemConfig& config) {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (system_status_ != ValidationSystemStatus::UNINITIALIZED) {
        setError("System already initialized");
        return false;
    }

    setSystemStatus(ValidationSystemStatus::INITIALIZING);
    config_ = config;

    if (config_.enable_detailed_logging) {
        std::cout << "Initializing Comprehensive Validation System..." << std::endl;
        std::cout << "  ECC Validation: " << (config_.enable_ecc_validation ? "ENABLED" : "DISABLED") << std::endl;
        std::cout << "  Deterministic Replay: " << (config_.enable_deterministic_replay ? "ENABLED" : "DISABLED") << std::endl;
        std::cout << " Constitutional Compliance: " << (config_.enableconstitutional_compliance ? "ENABLED" : "DISABLED") << std::endl;
        std::cout << " Integration Testing: " << (config_.enable_integration_testing ? "ENABLED" : "DISABLED") << std::endl;
        std::cout << " Baseline Validation: " << (config_.enable_baseline_validation ? "ENABLED" : "DISABLED") << std::endl;
    }

    // Initialize GPU devices
    if (!initializeGPUDevices()) {
        setError("Failed to initialize GPU devices");
        setSystemStatus(ValidationSystemStatus::ERROR);
        return false;
    }

    // Initialize individual frameworks
    bool frameworks_initialized = true;

    if (config_.enable_ecc_validation) {
        if (!ecc_framework_->initialize()) {
            setError("Failed to initialize ECC validation framework");
            frameworks_initialized = false;
        }
    }

    if (config_.enable_deterministic_replay) {
        if (!deterministic_framework_->initialize()) {
            setError("Failed to initialize deterministic replay framework");
            frameworks_initialized = false;
        }
    }

    if (config_.enableconstitutional_compliance) {
        if (!constitutional_framework_->initialize()) {
            setError("Failed to initialize constitutional compliance framework");
            frameworks_initialized = false;
        }
    }

    if (config_.enable_integration_testing) {
        IntegrationTestConfig integration_config;
        integration_config.batch_size = config_.batch_size;
        integration_config.performance_tolerance = config_.performance_tolerance / 100.0;
        integration_config.precision_tolerance = config_.precision_tolerance;
        integration_config.enable_deterministic_tests = config_.enable_deterministic_replay;
        integration_config.enableconstitutional_tests = config_.enableconstitutional_compliance;
        integration_config.enable_performance_tests = config_.enable_ecc_validation;
        integration_config.test_output_dir = config_.validation_output_directory + "integration/";
        integration_config.baseline_dir = config_.baseline_directory;

        if (!integration_framework_->initialize(integration_config)) {
            setError("Failed to initialize integration testing framework");
            frameworks_initialized = false;
        }
    }

    if (config_.enable_baseline_validation) {
        if (!baseline_validator_->initialize(config_.baseline_directory,
                                                 config_.validation_output_directory + "baselines/")) {
            setError("Failed to initialize baseline validator");
            frameworks_initialized = false;
        }
    }

    if (!frameworks_initialized) {
        setSystemStatus(ValidationSystemStatus::ERROR);
        return false;
    }

    // Initialize CI integration
    if (config_.enable_ci_integration) {
        initializeCIIntegration();
    }

    // Start progress monitoring if enabled
    if (config_.enable_progress_monitoring) {
        startProgressMonitoring();
    }

    // Generate validation session ID
    current_validation_session_id_ = comprehensive_validation_utils::generateValidationSessionId();

    setSystemStatus(ValidationSystemStatus::READY);

    if (config_.enable_detailed_logging) {
        std::cout << "Comprehensive Validation System initialized successfully" << std::endl;
        std::cout << "  Available GPU devices: " << available_gpu_devices_.size() << std::endl;
        std::cout << "  Selected GPU device: " << selected_gpu_device_ << std::endl;
        std::cout << "  Validation session ID: " << current_validation_session_id_ << std::endl;
    }

    return true;
}

bool ComprehensiveValidationSystem::shutdown() {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (system_status_ == ValidationSystemStatus::UNINITIALIZED) {
        return true; // Already shutdown
    }

    // Stop progress monitoring
    if (progress_monitoring_active_) {
        stopProgressMonitoring();
    }

    // Cancel any in-progress validation
    if (validation_in_progress_) {
        validation_cancelled_ = true;
    }

    // Wait for any current validation to complete
    while (validation_in_progress_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Shutdown frameworks
    ecc_framework_.reset();
    deterministic_framework_.reset();
    constitutional_framework_.reset();
    integration_framework_.reset();
    baseline_validator_.reset();
    ci_interface_.reset();
    telemetry_collector_.reset();

    setSystemStatus(ValidationSystemStatus::UNINITIALIZED);

    if (config_.enable_detailed_logging) {
        std::cout << "Comprehensive Validation System shutdown" << std::endl;
    }

    return true;
}

bool ComprehensiveValidationSystem::runComprehensiveValidation(ComprehensiveValidationResult& result) {
    return runValidationWithTimeout(result, config_.max_validation_timeout);
}

bool ComprehensiveValidationSystem::runValidationWithTimeout(ComprehensiveResult& result,
                                                           std::chrono::seconds timeout) {
    clearError();
    result = ComprehensiveValidationResult();

    if (!isSystemReady()) {
        setError("System not ready for validation");
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(state_mutex_);
        if (validation_in_progress_) {
            setError("Validation already in progress");
            return false;
        }
        validation_in_progress_ = true;
        validation_cancelled_ = false;
    }

    // Notify CI integration
    if (config_.enable_ci_integration) {
        notifyCIValidationStart();
    }

    // Start telemetry collection
    if (config_.enable_performance_telemetry) {
        startTelemetrySession();
    }

    // Set validation metadata
    result.validation_timestamp = std::chrono::system_clock::now();
    result.ci_pipeline_id = current_validation_session_id_;
    result.ci_environment = "production"; // TODO: Get from environment

    validation_start_time_ = std::chrono::system_clock::now();

    // Run validation with timeout
    std::future<bool> validation_future = std::async(std::launch::async,
        [this, &result]() {
            return this->orchestrateComprehensiveValidation(result);
        });

    // Wait for completion or timeout
    auto future_status = validation_future.wait_for(timeout);

    bool success = false;
    if (future_status == std::future_status::ready) {
        success = validation_future.get();
    } else {
        validation_cancelled_ = true;
        setError("Validation timeout");
        result.error_summary = "Validation exceeded timeout limit of " +
                         std::to_string(timeout.count()) + " seconds";
    }

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        validation_in_progress_ = false;
    }

    // Calculate total execution time
    result.total_execution_time_ms = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now() - validation_start_time_).count() / 1000.0;

    // Stop telemetry collection
    if (config_.enable_performance_telemetry) {
        stopTelemetrySession();
    }

    // Collect telemetry data
    if (config_.enable_performance_telemetry) {
        collectTelemetryData(result);
    }

    // Analyze results
    analyzeValidationResults(result);
    computeCompliancePercentage(result);
    detectCriticalIssues(result);

    // Generate CI reports
    if (config_.enable_ci_integration) {
        generateCIReports(result);
        validateCIGates(result);
        uploadCIArtifacts(result);
        notifyCIValidationComplete(result);
    }

    // Update statistics
    total_validations_run_++;
    if (success) {
        successful_validations_++;
    } else {
        failed_validations_++;
    }

    if (config_.enable_detailed_logging) {
        std::cout << "Comprehensive validation completed" << std::endl;
        std::cout << "  Overall status: " << (result.overall_passed ? "PASSED" : "FAILED") << std::endl;
        std::cout << "  Execution time: " << std::fixed << std::setprecision(2)
                  << result.total_execution_time_ms << " ms" << std::endl;
        std::cout << "  Constitutional compliance: " << std::fixed << std::setprecision(1)
                  << result.overall_compliance_percentage << "%" << std::endl;
    }

    return success && result.overall_passed;
}

bool ComprehensiveValidationSystem::orchestrateComprehensiveValidation(ComprehensiveValidationResult& result) {
    updateProgress("Initializing validation", 5.0);

    // Define validation pipeline steps
    std::vector<std::string> validation_pipeline;

    if (config_.enable_ecc_validation) {
        validation_pipeline.push_back("ecc_validation");
    }
    if (config_.enable_deterministic_replay) {
        validation_pipeline.push_back("deterministic_replay");
    }
    if (config_.enableconstitutional_compliance) {
        validation_pipeline.push_back("constitutional_compliance");
    }
    if (config_.enable_integration_testing) {
        validation_pipeline.push_back("integration_testing");
    }
    if (config_.enable_baseline_validation) {
        validation_pipeline.push_back("baseline_validation");
    }

    // Check for validation cancellation
    if (isValidationCancellationRequested()) {
        result.error_summary = "Validation cancelled by user or system";
        return false;
    }

    // Execute validation pipeline
    bool pipeline_success = executeValidationPipeline(validation_pipeline, result);

    return pipeline_success;
}

bool ComprehensiveValidationSystem::executeValidationPipeline(std::vector<std::string>& pipeline_steps,
                                                        ComprehensiveValidationResult& result) {
    bool pipeline_success = true;

    for (size_t i = 0; i < pipeline_steps.size(); ++i) {
        const std::string& step_name = pipeline_steps[i];
        double progress = 5.0 + (static_cast<double>(i) / pipeline_steps.size()) * 85.0;
        updateProgress(step_name, progress);

        // Check for cancellation before each step
        if (isValidationCancellationRequested()) {
            result.error_summary = "Validation cancelled during: " + step_name;
            return false;
        }

        // Define validation step functions
        std::map<std::string, std::function<bool(ComprehensiveValidationResult&)>> step_functions = {
            {"ecc_validation", [this](ComprehensiveValidationResult& r) { return this->validateECCFramework(r); }},
            {"deterministic_replay", [this](ComprehensiveValidationResult& r) { return this->validateDeterministicReplayFramework(r); }},
            {"constitutional_compliance", [this](ComprehensiveValidationResult& r) { return this->validateConstitutionalComplianceFramework(r); }},
            {"integration_testing", [this](ComprehensiveValidationResult& r) { return this->validateIntegrationTestingFramework(r); }},
            {"baseline_validation", [this](ComprehensiveValidationResult& r) { return this->validateBaselineFramework(r); }}
        };

        // Execute the step
        auto step_it = step_functions.find(step_name);
        if (step_it != step_functions.end()) {
            bool step_success = executeValidationStep(step_name, step_it->second, result);

            if (!step_success) {
                pipeline_success = false;
                addDiagnosticMessage("Validation step failed: " + step_name);
                break; // Continue with remaining steps to collect more diagnostic data
            }
        } else {
            addDiagnosticMessage("Unknown validation step: " + step_name);
            pipeline_success = false;
        }
    }

    updateProgress("Finalizing results", 95.0);

    return pipeline_success;
}

bool ComprehensiveValidationSystem::executeValidationStep(const std::string& step_name,
                                                          std::function<bool(ComprehensiveValidationResult&)> step_function,
                                                          ComprehensiveValidationResult& result) {
    auto step_start_time = startTiming();

    try {
        if (config_.enable_detailed_logging) {
            std::cout << "Executing validation step: " << step_name << std::endl;
        }

        bool success = step_function(result);

        double step_time = endTiming(step_start_time);

        if (config_.enable_detailed_logging) {
            std::cout << "Step " << step_name << " completed in "
                      << std::fixed << std::setprecision(2) << step_time << " ms - "
                      << (success ? "SUCCESS" : "FAILED") << std::endl;
        }

        return success;

    } catch (const std::exception& e) {
        std::string error_msg = "Exception in " + step_name + ": " + std::string(e.what());
        addDiagnosticMessage(error_msg);
        setError(error_msg);
        return false;
    }
}

bool ComprehensiveValidationSystem::validateECCFramework(ComprehensiveValidationResult& result) {
    if (!config_.enable_ecc_validation) {
        return true; // Skip if disabled
    }

    framework_validation_counts_["ecc_validation"]++;

    // Generate test data for ECC validation
    auto test_batch = integration_framework_->generateTestBatch(config_.batch_size);

    // Run ECC validation
    bool ecc_success = ecc_framework_->validateBatchOperations(test_batch);

    result.ecc_validation_passed = ecc_success;
    if (!ecc_success) {
        addDiagnosticMessage("ECC validation failed");
    }

    if (ecc_success) {
        framework_success_counts_["ecc_validation"]++;
    }

    return ecc_success;
}

bool ComprehensiveValidationSystem::validateDeterministicReplayFramework(ComprehensiveValidationResult& result) {
    if (!config_.enable_deterministic_replay) {
        return true; // Skip if disabled
    }

    framework_validation_counts_["deterministic_replay"]++;

    // Generate test data for deterministic replay validation
    auto test_batch = integration_framework_->generateTestBatch(config_.batch_size);

    // Run deterministic replay validation
    bool replay_success = deterministic_framework_->testDeterministicBehavior(test_batch);

    result.deterministic_replay_passed = replay_success;
    if (!replay_success) {
        addDiagnosticMessage("Deterministic replay validation failed");
    }

    if (replay_success) {
        framework_success_counts_["deterministic_replay"]++;
    }

    return replay_success;
}

bool ComprehensiveValidationFramework::validateConstitutionalComplianceFramework(ComprehensiveResult& result) {
    if (!config_.enableconstitutional_compliance) {
        return true; // Skip if disabled
    }

    framework_validation_counts_["constitutional_compliance"]++;

    // Run constitutional compliance validation
    std::vector<validation::ComplianceViolation> violations;
    bool compliance_success = constitutional_framework_->validateAllConstraints(violations);

    result.constitutional_compliance_passed = compliance_success && violations.empty();
    result.constitutional_violations = violations;
    result.constitutional_compliance_summary = constitutional_framework_->getCurrentMetrics().performance_metrics;

    if (!result.constitutional_compliance_passed) {
        addDiagnosticMessage("Constitutional compliance validation failed");
        if (!violations.empty()) {
            addDiagnosticMessage("Violations detected: " + std::to_string(violations.size()));
        }
    }

    if (compliance_success && violations.empty()) {
        framework_success_counts_["constitutional_compliance"]++;
    }

    return compliance_success && violations.empty();
}

bool ComprehensiveValidationSystem::validateIntegrationTestingFramework(ComprehensiveResult& result) {
    if (!config_.enable_integration_testing) {
        return true; // Skip if disabled
    }

    framework_validation_counts_["integration_testing"]++;

    // Run integration testing
    std::vector<validation::IntegrationTestResult> integration_results;
    bool integration_success = integration_framework_->runAllIntegrationTests(integration_results);

    result.integration_testing_passed = integration_success;
    result.integration_test_results = integration_results;

    if (!integration_success) {
        addDiagnosticMessage("Integration testing validation failed");

        // Count failed integration tests
        size_t failed_count = std::count_if(integration_results.begin(), integration_results.end(),
                                      [](const validation::IntegrationTestResult& r) { return !r.passed; });
        addDiagnosticMessage("Failed integration tests: " + std::to_string(failed_count));
    }

    if (integration_success) {
        framework_success_counts_["integration_testing"]++;
    }

    return integration_success;
}

bool ComprehensiveValidationSystem::validateBaselineFramework(ComprehensiveResult& result) {
    if (!config_.enable_baseline_validation) {
        return true; // Skip if disabled
    }

    framework_validation_counts_["baseline_validation"]++;

    // Check baseline integrity first
    std::vector<std::string> corrupted_baselines;
    bool integrity_success = baseline_validator_->verifyAllBaselinesIntegrity(corrupted_baselines);

    result.integrity_violations = corrupted_baselines;
    result.framework_integrity_status["baseline_validation"] = integrity_success;

    if (!integrity_success) {
        addDiagnosticMessage("Baseline integrity validation failed");
        if (!corrupted_baselines.empty()) {
            addDiagnosticMessage("Corrupted baselines: " + std::to_string(corrupted_baselines.size()));
        }
    }

    // Perform baseline comparisons
    if (integrity_success) {
        // Generate current test data for comparison
        auto test_batch = integration_framework_->generateTestBatch(config_.batch_size);
        std::map<std::string, double> current_metrics = {
            {"test_throughput", 1000.0}, // Placeholder metrics
            {"memory_efficiency", 95.0},
            {"gpu_utilization", 80.0}
        };

        std::vector<validation::ValidationResult> baseline_results;
        bool baseline_success = baseline_validator_->validateBatchAgainstBaselines(
            {{"comprehensive_test", test_batch}},
            {{"comprehensive_test", current_metrics}},
            baseline_results);

        result.baseline_validation_passed = baseline_success;
        result.baseline_validation_results = baseline_results;

        if (!baseline_success) {
            addDiagnosticMessage("Baseline comparison validation failed");
        }

        if (baseline_success) {
            framework_success_counts_["baseline_validation"]++;
        }
    }

    return integrity_success;
}

bool ComprehensiveValidationSystem::aggregateFrameworkResults(ComprehensiveValidationResult& result) {
    // Calculate overall passed status
    result.overall_passed = true;

    if (config_.enable_ecc_validation && !result.ecc_validation_passed) {
        result.overall_passed = false;
    }
    if (config_.enable_deterministic_replay && !result.deterministic_replay_passed) {
        result.overall_passed = false;
    }
    if (config_.enableconstitutional_compliance && !result.constitutional_compliance_passed) {
        result.overall_passed = false;
    }
    if (config_.enable_integration_testing && !result.integration_testing_passed) {
        result.overall_passed = false;
    }
    if (config_.enable_baseline_validation && !result.baseline_validation_passed) {
        result.overall_passed = false;
    }

    // Aggregate performance metrics from integration tests
    if (!result.integration_test_results.empty()) {
        for (const auto& test_result : result.integration_test_results) {
            for (const auto& [metric_name, value] : test_result.performance_metrics) {
                result.aggregate_performance_metrics[metric_name] += value;
            }
        }
    }

    // Add constitutional compliance metrics
    for (const auto& [metric_name, value] : result.constitutional_compliance_summary) {
        result.aggregate_performance_metrics["constitutional_" + metric_name] = value;
    }

    // Calculate average for aggregated metrics
    for (auto& [metric_name, value] : result.aggregate_performance_metrics) {
        if (!result.integration_test_results.empty()) {
            value /= result.integration_test_results.size();
        }
    }

    return true;
}

bool ComprehensiveValidationSystem::analyzeValidationResults(ComprehensiveValidationResult& result) {
    // Analyze performance regressions
    analyzePerformanceRegressions(result);

    // Verify constitutional compliance
    verifyConstitutionalV55Compliance(result);

    // Check system integrity
    verifyFrameworkIntegrity(result.framework_integrity_status);

    return true;
}

bool ComprehensiveValidationSystem::analyzePerformanceRegressions(ComprehensiveValidationResult& result) {
    result.performance_regressions.clear();
    result.performance_improvements.clear();

    // Define performance thresholds
    std::map<std::string, double> performance_thresholds = {
        {"gpu_utilization", config_.constitutional_gpu_utilization_minimum},
        {"memory_efficiency", config_.constitutional_memory_efficiency_minimum},
        {"test_throughput", 1000.0} // Minimum throughput
    };

    for (const auto& [metric_name, current_value] : result.aggregate_performance_metrics) {
        auto threshold_it = performance_thresholds.find(metric_name);
        if (threshold_it != performance_thresholds.end()) {
            double threshold = threshold_it->second;

            if (current_value < threshold) {
                double regression_percentage = ((current_value - threshold) / threshold) * 100.0;
                result.performance_regressions[metric_name] = regression_percentage;
            } else {
                double improvement_percentage = ((current_value - threshold) / threshold) * 100.0;
                result.performance_improvements[metric_name] = improvement_percentage;
            }
        }
    }

    return !result.performance_regressions.empty();
}

bool ComprehensiveValidationSystem::verifyConstitutionalV55Compliance(const ComprehensiveValidationResult& result) {
    result.constitutional_v55_compliant = result.constitutional_compliance_passed;

    // Check if all constitutional principles are satisfied
    if (result.constitutional_compliance_passed) {
        // Check GPU utilization
        auto gpu_util_it = result.constitutional_compliance_summary.find("gpu_utilization");
        if (gpu_util_it != result.constitutional_compliance_summary.end()) {
            result.constitutional_v55_compliant =
                (gpu_util_it->second >= config_.constitutional_gpu_utilization_minimum);
        }

        // Check memory efficiency
        auto mem_eff_it = result.constitutional_compliance_summary.find("memory_efficiency");
        if (mem_eff_it != result.constitutional_compliance_summary.end()) {
            result.constitutional_v55_compliant =
                (mem_eff_it->second >= config_.constitutional_memory_efficiency_minimum);
        }

        // Check determinism requirement
        auto det_it = result.constitutional_compliance_summary.find("determinism_exact_match_rate");
        if (det_it != result.constitutional_compliance_summary.end()) {
            result.constitutional_v55_compliant =
                (det_it->second >= config_.constitutional_determinism_requirement);
        }
    }

    // Constitutional v5.5 requires 100% compliance
    result.constitutional_v55_compliant = result.constitutional_v55_compliant &&
                                              result.constitutional_violations.empty();

    return result.constitutional_v55_compliant;
}

bool ComprehensiveValidationSystem::verifyFrameworkIntegrity(std::map<std::string, bool>& integrity_status) {
    integrity_status.clear();

    if (config_.enable_ecc_validation) {
        integrity_status["ecc_validation"] = true; // Assume good if no errors reported
    }
    if (config_.enable_deterministic_replay) {
        integrity_status["deterministic_replay"] = true;
    }
    if (config_.enableconstitutional_compliance) {
        integrity_status["constitutional_compliance"] = result.framework_integrity_status.count("constitutional_compliance") > 0 ?
                                                    result.framework_integrity_status.at("constitutional_compliance") : true;
    }
    if (config_.enable_integration_testing) {
        integrity_status["integration_testing"] = true;
    }
    if (config_.enable_baseline_validation) {
        integrity_status["baseline_validation"] = result.framework_integrity_status.count("baseline_validation") > 0 ?
                                                 result.framework_integrity_status.at("baseline_validation") : true;
    }

    // Return true if all enabled frameworks have good integrity
    for (const auto& [framework, integrity] : integrity_status) {
        if (!integrity) {
            return false;
        }
    }

    return true;
}

bool ComprehensiveValidationSystem::computeCompliancePercentage(ComprehensiveValidationResult& result) {
    size_t total_frameworks = 0;
    size_t passed_frameworks = 0;

    if (config_.enable_ecc_validation) {
        total_frameworks++;
        if (result.ecc_validation_passed) passed_frameworks++;
    }
    if (config_.enable_deterministic_replay) {
        total_frameworks++;
        if (result.deterministic_replay_passed) passed_frameworks++;
    }
    if (config_.enable_constitutional_compliance) {
        total_frameworks++;
        if (result.constitutional_compliance_passed) passed_frameworks++;
    }
    if (config_.enable_integration_testing) {
        total_frameworks++;
        if (result.integration_testing_passed) passed_frameworks++;
    }
    if (config_.enable_baseline_validation) {
        total_frameworks++;
        if (result.baseline_validation_passed) passed_frameworks++;
    }

    result.overall_compliance_percentage = (total_frameworks > 0) ?
                                                 (static_cast<double>(passed_frameworks) / total_frameworks) * 100.0 : 100.0;

    return true;
}

bool ComprehensiveValidationSystem::detectCriticalIssues(ComprehensiveResult& result) {
    std::vector<std::string> critical_issues;

    // Check for critical constitutional violations
    if (!result.constitutional_v55_compliant) {
        critical_issues.push_back("Constitutional v5.5 compliance violation");
    }

    // Check for critical performance regressions
    for (const auto& [metric, regression] : result.performance_regressions) {
        if (regression < -20.0) { // More than 20% regression is critical
            critical_issues.push_back("Critical performance regression in " + metric + ": " +
                                std::to_string(regression) + "%");
        }
    }

    // Check for integrity violations
    for (const auto& violation : result.integrity_violations) {
        critical_issues.push_back("Integrity violation in: " + violation);
    }

    // Add to error summary
    if (!critical_issues.empty()) {
        result.error_summary = "Critical issues detected: " + std::to_string(critical_issues.size());
        for (const auto& issue : critical_issues) {
            result.detailed_errors.push_back(issue);
        }
    }

    return critical_issues.empty();
}

bool ComprehensiveValidationSystem::generateCIReports(const ComprehensiveValidationResult& result) {
    if (!config_.enable_ci_integration || !ci_interface_) {
        return true; // CI integration disabled
    }

    bool success = true;

    // Create artifact directory
    if (!std::filesystem::exists(config_.ci_artifact_directory)) {
        std::filesystem::create_directories(config_.ci_artifact_directory);
    }

    // Generate JSON report
    if (config_.ci_report_format == "json" || config_.ci_report_format == "all") {
        std::string json_report_path = config_.ci_artifact_directory + "/validation_report.json";
        bool json_success = ci_interface_->generateCIReport(result, json_report_path);
        success = success && json_success;
    }

    // Generate JUnit report
    if (config_.generate_junit_reports) {
        std::string junit_report_path = config_.ci_artifact_directory + "/junit_report.xml";
        bool junit_success = ci_interface_->generateJunitReport(result, junit_report_path);
        success = success && junit_success;
    }

    // Generate HTML report
    if (config_.generate_html_reports) {
        std::string html_report_path = config_.ci_artifact_directory + "/validation_report.html";
        bool html_success = ci_interface_->generateHTMLReport(result, html_report_path);
        success = success && html_success;
    }

    // Generate comprehensive report
    std::string comprehensive_report_path = config_.ci_artifact_directory + "/comprehensive_report.txt";
    std::string comprehensive_report;
    generateComprehensiveReport(result, comprehensive_report);

    std::ofstream report_file(comprehensive_report_path);
    if (report_file.is_open()) {
        report_file << comprehensive_report;
        report_file.close();
    }

    return success;
}

bool ComprehensiveValidationSystem::validateCIGates(const ComprehensiveValidationResult& result) {
    if (!config_.enable_ci_integration || !ci_interface_) {
        return true; // CI integration disabled
    }

    bool gates_passed = true;

    // Check quality gates
    if (!ci_interface_->validateQualityGates(result)) {
        gates_passed = false;
    }

    // Check compliance gates
    if (!ci_interface_->checkComplianceGates(result)) {
        gates_passed = false;
    }

    // Check performance gates
    if (!ci_interface_->checkPerformanceGates(result)) {
        gates_passed = false;
    }

    return gates_passed;
}

bool ComprehensiveValidationSystem::uploadCIArtifacts(const ComprehensiveValidationResult& result) {
    if (!config_.enable_ci_integration || !ci_interface_) {
        return true; // CI integration disabled
    }

    std::vector<std::string> artifact_paths;

    // Add generated report files
    artifact_paths.push_back(config_.ci_artifact_directory + "/validation_report.json");
    artifact_paths.push_back(config_.ci_artifact_directory + "/junit_report.xml");
    artifact_paths.push_back(config_.ci_artifact_directory + "/validation_report.html");
    artifact_paths.push_back(config_.ci_artifact_directory + "/comprehensive_report.txt");

    // Upload artifacts
    return ci_interface_->uploadArtifacts(artifact_paths);
}

// Private helper methods
std::chrono::high_resolution_clock::time_point ComprehensiveValidationSystem::startTiming() {
    return std::chrono::high_resolution_clock::now();
}

double ComprehensiveValidationSystem::endTiming(const std::chrono::high_resolution_clock::time_point& start_time) {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    return duration.count() / 1000.0; // Convert to milliseconds
}

void ComprehensiveValidationSystem::setError(const std::string& error) {
    last_error_ = error;
    if (config_.enable_detailed_logging) {
        std::cerr << "ComprehensiveValidationSystem Error: " << error << std::endl;
    }
}

void ComprehensiveValidationSystem::addDiagnosticMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(diagnostics_mutex_);
    diagnostic_messages_.push_back(message);
    if (config_.enable_detailed_logging) {
        std::cout << "Diagnostic: " << message << std::endl;
    }
}

void ComprehensiveValidationSystem::clearError() {
    last_error_.clear();
}

void ComprehensiveValidationSystem::clearDiagnostics() {
    std::lock_guard<std::mutex> lock(diagnostics_mutex_);
    diagnostic_messages_.clear();
}

void ComprehensiveValidationSystem::setSystemStatus(ValidationSystemStatus status) {
    system_status_ = status;
    if (config_.enable_detailed_logging) {
        std::cout << "System status changed to: " << getSystemStatusString() << std::endl;
    }
}

bool ComprehensiveValidationSystem::isValidationCancellationRequested() const {
    return validation_cancelled_.load();
}

std::string ComprehensiveValidationSystem::getSystemStatusString() const {
    switch (system_status_) {
        case ValidationSystemStatus::UNINITIALIZED: return "UNINITIALIZED";
        case ValidationSystemStatus::INITIALIZING: return "INITIALIZING";
        case ValidationSystemStatus::READY: return "READY";
        case ValidationSystemStatus::VALIDATING: return "VALIDATING";
        case ValidationSystemStatus::VALIDATION_COMPLETE: return "VALIDATION_COMPLETE";
        case ValidationSystemStatus::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

bool ComprehensiveValidationSystem::isSystemReady() const {
    return system_status_ == ValidationSystemStatus::READY;
}

bool ComprehensiveValidationSystem::isValidationInProgress() const {
    return validation_in_progress_.load();
}

double ComprehensiveValidationSystem::getAverageValidationTime() const {
    if (total_validations_run_.load() == 0) return 0.0;

    double total_time = 0.0;
    size_t valid_count = 0;

    // We would need to track individual validation times
    // For now, return a reasonable estimate based on system configuration
    return 30000.0; // 30 seconds average
}

std::map<std::string, double> ComprehensiveValidationSystem::getFrameworkSuccessRates() const {
    std::map<std::string, double> rates;

    for (const auto& [framework, count] : framework_validation_counts_) {
        size_t success_count = framework_success_counts_.at(framework);
        rates[framework] = (count > 0) ? (static_cast<double>(success_count) / count) * 100.0 : 100.0;
    }

    return rates;
}

bool ComprehensiveValidationSystem::initializeGPUDevices() {
    available_gpu_devices_.clear();

    int device_count = 0;
    cudaError_t err = cudaGetDeviceCount(&device_count);
    if (err != cudaSuccess || device_count == 0) {
        setError("No CUDA devices available");
        return false;
    }

    // Determine which devices to use
    if (config_.target_gpu_devices.empty()) {
        // Use all available devices
        for (int i = 0; i < device_count; ++i) {
            available_gpu_devices_.push_back(i);
        }
    } else {
        // Use specified devices only
        for (int device : config_.target_gpu_devices) {
            if (device >= 0 && device < device_count) {
                available_gpu_devices_.push_back(device);
            } else {
                setError("Invalid GPU device specified: " + std::to_string(device));
                return false;
            }
        }
    }

    // Select optimal device (first available for now)
    if (!available_gpu_devices_.empty()) {
        selected_gpu_device_ = available_gpu_devices_[0];
        cudaSetDevice(selected_gpu_device_);
    }

    return true;
}

bool ComprehensiveValidationSystem::selectOptimalGPUDevice() {
    if (available_gpu_devices_.empty()) {
        return false;
    }

    // For now, select the first available device
    selected_gpu_device_ = available_gpu_devices_[0];
    return cudaSetDevice(selected_gpu_device_);
}

std::vector<int> ComprehensiveValidationSystem::getAvailableGPUDevices() {
    return available_gpu_devices_;
}

void ComprehensiveValidationSystem::startProgressMonitoring() {
    if (progress_monitoring_active_) {
        return; // Already monitoring
    }

    progress_monitoring_active_ = true;
    progress_start_time_ = std::chrono::steady_clock::now();
    current_progress_ = 0.0;

    progress_monitor_thread_ = std::thread(&ComprehensiveValidationSystem::progressMonitoringLoop, this);
}

void ComprehensiveValidationSystem::stopProgressMonitoring() {
    if (!progress_monitoring_active_) {
        return; // Not monitoring
    }

    progress_monitoring_active_ = false;
    if (progress_monitor_thread_.joinable()) {
        progress_monitor_thread_.join();
    }
}

void ComprehensiveValidationSystem::progressMonitoringLoop() {
    while (progress_monitoring_active_) {
        std::this_thread::sleep_for(std::chrono::seconds(config_.progress_update_interval));

        if (!progress_monitoring_active_) break;

        // Check if validation completed
        if (!isValidationInProgress()) {
            progress_monitoring_active_ = false;
            break;
        }
    }
}

void ComprehensiveValidationSystem::updateProgress(const std::string& current_step, double progress_percentage) {
    current_progress_ = progress_percentage;
    current_validation_step_ = current_step;

    if (config_.enable_detailed_logging) {
        std::cout << "Progress: " << std::fixed << std::setprecision(1)
                  << progress_percentage << "% - " << current_step << std::endl;
    }
}

bool ComprehensiveValidationSystem::startTelemetrySession() {
    if (!config_.enable_performance_telemetry || !telemetry_collector_) {
        return true; // Telemetry disabled
    }

    current_telemetry_session_id_ = comprehensive_validation_utils::generateValidationSessionId();
    return telemetry_collector_->startTelemetryCollection(current_telemetry_session_id_);
}

bool ComprehensiveValidationSystem::recordFrameworkMetrics(const std::string& framework_name,
                                                   const std::map<std::string, double>& metrics) {
    if (!config_.enable_performance_telemetry || !telemetry_collector_) {
        return true; // Telemetry disabled
    }

    return telemetry_collector_->recordFrameworkMetrics(framework_name, metrics);
}

bool ComprehensiveValidationSystem::stopTelemetrySession() {
    if (!config_.enable_performance_telemetry || !telemetry_collector_) {
        return true; // Telemetry disabled
    }

    return telemetry_collector_->stopTelemetryCollection();
}

bool ComprehensiveValidationSystem::collectTelemetryData(const ComprehensiveValidationResult& result) {
    if (!config_.enable_performance_telemetry || !telemetry_collector_) {
        return true; // Telemetry disabled
    }

    // Record framework metrics for each enabled framework
    if (config_.enable_ecc_validation) {
        recordFrameworkMetrics("ecc_validation", result.aggregate_performance_metrics);
    }
    if (config_.enable_deterministic_replay) {
        recordFrameworkMetrics("deterministic_replay", result.aggregate_performance_metrics);
    }
    if (config_.enable_constitutional_compliance) {
        recordFrameworkMetrics("constitutional_compliance", result.constitutional_compliance_summary);
    }
    if (config_.enable_integration_testing) {
        // Record integration test metrics
        for (const auto& test_result : result.integration_test_results) {
            recordFrameworkMetrics("integration_testing_" + test_result.test_name, test_result.performance_metrics);
        }
    }

    // Record overall metrics
    recordFrameworkMetrics("overall_validation", {
        {"total_time_ms", result.total_execution_time_ms},
        {"compliance_percentage", result.overall_compliance_percentage},
        {"frameworks_passed", static_cast<double>(result.ecc_validation_passed +
                                                    result.deterministic_replay_passed +
                                                    result.constitutional_compliance_passed +
                                                    result.integration_testing_passed +
                                                    result.baseline_validation_passed)},
        {"regressions_detected", static_cast<double>(result.performance_regressions.size())},
        {"improvements_detected", static_cast<double>(result.performance_improvements.size())}
    });

    return true;
}

bool ComprehensiveValidationSystem::initializeCIIntegration() {
    if (!ci_interface_ || !config_.enable_ci_integration) {
        return true; // CI integration disabled
    }

    // Set pipeline metadata from environment
    ci_interface_->setPipelineMetadata(
        current_validation_session_id_,
        std::to_string(std::time(nullptr)), // Build number from environment
        "production" // Environment from environment
    );

    return true;
}

bool ComprehensiveValidationSystem::notifyCIValidationStart() {
    if (!ci_interface_ || !config_.enable_ci_integration) {
        return true; // CI integration disabled
    }

    return ci_interface_->notifyValidationStart();
}

bool ComprehensiveValidationSystem::notifyCIValidationComplete(const ComprehensiveValidationResult& result) {
    if (!ci_interface_ || !config_.enable_ci_integration) {
        return true; // CI integration disabled
    }

    return ci_interface_->notifyValidationComplete(result);
}

bool ComprehensiveValidationSystem::notifyCIValidationFailure(const std::string& error) {
    if (!ci_interface_ || !config_.enable_ci_integration) {
        return true; // CI integration disabled
    }

    return ci_interface_->notifyValidationFailure(error);
}

// Default CI Integration implementation
bool DefaultCIIntegration::generateCIReport(const ComprehensiveValidationResult& result,
                                               const std::string& output_path) {
    std::ofstream file(output_path);
    if (!file.is_open()) {
        return false;
    }

    // Generate JSON report
    std::ostringstream json;
    json << "{" << std::endl;
    json << "  \"validation_session_id\": \"" << result.ci_pipeline_id << "\"," << std::endl;
    json << "  \"validation_timestamp\": \"" <<
        std::put_time(std::localtime(&std::tm{}), "%Y-%m-%d %H:%M:%S") << "\""
        << std::endl;
    json << "  \"overall_passed\": " << (result.overall_passed ? "true" : "false") << "," << std::endl;
    json << "  \"execution_time_ms\": " << result.total_execution_time_ms << "," << std::endl;
    json << "  \"constitutional_compliance_percentage\": " << result.overall_compliance_percentage << "," << std::endl;
    json << "  \"constitutional_v55_compliant\": " << (result.constitutional_v55_compliant ? "true" : "false") << "," << std::endl;
    json << "  \"framework_results\": {" << std::endl;

    // ECC validation results
    json << "    \"ecc_validation_passed\": " << (result.ecc_validation_passed ? "true" : "false") << "," << std::endl;
    json << "    \"deterministic_replay_passed\": " << (result.deterministic_replay_passed ? "true" : "false") << "," << std::endl;
    json << "    \"constitutional_compliance_passed\": " << (result.constitutional_compliance_passed ? "true" : "false") << "," << std::endl;
    json << "    \"integration_testing_passed\": " << (result.integration_testing_passed ? "true" : "false") << "," << std::endl;
    json << "    \"baseline_validation_passed\": " << (result.baseline_validation_passed ? "true" : "false") << std::endl;

    // Performance metrics
    json << "    \"aggregate_performance_metrics\": {" << std::endl;
    for (const auto& [name, value] : result.aggregate_performance_metrics) {
        json << "      \"" << name << "\": " << std::scientific << std::setprecision(15) << value;
        if (name != *result.aggregate_performance_metrics.rbegin()) {
            json << "," << std::endl;
        }
    }
    json << "    }," << std::endl;

    // Constitutional compliance summary
    json << "    \"constitutional_compliance_summary\": {" << std::endl;
    for (const auto& [name, value] : result.constitutional_compliance_summary) {
        json << "      \"" << name << "\": " << std::fixed << std::setprecision(6) << value;
        if (name != *result.constitutional_compliance_summary.rbegin()) {
            json << "," << std::endl;
        }
    }
    json << "    }," << std::endl;

    // Violations
    json << "    \"constitutional_violations\": [" << std::endl;
    for (size_t i = 0; i < result.constitutional_violations.size(); ++i) {
        json << "      \"" << result.constitutional_violations[i].constraint_name << "\"";
        if (i < result.constitutional_violations.size() - 1) {
            json << "," << std::endl;
        }
    }
    json << "    ]," << std::endl;

    // Integration test results summary
    json << "    \"integration_test_summary\": {" << std::endl;
    json << "      \"total_tests\": " << result.integration_test_results.size() << "," << std::endl;
    json << "      \"passed_tests\": " << std::count_if(result.integration_test_results.begin(),
                                              result.integration_test_results.end(),
                                              [](const validation::IntegrationTestResult& r) { return r.passed; }) << "," << std::endl;
    json << "      \"failed_tests\": " << std::count_if(result.integration_test_results.begin(),
                                              result.integration_test_results.end(),
                                              [](const validation::IntegrationTestResult& r) { return !r.passed; }) << std::endl;
    json << "    }," << std::endl;

    // Performance regressions and improvements
    json << "    \"performance_regressions\": {" << std::endl;
    for (const auto& [metric, regression] : result.performance_regressions) {
        json << "      \"" << metric << "\": " << std::fixed << std::setprecision(2) << regression;
        if (metric != *result.performance_regressions.rbegin()) {
            json << "," << std::endl;
        }
    }
    json << "    }," << std::endl;

    json << "    \"performance_improvements\": {" << std::endl;
    for (const auto& [metric, improvement] : result.performance_improvements) {
        json << "      \"" << metric << "\": " << std::fixed << std::setprecision(2) << improvement;
        if (metric != *result.performance_improvements.rbegin()) {
            json << "," << std::endl;
        }
    }
    json << "    }" << std::endl;

    // Error reporting
    json << "  \"error_summary\": \"" << result.error_summary << "\"," << std::endl;
    json << "  \"detailed_errors\": [" << std::endl;
    for (size_t i = 0; i < result.detailed_errors.size(); ++i) {
        json << "    \"" << result.detailed_errors[i] << "\"";
        if (i < result.detailed_errors.size() - 1) {
            json << "," << std::endl;
        }
    }
    json << "    ]" << std::endl;

    // CI metadata
    json << "  \"ci_metadata\": {" << std::endl;
    json << "    \"pipeline_id\": \"" << result.ci_pipeline_id << "\"," << std::endl;
    json << "    \"build_number\": \"" << result.ci_build_number << "\"," << std::endl;
    json << "    \"environment\": \"" << result.ci_environment << "\"," << std::endl;
    json << "    \"generated_at\": \"" <<
        std::put_time(std::localtime(&std::tm{}), "%Y-%m-%d %H:%M:%S") << "\"" << std::endl;
    json << "  }" << std::endl;

    file << json.str();
    return file.good();
}

bool DefaultCIIntegration::generateJunitReport(const ComprehensiveValidationResult& result,
                                                const std::string& output_path) {
    std::ofstream file(output_path);
    if (!file.is_open()) {
        return false;
    }

    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
    file << "<testsuite name=\"Puzzle71ComprehensiveValidation\" tests=\""
         << result.integration_test_results.size() << "\" failures=\""
         << (result.integration_test_results.size() -
          std::count_if(result.integration_test_results.begin(),
                        result.integration_test_results.end(),
                        [](const validation::IntegrationTestResult& r) { return !r.passed; })
         << "\" timestamp=\""
         << std::put_time(std::localtime(&std::tm{}), "%Y-%m-%d %H:%M:%S") << "\">" << std::endl;

    // Write test cases for each integration test
    for (const auto& test_result : result.integration_test_results) {
        file << "  <testcase classname=\"Puzzle71Validation\" name=\"" << test_result.test_name
             << "\" status=\"" << (test_result.passed ? "PASSED" : "FAILED") << "\">" << std::endl;
        file << "    <failure message=\"Validation failed\" type=\"\">" << std::endl;
        if (!test_result.error_message.empty()) {
            file << "      " << test_result.error_message << std::endl;
        }
        file << "    </failure>" << std::endl;
        file << "  </testcase>" << std::endl;
    }

    // Write baseline validation results
    for (const auto& baseline_result : result.baseline_validation_results) {
        file << "  <testcase classname=\"Puzzle71Validation\" name=\"" << baseline_result.test_name
             << "\" status=\"" << (baseline_result.passed ? "PASSED" : "FAILED") << "\">" << std::endl;
        file << "    <failure message=\"Baseline validation failed\" type=\"\">" << std::endl;
        if (!baseline_result.error_message.empty()) {
            file << "      " << baseline_result.error_message << std::endl;
        }
        file << "    </failure>" << std::endl;
        file << "  </testcase>" << std::endl;
    }

    file << "</testsuite>" << std::endl;
    return file.good();
}

bool DefaultCIIntegration::generateHTMLReport(const ComprehensiveValidationResult& result,
                                            const std::string& output_path) {
    std::ofstream file(output_path);
    if (!file.is_open()) {
        return false;
    }

    file << "<!DOCTYPE html>" << std::endl;
    file << "<html>" << std::endl;
    file << "<head>" << std::endl;
    file << "  <title>Puzzle71 Comprehensive Validation Report</title>" << std::endl;
    file << "  <style>" << std::endl;
    file << "    body { font-family: Arial, sans-serif; margin: 20px; }" << std::endl;
    file << "    .header { background-color: #2c3e50; color: white; padding: 20px; border-radius: 8px; margin-bottom: 20px; }" << std::endl;
    file << "    .status-passed { color: #28a745; font-weight: bold; }" << std::endl;
    file << "    .status-failed { color: #dc3545; font-weight: bold; }" << std::endl;
    file << "    .metric-card { background: #f8f9fa; border: 1px solid #ddd; border-radius: 8px; padding: 15px; margin: 10px 0; }" << std::endl;
    file << "    .summary-section { background: #e9ecef; border: 1px solid #ddd; border-radius: 8px; padding: 20px; margin: 20px 0; }" << std::endl;
    file << "    .framework-section { background: #f8f9fa; border: 1px solid #ddd; border-radius: 8px; padding: 20px; margin: 10px 0; }" << std::endl;
    file << "  </style>" << std::endl;
    file << "</head>" << std::endl;
    file << "<body>" << std::endl;

    // Header
    file << "<div class=\"header\">" << std::endl;
    file << "  <h1>Puzzle71 Comprehensive Validation Report</h1>" << std::endl;
    file << "  <div>Generated: " <<
        std::put_time(std::localtime(&std::tm{}), "%Y-%m-%d %H:%M:%S") << "</div>" << std::endl;
    file << "  <div class=\"status-" << (result.overall_passed ? "passed" : "failed") << "\">" << std::endl;
    file << "    <h2>Status: " << (result.overall_passed ? "PASSED" : "FAILED") << "</h2>" << std::endl;
    file << "    <p>Execution Time: " << std::fixed << std::setprecision(2)
              << result.total_execution_time_ms << " ms</p>" << std::endl;
    file << "  </div>" << std::endl;

    // Summary section
    file << "<div class=\"summary-section\">" << std::endl;
    file << "  <h2>Validation Summary</h2>" << std::endl;
    file << "  <div class=\"metric-card\">" << std::endl;
    file << "    <h4>Framework Results</h4>" << std::endl;
    file << "    <ul>" << std::endl;
    file << "      <li>ECC Validation: " << (result.ecc_validation_passed ? "✅" : "❌") << " "
              << (result.ecc_validation_passed ? "PASSED" : "FAILED") << "</li>" << std::endl;
    file << "      <li>Deterministic Replay: " << (result.deterministic_replay_passed ? "✅" : "❌") << " "
              << (result.deterministic_replay_passed ? "PASSED" : "FAILED") << "</li>" << std::endl;
    file << "      <li>Constitutional Compliance: " << (result.constitutional_compliance_passed ? "✅" : "❌") << " "
              << (result.constitutional_compliance_passed ? "PASSED" : "FAILED") << "</li>" << std::endl;
    file << "      <li>Integration Testing: " << (result.integration_testing_passed ? "✅" : "❌") << " "
              << (result.integration_testing_passed ? "PASSED" : "FAILED") << "</li>" << std::endl;
    file << "      <li>Baseline Validation: " << (result.baseline_validation_passed ? "✅" : "❌") << " "
              << (result.baseline_validation_passed ? "PASSED" : "FAILED") << "</li>" << std::endl;
    file << "    </ul>" << std::endl;
    file << "    <h4>Performance Metrics</h4>" << std::endl;
    file << "    <ul>" << std::endl;
    file << "      <li>Overall Compliance: " << std::fixed << std::setprecision(1)
              << result.overall_compliance_percentage << "%</li>" << std::endl;
    file << "      <li>Constitutional v5.5: " << (result.constitutional_v55_compliant ? "✅" : "❌") << " "
              << (result.constitutional_v55_compliant ? "COMPLIANT" : "NON-COMPLIANT") << "</li>" << std::endl;
    file << "    </ul>" << std::endl;
    file << "  </div>" << std::endl;

    // Constitutional compliance details
    if (!result.constitutional_violations.empty()) {
        file << "<div class=\"summary-section\">" << std::endl;
        file << "  <h2>Constitutional Violations</h2>" << std::endl;
        file << "  <div class=\"metric-card\">" << std::endl;
        file << "    <h4>Detected Violations:</h4>" << std::endl;
        file << "    <ul>" << std::endl;
        for (const auto& violation : result.constitutional_violations) {
            file << "      <li><strong>" << violation.constraint_name << "</strong>: " << violation.description << "</li>" << std::endl;
        }
        file << "    </ul>" << std::endl;
        file << "  </div>" << std::endl;
    }

    // Framework details
    file << "<div class=\"framework-section\">" << std::endl;
    file << "  <h2>Framework Validation Details</h2>" << std::endl;

    // ECC validation results
    if (config_.enable_ecc_validation) {
        file << "  <div class=\"metric-card\">" << std::endl;
        file << "    <h3>ECC Validation</h3>" << std::endl;
        file << "    <p><strong>Status:</strong> "
                  << (result.ecc_validation_passed ? "✅ PASSED" : "❌ FAILED") << "</p>" << std::endl;

        if (!result.integration_test_results.empty()) {
            // Find ECC-related integration tests
            for (const auto& test_result : result.integration_test_results) {
                if (test_result.test_name.find("ECC") != std::string::npos) {
                    file << "    <p><strong>" << test_result.test_name << ":</strong> "
                              << (test_result.passed ? "✅ PASSED" : "❌ FAILED") << "</p>" << std::endl;
                    file << "    <p>Execution time: " << std::fixed << std::setprecision(2)
                              << test_result.execution_time_ms << " ms</p>" << std::endl;
                }
            }
        }
        file << "  </div>" << std::endl;
    }

    // Integration testing results
    if (config_.enable_integration_testing) {
        file << "  <div class=\"metric-card\">" << std::endl;
        file << "    <h3>Integration Testing</h3>" << std::endl;
        file << "    <p><strong>Status:</strong> "
                  << (result.integration_testing_passed ? "✅ PASSED" : "❌ FAILED") << "</p>" << std::endl;
        file << "    <p>Total tests: " << result.integration_test_results.size() << "</p>" << std::endl;
        file << "    <p>Passed: " << std::count_if(result.integration_test_results.begin(),
                                          result.integration_test_results.end(),
                                          [](const validation::IntegrationTestResult& r) { return r.passed; })
              << "/" << result.integration_test_results.size() << "</p>" << std::endl;
        file << "    <p>Failed: " << std::count_if(result.integration_test_results.begin(),
                                          result.integration_test_results.end(),
                                          [](const validation::IntegrationTestResult& r) { return !r.passed; })
              << "/" << result.integration_test_results.size() << "</p>" << std::endl;
        file << "  </div>" << std::endl;
    }

    // Performance analysis
    if (!result.performance_regressions.empty() || !result.performance_improvements.empty()) {
        file << "<div class=\"summary-section\">" << std::endl;
        file << "  <h2>Performance Analysis</h2>" << std::endl;
        file << "  <div class=\"metric-card\">" << std::endl;
        file << "    <h4>Performance Regressions</h4>" << std::endl;
        file << "    <ul>" << std::endl;
        for (const auto& [metric, regression] : result.performance_regressions) {
            file << "      <li>" << metric << ": " << std::fixed << std::setprecision(2)
                      << regression_percentage << "% regression</li>" << std::endl;
        }
        file << "    </ul>" << std::endl;

        if (!result.performance_improvements.empty()) {
            file << "    <h4>Performance Improvements</h4>" << std::endl;
            file << "    <ul>" << std::endl;
            for (const auto& [metric, improvement] : result.performance_improvements) {
                file << "      <li>" << metric << ": +"
                          << std::fixed << std::setprecision(2)
                          << improvement_percentage << "% improvement</li>" << std::endl;
            }
            file << "    </ul>" << std::endl;
        }
        file << "  </div>" << std::endl;
    }

    // Error reporting
    if (!result.error_summary.empty() || !result.detailed_errors.empty()) {
        file << "<div class=\"summary-section error-section\">" << std::endl;
        file << "  <h2>Error Details</h2>" << std::endl;
        file << "  <div class=\"metric-card\">" << std::endl;
        file << "    <h4>Error Summary</h4>" << std::endl;
        file << "    <p>" << result.error_summary << "</p>" << std::endl;

        if (!result.detailed_errors.empty()) {
            file << "    <h4>Detailed Errors:</h4>" << std::endl;
            file << "    <ul>" << std::endl;
            for (const auto& error : result.detailed_errors) {
                file << "      <li>" << error << "</li>" << std::endl;
            }
            file << "    </ul>" << std::endl;
        }
        file << "  </div>" << std::endl;
    }

    file << "</body>" << std::endl;
    file << "</html>" << std::endl;

    return file.good();
}

bool DefaultCIIntegration::uploadArtifacts(const std::vector<std::string>& artifact_paths) {
    // In a real implementation, this would upload to CI systems
    std::cout << "Uploading " << artifact_paths.size() << " artifacts to CI system" << std::endl;

    for (const auto& path : artifact_paths) {
        std::cout << "  - " << path << std::endl;
    }

    return true; // Simulate success
}

bool DefaultCIIntegration::setPipelineMetadata(const std::string& pipeline_id,
                                             const std::string& build_number,
                                             const std::string& environment) {
    // This would set CI metadata in a real implementation
    return true;
}

bool DefaultCIIntegration::validateQualityGates(const ComprehensiveValidationResult& result) {
    // Constitutional v5.5 compliance is a hard requirement
    if (!result.constitutional_v55_compliant) {
        return false;
    }

    // Zero tolerance for functional regressions
    if (!result.integration_testing_passed) {
        return false;
    }

    // All frameworks should pass for quality gates
    return result.ecc_validation_passed &&
           result.deterministic_replay_passed &&
           result.constitutional_compliance_passed &&
           result.integration_testing_passed &&
           result.baseline_validation_passed;
}

bool DefaultCIIntegration::checkComplianceGates(const ComprehensiveValidationResult& result) {
    // Constitutional v5.5 compliance is a hard requirement
    return result.constitutional_v55_compliant;
}

bool DefaultCIIntegration::checkPerformanceGates(const ComprehensiveValidationResult& result) {
    // Check for critical performance regressions
    for (const auto& [metric, regression] : result.performance_regressions) {
        if (regression < -10.0) { // More than 10% regression is critical
            return false;
        }
    }

    return true;
}

bool DefaultCIIntegration::notifyValidationStart() {
    std::cout << "CI Pipeline Started" << std::endl;
    return true;
}

bool DefaultCIIntegration::notifyValidationComplete(const ComprehensiveValidationResult& result) {
    std::cout << "CI Pipeline Completed" << std::endl;
    std::cout << "  Status: " << (result.overall_passed ? "PASSED" : "FAILED") << std::endl;
    std::cout << "  Time: " << std::fixed << std::setprecision(2)
              << result.total_execution_time_ms << " ms" << std::endl;
    return true;
}

bool DefaultCIIntegration::notifyValidationFailure(const std::string& error) {
    std::cout << "CI Pipeline Failed: " << error << std::endl;
    return true;
}

// Default Telemetry Collector implementation
bool DefaultTelemetryCollector::startTelemetryCollection(const std::string& session_id) {
    current_session_id_ = session_id;
    session_start_time_ = std::chrono::system_clock::now();
    collection_active_ = true;
    framework_metrics_history_.clear();
    resource_usage_history_.clear();

    std::cout << "Started telemetry collection for session: " << session_id << std::endl;
    return true;
}

bool DefaultTelemetryCollector::recordFrameworkMetrics(const std::string& framework_name,
                                                   const std::map<std::string, double>& metrics) {
    if (!collection_active_) return false;

    framework_metrics_history_[framework_name].push_back(
        std::make_pair(framework_name, metrics));
    return true;
}

bool DefaultTelemetryCollector::recordValidationStep(const std::string& step_name,
                                                double execution_time_ms) {
    if (!collection_active_) return false;

    // Record resource usage if available
    // In a real implementation, this would collect actual system resource usage
    double cpu_usage = 0.0;
    double memory_usage = 0.0;
    double gpu_utilization = 0.0;

    resource_usage_history_.push_back(
        std::make_tuple(step_name, execution_time_ms, cpu_usage, memory_usage, gpu_utilization));

    return true;
}

bool DefaultTelemetryCollector::recordResourceUsage(double cpu_usage,
                                                double memory_usage_mb,
                                                double gpu_utilization) {
    if (!collection_active_) return false;

    resource_usage_history_.push_back(
        std::make_tuple("resource_usage", cpu_usage, memory_usage_mb, gpu_utilization));
    return true;
}

bool DefaultTelemetryCollector::stopTelemetryCollection() {
    if (!collection_active_) return false;

    collection_active_ = false;
    std::cout << "Stopped telemetry collection for session: " << current_session_id_ << std::endl;
    return true;
}

bool DefaultTelemetryCollector::analyzePerformanceTrends(std::map<std::string, double>& trends) {
    trends.clear();
    // In a real implementation, this would analyze historical telemetry data
    return true;
}

bool DefaultTelemetryCollector::generateTelemetryReport(std::string& report) {
    std::ostringstream oss;
    oss << "Telemetry Report" << std::endl;
    oss << "================" << std::endl;
    oss << "Session ID: " << current_session_id_ << std::endl;
    oss << "Collection Duration: " <<
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now() - session_start_time_).count() << " seconds" << std::endl;
    oss << std::endl;

    oss << "Framework Metrics Collected:" << std::endl;
    for (const auto& [framework, metrics_history] : framework_metrics_history_) {
        oss << "Framework: " << framework << std::endl;
        oss << "  Samples: " << metrics_history.size() << std::endl;
        if (!metrics_history.empty()) {
            auto latest = metrics_history.back();
            oss << "  Latest metrics: " << latest.first << std::endl;
        }
        oss << std::endl;
    }

    oss << "Resource Usage History:" << std::endl;
    oss << "  Total samples: " << resource_usage_history_.size() << std::endl;
    if (!resource_usage_history_.empty()) {
        auto latest = resource_usage_history_.back();
        oss << "  Latest: CPU: " << std::get<0>(latest) << "%"
            << ", Memory: " << std::get<1>(latest) << " MB"
            << ", GPU: " << std::get<2>(latest) << "%" << std::endl;
    }

    report = oss.str();
    return !report.empty();
}

// Utility functions implementation
namespace comprehensive_validation_utils {

bool optimizeValidationPipeline(const ValidationSystemConfig& config,
                                    const std::vector<std::string>& available_frameworks,
                                    ValidationOptimizationResult& result) {
    result.optimized = false;
    result.estimated_time_reduction_ms = 0.0;
    result.framework_priorities.clear();

    // Assign priorities based on configuration importance
    std::map<std::string, double> priorities = {
        {"ecc_validation", 0.8},
        {"deterministic_replay", 0.9},
        {"constitutional_compliance", 1.0},
        {"integration_testing", 0.7},
        {"baseline_validation", 0.6}
    };

    // Sort frameworks by priority (higher priority = more important)
    std::vector<std::pair<std::string, double>> sorted_frameworks;
    for (const auto& framework : available_frameworks) {
        auto priority_it = priorities.find(framework);
        if (priority_it != priorities.end()) {
            sorted_frameworks.push_back({framework, priority_it->second});
        }
    }
    std::sort(sorted_frameworks.begin(), sorted_frameworks.end(),
              [](const auto& a, const auto& b) { return b.second > a.second; });

    // Create optimized pipeline
    result.optimized = true;
    result.optimized_pipeline = sorted_frameworks;
    result.estimated_time_reduction_ms = sorted_frameworks.size() * 100.0; // Estimate 100ms per framework

    // Set framework priorities
    for (const auto& [framework, priority] : sorted_frameworks) {
        result.framework_priorities[framework] = priority;
    }

    return true;
}

bool createPerformanceBenchmark(const std::string& benchmark_name,
                              const std::map<std::string, double>& metrics,
                              PerformanceBenchmark& benchmark) {
    benchmark = PerformanceBenchmark();
    benchmark.benchmark_name = benchmark_name;
    benchmark.timestamp = std::chrono::system_clock::now();
    benchmark.baseline_metrics = metrics;
    benchmark.current_metrics = metrics;
    benchmark.performance_deltas.clear();
    benchmark.regression_detected = false;
    benchmark.improvement_detected = false;

    return true;
}

bool compareWithBenchmark(const std::string& benchmark_name,
                        const std::map<std::string, double>& current_metrics,
                        PerformanceBenchmark& comparison_result) {
    // Load existing benchmark
    // In a real implementation, this would load from storage
    comparison_result = PerformanceBenchmark();
    comparison_result.benchmark_name = benchmark_name;
    comparison_result.timestamp = std::chrono::system_clock::now();

    // Use current metrics as current metrics
    comparison_result.current_metrics = current_metrics;

    // Compare against baseline (placeholder implementation)
    for (const auto& [metric_name, baseline_value] : comparison_result.baseline_metrics) {
        auto current_it = current_metrics.find(metric_name);
        if (current_it != current_metrics.end()) {
            double current_value = current_it->second;
            comparison_result.current_metrics[metric_name] = current_value;

            // Calculate performance delta
            if (baseline_value != 0.0) {
                double delta = ((current_value - baseline_value) / baseline_value) * 100.0;
                comparison_result.performance_deltas[metric_name] = delta;

                comparison_result.regression_detected = (delta < -5.0); // 5% regression threshold
                comparison_result.improvement_detected = (delta > 5.0); // 5% improvement threshold
            }
        }
    }

    return true;
}

std::string generateValidationSessionId() {
    // Generate unique session ID using timestamp and random component
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000);

    std::ostringstream oss;
    oss << "validation_" << time_t << "_" << ms.count();

    return oss.str();
}

bool recordValidationSession(const ValidationSessionInfo& session_info) {
    // In a real implementation, this would save to persistent storage
    return true;
}

bool loadValidationHistory(std::vector<ValidationSessionInfo>& history) {
    // In a real implementation, this would load from persistent storage
    history.clear();
    return true;
}

std::string generateExecutiveSummaryTemplate() {
    return R"(
# Executive Summary - Puzzle71 Validation System

## Overview
This report provides a high-level overview of the validation system status and key metrics.

## System Status
- **Date**: {timestamp}
- **Overall Status**: {status}
- **Total Execution Time**: {total_time}ms
- **Constitutional Compliance**: {compliance}% (v5.5)

## Framework Validation Results
| Framework | Status | Time (ms) | Issues |
|----------|--------|-----------|--------|
{ecc_validation_table}|{ecc_status}|{ecc_time}|{ecc_issues}|
|deterministic_replay_table}|{deterministic_status}|{deterministic_time}|{deterministic_issues}|
|constitutional_compliance_table}|{compliance_status}|{compliance_time}|{compliance_issues}|
|integration_testing_table}|{integration_status}|{integration_time}|{integration_issues}|
|baseline_validation_table}|{baseline_status}|{baseline_time}|{baseline_issues}|

## Performance Metrics
- **Overall Compliance**: {overall_compliance}%
- **Performance Regressions**: {regressions_count}
- **Performance Improvements**: {improvements_count}

## Next Steps
1. Review detailed reports in artifact directory
2. Address any critical issues
3. Update baselines if needed
4. Schedule next validation session

---
*Generated: {timestamp}*"
    );
}

std::string generateTechnicalReportTemplate() {
    return R"(
# Technical Report - Puzzle71 Validation System

## Framework Architecture
The validation system integrates multiple specialized frameworks into a unified validation pipeline.

### Framework Components
1. **ECC Validation**: GPU cryptographic operations validation against CPU reference
2. **Deterministic Replay**: Reproducibility testing across multiple runs
3. **Constitutional Compliance**: v5.5 constitutional requirements validation
4. **Integration Testing**: End-to-end system validation
5. **Baseline Validation**: SHA-256 protected baseline comparison

### Data Flow
```
Input Data → Individual Frameworks → Aggregate Results → CI Reports → Artifacts
```

## Implementation Details
- **Threading Files**: {files_read_count}
- **Writing Files**: {files_written_count}
- **Code Coverage**: {coverage_percentage}%
- **Tests Executed**: {tests_run}
- **Validation Sessions**: {validation_sessions}

## Configuration
- **Performance Tolerance**: {performance_tolerance}%
- **Precision Tolerance**: {precision_tolerance}
- **Batch Size**: {batch_size}
- **Timeout**: {timeout}s
- **GPU Devices**: {gpu_count}

## Dependencies
- CUDA Toolkit {cuda_version}+
- libsecp256k1
- OpenSSL (for SHA-256)
- GoogleTest framework
- CMake build system

## Troubleshooting
See diagnostic messages and error reports in the main report or log files.
    );
}

std::string generateComplianceReportTemplate() {
    return R"(
# Constitutional Compliance Report - Puzzle71 v5.5

## Constitutional Requirements Validation

### Core Principles (v5.5)
1. **Bit-Level Accuracy** (<1e-10 precision)
2. **Memory Efficiency** (≥70% target, 95% goal)
3. **GPU Utilization** (≥70% target, 80% goal)
4. **Deterministic Behavior** (100% reproducibility)
5. **Integrity Protection** (Cryptographic integrity)
6. **Performance Consistency** (Stable performance)

### Validation Results Summary
- **Overall Status**: {status}
- **Compliance Percentage**: {compliance}%
- **Violations Detected**: {violations_count}
- **Critical Issues**: {critical_count}

### Detailed Framework Analysis

#### ECC Validation
- **Status**: {ecc_status}
- **Validation Count**: {ecc_count}
- **Success Rate**: {ecc_success_rate}%
- **Precision Error**: {ecc_precision}
- **Performance**: {ecc_throughputput} ops/sec

#### Deterministic Replay Validation
- **Status**: {deterministic_status}
- **Validation Count**: {deterministic_count}
- **Success Rate**: {deterministic_success_rate}%
- **Reproducibility**: {determinism_rate}%
- **Integrity Score**: {integrity_score}

#### Constitutional Compliance Validation
- **Status**: {compliance_status}
- **Validation Count**: {compliance_count}
    - **Bit-Level Accuracy**: {accuracy_compliance}
    - **Memory Efficiency**: {memory_efficiency_compliance}
    - **GPU Utilization**: {gpu_utilization_compliance}
    - **Deterministic Behavior**: {determinism_compliance}
    - **Integrity Protection**: {integrity_compliance}
    - **Performance Consistency**: {performance_consistency_compliance}

#### Integration Testing
- **Status**: {integration_status}
- **Total Tests**: {integration_count}
- **Passed Tests**: {integration_passed}
- **Failed Tests**: {integration_failed}
    - **ECC Integration**: {ecc_integration_passed}/{total_ecc_integration_tests}
    - **Deterministic Replay Integration**: {deterministic_integration_passed}/{total_deterministic_integration_tests}
    - **Constitutional Integration**: {constitutional_integration_passed}/{total_constitutional_integration_tests}
    - **Baseline Integration**: {baseline_integration_passed}/{total_baseline_integration_tests}

#### Baseline Validation
- **Status**: {baseline_status}
- **Integrity Check**: {baseline_integrity_status}
    - **Total Baselines**: {baseline_count}
    - **Valid Baselines**: {baseline_valid_count}
    - **Corrupted Baselines**: {baseline_corrupted_count}
    - **Baseline Comparisons**: {baseline_comparison_count}

### Performance Regression Analysis
- **Regressions Detected**: {regressions_count}
- **Improvements Identified**: {improvements_count}
- **Critical Regressions**: {critical_regressions_count}
- **Performance Impact**: {performance_impact}

### Recommendations
{recommendations}

---
*Report Generated: {timestamp}*"
*Constitutional Version: v5.5*"
*System Version: {system_version}*"
    );
}

// Performance utility functions implementation
namespace baseline_utils {

bool calculateBaselineStatistics(std::shared_ptr<SHA256BaselineValidator> validator,
                                 BaselineStatistics& stats) {
    stats = BaselineStatistics();
    stats.total_baselines = validator->getBaselineCount();

    if (stats.total_baselines == 0) {
        return true;
    }

    // Count corrupted baselines
    std::vector<std::string> corrupted_baselines;
    validator->verifyAllBaselinesIntegrity(corrupted_baselines);
    stats.corrupted_baselines = corrupted_baselines.size();
    stats.valid_baselines = stats.total_baselines - stats.corrupted_baselines;

    // Calculate size statistics
    std::vector<std::string> baseline_names = validator->getBaselineNames();
    if (!baseline_names.empty()) {
        stats.newest_baseline = std::chrono::system_clock::from_time_t(0);
        stats.oldest_baseline = std::chrono::system_clock::from_time_t(0);
        stats.average_baseline_size_kb = 0.0;
    } else {
        // Calculate actual size statistics
        double total_size = 0.0;
        std::chrono::system_clock::time_point newest_baseline = std::chrono::system_clock::from_time_t(0);
        std::chrono::system_clock::time_point oldest_baseline = std::chrono::system_clock::from_time_t(0);

        for (const auto& name : baseline_names) {
            BaselineEntry baseline;
            if (validator->loadBaseline(name, baseline)) {
                std::string serialized = validator->serializeBaselineEntry(baseline);
                total_size += serialized.length();
            }
        }

        if (stats.total_baselines > 0) {
            stats.average_baseline_size_kb = total_size / stats.total_baselines / 1024.0;
            stats.newest_baseline = newest_baseline;
            stats.oldest_baseline = oldest_baseline;
        }
    }

    return true;
}

} // namespace baseline_utils

} // namespace puzzle71::validation
} // namespace puzzle71
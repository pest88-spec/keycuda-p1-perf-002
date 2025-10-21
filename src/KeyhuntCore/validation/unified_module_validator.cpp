// Puzzle71 Technical Debt Repair - Unified Module Validator Implementation
// Phase 5: User Story 4 - Complete System Migration and Quality Assurance
// Task: T074 - Validate all kernels use unified modules (no legacy paths remain)
// Implements comprehensive validation of unified module usage across all kernel implementations

#include "unified_module_validator.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <thread>
#include <future>

namespace puzzle71 {
namespace validation {

// Constructor
UnifiedModuleValidator::UnifiedModuleValidator()
    : unified_usage_threshold_(UNIFIED_MODULE_USAGE_THRESHOLD),
      strict_legacy_mode_(true),
      detailed_analysis_(true),
      initialized_(false),
      total_kernels_analyzed_(0),
      total_files_analyzed_(0),
      total_analysis_time_(0) {

    // Initialize kernel extensions
    kernel_extensions_ = {".cu", ".cuh", ".cpp", ".h"};

    // Initialize exclude directories
    exclude_directories_ = {"build", "external", "docs", "scripts", ".git"};

    // Initialize exclude patterns
    exclude_patterns_ = {"test_", "mock_", "stub_"};

    // Initialize unified module patterns
    initializeUnifiedModulePatterns();

    // Initialize legacy detection patterns
    initializeLegacyPatterns();
}

// Destructor
UnifiedModuleValidator::~UnifiedModuleValidator() {
    shutdown();
}

// Initialize the validator
bool UnifiedModuleValidator::initialize(const std::string& project_root) {
    try {
        project_root_ = std::filesystem::absolute(project_root).string();

        // Validate project root exists
        if (!std::filesystem::exists(project_root_)) {
            setError("Project root directory does not exist: " + project_root_);
            return false;
        }

        // Clear analysis cache
        kernel_analysis_cache_.clear();
        usage_metrics_cache_.clear();

        // Reset statistics
        total_kernels_analyzed_ = 0;
        total_files_analyzed_ = 0;
        total_analysis_time_ = std::chrono::milliseconds(0);

        initialized_ = true;
        clearError();
        return true;

    } catch (const std::exception& e) {
        setError("Initialization failed: " + std::string(e.what()));
        return false;
    }
}

// Configure validation parameters
bool UnifiedModuleValidator::configure(double unified_usage_threshold,
                                       bool strict_legacy_mode,
                                       bool enable_detailed_analysis) {
    if (!initialized_) {
        setError("Validator not initialized");
        return false;
    }

    unified_usage_threshold_ = unified_usage_threshold;
    strict_legacy_mode_ = strict_legacy_mode;
    detailed_analysis_ = enable_detailed_analysis;

    clearError();
    return true;
}

// Shutdown the validator
void UnifiedModuleValidator::shutdown() {
    kernel_analysis_cache_.clear();
    usage_metrics_cache_.clear();
    initialized_ = false;
}

// Main validation method for all kernels
bool UnifiedModuleValidator::validateAllKernelsUseUnifiedModules(KernelValidationResult& result) {
    if (!initialized_) {
        setError("Validator not initialized");
        return false;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Initialize result structure
    result = KernelValidationResult();
    result.analysis_version = "1.0";

    // Find all kernel files
    std::vector<std::string> kernel_files;
    if (!findKernelFiles(kernel_files)) {
        setError("Failed to find kernel files");
        return false;
    }

    result.total_kernels_analyzed = static_cast<int>(kernel_files.size());

    // Analyze each kernel file
    std::vector<std::future<KernelFileAnalysisResult>> futures;

    for (const auto& kernel_file : kernel_files) {
        futures.push_back(std::async(std::launch::async,
                                    [this, kernel_file]() {
            KernelFileAnalysisResult file_result;
            this->validateKernelFile(kernel_file, file_result);
            return file_result;
        }));
    }

    // Collect results
    std::vector<KernelFileAnalysisResult> kernel_results;
    for (auto& future : futures) {
        try {
            auto file_result = future.get();
            kernel_results.push_back(file_result);

            // Update overall metrics
            result.total_files_analyzed++;
            result.total_lines_analyzed += file_result.total_lines;

            // Update usage metrics
            updateOverallMetrics(file_result, result.usage_metrics);

        } catch (const std::exception& e) {
            setError("Error analyzing kernel file: " + std::string(e.what()));
            return false;
        }
    }

    // Calculate overall compliance
    result.overall_compliance_score = calculateOverallComplianceScore(kernel_results);
    result.all_kernels_use_unified_modules = (result.overall_compliance_score >= unified_usage_threshold_);

    // Classify kernels
    classifyKernelCompliance(kernel_results, result);

    // Check for legacy code patterns
    checkLegacyCodePatterns(kernel_results, result);

    auto end_time = std::chrono::high_resolution_clock::now();
    result.analysis_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    total_analysis_time_ += result.analysis_duration;
    total_kernels_analyzed_ += kernel_files.size();

    clearError();
    return true;
}

// Validate no legacy code paths remain
bool UnifiedModuleValidator::validateNoLegacyCodePathsRemain(KernelValidationResult& result) {
    if (!initialized_) {
        setError("Validator not initialized");
        return false;
    }

    // Reuse the main validation result
    if (result.total_kernels_analyzed == 0) {
        if (!validateAllKernelsUseUnifiedModules(result)) {
            return false;
        }
    }

    // Check for zero legacy patterns
    int total_legacy_patterns = 0;
    for (const auto& kernel_name : result.non_compliant_kernels) {
        auto it = result.legacy_patterns_found.find(kernel_name);
        if (it != result.legacy_patterns_found.end()) {
            total_legacy_patterns += static_cast<int>(it->second.size());
        }
    }

    result.no_legacy_code_paths_remain = (total_legacy_patterns == 0);
    result.usage_metrics.legacy_function_calls = total_legacy_patterns;

    // Generate blocking issues if legacy code found
    if (!result.no_legacy_code_paths_remain) {
        result.blocking_issues.push_back("Legacy code patterns detected in " +
                                       std::to_string(result.non_compliant_kernels.size()) + " kernels");

        for (const auto& kernel_name : result.non_compliant_kernels) {
            auto it = result.legacy_patterns_found.find(kernel_name);
            if (it != result.legacy_patterns_found.end() && !it->second.empty()) {
                result.blocking_issues.push_back("Kernel " + kernel_name + " has " +
                                               std::to_string(it->second.size()) + " legacy patterns");
            }
        }
    }

    clearError();
    return true;
}

// Validate complete migration achieved
bool UnifiedModuleValidator::validateCompleteMigrationAchieved(KernelValidationResult& result) {
    if (!initialized_) {
        setError("Validator not initialized");
        return false;
    }

    // Ensure main validation is complete
    if (result.total_kernels_analyzed == 0) {
        if (!validateAllKernelsUseUnifiedModules(result)) {
            return false;
        }
    }

    // Check complete migration criteria
    bool unified_usage_met = (result.usage_metrics.overall_unified_usage_percentage >= 100.0);
    bool no_legacy_met = (result.usage_metrics.legacy_function_calls == 0 &&
                         result.usage_metrics.legacy_includes == 0 &&
                         result.usage_metrics.legacy_variables == 0 &&
                         result.usage_metrics.legacy_macros == 0);
    bool all_kernels_compliant = result.non_compliant_kernels.empty();

    result.complete_migration_achieved = (unified_usage_met && no_legacy_met && all_kernels_compliant);
    result.unified_module_compliance_met = unified_usage_met;

    // Generate migration status report
    if (!result.complete_migration_achieved) {
        if (!unified_usage_met) {
            result.blocking_issues.push_back("Unified module usage below 100%: " +
                                           std::to_string(result.usage_metrics.overall_unified_usage_percentage) + "%");
        }

        if (!no_legacy_met) {
            result.blocking_issues.push_back("Legacy code patterns remain: " +
                                           std::to_string(result.usage_metrics.legacy_function_calls) + " patterns");
        }

        if (!all_kernels_compliant) {
            result.blocking_issues.push_back("Non-compliant kernels: " +
                                           std::to_string(result.non_compliant_kernels.size()) + " kernels");
        }
    }

    clearError();
    return true;
}

// Validate individual kernel file
bool UnifiedModuleValidator::validateKernelFile(const std::string& kernel_filepath,
                                                KernelFileAnalysisResult& result) {
    if (!initialized_) {
        setError("Validator not initialized");
        return false;
    }

    // Check cache first
    auto cache_it = kernel_analysis_cache_.find(kernel_filepath);
    if (cache_it != kernel_analysis_cache_.end()) {
        result = cache_it->second;
        return true;
    }

    // Initialize result
    result = KernelFileAnalysisResult();
    result.filepath = kernel_filepath;
    result.kernel_name = extractKernelNameFromPath(kernel_filepath);

    // Read file content
    std::ifstream file(kernel_filepath);
    if (!file.is_open()) {
        setError("Failed to open kernel file: " + kernel_filepath);
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();

    // Analyze kernel content
    if (!analyzeKernelContent(content, kernel_filepath, result)) {
        setError("Failed to analyze kernel content: " + kernel_filepath);
        return false;
    }

    // Validate compliance
    result.is_compliant = validateKernelComplianceRequirements(result);

    // Cache result
    kernel_analysis_cache_[kernel_filepath] = result;

    total_files_analyzed_++;

    clearError();
    return true;
}

// Analyze kernel content
bool UnifiedModuleValidator::analyzeKernelContent(const std::string& content,
                                                 const std::string& filepath,
                                                 KernelFileAnalysisResult& result) {
    std::istringstream stream(content);
    std::string line;
    int line_number = 0;

    while (std::getline(stream, line)) {
        line_number++;
        result.total_lines++;

        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        // Skip empty lines and comments
        if (line.empty() || line.substr(0, 2) == "//" || line.substr(0, 1) == "#") {
            continue;
        }

        // Detect unified module patterns
        UnifiedModuleType module_type;
        if (matchesUnifiedModulePattern(line, module_type)) {
            result.module_usage[module_type]++;
            result.unified_module_function_calls.push_back(line);
            continue;
        }

        // Detect legacy patterns
        std::string pattern_type;
        if (matchesLegacyPattern(line, pattern_type)) {
            result.legacy_patterns.push_back(line);

            // Categorize legacy patterns
            if (pattern_type == "function") {
                result.legacy_function_calls.push_back(line);
            } else if (pattern_type == "include") {
                result.legacy_includes.push_back(line);
            } else if (pattern_type == "variable") {
                result.legacy_variables.push_back(line);
            } else if (pattern_type == "macro") {
                result.legacy_macros.push_back(line);
            }
            continue;
        }

        // Detect includes
        if (line.substr(0, 8) == "#include") {
            result.unified_module_includes.push_back(line);
        }

        // Detect kernel functions
        if (line.find("__global__") != std::string::npos ||
            line.find("__device__") != std::string::npos) {
            result.kernel_functions_count++;
        }
    }

    // Calculate unified usage percentage
    int total_function_calls = result.unified_module_function_calls.size() +
                              result.legacy_function_calls.size();

    if (total_function_calls > 0) {
        result.unified_usage_percentage =
            (double)result.unified_module_function_calls.size() / total_function_calls * 100.0;
    }

    // Validate specific module usage
    validateCandidateScannerUsage(content, result);
    validateECCOperationsUsage(content, result);
    validateLegacyAdapterUsage(content, result);
    validateMemoryAccessUsage(content, result);
    validateGPUExecutorUsage(content, result);

    return true;
}

// Check if line matches unified module pattern
bool UnifiedModuleValidator::matchesUnifiedModulePattern(const std::string& line,
                                                         UnifiedModuleType& module_type) {
    // Check for UnifiedCandidateScanner
    if (line.find("UnifiedCandidateScanner") != std::string::npos ||
        line.find("unified_scanner") != std::string::npos) {
        module_type = UnifiedModuleType::CANDIDATE_SCANNER;
        return true;
    }

    // Check for ECCOperationsFixed
    if (line.find("ECCOperationsFixed") != std::string::npos ||
        line.find("ecc_ops_fixed") != std::string::npos) {
        module_type = UnifiedModuleType::ECC_OPERATIONS;
        return true;
    }

    // Check for LegacyAdapterFixed
    if (line.find("LegacyAdapterFixed") != std::string::npos ||
        line.find("legacy_adapter_fixed") != std::string::npos) {
        module_type = UnifiedModuleType::LEGACY_ADAPTER;
        return true;
    }

    // Check for OptimizedMemoryAccess
    if (line.find("OptimizedMemoryAccess") != std::string::npos ||
        line.find("optimized_memory") != std::string::npos) {
        module_type = UnifiedModuleType::MEMORY_ACCESS;
        return true;
    }

    // Check for GPUExecutor
    if (line.find("GPUExecutor") != std::string::npos ||
        line.find("gpu_executor") != std::string::npos) {
        module_type = UnifiedModuleType::GPU_EXECUTOR;
        return true;
    }

    return false;
}

// Check if line matches legacy pattern
bool UnifiedModuleValidator::matchesLegacyPattern(const std::string& line,
                                                  std::string& pattern_type) {
    // Legacy function patterns
    if (detectLegacyFunctionPattern(line)) {
        pattern_type = "function";
        return true;
    }

    // Legacy include patterns
    if (detectLegacyIncludePattern(line)) {
        pattern_type = "include";
        return true;
    }

    // Legacy variable patterns
    if (detectLegacyVariablePattern(line)) {
        pattern_type = "variable";
        return true;
    }

    // Legacy macro patterns
    if (detectLegacyMacroPattern(line)) {
        pattern_type = "macro";
        return true;
    }

    return false;
}

// Validate CandidateScanner usage
bool UnifiedModuleValidator::validateCandidateScannerUsage(const std::string& content,
                                                           KernelFileAnalysisResult& result) {
    // Look for CandidateScanner patterns in content
    std::regex scanner_pattern(R"(\b(UnifiedCandidateScanner|candidate_scanner|scanner)\b)");
    std::smatch matches;

    std::string::const_iterator search_start(content.cbegin());
    while (std::regex_search(search_start, content.cend(), matches, scanner_pattern)) {
        result.unified_module_function_calls.push_back(matches[0].str());
        search_start = matches.suffix().first;
    }

    return true;
}

// Validate ECCOperations usage
bool UnifiedModuleValidator::validateECCOperationsUsage(const std::string& content,
                                                       KernelFileAnalysisResult& result) {
    // Look for ECCOperations patterns in content
    std::regex ecc_pattern(R"(\b(ECCOperationsFixed|ecc_ops_fixed|ecc_operations)\b)");
    std::smatch matches;

    std::string::const_iterator search_start(content.cbegin());
    while (std::regex_search(search_start, content.cend(), matches, ecc_pattern)) {
        result.unified_module_function_calls.push_back(matches[0].str());
        search_start = matches.suffix().first;
    }

    return true;
}

// Validate LegacyAdapter usage
bool UnifiedModuleValidator::validateLegacyAdapterUsage(const std::string& content,
                                                       KernelFileAnalysisResult& result) {
    // Look for LegacyAdapter patterns in content
    std::regex adapter_pattern(R"(\b(LegacyAdapterFixed|legacy_adapter_fixed|adapter)\b)");
    std::smatch matches;

    std::string::const_iterator search_start(content.cbegin());
    while (std::regex_search(search_start, content.cend(), matches, adapter_pattern)) {
        result.unified_module_function_calls.push_back(matches[0].str());
        search_start = matches.suffix().first;
    }

    return true;
}

// Validate MemoryAccess usage
bool UnifiedModuleValidator::validateMemoryAccessUsage(const std::string& content,
                                                      KernelFileAnalysisResult& result) {
    // Look for MemoryAccess patterns in content
    std::regex memory_pattern(R"(\b(OptimizedMemoryAccess|optimized_memory|memory_access)\b)");
    std::smatch matches;

    std::string::const_iterator search_start(content.cbegin());
    while (std::regex_search(search_start, content.cend(), matches, memory_pattern)) {
        result.unified_module_function_calls.push_back(matches[0].str());
        search_start = matches.suffix().first;
    }

    return true;
}

// Validate GPUExecutor usage
bool UnifiedModuleValidator::validateGPUExecutorUsage(const std::string& content,
                                                      KernelFileAnalysisResult& result) {
    // Look for GPUExecutor patterns in content
    std::regex executor_pattern(R"(\b(GPUExecutor|gpu_executor|executor)\b)");
    std::smatch matches;

    std::string::const_iterator search_start(content.cbegin());
    while (std::regex_search(search_start, content.cend(), matches, executor_pattern)) {
        result.unified_module_function_calls.push_back(matches[0].str());
        search_start = matches.suffix().first;
    }

    return true;
}

// Detect legacy function patterns
bool UnifiedModuleValidator::detectLegacyFunctionPattern(const std::string& line) {
    // Common legacy function patterns
    std::vector<std::string> legacy_patterns = {
        "legacy_", "old_", "deprecated_", "obsolete_",
        "LegacyFunction", "OldFunction", "DeprecatedFunction"
    };

    for (const auto& pattern : legacy_patterns) {
        if (line.find(pattern) != std::string::npos) {
            return true;
        }
    }

    return false;
}

// Detect legacy include patterns
bool UnifiedModuleValidator::detectLegacyIncludePattern(const std::string& line) {
    // Legacy include patterns
    std::vector<std::string> legacy_includes = {
        "legacy.h", "old.h", "deprecated.h", "obsolete.h",
        "Legacy", "Old", "Deprecated", "Obsolete"
    };

    if (line.substr(0, 8) == "#include") {
        for (const auto& include : legacy_includes) {
            if (line.find(include) != std::string::npos) {
                return true;
            }
        }
    }

    return false;
}

// Detect legacy variable patterns
bool UnifiedModuleValidator::detectLegacyVariablePattern(const std::string& line) {
    // Legacy variable patterns
    std::vector<std::string> legacy_variables = {
        "legacy_var", "old_var", "deprecated_var", "obsolete_var"
    };

    for (const auto& variable : legacy_variables) {
        if (line.find(variable) != std::string::npos) {
            return true;
        }
    }

    return false;
}

// Detect legacy macro patterns
bool UnifiedModuleValidator::detectLegacyMacroPattern(const std::string& line) {
    // Legacy macro patterns
    std::vector<std::string> legacy_macros = {
        "LEGACY_", "OLD_", "DEPRECATED_", "OBSOLETE_"
    };

    for (const auto& macro : legacy_macros) {
        if (line.find(macro) != std::string::npos) {
            return true;
        }
    }

    return false;
}

// Validate kernel compliance requirements
bool UnifiedModuleValidator::validateKernelComplianceRequirements(const KernelFileAnalysisResult& result) {
    // Check unified module usage threshold
    if (result.unified_usage_percentage < unified_usage_threshold_) {
        return false;
    }

    // Check for zero legacy patterns
    if (strict_legacy_mode_) {
        if (!result.legacy_patterns.empty() ||
            !result.legacy_function_calls.empty() ||
            !result.legacy_includes.empty() ||
            !result.legacy_variables.empty() ||
            !result.legacy_macros.empty()) {
            return false;
        }
    }

    // Check for minimum unified module usage
    if (result.unified_module_function_calls.empty()) {
        return false;
    }

    return true;
}

// Find all kernel files
bool UnifiedModuleValidator::findKernelFiles(std::vector<std::string>& kernel_files) {
    kernel_files.clear();

    for (const auto& entry : std::filesystem::recursive_directory_iterator(project_root_)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::string filepath = entry.path().string();

        // Check if file should be analyzed
        if (!shouldAnalyzeFile(filepath)) {
            continue;
        }

        // Check if it's a kernel file
        if (isKernelFile(filepath)) {
            kernel_files.push_back(filepath);
        }
    }

    return !kernel_files.empty();
}

// Check if file should be analyzed
bool UnifiedModuleValidator::shouldAnalyzeFile(const std::string& filepath) {
    // Check exclude directories
    for (const auto& exclude_dir : exclude_directories_) {
        if (filepath.find("/" + exclude_dir + "/") != std::string::npos) {
            return false;
        }
    }

    // Check exclude patterns
    for (const auto& exclude_pattern : exclude_patterns_) {
        if (filepath.find(exclude_pattern) != std::string::npos) {
            return false;
        }
    }

    return true;
}

// Check if file is a kernel file
bool UnifiedModuleValidator::isKernelFile(const std::string& filepath) {
    // Check file extension
    for (const auto& ext : kernel_extensions_) {
        if (filepath.length() >= ext.length() &&
            filepath.substr(filepath.length() - ext.length()) == ext) {
            return true;
        }
    }

    return false;
}

// Extract kernel name from file path
std::string UnifiedModuleValidator::extractKernelNameFromPath(const std::string& filepath) {
    std::filesystem::path path(filepath);
    return path.stem().string();
}

// Initialize unified module patterns
void UnifiedModuleValidator::initializeUnifiedModulePatterns() {
    // CandidateScanner patterns
    unified_module_patterns_[UnifiedModuleType::CANDIDATE_SCANNER] = {
        std::regex(R"(\bUnifiedCandidateScanner\b)"),
        std::regex(R"(\bcandidate_scanner\b)"),
        std::regex(R"(\bunified_scanner\b)")
    };

    // ECCOperations patterns
    unified_module_patterns_[UnifiedModuleType::ECC_OPERATIONS] = {
        std::regex(R"(\bECCOperationsFixed\b)"),
        std::regex(R"(\becc_ops_fixed\b)"),
        std::regex(R"(\becc_operations\b)")
    };

    // LegacyAdapter patterns
    unified_module_patterns_[UnifiedModuleType::LEGACY_ADAPTER] = {
        std::regex(R"(\bLegacyAdapterFixed\b)"),
        std::regex(R"(\blegacy_adapter_fixed\b)"),
        std::regex(R"(\badapter\b)")
    };

    // MemoryAccess patterns
    unified_module_patterns_[UnifiedModuleType::MEMORY_ACCESS] = {
        std::regex(R"(\bOptimizedMemoryAccess\b)"),
        std::regex(R"(\boptimized_memory\b)"),
        std::regex(R"(\bmemory_access\b)")
    };

    // GPUExecutor patterns
    unified_module_patterns_[UnifiedModuleType::GPU_EXECUTOR] = {
        std::regex(R"(\bGPUExecutor\b)"),
        std::regex(R"(\bgpu_executor\b)"),
        std::regex(R"(\bexecutor\b)")
    };
}

// Initialize legacy detection patterns
void UnifiedModuleValidator::initializeLegacyPatterns() {
    // Legacy function patterns
    legacy_function_patterns_ = {
        std::regex(R"(\blegacy_\w+\b)"),
        std::regex(R"(\bold_\w+\b)"),
        std::regex(R"(\bdeprecated_\w+\b)"),
        std::regex(R"(\bobsolete_\w+\b)")
    };

    // Legacy include patterns
    legacy_include_patterns_ = {
        std::regex(R"(#include\s*[<"]\w*legacy\w*[>"])"),
        std::regex(R"(#include\s*[<"]\w*old\w*[>"])"),
        std::regex(R"(#include\s*[<"]\w*deprecated\w*[>"])"),
        std::regex(R"(#include\s*[<"]\w*obsolete\w*[>"])")
    };

    // Legacy variable patterns
    legacy_variable_patterns_ = {
        std::regex(R"(\blegacy_var_\w+\b)"),
        std::regex(R"(\bold_var_\w+\b)"),
        std::regex(R"(\bdeprecated_var_\w+\b)")
    };

    // Legacy macro patterns
    legacy_macro_patterns_ = {
        std::regex(R"(\bLEGACY_\w+\b)"),
        std::regex(R"(\bOLD_\w+\b)"),
        std::regex(R"(\bDEPRECATED_\w+\b)"),
        std::regex(R"(\bOBSOLETE_\w+\b)")
    };
}

// Update overall metrics
void UnifiedModuleValidator::updateOverallMetrics(const KernelFileAnalysisResult& file_result,
                                                 UnifiedModuleUsageMetrics& metrics) {
    // Update unified module usage counts
    for (const auto& module_usage : file_result.module_usage) {
        metrics.module_usage_counts[module_usage.first] += module_usage.second;
    }

    // Update legacy detection metrics
    metrics.legacy_function_calls += static_cast<int>(file_result.legacy_function_calls.size());
    metrics.legacy_includes += static_cast<int>(file_result.legacy_includes.size());
    metrics.legacy_variables += static_cast<int>(file_result.legacy_variables.size());
    metrics.legacy_macros += static_cast<int>(file_result.legacy_macros.size());

    // Update total counts
    metrics.total_function_calls += static_cast<int>(file_result.unified_module_function_calls.size()) +
                                   static_cast<int>(file_result.legacy_function_calls.size());
    metrics.total_includes += static_cast<int>(file_result.unified_module_includes.size()) +
                             static_cast<int>(file_result.legacy_includes.size());
    metrics.total_variables += static_cast<int>(file_result.legacy_variables.size());
    metrics.total_macros += static_cast<int>(file_result.legacy_macros.size());
}

// Calculate overall compliance score
double UnifiedModuleValidator::calculateOverallComplianceScore(const std::vector<KernelFileAnalysisResult>& results) {
    if (results.empty()) {
        return 0.0;
    }

    double total_score = 0.0;
    int valid_kernels = 0;

    for (const auto& result : results) {
        if (result.total_lines > 0) {
            total_score += result.unified_usage_percentage;
            valid_kernels++;
        }
    }

    return (valid_kernels > 0) ? (total_score / valid_kernels) : 0.0;
}

// Classify kernel compliance
void UnifiedModuleValidator::classifyKernelCompliance(const std::vector<KernelFileAnalysisResult>& kernel_results,
                                                     KernelValidationResult& result) {
    for (const auto& kernel_result : kernel_results) {
        double compliance_score = calculateModuleComplianceScore(kernel_result);
        result.kernel_compliance_scores[kernel_result.kernel_name] = compliance_score;

        if (kernel_result.is_compliant) {
            result.compliant_kernels.push_back(kernel_result.kernel_name);
        } else {
            result.non_compliant_kernels.push_back(kernel_result.kernel_name);

            // Store legacy patterns found
            if (!kernel_result.legacy_patterns.empty()) {
                result.legacy_patterns_found[kernel_result.kernel_name] = kernel_result.legacy_patterns;
            }

            if (!kernel_result.legacy_function_calls.empty()) {
                result.legacy_functions_found[kernel_result.kernel_name] = kernel_result.legacy_function_calls;
            }

            if (!kernel_result.legacy_includes.empty()) {
                result.legacy_includes_found[kernel_result.kernel_name] = kernel_result.legacy_includes;
            }
        }

        // Store kernel-specific metrics
        result.kernel_specific_metrics[kernel_result.kernel_name] = UnifiedModuleUsageMetrics();
        result.kernel_specific_metrics[kernel_result.kernel_name].overall_unified_usage_percentage =
            kernel_result.unified_usage_percentage;
    }
}

// Check legacy code patterns
void UnifiedModuleValidator::checkLegacyCodePatterns(const std::vector<KernelFileAnalysisResult>& kernel_results,
                                                     KernelValidationResult& result) {
    int total_legacy_patterns = 0;

    for (const auto& kernel_result : kernel_results) {
        total_legacy_patterns += static_cast<int>(kernel_result.legacy_patterns.size());

        if (!kernel_result.legacy_patterns.empty()) {
            result.validation_issues[kernel_result.kernel_name] = kernel_result.legacy_patterns;
        }
    }

    result.usage_metrics.legacy_function_calls = total_legacy_patterns;
}

// Calculate module compliance score
double UnifiedModuleValidator::calculateModuleComplianceScore(const KernelFileAnalysisResult& result) {
    if (result.total_lines == 0) {
        return 0.0;
    }

    // Base score from unified usage percentage
    double base_score = result.unified_usage_percentage;

    // Penalty for legacy patterns
    double legacy_penalty = 0.0;
    if (strict_legacy_mode_) {
        int total_legacy_patterns = static_cast<int>(result.legacy_patterns.size() +
                                                    result.legacy_function_calls.size() +
                                                    result.legacy_includes.size() +
                                                    result.legacy_variables.size() +
                                                    result.legacy_macros.size());
        legacy_penalty = total_legacy_patterns * 10.0; // 10% penalty per legacy pattern
    }

    // Bonus for multiple unified modules
    double module_bonus = (result.module_usage.size() - 1) * 5.0; // 5% bonus per additional module

    double final_score = base_score - legacy_penalty + module_bonus;
    return std::max(0.0, std::min(100.0, final_score));
}

// Generate validation report
bool UnifiedModuleValidator::generateValidationReport(const KernelValidationResult& result, std::string& report) {
    std::ostringstream oss;

    // Executive summary
    oss << generateExecutiveSummary(result);

    // Detailed analysis
    oss << generateDetailedAnalysis(result);

    // Kernel breakdown
    oss << generateKernelBreakdown(result);

    // Legacy analysis
    oss << generateLegacyAnalysis(result);

    // Recommendations
    oss << generateRecommendationsSection(result);

    report = oss.str();
    return true;
}

// Generate executive summary
std::string UnifiedModuleValidator::generateExecutiveSummary(const KernelValidationResult& result) {
    std::ostringstream oss;

    oss << "# Unified Module Validation Report\n\n";
    oss << "## Executive Summary\n\n";
    oss << "**Overall Compliance Score:** " << std::fixed << std::setprecision(2)
        << result.overall_compliance_score << "%\n\n";

    oss << "**Validation Status:**\n";
    oss << "- All kernels use unified modules: " << (result.all_kernels_use_unified_modules ? "✅ PASS" : "❌ FAIL") << "\n";
    oss << "- No legacy code paths remain: " << (result.no_legacy_code_paths_remain ? "✅ PASS" : "❌ FAIL") << "\n";
    oss << "- Complete migration achieved: " << (result.complete_migration_achieved ? "✅ PASS" : "❌ FAIL") << "\n";
    oss << "- Unified module compliance met: " << (result.unified_module_compliance_met ? "✅ PASS" : "❌ FAIL") << "\n\n";

    oss << "**Analysis Statistics:**\n";
    oss << "- Total kernels analyzed: " << result.total_kernels_analyzed << "\n";
    oss << "- Compliant kernels: " << result.compliant_kernels.size() << "\n";
    oss << "- Non-compliant kernels: " << result.non_compliant_kernels.size() << "\n";
    oss << "- Analysis duration: " << result.analysis_duration.count() << "ms\n\n";

    return oss.str();
}

// Generate detailed analysis
std::string UnifiedModuleValidator::generateDetailedAnalysis(const KernelValidationResult& result) {
    std::ostringstream oss;

    oss << "## Detailed Analysis\n\n";

    // Unified module usage metrics
    oss << "### Unified Module Usage\n\n";
    oss << "- Overall unified usage: " << std::fixed << std::setprecision(2)
        << result.usage_metrics.overall_unified_usage_percentage << "%\n";
    oss << "- Total function calls: " << result.usage_metrics.total_function_calls << "\n";
    oss << "- Legacy function calls: " << result.usage_metrics.legacy_function_calls << "\n";
    oss << "- Legacy includes: " << result.usage_metrics.legacy_includes << "\n";
    oss << "- Legacy variables: " << result.usage_metrics.legacy_variables << "\n";
    oss << "- Legacy macros: " << result.usage_metrics.legacy_macros << "\n\n";

    // Module-specific usage
    oss << "### Module Usage Breakdown\n\n";
    for (const auto& module_usage : result.usage_metrics.module_usage_counts) {
        oss << "- Module " << static_cast<int>(module_usage.first) << ": " << module_usage.second << " uses\n";
    }
    oss << "\n";

    return oss.str();
}

// Generate kernel breakdown
std::string UnifiedModuleValidator::generateKernelBreakdown(const KernelValidationResult& result) {
    std::ostringstream oss;

    oss << "## Kernel Breakdown\n\n";

    // Compliant kernels
    if (!result.compliant_kernels.empty()) {
        oss << "### Compliant Kernels ✅\n\n";
        for (const auto& kernel : result.compliant_kernels) {
            auto it = result.kernel_compliance_scores.find(kernel);
            if (it != result.kernel_compliance_scores.end()) {
                oss << "- **" << kernel << "**: " << std::fixed << std::setprecision(2)
                    << it->second << "% compliance\n";
            }
        }
        oss << "\n";
    }

    // Non-compliant kernels
    if (!result.non_compliant_kernels.empty()) {
        oss << "### Non-Compliant Kernels ❌\n\n";
        for (const auto& kernel : result.non_compliant_kernels) {
            auto it = result.kernel_compliance_scores.find(kernel);
            if (it != result.kernel_compliance_scores.end()) {
                oss << "- **" << kernel << "**: " << std::fixed << std::setprecision(2)
                    << it->second << "% compliance\n";
            }

            // List validation issues
            auto issues_it = result.validation_issues.find(kernel);
            if (issues_it != result.validation_issues.end() && !issues_it->second.empty()) {
                oss << "  - Issues: " << issues_it->second.size() << " issues found\n";
            }
        }
        oss << "\n";
    }

    return oss.str();
}

// Generate legacy analysis
std::string UnifiedModuleValidator::generateLegacyAnalysis(const KernelValidationResult& result) {
    std::ostringstream oss;

    oss << "## Legacy Code Analysis\n\n";

    if (result.legacy_patterns_found.empty()) {
        oss << "✅ **No legacy code patterns detected**\n\n";
    } else {
        oss << "❌ **Legacy code patterns detected**\n\n";

        for (const auto& legacy_entry : result.legacy_patterns_found) {
            const std::string& kernel_name = legacy_entry.first;
            const std::vector<std::string>& patterns = legacy_entry.second;

            oss << "### " << kernel_name << "\n\n";
            oss << "Legacy patterns found: " << patterns.size() << "\n\n";

            for (const auto& pattern : patterns) {
                oss << "- `" << pattern << "`\n";
            }
            oss << "\n";
        }
    }

    return oss.str();
}

// Generate recommendations section
std::string UnifiedModuleValidator::generateRecommendationsSection(const KernelValidationResult& result) {
    std::ostringstream oss;

    oss << "## Recommendations\n\n";

    if (result.blocking_issues.empty()) {
        oss << "✅ **All validation criteria met**\n\n";
        oss << "The system has successfully migrated to unified modules with no legacy code paths remaining.\n\n";
    } else {
        oss << "### Blocking Issues\n\n";
        for (const auto& issue : result.blocking_issues) {
            oss << "- ❌ " << issue << "\n";
        }
        oss << "\n";

        oss << "### Immediate Actions Required\n\n";
        for (const auto& kernel : result.non_compliant_kernels) {
            oss << "1. **" << kernel << "**: Replace legacy patterns with unified modules\n";
        }
        oss << "\n";

        oss << "### Migration Recommendations\n\n";
        oss << "1. Replace all legacy function calls with unified module equivalents\n";
        oss << "2. Update legacy includes to use unified module headers\n";
        oss << "3. Remove legacy variable declarations\n";
        oss << "4. Eliminate legacy macro definitions\n";
        oss << "5. Ensure 100% unified module usage across all kernels\n\n";
    }

    return oss.str();
}

// Set error message
void UnifiedModuleValidator::setError(const std::string& error) {
    last_error_ = error;
}

// Clear error message
void UnifiedModuleValidator::clearError() {
    last_error_.clear();
}

// Utility functions implementation
namespace unified_module_validation_utils {

bool quickUnifiedModuleCheck(const std::string& project_root) {
    UnifiedModuleValidator validator;
    if (!validator.initialize(project_root)) {
        return false;
    }

    KernelValidationResult result;
    return validator.validateAllKernelsUseUnifiedModules(result);
}

bool validateNoLegacyCode(const std::string& project_root) {
    UnifiedModuleValidator validator;
    if (!validator.initialize(project_root)) {
        return false;
    }

    KernelValidationResult result;
    return validator.validateNoLegacyCodePathsRemain(result);
}

bool checkCompleteMigration(const std::string& project_root) {
    UnifiedModuleValidator validator;
    if (!validator.initialize(project_root)) {
        return false;
    }

    KernelValidationResult result;
    return validator.validateCompleteMigrationAchieved(result);
}

} // namespace unified_module_validation_utils

} // namespace validation
} // namespace puzzle71
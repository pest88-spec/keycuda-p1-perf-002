// Puzzle71Solver - Module Manager Implementation
// Centralized coordination system for unified modules (T023)

#include "module_manager.cuh"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <mutex>

using json = nlohmann::json;

namespace keyhunt {
namespace common {

// Static singleton instance
ModuleManager& ModuleManager::getInstance() {
    static ModuleManager instance;
    return instance;
}

bool ModuleManager::initializeModules(const ModuleConfiguration& config) {
    if (initialized_) {
        logInfo("Module manager already initialized");
        return true;
    }

    logInfo("Initializing unified modules...");

    configuration_ = config;

    // Initialize all modules
    bool success = true;

    success &= initializeResultEmitterModule();
    success &= initializeHashUtilsModule();
    success &= initializeECCOperationsModule();

    if (config.enable_legacy_adapter) {
        success &= initializeLegacyAdapterModule();
    }

    if (success) {
        initialized_ = true;
        logInfo("All modules initialized successfully");

        // Validate module health
        if (validateModuleHealth()) {
            logInfo("Module health validation passed");
        } else {
            logWarning("Module health validation failed - some modules may not be optimal");
        }
    } else {
        logError("Module initialization failed");
    }

    return success;
}

void ModuleManager::shutdownModules() {
    if (!initialized_) {
        return;
    }

    logInfo("Shutting down unified modules...");

    // Mark all modules as uninitialized
    for (auto& [name, metrics] : module_metrics_) {
        metrics.status = ModuleStatus::UNINITIALIZED;
    }

    initialized_ = false;
    logInfo("Module shutdown completed");
}

bool ModuleManager::areModulesReady() const {
    return initialized_;
}

const ModuleConfiguration& ModuleManager::getConfiguration() const {
    return configuration_;
}

bool ModuleManager::updateConfiguration(const ModuleConfiguration& config) {
    configuration_ = config;
    logInfo("Module configuration updated");

    // Reinitialize modules if needed
    if (initialized_) {
        return validateModuleHealth();
    }

    return true;
}

ModuleMetrics ModuleManager::getModuleMetrics(const std::string& module_name) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto it = module_metrics_.find(module_name);
    if (it != module_metrics_.end()) {
        return it->second;
    }

    // Return empty metrics if module not found
    ModuleMetrics empty_metrics;
    empty_metrics.module_name = module_name;
    empty_metrics.status = ModuleStatus::UNINITIALIZED;
    return empty_metrics;
}

std::unordered_map<std::string, ModuleMetrics> ModuleManager::getAllModuleMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return module_metrics_;
}

void ModuleManager::resetMetrics() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    for (auto& [name, metrics] : module_metrics_) {
        metrics.execution_time_ms = 0.0;
        metrics.memory_efficiency = 0.0;
        metrics.occupancy = 0.0;
        metrics.bandwidth_utilization = 0.0;
        metrics.memory_used_bytes = 0;
        metrics.call_count = 0;
        metrics.error_count = 0;
        metrics.recent_errors.clear();
        metrics.last_call_time = {};
    }

    logInfo("Module metrics reset");
}

std::string ModuleManager::getPerformanceSummary() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    json summary = {
        {"timestamp", getTimestamp()},
        {"manager_version", "1.0.0"},
        {"configuration", {
            {"debug_logging", configuration_.enable_debug_logging},
            {"performance_tracking", configuration_.enable_performance_tracking},
            {"memory_optimization", configuration_.enable_memory_optimization},
            {"shared_memory_size", configuration_.shared_memory_size},
            {"preferred_block_size", configuration_.preferred_block_size},
            {"max_registers_per_thread", configuration_.max_registers_per_thread}
        }},
        {"modules", json::array()},
        {"system_metrics", {
            {"total_modules", module_metrics_.size()},
            {"initialized_modules", std::count_if(module_metrics_.begin(), module_metrics_.end(),
                [](const auto& pair) { return pair.second.status == ModuleStatus::INITIALIZED ||
                                       pair.second.status == ModuleStatus::ACTIVE; })},
            {"total_calls", 0},
            {"total_errors", 0},
            {"system_efficiency", calculateSystemEfficiency()},
            {"performance_targets_met", meetsPerformanceTargets()}
        }}
    };

    size_t total_calls = 0;
    size_t total_errors = 0;

    for (const auto& [name, metrics] : module_metrics_) {
        total_calls += metrics.call_count;
        total_errors += metrics.error_count;

        json module_info = {
            {"name", name},
            {"status", static_cast<int>(metrics.status)},
            {"call_count", metrics.call_count},
            {"error_count", metrics.error_count},
            {"execution_time_ms", metrics.execution_time_ms},
            {"memory_efficiency", metrics.memory_efficiency},
            {"occupancy", metrics.occupancy},
            {"bandwidth_utilization", metrics.bandwidth_utilization},
            {"memory_used_bytes", metrics.memory_used_bytes},
            {"last_call_time", std::chrono::duration_cast<std::chrono::seconds>(
                metrics.last_call_time.time_since_epoch()).count()}
        };

        if (!metrics.recent_errors.empty()) {
            module_info["recent_errors"] = metrics.recent_errors;
        }

        summary["modules"].push_back(module_info);
    }

    summary["system_metrics"]["total_calls"] = total_calls;
    summary["system_metrics"]["total_errors"] = total_errors;

    // Add performance bottlenecks if any
    auto bottlenecks = getPerformanceBottlenecks();
    if (!bottlenecks.empty()) {
        summary["performance_bottlenecks"] = bottlenecks;
    }

    return summary.dump(2);
}

void ModuleManager::setDebugLogging(bool enable) {
    configuration_.enable_debug_logging = enable;
    logInfo(std::string("Debug logging ") + (enable ? "enabled" : "disabled"));
}

bool ModuleManager::isModuleEnabled(const std::string& module_name) const {
    auto metrics = getModuleMetrics(module_name);
    return metrics.status == ModuleStatus::INITIALIZED || metrics.status == ModuleStatus::ACTIVE;
}

bool ModuleManager::setModuleEnabled(const std::string& module_name, bool enable) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto it = module_metrics_.find(module_name);
    if (it == module_metrics_.end()) {
        logError("Module not found: " + module_name);
        return false;
    }

    if (enable) {
        if (it->second.status == ModuleStatus::UNINITIALIZED) {
            // Reinitialize the module
            bool success = false;
            if (module_name == "ResultEmitter") {
                success = initializeResultEmitterModule();
            } else if (module_name == "HashUtils") {
                success = initializeHashUtilsModule();
            } else if (module_name == "ECCOperations") {
                success = initializeECCOperationsModule();
            } else if (module_name == "LegacyAdapter") {
                success = initializeLegacyAdapterModule();
            }

            if (success) {
                logInfo("Module enabled: " + module_name);
                return true;
            } else {
                logError("Failed to enable module: " + module_name);
                return false;
            }
        } else {
            it->second.status = ModuleStatus::ACTIVE;
            logInfo("Module activated: " + module_name);
            return true;
        }
    } else {
        it->second.status = ModuleStatus::DISABLED;
        logInfo("Module disabled: " + module_name);
        return true;
    }
}

ModuleStatus ModuleManager::getModuleStatus(const std::string& module_name) const {
    auto metrics = getModuleMetrics(module_name);
    return metrics.status;
}

bool ModuleManager::validateModuleHealth() {
    bool all_healthy = true;

    for (const auto& [name, metrics] : module_metrics_) {
        bool module_healthy = true;

        // Check for errors
        if (metrics.error_count > 0) {
            module_healthy = false;
            logWarning("Module " + name + " has " + std::to_string(metrics.error_count) + " errors");
        }

        // Check performance targets
        if (configuration_.enable_performance_tracking) {
            if (metrics.memory_efficiency < configuration_.target_memory_efficiency * 0.8) {
                module_healthy = false;
                logWarning("Module " + name + " memory efficiency below threshold: " +
                          std::to_string(metrics.memory_efficiency));
            }

            if (metrics.occupancy < configuration_.target_occupancy * 0.8) {
                module_healthy = false;
                logWarning("Module " + name + " occupancy below threshold: " +
                          std::to_string(metrics.occupancy));
            }
        }

        // Check status
        if (metrics.status == ModuleStatus::ERROR || metrics.status == ModuleStatus::UNINITIALIZED) {
            module_healthy = false;
            logWarning("Module " + name + " status is problematic: " + std::to_string(static_cast<int>(metrics.status)));
        }

        if (!module_healthy) {
            all_healthy = false;
        }
    }

    return all_healthy;
}

std::string ModuleManager::getOptimizationRecommendations() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    json recommendations = {
        {"timestamp", getTimestamp()},
        {"current_configuration", {
            {"block_size", configuration_.preferred_block_size},
            {"max_registers", configuration_.max_registers_per_thread},
            {"shared_memory", configuration_.shared_memory_size}
        }},
        {"recommendations", json::array()}
    };

    // Analyze current performance and provide recommendations
    for (const auto& [name, metrics] : module_metrics_) {
        if (metrics.call_count == 0) continue; // Skip unused modules

        json module_recommendation = {
            {"module", name},
            "suggestions", json::array()
        };

        // Memory efficiency recommendations
        if (metrics.memory_efficiency < configuration_.target_memory_efficiency) {
            module_recommendation["suggestions"].push_back({
                {"type", "memory_optimization"},
                {"priority", "high"},
                {"description", "Memory efficiency below target"},
                {"current", metrics.memory_efficiency},
                {"target", configuration_.target_memory_efficiency},
                {"action", "Enable shared memory optimization or reduce memory usage"}
            });
        }

        // Occupancy recommendations
        if (metrics.occupancy < configuration_.target_occupancy) {
            module_recommendation["suggestions"].push_back({
                {"type", "occupancy_optimization"},
                {"priority", "medium"},
                {"description", "GPU occupancy below target"},
                {"current", metrics.occupancy},
                {"target", configuration_.target_occupancy},
                {"action", "Reduce register usage or increase block size"}
            });
        }

        // Block size recommendations
        if (metrics.call_count > 1000 && metrics.execution_time_ms > 10.0) {
            module_recommendation["suggestions"].push_back({
                {"type", "block_size_optimization"},
                {"priority", "medium"},
                {"description", "High call count with significant execution time"},
                {"current_calls", metrics.call_count},
                {"avg_execution_time_ms", metrics.execution_time_ms},
                {"action", "Consider optimizing block size for better throughput"}
            });
        }

        if (!module_recommendation["suggestions"].empty()) {
            recommendations["recommendations"].push_back(module_recommendation);
        }
    }

    // System-wide recommendations
    auto system_efficiency = calculateSystemEfficiency();
    if (system_efficiency < 0.8) {
        recommendations["recommendations"].push_back({
            {"module", "system"},
            "suggestions", {{
                {"type", "system_optimization"},
                {"priority", "high"},
                {"description", "System-wide efficiency below 80%"},
                {"current_efficiency", system_efficiency},
                {"action", "Review all modules for optimization opportunities"}
            }}
        });
    }

    return recommendations.dump(2);
}

bool ModuleManager::exportMetrics(const std::string& filename) const {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            logError("Cannot create export file: " + filename);
            return false;
        }

        json export_data = {
            {"export_timestamp", getTimestamp()},
            {"export_version", "1.0.0"},
            {"performance_summary", json::parse(getPerformanceSummary())},
            {"optimization_recommendations", json::parse(getOptimizationRecommendations())},
            {"configuration", {
                {"debug_logging", configuration_.enable_debug_logging},
                {"performance_tracking", configuration_.enable_performance_tracking},
                {"memory_optimization", configuration_.enable_memory_optimization},
                {"shared_memory_size", configuration_.shared_memory_size},
                {"preferred_block_size", configuration_.preferred_block_size},
                {"max_registers_per_thread", configuration_.max_registers_per_thread}
            }}
        };

        file << std::setw(4) << export_data << std::endl;
        file.close();

        logInfo("Metrics exported to: " + filename);
        return true;
    } catch (const std::exception& e) {
        logError("Failed to export metrics: " + std::string(e.what()));
        return false;
    }
}

bool ModuleManager::importConfiguration(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            logError("Cannot open configuration file: " + filename);
            return false;
        }

        json config_data;
        file >> config_data;
        file.close();

        if (config_data.contains("configuration")) {
            ModuleConfiguration new_config;

            auto config = config_data["configuration"];
            new_config.enable_debug_logging = config.value("debug_logging", false);
            new_config.enable_performance_tracking = config.value("performance_tracking", true);
            new_config.enable_memory_optimization = config.value("memory_optimization", true);
            new_config.shared_memory_size = config.value("shared_memory_size", 65536);
            new_config.preferred_block_size = config.value("preferred_block_size", 256);
            new_config.max_registers_per_thread = config.value("max_registers_per_thread", 40);

            return updateConfiguration(new_config);
        }

        logError("Invalid configuration file format");
        return false;
    } catch (const std::exception& e) {
        logError("Failed to import configuration: " + std::string(e.what()));
        return false;
    }
}

// Private implementation methods

bool ModuleManager::initializeResultEmitterModule() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    ModuleMetrics& metrics = module_metrics_["ResultEmitter"];
    metrics.module_name = "ResultEmitter";
    metrics.status = ModuleStatus::INITIALIZING;
    metrics.initialization_time = std::chrono::system_clock::now();

    // Initialize result emitter module
    // This would include setting up device buffers, validating constants, etc.

    metrics.status = ModuleStatus::INITIALIZED;
    logInfo("ResultEmitter module initialized");
    return true;
}

bool ModuleManager::initializeHashUtilsModule() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    ModuleMetrics& metrics = module_metrics_["HashUtils"];
    metrics.module_name = "HashUtils";
    metrics.status = ModuleStatus::INITIALIZING;
    metrics.initialization_time = std::chrono::system_clock::now();

    // Initialize hash utils module
    // This would include validating hash algorithms, constants, etc.

    metrics.status = ModuleStatus::INITIALIZED;
    logInfo("HashUtils module initialized");
    return true;
}

bool ModuleManager::initializeECCOperationsModule() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    ModuleMetrics& metrics = module_metrics_["ECCOperations"];
    metrics.module_name = "ECCOperations";
    metrics.status = ModuleStatus::INITIALIZING;
    metrics.initialization_time = std::chrono::system_clock::now();

    // Initialize ECC operations module
    // This would include validating ECC constants, memory layouts, etc.

    metrics.status = ModuleStatus::INITIALIZED;
    logInfo("ECCOperations module initialized");
    return true;
}

bool ModuleManager::initializeLegacyAdapterModule() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    ModuleMetrics& metrics = module_metrics_["LegacyAdapter"];
    metrics.module_name = "LegacyAdapter";
    metrics.status = ModuleStatus::INITIALIZING;
    metrics.initialization_time = std::chrono::system_clock::now();

    // Initialize legacy adapter module
    // This would include setting up tracking, validation, etc.

    metrics.status = ModuleStatus::INITIALIZED;
    logInfo("LegacyAdapter module initialized");
    return true;
}

double ModuleManager::calculateSystemEfficiency() const {
    if (module_metrics_.empty()) {
        return 0.0;
    }

    double total_efficiency = 0.0;
    size_t valid_metrics = 0;

    for (const auto& [name, metrics] : module_metrics_) {
        if (metrics.memory_efficiency > 0.0) {
            total_efficiency += metrics.memory_efficiency;
            valid_metrics++;
        }
    }

    return valid_metrics > 0 ? total_efficiency / valid_metrics : 0.0;
}

std::vector<std::string> ModuleManager::getPerformanceBottlenecks() const {
    std::vector<std::string> bottlenecks;

    for (const auto& [name, metrics] : module_metrics_) {
        if (metrics.execution_time_ms > 100.0) {
            bottlenecks.push_back(name + " has high execution time (" +
                                std::to_string(metrics.execution_time_ms) + "ms)");
        }

        if (metrics.memory_efficiency < 0.5) {
            bottlenecks.push_back(name + " has low memory efficiency (" +
                                std::to_string(metrics.memory_efficiency) + ")");
        }

        if (metrics.error_count > 10) {
            bottlenecks.push_back(name + " has high error rate (" +
                                std::to_string(metrics.error_count) + " errors)");
        }
    }

    return bottlenecks;
}

bool ModuleManager::meetsPerformanceTargets() const {
    for (const auto& [name, metrics] : module_metrics_) {
        if (metrics.memory_efficiency < configuration_.target_memory_efficiency * 0.8) {
            return false;
        }
        if (metrics.occupancy < configuration_.target_occupancy * 0.8) {
            return false;
        }
    }

    return true;
}

std::string ModuleManager::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S UTC");
    return ss.str();
}

void ModuleManager::logInfo(const std::string& message) const {
    std::cout << "[INFO] ModuleManager: " << message << std::endl;
}

void ModuleManager::logWarning(const std::string& message) const {
    std::cerr << "[WARNING] ModuleManager: " << message << std::endl;
}

void ModuleManager::logError(const std::string& message) const {
    std::cerr << "[ERROR] ModuleManager: " << message << std::endl;
}

void ModuleManager::logDebug(const std::string& message) const {
    if (configuration_.enable_debug_logging) {
        std::cout << "[DEBUG] ModuleManager: " << message << std::endl;
    }
}

} // namespace common
} // namespace keyhunt
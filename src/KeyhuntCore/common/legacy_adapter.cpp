// Puzzle71Solver - Legacy Adapter Implementation
// Backward compatibility adapter implementation (T022)

#include "legacy_adapter.cuh"
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace keyhunt {
namespace legacy {

// Global tracking for legacy function usage
namespace {
    std::mutex g_legacy_usage_mutex;
    std::unordered_map<std::string, size_t> g_legacy_usage_counts;
    bool g_legacy_usage_detected = false;
}

// Host-side implementation of legacy adapter functions

bool HasLegacyUsage() {
    std::lock_guard<std::mutex> lock(g_legacy_usage_mutex);
    return g_legacy_usage_detected;
}

std::string GetLegacyUsageStats() {
    std::lock_guard<std::mutex> lock(g_legacy_usage_mutex);

    json stats = {
        {"adapter_version", legacy_info::ADAPTER_VERSION},
        {"adapter_name", legacy_info::ADAPTER_NAME},
        {"usage_detected", g_legacy_usage_detected},
        {"function_calls", g_legacy_usage_counts},
        {"total_calls", 0},
        {"migration_priority", json::array()}
    };

    // Calculate total calls
    size_t total_calls = 0;
    for (const auto& [func_name, count] : g_legacy_usage_counts) {
        total_calls += count;
    }
    stats["total_calls"] = total_calls;

    // Create migration priority list based on usage frequency
    std::vector<std::pair<std::string, size_t>> sorted_functions(
        g_legacy_usage_counts.begin(), g_legacy_usage_counts.end());
    std::sort(sorted_functions.begin(), sorted_functions.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    for (const auto& [func_name, count] : sorted_functions) {
        stats["migration_priority"].push_back({
            {"function", func_name},
            {"call_count", count},
            {"percentage", static_cast<double>(count) / total_calls * 100.0}
        });
    }

    // Add migration recommendations
    if (total_calls > 0) {
        stats["recommendations"] = {
            {
                {"priority", "high"},
                {"action", "Replace most frequently used legacy functions"},
                {"functions", sorted_functions.size() > 0 ?
                    std::vector<std::string>{sorted_functions[0].first} :
                    std::vector<std::string>{}}
            },
            {
                {"priority", "medium"},
                {"action", "Add compile-time warnings for legacy usage"},
                {"setting", "Define LEGACY_ADAPTER_DEBUG to enable tracking"}
            },
            {
                {"priority", "low"},
                {"action", "Remove legacy adapter after complete migration"},
                {"condition", "When total_calls reaches 0"}
            }
        };
    }

    return stats.dump(2);
}

void ResetLegacyUsageTracking() {
    std::lock_guard<std::mutex> lock(g_legacy_usage_mutex);
    g_legacy_usage_counts.clear();
    g_legacy_usage_detected = false;
}

// Internal function to record legacy usage (called from device-side wrappers)
// Note: This would need to be implemented with device-host communication
// For now, we provide a host-side interface for testing

void RecordLegacyUsage(const std::string& function_name) {
    std::lock_guard<std::mutex> lock(g_legacy_usage_mutex);
    g_legacy_usage_counts[function_name]++;
    g_legacy_usage_detected = true;
}

// Utility functions for migration assistance

void PrintLegacyUsageReport() {
    if (!HasLegacyUsage()) {
        std::cout << "Legacy Adapter: No legacy function usage detected.\n";
        return;
    }

    std::cout << "=== Legacy Adapter Usage Report ===\n";
    std::cout << GetLegacyUsageStats() << std::endl;

    std::cout << "\nMigration Recommendations:\n";
    std::cout << "1. Replace legacy function calls with unified module calls\n";
    std::cout << "2. Use EMIT_CANDIDATE_MIGRATED() macro for gradual migration\n";
    std::cout << "3. Define LEGACY_ADAPTER_DEBUG to track usage during development\n";
    std::cout << "4. Remove this adapter after complete migration\n";
    std::cout << "========================================\n";
}

bool ValidateLegacyAdapter() {
    // Validate that the legacy adapter is working correctly
    std::cout << "Validating Legacy Adapter...\n";

    // Test basic functionality
    RecordLegacyUsage("test_function");

    if (!HasLegacyUsage()) {
        std::cerr << "ERROR: Legacy adapter tracking not working\n";
        return false;
    }

    ResetLegacyUsageTracking();

    std::cout << "Legacy Adapter validation passed\n";
    return true;
}

// Compatibility checks

bool IsLegacyCodePresent(const std::string& file_path) {
    // Simple heuristic to check if a file contains legacy function calls
    // In a real implementation, this would use more sophisticated parsing

    std::ifstream file(file_path);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::vector<std::string> legacy_patterns = {
        "EmitCandidate(",
        "FinalizeDigest(",
        "readInt(",
        "writeInt(",
        "copyBigInt(",
        "isInfinity(",
        "readIntLSW("
    };

    while (std::getline(file, line)) {
        for (const auto& pattern : legacy_patterns) {
            if (line.find(pattern) != std::string::npos) {
                // Check if it's not already using the unified namespace
                if (line.find("keyhunt::common::") == std::string::npos &&
                    line.find("keyhunt::legacy::") == std::string::npos) {
                    return true;
                }
            }
        }
    }

    return false;
}

std::vector<std::string> FindFilesWithLegacyCode(const std::string& directory) {
    std::vector<std::string> files_with_legacy;

    // This would scan the directory for files containing legacy patterns
    // For now, it's a placeholder implementation

    return files_with_legacy;
}

} // namespace legacy
} // namespace keyhunt
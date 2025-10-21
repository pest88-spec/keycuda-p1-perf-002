// Puzzle71Solver - Naming Conventions Implementation
// Architecture Modernization - Standardized naming conventions (T037)

#include "naming_conventions.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace keyhunt {
namespace standards {

bool NamingConventions::isCamelCase(const std::string& name) {
    if (name.empty()) return false;

    // First character must be lowercase
    if (!std::islower(name[0])) return false;

    // Check for valid characters and camelCase pattern
    bool next_should_be_upper = false;
    for (size_t i = 1; i < name.size(); ++i) {
        if (std::isupper(name[i])) {
            // Uppercase letters are allowed (new word boundaries)
            continue;
        } else if (std::islower(name[i]) || std::isdigit(name[i])) {
            // Lowercase letters and digits are always allowed
            continue;
        } else if (name[i] == '_') {
            // Underscores are not allowed in camelCase (except for special cases)
            return false;
        } else {
            // Other characters are not allowed
            return false;
        }
    }

    return true;
}

bool NamingConventions::isSnakeCase(const std::string& name) {
    if (name.empty()) return false;

    // All characters must be lowercase, digits, or underscores
    // Cannot start or end with underscore
    // Cannot have consecutive underscores

    if (name[0] == '_' || name[name.size() - 1] == '_') return false;

    bool prev_underscore = false;
    for (char c : name) {
        if (std::islower(c) || std::isdigit(c)) {
            prev_underscore = false;
            continue;
        } else if (c == '_') {
            if (prev_underscore) return false; // consecutive underscores
            prev_underscore = true;
        } else {
            // Uppercase letters and other characters not allowed
            return false;
        }
    }

    return true;
}

bool NamingConventions::isPascalCase(const std::string& name) {
    if (name.empty()) return false;

    // First character must be uppercase
    if (!std::isupper(name[0])) return false;

    // Check for valid characters and PascalCase pattern
    for (size_t i = 1; i < name.size(); ++i) {
        if (std::isupper(name[i]) || std::islower(name[i]) || std::isdigit(name[i])) {
            // Alphabetic characters and digits are allowed
            continue;
        } else {
            // Other characters (including underscores) are not allowed
            return false;
        }
    }

    return true;
}

std::string NamingConventions::pascalToCamel(const std::string& name) {
    if (name.empty()) return name;

    std::string result = name;
    if (!result.empty() && std::isupper(result[0])) {
        result[0] = std::tolower(result[0]);
    }

    return result;
}

std::string NamingConventions::snakeToCamel(const std::string& name) {
    if (name.empty()) return name;

    std::string result;
    bool capitalize_next = false;

    for (char c : name) {
        if (c == '_') {
            capitalize_next = true;
        } else if (capitalize_next) {
            result += std::toupper(c);
            capitalize_next = false;
        } else {
            result += c;
        }
    }

    return result;
}

std::string NamingConventions::suggestStandardName(const std::string& legacy_name, const std::string& type) {
    if (type == "function") {
        // Handle common legacy patterns
        if (legacy_name == "EmitCandidate") return StandardNames::EMIT_CANDIDATE;
        if (legacy_name == "FinalizeDigest") return StandardNames::FINALIZE_DIGEST;
        if (legacy_name == "ComputeEccPoint") return StandardNames::COMPUTE_ECC_POINT;
        if (legacy_name == "HashSha256") return StandardNames::HASH_SHA256;
        if (legacy_name == "HashRipemd160") return StandardNames::HASH_RIPEMD160;
        if (legacy_name == "CompareDigest") return StandardNames::COMPARE_DIGEST;
        if (legacy_name == "PrepareBatch") return StandardNames::PREPARE_BATCH;
        if (legacy_name == "ExecuteKernel") return StandardNames::EXECUTE_KERNEL;

        // If already camelCase, return as-is
        if (isCamelCase(legacy_name)) return legacy_name;

        // Convert PascalCase to camelCase
        if (isPascalCase(legacy_name)) return pascalToCamel(legacy_name);

        // Convert snake_case to camelCase
        if (isSnakeCase(legacy_name)) return snakeToCamel(legacy_name);

        // Default: try to convert by lowercasing first character
        if (!legacy_name.empty() && std::isupper(legacy_name[0])) {
            return pascalToCamel(legacy_name);
        }
    }
    else if (type == "variable") {
        // Handle common legacy patterns
        if (legacy_name == "deviceCount") return StandardNames::DEVICE_COUNT;
        if (legacy_name == "batchSize") return StandardNames::BATCH_SIZE;
        if (legacy_name == "targetHash") return StandardNames::TARGET_HASH;
        if (legacy_name == "privateKey") return StandardNames::PRIVATE_KEY;
        if (legacy_name == "publicKey") return StandardNames::PUBLIC_KEY;

        // If already snake_case, return as-is
        if (isSnakeCase(legacy_name)) return legacy_name;

        // Convert camelCase to snake_case
        std::string result;
        for (char c : legacy_name) {
            if (std::isupper(c)) {
                if (!result.empty()) result += '_';
                result += std::tolower(c);
            } else {
                result += c;
            }
        }
        return result;
    }
    else if (type == "constant") {
        // Handle common legacy patterns
        if (legacy_name == "MAX_BATCH_SIZE") return StandardNames::MAX_BATCH_SIZE;
        if (legacy_name == "DEFAULT_THREADS_PER_BLOCK") return StandardNames::DEFAULT_THREADS_PER_BLOCK;
        if (legacy_name == "MAX_WARP_SIZE") return StandardNames::MAX_WARP_SIZE;
        if (legacy_name == "GPU_MEMORY_ALIGNMENT") return StandardNames::GPU_MEMORY_ALIGNMENT;

        // If already PascalCase, return as-is
        if (isPascalCase(legacy_name)) return legacy_name;

        // Convert snake_case to PascalCase
        if (isSnakeCase(legacy_name)) {
            std::string result;
            bool capitalize_next = true;
            for (char c : legacy_name) {
                if (c == '_') {
                    capitalize_next = true;
                } else if (capitalize_next) {
                    result += std::toupper(c);
                    capitalize_next = false;
                } else {
                    result += c;
                }
            }
            return result;
        }

        // Convert camelCase to PascalCase
        if (!legacy_name.empty() && std::islower(legacy_name[0])) {
            return std::toupper(legacy_name[0]) + legacy_name.substr(1);
        }
    }

    // Default: return original name if no conversion rules apply
    return legacy_name;
}

} // namespace standards
} // namespace keyhunt
/**
 * @file test_config_validator.cpp
 * @brief Test program to demonstrate the configuration validator functionality
 */

#include "src/config/puzzle71_config_validator.h"
#include <iostream>

using namespace keyhunt::config;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file.yaml>" << std::endl;
        return 1;
    }

    std::string config_file = argv[1];
    std::cout << "=== Puzzle71 Configuration Validator Test ===" << std::endl;
    std::cout << "Validating configuration file: " << config_file << std::endl;
    std::cout << std::endl;

    // Validate configuration
    ValidationResult result;
    bool success = Puzzle71ConfigValidator::validate_config_file(config_file, result);

    // Generate and print report
    std::string report = Puzzle71ConfigValidator::generate_validation_report(result);
    std::cout << report << std::endl;

    // Print JSON output
    std::cout << "=== JSON Validation Result ===" << std::endl;
    std::cout << result.to_json() << std::endl;

    return success ? 0 : 1;
}
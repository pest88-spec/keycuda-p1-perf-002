// Puzzle71Solver - Simple Logging Utilities
// Basic logging functions for the performance monitoring components

#pragma once

#include <iostream>
#include <string>
#include <fstream>

namespace keyhunt {

inline void log_info(const std::string& message) {
    std::cout << "[INFO] " << message << std::endl;
}

inline void log_warning(const std::string& message) {
    std::cout << "[WARNING] " << message << std::endl;
}

inline void log_error(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

} // namespace keyhunt
// Puzzle71 Technical Debt Repair - Fixed Legacy Adapter Implementation
// Addresses P0/blocking and P1/high priority issues from v5.5 technical debt audit
// Implements T024-T025: Fixed adapter layer to eliminate code duplication

#include "legacy_adapter_fixed.cuh"
#include <cstring>
#include <chrono>
#include <cmath>

namespace keyhunt {
namespace adapter {

// Global adapter instance
std::unique_ptr<LegacyAdapterFixed> g_adapter_instance = nullptr;

// LegacyAdapterFixed Implementation

LegacyAdapterFixed::LegacyAdapterFixed()
    : initialized_(false), kernel_launch_count_(0), kernel_success_count_(0),
      kernel_failure_count_(0), total_allocated_(0), peak_allocation_(0) {

    memset(last_error_, 0, sizeof(last_error_));
}

LegacyAdapterFixed::~LegacyAdapterFixed() {
    cleanup();
}

bool LegacyAdapterFixed::initialize(const AdapterConfig& config) {
    if (initialized_) {
        update_error("LegacyAdapterFixed already initialized");
        return false;
    }

    // Validate configuration
    if (!validate_configuration()) {
        return false;
    }

    config_ = config;

    // Setup core components
    if (!setup_core_components()) {
        return false;
    }

    // Setup legacy compatibility
    if (!setup_legacy_compatibility()) {
        return false;
    }

    initialized_ = true;
    return true;
}

void LegacyAdapterFixed::cleanup() {
    if (ecc_operations_) {
        ecc_operations_->cleanup();
        ecc_operations_.reset();
    }

    initialized_ = false;
}

bool LegacyAdapterFixed::setup_ecc_operations(const ecc::ECCBatchConfig& ecc_config) {
    if (!initialized_) {
        update_error("Adapter not initialized");
        return false;
    }

    ecc_operations_ = std::make_unique<ecc::ECCOperationsFixed>();
    if (!ecc_operations_->initialize(ecc_config)) {
        update_error("Failed to initialize ECC operations");
        return false;
    }

    return true;
}

bool LegacyAdapterFixed::validate_configuration() {
    if (config_.pool_size_bytes == 0) {
        update_error("Pool size must be greater than 0");
        return false;
    }

    if (config_.alignment_bytes == 0 || (config_.alignment_bytes & (config_.alignment_bytes - 1)) != 0) {
        update_error("Alignment must be a power of 2");
        return false;
    }

    return true;
}

void LegacyAdapterFixed::update_error(const char* error) {
    strncpy(last_error_, error, sizeof(last_error_) - 1);
    last_error_[sizeof(last_error_) - 1] = '\0';
}

bool LegacyAdapterFixed::setup_core_components() {
    // Validate constitutional requirements
    if (config_.enforce_static_configuration && !config_.disable_runtime_device_queries) {
        update_error("Static configuration enforcement requires runtime device queries to be disabled");
        return false;
    }

    return true;
}

bool LegacyAdapterFixed::setup_legacy_compatibility() {
    // Setup compatibility with existing legacy interfaces
    return true;
}

// Global adapter functions

bool initialize_global_adapter(const AdapterConfig& config) {
    if (g_adapter_instance) {
        return true; // Already initialized
    }

    g_adapter_instance = std::make_unique<LegacyAdapterFixed>();
    return g_adapter_instance->initialize(config);
}

void cleanup_global_adapter() {
    if (g_adapter_instance) {
        g_adapter_instance->cleanup();
        g_adapter_instance.reset();
    }
}

LegacyAdapterFixed* get_global_adapter() {
    return g_adapter_instance.get();
}

} // namespace adapter
} // namespace keyhunt
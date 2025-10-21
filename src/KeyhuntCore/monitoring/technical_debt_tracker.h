/**
 * @file technical_debt_tracker.h
 * @brief Technical debt tracking system data structures and management
 *
 * This header defines the data structures and management system for tracking
 * technical debt items identified in the Puzzle71 audit v5.5, including their
 * resolution status, dependencies, and progress metrics.
 *
 * Requirements Addressed:
 * - T016: Initialize technical debt tracking system data structures
 * - User Story 4: Complete System Migration and Quality Assurance
 * - Audit v5.5: Technical debt tracking and resolution verification
 *
 * @author Puzzle71 Technical Debt Repair System
 * @version 5.5
 * @date 2025-10-20
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>

namespace puzzle71 {
namespace monitoring {

/**
 * @brief Technical debt priority classification
 *
 * Maps to the priority levels defined in audit v5.5.
 */
enum class DebtPriority {
    P0_BLOCKING = 0,    ///< P0: Blocking issues that prevent deployment
    P1_HIGH = 1,        ///< P1: High priority issues affecting reliability
    P2_MEDIUM = 2,      ///< P2: Medium priority issues for quality
    P3_LOW = 3           ///< P3: Low priority issues for maintenance
};

/**
 * @brief Technical debt category classification
 *
 * Categorizes the type of technical debt issue.
 */
enum class DebtCategory {
    ALGORITHM = 0,        ///< Algorithm-related issues
    PERFORMANCE = 1,      ///< Performance optimization issues
    ARCHITECTURE = 2,     ///< Architecture and design issues
    TESTING = 3,          ///< Testing and validation issues
    CONFIGURATION = 4,    ///< Configuration and build issues
    CODE_QUALITY = 5,      ///< Code quality and maintainability
    SECURITY = 6,         ///< Security-related issues
    DOCUMENTATION = 7     ///< Documentation and knowledge sharing
};

/**
 * @brief Resolution status of technical debt items
 */
enum class ResolutionStatus {
    IDENTIFIED = 0,       ///< Issue identified but not yet addressed
    IN_PROGRESS = 1,       ///< Currently being worked on
    RESOLVED = 2,         ///< Fix implemented but not yet verified
    VERIFIED = 3,          ///< Fix verified and confirmed
    REOPENED = 4,          ///< Issue reopened (regression or new findings)
    WONT_FIX = 5,          ## Decision made not to fix (justified)
    DEFERRED = 6           ## Deferred to future release
};

/**
 * @brief Technical debt item metadata
 *
 * Represents a single technical debt issue with complete tracking information.
 */
struct TechnicalDebtItem {
    std::string id;                           ///< Unique identifier (e.g., "P0-001")
    DebtPriority priority;                     ///< Priority classification
    DebtCategory category;                   ///< Issue category
    std::string title;                        ///< Brief description of the issue
    std::string description;                  ///< Detailed explanation of the problem
    std::string location;                    ///< File path and line number
    std::string component;                   /// Component or module affected
    ResolutionStatus status;                  ///< Current resolution status
    std::string assigned_to;                  ///< Developer or team responsible
    std::string created_by;                   ///< Who identified the issue
    std::chrono::system_clock::time_point created_at; ///< When issue was identified
    std::chrono::system_clock::time_point updated_at; ///< Last update timestamp
    std::chrono::system_clock::time_point resolved_at; ///< When issue was resolved
    std::string resolution_notes;              ///< Notes on resolution approach
    std::vector<std::string> dependencies;      ///< Other items this depends on
    std::vector<std::string> dependents;        ///< Items that depend on this
    std::string git_commit;                   ///< Git commit hash for resolution
    std::string evidence_file;                ///< File with resolution evidence
    std::map<std::string, std::string> metadata;    ///< Additional metadata

    /**
     * @brief Serialize item to JSON
     * @return JSON representation
     */
    nlohmann::json to_json() const;

    /**
     * @brief Load item from JSON
     * @param j JSON object
     * @return True if loading successful
     */
    bool from_json(const nlohmann::json& j);

    /**
     * @brief Get priority as string
     * @return Priority string representation
     */
    std::string get_priority_string() const;

    /**
     * @brief Get category as string
     * @return Category string representation
     */
    std::string get_category_string() const;

    /**
     * @brief Get status as string
     * @return Status string representation
     */
    std::string get_status_string() const;

    /**
     * @brief Check if item is blocking
     * @return True if P0 blocking issue
     */
    bool is_blocking() const { return priority == DebtPriority::P0_BLOCKING; }

    /**
     * @brief Check if item is resolved
     * @return True if status is RESOLVED or VERIFIED
     */
    bool is_resolved() const {
        return status == ResolutionStatus::RESOLVED ||
               status == ResolutionStatus::VERIFIED;
    }

    /**
     * @brief Check if item is active
     * @return True if status is IDENTIFIED or IN_PROGRESS
     */
    bool is_active() const {
        return status == ResolutionStatus::IDENTIFIED ||
               status == ResolutionStatus::IN_PROGRESS;
    }
};

/**
 * @brief Technical debt resolution metrics
 *
 * Tracks progress metrics for technical debt resolution.
 */
struct DebtResolutionMetrics {
    std::chrono::system_clock::time_point tracking_start; ///< When tracking started
    std::chrono::system_clock::time_point last_update;    ///< Last metrics update
    uint32_t total_items;                         ///< Total number of debt items
    uint32_t resolved_items;                     ///< Number of resolved items
    uint32_t in_progress_items;                   ///< Items currently in progress
    uint32_t blocking_items;                      ///< P0 blocking items
    uint32_t high_priority_items;                 ///< P1 high priority items
    uint32_t medium_priority_items;               ///< P2 medium priority items

    // Category breakdowns
    std::map<DebtCategory, uint32_t> items_by_category;
    std::map<DebtCategory, uint32_t> resolved_by_category;

    // Time-based metrics
    std::map<std::string, double> avg_resolution_time_days;  ///< Avg resolution time by category
    std::map<std::string, double> total_effort_hours;      ///< Total effort by category

    /**
     * @brief Calculate overall resolution rate
     * @return Resolution rate as percentage (0.0-1.0)
     */
    double get_resolution_rate() const {
        return total_items > 0 ? static_cast<double>(resolved_items) / total_items : 0.0;
    }

    /**
     * @brief Calculate blocking issue resolution rate
     * @return P0 resolution rate as percentage
     */
    double get_blocking_resolution_rate() const {
        return blocking_items > 0 ? static_cast<double>(resolved_items) / blocking_items : 1.0;
    }

    /**
     * @brief Serialize metrics to JSON
     * @return JSON representation
     */
    nlohmann::json to_json() const;

    /**
     * @brief Load metrics from JSON
     * @param j JSON object
     * @return True if loading successful
     */
    bool from_json(const nlohmann::json& j);

    /**
     * @brief Update metrics based on debt items
     * @param items List of technical debt items
     */
    void update_from_items(const std::vector<TechnicalDebtItem>& items);
};

/**
 * @brief Technical debt tracking session
 *
 * Represents a tracking session for analyzing and managing technical debt.
 */
struct DebtTrackingSession {
    std::string session_id;                      ///< Unique session identifier
    std::string description;                     ///< Session description
    std::chrono::system_clock::time_point start_time; ///< Session start time
    std::chrono::system_clock::time_point end_time;   ///< Session end time
    std::string analysis_scope;                 ///< Scope of analysis
    std::vector<std::string> analyzed_files;        ///< Files analyzed
    std::vector<TechnicalDebtItem> items_found;     ///< Items discovered
    DebtResolutionMetrics metrics;                ///< Resolution metrics

    /**
     * @brief Serialize session to JSON
     * @return JSON representation
     */
    nlohmann::json to_json() const;

    /**
     * @brief Load session from JSON
     * @param j JSON object
     * @return True if loading successful
     */
    bool from_json(const nlohmann::json& j);

    /**
     * @brief Calculate session duration
     * @return Duration in seconds
     */
    double duration_seconds() const {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            end_time - start_time);
        return duration.count();
    }

    /**
     * @brief Get session summary
     * @return Human-readable summary
     */
    std::string get_summary() const;
};

/**
 * @brief Technical debt tracker main class
 *
 * Provides comprehensive tracking and management of technical debt items
 * with persistence, analysis, and reporting capabilities.
 */
class TechnicalDebtTracker {
public:
    /**
     * @brief Constructor
     * @param storage_dir Directory for storing tracking data
     */
    explicit TechnicalDebtTracker(const std::string& storage_dir);

    /**
     * @brief Destructor
     */
    ~TechnicalDebtTracker();

    /**
     * @brief Load technical debt items from audit data
     * @param audit_file Path to audit v5.5 file
     * @return True if loading successful
     */
    bool load_from_audit(const std::string& audit_file);

    /**
     * @brief Add a new technical debt item
     * @param item Debt item to add
     * @return True if addition successful
     */
    bool add_item(const TechnicalDebtItem& item);

    /**
     * @brief Update an existing debt item
     * @param id Item identifier
     * @param updates Updates to apply
     * @return True if update successful
     */
    bool update_item(const std::string& id, const nlohmann::json& updates);

    /**
     * @brief Get all debt items
     * @return List of all debt items
     */
    std::vector<TechnicalDebtItem> get_all_items() const;

    /**
     * @brief Get items by priority
     * @param priority Priority level to filter by
     * @return List of items with specified priority
     */
    std::vector<TechnicalDebtItem> get_items_by_priority(DebtPriority priority) const;

    /**
     * @brief Get items by category
     * @param category Category to filter by
     * @return List of items in specified category
     */
    std::vector<TechnicalDebtItem> get_items_by_category(DebtCategory category) const;

    /**
     * @brief Get items by status
     * @param status Status to filter by
     * @return List of items with specified status
     */
    std::vector<TechnicalDebtItem> get_items_by_status(ResolutionStatus status) const;

    /**
     * @brief Get item by ID
     * @param id Item identifier
     * @return Pointer to item or nullptr if not found
     */
    const TechnicalDebtItem* get_item(const std::string& id) const;

    /**
     * @brief Start a new tracking session
     * @param description Session description
     * @param scope Analysis scope
     * @return Session ID for tracking
     */
    std::string start_session(const std::string& description,
                               const std::string& scope = "");

    /**
     * @brief End current tracking session
     * @param session_id Session ID to end
     * @return Session summary
     */
    DebtTrackingSession end_session(const std::string& session_id);

    /**
     * @brief Get current resolution metrics
     * @return Current metrics
     */
    DebtResolutionMetrics get_metrics() const;

    /**
     * @brief Generate tracking report
     * @param session_id Session ID to generate report for (optional)
     * @return Formatted report string
     */
    std::string generate_report(const std::string& session_id = "") const;

    /**
     * @brief Generate summary for specific filters
     * @param filters JSON object with filter criteria
     * @return Summary report
     */
    std::string generate_summary(const nlohmann::json& filters = {}) const;

    /**
     * @brief Export tracking data to file
     * @param output_file Output file path
     * @param format Export format (json, csv, markdown)
     * @return True if export successful
     */
    bool export_data(const std::string& output_file,
                      const std::string& format = "json") const;

    /**
     * @brief Import tracking data from file
     * @param input_file Input file path
     * @param format Import format (json, csv)
     * @return True if import successful
     */
    bool import_data(const std::string& input_file,
                      const std::string& format = "json");

    /**
     * @brief Validate tracking data integrity
     * @return True if data is consistent and valid
     */
    bool validate_data_integrity() const;

    /**
     * @brief Get dependency graph for items
     * @return Dependency graph as adjacency list
     */
    std::map<std::string, std::vector<std::string>> get_dependency_graph() const;

    /**
     * @brief Identify circular dependencies
     * @return List of circular dependency chains
     */
    std::vector<std::vector<std::string>> find_circular_dependencies() const;

    /**
     * @brief Get recommended resolution order
     * @return Ordered list of item IDs respecting dependencies
     */
    std::vector<std::string> get_resolution_order() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

    /**
     * @brief Get file path for tracking data
     * @param filename Base filename
     * @return Full file path
     */
    std::string get_data_path(const std::string& filename) const;

    /**
     * @brief Ensure storage directory exists
     */
    void ensure_storage_directory() const;

    /**
     * @brief Parse priority from string
     * @param priority_str Priority string
     * @return Priority enum value
     */
    DebtPriority parse_priority(const std::string& priority_str) const;

    /**
     * @brief Parse category from string
     * @param category_str Category string
     * @return Category enum value
     */
    DebtCategory parse_category(const std::string& category_str) const;

    /**
     * @brief Parse status from string
     * @param status_str Status string
     * @return Status enum value
     */
    ResolutionStatus parse_status(const std::string& status_str) const;

    /**
     * @brief Validate item consistency
     * @param item Item to validate
     * @return True if item is valid
     */
    bool validate_item(const TechnicalDebtItem& item) const;

    /**
     * @brief Update internal metrics
     */
    void update_metrics();

    /**
     * @brief Generate unique session ID
     * @return Unique session identifier
     */
    std::string generate_session_id() const;
};

/**
 * @brief RAII helper for tracking sessions
 *
 * Automatically manages tracking session lifecycle within a scope.
 */
class TrackingSessionGuard {
public:
    /**
     * @brief Constructor - starts tracking session
     * @param tracker Tracker instance
     * @param description Session description
     * @param scope Analysis scope
     */
    TrackingSessionGuard(TechnicalDebtTracker& tracker,
                        const std::string& description,
                        const std::string& scope = "");

    /**
     * @brief Destructor - ends session and generates report
     */
    ~TrackingSessionGuard();

    /**
     * @brief Get session ID
     * @return Session ID
     */
    const std::string& session_id() const { return session_id_; }

    /**
     * @brief Get session summary
     * @return Session summary
     */
    std::string get_summary() const;

private:
    TechnicalDebtTracker& tracker_;
    std::string session_id_;
    bool session_active_;
};

/**
 * @brief Technical debt analysis utilities
 */
namespace debt_analysis {
    /**
     * @brief Analyze technical debt trends
     * @param tracker Tracker instance
     * @param days Number of days to analyze
     * @return Trend analysis report
     */
    std::string analyze_trends(TechnicalDebtTracker& tracker, int days = 30);

    /**
     * @brief Identify hotspots in codebase
     * @param tracker Tracker instance
     * @return List of files with most debt items
     */
    std::vector<std::pair<std::string, uint32_t>> identify_hotspots(
        TechnicalDebtTracker& tracker);

    /**
     * @brief Estimate resolution effort
     * @param tracker Tracker instance
     * @return Effort estimation in person-hours
     */
    double estimate_resolution_effort(TechnicalDebtTracker& tracker);

    /**
     * @brief Generate resolution recommendations
     * @param tracker Tracker instance
     * @return Priority-ordered list of recommendations
     */
    std::vector<std::string> generate_recommendations(
        TechnicalDebtTracker& tracker);
}

} // namespace monitoring
} // namespace puzzle71
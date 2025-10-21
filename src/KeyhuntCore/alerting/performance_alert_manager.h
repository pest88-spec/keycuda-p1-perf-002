// Puzzle71Solver - Performance Alerting and Notification System (T049)
// Phase 6: User Story 4 - Performance Monitoring
// Multi-channel alerting system with email, Slack, Discord, SMS, and browser notifications

#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <queue>
#include <condition_variable>

#include "monitoring/real_time_monitor.h"
#include "regression/performance_regression_detector.h"

namespace puzzle71::alerting {

/**
 * @brief Alert channel types
 */
enum class AlertChannel {
    EMAIL,
    SLACK,
    DISCORD,
    SMS,
    BROWSER_NOTIFICATION,
    WEBHOOK,
    CONSOLE_LOG
};

/**
 * @brief Alert priority levels
 */
enum class AlertPriority {
    LOW,
    NORMAL,
    HIGH,
    CRITICAL
};

/**
 * @brief Alert message structure
 */
struct AlertMessage {
    std::string id;
    std::string title;
    std::string message;
    AlertPriority priority{AlertPriority::NORMAL};
    std::chrono::system_clock::time_point timestamp;
    std::string category;
    std::map<std::string, std::string> metadata;

    // Delivery tracking
    std::map<AlertChannel, bool> delivery_status;
    std::map<AlertChannel, std::chrono::system_clock::time_point> delivery_attempts;
    std::map<AlertChannel, std::vector<std::string>> delivery_errors;

    /**
     * @brief Check if alert has been successfully delivered to all channels
     */
    bool isFullyDelivered() const;

    /**
     * @brief Get delivery status as string
     */
    std::string getDeliveryStatus() const;

    /**
     * @brief Serialize alert to JSON
     */
    std::string toJson() const;

    /**
     * @brief Deserialize alert from JSON
     */
    static AlertMessage fromJson(const std::string& json);
};

/**
 * @brief Alert channel configuration
 */
struct ChannelConfiguration {
    AlertChannel type;
    bool enabled{true};
    AlertPriority minimum_priority{AlertPriority::NORMAL};
    std::chrono::seconds cooldown_period{std::chrono::seconds(300)};  // 5 minutes
    size_t max_retries{3};
    std::chrono::seconds retry_delay{std::chrono::seconds(30)};

    // Channel-specific settings
    std::map<std::string, std::string> channel_settings;
    std::map<std::string, std::string> custom_headers;
    std::string authentication_token;
    std::string webhook_url;

    // Rate limiting
    std::chrono::seconds rate_limit_window{std::chrono::seconds(60)};
    size_t max_alerts_per_window{10};

    /**
     * @brief Validate configuration
     */
    bool isValid() const;

    /**
     * @brief Serialize configuration to JSON
     */
    std::string toJson() const;

    /**
     * @brief Deserialize configuration from JSON
     */
    static ChannelConfiguration fromJson(const std::string& json);
};

/**
 * @brief Alert template configuration
 */
struct AlertTemplate {
    std::string id;
    std::string name;
    std::string subject_template;
    std::string body_template;
    std::map<std::string, std::string> default_variables;
    std::vector<AlertChannel> supported_channels;

    /**
     * @brief Process template with alert data
     */
    std::string processTemplate(const AlertMessage& alert) const;

    /**
     * @brief Validate template
     */
    bool isValid() const;

    /**
     * @brief Serialize template to JSON
     */
    std::string toJson() const;

    /**
     * @brief Deserialize template from JSON
     */
    static AlertTemplate fromJson(const std::string& json);
};

/**
 * @brief Alert delivery result
 */
struct DeliveryResult {
    AlertChannel channel;
    bool success{false};
    std::string response;
    std::string error_message;
    std::chrono::milliseconds delivery_time;
    int retry_count{0};
    std::chrono::system_clock::time_point delivered_at;

    /**
     * @brief Check if delivery was successful
     */
    bool isSuccessful() const { return success; }

    /**
     * @brief Get result summary
     */
    std::string getSummary() const;
};

/**
 * @brief Alert rate limiter
 */
class AlertRateLimiter {
public:
    explicit AlertRateLimiter(const ChannelConfiguration& config);

    /**
     * @brief Check if alert can be sent (rate limiting)
     */
    bool canSendAlert(const std::string& alert_id) const;

    /**
     * @brief Record alert attempt
     */
    void recordAlert(const std::string& alert_id);

    /**
     * @brief Reset rate limiting counters
     */
    void reset();

    /**
     * @brief Get rate limiting statistics
     */
    struct RateLimitStats {
        size_t alerts_in_window{0};
        size_t alerts_blocked{0};
        std::chrono::system_clock::time_point window_start;
        std::chrono::seconds window_duration;
    };

    RateLimitStats getStatistics() const;

private:
    ChannelConfiguration config_;
    mutable std::mutex mutex_;

    struct AlertRecord {
        std::chrono::system_clock::time_point timestamp;
        std::string alert_id;
    };

    std::queue<AlertRecord> alert_queue_;
    std::map<std::string, std::chrono::system_clock::time_point> last_alert_times_;
};

/**
 * @brief Email alert sender
 */
class EmailSender {
public:
    explicit EmailSender(const ChannelConfiguration& config);

    /**
     * @brief Send email alert
     */
    DeliveryResult sendAlert(const AlertMessage& alert, const AlertTemplate& template_);

private:
    ChannelConfiguration config_;

    std::string buildEmailContent(const AlertMessage& alert, const AlertTemplate& template_) const;
    bool sendEmail(const std::string& to, const std::string& subject, const std::string& content);
    DeliveryResult handleEmailResponse(const std::string& response);
};

/**
 * @brief Slack alert sender
 */
class SlackSender {
public:
    explicit SlackSender(const ChannelConfiguration& config);

    /**
     * @brief Send Slack alert
     */
    DeliveryResult sendAlert(const AlertMessage& alert, const AlertTemplate& template_);

private:
    ChannelConfiguration config_;

    std::string buildSlackPayload(const AlertMessage& alert, const AlertTemplate& template_) const;
    DeliveryResult sendSlackMessage(const std::string& payload);
};

/**
 * @brief Discord alert sender
 */
class DiscordSender {
public:
    explicit DiscordSender(const ChannelConfiguration& config);

    /**
     * @brief Send Discord alert
     */
    DeliveryResult sendAlert(const AlertMessage& alert, const AlertTemplate& template_);

private:
    ChannelConfiguration config_;

    std::string buildDiscordEmbed(const AlertMessage& alert, const AlertTemplate& template_) const;
    DeliveryResult sendDiscordMessage(const std::string& embed);
};

/**
 * @brief SMS alert sender
 */
class SMSSender {
public:
    explicit SMSSender(const ChannelConfiguration& config);

    /**
     * @brief Send SMS alert
     */
    DeliveryResult sendAlert(const AlertMessage& alert, const AlertTemplate& template_);

private:
    ChannelConfiguration config_;

    std::string buildSMSContent(const AlertMessage& alert, const AlertTemplate& template_) const;
    DeliveryResult sendSMS(const std::string& phone_number, const std::string& message);
};

/**
 * @brief Webhook alert sender
 */
class WebhookSender {
public:
    explicit WebhookSender(const ChannelConfiguration& config);

    /**
     * @brief Send webhook alert
     */
    DeliveryResult sendAlert(const AlertMessage& alert, const AlertTemplate& template_);

private:
    ChannelConfiguration config_;

    std::string buildWebhookPayload(const AlertMessage& alert, const AlertTemplate& template_) const;
    DeliveryResult sendWebhook(const std::string& payload, const std::map<std::string, std::string>& headers);
};

/**
 * @brief Browser notification sender
 */
class BrowserNotificationSender {
public:
    explicit BrowserNotificationSender(const ChannelConfiguration& config);

    /**
     * @brief Send browser notification
     */
    DeliveryResult sendAlert(const AlertMessage& alert, const AlertTemplate& template_);

private:
    ChannelConfiguration config_;

    std::string buildNotificationContent(const AlertMessage& alert) const;
    bool requestNotificationPermission();
    bool showBrowserNotification(const std::string& title, const std::string& message);
};

/**
 * @brief Performance alert manager
 *
 * Multi-channel alerting system for performance monitoring and regression
 * detection with template support, rate limiting, and delivery tracking.
 */
class PerformanceAlertManager {
public:
    explicit PerformanceAlertManager();
    ~PerformanceAlertManager();

    // Channel management
    void addChannel(const ChannelConfiguration& channel);
    void removeChannel(AlertChannel channel);
    void enableChannel(AlertChannel channel, bool enabled);
    std::vector<ChannelConfiguration> getChannels() const;

    // Template management
    void addTemplate(const AlertTemplate& template_);
    void removeTemplate(const std::string& template_id);
    AlertTemplate getTemplate(const std::string& template_id) const;
    std::vector<AlertTemplate> getTemplates() const;

    // Alert sending
    std::vector<DeliveryResult> sendAlert(const AlertMessage& alert);
    std::vector<DeliveryResult> sendAlert(const AlertMessage& alert, const std::string& template_id);
    std::vector<DeliveryResult> sendAlert(const AlertMessage& alert, const AlertTemplate& template_);

    // Alert management
    void setDefaultTemplate(const std::string& template_id);
    void setChannelTemplate(AlertChannel channel, const std::string& template_id);
    std::vector<AlertMessage> getPendingAlerts() const;
    void clearPendingAlerts();

    // Monitoring integration
    void connectToRegressionDetector(std::shared_ptr<puzzle71::regression::PerformanceRegressionDetector> detector);
    void connectToMonitor(std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor);

    // Configuration
    void setGlobalConfiguration(std::map<std::string, std::string> config);
    std::map<std::string, std::string> getGlobalConfiguration() const;

    // Statistics and reporting
    struct AlertStatistics {
        size_t total_alerts_sent{0};
        size_t total_alerts_failed{0};
        double success_rate{0.0};
        std::map<AlertChannel, size_t> channel_counts;
        std::map<AlertPriority, size_t> priority_counts;
        std::chrono::system_clock::time_point last_alert_sent;
        std::chrono::milliseconds average_delivery_time{0};
    };

    AlertStatistics getStatistics() const;
    std::string generateStatisticsReport() const;
    bool exportAlertHistory(const std::string& filename) const;

    // Health check
    bool isHealthy() const;
    std::vector<std::string> getHealthIssues() const;

    // Testing and validation
    bool testChannel(AlertChannel channel);
    bool testAllChannels();
    bool sendTestAlert(AlertPriority priority = AlertPriority::NORMAL);

private:
    // Channel management
    std::map<AlertChannel, ChannelConfiguration> channels_;
    std::map<AlertChannel, std::unique_ptr<AlertRateLimiter>> rate_limiters_;
    mutable std::mutex channels_mutex_;

    // Template management
    std::map<std::string, AlertTemplate> templates_;
    std::string default_template_id_;
    std::map<AlertChannel, std::string> channel_templates_;
    mutable std::mutex templates_mutex_;

    // Alert queue and processing
    std::queue<AlertMessage> pending_alerts_;
    std::map<std::string, std::vector<DeliveryResult>> alert_delivery_results_;
    std::atomic<bool> processing_active_{false};
    std::unique_ptr<std::thread> processing_thread_;
    std::condition_variable alert_queue_cv_;
    mutable std::mutex alert_queue_mutex_;

    // Alert history
    std::vector<AlertMessage> alert_history_;
    size_t max_history_size_{10000};
    mutable std::mutex history_mutex_;

    // Global configuration
    std::map<std::string, std::string> global_config_;
    mutable std::mutex config_mutex_;

    // Monitoring connections
    std::weak_ptr<puzzle71::regression::PerformanceRegressionDetector> regression_detector_;
    std::weak_ptr<puzzle71::monitoring::RealTimeMonitor> monitor_;

    // Private methods
    void processingLoop();
    std::vector<DeliveryResult> processAlert(const AlertMessage& alert);
    DeliveryResult sendToChannel(
        const AlertMessage& alert,
        AlertChannel channel,
        const ChannelConfiguration& config,
        const AlertTemplate& template_
    );

    // Channel senders
    std::unique_ptr<EmailSender> createEmailSender(const ChannelConfiguration& config);
    std::unique_ptr<SlackSender> createSlackSender(const ChannelConfiguration& config);
    std::unique_ptr<DiscordSender> createDiscordSender(const ChannelConfiguration& config);
    std::unique_ptr<SMSSender> createSMSSender(const ChannelConfiguration& config);
    std::unique_ptr<WebhookSender> createWebhookSender(const ChannelConfiguration& config);
    std::unique_ptr<BrowserNotificationSender> createBrowserNotificationSender(const ChannelConfiguration& config);

    // Template processing
    std::string getTemplateForChannel(AlertChannel channel) const;
    AlertTemplate getTemplateForAlert(const AlertMessage& alert) const;

    // Alert tracking
    void recordAlertInHistory(const AlertMessage& alert);
    void recordDeliveryResult(const std::string& alert_id, AlertChannel channel, const DeliveryResult& result);

    // Utility methods
    std::string generateAlertId() const;
    std::chrono::system_clock::time_point getCurrentTime() const;
    std::string formatTimestamp(const std::chrono::system_clock::time_point& timestamp) const;
    std::string escapeJsonString(const std::string& str) const;

    // Configuration defaults
    ChannelConfiguration createDefaultChannelConfig(AlertChannel channel) const;
    AlertTemplate createDefaultTemplate() const;
    void setupDefaultConfiguration();
};

/**
 * @brief Alert manager factory
 */
class AlertManagerFactory {
public:
    /**
     * @brief Create alert manager with default configuration
     */
    static std::unique_ptr<PerformanceAlertManager> create();

    /**
     * @brief Create alert manager for production
     */
    static std::unique_ptr<PerformanceAlertManager> createProductionManager();

    /**
     * @brief Create alert manager for development
     */
    static std::unique_ptr<PerformanceAlertManager> createDevelopmentManager();

    /**
     * @brief Create pre-configured manager with common channels
     */
    static std::unique_ptr<PerformanceAlertManager> createWithEmailAndSlack(
        const std::string& email_smtp_server,
        const std::string& email_username,
        const std::string& email_password,
        const std::string& slack_webhook = ""
    );

    /**
     * @brief Create channel configuration templates
     */
    static ChannelConfiguration createEmailConfig(
        const std::string& smtp_server,
        const std::string& username,
        const std::string& password,
        const std::vector<std::string>& recipients
    );

    static ChannelConfiguration createSlackConfig(const std::string& webhook_url);
    static ChannelConfiguration createDiscordConfig(const std::string& webhook_url);
    static ChannelConfiguration createSMSConfig(
        const std::string& api_key,
        const std::string& from_number,
        const std::vector<std::string>& to_numbers
    );
    static ChannelConfiguration createWebhookConfig(const std::string& webhook_url);
    static ChannelConfiguration createBrowserNotificationConfig();

    /**
     * @brief Create alert template templates
     */
    static AlertTemplate createPerformanceAlertTemplate();
    static AlertTemplate createRegressionAlertTemplate();
    static AlertTemplate createOptimizationAlertTemplate();
    static AlertTemplate createSystemHealthAlertTemplate();
};

/**
 * @brief Alert utilities
 */
namespace alert_utils {

/**
 * @brief Alert message formatting utilities
 */
std::string formatAlertTitle(const AlertMessage& alert);
std::string formatAlertMessage(const AlertMessage& alert);
std::string formatAlertMetadata(const AlertMessage& alert);

/**
 * @brief Priority color coding for different channels
 */
std::string getPriorityColor(AlertPriority priority);
std::string getPriorityEmoji(AlertPriority priority);

/**
 * @brief Channel-specific formatting
 */
std::string formatForEmail(const AlertMessage& alert, const AlertTemplate& template_);
std::string formatForSlack(const AlertMessage& alert, const AlertTemplate& template_);
std::string formatForDiscord(const AlertMessage& alert, const AlertTemplate& template_);
std::string formatForSMS(const AlertMessage& alert, const AlertTemplate& template_);
std::string formatForWebhook(const AlertMessage& alert, const AlertTemplate& template_);
std::string formatForBrowserNotification(const AlertMessage& alert);

/**
 * @brief Alert validation utilities
 */
bool validateAlertMessage(const AlertMessage& alert);
bool validateChannelConfiguration(const ChannelConfiguration& config);
bool validateAlertTemplate(const AlertTemplate& template_);

/**
 * @brief HTML generation for email alerts
 */
std::string generateAlertEmailHTML(const AlertMessage& alert);
std::string generateAlertEmailText(const AlertMessage& alert);

/**
 * @brief Markdown generation for chat platforms
 */
std::string generateAlertMarkdown(const AlertMessage& alert);

/**
 * @brief URL encoding utilities
 */
std::string urlEncode(const std::string& str);
std::string urlDecode(const std::string& str);

} // namespace alert_utils

} // namespace puzzle71::alerting
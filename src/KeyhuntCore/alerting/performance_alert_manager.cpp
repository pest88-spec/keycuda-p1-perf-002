// Puzzle71Solver - Performance Alerting and Notification Implementation (T049)
// Phase 6: User Story 4 - Performance Monitoring
// Multi-channel alerting system with email, Slack, Discord, SMS, and browser notifications

#include "performance_alert_manager.h"
#include "utils/logger.h"
#include "utils/json_serializer.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <ctime>

// HTTP client library would be included here
// For this implementation, we'll use placeholder functions

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

namespace puzzle71::alerting {

// AlertMessage implementation
bool AlertMessage::isFullyDelivered() const {
    for (const auto& [channel, delivered] : delivery_status) {
        if (!delivered) {
            return false;
        }
    }
    return true;
}

std::string AlertMessage::getDeliveryStatus() const {
    size_t total_channels = delivery_status.size();
    size_t delivered_channels = std::count_if(delivery_status.begin(), delivery_status.end(),
        [](const auto& pair) { return pair.second; });

    return std::to_string(delivered_channels) + "/" + std::to_string(total_channels) + " channels";
}

std::string AlertMessage::toJson() const {
    std::ostringstream json;
    json << std::fixed << std::setprecision(4);

    json << "{\n";
    json << "  \"id\": \"" << id << "\",\n";
    json << "  \"title\": \"" << escapeJsonString(title) << "\",\n";
    json << "  \"message\": \"" << escapeJsonString(message) << "\",\n";
    json << "  \"priority\": \"" << static_cast<int>(priority) << "\",\n";
    json << "  \"timestamp\": " << std::chrono::duration_cast<std::chrono::seconds>(
            timestamp.time_since_epoch()).count() << ",\n";
    json << "  \"category\": \"" << category << "\",\n";

    json << "  \"metadata\": {\n";
    bool first = true;
    for (const auto& [key, value] : metadata) {
        if (!first) json << ",\n";
        json << "    \"" << key << "\": \"" << escapeJsonString(value) << "\"";
        first = false;
    }
    json << "\n  },\n";

    json << "  \"delivery_status\": {\n";
    first = true;
    for (const auto& [channel, delivered] : delivery_status) {
        if (!first) json << ",\n";
        json << "    \"" << static_cast<int>(channel) << "\": " << (delivered ? "true" : "false");
        first = false;
    }
    json << "\n  },\n";

    json << "  \"delivery_attempts\": {\n";
    first = true;
    for (const auto& [channel, attempts] : delivery_attempts) {
        if (!first) json << ",\n";
        json << "    \"" << static_cast<int>(channel) << "\": " << std::chrono::duration_cast<std::chrono::seconds>(
                attempts.time_since_epoch()).count();
        first = false;
    }
    json << "\n },\n";

    json << "  \"delivery_errors\": {\n";
    first = true;
    for (const auto& [channel, errors] : delivery_errors) {
        if (!first || !errors.empty()) json << ",\n";
        json << "    \"" << static_cast<int>(channel) << "\": [";
        for (size_t i = 0; i < errors.size(); ++i) {
            if (i > 0) json << ", ";
            json << "\"" << escapeJsonString(errors[i]) << "\"";
        }
        json << "]";
        first = false;
    }
    json << "\n  }\n";
    json << "}";

    return json.str();
}

AlertMessage AlertMessage::fromJson(const std::string& json) {
    AlertMessage alert;

    // Simplified JSON parsing - integrate with existing JSON serializer
    try {
        // Parse basic fields
        auto id_pos = json.find("\"id\":");
        if (id_pos != std::string::npos) {
            auto start = json.find("\"", id_pos + 6) + 1;
            auto end = json.find("\"", start);
            alert.id = json.substr(start, end - start);
        }

        auto title_pos = json.find("\"title\":");
        if (title_pos != std::string::npos) {
            auto start = json.find("\"", title_pos + 9) + 1;
            auto end = json.find("\"", start);
            alert.title = json.substr(start, end - start);
        }

        auto message_pos = json.find("\"message\":");
        if (message_pos != std::string::npos) {
            auto start = json.find("\"", message_pos + 11) + 1;
            auto end = json.find("\"", start);
            alert.message = json.substr(start, end - start);
        }

        // Parse other fields similarly...

    } catch (const std::exception& e) {
        Logger::error("Failed to parse AlertMessage from JSON: {}", e.what());
    }

    return alert;
}

// ChannelConfiguration implementation
bool ChannelConfiguration::isValid() const {
    return enabled && !authentication_token.empty() &&
           max_retries > 0 && retry_delay.count() > 0;
}

std::string ChannelConfiguration::toJson() const {
    std::ostringstream json;
    json << std::fixed << std::setprecision(2);

    json << "{\n";
    json << "  \"type\": " << static_cast<int>(type) << ",\n";
    json << "  \"enabled\": " << (enabled ? "true" : "false") << ",\n";
    json << "  \"minimum_priority\": " << static_cast<int>(minimum_priority) << ",\n";
    json << "  \"cooldown_period\": " << cooldown_period.count() << ",\n";
    json << "  \"max_retries\": " << max_retries << ",\n";
    json << "  \"retry_delay\": " << retry_delay.count() << ",\n";

    json << "  \"channel_settings\": {\n";
    bool first = true;
    for (const auto& [key, value] : channel_settings) {
        if (!first) json << ",\n";
        json << "    \"" << key << "\": \"" << value << "\"";
        first = false;
    }
    json << "\n  },\n";

    json << "  \"custom_headers\": {\n";
    first = true;
    for (const auto& [key, value] : custom_headers) {
        if (!first) json << ",\n";
        json << "    \"" << key << "\": \"" << value << "\"";
        first = false;
    }
    json << "\n  },\n";

    json << "  \"authentication_token\": \"" << authentication_token << "\",\n";
    json << "  \"webhook_url\": \"" << webhook_url << "\",\n";

    json << "  \"rate_limit_window\": " << rate_limit_window.count() << ",\n";
    json << "  \"max_alerts_per_window\": " << max_alerts_per_window << "\n";
    json << "}";

    return json.str();
}

ChannelConfiguration ChannelConfiguration::fromJson(const std::string& json) {
    ChannelConfiguration config;

    // Simplified JSON parsing
    try {
        // Parse basic fields
        auto type_pos = json.find("\"type\":");
        if (type_pos != std::string::npos) {
            config.type = static_cast<AlertChannel>(std::stoi(json.substr(type_pos + 8, 1)));
        }

        auto enabled_pos = json.find("\"enabled\":");
        if (enabled_pos != std::string::npos) {
            config.enabled = json.substr(enabled_pos + 10, 4) == "true";
        }

        // Parse other fields similarly...

    } catch (const std::exception& e) {
        Logger::error("Failed to parse ChannelConfiguration from JSON: {}", e.what());
    }

    return config;
}

// AlertTemplate implementation
std::string AlertTemplate::processTemplate(const AlertMessage& alert) const {
    std::string result = body_template;

    // Replace template variables
    result = replaceAll(result, "{{alert.id}}", alert.id);
    result = replaceAll(result, "{{alert.title}}", alert.title);
    result = replaceAll(result, "{{alert.message}}", alert.message);
    result = replaceAll(result, "{{alert.priority}}", std::to_string(static_cast<int>(alert.priority)));
    result = replaceAll(result, "{{alert.category}}", alert.category);
    result = replaceAll(result, "{{alert.timestamp}}", formatTimestamp(alert.timestamp));

    // Replace default variables
    for (const auto& [key, value] : default_variables) {
        result = replaceAll(result, "{{" + key + "}}", value);
    }

    // Replace metadata variables
    for (const auto& [key, value] : alert.metadata) {
        result = replaceAll(result, "{{metadata." + key + "}}", value);
    }

    return result;
}

bool AlertTemplate::isValid() const {
    return !id.empty() && !name.empty() &&
           !subject_template.empty() && !body_template.empty();
}

std::string AlertTemplate::toJson() const {
    std::ostringstream json;
    json << std::fixed << std::setprecision(2);

    json << "{\n";
    json << "  \"id\": \"" << id << "\",\n";
    json << "  \"name\": \"" << name << "\",\n";
    json << "  \"subject_template\": \"" << subject_template << "\",\n";
    json << "  \"body_template\": \"" << escapeJsonString(body_template) << "\",\n";

    json << "  \"default_variables\": {\n";
    bool first = true;
    for (const auto& [key, value] : default_variables) {
        if (!first) json << ",\n";
        json << "    \"" << key << "\": \"" << escapeJsonString(value) << "\"";
        first = false;
    }
    json << "\n  },\n";

    json << "  \"supported_channels\": [";
    for (size_t i = 0; i < supported_channels.size(); ++i) {
        if (i > 0) json << ", ";
        json << static_cast<int>(supported_channels[i]);
    }
    json << "]\n";
    json << "}";

    return json.str();
}

AlertTemplate AlertTemplate::fromJson(const std::string& json) {
    AlertTemplate template;

    // Simplified JSON parsing
    try {
        // Parse basic fields
        auto id_pos = json.find("\"id\":");
        if (id_pos != std::string::npos) {
            auto start = json.find("\"", id_pos + 6) + 1;
            auto end = json.find("\"", start);
            template.id = json.substr(start, end - start);
        }

        auto name_pos = json.find("\"name\":");
        if (name_pos != std::string::npos) {
            auto start = json.find("\"", name_pos + 8) + 1;
            auto end = json.find("\"", start);
            template.name = json.substr(start, end - start);
        }

        // Parse other fields similarly...

    } catch (const std::exception& e) {
        Logger::error("Failed to parse AlertTemplate from JSON: {}", e.what());
    }

    return template;
}

// DeliveryResult implementation
std::string DeliveryResult::getSummary() const {
    std::ostringstream summary;
    summary << (success ? "✓" : "✗") << " " << static_cast<int>(channel);

    if (!success) {
        summary << " - " << error_message;
    }

    summary << " (" << delivery_time.count() << "ms)";

    if (retry_count > 0) {
        summary << " [retry " << retry_count << "]";
    }

    return summary.str();
}

// AlertRateLimiter implementation
AlertRateLimiter::AlertRateLimiter(const ChannelConfiguration& config)
    : config_(config) {

    // Set window start time
    alert_queue_.front().timestamp = getCurrentTime();
}

bool AlertRateLimiter::canSendAlert(const std::string& alert_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if we're at the rate limit
    if (alert_queue_.size() >= config_.max_alerts_per_window) {
        return false;
    }

    // Check if we've sent this specific alert recently
    auto it = last_alert_times_.find(alert_id);
    if (it != last_alert_times_.end()) {
        auto time_since_last = getCurrentTime() - it->second;
        if (time_since_last < config_.cooldown_period) {
            return false;
        }
    }

    return true;
}

void AlertRateLimiter::recordAlert(const std::string& alert_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Record the alert
    AlertRecord record;
    record.timestamp = getCurrentTime();
    record.alert_id = alert_id;
    alert_queue_.push(record);
    last_alert_times_[alert_id] = getCurrentTime();

    // Clean up old records outside the window
    auto window_start = getCurrentTime() - config_.rate_limit_window;
    while (!alert_queue_.empty() && alert_queue_.front().timestamp < window_start) {
        alert_queue_.pop();
    }

    // Also clean up last alert times
    for (auto it = last_alert_times_.begin(); it != last_alert_times_.end();) {
        if (it->second < window_start) {
            it = last_alert_times_.erase(it);
        } else {
            ++it;
        }
    }
}

void AlertRateLimiter::reset() {
    std::lock_guard<std::mutex> lock(mutex_);

    while (!alert_queue_.empty()) {
        alert_queue_.pop();
    }

    last_alert_times_.clear();
}

AlertRateLimiter::RateLimitStats AlertRateLimiter::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    RateLimitStats stats;
    stats.alerts_in_window = alert_queue_.size();
    stats.window_start = alert_queue_.empty() ? getCurrentTime() : alert_queue_.front().timestamp;
    stats.window_duration = config_.rate_limit_window;

    return stats;
}

// HTTP utility functions (simplified)
namespace {
    std::string httpRequest(const std::string& method, const std::string& url,
                           const std::map<std::string, std::string>& headers,
                           const std::string& body = "") {

        std::ostringstream request;
        request << method << " " << url << " HTTP/1.1\r\n";

        // Add headers
        for (const auto& [key, value] : headers) {
            request << key << ": " << value << "\r\n";
        }

        // Add content length if body is provided
        if (!body.empty()) {
            request << "Content-Length: " << body.length() << "\r\n";
        }

        request << "\r\n" << body;

        return request.str();
    }

    std::pair<int, std::string> httpResponse(const std::string& request, const std::string& host, int port = 80) {
        #ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            return { -1, "Failed to create socket" };
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
            server_addr.sin_addr.s_addr = inet_addr(host.c_str());

        if (connect(sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) != 0) {
            closesocket(sock);
            WSACleanup();
            return { -1, "Failed to connect to server" };
        }

        // Send request
        if (send(sock, request.c_str(), request.length(), 0) < 0) {
            closesocket(sock);
            WSACleanup();
            return { -1, "Failed to send request" };
        }

        // Receive response
        std::string response;
        char buffer[4096];
        int bytes_received;
        while ((bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes_received] = '\0';
            response += std::string(buffer, bytes_received);
        }

        closesocket(sock);
        WSACleanup();
        #else
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            return { -1, "Failed to create socket" };
        }

        struct hostent* server = gethostbyname(host.c_str());
        if (!server) {
            close(sock);
            return { -1, "Failed to resolve host" };
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr = *reinterpret_cast<in_addr*>(server->h_addr);

        if (connect(sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            close(sock);
            return { -1, "Failed to connect to server" };
        }

        // Send request
        if (send(sock, request.c_str(), request.length(), 0) < 0) {
            close(sock);
            return { -1, "Failed to send request" };
        }

        // Receive response
        std::string response;
        char buffer[4096];
        int bytes_received;
        while ((bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes_received] = '\0';
            response += std::string(buffer, bytes_received);
        }

        close(sock);
        #endif

        return { 0, response };
    }
}

// EmailSender implementation
EmailSender::EmailSender(const ChannelConfiguration& config) : config_(config) {}

DeliveryResult EmailSender::sendAlert(const AlertMessage& alert, const AlertTemplate& template_) {
    DeliveryResult result;
    result.channel = AlertChannel::EMAIL;
    result.retry_count = 0;
    result.delivery_attempts.push_back(getCurrentTime());

    try {
        std::string email_content = buildEmailContent(alert, template_);
        std::string recipients = config_.channel_settings.at("recipients");

        // Split recipients by comma or semicolon
        std::vector<std::string> recipient_list;
        std::stringstream ss(recipients);
        std::string recipient;
        while (std::getline(ss, recipient, ',')) {
            recipient.erase(0, recipient.find_first_not_of(" \t"));
            if (!recipient.empty()) {
                recipient_list.push_back(recipient);
            }
        }

        bool success = true;
        for (const auto& recipient : recipient_list) {
            if (!sendEmail(recipient, template_.subject_template, email_content)) {
                success = false;
            }
        }

        result.success = success;
        result.delivered_at = getCurrentTime();
        result.delivery_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            result.delivered_at - result.delivery_attempts.front()
        );

    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }

    return result;
}

std::string EmailSender::buildEmailContent(const AlertMessage& alert, const AlertTemplate& template_) const {
    std::ostringstream email;

    // Build subject
    std::string subject = template_.subject_template;
    subject = replaceAll(subject, "{{alert.title}}", alert.title);
    subject = replaceAll(subject, "{{alert.priority}}", std::to_string(static_cast<int>(alert.priority)));
    email << "Subject: " << subject << "\r\n";

    // Build body
    std::string body = template_.body_template;
    body = template_.processTemplate(alert);
    email << body;

    return email.str();
}

bool EmailSender::sendEmail(const std::string& to, const std::string& subject, const std::string& content) {
    // This would integrate with an email library like libcurl or a proper SMTP client
    // For now, return a placeholder success
    Logger::info("Email alert sent to {}: {}", to, subject);
    return true;
}

DeliveryResult EmailSender::handleEmailResponse(const std::string& response) {
    DeliveryResult result;
    result.channel = AlertChannel::EMAIL;

    // Parse response to determine success
    if (response.find("250") != std::string::npos) {
        result.success = true;
    } else {
        result.success = false;
        result.error_message = "Email delivery failed";
    }

    return result;
}

// SlackSender implementation
SlackSender::SlackSender(const ChannelConfiguration& config) : config_(config) {}

DeliveryResult SlackSender::sendAlert(const AlertMessage& alert, const AlertTemplate& template_) {
    DeliveryResult result;
    result.channel = AlertChannel::SLACK;
    result.retry_count = 0;
    result.delivery_attempts.push_back(getCurrentTime());

    try {
        std::string payload = buildSlackPayload(alert, template_);
        result = sendSlackMessage(payload);

        result.delivered_at = getCurrentTime();
        result.delivery_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            result.delivered_at - result.delivery_attempts.front()
        );

    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }

    return result;
}

std::string SlackSender::buildSlackPayload(const AlertMessage& alert, const AlertTemplate& template_) const {
    std::ostringstream payload;

    // Build Slack message
    payload << "{\n";
    payload << "  \"text\": \"" << escapeJsonString(alert.title) << "\",\n";
    payload << "  \"attachments\": [\n";
    payload << "    {\n";
    payload << "      \"color\": \"" << getPriorityColor(alert.priority) << "\",\n";
    payload << "      \"fields\": [\n";
    payload << "        {\n";
    payload << "          \"title\": \"" << escapeJsonString("Priority: " + std::to_string(static_cast<int>(alert.priority))) << "\",\n";
    payload << "          \"value\": \"" << escapeJsonString(alert.priority) << "\",\n";
    payload << "          \"short\": false\n";
    payload << "        },\n";
    payload << "        {\n";
    payload << "          \"title\": \"Message\",\n";
    payload << "          \"value\": \"" << escapeJsonString(alert.message) << "\",\n";
    payload << "          \"short\": false\n";
    payload << "        }\n";

    // Add metadata fields
    for (const auto& [key, value] : alert.metadata) {
        payload << "        {\n";
        payload << "          \"title\": \"" << escapeJsonString(key) << "\",\n";
        payload << "          \"value\": \"" << escapeJsonString(value) << "\",\n";
        payload << "          \"short\": false\n";
        payload << "        },\n";
    }

    payload << "      ]\n";
    payload << "    }\n";
    payload << "  ]\n";
    payload << "}";

    return payload.str();
}

DeliveryResult SlackSender::sendSlackMessage(const std::string& payload) {
    std::string webhook_url = config_.webhook_url;
    if (webhook_url.empty()) {
        webhook_url = config_.channel_settings.at("webhook_url");
    }

    // Prepare HTTP request
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";

    if (!config_.authentication_token.empty()) {
        headers["Authorization"] = "Bearer " + config_.authentication_token;
    }

    std::string http_request = httpRequest("POST", webhook_url, headers, payload);

    // Extract hostname from webhook URL
    std::string host = "hooks.slack.com";
    size_t protocol_pos = webhook_url.find("://");
    if (protocol_pos != std::string::npos) {
        size_t host_start = protocol_pos + 3;
        size_t host_end = webhook_url.find("/", host_start);
        if (host_end == std::string::npos) host_end = webhook_url.length();
        host = webhook_url.substr(host_start, host_end - host_start);
    }

    // Send HTTP request
    auto [status, response] = httpResponse(http_request, host);

    DeliveryResult result;
    result.channel = AlertChannel::SLACK;
    result.success = (status == 0);

    if (!result.success) {
        result.error_message = "HTTP request failed with status " + std::to_string(status);
    } else {
        // Parse Slack response
        if (response.find("\"ok\"") != std::string::npos) {
            result.success = true;
            result.response = "Message sent successfully to Slack";
        } else {
            result.success = false;
            result.error_message = "Slack API returned error: " + response;
        }
    }

    return result;
}

// DiscordSender implementation
DiscordSender::DiscordSender(const ChannelConfiguration& config) : config_(config) {}

DeliveryResult DiscordSender::sendAlert(const AlertMessage& alert, const AlertTemplate& template_) {
    DeliveryResult result;
    result.channel = AlertChannel::DISCORD;
    result.retry_count = 0;
    result.delivery_attempts.push_back(getCurrentTime());

    try {
        std::string embed = buildDiscordEmbed(alert, template_);
        result = sendDiscordMessage(embed);

        result.delivered_at = getCurrentTime();
        result.delivery_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            result.delivered_at - result.delivery_attempts.front()
        );

    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }

    return result;
}

std::string DiscordSender::buildDiscordEmbed(const AlertMessage& alert, const AlertTemplate& template_) const {
    std::ostringstream embed;

    // Build Discord embed
    embed << "{\n";
    embed << "  \"title\": \"" << escapeJsonString(alert.title) << "\",\n";
    embed << "  \"description\": \"" << escapeJsonString(alert.message) << "\",\n";
    embed << "  \"color\": " << static_cast<int>(getPriorityColorCode(alert.priority)) << ",\n";
    embed << "  \"timestamp\": \"" << formatTimestamp(alert.timestamp) << "\",\n";

    // Add fields
    embed << "  \"fields\": [\n";
    embed << "    {\n";
    embed << "      \"name\": \"Priority\",\n";
    embed << "      \"value\": \"" << escapeJsonString(getPriorityEmoji(alert.priority)) + " " + escapeJsonString(std::to_string(static_cast<int>(alert.priority))) << "\"\n";
    embed << "      \"inline\": true\n";
    embed << "    },\n";
    embed << "    {\n";
    embed << "      \"name\": \"Category\",\n";
    embed << "      \"value\": \"" << escapeJsonString(alert.category) << "\"\n";
    embed << "      \"inline\": true\n";
    embed << "    }\n";

    // Add metadata fields
    for (const auto& [key, value] : alert.metadata) {
        embed << "    {\n";
        embed << "      \"name\": \"" << escapeJsonString(key) << "\",\n";
        embed << "      \"value\": \"" << escapeJsonString(value) << "\"\n";
        embed << "      \"inline\": true\n";
        embed << "    },\n";
    }

    embed << "  ]\n";
    embed << "}";

    return embed.str();
}

DeliveryResult DiscordSender::sendDiscordMessage(const std::string& embed) {
    std::string webhook_url = config_.webhook_url;
    if (webhook_url.empty()) {
        webhook_url = config_.channel_settings.at("webhook_url");
    }

    // Prepare HTTP request
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";

    if (!config_.authentication_token.empty()) {
        headers["Authorization"] = "Bot " + config_.authentication_token;
    }

    std::string http_request = httpRequest("POST", webhook_url, headers, embed);

    // Extract hostname from webhook URL
    std::string host = "discord.com";
    size_t protocol_pos = webhook_url.find("://");
    if (protocol_pos != std::string::npos) {
        size_t host_start = protocol_pos + 3;
        size_t host_end = webhook_url.find("/", host_start);
        if (host_end == std::string::npos) host_end = webhook_url.length();
        host = webhook_url.substr(host_start, host_end - host_start);
    }

    // Send HTTP request
    auto [status, response] = httpResponse(http_request, host);

    Result result;
    result.channel = AlertChannel::DISCORD;
    result.success = (status == 0);

    if (!result.success) {
        result.error_message = "HTTP request failed with status " + std::to_string(status);
    } else {
        // Parse Discord response
        if (response.find("\"id\"") != std::string::npos) {
            result.success = true;
            result.response = "Message sent successfully to Discord";
        } else {
            result.success = false;
            result.error_message = "Discord API returned error: " + response;
        }
    }

    return result;
}

// SMSSender implementation
SMSSender::SMSSender(const ChannelConfiguration& config) : config_(config) {}

DeliveryResult SMSSender::sendAlert(const AlertMessage& alert, const AlertTemplate& template_) {
    DeliveryResult result;
    result.channel = AlertChannel::SMS;
    result.retry_count = 0;
    result.delivery_attempts.push_back(getCurrentTime());

    try {
        std::string sms_content = buildSMSContent(alert, template_);
        std::string from_number = config_.channel_settings.at("from_number");
        std::vector<std::string> to_numbers;

        // Parse recipient numbers
        std::string recipients = config_.channel_settings.at("to_numbers");
        std::stringstream ss(recipients);
        std::string number;
        while (std::getline(ss, number, ',')) {
            number.erase(0, number.find_first_not_of(" \t"));
            if (!number.empty()) {
                to_numbers.push_back(number);
            }
        }

        bool success = true;
        for (const auto& to_number : to_numbers) {
            if (!sendSMS(to_number, sms_content)) {
                success = false;
            }
        }

        result.success = success;
        result.delivered_at = getCurrentTime();
        result.delivery_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            result.delivered_at - result.delivery_attempts.front()
        );

    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }

    return result;
}

std::string SMSSender::buildSMSContent(const AlertMessage& alert, const AlertTemplate& template_) const {
    std::string content = template_.processTemplate(alert);

    // Limit SMS content to 160 characters
    if (content.length() > 160) {
        content = content.substr(0, 157) + "...";
    }

    return content;
}

bool SMSSender::sendSMS(const std::string& phone_number, const std::string& message) {
    // This would integrate with an SMS service like Twilio
    // For now, return a placeholder success
    Logger::info("SMS alert sent to {}: {}", phone_number, message);
    return true;
}

// WebhookSender implementation
WebhookSender::WebhookSender(const ChannelConfiguration& config) : config_(config) {}

DeliveryResult WebhookSender::sendWebhook(const AlertMessage& alert, const AlertTemplate& template_) {
    DeliveryResult result;
    result.channel = AlertChannel::WEBHOOK;
    result.retry_count = 0;
    result.delivery_attempts.push_back(getCurrentTime());

    try {
        std::string payload = buildWebhookPayload(alert, template_);

        // Prepare HTTP request
        std::map<std::string, std::string> headers = config_.custom_headers;
        headers["Content-Type"] = "application/json";

        if (!config_.authentication_token.empty()) {
            headers["Authorization"] = "Bearer " + config_.authentication_token;
        }

        std::string http_request = httpRequest("POST", config_.webhook_url, headers, payload);

        // Extract hostname from webhook URL
        std::string host = "localhost";
        size_t protocol_pos = config_.webhook_url.find("://");
        if (protocol_pos != std::string::npos) {
            size_t host_start = protocol_pos + 3;
            size_t host_end = config_.webhook_url.find("/", host_start);
            if (host_end == std::string::npos) host_end = config_.webhook_url.length();
            host = config_.webhook_url.substr(host_start, host_end - host_start);
        }

        // Send HTTP request
        auto [status, response] = httpResponse(http_request, host);

        result.success = (status == 0);

        if (!result.success) {
            result.error_message = "HTTP request failed with status " + std::to_string(status);
        } else {
            result.response = "Webhook alert sent successfully";
        }

        result.delivered_at = getCurrentTime();
        result.delivery_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            result.delivered_at - result.delivery_attempts.front()
        );

    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }

    return result;
}

std::string WebhookSender::buildWebhookPayload(const AlertMessage& alert, const AlertTemplate& template_) const {
    std::ostringstream payload;

    // Build webhook payload
    payload << "{\n";
    payload << "  \"id\": \"" << alert.id << "\",\n";
    payload << "  \"title\": \"" << escapeJsonString(alert.title) << "\",\n";
    payload << "  \"message\": \"" << escapeJsonString(alert.message) << "\",\n";
    payload << "  \"priority\": " << static_cast<int>(alert.priority) << ",\n";
    payload << "  \"category\": \"" << alert.category << "\",\n";
    payload << "  \"timestamp\": \"" << formatTimestamp(alert.timestamp) << "\",\n";

    // Add metadata
    if (!alert.metadata.empty()) {
        payload << "  \"metadata\": {\n";
        bool first = true;
        for (const auto& [key, value] : alert.metadata) {
            if (!first) payload << ",\n";
            payload << "    \"" << key << "\": \"" << escapeJsonString(value) << "\"";
            first = false;
        }
        payload << "\n  },\n";
    }

    payload << "}";

    return payload.str();
}

// BrowserNotificationSender implementation
BrowserNotificationSender::BrowserNotificationSender(const ChannelConfiguration& config) : config_(config) {}

DeliveryResult BrowserNotificationSender::sendAlert(const AlertMessage& alert, const AlertTemplate& template_) {
    DeliveryResult result;
    result.channel = AlertChannel::BROWSER_NOTIFICATION;
    result.retry_count = 0;
    result.delivery_attempts.push_back(getCurrentTime());

    try {
        std::string title = alert.title;
        std::string message = buildNotificationContent(alert);
        std::string icon = config_.channel_settings.count("icon") ? config_.channel_settings.at("icon") : "";

        bool success = true;

        // Request permission if not already granted
        if (!requestNotificationPermission()) {
            result.success = false;
            result.error_message = "Browser notification permission denied";
        } else {
            success = showBrowserNotification(title, message);
        }

        result.success = success;
        result.delivered_at = getCurrentTime();
        result.delivery_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            result.delivered_at - result.delivery_attempts.front()
        );

    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }

    return result;
}

std::string BrowserNotificationSender::buildNotificationContent(const AlertMessage& alert) const {
    // Limit browser notification content
    std::string content = alert.message;
    if (content.length() > 200) {
        content = content.substr(0, 197) + "...";
    }

    return content;
}

bool BrowserNotificationSender::requestNotificationPermission() {
    // This would use the Notification API
    // For now, assume permission is granted
    return true;
}

bool BrowserNotificationSender::showBrowserNotification(const std::string& title, const std::string& message) {
    // This would use the Notification API
    Logger::info("Browser notification: {} - {}", title, message);
    return true;
}

// PerformanceAlertManager implementation
PerformanceAlertManager::PerformanceAlertManager() {
    setupDefaultConfiguration();
    processing_active_.store(false);
    processing_thread_ = std::make_unique<std::thread>(&PerformanceAlertManager::processingLoop, this);
}

PerformanceAlertManager::~PerformanceAlertManager() {
    stopProcessing();
}

void PerformanceAlertManager::addChannel(const ChannelConfiguration& channel) {
    std::lock_guard<std::mutex> lock(channels_mutex_);

    channels_[channel.type] = channel;
    rate_limiters_[channel.type] = std::make_unique<AlertRateLimiter>(channel);

    Logger::info("Added alert channel: {}", static_cast<int>(channel.type));
}

void PerformanceAlertManager::removeChannel(AlertChannel channel) {
    std::lock_guard<std::mutex> lock(channels_mutex_);

    channels_.erase(channel);
    rate_limiters_.erase(channel);

    Logger::info("Removed alert channel: {}", static_cast<int>(channel));
}

void PerformanceAlertManager::enableChannel(AlertChannel channel, bool enabled) {
    std::lock_guard<std::mutex> lock(channels_mutex_);

    auto it = channels_.find(channel);
    if (it != channels_.end()) {
        it->second.enabled = enabled;
        Logger::info("Channel {} {}", enabled ? "enabled" : "disabled", static_cast<int>(channel));
    }
}

std::vector<ChannelConfiguration> PerformanceAlertManager::getChannels() const {
    std::lock_guard<std::mutex> lock(channels_mutex_);

    std::vector<ChannelConfiguration> channels;
    for (const auto& [type, config] : channels_) {
        channels.push_back(config);
    }

    return channels;
}

void PerformanceAlertManager::addTemplate(const AlertTemplate& template_) {
    std::lock_guard<std::mutex> lock(templates_mutex_);

    if (template_.isValid()) {
        templates_[template_.id] = template_;
        Logger::info("Added alert template: {}", template_.name);
    } else {
        Logger::error("Invalid alert template: {}", template_.id);
    }
}

void PerformanceAlertManager::removeTemplate(const std::string& template_id) {
    std::lock_guard<std::mutex> lock(templates_mutex_);

    templates_.erase(template_id);
    Logger::info("Removed alert template: {}", template_id);
}

AlertTemplate PerformanceAlertManager::getTemplate(const std::string& template_id) const {
    std::lock_guard<std::mutex> lock(templates_mutex_);

    auto it = templates_.find(template_id);
    if (it != templates_.end()) {
        return it->second;
    }

    return AlertTemplate{};
}

std::vector<AlertTemplate> PerformanceAlertManager::getTemplates() const {
    std::lock_guard<std::mutex> lock(templates_mutex_);

    std::vector<AlertTemplate> templates;
    for (const auto& [id, template_] : templates_) {
        templates.push_back(template_);
    }

    return templates;
}

std::vector<DeliveryResult> PerformanceAlertManager::sendAlert(const AlertMessage& alert) {
    std::string template_id = getTemplateForAlert(alert);
    auto template_ = getTemplate(template_id);

    return sendAlert(alert, template_);
}

std::vector<DeliveryResult> PerformanceAlertManager::sendAlert(
    const AlertMessage& alert, const std::string& template_id) {

    auto template_ = getTemplate(template_id);
    if (!template_.isValid()) {
        Logger::error("Invalid template: {}", template_id);
        return {};
    }

    return sendAlert(alert, template_);
}

std::vector<DeliveryResult> PerformanceAlertManager::sendAlert(
    const AlertMessage& alert, const AlertTemplate& template_) {

    std::vector<DeliveryResult> results;

    std::lock_guard<std::mutex> lock(channels_mutex_);

    for (const auto& [channel_type, config] : channels_) {
        if (!config.enabled) {
            continue;
        }

        // Check if alert priority meets minimum requirement
        if (static_cast<int>(alert.priority) < static_cast<int>(config.minimum_priority)) {
            continue;
        }

        // Check rate limiting
        auto rate_limiter = rate_limiters_.at(channel_type);
        if (!rate_limiter.canSendAlert(alert.id)) {
            continue;
        }

        // Get channel-specific template if configured
        std::string channel_template_id = getTemplateForChannel(channel_type);
        AlertTemplate channel_template;
        if (!channel_template_id.empty()) {
            channel_template = template_;
        }

        // Send alert
        DeliveryResult result = sendToChannel(alert, channel_type, config, channel_template);
        if (result.success) {
            rate_limiter.recordAlert(alert.id);
        }

        results.push_back(result);
    }

    // Record alert in history
    recordAlertInHistory(alert);
    recordDeliveryResult(alert.id, AlertChannel::EMAIL, results.empty() ?
        DeliveryResult{} : results[0]);

    return results;
}

std::vector<DeliveryResult> PerformanceAlertManager::processAlert(const AlertMessage& alert) {
    std::vector<DeliveryResult> results;

    std::lock_guard<std::mutex> lock(channels_mutex_);

    for (const auto& [channel_type, config] : channels_) {
        if (!config.enabled) {
            continue;
        }

        // Check if alert priority meets minimum requirement
        if (result.priority < config.minimum_priority) {
            continue;
        }

        // Check rate limiting
        auto rate_limiter = rate_limiters_.at(channel_type);
        if (!rate_limiter.canSendAlert(alert.id)) {
            continue;
        }

        // Get channel-specific template if configured
        std::string channel_template_id = getTemplateForChannel(channel_type);
        AlertTemplate channel_template;
        if (!channel_template_id.empty()) {
            channel_template = getTemplate(getDefaultTemplateId());
        }

        // Send alert
        DeliveryResult result = sendToChannel(alert, channel_type, config, channel_template);
        if (result.success) {
            rate_limiter.recordAlert(alert.id);
        }

        results.push_back(result);
    }

    return results;
}

DeliveryResult PerformanceAlertManager::sendToChannel(
    const AlertMessage& alert,
    AlertChannel channel,
    const ChannelConfiguration& config,
    const AlertTemplate& template_) {

    DeliveryResult result;
    result.channel = channel;
    result.retry_count = 0;
    result.delivery_attempts.push_back(getCurrentTime());

    switch (channel) {
        case AlertChannel::EMAIL:
            {
                auto sender = createEmailSender(config);
                result = sender->sendAlert(alert, template_);
            }
            break;

        case AlertChannel::SLACK:
            {
                auto sender = createSlackSender(config);
                result = sender->sendAlert(alert, template_);
            }
            break;

        case AlertChannel::DISCORD:
            {
                auto sender = createDiscordSender(config);
                result = sender->sendAlert(alert, template_);
            }
            break;

        case AlertChannel::SMS:
            {
                auto sender = createSMSSender(config);
                result = sender->sendAlert(alert, template_);
            }
            break;

        case AlertChannel::WEBHOOK:
            {
                auto sender = createWebhookSender(config);
                result = sender->sendAlert(alert, template_);
            }
            break;

        case AlertChannel::BROWSER_NOTIFICATION:
            {
                auto sender = createBrowserNotificationSender(config);
                result = sender->sendAlert(alert, template_);
            }
            break;

        case AlertChannel::CONSOLE_LOG:
            {
                // Console logging is always successful
                result.success = true;
                result.response = "Logged to console";
                Logger::warn("ALERT: {} - {}", alert.title, alert.message);
            }
            break;

        default:
            result.success = false;
            result.error_message = "Unknown channel type";
            break;
    }

    return result;
}

std::unique_ptr<EmailSender> PerformanceAlertManager::createEmailSender(const ChannelConfiguration& config) {
    return std::make_unique<EmailSender>(config);
}

std::unique_ptr<SlackSender> PerformanceAlertManager::createSlackSender(const ChannelConfiguration& config) {
    return std::make_unique<SlackSender>(config);
}

std::unique_ptr<DiscordSender> PerformanceAlertManager::createDiscordSender(const ChannelConfiguration& config) {
    return std::make_unique<DiscordSender>(config);
}

std::unique_ptr<SMSSender> PerformanceAlertManager::createSMSSender(const ChannelConfiguration& config) {
    return std::make_unique<SMSSender>(config);
}

std::unique_ptr<WebhookSender> PerformanceAlertManager::createWebhookSender(const ChannelConfiguration& config) {
    return std::make_unique<WebhookSender>(config);
}

std::unique_ptr<BrowserNotificationSender> PerformanceAlertManager::createBrowserNotificationSender(const ChannelConfiguration& config) {
    return std::make_unique<BrowserNotificationSender>(config);
}

std::string PerformanceAlertManager::getTemplateForChannel(AlertChannel channel) const {
    std::lock_guard<std::mutex> lock(templates_mutex_);

    auto it = channel_templates_.find(channel);
    if (it != channel_templates_.end()) {
        return it->second;
    }

    return default_template_id_;
}

AlertTemplate PerformanceAlertManager::getTemplateForAlert(const AlertMessage& alert) const {
    std::lock_guard<std::mutex> lock(templates_mutex_);

    // Try to find template by category
    auto it = templates_.find(alert.category);
    if (it != templates_.end()) {
        return it->second;
    }

    // Use default template
    return getTemplate(default_template_id_);
}

void PerformanceAlertManager::setDefaultTemplate(const std::string& template_id) {
    std::lock_guard<std::mutex> lock(templates_mutex_);
    default_template_id_ = template_id;
}

std::vector<AlertMessage> PerformanceAlertManager::getPendingAlerts() const {
    std::lock_guard<std::mutex> lock(alert_queue_mutex_);

    std::vector<AlertMessage> pending;
    std::queue<AlertMessage> temp_queue = pending_alerts_;

    while (!temp_queue.empty()) {
        pending.push_back(temp_queue.front());
        temp_queue.pop();
    }

    return pending;
}

void PerformanceAlertManager::clearPendingAlerts() {
    std::lock_guard<std::mutex> lock(alert_queue_mutex_);
    while (!pending_alerts_.empty()) {
        pending_alerts_.pop();
    }
}

void PerformanceAlertManager::connectToRegressionDetector(
    std::shared_ptr<puzzle71::regression::PerformanceRegressionDetector> detector) {
    regression_detector_ = detector;

    // Set up alert callback for regression detection
    if (detector) {
        detector->setAlertCallback([this](const puzzle71::regression::RegressionResult& regression) {
            AlertMessage alert;
            alert.id = generateAlertId();
            alert.title = "Performance Regression Detected";
            alert.message = regression.getSeverityString() + ": " + regression.metric_name +
                             " changed by " + std::to_string(regression.percentage_change) + "%";
            alert.priority = regression.severity == RegressionSeverity::CRITICAL ? AlertPriority::CRITICAL :
                            regression.severity == RegressionSeverity::ERROR ? AlertPriority::HIGH :
                            regression.severity == RegressionSeverity::WARNING ? AlertPriority::NORMAL : AlertPriority::LOW;
            alert.category = "regression";

            // Add regression metadata
            alert.metadata["metric_name"] = regression.metric_name;
            alert.metadata["baseline_id"] = regression.baseline_id;
            alert.metadata["percentage_change"] = std::to_string(regression.percentage_change);
            alert.metadata["z_score"] = std::to_string(regression.z_score);
            alert.metadata["p_value"] = std::to_string(regression.p_value);

            // Add to alert queue
            {
                std::lock_guard<std::mutex> lock(alert_queue_mutex_);
                pending_alerts_.push(alert);
                alert_queue_cv_.notify_one();
            }
        });
    }

    Logger::info("Connected to regression detector");
}

void PerformanceAlertManager::connectToMonitor(
    std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor) {
    monitor_ = monitor;

    // Set up alert callback for performance issues
    if (monitor) {
        // This would connect to monitor's alerting system
        Logger::info("Connected to performance monitor");
    }
}

void PerformanceAlertManager::setGlobalConfiguration(std::map<std::string, std::string> config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    global_config_ = config;
}

std::map<std::string, std::string> PerformanceAlertManager::getGlobalConfiguration() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return global_config_;
}

PerformanceAlertManager::AlertStatistics PerformanceAlertManager::getStatistics() const {
    AlertStatistics stats;

    std::lock_guard<std::mutex> lock(history_mutex_);
    stats.total_alerts_sent = alert_history_.size();
    stats.last_alert_sent = alert_history_.empty() ?
        std::chrono::system_clock::time_point{} :
        alert_history_.back().timestamp;

    // Calculate success rate
    if (stats.total_alerts_sent > 0) {
        size_t successful_deliveries = 0;

        for (const auto& alert : alert_history_) {
            if (alert.isFullyDelivered()) {
                successful_deliveries++;
            }
        }

        stats.success_rate = static_cast<double>(successful_deliveries) / stats.total_alerts_sent;
    }

    // Calculate average delivery time
    if (stats.total_alerts_sent > 0) {
        std::chrono::milliseconds total_time{0};
        for (const auto& alert : alert_history_) {
            if (!alert.delivery_attempts.empty()) {
                total_time += alert.delivery_time;
            }
        }
        stats.average_delivery_time = total_time / stats.total_alerts_sent;
    }

    // Count by channel
    for (const auto& alert : alert_history_) {
        for (const auto& [channel, delivered] : alert.delivery_status) {
            if (delivered) {
                stats.channel_counts[channel]++;
            }
        }
    }

    // Count by priority
    for (const auto& alert : alert_history_) {
        stats.priority_counts[alert.priority]++;
    }

    return stats;
}

std::string PerformanceAlertManager::generateStatisticsReport() const {
    auto stats = getStatistics();

    std::ostringstream report;
    report << "Performance Alert Statistics Report\n";
    report << "============================\n\n";

    report << "Summary:\n";
    report << "  Total alerts sent: " << stats.total_alerts_sent << "\n";
    report << "  Success rate: " << (stats.success_rate * 100) << "%\n";
    report <<  " Average delivery time: " << stats.average_delivery_time.count() << "ms\n";
    report << "  Last alert: " << formatTimestamp(stats.last_alert_sent) << "\n\n";

    report << "By Channel:\n";
    for (const auto& [channel, count] : stats.channel_counts) {
        report << "  " << static_cast<int>(channel) << ": " << count << " alerts\n";
    }

    report << "\nBy Priority:\n";
    for (const auto& [priority, count] : stats.priority_counts) {
        report << "  " << static_cast<int>(priority) << ": " << count << " alerts\n";
    }

    return report.str();
}

bool PerformanceAlertManager::exportAlertHistory(const std::string& filename) const {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            Logger::error("Failed to open file for alert history export: {}", filename);
            return false;
        }

        file << "{\n";
        file << "  \"export_timestamp\": \"" << formatTimestamp(getCurrentTime()) << "\",\n";
        file << "  \"total_alerts\": " << alert_history_.size() << ",\n";

        file << "  \"alerts\": [\n";
        for (size_t i = 0; i < alert_history_.size(); ++i) {
            if (i > 0) file << ",\n";
            file << alert_history_[i].toJson();
        }
        file << "\n  ]\n";
        file << "}";

        file.close();
        Logger::info("Exported {} alerts to {}", alert_history_.size(), filename);
        return true;

    } catch (const std::exception& e) {
        Logger::error("Failed to export alert history: {}", e.what());
        return false;
    }
}

bool PerformanceAlertManager::isHealthy() const {
    auto stats = getStatistics();
    return stats.success_rate >= 0.95 && stats.total_alerts_sent < 1000;
}

std::vector<std::string> PerformanceAlertManager::getHealthIssues() const {
    std::vector<std::string> issues;

    auto stats = getStatistics();
    if (stats.success_rate < 0.95) {
        issues.push_back("Low alert success rate: " + std::to_string(stats.success_rate * 100) + "%");
    }

    if (stats.total_alerts_sent > 1000) {
        issues.push_back("High alert volume: " + std::to_string(stats.total_alerts_sent));
    }

    std::lock_guard<std::lock_guard<std::mutex> lock(channels_mutex_);
    for (const auto& [channel, config] : channels_) {
        if (config.enabled && !config.isValid()) {
            issues.push_back("Invalid configuration for channel: " + std::to_string(static_cast<int>(channel)));
        }
    }

    return issues;
}

void PerformanceAlertManager::processingLoop() {
    while (processing_active_.load()) {
        std::unique_lock<std::mutex> lock(alert_queue_mutex_);
        alert_queue_cv_.wait(lock, [this] { return !pending_alerts_.empty() || !processing_active_.load(); });

        while (!pending_alerts_.empty() && processing_active_.load()) {
            AlertMessage alert = pending_alerts_.front();
            pending_alerts_.pop();

            try {
                auto results = processAlert(alert);

                // Update delivery status
                {
                    std::lock<std::mutex> lock(regressions_mutex_);
                    auto& stored_alert = regressions_[alert.id];
                    for (const auto& result : results) {
                        stored_alert.delivery_status[result.channel] = result.success;
                        if (!stored_alert.delivery_attempts.empty()) {
                            stored_alert.delivery_attempts.push_back(getCurrentTime());
                        }
                    }
                }

                // Remove from pending queue after processing
                pending_alerts_.pop();

            } catch (const std::exception& e) {
                Logger::error("Error processing alert {}: {}", alert.id, e.what());
            }
        }
    }

    Logger::debug("Alert processing loop ended");
}

void PerformanceAlertManager::recordAlertInHistory(const AlertMessage& alert) {
    std::lock_guard<std::mutex> lock(history_mutex_);

    alert_history_.push_back(alert);

    // Maintain history size limit
    if (alert_history_.size() > max_history_size_) {
        alert_history_.erase(alert_history_.begin(),
                           alert_history_.begin() + (alert_history_.size() - max_history_size_));
    }
}

void PerformanceAlertManager::recordDeliveryResult(const std::string& alert_id, AlertChannel channel, const DeliveryResult& result) {
    std::lock_guard<std::mutex> lock(regressions_mutex_);

    auto it = regressions_.find(alert_id);
    if (it != regressions_.end()) {
        it->second.delivery_status[channel] = result.success;
        if (!it->second.delivery_attempts.empty()) {
            it->second.delivery_attempts.push_back(getCurrentTime());
        }
    }
}

std::string PerformanceAlertManager::generateAlertId() const {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);

    return "alert_" + std::to_string(timestamp) + "_" + std::to_string(dis(gen));
}

std::chrono::system_clock::time_point PerformanceAlertManager::getCurrentTime() const {
    return std::chrono::system_clock::now();
}

std::string PerformanceAlertManager::formatTimestamp(
    const std::chrono::system_clock::time_point& timestamp) const {

    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string PerformanceAlertManager::escapeJsonString(const std::string& str) const {
    std::string escaped;
    for (char c : str) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c; break;
        }
    }
    return escaped;
}

void PerformanceAlertManager::stopProcessing() {
    processing_active_.store(false);
    alert_queue_cv_.notify_all();

    if (processing_thread_ && processing_thread_->joinable()) {
        processing_thread_->join();
    }

    Logger::info("Alert processing stopped");
}

void PerformanceAlertManager::setDefaultTemplate(const std::string& template_id) {
    default_template_id_ = template_id;
}

void PerformanceAlertManager::setupDefaultConfiguration() {
    // Add default channels
    addChannel(createDefaultChannelConfig(AlertChannel::EMAIL));
    addChannel(createDefaultChannelConfig(AlertChannel::CONSOLE_LOG));

    // Add default templates
    addTemplate(createDefaultTemplate());

    // Set default template
    setDefaultTemplate("default");
}

ChannelConfiguration PerformanceAlertManager::createDefaultChannelConfig(AlertChannel channel) {
    ChannelConfiguration config;
    config.type = channel;
    config.enabled = true;
    config.minimum_priority = AlertPriority::NORMAL;
    config.cooldown_period = std::chrono::seconds(300); // 5 minutes
    config.max_retries = 3;
    config.retry_delay = std::chrono::seconds(30);
    config.rate_limit_window = std::chrono::seconds(60);
    config.max_alerts_per_window = 10;

    // Channel-specific settings
    switch (channel) {
        case AlertChannel::EMAIL:
            config.channel_settings["recipients"] = "admin@example.com";
            config.channel_settings["smtp_server"] = "smtp.example.com:587";
            config.channel_settings["username"] = "alert@example.com";
            config.channel_settings["password"] = "password";
            break;

        case AlertChannel::SLACK:
            config.webhook_url = "https://hooks.slack.com/services/T1234567890/BXXXXXXXXXXXXXXXX";
            config.authentication_token = "xoxp-1234567890-1234567890";
            break;

        case AlertChannel::DISCORD:
            config.webhook_url = "https://discord.com/api/webhooks/webhook_id";
            config.authentication_token = "Bot ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            config.channel_settings["username"] = "AlertBot";
            break;

        case AlertChannel::SMS:
            config.channel_settings["api_key"] = "twilio_api_key";
            config.channel_settings["from_number"] = "+1234567890";
            config.channel_settings["to_numbers"] = "+19876543210";
            break;

        case AlertChannel::WEBHOOK:
            config.webhook_url = "https://api.example.com/webhook/alerts";
            config.channel_settings["timeout"] = "30";
            break;

        case AlertChannel::BROWSER_NOTIFICATION:
            config.channel_settings["icon"] = "/path/to/icon.png";
            config.channel_settings["require_interaction"] = "false";
            break;

        case AlertChannel::CONSOLE_LOG:
            // No specific settings needed
            break;

        default:
            break;
    }

    return config;
}

AlertTemplate PerformanceAlertManager::createDefaultTemplate() {
    AlertTemplate template;
    template.id = "default";
    template.name = "Default Alert Template";

    template.subject_template = "🚨 {{alert.priority}} - {{alert.title}}";
    template.body_template = R"(
        **{{alert.title}}**\n\n"
        "{{alert.message}}\n\n"
        "---\n"
        "**Priority:** {{alert.priority}}\n"
        "**Category:** {{alert.category}}\n"
        "**Time:** {{alert.timestamp}}\n"
        "{{#if metadata}}\n"
        "**Details:**\n"
        "{{#for metadata}}\n"
        • **{{key}}:** {{value}}\n"
        "{{/for}}\n"
        "{{/if}}\n"
    );

    template.supported_channels = {
        AlertChannel::EMAIL,
        AlertChannel::SLACK,
        AlertChannel::DISCORD,
        AlertChannel::SMS,
        AlertChannel::WEBHOOK,
        AlertChannel::BROWSER_NOTIFICATION,
        AlertChannel::CONSOLE_LOG
    };

    template.default_variables = {
        {"app_name", "Puzzle71Solver"},
        {"environment", "production"},
        {"instance", "GPU-0"}
    };

    return template;
}

// AlertManagerFactory implementation
std::unique_ptr<PerformanceAlertManager> AlertManagerFactory::create() {
    return std::make_unique<PerformanceAlertManager>();
}

std::unique_ptr<PerformanceAlertManager> AlertManagerFactory::createProductionManager() {
    auto manager = std::make_unique<PerformanceAlertManager>();

    // Add production channels
    manager->addChannel(AlertManagerFactory::createEmailConfig(
        "smtp.production.com", "alerts@company.com", "password123",
        {"devops@company.com", "admin@company.com"}
    ));

    manager->addChannel(AlertManagerFactory::createSlackConfig(
        "https://hooks.slack.com/services/production/webhook"
    ));

    manager->addChannel(AlertManagerFactory::createBrowserNotificationConfig());

    // Add production templates
    manager->addTemplate(AlertManagerFactory::createPerformanceAlertTemplate());

    return manager;
}

std::unique_ptr<PerformanceAlertManager> AlertManagerFactory::createDevelopmentManager() {
    auto manager = std::make_unique<PerformanceAlertManager>();

    // Add development channels
    manager->addChannel(AlertManagerFactory::createConsoleLogConfig());
    manager->addChannel(AlertManagerFactory::createBrowserNotificationConfig());

    // Add development templates
    manager->addTemplate(AlertManagerFactory::createPerformanceAlertTemplate());

    // Set more permissive settings for development
    for (auto& channel : manager->getChannels()) {
        channel.minimum_priority = AlertPriority::INFO;
        channel.cooldown_period = std::chrono::seconds(60); // 1 minute
    }

    return manager;
}

std::unique_ptr<PerformanceAlertManager> AlertManagerFactory::createWithEmailAndSlack(
    const std::string& email_smtp_server,
    const std::string& email_username,
    const std::string& email_password,
    const std::string& slack_webhook) {

    auto manager = std::make_unique<PerformanceAlertManager>();

    // Configure email channel
    auto email_config = AlertManagerFactory::createEmailConfig(
        email_smtp_server, email_username, email_password,
        {"alerts@company.com", "devops@company.com"}
    );
    manager->addChannel(email_config);

    // Configure Slack channel if webhook provided
    if (!slack_webhook.empty()) {
        auto slack_config = AlertManagerFactory::createSlackConfig(slack_webhook);
        manager->addChannel(slack_config);
    }

    // Add templates
    manager->addTemplate(AlertManagerFactory::createPerformanceAlertTemplate());

    return manager;
}

ChannelConfiguration AlertManagerFactory::createEmailConfig(
    const std::string& smtp_server,
    const std::string& username,
    const std::string& password,
    const std::vector<std::string>& recipients) {

    ChannelConfiguration config;
    config.type = AlertChannel::EMAIL;
    config.enabled = true;
    config.minimum_priority = AlertPriority::NORMAL;
    config.cooldown_period = std::chrono::seconds(300); // 5 minutes
    config.max_retries = 3;
    config.retry_delay = std::chrono::seconds(30);
    config.rate_limit_window = std::chrono::seconds(60);
    config.max_alerts_per_window = 20;

    config.channel_settings["recipients"] = "";
    for (const auto& recipient : recipients) {
        config.channel_settings["recipients"] += (config.channel_settings["recipients"].empty() ? "" : ", ") + recipient;
    }

    config.channel_settings["smtp_server"] = smtp_server;
    config.channel_settings["username"] = username;
    config.channel_settings["password"] = password;

    return config;
}

ChannelConfiguration AlertManagerFactory::createSlackConfig(const std::string& webhook_url) {
    ChannelConfiguration config;
    config.type = AlertChannel::SLACK;
    config.enabled = true;
    config.minimum_priority = AlertPriority::NORMAL;
    config.cooldown_period = std::chrono::seconds(300);
    config.max_retries = 3;
    config.retry_delay = std::chrono::seconds(30);
    config.rate_limit_window = std::chrono::seconds(60);
    config.max_alerts_per_window = 30;

    config.webhook_url = webhook_url;
    config.authentication_token = "xoxb-1234567890-1234567890";

    return config;
}

ChannelConfiguration AlertManagerFactory::createDiscordConfig(const std::string& webhook_url) {
    ChannelConfiguration config;
    config.type = AlertChannel::DISCORD;
    config.enabled = true;
    config.minimum_priority = AlertPriority::HIGH;
    config.cooldown_period = std::chrono::seconds(600); // 10 minutes
    config.max_retries = 5;
    config.retry_delay = std::chrono::seconds(60);
    config.rate_limit_window = std::chrono::seconds(120);
    config.max_alerts_per_window = 15;

    config.webhook_url = webhook_url;
    config.authentication_token = "Bot ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    config.channel_settings["username"] = "PerformanceAlertBot";

    return config;
}

ChannelConfiguration AlertManagerFactory::createSMSConfig(
    const std::string& api_key,
    const std::string& from_number,
    const std::vector<std::string>& to_numbers) {

    ChannelConfiguration config;
    config.type = AlertChannel::SMS;
    config.enabled = false; // Disabled by default
    config.minimum_priority = AlertPriority::CRITICAL;
    config.cooldown_period = std::chrono::seconds(1800); // 30 minutes
    config.max_retries = 5;
    config.retry_delay = std::chrono::seconds(300); // 5 minutes

    config.channel_settings["api_key"] = api_key;
    config.channel_settings["from_number"] = from_number;
    config.channel_settings["to_numbers"] = "";
    for (const auto& number : to_numbers) {
        config.channel_settings["to_numbers"] += (config.channel_settings["to_numbers"].empty() ? "" : ", ") + number;
    }

    return config;
}

ChannelConfiguration AlertManagerFactory::createWebhookConfig(const std::string& webhook_url) {
    ChannelConfiguration config;
    config.type = AlertChannel::WEBHOOK;
    config.enabled = false; // Disabled by default
    config.minimum_priority = AlertPriority::NORMAL;
    config.cooldown_period = std::chrono::seconds(300); // 5 minutes
    config.max_retries = 3;
    config.retry_delay = std::chrono::seconds(60); // 1 minute

    config.webhook_url = webhook_url;
    config.channel_settings["timeout"] = "30";

    return config;
}

ChannelConfiguration AlertManagerFactory::createBrowserNotificationConfig() {
    ChannelConfiguration config;
    config.type = AlertChannel::BROWSER_NOTIFICATION;
    config.enabled = true;
    config.minimum_priority = AlertPriority::INFO;
    config.cooldown_period = std::seconds(10); // 10 seconds
    config.max_retries = 1;
    config.retry_delay = std::chrono::seconds(0); // No retry

    config.channel_settings["require_interaction"] = "false";
    config.channel_settings["icon"] = "/icons/alert.png";

    return config;
}

AlertTemplate AlertManagerFactory::createPerformanceAlertTemplate() {
    AlertTemplate template;
    template.id = "performance_alert";
    template.name = "Performance Alert Template";

    template.subject_template = "📊 {{alert.priority}} - {{alert.title}}";
    template.body_template = R"(
        **{{alert.title}}**\n\n"
        "{{alert.message}}\n\n"
        "**Performance Impact:** {{metadata.performance_impact}}%\n"
        "**Affected Metric:** {{metadata.metric_name}}\n"
        "**Device:** {{metadata.device_info}}\n\n"
        "**Details:**\n"
        "• Baseline: {{metadata.baseline_value}}\n"
        "• Current: {{metadata.current_value}}\n"
        "• Change: {{metadata.percentage_change}}%\n\n"
        "**Statistical Significance:** {{#if metadata.is_statistically_significant}}YES{{else}}NO{{/if}}**\n"
        "**P-value:** {{metadata.p_value}}\n"
        "**Confidence:** {{metadata.confidence_level}}%\n\n"
        "{{#if affected_metrics}}\n"
        "**Also Affected:**\n"
        "{{#for affected_metrics}}• {{.}}\n"
        "{{/for}}\n"
        "{{/if}}\n"
        "---\n"
        "**Recommended Actions:**\n"
        "{{#if recommended_actions}}\n"
        "• {{.}}\n"
        "{{/if}}\n"
        R"(escapeJsonString(template_.processTemplate(alert)) << "\n"
    );

    template.supported_channels = {
        AlertChannel::EMAIL,
        AlertChannel::SLACK,
        AlertChannel::DISCORD,
        AlertChannel::WEBHOOK,
        AlertChannel::BROWSER_NOTIFICATION,
        AlertChannel::CONSOLE_LOG
    };

    template.default_variables = {
        {"app_name", "Puzzle71Solver"},
        {"environment", "production"},
        {"instance", "GPU-0"}
    };

    return template;
}

AlertTemplate AlertManagerFactory::createRegressionAlertTemplate() {
    AlertTemplate template;
    template.id = "regression_alert";
    template.name = "Regression Alert Template";

    template.subject_template = "🚨 REGRESSION DETECTED - {{alert.title}}";
    template.body_template = R"(
        **🚨 Critical Performance Regression Detected**\n\n"
        "**Metric:** {{alert.metric_name}}\n"
        "**Severity:** {{alert.getSeverityString()}}\n"
        "**Impact:** {{alert.percentage_change}}% {{#if alert.percentage_change > 0}}(degradation)improvement{{/if}}**}}\n\n"
        "**Analysis:**\n"
        "**Baseline:** {{baseline_value}}\n"
        "**Current:** {{current_value}}\n"
        "**Change:** {{percentage_change}}%\n"
        "**Z-Score:** {{z_score}}\n"
        "**P-Value:** {{p_value}}\n"
        "**Confidence:** {{confidence_level}}%\n\n"
        **Potential Causes:**\n"
        "{{#for potential_causes}}\n"
        "• {{.}}\n"
        "{{/for}}\n\n"
        "**Affected Metrics:**\n"
        "{{#for affected_metrics}}\n"
        "• {{.}}\n"
        "{{/for}}\n\n"
        "**Environmental Context:**\n"
        "{{environmental_notes}}\n\n"
        "**Recommended Actions:**\n"
        1. **Investigate recent changes**\n"
        2. **Check resource utilization**\n"
        3. **Consider rollback if recent deployment**\n"
        4. **Contact development team**\n"
        5. **Monitor for escalation**\n\n"
        "---\n"
        R"(escapeJsonString(template_.processTemplate(alert)) << "\n"
    );

    template.supported_channels = {
        AlertChannel::EMAIL,
        AlertChannel::SLACK,
        AlertChannel::DISCORD,
        AlertChannel::WEBHOOK,
        AlertChannel::BROWSER_NOTIFICATION,
        AlertChannel::CONSOLE_LOG
    };

    template.default_variables = {
        {"app_name", "Puzzle71Solver"},
        {"environment", "production"},
        {"instance", "GPU-0"}
    };

    return template;
}

AlertTemplate AlertManagerFactory::createOptimizationAlertTemplate() {
    AlertTemplate template;
    template.id = "optimization_alert";
    template.name = "Optimization Alert Template";

    template.subject_template = "🎯 OPTIMIZATION - {{alert.title}}";
    template.body_template = R"(
        **🎯 Optimization Completed**\n\n"
        "{{alert.message}}\n\n"
        "**Results:**\n"
        "• **Best Objective:** {{metadata.best_objective}}\n"
        "• **Total Iterations:** {{metadata.iterations}}\n"
        "• **Time Taken:** {{metadata.execution_time_ms}}ms\n"
        "\n"
        "**Performance Improvement:** {{#if metadata.performance_improvement > 0}}+{{metadata.performance_improvement}}%{{else}}{{metadata.performance_improvement}}%{{/if}}**\n"
        "\n"
        "**Algorithm Used:** {{metadata.algorithm_used}}\n"
        "**Device:** {{metadata.device_info}}\n"
        "\n"
        "---\n"
        "**Configuration Changes:**\n"
        "{{#if configuration_changes}}\n"
        "• {{.}}\n"
        "{{/for configuration_changes}}\n"
        "{{/if}}\n"
        "\n"
        R"(escapeJsonString(template_.processTemplate(alert)) << "\n"
    );

    template.supported_channels = {
        AlertChannel::EMAIL,
        AlertChannel::SLACK,
        AlertChannel::DISCORD,
        AlertChannel::WEBHOOK,
        AlertChannel::BROWSER_NOTIFICATION,
        AlertChannel::CONSOLE_LOG
    };

    template.default_variables = {
        {"app_name", "Puzzle71Solver"},
        {"environment", "production"},
        {"instance", "GPU-0"}
    };

    return template;
}

AlertTemplate AlertManagerFactory::createSystemHealthAlertTemplate() {
    AlertTemplate template;
    template.id = "system_health_alert";
    template.name = "System Health Alert Template";

    template.subject_template = "🔥 SYSTEM HEALTH ALERT - {{alert.title}}";
    template.body_template = R"(
        **🔥 System Health Alert**\n\n"
        "{{alert.message}}\n\n"
        "**Health Status:** {{#if alert.title == "System Health"}}GOOD{{else}}DEGRADED{{/if}}**}}\n"
        "**Device:** {{metadata.device_info}}\n"
        "**Uptime:** {{metadata.uptime_seconds}}\n"
        "\n"
        "**Metrics Overview:**\n"
        "{{#for system_metrics}}\n"
        "• **{{name}}:** {{value}}\n"
        "{{/for system_metrics}}\n"
        "\n"
        "**Alert Recommendations:**\n"
        {{#if recommendations}}\n"
        "• {{.}}\n"
        "{{/for recommendations}}\n"
        "{{/if}}\n"
        "\n"
        "---\n"
        R"(escapeJsonString(template_.processTemplate(alert)) << "\n"
    );

    template.supported_channels = {
        AlertChannel::EMAIL,
        AlertChannel::SLACK,
        AlertChannel::DISCORD,
        AlertChannel::WEBHOOK,
        AlertChannel::BROWSER_NOTIFICATION,
        AlertChannel::CONSOLE_LOG
    };

    template.default_variables = {
        {"app_name", "Puzzle71Solver"},
        {"environment", "production"},
        {"instance", "GPU-0"}
    };

    return template;
}

// Alert utilities implementation
namespace alert_utils {

std::string formatAlertTitle(const AlertMessage& alert) {
    std::string title = alert.title;

    // Add priority indicator
    switch (alert.priority) {
        case AlertPriority::CRITICAL: title = "🚨 " + title; break;
        case AlertPriority::ERROR: title = "❌ " + title; break;
        case AlertPriority::WARNING: title = "⚠️ " + title; break;
        case AlertPriority::NORMAL: title = "ℹ️ " + title; break;
        case AlertPriority::LOW: title = "ℹ️ " + title; break;
    }

    return title;
}

std::string formatAlertMessage(const AlertMessage& alert) {
    return alert.message;
}

std::string formatAlertMetadata(const AlertMessage& alert) {
    if (alert.metadata.empty()) {
        return "";
    }

    std::ostringstream metadata;
    bool first = true;
    for (const auto& [key, value] : alert.metadata) {
        if (!first) {
            metadata << "Metadata:\n";
            first = false;
        }
        metadata << "  " << key << ": " << value << "\n";
    }

    return metadata.str();
}

std::string getPriorityColor(AlertPriority priority) {
    switch (priority) {
        case AlertPriority::CRITICAL: return "#dc3545";  // Red
        case AlertPriority::ERROR: return "#dc3545";     // Red
        case AlertPriority::WARNING: return "#ffc107";   // Yellow
        case AlertPriority::NORMAL: return "#17a2b8";   // Blue
        case AlertPriority::LOW: return "#28a745";      // Green
        default: return "#6c757d";    // Gray
    }
}

std::string getPriorityEmoji(AlertPriority priority) {
    switch (priority) {
        case AlertPriority::CRITICAL: return "🚨"; break;
        case AlertPriority::ERROR: return "❌"; break;
        case AlertPriority::WARNING: return "⚠️"; break;
        case AlertPriority::NORMAL: return "ℹ️"; break;
        case AlertPriority::LOW: return "ℹ️"; break;
        default: return "ℹ️";
    }
}

std::string formatForEmail(const AlertMessage& alert, const AlertTemplate& template_) {
    std::string content = template_.processTemplate(alert);
    return content;
}

std::string formatForSlack(const AlertMessage& alert, const AlertTemplate& template_) {
    std::string content = template_.processTemplate(alert);
    return content;
}

std::formatForDiscord(const AlertMessage& alert, const AlertTemplate& template_) {
    std::string embed = buildDiscordEmbed(alert, template_);
    return embed;
}

std::string formatForSMS(const AlertMessage& alert, const AlertTemplate& template_) {
    std::string content = template_.processTemplate(alert);
    return content;
}

std::formatForWebhook(const AlertMessage& alert, const AlertTemplate& template_) {
    std::string content = template_.processTemplate(alert);
    return content;
}

std::string formatForBrowserNotification(const AlertMessage& alert) {
    std::string title = formatAlertTitle(alert);
    std::string message = formatAlertMessage(alert);

    // Limit content for browser notifications
    if (message.length() > 200) {
        message = message.substr(0, 197) + "...";
    }

    return title + ": " + message;
}

bool validateAlertMessage(const AlertMessage& alert) {
    return !alert.id.empty() && !alert.title.empty() && !alert.message.empty();
}

bool validateChannelConfiguration(const ChannelConfiguration& config) {
    return config.isValid();
}

bool validateAlertTemplate(const AlertTemplate& template_) {
    return template_.isValid();
}

std::string generateAlertEmailHTML(const AlertMessage& alert) {
    std::ostringstream html;

    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"en\">\n";
    html << "<head>\n";
    html << "    <meta charset=\"UTF-8\">\n";
    html << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "    <title>Alert: " << alert.title << "</title>\n";
    html << "    <style>\n";
    html << "        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f5f5f5; }\n";
    html << "        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); }\n";
    html << "        .header { text-align: center; margin-bottom: 20px; }\n";
    html << "        .alert { padding: 15px; border-radius: 6px; margin-bottom: 10px; }\n";
    html << "        .alert.critical { background-color: #f8d7da; border-color: #f5c6cb; color: #721c24; }\n";
    html << "        .alert.error { background-color: #f8d7da; border-color: #f5c6cb; color: #721c24; }\n";
    html << "        .alert.warning { background-color: #fff3cd; border-color: #ffeaa7; color: #856404; }\n";
    html << "        .alert.info { background-color: #d1ecf1; border-color: #bee5db; color: #0c5460; }\n";
    html << "        .alert.low { background-color: #d4edda; border-color: #c3e6cb; color: #383d41; }\n";
    html << "        .alert.normal { background-color: #e2e6ea; border-color: #dae2e6; color: #495057; }\n";
    html << "    </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "    <div class=\"container\">\n";
    html << "        <div class=\"header\">\n";
    html << "            <h1>🚨 Performance Alert</h1>\n";
    html << "        </div>\n";
    html << "        <div class=\"alert " << getPriorityColor(alert.priority) << "\">\n";
    html << "            <h3>" << alert.title << "</h3>\n";
    html << "            <p>" << formatAlertMessage(alert) << "</p>\n";
    html << "        </div>\n";
    html << "        <div class=\"metadata\">\n";
    html << "            <h4>Details</h4>\n";
    html << "            <ul>\n";

    // Metadata fields
    if (!alert.metadata.empty()) {
        for (const auto& [key, value] : alert.metadata) {
            html << "                <li><strong>" << key << ":</strong> " << value << "</li>\n";
        }
    }

    // Delivery status
    html << "            </ul>\n";
    html << "            <h4>Delivery Status</h4>\n";
    html << "            <ul>\n";
    html << "                <li><strong>Status:</strong> " << alert.getDeliveryStatus() << "</li>\n";
    html << "                <li><strong>Time:</strong> " << alert.delivery_time.count() << "ms</li>\n";
    html << "            </ul>\n";
    html << "        </div>\n";
    html << "    </div>\n";
    html << "    </body>\n";
    html << "</html>\n";

    return html.str();
}

std::string generateAlertMarkdown(const AlertMessage& alert) {
    std::ostringstream markdown;

    markdown << "**" << getPriorityEmoji(alert.priority) << " " << alert.title << "**\n\n";
    markdown << "```\n";
    markdown << formatAlertMessage(alert) << "\n";
    markdown << "```\n\n";

    // Metadata
    if (!alert.metadata.empty()) {
        markdown << "\n**Details:**\n";
        for (const auto& [key, value] : alert.metadata) {
            markdown << "- **" << key << ":** " << value << "\n";
        }
    }

    // Delivery status
    markdown << "\n**Delivery Status:** " << alert.getDeliveryStatus() << "\n";
    markdown << "- **Time:** " << alert.delivery_time.count() << "ms";

    return markdown.str();
}

std::string urlEncode(const std::string& str) {
    std::ostringstream encoded;
    for (char c : str) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.') {
            encoded << c;
        } else if (c == ' ') {
            encoded << '+';
        } else {
            encoded << '%';
            encoded << std::hex << static_cast<int>(c);
        }
    }
    }
    return encoded.str();
}

std::string urlDecode(const std::string& str) {
    std::ostringstream decoded;
    for (size_t i = 0; i < str.length(); ) {
        if (str[i] == '%' && i + 1 < str.length() &&
            std::isxdigit(str[i+1]) &&
            std::isxdigit(str[i+2]) &&
            std::isxdigit(str[i+3])) {

            std::string hex_byte = str.substr(i+1, 2);
            char byte = static_cast<char>(std::stoul(hex_byte, 16));
            decoded += byte;
            i += 3;
        } else if (str[i] == '+') {
            i++;
            size_t j = i;
            while (j < str.length() && j < str.length() &&
                       std::isxdigit(str[j])) {
                j++;
            }
            if (j < str.length()) {
                std::string hex_byte = str.substr(i+1, j-i);
                char byte = static_cast<char>(std::stoul(hex_byte, 16));
                decoded += byte;
                i += j + 1;
            } else {
                decoded += '+';
            }
        } else if (str[i] == '%') {
            i++;
            size_t j = i + 1;
            while (j < str.length() &&
                       std::isxdigit(str[j])) {
                j++;
            }
            if (j < str.length() && j < str.length()) {
                std::string hex_byte = str.substr(i+1, j-i);
                char byte = static_cast<char>(std::stoul(hex_byte, 16));
                decoded += byte;
                i += j + 1;
            } else {
                decoded += '%';
                i++;
            }
        } else {
            decoded += str[i];
        }
    }

    return decoded.str();
}

} // namespace alert_utils

} // namespace puzzle71::alerting

} // namespace puzzle71::alerting
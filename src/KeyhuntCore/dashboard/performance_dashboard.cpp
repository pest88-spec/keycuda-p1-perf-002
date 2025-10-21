// Puzzle71Solver - Performance Dashboard Implementation (T047)
// Phase 6: User Story 4 - Performance Monitoring
// Web-based performance dashboard with real-time metrics visualization

#include "performance_dashboard.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <regex>
#include <thread>
#include <ctime>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace puzzle71::dashboard {

// TimeSeriesData implementation
void TimeSeriesData::addPoint(const DataPoint& point) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    data_.push_back(point);

    // Limit data points to prevent memory issues
    if (data_.size() > max_points_) {
        data_.erase(data_.begin(), data_.begin() + (data_.size() - max_points_));
    }
}

std::vector<DataPoint> TimeSeriesData::getRecentPoints(std::chrono::seconds duration) const {
    std::lock_guard<std::mutex> lock(data_mutex_);

    auto cutoff_time = std::chrono::high_resolution_clock::now() - duration;

    std::vector<DataPoint> recent_points;
    for (const auto& point : data_) {
        if (point.timestamp >= cutoff_time) {
            recent_points.push_back(point);
        }
    }

    return recent_points;
}

std::vector<DataPoint> TimeSeriesData::getRecentPoints(size_t count) const {
    std::lock_guard<std::mutex> lock(data_mutex_);

    if (count >= data_.size()) {
        return data_;
    }

    return std::vector<DataPoint>(data_.end() - count, data_.end());
}

void TimeSeriesData::clear() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    data_.clear();
}

double TimeSeriesData::getLatestValue() const {
    std::lock_guard<std::mutex> lock(data_mutex_);

    if (data_.empty()) {
        return 0.0;
    }

    return data_.back().value;
}

double TimeSeriesData::getAverageValue(std::chrono::seconds duration) const {
    auto points = getRecentPoints(duration);

    if (points.empty()) {
        return 0.0;
    }

    double sum = 0.0;
    for (const auto& point : points) {
        sum += point.value;
    }

    return sum / points.size();
}

double TimeSeriesData::getMinValue(std::chrono::seconds duration) const {
    auto points = getRecentPoints(duration);

    if (points.empty()) {
        return 0.0;
    }

    auto min_it = std::min_element(points.begin(), points.end(),
        [](const DataPoint& a, const DataPoint& b) { return a.value < b.value; });

    return min_it->value;
}

double TimeSeriesData::getMaxValue(std::chrono::seconds duration) const {
    auto points = getRecentPoints(duration);

    if (points.empty()) {
        return 0.0;
    }

    auto max_it = std::max_element(points.begin(), points.end(),
        [](const DataPoint& a, const DataPoint& b) { return a.value < b.value; });

    return max_it->value;
}

// DashboardWidget implementation
DashboardWidget::DashboardWidget(const WidgetConfig& config) : config_(config) {}

std::string DashboardWidget::formatValue(double value) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    if (!config_.unit.empty()) {
        oss << value << " " << config_.unit;
    } else if (value >= 1000000.0) {
        oss << (value / 1000000.0) << "M";
    } else if (value >= 1000.0) {
        oss << (value / 1000.0) << "K";
    } else {
        oss << value;
    }

    return oss.str();
}

std::string DashboardWidget::getColorForValue(double value) const {
    double normalized = (value - config_.min_value) / (config_.max_value - config_.min_value);
    normalized = std::max(0.0, std::min(1.0, normalized));

    if (normalized < 0.3) {
        return "#28a745";  // Green
    } else if (normalized < 0.7) {
        return "#ffc107";  // Yellow
    } else {
        return "#dc3545";  // Red
    }
}

std::string DashboardWidget::getProgressBarHtml(double value, double max) const {
    double percentage = (max > 0) ? (value / max * 100.0) : 0.0;
    percentage = std::max(0.0, std::min(100.0, percentage));

    std::ostringstream html;
    html << "<div class=\"progress-bar-container\">";
    html << "<div class=\"progress-bar\" style=\"width: " << percentage << "%;";
    html << " background-color: " << getColorForValue(value) << ";\"></div>";
    html << "<span class=\"progress-text\">" << formatValue(value) << "</span>";
    html << "</div>";

    return html.str();
}

// MetricCardWidget implementation
MetricCardWidget::MetricCardWidget(const WidgetConfig& config) : DashboardWidget(config) {
    current_value_ = "0";
    trend_indicator_ = "";
    status_class_ = "status-normal";
}

std::string MetricCardWidget::renderHtml() const {
    std::ostringstream html;

    html << "<div class=\"widget metric-card\" id=\"" << config_.id << "\"";
    html << " style=\"grid-column: span " << config_.width << "; grid-row: span " << config_.height << ";\">";

    if (config_.show_title) {
        html << "<div class=\"widget-title\">" << config_.title << "</div>";
    }

    html << "<div class=\"metric-value " << status_class_ << "\">" << current_value_ << "</div>";

    if (!trend_indicator_.empty()) {
        html << "<div class=\"metric-trend\">" << trend_indicator_ << "</div>";
    }

    html << "</div>";

    return html.str();
}

void MetricCardWidget::updateData(const std::map<std::string, TimeSeriesData>& data_sources) {
    auto it = data_sources.find(config_.data_source);
    if (it != data_sources.end() && it->second.size() > 0) {
        double current_value = it->second.getLatestValue();
        current_value_ = formatValue(current_value);

        // Calculate trend
        auto recent_points = it->second.getRecentPoints(std::chrono::seconds(60));
        if (recent_points.size() >= 2) {
            double older_value = recent_points.front().value;
            double change_percent = ((current_value - older_value) / older_value) * 100.0;

            if (std::abs(change_percent) > 1.0) {
                trend_indicator_ = (change_percent > 0) ? "↑ " : "↓ ";
                trend_indicator_ += std::to_string(static_cast<int>(std::abs(change_percent))) + "%";

                status_class_ = (change_percent > 0) ? "status-good" : "status-bad";
            } else {
                trend_indicator_ = "→ 0%";
                status_class_ = "status-normal";
            }
        }
    }
}

// TimeSeriesWidget implementation
TimeSeriesWidget::TimeSeriesWidget(const WidgetConfig& config) : DashboardWidget(config) {
    // Extract chart type from custom settings
    auto chart_it = config_.custom_settings.find("chart_type");
    if (chart_it != config_.custom_settings.end()) {
        chart_type_ = chart_it->second;
    }
}

std::string TimeSeriesWidget::renderHtml() const {
    std::ostringstream html;

    html << "<div class=\"widget time-series\" id=\"" << config_.id << "\"";
    html << " style=\"grid-column: span " << config_.width << "; grid-row: span " << config_.height << ";\">";

    if (config_.show_title) {
        html << "<div class=\"widget-title\">" << config_.title << "</div>";
    }

    html << "<div class=\"chart-container\">";
    html << "<canvas id=\"chart-" << config_.id << "\" width=\"400\" height=\"200\"></canvas>";
    html << "</div>";

    if (config_.show_legend) {
        html << "<div class=\"chart-legend\" id=\"legend-" << config_.id << "\"></div>";
    }

    html << "</div>";

    return html.str();
}

std::string TimeSeriesWidget::renderJavaScript() const {
    std::ostringstream js;

    js << "function updateTimeSeriesChart_" << config_.id << "(data) {\n";
    js << "  const canvas = document.getElementById('chart-" << config_.id << "');\n";
    js << "  if (!canvas) return;\n";
    js << "  \n";
    js << "  const ctx = canvas.getContext('2d');\n";
    js << "  \n";
    js << "  // Clear canvas\n";
    js << "  ctx.clearRect(0, 0, canvas.width, canvas.height);\n";
    js << "  \n";
    js << "  if (!data || data.length === 0) return;\n";
    js << "  \n";
    js << "  const padding = 40;\n";
    js << "  const width = canvas.width - 2 * padding;\n";
    js << "  const height = canvas.height - 2 * padding;\n";
    js << "  \n";
    js << "  // Find min and max values\n";
    js << "  let minValue = Math.min(...data.map(d => d.value));\n";
    js << "  let maxValue = Math.max(...data.map(d => d.value));\n";
    js << "  const valueRange = maxValue - minValue || 1;\n";
    js << "  \n";
    js << "  // Draw axes\n";
    js << "  ctx.strokeStyle = '#ccc';\n";
    js << "  ctx.lineWidth = 1;\n";
    js << "  ctx.beginPath();\n";
    js << "  ctx.moveTo(padding, padding);\n";
    js << "  ctx.lineTo(padding, canvas.height - padding);\n";
    js << "  ctx.lineTo(canvas.width - padding, canvas.height - padding);\n";
    js << "  ctx.stroke();\n";
    js << "  \n";
    js << "  // Draw data\n";
    js << "  ctx.strokeStyle = '#007bff';\n";
    js << "  ctx.lineWidth = 2;\n";
    js << "  ctx.beginPath();\n";
    js << "  \n";
    js << "  data.forEach((point, index) => {\n";
    js << "    const x = padding + (index / (data.length - 1)) * width;\n";
    js << "    const y = canvas.height - padding - ((point.value - minValue) / valueRange) * height;\n";
    js << "    \n";
    js << "    if (index === 0) {\n";
    js << "      ctx.moveTo(x, y);\n";
    js << "    } else {\n";
    js << "      ctx.lineTo(x, y);\n";
    js << "    }\n";
    js << "  });\n";
    js << "  \n";
    js << "  ctx.stroke();\n";
    js << "}\n";

    return js.str();
}

void TimeSeriesWidget::updateData(const std::map<std::string, TimeSeriesData>& data_sources) {
    auto it = data_sources.find(config_.data_source);
    if (it != data_sources.end()) {
        // Data will be pulled via JavaScript when requested
    }
}

// GaugeWidget implementation
GaugeWidget::GaugeWidget(const WidgetConfig& config) : DashboardWidget(config) {
    // Define color zones for the gauge
    zones_ = {
        {0.0, "#28a745"},    // Green (0-33%)
        {33.0, "#ffc107"},   // Yellow (33-67%)
        {67.0, "#dc3545"}    // Red (67-100%)
    };
}

std::string GaugeWidget::renderHtml() const {
    std::ostringstream html;

    html << "<div class=\"widget gauge\" id=\"" << config_.id << "\"";
    html << " style=\"grid-column: span " << config_.width << "; grid-row: span " << config_.height << ";\">";

    if (config_.show_title) {
        html << "<div class=\"widget-title\">" << config_.title << "</div>";
    }

    html << "<div class=\"gauge-container\">";
    html << "<svg width=\"200\" height=\"120\" viewBox=\"0 0 200 120\">";
    html << "  <path d=\"M 20 100 A 80 80 0 0 1 180 100\" stroke=\"#e9ecef\" stroke-width=\"15\" fill=\"none\"/>";
    html << "  <path id=\"gauge-arc-" << config_.id << "\" d=\"M 20 100 A 80 80 0 0 1 180 100\" stroke=\"#007bff\" stroke-width=\"15\" fill=\"none\" stroke-dasharray=\"251 251\" stroke-dashoffset=\"251\"/>";
    html << "  <text x=\"100\" y=\"100\" text-anchor=\"middle\" font-size=\"24\" font-weight=\"bold\" id=\"gauge-value-" << config_.id << "\">0%</text>";
    html << "</svg>";
    html << "</div>";

    html << "</div>";

    return html.str();
}

std::string GaugeWidget::renderJavaScript() const {
    std::ostringstream js;

    js << "function updateGauge_" << config_.id << "(value) {\n";
    js << "  const percentage = Math.max(0, Math.min(100, value));\n";
    js << "  const arc = document.getElementById('gauge-arc-" << config_.id << "');\n";
    js << "  const text = document.getElementById('gauge-value-" << config_.id << "');\n";
    js << "  \n";
    js << "  if (arc && text) {\n";
    js << "    const offset = 251 - (percentage / 100) * 251;\n";
    js << "    arc.style.strokeDashoffset = offset;\n";
    js << "    text.textContent = Math.round(percentage) + '%';\n";
    js << "    \n";
    js << "    // Update color based on value\n";
    js << "    let color = '#28a745';\n";
    js << "    if (percentage > 67) color = '#dc3545';\n";
    js << "    else if (percentage > 33) color = '#ffc107';\n";
    js << "    arc.style.stroke = color;\n";
    js << "  }\n";
    js << "}\n";

    return js.str();
}

void GaugeWidget::updateData(const std::map<std::string, TimeSeriesData>& data_sources) {
    auto it = data_sources.find(config_.data_source);
    if (it != data_sources.end()) {
        current_value_ = it->second.getLatestValue();
    }
}

// StatusIndicatorWidget implementation
StatusIndicatorWidget::StatusIndicatorWidget(const WidgetConfig& config) : DashboardWidget(config) {
    status_colors_ = {
        {"good", "#28a745"},
        {"warning", "#ffc107"},
        {"error", "#dc3545"},
        {"unknown", "#6c757d"}
    };
}

std::string StatusIndicatorWidget::renderHtml() const {
    std::ostringstream html;

    html << "<div class=\"widget status-indicator\" id=\"" << config_.id << "\"";
    html << " style=\"grid-column: span " << config_.width << "; grid-row: span " << config_.height << ";\">";

    if (config_.show_title) {
        html << "<div class=\"widget-title\">" << config_.title << "</div>";
    }

    std::string color = status_colors_.count(current_status_) ? status_colors_.at(current_status_) : status_colors_.at("unknown");

    html << "<div class=\"status-container\">";
    html << "<div class=\"status-light\" style=\"background-color: " << color << ";\"></div>";
    html << "<div class=\"status-text\">" << status_message_ << "</div>";
    html << "</div>";

    html << "</div>";

    return html.str();
}

void StatusIndicatorWidget::updateData(const std::map<std::string, TimeSeriesData>& data_sources) {
    auto it = data_sources.find(config_.data_source);
    if (it != data_sources.end() && it->second.size() > 0) {
        double value = it->second.getLatestValue();

        // Determine status based on value ranges
        if (value >= 90.0) {
            current_status_ = "good";
            status_message_ = "Excellent";
        } else if (value >= 70.0) {
            current_status_ = "good";
            status_message_ = "Good";
        } else if (value >= 50.0) {
            current_status_ = "warning";
            status_message_ = "Fair";
        } else if (value > 0.0) {
            current_status_ = "error";
            status_message_ = "Poor";
        } else {
            current_status_ = "unknown";
            status_message_ = "No Data";
        }
    }
}

// DataTableWidget implementation
DataTableWidget::DataTableWidget(const WidgetConfig& config) : DashboardWidget(config) {
    auto sortable_it = config_.custom_settings.find("sortable");
    if (sortable_it != config_.custom_settings.end()) {
        sortable_ = (sortable_it->second == "true");
    }

    auto max_rows_it = config_.custom_settings.find("max_rows");
    if (max_rows_it != config_.custom_settings.end()) {
        max_rows_ = std::stoul(max_rows_it->second);
    }
}

std::string DataTableWidget::renderHtml() const {
    std::ostringstream html;

    html << "<div class=\"widget data-table\" id=\"" << config_.id << "\"";
    html << " style=\"grid-column: span " << config_.width << "; grid-row: span " << config_.height << ";\">";

    if (config_.show_title) {
        html << "<div class=\"widget-title\">" << config_.title << "</div>";
    }

    html << "<div class=\"table-container\">";
    html << "<table class=\"data-table-table\" id=\"table-" << config_.id << "\">";
    html << "  <thead>";
    html << "    <tr>";

    for (const auto& column : columns_) {
        html << "      <th>" << column << "</th>";
    }

    html << "    </tr>";
    html << "  </thead>";
    html << "  <tbody id=\"table-body-" << config_.id << "\">";
    html << "  </tbody>";
    html << "</table>";
    html << "</div>";

    html << "</div>";

    return html.str();
}

std::string DataTableWidget::renderJavaScript() const {
    std::ostringstream js;

    js << "function updateDataTable_" << config_.id << "(data) {\n";
    js << "  const tbody = document.getElementById('table-body-" << config_.id << "');\n";
    js << "  if (!tbody || !data) return;\n";
    js << "  \n";
    js << "  tbody.innerHTML = '';\n";
    js << "  \n";
    js << "  data.forEach(row => {\n";
    js << "    const tr = document.createElement('tr');\n";
    js << "    row.forEach(cell => {\n";
    js << "      const td = document.createElement('td');\n";
    js << "      td.textContent = cell;\n";
    js << "      tr.appendChild(td);\n";
    js << "    });\n";
    js << "    tbody.appendChild(tr);\n";
    js << "  });\n";
    js << "}\n";

    return js.str();
}

void DataTableWidget::updateData(const std::map<std::string, TimeSeriesData>& data_sources) {
    // For now, use a placeholder implementation
    // In production, this would pull from appropriate data sources
    table_data_.clear();

    // Example: Add recent performance metrics
    if (data_sources.count("throughput")) {
        auto throughput_data = data_sources.at("throughput").getRecentPoints(5);
        for (const auto& point : throughput_data) {
            auto time_t = std::chrono::system_clock::to_time_t(
                std::chrono::time_point_cast<std::chrono::system_clock::duration>(point.timestamp));
            std::ostringstream time_str;
            time_str << std::put_time(std::localtime(&time_t), "%H:%M:%S");

            table_data_.push_back({
                time_str.str(),
                std::to_string(static_cast<int>(point.value)),
                "Normal"
            });
        }
    }
}

// PerformanceDashboard implementation
PerformanceDashboard::PerformanceDashboard(int port) : port_(port) {
    // Add default layout
    addLayout(DashboardFactory::createDefaultLayout());
    active_layout_name_ = "default";

    Logger::info("PerformanceDashboard initialized on port {}", port);
}

PerformanceDashboard::~PerformanceDashboard() {
    stop();
}

void PerformanceDashboard::addLayout(const DashboardLayout& layout) {
    std::lock_guard<std::mutex> lock(layouts_mutex_);
    layouts_[layout.name] = layout;

    // Create widgets for this layout
    createWidgetsForLayout(layout);

    Logger::info("Added dashboard layout: {}", layout.name);
}

void PerformanceDashboard::setActiveLayout(const std::string& layout_name) {
    std::lock_guard<std::mutex> lock(layouts_mutex_);

    if (layouts_.find(layout_name) != layouts_.end()) {
        active_layout_name_ = layout_name;
        createWidgetsForLayout(layouts_.at(layout_name));
        Logger::info("Set active layout to: {}", layout_name);
    } else {
        Logger::warn("Layout not found: {}", layout_name);
    }
}

std::vector<std::string> PerformanceDashboard::getAvailableLayouts() const {
    std::lock_guard<std::mutex> lock(layouts_mutex_);

    std::vector<std::string> names;
    for (const auto& [name, layout] : layouts_) {
        names.push_back(name);
    }

    return names;
}

void PerformanceDashboard::addDataSource(const std::string& name, std::shared_ptr<TimeSeriesData> data) {
    std::lock_guard<std::mutex> lock(data_sources_mutex_);
    data_sources_[name] = data;
    Logger::info("Added data source: {}", name);
}

void PerformanceDashboard::addMonitor(const std::string& name, std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor) {
    std::lock_guard<std::mutex> lock(data_sources_mutex_);
    monitors_[name] = monitor;

    // Create data sources for key metrics
    auto throughput_data = std::make_shared<TimeSeriesData>();
    auto utilization_data = std::make_shared<TimeSeriesData>();
    auto temperature_data = std::make_shared<TimeSeriesData>();

    data_sources_[name + "_throughput"] = throughput_data;
    data_sources_[name + "_utilization"] = utilization_data;
    data_sources_[name + "_temperature"] = temperature_data;

    Logger::info("Added monitor: {}", name);
}

void PerformanceDashboard::addOptimizer(const std::string& name, std::shared_ptr<puzzle71::optimization::AutomatedOptimizer> optimizer) {
    std::lock_guard<std::mutex> lock(data_sources_mutex_);
    optimizers_[name] = optimizer;

    // Create data sources for optimization metrics
    auto objective_data = std::make_shared<TimeSeriesData>();
    auto iteration_data = std::make_shared<TimeSeriesData>();

    data_sources_[name + "_objective"] = objective_data;
    data_sources_[name + "_iterations"] = iteration_data;

    Logger::info("Added optimizer: {}", name);
}

bool PerformanceDashboard::start() {
    if (server_running_.load()) {
        Logger::warn("Dashboard server is already running");
        return false;
    }

    server_running_.store(true);
    server_thread_ = std::make_unique<std::thread>(&PerformanceDashboard::serverLoop, this);

    // Start data update thread
    update_thread_running_.store(true);
    update_thread_ = std::make_unique<std::thread>(&PerformanceDashboard::updateDataLoop, this);

    Logger::info("Dashboard server started on port {}", port_);
    return true;
}

void PerformanceDashboard::stop() {
    server_running_.store(false);
    update_thread_running_.store(false);

    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }

    if (update_thread_ && update_thread_->joinable()) {
        update_thread_->join();
    }

    Logger::info("Dashboard server stopped");
}

void PerformanceDashboard::setUpdateInterval(std::chrono::milliseconds interval) {
    update_interval_ = interval;
}

void PerformanceDashboard::setMaxDataPoints(size_t max_points) {
    max_data_points_ = max_points;
}

DashboardLayout PerformanceDashboard::createDefaultLayout() {
    DashboardLayout layout;
    layout.name = "default";
    layout.description = "Default performance monitoring dashboard";
    layout.columns = 12;
    layout.rows = 8;

    // Add throughput metric card
    layout.widgets.push_back(DashboardFactory::createMetricCard(
        "throughput_card", "Throughput", "throughput", 0, 0, 4, 2
    ));

    // Add GPU utilization gauge
    layout.widgets.push_back(DashboardFactory::createGauge(
        "utilization_gauge", "GPU Utilization", "utilization", 100.0, 4, 0, 4, 3
    ));

    // Add temperature gauge
    layout.widgets.push_back(DashboardFactory::createGauge(
        "temperature_gauge", "Temperature", "temperature", 100.0, 8, 0, 4, 3
    ));

    // Add throughput time series
    layout.widgets.push_back(DashboardFactory::createTimeSeries(
        "throughput_chart", "Throughput Over Time", "throughput", 0, 2, 8, 4
    ));

    // Add status indicator
    layout.widgets.push_back(DashboardFactory::createStatusIndicator(
        "system_status", "System Status", "health", 0, 6, 4, 2
    ));

    return layout;
}

void PerformanceDashboard::serverLoop() {
    // Simple HTTP server implementation
    // In production, use a proper web server library

    Logger::info("Dashboard server loop started");

    while (server_running_.load()) {
        // This is a simplified server implementation
        // In production, you'd use proper HTTP server libraries

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    Logger::info("Dashboard server loop ended");
}

void PerformanceDashboard::updateDataLoop() {
    Logger::info("Dashboard data update loop started");

    while (update_thread_running_.load()) {
        try {
            collectMonitoringData();
            collectOptimizationData();
            updateWidgets();
        } catch (const std::exception& e) {
            Logger::error("Error updating dashboard data: {}", e.what());
        }

        std::this_thread::sleep_for(update_interval_);
    }

    Logger::info("Dashboard data update loop ended");
}

void PerformanceDashboard::collectMonitoringData() {
    std::lock_guard<std::mutex> lock(data_sources_mutex_);

    for (const auto& [name, monitor] : monitors_) {
        if (monitor->isMonitoring()) {
            auto metrics = monitor->getCurrentMetrics();

            // Add data points to time series
            if (data_sources_.count(name + "_throughput")) {
                data_sources_[name + "_throughput"]->addPoint(
                    DataPoint(metrics.keys_per_second)
                );
            }

            if (data_sources_.count(name + "_utilization")) {
                data_sources_[name + "_utilization"]->addPoint(
                    DataPoint(metrics.gpu_utilization_percent)
                );
            }

            if (data_sources_.count(name + "_temperature")) {
                data_sources_[name + "_temperature"]->addPoint(
                    DataPoint(metrics.temperature_celsius)
                );
            }
        }
    }
}

void PerformanceDashboard::collectOptimizationData() {
    std::lock_guard<std::mutex> lock(data_sources_mutex_);

    for (const auto& [name, optimizer] : optimizers_) {
        auto state = optimizer->getCurrentState();

        if (data_sources_.count(name + "_objective")) {
            data_sources_[name + "_objective"]->addPoint(
                DataPoint(state.best_objective_value)
            );
        }

        if (data_sources_.count(name + "_iterations")) {
            data_sources_[name + "_iterations"]->addPoint(
                DataPoint(static_cast<double>(state.current_iteration))
            );
        }
    }
}

void PerformanceDashboard::updateWidgets() {
    std::lock_guard<std::mutex> widgets_lock(widgets_mutex_);
    std::lock_guard<std::mutex> data_lock(data_sources_mutex_);

    for (auto& [id, widget] : widgets_) {
        widget->updateData(data_sources_);
    }
}

void PerformanceDashboard::createWidgetsForLayout(const DashboardLayout& layout) {
    std::lock_guard<std::mutex> lock(widgets_mutex_);

    widgets_.clear();

    for (const auto& config : layout.widgets) {
        widgets_[config.id] = createWidget(config);
    }

    Logger::debug("Created {} widgets for layout: {}", widgets_.size(), layout.name);
}

std::unique_ptr<DashboardWidget> PerformanceDashboard::createWidget(const WidgetConfig& config) {
    switch (config.type) {
        case WidgetType::METRIC_CARD:
            return std::make_unique<MetricCardWidget>(config);
        case WidgetType::TIME_SERIES:
            return std::make_unique<TimeSeriesWidget>(config);
        case WidgetType::GAUGE:
            return std::make_unique<GaugeWidget>(config);
        case WidgetType::STATUS_INDICATOR:
            return std::make_unique<StatusIndicatorWidget>(config);
        case WidgetType::TABLE:
            return std::make_unique<DataTableWidget>(config);
        default:
            Logger::warn("Unsupported widget type: {}", static_cast<int>(config.type));
            return nullptr;
    }
}

// DashboardFactory implementation
std::unique_ptr<PerformanceDashboard> DashboardFactory::createMonitoringDashboard(
    int port, std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor) {

    auto dashboard = std::make_unique<PerformanceDashboard>(port);
    dashboard->addLayout(PerformanceDashboard::createMonitoringLayout());
    dashboard->setActiveLayout("monitoring");

    if (monitor) {
        dashboard->addMonitor("gpu0", monitor);
    }

    return dashboard;
}

std::unique_ptr<PerformanceDashboard> DashboardFactory::createOptimizationDashboard(
    int port, std::shared_ptr<puzzle71::optimization::AutomatedOptimizer> optimizer) {

    auto dashboard = std::make_unique<PerformanceDashboard>(port);
    dashboard->addLayout(PerformanceDashboard::createOptimizationLayout());
    dashboard->setActiveLayout("optimization");

    if (optimizer) {
        dashboard->addOptimizer("optimizer", optimizer);
    }

    return dashboard;
}

std::unique_ptr<PerformanceDashboard> DashboardFactory::createComprehensiveDashboard(
    int port, std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor,
    std::shared_ptr<puzzle71::optimization::AutomatedOptimizer> optimizer) {

    auto dashboard = std::make_unique<PerformanceDashboard>(port);
    dashboard->addLayout(PerformanceDashboard::createComprehensiveLayout());
    dashboard->setActiveLayout("comprehensive");

    if (monitor) {
        dashboard->addMonitor("gpu0", monitor);
    }

    if (optimizer) {
        dashboard->addOptimizer("optimizer", optimizer);
    }

    return dashboard;
}

WidgetConfig DashboardFactory::createMetricCard(
    const std::string& id, const std::string& title, const std::string& data_source,
    int x, int y, int width, int height) {

    WidgetConfig config;
    config.id = id;
    config.title = title;
    config.type = WidgetType::METRIC_CARD;
    config.data_source = data_source;
    config.x = x;
    config.y = y;
    config.width = width;
    config.height = height;
    config.update_frequency = UpdateFrequency::NORMAL;

    return config;
}

WidgetConfig DashboardFactory::createTimeSeries(
    const std::string& id, const std::string& title, const std::string& data_source,
    int x, int y, int width, int height) {

    WidgetConfig config;
    config.id = id;
    config.title = title;
    config.type = WidgetType::TIME_SERIES;
    config.data_source = data_source;
    config.x = x;
    config.y = y;
    config.width = width;
    config.height = height;
    config.update_frequency = UpdateFrequency::HIGH_FREQUENCY;

    return config;
}

WidgetConfig DashboardFactory::createGauge(
    const std::string& id, const std::string& title, const std::string& data_source,
    double max_value, int x, int y, int width, int height) {

    WidgetConfig config;
    config.id = id;
    config.title = title;
    config.type = WidgetType::GAUGE;
    config.data_source = data_source;
    config.max_value = max_value;
    config.x = x;
    config.y = y;
    config.width = width;
    config.height = height;
    config.update_frequency = UpdateFrequency::NORMAL;

    return config;
}

WidgetConfig DashboardFactory::createStatusIndicator(
    const std::string& id, const std::string& title, const std::string& data_source,
    int x, int y, int width, int height) {

    WidgetConfig config;
    config.id = id;
    config.title = title;
    config.type = WidgetType::STATUS_INDICATOR;
    config.data_source = data_source;
    config.x = x;
    config.y = y;
    config.width = width;
    config.height = height;
    config.update_frequency = UpdateFrequency::LOW_FREQUENCY;

    return config;
}

DashboardLayout PerformanceDashboard::createMonitoringLayout() {
    DashboardLayout layout;
    layout.name = "monitoring";
    layout.description = "Real-time GPU monitoring dashboard";
    layout.columns = 12;
    layout.rows = 8;

    layout.widgets.push_back(createMetricCard("throughput", "GPU Throughput", "gpu0_throughput", 0, 0, 3, 2));
    layout.widgets.push_back(createMetricCard("utilization", "GPU Utilization", "gpu0_utilization", 3, 0, 3, 2));
    layout.widgets.push_back(createMetricCard("temperature", "GPU Temperature", "gpu0_temperature", 6, 0, 3, 2));
    layout.widgets.push_back(createStatusIndicator("status", "System Status", "gpu0_utilization", 9, 0, 3, 2));

    layout.widgets.push_back(createTimeSeries("throughput_chart", "Throughput History", "gpu0_throughput", 0, 2, 6, 4));
    layout.widgets.push_back(createTimeSeries("utilization_chart", "Utilization History", "gpu0_utilization", 6, 2, 6, 4));

    layout.widgets.push_back(createGauge("memory_usage", "Memory Usage", "gpu0_utilization", 100.0, 0, 6, 4, 2));

    return layout;
}

DashboardLayout PerformanceDashboard::createOptimizationLayout() {
    DashboardLayout layout;
    layout.name = "optimization";
    layout.description = "Automated optimization progress dashboard";
    layout.columns = 12;
    layout.rows = 8;

    layout.widgets.push_back(createMetricCard("objective", "Best Objective", "optimizer_objective", 0, 0, 4, 2));
    layout.widgets.push_back(createMetricCard("iterations", "Iterations", "optimizer_iterations", 4, 0, 4, 2));
    layout.widgets.push_back(createStatusIndicator("convergence", "Convergence Status", "optimizer_objective", 8, 0, 4, 2));

    layout.widgets.push_back(createTimeSeries("objective_chart", "Objective Progress", "optimizer_objective", 0, 2, 8, 4));
    layout.widgets.push_back(createTimeSeries("convergence_chart", "Convergence Rate", "optimizer_objective", 8, 2, 4, 4));

    return layout;
}

DashboardLayout PerformanceDashboard::createComprehensiveLayout() {
    DashboardLayout layout;
    layout.name = "comprehensive";
    layout.description = "Comprehensive monitoring and optimization dashboard";
    layout.columns = 12;
    layout.rows = 10;

    // Top row - key metrics
    layout.widgets.push_back(createMetricCard("throughput", "Throughput", "gpu0_throughput", 0, 0, 2, 2));
    layout.widgets.push_back(createMetricCard("utilization", "GPU Utilization", "gpu0_utilization", 2, 0, 2, 2));
    layout.widgets.push_back(createMetricCard("temperature", "Temperature", "gpu0_temperature", 4, 0, 2, 2));
    layout.widgets.push_back(createMetricCard("objective", "Objective", "optimizer_objective", 6, 0, 2, 2));
    layout.widgets.push_back(createMetricCard("iterations", "Iterations", "optimizer_iterations", 8, 0, 2, 2));
    layout.widgets.push_back(createStatusIndicator("system_status", "Status", "gpu0_utilization", 10, 0, 2, 2));

    // Middle section - charts
    layout.widgets.push_back(createTimeSeries("throughput_chart", "Throughput History", "gpu0_throughput", 0, 2, 6, 4));
    layout.widgets.push_back(createTimeSeries("objective_chart", "Optimization Progress", "optimizer_objective", 6, 2, 6, 4));

    // Bottom section - gauges and status
    layout.widgets.push_back(createGauge("gpu_utilization_gauge", "GPU Utilization", "gpu0_utilization", 100.0, 0, 6, 4, 3));
    layout.widgets.push_back(createGauge("memory_usage_gauge", "Memory Usage", "gpu0_utilization", 100.0, 4, 6, 4, 3));
    layout.widgets.push_back(createGauge("temperature_gauge", "Temperature", "gpu0_temperature", 100.0, 8, 6, 4, 3));

    return layout;
}

// Dashboard utilities implementation
namespace dashboard_utils {

const DashboardTheme LIGHT_THEME = {
    "light",
    "#ffffff",
    "#333333",
    "#007bff",
    "#28a745",
    "#ffc107",
    "#dc3545",
    "#dee2e6",
    "#f8f9fa",
    {}
};

const DashboardTheme DARK_THEME = {
    "dark",
    "#1a1a1a",
    "#ffffff",
    "#0d6efd",
    "#198754",
    "#ffc107",
    "#dc3545",
    "#495057",
    "#212529",
    {}
};

std::string generateThemeCss(const DashboardTheme& theme) {
    std::ostringstream css;

    css << ":root {\n";
    css << "  --bg-color: " << theme.background_color << ";\n";
    css << "  --text-color: " << theme.text_color << ";\n";
    css << "  --primary-color: " << theme.primary_color << ";\n";
    css << "  --success-color: " << theme.success_color << ";\n";
    css << "  --warning-color: " << theme.warning_color << ";\n";
    css << "  --error-color: " << theme.error_color << ";\n";
    css << "  --border-color: " << theme.border_color << ";\n";
    css << "  --card-bg: " << theme.card_background << ";\n";
    css << "}\n";

    return css.str();
}

DashboardLayout createGpuMonitoringTemplate() {
    return PerformanceDashboard::createMonitoringLayout();
}

DashboardLayout createOptimizationProgressTemplate() {
    return PerformanceDashboard::createOptimizationLayout();
}

DashboardLayout createPerformanceAnalysisTemplate() {
    DashboardLayout layout;
    layout.name = "performance_analysis";
    layout.description = "Performance analysis and insights";
    layout.columns = 12;
    layout.rows = 8;

    // Add analysis-specific widgets
    layout.widgets.push_back(DashboardFactory::createMetricCard("peak_throughput", "Peak Throughput", "throughput", 0, 0, 3, 2));
    layout.widgets.push_back(DashboardFactory::createMetricCard("avg_throughput", "Avg Throughput", "throughput", 3, 0, 3, 2));
    layout.widgets.push_back(DashboardFactory::createTimeSeries("performance_trend", "Performance Trend", "throughput", 0, 2, 12, 6));

    return layout;
}

DashboardLayout createSystemHealthTemplate() {
    DashboardLayout layout;
    layout.name = "system_health";
    layout.description = "System health and diagnostics";
    layout.columns = 12;
    layout.rows = 6;

    layout.widgets.push_back(DashboardFactory::createStatusIndicator("gpu_health", "GPU Health", "gpu0_utilization", 0, 0, 3, 2));
    layout.widgets.push_back(DashboardFactory::createStatusIndicator("memory_health", "Memory Health", "gpu0_utilization", 3, 0, 3, 2));
    layout.widgets.push_back(DashboardFactory::createStatusIndicator("thermal_health", "Thermal Health", "gpu0_temperature", 6, 0, 3, 2));
    layout.widgets.push_back(DashboardFactory::createStatusIndicator("overall_health", "Overall Health", "gpu0_utilization", 9, 0, 3, 2));

    return layout;
}

} // namespace dashboard_utils

} // namespace puzzle71::dashboard
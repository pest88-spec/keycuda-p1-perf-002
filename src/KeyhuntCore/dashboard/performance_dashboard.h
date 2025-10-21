// Puzzle71Solver - Performance Dashboard and Visualization (T047)
// Phase 6: User Story 4 - Performance Monitoring
// Web-based performance dashboard with real-time metrics visualization

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "../monitoring/real_time_monitor.h"
#include "../optimization/automated_optimizer.h"

namespace puzzle71::dashboard {

/**
 * @brief Dashboard widget types
 */
enum class WidgetType {
    METRIC_CARD,        // Single metric display
    TIME_SERIES,        // Line chart over time
    GAUGE,              // Gauge meter
    BAR_CHART,          // Bar chart
    HEAT_MAP,           // Heat map visualization
    TABLE,              // Data table
    PROGRESS_BAR,       // Progress bar
    STATUS_INDICATOR,   // Status indicator
    CUSTOM_WIDGET       // Custom widget
};

/**
 * @brief Dashboard data update frequency
 */
enum class UpdateFrequency {
    REAL_TIME,          // Update every 100ms
    HIGH_FREQUENCY,     // Update every 500ms
    NORMAL,             // Update every 1 second
    LOW_FREQUENCY,      // Update every 5 seconds
    ON_DEMAND           // Update only when requested
};

/**
 * @brief Dashboard widget configuration
 */
struct WidgetConfig {
    std::string id;
    std::string title;
    WidgetType type;
    UpdateFrequency update_frequency{UpdateFrequency::NORMAL};

    // Position and size
    int x{0}, y{0};
    int width{4}, height{3};  // Grid units (12 columns, 8 rows default)

    // Data source
    std::string data_source;
    std::map<std::string, std::string> data_parameters;

    // Visual settings
    std::map<std::string, std::string> style_settings;
    std::string color_scheme{"default"};
    bool show_legend{true};
    bool show_title{true};

    // Data limits
    double min_value{0.0};
    double max_value{100.0};
    std::string unit{"";

    // Custom settings
    std::map<std::string, std::string> custom_settings;
};

/**
 * @brief Dashboard layout configuration
 */
struct DashboardLayout {
    std::string name;
    std::string description;
    int columns{12};
    int rows{8};
    std::vector<WidgetConfig> widgets;
    std::string theme{"light"};
    bool auto_refresh{true};
    std::chrono::seconds refresh_interval{std::chrono::seconds(5)};
};

/**
 * @brief Real-time data point
 */
struct DataPoint {
    std::chrono::high_resolution_clock::time_point timestamp;
    double value;
    std::map<std::string, std::string> metadata;

    DataPoint(double val, const std::map<std::string, std::string>& meta = {})
        : timestamp(std::chrono::high_resolution_clock::now()), value(val), metadata(meta) {}
};

/**
 * @brief Time series data container
 */
class TimeSeriesData {
public:
    void addPoint(const DataPoint& point);
    std::vector<DataPoint> getRecentPoints(std::chrono::seconds duration) const;
    std::vector<DataPoint> getRecentPoints(size_t count) const;
    void clear();
    size_t size() const { return data_.size(); }

    double getLatestValue() const;
    double getAverageValue(std::chrono::seconds duration) const;
    double getMinValue(std::chrono::seconds duration) const;
    double getMaxValue(std::chrono::seconds duration) const;

private:
    std::vector<DataPoint> data_;
    mutable std::mutex data_mutex_;
    size_t max_points_{10000};
};

/**
 * @brief Dashboard widget base class
 */
class DashboardWidget {
public:
    explicit DashboardWidget(const WidgetConfig& config);
    virtual ~DashboardWidget() = default;

    virtual std::string renderHtml() const = 0;
    virtual std::string renderJavaScript() const { return ""; }
    virtual void updateData(const std::map<std::string, TimeSeriesData>& data_sources) = 0;

    const WidgetConfig& getConfig() const { return config_; }
    const std::string& getId() const { return config_.id; }

protected:
    WidgetConfig config_;
    TimeSeriesData* data_source_{nullptr};

    std::string formatValue(double value) const;
    std::string getColorForValue(double value) const;
    std::string getProgressBarHtml(double value, double max = 100.0) const;
};

/**
 * @brief Metric card widget - displays a single key metric
 */
class MetricCardWidget : public DashboardWidget {
public:
    explicit MetricCardWidget(const WidgetConfig& config);

    std::string renderHtml() const override;
    void updateData(const std::map<std::string, TimeSeriesData>& data_sources) override;

private:
    std::string current_value_;
    std::string trend_indicator_;
    std::string status_class_;
};

/**
 * @brief Time series chart widget
 */
class TimeSeriesWidget : public DashboardWidget {
public:
    explicit TimeSeriesWidget(const WidgetConfig& config);

    std::string renderHtml() const override;
    std::string renderJavaScript() const override;
    void updateData(const std::map<std::string, TimeSeriesData>& data_sources) override;

private:
    std::vector<std::string> data_series_;
    std::string chart_type_{"line"};
};

/**
 * @brief Gauge widget for showing percentage or single values
 */
class GaugeWidget : public DashboardWidget {
public:
    explicit GaugeWidget(const WidgetConfig& config);

    std::string renderHtml() const override;
    std::string renderJavaScript() const override;
    void updateData(const std::map<std::string, TimeSeriesData>& data_sources) override;

private:
    double current_value_{0.0};
    std::vector<std::pair<double, std::string>> zones_;
};

/**
 * @brief Bar chart widget
 */
class BarChartWidget : public DashboardWidget {
public:
    explicit BarChartWidget(const WidgetConfig& config);

    std::string renderHtml() const override;
    std::string renderJavaScript() const override;
    void updateData(const std::map<std::string, TimeSeriesData>& data_sources) override;

private:
    std::vector<std::pair<std::string, double>> bar_data_;
    std::string orientation_{"vertical"};
};

/**
 * @brief Status indicator widget
 */
class StatusIndicatorWidget : public DashboardWidget {
public:
    explicit StatusIndicatorWidget(const WidgetConfig& config);

    std::string renderHtml() const override;
    void updateData(const std::map<std::string, TimeSeriesData>& data_sources) override;

private:
    std::string current_status_{"unknown"};
    std::string status_message_{"No data"};
    std::map<std::string, std::string> status_colors_;
};

/**
 * @brief Data table widget
 */
class DataTableWidget : public DashboardWidget {
public:
    explicit DataTableWidget(const WidgetConfig& config);

    std::string renderHtml() const override;
    std::string renderJavaScript() const override;
    void updateData(const std::map<std::string, TimeSeriesData>& data_sources) override;

private:
    std::vector<std::string> columns_;
    std::vector<std::vector<std::string>> table_data_;
    size_t max_rows_{10};
    bool sortable_{true};
};

/**
 * @brief Performance dashboard server
 */
class PerformanceDashboard {
public:
    explicit PerformanceDashboard(int port = 8080);
    ~PerformanceDashboard();

    // Dashboard configuration
    void addLayout(const DashboardLayout& layout);
    void setActiveLayout(const std::string& layout_name);
    std::vector<std::string> getAvailableLayouts() const;

    // Data sources
    void addDataSource(const std::string& name, std::shared_ptr<TimeSeriesData> data);
    void addMonitor(const std::string& name, std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor);
    void addOptimizer(const std::string& name, std::shared_ptr<puzzle71::optimization::AutomatedOptimizer> optimizer);

    // Server control
    bool start();
    void stop();
    bool isRunning() const { return server_running_.load(); }

    // Configuration
    void setUpdateInterval(std::chrono::milliseconds interval);
    void setMaxDataPoints(size_t max_points);

    // Built-in layouts
    static DashboardLayout createDefaultLayout();
    static DashboardLayout createMonitoringLayout();
    static DashboardLayout createOptimizationLayout();
    static DashboardLayout createComprehensiveLayout();

private:
    int port_;
    std::atomic<bool> server_running_{false};
    std::unique_ptr<std::thread> server_thread_;

    // Dashboard data
    std::map<std::string, DashboardLayout> layouts_;
    std::string active_layout_name_;
    mutable std::mutex layouts_mutex_;

    // Data sources
    std::map<std::string, std::shared_ptr<TimeSeriesData>> data_sources_;
    std::map<std::string, std::shared_ptr<puzzle71::monitoring::RealTimeMonitor>> monitors_;
    std::map<std::string, std::shared_ptr<puzzle71::optimization::AutomatedOptimizer>> optimizers_;
    mutable std::mutex data_sources_mutex_;

    // Widgets
    std::map<std::string, std::unique_ptr<DashboardWidget>> widgets_;
    mutable std::mutex widgets_mutex_;

    // Update configuration
    std::chrono::milliseconds update_interval_{std::chrono::milliseconds(1000)};
    size_t max_data_points_{10000};
    std::atomic<bool> update_thread_running_{false};
    std::unique_ptr<std::thread> update_thread_;

    // Server implementation
    void serverLoop();
    void handleRequest(const std::string& method, const std::string& path,
                     const std::string& body, std::string& response);

    // Route handlers
    void handleRootRequest(std::string& response);
    void handleDashboardRequest(std::string& response);
    void handleDataRequest(const std::string& data_source, std::string& response);
    void handleWidgetRequest(const std::string& widget_id, std::string& response);
    void handleLayoutRequest(const std::string& layout_name, std::string& response);
    void handleConfigRequest(std::string& response);
    void handleApiRequest(const std::string& endpoint, const std::string& body, std::string& response);

    // Data updates
    void updateDataLoop();
    void updateWidgets();
    void collectMonitoringData();
    void collectOptimizationData();

    // Widget creation
    std::unique_ptr<DashboardWidget> createWidget(const WidgetConfig& config);
    void createWidgetsForLayout(const DashboardLayout& layout);

    // HTML generation
    std::string generateMainPage() const;
    std::string generateDashboardHtml() const;
    std::string generateNavigationHtml() const;
    std::string generateLayoutSelectorHtml() const;
    std::string generateWidgetGridHtml() const;

    // JavaScript generation
    std::string generateDashboardJavaScript() const;
    std::string generateUpdateJavaScript() const;
    std::string generateWidgetJavaScript() const;
    std::string generateChartLibraryIncludes() const;

    // Utility methods
    std::string getContentType(const std::string& path) const;
    std::string urlDecode(const std::string& encoded) const;
    std::map<std::string, std::string> parseQueryString(const std::string& query) const;
    std::string generateJsonResponse(const std::map<std::string, std::string>& data) const;

    // Static resources
    std::string getStaticResource(const std::string& path) const;
    bool isStaticResource(const std::string& path) const;
};

/**
 * @brief Dashboard factory for creating pre-configured dashboards
 */
class DashboardFactory {
public:
    /**
     * @brief Create basic monitoring dashboard
     */
    static std::unique_ptr<PerformanceDashboard> createMonitoringDashboard(
        int port = 8080,
        std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor = nullptr
    );

    /**
     * @brief Create optimization dashboard
     */
    static std::unique_ptr<PerformanceDashboard> createOptimizationDashboard(
        int port = 8081,
        std::shared_ptr<puzzle71::optimization::AutomatedOptimizer> optimizer = nullptr
    );

    /**
     * @brief Create comprehensive dashboard with monitoring and optimization
     */
    static std::unique_ptr<PerformanceDashboard> createComprehensiveDashboard(
        int port = 8082,
        std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor = nullptr,
        std::shared_ptr<puzzle71::optimization::AutomatedOptimizer> optimizer = nullptr
    );

    /**
     * @brief Create custom dashboard with specified layout
     */
    static std::unique_ptr<PerformanceDashboard> createCustomDashboard(
        const DashboardLayout& layout,
        int port = 8080
    );

    /**
     * @brief Create widget configuration
     */
    static WidgetConfig createMetricCard(
        const std::string& id,
        const std::string& title,
        const std::string& data_source,
        int x = 0, int y = 0,
        int width = 3, int height = 2
    );

    static WidgetConfig createTimeSeries(
        const std::string& id,
        const std::string& title,
        const std::string& data_source,
        int x = 0, int y = 0,
        int width = 6, int height = 3
    );

    static WidgetConfig createGauge(
        const std::string& id,
        const std::string& title,
        const std::string& data_source,
        double max_value = 100.0,
        int x = 0, int y = 0,
        int width = 3, int height = 3
    );

    static WidgetConfig createStatusIndicator(
        const std::string& id,
        const std::string& title,
        const std::string& data_source,
        int x = 0, int y = 0,
        int width = 2, int height = 2
    );
};

/**
 * @brief Dashboard configuration and theme utilities
 */
namespace dashboard_utils {

/**
 * @brief Dashboard theme configuration
 */
struct DashboardTheme {
    std::string name;
    std::string background_color;
    std::string text_color;
    std::string primary_color;
    std::string success_color;
    std::string warning_color;
    std::string error_color;
    std::string border_color;
    std::string card_background;
    std::map<std::string, std::string> custom_colors;
};

/**
 * @brief Predefined themes
 */
extern const DashboardTheme LIGHT_THEME;
extern const DashboardTheme DARK_THEME;
extern const DashboardTheme HIGH_CONTRAST_THEME;
extern const DashboardTheme CORPORATE_THEME;

/**
 * @brief Theme utilities
 */
std::string generateThemeCss(const DashboardTheme& theme);
std::string applyThemeToWidget(const WidgetConfig& widget, const DashboardTheme& theme);
DashboardTheme loadThemeFromFile(const std::string& theme_file);
bool saveThemeToFile(const DashboardTheme& theme, const std::string& theme_file);

/**
 * @brief Export utilities
 */
std::string exportDashboardConfig(const PerformanceDashboard& dashboard);
bool importDashboardConfig(PerformanceDashboard& dashboard, const std::string& config);
std::string exportLayoutToHtml(const DashboardLayout& layout);
bool exportDashboardToHtml(const PerformanceDashboard& dashboard, const std::string& filename);

/**
 * @brief Widget templates
 */
DashboardLayout createGpuMonitoringTemplate();
DashboardLayout createOptimizationProgressTemplate();
DashboardLayout createPerformanceAnalysisTemplate();
DashboardLayout createSystemHealthTemplate();
DashboardLayout createMultiDeviceTemplate(const std::vector<int>& device_ids);

} // namespace dashboard_utils

} // namespace puzzle71::dashboard
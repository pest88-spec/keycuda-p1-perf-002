// Puzzle71Solver Performance Dashboard JavaScript
// Interactive web dashboard with real-time updates and responsive design

class PerformanceDashboard {
    constructor(config = {}) {
        this.config = {
            updateInterval: config.updateInterval || 1000,
            maxDataPoints: config.maxDataPoints || 100,
            theme: config.theme || 'light',
            autoRefresh: config.autoRefresh !== false,
            ...config
        };

        this.widgets = new Map();
        this.dataSources = new Map();
        this.updateTimer = null;
        this.isInitialized = false;

        this.init();
    }

    init() {
        this.setupTheme();
        this.setupEventListeners();
        this.setupWebSocket(); // For real-time updates
        this.startAutoRefresh();

        this.isInitialized = true;
        console.log('Performance Dashboard initialized');
    }

    setupTheme() {
        const savedTheme = localStorage.getItem('dashboard-theme') || this.config.theme;
        this.setTheme(savedTheme);
    }

    setTheme(theme) {
        document.documentElement.setAttribute('data-theme', theme);
        localStorage.setItem('dashboard-theme', theme);
        this.config.theme = theme;

        // Update theme toggle button
        const themeToggle = document.getElementById('theme-toggle');
        if (themeToggle) {
            themeToggle.textContent = theme === 'dark' ? '☀️ Light' : '🌙 Dark';
        }
    }

    toggleTheme() {
        const newTheme = this.config.theme === 'light' ? 'dark' : 'light';
        this.setTheme(newTheme);
    }

    setupEventListeners() {
        // Theme toggle
        const themeToggle = document.getElementById('theme-toggle');
        if (themeToggle) {
            themeToggle.addEventListener('click', () => this.toggleTheme());
        }

        // Layout selector
        const layoutSelect = document.getElementById('layout-selector');
        if (layoutSelect) {
            layoutSelect.addEventListener('change', (e) => {
                this.loadLayout(e.target.value);
            });
        }

        // Refresh button
        const refreshBtn = document.getElementById('refresh-btn');
        if (refreshBtn) {
            refreshBtn.addEventListener('click', () => this.refreshAllWidgets());
        }

        // Auto-refresh toggle
        const autoRefreshToggle = document.getElementById('auto-refresh-toggle');
        if (autoRefreshToggle) {
            autoRefreshToggle.addEventListener('change', (e) => {
                this.config.autoRefresh = e.target.checked;
                if (this.config.autoRefresh) {
                    this.startAutoRefresh();
                } else {
                    this.stopAutoRefresh();
                }
            });
        }

        // Window resize
        window.addEventListener('resize', this.debounce(() => {
            this.handleResize();
        }, 250));

        // Keyboard shortcuts
        document.addEventListener('keydown', (e) => {
            this.handleKeyboardShortcuts(e);
        });
    }

    setupWebSocket() {
        // WebSocket connection for real-time updates
        // In production, connect to actual WebSocket server
        try {
            this.ws = new WebSocket(`ws://${window.location.host}/ws`);

            this.ws.onopen = () => {
                console.log('WebSocket connected');
                this.showConnectionStatus('connected');
            };

            this.ws.onmessage = (event) => {
                const data = JSON.parse(event.data);
                this.handleWebSocketMessage(data);
            };

            this.ws.onclose = () => {
                console.log('WebSocket disconnected');
                this.showConnectionStatus('disconnected');
                // Attempt to reconnect after 5 seconds
                setTimeout(() => this.setupWebSocket(), 5000);
            };

            this.ws.onerror = (error) => {
                console.error('WebSocket error:', error);
                this.showConnectionStatus('error');
            };

        } catch (error) {
            console.warn('WebSocket not available, falling back to polling');
            this.startPolling();
        }
    }

    startPolling() {
        // Fallback polling mechanism
        this.pollingTimer = setInterval(() => {
            this.fetchUpdates();
        }, this.config.updateInterval);
    }

    fetchUpdates() {
        // Fetch latest data from server
        fetch('/api/data/latest')
            .then(response => response.json())
            .then(data => {
                this.updateDashboard(data);
            })
            .catch(error => {
                console.error('Error fetching updates:', error);
            });
    }

    handleWebSocketMessage(data) {
        switch (data.type) {
            case 'widget_update':
                this.updateWidget(data.widgetId, data.data);
                break;
            case 'data_update':
                this.updateDataSource(data.source, data.data);
                break;
            case 'alert':
                this.showAlert(data.message, data.level);
                break;
            default:
                console.log('Unknown message type:', data.type);
        }
    }

    startAutoRefresh() {
        if (this.updateTimer) {
            clearInterval(this.updateTimer);
        }

        this.updateTimer = setInterval(() => {
            this.refreshAllWidgets();
        }, this.config.updateInterval);
    }

    stopAutoRefresh() {
        if (this.updateTimer) {
            clearInterval(this.updateTimer);
            this.updateTimer = null;
        }
    }

    refreshAllWidgets() {
        this.widgets.forEach((widget, id) => {
            this.refreshWidget(id);
        });

        this.updateLastRefreshTime();
    }

    refreshWidget(widgetId) {
        const widget = this.widgets.get(widgetId);
        if (!widget) return;

        const widgetElement = document.getElementById(widgetId);
        if (!widgetElement) return;

        // Show loading state
        widgetElement.classList.add('loading');

        // Fetch widget data
        fetch(`/api/widgets/${widgetId}/data`)
            .then(response => response.json())
            .then(data => {
                this.updateWidget(widgetId, data);
                widgetElement.classList.remove('loading');
            })
            .catch(error => {
                console.error(`Error refreshing widget ${widgetId}:`, error);
                widgetElement.classList.remove('loading');
                this.showWidgetError(widgetId, error.message);
            });
    }

    updateWidget(widgetId, data) {
        const widget = this.widgets.get(widgetId);
        if (!widget) return;

        const widgetElement = document.getElementById(widgetId);
        if (!widgetElement) return;

        switch (widget.type) {
            case 'metric_card':
                this.updateMetricCard(widgetElement, data);
                break;
            case 'time_series':
                this.updateTimeSeries(widgetElement, data);
                break;
            case 'gauge':
                this.updateGauge(widgetElement, data);
                break;
            case 'status_indicator':
                this.updateStatusIndicator(widgetElement, data);
                break;
            case 'data_table':
                this.updateDataTable(widgetElement, data);
                break;
        }
    }

    updateMetricCard(element, data) {
        const valueElement = element.querySelector('.metric-value');
        const trendElement = element.querySelector('.metric-trend');

        if (valueElement && data.value !== undefined) {
            const oldValue = parseFloat(valueElement.textContent.replace(/[^0-9.-]/g, ''));
            const newValue = parseFloat(data.value);

            valueElement.textContent = this.formatValue(data.value, data.unit || '');

            // Update trend indicator
            if (trendElement && !isNaN(oldValue) && !isNaN(newValue)) {
                const change = ((newValue - oldValue) / oldValue) * 100;
                const trendClass = change > 5 ? 'status-good' :
                                  change < -5 ? 'status-bad' : 'status-normal';

                valueElement.className = `metric-value ${trendClass}`;

                const arrow = change > 0 ? '↑' : change < 0 ? '↓' : '→';
                trendElement.textContent = `${arrow} ${Math.abs(change).toFixed(1)}%`;
            }

            // Add animation
            valueElement.classList.add('fade-in');
            setTimeout(() => valueElement.classList.remove('fade-in'), 300);
        }
    }

    updateTimeSeries(element, data) {
        const canvas = element.querySelector('canvas');
        if (!canvas || !data.points) return;

        // Update chart using Chart.js or custom canvas drawing
        this.drawTimeSeriesChart(canvas, data.points, data.options || {});
    }

    updateGauge(element, data) {
        const gaugeFunction = window[`updateGauge_${element.id}`];
        if (typeof gaugeFunction === 'function' && data.value !== undefined) {
            gaugeFunction(data.value);
        }
    }

    updateStatusIndicator(element, data) {
        const statusLight = element.querySelector('.status-light');
        const statusText = element.querySelector('.status-text');

        if (statusLight && data.status) {
            statusLight.className = `status-light status-${data.status}`;
        }

        if (statusText && data.message) {
            statusText.textContent = data.message;
        }
    }

    updateDataTable(element, data) {
        const tbody = element.querySelector('tbody');
        if (!tbody || !data.rows) return;

        tbody.innerHTML = '';

        data.rows.forEach(row => {
            const tr = document.createElement('tr');
            row.forEach(cell => {
                const td = document.createElement('td');
                td.textContent = cell;
                tr.appendChild(td);
            });
            tbody.appendChild(tr);
        });
    }

    drawTimeSeriesChart(canvas, points, options = {}) {
        const ctx = canvas.getContext('2d');
        const rect = canvas.getBoundingClientRect();
        canvas.width = rect.width;
        canvas.height = rect.height;

        const padding = 40;
        const width = canvas.width - 2 * padding;
        const height = canvas.height - 2 * padding;

        // Clear canvas
        ctx.clearRect(0, 0, canvas.width, canvas.height);

        if (points.length === 0) return;

        // Find data range
        const values = points.map(p => p.value);
        const minValue = Math.min(...values);
        const maxValue = Math.max(...values);
        const valueRange = maxValue - minValue || 1;

        // Draw grid
        ctx.strokeStyle = getComputedStyle(document.documentElement).getPropertyValue('--border-color');
        ctx.lineWidth = 1;
        ctx.setLineDash([5, 5]);

        // Horizontal grid lines
        for (let i = 0; i <= 5; i++) {
            const y = padding + (i / 5) * height;
            ctx.beginPath();
            ctx.moveTo(padding, y);
            ctx.lineTo(canvas.width - padding, y);
            ctx.stroke();
        }

        // Vertical grid lines
        for (let i = 0; i <= 5; i++) {
            const x = padding + (i / 5) * width;
            ctx.beginPath();
            ctx.moveTo(x, padding);
            ctx.lineTo(x, canvas.height - padding);
            ctx.stroke();
        }

        ctx.setLineDash([]);

        // Draw axes
        ctx.strokeStyle = getComputedStyle(document.documentElement).getPropertyValue('--text-secondary');
        ctx.beginPath();
        ctx.moveTo(padding, padding);
        ctx.lineTo(padding, canvas.height - padding);
        ctx.lineTo(canvas.width - padding, canvas.height - padding);
        ctx.stroke();

        // Draw data line
        ctx.strokeStyle = options.color || getComputedStyle(document.documentElement).getPropertyValue('--chart-primary');
        ctx.lineWidth = 2;
        ctx.beginPath();

        points.forEach((point, index) => {
            const x = padding + (index / (points.length - 1)) * width;
            const y = canvas.height - padding - ((point.value - minValue) / valueRange) * height;

            if (index === 0) {
                ctx.moveTo(x, y);
            } else {
                ctx.lineTo(x, y);
            }
        });

        ctx.stroke();

        // Draw data points
        ctx.fillStyle = ctx.strokeStyle;
        points.forEach((point, index) => {
            const x = padding + (index / (points.length - 1)) * width;
            const y = canvas.height - padding - ((point.value - minValue) / valueRange) * height;

            ctx.beginPath();
            ctx.arc(x, y, 3, 0, 2 * Math.PI);
            ctx.fill();
        });

        // Draw labels
        ctx.fillStyle = getComputedStyle(document.documentElement).getPropertyValue('--text-secondary');
        ctx.font = '12px sans-serif';
        ctx.textAlign = 'center';

        // X-axis labels (time)
        const labelInterval = Math.ceil(points.length / 5);
        points.forEach((point, index) => {
            if (index % labelInterval === 0) {
                const x = padding + (index / (points.length - 1)) * width;
                const time = new Date(point.timestamp);
                ctx.fillText(time.toLocaleTimeString(), x, canvas.height - padding + 20);
            }
        });

        // Y-axis labels
        ctx.textAlign = 'right';
        for (let i = 0; i <= 5; i++) {
            const value = minValue + (i / 5) * valueRange;
            const y = canvas.height - padding - (i / 5) * height;
            ctx.fillText(this.formatValue(value), padding - 10, y + 4);
        }
    }

    formatValue(value, unit = '') {
        if (typeof value !== 'number') return value;

        let formattedValue;
        if (value >= 1000000000) {
            formattedValue = (value / 1000000000).toFixed(2) + 'G';
        } else if (value >= 1000000) {
            formattedValue = (value / 1000000).toFixed(2) + 'M';
        } else if (value >= 1000) {
            formattedValue = (value / 1000).toFixed(2) + 'K';
        } else {
            formattedValue = value.toFixed(2);
        }

        return unit ? `${formattedValue} ${unit}` : formattedValue;
    }

    loadLayout(layoutName) {
        fetch(`/api/layouts/${layoutName}`)
            .then(response => response.json())
            .then(layout => {
                this.renderLayout(layout);
                this.showNotification(`Layout "${layoutName}" loaded`, 'success');
            })
            .catch(error => {
                console.error('Error loading layout:', error);
                this.showNotification(`Failed to load layout "${layoutName}"`, 'error');
            });
    }

    renderLayout(layout) {
        const gridContainer = document.querySelector('.widget-grid');
        if (!gridContainer) return;

        // Clear existing widgets
        gridContainer.innerHTML = '';

        // Update grid layout
        gridContainer.style.gridTemplateColumns = `repeat(${layout.columns || 12}, 1fr)`;

        // Add widgets
        layout.widgets.forEach(widgetConfig => {
            const widgetElement = this.createWidgetElement(widgetConfig);
            gridContainer.appendChild(widgetElement);
        });

        // Initialize widgets
        setTimeout(() => {
            this.refreshAllWidgets();
        }, 100);
    }

    createWidgetElement(config) {
        const element = document.createElement('div');
        element.id = config.id;
        element.className = `widget ${config.type}`;
        element.style.gridColumn = `span ${config.width || 4}`;
        element.style.gridRow = `span ${config.height || 3}`;

        // Store widget configuration
        this.widgets.set(config.id, config);

        return element;
    }

    showConnectionStatus(status) {
        const statusElement = document.getElementById('connection-status');
        if (!statusElement) return;

        const statusConfig = {
            connected: { text: 'Connected', class: 'status-good' },
            disconnected: { text: 'Disconnected', class: 'status-warning' },
            error: { text: 'Error', class: 'status-bad' }
        };

        const config = statusConfig[status] || statusConfig.disconnected;
        statusElement.textContent = config.text;
        statusElement.className = `status-indicator ${config.class}`;
    }

    showAlert(message, level = 'info') {
        const alertContainer = document.getElementById('alert-container') || this.createAlertContainer();

        const alert = document.createElement('div');
        alert.className = `alert alert-${level}`;
        alert.innerHTML = `
            <span class="alert-message">${message}</span>
            <button class="alert-close" onclick="this.parentElement.remove()">×</button>
        `;

        alertContainer.appendChild(alert);

        // Auto-remove after 5 seconds
        setTimeout(() => {
            if (alert.parentElement) {
                alert.remove();
            }
        }, 5000);
    }

    showNotification(message, type = 'info') {
        // Use browser notification API if available
        if ('Notification' in window && Notification.permission === 'granted') {
            new Notification('Performance Dashboard', {
                body: message,
                icon: '/favicon.ico'
            });
        } else {
            this.showAlert(message, type);
        }
    }

    showWidgetError(widgetId, error) {
        const widgetElement = document.getElementById(widgetId);
        if (!widgetElement) return;

        const errorElement = document.createElement('div');
        errorElement.className = 'widget-error';
        errorElement.innerHTML = `
            <div class="error-icon">⚠️</div>
            <div class="error-message">${error}</div>
            <button class="retry-btn" onclick="dashboard.refreshWidget('${widgetId}')">Retry</button>
        `;

        widgetElement.appendChild(errorElement);

        // Auto-remove after 10 seconds
        setTimeout(() => {
            if (errorElement.parentElement) {
                errorElement.remove();
            }
        }, 10000);
    }

    createAlertContainer() {
        const container = document.createElement('div');
        container.id = 'alert-container';
        container.className = 'alert-container';
        document.body.appendChild(container);
        return container;
    }

    updateLastRefreshTime() {
        const refreshElement = document.getElementById('last-refresh');
        if (refreshElement) {
            refreshElement.textContent = `Last updated: ${new Date().toLocaleTimeString()}`;
        }
    }

    handleResize() {
        // Re-render charts on resize
        this.widgets.forEach((widget, id) => {
            if (widget.type === 'time_series') {
                this.refreshWidget(id);
            }
        });
    }

    handleKeyboardShortcuts(event) {
        // Ctrl/Cmd + R: Refresh all widgets
        if ((event.ctrlKey || event.metaKey) && event.key === 'r') {
            event.preventDefault();
            this.refreshAllWidgets();
        }

        // Ctrl/Cmd + T: Toggle theme
        if ((event.ctrlKey || event.metaKey) && event.key === 't') {
            event.preventDefault();
            this.toggleTheme();
        }

        // Ctrl/Cmd + E: Toggle auto-refresh
        if ((event.ctrlKey || event.metaKey) && event.key === 'e') {
            event.preventDefault();
            this.config.autoRefresh = !this.config.autoRefresh;
            if (this.config.autoRefresh) {
                this.startAutoRefresh();
            } else {
                this.stopAutoRefresh();
            }
        }
    }

    debounce(func, wait) {
        let timeout;
        return function executedFunction(...args) {
            const later = () => {
                clearTimeout(timeout);
                func(...args);
            };
            clearTimeout(timeout);
            timeout = setTimeout(later, wait);
        };
    }

    // Public API methods
    addDataSource(name, dataSource) {
        this.dataSources.set(name, dataSource);
    }

    removeDataSource(name) {
        this.dataSources.delete(name);
    }

    exportDashboardData() {
        const data = {
            widgets: Array.from(this.widgets.entries()),
            dataSources: Array.from(this.dataSources.entries()),
            config: this.config,
            timestamp: new Date().toISOString()
        };

        const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
        const url = URL.createObjectURL(blob);

        const a = document.createElement('a');
        a.href = url;
        a.download = `dashboard-export-${new Date().toISOString().split('T')[0]}.json`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }

    destroy() {
        this.stopAutoRefresh();

        if (this.updateTimer) {
            clearInterval(this.updateTimer);
        }

        if (this.pollingTimer) {
            clearInterval(this.pollingTimer);
        }

        if (this.ws) {
            this.ws.close();
        }

        this.widgets.clear();
        this.dataSources.clear();
    }
}

// Initialize dashboard when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    window.dashboard = new PerformanceDashboard({
        updateInterval: 1000,
        maxDataPoints: 100,
        theme: localStorage.getItem('dashboard-theme') || 'light',
        autoRefresh: true
    });

    // Request notification permission
    if ('Notification' in window && Notification.permission === 'default') {
        Notification.requestPermission();
    }

    console.log('Dashboard ready');
});

// Export for external use
window.PerformanceDashboard = PerformanceDashboard;
#pragma once

#include "GaugeWidget.hpp"

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QStatusBar>
#include <QString>

/**
 * @file DashboardWindow.hpp
 * @brief Main application window — clean operator dashboard
 *
 * Layout:
 *   ┌─────────────────────────────────────┐
 *   │  🍓 RPi System Monitor     ● LIVE   │  ← Header
 *   │   vSOME/IP | 192.168.x.x            │
 *   ├─────────────────────────────────────┤
 *   │  [CPU Load] [CPU Temp] [RAM Usage]  │  ← Gauges
 *   ├─────────────────────────────────────┤
 *   │  🔄 500ms   📁 telemetry.log        │  ← Status bar
 *   └─────────────────────────────────────┘
 *
 * No log panel. No terminal text. Just gauges.
 */
class DashboardWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DashboardWindow(QWidget* parent = nullptr);
    ~DashboardWindow() override = default;

public slots:
    /**
     * @brief Update CPU Load gauge (0-100%)
     */
    void setCpuLoad(double pct);

    /**
     * @brief Update CPU Temperature gauge (0-100°C)
     */
    void setCpuTemp(double celsius);

    /**
     * @brief Update RAM Usage gauge (0-100%)
     */
    void setRamUsage(double pct);

    /**
     * @brief Called when vSOME/IP connects
     */
    void onConnected(const QString& host);

    /**
     * @brief Called when vSOME/IP disconnects
     */
    void onDisconnected();

private:
    void setupUi();
    void setupHeader();
    void setupGauges();
    void setupStatusBar();
    void applyStyleSheet();

    // Header
    QLabel*      m_titleLabel    = nullptr;
    QLabel*      m_connBadge     = nullptr;
    QLabel*      m_hostLabel     = nullptr;

    // Gauges
    GaugeWidget* m_cpuLoadGauge  = nullptr;
    GaugeWidget* m_cpuTempGauge  = nullptr;
    GaugeWidget* m_ramGauge      = nullptr;

    // Status bar
    QLabel*      m_statusLabel   = nullptr;
};

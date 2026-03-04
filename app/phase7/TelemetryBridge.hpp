#pragma once

/**
 * @file TelemetryBridge.hpp
 * @brief Qt6 bridge between the C++ logging system and the GUI dashboard
 *
 * This class:
 * 1. Implements ILogSink — receives LogMessage objects from AsyncLogManager
 * 2. Parses the payload value from each message
 * 3. Emits Qt signals so the DashboardWindow gauges update safely
 *    (thread-safe: AsyncLogManager worker threads → Qt GUI thread via queued connection)
 *
 * DESIGN PATTERNS USED:
 * - Adapter: Adapts ILogSink interface to Qt signal/slot system
 * - Observer: Qt signals notify the Dashboard of new telemetry data
 */

#include "inc/logging/ILogSink.hpp"
#include "inc/logging/LogMessage.hpp"

#include <QObject>

class TelemetryBridge : public QObject, public logging::ILogSink
{
    Q_OBJECT

public:
    explicit TelemetryBridge(QObject* parent = nullptr);
    ~TelemetryBridge() override = default;

    /**
     * @brief Called by AsyncLogManager on every new log message
     *        (may be from a non-GUI thread — signals are auto-queued)
     */
    void write(const logging::LogMessage& msg) override;

signals:
    void cpuLoadUpdated(double pct);
    void cpuTempUpdated(double celsius);
    void ramUpdated(double pct);
    void connected(const QString& host);
    void disconnected();
};

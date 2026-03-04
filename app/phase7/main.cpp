/**
 * @file main.cpp
 * @brief Phase 7: Qt6 Dashboard Entry Point
 *
 * Starts the Qt application with a clean dashboard window.
 * The TelemetryApp (Facade) runs in the background, logging silently
 * to file while the GUI displays live gauges.
 *
 * No terminal output. No console spam. Just a clean operator dashboard.
 */

#include "DashboardWindow.hpp"
#include "TelemetryBridge.hpp"
#include "inc/Facade/TelemetryApp.hpp"

#include <QApplication>
#include <QThread>
#include <memory>
#include <thread>
#include <iostream>

int main(int argc, char* argv[])
{
    // ── 1. Create Qt Application ──────────────────────────────────────────────
    QApplication app(argc, argv);
    app.setApplicationName("RPi System Monitor");
    app.setOrganizationName("ITI");

    // ── 2. Create the TelemetryBridge (implements ILogSink, emits Qt signals) ─
    auto* bridge = new TelemetryBridge();

    // ── 3. Create the Dashboard Window ────────────────────────────────────────
    DashboardWindow window;

    // Wire bridge signals → dashboard gauge slots
    // Qt::QueuedConnection ensures thread safety between
    // the AsyncLogManager worker thread and the Qt GUI thread
    QObject::connect(bridge, &TelemetryBridge::cpuLoadUpdated,
                     &window, &DashboardWindow::setCpuLoad,
                     Qt::QueuedConnection);

    QObject::connect(bridge, &TelemetryBridge::cpuTempUpdated,
                     &window, &DashboardWindow::setCpuTemp,
                     Qt::QueuedConnection);

    QObject::connect(bridge, &TelemetryBridge::ramUpdated,
                     &window, &DashboardWindow::setRamUsage,
                     Qt::QueuedConnection);

    window.show();

    // ── 4. Start Telemetry Backend in a separate thread ───────────────────────
    // TelemetryApp uses the Facade pattern: reads config.json and
    // starts all sources + AsyncLogManager automatically.
    std::string configPath = "app/phase7/config.json";
    if (argc > 1) {
        configPath = argv[1];
    }

    std::thread backendThread([&bridge, &configPath]() {
        try {
            // Wrap bridge as shared_ptr (non-owning — Qt owns it via parent)
            auto bridgeSink = std::shared_ptr<logging::ILogSink>(
                bridge, [](logging::ILogSink*) { /* Qt owns it, don't delete */ }
            );

            facade::TelemetryApp telemetryApp(configPath, bridgeSink);
            telemetryApp.start();
            telemetryApp.waitForShutdown();
        } catch (const std::exception& e) {
            // If backend fails, notify via bridge
            QMetaObject::invokeMethod(bridge, [bridge]() {
                emit bridge->disconnected();
            }, Qt::QueuedConnection);
        }
    });

    // ── 5. Mark as connected (RPi-specific: update this after vSOME/IP handshake)
    window.onConnected("Local (Phase7 Demo)");

    // ── 6. Run Qt event loop (blocks until window closes) ─────────────────────
    int result = app.exec();

    // ── 7. Cleanup ────────────────────────────────────────────────────────────
    if (backendThread.joinable()) {
        backendThread.detach(); // TelemetryApp handles its own shutdown via SIGINT
    }

    return result;
}

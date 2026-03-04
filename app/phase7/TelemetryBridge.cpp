#include "TelemetryBridge.hpp"
#include "inc/logging/LogMessage.hpp"

#include <QString>
#include <string>
#include <regex>

TelemetryBridge::TelemetryBridge(QObject* parent)
    : QObject(parent)
{
}

/**
 * @brief Parse and route a LogMessage to the appropriate gauge signal.
 *
 * Expected message format (from LogMessage::getText()):
 *   "[timestamp] [CPU] [AppName] [INFO] Payload value is: 42%"
 *   "[timestamp] [RAM] [AppName] [INFO] Payload value is: 67%"
 *
 * For CPU Temp (Phase 7 RPi enhancement), the context is still CPU but
 * the Facade tags the source name differently — we detect via source name
 * passed through app_name field when constructed from RPi service.
 *
 * We emit Qt signals with QueuedConnection semantics automatically because
 * TelemetryBridge lives on the GUI thread and write() is called from
 * the AsyncLogManager worker thread.
 */
void TelemetryBridge::write(const logging::LogMessage& msg)
{
    // Extract numeric payload from text: "Payload value is: XX%"
    const std::string& text = msg.getText();
    std::smatch match;
    std::regex payloadRe(R"(Payload value is:\s*(\d+))");
    if (!std::regex_search(text, match, payloadRe)) {
        return;  // Not a payload-bearing message
    }

    double value = std::stod(match[1].str());

    // Route based on context
    switch (msg.getContext()) {
    case logging::Context::CPU:
        // Check if the app_name hints at temperature
        if (msg.getAppName().find("Temp") != std::string::npos ||
            msg.getAppName().find("temp") != std::string::npos) {
            QMetaObject::invokeMethod(this, [this, value]() {
                emit cpuTempUpdated(value);
            }, Qt::QueuedConnection);
        } else {
            QMetaObject::invokeMethod(this, [this, value]() {
                emit cpuLoadUpdated(value);
            }, Qt::QueuedConnection);
        }
        break;

    case logging::Context::RAM:
        QMetaObject::invokeMethod(this, [this, value]() {
            emit ramUpdated(value);
        }, Qt::QueuedConnection);
        break;

    case logging::Context::GPU:
        // GPU maps to CPU Temp visually on the RPi dashboard (RPi has no GPU metric)
        QMetaObject::invokeMethod(this, [this, value]() {
            emit cpuTempUpdated(value);
        }, Qt::QueuedConnection);
        break;
    }
}

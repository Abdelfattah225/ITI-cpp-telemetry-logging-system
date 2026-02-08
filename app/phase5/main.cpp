/**
 * @file main.cpp
 * @brief Phase 5 Integration Demo - vSOME/IP Telemetry with AsyncLogManager
 * 
 * This demonstrates the full pipeline:
 *   vSOME/IP Service → SomeIPTelemetrySourceImpl → AsyncLogManager → Sinks
 * 
 * Run with: VSOMEIP_CONFIGURATION=vsomeip_client.json ./main
 * (Start telemetry_service first in another terminal)
 */

#include "inc/AsyncLogging/AsyncLogManager.hpp"
#include "inc/logging/ConsoleSinkImpl.hpp"
#include "inc/logging/FileSinkImpl.hpp"
#include "inc/logging/LogMessage.hpp"
#include "inc/SmartDataHub/SomeIPTelemetrySourceImpl.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

std::atomic<bool> g_running{true};

void signalHandler(int signum)
{
    std::cout << "\n[Main] Shutting down (signal " << signum << ")" << std::endl;
    g_running = false;
}

void vsomeipTelemetryThread(
    async_logging::AsyncLogManager& logManager)
{
    std::cout << "[vSOME/IP Thread] Starting..." << std::endl;

    // Get singleton instance of SomeIPTelemetrySourceImpl
    auto& source = SmartDataHub::SomeIPTelemetrySourceImpl::getInstance();

    // Open the source (Adapter pattern: maps to init() + start())
    if (!source.openSource()) {
        std::cerr << "[vSOME/IP Thread] Failed to open source!" << std::endl;
        return;
    }

    std::cout << "[vSOME/IP Thread] Source opened, requesting telemetry..." << std::endl;

    while (g_running) {
        std::string rawData;

        // Read from source (Adapter pattern: maps to requestTelemetry())
        if (source.readSource(rawData)) {
            try {
                float value = std::stof(rawData);
                uint8_t payload = static_cast<uint8_t>(std::min(100.0f, std::max(0.0f, value)));
                
                // Create log message with CPU context
                logging::LogMessage msg("vSOME/IP_CPU", logging::Context::CPU, payload);

                if (!logManager.log(std::move(msg))) {
                    std::cerr << "[vSOME/IP Thread] Failed to log message" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "[vSOME/IP Thread] Parse error: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "[vSOME/IP Thread] Failed to read telemetry" << std::endl;
        }

        // Wait before next request (2 seconds)
        for (int i = 0; i < 20 && g_running; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::cout << "[vSOME/IP Thread] Stopped" << std::endl;
}

int main()
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "=== Phase 5: vSOME/IP Telemetry Integration ===" << std::endl;
    std::cout << "Make sure telemetry_service is running in another terminal!" << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;
    std::cout << std::endl;

    // Create sinks
    std::vector<std::shared_ptr<logging::ILogSink>> sinks;
    sinks.push_back(std::make_shared<logging::ConsoleSinkImpl>());
    sinks.push_back(std::make_shared<logging::FileSinkImpl>("vsomeip_telemetry_log.txt"));

    // Create AsyncLogManager with ThreadPool
    async_logging::AsyncLogManager asyncManager(
        "vSOME/IP_TelemetryApp",
        std::move(sinks),
        100,    // buffer capacity
        true,   // use thread pool
        4       // pool size
    );
    asyncManager.start();

    // Start vSOME/IP telemetry thread
    std::thread vsomeipThread(vsomeipTelemetryThread, std::ref(asyncManager));

    // Wait for shutdown signal
    vsomeipThread.join();

    asyncManager.stop();

    std::cout << "=== Phase 5 Demo Stopped ===" << std::endl;
    return 0;
}

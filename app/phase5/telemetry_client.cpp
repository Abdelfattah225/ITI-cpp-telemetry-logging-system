/**
 * @file telemetry_client.cpp
 * @brief Test client for vSOME/IP Telemetry Service
 * 
 * This demonstrates using SomeIPTelemetrySourceImpl (Singleton + Adapter)
 * through the ITelemetrySource interface.
 * 
 * Run with: VSOMEIP_CONFIGURATION=vsomeip_client.json ./telemetry_client
 */

#include "inc/SmartDataHub/SomeIPTelemetrySourceImpl.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

std::atomic<bool> g_running{true};

void signalHandler(int signum)
{
    std::cout << "\n[Client] Shutting down (signal " << signum << ")" << std::endl;
    g_running = false;
}

int main()
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "=== Telemetry Client (vSOME/IP) ===" << std::endl;
    std::cout << "Using SomeIPTelemetrySourceImpl (Singleton + Adapter)" << std::endl;
    std::cout << std::endl;

    // Get the singleton instance
    // This demonstrates the Singleton pattern
    auto& telemetrySource = SmartDataHub::SomeIPTelemetrySourceImpl::getInstance();

    // Open the source (Adapter: maps to init() + start())
    std::cout << "[Client] Opening telemetry source..." << std::endl;
    if (!telemetrySource.openSource()) {
        std::cerr << "[Client] Failed to open telemetry source!" << std::endl;
        return 1;
    }

    std::cout << "[Client] Source opened. Starting telemetry requests..." << std::endl;
    std::cout << "[Client] Press Ctrl+C to stop." << std::endl;
    std::cout << std::endl;

    // Periodically request telemetry data
    while (g_running) {
        std::string telemetryData;
        
        // Read from source (Adapter: maps to requestTelemetry())
        if (telemetrySource.readSource(telemetryData)) {
            std::cout << "[Client] Received telemetry: " << telemetryData << "%" << std::endl;
        } else {
            std::cerr << "[Client] Failed to read telemetry data" << std::endl;
        }

        // Wait before next request
        for (int i = 0; i < 20 && g_running; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::cout << "[Client] Stopped." << std::endl;
    return 0;
}

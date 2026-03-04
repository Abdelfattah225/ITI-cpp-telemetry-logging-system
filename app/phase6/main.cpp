/**
 * @file main.cpp
 * @brief Phase 6 Main Application
 * 
 * This file demonstrates the simplicity achieved by the Façade Pattern.
 * The entire complex system setup is reduced to 3 lines of code.
 */

#include "inc/Facade/TelemetryApp.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    // Default config path
    std::string configPath = "app/phase6/config.json";

    // Allow overriding via command line
    if (argc > 1) {
        configPath = argv[1];
    }

    std::cout << "=== Phase 6: System Wrap Up (Façade) ===" << std::endl;
    std::cout << "Using configuration: " << configPath << std::endl;

    try {
        // 1. Create the application (Façade)
        // This handles all initialization, config loading, and component creation
        facade::TelemetryApp app(configPath);

        // 2. Start the application
        // This starts all threads (logging, consumers, producers)
        app.start();

        // 3. Wait for shutdown
        // Blocks main thread until user presses Ctrl+C
        app.waitForShutdown();

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "=== Application Exited Cleanly ===" << std::endl;
    return 0;
}

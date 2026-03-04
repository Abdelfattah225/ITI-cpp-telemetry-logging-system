#pragma once

/**
 * @file TelemetryApp.hpp
 * @brief Façade class that wraps the entire telemetry logging system
 * 
 * FAÇADE PATTERN:
 * This class hides all the complexity of:
 *   - AsyncLogManager
 *   - ThreadPool
 *   - Multiple TelemetrySources
 *   - Multiple LogSinks
 *   - Configuration parsing
 *   - Thread management
 * 
 * Behind a simple interface:
 *   TelemetryApp app("config.json");
 *   app.start();
 *   // ... wait for Ctrl+C
 *   app.stop();
 */

#include "AppConfig.hpp"
#include "inc/AsyncLogging/AsyncLogManager.hpp"
#include "inc/SmartDataHub/ITelemetrySource.hpp"

#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>

namespace facade
{

    /**
     * @class TelemetryApp
     * @brief Main application Façade
     * 
     * Usage:
     *   TelemetryApp app("config.json");
     *   app.start();               // Non-blocking
     *   app.waitForShutdown();     // Blocking (until Ctrl+C)
     *   app.stop();                // Cleanup
     */
    class TelemetryApp
    {
    public:
        /**
         * @brief Construct from JSON config file
         * @param configPath Path to JSON configuration file
         * 
         * This constructor:
         * 1. Loads the configuration
         * 2. Creates log sinks based on config
         * 3. Creates AsyncLogManager
         * 4. Creates telemetry sources based on config
         */
        explicit TelemetryApp(const std::string& configPath);

        /**
         * @brief Construct with an extra injected sink (e.g. QtLogSink for Phase 7)
         * @param configPath Path to JSON configuration file
         * @param extraSink  Additional sink injected from outside (e.g. TelemetryBridge)
         */
        TelemetryApp(const std::string& configPath,
                     std::shared_ptr<logging::ILogSink> extraSink);

        /**
         * @brief Destructor - ensures clean shutdown
         */
        ~TelemetryApp();

        // Non-copyable, non-movable
        TelemetryApp(const TelemetryApp&) = delete;
        TelemetryApp& operator=(const TelemetryApp&) = delete;
        TelemetryApp(TelemetryApp&&) = delete;
        TelemetryApp& operator=(TelemetryApp&&) = delete;

        /**
         * @brief Start the application (non-blocking)
         * 
         * Starts:
         * - AsyncLogManager
         * - All enabled telemetry source threads
         */
        void start();

        /**
         * @brief Stop the application gracefully
         */
        void stop();

        /**
         * @brief Block until shutdown signal received (Ctrl+C)
         */
        void waitForShutdown();

        /**
         * @brief Check if application is running
         */
        bool isRunning() const;

        /**
         * @brief Get the loaded configuration
         */
        const AppConfig& getConfig() const;

    private:
        /**
         * @brief Create log sinks based on configuration
         */
        void createSinks();

        /**
         * @brief Create and start telemetry source threads
         */
        void createSourceThreads();

        /**
         * @brief Worker function for source reading
         */
        void sourceWorker(const std::string& sourceName, const SourceConfig& config);

        AppConfig m_config;
        std::shared_ptr<logging::ILogSink> m_extraSink; // optional injected sink (Phase 7)
        std::unique_ptr<async_logging::AsyncLogManager> m_logManager;
        std::vector<std::shared_ptr<logging::ILogSink>> m_sinks;
        std::vector<std::thread> m_sourceThreads;
        std::atomic<bool> m_running{false};
    };

} // namespace facade

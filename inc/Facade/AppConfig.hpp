#pragma once

/**
 * @file AppConfig.hpp
 * @brief Configuration structure for TelemetryApp
 * 
 * This file defines the configuration data structures that can be
 * loaded from a JSON file at runtime.
 * 
 * JSON Configuration Example:
 * {
 *     "appName": "TelemetryLogger",
 *     "bufferSize": 128,
 *     "threadPoolSize": 4,
 *     "logFilePath": "telemetry_log.txt",
 *     "sources": {
 *         "CPU": {
 *             "enabled": true,
 *             "type": "FILE",
 *             "path": "/proc/stat",
 *             "parseRateMs": 500,
 *             "sinks": ["CONSOLE", "FILE"]
 *         }
 *     }
 * }
 */

#include <string>
#include <vector>
#include <map>

namespace facade
{

    /**
     * @enum SourceType
     * @brief Type of telemetry source
     */
    enum class SourceType
    {
        FILE,    // Read from file (FileTelemetrySourceImpl)
        VSOMEIP, // Read from vSOME/IP service (SomeIPTelemetrySourceImpl)
        SOCKET   // Read from TCP server (TcpTelemetrySourceImpl)
    };

    /**
     * @enum SinkType
     * @brief Type of log sink
     */
    enum class SinkType
    {
        CONSOLE,
        FILE
    };

    /**
     * @struct SourceConfig
     * @brief Configuration for a single telemetry source
     */
    struct SourceConfig
    {
        bool enabled = false;          // Is this source enabled?
        SourceType type = SourceType::FILE;
        std::string path;              // File path (for FILE type)
        std::string host;              // TCP host (for SOCKET type)
        uint16_t    port = 9001;       // TCP port (for SOCKET type)
        int parseRateMs = 500;         // How often to read from source (ms)
        std::vector<SinkType> sinks;   // Which sinks to send logs to
    };

    /**
     * @struct AppConfig
     * @brief Main application configuration
     */
    struct AppConfig
    {
        std::string appName = "TelemetryApp";
        size_t bufferSize = 128;
        size_t threadPoolSize = 4;
        std::string logFilePath = "telemetry_log.txt";
        
        // Map of source name -> config
        // Keys: "CPU", "RAM", "GPU"
        std::map<std::string, SourceConfig> sources;

        /**
         * @brief Load configuration from JSON file
         * @param filePath Path to JSON configuration file
         * @return Parsed AppConfig
         * @throws std::runtime_error if file cannot be parsed
         */
        static AppConfig fromJson(const std::string& filePath);
        
        /**
         * @brief Print configuration to stdout (for debugging)
         */
        void print() const;
    };

} // namespace facade

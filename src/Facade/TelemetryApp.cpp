#include "TelemetryApp.hpp"
#include "inc/logging/ConsoleSinkImpl.hpp"
#include "inc/logging/FileSinkImpl.hpp"
#include "inc/logging/LogMessage.hpp"
#include "inc/SmartDataHub/FileTelemetrySourceImpl.hpp"
#include "inc/SmartDataHub/TcpTelemetrySourceImpl.hpp"
#include "inc/SmartDataHub/SomeIPTelemetrySourceImpl.hpp"
#include "inc/SmartDataHub/TelemetryParser.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <csignal>
#include <chrono>

namespace facade
{

    // Global flag for signal handling
    static std::atomic<bool> g_shutdownRequested{false};

    void signalHandler(int signum)
    {
        std::cout << "\n[TelemetryApp] Shutdown signal received (" << signum << ")" << std::endl;
        g_shutdownRequested = true;
    }

    TelemetryApp::TelemetryApp(const std::string& configPath)
        : TelemetryApp(configPath, nullptr)
    {}

    TelemetryApp::TelemetryApp(const std::string& configPath,
                               std::shared_ptr<logging::ILogSink> extraSink)
        : m_extraSink(std::move(extraSink))
    {
        // Step 1: Load configuration
        m_config = AppConfig::fromJson(configPath);

        // Step 2: Create sinks
        createSinks();

        // Step 3: Create AsyncLogManager
        m_logManager = std::make_unique<async_logging::AsyncLogManager>(
            m_config.appName,
            m_sinks,
            m_config.bufferSize,
            true,  // use thread pool
            m_config.threadPoolSize
        );
    }

    TelemetryApp::~TelemetryApp()
    {
        if (m_running) {
            stop();
        }
    }

    void TelemetryApp::createSinks()
    {
        // Collect unique sink types from all sources
        bool needConsole = false;
        bool needFile    = false;

        for (const auto& [name, srcConfig] : m_config.sources) {
            if (!srcConfig.enabled) continue;
            for (const auto& sink : srcConfig.sinks) {
                if (sink == SinkType::CONSOLE) needConsole = true;
                if (sink == SinkType::FILE)    needFile    = true;
            }
        }

        // Create required sinks
        if (needConsole) {
            m_sinks.push_back(std::make_shared<logging::ConsoleSinkImpl>());
        }
        if (needFile) {
            m_sinks.push_back(std::make_shared<logging::FileSinkImpl>(m_config.logFilePath));
        }

        // Inject optional extra sink (e.g. TelemetryBridge for Qt GUI)
        if (m_extraSink) {
            m_sinks.push_back(m_extraSink);
        }
    }

    void TelemetryApp::start()
    {
        if (m_running) return;

        m_running = true;
        g_shutdownRequested = false;

        std::signal(SIGINT,  signalHandler);
        std::signal(SIGTERM, signalHandler);

        m_logManager->start();
        createSourceThreads();
    }

    void TelemetryApp::createSourceThreads()
    {
        for (const auto& [name, srcConfig] : m_config.sources) {
            if (!srcConfig.enabled) continue;
            m_sourceThreads.emplace_back(&TelemetryApp::sourceWorker, this, name, srcConfig);
        }
    }

    void TelemetryApp::sourceWorker(const std::string& sourceName, const SourceConfig& config)
    {
        // ── VSOMEIP source — uses existing SomeIPTelemetrySourceImpl ─────────
        // Response format from rpi_someip_telemetry_service: "CPU:38\nRAM:71\nTEMP:45\n"
        // One "RPi" source drives ALL THREE gauges via a single vsomeip connection.
        if (config.type == SourceType::VSOMEIP) {
            auto& source = SmartDataHub::SomeIPTelemetrySourceImpl::getInstance();

            if (!source.openSource()) return;  // vsomeip init failed

            while (m_running && !g_shutdownRequested) {
                std::string block;
                if (source.readSource(block)) {
                    // Parse multi-value payload: "CPU:38\nRAM:71\nTEMP:45\n"
                    std::istringstream ss(block);
                    std::string line;
                    while (std::getline(ss, line)) {
                        if (line.empty()) continue;
                        auto colon = line.find(':');
                        if (colon == std::string::npos) continue;

                        std::string key = line.substr(0, colon);
                        int val = 0;
                        try { val = std::stoi(line.substr(colon + 1)); } catch (...) { continue; }
                        val = std::max(0, std::min(100, val));

                        if (key == "CPU") {
                            m_logManager->log(logging::LogMessage(
                                "CPU", logging::Context::CPU, (uint8_t)val));
                        } else if (key == "RAM") {
                            m_logManager->log(logging::LogMessage(
                                "RAM", logging::Context::RAM, (uint8_t)val));
                        } else if (key == "TEMP") {
                            // GPU context maps to CPU Temp gauge in TelemetryBridge
                            m_logManager->log(logging::LogMessage(
                                "GPU", logging::Context::GPU, (uint8_t)val));
                        }
                    }
                }
                for (int i = 0; i < config.parseRateMs / 100 && m_running && !g_shutdownRequested; ++i)
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            return;
        }

        // ── SOCKET (TCP) source — receives RPi telemetry stream ───────────────
        if (config.type == SourceType::SOCKET) {
            auto source = std::make_unique<SmartDataHub::TcpTelemetrySourceImpl>(
                config.host, config.port);

            while (m_running && !g_shutdownRequested) {
                if (!source->openSource()) {
                    // Retry on connection failure
                    for (int i = 0; i < 30 && m_running && !g_shutdownRequested; ++i)
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }

                while (m_running && !g_shutdownRequested) {
                    std::string block;
                    if (!source->readSource(block)) break; // disconnected

                    // Parse each key:value line in the received block
                    std::istringstream ss(block);
                    std::string line;
                    while (std::getline(ss, line)) {
                        if (line.empty()) continue;
                        auto colon = line.find(':');
                        if (colon == std::string::npos) continue;

                        std::string key = line.substr(0, colon);
                        int val = std::stoi(line.substr(colon + 1));
                        val = std::max(0, std::min(100, val));

                        if (key == "CPU") {
                            m_logManager->log(logging::LogMessage(
                                "CPU", logging::Context::CPU, (uint8_t)val));
                        } else if (key == "RAM") {
                            m_logManager->log(logging::LogMessage(
                                "RAM", logging::Context::RAM, (uint8_t)val));
                        } else if (key == "TEMP") {
                            // GPU context maps to CPU Temp gauge in TelemetryBridge
                            m_logManager->log(logging::LogMessage(
                                "GPU", logging::Context::GPU, (uint8_t)val));
                        }
                    }
                }
            }
            return;
        }

        logging::Context context = logging::Context::CPU;
        if (sourceName == "RAM") context = logging::Context::RAM;
        else if (sourceName == "GPU") context = logging::Context::GPU;


        // ── CPU Load: proper delta-based % using TelemetryParser ─────────────
        if (sourceName == "CPU" && config.type == SourceType::FILE) {
            SmartDataHub::TelemetryParser parser;
            // Prime the first read (always returns 0 due to no delta yet)
            parser.getCpuUsage();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            while (m_running && !g_shutdownRequested) {
                double usage = parser.getCpuUsage();
                if (usage >= 0.0) {
                    uint8_t payload = static_cast<uint8_t>(
                        std::min(100.0, std::max(0.0, usage)));
                    m_logManager->log(logging::LogMessage(sourceName, context, payload));
                }
                for (int i = 0; i < config.parseRateMs / 100 && m_running && !g_shutdownRequested; ++i)
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            return;
        }

        // ── RAM Usage: read /proc/meminfo directly (TelemetryParser loop
        // is broken because FileTelemetrySourceImpl::readSource always
        // seeks to beginning — so we bypass it here with ifstream) ─────────
        if (sourceName == "RAM" && config.type == SourceType::FILE) {
            while (m_running && !g_shutdownRequested) {
                std::ifstream memFile(config.path);
                if (memFile.is_open()) {
                    unsigned long long memTotal = 0, memAvailable = 0;
                    bool foundTotal = false, foundAvail = false;
                    std::string line;
                    while (std::getline(memFile, line) && !(foundTotal && foundAvail)) {
                        if (line.rfind("MemTotal:", 0) == 0) {
                            std::istringstream ss(line);
                            std::string lbl; ss >> lbl >> memTotal;
                            foundTotal = true;
                        } else if (line.rfind("MemAvailable:", 0) == 0) {
                            std::istringstream ss(line);
                            std::string lbl; ss >> lbl >> memAvailable;
                            foundAvail = true;
                        }
                    }
                    if (foundTotal && foundAvail && memTotal > 0) {
                        double used = static_cast<double>(memTotal - memAvailable);
                        double pct  = (used / memTotal) * 100.0;
                        uint8_t payload = static_cast<uint8_t>(std::min(100.0, std::max(0.0, pct)));
                        m_logManager->log(logging::LogMessage(sourceName, context, payload));
                    }
                }
                for (int i = 0; i < config.parseRateMs / 100 && m_running && !g_shutdownRequested; ++i)
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            return;
        }


        // ── CPU Temperature: read /sys/class/thermal/thermal_zone0/temp ───────
        // Raw value is in millidegrees Celsius — divide by 1000 to get °C
        if (sourceName == "GPU" && config.type == SourceType::FILE) {
            while (m_running && !g_shutdownRequested) {
                std::ifstream tempFile(config.path);
                if (tempFile.is_open()) {
                    long rawTemp = 0;
                    tempFile >> rawTemp;
                    // Convert millidegrees → degrees
                    double celsius = static_cast<double>(rawTemp) / 1000.0;
                    celsius = std::min(100.0, std::max(0.0, celsius));
                    uint8_t payload = static_cast<uint8_t>(celsius);
                    m_logManager->log(logging::LogMessage(sourceName, context, payload));
                }
                for (int i = 0; i < config.parseRateMs / 100 && m_running && !g_shutdownRequested; ++i)
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            return;
        }

        // ── Fallback for any other FILE source ────────────────────────────────
        auto source = std::make_unique<SmartDataHub::FileTelemetrySourceImpl>(config.path);
        if (!source->openSource()) return;

        while (m_running && !g_shutdownRequested) {
            std::string rawData;
            if (source->readSource(rawData)) {
                try {
                    float value = 0.0f;
                    size_t pos = rawData.find_first_of("0123456789");
                    if (pos != std::string::npos)
                        value = std::stof(rawData.substr(pos));
                    value = std::min(100.0f, std::max(0.0f, value));
                    m_logManager->log(logging::LogMessage(sourceName, context,
                                      static_cast<uint8_t>(value)));
                } catch (...) {}
            }
            for (int i = 0; i < config.parseRateMs / 100 && m_running && !g_shutdownRequested; ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }


    void TelemetryApp::stop()
    {
        if (!m_running) return;
        m_running = false;
        for (auto& thread : m_sourceThreads) {
            if (thread.joinable()) thread.join();
        }
        m_sourceThreads.clear();
        m_logManager->stop();
    }

    void TelemetryApp::waitForShutdown()
    {
        while (m_running && !g_shutdownRequested) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        stop();
    }

    bool TelemetryApp::isRunning() const
    {
        return m_running;
    }

    const AppConfig& TelemetryApp::getConfig() const
    {
        return m_config;
    }

} // namespace facade

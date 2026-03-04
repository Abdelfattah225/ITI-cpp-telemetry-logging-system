#include "AppConfig.hpp"
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

using json = nlohmann::json;

namespace facade
{

    /**
     * Helper function to convert string to SourceType
     */
    SourceType stringToSourceType(const std::string& str)
    {
        if (str == "FILE")    return SourceType::FILE;
        if (str == "VSOMEIP") return SourceType::VSOMEIP;
        if (str == "SOCKET")  return SourceType::SOCKET;
        throw std::runtime_error("Unknown source type: " + str);
    }

    /**
     * Helper function to convert string to SinkType
     */
    SinkType stringToSinkType(const std::string& str)
    {
        if (str == "CONSOLE") return SinkType::CONSOLE;
        if (str == "FILE") return SinkType::FILE;
        throw std::runtime_error("Unknown sink type: " + str);
    }

    AppConfig AppConfig::fromJson(const std::string& filePath)
    {
        // Open and parse JSON file
        std::ifstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open config file: " + filePath);
        }

        json j;
        try {
            j = json::parse(file);
        } catch (const json::parse_error& e) {
            throw std::runtime_error("JSON parse error: " + std::string(e.what()));
        }

        AppConfig config;

        // Parse top-level fields with defaults
        if (j.contains("appName")) {
            config.appName = j["appName"].get<std::string>();
        }
        if (j.contains("bufferSize")) {
            config.bufferSize = j["bufferSize"].get<size_t>();
        }
        if (j.contains("threadPoolSize")) {
            config.threadPoolSize = j["threadPoolSize"].get<size_t>();
        }
        if (j.contains("logFilePath")) {
            config.logFilePath = j["logFilePath"].get<std::string>();
        }

        // Parse sources
        if (j.contains("sources") && j["sources"].is_object()) {
            for (auto& [sourceName, sourceJson] : j["sources"].items()) {
                SourceConfig srcConfig;
                
                if (sourceJson.contains("enabled")) {
                    srcConfig.enabled = sourceJson["enabled"].get<bool>();
                }
                if (sourceJson.contains("type")) {
                    srcConfig.type = stringToSourceType(sourceJson["type"].get<std::string>());
                }
                if (sourceJson.contains("path")) {
                    srcConfig.path = sourceJson["path"].get<std::string>();
                }
                if (sourceJson.contains("host")) {
                    srcConfig.host = sourceJson["host"].get<std::string>();
                }
                if (sourceJson.contains("port")) {
                    srcConfig.port = static_cast<uint16_t>(sourceJson["port"].get<int>());
                }
                if (sourceJson.contains("parseRateMs")) {
                    srcConfig.parseRateMs = sourceJson["parseRateMs"].get<int>();
                }
                if (sourceJson.contains("sinks") && sourceJson["sinks"].is_array()) {
                    for (const auto& sink : sourceJson["sinks"]) {
                        srcConfig.sinks.push_back(stringToSinkType(sink.get<std::string>()));
                    }
                }

                config.sources[sourceName] = srcConfig;
            }
        }

        return config;
    }

    void AppConfig::print() const
    {
        std::cout << "=== Application Configuration ===" << std::endl;
        std::cout << "App Name: " << appName << std::endl;
        std::cout << "Buffer Size: " << bufferSize << std::endl;
        std::cout << "Thread Pool Size: " << threadPoolSize << std::endl;
        std::cout << "Log File Path: " << logFilePath << std::endl;
        std::cout << std::endl;

        std::cout << "Sources:" << std::endl;
        for (const auto& [name, src] : sources) {
            std::cout << "  " << name << ":" << std::endl;
            std::cout << "    Enabled: " << (src.enabled ? "true" : "false") << std::endl;
            std::cout << "    Type: " << (src.type == SourceType::FILE ? "FILE" : "VSOMEIP") << std::endl;
            std::cout << "    Path: " << src.path << std::endl;
            std::cout << "    Parse Rate: " << src.parseRateMs << "ms" << std::endl;
            std::cout << "    Sinks: ";
            for (const auto& sink : src.sinks) {
                std::cout << (sink == SinkType::CONSOLE ? "CONSOLE" : "FILE") << " ";
            }
            std::cout << std::endl;
        }
    }

} // namespace facade

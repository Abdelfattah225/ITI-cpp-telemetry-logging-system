#pragma once

/**
 * @file SomeIPTelemetrySourceImpl.hpp
 * @brief ITelemetrySource implementation using vSOME/IP
 * 
 * Design Patterns:
 * 
 * 1. ADAPTER PATTERN
 *    - Problem: SomeIPTelemetryClient has init(), start(), requestTelemetry() methods
 *    - Solution: Adapt these to ITelemetrySource's openSource(), readSource() interface
 *    - Benefit: Client code doesn't need to know about vsomeip specifics
 * 
 * 2. SINGLETON PATTERN
 *    - Problem: vsomeip app should only be created once per process
 *    - Solution: Static getInstance() method ensures single instance
 *    - Benefit: Resource efficiency and consistent state across the application
 */

#include "ITelemetrySource.hpp"
#include "SomeIPTelemetryClient.hpp"
#include <memory>
#include <mutex>

namespace SmartDataHub
{

    /**
     * @class SomeIPTelemetrySourceImpl
     * @brief Singleton adapter that bridges vsomeip client to ITelemetrySource
     * 
     * Usage:
     *   // Get the singleton instance
     *   auto& source = SomeIPTelemetrySourceImpl::getInstance();
     *   
     *   // Use through ITelemetrySource interface
     *   if (source.openSource()) {
     *       std::string data;
     *       if (source.readSource(data)) {
     *           std::cout << "Telemetry: " << data << std::endl;
     *       }
     *   }
     */
    class SomeIPTelemetrySourceImpl : public ITelemetrySource
    {
    public:
        /**
         * @brief Get the singleton instance
         * @return Reference to the single SomeIPTelemetrySourceImpl instance
         * 
         * Thread-safe: Uses static initialization (C++11 guarantees thread safety)
         */
        static SomeIPTelemetrySourceImpl& getInstance();

        // Delete copy and move (Singleton)
        SomeIPTelemetrySourceImpl(const SomeIPTelemetrySourceImpl&) = delete;
        SomeIPTelemetrySourceImpl& operator=(const SomeIPTelemetrySourceImpl&) = delete;
        SomeIPTelemetrySourceImpl(SomeIPTelemetrySourceImpl&&) = delete;
        SomeIPTelemetrySourceImpl& operator=(SomeIPTelemetrySourceImpl&&) = delete;

        /**
         * @brief Initialize and start the vsomeip client
         * @return true if successful
         * 
         * ADAPTER: Maps to client.init() + client.start()
         */
        bool openSource() override;

        /**
         * @brief Request telemetry data from the service
         * @param out Reference to store the result (float as string, 0-100)
         * @return true if data received successfully
         * 
         * ADAPTER: Maps to client.requestTelemetry()
         */
        bool readSource(std::string& out) override;

        /**
         * @brief Destructor - stops the vsomeip client
         */
        ~SomeIPTelemetrySourceImpl() override;

    private:
        /**
         * @brief Private constructor (Singleton pattern)
         * Only getInstance() can create the instance
         */
        SomeIPTelemetrySourceImpl();

        std::unique_ptr<SomeIPTelemetryClient> m_client;
        bool m_isOpen{false};
    };

} // namespace SmartDataHub

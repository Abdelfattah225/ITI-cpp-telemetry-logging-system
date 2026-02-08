#include "SomeIPTelemetrySourceImpl.hpp"
#include <iostream>

namespace SmartDataHub
{

    // ============= SINGLETON IMPLEMENTATION =============

    SomeIPTelemetrySourceImpl& SomeIPTelemetrySourceImpl::getInstance()
    {
        /**
         * Meyer's Singleton
         * 
         * This is thread-safe in C++11 and later. The standard guarantees
         * that static local variables are initialized exactly once, even
         * if multiple threads try to call getInstance() simultaneously.
         * 
         * The instance lives until program exit.
         */
        static SomeIPTelemetrySourceImpl instance;
        return instance;
    }

    SomeIPTelemetrySourceImpl::SomeIPTelemetrySourceImpl()
    {
        // Create the vsomeip client with a descriptive name
        m_client = std::make_unique<SomeIPTelemetryClient>("TelemetrySourceClient");
    }

    SomeIPTelemetrySourceImpl::~SomeIPTelemetrySourceImpl()
    {
        if (m_isOpen && m_client) {
            m_client->stop();
        }
    }

    // ============= ADAPTER IMPLEMENTATION =============

    bool SomeIPTelemetrySourceImpl::openSource()
    {
        /**
         * ADAPTER: openSource() → init() + start()
         * 
         * The ITelemetrySource interface expects openSource() to prepare
         * the data source. For vsomeip, this means:
         * 1. Initialize the application
         * 2. Start the event loop
         */
        if (m_isOpen) {
            return true;  // Already open
        }

        if (!m_client) {
            std::cerr << "[SomeIPTelemetrySourceImpl] Client not created!" << std::endl;
            return false;
        }

        if (!m_client->init()) {
            std::cerr << "[SomeIPTelemetrySourceImpl] Failed to initialize client" << std::endl;
            return false;
        }

        m_client->start();
        m_isOpen = true;

        std::cout << "[SomeIPTelemetrySourceImpl] Source opened successfully" << std::endl;
        return true;
    }

    bool SomeIPTelemetrySourceImpl::readSource(std::string& out)
    {
        /**
         * ADAPTER: readSource() → requestTelemetry()
         * 
         * The ITelemetrySource interface expects readSource() to fetch
         * the latest data. For vsomeip, this is a request/response call.
         */
        if (!m_isOpen) {
            std::cerr << "[SomeIPTelemetrySourceImpl] Source not opened!" << std::endl;
            return false;
        }

        if (!m_client) {
            return false;
        }

        return m_client->requestTelemetry(out);
    }

} // namespace SmartDataHub

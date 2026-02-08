#pragma once

/**
 * @file SomeIPTelemetryClient.hpp
 * @brief Low-level vSOME/IP client for telemetry data communication
 * 
 * This class handles the core vSOME/IP protocol:
 * - Application lifecycle (init, start, stop)
 * - Service discovery (waiting for service availability)
 * - Request/Response messaging
 * 
 * Key vsomeip concepts used:
 * - Application: Entry point to vsomeip (like a socket for networking)
 * - Service/Instance/Method IDs: Unique identifiers for RPC-style communication
 * - Message: Container for request/response payloads
 */

#include <vsomeip/vsomeip.hpp>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <string>

namespace SmartDataHub
{

    // Service identification constants
    // These MUST match between client and service!
    constexpr vsomeip::service_t TELEMETRY_SERVICE_ID   = 0x1234;
    constexpr vsomeip::instance_t TELEMETRY_INSTANCE_ID = 0x5678;
    constexpr vsomeip::method_t GET_TELEMETRY_METHOD_ID = 0x0001;

    /**
     * @class SomeIPTelemetryClient
     * @brief Core vsomeip client for request/response telemetry fetching
     * 
     * Usage:
     *   SomeIPTelemetryClient client("TelemetryClient");
     *   if (client.init()) {
     *       client.start();  // Starts vsomeip event loop in background
     *       std::string value;
     *       if (client.requestTelemetry(value)) {
     *           std::cout << "Got: " << value << std::endl;
     *       }
     *       client.stop();
     *   }
     */
    class SomeIPTelemetryClient
    {
    public:
        explicit SomeIPTelemetryClient(const std::string& appName);
        ~SomeIPTelemetryClient();

        // Delete copy (vsomeip app can't be copied)
        SomeIPTelemetryClient(const SomeIPTelemetryClient&) = delete;
        SomeIPTelemetryClient& operator=(const SomeIPTelemetryClient&) = delete;

        // Delete move (simplicity for singleton usage)
        SomeIPTelemetryClient(SomeIPTelemetryClient&&) = delete;
        SomeIPTelemetryClient& operator=(SomeIPTelemetryClient&&) = delete;

        /**
         * @brief Initialize vsomeip application
         * @return true if successful
         * 
         * Creates vsomeip runtime, application, and registers handlers.
         */
        bool init();

        /**
         * @brief Start vsomeip event loop (blocking call runs in separate thread)
         * Requests service and waits for availability.
         */
        void start();

        /**
         * @brief Stop vsomeip event loop and cleanup
         */
        void stop();

        /**
         * @brief Request telemetry data from service (blocking)
         * @param out Reference to store the received value (as string)
         * @param timeoutMs Maximum time to wait for response
         * @return true if response received, false on timeout/error
         * 
         * This is a synchronous call that:
         * 1. Waits for service to be available
         * 2. Sends a request
         * 3. Blocks until response received or timeout
         */
        bool requestTelemetry(std::string& out, int timeoutMs = 5000);

        /**
         * @brief Check if service is available
         */
        bool isServiceAvailable() const;

    private:
        // vsomeip callbacks
        void onState(vsomeip::state_type_e state);
        void onAvailability(vsomeip::service_t service, vsomeip::instance_t instance, bool isAvailable);
        void onMessage(const std::shared_ptr<vsomeip::message>& response);

        std::string m_appName;
        std::shared_ptr<vsomeip::application> m_app;
        std::thread m_runThread;
        
        // Synchronization for service availability
        std::mutex m_availabilityMutex;
        std::condition_variable m_availabilityCv;
        std::atomic<bool> m_serviceAvailable{false};
        
        // Synchronization for request/response
        std::mutex m_responseMutex;
        std::condition_variable m_responseCv;
        std::atomic<bool> m_responseReceived{false};
        std::string m_lastResponse;
        
        std::atomic<bool> m_running{false};
    };

} // namespace SmartDataHub

#include "SomeIPTelemetryClient.hpp"
#include <iostream>
#include <cstring>

namespace SmartDataHub
{

    SomeIPTelemetryClient::SomeIPTelemetryClient(const std::string& appName)
        : m_appName(appName)
    {
    }

    SomeIPTelemetryClient::~SomeIPTelemetryClient()
    {
        stop();
    }

    bool SomeIPTelemetryClient::init()
    {
        /**
         * Step 1: Get the vsomeip runtime
         * The runtime is a singleton that manages all vsomeip resources.
         * Think of it as the "vsomeip factory".
         */
        auto runtime = vsomeip::runtime::get();
        if (!runtime) {
            std::cerr << "[" << m_appName << "] Failed to get vsomeip runtime!" << std::endl;
            return false;
        }

        /**
         * Step 2: Create the application
         * An application is like a "client socket" - it represents this process
         * to the vsomeip framework. The name is used for configuration lookup.
         */
        m_app = runtime->create_application(m_appName);
        if (!m_app) {
            std::cerr << "[" << m_appName << "] Failed to create vsomeip application!" << std::endl;
            return false;
        }

        /**
         * Step 3: Initialize the application
         * This loads configuration from the JSON file specified by
         * VSOMEIP_CONFIGURATION environment variable.
         */
        if (!m_app->init()) {
            std::cerr << "[" << m_appName << "] Failed to initialize vsomeip application!" << std::endl;
            m_app.reset();
            return false;
        }

        /**
         * Step 4: Register callbacks
         * 
         * State handler: Called when app connects/disconnects from routing manager
         * Availability handler: Called when the service becomes available/unavailable
         * Message handler: Called when we receive a response to our request
         */
        m_app->register_state_handler(
            std::bind(&SomeIPTelemetryClient::onState, this, std::placeholders::_1)
        );

        m_app->register_availability_handler(
            TELEMETRY_SERVICE_ID,
            TELEMETRY_INSTANCE_ID,
            std::bind(&SomeIPTelemetryClient::onAvailability, this,
                      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        );

        m_app->register_message_handler(
            TELEMETRY_SERVICE_ID,
            TELEMETRY_INSTANCE_ID,
            GET_TELEMETRY_METHOD_ID,
            std::bind(&SomeIPTelemetryClient::onMessage, this, std::placeholders::_1)
        );

        std::cout << "[" << m_appName << "] Initialized successfully" << std::endl;
        return true;
    }

    void SomeIPTelemetryClient::start()
    {
        if (m_running) {
            return;
        }

        m_running = true;

        /**
         * Step 5: Request the service
         * This tells vsomeip we want to use service 0x1234, instance 0x5678.
         * vsomeip will notify us via onAvailability when it finds the service.
         */
        m_app->request_service(TELEMETRY_SERVICE_ID, TELEMETRY_INSTANCE_ID);

        /**
         * Step 6: Start the event loop in a separate thread
         * app->start() is BLOCKING - it runs the vsomeip event loop.
         * We run it in a thread so our main code can continue.
         */
        m_runThread = std::thread([this]() {
            std::cout << "[" << m_appName << "] Starting vsomeip event loop..." << std::endl;
            m_app->start();
            std::cout << "[" << m_appName << "] vsomeip event loop stopped." << std::endl;
        });

        std::cout << "[" << m_appName << "] Started" << std::endl;
    }

    void SomeIPTelemetryClient::stop()
    {
        if (!m_running) {
            return;
        }

        m_running = false;

        // Release the service request
        m_app->release_service(TELEMETRY_SERVICE_ID, TELEMETRY_INSTANCE_ID);

        // Stop the vsomeip event loop
        m_app->stop();

        // Wait for the run thread to finish
        if (m_runThread.joinable()) {
            m_runThread.join();
        }

        // Unregister handlers
        m_app->unregister_message_handler(
            TELEMETRY_SERVICE_ID, TELEMETRY_INSTANCE_ID, GET_TELEMETRY_METHOD_ID);
        m_app->unregister_availability_handler(
            TELEMETRY_SERVICE_ID, TELEMETRY_INSTANCE_ID);
        m_app->unregister_state_handler();

        std::cout << "[" << m_appName << "] Stopped" << std::endl;
    }

    bool SomeIPTelemetryClient::requestTelemetry(std::string& out, int timeoutMs)
    {
        /**
         * Wait for service to be available
         */
        {
            std::unique_lock<std::mutex> lock(m_availabilityMutex);
            if (!m_serviceAvailable) {
                std::cout << "[" << m_appName << "] Waiting for service availability..." << std::endl;
                auto status = m_availabilityCv.wait_for(
                    lock, 
                    std::chrono::milliseconds(timeoutMs),
                    [this]() { return m_serviceAvailable.load(); }
                );
                if (!status) {
                    std::cerr << "[" << m_appName << "] Service not available (timeout)" << std::endl;
                    return false;
                }
            }
        }

        /**
         * Create and send a request message
         * 
         * Message structure in vsomeip:
         * - service_id: Which service we're calling
         * - instance_id: Which instance of that service
         * - method_id: Which method/function we want to invoke
         * - payload: Optional data to send with the request
         */
        auto runtime = vsomeip::runtime::get();
        auto request = runtime->create_request();
        
        request->set_service(TELEMETRY_SERVICE_ID);
        request->set_instance(TELEMETRY_INSTANCE_ID);
        request->set_method(GET_TELEMETRY_METHOD_ID);
        request->set_reliable(true);  // Use TCP-like reliable transport

        // Reset response state
        m_responseReceived = false;
        m_lastResponse.clear();

        // Send the request
        std::cout << "[" << m_appName << "] Sending telemetry request..." << std::endl;
        m_app->send(request);

        /**
         * Wait for response
         * The onMessage callback will be triggered when response arrives.
         */
        {
            std::unique_lock<std::mutex> lock(m_responseMutex);
            auto status = m_responseCv.wait_for(
                lock,
                std::chrono::milliseconds(timeoutMs),
                [this]() { return m_responseReceived.load(); }
            );

            if (!status) {
                std::cerr << "[" << m_appName << "] Request timeout" << std::endl;
                return false;
            }

            out = m_lastResponse;
        }

        return true;
    }

    bool SomeIPTelemetryClient::isServiceAvailable() const
    {
        return m_serviceAvailable.load();
    }

    // ============= CALLBACK IMPLEMENTATIONS =============

    void SomeIPTelemetryClient::onState(vsomeip::state_type_e state)
    {
        /**
         * State handler is called when our application successfully
         * connects to the vsomeip routing manager.
         * 
         * REGISTERED = we're connected and ready to communicate
         * DEREGISTERED = we've been disconnected
         */
        if (state == vsomeip::state_type_e::ST_REGISTERED) {
            std::cout << "[" << m_appName << "] Application registered with vsomeip" << std::endl;
        } else {
            std::cout << "[" << m_appName << "] Application deregistered from vsomeip" << std::endl;
        }
    }

    void SomeIPTelemetryClient::onAvailability(
        vsomeip::service_t service,
        vsomeip::instance_t instance,
        bool isAvailable)
    {
        /**
         * Availability handler is called when the service we requested
         * becomes available or unavailable.
         * 
         * This is like getting a "service found" notification in network discovery.
         */
        std::cout << "[" << m_appName << "] Service 0x" << std::hex << service 
                  << " Instance 0x" << instance 
                  << " is " << (isAvailable ? "AVAILABLE" : "NOT AVAILABLE") 
                  << std::dec << std::endl;

        {
            std::lock_guard<std::mutex> lock(m_availabilityMutex);
            m_serviceAvailable = isAvailable;
        }
        m_availabilityCv.notify_all();
    }

    void SomeIPTelemetryClient::onMessage(const std::shared_ptr<vsomeip::message>& response)
    {
        /**
         * Message handler is called when we receive a response to our request.
         * 
         * The response contains:
         * - return_code: Success/failure status
         * - payload: The actual data (telemetry value in our case)
         */
        if (response->get_return_code() != vsomeip::return_code_e::E_OK) {
            std::cerr << "[" << m_appName << "] Received error response: " 
                      << static_cast<int>(response->get_return_code()) << std::endl;
            return;
        }

        // Extract payload
        auto payload = response->get_payload();
        if (payload && payload->get_length() > 0) {
            std::string data(reinterpret_cast<const char*>(payload->get_data()), 
                            payload->get_length());
            
            std::cout << "[" << m_appName << "] Received response: " << data << std::endl;

            {
                std::lock_guard<std::mutex> lock(m_responseMutex);
                m_lastResponse = data;
                m_responseReceived = true;
            }
            m_responseCv.notify_all();
        }
    }

} // namespace SmartDataHub

#pragma once

// Include the generated stub default header
// The path follows the package structure: v1/commonapi/telemetry/
#include <TelemetryServiceStubDefault.hpp>

// Include your existing TelemetryParser from Phase 2
#include "SmartDataHub/TelemetryParser.hpp"

// You can create your own namespace or use an existing one
namespace NetworkTelemetry {

/**
 * @brief Service implementation for TelemetryService
 * 
 * This class implements the server-side logic for handling
 * telemetry requests from clients over SOME/IP.
 */
class TelemetryServiceImpl : public v1::commonapi::telemetry::TelemetryServiceStubDefault {
public:
    // Constructor - should initialize the TelemetryParser
    TelemetryServiceImpl();
    
    // Virtual destructor (why virtual? think about inheritance!)
    virtual ~TelemetryServiceImpl();

    /**
     * @brief Handle getLoad requests from clients
     * 
     * @param _client   Identifies which client made the request
     * @param _source   Which telemetry source (CPU, RAM, GPU)
     * @param _reply    Callback function to send the response
     * 
     * TODO: Copy the exact signature from the generated stub
     * Hint: You found it in Part A!
     */
    // YOUR CODE HERE: Write the method declaration
    virtual void getLoad() override;

private:
    // Member to read actual telemetry data
    // YOUR CODE HERE: Declare a TelemetryParser member
    // Hint: SmartDataHub::TelemetryParser m_parser;
};

} // namespace NetworkTelemetry
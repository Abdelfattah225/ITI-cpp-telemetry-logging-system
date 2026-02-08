#include "inc/NetworkTelemetry/TelemetryServiceImpl.hpp"

void TelemetryServiceImpl::getLoad(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    TelemetryService::SourceType _source,
    getLoadReply_t _reply
) {
    float loadValue = 0.0f;
    bool success = false;
    
    // Read actual data based on source type
    if (_source == TelemetryService::SourceType::CPU) {
        // Read from /proc/stat, calculate CPU load
        loadValue = readCpuLoad();  // You implement this
        success = true;
    }
    // ... handle RAM, GPU
    
    // Send response back to client
    _reply(loadValue, success);
}
/**
 * @file telemetry_service.cpp
 * @brief vSOME/IP Telemetry Service (Server)
 * 
 * This service listens for telemetry requests and responds with
 * system load information (CPU percentage).
 * 
 * Communication Model: Request/Response
 * - Client sends: Empty request (just method call)
 * - Service responds: Payload containing load percentage (0-100) as string
 * 
 * Run with: VSOMEIP_CONFIGURATION=vsomeip_service.json ./telemetry_service
 */

#include <vsomeip/vsomeip.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <cstdlib>

// These MUST match the client's IDs
constexpr vsomeip::service_t TELEMETRY_SERVICE_ID   = 0x1234;
constexpr vsomeip::instance_t TELEMETRY_INSTANCE_ID = 0x5678;
constexpr vsomeip::method_t GET_TELEMETRY_METHOD_ID = 0x0001;

std::atomic<bool> g_running{true};
std::shared_ptr<vsomeip::application> g_app;

void signalHandler(int signum)
{
    std::cout << "\n[Service] Shutting down (signal " << signum << ")" << std::endl;
    g_running = false;
    if (g_app) {
        g_app->stop();
    }
}

/**
 * @brief Read CPU usage from /proc/stat
 * 
 * /proc/stat format:
 *   cpu  user nice system idle iowait irq softirq ...
 * 
 * CPU% = 100 * (total - idle) / total
 */
float readCpuUsage()
{
    static long long prevTotal = 0, prevIdle = 0;
    
    std::ifstream file("/proc/stat");
    if (!file.is_open()) {
        return 50.0f;  // Default fallback
    }

    std::string line;
    std::getline(file, line);  // First line is "cpu ..."
    
    std::istringstream iss(line);
    std::string cpu;
    long long user, nice, system, idle, iowait, irq, softirq, steal;
    
    iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    
    long long total = user + nice + system + idle + iowait + irq + softirq + steal;
    long long idleTime = idle + iowait;
    
    // Calculate delta
    long long totalDelta = total - prevTotal;
    long long idleDelta = idleTime - prevIdle;
    
    prevTotal = total;
    prevIdle = idleTime;
    
    if (totalDelta == 0) {
        return 50.0f;
    }
    
    float usage = 100.0f * (1.0f - static_cast<float>(idleDelta) / static_cast<float>(totalDelta));
    return std::max(0.0f, std::min(100.0f, usage));
}

/**
 * @brief Callback when client sends a request
 * 
 * Creates a response with the current CPU usage as payload.
 */
void onMessage(const std::shared_ptr<vsomeip::message>& request)
{
    std::cout << "[Service] Received request from client 0x" 
              << std::hex << request->get_client() << std::dec << std::endl;

    // Get CPU usage
    float cpuUsage = readCpuUsage();
    std::string payload = std::to_string(static_cast<int>(cpuUsage));
    
    std::cout << "[Service] CPU Usage: " << payload << "%" << std::endl;

    // Create response
    auto runtime = vsomeip::runtime::get();
    auto response = runtime->create_response(request);
    
    // Set payload
    auto responsePayload = runtime->create_payload();
    std::vector<vsomeip::byte_t> data(payload.begin(), payload.end());
    responsePayload->set_data(data);
    response->set_payload(responsePayload);
    
    // Send response
    g_app->send(response);
    std::cout << "[Service] Sent response: " << payload << std::endl;
}

/**
 * @brief Callback when app registers with vsomeip routing
 */
void onState(vsomeip::state_type_e state)
{
    if (state == vsomeip::state_type_e::ST_REGISTERED) {
        std::cout << "[Service] Registered with vsomeip - offering service..." << std::endl;
        
        // OFFER the service (make it discoverable)
        g_app->offer_service(TELEMETRY_SERVICE_ID, TELEMETRY_INSTANCE_ID);
    }
}

int main()
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "=== Telemetry Service (vSOME/IP) ===" << std::endl;
    std::cout << "Service ID: 0x" << std::hex << TELEMETRY_SERVICE_ID << std::dec << std::endl;
    std::cout << "Instance ID: 0x" << std::hex << TELEMETRY_INSTANCE_ID << std::dec << std::endl;
    std::cout << "Method ID: 0x" << std::hex << GET_TELEMETRY_METHOD_ID << std::dec << std::endl;
    std::cout << std::endl;

    // Step 1: Get runtime and create application
    auto runtime = vsomeip::runtime::get();
    g_app = runtime->create_application("TelemetryService");

    if (!g_app->init()) {
        std::cerr << "[Service] Failed to initialize vsomeip application!" << std::endl;
        return 1;
    }

    // Step 2: Register handlers
    g_app->register_state_handler(onState);
    g_app->register_message_handler(
        TELEMETRY_SERVICE_ID,
        TELEMETRY_INSTANCE_ID,
        GET_TELEMETRY_METHOD_ID,
        onMessage
    );

    // Step 3: Start (blocking)
    std::cout << "[Service] Starting... Press Ctrl+C to stop." << std::endl;
    
    // Prime the CPU reading (first read is always 0)
    readCpuUsage();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    g_app->start();

    // Cleanup
    g_app->stop_offer_service(TELEMETRY_SERVICE_ID, TELEMETRY_INSTANCE_ID);
    g_app->unregister_message_handler(
        TELEMETRY_SERVICE_ID, TELEMETRY_INSTANCE_ID, GET_TELEMETRY_METHOD_ID);
    g_app->unregister_state_handler();

    std::cout << "[Service] Stopped." << std::endl;
    return 0;
}
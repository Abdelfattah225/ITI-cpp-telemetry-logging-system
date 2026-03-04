/**
 * @file rpi_someip_telemetry_service.cpp
 * @brief Phase 7 RPi vSOME/IP Telemetry Service
 *
 * Extends Phase 5 telemetry_service.cpp to respond with ALL metrics:
 *   CPU:<load%>\nRAM:<usage%>\nTEMP:<celsius>\n
 *
 * Communication Model: Request/Response (same as Phase 5)
 *   - Client (PC) sends: Empty request (method call)
 *   - Service (RPi) responds: "CPU:38\nRAM:71\nTEMP:45\n"
 *
 * Service IDs: Same as Phase 5 (0x1234 / 0x5678 / 0x0001)
 *
 * Build on RPi (vsomeip installed in ~/someip-client/):
 *   g++ -std=c++17 -O2 \
 *       -I~/someip-client/include \
 *       -L~/someip-client/lib \
 *       -Wl,-rpath,/home/raspberry/someip-client/lib \
 *       -o rpi_someip_service rpi_someip_telemetry_service.cpp \
 *       -lvsomeip3 -lpthread
 *
 * Run on RPi:
 *   VSOMEIP_CONFIGURATION=vsomeip_service_rpi.json ./rpi_someip_service
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
#include <string>
#include <cmath>

// Service IDs — MUST match the client (SomeIPTelemetryClient.hpp)
constexpr vsomeip::service_t  TELEMETRY_SERVICE_ID   = 0x1234;
constexpr vsomeip::instance_t TELEMETRY_INSTANCE_ID  = 0x5678;
constexpr vsomeip::method_t   GET_TELEMETRY_METHOD_ID = 0x0001;

std::atomic<bool> g_running{true};
std::shared_ptr<vsomeip::application> g_app;

void signalHandler(int signum) {
    std::cout << "\n[RPi Service] Shutting down (signal " << signum << ")\n";
    g_running = false;
    if (g_app) g_app->stop();
}

// ── Telemetry Readers ─────────────────────────────────────────────────────

// ── Global persistent CPU state ─────────────────────────────────────────
static long long g_prevTotal = 0;
static long long g_prevIdle  = 0;
static float     g_lastCpu   = 0.0f;


void updateCpuUsage() {
    std::ifstream f("/proc/stat");
    if (!f.is_open()) return;

    std::string line;
    std::getline(f, line);
    std::istringstream iss(line);
    std::string cpu;
    long long user, nice, system, idle, iowait, irq, softirq, steal;
    iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    long long total     = user + nice + system + idle + iowait + irq + softirq + steal;
    long long idleTime  = idle + iowait;
    long long totalDiff = total    - g_prevTotal;
    long long idleDiff  = idleTime - g_prevIdle;
    g_prevTotal = total;
    g_prevIdle  = idleTime;

    if (totalDiff == 0) return;
    g_lastCpu = 100.0f * (1.0f - (float)idleDiff / (float)totalDiff);
    g_lastCpu = std::max(0.0f, std::min(100.0f, g_lastCpu));
}


float readRamUsage() {
    std::ifstream f("/proc/meminfo");
    if (!f.is_open()) return 0.0f;

    unsigned long long total = 0, available = 0;
    bool gotTotal = false, gotAvail = false;
    std::string line;
    while (std::getline(f, line) && !(gotTotal && gotAvail)) {
        if (line.rfind("MemTotal:", 0) == 0) {
            std::istringstream ss(line); std::string l; ss >> l >> total; gotTotal = true;
        } else if (line.rfind("MemAvailable:", 0) == 0) {
            std::istringstream ss(line); std::string l; ss >> l >> available; gotAvail = true;
        }
    }
    if (total == 0) return 0.0f;
    return (float)(total - available) / (float)total * 100.0f;
}

float readCpuTemp() {
    std::ifstream f("/sys/class/thermal/thermal_zone0/temp");
    if (!f.is_open()) return 0.0f;
    long raw = 0;
    f >> raw;
    return raw / 1000.0f;  // millidegrees → degrees
}

// ── vSOME/IP Callbacks ────────────────────────────────────────────────────

/**
 * @brief Called when PC client sends a request
 * Reads all 3 metrics and sends back: "CPU:38\nRAM:71\nTEMP:45\n"
 */
void onMessage(const std::shared_ptr<vsomeip::message>& request) {
    float cpu  = g_lastCpu;
    float ram  = readRamUsage();
    float temp = readCpuTemp();

    std::string payload =
        "CPU:"  + std::to_string((int)std::round(cpu))  + "\n" +
        "RAM:"  + std::to_string((int)std::round(ram))  + "\n" +
        "TEMP:" + std::to_string((int)std::round(temp)) + "\n";

    std::cout << "[RPi Service] Responding with: " << payload;

    auto runtime         = vsomeip::runtime::get();
    auto response        = runtime->create_response(request);
    auto responsePayload = runtime->create_payload();
    std::vector<vsomeip::byte_t> data(payload.begin(), payload.end());
    responsePayload->set_data(data);
    response->set_payload(responsePayload);
    g_app->send(response);
}

void onState(vsomeip::state_type_e state) {
    if (state == vsomeip::state_type_e::ST_REGISTERED) {
        std::cout << "[RPi Service] Registered — offering service 0x"
                  << std::hex << TELEMETRY_SERVICE_ID << std::dec << "\n";
        g_app->offer_service(TELEMETRY_SERVICE_ID, TELEMETRY_INSTANCE_ID);
    }
}

int main() {
    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "=== RPi Telemetry Service (vSOME/IP Phase 7) ===\n";
    std::cout << "Service: 0x" << std::hex << TELEMETRY_SERVICE_ID
              << "  Instance: 0x" << TELEMETRY_INSTANCE_ID
              << "  Method: 0x"   << GET_TELEMETRY_METHOD_ID << std::dec << "\n";
    std::cout << "Reads: /proc/stat, /proc/meminfo, /sys/class/thermal/thermal_zone0/temp\n\n";

    auto runtime = vsomeip::runtime::get();
    g_app = runtime->create_application("TelemetryService");

    if (!g_app->init()) {
        std::cerr << "[RPi Service] Failed to init vsomeip!\n";
        return 1;
    }

    g_app->register_state_handler(onState);
    g_app->register_message_handler(
        TELEMETRY_SERVICE_ID, TELEMETRY_INSTANCE_ID, GET_TELEMETRY_METHOD_ID, onMessage);
    // Start background CPU sampler (1s interval for accurate deltas)
    updateCpuUsage();  // Prime first reading
    std::thread cpuSampler([]() {
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            updateCpuUsage();
        }
    });

    std::cout << "[RPi Service] Running — waiting for client requests...\n";
    g_app->start();  // blocking

    // Cleanup
    g_running = false;
    if (cpuSampler.joinable()) cpuSampler.join();    // Cleanup
    g_app->stop_offer_service(TELEMETRY_SERVICE_ID, TELEMETRY_INSTANCE_ID);
    g_app->unregister_message_handler(TELEMETRY_SERVICE_ID, TELEMETRY_INSTANCE_ID, GET_TELEMETRY_METHOD_ID);
    g_app->unregister_state_handler();

    std::cout << "[RPi Service] Stopped.\n";
    return 0;
}

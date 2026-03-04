/**
 * @file rpi_telemetry_server.cpp
 * @brief Raspberry Pi Telemetry Server
 *
 * Reads CPU load, CPU temperature, and RAM usage from /proc and /sys,
 * then broadcasts them over a simple TCP connection to the PC dashboard.
 *
 * Protocol (one line per message, sent every 500ms):
 *   CPU:<value>\n
 *   RAM:<value>\n
 *   TEMP:<value>\n
 *
 * Build on RPi:
 *   g++ -std=c++17 -O2 -o rpi_telemetry_server rpi_telemetry_server.cpp -lpthread
 *
 * Run:
 *   ./rpi_telemetry_server 9001
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <atomic>
#include <cmath>

// POSIX networking
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

static std::atomic<bool> g_running{true};

void sigHandler(int) { g_running = false; }

// ── CPU Usage (delta-based like your existing TelemetryParser) ─────────────
struct CpuStats {
    unsigned long long user=0, nice=0, system=0, idle=0,
                       iowait=0, irq=0, softirq=0, steal=0;
    unsigned long long total()  const { return user+nice+system+idle+iowait+irq+softirq+steal; }
    unsigned long long idleAll()const { return idle+iowait; }
};

CpuStats readCpuStats() {
    CpuStats s;
    std::ifstream f("/proc/stat");
    std::string label;
    f >> label >> s.user >> s.nice >> s.system >> s.idle
               >> s.iowait >> s.irq >> s.softirq >> s.steal;
    return s;
}

double getCpuPercent() {
    static CpuStats prev = readCpuStats();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    CpuStats curr = readCpuStats();
    auto totalDiff = curr.total()   - prev.total();
    auto idleDiff  = curr.idleAll() - prev.idleAll();
    prev = curr;
    if (totalDiff == 0) return 0.0;
    return (double)(totalDiff - idleDiff) / totalDiff * 100.0;
}

// ── RAM Usage ─────────────────────────────────────────────────────────────
double getRamPercent() {
    std::ifstream f("/proc/meminfo");
    unsigned long long total=0, available=0;
    bool gotTotal=false, gotAvail=false;
    std::string line;
    while (std::getline(f, line) && !(gotTotal && gotAvail)) {
        if (line.rfind("MemTotal:", 0) == 0) {
            std::istringstream ss(line); std::string l; ss >> l >> total; gotTotal=true;
        } else if (line.rfind("MemAvailable:", 0) == 0) {
            std::istringstream ss(line); std::string l; ss >> l >> available; gotAvail=true;
        }
    }
    if (total == 0) return 0.0;
    return (double)(total - available) / total * 100.0;
}

// ── CPU Temperature ───────────────────────────────────────────────────────
double getCpuTemp() {
    std::ifstream f("/sys/class/thermal/thermal_zone0/temp");
    long raw = 0;
    f >> raw;
    return raw / 1000.0;
}

// ── Clamp helper ─────────────────────────────────────────────────────────
int clamp(double v, double lo=0, double hi=100) {
    return (int)std::min(hi, std::max(lo, v));
}

// ── Handle one connected client ───────────────────────────────────────────
void handleClient(int clientFd) {
    std::cout << "[Server] Client connected, streaming telemetry...\n";
    while (g_running) {
        double cpu  = getCpuPercent();   // ~200ms blocking
        double ram  = getRamPercent();
        double temp = getCpuTemp();

        std::string msg =
            "CPU:"  + std::to_string(clamp(cpu))  + "\n" +
            "RAM:"  + std::to_string(clamp(ram))  + "\n" +
            "TEMP:" + std::to_string(clamp(temp, 0, 120)) + "\n";

        ssize_t sent = send(clientFd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
        if (sent <= 0) {
            std::cout << "[Server] Client disconnected\n";
            break;
        }

        // Sleep remaining ~300ms (getCpuPercent already sleeps 200ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    close(clientFd);
}

int main(int argc, char* argv[]) {
    uint16_t port = 9001;
    if (argc > 1) port = (uint16_t)std::atoi(argv[1]);

    signal(SIGINT,  sigHandler);
    signal(SIGTERM, sigHandler);
    signal(SIGPIPE, SIG_IGN);  // prevent crash on disconnected client

    // Create TCP server socket
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(serverFd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    if (listen(serverFd, 5) < 0) { perror("listen"); return 1; }

    std::cout << "[RPi Telemetry Server] Listening on port " << port << "\n";
    std::cout << "  CPU:  /proc/stat\n";
    std::cout << "  RAM:  /proc/meminfo\n";
    std::cout << "  TEMP: /sys/class/thermal/thermal_zone0/temp\n";
    std::cout << "  Press Ctrl+C to stop\n\n";

    while (g_running) {
        sockaddr_in clientAddr{};
        socklen_t   clientLen = sizeof(clientAddr);

        // Accept with timeout (so Ctrl+C can exit)
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(serverFd, &readSet);
        timeval tv{1, 0};  // 1 second timeout
        if (select(serverFd + 1, &readSet, nullptr, nullptr, &tv) <= 0) continue;

        int clientFd = accept(serverFd, (sockaddr*)&clientAddr, &clientLen);
        if (clientFd < 0) continue;

        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, sizeof(clientIp));
        std::cout << "[Server] Connection from " << clientIp << "\n";

        // Handle client in a thread (supports multiple clients)
        std::thread(handleClient, clientFd).detach();
    }

    close(serverFd);
    std::cout << "[Server] Stopped.\n";
    return 0;
}

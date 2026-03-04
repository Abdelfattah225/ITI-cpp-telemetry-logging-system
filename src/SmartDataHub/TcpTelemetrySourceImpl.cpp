#include "TcpTelemetrySourceImpl.hpp"
#include <cstring>
#include <stdexcept>

namespace SmartDataHub {

TcpTelemetrySourceImpl::TcpTelemetrySourceImpl(const std::string& host, uint16_t port)
    : m_host(host), m_port(port) {}

TcpTelemetrySourceImpl::~TcpTelemetrySourceImpl() {
    if (m_sockfd >= 0) ::close(m_sockfd);
}

bool TcpTelemetrySourceImpl::openSource() {
    m_sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_sockfd < 0) return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(m_port);
    if (::inet_pton(AF_INET, m_host.c_str(), &addr.sin_addr) <= 0) return false;

    return ::connect(m_sockfd, (sockaddr*)&addr, sizeof(addr)) == 0;
}

bool TcpTelemetrySourceImpl::readSource(std::string& out) {
    if (m_sockfd < 0) return false;

    out.clear();
    char buf[256];
    // Read until we have a complete 3-line block (CPU:X\nRAM:X\nTEMP:X\n)
    while (true) {
        ssize_t n = ::recv(m_sockfd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) return false;
        buf[n] = '\0';
        out += buf;
        // A complete block has all 3 keys
        if (out.find("CPU:")  != std::string::npos &&
            out.find("RAM:")  != std::string::npos &&
            out.find("TEMP:") != std::string::npos) {
            return true;
        }
    }
}

} // namespace SmartDataHub

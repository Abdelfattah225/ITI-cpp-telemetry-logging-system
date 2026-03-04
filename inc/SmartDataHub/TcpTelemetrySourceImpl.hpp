#pragma once
/**
 * @file TcpTelemetrySourceImpl.hpp
 * @brief TCP client telemetry source — connects to rpi_telemetry_server
 *
 * Receives lines in format: CPU:38\nRAM:71\nTEMP:27\n
 * readSource() returns one complete "CPU:XX\nRAM:XX\nTEMP:XX\n" block.
 */
#include "ITelemetrySource.hpp"
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace SmartDataHub {

class TcpTelemetrySourceImpl : public ITelemetrySource
{
public:
    TcpTelemetrySourceImpl(const std::string& host, uint16_t port);
    ~TcpTelemetrySourceImpl() override;

    // Non-copyable
    TcpTelemetrySourceImpl(const TcpTelemetrySourceImpl&) = delete;
    TcpTelemetrySourceImpl& operator=(const TcpTelemetrySourceImpl&) = delete;

    bool openSource()  override;
    bool readSource(std::string& out) override;

private:
    std::string m_host;
    uint16_t    m_port;
    int         m_sockfd = -1;
};

} // namespace SmartDataHub

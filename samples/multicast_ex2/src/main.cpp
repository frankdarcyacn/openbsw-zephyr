// Copyright 2025 Accenture.

#include "ip/to_str.h"
#include "util/string/ConstString.h"
#include <iostream>
#include <zephyrEthAdapter/udp/ZephyrDatagramSocket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>

#include <zephyr/logging/log.h>
#include <util/logger/Logger.h>

#include "OpenBSWZephyrLogger.h"

LOG_MODULE_REGISTER(app, 4);

using namespace ::util::string;

#define IPV6_LOCAL_IPADDR "::"
#define IPV6_MULTICAST_IPADDR "ff14:0000:0000:0000:0000:0000:0004:0000"
#define IPV6_MULTICAST_PORT 22222

class DataListener : public ::udp::IDataListener
{
public:
    DataListener() {};

    void dataReceived(
        ::udp::AbstractDatagramSocket& socket,
        ::ip::IPAddress sourceAddress,
        uint16_t sourcePort,
        ::ip::IPAddress destinationAddress,
        uint16_t length) override
        {
            char sourceAddressBuffer[INET6_ADDRSTRLEN+1];
            ip::to_str(sourceAddress, sourceAddressBuffer);
            sourceAddressBuffer[INET6_ADDRSTRLEN] = '\0';
            char destinationAddressBuffer[INET6_ADDRSTRLEN+1];
            ip::to_str(destinationAddress, destinationAddressBuffer);
            destinationAddressBuffer[INET6_ADDRSTRLEN] = '\0';
            size_t maxBytesToRead = sizeof(_receiveData);
            if (length > maxBytesToRead)
            {
                LOG_INF("dataReceived: oversized packet length=%d.", length);
            }
            else
            {
                LOG_INF("dataReceived: packet length=%d.", length);
                maxBytesToRead = length;
            }

            size_t bytesRead = socket.read(&_receiveData[0U], maxBytesToRead);

            LOG_INF("dataReceived: read %d bytes from %s:%d to %s",
                bytesRead,
                sourceAddressBuffer,
                sourcePort,
                destinationAddressBuffer);
            size_t i=0;
            for (; i+3 < bytesRead; i=i+4)
            {
                LOG_INF("dataReceived: %02X%02X%02X%02X", _receiveData[i], _receiveData[i+1], _receiveData[i+2], _receiveData[i+3]);
            }
            for (; i < bytesRead; i++)
            {
                LOG_INF("dataReceived: %02X", _receiveData[i]);
            }
        }
private:
    uint8_t _receiveData[1518U];
};

OpenBSWZephyrLogger openBSWZephyrLogger;

int main(void)
{
    ::util::logger::Logger::init(openBSWZephyrLogger, openBSWZephyrLogger);

    ::udp::ZephyrDatagramSocket socket;
    struct in6_addr addr6;
    zsock_inet_pton(AF_INET6, IPV6_LOCAL_IPADDR, &addr6);
    ::ip::IPAddress ipAddr = ::ip::make_ip6(
        addr6.s6_addr32[0],
        addr6.s6_addr32[1],
        addr6.s6_addr32[2],
        addr6.s6_addr32[3]);
    DataListener dl;
    socket.setDataListener(&dl);

    LOG_INF("Bind socket to %s:%d", IPV6_LOCAL_IPADDR, IPV6_MULTICAST_PORT);
    if(socket.bind(&ipAddr, IPV6_MULTICAST_PORT) == ::udp::AbstractDatagramSocket::ErrorCode::UDP_SOCKET_OK)
    {
        LOG_INF("Socket bound to %s:%d", IPV6_LOCAL_IPADDR, IPV6_MULTICAST_PORT);
    }
    else
    {
        LOG_INF("Failed to bind socket to %s:%d", IPV6_LOCAL_IPADDR, IPV6_MULTICAST_PORT);
        return -1;
    }

    struct in6_addr multicast_addr6;
    zsock_inet_pton(AF_INET6, IPV6_MULTICAST_IPADDR, &multicast_addr6);
    ::ip::IPAddress multicastIpAddr = ::ip::make_ip6(
        multicast_addr6.s6_addr32[0],
        multicast_addr6.s6_addr32[1],
        multicast_addr6.s6_addr32[2],
        multicast_addr6.s6_addr32[3]);

    LOG_INF("Join multicast group %s", IPV6_MULTICAST_IPADDR);
    if(socket.join(multicastIpAddr) == ::udp::AbstractDatagramSocket::ErrorCode::UDP_SOCKET_OK)
    {
        LOG_INF("Joined multicast group %s", IPV6_MULTICAST_IPADDR);
    }
    else
    {
        LOG_INF("Failed to join multicast group %s", IPV6_MULTICAST_IPADDR);
        return -1;
    }

    LOG_INF("Up and running");

    return 0;
}

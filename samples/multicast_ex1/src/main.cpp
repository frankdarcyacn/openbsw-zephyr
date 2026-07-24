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

#define IPV4_LOCAL_IPADDR "0.0.0.0"
#define IPV4_MULTICAST_IPADDR "239.10.20.30"
#define IPV4_MULTICAST_PORT 30490


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
            char sourceAddressBuffer[INET_ADDRSTRLEN+1];
            ip::to_str(sourceAddress, sourceAddressBuffer);
            sourceAddressBuffer[INET_ADDRSTRLEN] = '\0';
            char destinationAddressBuffer[INET_ADDRSTRLEN+1];
            ip::to_str(destinationAddress, destinationAddressBuffer);
            destinationAddressBuffer[INET_ADDRSTRLEN] = '\0';
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
    struct in_addr addr4;
    zsock_inet_pton(AF_INET, IPV4_LOCAL_IPADDR, &addr4);
    ::ip::IPAddress ipAddr = ::ip::make_ip4(
        addr4.s4_addr[0],
        addr4.s4_addr[1],
        addr4.s4_addr[2],
        addr4.s4_addr[3]);
    DataListener dl;
    socket.setDataListener(&dl);

    LOG_INF("Bind socket to %s:%d", IPV4_LOCAL_IPADDR, IPV4_MULTICAST_PORT);
    if(socket.bind(&ipAddr, IPV4_MULTICAST_PORT) == ::udp::AbstractDatagramSocket::ErrorCode::UDP_SOCKET_OK)
    {
        LOG_INF("Socket bound to %s:%d", IPV4_LOCAL_IPADDR, IPV4_MULTICAST_PORT);
    }
    else
    {
        LOG_INF("Failed to bind socket to %s:%d", IPV4_LOCAL_IPADDR, IPV4_MULTICAST_PORT);
        return -1;
    }

    struct in_addr multicast_addr4;
    zsock_inet_pton(AF_INET, IPV4_MULTICAST_IPADDR, &multicast_addr4);
    ::ip::IPAddress multicastIpAddr = ::ip::make_ip4(
        multicast_addr4.s4_addr[0],
        multicast_addr4.s4_addr[1],
        multicast_addr4.s4_addr[2],
        multicast_addr4.s4_addr[3]);

    LOG_INF("Join multicast group %s", IPV4_MULTICAST_IPADDR);
    if(socket.join(multicastIpAddr) == ::udp::AbstractDatagramSocket::ErrorCode::UDP_SOCKET_OK)
    {
        LOG_INF("Joined multicast group %s", IPV4_MULTICAST_IPADDR);
    }
    else
    {
        LOG_INF("Failed to join multicast group %s", IPV4_MULTICAST_IPADDR);
        return -1;
    }

    LOG_INF("Up and running");

    return 0;
}

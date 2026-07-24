// Copyright 2025 Accenture.

#include "zephyrEthAdapter/udp/ZephyrDatagramSocket.h"
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/igmp.h>  // For IPv4
#include <zephyr/net/mld.h>   // For IPv6

#include "zephyrEthAdapter/utils/EthHelper.h"

#include <ip/to_str.h>
#include <udp/DatagramPacket.h>
#include <udp/IDataListener.h>
#include <udp/UdpLogger.h>

namespace udp
{
namespace logger = ::util::logger;
using ::ip::IPAddress;

ZephyrDatagramSocket::ZephyrDatagramSocket()
: AbstractDatagramSocket(), _socket(-1), _netContext(nullptr), _currentPkt(nullptr),
    _isBound(false), _isConnected(false)
{}

bool ZephyrDatagramSocket::isBound() const { return _isBound; }

bool ZephyrDatagramSocket::isConnected() const { return _isConnected; }

bool ZephyrDatagramSocket::isClosed() const { return _socket == -1; }

AbstractDatagramSocket::ErrorCode
ZephyrDatagramSocket::bind(ip::IPAddress const* pIpAddress, uint16_t port)
{
    int res;

    if (_netContext != nullptr)
    {
        logger::Logger::error(logger::UDP, " ZephyrDatagramSocket::bind(): socket already bound!");
        return AbstractDatagramSocket::ErrorCode::UDP_SOCKET_NOT_OK;
    }
    _addressFamily = addressFamilyOf(*pIpAddress);
    if (_addressFamily == ip::IPAddress::IPV4)
    {
        res = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &_netContext);
    }
    else
    {
        res = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &_netContext);
    }
    if (res < 0)
    {
        return ErrorCode::UDP_SOCKET_NOT_OK;
    }

    sockaddr addr = zethutils::toSockAddr(pIpAddress, port);

    res = net_context_bind(_netContext, &addr, sizeof(addr));
    if (res < 0)
    {
        net_context_put(_netContext);
        _netContext = nullptr;
        return ErrorCode::UDP_SOCKET_NOT_OK;
    }

    res = net_context_recv(_netContext, ZephyrDatagramSocket::zsock_received_cb, K_NO_WAIT, this);
    if (res < 0) {
        net_context_put(_netContext);
        _netContext = nullptr;
        return ErrorCode::UDP_SOCKET_NOT_OK;
    }

    _localAddr.emplace(addr);
    _isBound = true;
    return ErrorCode::UDP_SOCKET_OK;
}

void ZephyrDatagramSocket::zsock_received_cb(struct net_context *ctx,
                  struct net_pkt *pkt,
                  union net_ip_header *ip_hdr,
                  union net_proto_header *proto_hdr,
                  int status,
                  void *user_data)
{
    ZephyrDatagramSocket* self = static_cast<ZephyrDatagramSocket*>(user_data);
    self->receivedCallback(ctx, pkt, ip_hdr, proto_hdr, status);
}

void ZephyrDatagramSocket::receivedCallback(struct net_context * /*ctx*/,
                struct net_pkt *pkt,
                union net_ip_header *ip_hdr,
                union net_proto_header *proto_hdr,
                int status)
{
    ip::IPAddress srcAddr;

    if (status != 0 || pkt == nullptr)
    {
        logger::Logger::error(logger::UDP, "ZephyrDatagramSocket::receivedCallback(): Error: %d", status);
        return;
    }
    if (_addressFamily == ip::IPAddress::IPV4)
    {
        srcAddr = ip::make_ip4(ip_hdr->ipv4->src);
    }
    else
    {
        srcAddr = ip::make_ip6(ip_hdr->ipv6->src);
    }
    _currentPkt = pkt;
    _dataListener->dataReceived(
        *this,
        srcAddr,
        ntohs(proto_hdr->udp->src_port),
        zethutils::fromSockAddr(*_localAddr),
        net_pkt_remaining_data(pkt));
    _currentPkt = nullptr;
    net_pkt_unref(pkt);
}

AbstractDatagramSocket::ErrorCode ZephyrDatagramSocket::join(ip::IPAddress const& groupAddr)
{
    if (_netContext == nullptr)
    {
        logger::Logger::error(logger::UDP,
            "ZephyrDatagramSocket::join(): network context not found!");
        return ErrorCode::UDP_SOCKET_NOT_OK;
    }

    struct net_if *iface = net_context_get_iface(_netContext);
    if (iface == NULL)
    {
        logger::Logger::error(logger::UDP,
            "ZephyrDatagramSocket::join(): network interface not found!");
        return ErrorCode::UDP_SOCKET_NOT_OK;
    }

    int ret = 0;
    if (addressFamilyOf(groupAddr) == ip::IPAddress::IPV4)
    {
        struct in_addr addr4;
        addr4.s4_addr32[0] = htonl(ip::ip4_to_u32(groupAddr));
        ret = net_ipv4_igmp_join(iface, &addr4, NULL);
    }
    else
    {
        struct in6_addr addr6;
        addr6.s6_addr32[0] = ip::ip6_to_u32(groupAddr, 0);
        addr6.s6_addr32[1] = ip::ip6_to_u32(groupAddr, 1);
        addr6.s6_addr32[2] = ip::ip6_to_u32(groupAddr, 2);
        addr6.s6_addr32[3] = ip::ip6_to_u32(groupAddr, 3);
        ret = net_ipv6_mld_join(iface, &addr6);
    }

    if(ret == -EALREADY)
    {
        logger::Logger::warn(logger::UDP,
            "ZephyrDatagramSocket::join(): already joined multicast address!");
    }
    else if(ret < 0)
    {
        logger::Logger::error(logger::UDP,
            "ZephyrDatagramSocket::join(): failed to join multicast group! ret=%d", ret);
        return ErrorCode::UDP_SOCKET_NOT_OK;
    }

    return ErrorCode::UDP_SOCKET_OK;
}

size_t ZephyrDatagramSocket::read(uint8_t* buffer, size_t n)
{
    if (_currentPkt == nullptr)
    {
        return 0;
    }
    size_t remaining = net_pkt_remaining_data(_currentPkt);
    if (n > remaining)
    {
        n = remaining;
    }
    int res = net_pkt_read(_currentPkt, buffer, n);
    if (res == 0)
    {
        return n;
    }
    else
    {
        return 0;
    }
}

AbstractDatagramSocket::ErrorCode ZephyrDatagramSocket::connect(
    ip::IPAddress const& address, uint16_t port, ip::IPAddress* pLocalAddress)
{
    if (_isConnected)
    {
        logger::Logger::error(logger::UDP, " ZephyrDatagramSocket::connect(): socket already connected!");
        return ErrorCode::UDP_SOCKET_OK;
    }
    if (_netContext == nullptr)
    {
        logger::Logger::error(logger::UDP, " ZephyrDatagramSocket::connect(): socket not bound!");
        return ErrorCode::UDP_SOCKET_NOT_OK;
    }
    _peerAddr.emplace(zethutils::toSockAddr(&address, port));
    if (net_context_connect(_netContext, (struct sockaddr*)&(*_peerAddr), sizeof(*_peerAddr), NULL, K_NO_WAIT, NULL) < 0)
    {
        return ErrorCode::UDP_SOCKET_NOT_OK;
    }
    _isConnected = true;

    if (pLocalAddress && _localAddr.has_value())
    {
        *pLocalAddress = zethutils::fromSockAddr(*_localAddr);
    }
    return ErrorCode::UDP_SOCKET_OK;
}

void ZephyrDatagramSocket::disconnect()
{
    if (_netContext != nullptr)
    {
        // we only simulate disconnected state
        _isConnected = false;
        _peerAddr.reset();
    }
}

AbstractDatagramSocket::ErrorCode
ZephyrDatagramSocket::send(::etl::span<uint8_t const> const& data)
{
    if (_netContext == nullptr)
    {
        logger::Logger::error(logger::UDP, " ZephyrDatagramSocket::send(): socket not bound!");
        return ErrorCode::UDP_SOCKET_NOT_OK;
    }
    if (!_isConnected)
    {
        logger::Logger::error(logger::UDP, " ZephyrDatagramSocket::send(): socket not connected!");
        return ErrorCode::UDP_SOCKET_NOT_OK;
    }
    if (net_context_send(_netContext, data.data(), 
                         data.size(), NULL, K_NO_WAIT /* timeout unused */, NULL ) != 0)
    {
        return ErrorCode::UDP_SOCKET_NOT_OK;
    }
    return ErrorCode::UDP_SOCKET_OK;
}

AbstractDatagramSocket::ErrorCode ZephyrDatagramSocket::send(DatagramPacket const& packet)
{
    if (_netContext == nullptr)
    {
        logger::Logger::error(logger::UDP, " ZephyrDatagramSocket::send(): socket not bound!");
        return ErrorCode::UDP_SOCKET_NOT_OK;
    }
    ip::IPAddress ip           = packet.getAddress();
    ip::IPAddress const* ipPtr = &ip;
    sockaddr destAddr       = zethutils::toSockAddr(ipPtr, packet.getPort());

    if (net_context_sendto(_netContext, packet.getData(), 
                           packet.getLength(), 
                           &destAddr,
                           sizeof(destAddr),
                           NULL, K_NO_WAIT /* timeout unused */, NULL ) != 0)
    {
        return ErrorCode::UDP_SOCKET_NOT_OK;
    }
    return ErrorCode::UDP_SOCKET_OK;
}

void ZephyrDatagramSocket::close()
{
    if (_netContext == nullptr)
    {
        logger::Logger::debug(logger::UDP, "ZephyrDatagramSocket::close(): Socket already closed");
        return;
    }
    // Reset Callbacks
    (void)net_context_recv(_netContext, NULL, K_NO_WAIT, NULL);
    if (net_context_put(_netContext) < 0)
    {
        logger::Logger::error(logger::UDP, "ZephyrDatagramSocket::close(): Error closing socket %p", _netContext);
    }

    _netContext = nullptr;
    _isBound     = false;
    _isConnected = false;
    _localAddr.reset();
    _peerAddr.reset();

    logger::Logger::debug(logger::UDP, "ZephyrDatagramSocket::close(): Socket %p closed", _netContext);
}

ip::IPAddress const* ZephyrDatagramSocket::getIPAddress() const
{
    if (_localAddr.has_value())
    {
        static ip::IPAddress _localIpAddress = zethutils::fromSockAddr(*_localAddr);
        return &_localIpAddress;
    }
    return nullptr;
}

ip::IPAddress const* ZephyrDatagramSocket::getLocalIPAddress() const { return getIPAddress(); }

uint16_t ZephyrDatagramSocket::getPort() const
{
    if (_localAddr.has_value())
    {
        if (_addressFamily == ip::IPAddress::IPV4)
        {
            sockaddr_in addr = reinterpret_cast<sockaddr_in const&>(*(_localAddr));
            return ntohs(addr.sin_port);
        }
        else
        {
            sockaddr_in6 addr = reinterpret_cast<sockaddr_in6 const&>(*(_localAddr));
            return ntohs(addr.sin6_port);
        }
    }
    return 0;
}

uint16_t ZephyrDatagramSocket::getLocalPort() const { return getPort(); }

} // namespace udp

/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/

#include "cadence/ipendpoint.hpp"

#include <solace/posixErrorDomain.hpp>

#include <cstring>
#include <arpa/inet.h>


using namespace Solace;
using namespace cadence;



String
IPAddress::toString() const {
    char buffer[INET6_ADDRSTRLEN];

    const char *res = (isV4())
            ? inet_ntop(AF_INET, &_addr.ipv4, buffer, sizeof(buffer))
            : inet_ntop(AF_INET6, &_addr.ipv6, buffer, sizeof(buffer));

    return res
            ? makeString(res)
            : makeString("");
}


String
IPEndpoint::toString() const {
    // FIXME(abbyssoul): Should use StringBuilder instead.
    char buffer[6]; // port is u16, so no more then 65535 which is 5 char long.
    snprintf(buffer, sizeof (buffer), "%hu", _port);

    return makeStringJoin(_ipAddress.toString(), ":", StringView(buffer));
}

std::array<unsigned char, 4>
IPAddress::getBytes_v4() const {
    std::array<unsigned char, 4> a;
    std::memcpy(a.data(), _addr.bytes, a.size());

    return a;
}


std::array<unsigned char, 16>
IPAddress::getBytes_v6() const {
    std::array<unsigned char, 16> a;
    std::memcpy(a.data(), _addr.bytes, a.size());

    return a;
}


IPAddress::IPAddress(std::array<unsigned char, 4> const& addr)
    : _isV4(true)
{
    std::memcpy(_addr.bytes, addr.data(), addr.size());
}


IPAddress::IPAddress(std::array<unsigned char, 16> const& addr)
    : _isV4(!true)
{
    std::memcpy(_addr.bytes, addr.data(), addr.size());
}


IPAddress IPAddress::any() {
    return IPAddress(in_addr{htonl(INADDR_ANY)});
}

IPAddress IPAddress::loopback() {
    return IPAddress(in_addr{htonl(INADDR_LOOPBACK)});
}

IPAddress IPAddress::broadcast() {
    return IPAddress(in_addr{htonl(INADDR_BROADCAST)});
}



bool IPAddress::isLoopback() const noexcept {
    return (isV4())
            ? (_addr.ipv4.s_addr == htonl(INADDR_LOOPBACK))
            : ((_addr.ipv6.s6_addr[0] == 0) && (_addr.ipv6.s6_addr[1] == 0)
            && (_addr.ipv6.s6_addr[2] == 0) && (_addr.ipv6.s6_addr[3] == 0)
            && (_addr.ipv6.s6_addr[4] == 0) && (_addr.ipv6.s6_addr[5] == 0)
            && (_addr.ipv6.s6_addr[6] == 0) && (_addr.ipv6.s6_addr[7] == 0)
            && (_addr.ipv6.s6_addr[8] == 0) && (_addr.ipv6.s6_addr[9] == 0)
            && (_addr.ipv6.s6_addr[10] == 0) && (_addr.ipv6.s6_addr[11] == 0)
            && (_addr.ipv6.s6_addr[12] == 0) && (_addr.ipv6.s6_addr[13] == 0)
            && (_addr.ipv6.s6_addr[14] == 0) && (_addr.ipv6.s6_addr[15] == 1));
}


bool IPAddress::isUnspecified() const noexcept {
    return (isV4())
            ? (_addr.ipv4.s_addr == 0)
            : ((_addr.ipv6.s6_addr[0] == 0) && (_addr.ipv6.s6_addr[1] == 0)
            && (_addr.ipv6.s6_addr[2] == 0) && (_addr.ipv6.s6_addr[3] == 0)
            && (_addr.ipv6.s6_addr[4] == 0) && (_addr.ipv6.s6_addr[5] == 0)
            && (_addr.ipv6.s6_addr[6] == 0) && (_addr.ipv6.s6_addr[7] == 0)
            && (_addr.ipv6.s6_addr[8] == 0) && (_addr.ipv6.s6_addr[9] == 0)
            && (_addr.ipv6.s6_addr[10] == 0) && (_addr.ipv6.s6_addr[11] == 0)
            && (_addr.ipv6.s6_addr[12] == 0) && (_addr.ipv6.s6_addr[13] == 0)
            && (_addr.ipv6.s6_addr[14] == 0) && (_addr.ipv6.s6_addr[15] == 0));
}


bool IPAddress::isMulticast() const noexcept {
    return (isV4())
            ? (_addr.ipv4.s_addr & 0xF0000000) == 0xE0000000
            : (_addr.ipv6.s6_addr[0] == 0xff);
}


Result<IPAddress, Error>
cadence::parseIPAddress(StringView str) {
    in_addr ip4_addr;
    in6_addr ip6_addr;

    if (inet_pton(AF_INET, str.data(), &ip4_addr)) {
        return Ok(IPAddress{ip4_addr});
    } else if (inet_pton(AF_INET6, str.data(), &ip6_addr)) {
        return Ok(IPAddress{ip6_addr});
    }

    return Err(makeError(BasicError::InvalidInput, "parseIPAddress"));
}

Result<IPEndpoint, Error>
cadence::parseIPEndpoint(StringView str) {

    return Ok(IPEndpoint{IPAddress::any(), 0});
}

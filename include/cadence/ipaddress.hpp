/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: IP Endpoint
 *	@file		cadence/ipendpoint.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_IPADDRESS_HPP
#define CADENCE_IPADDRESS_HPP

#include <solace/types.hpp>
#include <solace/stringView.hpp>
#include <solace/string.hpp>
#include <solace/result.hpp>
#include <solace/error.hpp>


#include <netinet/in.h>

#include <array>

namespace cadence {

/// Implements IP version 4 address
class IPAddress {
public:

    /// Get an IPv4 address that represents any address
    static IPAddress any();

    /// Get an IPv4 address that represents the loopback address, 0x7F000001
    static IPAddress loopback();

    /// Get an IPv4 address that represents the broadcast address, 0xFFFFFFFF
    static IPAddress broadcast();

public:

    /// Default constructor.
    constexpr IPAddress() noexcept
        : _addr{{0}}
    { }

    /// Construct an address from raw bytes.
    explicit IPAddress(in_addr addr)
        : _isV4(true)
    {
        _addr.ipv4 = addr;
    }

    explicit IPAddress(in6_addr addr)
    {
        _addr.ipv6 = addr;
    }


    explicit IPAddress(std::array<unsigned char, 4> const& addr);
    explicit IPAddress(std::array<unsigned char, 16> const& addr);


    /// Construct an address from an unsigned integer in host byte order.
//    explicit IPAddress(uint32_t addr) noexcept;

    /// Copy constructor.
    IPAddress(IPAddress const& rhs) noexcept = default;

    /// Move constructor.
    IPAddress(IPAddress&& other) noexcept
        : _addr(other._addr)
        , _isV4(other._isV4)
    {
    }

    /// Move-assign from another address.
    IPAddress& operator= (IPAddress&& other) noexcept {
      _addr = other._addr;
      _isV4 = other._isV4;
      return *this;
    }

    /// Assign from another address.
    IPAddress& operator=(const IPAddress& other) noexcept = default;


    /// Determine whether the address is a multicast address.
    bool isMulticast() const noexcept;

    /// Determine whether the address is a loopback address.
    bool isLoopback() const noexcept;

    /// Determine whether the address is unspecified.
    bool isUnspecified() const noexcept;

    bool isV4() const noexcept { return _isV4; }
    bool isV6() const noexcept { return !_isV4; }

    std::array<unsigned char, 4>    getBytes_v4() const;
    std::array<unsigned char, 16>   getBytes_v6() const;

    Solace::String toString() const;

private:

    // The underlying IPv4 or IPv6 address.
    union {
        in_addr         ipv4;
        in6_addr        ipv6;
        unsigned char   bytes[16];
    } _addr;

    bool _isV4{};
};


inline
bool operator== (IPAddress const& lhs, IPAddress const& rhs) {
    if (lhs.isV4() && rhs.isV4())
        return lhs.getBytes_v4() == rhs.getBytes_v4();
    else if (lhs.isV6() && rhs.isV6())
        return lhs.getBytes_v6() == rhs.getBytes_v6();
    else
        return false;
}


Solace::Result<IPAddress, Solace::Error>
parseIPAddress(Solace::StringView str);


}  // End of namespace cadence
#endif  // CADENCE_IPADDRESS_HPP

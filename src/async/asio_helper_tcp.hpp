/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
#pragma once
#ifndef CADENCE_ASIO_HELPER_TCP_HPP
#define CADENCE_ASIO_HELPER_TCP_HPP

#include "cadence/ipendpoint.hpp"
#include "cadence/networkEndpoint.hpp"

#include <asio/ip/tcp.hpp>
#include <asio/ip/udp.hpp>

namespace cadence {

inline
asio::ip::address toAsioIPAddress(IPAddress addr) {
    if (addr.isV4()) {
        return asio::ip::address_v4(addr.getBytes_v4());
    } else {
        return asio::ip::address_v6(addr.getBytes_v6());
    }
}

/**
 * Convert cadence::IPEndpoint into asio tcp::endpoint.
 * This is not throwing version.
 *
 * @param addr IPEndpoint to convert.
 * @return asio endpoint represening the same input.
 */
inline
asio::ip::tcp::endpoint toAsioIPEndpoint(IPEndpoint const& addr) {
    return {toAsioIPAddress(addr.getAddress()), addr.getPort()};
}


/**
 * Convert cadence::IPEndpoint into asio tcp::endpoint.
 * Note: This is exception throwing version. It is to be used only in constructor where using error codes
 * is not an option.
 *
 * @param addr IPEndpoint to convert.
 * @return asio endpoint represening the same input.
 */
inline
asio::ip::tcp::endpoint toAsioIPEndpoint(NetworkEndpoint const& addr, asio::error_code& ec) {

    return std::visit([&ec](auto&& arg) -> asio::ip::tcp::endpoint {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, IPEndpoint>) {
            return toAsioIPEndpoint(arg);
        } else {
                ec = asio::error::make_error_code(asio::error::basic_errors::address_family_not_supported);
                return toAsioIPEndpoint(IPEndpoint{IPAddress::any(), 0});
        }
    }, addr);
}

inline
IPEndpoint fromAsioEndpoint(asio::ip::tcp::endpoint const& addr) {
    return IPEndpoint( addr.address().is_v4()
                       ? IPAddress(addr.address().to_v4().to_bytes())
                       : IPAddress(addr.address().to_v6().to_bytes())
                       , addr.port());
}

inline
IPEndpoint fromAsioEndpoint(asio::ip::udp::endpoint const& addr) {
    return IPEndpoint( addr.address().is_v4()
                       ? IPAddress(addr.address().to_v4().to_bytes())
                       : IPAddress(addr.address().to_v6().to_bytes())
                       , addr.port());
}

}  // end of namespace cadence
#endif  // CADENCE_ASIO_HELPER_TCP_HPP

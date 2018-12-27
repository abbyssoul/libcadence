/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
#pragma once
#ifndef CADENCE_ASIO_HELPER_LOCAL_HPP
#define CADENCE_ASIO_HELPER_LOCAL_HPP


#include "cadence/unixDomainEndpoint.hpp"
#include "cadence/networkEndpoint.hpp"

#include <asio/local/stream_protocol.hpp>
#include <asio/local/datagram_protocol.hpp>


namespace cadence {


inline
asio::local::stream_protocol::endpoint toAsioLocalEndpoint(NetworkEndpoint const& addr, asio::error_code& ec) {

    return std::visit([&ec](auto&& arg) -> asio::local::stream_protocol::endpoint {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, UnixEndpoint>) {
            return asio::local::stream_protocol::endpoint(arg.toString().to_str());
        } else {
            ec = asio::error::make_error_code(asio::error::basic_errors::address_family_not_supported);
            return  asio::local::stream_protocol::endpoint();
        }
    }, addr);
}


inline
NetworkEndpoint fromAsioEndpoint(asio::local::stream_protocol::endpoint const& addr) {
    const auto str = addr.path();
    return UnixEndpoint(makeString(str.data(), str.size()));
}

}  // end of namespace cadence
#endif  // CADENCE_ASIO_HELPER_LOCAL_HPP

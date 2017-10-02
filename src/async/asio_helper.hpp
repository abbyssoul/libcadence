/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
#pragma once
#ifndef CADENCE_ASIO_HELPER_HPP
#define CADENCE_ASIO_HELPER_HPP

#include "cadence/ipendpoint.hpp"
#include "asio.hpp"

namespace cadence {

    inline
    asio::ip::tcp::endpoint toAsioTCPEndpoint(const IPEndpoint& addr) {
        return asio::ip::tcp::endpoint(asio::ip::make_address(addr.getAddress().c_str()), addr.getPort());
    }

    inline
    asio::ip::udp::endpoint toAsioEndpoint(const IPEndpoint& addr) {
        auto ip = asio::ip::make_address(addr.getAddress().c_str());
        return asio::ip::udp::endpoint(ip, addr.getPort());
    }


    inline
    IPEndpoint fromAsioEndpoint(const asio::ip::tcp::endpoint& addr) {
        return IPEndpoint(addr.address().to_string(), addr.port());
    }

    inline
    IPEndpoint fromAsioEndpoint(const asio::ip::udp::endpoint& addr) {
        return IPEndpoint(addr.address().to_string(), addr.port());
    }

    inline
    asio::mutable_buffer asio_buffer(Solace::ByteBuffer& dest, Solace::ByteBuffer::size_type bytes) {
        return asio::buffer(dest.viewRemaining().slice(0, bytes).dataAddress(), bytes);
    }

}  // end of namespace cadence
#endif  // CADENCE_ASIO_HELPER_HPP

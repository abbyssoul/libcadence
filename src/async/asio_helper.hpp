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

#include <solace/error.hpp>

namespace cadence {

    inline asio::io_service& asAsioService(void* ioservice) {
        return *static_cast<asio::io_service*>(ioservice);
    }

    /**
     * Convert cadence::IPEndpoint into asio tcp::endpoint.
     * This is not throwing version.
     *
     * @param addr IPEndpoint to convert.
     * @return asio endpoint represening the same input.
     */
    inline
    asio::ip::tcp::endpoint toAsioTCPEndpoint(const IPEndpoint& addr, asio::error_code& ec) {
        auto asioAddr = asio::ip::make_address(addr.getAddress().c_str(), ec);

        if (ec) {
            return asio::ip::tcp::endpoint();
        }

        return asio::ip::tcp::endpoint(asioAddr, addr.getPort());
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
    asio::ip::tcp::endpoint toAsioTCPEndpoint(const IPEndpoint& addr) {
        return asio::ip::tcp::endpoint(asio::ip::make_address(addr.getAddress().c_str()), addr.getPort());
    }

    inline
    asio::ip::udp::endpoint toAsioEndpoint(const IPEndpoint& addr, asio::error_code& ec) {
        auto ip = asio::ip::make_address(addr.getAddress().c_str(), ec);

        if (ec) {
            return asio::ip::udp::endpoint();
        }

        return asio::ip::udp::endpoint(ip, addr.getPort());
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
    Solace::Error fromAsioError(const asio::error_code& err) {
        return Solace::Error(err.message(), err.value());
    }


    inline
    asio::mutable_buffer asio_buffer(Solace::ByteBuffer& dest, Solace::ByteBuffer::size_type bytes) {
        return asio::buffer(dest.viewRemaining().slice(0, bytes).dataAddress(), bytes);
    }

}  // end of namespace cadence
#endif  // CADENCE_ASIO_HELPER_HPP

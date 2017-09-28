/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * @file: async/DatagramDomainSocket.cpp
*******************************************************************************/
#include <cadence/async/datagramdomainsocket.hpp>


#include "asio.hpp"

#include <iostream>

using namespace Solace;
using namespace cadence::async;


DatagramDomainSocket::DatagramDomainSocket(EventLoop& ioContext, const endpoint_type& point) :
    _socket(ioContext.getIOService(), asio::local::datagram_protocol::endpoint(point.to_str()))
{
}

DatagramDomainSocket::DatagramDomainSocket(DatagramDomainSocket&& rhs) :
    _socket(std::move(rhs._socket)), _socket_endpoint(std::move(rhs._socket_endpoint))
{
}


Future<void>
DatagramDomainSocket::asyncRead(ByteBuffer& dest, std::size_t bytesToRead, endpoint_type serviceName) {
    Promise<void> promise;
    auto f = promise.getFuture();

    asio::local::datagram_protocol::endpoint destination(serviceName.c_str());
    _socket.async_receive_from(asio_buffer(dest, bytesToRead), destination,
        [pm = std::move(promise), &dest](const asio::error_code& error, std::size_t length) mutable {
        if (error) {
            pm.setError(Solace::Error(error.message(), error.value()));
        } else {
            dest.advance(length);
            pm.setValue();
        }
    });

    return f;
}


Future<void>
DatagramDomainSocket::asyncWrite(ByteBuffer& src, std::size_t bytesToWrite, endpoint_type serviceName) {
    Promise<void> promise;
    auto f = promise.getFuture();

    asio::local::datagram_protocol::endpoint destination(serviceName.c_str());
    _socket.async_send_to(asio_buffer(src, bytesToWrite), destination,
        [pm = std::move(promise), &src](const asio::error_code& error, std::size_t bytesTransferred) mutable {
        if (error) {
            pm.setError(Solace::Error(error.message(), error.value()));
        } else {
            src.advance(bytesTransferred);
            pm.setValue();
        }
    });

    return f;
}

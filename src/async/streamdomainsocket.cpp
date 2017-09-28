/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * @file: async/StreamDomainSocket.cpp
*******************************************************************************/
#include <cadence/async/streamdomainsocket.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "asio.hpp"


using namespace Solace;
using namespace cadence::async;


StreamDomainSocket::StreamDomainSocket(EventLoop& ioContext) :
	_socket(ioContext.getIOService())
{
}

StreamDomainSocket::StreamDomainSocket(StreamDomainSocket&& rhs) :
_socket(std::move(rhs._socket))
{
}

void StreamDomainSocket::connect(endpoint_type server) {
    _socket.connect(asio::local::stream_protocol::endpoint(server.c_str()));
}


Future<void>
StreamDomainSocket::asyncConnect(const endpoint_type& endpoint) {
    Promise<void> promise;
    auto f = promise.getFuture();

    asio::local::stream_protocol::endpoint dest(endpoint.to_str());

    _socket.async_connect(dest, [pm = std::move(promise)] (const asio::error_code& error) mutable {
        if (error) {
            pm.setError(Solace::Error(error.message(), error.value()));
        } else {
            pm.setValue();
    	}
    });

    return f;
}


Future<void>
StreamDomainSocket::asyncRead(ByteBuffer& dest, std::size_t bytesToRead) {
    Promise<void> promise;
    auto f = promise.getFuture();

    asio::async_read(_socket, asio_buffer(dest, bytesToRead),
        [pm = std::move(promise), &dest] (const asio::error_code& error, std::size_t bytes_transferred) mutable {
    		dest.advance(bytes_transferred);
            if (error) {
                pm.setError(Solace::Error(error.message(), error.value()));
            } else {
                pm.setValue();
    		}
    	});

    return f;
}


Future<void>
StreamDomainSocket::asyncWrite(ByteBuffer& src, std::size_t bytesToWrite) {
    Promise<void> promise;
    auto f = promise.getFuture();

    asio::async_write(_socket, asio_buffer(src, bytesToWrite),
        [pm = std::move(promise), &src] (const asio::error_code& error, std::size_t bytes_transferred) mutable {
    		src.advance(bytes_transferred);
            if (error) {
                pm.setError(Solace::Error(error.message(), error.value()));
            } else {
                pm.setValue();
    		}
    	});

    return f;
}


StreamDomainAcceptor::StreamDomainAcceptor(EventLoop& ioContext, const String& file) :
    _acceptor(ioContext.getIOService(), asio::local::stream_protocol::endpoint(file.to_str()))
{
}


Future<void>
StreamDomainAcceptor::asyncAccept(StreamDomainSocket& socket) {
    Promise<void> promise;
    auto f = promise.getFuture();

    _acceptor.async_accept(socket._socket, [pm = std::move(promise)](const asio::error_code& error) mutable {
        if (error) {
            pm.setError(Solace::Error(error.message(), error.value()));
        } else {
            pm.setValue();
        }
    });

    return f;
}

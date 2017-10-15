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

#include "asio_helper.hpp"


using namespace Solace;
using namespace cadence::async;


class StreamDomainSocket::StreamDomainSocketImpl {
public:

    StreamDomainSocketImpl(void* ioservice) :
        _socket(*static_cast<asio::io_service*>(ioservice))
    {}

    StreamDomainSocketImpl(StreamDomainSocketImpl&& other) :
        _socket(std::move(other._socket))
    {}

    void connect(const String& server) {
        _socket.connect(asio::local::stream_protocol::endpoint(server.c_str()));
    }


    Future<void> asyncConnect(const String& endpoint) {
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
    asyncRead(ByteBuffer& dest, std::size_t bytesToRead) {
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
    asyncWrite(ByteBuffer& src, std::size_t bytesToWrite) {
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


    asio::local::stream_protocol::socket& getSocket() noexcept { return _socket; }

private:
    asio::local::stream_protocol::socket _socket;
};


StreamDomainSocket::~StreamDomainSocket()
{

}

StreamDomainSocket::StreamDomainSocket(EventLoop& ioContext) :
    StreamSocket(ioContext),
    _pimpl(std::make_unique<StreamDomainSocketImpl>(ioContext.getIOService()))
{
}

StreamDomainSocket::StreamDomainSocket(StreamDomainSocket&& rhs) :
    StreamSocket(std::move(rhs)),
    _pimpl(std::make_unique<StreamDomainSocketImpl>(std::move(*rhs._pimpl.get())))
{
}


StreamDomainSocket& StreamDomainSocket::swap(StreamDomainSocket& rhs) noexcept {
    using std::swap;
    swap(_pimpl, rhs._pimpl);

    return *this;
}


void StreamDomainSocket::connect(const NetworkEndpoint& endpoint) {
    _pimpl->connect(endpoint.toString());
}


Future<void>
StreamDomainSocket::asyncConnect(const NetworkEndpoint& endpoint) {
    return _pimpl->asyncConnect(endpoint.toString());
}


Future<void>
StreamDomainSocket::asyncRead(ByteBuffer& dest, size_type bytesToRead) {
    return _pimpl->asyncRead(dest, bytesToRead);
}


Future<void>
StreamDomainSocket::asyncWrite(ByteBuffer& src, size_type bytesToWrite) {
    return _pimpl->asyncWrite(src, bytesToWrite);
}



class StreamDomainAcceptor::AcceptorImpl {
public:

    AcceptorImpl(void* ioservice, const String& file) :
        _acceptor(*static_cast<asio::io_service*>(ioservice), asio::local::stream_protocol::endpoint(file.to_str()))
    {
    }

    Future<void> asyncAccept(StreamDomainSocket::StreamDomainSocketImpl* socket) {
        Promise<void> promise;
        auto f = promise.getFuture();

        _acceptor.async_accept(socket->getSocket(), [pm = std::move(promise)](const asio::error_code& error) mutable {
            if (error) {
                pm.setError(Solace::Error(error.message(), error.value()));
            } else {
                pm.setValue();
            }
        });

        return f;
    }

private:
    asio::local::stream_protocol::acceptor _acceptor;
};

StreamDomainAcceptor::~StreamDomainAcceptor()
{
}


StreamDomainAcceptor::StreamDomainAcceptor(EventLoop& ioContext, const UnixEndpoint& endpoint) :
    _pimpl(std::make_unique<AcceptorImpl>(ioContext.getIOService(), endpoint.toString().to_str()))
{
}


Future<void>
StreamDomainAcceptor::asyncAccept(StreamDomainSocket& socket) {
    return _pimpl->asyncAccept(socket._pimpl.get());
}

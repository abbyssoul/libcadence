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


#include "asio_helper.hpp"


using namespace Solace;
using namespace cadence;
using namespace cadence::async;


class DatagramDomainSocket::SocketImpl {
public:


    SocketImpl(void* ioservice, const String& endpoing) :
        _socket(*static_cast<asio::io_service*>(ioservice),
            asio::local::datagram_protocol::endpoint(endpoing.to_str()))
    {}

    SocketImpl(void* ioservice) :
        _socket(*static_cast<asio::io_service*>(ioservice))
    {}


    Result<void, Error> connect(const NetworkEndpoint& endpoint) {
        asio::local::datagram_protocol::endpoint destination(endpoint.toString().c_str());
        asio::error_code ec;

        _socket.connect(destination, ec);
        if (ec) {
            return Err(fromAsioError(ec));
        }

        return Ok();
    }

    Future<void> asyncConnect(const NetworkEndpoint& peer) {
        Promise<void> promise;
        auto f = promise.getFuture();

        asio::local::datagram_protocol::endpoint endpoint(peer.toString().to_str());
        _socket.async_connect(endpoint,
            [pm = std::move(promise)](const asio::error_code& error) mutable {
            if (error) {
                pm.setError(fromAsioError(error));
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

        _socket.async_receive(asio_buffer(dest, bytesToRead),
            [pm = std::move(promise), &dest](const asio::error_code& error, std::size_t length) mutable {
            if (error) {
                pm.setError(fromAsioError(error));
            } else {
                dest.advance(length);
                pm.setValue();
            }
        });

        return f;
    }


    Future<void>
    asyncReadFrom(ByteBuffer& dest, std::size_t bytesToRead, const NetworkEndpoint& endpoint) {
        Promise<void> promise;
        auto f = promise.getFuture();

        asio::local::datagram_protocol::endpoint destination(endpoint.toString().c_str());
        _socket.async_receive_from(asio_buffer(dest, bytesToRead), destination,
            [pm = std::move(promise), &dest](const asio::error_code& error, std::size_t length) mutable {
            if (error) {
                pm.setError(fromAsioError(error));
            } else {
                dest.advance(length);
                pm.setValue();
            }
        });

        return f;
    }


    Future<void>
    asyncWrite(ByteBuffer& src, std::size_t bytesToWrite) {
        Promise<void> promise;
        auto f = promise.getFuture();

        _socket.async_send(asio_buffer(src, bytesToWrite),
            [pm = std::move(promise), &src](const asio::error_code& error, std::size_t bytesTransferred) mutable {
            if (error) {
                pm.setError(fromAsioError(error));
            } else {
                src.advance(bytesTransferred);
                pm.setValue();
            }
        });

        return f;
    }


    Future<void>
    asyncWriteTo(ByteBuffer& src, std::size_t bytesToWrite, const NetworkEndpoint& endpoint) {
        Promise<void> promise;
        auto f = promise.getFuture();

        asio::local::datagram_protocol::endpoint destination(endpoint.toString().c_str());
        _socket.async_send_to(asio_buffer(src, bytesToWrite), destination,
            [pm = std::move(promise), &src](const asio::error_code& error, std::size_t bytesTransferred) mutable {
            if (error) {
                pm.setError(fromAsioError(error));
            } else {
                src.advance(bytesTransferred);
                pm.setValue();
            }
        });

        return f;
    }


    void cancel() {
        _socket.cancel();
    }

    void close() {
        _socket.close();
    }


    bool isOpen() {
        return _socket.is_open();
    }

    UnixEndpoint getLocalEndpoint() const {
        return {_socket.local_endpoint().path()};
    }

    UnixEndpoint getRemoteEndpoint() const {
        return {_socket.remote_endpoint().path()};
    }

    void shutdown() {
        _socket.shutdown(asio::local::datagram_protocol::socket::shutdown_both);
    }


private:
    asio::local::datagram_protocol::socket      _socket;
};



DatagramDomainSocket::~DatagramDomainSocket()
{
}


DatagramDomainSocket::DatagramDomainSocket(EventLoop& ioContext) :
    Channel(ioContext),
    _pimpl(std::make_unique<SocketImpl>(ioContext.getIOService()))
{
}


DatagramDomainSocket::DatagramDomainSocket(EventLoop& ioContext, const NetworkEndpoint& endpoint) :
    Channel(ioContext),
    _pimpl(std::make_unique<SocketImpl>(ioContext.getIOService(), endpoint.toString()))
{
}



DatagramDomainSocket::DatagramDomainSocket(DatagramDomainSocket&& rhs) :
    Channel(std::move(rhs)),
    _pimpl(std::move(rhs._pimpl))
{
}


DatagramDomainSocket& DatagramDomainSocket::swap(DatagramDomainSocket& rhs) noexcept {
    using std::swap;
    swap(_pimpl, rhs._pimpl);

    return *this;
}



Future<void> DatagramDomainSocket::asyncConnect(const NetworkEndpoint& endpoint) {
    return _pimpl->asyncConnect(endpoint);
}

Future<void>
DatagramDomainSocket::asyncRead(ByteBuffer& dest, size_type bytesToRead) {
    return _pimpl->asyncRead(dest, bytesToRead);
}

Future<void>
DatagramDomainSocket::asyncReadFrom(ByteBuffer& dest, std::size_t bytesToRead, const NetworkEndpoint& endpoint) {
    return _pimpl->asyncReadFrom(dest, bytesToRead, endpoint);
}

Future<void>
DatagramDomainSocket::asyncWrite(ByteBuffer& src, size_type bytesToWrite)  {
    return _pimpl->asyncWrite(src, bytesToWrite);
}

Future<void>
DatagramDomainSocket::asyncWriteTo(ByteBuffer& src, std::size_t bytesToWrite, const NetworkEndpoint& endpoint) {
    return _pimpl->asyncWriteTo(src, bytesToWrite, endpoint);
}

void DatagramDomainSocket::cancel() {
    _pimpl->cancel();
}

void DatagramDomainSocket::close() {
    _pimpl->close();
}

void DatagramDomainSocket::connect(const NetworkEndpoint& endpoint) {
    _pimpl->connect(endpoint);
}

bool DatagramDomainSocket::isOpen() {
    return _pimpl->isOpen();
}

bool DatagramDomainSocket::isClosed() {
    return !_pimpl->isOpen();
}

UnixEndpoint DatagramDomainSocket::getLocalEndpoint() const {
    return _pimpl->getLocalEndpoint();
}

UnixEndpoint DatagramDomainSocket::getRemoteEndpoint() const {
    return _pimpl->getRemoteEndpoint();
}

void DatagramDomainSocket::shutdown() {
    _pimpl->shutdown();
}

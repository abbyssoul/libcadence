/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * @file: async/UdpSocket.cpp
 *******************************************************************************/
#include <cadence/async/udpsocket.hpp>

#include "asio_helper.hpp"


using namespace Solace;
using namespace cadence::async;


class UdpSocket::UdpImpl {
public:

    UdpImpl(void* ioservice) :
        _socket(asAsioService(ioservice)),
        _resolver(asAsioService(ioservice))
    {}

    UdpImpl(void* ioservice, uint16 port) :
        _socket(asAsioService(ioservice), asio::ip::udp::endpoint(asio::ip::udp::v4(), port)),
        _resolver(asAsioService(ioservice))
    {}


    Future<void>
    asyncRead(ByteBuffer& dest, size_type bytesToRead) {
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
    asyncReadFrom(ByteBuffer& dest, std::size_t bytesToRead, const IPEndpoint& addr) {
        Promise<void> promise;
        auto f = promise.getFuture();

        asio::error_code ec;
        auto senderEndpoint = toAsioEndpoint(addr, ec);
        if (ec) {
            promise.setError(fromAsioError(ec));
            return f;
        }

        // NOTE: We can pass sender endpoint to future here
        _socket.async_receive_from(asio_buffer(dest, bytesToRead), senderEndpoint,
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
    asyncWriteTo(ByteBuffer& src, size_type bytesToWrite, const IPEndpoint& addr) {
        Promise<void> promise;
        auto f = promise.getFuture();

        asio::error_code ec;
        const auto asioEndpoint = toAsioEndpoint(addr, ec);
        if (ec) {
            promise.setError(fromAsioError(ec));
            return f;
        }

        _socket.async_send_to(asio_buffer(src, bytesToWrite), asioEndpoint,
            [pm = std::move(promise), &src](asio::error_code error, std::size_t length) mutable {
            if (error) {
                pm.setError(fromAsioError(error));
            } else {
                src.advance(length);
                pm.setValue();
            }
        });

        return f;
    }


    Result<void, Error> read(ByteBuffer& dest, size_type bytesToRead) {
        asio::error_code ec;

        const auto len = _socket.receive(asio_buffer(dest, bytesToRead), 0, ec);
        if (ec) {
            return Err(fromAsioError(ec));
        } else {
            dest.advance(len);
        }

        return Ok();
    }

    Result<void, Error> write(ByteBuffer& src, size_type bytesToWrite) {
        asio::error_code ec;

        const auto len = _socket.send(asio_buffer(src, bytesToWrite), 0, ec);
        if (ec) {
            return Err(fromAsioError(ec));
        } else {
            src.advance(len);
        }

        return Ok();
    }

    void cancel() {
        _socket.cancel();
    }

    void close() {
        _socket.close();
    }

    Result<void, Error>
    connect(const IPEndpoint& addr) {
        asio::error_code ec;
        const auto asioEndpoint = toAsioEndpoint(addr, ec);
        if (ec) {
            return Err(fromAsioError(ec));
        }

        _socket.connect(asioEndpoint, ec);
        if (ec) {
            return Err(fromAsioError(ec));
        }

        return Ok();
    }

    bool isOpen() {
        return _socket.is_open();
    }

    IPEndpoint getLocalEndpoint() const {
        return fromAsioEndpoint(_socket.local_endpoint());
    }

    IPEndpoint getRemoteEndpoint() const {
        return fromAsioEndpoint(_socket.remote_endpoint());
    }

    void shutdown() {
        _socket.shutdown(asio::local::datagram_protocol::socket::shutdown_both);
    }


private:
    asio::ip::udp::socket    _socket;
    asio::ip::udp::resolver  _resolver;
};


UdpSocket::~UdpSocket()
{

}


UdpSocket::UdpSocket(EventLoop& ioContext, uint16 port) :
    Channel(ioContext),
    _pimpl(std::make_unique<UdpImpl>(ioContext.getIOService(), port))
{
}

UdpSocket::UdpSocket(EventLoop& ioContext) :
    Channel(ioContext),
    _pimpl(std::make_unique<UdpImpl>(ioContext.getIOService()))
{
}


UdpSocket::UdpSocket(UdpSocket&& rhs) :
    Channel(std::move(rhs)),
    _pimpl(std::move(rhs._pimpl))
{
}

UdpSocket& UdpSocket::swap(UdpSocket& rhs) noexcept {
    using std::swap;
    swap(_pimpl, rhs._pimpl);

    return *this;
}


Future<void>
UdpSocket::asyncRead(ByteBuffer& dest, size_type bytesToRead) {
    return _pimpl->asyncRead(dest, bytesToRead);
}


Future<void>
UdpSocket::asyncReadFrom(ByteBuffer& dest, size_type bytesToRead, const IPEndpoint& remote) {
    return _pimpl->asyncReadFrom(dest, bytesToRead, remote);
}

Future<void>
UdpSocket::asyncWrite(ByteBuffer& src, size_type bytesToWrite)  {
    return _pimpl->asyncWrite(src, bytesToWrite);
}


Future<void>
UdpSocket::asyncWriteTo(ByteBuffer& src, size_type bytesToWrite, const IPEndpoint& remote) {
    return _pimpl->asyncWriteTo(src, bytesToWrite, remote);
}

void UdpSocket::cancel() {
    _pimpl->cancel();
}

void UdpSocket::close() {
    _pimpl->close();
}

Result<void, Error>
UdpSocket::connect(const IPEndpoint& endpoint) {
    return _pimpl->connect(endpoint);
}

Result<void, Error> UdpSocket::read(ByteBuffer& dest, size_type bytesToRead) {
    return _pimpl->read(dest, bytesToRead);
}

Result<void, Error> UdpSocket::write(ByteBuffer& src, size_type bytesToWrite) {
    return _pimpl->write(src, bytesToWrite);
}

bool UdpSocket::isOpen() {
    return _pimpl->isOpen();
}

bool UdpSocket::isClosed() {
    return !_pimpl->isOpen();
}

cadence::IPEndpoint UdpSocket::getLocalEndpoint() const {
    return _pimpl->getLocalEndpoint();
}

cadence::IPEndpoint UdpSocket::getRemoteEndpoint() const {
    return _pimpl->getRemoteEndpoint();
}

void UdpSocket::shutdown() {
    _pimpl->shutdown();
}

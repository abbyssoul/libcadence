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
#include "cadence/async/udpsocket.hpp"

#include "asio_helper.hpp"
#include "asio_helper_tcp.hpp"


using namespace Solace;
using namespace cadence;
using namespace cadence::async;


class UdpSocket::UdpImpl {
public:
    using size_type = Channel::size_type;
    using Socket_type = asio::ip::udp::socket;


    UdpImpl(asio::io_context& ioservice)
        : _socket(ioservice)
    {
        _socket.open(asio::ip::udp::v4());
    }

    UdpImpl(asio::io_context& ioservice, uint16 port)
        : _socket(ioservice, asio::ip::udp::endpoint(asio::ip::udp::v4(), port))
    {}


    Future<void>
    asyncRead(ByteWriter& dest, size_type bytesToRead) {
        Promise<void> promise;
        auto f = promise.getFuture();

        _socket.async_receive(asio_buffer(dest, bytesToRead),
            [pm = std::move(promise), &dest](const asio::error_code& error, std::size_t length) mutable {
            if (error) {
                pm.setError(fromAsioError(error, "asyncRead"));
            } else {
                dest.advance(length);
                pm.setValue();
            }
        });

        return f;
    }

    struct ReadFromHandler {
        Promise<IPEndpoint> pm;
        std::reference_wrapper<ByteWriter> destBuffer;
        std::reference_wrapper<asio::ip::udp::endpoint> peerEndpoint;

        void operator() (const asio::error_code& ec, std::size_t length) {
            if (ec) {
                pm.setError(fromAsioError(ec, "asyncReadFrom"));
            } else {
                destBuffer.get().advance(length);
                pm.setValue(fromAsioEndpoint(peerEndpoint));
            }
        }
    };

    asio::ip::udp::endpoint pe;  // FIXME(abbyssoul): This is a gross hack but there seems to be no way around it.
    Future<IPEndpoint>
    asyncReadFrom(ByteWriter& dest, std::size_t bytesToRead) {
        ReadFromHandler handler{{}, dest, std::ref(pe)};
        auto f = handler.pm.getFuture();

        _socket.async_receive_from(asio_buffer(dest, bytesToRead), pe, std::move(handler));

        return f;

//        auto f = promise.getFuture();

//        auto const asioAddr = toAsioIPEndpoint(addr);
////        auto destEndpoint = Socket_type::endpoint_type{asioAddr.address(), asioAddr.port()};

//        // NOTE: We can pass sender endpoint to future here
//        _socket.async_receive_from(asio_buffer(dest, bytesToRead), destEndpoint,
//            [pm = std::move(promise), &dest](const asio::error_code& ec, std::size_t length) mutable {
//            if (ec) {
//                pm.setError(fromAsioError(ec));
//            } else {
//                dest.advance(length);
//                pm.setValue();
//            }
//        });

//        return f;
    }


    Future<void>
    asyncWrite(ByteReader& src, std::size_t bytesToWrite) {
        Promise<void> promise;
        auto f = promise.getFuture();

        _socket.async_send(asio_buffer(src, bytesToWrite),
            [pm = std::move(promise), &src](asio::error_code const& error, std::size_t bytesTransferred) mutable {
            if (error) {
                pm.setError(fromAsioError(error, "asyncWrite"));
            } else {
                src.advance(bytesTransferred);
                pm.setValue();
            }
        });

        return f;
    }

    Future<void>
    asyncWriteTo(IPEndpoint const& addr, ByteReader& src, size_type bytesToWrite) {
        Promise<void> promise;
        auto f = promise.getFuture();

        auto const destEndpoint = toAsioUDPEndpoint(addr);

        asio::error_code e;
        if (!_socket.is_open()) {
            _socket.open(destEndpoint.protocol(), e);
            if (!e) {
                promise.setError(fromAsioError(e, "asyncWriteTo:open"));
                return f;
            }
        }

        _socket.async_send_to(asio_buffer(src, bytesToWrite), destEndpoint,
            [pm = std::move(promise), &src](asio::error_code const& ec, std::size_t length) mutable {
            if (ec) {
                pm.setError(fromAsioError(ec, "asyncWriteTo:async_send_to"));
            } else {
                src.advance(length);
                pm.setValue();
            }
        });

        return f;
    }


    Result<void, Error> read(ByteWriter& dest, size_type bytesToRead) {
        asio::error_code ec;

        const auto len = _socket.receive(asio_buffer(dest, bytesToRead), 0, ec);
        if (ec) {
            return Err(fromAsioError(ec, "read"));
        }

        return dest.advance(len);
    }

    Result<void, Error> write(ByteReader& src, size_type bytesToWrite) {
        asio::error_code ec;

        const auto len = _socket.send(asio_buffer(src, bytesToWrite), 0, ec);
        if (ec) {
            return Err(fromAsioError(ec, "write"));
        }

        return src.advance(len);
    }

    void cancel() {
        _socket.cancel();
    }

    void close() {
        _socket.close();
    }

    Result<void, Error>
    connect(IPEndpoint const& addr) {
        auto const destEndpoint = toAsioUDPEndpoint(addr);

        asio::error_code ec;
        _socket.connect(destEndpoint, ec);
        if (ec) {
            return Err(fromAsioError(ec, "connect"));
        }

        return Ok();
    }

    bool isOpen() const {
        return _socket.is_open();
    }


    Result<void, Error>
    open() {
        asio::error_code ec;

        _socket.open(asio::ip::udp::v4(), ec);
        if (ec) {
            return Err(fromAsioError(ec, "open"));
        }

        return Ok();
    }

    IPEndpoint getLocalEndpoint() const {
        return fromAsioEndpoint(_socket.local_endpoint());
    }

    IPEndpoint getRemoteEndpoint() const {
        return fromAsioEndpoint(_socket.remote_endpoint());
    }

    void shutdown() {
        _socket.shutdown(Socket_type::shutdown_both);
    }


private:
    Socket_type    _socket;
};


UdpSocket::~UdpSocket() = default;


UdpSocket::UdpSocket(EventLoop& ioContext, uint16 port)
    : Channel(ioContext)
    , _pimpl(std::make_unique<UdpImpl>(asAsioService(ioContext.getIOService()), port))
{
}

UdpSocket::UdpSocket(EventLoop& ioContext)
    : Channel(ioContext)
    , _pimpl(std::make_unique<UdpImpl>(asAsioService(ioContext.getIOService())))
{
}


UdpSocket::UdpSocket(UdpSocket&& rhs) noexcept
    : Channel(std::move(rhs))
    , _pimpl(std::move(rhs._pimpl))
{
}

UdpSocket& UdpSocket::swap(UdpSocket& rhs) noexcept {
    using std::swap;
    swap(_pimpl, rhs._pimpl);

    return *this;
}


Future<void>
UdpSocket::asyncRead(ByteWriter& dest, size_type bytesToRead) {
    return _pimpl->asyncRead(dest, bytesToRead);
}


Future<IPEndpoint>
UdpSocket::asyncReadFrom(ByteWriter& dest, size_type bytesToRead) {
    return _pimpl->asyncReadFrom(dest, bytesToRead);
}

Future<void>
UdpSocket::asyncWrite(ByteReader& src, size_type bytesToWrite)  {
    return _pimpl->asyncWrite(src, bytesToWrite);
}


Future<void>
UdpSocket::asyncWriteTo(const IPEndpoint& remote, ByteReader& src, size_type bytesToWrite) {
    return _pimpl->asyncWriteTo(remote, src, bytesToWrite);
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

Result<void, Error> UdpSocket::read(ByteWriter& dest, size_type bytesToRead) {
    return _pimpl->read(dest, bytesToRead);
}

Result<void, Error> UdpSocket::write(ByteReader& src, size_type bytesToWrite) {
    return _pimpl->write(src, bytesToWrite);
}

bool UdpSocket::isOpen() const {
    return _pimpl->isOpen();
}

bool UdpSocket::isClosed() const {
    return !_pimpl->isOpen();
}


IPEndpoint UdpSocket::getLocalEndpoint() const {
    return _pimpl->getLocalEndpoint();
}


IPEndpoint UdpSocket::getRemoteEndpoint() const {
    return _pimpl->getRemoteEndpoint();
}

void UdpSocket::shutdown() {
    _pimpl->shutdown();
}

Result<void, Error>
UdpSocket::open() {
    return _pimpl->open();
}

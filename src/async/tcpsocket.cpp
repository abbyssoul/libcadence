/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * @file: async/TcpSocket.cpp
 *******************************************************************************/
#include <cadence/async/tcpsocket.hpp>


#include "asio_helper.hpp"


using namespace Solace;
using namespace cadence;
using namespace cadence::async;


class TcpSocket::TcpSocketImpl {
public:

    TcpSocketImpl(void* ioservice) :
        _socket(asAsioService(ioservice))
    {}

    TcpSocketImpl(TcpSocketImpl&& other) :
        _socket(std::move(other._socket))
    {}

    Future<void>
    asyncRead(ByteBuffer& dest, std::size_t bytesToRead) {
        Promise<void> promise;
        auto f = promise.getFuture();

        asio::async_read(_socket, asio_buffer(dest, bytesToRead),
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

        asio::async_write(_socket, asio_buffer(src, bytesToWrite),
            [pm = std::move(promise), &src](const asio::error_code& error, std::size_t length) mutable {
            if (error) {
                pm.setError(fromAsioError(error));
            } else {
                src.advance(length);
                pm.setValue();
            }
        });

        return f;
    }

    Future<void>
    asyncConnect(const IPEndpoint& endpoint) {
        Promise<void> promise;
        auto f = promise.getFuture();

        asio::error_code ec;
        const auto asioEndpoint = toAsioTCPEndpoint(endpoint, ec);
        if (ec) {
            promise.setError(fromAsioError(ec));
            return f;
        }

        _socket.async_connect(asioEndpoint, [pm = std::move(promise)] (const asio::error_code& error) mutable {
                if (error) {
                    pm.setError(fromAsioError(error));
                } else {
                    pm.setValue();
                }
            });

        return f;
    }

    Result<void, Error>
    connect(const IPEndpoint& endpoint) {

        asio::error_code ec;
        const auto asioEndpoint = toAsioTCPEndpoint(endpoint, ec);
        if (ec) {
            return Err(fromAsioError(ec));
        }

        _socket.connect(asioEndpoint, ec);
        if (ec) {
            return Err(fromAsioError(ec));
        }

        return Ok();
    }

    Result<void, Error> read(ByteBuffer& dest, size_type bytesToRead) {
        asio::error_code ec;

        const auto len = asio::read(_socket, asio_buffer(dest, bytesToRead), ec);
        if (ec) {
            return Err(fromAsioError(ec));
        } else {
            dest.advance(len);
        }

        return Ok();
    }

    Result<void, Error> write(ByteBuffer& src, size_type bytesToWrite) {
        asio::error_code ec;

        const auto len = asio::write(_socket, asio_buffer(src, bytesToWrite), ec);
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

    bool isOpen() {
        return _socket.is_open();
    }

    bool isClosed() {
        return !_socket.is_open();
    }

    IPEndpoint getLocalEndpoint() const {
        return fromAsioEndpoint(_socket.local_endpoint());
    }

    IPEndpoint getRemoteEndpoint() const {
        return fromAsioEndpoint(_socket.remote_endpoint());
    }

    void shutdown() {
        _socket.shutdown(asio::ip::tcp::socket::shutdown_both);
    }

    auto& getSocket() { return _socket; }

private:

    asio::ip::tcp::socket   _socket;

};




class TcpAcceptor::AcceptorImpl {
public:

    AcceptorImpl(void* ioservice) :
        _acceptor(asAsioService(ioservice))
    {
    }

    AcceptorImpl(void* ioservice, const IPEndpoint& endpoint) :
        _acceptor(asAsioService(ioservice), toAsioTCPEndpoint(endpoint))
    {
    }

    AcceptorImpl(void* ioservice, uint16 port) :
        _acceptor(asAsioService(ioservice), asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
    {
    }


    Future<void> asyncAccept(TcpSocket::TcpSocketImpl* socket) {
        Promise<void> promise;
        auto f = promise.getFuture();

        _acceptor.async_accept(socket->getSocket(), [pm = std::move(promise)](const asio::error_code& error) mutable {
            if (error) {
                pm.setError(fromAsioError(error));
            } else {
                pm.setValue();
            }
        });

        return f;
    }

    Result<void, Error>
    accept(TcpSocket::TcpSocketImpl* socket) {
        asio::error_code ec;
        _acceptor.accept(socket->getSocket(), ec);

        if (ec) {
            return Err(fromAsioError(ec));
        }

        return Ok();
    }

    Result<void, Error> open(const IPEndpoint& endpoint, int32 backlog, bool reuseAddr = true) {
        asio::error_code ec;

        auto e = toAsioTCPEndpoint(endpoint, ec);
        if (ec) {
            return Err(fromAsioError(ec));
        }

        if (!_acceptor.is_open()) {
            _acceptor.open(e.protocol(), ec);

            if (ec) {
                return Err(fromAsioError(ec));
            }
        }


        if (reuseAddr) {
          _acceptor.set_option(asio::socket_base::reuse_address(true), ec);

          if (ec) {
              return Err(fromAsioError(ec));
          }
        }

        _acceptor.bind(e, ec);
        if (ec) {
            return Err(fromAsioError(ec));
        }

        _acceptor.listen(backlog, ec);
        if (ec) {
            return Err(fromAsioError(ec));
        }

        return Ok();
    }

    bool nonBlocking() {
        return _acceptor.non_blocking();
    }

    bool nativeNonBlocking() {
        return _acceptor.native_non_blocking();
    }

    void nativeNonBlocking(bool mode) {
        _acceptor.native_non_blocking(mode);
    }

    void cancel() {
        _acceptor.cancel();
    }

    void close() {
        _acceptor.close();
    }

    bool isOpen() {
        return _acceptor.is_open();
    }

    bool isClosed() {
        return !_acceptor.is_open();
    }

    IPEndpoint getLocalEndpoint() const {
        return fromAsioEndpoint(_acceptor.local_endpoint());
    }


private:
    asio::ip::tcp::acceptor _acceptor;
};


TcpAcceptor::~TcpAcceptor()
{
}

TcpAcceptor::TcpAcceptor(EventLoop& ioContext, uint16 port) :
    _pimpl(std::make_unique<AcceptorImpl>(ioContext.getIOService(), port))
{
}

TcpAcceptor::TcpAcceptor(EventLoop& ioContext) :
    _pimpl(std::make_unique<AcceptorImpl>(ioContext.getIOService()))
{
}

TcpAcceptor::TcpAcceptor(EventLoop& ioContext, const IPEndpoint& endpoint)  :
    _pimpl(std::make_unique<AcceptorImpl>(ioContext.getIOService(), endpoint))
{
}


TcpAcceptor::TcpAcceptor(TcpAcceptor&& rhs) :
  _pimpl(std::move(rhs._pimpl))
{
}

TcpAcceptor& TcpAcceptor::swap(TcpAcceptor& rhs) noexcept {
    using std::swap;
    swap(_pimpl, rhs._pimpl);

    return *this;
}

Result<void, Error>
TcpAcceptor::accept(TcpSocket& socket) {
    return _pimpl->accept(socket._pimpl.get());
}

bool TcpAcceptor::nonBlocking() {
    return _pimpl->nonBlocking();
}

bool TcpAcceptor::nativeNonBlocking() {
    return _pimpl->nativeNonBlocking();
}

void TcpAcceptor::nativeNonBlocking(bool mode) {
    _pimpl->nativeNonBlocking(mode);
}


Result<void, Error>
TcpAcceptor::open(const IPEndpoint& endpoint, int32 backlog, bool reuseAddr) {
    return _pimpl->open(endpoint, backlog, reuseAddr);
}

void TcpAcceptor::cancel() {
    _pimpl->cancel();
}

void TcpAcceptor::close() {
    _pimpl->close();
}

bool TcpAcceptor::isOpen() {
    return _pimpl->isOpen();
}

bool TcpAcceptor::isClosed() {
    return _pimpl->isClosed();
}

Future<void>
TcpAcceptor::asyncAccept(TcpSocket& socket) {
    return _pimpl->asyncAccept(socket._pimpl.get());
}

IPEndpoint TcpAcceptor::getLocalEndpoint() const {
    return _pimpl->getLocalEndpoint();
}




TcpSocket::~TcpSocket()
{
}


TcpSocket::TcpSocket(EventLoop& ioContext) :
    StreamSocket(ioContext),
    _pimpl(std::make_unique<TcpSocketImpl>(ioContext.getIOService()))
{
}


TcpSocket::TcpSocket(TcpSocket&& rhs) :
    StreamSocket(std::move(rhs)),
    _pimpl(std::make_unique<TcpSocketImpl>(std::move(*rhs._pimpl.get())))
{
}

TcpSocket& TcpSocket::swap(TcpSocket& rhs) noexcept {
    using std::swap;
    swap(_pimpl, rhs._pimpl);

    return *this;
}


Future<void>
TcpSocket::asyncRead(ByteBuffer& dest, size_type bytesToRead) {
    return _pimpl->asyncRead(dest, bytesToRead);
}

Future<void>
TcpSocket::asyncWrite(ByteBuffer& src, size_type bytesToWrite) {
    return _pimpl->asyncWrite(src, bytesToWrite);
}

Future<void>
TcpSocket::asyncConnect(const NetworkEndpoint& endpoint) {
    return _pimpl->asyncConnect(*static_cast<const IPEndpoint*>(&endpoint));
}


void TcpSocket::cancel() {
    _pimpl->cancel();
}

void TcpSocket::close() {
    _pimpl->close();
}


Result<void, Error>
TcpSocket::connect(const NetworkEndpoint& endpoint) {
    // FIXME: Fucked-up upcast without type-check!
    return _pimpl->connect(*static_cast<const IPEndpoint*>(&endpoint));
}

Result<void, Error>
TcpSocket::read(ByteBuffer& dest, size_type bytesToRead) {
    return _pimpl->read(dest, bytesToRead);
}

Result<void, Error>
TcpSocket::write(ByteBuffer& src, size_type bytesToWrite) {
    return _pimpl->write(src, bytesToWrite);
}


bool TcpSocket::isOpen() {
    return _pimpl->isOpen();
}


bool TcpSocket::isClosed() {
    return _pimpl->isClosed();
}

IPEndpoint TcpSocket::getLocalEndpoint() const {
    return _pimpl->getLocalEndpoint();
}

IPEndpoint TcpSocket::getRemoteEndpoint() const {
    return _pimpl->getRemoteEndpoint();
}

void TcpSocket::shutdown() {
    _pimpl->shutdown();
}

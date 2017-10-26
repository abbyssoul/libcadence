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
        _socket(*static_cast<asio::io_service*>(ioservice))
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

        _socket.async_connect(toAsioTCPEndpoint(endpoint),
        [pm = std::move(promise)] (const asio::error_code& error) mutable {
            if (error) {
                pm.setError(fromAsioError(error));
            } else {
                pm.setValue();
            }
        });

        return f;
    }

    Result<void, Error> connect(const IPEndpoint& endpoint) {
        asio::error_code ec;

        _socket.connect(toAsioTCPEndpoint(endpoint), ec);
        if (ec) {
            return Err(fromAsioError(ec));
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
        _acceptor(*static_cast<asio::io_service*>(ioservice))
    {
    }

    // FIXME: We are listening on all interfaces over ipv4 here.
    AcceptorImpl(void* ioservice, uint16 port) :
        _acceptor(*static_cast<asio::io_service*>(ioservice), asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
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

        auto e = toAsioTCPEndpoint(endpoint);

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

Result<void, Error> TcpSocket::connect(const NetworkEndpoint& endpoint) {
    return _pimpl->connect(*static_cast<const IPEndpoint*>(&endpoint));
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

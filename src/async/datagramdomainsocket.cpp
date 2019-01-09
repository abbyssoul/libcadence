/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * @file: async/datagramdomainsocket.cpp
*******************************************************************************/
#include "cadence/async/datagramdomainsocket.hpp"


#include "asio_helper.hpp"
#include "asio_helper_local.hpp"

#include <asio/local/datagram_protocol.hpp>


using namespace Solace;
using namespace cadence;
using namespace cadence::async;



asio::local::datagram_protocol::endpoint
toAsioLocalDatagramEndpoint(UnixEndpoint const& addr) {  // TODO(abbyssoul): add version with, asio::error_code& ec) {
    sockaddr_un name;
    name.sun_family = AF_LOCAL;

    // FIXME(abbyssoul): Maybe return an error if the name is too long!
    auto const addrNameView = addr.toString().view();
    const auto totalSize = std::min<size_t>(addrNameView.size(), sizeof(name.sun_path));
    strncpy(name.sun_path, addrNameView.data(), totalSize);
    name.sun_path[totalSize - 1] = '\0';

    return asio::local::datagram_protocol::endpoint(name.sun_path);
}


class DatagramDomainSocket::SocketImpl {
public:

    SocketImpl(void* ioservice, UnixEndpoint const& endpoing)
        : _socket(asAsioService(ioservice), toAsioLocalDatagramEndpoint(endpoing))
    {}

    SocketImpl(void* ioservice) :
        _socket(asAsioService(ioservice))
    {}


    Future<void> asyncConnect(UnixEndpoint const& peer) {
        Promise<void> promise;
        auto f = promise.getFuture();

        auto const endpoint = toAsioLocalDatagramEndpoint(peer);
        _socket.async_connect(endpoint,
            [pm = std::move(promise)](const asio::error_code& error) mutable {
            if (error) {
                pm.setError(fromAsioError(error, "asyncConnect"));
            } else {
                pm.setValue();
            }
        });

        return f;
    }

    Future<void>
    asyncRead(ByteWriter& dest, std::size_t bytesToRead) {
        Promise<void> promise;
        auto f = promise.getFuture();

        _socket.async_receive(asio_buffer(dest, bytesToRead),
            [pm = std::move(promise), &dest](const asio::error_code& error, std::size_t length) mutable {
            if (error) {
                pm.setError(fromAsioError(error, "asynRead"));
            } else {
                dest.advance(length);
                pm.setValue();
            }
        });

        return f;
    }


    Future<void>
    asyncReadFrom(ByteWriter& dest, std::size_t bytesToRead, UnixEndpoint const& endpoint) {
        Promise<void> promise;
        auto f = promise.getFuture();

        auto destination = toAsioLocalDatagramEndpoint(endpoint);
        _socket.async_receive_from(asio_buffer(dest, bytesToRead), destination,
            [pm = std::move(promise), &dest](const asio::error_code& error, std::size_t length) mutable {
            if (error) {
                pm.setError(fromAsioError(error, "asyncReadFrom"));
            } else {
                dest.advance(length);
                pm.setValue();
            }
        });

        return f;
    }


    Future<void>
    asyncWrite(ByteReader& src, std::size_t bytesToWrite) {
        Promise<void> promise;
        auto f = promise.getFuture();

        _socket.async_send(asio_buffer(src, bytesToWrite),
            [pm = std::move(promise), &src](const asio::error_code& error, std::size_t bytesTransferred) mutable {
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
    asyncWriteTo(ByteReader& src, std::size_t bytesToWrite, UnixEndpoint const& endpoint) {
        Promise<void> promise;
        auto f = promise.getFuture();

        auto const destination = toAsioLocalDatagramEndpoint(endpoint);
        _socket.async_send_to(asio_buffer(src, bytesToWrite), destination,
            [pm = std::move(promise), &src](const asio::error_code& error, std::size_t bytesTransferred) mutable {
            if (error) {
                pm.setError(fromAsioError(error, "asyncWriteTo"));
            } else {
                src.advance(bytesTransferred);
                pm.setValue();
            }
        });

        return f;
    }


    Result<void, Error>
    connect(UnixEndpoint const& endpoint) {
        auto const destination = toAsioLocalDatagramEndpoint(endpoint);
        asio::error_code ec;

        _socket.connect(destination, ec);
        if (ec) {
            return Err(fromAsioError(ec, "connect"));
        }

        return Ok();
    }

    Result<void, Error> read(ByteWriter& dest, size_type bytesToRead) {
        asio::error_code ec;

        auto const len = _socket.receive(asio_buffer(dest, bytesToRead), 0, ec);
        if (ec) {
            return Err(fromAsioError(ec, "read"));
        }

        return dest.advance(len);
    }

    Result<void, Error> write(ByteReader& src, size_type bytesToWrite) {
        asio::error_code ec;

        auto const len = _socket.send(asio_buffer(src, bytesToWrite), 0, ec);
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


    bool isOpen() const {
        return _socket.is_open();
    }

    UnixEndpoint getLocalEndpoint() const {
        // TODO(abbyssoul): may throw and thus must use ec accepting version and return result<>
        auto localEndpoint = _socket.local_endpoint();
        auto data = localEndpoint.data();

        return {makeString(data->sa_data, localEndpoint.size())};
    }

    UnixEndpoint getRemoteEndpoint() const {
        // TODO(abbyssoul): may throw and thus must use ec accepting version and return result<>
        auto localEndpoint = _socket.remote_endpoint();
        auto data = localEndpoint.data();

        return {makeString(data->sa_data, localEndpoint.size())};
    }

    void shutdown() {
        _socket.shutdown(asio::local::datagram_protocol::socket::shutdown_both);
    }


private:
    asio::local::datagram_protocol::socket      _socket;
};



DatagramDomainSocket::~DatagramDomainSocket() = default;


DatagramDomainSocket::DatagramDomainSocket(EventLoop& ioContext) :
    Channel(ioContext),
    _pimpl(std::make_unique<SocketImpl>(ioContext.getIOService()))
{
}


DatagramDomainSocket::DatagramDomainSocket(EventLoop& ioContext, UnixEndpoint const& endpoint) :
    Channel(ioContext),
    _pimpl(std::make_unique<SocketImpl>(ioContext.getIOService(), endpoint))
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



Future<void> DatagramDomainSocket::asyncConnect(UnixEndpoint const& endpoint) {
    return _pimpl->asyncConnect(endpoint);
}

Future<void>
DatagramDomainSocket::asyncRead(ByteWriter& dest, size_type bytesToRead) {
    return _pimpl->asyncRead(dest, bytesToRead);
}

Future<void>
DatagramDomainSocket::asyncReadFrom(ByteWriter& dest, std::size_t bytesToRead, UnixEndpoint const& endpoint) {
    return _pimpl->asyncReadFrom(dest, bytesToRead, endpoint);
}

Future<void>
DatagramDomainSocket::asyncWrite(ByteReader& src, size_type bytesToWrite)  {
    return _pimpl->asyncWrite(src, bytesToWrite);
}

Future<void>
DatagramDomainSocket::asyncWriteTo(ByteReader& src, std::size_t bytesToWrite, UnixEndpoint const& endpoint) {
    return _pimpl->asyncWriteTo(src, bytesToWrite, endpoint);
}

void DatagramDomainSocket::cancel() {
    _pimpl->cancel();
}

void DatagramDomainSocket::close() {
    _pimpl->close();
}

void DatagramDomainSocket::connect(UnixEndpoint const& endpoint) {
    _pimpl->connect(endpoint);
}

Result<void, Error> DatagramDomainSocket::read(ByteWriter& dest, size_type bytesToRead) {
    return _pimpl->read(dest, bytesToRead);
}

Result<void, Error> DatagramDomainSocket::write(ByteReader& src, size_type bytesToWrite) {
    return _pimpl->write(src, bytesToWrite);
}

bool DatagramDomainSocket::isOpen() const {
    return _pimpl->isOpen();
}

bool DatagramDomainSocket::isClosed() const {
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

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
#include "cadence/async/streamsocket.hpp"

#include "streamsocket_impl.hpp"
#include "asio_helper.hpp"
#include "asio_helper_tcp.hpp"

#include <asio/read.hpp>
#include <asio/write.hpp>


using namespace Solace;
using namespace cadence;
using namespace cadence::async;


namespace {
// Anounimous namespace

class TcpSocketImpl :
        public StreamSocket::StreamSocketImpl {
public:

    using size_type = StreamSocket::size_type;
    using Socket_type = asio::ip::tcp::socket;


    TcpSocketImpl(void* ioservice) :
        _socket(asAsioService(ioservice))
    {}

    TcpSocketImpl(Socket_type&& other) :
        _socket(std::move(other))
    {}

    TcpSocketImpl(TcpSocketImpl&& other) :
        _socket(std::move(other._socket))
    {}

    Future<void>
    asyncRead(ByteWriter& dest, size_type bytesToRead) override {
        Promise<void> promise;
        auto f = promise.getFuture();

        asio::async_read(_socket, asio_buffer(dest, bytesToRead),
            [pm = std::move(promise), &dest](asio::error_code const& error, std::size_t length) mutable {
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
    asyncWrite(ByteReader& src, size_type bytesToWrite) override {
        Promise<void> promise;
        auto f = promise.getFuture();

        asio::async_write(_socket, asio_buffer(src, bytesToWrite),
            [pm = std::move(promise), &src](asio::error_code const& error, std::size_t length) mutable {
            if (error) {
                pm.setError(fromAsioError(error));
            } else {
                src.advance(length);
                pm.setValue();
            }
        });

        return f;
    }

    Result<void, Error>
    read(ByteWriter& dest, size_type bytesToRead) override {
        asio::error_code ec;

        auto const len = asio::read(_socket, asio_buffer(dest, bytesToRead), ec);
        if (ec) {
            return Err(fromAsioError(ec));
        } else {
            dest.advance(len);
        }

        return Ok();
    }

    Result<void, Error>
    write(ByteReader& src, size_type bytesToWrite) override {
        asio::error_code ec;

        auto const len = asio::write(_socket, asio_buffer(src, bytesToWrite), ec);
        if (ec) {
            return Err(fromAsioError(ec));
        } else {
            src.advance(len);
        }

        return Ok();
    }


    void cancel() override {
        _socket.cancel();
    }

    void close() override {
        _socket.close();
    }

    bool isOpen() override {
        return _socket.is_open();
    }

    bool isClosed() override {
        return !_socket.is_open();
    }

    NetworkEndpoint getLocalEndpoint() const override {
        return fromAsioEndpoint(_socket.local_endpoint());
    }

    NetworkEndpoint getRemoteEndpoint() const override {
        return fromAsioEndpoint(_socket.remote_endpoint());
    }

    void shutdown() override {
        _socket.shutdown(Socket_type::shutdown_both);
    }

    Future<void>
    asyncConnect(NetworkEndpoint const& endpoint) override {
        Promise<void> promise;
        auto f = promise.getFuture();

        asio::error_code ec;
        auto const asioEndpoint = toAsioIPEndpoint(endpoint, ec);
        if (ec) {
            promise.setError(fromAsioError(ec));
            return f;
        }

        _socket.async_connect(asioEndpoint, [pm = std::move(promise)] (asio::error_code const& error) mutable {
            if (error) {
                pm.setError(fromAsioError(error));
            } else {
                pm.setValue();
            }
        });

        return f;
    }

    Result<void, Error>
    connect(NetworkEndpoint const& endpoint) override {
        asio::error_code ec;

        auto const asioEndpoint = toAsioIPEndpoint(endpoint, ec);
        if (ec) {
            return Err(fromAsioError(ec));
        }

        _socket.connect(asioEndpoint, ec);
        if (ec) {
            return Err(fromAsioError(ec));
        }

        return Ok();
    }


    auto& getSocket() noexcept { return _socket; }

private:

    Socket_type   _socket;

};

}  // namespace


StreamSocket
cadence::async::createTCPSocket(EventLoop& loop) {
    return { loop, std::make_unique<TcpSocketImpl>(loop.getIOService()) };

}


StreamSocket
createTCPSocket(EventLoop& loop, asio::ip::tcp::socket&& socket) {
    return { loop, std::make_unique<TcpSocketImpl>(std::move(socket)) };
}

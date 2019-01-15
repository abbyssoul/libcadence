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
#include "cadence/async/streamsocket.hpp"

#include "streamsocket_impl.hpp"
#include "asio_helper.hpp"
#include "asio_helper_local.hpp"

#include <asio/write.hpp>
#include <asio/read.hpp>


using namespace Solace;
using namespace cadence;
using namespace cadence::async;


asio::local::stream_protocol::endpoint
cadence::toAsioLocalEndpoint(NetworkEndpoint const& addr, asio::error_code& ec) {
    return std::visit([&ec](auto&& arg) -> asio::local::stream_protocol::endpoint {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, UnixEndpoint>) {
            sockaddr_un name;
            name.sun_family = AF_LOCAL;

            // FIXME(abbyssoul): Maybe return an error if the name is too long!
            auto const addrNameView = arg.toString().view();
            const auto totalSize = std::min<size_t>(addrNameView.size(), sizeof(name.sun_path));
            strncpy(name.sun_path, addrNameView.data(), totalSize);
            name.sun_path[totalSize] = '\0';

            return asio::local::stream_protocol::endpoint(name.sun_path);
        } else {
            ec = asio::error::make_error_code(asio::error::basic_errors::address_family_not_supported);
            return asio::local::stream_protocol::endpoint();
        }
    }, addr);
}


NetworkEndpoint
cadence::fromAsioEndpoint(asio::local::stream_protocol::endpoint const& addr) {
    const auto str = addr.path();

    UnixEndpoint point(Solace::makeString(str.data(), str.size()));
    return std::move(point);
}



namespace {
// Anounimous namespace

class StreamDomainSocketImpl :
        public StreamSocket::StreamSocketImpl {
public:

    using size_type = StreamSocket::size_type;
    using Socket_type = asio::local::stream_protocol::socket;


    StreamDomainSocketImpl(asio::io_context& ioservice)
        : _socket(ioservice)
    {}

    StreamDomainSocketImpl(Socket_type&& other)
        : _socket(std::move(other))
    {}

    StreamDomainSocketImpl(StreamDomainSocketImpl&& other)
        : _socket(std::move(other._socket))
    {}


    Future<void>
    asyncRead(ByteWriter& dest, size_type bytesToRead) override {
        Promise<void> promise;
        auto f = promise.getFuture();

        asio::async_read(_socket, asio_buffer(dest, bytesToRead),
            [pm = std::move(promise), &dest] (asio::error_code const& error, std::size_t bytes_transferred) mutable {
                dest.advance(bytes_transferred);
                if (error) {
                    pm.setError(fromAsioError(error, "asyncRead"));
                } else {
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
            [pm = std::move(promise), &src] (asio::error_code const& error, std::size_t bytes_transferred) mutable {
                src.advance(bytes_transferred);
                if (error) {
                    pm.setError(fromAsioError(error, "asyncWrite"));
                } else {
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
            return Err(fromAsioError(ec, "read"));
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
            return Err(fromAsioError(ec, "write"));
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

    bool isOpen() const override {
        return _socket.is_open();
    }

    bool isClosed() const override {
        return !_socket.is_open();
    }

    NetworkEndpoint getLocalEndpoint() const override {
        return fromAsioEndpoint(_socket.local_endpoint());
    }

    NetworkEndpoint getRemoteEndpoint() const override {
        return fromAsioEndpoint(_socket.remote_endpoint());
    }

    void shutdown() override {
        _socket.shutdown(asio::local::stream_protocol::socket::shutdown_both);
    }

    Future<void>
    asyncConnect(NetworkEndpoint const& endpoint) override {
        Promise<void> promise;
        auto f = promise.getFuture();

        asio::error_code ec;
        auto const asioEndpoint = toAsioLocalEndpoint(endpoint, ec);
        if (ec) {
            promise.setError(fromAsioError(ec, "asyncConnect: local-endpoint"));
            return f;
        }

        _socket.async_connect(asioEndpoint, [pm = std::move(promise)] (asio::error_code const& error) mutable {
            if (error) {
                pm.setError(fromAsioError(error, "asyncConnect"));
            } else {
                pm.setValue();
            }
        });

        return f;
    }

    Result<void, Error>
    connect(NetworkEndpoint const& endpoint) override {
        asio::error_code ec;

        auto const asioEndpoint = toAsioLocalEndpoint(endpoint, ec);
        if (ec) {
            return Err(fromAsioError(ec, "connect: to-local-endpoint"));
        }

        _socket.connect(asioEndpoint, ec);
        if (ec) {
            return Err(fromAsioError(ec, "connect"));
        }

        return Ok();
    }


    auto& getSocket() noexcept { return _socket; }

private:

    asio::local::stream_protocol::socket _socket;

};

}  // namespace


StreamSocket
cadence::async::createUnixSocket(EventLoop& loop) {
    return { loop, std::make_unique<StreamDomainSocketImpl>(asAsioService(loop.getIOService())) };
}

StreamSocket
createUnixSocket(EventLoop& loop, asio::local::stream_protocol::socket&& socket) {
    return { loop, std::make_unique<StreamDomainSocketImpl>(std::move(socket)) };
}

/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * @file: async/tcpacceptor.cpp
 *******************************************************************************/
#include "cadence/async/acceptor.hpp"


#include "asio_helper.hpp"
#include "asio_helper_local.hpp"


using namespace Solace;
using namespace cadence;
using namespace cadence::async;

StreamSocket
createUnixSocket(EventLoop& loop, asio::local::stream_protocol::socket&& socket);


namespace {  // anoniomous namespace


class StremDomainAcceptor
        : public Acceptor::AcceptorImpl {
public:

    ~StremDomainAcceptor() override = default;

    StremDomainAcceptor(EventLoop& loop)
        : _loop(&loop)
        , _acceptor(asAsioService(loop.getIOService()))
    {
    }

    Result<void, Error>
    open(NetworkEndpoint const& endpoint) override {
        asio::error_code ec;
        const bool reuseAddr = true;
        const int backlog = 16;

        auto e = toAsioLocalEndpoint(endpoint, ec);
        if (ec) {
            return Err(fromAsioError(ec, "open: to endpoint"));
        }

        if (!_acceptor.is_open()) {
            _acceptor.open(e.protocol(), ec);

            if (ec) {
                return Err(fromAsioError(ec, "open"));
            }
        }


        if (reuseAddr) {
          _acceptor.set_option(asio::socket_base::reuse_address(true), ec);

          if (ec) {
              return Err(fromAsioError(ec, "open:set_option"));
          }
        }

        _acceptor.bind(e, ec);
        if (ec) {
            return Err(fromAsioError(ec, "open:bind"));
        }

        _acceptor.listen(backlog, ec);
        if (ec) {
            return Err(fromAsioError(ec, "open:listen"));
        }

        return Ok();
    }


    bool isOpen() override {
        return _acceptor.is_open();
    }

    void close() override {
        _acceptor.close();
    }

    bool isClosed() override {
        return !_acceptor.is_open();
    }


    Result<StreamSocket, Error>
    accept() override {
        asio::error_code ec;
        auto newSocket = _acceptor.accept(ec);

        if (ec) {
            return Err(fromAsioError(ec, "accept"));
        }

        return Ok(StreamSocket{createUnixSocket(*_loop, std::move(newSocket))});
    }


    Future<StreamSocket> asyncAccept() override {
        Promise<StreamSocket> prom;
        auto futureConnection = prom.getFuture();

        using socket_t =  asio::local::stream_protocol::socket;
        _acceptor.async_accept([pm = std::move(prom), l = _loop](asio::error_code const& ec, socket_t&& peer) mutable {
            if (ec) {
                pm.setError(fromAsioError(ec, "asyncAccept"));
            } else {
                pm.setValue(createUnixSocket(*l, std::move(peer)));
            }
        });

        return futureConnection;
    }


    bool nonBlocking() override {
        return _acceptor.non_blocking();
    }

    void nonBlocking(bool mode) override {
        return _acceptor.non_blocking(mode);
    }

    bool nativeNonBlocking() override {
        return _acceptor.native_non_blocking();
    }

    void nativeNonBlocking(bool mode) override {
        _acceptor.native_non_blocking(mode);
    }

    void cancel() override {
        _acceptor.cancel();
    }


    NetworkEndpoint getLocalEndpoint() const override {
        return fromAsioEndpoint(_acceptor.local_endpoint());
    }


private:
    EventLoop*              _loop;
    asio::local::stream_protocol::acceptor  _acceptor;
};

}  // namespace


std::unique_ptr<Acceptor::AcceptorImpl> createUnixDomainAcceptor(EventLoop& loop) {
    return std::make_unique<StremDomainAcceptor>(loop);
}

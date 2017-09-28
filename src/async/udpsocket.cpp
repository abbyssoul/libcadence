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

#include "asio.hpp"


using namespace Solace;
using namespace cadence::async;


UdpSocket::UdpSocket(EventLoop& eventLoop, uint16 port) :
    _socket(eventLoop.getIOService(), asio::ip::udp::endpoint(asio::ip::udp::v4(), port)),
    _resolver(eventLoop.getIOService())
{
}

UdpSocket::UdpSocket(EventLoop& ioContext) :
    _socket(ioContext.getIOService()), _resolver(ioContext.getIOService())
{
}


UdpSocket::UdpSocket(UdpSocket&& rhs) :
    _socket(std::move(rhs._socket)),
    _resolver(std::move(rhs._resolver))
{
}


Future<void>
UdpSocket::asyncRead(ByteBuffer& dest, std::size_t bytesToRead) {
	Promise<void> promise;
	auto f = promise.getFuture();

	asio::ip::udp::endpoint _sender_endpoint;
    _socket.async_receive_from(asio_buffer(dest, bytesToRead), _sender_endpoint,
        [pm = std::move(promise), &dest](const asio::error_code& error, std::size_t length) mutable {
        if (error) {
            pm.setError(Solace::Error(error.message(), error.value()));
        } else {
            dest.advance(length);
            pm.setValue();
        }
	});

	return f;
}


Future<void>
UdpSocket::asyncWrite(ByteBuffer& src,
                      const endpoint_type& receiver,
                      const String& serviceName,
                      std::size_t bytesToWrite)
{
	Promise<void> promise;
	auto f = promise.getFuture();

	asio::ip::udp::resolver::query query(receiver.to_str(), serviceName.to_str());
	asio::ip::udp::endpoint _receiver_endpoint = *_resolver.resolve(query);

    _socket.async_send_to(asio_buffer(src, bytesToWrite), _receiver_endpoint,
        [pm = std::move(promise), &src](asio::error_code error, std::size_t length) mutable {
        if (error) {
            pm.setError(Solace::Error(error.message(), error.value()));
        } else {
            src.advance(length);
            pm.setValue();
        }
	});

	return f;
}


/*Future<void>
UdpSocket::asyncServer(const endpoint_type& receiver, const String& serviceName) {
	Promise<void> promise;
	auto f = promise.getFuture();

	asio::ip::udp::resolver::query query(receiver.to_str(), serviceName.to_str());

	return f;
}*/

/*UdpsocketAcceptor::UdpsocketAcceptor(EventLoop& ioContext, const std::string& file)
: _acceptor(ioContext.getIOService(), asio::local::stream_protocol::endpoint(file))
{

}


Future<void>
UdpsocketAcceptor::asyncAccept(UdpSocket& socket) {
    Promise<void> promise;
    auto f = promise.getFuture();

    _acceptor.async_accept(socket._socket, [pm=std::move(promise)](const asio::error_code& error) mutable {
        if (!error) {
            pm.setValue();
        }
    });

    return f;
}*/

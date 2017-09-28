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


#include "asio.hpp"


using namespace Solace;
using namespace cadence::async;



TcpAcceptor::TcpAcceptor(EventLoop& ioContext, uint16 port) :
    _acceptor(ioContext.getIOService(), asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
{
}

TcpAcceptor::TcpAcceptor(EventLoop& ioContext) :
    _acceptor(ioContext.getIOService())
{
}

TcpAcceptor::TcpAcceptor(TcpAcceptor&& rhs) :
    _acceptor(std::move(rhs._acceptor))
{
}


void TcpAcceptor::accept(TcpSocket& socket) {
	_acceptor.accept(socket._socket);
}

bool TcpAcceptor::nonBlocking() {
	return _acceptor.non_blocking();
}

bool TcpAcceptor::nativeNonBlocking() {
	return _acceptor.native_non_blocking();
}

void TcpAcceptor::nativeNonBlocking(bool mode) {
	_acceptor.native_non_blocking(mode);
}

void TcpAcceptor::cancel() {
	_acceptor.cancel();
}

void TcpAcceptor::close() {
	_acceptor.close();
}

bool TcpAcceptor::isOpen() {
	return _acceptor.is_open();
}

bool TcpAcceptor::isClosed() {
	return !_acceptor.is_open();
}

Future<void>
TcpAcceptor::asyncAccept(TcpSocket& socket) {
    Promise<void> promise;
    auto f = promise.getFuture();

    _acceptor.async_accept(socket._socket, [pm = std::move(promise)](const asio::error_code& error) mutable {
        if (error) {
            pm.setError(Solace::Error(error.message(), error.value()));
        } else {
            pm.setValue();
    	}
    });

    return f;
}




TcpSocket::TcpSocket(EventLoop& eventLoop, uint16 port) :
    _socket(eventLoop.getIOService()),
    _port(port)
{
}

TcpSocket::TcpSocket(EventLoop& ioContext) :
    _socket(ioContext.getIOService())
{
}


TcpSocket::TcpSocket(TcpSocket&& rhs) :
    _socket(std::move(rhs._socket))
{
}


void TcpSocket::connect(endpoint_type server) {
	asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(server.c_str()), _port);

	_socket.connect(endpoint);
}


Future<void>
TcpSocket::asyncRead(ByteBuffer& dest, std::size_t bytesToRead) {
	Promise<void> promise;
	auto f = promise.getFuture();

    _socket.async_receive(asio_buffer(dest, bytesToRead),
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
TcpSocket::asyncWrite(ByteBuffer& src, std::size_t bytesToWrite) {
	Promise<void> promise;
	auto f = promise.getFuture();

    _socket.async_send(asio_buffer(src, bytesToWrite),
        [pm = std::move(promise), &src](const asio::error_code& error, std::size_t length) mutable {
        if (error) {
            pm.setError(Solace::Error(error.message(), error.value()));
        } else {
            src.advance(length);
            pm.setValue();
        }
	});

	return f;
}

Future<void>
TcpSocket::asyncConnect(const endpoint_type& server) {
 	Promise<void> promise;
    auto f = promise.getFuture();

    asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(server.c_str()), _port);

    _socket.async_connect(endpoint, [pm = std::move(promise)] (const asio::error_code& error) mutable {
        if (error) {
            pm.setError(Solace::Error(error.message(), error.value()));
        } else {
            pm.setValue();
        }
    });

    return f;
}


void TcpSocket::cancel() {
	_socket.cancel();
}


void TcpSocket::close() {
	_socket.close();
}


bool TcpSocket::isOpen() {
	return _socket.is_open();
}


bool TcpSocket::isClosed() {
	return !_socket.is_open();
}

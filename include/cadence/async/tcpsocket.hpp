/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Async TCP socket
 *	@file		cadence/async/TcpSocket.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNC_TCPSOCKET_HPP
#define CADENCE_ASYNC_TCPSOCKET_HPP

#include "cadence/async/channel.hpp"

#include <solace/string.hpp>


namespace cadence { namespace async {

/**
 * An async wrapper for the POSIX TcpSocket
 */
class TcpSocket {
public:
    typedef Solace::String endpoint_type;

public:

    ~TcpSocket() = default;

    TcpSocket(EventLoop& eventLoop, Solace::uint16 port);

    TcpSocket(const TcpSocket& rhs) = delete;

    TcpSocket& operator= (const TcpSocket& rhs) = delete;

    TcpSocket(EventLoop& ioContext);

    TcpSocket(TcpSocket&& rhs);

    TcpSocket& operator= (TcpSocket&& rhs) noexcept {
        return swap(rhs);
    }

    TcpSocket& swap(TcpSocket& rhs) noexcept {
        using std::swap;
        swap(_socket, rhs._socket);

        return *this;
    }

	void connect(endpoint_type server);

    Solace::Future<void> asyncConnect(const endpoint_type& endpoint);

    void cancel();

    void close();

    bool isOpen();

    bool isClosed();

	/**
     * Post an async read request to read data from this IO object into the given buffer.
     * This method reads the data until the provided destination buffer is full.
     *
     * @param dest The provided destination buffer to read data into.
     * @return A future that will be resolved one the buffer has been filled.
     */
    Solace::Future<void> asyncRead(Solace::ByteBuffer& dest) {
        return asyncRead(dest, dest.remaining());
    }

    /**
     * Post an async read request to read specified amount of data from this IO object into the given buffer.
     *
     * @param dest The provided destination buffer to read data into.
     * @param bytesToRead Amount of data (in bytes) to read from this IO object.
     * @return A future that will be resolved one the scpecified number of bytes has been read.
     *
     * @note If the provided destination buffer is too small to hold requested amount of data - an exception is raised.
     */
    Solace::Future<void> asyncRead(Solace::ByteBuffer& dest, std::size_t bytesToRead);

    /**
     * Post an async write request to write specified amount of data into this IO object.
     * This method writes whole content of the provided buffer into the IO objec.
     *
     * @param src The provided source buffer to read data from.
     * @return A future that will be resolved one the scpecified number of bytes has been written into the IO object.
     */
    Solace::Future<void> asyncWrite(Solace::ByteBuffer& src) {
        return asyncWrite(src, src.remaining());
    }

    /**
     * Post an async write request to write specified amount of data into this IO object.
     *
     * @param src The provided source buffer to read data from.
     * @param bytesToWrite Amount of data (in bytes) to write from the buffer into this IO object.
     * @return A future that will be resolved one the scpecified number of bytes has been written into the IO object.
     *
     * @note If the provided source buffer does not have requested amount of data - an exception is raised.
     */
    Solace::Future<void> asyncWrite(Solace::ByteBuffer& src, std::size_t bytesToWrite);

private:

    friend class TcpAcceptor;

    asio::ip::tcp::socket   _socket;
    Solace::uint16          _port;
};




class TcpAcceptor {
public:

    ~TcpAcceptor() = default;

    TcpAcceptor(EventLoop& ioContext, Solace::uint16 port);

    TcpAcceptor(const TcpAcceptor& rhs) = delete;

    TcpAcceptor& operator= (const TcpAcceptor& rhs) = delete;

    TcpAcceptor(EventLoop& ioContext);

    TcpAcceptor(TcpAcceptor&& rhs);

    TcpAcceptor& operator= (TcpAcceptor&& rhs) noexcept {
        return swap(rhs);
    }

    TcpAcceptor& swap(TcpAcceptor& rhs) noexcept {
        using std::swap;
        swap(_acceptor, rhs._acceptor);

        return *this;
    }

    bool isOpen();
    bool isClosed();
    void listen(int backlog = 9999);

    void accept(TcpSocket& socket);

    Solace::Future<void> asyncAccept(TcpSocket& socket);


    template <typename SettableSocketOption>
    void setOption(const SettableSocketOption& option) {
        _acceptor.set_option(option);
    }

    template <typename GettableSocketOption>
    void getOption(GettableSocketOption& option) {
        _acceptor.get_option(option);
    }

    bool nonBlocking();
    bool nativeNonBlocking();
    void nativeNonBlocking(bool mode);

    void cancel();
    void close();

private:

    asio::ip::tcp::acceptor _acceptor;
};


inline void swap(TcpSocket& lhs, TcpSocket& rhs) noexcept {
    lhs.swap(rhs);
}

inline void swap(TcpAcceptor& lhs, TcpAcceptor& rhs) noexcept {
    lhs.swap(rhs);
}

}  // End of namespace async
}  // End of namespace cadence
#endif  // CADENCE_ASYNC_TCPSOCKET_HPP

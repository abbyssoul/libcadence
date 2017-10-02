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
#include "cadence/ipendpoint.hpp"


namespace cadence { namespace async {

/**
 * An async wrapper for the POSIX TcpSocket
 */
class TcpSocket : public Channel {
public:

    ~TcpSocket();

    TcpSocket(EventLoop& ioContext);

    TcpSocket(const TcpSocket& rhs) = delete;
    TcpSocket& operator= (const TcpSocket& rhs) = delete;

    TcpSocket(TcpSocket&& rhs);

    TcpSocket& operator= (TcpSocket&& rhs) noexcept {
        return swap(rhs);
    }

    TcpSocket& swap(TcpSocket& rhs) noexcept;

    using Channel::asyncRead;
    using Channel::asyncWrite;


    Solace::Future<void> asyncConnect(const IPEndpoint& endpoint);

    /**
     * Post an async read request to read specified amount of data from this IO object into the given buffer.
     *
     * @param dest The provided destination buffer to read data into.
     * @param bytesToRead Amount of data (in bytes) to read from this IO object.
     * @return A future that will be resolved one the scpecified number of bytes has been read.
     *
     * @note If the provided destination buffer is too small to hold requested amount of data - an exception is raised.
     */
    Solace::Future<void> asyncRead(Solace::ByteBuffer& dest, size_type bytesToRead) override;

    /**
     * Post an async write request to write specified amount of data into this IO object.
     *
     * @param src The provided source buffer to read data from.
     * @param bytesToWrite Amount of data (in bytes) to write from the buffer into this IO object.
     * @return A future that will be resolved one the scpecified number of bytes has been written into the IO object.
     *
     * @note If the provided source buffer does not have requested amount of data - an exception is raised.
     */
    Solace::Future<void> asyncWrite(Solace::ByteBuffer& src, size_type bytesToWrite) override;

    /**
     * Cancel all asynchronous operations associated with the socket.
     */
    void cancel();

    /**
     * Close the socket.
     */
    void close();

    /**
     * Connect the socket to the specified endpoint syncroniosly.
     * @param endpoint
     */
    void connect(const IPEndpoint& endpoint);

    /**
     * Determine whether the socket is open.
     * @return True if socket is opened.
     */
    bool isOpen();

    /**
     * Determine whether the socket is closed.
     * @return True if socket is NOT opened.
     */
    bool isClosed();

    /**
     * Get the local endpoint of the socket.
     * @return Local endpoint this socket is bound to.
     */
    IPEndpoint getLocalEndpoint() const;

    /**
     * Get the remote endpoint of the socket.
     * @return Remote endpoint this socket is connected to if any.
     */
    IPEndpoint getRemoteEndpoint() const;

    /**
     * Disable sends or receives on the socket.
     */
    void shutdown();

private:

    friend class TcpAcceptor;
    class TcpSocketImpl;
    std::unique_ptr<TcpSocketImpl> _pimpl;
};




class TcpAcceptor {
public:

    ~TcpAcceptor();

    TcpAcceptor(EventLoop& ioContext, Solace::uint16 port);

    TcpAcceptor(const TcpAcceptor& rhs) = delete;

    TcpAcceptor& operator= (const TcpAcceptor& rhs) = delete;

    TcpAcceptor(EventLoop& ioContext);

    TcpAcceptor(TcpAcceptor&& rhs);

    TcpAcceptor& operator= (TcpAcceptor&& rhs) noexcept {
        return swap(rhs);
    }

    TcpAcceptor& swap(TcpAcceptor& rhs) noexcept;

    bool isOpen();
    bool isClosed();
    void listen(int backlog = 9999);

    void accept(TcpSocket& socket);

    Solace::Future<void> asyncAccept(TcpSocket& socket);


//    template <typename SettableSocketOption>
//    void setOption(const SettableSocketOption& option) {
//        _acceptor.set_option(option);
//    }

//    template <typename GettableSocketOption>
//    void getOption(GettableSocketOption& option) {
//        _acceptor.get_option(option);
//    }

    bool nonBlocking();
    bool nativeNonBlocking();
    void nativeNonBlocking(bool mode);

    void cancel();
    void close();

private:

    class AcceptorImpl;
    std::unique_ptr<AcceptorImpl> _pimpl;

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

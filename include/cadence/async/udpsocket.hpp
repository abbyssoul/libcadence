/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Async UDP socket
 *	@file		cadence/async/udpsocket.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNC_UDPSOCKET_HPP
#define CADENCE_ASYNC_UDPSOCKET_HPP

#include "cadence/async/channel.hpp"
#include "cadence/ipendpoint.hpp"


namespace cadence { namespace async {

/**
 * An async wrapper for the POSIX UdpSocket
 */
class UdpSocket : public Channel {
public:

    ~UdpSocket();

    UdpSocket(EventLoop& eventLoop, Solace::uint16 port);

    UdpSocket(const UdpSocket& rhs) = delete;
    UdpSocket& operator= (const UdpSocket& rhs) = delete;

    UdpSocket(EventLoop& ioContext);

    UdpSocket(UdpSocket&& rhs);

    UdpSocket& operator= (UdpSocket&& rhs) noexcept {
        return swap(rhs);
    }

    UdpSocket& swap(UdpSocket& rhs) noexcept;

    using Channel::asyncRead;
    using Channel::asyncWrite;

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
     * Post an async read request to read data from this IO object into the given buffer.
     * This method reads the data until the provided destination buffer is full.
     *
     * @param dest The provided destination buffer to read data into.
     * @return A future that will be resolved one the buffer has been filled.
     */
    Solace::Future<void> asyncReadFrom(Solace::ByteBuffer& dest, const IPEndpoint& remote) {
        return asyncReadFrom(dest, dest.remaining(), remote);
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
    Solace::Future<void> asyncReadFrom(Solace::ByteBuffer& dest, size_type bytesToRead, const IPEndpoint& remote);

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
     * Post an async write request to write specified amount of data into this IO object.
     * This method writes whole content of the provided buffer into the IO objec.
     *
     * @param src The provided source buffer to read data from.
     * @return A future that will be resolved one the scpecified number of bytes has been written into the IO object.
     */
    Solace::Future<void> asyncWriteTo(Solace::ByteBuffer& data, const IPEndpoint& dest) {
        return asyncWriteTo(data, data.remaining(), dest);
    }

    /**
     * Post an async write request to write specified amount of data into this IO object.
     * This method writes whole content of the provided buffer into the IO objec.
     *
     * @param src The provided source buffer to read data from.
     * @return A future that will be resolved one the scpecified number of bytes has been written into the IO object.
     */
    Solace::Future<void> asyncWriteTo(Solace::ByteBuffer& data, size_type bytesToWrite, const IPEndpoint& dest);

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
    Solace::Result<void, Solace::Error> connect(const IPEndpoint& endpoint);

    /** @see Channel::read */
    Solace::Result<void, Solace::Error> read(Solace::ByteBuffer& dest, size_type bytesToRead) override;

    /** @see Channel::write */
    Solace::Result<void, Solace::Error> write(Solace::ByteBuffer& src, size_type bytesToWrite) override;

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

    class UdpImpl;
    std::unique_ptr<UdpImpl> _pimpl;
};


inline void swap(UdpSocket& lhs, UdpSocket& rhs) noexcept {
    lhs.swap(rhs);
}

}  // End of namespace async
}  // End of namespace cadence
#endif  // CADENCE_ASYNC_UDPSOCKET_HPP

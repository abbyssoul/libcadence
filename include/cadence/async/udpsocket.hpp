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
class UdpSocket :
        public Channel {
public:

    ~UdpSocket() override;

    UdpSocket(EventLoop& eventLoop, Solace::uint16 port);

    UdpSocket(UdpSocket const& rhs) = delete;
    UdpSocket& operator= (UdpSocket const& rhs) = delete;

    UdpSocket(EventLoop& ioContext);

    UdpSocket(UdpSocket&& rhs) noexcept;

    UdpSocket& operator= (UdpSocket&& rhs) noexcept {
        return swap(rhs);
    }

    UdpSocket& swap(UdpSocket& rhs) noexcept;

    using Channel::asyncRead;
    using Channel::asyncWrite;
    using Channel::read;
    using Channel::write;

    /**
     * Post an async read request to read specified amount of data from this IO object into the given buffer.
     *
     * @param dest The provided destination buffer to read data into.
     * @param bytesToRead Amount of data (in bytes) to read from this IO object.
     * @return A future that will be resolved one the scpecified number of bytes has been read.
     *
     * @note If the provided destination buffer is too small to hold requested amount of data - an exception is raised.
     */
    Solace::Future<void> asyncRead(Solace::ByteWriter& dest, size_type bytesToRead) override;

    /**
     * Post an async read request to read data from this IO object into the given buffer.
     * This method reads the data until the provided destination buffer is full.
     *
     * @param dest The provided destination buffer to read data into.
     * @return A future that will be resolved one the buffer has been filled.
     */
    Solace::Future<IPEndpoint> asyncReadFrom(Solace::ByteWriter& dest) {
        return asyncReadFrom(dest, dest.remaining());
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
    Solace::Future<IPEndpoint> asyncReadFrom(Solace::ByteWriter& dest, size_type bytesToRead);

    /**
     * Post an async write request to write specified amount of data into this IO object.
     *
     * @param src The provided source buffer to read data from.
     * @param bytesToWrite Amount of data (in bytes) to write from the buffer into this IO object.
     * @return A future that will be resolved one the scpecified number of bytes has been written into the IO object.
     *
     * @note If the provided source buffer does not have requested amount of data - an exception is raised.
     */
    Solace::Future<void> asyncWrite(Solace::ByteReader& src, size_type bytesToWrite) override;

    /**
     * Post an async write request to write specified amount of data into this IO object.
     * This method writes whole content of the provided buffer into the IO objec.
     *
     * @param src The provided source buffer to read data from.
     * @return A future that will be resolved one the scpecified number of bytes has been written into the IO object.
     */
    Solace::Future<void> asyncWriteTo(IPEndpoint const& dest, Solace::ByteReader& data) {
        return asyncWriteTo(dest, data, data.remaining());
    }

    /**
     * Post an async write request to write specified amount of data into this IO object.
     * This method writes whole content of the provided buffer into the IO objec.
     *
     * @param src The provided source buffer to read data from.
     * @return A future that will be resolved one the scpecified number of bytes has been written into the IO object.
     */
    Solace::Future<void> asyncWriteTo(IPEndpoint const& dest, Solace::ByteReader& data, size_type bytesToWrite);

    /**
     * Cancel all asynchronous operations associated with the socket.
     */
    void cancel() override;

    /**
     * Close the socket.
     */
    void close() override;

    /**
     * Connect the socket to the specified endpoint syncroniosly.
     * @param endpoint
     */
    Solace::Result<void, Solace::Error> connect(IPEndpoint const& endpoint);

    /** @see Channel::read */
    Solace::Result<void, Solace::Error> read(Solace::ByteWriter& dest, size_type bytesToRead) override;

    /** @see Channel::write */
    Solace::Result<void, Solace::Error> write(Solace::ByteReader& src, size_type bytesToWrite) override;

    /**
     * Determine whether the socket is open.
     * @return True if socket is opened.
     */
    bool isOpen() const override;

    /**
     * Determine whether the socket is closed.
     * @return True if socket is NOT opened.
     */
    bool isClosed() const override;

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

    /**
     * Open the socket if it was not opened alread.
     * @return Open result or an error.
     */
    Solace::Result<void, Solace::Error>
    open();

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

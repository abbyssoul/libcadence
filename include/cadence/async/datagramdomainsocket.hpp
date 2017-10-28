/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: DatagramDomainSocket
 *	@file		cadence/async/datagramdomainsocket.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNC_DATAGRAMDOMAINSOCKET_HPP
#define CADENCE_ASYNC_DATAGRAMDOMAINSOCKET_HPP

#include "cadence/async/channel.hpp"
#include "cadence/unixDomainEndpoint.hpp"


namespace cadence { namespace async {

class DatagramDomainSocket
        : public Channel {
public:

    ~DatagramDomainSocket();

    DatagramDomainSocket(const DatagramDomainSocket& rhs) = delete;
    DatagramDomainSocket& operator= (const DatagramDomainSocket& rhs) = delete;

    DatagramDomainSocket(EventLoop& ioContext);

    DatagramDomainSocket(EventLoop& ioContext, const NetworkEndpoint& endpoint);

    DatagramDomainSocket(DatagramDomainSocket&& rhs);

    DatagramDomainSocket& operator= (DatagramDomainSocket&& rhs) noexcept {
        return swap(rhs);
    }

    DatagramDomainSocket& swap(DatagramDomainSocket& rhs) noexcept;

    using Channel::asyncRead;
    using Channel::asyncWrite;


    Solace::Future<void> asyncConnect(const NetworkEndpoint& endpoint);

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
    Solace::Future<void> asyncReadFrom(Solace::ByteBuffer& dest, const NetworkEndpoint& endpoint) {
        return asyncReadFrom(dest, dest.remaining(), endpoint);
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
    Solace::Future<void> asyncReadFrom(Solace::ByteBuffer& dest, std::size_t bytesToRead,
                                       const NetworkEndpoint& endpoint);

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
    Solace::Future<void> asyncWriteTo(Solace::ByteBuffer& src, const NetworkEndpoint& endpoint) {
        return asyncWriteTo(src, src.remaining(), endpoint);
    }
    /**
     * Post an async write request to write specified amount of data into this IO object.
     *
     * @param src The provided source buffer to read data from.
     * @param bytesToWrite Amount of data (in bytes) to write from the buffer into this IO object.
     * @param endpoint A remote end point to send bytes to.
     * @return A future that will be resolved one the scpecified number of bytes has been written into the IO object.
     *
     * @note If the provided source buffer does not have requested amount of data - an exception is raised.
     */
    Solace::Future<void>
    asyncWriteTo(Solace::ByteBuffer& src, std::size_t bytesToWrite, const NetworkEndpoint& endpoint);

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
    void connect(const NetworkEndpoint& endpoint);

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
    UnixEndpoint getLocalEndpoint() const;

    /**
     * Get the remote endpoint of the socket.
     * @return Remote endpoint this socket is connected to if any.
     */
    UnixEndpoint getRemoteEndpoint() const;

    /**
     * Disable sends or receives on the socket.
     */
    void shutdown();

protected:

    class SocketImpl;
    std::unique_ptr<SocketImpl> _pimpl;
};


inline void swap(DatagramDomainSocket& lhs, DatagramDomainSocket& rhs) noexcept {
    lhs.swap(rhs);
}

}  // End of namespace async
}  // End of namespace cadence
#endif  // CADENCE_ASYNC_DATAGRAMDOMAINSOCKET_HPP

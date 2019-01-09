/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Async Stream Sockets
 *	@file		cadence/async/streamsocket.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNC_STREAMSOCKET_HPP
#define CADENCE_ASYNC_STREAMSOCKET_HPP

#include "cadence/async/channel.hpp"
#include "cadence/networkEndpoint.hpp"



namespace cadence { namespace async {

/**
 * Base class for stream-oriented sockets
 */
class StreamSocket :
        public Channel {
public:

    using Channel::size_type;

    ~StreamSocket() override;

    StreamSocket(StreamSocket const &) = delete;
    StreamSocket& operator= (StreamSocket const& ) = delete;

    StreamSocket(StreamSocket&& rhs) = default;
    StreamSocket& operator= (StreamSocket&& rhs) noexcept = default;

    StreamSocket& swap(StreamSocket& rhs) noexcept {
        using std::swap;
        swap(_pimpl, rhs._pimpl);

        return *this;
    }


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
     * Post an async write request to write specified amount of data into this IO object.
     *
     * @param src The provided source buffer to read data from.
     * @param bytesToWrite Amount of data (in bytes) to write from the buffer into this IO object.
     * @return A future that will be resolved one the scpecified number of bytes has been written into the IO object.
     *
     * @note If the provided source buffer does not have requested amount of data - an exception is raised.
     */
    Solace::Future<void> asyncWrite(Solace::ByteReader& src, size_type bytesToWrite) override;

    /** @see Channel::cancel */
    void cancel() override;

    /** @see Channel::close */
    void close() override;

    /** @see Channel::isOpen */
    bool isOpen() const override;

    /** @see Channel::isClosed */
    bool isClosed() const override;


    /** @see Channel::read */
    Solace::Result<void, Solace::Error> read(Solace::ByteWriter& dest, size_type bytesToRead) override;

    /** @see Channel::write */
    Solace::Result<void, Solace::Error> write(Solace::ByteReader& src, size_type bytesToWrite) override;

    /**
     * Get the local endpoint of the socket.
     * @return Local endpoint this socket is bound to.
     */
    NetworkEndpoint getLocalEndpoint() const;

    /**
     * Get the remote endpoint of the socket.
     * @return Remote endpoint this socket is connected to if any.
     */
    NetworkEndpoint getRemoteEndpoint() const;

    /**
     * Disable sends or receives on the socket.
     */
    void shutdown();


    /**
     * Start an syncronous connection to the given endpoint.
     * This call will block until a connection is complete (either successfully or in an error)
     * @param endpoint An endpoint to connect to.
     */
    Solace::Result<void, Solace::Error> connect(NetworkEndpoint const& endpoint);


    /**
     * Start an asynchronous connection to the given endpoint.
     * @param endpoint An endpoint to connect to.
     * @return Future that is resolved when connection is establised or an error occured.
     */
    Solace::Future<void> asyncConnect(NetworkEndpoint const& endpoint);


public:

    class StreamSocketImpl {
    public:

        virtual ~StreamSocketImpl();

        virtual
        Solace::Future<void>
        asyncRead(Solace::ByteWriter& dest, size_type bytesToRead) = 0;

        virtual
        Solace::Future<void>
        asyncWrite(Solace::ByteReader& src, size_type bytesToWrite) = 0;

        virtual
        Solace::Result<void, Solace::Error>
        read(Solace::ByteWriter& dest, size_type bytesToRead) = 0;

        virtual
        Solace::Result<void, Solace::Error>
        write(Solace::ByteReader& src, size_type bytesToWrite) = 0;

        virtual
        void cancel() = 0;

        virtual
        void close() = 0;

        virtual
        bool isOpen() const = 0;

        virtual
        bool isClosed() const = 0;

        virtual
        NetworkEndpoint getLocalEndpoint() const = 0;

        virtual
        NetworkEndpoint getRemoteEndpoint() const = 0;

        virtual
        void shutdown() = 0;

        virtual Solace::Future<void>
        asyncConnect(NetworkEndpoint const& endpoint) = 0;

        virtual Solace::Result<void, Solace::Error>
        connect(NetworkEndpoint const& endpoint) = 0;

    };

    StreamSocket(EventLoop& ioContext, std::unique_ptr<StreamSocketImpl> impl);

private:
    std::unique_ptr<StreamSocketImpl> _pimpl;

};


inline void swap(StreamSocket& lhs, StreamSocket& rhs) noexcept {
    lhs.swap(rhs);
}


StreamSocket createTCPSocket(EventLoop& loop);
StreamSocket createUnixSocket(EventLoop& loop);


}  // End of namespace async
}  // End of namespace cadence
#endif  // CADENCE_ASYNC_STREAMSOCKET_HPP

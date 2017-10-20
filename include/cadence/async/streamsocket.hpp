/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Async Stream Domain/Unix Sockets
 *	@file		cadence/async/streamdomainsocket.hpp
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

    virtual ~StreamSocket() = default;


    StreamSocket(EventLoop& ioContext) :
        Channel(ioContext)
    { }

    StreamSocket(StreamSocket&& rhs) :
        Channel(std::move(rhs))
    { }

    /**
     * Start an syncronous connection to the given endpoint.
     * This call will block until a connection is complete (either successfully or in an error)
     * @param endpoint An endpoint to connect to.
     */
    virtual Solace::Result<void, Solace::Error> connect(const NetworkEndpoint& endpoint) = 0;

    /**
     * Start an asynchronous connection to the given endpoint.
     * @param endpoint An endpoint to connect to.
     * @return Future that is resolved when connection is establised or an error occured.
     */
    virtual Solace::Future<void> asyncConnect(const NetworkEndpoint& endpoint) = 0;

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
    virtual Solace::Future<void> asyncRead(Solace::ByteBuffer& dest, size_type bytesToRead) = 0;

    /**
     * Post an async write request to write specified amount of data into this IO object.
     *
     * @param src The provided source buffer to read data from.
     * @param bytesToWrite Amount of data (in bytes) to write from the buffer into this IO object.
     * @return A future that will be resolved one the scpecified number of bytes has been written into the IO object.
     *
     * @note If the provided source buffer does not have requested amount of data - an exception is raised.
     */
    virtual Solace::Future<void> asyncWrite(Solace::ByteBuffer& src, size_type bytesToWrite) = 0;
};

}  // End of namespace async
}  // End of namespace cadence
#endif  // CADENCE_ASYNC_STREAMSOCKET_HPP

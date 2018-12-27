/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Channel interface for async events
 *	@file		cadence/async/channel.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNC_CHANNEL_HPP
#define CADENCE_ASYNC_CHANNEL_HPP


#include "cadence/async/eventloop.hpp"


#include <solace/byteReader.hpp>
#include <solace/byteWriter.hpp>
#include <solace/future.hpp>


namespace cadence { namespace async {


/**
 * Channel is the base of the io objects.
 * Subclasses of this class provide access to the real IO subsystems
 */
class Channel {
public:

    using size_type = Solace::ByteWriter::size_type;

public:

    virtual ~Channel();

    Channel(const Channel&) = delete;
    Channel& operator= (const Channel& rhs) = delete;

    Channel(EventLoop& ioContext) :
        _ioContext(&ioContext)
    {}

    Channel(Channel&& rhs) :
        _ioContext(std::exchange(rhs._ioContext, nullptr))
    {}

    Channel& operator= (Channel&& rhs) noexcept {
        return swap(rhs);
    }

    Channel& swap(Channel& rhs) noexcept {
        std::swap(_ioContext, rhs._ioContext);

        return *this;
    }

    EventLoop& getIOContext() noexcept {
        return *_ioContext;
    }

    const EventLoop& getIOContext() const noexcept {
        return *_ioContext;
    }

    /**
     * Post an async read request to read data from this IO object into the given buffer.
     * This method reads the data until the provided destination buffer is full.
     *
     * @param dest The provided destination buffer to read data into.
     * @return A future that will be resolved one the buffer has been filled.
     */
    Solace::Future<void> asyncRead(Solace::ByteWriter& dest) {
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
    virtual Solace::Future<void> asyncRead(Solace::ByteWriter& dest, size_type bytesToRead) = 0;

    /**
     * Post an async write request to write specified amount of data into this IO object.
     * This method writes whole content of the provided buffer into the IO objec.
     *
     * @param src The provided source buffer to read data from.
     * @return A future that will be resolved one the scpecified number of bytes has been written into the IO object.
     */
    Solace::Future<void> asyncWrite(Solace::ByteReader& src) {
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
    virtual Solace::Future<void> asyncWrite(Solace::ByteReader& src, size_type bytesToWrite) = 0;

    /**
     * Read request synchroniously from this IO object into the given buffer.
     * This method reads the data until the provided destination buffer is full.
     *
     * @param dest The provided destination buffer to read data into.
     * @return A operation result or an error.
     */
    Solace::Result<void, Solace::Error> read(Solace::ByteWriter& dest) {
        return read(dest, dest.remaining());
    }

    /**
     * Read request synchroniously from this IO object into the given buffer.
     * This method reads the data until the provided destination buffer is full.
     *
     * @param dest The provided destination buffer to read data into.
     * @param bytesToRead Amount of data (in bytes) to read from this IO object.
     * @return A operation result or an error.
     *
     * @note If the provided destination buffer is too small to hold requested amount of data - an exception is raised.
     */
    virtual Solace::Result<void, Solace::Error> read(Solace::ByteWriter& dest, size_type bytesToRead) = 0;

    /**
     * Post an async write request to write specified amount of data into this IO object.
     * This method writes whole content of the provided buffer into the IO objec.
     *
     * @param src The provided source buffer to read data from.
     * @return A operation result or an error.
     */
    Solace::Result<void, Solace::Error> write(Solace::ByteReader& src) {
        return write(src, src.remaining());
    }

    /**
     * Post an async write request to write specified amount of data into this IO object.
     *
     * @param src The provided source buffer to read data from.
     * @param bytesToWrite Amount of data (in bytes) to write from the buffer into this IO object.
     * @return A operation result or an error.
     *
     * @note If the provided source buffer does not have requested amount of data - an exception is raised.
     */
    virtual Solace::Result<void, Solace::Error> write(Solace::ByteReader& src, size_type bytesToWrite) = 0;


    /**
     * Cancel all asynchronous operations associated with the channel.
     */
    virtual void cancel() = 0;

    /**
     * Close the channel.
     */
    virtual void close() = 0;


    /**
     * Determine whether the channel is open.
     * @return True if the channel is opened.
     */
    virtual bool isOpen() = 0;

    /**
     * Determine whether the channel is closed.
     * @return True if the channel is NOT opened.
     */
    virtual bool isClosed() = 0;

private:

    EventLoop*   _ioContext;
};


inline void swap(Channel& lhs, Channel& rhs) noexcept {
    lhs.swap(rhs);
}

}  // End of namespace async
}  // End of namespace cadence
#endif  // CADENCE_ASYNC_CHANNEL_HPP

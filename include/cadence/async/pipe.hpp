/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Async Pipe
 *	@file		candece/async/pipe.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNC_PIPE_HPP
#define CADENCE_ASYNC_PIPE_HPP

#include "cadence/async/channel.hpp"


namespace cadence { namespace async {

/**
 * An async wrapper for the POSIX pipe
 */
class Pipe :
        public Channel {
public:

    ~Pipe() override;

    Pipe(const Pipe& rhs) = delete;
    Pipe& operator= (const Pipe& rhs) = delete;

    Pipe(EventLoop& ioContext);

    Pipe(Pipe&& rhs) noexcept;

    Pipe& operator= (Pipe&& rhs) noexcept {
        return swap(rhs);
    }

    Pipe& swap(Pipe& rhs) noexcept;

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

    /** @see Channel::read */
    Solace::Result<void, Solace::Error> read(Solace::ByteWriter& dest, size_type bytesToRead) override;

    /** @see Channel::write */
    Solace::Result<void, Solace::Error> write(Solace::ByteReader& src, size_type bytesToWrite) override;


    /** @see Channel::cancel */
    void cancel() override;

    /** @see Channel::close */
    void close() override;

    /** @see Channel::isOpen */
    bool isOpen() const override;

    /** @see Channel::isClosed */
    bool isClosed() const override;


private:

    class PipeImpl;
    std::unique_ptr<PipeImpl> _pimpl;
};


inline void swap(Pipe& lhs, Pipe& rhs) noexcept {
    lhs.swap(rhs);
}

}  // End of namespace async
}  // End of namespace cadence
#endif  // CADENCE_ASYNC_PIPE_HPP

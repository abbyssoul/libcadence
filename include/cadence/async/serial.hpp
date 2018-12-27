/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Async interface for Serial Channel
 *	@file		cadence/async/serial.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_ASYNC_SERIAL_HPP
#define CADENCE_ASYNC_SERIAL_HPP

#include "cadence/async/channel.hpp"
#include "cadence/io/serial.hpp"


namespace cadence { namespace async {

class SerialChannel :
        public Channel {
public:

    ~SerialChannel() override;

    SerialChannel(EventLoop& ioContext,
           Solace::Path const& file,
           Solace::uint32 baudrate = 9600,
           Serial::Bytesize bytesize = Serial::Bytesize::eightbits,
           Serial::Parity parity = Serial::Parity::none,
           Serial::Stopbits stopbits = Serial::Stopbits::one,
           Serial::Flowcontrol flowcontrol = Serial::Flowcontrol::none);

    SerialChannel(const SerialChannel& rhs) = delete;
    SerialChannel& operator= (const SerialChannel& rhs) = delete;

    SerialChannel(SerialChannel&& rhs);

    SerialChannel& operator= (SerialChannel&& rhs) noexcept {
        return swap(rhs);
    }

    SerialChannel& swap(SerialChannel& rhs) noexcept;

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


    /** @see Channel::read */
    Solace::Result<void, Solace::Error> read(Solace::ByteWriter& dest, size_type bytesToRead) override;

    /** @see Channel::write */
    Solace::Result<void, Solace::Error> write(Solace::ByteReader& src, size_type bytesToWrite) override;


    /** @see Channel::cancel */
    void cancel() override;

    /** @see Channel::close */
    void close() override;

    /** @see Channel::isOpen */
    bool isOpen() override;

    /** @see Channel::isClosed */
    bool isClosed() override;


private:

    class SerialImpl;
    std::unique_ptr<SerialImpl> _pimpl;
};


inline void swap(SerialChannel& lhs, SerialChannel& rhs) noexcept {
    lhs.swap(rhs);
}

}  // End of namespace async
}  // End of namespace cadence
#endif  // CADENCE_ASYNC_SERIAL_HPP

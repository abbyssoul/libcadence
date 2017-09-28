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

#include <solace/io/serial.hpp>


namespace cadence { namespace async {

class SerialChannel {
public:
    typedef std::size_t size_type;

public:

    ~SerialChannel() = default;

    SerialChannel(EventLoop& ioContext,
           const Solace::Path& file,
           Solace::uint32 baudrate = 9600,
           Solace::IO::Serial::Bytesize bytesize = Solace::IO::Serial::Bytesize::eightbits,
           Solace::IO::Serial::Parity parity = Solace::IO::Serial::Parity::none,
           Solace::IO::Serial::Stopbits stopbits = Solace::IO::Serial::Stopbits::one,
           Solace::IO::Serial::Flowcontrol flowcontrol = Solace::IO::Serial::Flowcontrol::none);

    SerialChannel(const SerialChannel& rhs) = delete;

    SerialChannel& operator= (const SerialChannel& rhs) = delete;

    SerialChannel(SerialChannel&& rhs);

    SerialChannel& operator= (SerialChannel&& rhs) noexcept {
        return swap(rhs);
    }

    SerialChannel& swap(SerialChannel& rhs) noexcept {
        using std::swap;

        swap(_serial, rhs._serial);

        return *this;
    }
    /**
	 * Post an async read request to read data from this IO object into the given buffer.
	 * This method reads the data until the provided destination buffer is full.
	 *
	 * @param dest The provided destination buffer to read data into.
	 * @return A future that will be resolved one the buffer has been filled.
	 */
    Solace::Future<size_type> asyncRead(Solace::ByteBuffer& dest) {
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
    Solace::Future<size_type> asyncRead(Solace::ByteBuffer& dest, size_type bytesToRead);

	/**
	 * Post an async write request to write specified amount of data into this IO object.
	 * This method writes whole content of the provided buffer into the IO objec.
	 *
	 * @param src The provided source buffer to read data from.
	 * @return A future that will be resolved one the scpecified number of bytes has been written into the IO object.
	 */
    Solace::Future<size_type> asyncWrite(Solace::ByteBuffer& src) {
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
    Solace::Future<size_type> asyncWrite(Solace::ByteBuffer& src, size_type bytesToWrite);

private:

	asio::serial_port _serial;

};


inline void swap(SerialChannel& lhs, SerialChannel& rhs) noexcept {
    lhs.swap(rhs);
}

}  // End of namespace async
}  // End of namespace cadence
#endif  // CADENCE_ASYNC_SERIAL_HPP

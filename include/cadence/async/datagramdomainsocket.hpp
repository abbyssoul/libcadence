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

#include <solace/string.hpp>


namespace cadence { namespace async {

class DatagramDomainSocket {
public:
    typedef Solace::String endpoint_type;

public:

    ~DatagramDomainSocket() = default;

    DatagramDomainSocket(const DatagramDomainSocket& rhs) = delete;

    DatagramDomainSocket& operator= (const DatagramDomainSocket& rhs) = delete;

     DatagramDomainSocket(EventLoop& ioContext, const endpoint_type& serviceName);

//    DatagramDomainSocket(EventLoop& ioContext);

    DatagramDomainSocket(DatagramDomainSocket&& rhs);

    DatagramDomainSocket& operator= (DatagramDomainSocket&& rhs) noexcept {
        return swap(rhs);
    }

    DatagramDomainSocket& swap(DatagramDomainSocket& rhs) noexcept {
        using std::swap;
        swap(_socket, rhs._socket);

        return *this;
    }

    /**
     * Post an async read request to read data from this IO object into the given buffer.
     * This method reads the data until the provided destination buffer is full.
     *
     * @param dest The provided destination buffer to read data into.
     * @return A future that will be resolved one the buffer has been filled.
     */
    Solace::Future<void> asyncRead(Solace::ByteBuffer& dest, endpoint_type serviceName) {
        return asyncRead(dest, dest.remaining(), serviceName);
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
    Solace::Future<void> asyncRead(Solace::ByteBuffer& dest, std::size_t bytesToRead, endpoint_type serviceName);

    /**
     * Post an async write request to write specified amount of data into this IO object.
     * This method writes whole content of the provided buffer into the IO objec.
     *
     * @param src The provided source buffer to read data from.
     * @return A future that will be resolved one the scpecified number of bytes has been written into the IO object.
     */
    Solace::Future<void> asyncWrite(Solace::ByteBuffer& src, const endpoint_type& serviceName) {
        return asyncWrite(src, src.remaining(), serviceName);
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
    Solace::Future<void> asyncWrite(Solace::ByteBuffer& src, std::size_t bytesToWrite, endpoint_type serviceName);

protected:

    asio::local::datagram_protocol::socket _socket;
    asio::local::datagram_protocol::endpoint _socket_endpoint;

};


inline void swap(DatagramDomainSocket& lhs, DatagramDomainSocket& rhs) noexcept {
    lhs.swap(rhs);
}

}  // End of namespace async
}  // End of namespace cadence
#endif  // CADENCE_ASYNC_DATAGRAMDOMAINSOCKET_HPP

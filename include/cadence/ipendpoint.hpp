#include <utility>

/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: IP Endpoint
 *	@file		cadence/ipendpoint.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_IPENDPOINT_HPP
#define CADENCE_IPENDPOINT_HPP

#include "ipaddress.hpp"

#include <solace/types.hpp>


namespace cadence {

/**
 * TODO(abbyssoul): document this class
 */
class IPEndpoint {
public:

    IPEndpoint(IPEndpoint const& ) = default;
    IPEndpoint(IPEndpoint&& ) = default;
    IPEndpoint& operator= (IPEndpoint&& ) = default;
    IPEndpoint& operator= (IPEndpoint const& ) = default;

    IPEndpoint(IPAddress address, Solace::uint16 port) noexcept
        : _port(port)
        , _ipAddress(std::move(address))
    {
    }


    IPAddress getAddress() const noexcept {
        return _ipAddress;
    }

    Solace::uint16 getPort() const noexcept {
        return _port;
    }


    //!< @see Solace::IFormattable::toString
    Solace::String toString() const;

private:

//    sockaddr_storage    _addr;
    Solace::uint16  _port;
    IPAddress       _ipAddress;
};


Solace::Result<IPEndpoint, Solace::Error>
parseIPEndpoint(Solace::StringView str);

}  // End of namespace cadence
#endif  // CADENCE_IPENDPOINT_HPP

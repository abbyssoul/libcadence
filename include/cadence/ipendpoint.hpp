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

#include "networkEndpoint.hpp"

#include <solace/string.hpp>

namespace cadence {

/**
 * TODO(abbyssoul): document this class
 */
class IPEndpoint :
        public NetworkEndpoint {
public:

    IPEndpoint(const Solace::String& address, Solace::uint16 port) :
        _ipAddress(address),
        _port(port)
    {
    }

    const Solace::String& getAddress() const {
        return _ipAddress;
    }

    Solace::uint16 getPort() const {
        return _port;
    }


    //!< @see Solace::IFormattable::toString
    Solace::String toString() const override;

private:
    // FIXME: Should use IPAddress type instead of the String
    Solace::String  _ipAddress;
    Solace::uint16  _port;
};

}  // End of namespace cadence
#endif  // CADENCE_IPENDPOINT_HPP

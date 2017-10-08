/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Unix Domain Endpoint type
 *	@file		cadence/unixDomainEndpoint.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_UNIXDOMAINENDPOINT_HPP
#define CADENCE_UNIXDOMAINENDPOINT_HPP

#include "networkEndpoint.hpp"

#include <solace/string.hpp>

namespace cadence {

/**
 * An abstract class to represent network address.
 */
class UnixEndpoint :
        public NetworkEndpoint {
public:

    UnixEndpoint(const Solace::String& str) :
        _str(str)
    {}

    //!< @see Solace::IFormattable::toString
    Solace::String toString() const override {
        return _str;
    }

private:
    const Solace::String _str;
};

}  // End of namespace cadence
#endif  // CADENCE_UNIXDOMAINENDPOINT_HPP

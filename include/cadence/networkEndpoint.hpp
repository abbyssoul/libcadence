/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Abstract network endpoint type
 *	@file		cadence/networkEndpoint.hpp
 *	@author		$LastChangedBy$
 *	@date		$LastChangedDate$
 *	ID:			$Id$
 ******************************************************************************/
#pragma once
#ifndef CADENCE_NETWORKENDPOINT_HPP
#define CADENCE_NETWORKENDPOINT_HPP


#include <solace/traits/iformattable.hpp>  // toString()


namespace cadence {

/**
 * An abstract class to represent network address.
 */
class NetworkEndpoint :
        public Solace::IFormattable {
public:
    virtual ~NetworkEndpoint() = default;


    //!< @see Solace::IFormattable::toString
    virtual Solace::String toString() const = 0;
};

}  // End of namespace cadence
#endif  // CADENCE_NETWORKENDPOINT_HPP

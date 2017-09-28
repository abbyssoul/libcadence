/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence: Library version compile and linked version
 *	@file		version.cpp
 ******************************************************************************/
#include "cadence/version.hpp"


Solace::Version cadence::getBuildVersion() {
    return {CADENCE_VERSION_MAJOR,
            CADENCE_VERSION_MINOR,
            CADENCE_VERSION_REVISION,
            CADENCE_VERSION_BUILD};
}

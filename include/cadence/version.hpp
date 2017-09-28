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
 *	@file		cadence/version.hpp
 ******************************************************************************/
#pragma once
#ifndef CADENCE_VERSION_HPP
#define CADENCE_VERSION_HPP

#include <solace/version.hpp>

#define CADENCE_VERSION_MAJOR       0
#define CADENCE_VERSION_MINOR       0
#define CADENCE_VERSION_REVISION    1
#define CADENCE_VERSION_BUILD       "Proto"
#define CADENCE_VERSION (100000*CADENCE_VERSION_MAJOR + 100 * CADENCE_VERSION_MINOR + CADENCE_VERSION_REVISION)

namespace cadence {

/**
 * Get build version of the linked library.
 * @return Build version of libcadence we are linked against.
 */
Solace::Version getBuildVersion();

}  // End of namespace cadence
#endif  // CADENCE_VERSION_HPP

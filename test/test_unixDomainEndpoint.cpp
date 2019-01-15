/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence Unit Test Suit
 * @file: test/test_unixDomainEndpoint.cpp
 *******************************************************************************/
#include <cadence/unixDomainEndpoint.hpp>  // Class being tested

#include "gtest/gtest.h"


using namespace Solace;
using namespace cadence;


TEST(TestUnixDomainAddress, testIdentity) {
    const char* cname = "some/unix/socket";

    ASSERT_EQ(UnixEndpoint(makeString(cname)).toString(), cname);
}


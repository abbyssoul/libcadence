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
 * @file: test/test_ipaddress.cpp
 *******************************************************************************/
#include <cadence/ipaddress.hpp>  // Class being tested

#include "gtest/gtest.h"


using namespace Solace;
using namespace cadence;


TEST(TestIPAddress, testIdentity) {
    ASSERT_TRUE(IPAddress::loopback().isLoopback());
    ASSERT_TRUE(IPAddress::any().isUnspecified());
}


TEST(TestIPAddress, testParsingValidIP_v4) {
    auto const localhostRes = parseIPAddress("127.0.0.1");
    ASSERT_TRUE(localhostRes.isOk());
    ASSERT_EQ(IPAddress::loopback(), localhostRes.unwrap());

    auto const anyRes = parseIPAddress("0.0.0.0");
    ASSERT_TRUE(anyRes.isOk());
    ASSERT_EQ(IPAddress::any(), anyRes.unwrap());

    auto const someRes = parseIPAddress("43.18.254.17");
    ASSERT_TRUE(someRes.isOk());

    std::array<unsigned char, 4> someBytes{{43, 18, 254, 17}};
    ASSERT_EQ(IPAddress{someBytes}, someRes.unwrap());
}


TEST(TestIPAddress, testParsingInvalidIP_v4) {
    ASSERT_TRUE(parseIPAddress("127.0.0").isError());
    ASSERT_TRUE(parseIPAddress("12x.7.0.0").isError());
    ASSERT_TRUE(parseIPAddress("random-place").isError());
}


TEST(TestIPAddress, testToString) {
    ASSERT_EQ(StringView("127.0.0.1"), IPAddress::loopback().toString());
    ASSERT_EQ(StringView("0.0.0.0"), IPAddress::any().toString());

    auto const someAddr = IPAddress{std::array<unsigned char, 4>{{43, 18, 254, 17}}};
    ASSERT_EQ(StringView{"43.18.254.17"}, someAddr.toString());
}

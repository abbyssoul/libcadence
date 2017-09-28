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
 * @file: test/protocols/test_9p2000x.cpp
 * @author: soultaker
 *
 *******************************************************************************/
#include <cadence/protocols/9p2000x.hpp>  // Class being tested

#include <cppunit/extensions/HelperMacros.h>

using namespace Solace;
using namespace cadence;


class Test9p2000x : public CppUnit::TestFixture {

    CPPUNIT_TEST_SUITE(Test9p2000x);
        CPPUNIT_TEST(testHeaderSize);
        CPPUNIT_TEST(createVersionRequest);
    CPPUNIT_TEST_SUITE_END();

protected:
public:

    void setUp() override {
    }

    void tearDown() override {
    }

    void testHeaderSize() {
        P9Protocol proc;

        CPPUNIT_ASSERT_EQUAL(4u + 1u + 2u, proc.headerSize());
    }

    void createVersionRequest() {
        MemoryManager _mem(1024);
        ByteBuffer buffer(_mem.create(512));

        String testVersion = P9Protocol::PROTOCOL_VERSION;
        const uint64 versionStringLen = testVersion.size();

        P9Protocol proc;
        proc.createVersionRequest(0, buffer, testVersion);

        CPPUNIT_ASSERT_EQUAL(proc.headerSize() + 4 + 2 + versionStringLen,
                    buffer.position()
                    );
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION(Test9p2000x);

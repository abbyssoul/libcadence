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
 * @file: test/async/test_event.cpp
 * @author: soultaker
 *
 * Created on: 10/10/2016
 *******************************************************************************/
#include <cadence/async/event.hpp>  // Class being tested

#include <cppunit/extensions/HelperMacros.h>

using namespace Solace;
using namespace cadence::async;


class TestAsyncEvent : public CppUnit::TestFixture {

	CPPUNIT_TEST_SUITE(TestAsyncEvent);
        CPPUNIT_TEST(testSubscription);
	CPPUNIT_TEST_SUITE_END();

protected:
public:

	void testSubscription() {
        EventLoop iocontext;
        Event event(iocontext);

        bool eventWasCalled = false;

        event.asyncWait().then([&eventWasCalled, &iocontext]() {
            eventWasCalled = true;
        });

        asio::thread t([&iocontext]() {
            using namespace std::chrono_literals;
            iocontext.run();

            std::this_thread::sleep_for(200ms);


        });
        event.notify();

        t.join();

        CPPUNIT_ASSERT(eventWasCalled);
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION(TestAsyncEvent);

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
 * @file: test/async/test_timer.cpp
 * @author: soultaker
 *
 * Created on: 10/10/2016
 *******************************************************************************/
#include <cadence/async/timer.hpp>  // Class being tested

#include <cppunit/extensions/HelperMacros.h>

#include <thread>
#include <chrono>

using namespace Solace;
using namespace cadence::async;
using namespace std::chrono_literals;


class TestAsyncTimer : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(TestAsyncTimer);
        CPPUNIT_TEST(testTimeout);
        CPPUNIT_TEST(testGetTimeout);
	CPPUNIT_TEST_SUITE_END();

protected:
public:

	void testTimeout() {
        EventLoop iocontext;
        int64 nbTimesCalled = 0;

        Timer timer(iocontext, boost::posix_time::millisec(15));

        // We managed to cancel the timer. Start new asynchronous wait.
        timer.asyncWait().then([&nbTimesCalled, &iocontext](int64 numberOfExpirations) {
            nbTimesCalled += numberOfExpirations;
        });

        iocontext.run();
        std::thread watchdog([&iocontext]() {
            using namespace std::chrono_literals;

            std::this_thread::sleep_for(30ms);
            iocontext.stop();
        });

        // Should block untill event is triggered
        watchdog.join();

        CPPUNIT_ASSERT_EQUAL(1L, nbTimesCalled);
    }

	void testGetTimeout() {
        EventLoop iocontext;
        int nbTimesCalled = 0;

        const int64 timeoutTime = 15;
        Timer timer(iocontext, boost::posix_time::millisec(timeoutTime));
        timer.asyncWait().then([&nbTimesCalled, &iocontext](int64 numberOfExpirations) {
            nbTimesCalled += numberOfExpirations;
        });

        const auto initTimeout = timer.getTimeout();
        CPPUNIT_ASSERT((timeoutTime - initTimeout.total_milliseconds()) < 3);

        iocontext.run();

        const auto timeout = timer.getTimeout();
        CPPUNIT_ASSERT_EQUAL(timeoutTime, timeoutTime - timeout.total_milliseconds());

        std::thread watchdog([&iocontext]() {

            std::this_thread::sleep_for(300ms);
            iocontext.stop();
        });

        // Should block untill event is triggered
        watchdog.join();

        CPPUNIT_ASSERT_EQUAL(1, nbTimesCalled);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestAsyncTimer);

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
 * @file: test/async/test_signalSet.cpp
 * @author: soultaker
 *
 * Created on: 10/10/2016
 *******************************************************************************/
#include <cadence/async/signalSet.hpp>  // Class being tested

#include <cppunit/extensions/HelperMacros.h>

#include <thread>
#include <chrono>

#include <signal.h>

using namespace Solace;
using namespace cadence::async;

class TestAsyncSignalSet: public CppUnit::TestFixture {

	CPPUNIT_TEST_SUITE(TestAsyncSignalSet);
        CPPUNIT_TEST(testSubscription);
        CPPUNIT_TEST(testSubscription2);
        CPPUNIT_TEST(testSubscriptionNonLeakage);
	CPPUNIT_TEST_SUITE_END();

protected:
public:

	void testSubscription() {
        EventLoop iocontext;
        SignalSet signalSet(iocontext, { SIGUSR1 });

        bool eventWasCalled = false;

        raise(SIGUSR1);

        signalSet.asyncWait().then([&eventWasCalled, &iocontext](int signalId) {

            CPPUNIT_ASSERT_EQUAL(signalId, SIGUSR1);

            eventWasCalled = true;
        });

        iocontext.run();

        std::thread t([&iocontext]() {
            using namespace std::chrono_literals;

            std::this_thread::sleep_for(std::chrono::seconds(1));

            iocontext.getIOService().stop();

        });

        // Should block untill event is triggered
        t.join();
    }

	void testSubscription2() {
        EventLoop iocontext;
        SignalSet signalSet(iocontext, { SIGUSR1, SIGUSR2 });

        bool eventWasCalled = false;

        raise(SIGUSR1);
        raise(SIGUSR2);

        signalSet.asyncWait().then([&eventWasCalled](int signalId) {

            CPPUNIT_ASSERT((signalId == SIGUSR1) || (signalId == SIGUSR2));

            eventWasCalled = true;
        });

        iocontext.run();

        std::thread t([&iocontext]() {
            using namespace std::chrono_literals;

            std::this_thread::sleep_for(200ms);
            iocontext.getIOService().stop();
        });

        // Should block untill event is triggered
        t.join();
    }

    void testSubscriptionNonLeakage() {
        EventLoop iocontext;
        SignalSet signalSet1(iocontext, { SIGUSR1 });
        SignalSet signalSet2(iocontext, { SIGUSR2 });

        bool event1_wasCalled = false;
        bool event2_wasCalled = false;

        CPPUNIT_ASSERT(!event1_wasCalled);
        CPPUNIT_ASSERT(!event2_wasCalled);

        raise(SIGUSR1);
        raise(SIGUSR2);

        signalSet1.asyncWait().then([&event1_wasCalled](int signalId) {
            CPPUNIT_ASSERT_EQUAL(SIGUSR1, signalId);

            event1_wasCalled = true;
        });

        signalSet2.asyncWait().then([&event2_wasCalled](int signalId) {
            CPPUNIT_ASSERT_EQUAL(SIGUSR2, signalId);

            event2_wasCalled = true;
        });

        iocontext.run();

        std::thread t([&iocontext]() {
            using namespace std::chrono_literals;

            std::this_thread::sleep_for(200ms);
            iocontext.getIOService().stop();
        });

        // Should block untill event is triggered
        t.join();
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestAsyncSignalSet);

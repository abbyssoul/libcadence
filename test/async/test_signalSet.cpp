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

#include "gtest/gtest.h"

#include <thread>
#include <chrono>

#include <signal.h>

using namespace Solace;
using namespace cadence::async;
using namespace std::chrono_literals;


TEST(TestAsyncSignalSet, testSubscription) {
    EventLoop iocontext;
    SignalSet signalSet(iocontext, { SIGUSR1 });

    bool eventWasCalled = false;

    raise(SIGUSR1);

    signalSet.asyncWait().then([&eventWasCalled, &iocontext](int signalId) {

        eventWasCalled = (signalId == SIGUSR1);
    });

    std::thread t([&iocontext]() {
        std::this_thread::sleep_for(200ms);
        iocontext.stop();
    });

    iocontext.run();
    // Should block untill event is triggered
    t.join();


    ASSERT_TRUE(eventWasCalled);
}


TEST(TestAsyncSignalSet, testSubscription2) {
    EventLoop iocontext;
    SignalSet signalSet(iocontext, { SIGUSR1, SIGUSR2 });

    bool eventWasCalled = false;

    raise(SIGUSR1);
    raise(SIGUSR2);

    signalSet.asyncWait().then([&eventWasCalled](int signalId) {
        eventWasCalled = (signalId == SIGUSR1) || (signalId == SIGUSR2);
    });

    std::thread t([&iocontext]() {
        std::this_thread::sleep_for(200ms);
        iocontext.stop();
    });

    iocontext.run();
    // Should block untill event is triggered
    t.join();

    ASSERT_TRUE(eventWasCalled);
}


TEST(TestAsyncSignalSet, testSubscriptionNonLeakage) {
    EventLoop iocontext;
    SignalSet signalSet1(iocontext, { SIGUSR1 });
    SignalSet signalSet2(iocontext, { SIGUSR2 });

    bool event1_wasCalled = false;
    bool event2_wasCalled = false;

    raise(SIGUSR1);
    raise(SIGUSR2);

    signalSet1.asyncWait().then([&event1_wasCalled](int signalId) {
        event1_wasCalled = (SIGUSR1 == signalId);
    });

    signalSet2.asyncWait().then([&event2_wasCalled](int signalId) {
        event2_wasCalled = (SIGUSR2 == signalId);
    });

    std::thread t([&iocontext]() {
        std::this_thread::sleep_for(200ms);
        iocontext.stop();
    });

    iocontext.run();
    // Should block untill event is triggered
    t.join();

    ASSERT_TRUE(event1_wasCalled);
    ASSERT_TRUE(event2_wasCalled);
}

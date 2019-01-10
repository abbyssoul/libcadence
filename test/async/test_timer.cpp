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

#include <solace/output_utils.hpp>

#include "gtest/gtest.h"


#include <thread>
#include <chrono>

using namespace Solace;
using namespace cadence::async;
using namespace std::chrono_literals;


std::ostream& operator<< (std::ostream& ostr, std::chrono::milliseconds const& dt) {
    return ostr << dt.count() << "ms";
}


TEST(TestAsyncTimer, testTimeout) {
    EventLoop iocontext;
    int64 nbTimesCalled = 0;

    Timer timer(iocontext, std::chrono::milliseconds(15));

    // We managed to cancel the timer. Start new asynchronous wait.
    timer.asyncWait()
        .then([&nbTimesCalled](int64 numberOfExpirations) {
            nbTimesCalled += numberOfExpirations;
        });

    iocontext.run();
    std::thread watchdog([&iocontext]() {
        std::this_thread::sleep_for(30ms);

        iocontext.stop();
    });

    // Should block untill event is triggered
    watchdog.join();

    ASSERT_EQ(1L, nbTimesCalled);
}


TEST(TestAsyncTimer, testGetTimeout) {
    EventLoop iocontext;
    int nbTimesCalled = 0;

    auto const timeoutTimeMs = std::chrono::milliseconds(15);
    auto const timerCreated = std::chrono::steady_clock::now();
    Timer timer(iocontext, timeoutTimeMs);

    timer.asyncWait()
        .then([&nbTimesCalled](int64 nbOfExpirations) {
            nbTimesCalled += nbOfExpirations;
        });

    auto const dt = std::chrono::duration_cast<std::chrono::milliseconds>(timer.getTimeout() - timerCreated);
    ASSERT_LE(dt, timeoutTimeMs);

    iocontext.run();

//    const auto timeout = timer.getTimeout();
//    ASSERT_EQ(timeoutTime, timeoutTime - timeout.count());

    std::thread watchdog([&iocontext]() {
        std::this_thread::sleep_for(300ms);

        iocontext.stop();
    });

    // Should block untill event is triggered
    watchdog.join();

    ASSERT_EQ(1, nbTimesCalled);
}

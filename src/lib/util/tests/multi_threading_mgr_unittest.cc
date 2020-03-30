// Copyright (C) 2019-2020 Internet Systems Consortium, Inc. ("ISC")
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <config.h>

#include <exceptions/exceptions.h>
#include <util/multi_threading_mgr.h>

#include <gtest/gtest.h>

using namespace isc::util;
using namespace isc;

/// @brief Verifies that the default mode is false (MT disabled).
TEST(MultiThreadingMgrTest, defaultMode) {
    // MT should be disabled
    EXPECT_FALSE(MultiThreadingMgr::instance().getMode());
}

/// @brief Verifies that the mode setter works.
TEST(MultiThreadingMgrTest, setMode) {
    // enable MT
    EXPECT_NO_THROW(MultiThreadingMgr::instance().setMode(true));
    // MT should be enabled
    EXPECT_TRUE(MultiThreadingMgr::instance().getMode());
    // disable MT
    EXPECT_NO_THROW(MultiThreadingMgr::instance().setMode(false));
    // MT should be disabled
    EXPECT_FALSE(MultiThreadingMgr::instance().getMode());
}

/// @brief Verifies that the thread pool size setter works.
TEST(MultiThreadingMgrTest, pktThreadPoolSize) {
    // default thread count is 0
    EXPECT_EQ(MultiThreadingMgr::instance().getPktThreadPoolSize(), 0);
    // set thread count to 16
    EXPECT_NO_THROW(MultiThreadingMgr::instance().setPktThreadPoolSize(16));
    // thread count should be 16
    EXPECT_EQ(MultiThreadingMgr::instance().getPktThreadPoolSize(), 16);
    // set thread count to 0
    EXPECT_NO_THROW(MultiThreadingMgr::instance().setPktThreadPoolSize(0));
    // thread count should be 0
    EXPECT_EQ(MultiThreadingMgr::instance().getPktThreadPoolSize(), 0);
}

/// @brief Verifies that determining supported thread count works.
TEST(MultiThreadingMgrTest, supportedThreadCount) {
    // determining supported thread count should work
    EXPECT_NE(MultiThreadingMgr::supportedThreadCount(), 0);
}

/// @brief Verifies that accessing the thread pool works.
TEST(MultiThreadingMgrTest, pktThreadPool) {
    // get the thread pool
    EXPECT_NO_THROW(MultiThreadingMgr::instance().getPktThreadPool());
}

/// @brief Verifies that apply settings works.
TEST(MultiThreadingMgrTest, applyConfig) {
    // get the thread pool
    auto& thread_pool = MultiThreadingMgr::instance().getPktThreadPool();
    // MT should be disabled
    EXPECT_FALSE(MultiThreadingMgr::instance().getMode());
    // default thread count is 0
    EXPECT_EQ(MultiThreadingMgr::instance().getPktThreadPoolSize(), 0);
    // thread pool should be stopped
    EXPECT_EQ(thread_pool.size(), 0);
    // enable MT with 16 threads
    EXPECT_NO_THROW(MultiThreadingMgr::instance().apply(true, 16));
    // MT should be enabled
    EXPECT_TRUE(MultiThreadingMgr::instance().getMode());
    // thread count should be 16
    EXPECT_EQ(MultiThreadingMgr::instance().getPktThreadPoolSize(), 16);
    // thread pool should be started
    EXPECT_EQ(thread_pool.size(), 16);
    // disable MT
    EXPECT_NO_THROW(MultiThreadingMgr::instance().apply(false, 16));
    // MT should be disabled
    EXPECT_FALSE(MultiThreadingMgr::instance().getMode());
    // thread count should be 0
    EXPECT_EQ(MultiThreadingMgr::instance().getPktThreadPoolSize(), 0);
    // thread pool should be stopped
    EXPECT_EQ(thread_pool.size(), 0);
    // enable MT with auto scaling
    EXPECT_NO_THROW(MultiThreadingMgr::instance().apply(true, 0));
    // MT should be enabled
    EXPECT_TRUE(MultiThreadingMgr::instance().getMode());
    // thread count should be maximum
    EXPECT_EQ(MultiThreadingMgr::instance().getPktThreadPoolSize(), MultiThreadingMgr::supportedThreadCount());
    // thread pool should be started
    EXPECT_EQ(thread_pool.size(), MultiThreadingMgr::supportedThreadCount());
    // disable MT
    EXPECT_NO_THROW(MultiThreadingMgr::instance().apply(false, 0));
    // MT should be disabled
    EXPECT_FALSE(MultiThreadingMgr::instance().getMode());
    // thread count should be 0
    EXPECT_EQ(MultiThreadingMgr::instance().getPktThreadPoolSize(), 0);
    // thread pool should be stopped
    EXPECT_EQ(thread_pool.size(), 0);
}

/// @brief Verifies that the critical section flag works.
TEST(MultiThreadingMgrTest, criticalSectionFlag) {
    // get the thread pool
    auto& thread_pool = MultiThreadingMgr::instance().getPktThreadPool();
    // MT should be disabled
    EXPECT_FALSE(MultiThreadingMgr::instance().getMode());
    // critical section should be disabled
    EXPECT_FALSE(MultiThreadingMgr::instance().isInCriticalSection());
    // thread count should be 0
    EXPECT_EQ(MultiThreadingMgr::instance().getPktThreadPoolSize(), 0);
    // thread pool should be stopped
    EXPECT_EQ(thread_pool.size(), 0);
    // exit critical section
    EXPECT_THROW(MultiThreadingMgr::instance().exitCriticalSection(), InvalidOperation);
    // critical section should be disabled
    EXPECT_FALSE(MultiThreadingMgr::instance().isInCriticalSection());
    // enter critical section
    EXPECT_NO_THROW(MultiThreadingMgr::instance().enterCriticalSection());
    // critical section should be enabled
    EXPECT_TRUE(MultiThreadingMgr::instance().isInCriticalSection());
    // enable MT with 16 threads
    EXPECT_NO_THROW(MultiThreadingMgr::instance().apply(true, 16));
    // MT should be enabled
    EXPECT_TRUE(MultiThreadingMgr::instance().getMode());
    // thread count should be 16
    EXPECT_EQ(MultiThreadingMgr::instance().getPktThreadPoolSize(), 16);
    // thread pool should be stopped
    EXPECT_EQ(thread_pool.size(), 0);
    // exit critical section
    EXPECT_NO_THROW(MultiThreadingMgr::instance().exitCriticalSection());
    // critical section should be disabled
    EXPECT_FALSE(MultiThreadingMgr::instance().isInCriticalSection());
    // exit critical section
    EXPECT_THROW(MultiThreadingMgr::instance().exitCriticalSection(), InvalidOperation);
    // critical section should be disabled
    EXPECT_FALSE(MultiThreadingMgr::instance().isInCriticalSection());
    // disable MT
    EXPECT_NO_THROW(MultiThreadingMgr::instance().apply(false, 0));
    // MT should be disabled
    EXPECT_FALSE(MultiThreadingMgr::instance().getMode());
    // thread count should be 0
    EXPECT_EQ(MultiThreadingMgr::instance().getPktThreadPoolSize(), 0);
    // thread pool should be stopped
    EXPECT_EQ(thread_pool.size(), 0);
}

/// @brief Verifies that the critical section works.
TEST(MultiThreadingMgrTest, criticalSection) {
    // get the thread pool instance
    auto& thread_pool = MultiThreadingMgr::instance().getPktThreadPool();
    // thread pool should be stopped
    EXPECT_EQ(thread_pool.size(), 0);
    // apply multi-threading configuration with 16 threads
    MultiThreadingMgr::instance().apply(true, 16);
    // thread count should match
    EXPECT_EQ(thread_pool.size(), 16);
    // use scope to test constructor and destructor
    {
        MultiThreadingCriticalSection cs;
        // thread pool should be stopped
        EXPECT_EQ(thread_pool.size(), 0);
        // use scope to test constructor and destructor
        {
            MultiThreadingCriticalSection inner_cs;
            // thread pool should be stopped
            EXPECT_EQ(thread_pool.size(), 0);
        }
        // thread pool should be stopped
        EXPECT_EQ(thread_pool.size(), 0);
    }
    // thread count should match
    EXPECT_EQ(thread_pool.size(), 16);
    // use scope to test constructor and destructor
    {
        MultiThreadingCriticalSection cs;
        // thread pool should be stopped
        EXPECT_EQ(thread_pool.size(), 0);
        // apply multi-threading configuration with 64 threads
        MultiThreadingMgr::instance().apply(true, 64);
        // thread pool should be stopped
        EXPECT_EQ(thread_pool.size(), 0);
    }
    // thread count should match
    EXPECT_EQ(thread_pool.size(), 64);
    // use scope to test constructor and destructor
    {
        MultiThreadingCriticalSection cs;
        // thread pool should be stopped
        EXPECT_EQ(thread_pool.size(), 0);
        // apply multi-threading configuration with 0 threads
        MultiThreadingMgr::instance().apply(false, 64);
        // thread pool should be stopped
        EXPECT_EQ(thread_pool.size(), 0);
    }
    // thread count should match
    EXPECT_EQ(thread_pool.size(), 0);
    // use scope to test constructor and destructor
    {
        MultiThreadingCriticalSection cs;
        // thread pool should be stopped
        EXPECT_EQ(thread_pool.size(), 0);
        // use scope to test constructor and destructor
        {
            MultiThreadingCriticalSection inner_cs;
            // thread pool should be stopped
            EXPECT_EQ(thread_pool.size(), 0);
        }
        // thread pool should be stopped
        EXPECT_EQ(thread_pool.size(), 0);
    }
    // thread count should match
    EXPECT_EQ(thread_pool.size(), 0);
    // use scope to test constructor and destructor
    {
        MultiThreadingCriticalSection cs;
        // thread pool should be stopped
        EXPECT_EQ(thread_pool.size(), 0);
        // apply multi-threading configuration with 64 threads
        MultiThreadingMgr::instance().apply(true, 64);
        // thread pool should be stopped
        EXPECT_EQ(thread_pool.size(), 0);
    }
    // thread count should match
    EXPECT_EQ(thread_pool.size(), 64);
    // apply multi-threading configuration with 0 threads
    MultiThreadingMgr::instance().apply(false, 0);
}
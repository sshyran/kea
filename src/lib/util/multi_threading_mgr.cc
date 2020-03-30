// Copyright (C) 2019-2020 Internet Systems Consortium, Inc. ("ISC")
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <util/multi_threading_mgr.h>

namespace isc {
namespace util {

MultiThreadingMgr::MultiThreadingMgr()
    : enabled_(false), critical_section_count_(0) {
}

MultiThreadingMgr::~MultiThreadingMgr() {
}

MultiThreadingMgr&
MultiThreadingMgr::instance() {
    static MultiThreadingMgr manager;
    return (manager);
}

bool
MultiThreadingMgr::getMode() const {
    return (enabled_);
}

void
MultiThreadingMgr::setMode(bool enabled) {
    enabled_ = enabled;
}

void
MultiThreadingMgr::enterCriticalSection() {
    stopPktProcessing();
    ++critical_section_count_;
}

void
MultiThreadingMgr::exitCriticalSection() {
    if (critical_section_count_ == 0) {
        isc_throw(InvalidOperation, "invalid negative value for override");
    }
    --critical_section_count_;
    startPktProcessing();
}

bool
MultiThreadingMgr::isInCriticalSection() {
    return (critical_section_count_ != 0);
}

ThreadPool<std::function<void()>>&
MultiThreadingMgr::getPktThreadPool() {
    return pkt_thread_pool_;
}

uint32_t
MultiThreadingMgr::getPktThreadPoolSize() const {
    return (pkt_thread_pool_size_);
}

void
MultiThreadingMgr::setPktThreadPoolSize(uint32_t size) {
    pkt_thread_pool_size_ = size;
}

uint32_t
MultiThreadingMgr::supportedThreadCount() {
    return (std::thread::hardware_concurrency());
}

void
MultiThreadingMgr::apply(bool enabled, uint32_t thread_count) {
    // check the enabled flag
    if (enabled) {
        // check for auto scaling (enabled flag true but thread_count 0)
        if (!thread_count) {
            // might also return 0
            thread_count = MultiThreadingMgr::supportedThreadCount();
        }
    } else {
        thread_count = 0;
    }
    // check enabled flag and explicit number of threads or system supports
    // hardware concurrency
    if (thread_count) {
        if (pkt_thread_pool_.size()) {
            pkt_thread_pool_.stop();
        }
        setPktThreadPoolSize(thread_count);
        setMode(true);
        if (!isInCriticalSection()) {
            pkt_thread_pool_.start(thread_count);
        }
    } else {
        pkt_thread_pool_.reset();
        setMode(false);
        setPktThreadPoolSize(thread_count);
    }
}

void
MultiThreadingMgr::stopPktProcessing() {
    if (getMode() && getPktThreadPoolSize() && !isInCriticalSection()) {
        pkt_thread_pool_.stop();
    }
}

void
MultiThreadingMgr::startPktProcessing() {
    if (getMode() && getPktThreadPoolSize() && !isInCriticalSection()) {
        pkt_thread_pool_.start(getPktThreadPoolSize());
    }
}

MultiThreadingCriticalSection::MultiThreadingCriticalSection() {
    MultiThreadingMgr::instance().enterCriticalSection();
}

MultiThreadingCriticalSection::~MultiThreadingCriticalSection() {
    MultiThreadingMgr::instance().exitCriticalSection();
}

}  // namespace util
}  // namespace isc
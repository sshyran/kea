// Copyright (C) 2016-2019  Internet Systems Consortium, Inc. ("ISC")
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <config.h>

#ifdef ENABLE_AFL

#ifndef __AFL_LOOP
#error To use American Fuzzy Lop you have to set CXX to afl-clang-fast++
#endif

#include <dhcp/dhcp6.h>
#include <dhcpsrv/fuzz.h>
#include <dhcpsrv/fuzz_log.h>

#include <boost/lexical_cast.hpp>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>

using namespace isc;
using namespace isc::dhcp;
using namespace std;

// Constants defined in the Fuzz class definition.
constexpr size_t        Fuzz::BUFFER_SIZE;
constexpr size_t        Fuzz::MAX_SEND_SIZE;
constexpr long          Fuzz::MAX_LOOP_COUNT;

// Constructor
Fuzz::Fuzz(int ipversion, uint16_t port) :
    address_(nullptr), interface_(nullptr), loop_max_(MAX_LOOP_COUNT),
    port_(port), sockaddr_len_(0), sockaddr_ptr_(nullptr), sockfd_(-1) {

    try {
        stringstream reason;    // Used to construct exception messages

        // Set up address structures.
        setAddress(ipversion);

        // Create the socket through which packets read from stdin will be sent
        // to the port on which Kea is listening.  This is closed in the
        // destructor.
        sockfd_ = socket((ipversion == 4) ? AF_INET : AF_INET6, SOCK_DGRAM, 0);
        if (sockfd_ < 0) {
            LOG_FATAL(fuzz_logger, FUZZ_SOCKET_CREATE_FAIL)
                      .arg(strerror(errno));
            return;
        }

        // Check if the hard-coded maximum loop count is being overridden
        const char *loop_max_ptr = getenv("KEA_AFL_LOOP_MAX");
        if (loop_max_ptr != 0) {
            try {
                loop_max_ = boost::lexical_cast<long>(loop_max_ptr);
            } catch (const boost::bad_lexical_cast&) {
                reason << "cannot convert port number specification "
                       << loop_max_ptr << " to an integer";
                isc_throw(FuzzInitFail, reason.str());
            }

            if (loop_max_ <= 0) {
                reason << "KEA_AFL_LOOP_MAX is " << loop_max_ << ". "
                       << "It must be an integer greater than zero.";
                isc_throw(FuzzInitFail, reason.str());
            }
        }

    } catch (const FuzzInitFail& e) {
        // AFL tends to make it difficult to find out what exactly has failed:
        // make sure that the error is logged.
        LOG_FATAL(fuzz_logger, FUZZ_INIT_FAIL).arg(e.what());
        throw;
    }

    LOG_INFO(fuzz_logger, FUZZ_INIT_COMPLETE).arg(interface_).arg(address_)
             .arg(port_).arg(loop_max_);
}

// Destructor
Fuzz::~Fuzz() {
    static_cast<void>(close(sockfd_));
}

// Parse IP address/port/interface and set up address structures.
void
Fuzz::setAddress(int ipversion) {
    stringstream reason;    // Used in error messages

    // Get the environment for the fuzzing: interface, address and port.
    interface_ = getenv("KEA_AFL_INTERFACE");
    if (! interface_) {
        isc_throw(FuzzInitFail, "no fuzzing interface has been set");
    }

    // Now the address. (The port is specified via the "-p" command-line
    // switch and passed to this object through the constructor.)
    address_ = getenv("KEA_AFL_ADDRESS");
    if (address_ == 0) {
        isc_throw(FuzzInitFail, "no fuzzing address has been set");
    }

    // Set up the appropriate data structure depending on the address given.
    if ((strstr(address_, ":") != NULL) && (ipversion == 6)) {
        // Expecting IPv6 and the address contains a colon, so assume it is an
        // an IPv6 address.
        memset(&servaddr6_, 0, sizeof (servaddr6_));

        servaddr6_.sin6_family = AF_INET6;
        if (inet_pton(AF_INET6, address_, &servaddr6_.sin6_addr) != 1) {
            reason << "inet_pton() failed: can't convert "
                   << address_ << " to an IPv6 address" << endl;
            isc_throw(FuzzInitFail, reason.str());
        }
        servaddr6_.sin6_port = htons(port_);

        // Interface ID is needed for IPv6 address structures.
        servaddr6_.sin6_scope_id = if_nametoindex(interface_);
        if (servaddr6_.sin6_scope_id == 0) {
            reason << "error retrieving interface ID for "
                   << interface_ << ": " << strerror(errno);
            isc_throw(FuzzInitFail, reason.str());
        }

        sockaddr_ptr_ = reinterpret_cast<sockaddr*>(&servaddr6_);
        sockaddr_len_ = sizeof(servaddr6_);

    } else if ((strstr(address_, ".") != NULL) && (ipversion == 4)) {
        // Expecting an IPv4 address and it contains a dot, so assume it is.
        // This check is done after the IPv6 check, as it is possible for an
        // IPv4 address to be emnbedded in an IPv6 one.
        memset(&servaddr4_, 0, sizeof(servaddr4_));

        servaddr4_.sin_family = AF_INET;
        if (inet_pton(AF_INET, address_, &servaddr4_.sin_addr) != 1) {
            reason << "inet_pton() failed: can't convert "
                   << address_ << " to an IPv6 address" << endl;
            isc_throw(FuzzInitFail, reason.str());
        }
        servaddr4_.sin_port = htons(port_);

        sockaddr_ptr_ = reinterpret_cast<sockaddr*>(&servaddr4_);
        sockaddr_len_ = sizeof(servaddr4_);

    } else {
        reason << "Expected IP version (" << ipversion << ") is not "
               << "4 or 6, or the given address " << address_ << " does not "
               << "match the IP version expected";
        isc_throw(FuzzInitFail, reason.str());
    }

}


// This is the main fuzzing function. It receives data from fuzzing engine over
// stdin and then sends it to the configured UDP socket.
void
Fuzz::transfer(void) {

    // Read from stdin.  Just return if nothing is read (or there is an error)
    // and hope that this does not cause a hang.
    char buf[BUFFER_SIZE];
    ssize_t length = read(0, buf, sizeof(buf));

    // Save the errno in case there was an error because if debugging is
    // enabled, the following LOG_DEBUG call may destroy its value.
    int errnum = errno;
    LOG_DEBUG(fuzz_logger, FUZZ_DBG_TRACE_DETAIL, FUZZ_DATA_READ)
              .arg(length);

    if (length > 0) {
        // Now send the data to the UDP port on which Kea is listening.
        // Send the data to the main Kea thread.  Limit the size of the
        // packets that can be sent.
        size_t send_len = (length < MAX_SEND_SIZE) ? length : MAX_SEND_SIZE;
        ssize_t sent = sendto(sockfd_, buf, send_len, 0, sockaddr_ptr_,
                              sockaddr_len_);
        if (sent > 0) {
            LOG_DEBUG(fuzz_logger, FUZZ_DBG_TRACE_DETAIL, FUZZ_SEND).arg(sent);
        } else if (sent != length) {
            LOG_WARN(fuzz_logger, FUZZ_SHORT_SEND).arg(length).arg(sent);
        } else {
            LOG_ERROR(fuzz_logger, FUZZ_SEND_ERROR).arg(strerror(errno));
        }
    } else {
        // Read did not get any bytes.  A zero-length read (EOF) may have been
        // generated by AFL, so don't log that.  But otherwise log an error.
        if (length != 0) {
            LOG_ERROR(fuzz_logger, FUZZ_READ_FAIL).arg(strerror(errnum));
        }
    }

}

#endif  // ENABLE_AFL

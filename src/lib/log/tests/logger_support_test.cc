// Copyright (C) 2011  Internet Systems Consortium, Inc. ("ISC")
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

/// \brief Example Program
///
/// Simple example program showing how to use the logger.

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <iostream>

#include <log/logger.h>
#include <log/macros.h>
#include <log/logger_support.h>
#include <log/root_logger_name.h>

// Include a set of message definitions.
#include <log/messagedef.h>

using namespace isc::log;

// Declare logger to use an example.
Logger logger_ex("example");

// The program is invoked:
//
// logger_support_test [-s severity] [-d level ] [local_file]
//
// "severity" is one of "debug", "info", "warn", "error", "fatal"
// "level" is the debug level, a number between 0 and 99
// "local_file" is the name of a local file.
//
// The program sets the attributes on the root logger and logs a set of
// messages.  Looking at the output determines whether the program worked.

int main(int argc, char** argv) {

    isc::log::Severity  severity = isc::log::INFO;  // Default logger severity
    int                 dbglevel = -1;              // Logger debug level
    const char*         localfile = NULL;           // Local message file
    int                 option;                     // For getopt() processing
    Logger              logger_dlm("dlm", true);    // Another example logger

    // Parse options
    while ((option = getopt(argc, argv, "s:d:")) != -1) {
        switch (option) {
            case 's':
                if (strcmp(optarg, "debug") == 0) {
                    severity = isc::log::DEBUG;
                } else if (strcmp(optarg, "info") == 0) {
                    severity = isc::log::INFO;
                } else if (strcmp(optarg, "warn") == 0) {
                    severity = isc::log::WARN;
                } else if (strcmp(optarg, "error") == 0) {
                    severity = isc::log::ERROR;
                } else if (strcmp(optarg, "fatal") == 0) {
                    severity = isc::log::FATAL;
                } else {
                    std::cout << "Unrecognised severity option: " <<
                        optarg << "\n";
                    exit(1);
                }
                break;

            case 'd':
                dbglevel = atoi(optarg);
                break;

            default:
                std::cout << "Unrecognised option: " <<
                    static_cast<char>(option) << "\n";
        }
    }

    if (optind < argc) {
        localfile = argv[optind];
    }

    // Update the logging parameters
    initLogger("alpha", severity, dbglevel, localfile);

    // Log a few messages
    LOG_FATAL(logger_ex, MSG_WRITERR).arg("test1").arg("42");
    LOG_ERROR(logger_ex, MSG_RDLOCMES).arg("dummy/file");
    LOG_WARN(logger_dlm, MSG_READERR).arg("a.txt").arg("dummy reason");
    LOG_INFO(logger_dlm, MSG_OPENIN).arg("example.msg").arg("dummy reason");
    LOG_DEBUG(logger_ex, 0, MSG_RDLOCMES).arg("dummy/0");
    LOG_DEBUG(logger_ex, 24, MSG_RDLOCMES).arg("dummy/24");
    LOG_DEBUG(logger_ex, 25, MSG_RDLOCMES).arg("dummy/25");
    LOG_DEBUG(logger_ex, 26, MSG_RDLOCMES).arg("dummy/26");

    return (0);
}

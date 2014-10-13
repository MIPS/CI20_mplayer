/*
 * Copyright (C) 2006-2014 Ingenic Semiconductor CO.,LTD.
 *
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

//
// C/C++ logging functions.  See the logging documentation for API details.
//
// We'd like these to be available from C code (in case we import some from
// somewhere), so this has a C interface.
//
// The output will be correct when the log file is shared between multiple
// threads and/or multiple processes so long as the operating system
// supports O_APPEND.  These calls have mutex-protected data structures
// and so are NOT reentrant.  Do not use LOG in a signal handler.
//
#ifndef _LIBS_UTILS_LOG_H
#define _LIBS_UTILS_LOG_H

#include "Log.h"
#include <sys/types.h>

#ifdef __cplusplus

namespace android {

/*
 * A very simple utility that yells in the log when an operation takes too long.
 */
class LogIfSlow {
public:
    LogIfSlow(const char* tag, android_LogPriority priority,
            int timeoutMillis, const char* message);
    ~LogIfSlow();

private:
    const char* const mTag;
    const android_LogPriority mPriority;
    const int mTimeoutMillis;
    const char* const mMessage;
    const int64_t mStart;
};

/*
 * Writes the specified debug log message if this block takes longer than the
 * specified number of milliseconds to run.  Includes the time actually taken.
 *
 * {
 *     ALOGD_IF_SLOW(50, "Excessive delay doing something.");
 *     doSomething();
 * }
 */
#define ALOGD_IF_SLOW(timeoutMillis, message) \
        LogIfSlow _logIfSlow(LOG_TAG, ANDROID_LOG_DEBUG, timeoutMillis, message);

} // namespace android

#endif // __cplusplus

#endif // _LIBS_UTILS_LOG_H

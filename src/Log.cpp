#include "Log.h"

#include <cstdarg>
#include <cstdio>

#include "XPLMUtilities.h"

void LogMsg(const char* fmt, ...)
{
    char body[1024];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(body, sizeof(body), fmt, args);
    va_end(args);

    char line[1100];
    std::snprintf(line, sizeof(line), "RealRain: %s\n", body);
    XPLMDebugString(line);
}

#pragma once

#include <Windows.h>
#include <vector>
#include "types.h"


namespace CurrentThread {
    extern __thread int t_cachedTid;
    extern __thread char t_tidString[32];
    extern __thread int t_tidStringLength;
    extern __thread char t_threadName[256];

#ifdef WIN32
    typedef struct tagTHREADNAME_INFO {
        DWORD dwType; // must be 0x1000
        LPCSTR szName; // pointer to name (in user addr space)
        DWORD dwThreadID; // thread ID (-1=caller thread)
        DWORD dwFlags; // reserved for future use, must be zero
    } THREADNAME_INFO;

    void setThreadName(DWORD dwThreadID, LPCSTR szThreadName);
#endif

    void cacheTid();

    inline int tid() {
        __assume(t_cachedTid == 0);
        if (t_cachedTid == 0) {
            cacheTid();
        }

        return t_cachedTid;
    }

    inline const char* tidString() {
        return t_tidString;
    }

    inline int tidStringLength() {
        return t_tidStringLength;
    }

    inline const char* name() {
        return t_threadName;
    }

    inline int setName(const char* name) {
        return strcpy_s(t_threadName, sizeof(t_threadName), name);
    }

    struct StackFrame {
        DWORD64 address;
        string name;
        string module;
        unsigned int line;
        string file;
    };

    std::vector<StackFrame> stacktrace();

    string convertStackFrame(std::vector<StackFrame>&& frames);
}
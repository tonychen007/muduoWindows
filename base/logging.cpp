#include "logging.h"
#include <assert.h>

__thread char t_errnobuf[512];
__thread char t_time[64];
__thread time_t t_lastSecond;

const char* strerror_tl(int savedErrno) {
	strerror_s(t_errnobuf, savedErrno);
	return t_errnobuf;
}

Logger::LogLevel initLogLevel() {
    char* buf;
    size_t sz;
    if (!_dupenv_s(&buf, &sz, "MUDUO_LOG_TRACE"))
        return Logger::ETRACE;
    else if (!_dupenv_s(&buf, &sz, "MUDUO_LOG_DEBUG"))
        return Logger::EDEBUG;
    else
        return Logger::EINFO;
}

Logger::LogLevel g_logLevel = initLogLevel();

const char* LogLevelName[Logger::NUM_LOG_LEVELS] = {
  "TRACE ",
  "DEBUG ",
  "INFO  ",
  "WARN  ",
  "ERROR ",
  "FATAL ",
};

// helper class for known string length at compile time
class T {
public:
    T(const char* str, unsigned len)
        :str_(str),
        len_(len) {
        assert(strlen(str) == len_);
    }

    const char* str_;
    const unsigned len_;
};

inline LogStream& operator<<(LogStream& s, T v) {
    s.append(v.str_, v.len_);
    return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v) {
    s.append(v.data_, v.size_);
    return s;
}
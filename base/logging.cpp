#include <assert.h>
#include "logging.h"
#include "currentThread.h"
#include "timezone.h"


__thread char t_errnobuf[512];
__thread char t_time[64];
__thread time_t t_lastSecond;

const char* strerror_tl(int savedErrno) {
	strerror_s(t_errnobuf, savedErrno);
	return t_errnobuf;
}

inline int hasEnv(const char* var) {
    char* buf;
    size_t sz;

    _dupenv_s(&buf, &sz, var);
    if (buf)
        if (strcmp(buf, "1") == 0)
            return 1;

    return 0;
}

Logger::LogLevel initLogLevel() {
    char* buf;
    size_t sz;

    if (hasEnv("MUDUO_LOG_TRACE"))
        return Logger::ETRACE;
    else if (hasEnv("MUDUO_LOG_DEBUG"))
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

void defaultOutput(const char* msg, int len) {
    size_t n = fwrite(msg, 1, len, stdout);
}

void defaultFlush() {
    fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

void Logger::setLogLevel(Logger::LogLevel level) {
    g_logLevel = level;
}

void Logger::setOutput(OutputFunc out) {
    g_output = out;
}

void Logger::setFlush(FlushFunc flush) {
    g_flush = flush;
}

Logger::Logger(SourceFile file, int line)
    : impl_(EINFO, 0, file, line) {
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
    : impl_(level, 0, file, line) {
    impl_.stream_ << func << ' ';
}

Logger::Logger(SourceFile file, int line, LogLevel level)
    : impl_(level, 0, file, line) {
}

Logger::Logger(SourceFile file, int line, bool toAbort)
    : impl_(toAbort ? EFATAL : EERROR, errno, file, line) {
}

Logger::~Logger() {
    impl_.finish();
    const LogStream::Buffer& buf(stream().buffer());
    g_output(buf.data(), buf.length());

    if (impl_.level_ == EFATAL) {
        g_flush();
        abort();
    }
}

Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
    : time_(Timestamp::now()),
    stream_(),
    level_(level),
    line_(line),
    basename_(file) {

    formatTime();
    CurrentThread::tid();
    stream_ << T(CurrentThread::tidString(), CurrentThread::tidStringLength());
    stream_ << " ";
    stream_ << T(LogLevelName[level], 6);
    if (savedErrno != 0) {
        stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

void Logger::Impl::formatTime() {
    int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(microSecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond);
    
    if (seconds != t_lastSecond) {
        t_lastSecond = seconds;
        struct DateTime dt;

        dt = TimeZone::toUtcTime(seconds);
        int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
            dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
    }

    Fmt us(".%06dZ ", microseconds);
    assert(us.length() == 9);
    stream_ << T(t_time, 17) << T(us.data(), 9);
}

void Logger::Impl::finish() {
    stream_ << " - " << basename_ << ':' << line_ << '\n';
}


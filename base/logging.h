#pragma once

#include "logStream.h"
#include "timestamp.h"
#include "types.h"

class Logger {
public:
    enum LogLevel {
        ETRACE,
        EDEBUG,
        EINFO,
        EWARN,
        EERROR,
        EFATAL,
        NUM_LOG_LEVELS,
    };
    
    class SourceFile {
    public:
        template<int N>
        SourceFile(const char(&arr)[N])
            : data_(arr),
            size_(N - 1) {
            
            const char* slash = strrchr(data_, '/'); // builtin function
            if (slash) {
                data_ = slash + 1;
                size_ -= static_cast<int>(data_ - arr);
            }
        }

        explicit SourceFile(const char* filename)
            : data_(filename) {
            const char* slash = strrchr(filename, '/');
            if (slash) {
                data_ = slash + 1;
            }
            size_ = static_cast<int>(strlen(data_));
        }

        const char* data_;
        int size_;
    };

    
    Logger(SourceFile file, int line);
    Logger(SourceFile file, int line, LogLevel level);
    Logger(SourceFile file, int line, LogLevel level, const char* func);
    Logger(SourceFile file, int line, bool toAbort);
    ~Logger();

    LogStream& stream() { return impl_.stream_; }

    static LogLevel logLevel();
    static void setLogLevel(LogLevel level);

    typedef void (*OutputFunc)(const char* msg, int len);
    typedef void (*FlushFunc)();

    static void setOutput(OutputFunc);
    static void setFlush(FlushFunc);

private:
    class Impl {
    public:
        typedef Logger::LogLevel LogLevel;
        Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
        void formatTime();
        void finish();

        Timestamp time_;
        LogStream stream_;
        LogLevel level_;
        int line_;
        SourceFile basename_;
    };

    Impl impl_;
};

extern Logger::LogLevel g_logLevel;

Logger::LogLevel initLogLevel();

inline Logger::LogLevel Logger::logLevel() {
    return g_logLevel;
}

//
// CAUTION: do not write:
//
// if (good)
//   LOG_INFO << "Good news";
// else
//   LOG_WARN << "Bad news";
//
// this expends to
//
// if (good)
//   if (logging_INFO)
//     logInfoStream << "Good news";
//   else
//     logWarnStream << "Bad news";
//
#define LOG_TRACE if (Logger::logLevel() <= Logger::ETRACE) \
  Logger(__FILE__, __LINE__, Logger::ETRACE, __func__).stream()
#define LOG_DEBUG if (Logger::logLevel() <= Logger::EDEBUG) \
  Logger(__FILE__, __LINE__, Logger::EDEBUG, __func__).stream()
#define LOG_INFO if (Logger::logLevel() <= Logger::EINFO) \
  Logger(__FILE__, __LINE__).stream()
#define LOG_WARN Logger(__FILE__, __LINE__, Logger::EWARN).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::EERROR).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::EFATAL).stream()
#define LOG_SYSERR Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL Logger(__FILE__, __LINE__, true).stream()

const char* strerror_tl(int savedErrno);

#define CHECK_NOTNULL(val) \
  CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

template <typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char* names, T* ptr) {
    if (ptr == NULL) {
        Logger(file, line, Logger::EFATAL).stream() << names;
    }
    return ptr;
}

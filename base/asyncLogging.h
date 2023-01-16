// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#pragma once


#include "blockingQueue.h"
#include "boundedBlockingQueue.h"
#include "countDownLatch.h"
#include "MutexGuard.h"
#include "thread.h"
#include "logStream.h"

#include <atomic>
#include <vector>

class LogFile;

class AsyncLogging : noncopyable {
public:
    AsyncLogging(const string& basename,
        off_t rollSize,
        int flushInterval = 3);

    ~AsyncLogging() {
        if (running_) {
            stop();
        }
    }

    void append(const char* logline, int len);

    void start() {
        running_ = true;
        thread_.start();
        latch_.wait();
    }

    void stop() {
        running_ = false;
        cond_.notify();
        thread_.join();
    }

    void deleteLogFile();
private:

    void threadFunc();

    typedef FixedBuffer<kLargeBuffer> Buffer;
    typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
    typedef BufferVector::value_type BufferPtr;

    const int flushInterval_;
    std::atomic<bool> running_;
    const string basename_;
    std::vector<std::unique_ptr<LogFile>> logFiles_;
    const off_t rollSize_;
    Thread thread_;
    CountDownLatch latch_;
    MutexLock mutex_;
    Condition cond_;
    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
};

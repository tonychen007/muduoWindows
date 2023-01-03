#pragma once

#include "mutexGuard.h"
#include "currentThread.h"
#include "condition.h"
#include "thread.h"
#include "threadpool.h"
#include "exception.h"
#include "countDownLatch.h"
#include "timestamp.h"
#include "blockingQueue.h"
#include "boundedBlockingQueue.h"
#include "atomic.h"
#include "singleton.h"
#include "weakCallback.h"
#include "date.h"

#include <sstream>
#include <vector>
#include <thread>
#include <memory>
#include <map>
#include <chrono>

using namespace std;
using namespace chrono;

using uniqueLock = std::unique_lock<std::mutex>;

void testMutex(int kThread);
void testMutexHelper();
void testCurrentThread();
void testCondition();
void testThread();
void testThreadException();
void testThreadpool(int num);
void testThreadpoolHelper();
void testTimestamp();
void testBlockingQueue();
void testBlockingQueue2(int kThread);
void testBlockingQueue2Helper();
void testBoundedBlockingQueue();
void testAtomic();
void testSingleton();
void testWeakCallback();
void testDate();
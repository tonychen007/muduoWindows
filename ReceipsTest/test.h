#pragma once

#include "mutexGuard.h"
#include "currentThread.h"
#include "condition.h"
#include "thread.h"
#include "threadpool.h"
#include "exception.h"
#include "countDownLatch.h"
#include "timestamp.h"

#include <sstream>
#include <vector>
#include <thread>
#include <memory>
#include <chrono>

using namespace std;
using namespace chrono;

void testMutex(int kThread);
void testMutexHelper();
void testCurrentThread();
void testCondition();
void testThread();
void testThreadException();
void testThreadpool(int num);
void testThreadpoolHelper();
void testTimestamp();
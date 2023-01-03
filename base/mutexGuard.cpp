#include "mutexGuard.h"
#include "currentThread.h"

void MutexLock::lock() {
	mutex_.lock();
	assignedHolder();
}

void MutexLock::unlock() {
	mutex_.unlock();
	unassignedHolder();
}

int MutexLock::getCurrentThreadId() {
	return CurrentThread::tid();
}
#pragma once

#include <assert.h>
#include <mutex>
#include <thread>
#include "noncopyable.h"

class MutexLock:noncopyable {
public:
	MutexLock():pid_(0) {}

	~MutexLock() {
		assert(pid_ == 0);
	}

	std::mutex* getMutex() {
		return &mutex_;
	}

	void lock();
	void unlock();

	void assignedHolder() {
		std::thread::id id = std::this_thread::get_id();
		pid_ = *reinterpret_cast<int*>(&id);
	}

	void unassignedHolder() { pid_ = 0; }

	bool isLockedByCurrentThread() {
		return pid_ == getCurrentThreadId();
	}

private:
	int getCurrentThreadId();

private:
	std::mutex mutex_;
	int pid_;
};

class MutexLockGuard:noncopyable {
public:
	explicit MutexLockGuard(MutexLock& mutex) :mutex_(mutex) {
		mutex.lock();
	}

	~MutexLockGuard() {
		mutex_.unlock();
	}

private:
	MutexLock& mutex_;
};
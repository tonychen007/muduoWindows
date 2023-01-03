#pragma once

#include "mutexGuard.h"
#include <memory>
#include <condition_variable>
#include <chrono>

class Condition :noncopyable {
public:
	explicit Condition(MutexLock& mutex) :mutex_(mutex) {}

	~Condition() {}

	void wait(std::unique_lock<std::mutex>& lock) {
		cond_.wait(lock);
	}

	void waitfor(std::unique_lock<std::mutex>& lock, int mills) {
		cond_.wait_for(lock, std::chrono::milliseconds(mills));
	}

	void notify() {
		cond_.notify_one();
	}

	void notifyAll() {
		cond_.notify_all();
	}

	std::unique_lock<std::mutex> getUniqueLock() {
		return std::unique_lock<std::mutex>(*mutex_.getMutex());
	}

private:
	MutexLock& mutex_;
	std::condition_variable cond_;
};
#pragma once

#include "noncopyable.h"
#include "condition.h"
#include "mutexGuard.h"
#include "types.h"
#include <deque>

template<class T>
class BlockingQueue :noncopyable {
public:
	BlockingQueue(): mutex_(), notEmpty_(mutex_), queue_() {}

	void put(const T& t) {
		MutexLockGuard lock(mutex_);
		queue_.push_back(t);
		notEmpty_.notify();
	}

	void put(T&& t) {
		MutexLockGuard lock(mutex_);
		queue_.push_back(std::move(t));
		notEmpty_.notify();
	}

	T take() {
		uniqueLock lock = notEmpty_.getUniqueLock();
		while (queue_.empty())
			notEmpty_.wait(lock);

		T t(std::move(queue_.front()));
		queue_.pop_front();
		return t;
	}

	std::deque<T> drain() {
		std::deque<T> queue;
		{
			MutexLockGuard lock(mutex_);
			queue = std::move(queue_);
		}
		return queue;
	}

	size_t size() const {
		MutexLockGuard lock(mutex_);
		return queue_.size();
	}

private:
	mutable MutexLock mutex_;
	Condition notEmpty_;
	std::deque<T> queue_;
};


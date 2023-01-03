#pragma once

#include "noncopyable.h"
#include "condition.h"
#include "mutexGuard.h"
#include "types.h"
#include <deque>

template<class T>
class BoundedBlockingQueue :noncopyable {
public:
	explicit BoundedBlockingQueue(int maxSize)
	  : mutex_(),
		notEmpty_(mutex_),
		notFull_(mutex_),
		maxSize_(maxSize) {}
	

	void put(const T& x) {
		uniqueLock lock = notFull_.getUniqueLock();
		while (full()) 
			notFull_.wait(lock);
		
		assert(!full());
		queue_.push_back(x);
		notEmpty_.notify();
	}

	void put(T&& x) {
		uniqueLock lock = notFull_.getUniqueLock();
		while (full())
			notFull_.wait(lock);
		
		assert(!full());
		queue_.push_back(std::move(x));
		notEmpty_.notify();
	}

	T take() {
		uniqueLock lock = notEmpty_.getUniqueLock();
		while (queue_.empty())
			notEmpty_.wait(lock);

		T t(std::move(queue_.front()));
		queue_.pop_front();
		notFull_.notify();
		return t;
	}

	bool empty() const {
		return queue_.empty();
	}

	bool full() const {
		return queue_.size() == maxSize_;
	}

	size_t size() const {
		MutexLockGuard lock(mutex_);
		return queue_.size();
	}

	size_t capacity() const {
		MutexLockGuard lock(mutex_);
		return maxSize_ - queue_.size();
	}

private:
	mutable MutexLock mutex_;
	Condition notEmpty_;
	Condition notFull_;
	std::deque<T> queue_;
	int maxSize_;
};
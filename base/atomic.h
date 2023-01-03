#pragma once

#include <atomic>
#include "noncopyable.h"


class AtomicInt:noncopyable {
public:
	AtomicInt(): value_(0) {}

	int64_t get() {
		return value_.load();
	}
	
	int64_t getAndAdd(int64_t x) {
		int64_t t = value_.fetch_add(x);
		return t;
	}

	int64_t addAndGet(int64_t x) {
		int64_t t = value_.fetch_add(x) + x;
		return t;
	}

	int64_t incrAndGet() {
		return addAndGet(1);
	}

	int64_t decrAndGet() {
		return addAndGet(-1);
	}

	void add(int64_t x) {
		getAndAdd(x);
	}

	void incr() {
		incrAndGet();
	}

	void decr() {
		decrAndGet();
	}

	int64_t getAndSet(int64_t t) {
		int64_t v = value_.load();
		value_.store(t);
		return v;
	}

private:
	std::atomic<int64_t> value_;
};
#pragma once

#include "noncopyable.h"
#include "types.h"
#include "atomic.h"
#include <thread>
#include <functional>
#include <Windows.h>


class Thread:noncopyable {
public:
	typedef std::function<void()> ThreadFunc;

	Thread(ThreadFunc func, const string& name = string());
	~Thread();

	void start();
	void join();

	bool started() const { return started_; }
	int tid() const { return tid_; }
	const string& name() const { return name_; }

	static int64_t numCreated() { return num_.get(); }

	HANDLE handle() {
		return thread_.native_handle();
	}
private:
	void setDefaultName();

	bool started_;
	bool joined_;
	int  tid_;
	std::thread thread_;
	ThreadFunc func_;
	string name_;

	static AtomicInt num_;
};

struct ThreadData {
	typedef Thread::ThreadFunc ThreadFunc;
	ThreadFunc func_;
	string name_;
	int* tid_;

	ThreadData(ThreadFunc&& func, const string& name, int* tid);
	void runInThread();
};
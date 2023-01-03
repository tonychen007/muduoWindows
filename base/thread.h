#pragma once

#include "noncopyable.h"
#include <string>
#include <thread>
#include <functional>
#include <atomic>

class Thread:noncopyable {
public:
	typedef std::function<void()> ThreadFunc;

	Thread(ThreadFunc func, const std::string& name = std::string());
	~Thread();

	void start();
	void join();

	bool started() const { return started_; }
	int tid() const { return tid_; }
	const std::string& name() const { return name_; }

	static int64_t numCreated() { return num_.load(); }
private:
	void setDefaultName();

	bool started_;
	bool joined_;
	int  tid_;
	std::thread thread_;
	ThreadFunc func_;
	std::string name_;

	static std::atomic<int64_t> num_;
};

struct ThreadData {
	typedef Thread::ThreadFunc ThreadFunc;
	ThreadFunc func_;
	std::string name_;
	int* tid_;

	ThreadData(ThreadFunc&& func, const std::string& name, int* tid);
	void runInThread();
};
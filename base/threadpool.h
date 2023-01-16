#pragma once

#include "thread.h"
#include "mutexGuard.h"
#include "condition.h"

#include <vector>
#include <deque>
#include <string>

class Threadpool :noncopyable {
public:
	typedef std::function<void()> Task;

	explicit Threadpool(const string& name = string("Threadpool"));
	~Threadpool();

	void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
	void setThreadInitCallback(const Task& cb) { threadInitCallback_ = cb; }

	void start(int numThreads);
	void stop();
	void run(Task f);

	const string& name() const { return name_; }

	size_t queueSize() const;
private:
	bool isFull() const;
	void runInThread();
	Task take();

	mutable MutexLock mutex_;
	Condition notEmpty_;
	Condition notFull_;
	string name_;
	std::vector<std::unique_ptr<Thread>> threads_;
	std::deque<Task> queue_;
	size_t maxQueueSize_;
	bool running_;
	Task threadInitCallback_;
};
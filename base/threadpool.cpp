#include "threadpool.h"
#include "currentThread.h"
#include "exception.h"
#include "types.h"


Threadpool::Threadpool(const string& name)
:	mutex_(),
	notEmpty_(mutex_),
	notFull_(mutex_),
	name_(name),
	maxQueueSize_(0),
	running_(false) {

	threads_.reserve(10);
}

Threadpool::~Threadpool() {
	if (running_)
		stop();
}

void Threadpool::start(int numThreads) {
	assert(threads_.empty());
	running_ = true;
	threads_.reserve(numThreads);
	for (int i = 0; i < numThreads; ++i) {
		char id[32];
		snprintf(id, sizeof id, "%d", i + 1);
		threads_.emplace_back(new Thread(
			std::bind(&Threadpool::runInThread, this), name_ + id));
		threads_[i]->start();
	}
}

void Threadpool::stop() {
	{
		MutexLockGuard lock(mutex_);
		running_ = false;
		notEmpty_.notifyAll();
		notFull_.notifyAll();
	}

	for (auto& th : threads_) {
		th->join();
	}
}

void Threadpool::run(Task f) {
	if (threads_.empty()) {
		f();
	}
	else {
		uniqueLock lock = notFull_.getUniqueLock();
		mutex_.assignedHolder();
		while (isFull() && running_) {
			notFull_.wait(lock);
		}
		
		if (!running_) {
			mutex_.unassignedHolder();
			return;
		}
		assert(!isFull());

		queue_.push_back(std::move(f));
		notEmpty_.notify();
		mutex_.unassignedHolder();
	}	
}

bool Threadpool::isFull() const {
	assert((mutex_.isLockedByCurrentThread()));
	return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void Threadpool::runInThread() {
	try {
		if (threadInitCallback_) {
			threadInitCallback_();
		}

		while (running_) {
			Task f(take());
			if (f) {
				f();
			}
		}
	}
	catch (const Exception& ex) {
		fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
		fprintf(stderr, "reason: %s\n", ex.what());
		fprintf(stderr, "Callstack: %s\n", ex.stackTrace());
	}
	catch (const std::exception& ex) {
		fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
		fprintf(stderr, "reason: %s\n", ex.what());
	}
	catch (...) {
		fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
		throw;
	}
	
}

Threadpool::Task Threadpool::take() {
	uniqueLock lock = notEmpty_.getUniqueLock();
	while (queue_.empty() && running_)
		notEmpty_.wait(lock);

	Task task;
	if (!queue_.empty()) {
		task = queue_.front();
		queue_.pop_front();
		if (maxQueueSize_ > 0) {
			notFull_.notify();
		}
	}

	return task;
}
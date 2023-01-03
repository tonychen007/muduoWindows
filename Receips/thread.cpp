#include "thread.h"
#include "currentThread.h"
#include "exception.h"

std::atomic<int64_t> Thread::num_;

ThreadData::ThreadData(ThreadFunc&& func, const std::string& name, int* tid) 
	:func_(std::move(func)), name_(name), tid_(tid) {
}

void ThreadData::runInThread() {
	*tid_ = CurrentThread::tid();
	CurrentThread::setName(name_.c_str());
	CurrentThread::setThreadName(*tid_, name_.c_str());
	tid_ = NULL;

	try {
		func_();
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

void* startThread(void* obj) {
	ThreadData* data = static_cast<ThreadData*>(obj);
	data->runInThread();
	delete data;
	return nullptr;
}

Thread::Thread(ThreadFunc func, const std::string& name)
	:started_(0),joined_(0), tid_(0), name_(name), func_(std::move(func)) {
	setDefaultName();
}

Thread::~Thread() {
	if (started_ && !joined_)
		thread_.detach();
}

void Thread::setDefaultName() {
	int64_t num = num_.fetch_add(1);
	if (name_.empty()) {
		char buf[32];
		snprintf(buf, sizeof buf, "Thread%lld", num);
		name_ = buf;
	}
}

void Thread::start() {
	started_ = 1;
	ThreadData* data = new ThreadData(std::move(func_), name_, &tid_);
	thread_ = std::thread(startThread, data);
	if (thread_.native_handle() == nullptr) {
		delete data;
		started_ = 0;
	}
}

void Thread::join() {
	joined_ = 1;
	thread_.join();
}
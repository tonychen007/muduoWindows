#pragma once

#include "base/condition.h"
#include "base/mutexGuard.h"
#include "base/thread.h"

namespace net {
	class EventLoop;

	class EventLoopThread :noncopyable {
	public:
		typedef std::function<void(EventLoop*)> ThreadInitCallback;

		EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
			const string& name = string());
		~EventLoopThread();

		EventLoop* startLoop();

	private:
		void threadFunc();

		EventLoop* loop_;
		bool exiting_;
		Thread thread_;
		MutexLock mutex_;
		Condition cond_;
		ThreadInitCallback callback_;
	};
}
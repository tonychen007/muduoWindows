#pragma once

#include <set>
#include <vector>

#include "base/MutexGuard.h"
#include "base/Timestamp.h"
#include "callbacks.h"
#include "channel.h"
#include "timeMinHeap.h"


namespace net {
	class EventLoop;
	class Timer;
	class TimerId;

	class TimerQueue :noncopyable {
	public:
		TimerQueue(EventLoop* loop);
		~TimerQueue();

		TimerId addTimer(const TimerCallback& cb, Timestamp when, double interval);

		void cancel(TimerId timerId);

		void expiredProcess(Timestamp now);
		int earliestExpiredTime(Timestamp now);
	private:
		typedef std::set<Timer*> TimerSet;

		void addTimerInLoop(Timer* timer);
		void cancelInLoop(TimerId timerId);

		EventLoop* loop_;
		TimerSet timerSet_;
		min_heap_t timeMinHeap_;
		bool callingExpiredTimers_;
	};
}
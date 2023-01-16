#pragma once

#include <Windows.h>
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

	class TimerQueueWin :noncopyable {
	public:
		TimerQueueWin(EventLoop* loop);
		~TimerQueueWin();

		TimerId addTimer(const TimerCallback& cb, Timestamp when, double interval);

	private:
		typedef std::pair<Timestamp, Timer*> Entry;
		typedef std::set<Entry> TimerList;

		// called when timerfd alarms
		void handleRead();
		// move out all expired timers
		std::vector<Entry> getExpired(Timestamp now);
		void reset(const std::vector<Entry>& expired, Timestamp now);

		bool insert(Timer* timer);
		EventLoop* loop_;
		HANDLE hTimer_;
		HANDLE hTimerQueue_;
		Channel timerfdChannel_;
		TimerList timers_;
	};
}
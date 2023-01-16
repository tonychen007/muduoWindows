#pragma once

#include "poller.h"

struct pollfd;

//copy from muduo-win gjm

namespace net {
	class SelectPoller :public Poller {
	public:
		SelectPoller(EventLoop* loop);
		virtual ~SelectPoller();

		virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
		virtual void updateChannel(Channel* channel);
		virtual void removeChannel(Channel* channel);
	private:
		void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

		typedef std::vector<struct pollfd> PollFdList;
		PollFdList pollfds_;
		fd_set rfds_;
		fd_set wfds_;
		fd_set efds_;
	};// end class SelectPoller
}// end namespace net
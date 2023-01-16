#pragma once

#include "poller.h"
#include "epoll.h"

struct epoll_event;

namespace net {
	class EPollPoller : public Poller {
	public:
		EPollPoller(EventLoop* loop);
		~EPollPoller() override;
		
		virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
		virtual void updateChannel(Channel* channel);
		virtual void removeChannel(Channel* channel);

	private:
		static const int kInitEventListSize = 16;
		static const char* operationToString(int op);

		void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
		void update(int operation, Channel* channel);

		typedef std::vector<struct epoll_event> EventList;

		epoll_t epollfd_;
		EventList events_;
	};
}
#pragma once

#include <map>

#include "base/timestamp.h"
#include "eventLoop.h"

namespace net {

	class Channel;

	///
	/// Base class for IO Multiplexing
	///
	/// This class doesn't own the Channel objects.
	class Poller : noncopyable {
	public:
		typedef std::vector<Channel*> ChannelList;

		Poller(EventLoop* loop);
		virtual ~Poller();

		/// Polls the I/O events.
		/// Must be called in the loop thread.
		virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

		/// Changes the interested I/O events.
		/// Must be called in the loop thread.
		virtual void updateChannel(Channel* channel) = 0;

		/// Remove the channel, when it destructs.
		/// Must be called in the loop thread.
		virtual void removeChannel(Channel* channel) = 0;

		virtual bool hasChannel(Channel* channel) const;

		static Poller* newDefaultPoller(EventLoop* loop);

		void assertInLoopThread() const {
			ownerLoop_->assertInLoopThread();
		}

	protected:
		typedef std::map<int, Channel*> ChannelMap;
		ChannelMap channels_;

	private:
		EventLoop* ownerLoop_;
	};
}

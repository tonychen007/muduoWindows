#pragma once

#include <functional>

#include "base/noncopyable.h"
#include "channel.h"
#include "socket.h"

namespace net {
	class EventLoop;
	class InetAddress;

	class Acceptor : noncopyable {
	public:
		typedef std::function<void(int sockfd, const InetAddress&)> NewConnectionCallback;

		Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport = false);
		~Acceptor();

		void setNewConnectionCallback(const NewConnectionCallback& cb) {
			newConnectionCallback_ = cb;
		}

		void listen();
		bool listening() const { return listening_; }

		int fd() const { return acceptSocket_.fd(); }

	private:
		void handleRead();

		EventLoop* loop_;
		Socket acceptSocket_;
		Channel acceptChannel_;
		NewConnectionCallback newConnectionCallback_;
		bool listening_;
		int idleFd_;
	};
}
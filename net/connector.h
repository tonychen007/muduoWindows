#pragma once

#include "base/noncopyable.h"
#include "inetAddress.h"

#include <functional>
#include <memory>

namespace net {
	class Channel;
	class EventLoop;

	class Connector : noncopyable,
		public std::enable_shared_from_this<Connector> {
	public:
		typedef std::function<void(int sockfd)> NewConnectionCallback;

		Connector(EventLoop* loop, const InetAddress& serverAddr);
		~Connector();

		void setNewConnectionCallback(const NewConnectionCallback& cb) {
			newConnectionCallback_ = cb;
		}

		void start();  // can be called in any thread
		void restart();  // must be called in loop thread
		void stop();  // can be called in any thread

		const InetAddress& serverAddress() const { return serverAddr_; }

	private:
		enum States { kDisconnected, kConnecting, kConnected };
		static const int kMaxRetryDelayMs = 30 * 1000;
		static const int kInitRetryDelayMs = 500;

		void setState(States s) { state_ = s; }
		void startInLoop();
		void stopInLoop();
		void connect();
		void connecting(int sockfd);
		void handleWrite();
		void handleError();
		void retry(int sockfd);
		int removeAndResetChannel();
		void resetChannel();

		EventLoop* loop_;
		InetAddress serverAddr_;
		bool connect_; // atomic
		States state_;  // FIXME: use atomic variable
		std::unique_ptr<Channel> channel_;
		NewConnectionCallback newConnectionCallback_;
		int retryDelayMs_;
	};
}

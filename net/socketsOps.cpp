#include "socketsOps.h"
#include "endian.h"
#include "base/types.h"
#include "base/logging.h"
#include <assert.h>
#include <Windows.h>

namespace sockets {
	void setNonBlockAndCloseOnExec(int sockfd) {
		u_long mode = 1;
		ioctlsocket(sockfd, FIONBIO, &mode);

		// emulate FD_CLOEXEC
		int ret = SetHandleInformation((HANDLE)sockfd, HANDLE_FLAG_INHERIT, FALSE);
		if (!ret) {
			LOG_INFO << "Error to SetHandleInformation on socket";
		}
		else {
			LOG_INFO << "Success to SetHandleInformation on socket";
		}
	}

	int createNonblockingOrDie(sa_family_t family) {
		int sockfd = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
		if (sockfd < 0) {
			LOG_SYSFATAL << "sockets::createNonblockingOrDie";
		}

		setNonBlockAndCloseOnExec(sockfd);
		return sockfd;
	}

	int  connect(int sockfd, const struct sockaddr* addr) {
		return ::connect(sockfd, addr, static_cast<int>(sizeof(struct sockaddr_in6)));
	}

	void bindOrDie(int sockfd, const struct sockaddr* addr) {
		int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
		if (ret < 0) {
			LOG_SYSFATAL << "sockets::bindOrDie";
		}
	}

	void listenOrDie(int sockfd) {
		int ret = ::listen(sockfd, SOMAXCONN);
		if (ret < 0) {
			LOG_SYSFATAL << "sockets::listenOrDie";
		}
	}

	int  accept(int sockfd, struct sockaddr_in6* addr) {
		return 0;
	}

	ssize_t read(int sockfd, void* buf, size_t count) {
		return ::recv(sockfd, (char*)buf, count, 0);
	}

	ssize_t readv(int sockfd, iovec* iov, int iovcnt) {
		DWORD bytesRead = 0;
		DWORD flags = 0;

		WSARecv(sockfd, iov, iovcnt, &bytesRead, &flags, 0, 0);
		return bytesRead;
	}

	ssize_t write(int sockfd, const void* buf, size_t count) {
		return ::send(sockfd, (const char*)buf, count, 0);
	}

	void close(int sockfd) {
		if (::closesocket(sockfd) < 0) {
			LOG_SYSERR << "sockets::close";
		}
	}

	void shutdownWrite(int sockfd) {
		if (::shutdown(sockfd, SD_SEND) < 0) {
			LOG_SYSERR << "sockets::shutdownWrite";
		}
	}

	void toIpPort(char* buf, size_t size, const struct sockaddr* addr) {
		if (addr->sa_family == AF_INET6) {
			buf[0] = '[';
			toIp(buf + 1, size - 1, addr);
			size_t end = ::strlen(buf);
			const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
			uint16_t port = networkToHost16(addr6->sin6_port);
			assert(size > end);
			snprintf(buf + end, size - end, "]:%u", port);
			return;
		}
		toIp(buf, size, addr);
		size_t end = ::strlen(buf);
		const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
		uint16_t port = networkToHost16(addr4->sin_port);
		assert(size > end);
		snprintf(buf + end, size - end, ":%u", port);
	}

	void toIp(char* buf, size_t size, const struct sockaddr* addr) {
		if (addr->sa_family == AF_INET) {
			assert(size >= INET_ADDRSTRLEN);
			const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
			::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
		}
		else if (addr->sa_family == AF_INET6) {
			assert(size >= INET6_ADDRSTRLEN);
			const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
			::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
		}
	}

	void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr) {
		addr->sin_family = AF_INET;
		addr->sin_port = hostToNetwork16(port);
		if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
			LOG_SYSERR << "sockets::fromIpPort";
		}
	}

	void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr) {
		addr->sin6_family = AF_INET6;
		addr->sin6_port = hostToNetwork16(port);
		if (::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0) {
			LOG_SYSERR << "sockets::fromIpPort";
		}
	}

	int getSocketError(int sockfd) {
		char optval;
		socklen_t optlen = static_cast<socklen_t>(sizeof optval);

		if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
			return errno;
		}
		else {
			return optval;
		}
	}

	struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in6* addr) {
		return static_cast<struct sockaddr*>((void*)addr);
	}

	struct sockaddr* sockets::sockaddr_cast(struct sockaddr_in6* addr) {
		return static_cast<struct sockaddr*>((void*)(addr));
	}

	struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in* addr) {
		return static_cast<struct sockaddr*>((void*)(addr));
	}

	struct sockaddr_in* sockets::sockaddr_in_cast(const struct sockaddr* addr) {
		return static_cast<struct sockaddr_in*>((void*)(addr));
	}

	struct sockaddr_in6* sockets::sockaddr_in6_cast(const struct sockaddr* addr) {
		return static_cast<struct sockaddr_in6*>((void*)(addr));
	}

	struct sockaddr_in6 getLocalAddr(int sockfd) {
		struct sockaddr_in6 localaddr;
		memset(&localaddr, 0,sizeof localaddr);
		socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
		if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0) {
			LOG_SYSERR << "sockets::getLocalAddr";
		}
		return localaddr;
	}

	struct sockaddr_in6 getPeerAddr(int sockfd) {
		struct sockaddr_in6 peeraddr;
		memset(&peeraddr, 0, sizeof peeraddr);
		socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
		if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0) {
			LOG_SYSERR << "sockets::getPeerAddr";
		}
		return peeraddr;
	}

	bool isSelfConnect() {
		return 0;
	}

	void InitSocket() {
		WSADATA wsa;
		WSAStartup(MAKEWORD(2, 2), &wsa);
	}

	void DestorySocket() {
		WSACleanup();
	}
}

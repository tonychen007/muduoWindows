#include "socketsOps.h"
#include "endian.h"
#include "base/types.h"
#include "base/logging.h"
#include <assert.h>
#include <Windows.h>

namespace sockets {
	void getAddrInfo(struct addrinfo** addr, const char* hostname, const char* port, int ip46) {
		struct addrinfo hints;

		memset(&hints, 0, sizeof(hints));
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_family = ip46;
		getaddrinfo(hostname, port, &hints, addr);
	}

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
		int sockfd = createBlockingOrDie(family);
		setNonBlockAndCloseOnExec(sockfd);
		return sockfd;
	}

	int createBlockingOrDie(sa_family_t family) {
		int sockfd = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
		if (sockfd < 0) {
			LOG_SYSFATAL << "sockets::createNonblockingOrDie";
		}

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
		socklen_t addrlen = static_cast<socklen_t>(sizeof * addr);
		int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
		setNonBlockAndCloseOnExec(connfd);

		if (connfd < 0) {
			int savedErrno = GetLastError();
			LOG_SYSERR << "Socket::accept";
			switch (savedErrno) {
			case EAGAIN:
			case ECONNABORTED:
			case EINTR:
			case EPROTO: // ???
			case EPERM:
			case EMFILE: // per-process lmit of open file desctiptor ???
			  // expected errors
				errno = savedErrno;
				break;
			case EBADF:
			case EFAULT:
			case EINVAL:
			case ENFILE:
			case ENOBUFS:
			case ENOMEM:
			case ENOTSOCK:
			case EOPNOTSUPP:
				// unexpected errors
				LOG_FATAL << "unexpected error of ::accept " << savedErrno;
				break;
			default:
				LOG_FATAL << "unknown error of ::accept " << savedErrno;
				break;
			}
		}

		return connfd;
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

	const struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in* addr) {
		return static_cast<const struct sockaddr*>((const void*)addr);
	}

	const struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in6* addr) {
		return static_cast<const struct sockaddr*>((const void*)(addr));
	}

	struct sockaddr* sockets::sockaddr_cast(struct sockaddr_in6* addr) {
		return static_cast<struct sockaddr*>((void*)(addr));
	}

	const struct sockaddr_in* sockets::sockaddr_in_cast(const struct sockaddr* addr) {
		return static_cast<const struct sockaddr_in*>((const void*)(addr));
	}

	const struct sockaddr_in6* sockets::sockaddr_in6_cast(const struct sockaddr* addr) {
		return static_cast<const struct sockaddr_in6*>((const void*)(addr));
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

	bool isSelfConnect(int sockfd) {
		struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
		struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
		if (localaddr.sin6_family == AF_INET) {
			const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
			const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
			return laddr4->sin_port == raddr4->sin_port
				&& laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
		}
		else if (localaddr.sin6_family == AF_INET6) {
			return localaddr.sin6_port == peeraddr.sin6_port
				&& memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
		}
		else {
			return false;
		}
	}

	void InitSocket() {
		WSADATA wsa;
		WSAStartup(MAKEWORD(2, 2), &wsa);
	}

	void DestorySocket() {
		WSACleanup();
	}

	// copy from muduo-win gjm
	stPipe pipe() {
		stPipe sock_pipe;
		sock_pipe.pipe_read = -1;
		sock_pipe.pipe_write = -1;
		int tcp1, tcp2;
		sockaddr_in name;
		memset(&name, 0, sizeof(name));
		name.sin_family = AF_INET;
		name.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		int namelen = sizeof(name);
		tcp1 = tcp2 = -1;
		
		int tcp = socket(AF_INET, SOCK_STREAM, 0);
		if (tcp == -1) {
			goto clean;
		}
		if (bind(tcp, (sockaddr*)&name, namelen) == -1) {
			goto clean;
		}
		if (listen(tcp, 5) == -1) {
			goto clean;
		}
		if (getsockname(tcp, (sockaddr*)&name, &namelen) == -1) {
			goto clean;
		}
		tcp1 = socket(AF_INET, SOCK_STREAM, 0);
		if (tcp1 == -1) {
			goto clean;
		}
		if (-1 == connect(tcp1, (sockaddr*)&name, namelen)) {
			goto clean;
		}
		tcp2 = accept(tcp, (sockaddr*)&name, &namelen);
		if (tcp2 == -1) {
			goto clean;
		}
		if (closesocket(tcp) == -1) {
			goto clean;
		}
		sock_pipe.pipe_read = tcp1;
		sock_pipe.pipe_write = tcp2;
		return sock_pipe;
	clean:
		if (tcp != -1) {
			closesocket(tcp);
		}
		if (tcp2 != -1) {
			closesocket(tcp2);
		}
		if (tcp1 != -1) {
			closesocket(tcp1);
		}
		return sock_pipe;
	}
}

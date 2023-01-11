#pragma once

#include "types.h"


namespace sockets {
	typedef struct _stPipe {
		int pipe_read;
		int pipe_write;
	} stPipe;

	void InitSocket();
	void DestorySocket();

	void setNonBlockAndCloseOnExec(int sockfd);
	int createNonblockingOrDie(sa_family_t family);
	int createBlockingOrDie(sa_family_t family);

	int  connect(int sockfd, const struct sockaddr* addr);
	void bindOrDie(int sockfd, const struct sockaddr* addr);
	void listenOrDie(int sockfd);
	int  accept(int sockfd, struct sockaddr_in6* addr);
	ssize_t read(int sockfd, void* buf, size_t count);
	ssize_t readv(int sockfd, iovec* iov, int iovcnt);
	ssize_t write(int sockfd, const void* buf, size_t count);
	void close(int sockfd);
	void shutdownWrite(int sockfd);

	void toIpPort(char* buf, size_t size, const struct sockaddr* addr);
	void toIp(char* buf, size_t size, const struct sockaddr* addr);

	void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr);
	void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr);

	int getSocketError(int sockfd);

	const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
	const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
	struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr);
	const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
	const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr);

	struct sockaddr_in6 getLocalAddr(int sockfd);
	struct sockaddr_in6 getPeerAddr(int sockfd);
	bool isSelfConnect(int sockfd);

	stPipe pipe();
}
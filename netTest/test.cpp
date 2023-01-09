#pragma once

#include "test.h"

void getAddrInfo(struct addrinfo** addr, const char* hostname, const char* port, int ip46) {
	struct addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = ip46;
	getaddrinfo(hostname, port, &hints, addr);
}

const char* buildHttpMsg(char* buffer, int size) {
	int i;

	i = sprintf_s(buffer, size, "GET /%s HTTP/1.1\r\n", "/");
	//i += sprintf_s(buffer + strlen(buffer), sizeof(buffer) - i, "Host: %s:%s\r\n", hostname, port);
	i += sprintf_s(buffer + strlen(buffer), size - i, "Connection: close\r\n");
	i += sprintf_s(buffer + strlen(buffer), size - i, "User-Agent: honpwc web_get 1.0\r\n");
	i += sprintf_s(buffer + strlen(buffer), size - i, "\r\n");
	
	return buffer;
}

void testSocketOps() {
	sockets::InitSocket();

	char buf[2048];
	const char* hostname = "www.baidu.com";
	const char* port = "80";
	const char* msg = buildHttpMsg(buf, sizeof(buf));
	struct addrinfo* addr;
	sockaddr_in addr4;
	sockaddr_in6 addr6;
	int ret;
	int bys = 0;
	int s = sockets::createNonblockingOrDie(AF_INET);

	getAddrInfo(&addr, hostname, port, AF_INET);
	ret = sockets::connect(s, addr->ai_addr);
	
	fd_set reads;
	fd_set writes;
	FD_ZERO(&reads);
	FD_ZERO(&writes);
	FD_SET(s, &reads);
	FD_SET(s, &writes);

	select(s + 1, 0, &writes, 0, nullptr);
	if (FD_ISSET(s, &writes)) {
		LOG_INFO << "remote socket is ready to write.";
		int len = strlen(msg);
		bys = sockets::write(s, msg, strlen(msg));
		assert(bys == len);
		LOG_INFO << "write to remote socket successfully.";
	}

	select(s + 1, &reads, 0, 0, nullptr);
	if (FD_ISSET(s, &reads)) {
		char buf[512];
		WSABUF wsabuf;
		wsabuf.buf = buf;
		wsabuf.len = 512;

		//bys = sockets::read(s, buf, sizeof(buf));
		bys = sockets::readv(s, &wsabuf, 1);
		buf[511] = '\0';
		if (bys > 0) {
			LOG_INFO << "Receive: " << buf;
		}
	}

	int err = sockets::getSocketError(s);

	printf("\n");
	sockets::fromIpPort("8.8.8.8", 80, &addr4);
	sockets::fromIpPort("::ffff:808:808", 80, &addr6);

	sockets::toIpPort(buf, sizeof(buf), (sockaddr*)&addr4);
	LOG_INFO << buf;
	sockets::toIpPort(buf, sizeof(buf), (sockaddr*)&addr6);
	LOG_INFO << buf;

	addr6 = sockets::getLocalAddr(s);
	sockets::toIpPort(buf, sizeof(buf), (sockaddr*)&addr6);
	LOG_INFO << "Local addr:" << buf;
	
	addr6 = sockets::getPeerAddr(s);
	sockets::toIpPort(buf, sizeof(buf), (sockaddr*)&addr6);
	LOG_INFO << "Peer addr:" << buf;

	sockets::isSelfConnect(s);
	sockets::close(s);
	sockets::DestorySocket();
}
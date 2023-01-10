#pragma once

#include "test.h"

class GWinSock {
public:
	GWinSock() {
		sockets::InitSocket();
	}

	~GWinSock() {
		sockets::DestorySocket();
	}
};

GWinSock gWinSock;

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

void testInetAddress() {
	char buf[MAX_PATH];
	net::InetAddress inetAddr1(1234, 1);
	net::InetAddress inetAddr2("0.0.0.0", 1234);
	net::InetAddress outAddr;
	net::InetAddress::resolve("www.baidu.com", &outAddr);

	LOG_INFO << "IpPort is:" << inetAddr1.toIpPort();
	LOG_INFO << "Ipv4:" << inetAddr1.ipv4NetEndian();
	LOG_INFO << "port:" << inetAddr1.port();
	LOG_INFO << "port netEndian:" << inetAddr1.portNetEndian();
}

void testSocketClient() {
	const char* hostname = "www.baidu.com";
	const char* port = "80";
	struct addrinfo* addr;

	int s = sockets::createNonblockingOrDie(AF_INET);
	getAddrInfo(&addr, hostname, port, AF_INET);
	int ret = sockets::connect(s, addr->ai_addr);
	net::Socket sock(s);

	fd_set writes;
	FD_ZERO(&writes);
	FD_SET(s, &writes);

	select(s + 1, 0, &writes, 0, nullptr);
	if (FD_ISSET(s, &writes)) {
		char buf[512] = { 0 };
		sock.getTcpInfoString(buf, sizeof(buf));
		printf("\nTcp Info:\n%s\n", buf);
	}
}

void testSocketServer() {
	int s = sockets::createBlockingOrDie(AF_INET);
	net::InetAddress peerAddr;

	net::Socket sock(s);
	sock.setReuseAddr(1);
	sock.setTcpNoDelay(1);
	net::InetAddress listenAddr(2000, false);
	sock.bindAddress(listenAddr);
	sock.listen();
	int connfd = sock.accept(&peerAddr);
	ssize_t bys = 0;

	// use while easy for test
	char buf[512];
	sockets::read(connfd, buf, sizeof(buf));
	DWORD err = GetLastError();
	while (err == WSAEWOULDBLOCK) {
		bys = sockets::read(connfd, buf, sizeof(buf));
		err = GetLastError();
		Sleep(50);
	}

	buf[bys] = '\0';
	printf("read buf:%s\n", buf);
}

void testTimer() {
	net::Timer timer([] {
		printf("Timer callback\n");
		}, Timestamp::now(), 1);

	timer.run();
	timer.repeat();
	timer.restart(Timestamp::now());
	int64_t s = timer.numCreated();
	Timestamp expr = timer.expiration();
	printf("%s\n", expr.toString().c_str());
}

void testEventloop() {
	//_putenv_s("MUDUO_LOG_TRACE", "1");
	//g_logLevel = initLogLevel();

	net::EventLoop loop;

	Thread th1([&] {
		LOG_INFO << "threadFunc:pid = " << CurrentThread::tid();
		loop.runInLoop([&] {
			LOG_INFO << "func:pid = " << CurrentThread::tid();
			});
		});

	th1.start();
	loop.loop();
}

int g_cnt = 0;

void print(const char* msg, net::EventLoop* pLoop) {
	LOG_INFO << "time:" << Timestamp::now().toFormattedString() << " msg:" << msg;
	if (++g_cnt == 100) {
		pLoop->quit();
	}
}

void cancel(TimerId timer, net::EventLoop* pLoop) {
	pLoop->cancel(timer);
	printf("cancelled at %s\n", Timestamp::now().toString().c_str());
}

void testTimerQueue() {
	net::EventLoop loop;
	
	loop.runAfter(1, std::bind(print, "once1", &loop));
	loop.runAfter(2.5, std::bind(print, "once2.5", &loop));
	loop.runAfter(3, std::bind(print, "once3", &loop));
	TimerId t45 = loop.runAfter(4.5, std::bind(print, "once4.5", &loop));
	loop.runAfter(4.2, std::bind(cancel, t45, &loop));
	loop.runAfter(4.8, std::bind(cancel, t45, &loop));
	loop.runEvery(2, std::bind(print, "every2", &loop));
	TimerId t3 = loop.runEvery(3, std::bind(print, "every3", &loop));
	loop.runAfter(9.001, std::bind(cancel, t3, &loop));

	loop.loop();
}
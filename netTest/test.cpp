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

	sockets::getAddrInfo(&addr, hostname, port, AF_INET);
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
	sockets::getAddrInfo(&addr, hostname, port, AF_INET);
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

void testBuffer() {

	{
		Buffer buf;
		assert(buf.readableBytes() == 0);
		assert(buf.writableBytes() == Buffer::kInitialSize);
		assert(buf.prependableBytes() == Buffer::kCheapPrepend);

		const string str(200, 'x');
		buf.append(str);
		assert(buf.readableBytes() == str.size());
		assert(buf.writableBytes() == Buffer::kInitialSize - str.size());
		assert(buf.prependableBytes() == Buffer::kCheapPrepend);

		const string str2 = buf.retrieveAsString(50);
		assert(str2.size() == 50);
		assert(buf.readableBytes() == str.size() - str2.size());
		assert(buf.writableBytes() == Buffer::kInitialSize - str.size());
		assert(buf.prependableBytes() == Buffer::kCheapPrepend + str2.size());
		assert(str2 == string(50, 'x'));

		buf.append(str);
		assert(buf.readableBytes() == 2 * str.size() - str2.size());
		assert(buf.writableBytes() == Buffer::kInitialSize - 2 * str.size());
		assert(buf.prependableBytes() == Buffer::kCheapPrepend + str2.size());

		const string str3 = buf.retrieveAllAsString();
		assert(str3.size() == 350);
		assert(buf.readableBytes() == 0);
		assert(buf.writableBytes() == Buffer::kInitialSize);
		assert(buf.prependableBytes() == Buffer::kCheapPrepend);
		assert(str3 == string(350, 'x'));
	}

	{
		Buffer buf;
		buf.append(string(400, 'y'));
		assert(buf.readableBytes() == 400);
		assert(buf.writableBytes() == Buffer::kInitialSize - 400);

		buf.retrieve(50);
		assert(buf.readableBytes() == 350);
		assert(buf.writableBytes() == Buffer::kInitialSize - 400);
		assert(buf.prependableBytes() == Buffer::kCheapPrepend + 50);

		buf.append(string(1000, 'z'));
		assert(buf.readableBytes() == 1350);
		assert(buf.writableBytes() == 0);
		assert(buf.prependableBytes() == Buffer::kCheapPrepend + 50); // FIXME

		buf.retrieveAll();
		assert(buf.readableBytes() == 0);
		assert(buf.writableBytes() == 1400); // FIXME
		assert(buf.prependableBytes() == Buffer::kCheapPrepend);
	}

	{
		Buffer buf;
		buf.append(string(800, 'y'));
		assert(buf.readableBytes() == 800);
		assert(buf.writableBytes() == Buffer::kInitialSize - 800);

		buf.retrieve(500);
		assert(buf.readableBytes() == 300);
		assert(buf.writableBytes() == Buffer::kInitialSize - 800);
		assert(buf.prependableBytes() == Buffer::kCheapPrepend + 500);

		buf.append(string(300, 'z'));
		assert(buf.readableBytes() == 600);
		assert(buf.writableBytes() == Buffer::kInitialSize - 600);
		assert(buf.prependableBytes() == Buffer::kCheapPrepend);
	}

	{
		Buffer buf;
		buf.append(string(2000, 'y'));
		assert(buf.readableBytes() == 2000);
		assert(buf.writableBytes() == 0);
		assert(buf.prependableBytes() == Buffer::kCheapPrepend);

		buf.retrieve(1500);
		assert(buf.readableBytes() == 500);
		assert(buf.writableBytes() == 0);
		assert(buf.prependableBytes() == Buffer::kCheapPrepend + 1500);

		buf.shrink(0);
		assert(buf.readableBytes() == 500);
		assert(buf.writableBytes() == Buffer::kInitialSize - 500);
		assert(buf.retrieveAllAsString() == string(500, 'y'));
		assert(buf.prependableBytes() == Buffer::kCheapPrepend);
	}
	
	{
		Buffer buf;
		buf.append(string(200, 'y'));
		assert(buf.readableBytes(), 200);
		assert(buf.writableBytes() == Buffer::kInitialSize - 200);
		assert(buf.prependableBytes() == Buffer::kCheapPrepend);

		int x = 0;
		buf.prepend(&x, sizeof x);
		assert(buf.readableBytes() == 204);
		assert(buf.writableBytes() == Buffer::kInitialSize - 200);
		assert(buf.prependableBytes() == Buffer::kCheapPrepend - 4);
	}
	
	{
		Buffer buf;
		buf.append("HTTP");

		assert(buf.readableBytes() == 4);
		assert(buf.peekInt8() == 'H');
		int top16 = buf.peekInt16();
		assert(top16 == 'H' * 256 + 'T');
		assert(buf.peekInt32() == top16 * 65536 + 'T' * 256 + 'P');

		assert(buf.readInt8() == 'H');
		assert(buf.readInt16() == 'T' * 256 + 'T');
		assert(buf.readInt8() == 'P');
		assert(buf.readableBytes() == 0);
		assert(buf.writableBytes() == Buffer::kInitialSize);

		buf.appendInt8(-1);
		buf.appendInt16(-2);
		buf.appendInt32(-3);
		assert(buf.readableBytes() == 7);
		assert(buf.readInt8() == -1);
		assert(buf.readInt16() == -2);
		assert(buf.readInt32() == -3);
	}
	
	{
		Buffer buf;
		buf.append(string(100000, 'x'));
		const char* null = NULL;
		assert(buf.findEOL() == null);
		assert(buf.findEOL(buf.peek() + 90000) == null);
	}

	Buffer buf;
	buf.append("muduo", 5);
	const void* inner = buf.peek();
	// printf("Buffer at %p, inner %p\n", &buf, inner);
	//output(std::move(buf), inner);
	Buffer newbuf(std::move(buf));
	assert(inner, newbuf.peek());
}

void testEventLoopThread() {
	{
		EventLoopThread thr1;  // never start
	}

	{
		// dtor calls quit()
		EventLoopThread thr2;
		EventLoop* loop = thr2.startLoop();
		loop->runInLoop([&] {
			printf("print: pid = %d, tid = %d, loop = %p\n",
				_getpid(), CurrentThread::tid(), loop);
		});
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	// quit() before dtor
	EventLoopThread thr3;
	EventLoop* loop = thr3.startLoop();
	loop->runInLoop([&] {
		printf("print: pid = %d, tid = %d, loop = %p\n",
			_getpid(), CurrentThread::tid(), loop);
		loop->quit();
	});
	std::this_thread::sleep_for(std::chrono::seconds(1));
}


void init(EventLoop* p) {
	printf("init(): pid = %d, tid = %d, loop = %p\n",
		getpid(), CurrentThread::tid(), p);
}

void testEventLoopThreadPool() {
	EventLoop loop;
	loop.runAfter(10, std::bind(&EventLoop::quit, &loop));
	
	{
		printf("Single thread %p:\n", &loop);
		EventLoopThreadPool model(&loop, "single");
		model.setThreadNum(0);
		model.start(init);
		assert(model.getNextLoop() == &loop);
		assert(model.getNextLoop() == &loop);
		assert(model.getNextLoop() == &loop);
	}

	{
		printf("Another thread:\n");
		EventLoopThreadPool model(&loop, "another");
		model.setThreadNum(1);
		model.start(init);
		EventLoop* nextLoop = model.getNextLoop();
		nextLoop->runAfter(2, [&] {
			printf("main(): pid = %d, tid = %d, loop = %p\n",
				getpid(), CurrentThread::tid(), nextLoop);
		});
		assert(nextLoop != &loop);
		assert(nextLoop == model.getNextLoop());
		assert(nextLoop == model.getNextLoop());
		std::this_thread::sleep_for(std::chrono::seconds(3));
	}

	{
		printf("Three threads:\n");
		EventLoopThreadPool model(&loop, "three");
		model.setThreadNum(3);
		model.start(init);
		EventLoop* nextLoop = model.getNextLoop();
		nextLoop->runInLoop([&] {
			printf("main(): pid = %d, tid = %d, loop = %p\n",
				getpid(), CurrentThread::tid(), nextLoop);
		});
		assert(nextLoop != &loop);
		assert(nextLoop != model.getNextLoop());
		assert(nextLoop != model.getNextLoop());
		assert(nextLoop == model.getNextLoop());
	}

	loop.loop();
}

void newConnection(int sockfd, const InetAddress& peerAddr)
{
	LOG_INFO << "new connection from " << peerAddr.toIpPort();
	// can not use write in windows socket. use send 
	sockets::write(sockfd, "hello\r\n", strlen("hello\r\n"));
	sockets::close(sockfd);
	LOG_INFO << "close";
}

void testAcceptor() {
	EventLoop loop;
	InetAddress addr(2000);
	
	Acceptor accp(&loop, addr, true);
	accp.setNewConnectionCallback(newConnection);
	accp.listen();
	loop.loop();
}

void connectCallback(int sockfd) {
	LOG_INFO << "connected";
	sockets::close(sockfd);
}

void testConnector() {
	EventLoop loop;
	InetAddress addr(2000,1);

	Acceptor accp(&loop, addr, true);
	accp.setNewConnectionCallback(newConnection);
	accp.listen();

	std::shared_ptr<Connector> conn(new Connector(&loop, addr));
	conn->setNewConnectionCallback(connectCallback);
	conn->start();

	loop.runAfter(3, [&] {
		loop.quit();
	});
	loop.loop();
}

void testTcpConnection() {
	EventLoop loop;
	int s = sockets::createNonblockingOrDie(AF_INET);
	
	struct addrinfo* addr;
	sockets::getAddrInfo(&addr, "www.baidu.com", "80", AF_INET);
	InetAddress local(2000);
	InetAddress peer((sockaddr_in&)(addr->ai_addr));
	std::shared_ptr<TcpConnection> tcpConn(new TcpConnection(&loop, "tcpConn", s, local, peer));
	
	tcpConn->setConnectionCallback(nullptr);
	tcpConn->connectEstablished();
	tcpConn->connectDestroyed();
	tcpConn->forceCloseWithDelay(0);

	loop.runAfter(3, [&] {
		loop.quit();
	});
	loop.loop();
}

void testTcpClient1() {
	EventLoop loop;
	InetAddress serverAddr("127.0.0.1", 2); // no such server
	net::TcpClient client(&loop, serverAddr, "TcpClient");
	
	loop.runAfter(0.0, [&] {
		LOG_INFO << "timeout";
		client.stop();
	});
	loop.runAfter(5.0, std::bind(&EventLoop::quit, &loop));
	client.connect();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	loop.loop();
}

void testTcpClient2() {
	InetAddress serverAddr("127.0.0.1", 1234); // should succeed
	EventLoop loop;
	Acceptor accp(&loop, serverAddr, true);
	accp.setNewConnectionCallback(newConnection);
	accp.listen();

	loop.runAfter(3.0, std::bind(&EventLoop::quit, &loop));
	Thread thr([&] {
		TcpClient client(&loop, serverAddr, "TcpClient");
		client.connect();
		std::this_thread::sleep_for(std::chrono::seconds(1));
		client.disconnect();
	});
	thr.start();
	loop.loop();
}

void testTcpServer() {
}
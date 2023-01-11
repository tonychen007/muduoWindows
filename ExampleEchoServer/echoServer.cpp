#include "net/tcpServer.h"
#include "base/logging.h"
#include "base/thread.h"
#include "net/eventLoop.h"
#include "net/inetAddress.h"

using namespace net;

int numThreads = 4;

class EchoServer {
public:
    EchoServer(EventLoop* loop, const InetAddress& listenAddr)
        : loop_(loop),
        server_(loop, listenAddr, "EchoServer") {
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, _1));
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this, _1, _2, _3));
        server_.setThreadNum(numThreads);
    }

    void start() {
        server_.start();
    }
    // void stop();

private:
    void onConnection(const TcpConnectionPtr& conn) {
        LOG_TRACE << conn->peerAddress().toIpPort() << " -> "
            << conn->localAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
        LOG_INFO << conn->getTcpInfoString();

        conn->send("hello\n");
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
        string msg(buf->retrieveAllAsString());
        LOG_INFO << conn->name() << " recv " << msg.size() << " bytes at " << time.toString();
        if (msg == "exit\n") {
            conn->send("bye\n");
            conn->shutdown();
        }
        if (msg == "quit\n") {
            loop_->quit();
        }
        conn->send(msg);
    }

    EventLoop* loop_;
    TcpServer server_;
};

int main() {
    sockets::InitSocket();

    LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
    LOG_INFO << "sizeof TcpConnection = " << sizeof(TcpConnection);

    EventLoop loop;
    InetAddress listenAddr(2000, true, 0);
    EchoServer server(&loop, listenAddr);

    server.start();
    loop.loop();

    sockets::DestorySocket();
}

#include "acceptor.h"

#include "base/logging.h"
#include "eventLoop.h"
#include "inetAddress.h"
#include "socketsOps.h"

#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>  

using namespace net;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
    : loop_(loop),
    acceptSocket_(sockets::createNonblockingOrDie(listenAddr.family())),
    acceptChannel_(loop, acceptSocket_.fd()),
    listening_(false),
    idleFd_(::_open("nul", O_RDONLY))
{
    assert(idleFd_ >= 0);
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::_close(idleFd_);
}

void Acceptor::listen() {
    loop_->assertInLoopThread();
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead() {
    loop_->assertInLoopThread();
    InetAddress peerAddr;
    //FIXME loop until no more
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0) {
        // string hostport = peerAddr.toIpPort();
        // LOG_TRACE << "Accepts of " << hostport;
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        } else {
            sockets::close(connfd);
        }
    } else {
        LOG_SYSERR << "in Acceptor::handleRead";
        // Read the section named "The special problem of
        // accept()ing when you can't" in libev's doc.
        // By Marc Lehmann, author of libev.
        if (errno == EMFILE) {
            ::_close(idleFd_);
            idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
            ::_close(idleFd_);
            idleFd_ = ::_open("nul", O_RDONLY);
        }
    }
}
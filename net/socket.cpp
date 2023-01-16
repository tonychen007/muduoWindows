#include "socket.h"
#include "base/logging.h"
#include "net/inetAddress.h"
#include "net/socketsOps.h"

using namespace net;

Socket::Socket(Socket&& sockfd)
	: sockfd_(std::move(sockfd.sockfd_)) {
}

Socket::~Socket() {
	sockets::close(sockfd_);
}

bool Socket::getTcpInfo(TCP_INFO_v0* tcpInfo) const {
	// 0 for TCP_INFO_v0
	DWORD ver = 0;
	DWORD bytesReturned = 0;

	return WSAIoctl(sockfd_, SIO_TCP_INFO, &ver, sizeof(ver), tcpInfo, sizeof(TCP_INFO_v0), &bytesReturned, nullptr, nullptr) == 0;
}

bool Socket::getTcpInfoString(char* buf, int len) const {
    /*https://learn.microsoft.com/en-us/windows/win32/api/mstcpip/ns-mstcpip-tcp_info_v1*/

    TCP_INFO_v0 tcpi;
    bool ok = getTcpInfo(&tcpi);
    if (ok) {
        snprintf(buf, len,
            "rto=%u\nsnd_wnd=%u\nrcv_wnd=%u\n"
            "rtt=%u\ncwnd=%u\ntotal_retrans=%u\n",
            /*
            tcpi.tcpi_rto,          // Retransmit timeout in usec
            tcpi.tcpi_snd_mss,
            tcpi.tcpi_rcv_mss,
            tcpi.tcpi_rtt,          // Smoothed round trip time in usec
            tcpi.tcpi_snd_cwnd,
            tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
            */
            tcpi.TimeoutEpisodes,
            tcpi.SndWnd,
            tcpi.RcvWnd,
            tcpi.RttUs,
            tcpi.Cwnd,
            tcpi.BytesRetrans);
    }

    return ok;
}

void Socket::bindAddress(const InetAddress& addr) {
    sockets::bindOrDie(sockfd_, addr.getSockAddr());
}

void Socket::listen() {
    sockets::listenOrDie(sockfd_);
}

int Socket::accept(InetAddress* peeraddr) {
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof addr);
    int connfd = sockets::accept(sockfd_, &addr);
    if (connfd >= 0) {
        peeraddr->setSockAddrInet6(addr);
    }

    return connfd;
}

void Socket::shutdownWrite() {
    sockets::shutdownWrite(sockfd_);
}

void Socket::setTcpNoDelay(bool on) {
    char optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));
    // FIXME CHECK
}

void Socket::setReuseAddr(bool on) {
    char optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
    // FIXME CHECK
}

void Socket::setReusePort(bool on) {
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
        &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on)
    {
        LOG_SYSERR << "SO_REUSEPORT failed.";
    }
#else
    if (on) {
        LOG_ERROR << "SO_REUSEPORT is not supported.";
    }
#endif
}

void Socket::setKeepAlive(bool on) {
    char optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof optval));
    // FIXME CHECK
}

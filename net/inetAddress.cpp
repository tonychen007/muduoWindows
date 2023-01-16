#include "types.h"
#include "inetAddress.h"
#include "endian.h"
#include "socketsOps.h"
#include "base/logging.h"

static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;

namespace net {

    //
    //		struct sockaddr_in {
    //			ADDRESS_FAMILY sin_family;
    //			USHORT sin_port;
    //			IN_ADDR sin_addr;
    //			CHAR sin_zero[8];
    //		};
    //
    //
    //
    //		struct in_addr {
    //			union {
    //				struct { UCHAR s_b1, s_b2, s_b3, s_b4; } S_un_b;
    //				struct { USHORT s_w1, s_w2; } S_un_w;
    //				ULONG S_addr;
    //			} S_un;
    //		#define s_addr  S_un.S_addr /* can be used for most tcp & ip code */
    //		#define s_host  S_un.S_un_b.s_b2    // host on imp
    //		#define s_net   S_un.S_un_b.s_b1    // network
    //		#define s_imp   S_un.S_un_w.s_w2    // imp
    //		#define s_impno S_un.S_un_b.s_b4    // imp #
    //		#define s_lh    S_un.S_un_b.s_b3    // logical host
    //		} IN_ADDR, * PIN_ADDR, FAR* LPIN_ADDR;
    //

    static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in6), "InetAddress is same size as sockaddr_in6");
    static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
    static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
    static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
    static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");

    static __thread char t_resolveBuffer[64 * 1024];

    InetAddress::InetAddress(uint16_t portArg, bool loopbackOnly, bool ipv6) {
        static_assert(offsetof(InetAddress, addr6_) == 0, "addr6_ offset 0");
        static_assert(offsetof(InetAddress, addr_) == 0, "addr_ offset 0");

        if (ipv6) {
            memset(&addr6_, 0, sizeof addr6_);
            addr6_.sin6_family = AF_INET6;
            in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
            addr6_.sin6_addr = ip;
            addr6_.sin6_port = sockets::hostToNetwork16(portArg);
        }
        else {
            memset(&addr_, 0, sizeof addr_);
            addr_.sin_family = AF_INET;
            in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
            addr_.sin_addr.s_addr = sockets::hostToNetwork32(ip);
            addr_.sin_port = sockets::hostToNetwork16(portArg);
        }
    }

    InetAddress::InetAddress(StringArg ip, uint16_t portArg, bool ipv6) {
        if (ipv6 || strchr(ip.c_str(), ':')) {
            memset(&addr6_, 0, sizeof addr6_);
            sockets::fromIpPort(ip.c_str(), portArg, &addr6_);
        }
        else {
            memset(&addr_, 0, sizeof addr_);
            sockets::fromIpPort(ip.c_str(), portArg, &addr_);
        }
    }

    string InetAddress::toIpPort() const {
        char buf[64] = "";
        sockets::toIpPort(buf, sizeof buf, getSockAddr());
        return buf;
    }

    string InetAddress::toIp() const {
        char buf[64] = "";
        sockets::toIp(buf, sizeof buf, getSockAddr());
        return buf;
    }

    uint32_t InetAddress::ipv4NetEndian() const {
        assert(family() == AF_INET);
        return addr_.sin_addr.s_addr;
    }

    uint16_t InetAddress::port() const {
        return sockets::networkToHost16(portNetEndian());
    }

    bool InetAddress::resolve(StringArg hostname, InetAddress* out) {
        assert(out != NULL);
        struct hostent* he = NULL;

        he = gethostbyname(hostname.c_str());
        if (he != NULL) {
            assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
            out->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
            return true;
        }
        else {
            LOG_SYSERR << "InetAddress::resolve";
            return false;
        }
    }

    void InetAddress::setScopeId(uint32_t scope_id) {
        if (family() == AF_INET6) {
            addr6_.sin6_scope_id = scope_id;
        }
    }
}
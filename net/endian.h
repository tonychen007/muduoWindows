#pragma once

#include <inttypes.h>
#include <WinSock2.h>

namespace sockets {
	inline uint64_t hostToNetwork64(uint64_t host64) {
		return htonll(host64);
	}

	inline uint32_t hostToNetwork32(uint32_t host32) {
		return htonl(host32);
	}

	inline uint16_t hostToNetwork16(uint16_t host16) {
		return htons(host16);
	}

	inline uint64_t networkToHost64(uint64_t net64) {
		return ntohll(net64);
	}

	inline uint32_t networkToHost32(uint32_t net32) {
		return ntohl(net32);
	}

	inline uint16_t networkToHost16(uint16_t net16) {
		return ntohs(net16);
	}
}
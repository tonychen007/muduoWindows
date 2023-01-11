#pragma once

#define FD_SETSIZE 2048

#include <ws2tcpip.h>
#include <inaddr.h>
#include <assert.h>
#include <stdint.h>

#define iovec WSABUF

typedef size_t ssize_t;
typedef unsigned short sa_family_t;
typedef ULONG in_addr_t;

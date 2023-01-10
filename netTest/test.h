#pragma once

#include "base/logging.h"
#include "net/socketsOps.h"
#include "net/inetAddress.h"
#include "net/socket.h"
#include <Windows.h>

void testSocketOps();
void testInetAddress();
void testSocketClient();
void testSocketServer();
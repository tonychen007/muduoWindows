#pragma once

#include "net/socketsOps.h"
#include "base/logging.h"
#include "base/Thread.h"
#include "net/inetAddress.h"
#include "net/socket.h"
#include "net/timer.h"
#include "net/timerId.h"
#include "net/poller.h"
#include "net/channel.h"
#include "net/eventLoop.h"
#include <Windows.h>

void testSocketOps();
void testInetAddress();
void testSocketClient();
void testSocketServer();
void testTimer();
void testEventloop();

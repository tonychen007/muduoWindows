#include "poller.h"
#include "selectPoller.h"

using namespace net;

Poller* Poller::newDefaultPoller(EventLoop* loop) {
	return new SelectPoller(loop);
}
#include "poller.h"
#include "selectPoller.h"
#include "epollPoller.h"

using namespace net;

Poller* Poller::newDefaultPoller(EventLoop* loop) {
	//return new SelectPoller(loop);
	return new EPollPoller(loop);
}
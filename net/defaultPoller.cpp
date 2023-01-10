#include "poller.h"
#include "selectPoller.h"

using namespace net;

Poller* Poller::newDefaultPoller(EventLoop* loop) {
	if (1) {
		return new SelectPoller(loop);
	}
}
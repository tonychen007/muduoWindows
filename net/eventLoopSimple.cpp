#include "eventLoopSimple.h"
#include "channel.h"
#include "poller.h"
#include "timerQueueWin.h"

#include "base/logging.h"

#include <assert.h>

using namespace net;

__thread EventLoopSimple* t_loopInThisThread = 0;
const int kPollTimeMs = 10000;

EventLoopSimple::EventLoopSimple()
    : looping_(false),
    quit_(false),
    threadId_(CurrentThread::tid()),
    poller_(Poller::newDefaultPoller(this)),
    timerQueue_(new TimerQueueWin(this))
{
    LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
    if (t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
            << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }
}

EventLoop::~EventLoop()
{
    assert(!looping_);
    t_loopInThisThread = NULL;
}
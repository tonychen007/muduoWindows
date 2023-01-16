
#include "base/logging.h"
#include "eventLoop.h"
#include "timer.h"
#include "timerId.h"
#include "TimerQueueWin.h"

#include <iterator>

using namespace net;

VOID CALLBACK TimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired) {
    printf("Timer callback\n");
}

HANDLE createTimerfd() {
    return CreateTimerQueue();
}

struct timespec howMuchTimeFromNow(Timestamp when) {
    int64_t microseconds = when.microSecondsSinceEpoch()
        - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100) {
        microseconds = 100;
    }

    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);

    return ts;
}

void resetTimerfd(HANDLE hTimerQueue, Timestamp expiration) {
    HANDLE hTimer;
    timespec tm = howMuchTimeFromNow(expiration);
    if (!CreateTimerQueueTimer(&hTimer, hTimerQueue,
        (WAITORTIMERCALLBACK)TimerRoutine, nullptr, tm.tv_sec * 1000, 0, 0)) {

        LOG_SYSERR << "timerfd_settime()";
    }
}

TimerQueueWin::TimerQueueWin(EventLoop* loop)
    : loop_(loop),
    hTimerQueue_(createTimerfd()),
    timerfdChannel_(loop, (int)hTimerQueue_),
    timers_()
{
    timerfdChannel_.setReadCallback(
        std::bind(&TimerQueueWin::handleRead, this));
    // we are always reading the timerfd, we disarm it with timerfd_settime.
    timerfdChannel_.enableReading();
}

TimerQueueWin::~TimerQueueWin()
{
    //DeleteTimerQueue(hTimerQueue_);
    //::CloseHandle(hTimerQueue_);
    //::CloseHandle(hTimer_);
    // do not remove channel, since we're in EventLoop::dtor();
    for (TimerList::iterator it = timers_.begin();
        it != timers_.end(); ++it)
    {
        delete it->second;
    }
}
TimerId TimerQueueWin::addTimer(const TimerCallback& cb,
    Timestamp when,
    double interval)
{
    Timer* timer = new Timer(cb, when, interval);
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);

    if (earliestChanged)
    {
        resetTimerfd(hTimerQueue_ , timer->expiration());
    }

    return TimerId(timer, 0);
}

void TimerQueueWin::handleRead()
{
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    //readTimerfd(timerfd_, now);

    std::vector<Entry> expired = getExpired(now);

    // safe to callback outside critical section
    for (std::vector<Entry>::iterator it = expired.begin();
        it != expired.end(); ++it)
    {
        it->second->run();
    }

    reset(expired, now);
}

std::vector<TimerQueueWin::Entry> TimerQueueWin::getExpired(Timestamp now)
{
    std::vector<Entry> expired;
    Entry sentry = std::make_pair(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    TimerList::iterator it = timers_.lower_bound(sentry);
    assert(it == timers_.end() || now < it->first);
    std::copy(timers_.begin(), it, back_inserter(expired));
    timers_.erase(timers_.begin(), it);

    return expired;
}

void TimerQueueWin::reset(const std::vector<Entry>& expired, Timestamp now)
{
    Timestamp nextExpire;

    for (std::vector<Entry>::const_iterator it = expired.begin();
        it != expired.end(); ++it)
    {
        if (it->second->repeat())
        {
            it->second->restart(now);
            insert(it->second);
        }
        else
        {
            // FIXME move to a free list
            delete it->second;
        }
    }

    if (!timers_.empty())
    {
        nextExpire = timers_.begin()->second->expiration();
    }

    if (nextExpire.valid())
    {
        resetTimerfd(hTimerQueue_, nextExpire);
    }
}

bool TimerQueueWin::insert(Timer* timer)
{
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if (it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }
    std::pair<TimerList::iterator, bool> result =
        timers_.insert(std::make_pair(when, timer));
    assert(result.second);
    return earliestChanged;
}
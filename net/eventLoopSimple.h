#pragma once

#include <vector>
#include "base/noncopyable.h"
#include "base/timestamp.h"
#include "base/thread.h"
#include "callbacks.h"
#include "timerId.h"

class Channel;
class Poller;
class TimerQueueWin;

namespace net {
    class EventLoopSimple : noncopyable
    {
    public:

        EventLoopSimple();

        // force out-line dtor, for scoped_ptr members.
        ~EventLoopSimple();

        ///
        /// Loops forever.
        ///
        /// Must be called in the same thread as creation of the object.
        ///
        void loop();

        void quit();

        ///
        /// Time when poll returns, usually means data arrivial.
        ///
        Timestamp pollReturnTime() const { return pollReturnTime_; }

        // timers

        ///
        /// Runs callback at 'time'.
        ///
        TimerId runAt(const Timestamp& time, const TimerCallback& cb);
        ///
        /// Runs callback after @c delay seconds.
        ///
        TimerId runAfter(double delay, const TimerCallback& cb);
        ///
        /// Runs callback every @c interval seconds.
        ///
        TimerId runEvery(double interval, const TimerCallback& cb);

        // void cancel(TimerId timerId);

        // internal use only
        void updateChannel(Channel* channel);
        // void removeChannel(Channel* channel);

        void assertInLoopThread()
        {
            if (!isInLoopThread())
            {
                abortNotInLoopThread();
            }
        }

        bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    private:

        void abortNotInLoopThread();

        typedef std::vector<Channel*> ChannelList;

        bool looping_; /* atomic */
        bool quit_; /* atomic */
        const int64_t threadId_;
        Timestamp pollReturnTime_;
        std::unique_ptr<Poller> poller_;
        std::unique_ptr<TimerQueueWin> timerQueue_;
        ChannelList activeChannels_;
    };
}
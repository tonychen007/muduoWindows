#include "countDownLatch.h"
#include <memory>

CountDownLatch::CountDownLatch(int count)
	: mutex_(),
	  condition_(mutex_),
	  count_(count) {

}

void CountDownLatch::wait() {
	std::unique_lock<std::mutex> l = condition_.getUniqueLock();
	while (count_ > 0)
		condition_.wait(l);
}

void CountDownLatch::countDown() {
	MutexLockGuard l(mutex_);
	--count_;
	if (count_ == 0)
		condition_.notifyAll();
}

int CountDownLatch::getCount() const {
	MutexLockGuard l(mutex_);
	return count_;
}

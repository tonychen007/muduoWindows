#include "test.h"

int getThreadId(std::thread& th) {
	std::thread::id id = th.get_id();
	return *reinterpret_cast<int*>(&id);
}

HANDLE GetThreadById(DWORD id) {
	return OpenThread(THREAD_ALL_ACCESS, TRUE, id);
}

void testMutex(int kThread) {
	int kCount = 1000 * 1000 * 10;
	vector<unique_ptr<Thread>> ths;
	vector<int> vec;
	MutexLock m;
	
	Timestamp start(Timestamp::now());
	for (int i = 0; i < kThread; i++) {
		Thread* th = new Thread([&] {
				for (int i = 0; i < kCount; ++i) {
					MutexLockGuard lock(m);
					vec.push_back(i);
				}
			});
		ths.emplace_back(th);

	}

	for (int i = 0; i < kThread; i++) {
		ths[i].get()->start();
	}

	for (int i = 0; i < kThread; i++) {
		ths[i].get()->join();
	}
	
	Timestamp end(Timestamp::now());
	double time = timeDifference(end, start);
	printf("Total time for %d threads insert %d items with lock is %fs\n", kThread, kCount, time);
}

void testMutexHelper() {
	testMutex(1);
	testMutex(2);
	testMutex(4);
	testMutex(8);
}

void testCurrentThread() {
	vector<unique_ptr<Thread>> ths;
	int kThread = 10;
	HANDLE hEv = CreateEvent(NULL, FALSE, FALSE, NULL);

	int tid = CurrentThread::tid();
	CurrentThread::setName("Main");
	CurrentThread::setThreadName(tid, CurrentThread::name());
	printf("Before thread:\n");
	printf("[id:%d]-Name:%s\n", CurrentThread::tid(), CurrentThread::name());


	for (int i = 0; i < kThread; i++) {
		Thread* th = new Thread([&] {
			int tid = CurrentThread::tid();
			printf("[id:%d]-Name:%s\n", CurrentThread::tid(), CurrentThread::name());
			Sleep(5000);
		});
		ths.emplace_back(th);
	}

	for (int i = 0; i < kThread; i++) {
		ths[i].get()->start();
	}

	Sleep(1000);
	// break here, and watch through [thread] module
	printf("After thread:\n");
	printf("[id:%d]-Name:%s\n", CurrentThread::tid(), CurrentThread::name());

	for (int i = 0; i < kThread; i++) {
		ths[i].get()->join();
	}
}

void testCondition() {
	vector<int> vec;
	MutexLock m;
	Condition cond(m);
	volatile int isShutdown = 0;
	
	Thread takeTh([&] {
		while (1) {
			uniqueLock lock = cond.getUniqueLock();
			while (vec.empty())
				cond.wait(lock);

			Sleep(100);
			int v = vec.back();
			vec.pop_back();
			printf("pop back: %d\n", v);
			if (isShutdown) break;
		}
	}, "Take Thread");

	Thread putTh1([&] {
		while (1) {
			uniqueLock lock = cond.getUniqueLock();
			cond.waitfor(lock, 10);
			vec.push_back(rand() % 32767);
			cond.notify();
			if (isShutdown) break;
		}
	}, "Put Thread1");

	Thread putTh2([&] {
		while (1) {
			uniqueLock lock = cond.getUniqueLock();
			cond.waitfor(lock, 5);
			vec.push_back(rand() % 32767);
			cond.notify();
			if (isShutdown) break;
		}
	}, "Put Thread2");
	
	takeTh.start();
	putTh1.start();
	putTh2.start();

	Sleep(5000);
	isShutdown = 1;
	
	takeTh.join();
	putTh1.join();
	putTh2.join();

	Sleep(1000);
	printf("Remain...\n");
	while (!vec.empty()) {
		int v = vec.back();
		vec.pop_back();
		printf("pop back: %d\n", v);
	}

	printf("All is done.\n");
	getchar();
}

void foo() {
	printf("foo\n");
}

void fooException() {
	printf("fooException\n");
	throw Exception("fooException");
}

void testThread() {
	Thread th(foo);
	th.start();
	th.join();
}

void testThreadException() {
	Thread th(fooException);
	th.start();
	th.join();
}

void printString(const std::string& str) {
	stringstream ss;
	Sleep(100);
}

void testThreadpool(int num) {
	Threadpool pool("MainThreadPool");
	pool.setMaxQueueSize(num);
	
	Timestamp start(Timestamp::now());
	pool.start(10);
	for (int i = 0; i < 100; ++i) {
		char buf[32];
		snprintf(buf, sizeof buf, "task %d", i);
		pool.run(std::bind(printString, std::string(buf)));
	}

	CountDownLatch latch(1);
	pool.run(std::bind(&CountDownLatch::countDown, &latch));
	latch.wait();
	pool.stop();

	Timestamp end(Timestamp::now());
	double time = timeDifference(end, start);
	printf("Total time for threadpool with %d threads is: %fs\n", num, time);
}

void testThreadpoolHelper() {
	testThreadpool(0);
	testThreadpool(1);
	testThreadpool(5);
	testThreadpool(10);
	testThreadpool(50);
	testThreadpool(100);
}

void testTimestamp() {
	Timestamp ts;
	ts = ts.now();
	printf("%s\n",ts.toString().c_str());
	printf("%s\n", ts.toFormattedString().c_str());
}

void testBlockingQueue() {
	const int kThread = 10;
	const int kCount = 100000;
	CountDownLatch latch(kThread);
	BlockingQueue<Timestamp> queue;
	BlockingQueue<int> delayQueue;
	vector<unique_ptr<Thread>> threads;

	for (int i = 0; i < kThread; i++) {
		char name[32];
		snprintf(name, sizeof name, "work thread %d", i);

		threads.emplace_back(new Thread([&] {
			printf("tid=%d, %s started\n", CurrentThread::tid(), CurrentThread::name());
			
			latch.countDown();
			std::map<int, int> delays;
			bool running = true;

			while (running) {
				Timestamp t(queue.take());
				Timestamp now(Timestamp::now());

				if (t.valid()) {
					int delay = static_cast<int>(timeDifference(now, t) * 1000000);
					++delays[delay];
					delayQueue.put(delay);
				}
				running = t.valid();
			}
		
			printf("tid=%d, %s stopped\n", CurrentThread::tid(), CurrentThread::name());
			for (const auto& delay : delays) {
				printf("tid = %d, delay = %d, count = %d\n", CurrentThread::tid(), delay.first, delay.second);
			}

		},name));
	}

	for (auto& thr : threads) {
		thr->start();
	}

	latch.wait();
	int64_t totalDelay = 0;
	for (int i = 0; i < kCount; ++i) {
		Timestamp now(Timestamp::now());
		queue.put(now);
		totalDelay += delayQueue.take();
	}

	// join threads
	for (size_t i = 0; i < threads.size(); ++i) {
		queue.put(Timestamp::invalid());
	}
	for (auto& thr : threads) {
		thr->join();
	}

	printf("Average delay: %.3fus\n", static_cast<double>(totalDelay) / kCount);
}

void testBlockingQueue2(int kThread) {
	CountDownLatch latch(kThread);
	vector<unique_ptr<BlockingQueue<int>>> queue;
	BlockingQueue<pair<int, Timestamp>> timequeue;
	vector<unique_ptr<Thread>> threads;

	for (int i = 0; i < kThread; ++i) {
		queue.emplace_back(new BlockingQueue<int>());
		
		char name[32];
		snprintf(name, sizeof name, "work thread %d", i);
		threads.emplace_back(new Thread([&,i] {
			latch.countDown();

			BlockingQueue<int>* input = queue[i].get();
			BlockingQueue<int>* output = queue[(i+1) % queue.size()].get();
			while (true) {
				int value = input->take();
				if (value > 0) {
					output->put(std::move(value - 1));
					//printf("thread %d, got %d\n", i, value);
					continue;
				}

				if (value == 0) {
					timequeue.put(std::make_pair(i, Timestamp::now()));
				}
				break;
			}
		}, name));
	}

	// start
	Timestamp start = Timestamp::now();
	for (auto& thr : threads) {
		thr->start();
	}

	latch.wait();
	Timestamp started = Timestamp::now();
	printf("all %zd threads started, %.3fms\n", threads.size(), 1e3 * timeDifference(started, start));

	// run
	start = Timestamp::now();
	const int rounds = 10003;
	queue[0]->put(rounds);

	auto done = timequeue.take();
	double elapsed = timeDifference(done.second, start);
	printf("thread id=%d done, total %.3fms, %.3fus / round\n", done.first, 1e3 * elapsed, 1e6 * elapsed / rounds);

	// join threads
	Timestamp stop = Timestamp::now();
	for (const auto& queue : queue) {
		queue->put(-1);
	}
	for (auto& thr : threads) {
		thr->join();
	}
	Timestamp t2 = Timestamp::now();
	printf("all %zd threads joined, %.3fms\n", threads.size(), 1e3 * timeDifference(t2, stop));	
}

void testBlockingQueue2Helper() {
	testBlockingQueue2(1);
	printf("\n");
	testBlockingQueue2(4);
	printf("\n");
	testBlockingQueue2(8);
	printf("\n");
	testBlockingQueue2(16);
	printf("\n");
}
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
	vector<unique_ptr<ThreadWrapper>> ths;
	vector<int> vec;
	MutexLock m;

	//auto st = system_clock::now();
	Timestamp start(Timestamp::now());
	for (int i = 0; i < kThread; i++) {
		ThreadWrapper* th = new ThreadWrapper([&] {
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
	vector<unique_ptr<ThreadWrapper>> ths;
	int kThread = 10;
	HANDLE hEv = CreateEvent(NULL, FALSE, FALSE, NULL);

	int tid = CurrentThread::tid();
	CurrentThread::setName("Main");
	CurrentThread::setThreadName(tid, CurrentThread::name());
	printf("Before thread:\n");
	printf("[id:%d]-Name:%s\n", CurrentThread::tid(), CurrentThread::name());


	for (int i = 0; i < kThread; i++) {
		ThreadWrapper* th = new ThreadWrapper([&] {
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
	
	ThreadWrapper takeTh([&] {
		while (1) {
			std::unique_lock<std::mutex> lock = cond.getUniqueLock();
			while (vec.empty())
				cond.wait(lock);

			Sleep(100);
			int v = vec.back();
			vec.pop_back();
			printf("pop back: %d\n", v);
			if (isShutdown) break;
		}
	}, "Take Thread");

	ThreadWrapper putTh1([&] {
		while (1) {
			std::unique_lock<std::mutex> lock = cond.getUniqueLock();
			cond.waitfor(lock, 10);
			vec.push_back(rand() % 32767);
			cond.notify();
			if (isShutdown) break;
		}
	}, "Put Thread1");

	ThreadWrapper putTh2([&] {
		while (1) {
			std::unique_lock<std::mutex> lock = cond.getUniqueLock();
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
	ThreadWrapper th(foo);
	th.start();
	th.join();
}

void testThreadException() {
	ThreadWrapper th(fooException);
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
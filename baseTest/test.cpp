#include "test.h"

int64_t g_total;
FILE* g_file;

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

void printString(const string& str) {
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
		pool.run(std::bind(printString, string(buf)));
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

void testBoundedBlockingQueue() {
	const int kThread = 8;
	const int kCount = 10000;
	BoundedBlockingQueue<const char*> q(20);
	CountDownLatch latch(kThread);
	vector<unique_ptr<Thread>> threads;
	
	for (int i = 0; i < kThread; i++) {
		char name[32];
		snprintf(name, sizeof name, "work thread %d", i);
		threads.emplace_back(new Thread([&] {
			printf("tid=%d, %s started\n", CurrentThread::tid(), CurrentThread::name());
			latch.countDown();
			bool running = true;
			while (running) {
				string d(q.take());
				printf("tid=%d, get data = %s, size = %zd\n", CurrentThread::tid(), d.c_str(), q.size());
				running = (d != "stop");
			}
			printf("tid=%d, %s stopped\n", CurrentThread::tid(), CurrentThread::name());
		}, name));
	}
	for (auto& thr : threads) {
		thr->start();
	}

	latch.wait();
	for (int i = 0; i < kCount; ++i) {
		char buf[32];
		snprintf(buf, sizeof buf, "hello %d", i);
		q.put(buf);
		printf("tid=%d, put data = %s, size = %zd\n", CurrentThread::tid(), buf, q.size());
	}

	for (size_t i = 0; i < threads.size(); ++i) {
		q.put("stop");
	}
	for (auto& thr : threads) {
		thr->join();
	}

	printf("number of created threads %lld\n", Thread::numCreated());
}

void testAtomic() {
	AtomicInt a0;

	assert(a0.get() == 0);
	assert(a0.getAndAdd(1) == 0);
	assert(a0.get() == 1);
	assert(a0.addAndGet(2) == 3);
	assert(a0.get() == 3);
	assert(a0.incrAndGet() == 4);
	assert(a0.get() == 4);
	a0.incr();
	assert(a0.get() == 5);
	assert(a0.addAndGet(-3) == 2);
	assert(a0.getAndSet(100) == 2);
	assert(a0.get() == 100);
}

void testSingleton() {
	struct Test {
		Test() {
			printf("tid=%d, constructing %p\n", CurrentThread::tid(), this);
		}

		~Test() {
			printf("tid=%d, destructing %p %s\n", CurrentThread::tid(), this, name_.c_str());
		}

		const string& name() const { return name_; }
		void setName(const string& n) { name_ = n; }

		string name_;
	};

	struct TestNoDestroy {
	public:
		void no_destroy();
		TestNoDestroy() {
			printf("tid=%d, constructing TestNoDestroy %p\n", CurrentThread::tid(), this);
		}

		~TestNoDestroy() {
			printf("tid=%d, destructing TestNoDestroy %p\n", CurrentThread::tid(), this);
		}
	};

	Singleton<Test>::instance().setName("only one");
	Thread t1([&] {
		printf("tid=%d, %p name=%s\n",
			CurrentThread::tid(),
			&Singleton<Test>::instance(),
			Singleton<Test>::instance().name().c_str());
		Singleton<Test>::instance().setName("only one, changed");
	});
	t1.start();
	t1.join();
	printf("tid=%d, %p name=%s\n", CurrentThread::tid(), &Singleton<Test>::instance(), Singleton<Test>::instance().name().c_str());
}

void testWeakCallback() {
	struct Foo {
		void bar() {
			printf("Foo::bar\n");
		}
	};

	shared_ptr<Foo> sf(new Foo());
	makeWeakCallback(sf, &Foo::bar)();
}

void testDate() {
	const int kMonthsOfYear = 12;

	struct Tool {
		int isLeapYear(int year) {
			if (year % 400 == 0)
				return 1;
			else if (year % 100 == 0)
				return 0;
			else if (year % 4 == 0)
				return 1;
			else
				return 0;
		}

		int daysOfMonth(int year, int month) {
			static int days[2][kMonthsOfYear + 1] =
			{
			  { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
			  { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
			};
			return days[isLeapYear(year)][month];
		}

		void passByConstReference(const Date& x) {
			printf("%s\n", x.toIsoString().c_str());
		}

		void passByValue(Date x) {
			printf("%s\n", x.toIsoString().c_str());
		}
	};

	Tool tool;

	time_t now = time(NULL);
	struct tm t1; 
	struct tm t2;
	gmtime_s(&t1, &now);
	localtime_s(&t2, &now);
	Date someDay(2008, 9, 10);
	printf("%s\n", someDay.toIsoString().c_str());
	tool.passByValue(someDay);
	tool.passByConstReference(someDay);
	Date todayUtc(t1);
	printf("%s\n", todayUtc.toIsoString().c_str());
	Date todayLocal(t2);
	printf("%s\n", todayLocal.toIsoString().c_str());

	int julianDayNumber = 2415021;
	int weekDay = 1; // Monday

	for (int year = 1900; year < 2100; ++year) {
		assert(Date(year, 3, 1).julianDayNumber() - Date(year, 2, 29).julianDayNumber()
			== tool.isLeapYear(year));
		for (int month = 1; month <= kMonthsOfYear; ++month) {
			for (int day = 1; day <= tool.daysOfMonth(year, month); ++day) {
				Date d(year, month, day);
				// printf("%s %d\n", d.toString().c_str(), d.weekDay());
				assert(year == d.year());
				assert(month == d.month());
				assert(day == d.day());
				assert(weekDay == d.weekDay());
				assert(julianDayNumber == d.julianDayNumber());

				Date d2(julianDayNumber);
				assert(year == d2.year());
				assert(month == d2.month());
				assert(day == d2.day());
				assert(weekDay == d2.weekDay());
				assert(julianDayNumber == d2.julianDayNumber());

				++julianDayNumber;
				weekDay = (weekDay + 1) % 7;
			}
		}
	}
	printf("All is passed\n");
}

void testReadFile() {
	const char* filename = "../base/Date.cpp";
	const char* buff;
	string content;
	int64_t fsize;
	int64_t mtime;
	int64_t ctime;
	int64_t bys;

	ReadSmallFile sm(filename);
	sm.readToBuffer(nullptr);
	buff = sm.buffer();

	readFile(filename, 10, &content, &fsize, &mtime, &ctime);
	printf("read %d, file size is %zd\n", 10, fsize);
	buff = sm.buffer();

	readFile(filename, 1024, &content, &fsize, &mtime, &ctime);
	printf("read %d, file size is %zd\n", 1024, fsize);
	buff = sm.buffer();

	readFile(filename, 102400, &content, &fsize, &mtime, &ctime);
	printf("read %d, file size is %zd\n", 102400, fsize);
	buff = sm.buffer();
	
	AppendFile ap("z:/1.txt");
	string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	for (int i = 0; i < 1000000; i++) {
		ap.append(line.c_str(), 10);
		bys = ap.writtenBytes();
		ap.append(line.c_str(), line.length());
		bys = ap.writtenBytes();
		ap.flush();
		bys = ap.writtenBytes();
	}

	buff = ap.buf();
	printf("buf is %s\n", buff);
}

void testProcessInfo() {
	CToolhelp th32;
	
	printf("pid=%d\n",ProcessInfo::pid());
	printf("pidString=%s\n",ProcessInfo::pidString().c_str());
	printf("username=%s\n",ProcessInfo::username().c_str());
	printf("sid=%s\n", ProcessInfo::sid().c_str());
	printf("page size=%d\n",ProcessInfo::pageSize());
	printf("clock tick per sec=%lld\n", ProcessInfo::clockTicksPerSecond());
	printf("hostname=%s\n", ProcessInfo::hostname().c_str());
	printf("procname=%s\n", ProcessInfo::procname().c_str());

	for (int i = 0; i < 100; i++) {
		ProcessInfo::procname();
	}
	ProcessInfo::CpuTime cpuTime =  ProcessInfo::cpuTime();
	printf("kernel time=%g\n", cpuTime.systemSeconds);
	printf("user time=%g\n", cpuTime.userSeconds);

	printf("threads=%d\n", ProcessInfo::numThreads());
	
	ProcessInfo::threadsInfo ths = ProcessInfo::threads();
	/*
	for (auto& th : ths) {
		printf("Thread id:%d--%s\n", th.first, th.second.c_str());
	}
	*/
	size_t files = 0;
	ProcessInfo::fileInfo fileInfo = ProcessInfo::openedFiles();
	for (auto& a : fileInfo) {
		files += a.second.size();
	}
	printf("opened files=%zd\n", files);
}


template<typename T, int N>
void benchPrintf(const char* fmt) {
	char buf[32];
	Timestamp start(Timestamp::now());
	for (size_t i = 0; i < N; ++i)
		snprintf(buf, sizeof buf, fmt, (T)(i));
	Timestamp end(Timestamp::now());

	printf("benchPrintf %f\n", timeDifference(end, start));
}

template<typename T, int N>
void benchStringStream() {
	Timestamp start(Timestamp::now());
	std::ostringstream os;

	for (size_t i = 0; i < N; ++i) {
		os << (T)(i);
		os.seekp(0, std::ios_base::beg);
	}
	Timestamp end(Timestamp::now());

	printf("benchStringStream %f\n", timeDifference(end, start));
}

template<typename T, int N>
void benchLogStream() {
	Timestamp start(Timestamp::now());
	LogStream os;
	for (size_t i = 0; i < N; ++i) {
		os << (T)(i);
		os.resetBuffer();
	}
	Timestamp end(Timestamp::now());

	printf("benchLogStream %f\n", timeDifference(end, start));
}

void testBuffer1() {
	LogStream log;
	const LogStream::Buffer& buffer = log.buffer();
	
	log << true;
	log << (uint16_t)1;
	log << (int16_t)2;
	log << (uint32_t)3;
	log << (int32_t)4;
	log << 5L;
	log << 6UL;
	log << 7LL;
	log << 8ULL;
	log << '2';
	assert(buffer.toString() == string("1123456782"));
	log.resetBuffer();

	log << 3.14f;
	log << 6.2812;
	printf("%s\n", buffer.toString().c_str());

	log.resetBuffer();
	log << "test";
	log << (unsigned char*)"tony";
	log << string("string");
	log << StringPiece("Piece");
	log << log.buffer();
	assert(buffer.toString() == string("testtonystringPiecetesttonystringPiece"));
	log.resetBuffer();
}

void testBuffer2() {
	LogStream os;
	const LogStream::Buffer& buffer = os.buffer();

	os << 0.0;
	printf("%s\n", buffer.toString().c_str());
	os.resetBuffer();

	os << 1.0;
	printf("%s\n", buffer.toString().c_str());
	os.resetBuffer();

	os << 0.1;
	printf("%s\n", buffer.toString().c_str());
	os.resetBuffer();

	os << 0.05;
	printf("%s\n", buffer.toString().c_str());
	os.resetBuffer();

	os << 0.15;
	printf("%s\n", buffer.toString().c_str());
	os.resetBuffer();

	double a = 0.1;
	os << a;
	printf("%s\n", buffer.toString().c_str());
	os.resetBuffer();

	double b = 0.05;
	os << b;
	printf("%s\n", buffer.toString().c_str());
	os.resetBuffer();

	double c = 0.15;
	os << c;
	printf("%s\n", buffer.toString().c_str());
	os.resetBuffer();

	os << a + b;
	printf("%s\n", buffer.toString().c_str());
	os.resetBuffer();

	os << 1.23456789;
	printf("%s\n", buffer.toString().c_str());
	os.resetBuffer();

	os << 1.234567;
	printf("%s\n", buffer.toString().c_str());
	os.resetBuffer();

	os << -123.456;
	printf("%s\n", buffer.toString().c_str());
	os.resetBuffer();

	os << (void*)8888;
	printf("%s\n", buffer.toString().c_str());
	os.resetBuffer();
}

void testBufferFmt() {
	LogStream os;
	const LogStream::Buffer& buffer = os.buffer();

	os << Fmt("%4d", 1);
	os << Fmt("%4.2f", 1.2) << Fmt("%4lld", 3LL);
	printf("%s\n", buffer.toString().c_str());
	os.resetBuffer();
}

void testBufferSIIEC() {
	assert(formatSI(0) == string("0"));
	assert(formatSI(999) == string("999"));
	assert(formatSI(1000) == string("1.00k"));
	assert(formatSI(9990) == string("9.99k"));
	assert(formatSI(9994) == string("9.99k"));
	assert(formatSI(9995) == string("10.0k"));
	assert(formatSI(10000) == string("10.0k"));
	assert(formatSI(10049) == string("10.0k"));
	assert(formatSI(10050) == string("10.1k"));
	assert(formatSI(99900) == string("99.9k"));
	assert(formatSI(99949) == string("99.9k"));
	assert(formatSI(99950) == string("100k"));
	assert(formatSI(100499) == string("100k"));
	assert(formatSI(100501) == string("101k"));
	assert(formatSI(100500) == string("101k"));
	assert(formatSI(999499) == string("999k"));
	assert(formatSI(999500) == string("1.00M"));
	assert(formatSI(1004999) == string("1.00M"));
	assert(formatSI(1005000) == string("1.01M"));
	assert(formatSI(1005001) == string("1.01M"));
	assert(formatSI(INT64_MAX) == string("9.22E"));

	assert(formatIEC(0) == string("0"));
	assert(formatIEC(1023) == string("1023"));
	assert(formatIEC(1024) == string("1.00Ki"));
	assert(formatIEC(10234) == string("9.99Ki"));
	assert(formatIEC(10235) == string("10.0Ki"));
	assert(formatIEC(10240) == string("10.0Ki"));
	assert(formatIEC(10291) == string("10.0Ki"));
	assert(formatIEC(10292) == string("10.1Ki"));
	assert(formatIEC(102348) == string("99.9Ki"));
	assert(formatIEC(102349) == string("100Ki"));
	assert(formatIEC(102912) == string("100Ki"));
	assert(formatIEC(102913) == string("101Ki"));
	assert(formatIEC(1022976) == string("999Ki"));
	assert(formatIEC(1047552) == string("1023Ki"));
	assert(formatIEC(1047961) == string("1023Ki"));
	assert(formatIEC(1048063) == string("1023Ki"));
	assert(formatIEC(1048064) == string("1.00Mi"));
	assert(formatIEC(1048576) == string("1.00Mi"));
	assert(formatIEC(10480517) == string("9.99Mi"));
	assert(formatIEC(10480518) == string("10.0Mi"));
	assert(formatIEC(INT64_MAX) == string("8.00Ei"));
}

void testBufferBench() {
	const int N = 5000000;

	puts("int");
	benchPrintf<int, N>("%d");
	benchStringStream<int, N>();
	benchLogStream<int, N>();

	puts("double");
	benchPrintf<double, N>("%.12g");
	benchStringStream<double, N>();
	benchLogStream<double, N>();

	puts("int64_t");
	benchPrintf<int64_t, N>("%" PRId64);
	benchStringStream<int64_t, N>();
	benchLogStream<int64_t, N>();

	puts("void*");
	benchPrintf<void*, N>("%p");
	benchStringStream<void*, N>();
	benchLogStream<void*, N>();
}

void dummyOutput(const char* msg, int len) {
	g_total += len;
	if (g_file) {
		fwrite(msg, 1, len, g_file);
	}
}

void logInThread() {
	LOG_INFO << "logInThread";
	Sleep(100);
}

void bench(const char* type, bool kLongLog = false) {
	Logger::setOutput(dummyOutput);
	Timestamp start(Timestamp::now());
	g_total = 0;

	int n = 100 * 100;
	string empty = " ";
	string longStr(3000, 'X');
	longStr += " ";
	for (int i = 0; i < n; ++i) {
		LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz"
			<< (kLongLog ? longStr : empty)
			<< i;
	}
	Timestamp end(Timestamp::now());
	double seconds = timeDifference(end, start);
	printf("%12s: %f seconds, %lld bytes, %10.2f msg/s, %.2f MiB/s\n",
		type, seconds, g_total, n / seconds, g_total / seconds / (1024 * 1024));
}

void testLogging() {
	_putenv_s("MUDUO_LOG_DEBUG", "1");
	g_logLevel = initLogLevel();
	char buffer[64 * 1024];
	
	Threadpool pool("pool");
	pool.start(5);
	pool.run(logInThread);
	pool.run(logInThread);
	pool.run(logInThread);
	pool.run(logInThread);
	pool.run(logInThread);

	LOG_TRACE << "trace";
	LOG_DEBUG << "debug";
	LOG_INFO << "Hello";
	LOG_WARN << "World";
	LOG_ERROR << "Error";
	LOG_INFO << sizeof(Logger);
	LOG_INFO << sizeof(LogStream);
	LOG_INFO << sizeof(Fmt);
	LOG_INFO << sizeof(LogStream::Buffer);
	Sleep(1000);

	fopen_s(&g_file, "z:/null", "w");
	setvbuf(g_file, buffer, _IOFBF, sizeof buffer);
	bench("z:/null");
	fclose(g_file);

	fopen_s(&g_file, "z:/null", "w");
	setvbuf(g_file, buffer, _IOFBF, sizeof buffer);
	bench("z:/null", true);
	fclose(g_file);

	g_file = stdout;
	Sleep(1000);
	LOG_TRACE << "trace CST";
	LOG_DEBUG << "debug CST";
	LOG_INFO << "Hello CST";
	LOG_WARN << "World CST";
	LOG_ERROR << "Error CST";
	
}
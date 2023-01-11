#pragma once

#include "noncopyable.h"
#include "types.h"
#include "mutexGuard.h"
#include <vector>

class AppendFile;

class LogFile : noncopyable {
public:
	LogFile(const string& basename,
		off_t rollSize,
		bool threadSafe = true,
		int flushInterval = 3,
		int checkEveryN = 1024);
	~LogFile();

	void append(const char* logline, int len);
	void flush();
	bool rollFile();
	std::vector<string> getLogFileNames() {
		return logFilenames_;
	}

	void deleteLogFiles();
private:
	void append_unlocked(const char* logline, int len);
	static string getLogFileName(const string& basename, time_t* now);
	const string basename_;
	const off_t rollSize_;
	const int flushInterval_;
	const int checkEveryN_;

	int count_;

	std::unique_ptr<MutexLock> mutex_;
	time_t startOfPeriod_;
	time_t lastRoll_;
	time_t lastFlush_;
	std::unique_ptr<AppendFile> file_;

	std::vector<string> logFilenames_;

	const static int kRollPerSeconds_ = 60 * 60 * 24;
};
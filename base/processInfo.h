#pragma once

#include <Windows.h>
#include <vector>
#include <map>
#include "types.h"
#include "timestamp.h"
#include "tool.h"


namespace ProcessInfo {
	using threadsInfo = std::vector<std::pair<int, string>>;
	using fileInfo = std::map<int, std::vector<std::string>>;

	int pid();
	string pidString();
	string sid();
	string username();
	Timestamp startTime();
	int64_t clockTicksPerSecond();
	int pageSize();
	bool isDebugBuild();

	string hostname();
	string procname();
	string procname(int id);

	struct CpuTime {
		double userSeconds;
		double systemSeconds;

		CpuTime() : userSeconds(0.0), systemSeconds(0.0) { }

		double total() const { return userSeconds + systemSeconds; }
	};

	CpuTime cpuTime();

	int numThreads();
	threadsInfo threads();
	fileInfo openedFiles(int getNamedPipe = 1, int timeout = 500, int print = 0);
	int getFileTypeIndex();
}
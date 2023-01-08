#include "base\logFile.h"
#include "test.h"

void foo() {
	LogFile logFile("1.log", 1024 * 1024);
	logFile.deleteLogFiles();
}
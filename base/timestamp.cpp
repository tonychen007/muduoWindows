#include "timestamp.h"
#include <time.h>
#include <chrono>
#include <inttypes.h>

using namespace std;

string Timestamp::toString() const {
	char buf[32] = { 0 };
	int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
	int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
	snprintf(buf, sizeof(buf), "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
	return buf;
}

string Timestamp::toFormattedString(bool showMicroseconds) const {
	char buf[64] = { 0 };
	time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
	struct tm tm_time;
	//gmtime_s(&tm_time, &seconds);
	localtime_s(&tm_time, &seconds);

	if (showMicroseconds) {
		int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
		snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d.%06d",
			tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
			tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
			microseconds);
	}
	else {
		snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d",
			tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
			tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
	}

	return buf;
}

Timestamp Timestamp::now() {
	auto time_now = std::chrono::system_clock::now();
	auto microsec = std::chrono::duration_cast<std::chrono::microseconds>(time_now.time_since_epoch()).count();
	
	return Timestamp(microsec);
}
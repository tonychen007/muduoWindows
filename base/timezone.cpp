#include "timezone.h"
#include "date.h"

const int kSecondsPerDay = 24 * 60 * 60;

inline void fillHMS(unsigned seconds, struct DateTime* dt) {
    dt->second = seconds % 60;
    unsigned minutes = seconds / 60;
    dt->minute = minutes % 60;
    dt->hour = minutes / 60;
}

DateTime BreakTime(int64_t t) {
    struct DateTime dt;
    int seconds = static_cast<int>(t % kSecondsPerDay);
    int days = static_cast<int>(t / kSecondsPerDay);
    // C++11 rounds towards zero.
    if (seconds < 0)
    {
        seconds += kSecondsPerDay;
        --days;
    }
    
    fillHMS(seconds, &dt);
    Date date(days + Date::kJulianDayOf1970_01_01);
    Date::YearMonthDay ymd = date.yearMonthDay();
    dt.year = ymd.year;
    dt.month = ymd.month;
    dt.day = ymd.day;

    return dt;
}

// gmtime(3)
struct DateTime TimeZone::toUtcTime(int64_t secondsSinceEpoch) {
	return BreakTime(secondsSinceEpoch);
}

// timegm(3)
int64_t TimeZone::fromUtcTime(const struct DateTime& dt) {
    Date date(dt.year, dt.month, dt.day);
    int secondsInDay = dt.hour * 3600 + dt.minute * 60 + dt.second;
    int64_t days = date.julianDayNumber() - Date::kJulianDayOf1970_01_01;
    return days * kSecondsPerDay + secondsInDay;
}
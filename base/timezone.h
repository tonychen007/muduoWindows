#pragma once

#include "types.h"

struct DateTime {
    DateTime() {}
    explicit DateTime(const struct tm&);
    DateTime(int _year, int _month, int _day, int _hour, int _minute, int _second)
        : year(_year), month(_month), day(_day), hour(_hour), minute(_minute), second(_second)
    {
    }

    // "2011-12-31 12:34:56"
    string toIsoString() const;

    int year = 0;     // [1900, 2500]
    int month = 0;    // [1, 12]
    int day = 0;      // [1, 31]
    int hour = 0;     // [0, 23]
    int minute = 0;   // [0, 59]
    int second = 0;   // [0, 59]
};


class TimeZone {
public:
    // gmtime(3)
    static struct DateTime toUtcTime(int64_t secondsSinceEpoch);
    // timegm(3)
    static int64_t fromUtcTime(const struct DateTime&);
};
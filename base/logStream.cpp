#include "logStream.h"
#include <assert.h>
#include <inttypes.h>

template<int SIZE>
void FixedBuffer<SIZE>::cookieStart() {}

template<int SIZE>
void FixedBuffer<SIZE>::cookieEnd() {}

template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;


const char digits[] = "9876543210123456789";
const char* zero = digits + 9;
static_assert(sizeof(digits) == 20, "wrong number of digits");

const char digitsHex[] = "0123456789ABCDEF";
static_assert(sizeof digitsHex == 17, "wrong number of digitsHex");

// Efficient Integer to String Conversions, by Matthew Wilson.
template<typename T>
size_t convert(char buf[], T value) {
    T i = value;
    char* p = buf;

    do {
        int lsd = static_cast<int>(i % 10);
        i /= 10;
        *p++ = zero[lsd];
    } while (i != 0);

    if (value < 0) {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}

size_t convertHex(char buf[], uintptr_t value) {
    uintptr_t i = value;
    char* p = buf;

    do
    {
        int lsd = static_cast<int>(i % 16);
        i /= 16;
        *p++ = digitsHex[lsd];
    } while (i != 0);

    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}


LogStream::self& LogStream::operator<<(bool v) {
    buffer_.append(v ? "1" : "0", 1);
    return *this;
}

LogStream::self& LogStream::operator<<(short v) {
    *this << static_cast<int>(v);
    return *this;
}

LogStream::self& LogStream::operator<<(unsigned short v) {
    *this << static_cast<unsigned int>(v);
    return *this;
}

LogStream::self& LogStream::operator<<(int v) {
    formatInteger(v);
    return *this;
}

LogStream::self& LogStream::operator<<(unsigned int v) {
    formatInteger(v);
    return *this;
}

LogStream::self& LogStream::operator<<(long v) {
    formatInteger(v);
    return *this;
}

LogStream::self& LogStream::operator<<(unsigned long v) {
    formatInteger(v);
    return *this;
}

LogStream::self& LogStream::operator<<(long long v) {
    formatInteger(v);
    return *this;
}

LogStream::self& LogStream::operator<<(unsigned long long v) {
    formatInteger(v);
    return *this;
}

LogStream::self& LogStream::operator<<(const void* p) {
    uintptr_t v = reinterpret_cast<uintptr_t>(p);
    if (buffer_.avail() >= kMaxNumericSize) {
        char* buf = buffer_.current();
        buf[0] = '0';
        buf[1] = 'x';
        size_t len = convertHex(buf + 2, v);
        buffer_.add(len + 2);
    }

    return *this;
}

LogStream::self& LogStream::operator<<(float v) {
    *this << static_cast<double>(v);
    return *this;
}

LogStream::self& LogStream::operator<<(double v) {
    if (buffer_.avail() >= kMaxNumericSize) {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
        buffer_.add(len);
    }

    return *this;
}

LogStream::self& LogStream::operator<<(char v) {
    buffer_.append(&v, 1);
    return *this;
}

LogStream::self& LogStream::operator<<(const char* str) {
    if (str) {
        buffer_.append(str, strlen(str));
    }
    else {
        buffer_.append("(null)", 6);
    }
    
    return *this;
}

LogStream::self& LogStream::operator<<(const unsigned char* str) {
    *this <<(reinterpret_cast<const char*>(str));
    return *this;
}

LogStream::self& LogStream::operator<<(const string& v) {
    buffer_.append(v.c_str(), v.size());
    return *this;
}

LogStream::self& LogStream::operator<<(const StringPiece& v) {
    buffer_.append(v.data(), v.size());
    return *this;
}

LogStream::self& LogStream::operator<<(const Buffer& v) {
    *this << v.toStringPiece();
    return *this;
}

template<typename T>
void LogStream::formatInteger(T v) {
    int avail = buffer_.avail();
    if ( avail >= kMaxNumericSize) {
        size_t len = convert(buffer_.current(), v);
        buffer_.add(len);
    }
}

template<typename T>
Fmt::Fmt(const char* fmt, T val) {
    static_assert(std::is_arithmetic<T>::value == true, "Must be arithmetic type");

    length_ = snprintf(buf_, sizeof buf_, fmt, val);
    assert(static_cast<size_t>(length_) < sizeof buf_);
}

template Fmt::Fmt(const char* fmt, char);
template Fmt::Fmt(const char* fmt, short);
template Fmt::Fmt(const char* fmt, unsigned short);
template Fmt::Fmt(const char* fmt, int);
template Fmt::Fmt(const char* fmt, unsigned int);
template Fmt::Fmt(const char* fmt, long);
template Fmt::Fmt(const char* fmt, unsigned long);
template Fmt::Fmt(const char* fmt, long long);
template Fmt::Fmt(const char* fmt, unsigned long long);
template Fmt::Fmt(const char* fmt, float);
template Fmt::Fmt(const char* fmt, double);


std::string formatSI(int64_t s) {
    double n = static_cast<double>(s);
    n += 0.5;
    char buf[64];
    if (s < 1000)
        snprintf(buf, sizeof(buf), "%" PRId64, s);
    else if (s < 9995)
        snprintf(buf, sizeof(buf), "%.2fk", n / 1e3);
    else if (s < 99950)
        snprintf(buf, sizeof(buf), "%.1fk", n / 1e3);
    else if (s < 999500)
        snprintf(buf, sizeof(buf), "%.0fk", n / 1e3);
    else if (s < 9995000)
        snprintf(buf, sizeof(buf), "%.2fM", n / 1e6);
    else if (s < 99950000)
        snprintf(buf, sizeof(buf), "%.1fM", n / 1e6);
    else if (s < 999500000)
        snprintf(buf, sizeof(buf), "%.0fM", n / 1e6);
    else if (s < 9995000000)
        snprintf(buf, sizeof(buf), "%.2fG", n / 1e9);
    else if (s < 99950000000)
        snprintf(buf, sizeof(buf), "%.1fG", n / 1e9);
    else if (s < 999500000000)
        snprintf(buf, sizeof(buf), "%.0fG", n / 1e9);
    else if (s < 9995000000000)
        snprintf(buf, sizeof(buf), "%.2fT", n / 1e12);
    else if (s < 99950000000000)
        snprintf(buf, sizeof(buf), "%.1fT", n / 1e12);
    else if (s < 999500000000000)
        snprintf(buf, sizeof(buf), "%.0fT", n / 1e12);
    else if (s < 9995000000000000)
        snprintf(buf, sizeof(buf), "%.2fP", n / 1e15);
    else if (s < 99950000000000000)
        snprintf(buf, sizeof(buf), "%.1fP", n / 1e15);
    else if (s < 999500000000000000)
        snprintf(buf, sizeof(buf), "%.0fP", n / 1e15);
    else
        snprintf(buf, sizeof(buf), "%.2fE", n / 1e18);
    return buf;
}

/*
 [0, 1023]
 [1.00Ki, 9.99Ki]
 [10.0Ki, 99.9Ki]
 [ 100Ki, 1023Ki]
 [1.00Mi, 9.99Mi]
*/
std::string formatIEC(int64_t s) {
    double n = static_cast<double>(s);
    char buf[64];
    const double Ki = 1024.0;
    const double Mi = Ki * 1024.0;
    const double Gi = Mi * 1024.0;
    const double Ti = Gi * 1024.0;
    const double Pi = Ti * 1024.0;
    const double Ei = Pi * 1024.0;

    if (n < Ki)
        snprintf(buf, sizeof buf, "%" PRId64, s);
    else if (n < Ki * 9.995)
        snprintf(buf, sizeof buf, "%.2fKi", n / Ki);
    else if (n < Ki * 99.95)
        snprintf(buf, sizeof buf, "%.1fKi", n / Ki);
    else if (n < Ki * 1023.5)
        snprintf(buf, sizeof buf, "%.0fKi", n / Ki);

    else if (n < Mi * 9.995)
        snprintf(buf, sizeof buf, "%.2fMi", n / Mi);
    else if (n < Mi * 99.95)
        snprintf(buf, sizeof buf, "%.1fMi", n / Mi);
    else if (n < Mi * 1023.5)
        snprintf(buf, sizeof buf, "%.0fMi", n / Mi);

    else if (n < Gi * 9.995)
        snprintf(buf, sizeof buf, "%.2fGi", n / Gi);
    else if (n < Gi * 99.95)
        snprintf(buf, sizeof buf, "%.1fGi", n / Gi);
    else if (n < Gi * 1023.5)
        snprintf(buf, sizeof buf, "%.0fGi", n / Gi);

    else if (n < Ti * 9.995)
        snprintf(buf, sizeof buf, "%.2fTi", n / Ti);
    else if (n < Ti * 99.95)
        snprintf(buf, sizeof buf, "%.1fTi", n / Ti);
    else if (n < Ti * 1023.5)
        snprintf(buf, sizeof buf, "%.0fTi", n / Ti);

    else if (n < Pi * 9.995)
        snprintf(buf, sizeof buf, "%.2fPi", n / Pi);
    else if (n < Pi * 99.95)
        snprintf(buf, sizeof buf, "%.1fPi", n / Pi);
    else if (n < Pi * 1023.5)
        snprintf(buf, sizeof buf, "%.0fPi", n / Pi);

    else if (n < Ei * 9.995)
        snprintf(buf, sizeof buf, "%.2fEi", n / Ei);
    else
        snprintf(buf, sizeof buf, "%.1fEi", n / Ei);
    return buf;
}
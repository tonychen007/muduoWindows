#pragma once

#include "noncopyable.h"
#include "types.h"
#include "stringPiece.h"
#include <memory.h>

const int kSmallBuffer = 4096;
const int kLargeBuffer = 4096 * 1024;

template<int SIZE>
class FixedBuffer : noncopyable {
public:
	FixedBuffer()
		: cur_(data_) {
		setCookie(cookieStart);
	}

	~FixedBuffer() {
		setCookie(cookieEnd);
	}

	void append(const char* buf, size_t len) {
		// FIXME: append partially
		if (avail() > len) {
			memcpy(cur_, buf, len);
			cur_ += len;
		}
	}

	const char* data() const { return data_; }
	int length() const { return static_cast<int>(cur_ - data_); }

	// write to data_ directly
	char* current() { return cur_; }
	int avail() const { return static_cast<int>(end() - cur_); }
	void add(size_t len) { cur_ += len; }

	void reset() { cur_ = data_; bzero(); }
	void bzero() { memset(data_, 0, sizeof data_); }

	const char* debugString();
	void setCookie(void (*cookie)()) { cookie_ = cookie; }

	// for used by unit test
	string toString() const { return string(data_, length()); }
	StringPiece toStringPiece() const { return StringPiece(data_, length()); }


private:
	const char* end() const { return data_ + sizeof data_; }
	// Must be outline function for cookies.
	static void cookieStart();
	static void cookieEnd();

	void (*cookie_)();
	char data_[SIZE];
	char* cur_;
};

class LogStream : noncopyable {
	typedef LogStream self;
public:
	typedef FixedBuffer<kSmallBuffer> Buffer;

	void append(const char* data, int len) { buffer_.append(data, len); }
	const Buffer& buffer() const { return buffer_; }
	void resetBuffer() { buffer_.reset(); }

	self& operator<<(bool v);
	self& operator<<(short);
	self& operator<<(unsigned short);
	self& operator<<(int);
	self& operator<<(unsigned int);
	self& operator<<(long);
	self& operator<<(unsigned long);
	self& operator<<(long long);
	self& operator<<(unsigned long long);
	self& operator<<(const void*);
	self& operator<<(float v);
	self& operator<<(double);
	self& operator<<(char v);
	self& operator<<(const char* str);
	self& operator<<(const unsigned char* str);
	self& operator<<(const string& v);
	self& operator<<(const StringPiece& v);
	self& operator<<(const Buffer& v);

private:
	template<typename T>
	void formatInteger(T v);

	Buffer buffer_;
	static const int kMaxNumericSize = 48;
};

class Fmt:noncopyable {
public:
	template<typename T>
	Fmt(const char* fmt, T val);

	const char* data() const { return buf_; }
	int length() const { return length_; }

private:
	char buf_[32];
	int length_;
};

inline LogStream& operator<<(LogStream& s, const Fmt& fmt) {
	s.append(fmt.data(), fmt.length());
	return s;
}

// Format quantity n in SI units (k, M, G, T, P, E).
// The returned string is atmost 5 characters long.
// Requires n >= 0
string formatSI(int64_t n);

// Format quantity n in IEC (binary) units (Ki, Mi, Gi, Ti, Pi, Ei).
// The returned string is atmost 6 characters long.
// Requires n >= 0
string formatIEC(int64_t n);
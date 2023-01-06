#pragma once

#include "noncopyable.h"
#include "stringPiece.h"

class ReadSmallFile:noncopyable {
public:
	ReadSmallFile(StringArg name);
	~ReadSmallFile();

	template<typename String>
	int readToString(int maxSize,
		String* content,
		int64_t* fileSize,
		int64_t* modifyTime,
		int64_t* createTime);
	
	int readToBuffer(int* size);
	const char* buffer() const { return buf_; }

	static const int kBufferSize = 64 * 1024;
private:
	FILE* fp_;
	int err_;
	char buf_[kBufferSize];
};

// read the file content, returns errno if error happens.
template<typename String>
int readFile(StringArg filename,
	int maxSize,
	String* content,
	int64_t* fileSize = NULL,
	int64_t* modifyTime = NULL,
	int64_t* createTime = NULL) {

	ReadSmallFile file(filename);
	return file.readToString(maxSize, content, fileSize, modifyTime, createTime);
}

class AppendFile :noncopyable {
public:
	explicit AppendFile(StringArg filename);
	~AppendFile();

	void append(const char* logline, size_t len);
	void flush();
	int64_t writtenBytes() const { return writtenBytes_; }
	const char* buf() const { return buf_; }

	static const int kBufferSize = 64*1024;
private:
	size_t write(const char* logline, size_t len);

	FILE* fp_;
	char buf_[kBufferSize];
	int64_t writtenBytes_;
};
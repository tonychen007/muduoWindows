#include "fileUtil.h"
#include <stdio.h>
#include <sys/stat.h>

ReadSmallFile::ReadSmallFile(StringArg name):err_(0) {
	fopen_s(&fp_, name.c_str(), "rb");
	buf_[0] = '\0';
	if (!fp_) {
		err_ = errno;
	}
}

ReadSmallFile::~ReadSmallFile() {
	if (fp_ >= 0) {
		fclose(fp_); // FIXME: check EINTR
	}
}

int ReadSmallFile::readToBuffer(int* size) {
	int err = err_;
	
	if (!fp_)
		return err;

	size_t n = fread_s(buf_, sizeof(buf_), sizeof(char), sizeof(buf_) - 1, fp_);
	if (n >= 0) {
		if (size) {
			*size = static_cast<int>(n);
		}
		buf_[n] = '\0';
	}
	else {
		err = errno;
	}
		
	return err;
}

template<typename String>
int ReadSmallFile::readToString(int maxSize,
	String* content,
	int64_t* fileSize,
	int64_t* modifyTime,
	int64_t* createTime) {

	int err = err_;
	
	if (!fp_)
		return err;

	content->clear();
	if (fileSize) {
		int fd = _fileno(fp_);
		struct stat statbuf;
		if (fstat(fd, &statbuf) == 0) {
			if (statbuf.st_mode & S_IFREG) {
				*fileSize = statbuf.st_size;
				int s = static_cast<int>(std::min(static_cast<int64_t>(maxSize), *fileSize));
				content->reserve(s);
			}
			else if (statbuf.st_mode & S_IFDIR) {
				err = EISDIR;
			}
			if (modifyTime) {
				*modifyTime = statbuf.st_mtime;
			}
			if (createTime) {
				*createTime = statbuf.st_ctime;
			}
		}
		else {
			err = errno;
		}
	}

	while (content->size() < static_cast<size_t>(maxSize)) {
		size_t toRead = std::min(sizeof(buf_), static_cast<size_t>(maxSize) - content->size());
		size_t n = fread_s(buf_, sizeof(buf_), sizeof(char), toRead, fp_);
		
		if (n > 0) {
			content->append(buf_, n);
		}
		else {
			if (n < 0)
				err = errno;
			break;
		}
	}

	return err;
}

template int ReadSmallFile::readToString(int, string*, int64_t*, int64_t*, int64_t*);

AppendFile::AppendFile(StringArg filename)
	:writtenBytes_(0) {
	fopen_s(&fp_, filename.c_str(), "ab+");
	if (fp_)
		setvbuf(fp_, buf_, _IOFBF, sizeof(buf_));
}

AppendFile::~AppendFile() {
	fclose(fp_);
	writtenBytes_ = 0;
}

void AppendFile::append(const char* logline, size_t len) {
	size_t written = 0;

	while (written != len) {
		size_t remain = len - written;
		size_t n = write(logline + written, remain);
		if (n != remain) {
			int err = ferror(fp_);
			if (err) {
				char buf[256];
				fprintf(stderr, "AppendFile::append() failed %s\n", strerror_s(buf, sizeof(buf), errno));
				break;
			}
		}

		written += n;
	}

	writtenBytes_ += written;
}

void AppendFile::flush() {
	fflush(fp_);
}

size_t AppendFile::write(const char* logline, size_t len) {
	return _fwrite_nolock(logline, sizeof(char), len, fp_);
}
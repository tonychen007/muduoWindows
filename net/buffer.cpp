#include "buffer.h"
#include "SocketsOps.h"
#include "types.h"

#include <errno.h>

using namespace net;

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

ssize_t Buffer::readFd(int fd, int* savedErrno) {
    // saved an ioctl()/FIONREAD call to tell how much to read
    char extrabuf[65536];
    iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].buf = begin() + writerIndex_;
    vec[0].len = writable;
    vec[1].buf = extrabuf;
    vec[1].len = sizeof extrabuf;
    // when there is enough space in this buffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1 bytes at most.
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = sockets::readv(fd, vec, iovcnt);
    if (n < 0) {
        *savedErrno = errno;
    }
    else if (implicit_cast<size_t>(n) <= writable) {
        writerIndex_ += n;
    }
    else {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    // if (n == writable + sizeof extrabuf)
    // {
    //   goto line_30;
    // }
    return n;
}
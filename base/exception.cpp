#include "exception.h"
#include "currentThread.h"

Exception::Exception(string what)
	:msg_(std::move(what)),
	stack_(CurrentThread::convertStackFrame(CurrentThread::stacktrace()).c_str()) {
}
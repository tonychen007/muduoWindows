#pragma once

#include <exception>
#include "types.h"

class Exception : public std::exception {
public:
	Exception(string what);
	~Exception() noexcept override = default;

	const char* what() const noexcept override {
		return msg_.c_str();
	}

	const char* stackTrace() const noexcept {
		return stack_.c_str();
	}

private:
	string msg_;
	string stack_;
};

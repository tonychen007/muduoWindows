#pragma once

#include <memory>
#include <mutex>
#include <string>

#define __thread _declspec(thread)

using uniqueLock = std::unique_lock<std::mutex>;
using std::string;

template<typename To, typename From>
inline To implicit_cast(From const& f) {
	return f;
}
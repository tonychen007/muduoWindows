#pragma once

#include <memory>
#include <mutex>
#include <string>

#define __thread _declspec(thread)

using uniqueLock = std::unique_lock<std::mutex>;
using std::string;

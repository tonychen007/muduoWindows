#pragma once

#include <memory>
#include <mutex>
#include <string>

using uniqueLock = std::unique_lock<std::mutex>;
using std::string;

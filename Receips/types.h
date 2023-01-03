#pragma once

#include <memory>
#include <mutex>

using uniqueLock = std::unique_lock<std::mutex>;

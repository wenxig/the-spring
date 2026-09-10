#include "clock_service.hpp"
#include "esp_timer.h"
#include <ctime>

namespace {
std::uint64_t revision = 0;
}

spring::clock::Snapshot spring::clock::now() {
  return {.unix_seconds = static_cast<std::int64_t>(std::time(nullptr)), .revision = ++revision};
}

void spring::clock::start() {}

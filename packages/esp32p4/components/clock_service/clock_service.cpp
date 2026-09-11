#include "clock_service.hpp"
#include "esp_timer.h"
#include <sys/time.h>
#include <cstdlib>
#include <ctime>

namespace {
std::uint64_t revision = 0;
}

spring::clock::Snapshot spring::clock::now() {
  return {.unix_seconds = static_cast<std::int64_t>(std::time(nullptr)), .revision = ++revision};
}

void spring::clock::start() {
  (void)setenv("TZ", "CST-8", 1);
  tzset();
}

void spring::clock::set_unix_seconds(std::int64_t unix_seconds) {
  const timeval value{.tv_sec = static_cast<time_t>(unix_seconds), .tv_usec = 0};
  (void)settimeofday(&value, nullptr);
}

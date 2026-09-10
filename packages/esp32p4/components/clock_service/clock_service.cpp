#include "clock_service.hpp"
#include "esp_timer.h"

namespace {
std::uint64_t revision = 0;
}

spring::clock::Snapshot spring::clock::now() {
  return {.unix_seconds = esp_timer_get_time() / 1000000, .revision = ++revision};
}

void spring::clock::start() {}

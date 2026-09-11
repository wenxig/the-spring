#pragma once

#include <cstdint>

namespace spring::clock {
struct Snapshot { std::int64_t unix_seconds; std::uint64_t revision; };
Snapshot now();
void start();
void set_unix_seconds(std::int64_t unix_seconds);
}  // namespace spring::clock

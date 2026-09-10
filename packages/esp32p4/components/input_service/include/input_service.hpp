#pragma once

#include <cstdint>

namespace spring::input {
enum class Key : std::uint8_t { up, down, left, right, confirm, cancel, sleep, home };
void start();
}  // namespace spring::input

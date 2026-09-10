#pragma once

#include <cstdint>

namespace spring::input {
enum class Key : std::uint8_t { up, down, left, right, confirm, cancel, sleep, home };
void start();
bool next(Key& key, std::uint32_t timeout_ms);
}  // namespace spring::input

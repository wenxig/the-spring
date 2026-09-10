#pragma once

namespace spring::power {
enum class State { active, sleeping };
void start();
void request_sleep();
void wake();
State state();
}  // namespace spring::power

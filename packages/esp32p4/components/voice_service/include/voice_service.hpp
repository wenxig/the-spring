#pragma once

#include <string_view>

namespace spring::voice {
bool dial(std::string_view number);
bool answer();
bool hangup();
}  // namespace spring::voice

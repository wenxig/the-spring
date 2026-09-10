#pragma once

#include <string_view>

namespace spring::ota {
bool mark_boot_valid();
bool begin(std::string_view firmware_url);
}  // namespace spring::ota

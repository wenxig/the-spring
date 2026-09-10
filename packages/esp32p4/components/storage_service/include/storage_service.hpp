#pragma once

#include <cstddef>
#include <string_view>

namespace spring::storage {
bool mount_sdcard();
bool append_event(std::string_view type, std::string_view payload);
}  // namespace spring::storage

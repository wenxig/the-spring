#pragma once

#include <cstddef>
#include <string_view>

namespace spring::storage {
// Requires the ESP-Hosted SDMMC controller to be initialized first.
bool mount_sdcard();
bool ui_assets_ready();
bool append_event(std::string_view type, std::string_view payload);
} // namespace spring::storage

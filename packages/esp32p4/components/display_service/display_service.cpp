#include "display_service.hpp"
#include "ui_core.hpp"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <algorithm>
#include <array>

namespace {
constexpr char kTag[] = "display";
constexpr gpio_num_t kBusy = GPIO_NUM_22;
constexpr gpio_num_t kReset = GPIO_NUM_21;
constexpr gpio_num_t kDataCommand = GPIO_NUM_6;
constexpr gpio_num_t kChipSelect = GPIO_NUM_5;
constexpr gpio_num_t kClock = GPIO_NUM_2;
constexpr gpio_num_t kMosi = GPIO_NUM_3;
constexpr std::uint32_t kSpiFrequency = 1'000'000;
constexpr std::int64_t kBusyTimeoutUs = 8'000'000;
constexpr std::uint16_t kPartialRefreshLimit = 20;
constexpr std::uint8_t kCommandSoftwareReset = 0x12;
constexpr std::uint8_t kCommandDriverOutput = 0x01;
constexpr std::uint8_t kCommandDataEntry = 0x11;
constexpr std::uint8_t kCommandRamXWindow = 0x44;
constexpr std::uint8_t kCommandRamYWindow = 0x45;
constexpr std::uint8_t kCommandRamXCounter = 0x4E;
constexpr std::uint8_t kCommandRamYCounter = 0x4F;
constexpr std::uint8_t kCommandBorder = 0x3C;
constexpr std::uint8_t kCommandTemperature = 0x18;
constexpr std::uint8_t kCommandDisplayUpdate = 0x22;
constexpr std::uint8_t kCommandDisplayRefresh = 0x20;
constexpr std::uint8_t kCommandBlackRam = 0x24;
constexpr std::uint8_t kCommandPreviousRam = 0x26;
constexpr std::uint8_t kCommandDeepSleep = 0x10;

bool baseline_valid = false;
std::uint8_t frame[spring::display::kFrameBytes]{};
spring::ui::Frame committed_frame;
spring::display::Rect dirty{0, 0, 0, 0};
spring::display::Backend active_backend{spring::display::Backend::buffer_only};
spi_device_handle_t spi_device = nullptr;
bool epaper_ready = false;
std::uint16_t partial_refreshes = 0;

bool wait_until_ready() {
  const auto deadline = esp_timer_get_time() + kBusyTimeoutUs;
  while (gpio_get_level(kBusy) != 0) {
    if (esp_timer_get_time() >= deadline) {
      ESP_LOGE(kTag, "BUSY timeout");
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(2));
  }
  return true;
}

bool transfer(bool command, std::span<const std::uint8_t> bytes) {
  if (spi_device == nullptr || bytes.empty()) return false;
  gpio_set_level(kDataCommand, command ? 0 : 1);
  spi_transaction_t transaction{};
  transaction.length = bytes.size() * 8;
  transaction.tx_buffer = bytes.data();
  return spi_device_transmit(spi_device, &transaction) == ESP_OK;
}

bool command(std::uint8_t value, std::span<const std::uint8_t> data = {}) {
  return transfer(true, {&value, 1}) && (data.empty() || transfer(false, data));
}

bool set_window(spring::display::Rect area) {
  const auto x_start = static_cast<std::uint8_t>(area.x / 8);
  const auto x_end = static_cast<std::uint8_t>((area.x + area.width) / 8 - 1);
  const std::array<std::uint8_t, 2> x_window{x_start, x_end};
  const auto y_end = static_cast<std::uint16_t>(area.y + area.height - 1);
  const std::array<std::uint8_t, 4> y_window{
      static_cast<std::uint8_t>(area.y), static_cast<std::uint8_t>(area.y >> 8),
      static_cast<std::uint8_t>(y_end), static_cast<std::uint8_t>(y_end >> 8)};
  const std::array<std::uint8_t, 1> x_counter{x_start};
  const std::array<std::uint8_t, 2> y_counter{
      static_cast<std::uint8_t>(area.y), static_cast<std::uint8_t>(area.y >> 8)};
  return command(kCommandRamXWindow, x_window) && command(kCommandRamYWindow, y_window) &&
         command(kCommandRamXCounter, x_counter) && command(kCommandRamYCounter, y_counter);
}

bool write_frame(std::uint8_t ram_command, spring::display::Rect area, const std::uint8_t* source) {
  if (!set_window(area)) return false;
  if (!command(ram_command)) return false;
  const auto first_byte = static_cast<std::size_t>(area.y) * 50U + area.x / 8U;
  const auto row_bytes = static_cast<std::size_t>(area.width / 8U);
  for (std::uint16_t row{}; row < area.height; ++row) {
    const auto offset = first_byte + static_cast<std::size_t>(row) * 50U;
    if (!transfer(false, {source + offset, row_bytes})) return false;
  }
  return true;
}

bool refresh(spring::display::Rect area, bool full) {
  if (!epaper_ready || !wait_until_ready()) return false;
  if (full) {
    const std::array<std::uint8_t, 3> driver_output{0x2B, 0x01, 0x00};
    const std::array<std::uint8_t, 1> entry_mode{0x03};
    const std::array<std::uint8_t, 1> border{0x05};
    const std::array<std::uint8_t, 1> temperature{0x80};
    if (!command(kCommandSoftwareReset) || !wait_until_ready() ||
        !command(kCommandDriverOutput, driver_output) || !command(kCommandDataEntry, entry_mode) ||
        !command(kCommandBorder, border) || !command(kCommandTemperature, temperature)) return false;
  }
  if (!full && !write_frame(kCommandPreviousRam, area, committed_frame.bytes().data())) return false;
  if (!write_frame(kCommandBlackRam, area, frame)) return false;
  const std::array<std::uint8_t, 1> update_mode{static_cast<std::uint8_t>(full ? 0xF7 : 0xFF)};
  return command(kCommandDisplayUpdate, update_mode) && command(kCommandDisplayRefresh) && wait_until_ready();
}
}

void spring::display::start(Backend selected) {
  active_backend = selected;
  ESP_LOGI(kTag, "backend=%s", selected == Backend::epaper ? "epaper" : "buffer-only");
  if (selected == Backend::buffer_only) return;
  gpio_set_direction(kBusy, GPIO_MODE_INPUT);
  gpio_set_direction(kReset, GPIO_MODE_OUTPUT);
  gpio_set_direction(kDataCommand, GPIO_MODE_OUTPUT);
  gpio_set_direction(kChipSelect, GPIO_MODE_OUTPUT);
  gpio_set_direction(kClock, GPIO_MODE_OUTPUT);
  gpio_set_direction(kMosi, GPIO_MODE_OUTPUT);
  gpio_set_level(kChipSelect, 1);
  gpio_set_level(kReset, 1);
  spi_bus_config_t bus_config{};
  bus_config.mosi_io_num = kMosi;
  bus_config.miso_io_num = GPIO_NUM_NC;
  bus_config.sclk_io_num = kClock;
  bus_config.quadwp_io_num = GPIO_NUM_NC;
  bus_config.quadhd_io_num = GPIO_NUM_NC;
  bus_config.max_transfer_sz = kFrameBytes;
  if (spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO) != ESP_OK) {
    ESP_LOGE(kTag, "SPI2 initialization failed");
    return;
  }
  spi_device_interface_config_t device_config{};
  device_config.clock_speed_hz = kSpiFrequency;
  device_config.mode = 0;
  device_config.spics_io_num = kChipSelect;
  device_config.queue_size = 1;
  if (spi_bus_add_device(SPI2_HOST, &device_config, &spi_device) != ESP_OK) {
    ESP_LOGE(kTag, "SPI2 device registration failed");
    spi_bus_free(SPI2_HOST);
    return;
  }
  gpio_set_level(kReset, 0);
  vTaskDelay(pdMS_TO_TICKS(10));
  gpio_set_level(kReset, 1);
  vTaskDelay(pdMS_TO_TICKS(10));
  epaper_ready = wait_until_ready();
  baseline_valid = false;
  partial_refreshes = 0;
}

spring::display::Backend spring::display::backend() { return active_backend; }

std::span<const std::uint8_t> spring::display::frame_bytes() {
  return {frame, kFrameBytes};
}

std::uint32_t spring::display::frame_checksum() {
  std::uint32_t hash{2166136261U};
  for (const auto byte : frame) { hash ^= byte; hash *= 16777619U; }
  return hash;
}

void spring::display::invalidate(Rect area) {
  (void)area;
  if (dirty.width == 0 || dirty.height == 0) {
    dirty = area;
  } else {
    const auto right = std::max<std::uint16_t>(dirty.x + dirty.width, area.x + area.width);
    const auto bottom = std::max<std::uint16_t>(dirty.y + dirty.height, area.y + area.height);
    dirty.x = std::min(dirty.x, area.x);
    dirty.y = std::min(dirty.y, area.y);
    dirty.width = right - dirty.x;
    dirty.height = bottom - dirty.y;
  }
  if (!baseline_valid) force_full_refresh();
}

void spring::display::force_full_refresh() { baseline_valid = false; dirty = {0, 0, 400, 300}; }

void spring::display::sleep() {
  if (active_backend != Backend::epaper || !epaper_ready) return;
  const std::array<std::uint8_t, 1> mode{0x01};
  if (!wait_until_ready() || !command(kCommandDeepSleep, mode)) {
    epaper_ready = false;
    baseline_valid = false;
  }
}

void spring::display::wake() {
  if (active_backend != Backend::epaper) return;
  gpio_set_level(kReset, 0);
  vTaskDelay(pdMS_TO_TICKS(10));
  gpio_set_level(kReset, 1);
  vTaskDelay(pdMS_TO_TICKS(10));
  epaper_ready = wait_until_ready();
  force_full_refresh();
}

void spring::display::complete_refresh() { dirty = {0, 0, 0, 0}; }

void spring::display::present(const spring::ui::Frame& next) {
  const auto& bytes = next.bytes();
  std::copy(bytes.begin(), bytes.end(), frame);
  const auto area = baseline_valid ? next.difference(committed_frame) : spring::ui::Rect{0, 0, 400, 300};
  if (area.width == 0 || area.height == 0) return;
  dirty = {area.x, area.y, area.width, area.height};
  const auto full = !baseline_valid || partial_refreshes >= kPartialRefreshLimit;
  if (active_backend == Backend::epaper) {
    if (!refresh(dirty, full)) {
      ESP_LOGE(kTag, "refresh failed; display baseline invalid");
      force_full_refresh();
      return;
    }
  }
  committed_frame = next;
  baseline_valid = true;
  partial_refreshes = full ? 0 : static_cast<std::uint16_t>(partial_refreshes + 1);
}

spring::display::Rect spring::display::pending_area() { return dirty; }

spring::display::Rect spring::display::take_pending_area() {
  const auto area = dirty;
  dirty = {0, 0, 0, 0};
  return area;
}

bool spring::display::write_pixel(std::uint16_t x, std::uint16_t y, bool black) {
  if (x >= 400 || y >= 300) return false;
  const auto index = static_cast<std::size_t>(y) * 50U + x / 8U;
  const auto mask = static_cast<std::uint8_t>(0x80U >> (x % 8U));
  if (black) frame[index] |= mask;
  else frame[index] &= static_cast<std::uint8_t>(~mask);
  invalidate({x, y, 1, 1});
  return true;
}

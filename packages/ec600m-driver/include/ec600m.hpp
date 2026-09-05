#pragma once

#include "esp_err.h"
#include "esp_modem_c_api_types.h"
#include "esp_netif_types.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace spring::cellular {

struct ModemUartConfig {
  int txPin{0}; // Default to Waveshare Pico Pin 1 (GPIO0)
  int rxPin{1}; // Default to Waveshare Pico Pin 2 (GPIO1)
  int rtsPin{-1};
  int ctsPin{-1};
  int baudRate{115200};
  int uartPort{1};
  int rxBufferSize{4096};
  int txBufferSize{2048};
};

struct SignalQuality {
  int rssi{99};
  int ber{99};
};

enum class NetworkStatus : std::uint8_t {
  NotRegistered = 0,
  RegisteredHome = 1,
  Searching = 2,
  RegistrationDenied = 3,
  Unknown = 4,
  RegisteredRoaming = 5
};

class Ec600mModem {
public:
  explicit Ec600mModem(ModemUartConfig config);
  ~Ec600mModem();

  Ec600mModem(const Ec600mModem&) = delete;
  Ec600mModem& operator=(const Ec600mModem&) = delete;
  Ec600mModem(Ec600mModem&&) noexcept;
  Ec600mModem& operator=(Ec600mModem&&) noexcept;

  [[nodiscard]] auto init() -> esp_err_t;
  [[nodiscard]] auto startPpp() -> esp_err_t;
  [[nodiscard]] auto stopPpp() -> esp_err_t;

  [[nodiscard]] auto getSignalQuality() -> std::optional<SignalQuality>;
  [[nodiscard]] auto getImsi() -> std::optional<std::string>;
  [[nodiscard]] auto getImei() -> std::optional<std::string>;
  [[nodiscard]] auto getRegistrationStatus() -> std::optional<NetworkStatus>;
  [[nodiscard]] auto setApn(std::string_view apn) -> esp_err_t;
  [[nodiscard]] auto powerDown() -> esp_err_t;

  [[nodiscard]] auto getNetif() const noexcept -> esp_netif_t* { return netif_; }
  [[nodiscard]] auto isConnected() const noexcept -> bool { return connected_; }

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  ModemUartConfig config_;
  esp_netif_t* netif_{nullptr};
  bool connected_{false};
  bool eventHandlersRegistered_{false};

  auto cleanupNetif() noexcept -> void;
  auto unregisterEventHandlers() noexcept -> void;
};

} // namespace spring::cellular

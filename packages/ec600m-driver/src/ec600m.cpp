#include "ec600m.hpp"

#include "cxx_include/esp_modem_api.hpp"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_modem_config.h"
#include "esp_netif.h"
#include "esp_netif_ppp.h"

#include <exception>
#include <utility>

namespace spring::cellular {

namespace {
constexpr auto kTag = "ec600m";

auto onPppEvent([[maybe_unused]] void* arg, esp_event_base_t base, int32_t eventId, void* data)
    -> void {
  if (base == NETIF_PPP_STATUS) {
    ESP_LOGI(kTag, "PPP status event: %ld", eventId);
    return;
  }

  if (base != IP_EVENT) {
    return;
  }

  if (eventId == IP_EVENT_PPP_GOT_IP) {
    const auto* event = static_cast<const ip_event_got_ip_t*>(data);
    ESP_LOGI(kTag, "PPP Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    return;
  }

  if (eventId == IP_EVENT_PPP_LOST_IP) {
    ESP_LOGW(kTag, "PPP Lost IP");
    return;
  }
}
} // namespace

struct Ec600mModem::Impl {
  std::shared_ptr<esp_modem::DTE> dte{nullptr};
  std::unique_ptr<esp_modem::DCE> dce{nullptr};
};

Ec600mModem::Ec600mModem(ModemUartConfig config)
    : impl_(std::make_unique<Impl>()), config_(config) {}

Ec600mModem::~Ec600mModem() {
  if (connected_) {
    stopPpp();
  }

  unregisterEventHandlers();
  cleanupNetif();
}

Ec600mModem::Ec600mModem(Ec600mModem&&) noexcept = default;
Ec600mModem& Ec600mModem::operator=(Ec600mModem&&) noexcept = default;

auto Ec600mModem::cleanupNetif() noexcept -> void {
  if (!netif_) {
    return;
  }
  esp_netif_destroy(netif_);
  netif_ = nullptr;
}

auto Ec600mModem::unregisterEventHandlers() noexcept -> void {
  if (!eventHandlersRegistered_) {
    return;
  }
  esp_event_handler_unregister(IP_EVENT, IP_EVENT_PPP_GOT_IP, &onPppEvent);
  esp_event_handler_unregister(IP_EVENT, IP_EVENT_PPP_LOST_IP, &onPppEvent);
  esp_event_handler_unregister(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, &onPppEvent);
  eventHandlersRegistered_ = false;
}

auto Ec600mModem::init() -> esp_err_t {
  ESP_LOGI(kTag, "Initializing EC600M on UART TX: %d, RX: %d @ %d baud", config_.txPin,
           config_.rxPin, config_.baudRate);

  auto dteConfig = ESP_MODEM_DTE_DEFAULT_CONFIG();
  dteConfig.uart_config.port_num = static_cast<uart_port_t>(config_.uartPort);
  dteConfig.uart_config.tx_io_num = config_.txPin;
  dteConfig.uart_config.rx_io_num = config_.rxPin;
  dteConfig.uart_config.rts_io_num = config_.rtsPin;
  dteConfig.uart_config.cts_io_num = config_.ctsPin;
  dteConfig.uart_config.baud_rate = config_.baudRate;
  dteConfig.uart_config.rx_buffer_size = config_.rxBufferSize;
  dteConfig.uart_config.tx_buffer_size = config_.txBufferSize;

  auto dceConfig = ESP_MODEM_DCE_DEFAULT_CONFIG("CMNET");

  const auto netifPppConfig = ESP_NETIF_DEFAULT_PPP();
  netif_ = esp_netif_new(&netifPppConfig);
  if (!netif_) {
    ESP_LOGE(kTag, "Failed to create PPP netif");
    return ESP_FAIL;
  }

  esp_event_handler_register(IP_EVENT, IP_EVENT_PPP_GOT_IP, &onPppEvent, this);
  esp_event_handler_register(IP_EVENT, IP_EVENT_PPP_LOST_IP, &onPppEvent, this);
  esp_event_handler_register(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, &onPppEvent, this);
  eventHandlersRegistered_ = true;

  try {
    impl_->dce = esp_modem::create_EC20_dce(&dceConfig, &dteConfig, netif_);
    if (!impl_->dce) {
      ESP_LOGE(kTag, "Failed to create DCE instance");
      cleanupNetif();
      unregisterEventHandlers();
      return ESP_FAIL;
    }
  } catch (const std::exception& e) {
    ESP_LOGE(kTag, "Exception during DCE initialization: %s", e.what());
    cleanupNetif();
    unregisterEventHandlers();
    return ESP_FAIL;
  }

  ESP_LOGI(kTag, "EC600M DCE initialized successfully");
  return ESP_OK;
}

auto Ec600mModem::startPpp() -> esp_err_t {
  if (!impl_ || !impl_->dce) {
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(kTag, "Switching to PPP data mode...");
  if (!impl_->dce->set_mode(esp_modem::modem_mode::DATA_MODE)) {
    ESP_LOGE(kTag, "Failed to enter PPP mode");
    return ESP_FAIL;
  }

  connected_ = true;
  ESP_LOGI(kTag, "Entered PPP mode, waiting for IP");
  return ESP_OK;
}

auto Ec600mModem::stopPpp() -> esp_err_t {
  if (!impl_ || !impl_->dce) {
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(kTag, "Switching back to command mode...");
  if (!impl_->dce->set_mode(esp_modem::modem_mode::COMMAND_MODE)) {
    ESP_LOGE(kTag, "Failed to leave PPP mode");
    return ESP_FAIL;
  }

  connected_ = false;
  return ESP_OK;
}

auto Ec600mModem::getSignalQuality() -> std::optional<SignalQuality> {
  if (!impl_ || !impl_->dce) {
    return std::nullopt;
  }

  auto rssi = 99;
  auto ber = 99;
  if (impl_->dce->get_signal_quality(rssi, ber) != esp_modem::command_result::OK) {
    return std::nullopt;
  }

  return SignalQuality{.rssi = rssi, .ber = ber};
}

auto Ec600mModem::getImsi() -> std::optional<std::string> {
  if (!impl_ || !impl_->dce) {
    return std::nullopt;
  }

  auto imsi = std::string{};
  if (impl_->dce->get_imsi(imsi) != esp_modem::command_result::OK) {
    return std::nullopt;
  }

  return imsi;
}

auto Ec600mModem::getImei() -> std::optional<std::string> {
  if (!impl_ || !impl_->dce) {
    return std::nullopt;
  }

  auto imei = std::string{};
  if (impl_->dce->get_imei(imei) != esp_modem::command_result::OK) {
    return std::nullopt;
  }

  return imei;
}

auto Ec600mModem::getRegistrationStatus() -> std::optional<NetworkStatus> {
  if (!impl_ || !impl_->dce) {
    return std::nullopt;
  }

  return NetworkStatus::RegisteredHome;
}

auto Ec600mModem::setApn(std::string_view apn) -> esp_err_t {
  if (!impl_ || !impl_->dce) {
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(kTag, "Setting APN to %.*s", static_cast<int>(apn.length()), apn.data());
  return ESP_OK;
}

auto Ec600mModem::powerDown() -> esp_err_t {
  if (!impl_ || !impl_->dce) {
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(kTag, "Sending soft power down (AT+QPOWD=1)");
  return impl_->dce->power_down() ? ESP_OK : ESP_FAIL;
}

} // namespace spring::cellular

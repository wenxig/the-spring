#include "http_transport.hpp"

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_netif.h"
#include "esp_tls_errors.h"
#include "lwip/sockets.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <climits>
#include <memory>
#include <string_view>
#include <strings.h>

namespace {
using namespace spring::network;
using Clock = std::chrono::steady_clock;

esp_http_client_method_t method_for(Method method) {
  switch (method) {
  case Method::get:
    return HTTP_METHOD_GET;
  case Method::post:
    return HTTP_METHOD_POST;
  case Method::put:
    return HTTP_METHOD_PUT;
  case Method::patch:
    return HTTP_METHOD_PATCH;
  case Method::delete_:
    return HTTP_METHOD_DELETE;
  }
  return HTTP_METHOD_GET;
}

TransportError classify(esp_http_client_handle_t client, esp_err_t error, bool connected) {
  int tls_code{};
  int tls_flags{};
  const auto tls_error =
      esp_http_client_get_and_clear_last_tls_error(client, &tls_code, &tls_flags);
  if (tls_error == ESP_ERR_ESP_TLS_CANNOT_RESOLVE_HOSTNAME)
    return TransportError::dns;
  if (error == ESP_ERR_TIMEOUT || error == ESP_ERR_HTTP_READ_TIMEOUT ||
      esp_http_client_get_errno(client) == ETIMEDOUT ||
      tls_error == ESP_ERR_ESP_TLS_CONNECTION_TIMEOUT)
    return TransportError::timeout;
  if (tls_code != 0 || tls_flags != 0)
    return TransportError::tls;
  if (error == ESP_ERR_HTTP_INCOMPLETE_DATA || error == ESP_ERR_HTTP_FETCH_HEADER)
    return TransportError::protocol;
  return connected ? TransportError::io : TransportError::connection;
}

// esp_http_client stores one value per key. Serialize the validated ordered field list
// into a single entry so repeated fields remain separate lines on the wire.
esp_err_t set_headers(esp_http_client_handle_t client, const Headers& headers) {
  for (std::size_t index{}; index < headers.size();) {
    const auto& key = headers[index].first;
    std::string value = headers[index].second;
    std::size_t next = index + 1;
    for (; next < headers.size(); ++next) {
      if (strcasecmp(headers[next].first.c_str(), key.c_str()) != 0)
        break;
      value += ", " + headers[next].second;
    }
    if (esp_http_client_set_header(client, key.c_str(), value.c_str()) != ESP_OK)
      return ESP_FAIL;
    index = next;
  }
  return ESP_OK;
}

bool has_header(const Headers& headers, std::string_view name) {
  return std::ranges::any_of(headers, [name](const auto& header) {
    return strcasecmp(header.first.c_str(), std::string{name}.c_str()) == 0;
  });
}

std::string_view origin(std::string_view url) {
  const auto start = url.find("://");
  return url.substr(0, url.find('/', start == std::string_view::npos ? 0 : start + 3));
}
} // namespace

spring::network::TransportResult spring::http::perform(const network::Request& request,
                                                       network::CancellationToken cancellation,
                                                       esp_netif_obj* netif) {
  TransportResult result{};
  auto fail = [&](TransportError error, std::string detail) {
    result.response.error = error;
    result.response.error_detail = std::move(detail);
  };
  if (netif == nullptr || !esp_netif_is_netif_up(netif)) {
    fail(TransportError::unavailable, "network interface is down");
    return result;
  }
  ifreq interface{};
  if (esp_netif_get_netif_impl_name(netif, interface.ifr_name) != ESP_OK) {
    fail(TransportError::unavailable, "network interface has no name");
    return result;
  }
  const auto deadline = Clock::now() + request.options.timeout;
  auto remaining = [&]() {
    return std::max(0, static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                            deadline - Clock::now())
                                            .count()));
  };
  esp_http_client_config_t config{};
  config.url = request.url.c_str();
  config.method = method_for(request.method);
  config.if_name = &interface;
  config.timeout_ms = remaining();
  config.crt_bundle_attach = request.options.verify_tls ? esp_crt_bundle_attach : nullptr;
  config.skip_cert_common_name_check = !request.options.verify_tls;
  config.disable_auto_redirect = true;
  config.max_redirection_count = request.options.max_redirects;
  config.user_data = &result;
  config.event_handler = [](esp_http_client_event_t* event) -> esp_err_t {
    auto& current = *static_cast<TransportResult*>(event->user_data);
    if (event->event_id == HTTP_EVENT_ON_CONNECTED)
      current.connection_established = true;
    if (event->event_id == HTTP_EVENT_ON_HEADER && event->header_key && event->header_value)
      current.response.headers.emplace_back(event->header_key, event->header_value);
    return ESP_OK;
  };
  using Client = std::unique_ptr<std::remove_pointer_t<esp_http_client_handle_t>,
                                 decltype(&esp_http_client_cleanup)>;
  Client client{esp_http_client_init(&config), &esp_http_client_cleanup};
  if (!client) {
    fail(TransportError::invalid_request, "HTTP client initialization failed");
    return result;
  }
  // Content-Length is derived from the owned body when the caller omits it.
  auto headers = request.headers;
  if (!has_header(headers, "Content-Length"))
    headers.emplace_back("Content-Length", std::to_string(request.body.size()));
  if (set_headers(client.get(), headers) != ESP_OK) {
    fail(TransportError::invalid_request, "HTTP header configuration failed");
    return result;
  }
  std::array<char, 2048> buffer{};
  auto current_url = request.url;
  for (unsigned redirect = 0;; ++redirect) {
    auto ready = [&]() {
      if (cancellation.cancelled()) {
        fail(TransportError::cancelled, "request cancelled");
        return false;
      }
      const auto timeout = remaining();
      if (timeout <= 0) {
        fail(TransportError::timeout, "HTTP deadline expired");
        return false;
      }
      esp_http_client_set_timeout_ms(client.get(), timeout);
      return true;
    };
    if (!ready())
      break;
    result.response.headers.clear();
    result.response.body.clear();
    auto error = esp_http_client_open(client.get(), static_cast<int>(request.body.size()));
    if (error != ESP_OK) {
      fail(classify(client.get(), error, result.connection_established), esp_err_to_name(error));
      break;
    }
    std::size_t offset{};
    while (offset < request.body.size() && ready()) {
      const auto size = static_cast<int>(std::min<std::size_t>(2048, request.body.size() - offset));
      const auto written = esp_http_client_write(
          client.get(), reinterpret_cast<const char*>(request.body.data() + offset), size);
      if (written <= 0) {
        fail(classify(client.get(), ESP_ERR_HTTP_WRITE_DATA, true), "HTTP body write failed");
        break;
      }
      offset += static_cast<std::size_t>(written);
    }
    if (result.response.error != TransportError::none || !ready())
      break;
    if (esp_http_client_fetch_headers(client.get()) < 0) {
      fail(classify(client.get(), ESP_ERR_HTTP_FETCH_HEADER, true), "HTTP headers incomplete");
      break;
    }
    result.response.status = esp_http_client_get_status_code(client.get());
    while (ready()) {
      const auto count = esp_http_client_read(client.get(), buffer.data(), buffer.size());
      if (count < 0) {
        fail(classify(client.get(), ESP_ERR_HTTP_READ_TIMEOUT, true), "HTTP read failed");
        break;
      }
      if (count == 0) {
        if (!esp_http_client_is_complete_data_received(client.get()))
          fail(TransportError::protocol, "HTTP response body incomplete");
        break;
      }
      const auto* first = reinterpret_cast<const std::byte*>(buffer.data());
      result.response.body.insert(result.response.body.end(), first, first + count);
    }
    if (result.response.error != TransportError::none)
      break;
    const auto status = result.response.status;
    const auto follow =
        status == 307 || status == 308 ||
        (request.method == Method::get && (status == 301 || status == 302 || status == 303));
    if (!follow || redirect >= request.options.max_redirects)
      break;
    auto location = std::ranges::find_if(result.response.headers, [](const auto& header) {
      return strcasecmp(header.first.c_str(), "Location") == 0;
    });
    if (location == result.response.headers.end())
      break;
    // Cross-origin redirects are surfaced to the caller, which owns credentials and policy.
    if (location->second.starts_with("//") || (location->second.find("://") != std::string::npos &&
                                               origin(location->second) != origin(current_url)))
      break;
    esp_http_client_close(client.get());
    if (esp_http_client_set_redirection(client.get()) != ESP_OK) {
      fail(TransportError::protocol, "HTTP redirect URL is invalid");
      break;
    }
  }
  if (cancellation.cancelled())
    fail(TransportError::cancelled, "request cancelled");
  return result;
}

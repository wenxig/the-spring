#include "network_service.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <climits>
#include <deque>
#include <mutex>
#include <optional>
#include <string_view>

namespace {
using namespace spring::network;
using Clock = std::chrono::steady_clock;
struct PendingRequest {
  RequestId id{};
  Request request;
  Completion completion;
  std::shared_ptr<std::atomic_bool> cancelled;
};
struct ActiveRequest {
  RequestId id{};
  std::shared_ptr<std::atomic_bool> cancelled;
};
std::array<HttpTransport*, 2> transports{};
std::deque<PendingRequest> pending;
std::optional<ActiveRequest> active;
std::mutex state_mutex;
RequestId next_id{1};
TaskHandle_t worker_handle{};

std::array<HttpTransport*, 2> registered_transports() {
  std::lock_guard lock(state_mutex);
  return transports;
}

bool valid_request(const Request& request) {
  if ((!request.url.starts_with("http://") && !request.url.starts_with("https://")) ||
      request.url.find_first_of("\r\n ") != std::string::npos ||
      request.url.find('\0') != std::string::npos || request.options.timeout.count() <= 0 ||
      request.options.timeout.count() > INT_MAX || request.body.size() > INT_MAX ||
      request.method > Method::delete_)
    return false;
  for (const auto& [name, value] : request.headers) {
    if (name.empty() ||
        !std::ranges::all_of(name,
                             [](unsigned char c) {
                               return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                                      (c >= '0' && c <= '9') ||
                                      std::string_view{"!#$%&'*+-.^_`|~"}.find(
                                          static_cast<char>(c)) != std::string_view::npos;
                             }) ||
        !std::ranges::all_of(value,
                             [](unsigned char c) { return c == '\t' || (c >= 32 && c != 127); }))
      return false;
    auto lower = name;
    std::ranges::transform(lower, lower.begin(), [](unsigned char c) {
      return static_cast<char>(c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c);
    });
    if (lower == "transfer-encoding")
      return false; // Complete bodies use Content-Length framing.
    if (lower == "content-length") {
      std::size_t length{};
      const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), length);
      if (error != std::errc{} || end != value.data() + value.size() ||
          length != request.body.size())
        return false;
    }
  }
  return true;
}

bool connection_failure(const TransportResult& result) {
  return !result.connection_established && (result.response.error == TransportError::unavailable ||
                                            result.response.error == TransportError::dns ||
                                            result.response.error == TransportError::tls ||
                                            result.response.error == TransportError::connection ||
                                            result.response.error == TransportError::timeout);
}

Response error_response(TransportError error, std::string detail, Link link = Link::unavailable) {
  Response response{};
  response.link = link;
  response.error = error;
  response.error_detail = std::move(detail);
  return response;
}

Response dispatch(PendingRequest& work) {
  if (!valid_request(work.request))
    return error_response(TransportError::invalid_request, "invalid HTTP request");
  const auto backends = registered_transports();
  auto selected = Link::unavailable;
  auto* transport = static_cast<HttpTransport*>(nullptr);
  for (std::size_t i = 0; i < backends.size(); ++i) {
    if (backends[i] && backends[i]->available()) {
      selected = i == 0 ? Link::wifi : Link::cellular;
      transport = backends[i];
      break;
    }
  }
  if (!transport)
    return error_response(TransportError::unavailable, "no network link available");
  const auto began = Clock::now();
  auto result = transport->perform(work.request, CancellationToken{work.cancelled});
  result.response.link = selected;
  if (selected == Link::wifi && connection_failure(result) && !work.cancelled->load() &&
      backends[1] && backends[1]->available()) {
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - began);
    if (elapsed < work.request.options.timeout) {
      work.request.options.timeout -= elapsed;
      ESP_LOGW("network", "request=%llu fallback=cellular",
               static_cast<unsigned long long>(work.id));
      result = backends[1]->perform(work.request, CancellationToken{work.cancelled});
      result.response.link = Link::cellular;
    }
  }
  return std::move(result.response);
}

void worker_task(void*) {
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    while (true) {
      PendingRequest work;
      {
        std::lock_guard lock(state_mutex);
        if (pending.empty())
          break;
        work = std::move(pending.front());
        pending.pop_front();
        active = ActiveRequest{work.id, work.cancelled};
      }
      Response response;
      if (!work.cancelled->load())
        response = dispatch(work);
      {
        // Removing the active ID and checking cancellation form one completion boundary.
        std::lock_guard lock(state_mutex);
        if (work.cancelled->load()) {
          const auto link = response.link;
          response = error_response(TransportError::cancelled, "request cancelled", link);
        }
        active.reset();
      }
      try {
        work.completion(work.id, std::move(response));
      } catch (...) {
        ESP_LOGE("network", "completion threw: request=%llu",
                 static_cast<unsigned long long>(work.id));
      }
    }
  }
}
} // namespace

bool spring::network::Response::ok() const noexcept {
  return error == TransportError::none && status >= 200 && status < 300;
}
spring::network::CancellationToken::CancellationToken(
    std::shared_ptr<std::atomic_bool> state) noexcept
    : state_(std::move(state)) {}
bool spring::network::CancellationToken::cancelled() const noexcept {
  return state_ && state_->load();
}
void spring::network::start() {
  std::lock_guard lock(state_mutex);
  if (worker_handle)
    return;
  if (xTaskCreate(worker_task, "network", 12288, nullptr, 5, &worker_handle) != pdPASS) {
    worker_handle = nullptr;
    ESP_LOGE("network", "worker allocation failed");
    return;
  }
  xTaskNotifyGive(worker_handle);
}
void spring::network::register_transport(Link link, HttpTransport& transport) {
  if (link == Link::unavailable)
    return;
  std::lock_guard lock(state_mutex);
  transports[link == Link::wifi ? 0 : 1] = &transport;
}
spring::network::RequestId spring::network::request(Request request, Completion completion) {
  if (!completion)
    return 0;
  start();
  std::lock_guard lock(state_mutex);
  if (!worker_handle)
    return 0;
  const auto id = next_id++;
  pending.push_back(
      {id, std::move(request), std::move(completion), std::make_shared<std::atomic_bool>(false)});
  xTaskNotifyGive(worker_handle);
  return id;
}
bool spring::network::cancel(RequestId id) {
  std::lock_guard lock(state_mutex);
  if (active && active->id == id)
    return !active->cancelled->exchange(true);
  for (auto& work : pending) {
    if (work.id == id)
      return !work.cancelled->exchange(true);
  }
  return false;
}
spring::network::RequestId spring::network::get(Request value, Completion done) {
  value.method = Method::get;
  return request(std::move(value), std::move(done));
}
spring::network::RequestId spring::network::post(Request value, Completion done) {
  value.method = Method::post;
  return request(std::move(value), std::move(done));
}
spring::network::RequestId spring::network::put(Request value, Completion done) {
  value.method = Method::put;
  return request(std::move(value), std::move(done));
}
spring::network::RequestId spring::network::patch(Request value, Completion done) {
  value.method = Method::patch;
  return request(std::move(value), std::move(done));
}
spring::network::RequestId spring::network::del(Request value, Completion done) {
  value.method = Method::delete_;
  return request(std::move(value), std::move(done));
}
spring::network::Link spring::network::active_link() {
  const auto backends = registered_transports();
  if (backends[0] && backends[0]->available())
    return Link::wifi;
  if (backends[1] && backends[1]->available())
    return Link::cellular;
  return Link::unavailable;
}

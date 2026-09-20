#include "network_service.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <array>
#include <atomic>
#include <deque>
#include <mutex>
#include <optional>
#include <utility>

namespace {
constexpr char kTag[] = "network";

struct PendingRequest {
  spring::network::RequestId id{};
  spring::network::Request request;
  spring::network::Completion completion;
  std::shared_ptr<std::atomic_bool> cancelled;
};

std::array<spring::network::HttpTransport*, 2> transports{};
std::deque<PendingRequest> pending;
std::optional<PendingRequest> active_request;
std::mutex state_mutex;
std::atomic<spring::network::RequestId> next_id{1};
TaskHandle_t worker_handle{nullptr};

constexpr std::size_t index_for(spring::network::Link link) noexcept {
  return link == spring::network::Link::wifi ? 0U : 1U;
}

spring::network::HttpTransport* transport_for(spring::network::Link link) noexcept {
  return transports[index_for(link)];
}

bool is_connection_failure(const spring::network::TransportResult& result) noexcept {
  return !result.connection_established &&
         (result.response.error == spring::network::TransportError::unavailable ||
          result.response.error == spring::network::TransportError::connection ||
          result.response.error == spring::network::TransportError::dns ||
          result.response.error == spring::network::TransportError::timeout);
}

spring::network::Response cancelled_response(spring::network::Link link) {
  spring::network::Response response{};
  response.link = link;
  response.error = spring::network::TransportError::cancelled;
  response.error_detail = "request cancelled";
  return response;
}

void invoke_completion(PendingRequest request, spring::network::Response response) {
  if (request.completion) {
    request.completion(request.id, std::move(response));
  }
}

void worker_task(void*) {
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    while (true) {
      PendingRequest request;
      {
        std::lock_guard lock(state_mutex);
        if (pending.empty()) {
          active_request.reset();
          break;
        }
        request = std::move(pending.front());
        pending.pop_front();
        active_request = request;
      }

      if (request.cancelled->load()) {
        invoke_completion(std::move(request), cancelled_response(Link::unavailable));
        continue;
      }

      Link selected{Link::unavailable};
      auto* transport = static_cast<spring::network::HttpTransport*>(nullptr);
      {
        std::lock_guard lock(state_mutex);
        if (transports[0] != nullptr && transports[0]->available()) {
          selected = Link::wifi;
          transport = transports[0];
        } else if (transports[1] != nullptr && transports[1]->available()) {
          selected = Link::cellular;
          transport = transports[1];
        }
      }

      spring::network::TransportResult result{};
      if (transport == nullptr) {
        result.response.link = Link::unavailable;
        result.response.error = spring::network::TransportError::unavailable;
        result.response.error_detail = "no network transport is available";
      } else {
        result = transport->perform(request.request,
                                    spring::network::CancellationToken{request.cancelled});
        result.response.link = selected;
        if (selected == Link::wifi && is_connection_failure(result) &&
            !request.cancelled->load()) {
          auto* cellular = transport_for(Link::cellular);
          if (cellular != nullptr && cellular->available()) {
            ESP_LOGW(kTag, "request %llu falling back from Wi-Fi to cellular",
                     static_cast<unsigned long long>(request.id));
            result = cellular->perform(request.request,
                                       spring::network::CancellationToken{request.cancelled});
            result.response.link = Link::cellular;
          }
        }
      }

      if (request.cancelled->load()) {
        result.response = cancelled_response(result.response.link);
      }
      invoke_completion(std::move(request), std::move(result.response));
      {
        std::lock_guard lock(state_mutex);
        active_request.reset();
      }
    }
  }
}

spring::network::RequestId submit(spring::network::Request request,
                                  spring::network::Completion completion) {
  if (request.url.empty() || !completion || request.options.timeout.count() <= 0) {
    return 0;
  }
  const auto id = next_id.fetch_add(1);
  PendingRequest pending_request{id, std::move(request), std::move(completion),
                                 std::make_shared<std::atomic_bool>(false)};
  {
    std::lock_guard lock(state_mutex);
    pending.push_back(std::move(pending_request));
  }
  if (worker_handle != nullptr) {
    xTaskNotifyGive(worker_handle);
  }
  return id;
}

} // namespace

bool spring::network::Response::ok() const noexcept {
  return error == TransportError::none && status >= 200 && status < 300;
}

spring::network::CancellationToken::CancellationToken(
    std::shared_ptr<std::atomic_bool> state) noexcept
    : state_(std::move(state)) {}

bool spring::network::CancellationToken::cancelled() const noexcept {
  return state_ != nullptr && state_->load();
}

void spring::network::start() {
  if (worker_handle != nullptr) {
    return;
  }
  xTaskCreate(worker_task, "network", 8192, nullptr, 5, &worker_handle);
  if (worker_handle != nullptr) {
    xTaskNotifyGive(worker_handle);
  }
  ESP_LOGI(kTag, "network service ready; Wi-Fi preferred with cellular fallback");
}

void spring::network::register_transport(Link link, HttpTransport& transport) {
  if (link == Link::unavailable) {
    return;
  }
  std::lock_guard lock(state_mutex);
  transports[index_for(link)] = &transport;
  if (worker_handle != nullptr) {
    xTaskNotifyGive(worker_handle);
  }
}

spring::network::RequestId spring::network::request(Request request, Completion completion) {
  return submit(std::move(request), std::move(completion));
}

bool spring::network::cancel(RequestId request_id) {
  if (request_id == 0) {
    return false;
  }
  std::lock_guard lock(state_mutex);
  for (auto& request : pending) {
    if (request.id == request_id) {
      request.cancelled->store(true);
      if (worker_handle != nullptr) {
        xTaskNotifyGive(worker_handle);
      }
      return true;
    }
  }
  if (active_request.has_value() && active_request->id == request_id) {
    active_request->cancelled->store(true);
    return true;
  }
  return false;
}

spring::network::RequestId spring::network::get(Request request, Completion completion) {
  request.method = Method::get;
  return submit(std::move(request), std::move(completion));
}

spring::network::RequestId spring::network::post(Request request, Completion completion) {
  request.method = Method::post;
  return submit(std::move(request), std::move(completion));
}

spring::network::RequestId spring::network::put(Request request, Completion completion) {
  request.method = Method::put;
  return submit(std::move(request), std::move(completion));
}

spring::network::RequestId spring::network::patch(Request request, Completion completion) {
  request.method = Method::patch;
  return submit(std::move(request), std::move(completion));
}

spring::network::RequestId spring::network::del(Request request, Completion completion) {
  request.method = Method::delete_;
  return submit(std::move(request), std::move(completion));
}

spring::network::Link spring::network::active_link() {
  std::lock_guard lock(state_mutex);
  if (transports[0] != nullptr && transports[0]->available()) {
    return Link::wifi;
  }
  if (transports[1] != nullptr && transports[1]->available()) {
    return Link::cellular;
  }
  return Link::unavailable;
}

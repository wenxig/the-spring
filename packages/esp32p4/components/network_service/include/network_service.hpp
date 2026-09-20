#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace spring::network {

enum class Method : std::uint8_t { get, post, put, patch, delete_ };
enum class Link : std::uint8_t { wifi, cellular, unavailable };
enum class TransportError : std::uint8_t {
  none,
  invalid_request,
  unavailable,
  dns,
  connection,
  tls,
  timeout,
  io,
  protocol,
  cancelled,
};

using Header = std::pair<std::string, std::string>;
using Headers = std::vector<Header>;
using Body = std::vector<std::byte>;
using RequestId = std::uint64_t;

struct RequestOptions {
  std::chrono::milliseconds timeout{15'000};
  std::uint8_t max_redirects{3};
  bool verify_tls{true};
};

struct Request {
  Method method{Method::get};
  std::string url;
  Headers headers;
  Body body;
  RequestOptions options{};
};

struct Response {
  int status{-1};
  Headers headers;
  Body body;
  Link link{Link::unavailable};
  TransportError error{TransportError::none};
  std::string error_detail;

  [[nodiscard]] bool ok() const noexcept;
};

class CancellationToken {
public:
  explicit CancellationToken(std::shared_ptr<std::atomic_bool> state) noexcept;
  [[nodiscard]] bool cancelled() const noexcept;

private:
  std::shared_ptr<std::atomic_bool> state_;
};

struct TransportResult {
  Response response;
  bool connection_established{};
};

class HttpTransport {
public:
  virtual ~HttpTransport() = default;
  [[nodiscard]] virtual bool available() const noexcept = 0;
  virtual TransportResult perform(const Request& request,
                                  CancellationToken cancellation) = 0;
};

using Completion = std::function<void(RequestId, Response)>;

void start();
void register_transport(Link link, HttpTransport& transport);
RequestId request(Request request, Completion completion);
bool cancel(RequestId request_id);

RequestId get(Request request, Completion completion);
RequestId post(Request request, Completion completion);
RequestId put(Request request, Completion completion);
RequestId patch(Request request, Completion completion);
RequestId del(Request request, Completion completion);

[[nodiscard]] Link active_link();

} // namespace spring::network

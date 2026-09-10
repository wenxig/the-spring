#pragma once

#include <string>
#include <string_view>

namespace spring::network {
enum class Link { wifi, cellular, unavailable };
struct Response { int status{}; std::string body{}; };
void start();
void set_link_available(Link link, bool available);
Link active_link();
Response get(std::string_view url);
Response post(std::string_view url, std::string_view body);
}  // namespace spring::network

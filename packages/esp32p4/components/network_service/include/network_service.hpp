#pragma once

#include <string>
#include <string_view>

namespace spring::network {
struct Response { int status{}; std::string body{}; };
void start();
Response get(std::string_view url);
Response post(std::string_view url, std::string_view body);
}  // namespace spring::network

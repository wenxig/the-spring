#pragma once

namespace spring::network {
class HttpTransport;
}

namespace spring::wifi {
bool prepare_transport();
void start();
void set_connected(bool connected);
spring::network::HttpTransport& transport();
} // namespace spring::wifi

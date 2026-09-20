#pragma once
#include "network_service.hpp"
struct esp_netif_obj;
namespace spring::http {
network::TransportResult perform(const network::Request& request,
                                 network::CancellationToken cancellation,
                                 esp_netif_obj* netif);
}

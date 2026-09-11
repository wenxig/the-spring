#include "location_service.hpp"
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <string>
#include <string_view>

namespace {
bool parse_coordinate(std::string_view value, double& coordinate) {
  if (value.size() < 2) return false;
  char hemisphere{};
  const auto text = std::string{value};
  if (std::sscanf(text.c_str(), "%lf%c", &coordinate, &hemisphere) != 2) return false;
  if (coordinate > 180.0) {
    const auto degrees = std::floor(coordinate / 100.0);
    coordinate = degrees + (coordinate - degrees * 100.0) / 60.0;
  }
  if (hemisphere == 'S' || hemisphere == 'W') coordinate = -coordinate;
  return true;
}
}

bool spring::location::refresh() {
  (void)spring::modem::execute("AT+QGPS=1", 10000);
  std::string response;
  if (spring::modem::execute_capture("AT+QGPSLOC=2", 10000, response) == spring::modem::Result::ok) {
    const auto start = response.find("+QGPSLOC:");
    if (start != std::string::npos) {
      double latitude{}, longitude{};
      const auto line = response.substr(start);
      const auto first = line.find(',');
      const auto second = first == std::string::npos ? std::string::npos : line.find(',', first + 1);
      const auto third = second == std::string::npos ? std::string::npos : line.find(',', second + 1);
      if (first != std::string::npos && second != std::string::npos && third != std::string::npos &&
          parse_coordinate(line.substr(first + 1, second - first - 1), latitude) &&
          parse_coordinate(line.substr(second + 1, third - second - 1), longitude)) {
        spring::modem::set_location(latitude, longitude);
        return true;
      }
    }
  }
  if (spring::modem::execute_capture("AT+QCELL=1,\"CMNET\"", 30000, response) == spring::modem::Result::ok) {
    const auto start = response.find("+QCELL:");
    if (start != std::string::npos) {
      double longitude{}, latitude{};
      const auto line = response.substr(start);
      if (std::sscanf(line.c_str(), "+QCELL: %lf,%lf", &longitude, &latitude) == 2) {
        spring::modem::set_location(latitude, longitude);
        return true;
      }
    }
  }
  return false;
}

spring::modem::Snapshot spring::location::current() { return spring::modem::snapshot(); }

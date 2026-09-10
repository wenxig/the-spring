#include "location_service.hpp"

bool spring::location::refresh() {
  return spring::modem::execute("AT+QGPS=1", 10000) == spring::modem::Result::ok;
}

spring::modem::Snapshot spring::location::current() { return spring::modem::snapshot(); }

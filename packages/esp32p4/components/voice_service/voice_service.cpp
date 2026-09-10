#include "voice_service.hpp"
#include "at_engine.hpp"
#include <string>

bool spring::voice::dial(std::string_view number) {
  if (number.empty()) return false;
  std::string command{"ATD"};
  command.append(number);
  command.push_back(';');
  return spring::modem::execute(command, 30000) == spring::modem::Result::ok;
}

bool spring::voice::answer() {
  return spring::modem::execute("ATA", 10000) == spring::modem::Result::ok;
}

bool spring::voice::hangup() {
  return spring::modem::execute("ATH", 10000) == spring::modem::Result::ok;
}

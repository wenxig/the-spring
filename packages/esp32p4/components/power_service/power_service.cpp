#include "power_service.hpp"
#include "display_service.hpp"
#include "at_engine.hpp"

namespace {
spring::power::State current_state = spring::power::State::active;
}

void spring::power::start() { current_state = State::active; }

void spring::power::request_sleep() {
  current_state = State::sleeping;
}

void spring::power::wake() {
  current_state = State::active;
  spring::display::force_full_refresh();
  (void)spring::modem::execute("AT", 3000);
}

spring::power::State spring::power::state() { return current_state; }

#include "clock_app.hpp"
#include "app_runtime.hpp"

namespace {
class ClockApplication final : public spring::app::Application {
 public:
  const char* name() const override { return "clock"; }
  void on_event(std::uint32_t) override {}
};
ClockApplication application;
}

void spring::clock_app::register_app() { spring::app::register_application(application); }

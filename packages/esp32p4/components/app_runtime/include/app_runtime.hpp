#pragma once

#include <cstdint>

namespace spring::app {
enum class Lifecycle : std::uint8_t { installed, started, paused, stopped };

class Application {
 public:
  virtual ~Application() = default;
  virtual const char* name() const = 0;
  virtual void on_event(std::uint32_t event) = 0;
};

void start();
bool register_application(Application& application);
bool navigate_home();
}  // namespace spring::app

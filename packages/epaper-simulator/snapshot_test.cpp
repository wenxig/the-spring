#include "ui_core.hpp"
#include <cassert>

int main() {
  spring::ui::Router router;
  spring::ui::Frame offline;
  spring::ui::Frame online;
  spring::ui::Snapshot disconnected{.hour = 0, .minute = 0, .signal = 0, .registered = false, .locating = true, .in_call = false};
  spring::ui::Snapshot connected{.hour = 23, .minute = 59, .signal = 4, .registered = true, .locating = false, .in_call = true};
  router.render(offline, disconnected);
  router.render(online, connected);
  assert(offline.bytes() != online.bytes());
  assert(router.dispatch(spring::ui::Event::right));
  router.render(offline, disconnected);
  router.render(online, connected);
  assert(offline.bytes() != online.bytes());
  return 0;
}

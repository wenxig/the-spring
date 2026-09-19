#include "calendar.hpp"
#include <cassert>

int main() {
  using spring::ui::exam_countdown;
  assert(exam_countdown(2026, 6, 6).days == 1);
  for (unsigned day = 7; day <= 9; ++day) {
    assert(exam_countdown(2026, 6, day).active);
    assert(exam_countdown(2026, 6, day).days == 0);
  }
  assert(exam_countdown(2026, 6, 10).days == 362);
  assert(exam_countdown(2026, 9, 18).days == 262);
  assert(exam_countdown(2028, 2, 28).days == 100);
}

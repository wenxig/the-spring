#include "ui_core.hpp"
#include <cassert>

int main() {
  spring::ui::Frame chinese;
  spring::ui::Frame ascii;
  chinese.text_utf8(10, 10, "时间：12:34");
  ascii.text(10, 10, "TIME:12:34");
  assert(chinese.bytes() != ascii.bytes());

  spring::ui::Frame malformed;
  malformed.text_utf8(10, 10, "网络\xE4\xB8");
  assert(malformed.is_black(10, 10));
  return 0;
}

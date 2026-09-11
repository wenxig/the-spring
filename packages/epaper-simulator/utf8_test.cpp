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
  assert(malformed.bytes() != spring::ui::Frame{}.bytes());

  spring::ui::Frame labels;
  labels.text_utf8(10, 10, "日期星期温度天气");
  for (std::uint16_t x = 0; x < spring::ui::kWidth; ++x) assert(!labels.is_black(x, 25));

  spring::ui::Frame outside;
  outside.text_utf8(10, 285, "日期");
  assert(outside.bytes() == spring::ui::Frame{}.bytes());
  return 0;
}

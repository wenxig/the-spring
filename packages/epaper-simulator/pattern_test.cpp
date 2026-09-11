#include "ui_core.hpp"
#include <cassert>
#include <fstream>
#include <string>

namespace {
void save(const std::string& path, const spring::ui::Frame& frame) {
  std::ofstream output(path, std::ios::binary);
  output << "P4\n400 300\n";
  const auto& data = frame.bytes();
  output.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
  assert(output.good());
}
}

int main(int argc, char** argv) {
  assert(argc == 2);
  const std::string prefix = argv[1];
  spring::ui::Frame frame;
  save(prefix + "white.pbm", frame);
  for (std::uint16_t y = 0; y < spring::ui::kHeight; ++y)
    for (std::uint16_t x = 0; x < spring::ui::kWidth; ++x) frame.pixel(x, y);
  save(prefix + "black.pbm", frame);
  frame.clear();
  for (std::uint16_t y = 0; y < spring::ui::kHeight; ++y)
    for (std::uint16_t x = 0; x < spring::ui::kWidth; ++x)
      if (((x / 8) + (y / 8)) % 2 == 0) frame.pixel(x, y);
  save(prefix + "checkerboard.pbm", frame);
  frame.clear();
  frame.box({0, 0, spring::ui::kWidth, spring::ui::kHeight});
  frame.pixel(0, 0);
  frame.pixel(399, 0);
  frame.pixel(0, 299);
  frame.pixel(399, 299);
  assert(frame.is_black(0, 0) && frame.is_black(399, 299));
  save(prefix + "corners_marked.pbm", frame);
  return 0;
}

#include "ui_core.hpp"
#include <cassert>
#include <fstream>
#include <string>
int main(int argc, char** argv) { assert(argc == 2); const std::string prefix = argv[1]; spring::ui::Router router; spring::ui::Frame frame; for (int i = 0; i < 5; ++i) { router.render(frame, {}); std::ofstream output(prefix + std::to_string(i) + ".pbm", std::ios::binary); output << "P4\n400 300\n"; const auto& data = frame.bytes(); output.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size())); assert(output.good()); assert(data.size() == spring::ui::kBytes); assert(frame.is_black(0, 0)); assert(frame.is_black(399, 299)); assert(router.dispatch(spring::ui::Event::right)); } return 0; }

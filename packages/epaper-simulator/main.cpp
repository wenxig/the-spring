#include "ui_core.hpp"
#include <fstream>
#include <string>
int main(int argc, char** argv) { const std::string path = argc > 1 ? argv[1] : "clock.pbm"; spring::ui::Router router; spring::ui::Frame frame; router.render(frame, {}); if (router.route() != spring::ui::Route::clock || !frame.is_black(0, 0) || !frame.is_black(399, 299)) return 2; std::ofstream out(path, std::ios::binary); out << "P4\n400 300\n"; const auto& bytes = frame.bytes(); out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())); return out.good() ? 0 : 1; }
